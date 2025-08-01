#include "core/stdafx.h"
#include "options/pin_options.h"
#include "core/application.h"
#include "options/options.h"
#include "pin/pin_window.h"
#include "ui/main_window.h"
#include "system/language_manager.h"
#include "foundation/string_utils.h"
#include <commdlg.h>
#include <shlwapi.h>


#define OWNERDRAW_CLR_BUTTON  0


bool OptPins::evInitDialog(HWND wnd, HWND focus, LPARAM lparam)
{
    // must have a valid data ptr
    if (!lparam) {
        EndDialog(wnd, IDCANCEL);
        return false;
    }

    // save the data ptr
    PROPSHEETPAGE& psp = *reinterpret_cast<PROPSHEETPAGE*>(lparam);
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(psp.lParam)->opt;
    SetWindowLongPtr(wnd, GWLP_USERDATA, psp.lParam);

    // 获取当前DPI并现代化对话框
    int currentDpi = Graphics::DpiManager::getDpiForWindow(wnd);
    Util::Dialog::modernizeDialog(wnd, currentDpi);

    // 本地化对话框文本
    Util::Dialog::localizeDialog(wnd, L"pins");
    // 手动刷新按钮文本，排查本地化失效问题
    OutputDebugString(L"[DEBUG] 刷新按钮文本: Change\n");
    Util::Dialog::localizeControl(wnd, IDC_PIN_ICON_CHANGE, L"pins", L"change");
    OutputDebugString(L"[DEBUG] 刷新按钮文本: Reset Default\n");
    Util::Dialog::localizeControl(wnd, IDC_PIN_ICON_RESET, L"pins", L"reset_default");

    SendDlgItemMessage(wnd, IDC_POLL_RATE_UD, UDM_SETRANGE, 0, 
        MAKELONG(opt.trackRate.maxV,opt.trackRate.minV));
    SendDlgItemMessage(wnd, IDC_POLL_RATE_UD, UDM_SETPOS, 0, 
        MAKELONG(opt.trackRate.value,0));

    CheckDlgButton(wnd, 
        opt.dblClkTray ? IDC_TRAY_DOUBLE_CLICK : IDC_TRAY_SINGLE_CLICK,
        BST_CHECKED);

    if (opt.runOnStartup)
        CheckDlgButton(wnd, IDC_RUN_ON_STARTUP, BST_CHECKED);

    return false;
}


void OptPins::evTermDialog(HWND wnd)
{
    // 对话框终止时的清理工作（如果需要的话）
}


bool OptPins::validate(HWND wnd)
{
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(GetWindowLongPtr(wnd, GWLP_USERDATA))->opt;
    return opt.trackRate.validateUI(wnd, IDC_POLL_RATE, true);
}


void OptPins::updatePinWnds()
{
    // 枚举所有图钉窗口并刷新显示
    EnumWindows([](HWND wnd, LPARAM) -> BOOL {
        Window::WndHelper helper(wnd);
        if (Foundation::StringUtils::strimatch(PinWnd::className, helper.getClassName().c_str()))
            InvalidateRect(wnd, nullptr, false);
        return TRUE;    // continue
    }, 0);
}


BOOL CALLBACK OptPins::resetPinTimersEnumProc(HWND wnd, LPARAM param)
{
    Window::WndHelper helper(wnd);
    if (Foundation::StringUtils::strimatch(PinWnd::className, helper.getClassName().c_str()))
        SendMessage(wnd, App::WM_PIN_RESETTIMER, param, 0);
    return true;    // continue
}


void OptPins::apply(HWND wnd)
{
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(GetWindowLongPtr(wnd, GWLP_USERDATA))->opt;

    // 处理跟踪频率变更
    int rate = opt.trackRate.getUI(wnd, IDC_POLL_RATE);
    if (opt.trackRate.value != rate)
        EnumWindows(resetPinTimersEnumProc, opt.trackRate.value = rate);

    // 处理托盘双击设置
    opt.dblClkTray = IsDlgButtonChecked(wnd, IDC_TRAY_DOUBLE_CLICK) == BST_CHECKED;

    // 处理开机自启动设置
    opt.runOnStartup = IsDlgButtonChecked(wnd, IDC_RUN_ON_STARTUP) == BST_CHECKED;
    
    // 立即保存设置到INI文件
    opt.saveImmediately();
    
    // 更新图钉窗口显示
    updatePinWnds();
    
    // 更新托盘图标
    MainWnd::updateTrayIcon();
}


