#include "../zero/mutex.h"
#include "zero/log.h"
#include <cstdlib>
#include <deque>
#include <semaphore.h>
#include <string>
#include <thread>
#include <unistd.h>
zero::Logger::ptr g_logger = ZERO_LOG_ROOT();
zero::Logger::ptr test_mutex = ZERO_LOG_NAME("test_mutex");

zero::Semaphore s;

void Test_Semaphore_Wait() {
    s.wait();
    ZERO_LOG_INFO(g_logger) << "thread working...";
}

void Test_Semaphore_Post() {
    s.notify();
}

void Test_SemPhore() {
    std::thread t1(&Test_Semaphore_Wait);
    std::thread t2(&Test_Semaphore_Post);

    t1.join();
    t2.join();
}

/**
 * @brief 生产者消费者问题，信号量实现
 *        要求：队列为空时，消费者不可取，队列为满时，生产者不可生产，同一时刻只有一个线程能持有资源
 * 
 */
std::deque<int> dq;
// 如果多个生产者 多个消费者则需要对共享资源加锁
// 共享资源
static int pc = 0;
// 等于1时，并且不会出现掠夺相同资源的情况，此时运行正常，
// 大于1时，会出现多个生产者生产相同的资源，多个消费者消费相同的资源，此时需要为共享资源加锁
// p >= 1 保证生产者先工作
zero::Semaphore p(5);
zero::Semaphore c;
zero::Mutex mutex;

void Produce() {
    while (1) {
        p.wait();
        mutex.lock();
        // 多个生产者进入到该函数时，也会导致生产出相同的资源 ++操作本身不是原子性的
        pc++;
        ZERO_LOG_INFO(test_mutex) << "生产者正在生产资源 : " << std::to_string(pc);
        dq.push_back(pc);
        // 生产完后通知阻塞的消费者
        mutex.unlock();
        c.notify();
       
        // sleep(3);
    }
}

void Custom() {
    while (1) {
        c.wait();
        mutex.lock();
        auto res = dq.front();
        ZERO_LOG_INFO(test_mutex) << "消费者正在消费资源 : " << std::to_string(res);

        // 当多个消费者同时使用资源时，很有可能会在这里出现问题
        // c1 在pop_front之前，c2也进入到了此地，二者持有的res会出现相同的情况
        dq.pop_front();
        // 消费后可以通知生产者再次生产
        mutex.lock();
        p.notify();
        
    }
}

void TestPC() {
    srand(time(0));
    std::thread t1(&Produce);  // 生产者
    std::thread t3(&Produce);  // 生产者
    std::thread t2(&Custom);   // 消费者
    std::thread t4(&Custom);   // 消费者

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

int main() {
    zero::FileLogAppender::ptr file_appender(new zero::FileLogAppender("../log/test_mutex.txt"));
    test_mutex->addAppender(file_appender);
    // Test_SemPhore();
    TestPC();

    return 0;
}