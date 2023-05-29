#include "zero/myendian.h"
#include "zero/log.h"
#include <byteswap.h>
#include <cstdint>
#include <iostream>
#include <arpa/inet.h>

static zero::Logger::ptr g_logger = ZERO_LOG_ROOT();

/// 发现了myendian中的一个问题
/// 如果不包含系统的#include <endian.h>头文件，大小端宏识别有问题
#if BYTE_ORDER == LITTLE_ENDIAN
#define TEST_BYTE_ORDER 1
#else
#define TEST_BYTE_ORDER 0
#endif                                                                                                  
void Test_Endian() {

    /// 联合体公用一块空间
    union Data {
        uint16_t num;
        unsigned char c[2];
    };

    Data data;
    /// 16进制下，01 占一个字节，02占一个字节
    data.num = 0x0102;
    /// 大端：低位数据存在高地址上，高位数据存在低地址上
    /// 小端：低位数据存在低地址上，高位数据存在高地址上
    /// 内存是由低向高增长的
    if (data.c[0] == 0x01 && data.c[1] == 0x02) {
        ZERO_LOG_INFO(g_logger) << "大端";
    } else if(data.c[1] == 0x01 && data.c[0] == 0x02) {
        ZERO_LOG_INFO(g_logger) << "小端";
    } else {
        ZERO_LOG_ERROR(g_logger) << "error";
    }

}

/// 如果当前机器是小端的，那么调用byteswapOnLittleEndian函数即可
/// 如果当前机器是大端的，那么调用byteswapOnBigEndian函数即可
/// ntohs 仅支持2和4字节的网络字节序到主机字节序的转换，所以封装了支持8字节序的转换函数
void Test_BSwap() {
    /// 8字节序交换
    uint64_t x = 0x0123456789abcdef;
    
    ZERO_LOG_INFO(g_logger) << std::hex << x << " x ==> swap(x) " << std::hex << bswap_64(x);
    ZERO_LOG_INFO(g_logger) << std::hex << x << " x ==> swap(x) " << std::hex << zero::byteswapOnLittleEndian(x);
}

int main() {
    Test_Endian();
    Test_BSwap();



    return 0;
}