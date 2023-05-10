#include "zero/noncopyable.h"
#include <iostream>

class Test_Noncopyable : public zero::Noncopyable {
public:
    Test_Noncopyable() {
        std::cout << "constructor..." << std::endl;
    }
};

int main() {
    Test_Noncopyable tn;
    // Test_Noncopyable tn_copy(tn);
    return 0;
}