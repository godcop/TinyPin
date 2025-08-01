#include "core/stdafx.h"
#include "ui/main_window.h"
#include "core/application.h"
#include "pin/pin_manager.h"
#include "pin/pin_window.h"
#include "pin/pin_layer_window.h"
#include "pin/auto_pin_manager.h"
#include "window/window_monitor.h"
#include "options/options.h"
#include "options/options_dialog.h"
#include "options/pin_options.h"
#include "options/auto_pin_options.h"
#include "options/hotkey_options.h"
#include "options/language_options.h"
#include "ui/tray_icon.h"
#include "system/language_manager.h"
#include "system/logger.h"

LPCWSTR MainWnd::className = L"EFTinyPin";

namespace {
    // 常量定义
    constexpr struct {
        int id;
        LPCWSTR title;
        LPCWSTR url;
    } DIALOG_LINKS[] = {
        { IDC_WEB, L"Github", L"https://github.com/godcop/TinyPin" }
    };

    // RAII字体管理器
    class FontManager {
        Util::RAII::FontGuard m_boldFont;
        Util::RAII::FontGuard m_underlineFont;
        
    public:
        FontManager() : m_boldFont(nullptr, Util::RAII::ObjectDeleter{}) , m_underlineFont(nullptr, Util::RAII::ObjectDeleter{}) {}
        
        void setBoldFont(HFONT font) { 
            m_boldFont = Util::RAII::makeFontGuard(font);
        }
        
        void setUnderlineFont(HFONT font) { 
            m_underlineFont = Util::RAII::makeFontGuard(font);
        }
        
        HFONT getBoldFont() const { return m_boldFont.get(); }
        HFONT getUnderlineFont() const { return m_underlineFont.get(); }
    };

    // 托盘菜单装饰器
    class TrayMenuDecorations {
        Util::RAII::BitmapGuard m_bmpClose;
        Util::RAII::BitmapGuard m_bmpAbout;

    public:
        explicit TrayMenuDecorations(HMENU menu) 
            : m_bmpClose(nullptr), m_bmpAbout(nullptr) {
            const int w = GetSystemMetrics(SM_CXMENUCHECK);
            const int h = GetSystemMetrics(SM_CYMENUCHECK);

            auto dcGuard = Util::RAII::makeDCGuard(CreateCompatibleDC(nullptr));
            if (!dcGuard.isValid()) return;

            auto fontGuard = Util::RAII::makeFontGuard(
                Util::Font::FontHelper::create(L"Marlett", h, SYMBOL_CHARSET, Util::Font::noStyle));
            if (fontGuard.isValid()) {
                HGDIOBJ orgFont = SelectObject(dcGuard.get(), fontGuard.get());
                HBRUSH bkBrush = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
                
                // 创建位图并转移所有权
                HBITMAP closeBmp = createMenuBitmap(dcGuard.get(), w, h, bkBrush, menu, CM_CLOSE, L"r");
                HBITMAP aboutBmp = createMenuBitmap(dcGuard.get(), w, h, bkBrush, menu, CM_ABOUT, L"s");
                
                m_bmpClose = Util::RAII::makeBitmapGuard(closeBmp);
                m_bmpAbout = Util::RAII::makeBitmapGuard(aboutBmp);
                
                SelectObject(dcGuard.get(), orgFont);
            }
        }

    private:
        HBITMAP createMenuBitmap(HDC dc, int w, int h, HBRUSH bkBrush, HMENU menu, int idCmd, LPCWSTR text) {
            HBITMAP bitmap = CreateBitmap(w, h, 1, 1, nullptr);
            if (!bitmap) return nullptr;

            HGDIOBJ orgBmp = SelectObject(dc, bitmap);
            RECT rc = {0, 0, w, h};
            
            FillRect(dc, &rc, bkBrush);
            DrawText(dc, text, 1, &rc, DT_CENTER | DT_NOPREFIX);
            SetMenuItemBitmaps(menu, idCmd, MF_BYCOMMAND, bitmap, bitmap);
            
            SelectObject(dc, orgBmp);
            return bitmap;
        }
    };
}

