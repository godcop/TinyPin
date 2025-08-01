#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <filesystem>

// 日志级别枚举
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERR,  // 避免使用 ERROR 关键字，它可能是 Windows API 中的预定义宏
    FATAL
};

// 日志系统类
class Logger {
public:
    // 获取单例实例
    static Logger& getInstance();

    // 初始化日志系统
    bool init(const std::wstring& logDir = L"log");

    // 写入日志
    void log(LogLevel level, const std::wstring& message);

    // 不同级别的日志方法
    void debug(const std::wstring& message);
    void info(const std::wstring& message);
    void warning(const std::wstring& message);
    void error(const std::wstring& message);
    void fatal(const std::wstring& message);

    // 设置日志级别
    void setLogLevel(LogLevel level);

    // 获取当前日志级别
    LogLevel getLogLevel() const;

    // 清理旧的日志文件，只保留最近的指定数量
    void cleanupOldLogFiles(int keepCount = 10);

    // 程序退出时的清理工作
    void shutdown();

    // 强制刷新缓冲区
    void flush();

private:
    // 私有构造函数，防止外部创建实例
    Logger();
    ~Logger();

    // 禁止复制和移动
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // 获取日志级别的字符串表示
    std::wstring getLevelString(LogLevel level) const;

    // 获取当前时间的字符串表示
    std::wstring getCurrentTimeString() const;

    // 创建日志目录
    bool createLogDirectory(const std::wstring& logDir);

    // 生成日志文件名
    std::wstring generateLogFileName() const;

    // 批量写入缓冲区到文件
    void flushBuffer();

    // 检查是否需要刷新缓冲区
    bool shouldFlush(LogLevel level) const;

    // 成员变量
    std::ofstream m_logFile;
    std::mutex m_mutex;
    LogLevel m_logLevel;
    std::wstring m_logDir;
    std::wstring m_logFilePath;
    bool m_initialized;
    
    // 批量写入相关成员
    std::vector<std::string> m_logBuffer;      // 日志缓冲区
    size_t m_bufferSize;                       // 缓冲区大小限制
    std::chrono::steady_clock::time_point m_lastFlush;  // 上次刷新时间
    std::chrono::milliseconds m_flushInterval;          // 刷新间隔
    
    // 辅助方法：将宽字符串转换为UTF-8
    std::string wstringToUtf8(const std::wstring& wstr) const;
};

// 全局日志宏，方便使用
#define LOG_DEBUG(message) Logger::getInstance().debug(message)
#define LOG_INFO(message) Logger::getInstance().info(message)
#define LOG_WARNING(message) Logger::getInstance().warning(message)
#define LOG_ERROR(message) Logger::getInstance().error(message)
#define LOG_FATAL(message) Logger::getInstance().fatal(message)