#include "core/stdafx.h"
#include "ui/custom_controls.h"
#include "core/application.h"
#include "resource.h"

// 确保SS_GROUPBOX常量可用
#ifndef SS_GROUPBOX
#define SS_GROUPBOX 0x00000007L
#endif

namespace Util {
namespace CustomControls {

// LinkCtrl 实现
LinkCtrl::LinkCtrl(HWND parent, int id) 
    : m_parent(parent), m_id(id), m_isHover(false), m_originalProc(nullptr)
{
    m_hwnd = GetDlgItem(parent, id);
    m_normalColor = RGB(0, 0, 255);
    m_hoverColor = RGB(255, 0, 0);
    m_visitedColor = RGB(128, 0, 128);
    m_normalFont = m_hoverFont = m_visitedFont = nullptr;
}

LinkCtrl* LinkCtrl::subclass(HWND parent, int id)
{
    auto ctrl = std::make_unique<LinkCtrl>(parent, id);
    if (ctrl->m_hwnd) {
        SetWindowLongPtr(ctrl->m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctrl.get()));
        ctrl->m_originalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(ctrl->m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(linkProc)));
        return ctrl.release(); // 转移所有权给调用者
    }
    return nullptr;
}

std::unique_ptr<LinkCtrl> LinkCtrl::subclassUnique(HWND parent, int id)
{
    auto ctrl = std::make_unique<LinkCtrl>(parent, id);
    if (ctrl->m_hwnd) {
        SetWindowLongPtr(ctrl->m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctrl.get()));
        ctrl->m_originalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(ctrl->m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(linkProc)));
        return ctrl;
    }
    return nullptr;
}

LinkCtrl::~LinkCtrl()
{
    // 恢复原始窗口过程
    if (m_hwnd && m_originalProc) {
        SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalProc));
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);
    }
}

void LinkCtrl::setFonts(HFONT normal, HFONT hover, HFONT visited)
{
    m_normalFont = normal;
    m_hoverFont = hover;
    m_visitedFont = visited;
    SendMessage(m_hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(normal), TRUE);
}

void LinkCtrl::setColors(COLORREF normal, COLORREF hover, COLORREF visited)
{
    m_normalColor = normal;
    m_hoverColor = hover;
    m_visitedColor = visited;
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void LinkCtrl::setUrl(LPCWSTR url)
{
    m_url = url;
}

LRESULT CALLBACK LinkCtrl::linkProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LinkCtrl* ctrl = reinterpret_cast<LinkCtrl*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!ctrl) return DefWindowProc(hwnd, msg, wParam, lParam);
    
    switch (msg) {
        case WM_LBUTTONDOWN:
            if (!ctrl->m_url.empty()) {
                ShellExecuteW(nullptr, L"open", ctrl->m_url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
            return 0;
            
        case WM_MOUSEMOVE:
            if (!ctrl->m_isHover) {
                ctrl->m_isHover = true;
                if (ctrl->m_hoverFont) {
                    SendMessage(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(ctrl->m_hoverFont), TRUE);
                }
                InvalidateRect(hwnd, nullptr, TRUE);
                
                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
            }
            break;
            
        case WM_MOUSELEAVE:
            ctrl->m_isHover = false;
            if (ctrl->m_normalFont) {
                SendMessage(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(ctrl->m_normalFont), TRUE);
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
            
        case WM_CTLCOLORSTATIC:
            {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                SetTextColor(hdc, ctrl->m_isHover ? ctrl->m_hoverColor : ctrl->m_normalColor);
                SetBkMode(hdc, TRANSPARENT);
                return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
            }
            
        case WM_SETCURSOR:
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            return TRUE;
    }
    
    return static_cast<BOOL>(CallWindowProc(ctrl->m_originalProc, hwnd, msg, wParam, lParam));
}

} // namespace CustomControls
} // namespace Util