// 关于对话框类
class AboutDlg {
    static FontManager s_fontManager;

public:
    static INT_PTR CALLBACK proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        switch (msg) {
            case WM_INITDIALOG:
                return initializeDialog(wnd);
            case WM_DESTROY:
                app.aboutDlg = nullptr;
                return TRUE;
            case App::WM_PINSTATUS:
                SetDlgItemInt(wnd, IDC_STATUS, app.pinsUsed, FALSE);
                return TRUE;
            case WM_COMMAND:
                return handleCommand(wnd, wparam);
            case WM_LBUTTONDOWN:
                {
                    POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
                    HWND childWnd = ChildWindowFromPoint(wnd, pt);
                    int childId = childWnd ? GetDlgCtrlID(childWnd) : 0;
                    
                    if (childWnd && childId == IDC_WEB) {
                        SendMessage(childWnd, WM_LBUTTONDOWN, wparam, lparam);
                        return TRUE;
                    }
                    
                    return FALSE;
                }
        }
        return FALSE;
    }

private:
    static BOOL initializeDialog(HWND wnd) {
        app.aboutDlg = wnd;
        SetDlgItemInt(wnd, IDC_STATUS, app.pinsUsed, FALSE);

        const int dpi = Graphics::DpiManager::getDpiForWindow(wnd);
        setupFonts(wnd, dpi);
        setupIcon(wnd, dpi);
        setupLinks(wnd);
        
        // 本地化对话框文本
        Util::Dialog::localizeDialog(wnd, L"about");
        
        return TRUE;
    }

    static void setupFonts(HWND wnd, int dpi) {
        HFONT modernFont = Util::Font::FontHelper::getModernSystemFont(dpi);
        if (!modernFont) return;

        HFONT boldFont = Util::Font::FontHelper::createDpiAware(modernFont, Util::Font::bold, dpi);
        HFONT underlineFont = Util::Font::FontHelper::createDpiAware(modernFont, Util::Font::underline, dpi);

        s_fontManager.setBoldFont(boldFont);
        s_fontManager.setUnderlineFont(underlineFont);

        // 设置状态控件字体
        if (HWND statusWnd = GetDlgItem(wnd, IDC_STATUS)) {
            SendMessage(statusWnd, WM_SETFONT, reinterpret_cast<WPARAM>(boldFont), TRUE);
            if (HWND prevWnd = GetWindow(statusWnd, GW_HWNDPREV)) {
                SendMessage(prevWnd, WM_SETFONT, reinterpret_cast<WPARAM>(boldFont), TRUE);
            }
        }

        // 为其他控件设置现代字体
        EnumChildWindows(wnd, [](HWND child, LPARAM lParam) -> BOOL {
            const int ctrlId = GetDlgCtrlID(child);
            if (ctrlId == IDC_STATUS || ctrlId == IDC_MAIL || ctrlId == IDC_WEB) {
                return TRUE;
            }
            
            WCHAR className[32];
            if (GetClassName(child, className, 32) > 0) {
                if (wcscmp(className, L"Static") == 0 || wcscmp(className, L"Button") == 0) {
                    SendMessage(child, WM_SETFONT, lParam, TRUE);
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(modernFont));
    }

    static void setupIcon(HWND wnd, int dpi) {
        const int iconSize = Util::Font::FontHelper::scaleSizeForDpi(16, dpi);
        HICON appIcon = static_cast<HICON>(LoadImage(app.inst, MAKEINTRESOURCE(IDI_APP), 
                                                   IMAGE_ICON, iconSize, iconSize, LR_DEFAULTCOLOR));
        if (!appIcon) {
            appIcon = LoadIcon(app.inst, MAKEINTRESOURCE(IDI_APP));
        }
        
        if (appIcon) {
            SendMessage(wnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(appIcon));
            SendMessage(wnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(app.smIcon));
            if (app.resMod) {
                SendDlgItemMessage(wnd, IDC_LOGO, STM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(appIcon));
            }
        }
    }

    static void setupLinks(HWND wnd) {
        HFONT modernFont = Util::Font::FontHelper::getModernSystemFont(Graphics::DpiManager::getDpiForWindow(wnd));
        
        for (const auto& link : DIALOG_LINKS) {
            SetDlgItemText(wnd, link.id, link.title);
            
            if (auto* linkCtrl = Util::CustomControls::LinkCtrl::subclass(wnd, link.id)) {
                linkCtrl->setFonts(modernFont, 
                                 s_fontManager.getUnderlineFont() ? s_fontManager.getUnderlineFont() : modernFont, 
                                 modernFont);
                linkCtrl->setColors(RGB(0,102,204), RGB(220,20,60), RGB(128,0,128));
                linkCtrl->setUrl(link.url);
            }
        }
    }

    static BOOL handleCommand(HWND wnd, WPARAM wparam) {
        const int id = LOWORD(wparam);
        const int code = HIWORD(wparam);
        
        switch (id) {
            case IDOK:
            case IDCANCEL:
                DestroyWindow(wnd);
                return TRUE;
            case IDC_LOGO:
                if (code == STN_DBLCLK) {
                    MessageBox(wnd, LANG_MGR.getString(L"build_info").c_str(), App::APPNAME, MB_ICONINFORMATION);
                    return TRUE;
                }
                break;
        }
        return FALSE;
    }
};

FontManager AboutDlg::s_fontManager;

ATOM MainWnd::registerClass() {
    return Window::WindowRegistrar::registerSimpleClass(className, proc);
}

LRESULT CALLBACK MainWnd::proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    static const UINT taskbarMsg = RegisterWindowMessage(L"TaskbarCreated");
    static PendingWindows pendWnds;
    static std::unique_ptr<WindowCreationMonitor> winCreMon;
    static Options* opt = nullptr;

    switch (msg) {
        case WM_CREATE:
            return handleCreate(wnd, lparam, winCreMon, opt);
        case WM_DESTROY:
            return handleDestroy(wnd, winCreMon, opt);
        case App::WM_TRAYICON:
            evTrayIcon(wnd, wparam, lparam, opt);
            break;
        case App::WM_PINREQ:
            evPinReq(wnd, static_cast<int>(wparam), static_cast<int>(lparam), opt);
            break;
        case WM_HOTKEY:
            evHotkey(wnd, static_cast<int>(wparam), opt);
            break;
        case WM_TIMER:
            if (wparam == App::TIMERID_AUTOPIN) {
                pendWnds.check(wnd, *opt);
            }
            break;
        case App::WM_QUEUEWINDOW:
            pendWnds.add(reinterpret_cast<HWND>(wparam));
            break;
        case App::WM_PINSTATUS:
            handlePinStatus(lparam);
            break;
        case WM_COMMAND:
            return handleCommand(wnd, wparam, *winCreMon, opt);
        case WM_ENDSESSION:
            if (wparam && opt) {
                opt->save();
            }
            return 0;
        case WM_DWMCOMPOSITIONCHANGED:
            app.dwm.wmDwmCompositionChanged();
            return 0;
        case WM_DPICHANGED:
            return handleDpiChanged(wnd, wparam, lparam, opt);
        default:
            if (msg == taskbarMsg) {
                app.trayIcon.create(app.smIcon, app.trayIconTip().c_str());
                break;
            }
            return DefWindowProc(wnd, msg, wparam, lparam);
    }
    return 0;
}

LRESULT MainWnd::handleCreate(HWND wnd, LPARAM lparam, std::unique_ptr<WindowCreationMonitor>& winCreMon, Options*& opt) {
    CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lparam);
    if (!cs || !cs->lpCreateParams) {
        LOG_ERROR(L"创建主窗口失败：无效的创建参数");
        return -1;
    }
    
    opt = static_cast<Options*>(cs->lpCreateParams);
    app.mainWnd = wnd;
    
    // 初始化窗口创建监控器
    winCreMon = std::make_unique<EventHookWindowCreationMonitor>();
    if (opt->autoPinOn && !winCreMon->init(wnd, App::WM_QUEUEWINDOW)) {
        LOG_WARNING(L"无法初始化窗口创建监控器，自动图钉功能将被禁用");
        // 优先从本地化文件获取错误消息
        std::wstring errorMsg = LanguageManager::getInstance().getString(L"hook_dll_error");
        if (errorMsg.empty() || errorMsg == L"hook_dll_error") {
            // 如果本地化失败，从资源文件加载作为回退
            WCHAR buffer[512];
            if (LoadStringW(app.resMod ? app.resMod : app.inst, IDS_ERR_HOOKDLL, buffer, 512) > 0) {
                errorMsg = buffer;
            }
        }
        // 只有在有有效错误消息时才显示
        if (!errorMsg.empty()) {
            Foundation::ErrorHandler::warning(wnd, errorMsg.c_str());
        }
        opt->autoPinOn = false;
    }

    initializeIcons(opt);
    
    if (!setupTrayIcon(wnd)) {
        LOG_ERROR(L"设置托盘图标失败");
        return -1;
    }
    
    setupHotkeys(wnd, opt);
    
    if (opt->autoPinOn) {
        SetTimer(wnd, App::TIMERID_AUTOPIN, opt->autoPinDelay.value, nullptr);
    }

    initializeDpiSettings(wnd, opt);
    
    // 初始化图钉计数 - 计算已存在的图钉窗口数量
    int pinCount = 0;
    HWND pin = nullptr;
    while ((pin = FindWindowEx(nullptr, pin, PinWnd::className, nullptr)) != nullptr) {
        pinCount++;
    }
    app.pinsUsed = pinCount;
    
    // 更新托盘图标提示
    app.trayIcon.setTip(app.trayIconTip().c_str());
    
    return 0;
}

