#pragma once

#include "core/common.h"

namespace Foundation {
namespace ErrorHandler {

    // 错误级别枚举
    enum class ErrorLevel {
        Info,
        Warning,
        Error,
        Critical
    };

    // 统一的错误处理类
    class Handler {
    public:
        // 显示错误消息
        static void show(HWND parent, ErrorLevel level, LPCWSTR message, LPCWSTR title = nullptr);
        
        // 显示格式化的错误消息
        static void showFormatted(HWND parent, ErrorLevel level, LPCWSTR format, ...);
        
        // 显示系统错误
        static void showSystemError(HWND parent, DWORD errorCode, LPCWSTR context = nullptr);
        
        // 显示资源字符串错误
        static void showResourceError(HWND parent, ErrorLevel level, UINT stringId, ...);
        
        // 获取错误级别对应的图标
        static UINT getIconForLevel(ErrorLevel level);
        
        // 获取错误级别对应的默认标题
        static LPCWSTR getTitleForLevel(ErrorLevel level);
        
        // 设置自动图钉状态，用于避免无限弹窗
        static void setAutoPinInProgress(bool inProgress);
    };

    // 兼容性函数（保持向后兼容）
    void error(HWND wnd, LPCWSTR s);
    void warning(HWND wnd, LPCWSTR s);

} // namespace ErrorHandler
} // namespace Foundation