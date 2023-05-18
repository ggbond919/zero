#include "util.h"
#include "log.h"
#include "fiber.h"
#include <algorithm>  // for std::transform()
#include <cstddef>
#include <cstdlib>
#include <cxxabi.h>   // for abi::__cxa_demangle()
#include <dirent.h>
#include <execinfo.h>  // for backtrace()
#include <pthread.h>
#include <signal.h>    // for kill()
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include <vector>
// #include "fiber.h"

namespace zero {

static zero::Logger::ptr g_logger = ZERO_LOG_NAME("system");

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint64_t GetFiberId() {
    return Fiber::GetFiberId();
}

uint64_t GetElapsedMS() {
    /// TODO:
    return 0;
}

std::string GetThreadName() {
    char thread_name[16] = {0};
    pthread_getname_np(pthread_self(), thread_name, 16);
    return std::string(thread_name);
}

static std::string demangle(const char *str) {
    size_t size = 0;
    int status  = 0;
    std::string rt;
    rt.resize(256);
    if (1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0])) {
        char *v = abi::__cxa_demangle(&rt[0], nullptr, &size, &status);
        if (v) {
            std::string result(v);
            free(v);
            return result;
        }
    }
    if (1 == sscanf(str, "%255s", &rt[0])) {
        return rt;
    }
    return str;
}

void Backtrace(std::vector<std::string> &bt, int size, int skip){
    void  **array = (void **)malloc(sizeof(void *) * size);
    size_t s = ::backtrace(array, size);
    char **strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        ZERO_LOG_ERROR(g_logger) << "backtrace_synbols error";
        return;
    }
    for(size_t i = skip;i < s; ++i) {
        bt.push_back(demangle(strings[i]));
    }
    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string &prefix){
    std::vector<std::string> bt;
    Backtrace(bt,size,skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

}  // namespace zero