LRESULT MainWnd::handleDestroy(HWND wnd, std::unique_ptr<WindowCreationMonitor>& winCreMon, Options* opt) {
    app.mainWnd = nullptr;

    // 首先清理托盘图标，确保它被正确移除
    app.trayIcon.destroy();

    if (winCreMon) {
        winCreMon->term();
        winCreMon.reset();
    }

    SendMessage(wnd, WM_COMMAND, CM_REMOVEPINS, 0);

    // 清理所有定时器 - 确保完整清理
    KillTimer(wnd, App::TIMERID_AUTOPIN);
    // 清理可能存在的其他定时器ID
    for (int timerId = 1; timerId <= 10; ++timerId) {
        KillTimer(wnd, timerId);
    }

    if (opt) {
        opt->hotEnterPin.unset(wnd);
        opt->hotTogglePin.unset(wnd);
    }

    // 确保关闭关于对话框
    if (app.aboutDlg) {
        DestroyWindow(app.aboutDlg);
        app.aboutDlg = nullptr;
    }

    // 确保关闭选项对话框
    if (app.optionsDlg) {
        DestroyWindow(app.optionsDlg);
        app.optionsDlg = nullptr;
    }

    // 确保关闭图钉层窗口
    if (app.layerWnd) {
        DestroyWindow(app.layerWnd);
        app.layerWnd = nullptr;
    }

    PostQuitMessage(0);
    return 0;
}

