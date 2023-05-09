#include "../zero/log.h"
#include "zero/config.h"
#include <sstream>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>

// 方式一，通过该宏获得全局名为root的logger
// ZERO_LOG_ROOT使用loggerManager管理的日志器
// 默认级别为INFO,默认输出地为终端
zero::Logger::ptr g_logger = ZERO_LOG_ROOT();
zero::Logger::ptr test_system = ZERO_LOG_NAME("system");

int main() {

    // 输出级别大于等于INFO的日志
    ZERO_LOG_INFO(g_logger) << "test log way 1";
    ZERO_LOG_WARN(g_logger) << "this level is warn...";
    ZERO_LOG_DEBUG(g_logger) << "can not print..";  // 不会输出

    // 使用C风格输出日志
    ZERO_LOG_FMT_INFO(g_logger, "INFO %s:%d", "该日志级别为", 4);

    // 可以通过设置日志级别改变输出等级
    ZERO_LOG_INFO(g_logger) << g_logger->getLevel();
    g_logger->setLevel(zero::LogLevel::DEBUG);
    ZERO_LOG_INFO(g_logger) << g_logger->getLevel();
    ZERO_LOG_DEBUG(g_logger) << "can print..";  // 会输出

    zero::FileLogAppender::ptr file_appender(new zero::FileLogAppender("../log/test_log.txt"));
    test_system->addAppender(file_appender);
    ZERO_LOG_INFO(test_system) << "test log appender";
    ZERO_LOG_INFO(test_system) << "test log appender";  // 文件默认从尾部开始写

    // 调用日志方式二
    // 查找的过程中，如果不存在会自动创建一个logger，但是没有appender
    // 需要手动创建appender
    zero::Logger::ptr my_logger = ZERO_LOG_NAME("my_logger");
    zero::LogAppender::ptr test_appender(new zero::StdoutLogAppender);
    // 有默认的fomatter和level，也可以自己去设定输出格式和级别
    my_logger->addAppender(test_appender);
    ZERO_LOG_INFO(my_logger) << "test my logger";
    // 更改输出格式
    zero::LogFormatter::ptr my_formatter(new zero::LogFormatter("%d%T%p%T%c%T%f:%l %m%n"));
    test_appender->setFormatter(my_formatter);
    ZERO_LOG_INFO(my_logger) << "test new formmatter";

    // 调用方式三 动态配置的方式进行加载配置文件 配置变更的注册回调见log.cc文件末尾
    YAML::Node n = YAML::LoadFile("/home/ubuntu/Zero/conf/log.yml");
    zero::Config::LoadFromYaml(n);
    // 因为加载的配置文件中，重新规定了输出格式，所以此处my_logger的输出格式会再次发生改变
    ZERO_LOG_INFO(my_logger) << "test new formmatter";
    // 而在配置文件中system的logger级别发生了变化，所以只能输出error级别的日志，并且appender也发生了变化
    ZERO_LOG_INFO(test_system) << "test log config";   // 不输出
    ZERO_LOG_ERROR(test_system) << "test log config";  // 输出

    return 0;
}