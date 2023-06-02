#include "zero/address.h"
#include "zero/log.h"
#include "zero/myendian.h"
#include <arpa/inet.h>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <sstream>

static zero::Logger::ptr g_logger = ZERO_LOG_ROOT();

/// 关于address模块的一些解惑
/// 1.为什么使用getaddrinfo 而非 gethostbyname 前者支持ipv6
/// 2.sockaddr为什么可以强转为sockaddr_in6类型？只要知道首地址，以及该结构的大小，便可以转化，得到整个结构体内容
/// 3.存储在sockaddr结构体中的成员是网络字节序，所以保存时需要转化为本机的字节序，即大小端的问题
/// 4.inet_pton适用于ipv4和ipv6
namespace test_address {

class A {
public:
    virtual ~A() {
        ZERO_LOG_INFO(g_logger) << " ~A()";
    };
    virtual void test() = 0;
    void Create(int len) {
        ZERO_LOG_INFO(g_logger) << "A::Create()";
    }
};

class B : public A {
public:
    /// 最顶层析构即可
    ~B() {
        ZERO_LOG_INFO(g_logger) << " ~B()";
    }
    virtual void test1() = 0;
    void Create() {
        A::Create(1);
        ZERO_LOG_INFO(g_logger) << "B::Create()";
        test1();
    }
};

class C : public B {
public:
    ~C() {
        ZERO_LOG_INFO(g_logger) << " ~C()";
    }
    void test() {}
    void test1() {
        ZERO_LOG_INFO(g_logger) << "C test...";
    }
};

template <class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

template <class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for (; value; ++result) {
        value &= value - 1;
    }
    return result;
}

void Test_CreateMask() {
    ZERO_LOG_INFO(g_logger) << std::bitset<32>(CreateMask<uint32_t>(24));
    ZERO_LOG_INFO(g_logger) << CountBytes<uint32_t>(CreateMask<uint32_t>(24));
    ZERO_LOG_INFO(g_logger) << std::bitset<32>(~CreateMask<uint32_t>(24));

    ZERO_LOG_INFO(g_logger) << std::bitset<32>(CreateMask<uint32_t>(16));
    ZERO_LOG_INFO(g_logger) << CountBytes<uint32_t>(CreateMask<uint32_t>(16));
    ZERO_LOG_INFO(g_logger) << std::bitset<32>(~CreateMask<uint32_t>(16));

    C c;
    c.Create();
}

}  // namespace test_address

void Test_Net_Func() {
    ZERO_LOG_INFO(g_logger) << sizeof(sockaddr_in);
    ZERO_LOG_INFO(g_logger) << sizeof(sockaddr_in6);
    ZERO_LOG_INFO(g_logger) << sizeof(sockaddr);

    sockaddr_in m_addr;
    /// 小转大
    /// 1100 0000 1010 1000 0000 0000 0000 0001|3232235521
    inet_pton(AF_INET, "192.168.0.1", &m_addr.sin_addr.s_addr);
    /// 大转小
    auto address = zero::byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    ZERO_LOG_INFO(g_logger) << address;
    std::stringstream ss;
    ss << ((address >> 24) & 0xff) << "."
       << ((address >> 16) & 0xff) << "."
       << ((address >> 8) & 0xff) << "."
       << (address & 0xff);
    ZERO_LOG_INFO(g_logger) << ss.str();
    ZERO_LOG_INFO(g_logger) << std::bitset<32>(3232235521);
    ZERO_LOG_INFO(g_logger) << std::bitset<32>(test_address::CreateMask<uint32_t>(24));

}

/// sylar::IPv4Address::networdAddress()函数写法错误，忘记取反操作
void Test_IPAddress_Func() {
    sockaddr_in m_addr;
    inet_pton(AF_INET, "192.168.0.1", &m_addr.sin_addr.s_addr);
    /// 大转小 
    auto address = zero::byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    ZERO_LOG_INFO(g_logger) << address;
    /// 本机字节序二进制表示
    ZERO_LOG_INFO(g_logger) << std::bitset<32>(address);

    zero::IPv4Address::ptr ipv4(new zero::IPv4Address(address));

    /// 网络部分的IP已经转成了大端
    auto netword = ipv4->networdAddress(24);
    auto addr = netword->getAddr();
    /// 网络字节序二进制表示
    ZERO_LOG_INFO(g_logger) << std::bitset<32>(((sockaddr_in *)addr)->sin_addr.s_addr);
}

void Test_IPv6_Out() {

    sockaddr_in6 m_addr;
    m_addr.sin6_port = zero::byteswapOnLittleEndian((uint16_t)8080);
    inet_pton(AF_INET6, "FEDC:BA98:7654:0:0:BA98:7654:0", &m_addr.sin6_addr.s6_addr);
    /// 大转小 
    /// auto address = zero::byteswapOnLittleEndian(m_addr.sin6_addr.s6_addr);

    std::stringstream os;
    os << "[";
    /// ipv6每16位为一组
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for(size_t i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int)zero::byteswapOnLittleEndian(addr[i]) << std::dec;
    }
    if(!used_zeros &&  addr[7] == 0) {
        os << "::";
    }
    os << "]:" << zero::byteswapOnLittleEndian(m_addr.sin6_port);
    ZERO_LOG_INFO(g_logger) << os.str();

    ZERO_LOG_INFO(g_logger) << std::bitset<32>(test_address::CreateMask<uint8_t>(63 % 8));
}

void Test_Look_Up() {
    std::string host = "[fedc:ba98:7654::ba98:7654:0]:8080";
    std::string node;
    const char* service = NULL;

    /// 检查ipv6address service
    if(!host.empty() && host[0] == '[') {
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6) {
            /// TODO: check out of range
            if(*(endipv6 + 1) == ':') {
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    /// 检查node service
    if(node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        } 
    }

    if(node.empty()) {
        node = host;
    }
    ZERO_LOG_INFO(g_logger) << service;
    ZERO_LOG_INFO(g_logger) << node;

}

void Test_Look_Up1(const char *host) {
    std::vector<zero::Address::ptr> res;
    zero::Address::Lookup(res, host);
    for(auto& i : res) {
        ZERO_LOG_INFO(g_logger) << i->toString();
    }

}

int main() {
    // test_address::Test_CreateMask();
    // Test_Net_Func();
    // Test_IPAddress_Func();
    // Test_IPv6_Out();
    // Test_Look_Up();
    Test_Look_Up1("www.baidu.com");

    Test_Look_Up1("www.baidu.com:http");

    // Test_Look_Up1("www.google.com");

    // Test_Look_Up1("qq.com");

    // Test_Look_Up1("ipv6.baidu.com");

    int flag = 0;
    if(flag) {
        std::cout << flag << std::endl;
    }


    return 0;
}