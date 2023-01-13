cmake_minimum_required(VERSION 3.0.0)
project(zero)

# 将整体的一些配置信息包含进来，其他cmakelists文件也可以使用
include(cmake/utils.cmake)

#指定编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -Wall -Werror")

#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC") 

# -Wno-unused-function: 不要警告未使用函数
# -Wno-builtin-macro-redefined: 不要警告内置宏重定义，用于重定义内置的__FILE__宏
# -Wno-deprecated: 不要警告过时的特性
# -Wno-deprecated-declarations: 不要警告使用带deprecated属性的变量，类型，函数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")

include_directories(.)

option(BUILD_TEST "ON for compile test" ON)

find_package(Boost REQUIRED) 
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()
message(${Boost_INCLUDE_DIRS})

set(LIB_SRC
    zero/log.cc    
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

endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
