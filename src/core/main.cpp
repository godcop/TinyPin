#include "core/stdafx.h"
#include "pin/pin_shape.h"
#include "pin/pin_window.h"
#include "pin/pin_layer_window.h"
#include "options/options.h"
#include "core/application.h"
#include "resource.h"
#include "system/logger.h"
#include "system/language_manager.h"

// 启用视觉样式
#pragma comment(linker, "/manifestdependency:\""                               \
    "type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' " \
    "processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'" \
    "\"")

// 全局应用对象
App app;

// 全局选项对象
Options opt;


int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int)
{
    app.inst = inst;

    // 尽早初始化DPI感知
    Graphics::DpiManager::initDpiAwareness();

    // 初始化日志系统
    if (!Logger::getInstance().init()) {
        MessageBox(nullptr, LANG_MGR.getString(L"logging_init_failed").c_str(), App::APPNAME, MB_ICONERROR);
    }

    // 尽快加载设置
    opt.load();

    // 初始化语言管理器
    if (!LANG_MGR.initialize()) {
        LOG_WARNING(LANG_MGR.getString(L"language_manager_init_failed"));
    }

    // 设置语言
    if (opt.language.empty()) {
        LANG_MGR.autoSelectLanguage();
    } else {
        LANG_MGR.setLanguage(opt.language);
    }

    if (!app.chkPrevInst()) {
        return 0;
    }
    
    if (!app.initComctl()) {
        LOG_ERROR(LANG_MGR.getString(L"common_controls_init_failed"));
        return 0;
    }

    if (!app.regWndCls()) {
        LOG_ERROR(LANG_MGR.getString(L"window_class_register_failed"));
        Foundation::ErrorHandler::error(nullptr, LANG_MGR.getString(L"window_class_register_error").c_str());
        return 0;
    }
    
    if (!app.createMainWnd(opt)) {
        LOG_ERROR(LANG_MGR.getString(L"main_window_create_failed"));
        Foundation::ErrorHandler::error(nullptr, LANG_MGR.getString(L"window_class_register_error").c_str());
        return 0;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 确保设置被保存
    opt.save();
    
    // 确保托盘图标被移除
    app.trayIcon.destroy();
    
    // 清理所有可能残留的窗口
    HWND pin;
    while ((pin = FindWindow(PinWnd::className, nullptr)) != nullptr) {
        DestroyWindow(pin);
    }
    
    // 清理图钉层窗口
    while ((pin = FindWindow(PinLayerWnd::className, nullptr)) != nullptr) {
        DestroyWindow(pin);
    }
    
    return static_cast<int>(msg.wParam);
}
