cmake_minimum_required(VERSION 3.0.0)
project(zero)

# 将整体的一些配置信息包含进来，其他cmakelists文件也可以使用
include(cmake/utils.cmake)

# 指定编译选项
# -Werror
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -fno-limit-debug-info -Wall") 

#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC") 

# -Wno-unused-function: 不要警告未使用函数
# -Wno-builtin-macro-redefined: 不要警告内置宏重定义，用于重定义内置的__FILE__宏
# -Wno-deprecated: 不要警告过时的特性
# -Wno-deprecated-declarations: 不要警告使用带deprecated属性的变量，类型，函数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")
set(CMAKE_EXPORT_COMPILECOMMANDS ON)
include_directories(.)

option(BUILD_TEST "ON for compile test" ON)

find_package(Boost REQUIRED) 
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()
message("boost path" ${Boost_INCLUDE_DIRS})

set(LIB_SRC
    zero/log.cc
    zero/util.cc
    zero/config.cc
    zero/thread.cc
    zero/mutex.cc    
    zero/fiber.cc 
    zero/scheduler.cc
    zero/iomanager.cc
    zero/timer.cc
    zero/fd_manager.cc
    zero/hook.cc
    zero/address.cc
    zero/socket.cc
    zero/bytearray.cc
    zero/tcp_server.cc
    zero/stream.cc
    zero/streams/socket_stream.cc
)

add_library(zero SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(zero)

set(LIBS
    zero
    pthread
    dl
    yaml-cpp
)

if(BUILD_TEST)
# 见cmake文件夹下utils.cmake
zero_add_executable(test_timefunc "tests/test_timefunc.cc" zero "${LIBS}")
zero_add_executable(test_mapfunc "tests/test_mapfunc.cc" zero "${LIBS}")
zero_add_executable(test_constructor "tests/test_constructor.cc" zero "${LIBS}")
zero_add_executable(test_singleton "tests/test_singleton.cc" zero "${LIBS}")
zero_add_executable(test_log "tests/test_log.cc" zero "${LIBS}")
zero_add_executable(test_config "tests/test_config.cc" zero "${LIBS}")
zero_add_executable(test_mutex "tests/test_mutex.cc" zero "${LIBS}")
zero_add_executable(test_noncopyable "tests/test_noncopyable.cc" zero "${LIBS}")
zero_add_executable(test_thread "tests/test_thread.cc" zero "${LIBS}")
zero_add_executable(test_ucontext "tests/test_ucontext.cc" zero "${LIBS}")
zero_add_executable(test_fiber "tests/test_fiber.cc" zero "${LIBS}")
zero_add_executable(test_iomanager "tests/test_iomanager.cc" zero "${LIBS}")
zero_add_executable(test_timer "tests/test_timer.cc" zero "${LIBS}")
zero_add_executable(test_scheduler "tests/test_scheduler.cc" zero "${LIBS}")
zero_add_executable(test_endian "tests/test_endian.cc" zero "${LIBS}")
zero_add_executable(test_address "tests/test_address.cc" zero "${LIBS}")
zero_add_executable(test_socket "tests/test_socket.cc" zero "${LIBS}")
zero_add_executable(test_bytearray "tests/test_bytearray.cc" zero "${LIBS}")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
