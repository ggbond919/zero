#ifndef __ZERO_LOG_H__
#define __ZERO_LOG_H__

#include "singleton.h"
#include "util.h"
#include "mutex.h"
#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <bits/types/time_t.h>
#include <cstdarg>  // 可变参数需包含
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

// TODO
// util类，协程
/**
 * @brief
 * 获取root日志器
 *
 */
#define ZERO_LOG_ROOT() zero::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief
 * 获取指定名称的日志器
 *
 */
#define ZERO_LOG_NAME(name) zero::LoggerMgr::GetInstance()->getLogger(name)

/**
 * @brief
 * 构造一个LogeEventWrap对象，包含日志器和日志事件，在该对象析构时调用log()函数进行日志的输出
 *
 */
#define ZERO_LOG_LEVEL(logger, level)                                                                                                 \
    if (level <= logger->getLevel())                                                                                                  \
    zero::LogEventWrap(logger, zero::LogEvent::ptr(new zero::LogEvent(                                                                \
                                   logger->getName(), level, __FILE__, __LINE__, zero::GetElapsedMS() - logger->getCreateTime(),      \
                                   zero::GetThreadId(), zero::GetFiberId(), time(0), zero::GetThreadName())))                         \
        .getLogEvent()                                                                                                                \
        ->getSS()

#define ZERO_LOG_FATAL(logger) ZERO_LOG_LEVEL(logger, zero::LogLevel::FATAL)

#define ZERO_LOG_ALERT(logger) ZERO_LOG_LEVEL(logger, zero::LogLevel::ALERT)

#define ZERO_LOG_CRIT(logger) ZERO_LOG_LEVEL(logger, zero::LogLevel::CRIT)

#define ZERO_LOG_ERROR(logger) ZERO_LOG_LEVEL(logger, zero::LogLevel::ERROR)

#define ZERO_LOG_WARN(logger) ZERO_LOG_LEVEL(logger, zero::LogLevel::WARN)

#define ZERO_LOG_NOTICE(logger) ZERO_LOG_LEVEL(logger, zero::LogLevel::NOTICE)

#define ZERO_LOG_INFO(logger) ZERO_LOG_LEVEL(logger, zero::LogLevel::INFO)

#define ZERO_LOG_DEBUG(logger) ZERO_LOG_LEVEL(logger, zero::LogLevel::DEBUG)

/**
 * @brief 使用C
 * printf方式，输出日志
 *
 */
#define ZERO_LOG_FMT_LEVEL(logger, level, fmt, ...)                                                                                   \
    if (level <= logger->getLevel())                                                                                                  \
    zero::LogEventWrap(logger, zero::LogEvent::ptr(new zero::LogEvent(                                                                \
                                   logger->getName(), level, __FILE__, __LINE__, zero::GetElapsedMS() - logger->getCreateTime(),      \
                                   zero::GetThreadId(), zero::GetFiberId(), time(0), zero::GetThreadName())))                         \
        .getLogEvent()                                                                                                                \
        ->printf(fmt, __VA_ARGS__)

#define ZERO_LOG_FMT_FATAL(logger, fmt, ...) ZERO_LOG_FMT_LEVEL(logger, zero::LogLevel::FATAL, fmt, __VA_ARGS__)

#define ZERO_LOG_FMT_ALERT(logger, fmt, ...) ZERO_LOG_FMT_LEVEL(logger, zero::LogLevel::ALERT, fmt, __VA_ARGS__)

#define ZERO_LOG_FMT_CRIT(logger, fmt, ...) ZERO_LOG_FMT_LEVEL(logger, zero::LogLevel::CRIT, fmt, __VA_ARGS__)

#define ZERO_LOG_FMT_ERROR(logger, fmt, ...) ZERO_LOG_FMT_LEVEL(logger, zero::LogLevel::ERROR, fmt, __VA_ARGS__)

#define ZERO_LOG_FMT_WARN(logger, fmt, ...) ZERO_LOG_FMT_LEVEL(logger, zero::LogLevel::WARN, fmt, __VA_ARGS__)

#define ZERO_LOG_FMT_NOTICE(logger, fmt, ...) ZERO_LOG_FMT_LEVEL(logger, zero::LogLevel::NOTICE, fmt, __VA_ARGS__)

#define ZERO_LOG_FMT_INFO(logger, fmt, ...) ZERO_LOG_FMT_LEVEL(logger, zero::LogLevel::INFO, fmt, __VA_ARGS__)

