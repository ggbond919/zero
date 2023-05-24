#ifndef __ZERO_TIMER_H__
#define __ZERO_TIMER_H__

#include "thread.h"
#include "zero/mutex.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <vector>

namespace zero {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;

    /**
     * @brief 取消定时器
     * 
     * @return true 
     * @return false 
     */
    bool cancel();

    /**
     * @brief 刷新设置的定时器的执行时间
     * 
     * @return true 
     * @return false 
     */
    bool refresh();

    /**
     * @brief 重置定时器的执行时间
     * 
     * @param ms 定时器执行间隔时间 毫秒
     * @param from_now 是否从当前时间开始计算
     * @return true 
     * @return false 
     */
    bool reset(uint64_t ms, bool from_now);

    /**
     * @brief 获取当前timer的精确执行时间
     * 
     * @return uint64_t 
     */
    uint64_t getAccurateTime() { return m_next; }

private:
    /**
     * @brief Construct a new Timer object
     * 
     * @param ms 定时器执行间隔时间
     * @param cb 
     * @param recurring 是否循环执行
     * @param manager 定时器管理器
     */
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);

    /**
     * @brief Construct a new Timer object
     * 
     * @param next 执行的时间戳 毫秒
     */
    Timer(uint64_t next);

private:
    /// 是否循环定时器
    bool m_recurring = false;
    /// 执行间隔
    uint64_t m_ms = 0;
    /// 精确的执行时间
    uint64_t m_next = 0;
    /// 回调函数
    std::function<void()> m_cb;
    /// 定时器管理器
    TimerManager* m_manager = nullptr;

private:
    /**
     * @brief 定时器比较仿函数
     * 
     */
    struct Comparator {
        /**
         * @brief 比较定时器的智能指针的大小（按照执行时间进行排序）
         * 
         * @param lhs 
         * @param rhs 
         * @return true 
         * @return false 
         */
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};

/**
 * @brief 定时器管理类,内有纯虚函数,需要IOManager类去实现
 * 
 */
class TimerManager {
    friend class Timer;

public:
    typedef RWMutex RWMutexType;

    TimerManager();

    virtual ~TimerManager();

    /**
     * @brief 添加定时器
     * 
     * @param ms 定时器执行间隔时间
     * @param cb 定时器回调函数
     * @param recurring 是否循环定时器
     * @return Timer::ptr 
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    /**
     * @brief 添加条件定时器
     * 
     * @param ms 
     * @param cb 
     * @param weak_cond 条件
     * @param recurring 
     * @return Timer::ptr 
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

    /**
     * @brief 到最近一个定时器的执行时间间隔
     * 
     * @return uint64_t 
     */
    uint64_t getNextTimer();

    /**
     * @brief 获取超时的timer回调函数集合
     * 
     * @param cbs 
     */
    void listExpiredCb(std::vector<std::function<void()>>& cbs);

    /**
     * @brief 是否有定时器
     * 
     * @return true 
     * @return false 
     */
    bool hasTimer();

protected:
    /**
     * @brief 当有新的定时器插入到定时器的首部，执行该函数
     * 
     */
    virtual void onTimerInsertedAtFront() = 0;

    /**
     * @brief 将定时器添加到管理器中
     * 
     * @param var 
     * @param lock 
     */
    void addTimer(Timer::ptr var, RWMutexType::WriteLock& lock);

private:
    /**
     * @brief 检测服务器时间是否被调后了
     * 
     * @param now_ms 
     * @return true 
     * @return false 
     */
    bool detectClockRollover(uint64_t now_ms);

private:
    RWMutexType m_mutex;
    /// 定时器集合 最小堆
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    /// 是否触发onTimerInsertedAtFront
    bool m_tickled = false;
    /// 上次执行时间
    uint64_t m_previousTime = 0;
};

}  // namespace zero

#endif