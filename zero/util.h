#ifndef __ZERO_UTIL_H__
#define __ZERO_UTIL_H__

#include <bits/stdint-uintn.h>
#include <cxxabi.h>  // for abi::__cxa_demangle()
#include <iostream>
#include <stdint.h>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <vector>

namespace zero {

/**
 * @brief 获取线程ID
 *      pthread_t 同一进程内唯一，不同进程内会重复
 *      pid_t 全局内唯一
 */
pid_t GetThreadId();

/**
 * @brief 获取协程ID
 * 暂时返回0
 */
uint64_t GetFiberId();

/**
 * @brief 获取服务器当前启动的毫秒数
 *
 */
uint64_t GetElapsedMS();

/**
 * @brief 获取线程名称
 *
 */
std::string GetThreadName();

}  // namespace zero

#endif