#define ZERO_LOG_FMT_DEBUG(logger, fmt, ...) ZERO_LOG_FMT_LEVEL(logger, zero::LogLevel::DEBUG, fmt, __VA_ARGS__)

namespace zero {

/**
 * @brief 日志级别
 */
class LogLevel {
public:
    /**
     * @brief
     * 日志级别枚举
     */
    enum Level {
        FATAL = 0,
        ALERT = 100,
        CRIT = 200,
        ERROR = 300,
        WARN = 400,
        NOTICE = 500,
        INFO = 600,
        DEBUG = 700,
        NOTSET = 800,
    };

    /**
     * @brief
     * 日志级别转字符串
     *
     * @param level
     * @return const
     * char*
     */
    static const char* ToString(LogLevel::Level level);

    /**
     * @brief
     * 字符串转日志级别
     *
     * @param str
     * @return
     * LogLevel::Level
     */
    static LogLevel::Level FromString(const std::string& str);
};

/**
 * @brief 日志事件
 */
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(const std::string& logger_name, LogLevel::Level level, const char* file, int32_t line, int64_t elapse, uint32_t thread_id,
             uint64_t fiber_id, time_t time, const std::string& thread_name);
    LogLevel::Level getLevel() const {
        return m_level;
    }
    std::string getContent() const {
        return m_ss.str();
    }
    std::string getFile() const {
        return m_file;
    }
    int32_t getLine() const {
        return m_line;
    }
    int64_t getElapse() const {
        return m_elapse;
    }
    uint32_t getThreadId() const {
        return m_threadId;
    }
    uint64_t getFiberId() const {
        return m_fiberId;
    }
    time_t getTime() const {
        return m_time;
    }
    const std::string& getThreadName() const {
        return m_threadName;
    }
    std::stringstream& getSS() {
        return m_ss;
    }
    const std::string& getLoggerName() const {
        return m_loggerName;
    }
    /**
     * @brief C
     * printf风格写入日志，可变参数需要包含cstdarg头文件
     *
     * @param fmt
     * @param ...
     */
    void printf(const char* fmt, ...);
    void vprintf(const char* fmt, va_list ap);

private:
    LogLevel::Level m_level;
    /// stringstream流式写入日志内容
    std::stringstream m_ss;
    const char* m_file = nullptr;
    int32_t m_line = 0;
    int64_t m_elapse = 0;
    uint32_t m_threadId = 0;
    uint64_t m_fiberId = 0;
    time_t m_time;
    std::string m_threadName;
    std::string m_loggerName;
};

/**
 * @brief 日志格式化
 */
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;

    /**
     * @brief      
     * - %%m 消息
     * - %%p 日志级别
     * - %%c 日志器名称
     * - %%d 日期时间，后面可跟一对括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这里的格式字符与C语言strftime一致
     * - %%r 该日志器创建后的累计运行毫秒数
     * - %%f 文件名
     * - %%l 行号
     * - %%t 线程id
     * - %%F 协程id
     * - %%N 线程名称
     * - %%% 百分号
     * - %%T 制表符
     * - %%n 换行
     * 
     * 默认格式：%%d{%%Y-%%m-%%d %%H:%%M:%%S}%%T%%t%%T%%N%%T%%F%%T[%%p]%%T[%%c]%%T%%f:%%l%%T%%m%%n
     * 
     * 默认格式描述：年-月-日 时:分:秒 [累计运行毫秒数] \\t 线程id \\t 线程名称 \\t 协程id \\t [日志级别] \\t [日志器名称] \\t 文件名:行号 \\t 日志消息 换行符
     * 
     */
    LogFormatter(const std::string& pattern = "%d{%Y-%m-%d %H:%M:%S} [%rms]%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");

    /**
     * @brief
     * 解析日志输出格式
     *
     */
    void init();

    /**
     * @brief
     * 模板解析格式判断
     *
     * @return true
     * @return false
     */
    bool isError() const {
        return m_error;
    }

    /**
     * @brief
     * 返回格式化后的日志文本
     *
     * @param event
     * @return
     * std::string
     */
    std::string format(LogEvent::ptr event);

    /**
     * @brief
     * 返回格式化后的日志流
     *
     * @param os
     * @param event
     * @return
     * std::ostream&
     */
    std::ostream& format(std::ostream& os, LogEvent::ptr event);
    std::string getPattern() const {
        return m_pattern;
    }

