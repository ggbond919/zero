#include "zero/fiber.h"
#include "zero/log.h"
#include "zero/macro.h"
#include "zero/thread.h"
#include "zero/util.h"
#include <functional>

zero::Logger::ptr g_logger = ZERO_LOG_ROOT();

/// Fiber类仅能一个主协程和一个子协程进行切换
/// 注意子协程和主协程析构的时机
void Test_UCFiber_Func() {
    zero::Fiber* raw_ptr;

    ZERO_LOG_INFO(g_logger) << "user caller fiber running...";
    /// 子协程每次Getthis()都会使得引用计数+1
    zero::Fiber::ptr cur = zero::Fiber::GetThis();
    ZERO_LOG_INFO(g_logger) << cur.use_count();
    ZERO_LOG_INFO(g_logger) << cur.get();
    ZERO_LOG_INFO(g_logger) << "current state : " << cur->getState();
    raw_ptr = cur.get();
    cur.reset();

    /// 如果子协程执行任务过程中再调用back，会导致引用计数无法减为0，从而无法释放
    raw_ptr->back();
    ZERO_LOG_INFO(g_logger) << "never reach here";
}

void Test_Usercaller_Fiber() {
    ZERO_LOG_INFO(g_logger) << "test thread start";
    // 主协程
    zero::Fiber::ptr cur = zero::Fiber::GetThis();
    ZERO_LOG_INFO(g_logger) << "main fiber address : " << cur.get();
    ZERO_ASSERT(cur->getId() == zero::Fiber::GetFiberId());
    ZERO_ASSERT(cur->getId() == zero::GetFiberId());

    zero::Fiber::ptr uc_fiber(new zero::Fiber(std::bind(&Test_UCFiber_Func), 128 * 1024, true));
    ZERO_LOG_INFO(g_logger) << uc_fiber.use_count();
    ZERO_LOG_INFO(g_logger) << "worker fiber address : " << uc_fiber.get();
    ZERO_LOG_INFO(g_logger) << "init state : " << uc_fiber->getState();

    uc_fiber->call();
    ZERO_LOG_INFO(g_logger) << uc_fiber.use_count();
    ZERO_LOG_INFO(g_logger) << "back state : " << uc_fiber->getState();

    ZERO_LOG_INFO(g_logger) << "test thread working...";
    ZERO_LOG_INFO(g_logger) << "test thread end...";
    /// 必须让子协程执行完，否则Fiber::CallerMainFunc()无法继续执行，导致引用计数不能减为0，而无法释放。
    uc_fiber->call();
}

int main() {
    ZERO_LOG_INFO(g_logger) << "main start...";
    zero::Thread t(std::bind(&Test_Usercaller_Fiber), "test_thread");
    t.join();
    ZERO_LOG_INFO(g_logger) << "main end...";
    return 0;
}