void MainWnd::initializeIcons(Options* opt) {
    std::wstring moduleDir = Foundation::FileUtils::getDirPath(Foundation::FileUtils::getModulePath(app.inst));
    
    // 根据系统DPI计算合适的图标尺寸
    int systemDpi = Graphics::DpiManager::getSystemDpi();
    int iconSize = static_cast<int>(16 * systemDpi / 96.0 + 0.5); // 四舍五入
    
    // 确保图标尺寸在合理范围内
    iconSize = (iconSize > 16) ? iconSize : 16;  // max(16, iconSize)
    iconSize = (iconSize < 32) ? iconSize : 32;  // min(iconSize, 32)
    
    // 优先使用配置文件中指定的图钉图像作为托盘图标
    std::wstring trayImagePath;
    if (opt && !opt->pinImagePath.empty()) {
        // 如果是相对路径，则相对于程序目录
        if (PathIsRelativeW(opt->pinImagePath.c_str())) {
            trayImagePath = moduleDir + opt->pinImagePath;
        } else {
            trayImagePath = opt->pinImagePath;
        }
        // 使用通用图像加载函数，支持所有stb_image支持的格式
        app.smIcon = Graphics::Drawing::Image::LoadImageAsIcon(trayImagePath.c_str(), iconSize, iconSize);
    }
    
    // 如果配置的图像加载失败，尝试加载默认的TinyPin.png
    if (!app.smIcon) {
        std::wstring defaultPngPath = moduleDir + L"assets\\images\\TinyPin.png";
        app.smIcon = Graphics::Drawing::Image::LoadImageAsIcon(defaultPngPath.c_str(), iconSize, iconSize);
    }
    
    // 如果TinyPin.png也加载失败，尝试加载app.png
    if (!app.smIcon) {
        std::wstring appPngPath = moduleDir + L"resources\\app.png";
        app.smIcon = Graphics::Drawing::Image::LoadImageAsIcon(appPngPath.c_str(), iconSize, iconSize);
    }
    
    // 如果PNG都加载失败，回退到ICO格式
    if (!app.smIcon) {
        app.smIcon = static_cast<HICON>(LoadImage(app.inst, MAKEINTRESOURCE(IDI_APP), 
                                                IMAGE_ICON, iconSize, iconSize, LR_DEFAULTCOLOR));
        if (!app.smIcon) {
            app.smIcon = LoadIcon(app.inst, MAKEINTRESOURCE(IDI_APP));
        }
    }
}

