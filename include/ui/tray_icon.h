#pragma once


// 程序通知图标管理。
//
class TrayIcon {
public:
    // 禁止复制
    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;
public:
    TrayIcon(UINT msg, UINT id);
    ~TrayIcon();

    bool setWnd(HWND wnd_);

    bool create(HICON icon, LPCTSTR tip);
    bool destroy();

    bool setTip(LPCTSTR tip);
    bool setIcon(HICON icon);

private:
    HWND wnd;
    UINT id;
    UINT msg;
};