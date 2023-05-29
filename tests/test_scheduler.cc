#include "zero/fiber.h"
#include "zero/log.h"
#include "zero/scheduler.h"
#include "zero/thread.h"
#include "zero/util.h"
#include <functional>
#include <unistd.h>

static zero::Logger::ptr g_logger = ZERO_LOG_ROOT();

/// scheduler两种方式，一种使用user_caller,一种不使用

void Test_Func() {
    ZERO_LOG_INFO(g_logger) << "current thread of fiber id : " << zero::GetThreadId();
    ZERO_LOG_INFO(g_logger) << "fiber start working...";
    auto cur_fiber = zero::Fiber::GetThis();
    ZERO_LOG_INFO(g_logger) << "fiber id : " << cur_fiber->GetFiberId();
    /// scheduler会再将该fiber加入到schedule()中去
    cur_fiber->YieldToReady();
    ZERO_LOG_INFO(g_logger) << "current thread of fiber id : " << zero::GetThreadId();
    ZERO_LOG_INFO(g_logger) << "fiber end working...";
}

void Test_Func1(int val) {
    ZERO_LOG_INFO(g_logger) << "current thread id : " << zero::GetThreadId();
    ZERO_LOG_INFO(g_logger) << "var = " << val;
}

/// 主线程中想获取当前线程调度器时，必须是use_caller为true的情况，否则指针为空
void Test_Get_SC() {
    auto current_thread_sc = zero::Scheduler::GetThis();
    ZERO_LOG_INFO(g_logger) << current_thread_sc->getName();
    current_thread_sc->schedule(std::bind(&Test_Func1, 1));
    current_thread_sc->schedule(std::bind(&Test_Func1, 2));
    current_thread_sc->schedule(std::bind(&Test_Func1, 3));
}

void Test_Not_User_Caller() {
    zero::Scheduler sc(5, false, "test_not_user_caller");
    sc.start();
    // sleep(2);
    sc.schedule([]() { ZERO_LOG_INFO(g_logger) << "scheduler cb working"; });
    sc.schedule(zero::Fiber::ptr(new zero::Fiber(std::bind(&Test_Func), 128 * 1024)));
    /// 采用use_caller方式时，应当主动调用stop()
    sc.stop();
    ZERO_LOG_INFO(g_logger) << "test_schedule end";
}

/// 测试时不建议开启use_caller，因为当前主线程也会参与调度，陷入到idle协程上忙等待，无法观察协程对象释放过程
void Test_User_Caller() {
    zero::Scheduler sc(3, true, "test_user_caller");
    sc.start(); 
    sc.schedule(&Test_Get_SC);
    sc.stop(); 
    ZERO_LOG_INFO(g_logger) << "test_schedule end";  
}

void test_fiber() {
    static int s_count = 5;
    ZERO_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if (--s_count >= 0) {
        zero::Scheduler::GetThis()->schedule(&test_fiber, zero::GetThreadId());
    }
}

void sylar_test() {
    ZERO_LOG_INFO(g_logger) << "main";
    zero::Scheduler sc(3, false, "test");
    sc.start();
    sleep(2);
    ZERO_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    ZERO_LOG_INFO(g_logger) << "over";
}

int main() {
    Test_Not_User_Caller();
    sylar_test();
    // Test_User_Caller();  /// 会陷入忙等待
    return 0;
}