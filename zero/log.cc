#include "log.h"
#include "config.h"
#include "zero/mutex.h"
#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <bits/types/time_t.h>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>

namespace zero {

const char* LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name)         \
    case LogLevel::name: \
        return #name;
        XX(FATAL);
        XX(ALERT);
        XX(CRIT);
        XX(ERROR);
        XX(WARN);
        XX(NOTICE);
        XX(INFO);
        XX(DEBUG);
#undef XX
    default:
        return "NOTSET";
    }
    return "NOTSET";
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v)            \
    if (str == #v) {            \
        return LogLevel::level; \
    }
    XX(FATAL, fatal);
    XX(ALERT, alert);
    XX(CRIT, crit);
    XX(ERROR, error);
    XX(WARN, warn);
    XX(NOTICE, notice);
    XX(INFO, info);
    XX(DEBUG, debug);

    XX(FATAL, FATAL);
    XX(ALERT, ALERT);
    XX(CRIT, CRIT);
    XX(ERROR, ERROR);
    XX(WARN, WARN);
    XX(NOTICE, NOTICE);
    XX(INFO, INFO);
    XX(DEBUG, DEBUG);
#undef XX
    return LogLevel::NOTSET;
}

LogEvent::LogEvent(const std::string& logger_name, LogLevel::Level level, const char* file, int32_t line, int64_t elapse,
                   uint32_t thread_id, uint64_t fiber_id, time_t time, const std::string& thread_name)
    : m_level(level), m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time),
      m_threadName(thread_name), m_loggerName(logger_name) {}

/**
 * @brief 重写printf函数，自定义printf格式来进行输出
 * 入栈顺序从右向左，出栈时通过fmt确定参数类型及个数
 * @param fmt
 * @param ...
 */
void LogEvent::printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void LogEvent::vprintf(const char* fmt, va_list ap) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, ap);
    if (len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    // 常引用既不可以修改指向，也不可以修改内容
    MessageFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << LogLevel::ToString(event->getLevel());
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << event->getElapse();
    }
};

class LoggerNameFormatItem : public LogFormatter::FormatItem {
public:
    LoggerNameFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << event->getLoggerName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << event->getThreadName();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S") : m_format(format) {
        if (m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::ostream& os, LogEvent::ptr event) {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        std::strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }

private:
    std::string m_format;
};

class FileNameFormatItem : public LogFormatter::FormatItem {
public:
    FileNameFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str) : m_string(str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << m_string;
    }

private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << "\t";
    }
};

class PercentSignFormatItem : public LogFormatter::FormatItem {
public:
    PercentSignFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) {
        os << "%";
    }
};

LogFormatter::LogFormatter(const std::string& pattern) : m_pattern(pattern) {
    init();
}

void LogFormatter::init() {
    // 每个pattern包括一个整数和一个字符串，0表示常规字符串，1表示需要转义
    std::vector<std::pair<int, std::string>> patterns;
    // 临时存储常规字符串
    std::string tmp;
    // 日期格式字符串，默认把位于%d后面的大括号对里的全部字符都当作格式字符，不校验格式是否合法
    std::string dateformat;
    // 错误标识
    bool error = false;
    // 是否正在解析常规字符，初始化为true
    bool parsing_string = true;

    size_t i = 0;
    while (i < m_pattern.size()) {
        // std::string(size_t n,char c) 该构造函数创建n个字符为c的字符串
        std::string c = std::string(1, m_pattern[i]);
        // 判断当前%是在常规字符串，亦或是模板字符串
        if (c == "%") {
            if (parsing_string) {
                if (!tmp.empty()) {
                    patterns.push_back(std::make_pair(0, tmp));
                }
                tmp.clear();
                // 若在常规字符串中遇到%说明接下来要开始匹配模板字符串
                parsing_string = false;
                i++;
                continue;
            } else {
                // 若在匹配模板字符串的过程中遇到了%，说明该%为转义字符
                patterns.push_back(std::make_pair(1, c));
                // 接下来是常规字符
                parsing_string = true;
                i++;
                continue;
            }
        } else {
            if (parsing_string) {
                tmp += c;
                i++;
                continue;
            } else {
                patterns.push_back(std::make_pair(1, c));
                parsing_string = true;
                // 后面是对%d的特殊处理，如果%d后面直接跟了一对大括号，那么把大括号里面的内容提取出来作为dateformat
                // 且对于日期格式，d后面应该有{%Y-%m-%d %H:%M:%S}，否则dataItem构造会默认一个
                if (c != "d") {
                    i++;
                    continue;
                }
                i++;
                if (i < m_pattern.size() && m_pattern[i] != '{') {
                    continue;
                }
                i++;
                while (i < m_pattern.size() && m_pattern[i] != '}') {
                    dateformat.push_back(m_pattern[i]);
                    i++;
                }
                // 越界的风险
                if (m_pattern.size() == i || m_pattern[i] != '}') {
                    std::cout << "[ERROR] LogFormatter::init() "
                              << "pattern: [" << m_pattern << "] '{' not closed" << std::endl;
                    error = true;
                    break;
                }
                i++;
                continue;
            }
        }
    }

    if (error) {
        m_error = true;
        return;
    }

    if (!tmp.empty()) {
        patterns.push_back(std::make_pair(0, tmp));
        tmp.clear();
    }

    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
#define XX(str, C)                                                               \
    {                                                                            \
#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt)); } \
    }
        XX(m, MessageFormatItem),    XX(p, LevelFormatItem),       XX(c, LoggerNameFormatItem), XX(r, ElapseFormatItem),
        XX(f, FileNameFormatItem),   XX(l, LineFormatItem),        XX(t, ThreadIdFormatItem),   XX(F, FiberIdFormatItem),
        XX(N, ThreadNameFormatItem), XX(%, PercentSignFormatItem), XX(T, TabFormatItem),        XX(n, NewLineFormatItem),
#undef XX
    };

    // 根据patter将所有FormatItem添加到vector中
    for (auto& v : patterns) {
        if (v.first == 0) {
            // 正常字符 如 "-" " "
            m_items.push_back(FormatItem::ptr(new StringFormatItem(v.second)));
        } else if (v.second == "d") {
            // date
            m_items.push_back(FormatItem::ptr(new DateTimeFormatItem(dateformat)));
        } else {
            // 匹配格式
            // find(key)
            // key:模式 value:对应的ptr
            auto it = s_format_items.find(v.second);
            if (it == s_format_items.end()) {
                std::cout << "[ERROR] LogFormatter::init() "
                          << "pattern: [" << m_pattern << "] "
                          << "unknown format item: " << v.second << std::endl;
                error = true;
                break;
            } else {
                // 这里可以看tests/test_mapfunc.cc
                m_items.push_back(it->second(v.second));
            }
        }
    }

    if (error) {
        m_error = true;
        return;
    }
}

