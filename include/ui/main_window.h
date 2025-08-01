#pragma once

#include <memory>

class Options;
class WindowCreationMonitor;

// 主窗口类 - 处理通知图标菜单、热键、自动图钉
class MainWnd {
public:
    static ATOM registerClass();
    static LRESULT CALLBACK proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static LPCWSTR className;
    static void updateTrayIcon();

protected:
    // 事件处理方法
    static void evHotkey(HWND wnd, int idHotKey, Options* opt);
    static void evPinReq(HWND wnd, int x, int y, Options* opt);
    static void evTrayIcon(HWND wnd, WPARAM id, LPARAM msg, Options* opt);

    // 命令处理方法
    static void cmNewPin(HWND wnd);
    static void cmRemovePins(HWND wnd);
    static void cmOptions(HWND wnd, WindowCreationMonitor& winCreMon, Options* opt);

private:
    // 消息处理辅助方法
    static LRESULT handleCreate(HWND wnd, LPARAM lparam, std::unique_ptr<WindowCreationMonitor>& winCreMon, Options*& opt);
    static LRESULT handleDestroy(HWND wnd, std::unique_ptr<WindowCreationMonitor>& winCreMon, Options* opt);
    static LRESULT handleCommand(HWND wnd, WPARAM wparam, WindowCreationMonitor& winCreMon, Options* opt);
    static LRESULT handleDpiChanged(HWND wnd, WPARAM wparam, LPARAM lparam, Options* opt);
    
    // 初始化辅助方法
    static void initializeIcons(Options* opt);
    static bool setupTrayIcon(HWND wnd);
    static void setupHotkeys(HWND wnd, Options* opt);
    static void initializeDpiSettings(HWND wnd, Options* opt);
private:
    
    // 状态处理方法
    static void handlePinStatus(LPARAM lparam);
    static void showAboutDialog();
    static void showTrayContextMenu(HWND wnd);
};