public:
    /**
     * @brief
     * 日志内容格式化，虚基类，可派生出指定类型的格式
     *
     */
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}
        virtual void format(std::ostream& os, LogEvent::ptr event) = 0;
    };

private:
    /// 日志格式模板
    std::string m_pattern;
    /// 解析后的格式模板数组
    std::vector<FormatItem::ptr> m_items;
    /// 是否有误
    bool m_error = false;
};

/**
 * @brief
 * 日志输出地，虚基类，可派生出不同日志输出方式
 *
 */
class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    /**
     * @brief
     * Construct a
     * new Log
     * Appender
     * object
     *
     * @param
     * default_formatter
     * 默认日志器
     */
    LogAppender(LogFormatter::ptr default_formatter);
    virtual ~LogAppender() {}
    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();

    /**
     * @brief
     * 写入日志
     *
     * @param event
     */
    virtual void log(LogEvent::ptr event) = 0;

    /**
     * @brief
     * 将日志输出目标的配置转成Yaml
     *
     * @return
     * std::string
     */
    virtual std::string toYamlString() = 0;

protected:
    /// 自旋锁
    MutexType m_mutex;
    /// 日志格式器
    LogFormatter::ptr m_formatter;
    /// 默认日志器
    LogFormatter::ptr m_defaultFormatter;
};

/**
 * @brief
 * 输出到控制台的Appender
 *
 */
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    StdoutLogAppender();
    void log(LogEvent::ptr event) override;
    std::string toYamlString() override;
};

/**
 * @brief 输出到文件
 *
 */
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    /**
     * @brief
     * Construct a
     * new File Log
     * Appender
     * object
     *
     * @param file
     * 日志文件路径
     */
    FileLogAppender(const std::string& file);
    void log(LogEvent::ptr event) override;

    /**
     * @brief
     * 重新打开日志文件
     *
     * @return true
     * @return false
     */
    bool reopen();
    std::string toYamlString() override;

private:
    /// 文件路径
    std::string m_filename;
    /// 文件流
    std::ofstream m_filestream;
    /// 上次重新打开时间
    uint64_t m_lastTime = 0;
    /// 文件打开错误标识
    bool m_reopenError = false;
};

/**
 * @brief 日志器类
 *
 */
class Logger {
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    /**
     * @brief
     * Construct a
     * new Logger
     * object
     *
     * @param name
     * 日志器名称
     */
    Logger(const std::string& name = "default");
    const std::string& getName() const {
        return m_name;
    }
    const uint64_t& getCreateTime() const {
        return m_createTime;
    }
    void setLevel(LogLevel::Level level) {
        m_level = level;
    }
    LogLevel::Level getLevel() const {
        return m_level;
    }
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();

    /**
     * @brief 写日志
     *
     * @param event
     */
    void log(LogEvent::ptr event);
    std::string toYamlString();

private:
    /// 自旋锁
    MutexType m_mutex;
    /// 日志器名称
    std::string m_name;
    /// 日志器等级
    /// 默认INFO，在构造函数里已经初始化
    LogLevel::Level m_level;
    /// Appender集合
    std::list<LogAppender::ptr> m_appenders;
    /// 创建时间（毫秒）
    uint64_t m_createTime;
};

/**
 * @brief
 * 日志事件包装器，方便宏定义，内部包含日志事件和日志器
 *
 */
class LogEventWrap {
public:
    LogEventWrap(Logger::ptr logger, LogEvent::ptr event);
    ~LogEventWrap();
    LogEvent::ptr getLogEvent() const {
        return m_event;
    }

private:
    /// 日志器
    Logger::ptr m_logger;
    /// 日志事件
    LogEvent::ptr m_event;
};

/**
 * @brief
 * 日志器管理类
 *
 */
class LoggerManager {
public:
    typedef Spinlock MutexType;
    LoggerManager();

    /**
     * @brief
     * 结合配置模块实现日志相关初始化
     *
     */
    void init();

    /**
     * @brief
     * 获得指定名称的日志器
     * @param name
     * @return
     * Logger::ptr
     */
    Logger::ptr getLogger(const std::string& name);

    /**
     * @brief
     * 获得root日志器，等同于getLogger("root")
     *
     * @return
     * Logger::ptr
     */
    Logger::ptr getRoot() {
        return m_root;
    }
    std::string toYamlString();

private:
    /// 自旋锁
    MutexType m_mutex;
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

/// 日志管理器单例
typedef zero::Singleton<LoggerManager> LoggerMgr;

}  // namespace zero

#endif