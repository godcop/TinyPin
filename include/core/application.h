#pragma once

#include "pin/pin_shape.h"
#include "ui/tray_icon.h"
#include "platform/library_manager.h"
#include "platform/process_manager.h"
#include "options/options.h"


// 桌面窗口管理器管理。
// 动态加载dwmapi.dll并提供对缓存API状态的访问。
// 主窗口需要调用此功能来处理广播消息。
//
class Dwm {
    using DwmIsCompositionEnabled_Ptr = HRESULT (WINAPI*)(BOOL*);
public:
    Dwm() {
        dll = libraryManager_.loadLibrary(L"dwmapi");
        if (dll) {
            DwmIsCompositionEnabled_ = libraryManager_.getProcAddress<DwmIsCompositionEnabled_Ptr>(dll, "DwmIsCompositionEnabled");
        }
        wmDwmCompositionChanged();
    }
    ~Dwm() = default;
    Dwm(const Dwm&) = delete;
    Dwm& operator=(const Dwm&) = delete;
    Dwm(Dwm&& other) noexcept = default;
    Dwm& operator=(Dwm&& other) noexcept = default;
    bool isCompositionEnabled() const { return cachedIsCompositionEnabled; }
    void wmDwmCompositionChanged() {
        BOOL b;
        cachedIsCompositionEnabled = dll && DwmIsCompositionEnabled_ && SUCCEEDED(DwmIsCompositionEnabled_(&b)) && b;
    }
private:
    Platform::LibraryManager libraryManager_;
    HMODULE dll = nullptr;
    DwmIsCompositionEnabled_Ptr DwmIsCompositionEnabled_ = nullptr;
    bool cachedIsCompositionEnabled = false;
};


// 主应用程序状态。
// 所有翻译单元都可以使用单个全局实例。
//
struct App {
    Platform::PrevInstance prevInst{L"TinyPinRunning"};
    HWND mainWnd = nullptr, aboutDlg = nullptr, optionsDlg = nullptr, layerWnd = nullptr;
    HINSTANCE inst = nullptr;
    HMODULE resMod = nullptr;
    PinShape pinShape;
    HICON smIcon = nullptr;
    int pinsUsed = 0;
    int optionPage = 0;
    TrayIcon trayIcon{WM_TRAYICON, 0};
    Dwm dwm;
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    static LPCWSTR APPNAME;
    enum {
        WM_TRAYICON = WM_APP,
        WM_PINSTATUS,
        WM_PINREQ,
        WM_QUEUEWINDOW,
        WM_CMDLINE_OPTION,
        WM_PIN_ASSIGNWND = WM_USER,
        WM_PIN_RESETTIMER,
        WM_PIN_GETPINNEDWND,
        HOTID_ENTERPINMODE = 0,
        HOTID_TOGGLEPIN = 1,
        TIMERID_AUTOPIN = 1,
    };
    App() = default;
    ~App() { 
        // 清理托盘图标
        trayIcon.destroy();
        
        // 清理资源模块
        freeResMod(); 
        
        // 清理图标资源
        if (smIcon) { 
            DestroyIcon(smIcon); 
            smIcon = nullptr; 
        } 
        
        // 确保所有窗口都被关闭
        if (aboutDlg) {
            DestroyWindow(aboutDlg);
            aboutDlg = nullptr;
        }
        if (optionsDlg) {
            DestroyWindow(optionsDlg);
            optionsDlg = nullptr;
        }
        if (layerWnd) {
            DestroyWindow(layerWnd);
            layerWnd = nullptr;
        }
    }
    bool loadResMod(const std::wstring& file, HWND msgParent);
    void freeResMod();
    bool initComctl();
    bool chkPrevInst();
    bool regWndCls();
    bool createMainWnd(Options& opt);
    std::wstring trayIconTip();
private:
    Platform::LibraryManager libraryManager_;
};