namespace {
    // 获取本地化错误消息的辅助函数
    std::wstring getLocalizedErrorMessage(const std::wstring& key, UINT resourceId) {
        std::wstring errorMsg = LanguageManager::getInstance().getString(key);
        if (errorMsg.empty() || errorMsg == key) {
            // 如果本地化失败，从资源文件加载作为回退
            WCHAR buffer[512];
            if (LoadStringW(app.resMod ? app.resMod : app.inst, resourceId, buffer, 512) > 0) {
                errorMsg = buffer;
            }
        }
        return errorMsg;
    }
}

bool MainWnd::setupTrayIcon(HWND wnd) {
    if (!app.trayIcon.setWnd(wnd)) {
        std::wstring errorMsg = getLocalizedErrorMessage(L"tray_set_window_error", IDS_ERR_TRAYSETWND);
        if (!errorMsg.empty()) {
            Foundation::ErrorHandler::error(wnd, errorMsg.c_str());
        }
        return false;
    }

    if (!app.trayIcon.create(app.smIcon, app.trayIconTip().c_str())) {
        std::wstring errorMsg = getLocalizedErrorMessage(L"tray_create_error", IDS_ERR_TRAYCREATE);
        if (!errorMsg.empty()) {
            Foundation::ErrorHandler::warning(wnd, errorMsg.c_str());
        }
        return false;
    }

    return true;
}

void MainWnd::setupHotkeys(HWND wnd, Options* opt) {
    if (!opt->hotkeysOn) return;
    
    if (!opt->hotEnterPin.set(wnd) || !opt->hotTogglePin.set(wnd)) {
        std::wstring errorMsg = getLocalizedErrorMessage(L"hotkeys_set_error", IDS_ERR_HOTKEYSSET);
        if (!errorMsg.empty()) {
            Foundation::ErrorHandler::Handler::show(wnd, 
                Foundation::ErrorHandler::ErrorLevel::Warning, errorMsg.c_str());
        }
    }
}

void MainWnd::initializeDpiSettings(HWND wnd, Options* opt) {
    const int dpi = Graphics::DpiManager::getDpiForWindow(wnd);
    app.pinShape.initShapeForDpi(dpi);
    app.pinShape.initImageForDpi(dpi);
}



void MainWnd::handlePinStatus(LPARAM lparam) {
    app.pinsUsed += lparam ? 1 : -1;
    
    if (app.aboutDlg) {
        SendMessage(app.aboutDlg, App::WM_PINSTATUS, 0, 0);
    }
    app.trayIcon.setTip(app.trayIconTip().c_str());
}

