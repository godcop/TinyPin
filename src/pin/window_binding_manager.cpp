#include "core/stdafx.h"
#include "pin/window_binding_manager.h"
#include "pin/pin_window.h"
#include "pin/pin_manager.h"
#include "core/application.h"
#include "system/logger.h"
#include <vector>
#include <algorithm>

namespace Pin {

// 静态成员变量定义
bool WindowBindingManager::s_bindingEnabled = false;
std::unordered_set<HWND> WindowBindingManager::s_boundWindows;
HWINEVENTHOOK WindowBindingManager::s_eventHook = nullptr;

bool WindowBindingManager::initialize() {
    if (s_eventHook) {
        return true; // 已经初始化
    }
    
    // 设置窗口事件钩子，监听窗口最小化和恢复事件
    s_eventHook = SetWinEventHook(
        EVENT_SYSTEM_MINIMIZESTART,     // 最小化开始
        EVENT_SYSTEM_MINIMIZEEND,       // 最小化结束
        nullptr,                        // 不指定模块
        WinEventProc,                   // 回调函数
        0,                              // 所有进程
        0,                              // 所有线程
        WINEVENT_OUTOFCONTEXT          // 异步调用
    );
    
    if (!s_eventHook) {
        LOG_ERROR(L"无法设置窗口事件钩子");
        return false;
    }
    
    return true;
}

void WindowBindingManager::cleanup() {
    if (s_eventHook) {
        UnhookWinEvent(s_eventHook);
        s_eventHook = nullptr;
    }
    
    s_boundWindows.clear();
    s_bindingEnabled = false;
}

void WindowBindingManager::setBindingEnabled(bool enabled) {
    s_bindingEnabled = enabled;
    
    if (enabled) {
        // 启用绑定时，更新绑定窗口列表
        updateBoundWindows();
    } else {
        // 禁用绑定时，清空绑定窗口列表
        s_boundWindows.clear();
    }
}

void WindowBindingManager::updateBoundWindows() {
    if (!s_bindingEnabled) {
        return;
    }
    
    // 清空当前绑定列表
    s_boundWindows.clear();
    
    // 获取所有已置顶的窗口
    std::vector<HWND> pinnedWindows = getAllPinnedWindows();
    
    // 将所有已置顶的窗口添加到绑定列表
    for (HWND hwnd : pinnedWindows) {
        s_boundWindows.insert(hwnd);
    }
}

std::vector<HWND> WindowBindingManager::getAllPinnedWindows() {
    std::vector<HWND> pinnedWindows;
    
    // 枚举所有图钉窗口，获取被置顶的窗口
    HWND pin = nullptr;
    while ((pin = FindWindowEx(nullptr, pin, PinWnd::className, nullptr)) != nullptr) {
        HWND pinnedWnd = reinterpret_cast<HWND>(SendMessage(pin, ::App::WM_PIN_GETPINNEDWND, 0, 0));
        if (pinnedWnd && IsWindow(pinnedWnd)) {
            pinnedWindows.push_back(pinnedWnd);
        }
    }
    
    return pinnedWindows;
}

bool WindowBindingManager::isWindowBound(HWND hwnd) {
    return s_boundWindows.find(hwnd) != s_boundWindows.end();
}

void CALLBACK WindowBindingManager::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, 
                                                 HWND hwnd, LONG idObject, LONG idChild, 
                                                 DWORD dwEventThread, DWORD dwmsEventTime) {
    // 只处理窗口对象的事件
    if (idObject != OBJID_WINDOW || !hwnd) {
        return;
    }
    
    // 检查是否启用绑定功能
    if (!s_bindingEnabled) {
        return;
    }
    
    // 检查触发事件的窗口是否在绑定列表中
    if (!isWindowBound(hwnd)) {
        return;
    }
    
    switch (event) {
        case EVENT_SYSTEM_MINIMIZESTART:
            // 窗口开始最小化，最小化所有其他绑定窗口
            minimizeAllBoundWindows(hwnd);
            break;
            
        case EVENT_SYSTEM_MINIMIZEEND:
            // 窗口最小化结束（恢复），恢复所有其他绑定窗口
            // 检查窗口是否真的恢复了（而不是最小化）
            if (!IsIconic(hwnd)) {
                restoreAllBoundWindows(hwnd);
            }
            break;
    }
}

void WindowBindingManager::minimizeAllBoundWindows(HWND excludeWindow) {
    for (HWND hwnd : s_boundWindows) {
        if (hwnd != excludeWindow && IsWindow(hwnd) && !IsIconic(hwnd)) {
            // 最小化窗口
            ShowWindow(hwnd, SW_MINIMIZE);
        }
    }
}

void WindowBindingManager::restoreAllBoundWindows(HWND excludeWindow) {
    for (HWND hwnd : s_boundWindows) {
        if (hwnd != excludeWindow && IsWindow(hwnd) && IsIconic(hwnd)) {
            // 恢复窗口
            ShowWindow(hwnd, SW_RESTORE);
            // 将窗口带到前台
            SetForegroundWindow(hwnd);
        }
    }
}

} // namespace Pin