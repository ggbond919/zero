#include "timer.h"
#include "util.h"
#include "zero/mutex.h"
#include "zero/log.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace zero {

static zero::Logger::ptr g_logger = ZERO_LOG_NAME("system");

bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    /// 升序排列，最小堆
    if (lhs->m_next < rhs->m_next) {
        return true;
    }
    if (rhs->m_next < lhs->m_next) {
        return false;
    }
    /// 时间相等，比较地址
    return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager)
    : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager) {
    m_next = zero::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next) : m_next(next) {}

bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    /// 已经执行完了不处理
    if(!m_cb) {
        return false;
    }

    /// 已经处理过了
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    /// 先删除，再insert，否则会对堆的排序造成影响
    m_manager->m_timers.erase(it);
    /// 刷新该定时器下次执行时间
    m_next = zero::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    /// 无意义的重置，应该也是false
    if(ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now) {
        start = zero::GetCurrentMS();
    } else {
        /// 先把初始的执行间隔消除掉
        start = m_next - m_ms;
    }
    /// 再重置时间间隔
    /// 可以提前执行，也可以延后执行
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(),lock);
    return true;
}

TimerManager::TimerManager() {
    m_previousTime = zero::GetCurrentMS();
}

TimerManager::~TimerManager() {}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
    Timer::ptr timer(new Timer(ms,cb,recurring,this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()) {
        return ~0ull;
    }
    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = zero::GetCurrentMS();
    /// now_ms >= next->m_next 该timer已经执行过了
    /// 否则获取将要执行的timer毫秒
    if(now_ms >= next->m_next) {
        return 0;
    } else {
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs) {
    uint64_t now_ms = zero::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutex::ReadLock lock(m_mutex);
        if(m_timers.empty()) {
            return;
        }
    }

    RWMutexType::WriteLock lock(m_mutex);
    if(m_timers.empty()) {
        return;
    }
    bool rollover = detectClockRollover(now_ms);

    /// 系统时间未回调，且最早应该执行的timer也尚未到执行时间，则直接返回
    if(!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    /// 以now_timer为分界点，返回第一个大于等于now_timer的位置
    /// 如果系统时刻回调了至少一小时，即rollover为true时，则将timer集合全部加入到待执行数组中
    /// 否则将已经超时的timer回调函数加入到待执行数组中
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    /// 将等于now_timer执行时间的timer也包含在expierd待执行数组中，即it后移
    while(it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    /// ZERO_LOG_INFO(g_logger) << (*it).get();
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());

    for(auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        /// ZERO_LOG_INFO(g_logger) << timer.get();
        if(timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}

void TimerManager::addTimer(Timer::ptr var, RWMutexType::WriteLock& lock) {
    auto it = m_timers.insert(var).first;
    /// 如果新插入的Timer 其执行时间最早则立刻执行
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if(at_front) {
        m_tickled = true;
    }
    lock.unlock();
    /// 加锁的粒度
    /// 立刻执行
    if(at_front) {
        onTimerInsertedAtFront();
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    /// 当前系统时间发生改变，且至少回调了一个小时
    /// 实现确实有瑕疵
    if(now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previousTime = now_ms;
    return rollover;
}

bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

}  // namespace zero