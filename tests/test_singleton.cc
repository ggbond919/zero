#include <iostream>

/**
 * @brief 多例模式封装类
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template <class T, typename X = void, int N = 0> class Singleton {
public:
    /**
     * @brief 返回单例裸指针
     */
    static T* GetInstance() {
        static T v;
        return &v;
        // return &GetInstanceX<T, X, N>();
    }
};

class Test_Singleton {
public:
    class Tag1 {};
    class Tag2 {};
    class Tag3 {};
    class Tag4 {};
};

int main() {
    // 默认单例
    auto s1 = Singleton<Test_Singleton>::GetInstance();
    std::cout << s1 << std::endl;
    auto s2 = Singleton<Test_Singleton>::GetInstance();
    std::cout << s2 << std::endl;
    std::cout << "--------------------------" << std::endl;

    // 打上不同tag
    auto s3 = Singleton<Test_Singleton, Test_Singleton::Tag1, 0>::GetInstance();
    std::cout << s3 << std::endl;

    auto s4 = Singleton<Test_Singleton, Test_Singleton::Tag1, 1>::GetInstance();
    std::cout << s4 << std::endl;
    auto s4_1 = Singleton<Test_Singleton, Test_Singleton::Tag1, 1>::GetInstance();
    std::cout << s4_1 << std::endl;
    std::cout << "--------------------------" << std::endl;

    auto s5 = Singleton<Test_Singleton, Test_Singleton::Tag1, 2>::GetInstance();
    std::cout << s5 << std::endl;
    auto s6 = Singleton<Test_Singleton, Test_Singleton::Tag2, 0>::GetInstance();
    std::cout << s6 << std::endl;
    std::cout << "--------------------------" << std::endl;

    // 多例可以使用整形区分，也可以是用类或者其他类型来区分
    auto s7 = Singleton<Test_Singleton, void, 0>::GetInstance();
    std::cout << s7 << std::endl;
    auto s8 = Singleton<Test_Singleton, void, 1>::GetInstance();
    std::cout << s8 << std::endl;

    auto s9 = Singleton<Test_Singleton, Test_Singleton::Tag3, 0>::GetInstance();
    std::cout << s9 << std::endl;
    auto s10 = Singleton<Test_Singleton, Test_Singleton::Tag4, 1>::GetInstance();
    std::cout << s10 << std::endl;

    return 0;
}