void MainWnd::updateTrayIcon() {
    extern Options opt;
    
    // 重新初始化图标
    if (app.smIcon) {
        DestroyIcon(app.smIcon);
        app.smIcon = nullptr;
    }
    
    // 重新加载图标
    initializeIcons(&opt);
    
    // 更新托盘图标
    if (app.smIcon) {
        app.trayIcon.setIcon(app.smIcon);
    }
    
    // 重新初始化图钉形状和图像
    int currentDpi = Graphics::DpiManager::getDpiForWindow(app.mainWnd);
    app.pinShape.initImageForDpi(currentDpi);
    app.pinShape.initShapeForDpi(currentDpi);
    
    // 更新所有现有的图钉窗口
    EnumWindows([](HWND wnd, LPARAM) -> BOOL {
        WCHAR className[Constants::MAX_CLASSNAME_LEN];
    if (GetClassName(wnd, className, Constants::MAX_CLASSNAME_LEN) > 0) {
            if (wcscmp(className, PinWnd::className) == 0) {
                InvalidateRect(wnd, nullptr, TRUE);
            }
        }
        return TRUE;
    }, 0);
}

LRESULT MainWnd::handleCommand(HWND wnd, WPARAM wparam, WindowCreationMonitor& winCreMon, Options* opt) {
    switch (LOWORD(wparam)) {
        case CM_ABOUT:
            showAboutDialog();
            break;
        case CM_NEWPIN: 
            cmNewPin(wnd);
            break;
        case CM_REMOVEPINS:
            cmRemovePins(wnd);
            break;
        case CM_OPTIONS:
            cmOptions(wnd, winCreMon, opt);
            break;
        case CM_CLOSE:
            // 不需要在这里保存设置，main.cpp的消息循环结束后会统一保存
            DestroyWindow(wnd);
            break;
    }
    return 0;
}

LRESULT MainWnd::handleDpiChanged(HWND wnd, WPARAM wparam, LPARAM lparam, Options* opt) {
    const int newDpi = HIWORD(wparam);
    RECT* newRect = reinterpret_cast<RECT*>(lparam);
    
    if (newRect) {
        SetWindowPos(wnd, nullptr, 
            newRect->left, newRect->top,
            newRect->right - newRect->left,
            newRect->bottom - newRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
    
    app.pinShape.initShapeForDpi(newDpi);
    app.pinShape.initImageForDpi(newDpi);
    
    // 重新创建托盘图标（使用默认图标）
    app.trayIcon.create(app.smIcon, app.trayIconTip().c_str());
    
    // 通知所有图钉窗口更新DPI设置
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        // 直接获取窗口信息，无需使用WindowInfo结构体
        std::wstring className = Window::getWindowClassName(hwnd);
        std::wstring windowText = Window::getWindowText(hwnd);
        if (wcscmp(className.c_str(), PinWnd::className) == 0) {
            SendMessage(hwnd, WM_DPICHANGED, MAKEWPARAM(lParam, lParam), 0);
        }
        return TRUE;
    }, static_cast<LPARAM>(newDpi));
    
    return 0;
}

void MainWnd::showAboutDialog() {
    if (app.aboutDlg) {
        SetForegroundWindow(app.aboutDlg);
    } else {
        app.aboutDlg = Util::Res::CreateLocalizedDialog(IDD_ABOUT, nullptr, AboutDlg::proc);
        if (app.aboutDlg) {
            ShowWindow(app.aboutDlg, SW_SHOW);
        }
    }
}

void MainWnd::evHotkey(HWND wnd, int idHotKey, Options* opt) {
    if (app.layerWnd) return;

    switch (idHotKey) {
    case App::HOTID_ENTERPINMODE:
        PostMessage(wnd, WM_COMMAND, CM_NEWPIN, 0);
        break;
    case App::HOTID_TOGGLEPIN:
        Pin::PinManager::togglePin(wnd, GetForegroundWindow(), opt->trackRate.value);
        break;
    }
}

void MainWnd::evPinReq(HWND wnd, int x, int y, Options* opt) {
    const POINT pt = {x, y};
    HWND hitWnd = Window::getTopParent(WindowFromPoint(pt));
    
    if (!hitWnd) {
        LOG_WARNING(L"在指定位置未找到窗口");
    }
    
    Pin::PinManager::pinWindow(wnd, hitWnd, opt->trackRate.value);
}

