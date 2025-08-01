#include "core/stdafx.h"
#include "core/application.h"
#include "pin/pin_layer_window.h"
#include "resource.h"
#include "system/logger.h"


LPCWSTR PinLayerWnd::className = L"EFPinLayerWnd";
bool PinLayerWnd::gotInitLButtonDown = false;


ATOM PinLayerWnd::registerClass()
{
    HCURSOR cursor = LoadCursor(app.inst, MAKEINTRESOURCE(IDC_PLACEPIN));
    ATOM result = Window::WindowRegistrar::registerSimpleClass(className, proc, cursor);
    
    if (!result) {
        LOG_ERROR(L"图钉层窗口类注册失败");
    }
    
    return result;
}


void PinLayerWnd::evLButtonDown(HWND wnd, UINT mk, int x, int y)
{
    SetCapture(wnd);

    if (!gotInitLButtonDown) {
        gotInitLButtonDown = true;
    } else {
        ReleaseCapture();
        POINT pt = { x, y };
        if (ClientToScreen(wnd, &pt)) {
            PostMessage(GetParent(wnd), App::WM_PINREQ, static_cast<WPARAM>(pt.x), static_cast<LPARAM>(pt.y));
        } else {
            LOG_WARNING(L"无法转换客户端坐标到屏幕坐标");
        }
        DestroyWindow(wnd);
    }
}


LRESULT CALLBACK PinLayerWnd::proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_CREATE:
        gotInitLButtonDown = false;
        return false;

    case WM_DESTROY:
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        if (wnd == app.layerWnd) {
            app.layerWnd = NULL;
        }
        return false;

    case WM_LBUTTONDOWN:
        evLButtonDown(wnd, static_cast<UINT>(wparam), GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        return false;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KILLFOCUS:
        ReleaseCapture();
        DestroyWindow(wnd);
        return false;
    }

    return DefWindowProc(wnd, msg, wparam, lparam);
}
