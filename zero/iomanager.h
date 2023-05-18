#ifndef __ZERO_IOMANAGER_H__
#define __ZERO_IOMANAGER_H__

#include "scheduler.h"
#include "zero/fiber.h"
#include "zero/mutex.h"
#include <atomic>
#include <bits/stdint-uintn.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>
/// TODO:timer

namespace zero {

/// TODO:timer
class IOManager : public Scheduler {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    /**
     * @brief io事件
     * 
     */
    enum Event {
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x4,
    };

private:
    /**
     * @brief socket事件上下文
     * 
     */
    struct FdContext {
        typedef Mutex MutexType;
        /**
         * @brief 事件上下文
         * 
         */
        struct EventContext {
            Scheduler* scheduler = nullptr;
            Fiber::ptr fiber;
            std::function<void()> cb;
        };

        /**
         * @brief 获取事件上下文
         * 
         * @param event 
         * @return EventContext& 
         */
        EventContext& getContext(Event event);

        /**
         * @brief 重置fd上下文
         * 
         * @param ctx 
         */
        void resetContext(EventContext& ctx);

        /**
         * @brief 触发事件
         * 
         * @param event 
         */
        void triggerEvent(Event event);

        /// 读事件上下文
        EventContext read;
        /// 写事件上下文
        EventContext write;
        /// 事件关联句柄
        int fd = 0;
        /// 当前事件
        Event events = NONE;
        MutexType mutex;
    };

public:
    IOManager(size_t threads = -1, bool use_caller = true, const std::string& name = "");

    ~IOManager();

    /**
     * @brief 添加事件
     * 
     * @param fd 
     * @param event 
     * @param cb 
     * @return int 成功返回0，失败返回-1
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    /**
     * @brief 删除事件,不会触发事件
     * 
     * @param fd 
     * @param event 
     * @return true 
     * @return false 
     */
    bool delEvent(int fd, Event event);

    /**
     * @brief 取消事件，如果事件存在则会触发事件
     * 
     * @param fd 
     * @param event 
     * @return true 
     * @return false 
     */
    bool cancelEvent(int fd, Event event);

    /**
     * @brief 取消所有事件
     * 
     * @param fd 
     * @return true 
     * @return false 
     */
    bool cancelAll(int fd);

    /**
     * @brief 返回当前的IOManager
     * 
     * @return IOManager* 
     */
    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    /// TODO: timer

    /**
     * @brief 重置socker句柄上下文的容器大小
     * 
     * @param size 
     */
    void contextResize(size_t size);
    bool stopping(uint64_t& timeout);

private:
    /// epoll文件句柄
    int m_epfd = 0;
    /// pipe文件句柄
    int m_tickleFds[2];
    /// 当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = { 0 };
    RWMutexType m_mutex;
    /// socker事件上下文的容器
    std::vector<FdContext*> m_fdContexts;
};

}  // namespace zero

#endif