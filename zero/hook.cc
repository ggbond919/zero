#include "hook.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdint>
#include <ctime>
#include <dlfcn.h>
#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include "config.h"
#include "fd_manager.h"
#include "fiber.h"
#include "iomanager.h"
#include "log.h"
#include "macro.h"
#include "zero/scheduler.h"
#include "zero/timer.h"

zero::Logger::ptr g_logger = ZERO_LOG_NAME("system");

namespace zero {

static zero::ConfigVar<int>::ptr g_tcp_connect_timeout = zero::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)

/// 获取系统函数指针
void hook_init() {
    static bool is_inited = false;
    if (is_inited) {
        return;
    }
    // sleep_f = (sleep_fun) dlsym(RTLD_NEXT,"sleep");
#define XX(name) name##_f = ( name##_fun )dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value) {
            ZERO_LOG_INFO(g_logger) << "tcp connect timeout config changed from " << old_value << "to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};

/// main之前初始化完成
static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}  // namespace zero

/// 条件定时器使用
struct timer_info {
    int cancelled = 0;
};

template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, uint32_t event, int timeout_so, Args&&... args) {
    if (!zero::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    zero::FdCtx::ptr ctx = zero::FdMgr::GetInstance()->get(fd);
    if (!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if (ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    /// 如果发时缓冲区已满或信号中断，则交由IOManager，继续监听是否可写的事件，直到 n > 0 或 n = 0 为止
    /// 如果读时缓冲区为空或信号中断，则交由IOManager，继续监听是否可读的事件，直到 n > 0 为 n = 0 为止
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    /// 信号中断继续尝试
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    /// 读写缓冲区已满的情况
    if (n == -1 && errno == EAGAIN) {
        zero::IOManager* iom = zero::IOManager::GetThis();
        zero::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        if (to != ( uint64_t )-1) {
            timer = iom->addConditionTimer(
                to,
                [winfo, fd, iom, event]() {
                    auto t = winfo.lock();
                    if (!t || t->cancelled) {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (zero::IOManager::Event)(event));
                },
                winfo);
        }

        int rt = iom->addEvent(fd, (zero::IOManager::Event)(event));
        /// -1
        if (ZERO_UNLIKELY(rt)) {
            ZERO_LOG_ERROR(g_logger) << hook_fun_name << " addEvent(" << fd << ", " << event << ")";
            if (timer) {
                timer->cancel();
            }
            return -1;
        } else {
            zero::Fiber::YieldToHold();
            if (timer) {
                timer->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        }
    }

    return n;
}

extern "C" {
// sleep_fun sleep_f = nullptr;
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if (!zero::t_hook_enable) {
        return sleep_f(seconds);
    }

    zero::Fiber::ptr fiber = zero::Fiber::GetThis();
    zero::IOManager* iom = zero::IOManager::GetThis();
    iom->addTimer(seconds * 1000,
                  std::bind((void(zero::Scheduler::*)(zero::Fiber::ptr, int thread)) & zero::IOManager::schedule, iom, fiber, -1));
    zero::Fiber::YieldToHold();
    
    return 0;
}

int usleep(useconds_t usec) {
    if (!zero::t_hook_enable) {
        return usleep_f(usec);
    }

    /// 获取当前正在执行任务的协程，及iomanager
    /// 为该协程添加一个timer，待超时时间到达时再次将当前fiber调度起来，达到usleep的效果
    zero::Fiber::ptr fiber = zero::Fiber::GetThis();
    zero::IOManager* iom = zero::IOManager::GetThis();
    // std::bind 绑定模板函数
    // https://www.lmlphp.com/user/152247/article/item/3660974/
    // iom->addTimer(usec / 1000,
    //               std::bind(static_cast<void(zero::Scheduler::*)(zero::Fiber::ptr,int thread)>(& zero::IOManager::schedule),iom,fiber,-1));
    iom->addTimer(usec / 1000,
                  std::bind((void(zero::Scheduler::*)(zero::Fiber::ptr, int thread)) & zero::IOManager::schedule, iom, fiber, -1));
    zero::Fiber::YieldToHold();
    
    return 0;
}

int nanosleep(const struct timespec* req, struct timespec* rem) {
    if (!zero::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    zero::Fiber::ptr fiber = zero::Fiber::GetThis();
    zero::IOManager* iom = zero::IOManager::GetThis();
    iom->addTimer(timeout_ms,
                  std::bind((void(zero::Scheduler::*)(zero::Fiber::ptr, int thread)) & zero::IOManager::schedule, iom, fiber, -1));
    zero::Fiber::YieldToHold();

    return 0;
}

int socket(int domain, int type, int protocol) {
    if (!zero::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if (fd == -1) {
        return fd;
    }
    zero::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if (!zero::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    zero::FdCtx::ptr ctx = zero::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClose()) {
        errno = EBADE;
        return -1;
    }

    if (!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }

    if (ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }
    /// 对于阻塞的fd，connect会等三次握手完成后返回
    /// 对于非阻塞的fd，connect会立即返回，0则成功，如果返回-1 且errno = EINPROGRESS，说明正在尝试连接
    int n = connect_f(fd, addr, addrlen);
    if (n == 0) {
        return 0;
    } else if (n != -1 || errno != EINPROGRESS) {
        return n;
    }

    zero::IOManager* iom = zero::IOManager::GetThis();
    zero::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    /// 代码执行到这里，说明连接正在进行中...
    /// 参考文章：https://zhuanlan.zhihu.com/p/439530130
    /// 几种情况
    /// 1.未设置超时
    /// 2.设置了超时
    ///     2.1 timer超时之前完成了连接
    ///     2.2 timer超时之后还未完成连接
    if (timeout_ms != ( uint64_t )-1) {
        timer = iom->addConditionTimer(
            timeout_ms,
            [winfo, fd, iom]() {
                auto t = winfo.lock();
                /// 如果在未超时之前便已经连接成功了，则无需处理，直接返回即可
                /// 所以使用了一个weak_ptr来做，如果weak_ptr为空时，说明connect_with_timeout()函数已经执行完毕
                if (!t || t->cancelled) {
                    return;
                }

                /// 未触发过则设置超时标志，触发一次写事件
                t->cancelled = ETIMEDOUT;

                /// 如果超时时间特别短，先addEvent的情况下执行了，因为当前fd还没有设置WRITE事件，ZERO_UNLIKELY失败，所以并不会去调度
                /// 退一万步讲，即便是被设置了WRITE事件，但是因为FdContext未被添加，所以其回调函数和fiber都是空的，也无效
                /// 当且仅当 下面的iom->addEvent(fd, zero::IOManager::WRITE);执行完，才有意义
                /// 当上述成立之后，强制触发了WRITE事件时，说明调用失败了，因为已然超时
                iom->cancelEvent(fd, zero::IOManager::WRITE);
            },
            winfo);
    }

    /// addEvent时，会将其读事件的上下文中的fiber设置为正在调度的fiber
    int rt = iom->addEvent(fd, zero::IOManager::WRITE);
    if (rt == 0) {
        /// 添加成功后，让出上下文
        zero::Fiber::YieldToHold();
        /// 当触发WRITE事件，fiber继续从此处执行
        /// 如果timer存在的话，取消掉
        if (timer) {
            timer->cancel();
        }
        /// 如果设置过超时标志的话，说明在指定的时间里没有连接成功，也算失败
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if (timer) {
            timer->cancel();
        }
        ZERO_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    /// 触发WRITE事件的fd需要检查一下是否连接成功
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    /// 没有error说明连接成功
    if (!error) {
        return 0;
    } else {    /// 否则失败，并设置errno
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, zero::s_connect_timeout);
}

int accept(int s, struct sockaddr* addr, socklen_t* addr_len) {
    int fd = do_io(s, accept_f, "accept", zero::IOManager::READ, SO_REUSEADDR, addr, addr_len);
    if (fd >= 0) {
        zero::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void* buf, size_t count) {
    return do_io(fd, read_f, "read", zero::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec* iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", zero::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", zero::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", zero::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", zero::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void* buf, size_t count) {
    return do_io(fd, write_f, "write", zero::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", zero::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void* msg, size_t len, int flags) {
    return do_io(s, send_f, "send", zero::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", zero::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr* msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", zero::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if (!zero::t_hook_enable) {
        return close_f(fd);
    }

    zero::FdCtx::ptr ctx = zero::FdMgr::GetInstance()->get(fd);
    if (ctx) {
        auto iom = zero::IOManager::GetThis();
        if (iom) {
            /// 如果还有事件未完成则强制触发一下
            iom->cancelAll(fd);
        }
        zero::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */) {
    va_list va;
    va_start(va, cmd);
    switch (cmd) {
    case F_SETFL: {
        int arg = va_arg(va, int);
        va_end(va);
        zero::FdCtx::ptr ctx = zero::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return fcntl_f(fd, cmd, arg);
        }
        ctx->setUserNonblock(arg & O_NONBLOCK);
        if (ctx->getSysNonblock()) {
            arg |= O_NONBLOCK;
        } else {
            arg &= ~O_NONBLOCK;
        }
        return fcntl_f(fd, cmd, arg);
    } break;
    case F_GETFL: {
        va_end(va);
        int arg = fcntl_f(fd, cmd);
        zero::FdCtx::ptr ctx = zero::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return arg;
        }
        if (ctx->getUserNonblock()) {
            return arg | O_NONBLOCK;
        } else {
            return arg & ~O_NONBLOCK;
        }
    } break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
#ifdef F_SETPIPE_SZ
    case F_SETPIPE_SZ:
#endif
    {
        int arg = va_arg(va, int);
        va_end(va);
        return fcntl_f(fd, cmd, arg);
    } break;
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
#ifdef F_GETPIPE_SZ
    case F_GETPIPE_SZ:
#endif
    {
        va_end(va);
        return fcntl_f(fd, cmd);
    } break;
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK: {
        struct flock* arg = va_arg(va, struct flock*);
        va_end(va);
        return fcntl_f(fd, cmd, arg);
    } break;
    case F_GETOWN_EX:
    case F_SETOWN_EX: {
        struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
        va_end(va);
        return fcntl_f(fd, cmd, arg);
    } break;
    default:
        va_end(va);
        return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if (FIONBIO == request) {
        bool user_nonblock = !!*( int* )arg;
        zero::FdCtx::ptr ctx = zero::FdMgr::GetInstance()->get(d);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
    if (!zero::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if (level == SOL_SOCKET) {
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            zero::FdCtx::ptr ctx = zero::FdMgr::GetInstance()->get(sockfd);
            if (ctx) {
                const timeval* v = ( const timeval* )optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}
}