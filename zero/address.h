#ifndef __ZERO_ADDRESS_H__
#define __ZERO_ADDRESS_H__

#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <utility>
#include <vector>
#include <map>

namespace zero {

class IPAddress;

class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    /**
     * @brief 通过sockaddr指针 创建Address
     * 
     * @param addr sockaddr指针
     * @param addrlen sockaddr的长度
     * @return Address::ptr 
     */
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

    /**
     * @brief 通过host地址返回对应条件的所有Address
     * 
     * @param result 保存满足条件的Address
     * @param host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
     * @param family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @param type socketl类型SOCK_STREAM、SOCK_DGRAM 等
     * @param protocal 协议,IPPROTO_TCP、IPPROTO_UDP 等
     * @return true 
     * @return false 
     */
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host, int family = AF_INET, int type = 0, int protocal = 0);

    /**
     * @brief 通过host地址返回对应条件的任意Address
     * 
     * @param host 域名，服务器名等，举例: www.sylar.top[:80] (方括号为可选内容)
     * @param family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @param type socketl类型SOCK_STREAM、SOCK_DGRAM 等
     * @param protocal 协议,IPPROTO_TCP、IPPROTO_UDP 等
     * @return Address::ptr 
     */
    static Address::ptr LookupAny(const std::string& host, int family = AF_INET, int type = 0, int protocal = 0);

    /**
     * @brief 通过host地址返回对应条件的任意IPAddress
     * 
     * @param host 域名，服务器名等，举例: www.sylar.top[:80] (方括号为可选内容)
     * @param family 协议簇
     * @param type socket类型SOCK_STREAM ,SOCK_DGRAM等
     * @param protocal 协议，IPPROTO_TCP,IPPROTO_UDP等
     * @return std::shared_ptr<IPAddress> 
     */
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host, int family = AF_INET, int type = 0, int protocal = 0);

    /**
     * @brief 返回本机的所有网卡的<网卡名，地址，子网掩码位数>
     * 
     * @param result 保存本机所有地址
     * @param family 协议簇(AF_INT, AF_INT6, AF_UNIX)
     * @return true 
     * @return false 
     */
    static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result, int family = AF_INET);

    /**
     * @brief 获取指定网卡的地址和子网掩码位数
     * 
     * @param result 保存指定网卡的所有地址
     * @param iface 网卡名称
     * @param family 协议簇(AF_INT, AF_INT6, AF_UNIX)
     * @return true 
     * @return false 
     */
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result, const std::string& iface, int family = AF_INET);

    virtual ~Address() {};

    /**
     * @brief 返回协议簇
     * 
     * @return int 
     */
    int getFamily() const;

    /**
     * @brief 返回sockaddr指针，只读
     * 
     * @return const sockaddr* 
     */
    virtual const sockaddr* getAddr() const = 0;

    /**
     * @brief 返回sockaddr指针 可读写
     * 
     * @return sockaddr* 
     */
    virtual sockaddr* getAddr() = 0;

    /**
     * @brief 返回sockaddr的长度
     * 
     * @return socklen_t 
     */
    virtual socklen_t getAddrLen() const = 0;

    /**
     * @brief 可读性输出地址
     * 
     * @param os 
     * @return std::ostream& 
     */
    virtual std::ostream& insert(std::ostream& os) const = 0;

    /**
     * @brief 返回可读性字符串
     * 
     * @return std::string 
     */
    std::string toString() const;

    bool operator<(const Address& rhs) const;

    bool operator==(const Address& rhs) const;

    bool operator!=(const Address& rhs) const;
};

/**
 * @brief IP地址的基类
 * 
 */
class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    /**
     * @brief 通过域名，IP,服务器名创建IPAddress
     * 
     * @param address 域名，IP,服务器名等，
     * @param port 端口号
     * @return IPAddress::ptr 
     */
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    /**
     * @brief 获取该地址的广播地址
     * 
     * @param prefix_len 子网掩码位数
     * @return IPAddress::ptr 
     */
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

    /**
     * @brief 获取该地址的网段
     * 
     * @param prefix_len 子网掩码位数
     * @return IPAddress::ptr 
     */
    virtual IPAddress::ptr networdAddress(uint32_t  prefix_len) = 0;

    /**
     * @brief 获取子网掩码地址
     * 
     * @param prefix_len 子网掩码位数
     * @return IPAddress::ptr 
     */
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    /**
     * @brief 返回端口号
     * 
     * @return uint32_t 
     */
    virtual uint32_t getPort() const = 0;

    /**
     * @brief 设置端口号
     * 
     * @param v 
     */
    virtual void setPort(uint16_t v) = 0;
};

/**
 * @brief IPv4
 * 
 */
class IPv4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    /**
     * @brief 使用点分十进制地址创建IPv4Address
     * 
     * @param address 点分十进制
     * @param prot 端口号
     * @return IPv4Address::ptr 
     */
    static IPv4Address::ptr Create(const char* address, uint16_t prot = 0);

    /**
     * @brief 通过sockaddr_in 构造IPv4Address
     * 
     * @param address 
     */
    IPv4Address(const sockaddr_in& address);

    /**
     * @brief 通过二进制地址构造IPv4Address
     * 
     * @param address 二进制地址
     * @param port 端口号
     */
    IPv4Address(uint32_t address = INADDR_ANY,  uint16_t port = 0);

    const sockaddr* getAddr() const override;

    sockaddr* getAddr() override;

    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

    /**
     * @brief ERROR:sylar实现有问题，忘记了对掩码的取反操作
     * 
     * @param prefix_len 
     * @return IPAddress::ptr 
     */
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;

    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;

    void setPort(uint16_t v) override;

private:
    sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;

    /**
     * @brief 通过IPv6地址字符串构造IPv6Address
     * 
     * @param address IPV6字符串
     * @param port 
     * @return IPv6Address::ptr 
     */
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    IPv6Address();

    /**
     * @brief 通过sockaddr_in6构造IPv6Address
     * 
     * @param address 
     * @param port 
     */
    IPv6Address(const sockaddr_in6& address);

    /**
     * @brief 通过IPv6二进制地址构造IPv6Address
     * 
     * @param address IPv6二进制
     * @param port 
     */
    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    const sockaddr* getAddr() const override;

    sockaddr* getAddr() override;

    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

    IPAddress::ptr networdAddress(uint32_t prefix_len) override;

    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;

    void setPort(uint16_t v) override;
private:
    sockaddr_in6 m_addr;
};

/**
 * @brief UnixSocket地址
 * 
 */
class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;

    UnixAddress();

    /**
     * @brief 通过路径构造UnixAddress
     * 
     * @param path 长度小于UNIX_PATH_MAX
     */
    UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;

    sockaddr* getAddr() override;

    socklen_t getAddrLen() const override;

    void setAddrLen(uint32_t v);

    std::string getPath() const;

    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

/**
 * @brief 未知地址
 * 
 */
class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr m_addr;
};

/**
 * @brief 流式输出Address
 * 
 * @param os 
 * @param addr 
 * @return std::ostream& 
 */
std::ostream& operator<<(std::ostream& os, const Address& addr);

}





#endif