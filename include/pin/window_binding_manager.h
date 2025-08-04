#pragma once

#include "core/common.h"
#include <vector>
#include <unordered_set>

namespace Pin {

    // 窗口绑定管理器
    // 负责管理置顶窗口的绑定行为，当一个窗口最小化/恢复时，绑定的其他窗口也会跟随
    class WindowBindingManager {
    private:
        static bool s_bindingEnabled;                           // 绑定功能是否启用
        static std::unordered_set<HWND> s_boundWindows;        // 已绑定的窗口集合
        static HWINEVENTHOOK s_eventHook;                      // 窗口事件钩子
        
        // 窗口事件回调函数
        static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, 
                                         HWND hwnd, LONG idObject, LONG idChild, 
                                         DWORD dwEventThread, DWORD dwmsEventTime);
        
        // 获取所有已置顶的窗口
        static std::vector<HWND> getAllPinnedWindows();
        
        // 最小化所有绑定的窗口（除了触发窗口）
        static void minimizeAllBoundWindows(HWND excludeWindow);
        
        // 恢复所有绑定的窗口（除了触发窗口）
        static void restoreAllBoundWindows(HWND excludeWindow);
        
    public:
        // 初始化窗口绑定管理器
        static bool initialize();
        
        // 清理窗口绑定管理器
        static void cleanup();
        
        // 启用/禁用窗口绑定功能
        static void setBindingEnabled(bool enabled);
        
        // 获取绑定功能是否启用
        static bool isBindingEnabled() { return s_bindingEnabled; }
        
        // 更新绑定的窗口列表（当有新图钉创建或删除时调用）
        static void updateBoundWindows();
        
        // 检查窗口是否在绑定列表中
        static bool isWindowBound(HWND hwnd);
    };

} // namespace Pin