#ifndef __ZERO_MACRO_H__
#define __ZERO_MACRO_H__

#include "log.h"
#include "util.h"
#include <assert.h>
#include <string.h>

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#define ZERO_LIKELY(x) __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define ZERO_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define ZERO_LIKELY(x) (x)
#define ZERO_UNLIKELY(x) (x)
#endif

/// 断言宏封装
#define ZERO_ASSERT(x)                                                                                                      \
    if (ZERO_UNLIKELY(!(x))) {                                                                                              \
        ZERO_LOG_ERROR(ZERO_LOG_ROOT()) << "ASSERTION: " #x << "\nbacktrace:\n" << zero::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                                                          \
    }

/// 断言宏封装
#define ZERO_ASSERT2(x, w)                                                          \
    if (ZERO_UNLIKELY(!(x))) {                                                      \
        ZERO_LOG_ERROR(ZERO_LOG_ROOT()) << "ASSERTION: " #x << "\n"                 \
                                        << w << "\nbacktrace:\n"                    \
                                        << zero::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                  \
    }
#endif