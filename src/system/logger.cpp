#include "core/stdafx.h"
#include "system/logger.h"
#include "core/application.h"

// 获取单例实例
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

// 构造函数
Logger::Logger()
#ifdef _DEBUG
    : m_logLevel(LogLevel::DEBUG)
#else
    : m_logLevel(LogLevel::WARNING)  // Release版本只记录警告和错误
#endif
    , m_initialized(false),
      m_bufferSize(50),  // 缓冲区最多存储50条日志
      m_flushInterval(std::chrono::milliseconds(2000))  // 2秒刷新间隔
{
    m_lastFlush = std::chrono::steady_clock::now();
    m_logBuffer.reserve(m_bufferSize);  // 预分配缓冲区空间
}

// 析构函数
Logger::~Logger() {
    shutdown();
}

// 程序退出时的清理工作
void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 先刷新缓冲区，确保所有日志都被写入
    flushBuffer();
    
    // 先清理旧的日志文件（此时日志文件还开着，可以记录删除操作）
    cleanupOldLogFiles(10);
    
    if (m_logFile.is_open()) {
        // 写入日志结束标记
        m_logFile << wstringToUtf8(L"===================================") << std::endl;
        m_logFile << wstringToUtf8(L"微钉 日志结束 - " + getCurrentTimeString()) << std::endl;
        m_logFile << wstringToUtf8(L"===================================") << std::endl;
        m_logFile.close();
    }
    
    m_initialized = false;
}

// 初始化日志系统
bool Logger::init(const std::wstring& logDir) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true; // 已经初始化过
    }
    
    // 获取应用程序路径
    std::wstring appPath = Foundation::FileUtils::getModulePath(app.inst);
    std::wstring appDir = Foundation::FileUtils::getDirPath(appPath);
    
    // 设置日志目录
    m_logDir = appDir + logDir;
    
    // 创建日志目录
    if (!createLogDirectory(m_logDir)) {
        return false;
    }
    
    // 生成日志文件名
    m_logFilePath = m_logDir + L"\\" + generateLogFileName();
    
    // 打开日志文件
    m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);
    if (!m_logFile.is_open()) {
        return false;
    }
    
    // 写入UTF-8 BOM以确保正确的编码识别
    m_logFile << "\xEF\xBB\xBF";
    
    // 写入日志头
    m_logFile << wstringToUtf8(L"===================================") << std::endl;
    m_logFile << wstringToUtf8(L"微钉 日志开始 - " + getCurrentTimeString()) << std::endl;
    m_logFile << wstringToUtf8(L"===================================") << std::endl;
    
    m_initialized = true;
    return true;
}

// 写入日志
void Logger::log(LogLevel level, const std::wstring& message) {
    if (level < m_logLevel || !m_initialized) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_logFile.is_open()) {
        std::wstring logLine = getCurrentTimeString() + L" [" + getLevelString(level) + L"] " + message;
        std::string utf8LogLine = wstringToUtf8(logLine);
        
        // 添加到缓冲区
        m_logBuffer.push_back(utf8LogLine);
        
        // 检查是否需要立即刷新
        if (shouldFlush(level)) {
            flushBuffer();
        }
    }
}

// 检查是否需要刷新缓冲区
bool Logger::shouldFlush(LogLevel level) const {
    // 错误和致命错误立即刷新
    if (level >= LogLevel::ERR) {
        return true;
    }
    
    // 缓冲区满了需要刷新
    if (m_logBuffer.size() >= m_bufferSize) {
        return true;
    }
    
    // 超过时间间隔需要刷新
    auto now = std::chrono::steady_clock::now();
    if (now - m_lastFlush >= m_flushInterval) {
        return true;
    }
    
    return false;
}

// 批量写入缓冲区到文件
void Logger::flushBuffer() {
    if (m_logBuffer.empty() || !m_logFile.is_open()) {
        return;
    }
    
    // 批量写入所有缓冲的日志
    for (const auto& logLine : m_logBuffer) {
        m_logFile << logLine << std::endl;
    }
    
    // 刷新文件流
    m_logFile.flush();
    
    // 清空缓冲区并更新时间
    m_logBuffer.clear();
    m_lastFlush = std::chrono::steady_clock::now();
}

// 强制刷新缓冲区
void Logger::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    flushBuffer();
}

// 不同级别的日志方法
void Logger::debug(const std::wstring& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::wstring& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::wstring& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::wstring& message) {
    log(LogLevel::ERR, message);
}

