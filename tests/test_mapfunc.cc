#include <functional>
#include <iostream>
#include <map>

void fun1(int i) {
    printf("fun1()\n");
    printf("%d\n", i);
}
void fun2(int j) {
    printf("fun2()\n");
}
//typedef void *(*T)(void);
typedef std::function<void(int)> T;
int main() {
    std::map<int, T> m;
    m[1]     = fun1;
    m[2]     = fun2;
    auto it1 = m.find(1);
    it1->second(1);
    auto it = it1->second;
    it(2);

    // auto it = m.begin();
    // std::cout << it->first << "," << it->second << "," << it->second() << std::endl;
    // //auto f = (*it->second)();
    // printf("it->second:%x, it->second():%x\n", it->second, it->second());
    return 0;
}
