#pragma once

#include "core/common.h"
#include <string>

namespace Graphics {
namespace Color {
    // 颜色枚举
    enum Colors {
        black   = 0x000000, maroon  = 0x000080, green   = 0x008000, olive   = 0x008080, 
        navy    = 0x800000, purple  = 0x800080, teal    = 0x808000, silver  = 0xc0c0c0, 
        gray    = 0x808080, red     = 0x0000ff, lime    = 0x00ff00, yellow  = 0x00ffff,
        blue    = 0xff0000, magenta = 0xff00ff, cyan    = 0xffff00, white   = 0xffffff
    };
    
    // 颜色亮度调整
    COLORREF light(COLORREF clr);
    COLORREF dark(COLORREF clr);
    
    // RGB格式转换函数
    std::wstring colorToRgbString(COLORREF color);
    COLORREF rgbStringToColor(const std::wstring& rgbStr);
    bool isRgbFormat(const std::wstring& str);
}
}