void MainWnd::evTrayIcon(HWND wnd, WPARAM id, LPARAM msg, Options* opt) {
    static bool gotLButtonDblClk = false;

    if (id != 0) return;

    switch (msg) {
        case WM_LBUTTONUP:
            if (!opt->dblClkTray || gotLButtonDblClk) {
                SendMessage(wnd, WM_COMMAND, CM_NEWPIN, 0);
                gotLButtonDblClk = false;
            }
            break;
        case WM_LBUTTONDBLCLK:
            gotLButtonDblClk = true;
            break;
        case WM_RBUTTONDOWN:
            showTrayContextMenu(wnd);
            break;
    }
}

void MainWnd::showTrayContextMenu(HWND wnd) {
    SetForegroundWindow(wnd);
    
    HMENU dummy = Util::Res::LoadLocalizedMenu(IDM_TRAY);
    HMENU menu = GetSubMenu(dummy, 0);
    SetMenuDefaultItem(menu, CM_NEWPIN, FALSE);

    // 本地化菜单文本
    Util::Dialog::localizeMenu(menu, L"tray");

    TrayMenuDecorations tmd(menu);

    POINT pt;
    GetCursorPos(&pt);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, wnd, nullptr);
    DestroyMenu(dummy);
    SendMessage(wnd, WM_NULL, 0, 0);
}

void MainWnd::cmNewPin(HWND wnd) {
    if (app.layerWnd) {
        return;
    }

    const POINT layerWndPos = Platform::SystemInfo::isWin8orGreater() ? 
        POINT{Constants::DEFAULT_LAYER_WND_POS, Constants::DEFAULT_LAYER_WND_POS} : 
        POINT{0, 0};
    
    app.layerWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        PinLayerWnd::className,
        L"DeskPin",
        WS_POPUP | WS_VISIBLE,
        layerWndPos.x, layerWndPos.y, 1, 1,
        wnd, nullptr, app.inst, nullptr);

    if (!app.layerWnd) {
        LOG_ERROR(L"创建图钉层窗口失败");
        return;
    }

    ShowWindow(app.layerWnd, SW_SHOW);

    POINT pt;
    GetCursorPos(&pt);
    SetCursorPos(layerWndPos.x, layerWndPos.y);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    SetCursorPos(pt.x, pt.y);
}

void MainWnd::cmRemovePins(HWND wnd) {
    int count = 0;
    HWND pin;
    while ((pin = FindWindow(PinWnd::className, nullptr)) != nullptr) {
        DestroyWindow(pin);
        count++;
    }
}

void MainWnd::cmOptions(HWND wnd, WindowCreationMonitor& winCreMon, Options* opt) {
    static bool isOptDlgOn = false;
    if (isOptDlgOn) return;
    
    struct DialogSentinel {
        bool& flag;
        explicit DialogSentinel(bool& f) : flag(f) { flag = true; }
        ~DialogSentinel() { flag = false; }
    } sentinel(isOptDlgOn);

    constexpr int PAGE_COUNT = 4;
    PROPSHEETPAGE psp[PAGE_COUNT];
    PROPSHEETHEADER psh;
    
    static const int dlgIds[PAGE_COUNT] = {
        IDD_OPT_PINS, IDD_OPT_AUTOPIN, IDD_OPT_HOTKEYS, IDD_OPT_LANG
    };
    
    static const DLGPROC dlgProcs[PAGE_COUNT] = { 
        reinterpret_cast<DLGPROC>(OptPins::dlgProc), 
        reinterpret_cast<DLGPROC>(OptAutoPin::dlgProc), 
        reinterpret_cast<DLGPROC>(OptHotKeys::dlgProc), 
        reinterpret_cast<DLGPROC>(OptLang::dlgProc)
    };

    OptionsPropSheetData data = { *opt, winCreMon };
    Util::Res::ResStr capStr(IDS_OPTIONSTITLE);
    OptionsDlg::buildOptPropSheet(psh, psp, const_cast<int*>(dlgIds), const_cast<DLGPROC*>(dlgProcs), PAGE_COUNT, 0, data, capStr);

    const INT_PTR result = PropertySheet(&psh);
    if (result == -1) {
        Foundation::ErrorHandler::Handler::showSystemError(wnd, GetLastError(), 
            LanguageManager::getInstance().getString(L"dialog_create_error").c_str());
    }

    app.trayIcon.setTip(app.trayIconTip().c_str());
}
