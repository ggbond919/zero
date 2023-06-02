#include "zero/address.h"
#include "zero/hook.h"
#include "zero/myendian.h"
#include "zero/socket.h"
#include "zero/iomanager.h"
#include "zero/log.h"
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

/// 简单编写出客户端和服务端代码
/// 可以用nc/telnet命令分别连接或监听，查看相关信息

static zero::Logger::ptr g_logger = ZERO_LOG_ROOT();

void Test_Simple_Server() {
    auto sock = zero::Socket::CreateTCPSocket();

    // sockaddr_in sa;
    // inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr.s_addr);
    // sa.sin_family = AF_INET;
    // sa.sin_port = zero::byteswapOnLittleEndian((uint16_t)6666);
    // socklen_t size = sizeof(sa);
    // zero::Address::ptr address = zero::Address::Create((const sockaddr*)&sa, size);

    zero::IPv4Address::ptr serv_address = zero::IPv4Address::Create("192.168.0.228",8666);
    // serv_address->setPort(6666);
    auto address = std::dynamic_pointer_cast<zero::Address>(serv_address);
    if(!address) {
        ZERO_LOG_ERROR(g_logger) << "address nullptr";
        return;
    }

    sock->bind(address);
    sock->listen();
    auto client_sock = sock->accept();
    if(!client_sock) {
        ZERO_LOG_ERROR(g_logger) << "client_sock nullptr";
    }

    auto cleint_addres = sock->getRemoteAddress();
    auto local_address = sock->getLocalAddress();

    ZERO_LOG_INFO(g_logger) << "local_address info " << local_address->toString();
    ZERO_LOG_INFO(g_logger) << "remote_address info " << cleint_addres->toString();
    ZERO_LOG_INFO(g_logger) << "client info " << client_sock->getLocalAddress()->toString();

    /// 注意协程栈的影响
    char buf[64];
    
    while(1) {
        if(client_sock->recv(buf,sizeof(buf)) > 0) {
            ZERO_LOG_INFO(g_logger) << buf;
            std::string str;
            std::cin >> str;
            client_sock->send(str.data(),str.size());
            memset(buf, 0, 64);
        }
    }

    ZERO_LOG_INFO(g_logger) << "client exit";
}

void Test_Simple_Client() {
    auto sock = zero::Socket::CreateTCPSocket();
    zero::IPv4Address::ptr serv_address = zero::IPv4Address::Create("192.168.0.228",8666);
    // serv_address->setPort(6666);
    auto address = std::dynamic_pointer_cast<zero::Address>(serv_address);
    if(!address) {
        ZERO_LOG_ERROR(g_logger) << "address nullptr";
        return;
    }
    sleep(2);
    sock->connect(address);
    /// 注意协程栈的影响
    char buf[64];
    
    while(1) {
        std::string str;
        std::cin >> str;
        if(sock->send(str.data(),str.size()) > 0) {
            sock->recv(buf,sizeof(buf));
            ZERO_LOG_INFO(g_logger) << buf;
           
            
            memset(buf, 0, 64);
        }
    }
}



int main() {

    zero::IOManager iom;
    iom.schedule(&Test_Simple_Server);
    // iom.schedule(&Test_Simple_Client);

    return 0;
}