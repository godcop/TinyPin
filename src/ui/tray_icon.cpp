#include "core/stdafx.h"
#include "ui/tray_icon.h"


TrayIcon::TrayIcon(UINT msg, UINT id)
:
    wnd(nullptr), msg(msg), id(id)
{
}


TrayIcon::~TrayIcon()
{
    destroy();
}


// 只能调用一次
bool TrayIcon::setWnd(HWND wnd_)
{
    if (wnd) return false;

    wnd = wnd_;
    return true;
}


bool TrayIcon::create(HICON icon, LPCTSTR tip)
{
    NOTIFYICONDATA nid;
    nid.cbSize           = sizeof(NOTIFYICONDATA);
    nid.hWnd             = wnd;
    nid.uID              = id;
    nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = msg;
    nid.hIcon            = icon;
    lstrcpyn(nid.szTip, tip, sizeof(nid.szTip));

    return !!Shell_NotifyIcon(NIM_ADD, &nid);
}


bool TrayIcon::destroy()
{
    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd   = wnd;
    nid.uID    = id;

    return !!Shell_NotifyIcon(NIM_DELETE, &nid);
}


bool TrayIcon::setTip(LPCTSTR tip)
{
    NOTIFYICONDATA nid;
    nid.cbSize           = sizeof(NOTIFYICONDATA);
    nid.hWnd             = wnd;
    nid.uID              = id;
    nid.uFlags           = NIF_TIP;
    lstrcpyn(nid.szTip, tip, sizeof(nid.szTip));

    return !!Shell_NotifyIcon(NIM_MODIFY, &nid);
}


bool TrayIcon::setIcon(HICON icon)
{
    NOTIFYICONDATA nid;
    nid.cbSize           = sizeof(NOTIFYICONDATA);
    nid.hWnd             = wnd;
    nid.uID              = id;
    nid.uFlags           = NIF_ICON;
    nid.hIcon            = icon;

    return !!Shell_NotifyIcon(NIM_MODIFY, &nid);
}
