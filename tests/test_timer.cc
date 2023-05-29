#include "zero/timer.h"
#include "zero/log.h"
#include "zero/iomanager.h"
#include "zero/util.h"
#include <cstdint>
#include <functional>
#include <unistd.h>

zero::Logger::ptr g_logger = ZERO_LOG_ROOT();

void Test_Timer_Func1(int val) {
    // ZERO_LOG_INFO(g_logger) << "timer runing: " << "var = " << val;
}



void Test() {
    zero::IOManager iom;

    ZERO_LOG_INFO(g_logger) << "Test start...";
    zero::Timer::ptr timer1 = iom.addTimer(2000, std::bind(&Test_Timer_Func1,3),false);
    sleep(3);
    ZERO_LOG_INFO(g_logger) << (int64_t)(zero::GetCurrentMS() - timer1->getAccurateTime());


    zero::Timer::ptr timer2 = iom.addTimer(3000, std::bind(&Test_Timer_Func1,3),false);
    ZERO_LOG_INFO(g_logger) << zero::GetCurrentMS() - timer2->getAccurateTime();

    zero::Timer::ptr timer3 = iom.addTimer(3000, std::bind(&Test_Timer_Func1,3),false);
    ZERO_LOG_INFO(g_logger) << zero::GetCurrentMS() - timer3->getAccurateTime();    
    std::vector<std::function<void ()>> cbs;
    iom.listExpiredCb(cbs);
    ZERO_LOG_INFO(g_logger) << timer1.get();
    ZERO_LOG_INFO(g_logger) << "Test end...";
}

int main() {
    Test();


    return 0;
}