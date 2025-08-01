#include "core/stdafx.h"
#include "core/application.h"
#include "options/options.h"
#include "options/options_dialog.h"
#include "system/logger.h"
#include "system/language_manager.h"


WNDPROC OptionsDlg::orgOptPSProc = nullptr;


void OptionsDlg::buildOptPropSheet(PROPSHEETHEADER& psh, PROPSHEETPAGE psp[], 
    int dlgIds[], DLGPROC dlgProcs[], int pageCnt, HWND parentWnd, 
    OptionsPropSheetData& data, Util::Res::ResStr& capStr)
{
    // 选项卡标题常量数组
    static std::wstring tabTitles[4];
    const int tabCtrlIds[4] = {IDC_TAB_PINS_TITLE, IDC_TAB_AUTOPIN_TITLE, IDC_TAB_HOTKEYS_TITLE, IDC_TAB_LANGUAGE_TITLE};
    // 英文后备标题
    const wchar_t* fallbackTitles[4] = {L"Pins", L"Auto Pin", L"Hotkeys", L"Language"};
    
    for (int n = 0; n < pageCnt; ++n) {
        psp[n].dwSize      = sizeof(psp[n]);
        psp[n].dwFlags     = 0;  // 移除PSP_HASHELP标志
        psp[n].hInstance   = app.resMod ? app.resMod : app.inst;
        psp[n].pszTemplate = MAKEINTRESOURCE(dlgIds[n]);
        psp[n].hIcon       = nullptr;
        // 使用控件ID映射系统获取本地化标题，如果失败则使用英文后备
        if (n < 4) {
            tabTitles[n] = LANG_MGR.getControlText(tabCtrlIds[n]);
            if (tabTitles[n].empty()) {
                tabTitles[n] = fallbackTitles[n];  // 使用英文后备标题
            }
            psp[n].pszTitle = tabTitles[n].c_str();
            psp[n].dwFlags |= PSP_USETITLE;
        } else {
            psp[n].pszTitle = nullptr;
        }
        psp[n].pfnDlgProc  = dlgProcs[n];
        psp[n].lParam      = reinterpret_cast<LPARAM>(&data);
        psp[n].pfnCallback = nullptr;
        psp[n].pcRefParent = nullptr;
    }

    psh.dwSize      = sizeof(psh);
    psh.dwFlags     = PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_USEHICON;  // 移除PSH_HASHELP标志
    psh.hwndParent  = parentWnd;
    psh.hInstance   = app.resMod ? app.resMod : app.inst;
    psh.hIcon       = app.smIcon;
    psh.pszCaption  = capStr;
    psh.nPages      = pageCnt;
    psh.nStartPage  = (app.optionPage >= 0 && app.optionPage < pageCnt) 
        ? app.optionPage : 0;
    psh.ppsp        = psp;
    psh.pfnCallback = optPSCallback;

}


void OptionsDlg::fixOptPSPos(HWND wnd)
{
    // - find taskbar ("Shell_TrayWnd")
    // - find notification area ("TrayNotifyWnd") (child of taskbar)
    // - get center of notification area
    // - determine quadrant of screen which includes the center point
    // - position prop sheet at proper corner of workarea
    RECT rc, rcWA, rcNW;
    HWND trayWnd, notityWnd;
    if (GetWindowRect(wnd, &rc)
        && SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWA, 0)
        && (trayWnd = FindWindow(L"Shell_TrayWnd", L""))
        && (notityWnd = FindWindowEx(trayWnd, nullptr, L"TrayNotifyWnd", L""))
        && GetWindowRect(notityWnd, &rcNW))
    {
        // '/2' simplified from the following two inequalities
        bool isLeft = (rcNW.left + rcNW.right) < GetSystemMetrics(SM_CXSCREEN);
        bool isTop  = (rcNW.top + rcNW.bottom) < GetSystemMetrics(SM_CYSCREEN);
        int x = isLeft ? rcWA.left : rcWA.right - (rc.right - rc.left);
        int y = isTop ? rcWA.top : rcWA.bottom - (rc.bottom - rc.top);
        OffsetRect(&rc, x-rc.left, y-rc.top);
        Window::moveWindow(wnd, rc);
    }

}


LRESULT CALLBACK OptionsDlg::optPSSubclass(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_USER + 1000) {
        // 延迟本地化属性表对话框标题
        Util::Dialog::localizeDialogTitle(wnd, L"options");
        // 延迟本地化属性表按钮
        Util::Dialog::localizePropertySheetButtons(wnd, L"options");
        
        // 本地化完成后，恢复原始窗口过程
        if (orgOptPSProc) {
            SetWindowLongPtr(wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(orgOptPSProc));
            orgOptPSProc = nullptr;
        }
        return 0;
    }
    
    if (msg == WM_SHOWWINDOW) {
        fixOptPSPos(wnd);

        // 同时设置大图标（用于Alt-Tab）
        SendMessage(wnd, WM_SETICON, ICON_BIG, 
            LPARAM(LoadIcon(app.inst, MAKEINTRESOURCE(IDI_APP))));

        return static_cast<BOOL>(CallWindowProc(orgOptPSProc, wnd, msg, wparam, lparam));
    }

    return static_cast<BOOL>(CallWindowProc(orgOptPSProc, wnd, msg, wparam, lparam));
}


// 移除WS_EX_CONTEXTHELP并子类化以修复位置
int CALLBACK OptionsDlg::optPSCallback(HWND wnd, UINT msg, LPARAM param)
{
    if (msg == PSCB_INITIALIZED) {
        // 移除标题帮助按钮
        Window::WndHelper(wnd).modifyExStyle(WS_EX_CONTEXTHELP, 0);
        orgOptPSProc = WNDPROC(SetWindowLongPtr(wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(optPSSubclass)));
        
        // 延迟本地化属性表对话框，确保所有按钮都已创建
        PostMessage(wnd, WM_USER + 1000, 0, 0);
    }

    return 0;
}
