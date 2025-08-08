#pragma once

#include "core/common.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace Pin {

    // 窗口绑定管理器
    // 负责管理置顶窗口的绑定行为，当一个窗口最小化/恢复时，绑定的其他窗口也会跟随
    // 当一个窗口移动时，其他绑定窗口也会跟随移动，保持相对位置
    class WindowBindingManager {
    private:
        static bool s_bindingEnabled;                           // 绑定功能是否启用
        static std::unordered_set<HWND> s_boundWindows;        // 已绑定的窗口集合
        static HWINEVENTHOOK s_minimizeEventHook;              // 窗口最小化事件钩子
        static HWINEVENTHOOK s_moveEventHook;                  // 窗口移动事件钩子
        static std::unordered_map<HWND, POINT> s_windowPositions; // 存储窗口位置，用于计算相对位置
        
        // 窗口事件回调函数 - 最小化/恢复事件
        static void CALLBACK MinimizeEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, 
                                         HWND hwnd, LONG idObject, LONG idChild, 
                                         DWORD dwEventThread, DWORD dwmsEventTime);
        
        // 窗口事件回调函数 - 位置变化事件
        static void CALLBACK MoveEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, 
                                         HWND hwnd, LONG idObject, LONG idChild, 
                                         DWORD dwEventThread, DWORD dwmsEventTime);
        
        // 获取所有已置顶的窗口
        static std::vector<HWND> getAllPinnedWindows();
        
        // 最小化所有绑定的窗口（除了触发窗口）
        static void minimizeAllBoundWindows(HWND excludeWindow);
        
        // 恢复所有绑定的窗口（除了触发窗口）
        static void restoreAllBoundWindows(HWND excludeWindow);
        
        // 移动所有绑定的窗口（除了触发窗口）
        static void moveAllBoundWindows(HWND movedWindow, int deltaX, int deltaY);
        
        // 更新窗口位置缓存
        static void updateWindowPositions();
        
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