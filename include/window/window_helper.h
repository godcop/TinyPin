#pragma once

#include "core/common.h"
#include <string>

namespace Window {

    // 窗口基本属性获取函数
    std::wstring getWindowText(HWND wnd);
    std::wstring getWindowClassName(HWND wnd);
    
    // 窗口状态检查函数
    bool isWndRectEmpty(HWND wnd);
    bool isChild(HWND wnd);
    bool isTopMost(HWND wnd);
    
    // 窗口层次结构操作
    HWND getNonChildParent(HWND wnd);
    HWND getTopParent(HWND wnd);
    
    // 获取窗口的实际可视边框（考虑DWM扩展边框）
    bool getVisibleWindowRect(HWND wnd, RECT& rect);
    
    // 窗口操作函数
    BOOL moveWindow(HWND wnd, const RECT& rc, BOOL repaint = TRUE);
    bool psChanged(HWND page);
    void enableGroup(HWND wnd, int id, bool mode);
    
    // 窗口辅助类，用于替代ef::Win::WndH
    class WndHelper {
    public:
        WndHelper(HWND hwnd);
        
        // 获取窗口文本
        std::wstring getText() const;
        
        // 设置窗口文本
        void setText(const std::wstring& text) const;
        
        // 获取窗口类名
        std::wstring getClassName() const;
        
        // 获取窗口样式
        LONG getStyle() const;
        
        // 获取扩展窗口样式
        LONG getExStyle() const;
        
        // 修改扩展窗口样式
        void modifyExStyle(LONG exStyleToRemove, LONG exStyleToAdd) const;
        
        // 更新窗口
        void update() const;
        
    private:
        HWND m_hwnd;
    };
    
    // 窗口类注册配置结构
    struct WindowClassConfig {
        UINT style = CS_HREDRAW | CS_VREDRAW;
        WNDPROC wndProc = nullptr;
        int cbClsExtra = 0;
        int cbWndExtra = 0;
        HICON icon = nullptr;
        HCURSOR cursor = nullptr;
        HBRUSH background = nullptr;
        LPCWSTR menuName = nullptr;
        LPCWSTR className = nullptr;
        HICON iconSm = nullptr;
    };
    
    // 窗口类注册辅助类
    class WindowRegistrar {
    public:
        // 注册完整的窗口类
        static ATOM registerWindowClass(const WindowClassConfig& config);
        
        // 注册简单的窗口类
        static ATOM registerSimpleClass(LPCWSTR className, WNDPROC wndProc, 
                                       HCURSOR cursor = nullptr, int cbWndExtra = 0);
    };

} // namespace Window