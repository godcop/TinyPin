#pragma once


// 常规选项标签页。
//
class OptPins
{
public:
    static BOOL CALLBACK dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

protected:
    static bool validate(HWND wnd);
    static void apply(HWND wnd);

    static void updatePinWnds();
    static BOOL CALLBACK resetPinTimersEnumProc(HWND wnd, LPARAM param);
    
    static bool selectIconFile(HWND wnd);
    static bool resetToDefault(HWND wnd);

    static bool evInitDialog(HWND wnd, HWND focus, LPARAM lparam);
    static void evTermDialog(HWND wnd);
};