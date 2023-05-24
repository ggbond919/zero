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
uint64_t GetCurrentMS();

/**
 * @brief 获取当前时间的微妙
 * 
 * @return uint64_t 
 */
uint64_t GetCurrentUS();

/**
 * @brief 获取线程名称
 *
 */
std::string GetThreadName();

/**
 * @brief 获取当前函数调用栈
 * 
 * @param bt 保存栈
 * @param size 最多返回层数
 * @param skip 跳过栈顶
 */
void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

/**
 * @brief 栈信息转字符串
 * 
 * @param size 
 * @param skip 
 * @param prefix 栈信息前缀
 * @return std::string 
 */
std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");


}  // namespace zero

#endif