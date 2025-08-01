#include "core/stdafx.h"
#include "pin/pin_manager.h"
#include "pin/pin_window.h"
#include "core/application.h"
#include "system/language_manager.h"
#include "foundation/error_handler.h"

namespace Pin {

bool PinManager::pinWindow(HWND wnd, HWND hitWnd, int trackRate, bool autoPin)
{
    int err = 0, wrn = 0;

    if (!hitWnd)
        wrn = IDS_ERR_COULDNOTFINDWND;
    else if (Window::isProgManWnd(hitWnd))
        wrn = IDS_ERR_CANNOTPINDESKTOP;
    // 注意：创建层窗口后，任务栏变为非置顶；
    // 使用此检查来避免固定它
    else if (Window::isTaskBar(hitWnd))
        wrn = IDS_ERR_CANNOTPINTASKBAR;
    else if (Window::isTopMost(hitWnd))
        wrn = IDS_ERR_ALREADYTOPMOST;
    // 隐藏窗口由代理机制处理
    //else if (!IsWindowVisible(hitWnd))
    //  Error(wnd, "Cannot pin a hidden window");
    else {
        // 创建一个图钉窗口（先创建为不可见，避免在错误位置闪现）
        HWND pin = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            PinWnd::className,
            L"",
            WS_POPUP,  // 移除WS_VISIBLE，稍后在正确位置设置后再显示
            0, 0, 0, 0,   // 实际位置/大小在窗口分配时设置
            0, 0, app.inst, 0);

        if (!pin)
            err = IDS_ERR_PINCREATE;
        else {
            // 设置透明色为黑色 (RGB(0,0,0))，这样图钉的背景会透明
            SetLayeredWindowAttributes(pin, RGB(0, 0, 0), 0, LWA_COLORKEY);
            
            if (!SendMessage(pin, ::App::WM_PIN_ASSIGNWND, WPARAM(hitWnd), trackRate)) {
                err = IDS_ERR_SETTOPMOSTFAIL;  // 使用更具体的置顶失败错误码
                DestroyWindow(pin);
            }
        }
    }

    if (!autoPin && (err || wrn)) {
        if (err) {
            // 使用语言管理器获取本地化错误消息
            std::wstring errorMsg = LANG_MGR.getControlText(err);
            if (errorMsg.empty()) {
                errorMsg = Util::Res::ResStr(err); // 回退到RC资源
            }
            Foundation::ErrorHandler::error(wnd, errorMsg.c_str());
        } else {
            // 使用语言管理器获取本地化警告消息
            std::wstring warningMsg = LANG_MGR.getControlText(wrn);
            if (warningMsg.empty()) {
                warningMsg = Util::Res::ResStr(wrn); // 回退到RC资源
            }
            Foundation::ErrorHandler::warning(wnd, warningMsg.c_str());
        }
    }

    return (err == 0 && wrn == 0);
}

bool PinManager::hasPin(HWND wnd)
{
    // 枚举所有图钉窗口
    HWND pin = nullptr;
    while ((pin = FindWindowEx(nullptr, pin, PinWnd::className, nullptr)) != nullptr) {
        if (HWND(SendMessage(pin, ::App::WM_PIN_GETPINNEDWND, 0, 0)) == wnd) {
            return true;
        }
    }

    return false;
}

bool PinManager::togglePin(HWND wnd, HWND target, int trackRate)
{
    target = Window::getTopParent(target);
    
    // 检查是否已经有图钉
    HWND pin = nullptr;
    while ((pin = FindWindowEx(nullptr, pin, PinWnd::className, nullptr)) != nullptr) {
        if (HWND(SendMessage(pin, ::App::WM_PIN_GETPINNEDWND, 0, 0)) == target) {
            DestroyWindow(pin);
            return true;
        }
    }
    
    // 没有图钉，创建新的
    return pinWindow(wnd, target, trackRate);
}

} // namespace Pin