bool OptPins::selectIconFile(HWND wnd)
{
    OPENFILENAME ofn = {};
    WCHAR szFile[MAX_PATH] = {};
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = wnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
    ofn.lpstrFilter = L"图像文件\0*.png;*.jpg;*.jpeg;*.bmp;*.ico\0PNG文件\0*.png\0JPEG文件\0*.jpg;*.jpeg\0位图文件\0*.bmp\0图标文件\0*.ico\0所有文件\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = L"选择图标文件";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    
    if (GetOpenFileName(&ofn)) {
        // 获取目标目录路径
        WCHAR targetDir[MAX_PATH];
        GetModuleFileName(nullptr, targetDir, MAX_PATH);
        PathRemoveFileSpec(targetDir);
        wcscat_s(targetDir, L"\\temp\\uploads\\");
        
        // 确保目录存在
        SHCreateDirectoryEx(nullptr, targetDir, nullptr);
        
        // 获取源文件名
        WCHAR* fileName = PathFindFileName(szFile);
        
        // 构建目标文件路径
        WCHAR targetPath[MAX_PATH];
        wcscpy_s(targetPath, targetDir);
        wcscat_s(targetPath, fileName);
        
        // 复制文件
        if (CopyFile(szFile, targetPath, FALSE)) {
            // 更新配置
            Options& opt = reinterpret_cast<OptionsPropSheetData*>(GetWindowLongPtr(wnd, GWLP_USERDATA))->opt;
            
            // 构建相对路径
            WCHAR relativePath[MAX_PATH];
            wcscpy_s(relativePath, L"temp\\uploads\\");
            wcscat_s(relativePath, fileName);
            
            opt.pinImagePath = relativePath;
            Window::psChanged(wnd);
            return true;
        } else {
            MessageBox(wnd, L"复制文件失败！", L"错误", MB_OK | MB_ICONERROR);
        }
    }
    return false;
}


bool OptPins::resetToDefault(HWND wnd)
{
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(GetWindowLongPtr(wnd, GWLP_USERDATA))->opt;
    opt.pinImagePath = L"assets\\images\\TinyPin.png";
    
    // 立即保存设置到INI文件
    opt.saveImmediately();
    
    // 更新图钉窗口显示
    updatePinWnds();
    
    // 更新托盘图标
    MainWnd::updateTrayIcon();
    
    Window::psChanged(wnd);
    return true;
}


BOOL CALLBACK OptPins::dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{

    switch (msg) {
        case WM_INITDIALOG:  return evInitDialog(wnd, HWND(wparam), lparam);
        case WM_DESTROY:     evTermDialog(wnd); return true;
        case WM_NOTIFY: {
            NMHDR nmhdr = *reinterpret_cast<NMHDR*>(lparam);
            switch (nmhdr.code) {
                case PSN_SETACTIVE: {
                    HWND tab = PropSheet_GetTabControl(nmhdr.hwndFrom);
                    app.optionPage = static_cast<int>(SendMessage(tab, TCM_GETCURSEL, 0, 0));
                    SetWindowLongPtr(wnd, DWLP_MSGRESULT, 0);
                    return true;
                }
                case PSN_KILLACTIVE: {
                    SetWindowLongPtr(wnd, DWLP_MSGRESULT, !validate(wnd));
                    return true;
                }
                case PSN_APPLY:
                    apply(wnd);
                    return true;
                case UDN_DELTAPOS: {
                    if (wparam == IDC_POLL_RATE_UD) {
                        NM_UPDOWN& nmud = *(NM_UPDOWN*)lparam;
                        Options& opt = 
                            reinterpret_cast<OptionsPropSheetData*>(GetWindowLongPtr(wnd, GWLP_USERDATA))->opt;
                        nmud.iDelta *= opt.trackRate.step;
                        SetWindowLongPtr(wnd, DWLP_MSGRESULT, FALSE);   // allow change
                        return true;
                    }
                    else
                        return false;
                }
                default:
                    return false;
            }
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wparam), code = HIWORD(wparam);
            switch (id) {
                case IDC_TRAY_SINGLE_CLICK:
                case IDC_TRAY_DOUBLE_CLICK: Window::psChanged(wnd); return true;
                case IDC_POLL_RATE:         if (code == EN_CHANGE) Window::psChanged(wnd); return true;
                case IDC_PIN_ICON_CHANGE:   if (code == BN_CLICKED) { selectIconFile(wnd); return true; }
                case IDC_PIN_ICON_RESET:    if (code == BN_CLICKED) { resetToDefault(wnd); return true; }
                case IDC_RUN_ON_STARTUP:    Window::psChanged(wnd); return true;
                default:                    return false;
            }
        }
        default:
            return false;
    }

}
