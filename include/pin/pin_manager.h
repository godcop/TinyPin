#pragma once

#include "core/common.h"

namespace Pin {

    // 图钉管理器
    // 负责图钉的创建、检查和切换操作
    class PinManager {
    public:
        // 为指定窗口创建图钉
        // wnd: 图钉窗口句柄
        // targetWnd: 目标窗口句柄
        // trackRate: 跟踪频率
        // autoPin: 是否自动图钉
        // 返回: 成功返回true，失败返回false
        static bool pinWindow(HWND wnd, HWND targetWnd, int trackRate, bool autoPin = false);
        
        // 检查窗口是否已被图钉
        // targetWnd: 目标窗口句柄
        // 返回: 已被图钉返回true，否则返回false
        static bool hasPin(HWND targetWnd);
        
        // 切换窗口的图钉状态
        // wnd: 图钉窗口句柄
        // targetWnd: 目标窗口句柄
        // trackRate: 跟踪频率
        // 返回: 成功返回true，失败返回false
        static bool togglePin(HWND wnd, HWND targetWnd, int trackRate);
    };

} // namespace Pin