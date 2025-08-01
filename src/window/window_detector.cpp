#include "core/stdafx.h"
#include "window/window_detector.h"
#include "window/window_helper.h"
#include "window/window_cache.h"  // 添加窗口缓存支持
#include "foundation/string_utils.h"
#include <appmodel.h>  // 用于GetPackageFullName

bool Window::isProgManWnd(HWND wnd)
{ 
    // 使用缓存的API减少系统调用
    std::wstring className = Window::Cached::getWindowClassName(wnd);
    std::wstring windowText = Window::Cached::getWindowText(wnd);
    
    return Foundation::StringUtils::strimatch(className.c_str(), L"ProgMan")
        && Foundation::StringUtils::strimatch(windowText.c_str(), L"Program Manager");
}

bool Window::isTaskBar(HWND wnd)
{
    // 使用缓存的API减少系统调用
    std::wstring className = Window::Cached::getWindowClassName(wnd);
    return Foundation::StringUtils::strimatch(className.c_str(), L"Shell_TrayWnd");
}

bool Window::isVCLAppWnd(HWND wnd)
{
    // 使用缓存的API减少系统调用
    std::wstring className = Window::Cached::getWindowClassName(wnd);
    return Foundation::StringUtils::strimatch(L"TApplication", className.c_str()) 
        && Window::isWndRectEmpty(wnd);
}

// 检测现代Windows应用程序（UWP、Win32包装等）
bool Window::isModernWindowsApp(HWND wnd)
{
    // 使用缓存的API减少系统调用
    std::wstring className = Window::Cached::getWindowClassName(wnd);
    
    // 检查常见的现代Windows应用类名
    if (Foundation::StringUtils::strimatch(L"ApplicationFrameWindow", className.c_str()) ||
        Foundation::StringUtils::strimatch(L"Windows.UI.Core.CoreWindow", className.c_str()) ||
        Foundation::StringUtils::strimatch(L"XAML_WindowedPopupClass", className.c_str())) {
        return true;
    }
    
    // 检查进程是否为UWP应用
    DWORD processId;
    GetWindowThreadProcessId(wnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (hProcess) {
        // 检查是否为UWP应用包
        UINT32 length = 0;
        LONG result = GetPackageFullName(hProcess, &length, nullptr);
        CloseHandle(hProcess);
        
        // 如果GetPackageFullName返回ERROR_INSUFFICIENT_BUFFER，说明这是一个UWP应用
        if (result == ERROR_INSUFFICIENT_BUFFER) {
            return true;
        }
    }
    
    return false;
}

// 检测是否需要代理模式的综合函数
bool Window::needsProxyMode(HWND wnd)
{
    // 原有的VCL应用检测
    if (isVCLAppWnd(wnd)) {
        return true;
    }
    
    // 现代Windows应用检测
    if (isModernWindowsApp(wnd)) {
        return true;
    }
    
    // 窗口不可见或矩形为空
    // 使用缓存的API减少系统调用
    if (!Window::Cached::isWindowVisible(wnd) || isWndRectEmpty(wnd)) {
        return true;
    }
    
    return false;
}