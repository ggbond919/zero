#ifndef __ZERO_FIBER_H__
#define __ZERO_FIBER_H__

#include <bits/stdint-uintn.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <sys/ucontext.h>
#include <ucontext.h>

namespace zero {

class Scheduler;
/// enable_shared_from_this即只有this指针时，如何安全得到this的shared_ptr
class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        /// 初始态
        INIT,
        /// 暂停状态
        HOLD,
        /// 执行中
        EXEC,
        /// 结束状态
        TERM,
        /// 可执行
        READY,
        /// 异常状态
        EXCEPT
    };

private:
    /**
     * @brief 主协程，负责调度其他协程
     * 
     */
    Fiber();

public:
    /**
     * @brief Construct a new Fiber object
     * 
     * @param cb 协程执行函数
     * @param stacksize 协程栈大小 
     * @param use_caller 是否在MainFiber上调度
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

    ~Fiber();

    /**
     * @brief 重置协程执行函数，并设置状态
     * 
     * @param cb 
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 将当前协程切换到运行状态
     * 
     */
    void swapIn();

    /**
     * @brief 将当前协程切换到后台
     * 
     */
    void swapOut();

    /**
     * @brief 将当前线程切换到执行状态，执行当前线程的主协程
     * 
     */
    void call();

    /**
     * @brief 将当前协程切换到后台，执行主协程上下文
     * 
     */
    void back();

    /**
     * @brief 返回协程id
     * 
     * @return uint64_t 
     */
    uint64_t getId() const {
        return m_id;
    }

    /**
     * @brief 返回协程状态
     * 
     * @return State 
     */
    State getState() const {
        return m_state;
    }

public:
    /**
     * @brief 设置当前线程的运行协程
     * 
     * @param f 
     */
    static void SetThis(Fiber* f);

    /**
     * @brief 返回当前所在协程
     * 
     * @return Fiber::ptr 
     */
    static Fiber::ptr GetThis();

    /**
     * @brief 当前协程让出CPU切换到后台,并设置READY状态
     * 
     */
    static void YieldToReady();

    /**
     * @brief 当前协程切换到后台,并设置HOLD状态
     * 
     */
    static void YieldToHold();

    /**
     * @brief 返回总协程数量
     * 
     * @return uint64_t 
     */
    static uint64_t TotalFibers();

    /**
     * @brief 协程执行函数,执行完返回到线程的主协程
     * 
     */
    static void MainFunc();

    /**
     * @brief 协程执行函数，执行完成后返回到线程调度的协程
     * 
     */
    static void CallerMainFunc();

    /**
     * @brief Get the Fiber Id object获取当前协程id
     * 
     */
    static uint64_t GetFiberId();

private:
    /// 协程id
    uint64_t m_id = 0;
    /// 协程栈
    uint32_t m_stacksize = 0;
    /// 协程状态
    State m_state = INIT;
    /// 协程上下文
    ucontext_t m_ctx;
    /// 协程运行栈指针
    void* m_stack = nullptr;
    /// 协程运行函数
    std::function<void()> m_cb;
};

}  // namespace zero

#endif