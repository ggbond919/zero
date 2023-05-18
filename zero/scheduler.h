#ifndef __ZERO_SCHEDULER_H__
#define __ZERO_SCHEDULER_H__

#include "fiber.h"
#include "mutex.h"
#include "thread.h"
#include "zero/noncopyable.h"
#include <atomic>
#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <ostream>
#include <vector>

namespace zero {

/**
 * @brief 协程调度器，内有线程池，支持协程在线程池中切换，封装N-M的协程调度器
 * 
 */
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Spinlock MutexType;

    /**
     * @brief Construct a new Scheduler object
     * 
     * @param threads 线程数量
     * @param use_caller 是否将当前线程作为调度线程
     * @param name 协程调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    virtual ~Scheduler();

    /**
     * @brief 返回协程调度器名称
     * 
     * @return const std::string& 
     */
    const std::string& getName() const {
        return m_name;
    }

    /**
     * @brief 返回当前协程调度器
     * 
     * @return Scheduler* 
     */
    static Scheduler* GetThis();

    /**
     * @brief 返回当前协程调度器的调度协程
     * 
     * @return Fiber* 
     */
    static Fiber* GetMainFiber();

    /**
     * @brief 启动协程调度器
     * 
     */
    void start();

    /**
     * @brief 停止协程调度器
     * 
     */
    void stop();

    /**
     * @brief 调度协程
     * 
     * @tparam FiberOrCb 
     * @param fc 协程或函数
     * @param thread 协程执行的线程id，-1表示任意线程
     */
    template <class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if (need_tickle) {
            tickle();
        }
    }

    /**
     * @brief 批量协程调度
     * 
     * @tparam InputIterator 
     * @param begin 
     * @param end 
     */
    template <class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if (need_tickle) {
            tickle();
        }
    }

    void switchTo(int thread = -1);
    std::ostream& dump(std::ostream& os);

private:
    /**
     * @brief 无锁的调度启动
     * 
     * @tparam FiberOrCb 
     * @param fc 
     * @param thread 
     * @return true 
     * @return false 
     */
    template <class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

protected:
    /**
     * @brief 通知协程调度器有任务了
     * 
     */
    virtual void tickle();

    /**
     * @brief 协程调度函数，多个线程去使用该函数调度携程
     * 
     */
    void run();

    /**
     * @brief 是否可以停止调度
     * 
     * @return true 
     * @return false 
     */
    virtual bool stopping();

    /**
     * @brief 协程无任务可调度时，执行idle协程
     * 
     */
    virtual void idle();

    /**
     * @brief 设置当前的协程调度器
     * 
     */
    void setThis();

    /**
     * @brief 是否有空闲协程
     * 
     * @return true 
     * @return false 
     */
    bool hasIdleThreads() {
        return m_idleThreadCount > 0;
    }

private:
    /**
     * @brief 协程/函数/线程组
     * 
     */
    struct FiberAndThread {
        /// 协程
        Fiber::ptr fiber;
        /// 协程执行函数
        std::function<void()> cb;
        /// 线程id
        int thread;

        /**
         * @brief Construct a new Fiber And Thread object
         * 
         * @param f 协程智能指针 
         * @param thr 线程id
         */
        FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}

        /**
         * @brief Construct a new Fiber And Thread object
         * 
         * @param f 协程智能指针的指针
         * @param thr 线程id
         */
        FiberAndThread(Fiber::ptr* f, int thr) : thread(thr) {
            fiber.swap(*f);
        }

        /**
         * @brief Construct a new Fiber And Thread object
         * 
         * @param f 协程执行函数
         * @param thr 
         */
        FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr) {}

        /**
         * @brief Construct a new Fiber And Thread object
         * 
         * @param f 协程执行函数指针
         * @param thr 
         */
        FiberAndThread(std::function<void()>* f, int thr) : thread(thr) {
            cb.swap(*f);
        }

        /**
         * @brief Construct a new Fiber And Thread object
         * 
         */
        FiberAndThread() : thread(-1) {}

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    MutexType m_mutex;
    /// 线程池
    std::vector<Thread::ptr> m_threads;
    /// 待执行协程队列
    std::list<FiberAndThread> m_fibers;
    /// use_caller为true时有效，调度协程
    Fiber::ptr m_rootFiber;
    /// 协程调度器名称
    std::string m_name;

protected:
    /// 协程下的线程id数组
    std::vector<int> m_threadIds;
    /// 线程数量
    size_t m_threadCount = 0;
    /// 工作线程数量
    std::atomic<size_t> m_activeThreadCount = { 0 };
    /// 空闲线程数量
    std::atomic<size_t> m_idleThreadCount = { 0 };
    /// 是否正在停止
    bool m_stopping = true;
    /// 是否自动停止
    bool m_autoStop = false;
    /// 主线程id(use_caller)
    int m_rootThread = 0;
};

class SchedulerSwitcher : public Noncopyable {
public:
    SchedulerSwitcher(Scheduler* target = nullptr);
    ~SchedulerSwitcher();
private:
    Scheduler* m_caller;
};

}  // namespace zero

#endif