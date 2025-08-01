#include "core/stdafx.h"
#include "options/hotkey_options.h"
#include "core/application.h"
#include "options/options.h"
#include "window/window_helper.h"
#include "system/language_manager.h"


bool OptHotKeys::cmHotkeysOn(HWND wnd)
{
    bool b = IsDlgButtonChecked(wnd, IDC_HOTKEYS_ON) == BST_CHECKED;
    Window::enableGroup(wnd, IDC_HOTKEYS_GROUP, b);
    Window::psChanged(wnd);
    return true;
}


bool OptHotKeys::evInitDialog(HWND wnd, HWND focus, LPARAM param)
{
    // must have a valid data ptr
    if (!param) {
        EndDialog(wnd, IDCANCEL);
        return false;
    }

    // save the data ptr
    PROPSHEETPAGE& psp = *reinterpret_cast<PROPSHEETPAGE*>(param);
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(psp.lParam)->opt;
    SetWindowLongPtr(wnd, GWLP_USERDATA, psp.lParam);

    // 获取当前DPI并现代化对话框
    int currentDpi = Graphics::DpiManager::getDpiForWindow(wnd);
    Util::Dialog::modernizeDialog(wnd, currentDpi);

    // 本地化对话框文本
    Util::Dialog::localizeDialog(wnd, L"hotkeys");

    CheckDlgButton(wnd, IDC_HOTKEYS_ON, opt.hotkeysOn);
    cmHotkeysOn(wnd);

    opt.hotEnterPin.setUI(wnd, IDC_HOT_PINMODE);
    opt.hotTogglePin.setUI(wnd, IDC_HOT_TOGGLEPIN);

    return false;
}


bool OptHotKeys::validate(HWND wnd)
{
    return true;
}


bool OptHotKeys::changeHotkey(HWND wnd, 
    const HotKey& newHotkey, bool newState, 
    const HotKey& oldHotkey, bool oldState)
{
    // get old & new state to figure out transition
    bool wasOn = oldState && oldHotkey.vk;
    bool isOn  = newState && newHotkey.vk;

    // bail out if it's still off
    if (!wasOn && !isOn)
        return true;

    // turning off
    if (wasOn && !isOn)
        return newHotkey.unset(app.mainWnd);

    // turning on OR key change
    if ((!wasOn && isOn) || (newHotkey != oldHotkey))
        return newHotkey.set(app.mainWnd);

    // same key is still on
    return true;
}


void OptHotKeys::apply(HWND wnd)
{
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(GetWindowLongPtr(wnd, GWLP_USERDATA))->opt;

    bool hotkeysOn = IsDlgButtonChecked(wnd, IDC_HOTKEYS_ON) == BST_CHECKED;

    HotKey enterKey(App::HOTID_ENTERPINMODE);
    HotKey toggleKey(App::HOTID_TOGGLEPIN);

    enterKey.getUI(wnd, IDC_HOT_PINMODE);
    toggleKey.getUI(wnd, IDC_HOT_TOGGLEPIN);

    // Set hotkeys and report error if any failed.
    // Get separate success flags to avoid short-circuiting
    // (and set as many keys as possible)
    bool allKeysSet = true;
    allKeysSet &= changeHotkey(
        wnd, enterKey, hotkeysOn, opt.hotEnterPin, opt.hotkeysOn);
    allKeysSet &= changeHotkey(
        wnd, toggleKey, hotkeysOn, opt.hotTogglePin, opt.hotkeysOn);

    opt.hotkeysOn = hotkeysOn;
    opt.hotEnterPin = enterKey;
    opt.hotTogglePin = toggleKey;

    if (!allKeysSet)
        Foundation::ErrorHandler::error(wnd, LanguageManager::getInstance().getString(L"hotkeys_set_error").c_str());
    
    // 立即保存设置到INI文件
    opt.saveImmediately();
}


BOOL CALLBACK OptHotKeys::dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
        case WM_INITDIALOG:  return evInitDialog(wnd, HWND(wparam), lparam);
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
                case PSN_APPLY: {
                    apply(wnd);
                    return true;
                }
                default:
                    return false;
            }
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wparam), code = HIWORD(wparam);
            switch (id) {
                case IDC_HOTKEYS_ON:    cmHotkeysOn(wnd); return true;
                case IDC_HOT_PINMODE:
                case IDC_HOT_TOGGLEPIN: if (code == EN_CHANGE) Window::psChanged(wnd); return true;
                default:                return false;
            }
        }
        default:
            return false;
    }
}
