#pragma once


// 用于在目标上放置图钉的跟踪窗口。
// 这是一个透明的1x1窗口，只是捕获鼠标
// 并响应左/右点击。
// 它的名称中有'layer'，因为在以前的版本中
// 它曾经覆盖整个屏幕。
//
class PinLayerWnd
{
public:
    static ATOM registerClass();
    static LRESULT CALLBACK proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static LPCWSTR className;

protected:
    static bool gotInitLButtonDown;

    static void evLButtonDown(HWND wnd, UINT mk, int x, int y);
};