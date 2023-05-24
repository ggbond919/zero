#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "util.h"
#include <atomic>
#include <cstddef>
#include <string>
#include <vector>
/// TODO:hook

namespace zero {

static zero::Logger::ptr g_logger = ZERO_LOG_NAME("system");

/// 当前线程的调度器
static thread_local Scheduler* t_scheduler = nullptr;
/// use_caller线程的调度协程，其他线程的主协程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) : m_name(name) {
    ZERO_ASSERT(threads > 0);

    if (use_caller) {
        // 提前创建一个属于use_caller线程的主协程
        zero::Fiber::GetThis();
        --threads;

        ZERO_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        // use_caller的调度协程 与其他线程任务相同，都是负责调度
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        zero::Thread::SetName(m_name);

        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread = zero::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    ZERO_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) {
        return;
    }
    m_stopping = false;
    ZERO_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();
}

void Scheduler::stop() {
    m_autoStop = true;
    if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)) {
        ZERO_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if (stopping()) {
            return;
        }
    }

    /// caller
    if (m_rootThread != -1) {
        ZERO_ASSERT(GetThis() == this)
    } else {
        ZERO_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if (m_rootFiber) {
        tickle();
    }

    /// 如果是use_caller 执行调度协程的调度之后再返回到use_caller的主协程上下文
    if (m_rootFiber) {
        if (!stopping()) {
            m_rootFiber->call();
        }
    }

    /// 测试一下引用计数问题
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for (auto& i : thrs) {
        i->join();
    }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    ZERO_LOG_INFO(g_logger) << m_name << " run";
    /// TODO:HOOK
    setThis();
    /// 如果当前线程不是caller线程，则为当前线程创建一个主协程
    if (zero::GetThreadId() != m_rootThread) {
        /// t_scheduler_fiber为线程局部对象，每个线程独占一份
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while (true) {
        ft.reset();
        /// 不会存在线程安全的问题吗？
        /// tickle_me继续调度  is_active有空闲任务
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                /// 需要到指定线程去调度，跳过
                if (it->thread != -1 && it->thread != zero::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                ZERO_ASSERT(it->fiber || it->cb);
                /// 该任务正在被调度，跳过
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            /// 有剩余任务继续通知线程进行调度
            tickle_me |= it != m_fibers.end();
        }

        /// 有任务到来，通知idle协程让出上下文
        if (tickle_me) {
            tickle();
        }

        if (ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn();
            --m_activeThreadCount;
            /// 当前任务可能被调用者在回调函数中执行了YieldToReady，只执行了一部分，需要将其再次放入到执行队列中去调度
            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
                /// YieldToHold 不用管了
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if (ft.cb) {
            /// 函数的调度也是由协程来承载的
            if (cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {  //if(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            /// 多线程状态下有可能任务队列已经为空了，但是该线程还是去取过任务，说明这个线程活被人抢了，啥也没干，混子线程
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM) {
                ZERO_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle() {
    ZERO_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    ZERO_LOG_INFO(g_logger) << "idle";
    while (!stopping()) {
        zero::Fiber::YieldToHold();
    }
}

void Scheduler::switchTo(int thread) {
    ZERO_ASSERT(Scheduler::GetThis() != nullptr);
    if (Scheduler::GetThis() == this) {
        if (thread == -1 || thread == zero::GetThreadId()) {
            return;
        }
    }
    schedule(Fiber::GetThis(), thread);
    Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream& os) {
    os << "[Scheduler name=" << m_name << " size=" << m_threadCount << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount << " stopping=" << m_stopping << " ]" << std::endl
       << "    ";
    for (size_t i = 0; i < m_threadIds.size(); ++i) {
        if (i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler* target) {
    m_caller = Scheduler::GetThis();
    if (target) {
        target->switchTo();
    }
}

SchedulerSwitcher::~SchedulerSwitcher() {
    if (m_caller) {
        m_caller->switchTo();
    }
}

}  // namespace zero