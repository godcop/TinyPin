#include "core/stdafx.h"
#include "pin/window_binding_manager.h"
#include "pin/pin_window.h"
#include "pin/pin_manager.h"
#include "core/application.h"
#include "system/logger.h"
#include "window/window_helper.h"
#include <vector>
#include <algorithm>

namespace Pin {

// 静态成员变量定义
bool WindowBindingManager::s_bindingEnabled = false;
std::unordered_set<HWND> WindowBindingManager::s_boundWindows;
HWINEVENTHOOK WindowBindingManager::s_minimizeEventHook = nullptr;
HWINEVENTHOOK WindowBindingManager::s_moveEventHook = nullptr;
std::unordered_map<HWND, POINT> WindowBindingManager::s_windowPositions;

bool WindowBindingManager::initialize() {
    if (s_minimizeEventHook && s_moveEventHook) {
        return true; // 已经初始化
    }
    
    // 设置窗口事件钩子，监听窗口最小化和恢复事件
    s_minimizeEventHook = SetWinEventHook(
        EVENT_SYSTEM_MINIMIZESTART,     // 最小化开始
        EVENT_SYSTEM_MINIMIZEEND,       // 最小化结束
        nullptr,                        // 不指定模块
        MinimizeEventProc,              // 回调函数
        0,                              // 所有进程
        0,                              // 所有线程
        WINEVENT_OUTOFCONTEXT          // 异步调用
    );
    
    // 设置窗口事件钩子，监听窗口位置变化事件
    s_moveEventHook = SetWinEventHook(
        EVENT_OBJECT_LOCATIONCHANGE,    // 位置变化事件
        EVENT_OBJECT_LOCATIONCHANGE,    // 位置变化事件
        nullptr,                        // 不指定模块
        MoveEventProc,                  // 回调函数
        0,                              // 所有进程
        0,                              // 所有线程
        WINEVENT_OUTOFCONTEXT          // 异步调用
    );
    
    if (!s_minimizeEventHook || !s_moveEventHook) {
        LOG_ERROR(L"无法设置窗口事件钩子");
        cleanup(); // 清理已创建的钩子
        return false;
    }
    
    return true;
}

void WindowBindingManager::cleanup() {
    if (s_minimizeEventHook) {
        UnhookWinEvent(s_minimizeEventHook);
        s_minimizeEventHook = nullptr;
    }
    
    if (s_moveEventHook) {
        UnhookWinEvent(s_moveEventHook);
        s_moveEventHook = nullptr;
    }
    
    s_boundWindows.clear();
    s_windowPositions.clear();
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
        s_windowPositions.clear();
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
    
    // 更新窗口位置缓存
    updateWindowPositions();
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

void CALLBACK WindowBindingManager::MinimizeEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, 
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
                // 窗口恢复后，更新窗口位置缓存
                updateWindowPositions();
            }
            break;
    }
}

void CALLBACK WindowBindingManager::MoveEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, 
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
    
    // 处理窗口位置变化事件
    if (event == EVENT_OBJECT_LOCATIONCHANGE) {
        // 获取窗口当前位置
        RECT currentRect;
        if (!GetWindowRect(hwnd, &currentRect)) {
            return;
        }
        
        // 检查窗口位置是否在缓存中
        auto it = s_windowPositions.find(hwnd);
        if (it == s_windowPositions.end()) {
            // 如果窗口位置不在缓存中，添加到缓存
            POINT currentPos = {currentRect.left, currentRect.top};
            s_windowPositions[hwnd] = currentPos;
            return;
        }
        
        // 计算位置变化量
        int deltaX = currentRect.left - it->second.x;
        int deltaY = currentRect.top - it->second.y;
        
        // 如果位置有变化，移动其他绑定窗口
        if (deltaX != 0 || deltaY != 0) {
            // 更新缓存中的位置
            it->second.x = currentRect.left;
            it->second.y = currentRect.top;
            
            // 移动其他绑定窗口
            moveAllBoundWindows(hwnd, deltaX, deltaY);
        }
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

void WindowBindingManager::moveAllBoundWindows(HWND movedWindow, int deltaX, int deltaY) {
    // 遍历所有绑定窗口
    for (HWND hwnd : s_boundWindows) {
        // 跳过触发移动的窗口和已最小化的窗口
        if (hwnd == movedWindow || !IsWindow(hwnd) || IsIconic(hwnd)) {
            continue;
        }
        
        // 获取窗口当前位置
        RECT currentRect;
        if (!GetWindowRect(hwnd, &currentRect)) {
            continue;
        }
        
        // 计算新位置
        int newX = currentRect.left + deltaX;
        int newY = currentRect.top + deltaY;
        
        // 移动窗口
        RECT newRect = {
            newX,
            newY,
            newX + (currentRect.right - currentRect.left),
            newY + (currentRect.bottom - currentRect.top)
        };
        
        Window::moveWindow(hwnd, newRect, TRUE);
        
        // 更新缓存中的位置
        if (s_windowPositions.find(hwnd) != s_windowPositions.end()) {
            s_windowPositions[hwnd].x = newX;
            s_windowPositions[hwnd].y = newY;
        } else {
            s_windowPositions[hwnd] = {newX, newY};
        }
    }
}

void WindowBindingManager::updateWindowPositions() {
    // 清空当前位置缓存
    s_windowPositions.clear();
    
    // 遍历所有绑定窗口，更新位置缓存
    for (HWND hwnd : s_boundWindows) {
        if (IsWindow(hwnd) && !IsIconic(hwnd)) {
            RECT rect;
            if (GetWindowRect(hwnd, &rect)) {
                s_windowPositions[hwnd] = {rect.left, rect.top};
            }
        }
    }
}

} // namespace Pin