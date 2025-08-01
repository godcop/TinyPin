#pragma once

#include "core/common.h"

namespace Graphics {

    // 窗口视觉效果函数
    // 高亮标记指定窗口
    // wnd: 目标窗口句柄
    // mode: true为高亮显示，false为取消高亮
    void markWindow(HWND wnd, bool mode);

} // namespace Graphics