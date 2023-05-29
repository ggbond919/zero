#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include <cstdint>
#include <memory>
#include <vector>
#include "thread.h"
#include "singleton.h"
#include "zero/mutex.h"

namespace zero {

/**
 * @brief 文件句柄上下文，验证文件句柄信息
 * 
 */
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;

    /**
     * @brief 通过文件句柄构造FdCtx
     * 
     * @param fd 
     */
    FdCtx(int fd);

    ~FdCtx();

    /**
     * @brief 是否初始化完成
     * 
     * @return true 
     * @return false 
     */
    bool isInit() const { return m_isInit; }

    /**
     * @brief 是否是socket句柄
     * 
     * @return true 
     * @return false 
     */
    bool isSocket() const { return m_isSocket; }

    /**
     * @brief 是否已关闭
     * 
     * @return true 
     * @return false 
     */
    bool isClose() const { return m_isClosed; }

    /**
     * @brief 设置用户主动设置非阻塞
     * 
     * @param v 
     */
    void setUserNonblock(bool v) { m_userNonblock = v; }

    /**
     * @brief 获取是否用户主动设置的非阻塞
     * 
     * @return true 
     * @return false 
     */
    bool getUserNonblock() const { return m_userNonblock; }

    /**
     * @brief 设置系统非阻塞
     * 
     * @param v 
     */
    void setSysNonblock(bool v) { m_sysNonblock = v; }

    /**
     * @brief 获取系统非阻塞
     * 
     * @return true 
     * @return false 
     */
    bool getSysNonblock() const { return m_sysNonblock; }

    /**
     * @brief 设置超时时间
     * 
     * @param type SO_RCVTIMEO(读超时) SO_SNDTIMEO(写超时)
     * @param v 时间 毫秒
     */
    void setTimeout(int type, uint64_t v);

    /**
     * @brief 获取超时时间
     * 
     * @param type SO_RCVTIMEO(读超时) SO_SNDTIMEO(写超时)
     * @return uint64_t 
     */
    uint64_t getTimeout(int type);

private:
    /**
     * @brief 初始化
     * 
     * @return true 
     * @return false 
     */
    bool init();

private:
    /// 是否初始化
    /// 指定位数
    bool m_isInit: 1;
    /// 是否为socket
    bool m_isSocket: 1;
    /// 是否hook非阻塞
    bool m_sysNonblock: 1;
    /// s是否用户主动设置非阻塞模式
    bool m_userNonblock: 1;
    /// 是否关闭
    bool m_isClosed: 1;
    /// 文件句柄
    int m_fd;
    /// 读超时时间 毫秒
    uint64_t m_recvTimeout;
    /// 写超时时间 毫秒
    uint64_t m_sendTimeout;
};

/**
 * @brief 文件句柄管理类
 * 
 */
class FdManager {
public:
    typedef RWMutex RWMutexType;

    FdManager();

    /**
     * @brief 获取/创建文件句柄类FdCtx
     * 
     * @param fd 
     * @param auto_create 是否自动创建
     * @return FdCtx::ptr 
     */
    FdCtx::ptr get(int fd, bool auto_create = false);

    /**
     * @brief 删除文件句柄类
     * 
     * @param fd 
     */
    void del(int fd);

private:
    /// 读写锁
    RWMutexType m_mutex;
    /// 文件句柄集合
    std::vector<FdCtx::ptr> m_datas;
};

/// 文件句柄单例
typedef Singleton<FdManager> FdMgr;


}


#endif