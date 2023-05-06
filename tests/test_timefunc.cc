#include <ctime>
#include <iostream>
#include <ostream>
#include <time.h>

int main() {
    struct tm tm;
    time_t t = time(NULL);
    char buf[64];
    localtime_r(&t, &tm);
    const std::string& format = "%Y-%m-%d %H:%M:%S";
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    std::cout << buf;

    return 0;
}