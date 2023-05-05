#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <signal.h> // for kill()
#include <sys/syscall.h>
#include <sys/stat.h>
#include <execinfo.h> // for backtrace()
#include <cxxabi.h>   // for abi::__cxa_demangle()
#include <algorithm>  // for std::transform()
#include "util.h"
#include "log.h"
// #include "fiber.h"

namespace zero{

pid_t GetThreadId(){
    return syscall(SYS_gettid);
}

uint64_t GetFiberId(){
    /// TODO:协程模块完善
    return 0;
}

 uint64_t GetElapsedMS(){
    /// TODO:
    return 0;
 }

 
 std::string GetThreadName(){
    /// TODO:
    return "";
 }

}
