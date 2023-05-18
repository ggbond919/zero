#include "zero/fiber.h"
#include "zero/log.h"
#include "zero/macro.h"
#include "zero/thread.h"
#include "zero/util.h"
#include <functional>

zero::Logger::ptr g_logger = ZERO_LOG_ROOT();

void Test_UCFiber_Func() {
    ZERO_LOG_INFO(g_logger) << "user caller fiber running...";
    zero::Fiber::ptr cur = zero::Fiber::GetThis();
    ZERO_LOG_INFO(g_logger) << "current state : " << cur->getState();
    cur->back();
    ZERO_LOG_INFO(g_logger) << "user caller fiber end...";
    cur->back();
}

void Test_Usercaller_Fiber() {
    ZERO_LOG_INFO(g_logger) << "test thread start";
    // 主协程
    zero::Fiber::ptr cur = zero::Fiber::GetThis();
    ZERO_ASSERT(cur->getId() == zero::Fiber::GetFiberId());
    ZERO_ASSERT(cur->getId() == zero::GetFiberId());

    zero::Fiber::ptr uc_fiber(new zero::Fiber(std::bind(&Test_UCFiber_Func), 128 * 1024, true));
    ZERO_LOG_INFO(g_logger) << "init state : " << uc_fiber->getState();
    uc_fiber->call();
    ZERO_LOG_INFO(g_logger) << "back state : " << uc_fiber->getState();
    ZERO_LOG_INFO(g_logger) << "test thread working...";
    uc_fiber->call();
}

int main() {
    ZERO_LOG_INFO(g_logger) << "main start...";
    zero::Thread t(std::bind(&Test_Usercaller_Fiber), "test_thread");
    t.join();
    ZERO_LOG_INFO(g_logger) << "main end...";
    return 0;
}