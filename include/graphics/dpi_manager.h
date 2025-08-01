#pragma once

#include "core/common.h"

namespace Graphics {
namespace DpiManager {
    // DPI感知初始化
    bool initDpiAwareness();
    
    // 获取窗口DPI
    int getDpiForWindow(HWND hwnd);
    
    // 获取系统DPI
    int getSystemDpi();
    
    // 获取DPI缩放比例
    double getDpiScale(HWND hwnd = nullptr);
    
    // 根据DPI缩放尺寸
    void scaleSizeForDpi(SIZE& size, int dpi);
    
    // 根据DPI缩放矩形
    void scaleRectForDpi(RECT& rect, int dpi);
}
}