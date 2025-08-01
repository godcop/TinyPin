#pragma once

#include "core/common.h"

namespace Graphics {
namespace Geometry {
    // 点结构，用于替代ef::Win::Point
    class Point : public POINT {
    public:
        // 默认构造函数
        Point() { x = y = 0; }
        
        // 带坐标的构造函数
        Point(int x, int y) { this->x = x; this->y = y; }
        
        // 从LPARAM构造
        static Point Packed(LPARAM lParam) {
            return Point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        
        // 打包为LPARAM
        LPARAM packed() const {
            return MAKELPARAM(x, y);
        }
    };
}
}