#include <iostream>

int main() {
    class test_construtor1 {
    public:
        test_construtor1(int i) {}
    };
    class test_construtor2 : public test_construtor1 {
    public:
        test_construtor2()
            : test_construtor1(1) {}
    };

    return 0;
}