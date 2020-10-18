#pragma once
// 定义日志级别
#include <string>

#include "noncopyable.h"

//#define LOG_INFO("%d", args)
// 宏对应多行代码时用do{...} while(0) 防止意想不到的错误
// snprintf比较安全
#define LOG_INFO(logmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while (0)
#define LOG_ERROR(logmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while (0) 

#define LOG_FATAL(logmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    }while (0) 
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while (0)
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

enum LogLevel {
    INFO,  // 普通信息
    ERROR, // 普通错误
    FATAL, // core信息 大错误
    DEBUG, // 调试信息
};

// 输出一个日志类, 要写成单例
class Logger : noncopyable{
public:
    // 获取日志唯一的实例对象
    static Logger& instance(); //返回引用, 调用时用.
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);
private:
    int logLevel_; // _放在后面
    Logger() {}
};
