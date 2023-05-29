#include "fiber.h"
#include "config.h"
#include "log.h"
#include "macro.h"
#include "zero/util.h"
#include "scheduler.h"
#include <atomic>
#include <bits/stdint-uintn.h>
#include <bits/types/FILE.h>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <functional>
#include <ostream>
#include <ucontext.h>

namespace zero {

/// 需要加appender
static Logger::ptr g_logger = ZERO_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{ 0 };
static std::atomic<uint64_t> s_fiber_count{ 0 };

/// 当前正在运行的协程
static thread_local Fiber* t_fiber = nullptr;
/// 主协程
static thread_local Fiber::ptr t_threadFiber = nullptr;

/// 配置文件中读取协程栈大小，默认128k
static ConfigVar<uint32_t>::ptr g_fiber_static_size = Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if (getcontext(&m_ctx)) {
        ZERO_ASSERT2(false, "getcontext");
    }
    ++s_fiber_count;
    ZERO_LOG_INFO(g_logger) << "Fiber::Fiber main";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller) : m_id(++s_fiber_id), m_cb(cb) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_static_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if (getcontext(&m_ctx)) {
        ZERO_ASSERT2(false, "getcontext");
    }

    /// 由调度器来调度切换
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if (!use_caller) {
        /// 调度器调度
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        /// 自己调度
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
    ZERO_LOG_INFO(g_logger) << "Fiber::Fiber id = " << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if(this == t_threadFiber.get()) {
        /// 所有主协程id都是0，
        ZERO_LOG_INFO(g_logger) << "Fiber::~Fiber MainFiber id = " << m_id << " total = " << s_fiber_count;
    } else {
        ZERO_LOG_INFO(g_logger) << "Fiber::~Fiber id = " << m_id << " total = " << s_fiber_count;
    }
    if (m_stack) {
        ZERO_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        ZERO_ASSERT(!m_cb);
        ZERO_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }
}

void Fiber::reset(std::function<void()> cb) {
    ZERO_ASSERT(m_stack);
    ZERO_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        ZERO_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

/// 主协程上下文保存在调用该函数的地方
void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        ZERO_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back() {
    SetThis(t_threadFiber.get());
    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        ZERO_ASSERT2(false, "swapcontext");
    }
}

/// 主协程上下文保存在调用该函数的地方
void Fiber::swapIn() {
    SetThis(this);
    ZERO_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        ZERO_ASSERT2(false, "swapcontext");
    }
}

/// 切换至主协程上下文中去
void Fiber::swapOut() {
    /// 多线程情况下，切换到执行线程的主协程中去
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        ZERO_ASSERT2(false, "swapcontext");
    }
}

void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }

    // 主协程只调用无参构造一次
    Fiber::ptr main_fiber(new Fiber);
    ZERO_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    ZERO_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    ZERO_ASSERT(cur->m_state == EXEC);
    cur->swapOut();
}

uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    ZERO_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        ZERO_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << " fiber_id = " << cur->getId() << std::endl
                                 << zero::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        ZERO_LOG_ERROR(g_logger) << "Fiber Except"
                                 << " fiber_id = " << cur->getId() << std::endl
                                 << zero::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    /// 让子协程引用计数减1
    /// 当外部函数执行完，引用计数为0时，自动释放
    cur.reset();  
    raw_ptr->swapOut();

    ZERO_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc() {
    /// 子协程引用计数会加1
    Fiber::ptr cur = GetThis();
    ZERO_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        ZERO_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << " fiber_id=" << cur->getId() << std::endl
                                 << zero::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        ZERO_LOG_ERROR(g_logger) << "Fiber Except"
                                 << " fiber_id=" << cur->getId() << std::endl
                                 << zero::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    /// 让子协程引用计数减1
    /// 当外部函数执行完，引用计数为0时，自动释放
    cur.reset();
    raw_ptr->back();
    ZERO_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

}  // namespace zero