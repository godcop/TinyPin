#pragma once

// 前向声明
class WindowCreationMonitor;

// 自动图钉选项标签页。
//
class OptAutoPin
{
public:
    static BOOL CALLBACK dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

protected:
    static void apply(HWND wnd, WindowCreationMonitor& winCreMon);
    static bool validate(HWND wnd);
    static void uiUpdate(HWND wnd);

    static bool evInitDialog(HWND wnd, HWND focus, LPARAM param);

    static bool cmAutoPinOn(HWND wnd);
};