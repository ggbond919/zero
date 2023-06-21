#ifndef __ZERO_TCP_SERVER_H__
#define __ZERO_TCP_SERVER_H__

#include <cstdint>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include "address.h"
#include "iomanager.h"
#include "socket.h"
#include "noncopyable.h"
#include "config.h"

namespace zero {

class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;
    
    TcpServer(zero::IOManager* worker = zero::IOManager::GetThis(), zero::IOManager* io_woker = zero::IOManager::GetThis(), zero::IOManager* accept_worker = zero::IOManager::GetThis());

    virtual ~TcpServer();

    virtual bool bind(zero::Address::ptr addr, bool ssl = false);

    virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails, bool ssl = false);

    // bool loadCertificates(const std::string& cert_file, const std::string& key_file);

    virtual bool start();

    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout; }

    std::string getName() const { return m_name; }

    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }

    virtual void setName(const std::string& v) { m_name = v; }

    bool isStop() const { return m_isStop; }

    virtual std::string toString(const std::string& prefix = "");

    std::vector<Socket::ptr> getSocks() const { return m_socks; }

protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);

protected:
    std::vector<Socket::ptr> m_socks;
    IOManager* m_worker;
    IOManager* m_ioWorker;
    IOManager* m_acceptWorker;
    uint64_t m_recvTimeout;
    std::string m_name;
    std::string m_type = "tcp";
    bool m_isStop;
    bool m_ssl = false;

};

}

#endif