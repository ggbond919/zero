#ifndef __ZERO_NONCOPYABLE_H__
#define __ZERO_NONCOPYABLE_H__

namespace zero {

/**
 * @brief 禁止拷贝函数和赋值运算
 * 
 */
class Noncopyable {
public:
    Noncopyable() = default;

    ~Noncopyable() = default;

    Noncopyable(const Noncopyable&) = delete;

    Noncopyable& operator=(const Noncopyable&) = delete;
};

}  // namespace zero

#endif