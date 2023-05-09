#ifndef __ZERO_SINGLETON_H__
#define __ZERO_SINGLETON_H__

#include <memory>

namespace zero {

/**
 * @brief 多例的实现，一个类可以多个实例，默认情况下为单例
 *        可以通过 X或者N 来控制多个实例
 * @tparam T
 * @tparam X
 * @tparam N
 */
template <class T, class X = void, int N = 0>
class Singleton {
public:
    static T* GetInstance() {
        static T v;
        return &v;
    }
};

template <class T, class X = void, int N = 0>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}  // namespace zero

#endif