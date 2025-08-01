#include "core/stdafx.h"
#include "core/application.h"
#include "foundation/error_handler.h"
#include "system/language_manager.h"

namespace Foundation {
namespace ErrorHandler {

// 静态变量，用于跟踪自动图钉状态
static bool s_isAutoPinInProgress = false;

void Handler::show(HWND parent, ErrorLevel level, LPCWSTR message, LPCWSTR title) {
    // 如果正在进行自动图钉操作，避免显示错误弹窗以防止无限循环
    if (s_isAutoPinInProgress) {
        // 静默处理错误，不记录日志以避免干扰
        return;
    }
    
    UINT icon = getIconForLevel(level);
    LPCWSTR actualTitle = title ? title : getTitleForLevel(level);
    MessageBox(parent, message, actualTitle, icon | MB_TOPMOST);
}

// 设置自动图钉状态的辅助函数
void Handler::setAutoPinInProgress(bool inProgress) {
    s_isAutoPinInProgress = inProgress;
}

void Handler::showFormatted(HWND parent, ErrorLevel level, LPCWSTR format, ...) {
    WCHAR buffer[Constants::LARGE_BUFFER_SIZE];
    
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, Constants::LARGE_BUFFER_SIZE, format, args);
    va_end(args);
    
    show(parent, level, buffer);
}

void Handler::showSystemError(HWND parent, DWORD errorCode, LPCWSTR context) {
    LPWSTR messageBuffer = nullptr;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer, 0, nullptr);
    
    if (size > 0 && messageBuffer) {
        WCHAR fullMessage[Constants::LARGE_BUFFER_SIZE];
        std::wstring systemErrorText = LANG_MGR.getString(L"common.system_error");
        if (systemErrorText.empty()) systemErrorText = L"系统错误";
        
        if (context) {
            swprintf_s(fullMessage, Constants::LARGE_BUFFER_SIZE, 
                L"%s\n\n%s %lu: %s", context, systemErrorText.c_str(), errorCode, messageBuffer);
        } else {
            swprintf_s(fullMessage, Constants::LARGE_BUFFER_SIZE, 
                L"%s %lu: %s", systemErrorText.c_str(), errorCode, messageBuffer);
        }
        
        show(parent, ErrorLevel::Error, fullMessage);
        LocalFree(messageBuffer);
    } else {
        WCHAR fallbackMessage[Constants::SMALL_BUFFER_SIZE];
        std::wstring unknownErrorText = LANG_MGR.getString(L"common.unknown_system_error");
        if (unknownErrorText.empty()) unknownErrorText = L"未知系统错误";
        
        if (context) {
            swprintf_s(fallbackMessage, Constants::SMALL_BUFFER_SIZE, 
                L"%s\n\n%s: %lu", context, unknownErrorText.c_str(), errorCode);
        } else {
            swprintf_s(fallbackMessage, Constants::SMALL_BUFFER_SIZE, 
                L"%s: %lu", unknownErrorText.c_str(), errorCode);
        }
        show(parent, ErrorLevel::Error, fallbackMessage);
    }
}

void Handler::showResourceError(HWND parent, ErrorLevel level, UINT stringId, ...) {
    WCHAR format[Constants::LARGE_BUFFER_SIZE];
    WCHAR message[Constants::LARGE_BUFFER_SIZE];
    
    // 加载资源字符串
    if (LoadStringW(app.resMod ? app.resMod : app.inst, stringId, format, Constants::LARGE_BUFFER_SIZE) > 0) {
        va_list args;
        va_start(args, stringId);
        vswprintf_s(message, Constants::LARGE_BUFFER_SIZE, format, args);
        va_end(args);
        
        show(parent, level, message);
    } else {
        // 如果无法加载资源字符串，显示通用错误
        std::wstring errorFormat = LANG_MGR.getString(L"common.resource_string_load_error");
        if (errorFormat.empty()) errorFormat = L"错误: 无法加载资源字符串 ID %u";
        
        swprintf_s(message, Constants::LARGE_BUFFER_SIZE, errorFormat.c_str(), stringId);
        show(parent, ErrorLevel::Error, message);
    }
}

UINT Handler::getIconForLevel(ErrorLevel level) {
    switch (level) {
        case ErrorLevel::Info:
            return MB_ICONINFORMATION;
        case ErrorLevel::Warning:
            return MB_ICONWARNING;
        case ErrorLevel::Error:
            return MB_ICONSTOP;
        case ErrorLevel::Critical:
            return MB_ICONSTOP;
        default:
            return MB_ICONINFORMATION;
    }
}

LPCWSTR Handler::getTitleForLevel(ErrorLevel level) {
    static std::wstring infoTitle, warningTitle, errorTitle, criticalTitle;
    
    switch (level) {
        case ErrorLevel::Info:
            infoTitle = LANG_MGR.getString(L"common.information");
            if (infoTitle.empty()) infoTitle = L"信息";
            return infoTitle.c_str();
        case ErrorLevel::Warning:
            warningTitle = LANG_MGR.getString(L"common.warning");
            if (warningTitle.empty()) warningTitle = L"警告";
            return warningTitle.c_str();
        case ErrorLevel::Error:
            errorTitle = LANG_MGR.getString(L"common.error");
            if (errorTitle.empty()) errorTitle = L"错误";
            return errorTitle.c_str();
        case ErrorLevel::Critical:
            criticalTitle = LANG_MGR.getString(L"common.critical_error");
            if (criticalTitle.empty()) criticalTitle = L"严重错误";
            return criticalTitle.c_str();
        default:
            return App::APPNAME;
    }
}

// 兼容性函数实现
void error(HWND wnd, LPCWSTR s) {
    Handler::show(wnd, ErrorLevel::Error, s);
}

void warning(HWND wnd, LPCWSTR s) {
    Handler::show(wnd, ErrorLevel::Warning, s);
}

} // namespace ErrorHandler
} // namespace Foundation