/**
 * @brief 输出各种Item信息
 * ? 返回值作用？
 * @param event
 * @return std::string
 */
std::string LogFormatter::format(LogEvent::ptr event) {
    std::stringstream ss;
    for (auto& i : m_items) {
        i->format(ss, event);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& os, LogEvent::ptr event) {
    for (auto& i : m_items) {
        i->format(os, event);
    }
    return os;
}

LogAppender::LogAppender(LogFormatter::ptr default_formatter) : m_defaultFormatter(default_formatter) {}

void LogAppender::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
}

LogFormatter::ptr LogAppender::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter ? m_formatter : m_defaultFormatter;
}

/**
 * @brief 父类构造函数带参数，子类需要显示调用父类带参构造函数
 *        见tests/test_constructor.cc
 */
StdoutLogAppender::StdoutLogAppender() : LogAppender(LogFormatter::ptr(new LogFormatter)) {}

void StdoutLogAppender::log(LogEvent::ptr event) {
    if (m_formatter) {
        m_formatter->format(std::cout, event);
    } else {
        m_defaultFormatter->format(std::cout, event);
    }
}

std::string StdoutLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    node["pattern"] = m_formatter->getPattern();
    std::stringstream ss;
    ss << node;
    return ss.str();
}

FileLogAppender::FileLogAppender(const std::string& file) : LogAppender(LogFormatter::ptr(new LogFormatter)) {
    m_filename = file;
    reopen();
    if (m_reopenError) {
        std::cout << "reopen file " << m_filename << " error" << std::endl;
    }
}

void FileLogAppender::log(LogEvent::ptr event) {
    uint64_t now = event->getTime();
    if (now >= (m_lastTime + 3)) {
        reopen();
        if (m_reopenError) {
            std::cout << "reopen file " << m_filename << " error" << std::endl;
        }
        m_lastTime = now;
    }
    if (m_reopenError) {
        return;
    }
    
    MutexType::Lock lock(m_mutex);

    if (m_formatter) {
        if (!m_formatter->format(m_filestream, event)) {
            std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
        }
    } else {
        if (!m_defaultFormatter->format(m_filestream, event)) {
            std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
        }
    }
}

bool FileLogAppender::reopen() {
    MutexType::Lock lock(m_mutex);
    if (m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename, std::ios::app);
    m_reopenError = !m_filestream;
    return !m_reopenError;
}

std::string FileLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    node["pattern"] = m_formatter ? m_formatter->getPattern() : m_defaultFormatter->getPattern();
    std::stringstream ss;
    ss << node;
    return ss.str();
}

Logger::Logger(const std::string& name)
    : m_name(name), m_level(LogLevel::INFO)
      /// TODO: GetElapsedMS()
      ,
      m_createTime(0) {}

