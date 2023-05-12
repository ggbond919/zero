#include "thread.h"
#include "log.h"
#include "zero/mutex.h"
#include "zero/util.h"
#include <functional>
#include <pthread.h>
#include <stdexcept>
#include <unistd.h>

namespace zero {

// 每个线程独占一份thread_local变量
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

// 没有添加appender 不会输出的
static zero::Logger::ptr g_logger = ZERO_LOG_NAME("system");

Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if (name.empty()) {
        return;
    }
    if (t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name) : m_cb(cb), m_name(name) {
    if (name.empty()) {
        m_name = "UNKNOW";
    }

    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (rt) {
        ZERO_LOG_ERROR(g_logger) << "pthread_create thread fail, rt" << rt << " namme=" << name;
        throw std::logic_error("pthread_create error");
    }
    // sleep(3);

    // 不设置信号量，可能该Thread对象先执行了析构函数，再去运行run
    // 这时就会出现问题，因为该对象已经被析构了
    m_semaphore.wait();
}

Thread::~Thread() {
    // system日志没加appender，会无法打印，所以调试信息暂时用了cout
    // std::cout << "deconstructor..." << std::endl;
    if (m_thread) {
        pthread_detach(m_thread);
    }
}

void Thread::join() {
    if (m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if (rt) {
            ZERO_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        // 保证析构函数执行时，不再detach
        m_thread = 0;
    }
}

void* Thread::run(void* arg) {
    Thread* thread = ( Thread* )arg;
    // 每个线程独占一份
    t_thread = thread;
    t_thread_name = thread->m_name;
    // 线程号
    thread->m_id = zero::GetThreadId();
    // 设置独一无二的线程名称，字符最多16个
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    // 减少一下原本function里面智能指针的引用，如果有的话
    std::function<void()> cb;
    cb.swap(thread->m_cb);

    // 防止该Thread对象提前调用析构函数
    thread->m_semaphore.notify();

    cb();
    return 0;
}

}  // namespace zero