void Logger::fatal(const std::wstring& message) {
    log(LogLevel::FATAL, message);
}

// 设置日志级别
void Logger::setLogLevel(LogLevel level) {
    m_logLevel = level;
}

// 获取当前日志级别
LogLevel Logger::getLogLevel() const {
    return m_logLevel;
}

// 获取日志级别的字符串表示
std::wstring Logger::getLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:   return L"调试";
        case LogLevel::INFO:    return L"信息";
        case LogLevel::WARNING: return L"警告";
        case LogLevel::ERR:   return L"错误";
        case LogLevel::FATAL:   return L"致命";
        default:                return L"未知";
    }
}

// 获取当前时间的字符串表示
std::wstring Logger::getCurrentTimeString() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::wstringstream ss;
    std::tm tm_buf;
    localtime_s(&tm_buf, &time);
    
    ss << std::setfill(L'0')
       << std::setw(4) << (tm_buf.tm_year + 1900) << L'-'
       << std::setw(2) << (tm_buf.tm_mon + 1) << L'-'
       << std::setw(2) << tm_buf.tm_mday << L' '
       << std::setw(2) << tm_buf.tm_hour << L':'
       << std::setw(2) << tm_buf.tm_min << L':'
       << std::setw(2) << tm_buf.tm_sec << L'.'
       << std::setw(3) << ms.count();
    
    return ss.str();
}

// 将宽字符串转换为UTF-8
std::string Logger::wstringToUtf8(const std::wstring& wstr) const {
    if (wstr.empty()) {
        return std::string();
    }
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// 创建日志目录
bool Logger::createLogDirectory(const std::wstring& logDir) {
    try {
        if (!std::filesystem::exists(logDir)) {
            return std::filesystem::create_directories(logDir);
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// 生成日志文件名
std::wstring Logger::generateLogFileName() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::wstringstream ss;
    std::tm tm_buf;
    localtime_s(&tm_buf, &time);
    
    ss << L"tinypin_"
       << std::setfill(L'0')
       << std::setw(4) << (tm_buf.tm_year + 1900)
       << std::setw(2) << (tm_buf.tm_mon + 1)
       << std::setw(2) << tm_buf.tm_mday << L"_"
       << std::setw(2) << tm_buf.tm_hour
       << std::setw(2) << tm_buf.tm_min
       << std::setw(2) << tm_buf.tm_sec << L".log";
    
    return ss.str();
}

// 清理旧的日志文件，只保留最近的指定数量
void Logger::cleanupOldLogFiles(int keepCount) {
    if (!m_initialized || m_logDir.empty()) {
        return;
    }
    
    try {
        std::vector<std::filesystem::directory_entry> logFiles;
        
        // 遍历日志目录，收集所有日志文件
        for (const auto& entry : std::filesystem::directory_iterator(m_logDir)) {
            if (entry.is_regular_file()) {
                std::wstring filename = entry.path().filename().wstring();
                // 检查是否是我们的日志文件（以tinypin_开头，以.log结尾）
                if (filename.length() > 12 && 
                    filename.substr(0, 8) == L"tinypin_" && 
                    filename.substr(filename.length() - 4) == L".log") {
                    logFiles.push_back(entry);
                }
            }
        }
        
        // 如果日志文件数量不超过保留数量，则不需要清理
        if (static_cast<int>(logFiles.size()) <= keepCount) {
            return;
        }
        
        // 按修改时间排序（最新的在前）
        std::sort(logFiles.begin(), logFiles.end(), 
                  [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
                      return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                  });
        
        // 删除多余的旧文件
        for (size_t i = keepCount; i < logFiles.size(); ++i) {
            try {
                std::filesystem::remove(logFiles[i].path());
                // 记录删除操作（如果当前日志文件还可用的话）
                if (m_logFile.is_open()) {
                    std::wstring deletedFile = logFiles[i].path().filename().wstring();
                    std::wstring logMessage = L"已删除旧日志文件: " + deletedFile;
                    m_logFile << wstringToUtf8(getCurrentTimeString() + L" [信息] " + logMessage) << std::endl;
                    m_logFile.flush();
                }
            } catch (const std::exception&) {
                // 删除失败时忽略错误，避免影响程序正常退出
            }
        }
        
    } catch (const std::exception&) {
        // 清理过程中出现异常时忽略，避免影响程序正常退出
    }
}