void Logger::addAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    for (auto it = m_appenders.begin(); it != m_appenders.end(); it++) {
        if (*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

void Logger::log(LogEvent::ptr event) {
    if (event->getLevel() <= m_level) {
        for (auto& i : m_appenders) {
            i->log(event);
        }
    }
}

/// TODO: yaml
std::string Logger::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    node["level"] = LogLevel::ToString(m_level);
    for (auto& i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogEventWrap::LogEventWrap(Logger::ptr logger, LogEvent::ptr event) : m_logger(logger), m_event(event) {}

LogEventWrap::~LogEventWrap() {
    m_logger->log(m_event);
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger("root"));
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers[m_root->getName()] = m_root;
    init();
}

/**
 * 如果指定名称的日志器未找到，那会就新创建一个，但是新创建的Logger是不带Appender的，
 * 需要手动添加Appender
 */
Logger::ptr LoggerManager::getLogger(const std::string& name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) {
        return it->second;
    }
    Logger::ptr logger(new Logger(name));

    // 增加一下appender
    logger->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers[name] = logger;
    return logger;
}

/// TODO:
void LoggerManager::init() {}

std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for (auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

/**
 * @brief 日志输出地配置相关定义
 * 
 */
struct LogAppenderDefine {
    // 1 File 2 Stdout
    int type = 0;
    std::string pattern;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type && pattern == oth.pattern && file == oth.file;
    }
};

/**
 * @brief 自定义日志器配置
 * 
 */
struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::NOTSET;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name && level == oth.level && appenders == oth.appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }

    bool isValid() const {
        return !name.empty();
    }
};

/**
 * @brief 类型转换偏特化，具体见config.h
 * 
 * @tparam  
 */
template <>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string& v) {
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
        // 日志器必须要有名称
        if (!n["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << n << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = n["name"].as<std::string>();
        ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");

        if (n["appenders"].IsDefined()) {
            for (size_t i = 0; i < n["appenders"].size(); i++) {
                auto a = n["appenders"][i];
                // 没有type则忽略该appender的配置
                if (!a["type"].IsDefined()) {
                    std::cout << "log appender config error: appender type is null, " << a << std::endl;
                    continue;
                }

                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if (type == "FileLogAppender") {
                    lad.type = 1;
                    if (!a["file"].IsDefined()) {
                        std::cout << "log appender config error: file appender file is null, " << a << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if (a["pattern"].IsDefined()) {
                        lad.pattern = a["pattern"].as<std::string>();
                    }
                } else if (type == "StdoutLogAppender") {
                    lad.type = 2;
                    if (a["pattern"].IsDefined()) {
                        lad.pattern = a["pattern"].as<std::string>();
                    }
                } else {
                    std::cout << "log appender config error: appender type is invalid, " << a << std::endl;
                    continue;
                }
                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template <>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine& i) {
        YAML::Node n;
        n["name"] = i.name;
        n["level"] = LogLevel::ToString(i.level);
        for (auto& a : i.appenders) {
            YAML::Node node;
            if (a.type == 1) {
                node["type"] = "FileLogAppender";
                node["file"] = a.file;
            } else if (a.type == 2) {
                node["type"] = "StdoutLogAppender";
            }

            if (!a.pattern.empty()) {
                node["pattern"] = a.pattern;
            }
            n["appenders"].push_back(node);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

/// main函数执行之前开始读取配置
zero::ConfigVar<std::set<LogDefine>>::ptr g_log_defines = zero::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value) {
            ZERO_LOG_INFO(ZERO_LOG_ROOT()) << "on log config changed";
            for (auto& i : new_value) {
                auto it = old_value.find(i);
                zero::Logger::ptr logger;
                if (it == old_value.end()) {
                    // 新增logger的情况
                    logger = ZERO_LOG_NAME(i.name);
                } else {
                    if (!(i == *it)) {
                        // 修改logger的情况
                        logger = ZERO_LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                logger->clearAppenders();
                for (auto& a : i.appenders) {
                    zero::LogAppender::ptr ap;
                    if (a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if (a.type == 2) {
                        /// TODO:EnvMgr
                        ap.reset(new StdoutLogAppender);
                    }

                    // 决定是否使用默认模板输出格式
                    if (!a.pattern.empty()) {
                        ap->setFormatter(LogFormatter::ptr(new LogFormatter(a.pattern)));
                    } else {
                        ap->setFormatter(LogFormatter::ptr(new LogFormatter));
                    }
                    logger->addAppender(ap);
                }
            }

            // 以配置文件为主，如果程序里定义了配置文件中未定义的logger，那么把程序里定义的logger设置成无效
            for (auto& i : old_value) {
                auto it = new_value.find(i);
                if (it == new_value.end()) {
                    auto logger = ZERO_LOG_NAME(i.name);
                    logger->setLevel(LogLevel::NOTSET);
                    logger->clearAppenders();
                }
            }
        });
    }
};

// main函数之前注册回调
static LogIniter __log_init;

}  // namespace zero
