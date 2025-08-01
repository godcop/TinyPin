#pragma once

#include "core/common.h"

namespace Window {

    // 特殊窗口检测函数
    bool isProgManWnd(HWND wnd);
    bool isTaskBar(HWND wnd);
    bool isVCLAppWnd(HWND wnd);
    bool isModernWindowsApp(HWND wnd);
    bool needsProxyMode(HWND wnd);

} // namespace Window