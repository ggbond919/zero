#include "zero/log.h"
#include "zero/mutex.h"
#include "zero/thread.h"
#include "zero/util.h"
#include <cstddef>
#include <functional>
#include <unistd.h>

static int test_thread_val = 0;
zero::Logger::ptr g_logger = ZERO_LOG_ROOT();

// 读写锁 互斥量 自旋锁，乐观锁，悲观锁
// https://cloud.tencent.com/developer/news/695304
zero::Mutex g_mutex;
zero::RWMutex g_wrmutex;
zero::Spinlock g_spinmutex;

void Test_Thread_Func() {
    zero::Thread* t = zero::Thread::GetThis();
    ZERO_LOG_INFO(g_logger) << "current thread name : " << zero::Thread::GetName() << " == this name : " << t->getName()
                            << " == util method thread name : " << zero::GetThreadName() << "\n"
                            << "current thread id : " << t->getId() << " == util method thread id : " << zero::GetThreadId();
}

void Test_Thread() {
    for (int i = 0; i < 10000; ++i) {
        // zero::Mutex::Lock lock(g_mutex);
        // zero::RWMutex::WriteLock lock(g_wrmutex);
        // zero::Spinlock::Lock lock(g_spinmutex);
        test_thread_val++;
    }
}

// 该测试用例主要测试如果不在Thread类构造函数和run函数中加信号量会发生什么情况
// 如果不加信号量，很有可能先执行了Thread的析构函数，待析构完成又后去执行了run函数，这样在run中就访问了异常的内存
// 导致程序死掉
void Test_Thread_Construct_Deconstruct() {
    std::vector<zero::Thread::ptr> thrs;
    for (int i = 0; i < 3; i++) {
        zero::Thread::ptr thr(new zero::Thread(std::bind(Test_Thread_Func), "thread_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    // 如果b不进行join的话，析构时会detach，很可能主函数执行完了，线程还没来得及执行
    for (size_t i = 0; i < thrs.size(); i++) {
        thrs[i]->join();
    }
}

// 局部锁的简单测试
void Test_Thread_Mutex() {
    std::vector<zero::Thread::ptr> thrs;
    for (int i = 0; i < 5; i++) {
        zero::Thread::ptr thr(new zero::Thread(std::bind(Test_Thread), "thread_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    // 如果b不进行join的话，析构时会detach，很可能主函数执行完了，线程还没来得及执行
    for (size_t i = 0; i < thrs.size(); i++) {
        thrs[i]->join();
    }
}

int main() {
    Test_Thread_Mutex();
    Test_Thread_Construct_Deconstruct();
    ZERO_LOG_INFO(g_logger) << "total count : " << test_thread_val;
}