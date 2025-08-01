#include "core/stdafx.h"
#include "core/application.h"
#include "ui/main_window.h"
#include "pin/pin_window.h"
#include "pin/pin_layer_window.h"
#include "resource.h"
#include "system/language_manager.h"

LPCWSTR App::APPNAME = L"TinyPin";

bool App::loadResMod(const std::wstring& file, HWND msgParent) {
    freeResMod();

    if (file.empty()) return true;

    std::wstring modulePath = Foundation::FileUtils::getModulePath(inst);
    std::wstring s = Foundation::FileUtils::getDirPath(modulePath);
    if (!s.empty()) {
        s += file;
        resMod = LoadLibrary(s.c_str());
    }

    if (!resMod) {
        WCHAR buf[MAX_PATH + Constants::SMALL_BUFFER_SIZE];
        std::wstring errorTemplate = LanguageManager::getInstance().getString(L"language_file_load_failed");
        wsprintf(buf, errorTemplate.c_str(), file.c_str());
        Foundation::ErrorHandler::error(msgParent, buf);
    }

    return resMod != 0;
}

void App::freeResMod() {
    if (resMod) {
        FreeLibrary(resMod);
        resMod = nullptr;
    }
}

bool App::initComctl() {
    INITCOMMONCONTROLSEX iccx = { sizeof(iccx), ICC_WIN95_CLASSES };
    bool ret = !!InitCommonControlsEx(&iccx);
    if (!ret) {
        Foundation::ErrorHandler::error(nullptr, LanguageManager::getInstance().getString(L"common_controls_init_error").c_str());
    }
    return ret;
}

bool App::chkPrevInst() {
    bool isRunning = prevInst.isRunning();
    if (isRunning) {
        // 显示已有实例运行的提示消息
        MessageBox(nullptr, LanguageManager::getInstance().getString(L"already_running").c_str(), 
                  APPNAME, MB_ICONINFORMATION);
        return false;
    }
    
    // 没有其他实例在运行，可以继续启动
    return true;
}

bool App::regWndCls() {
    return MainWnd::registerClass() && 
           PinWnd::registerClass() && 
           PinLayerWnd::registerClass();
}

bool App::createMainWnd(Options& opt) {
    CreateWindow(MainWnd::className, App::APPNAME, 
        WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, app.inst, &opt);
    return app.mainWnd != nullptr;
}

std::wstring App::trayIconTip() {
    WCHAR s[Constants::SMALL_BUFFER_SIZE];
    std::wstring localizedPin = LanguageManager::getInstance().getControlText(IDS_TRAYTIP);
    wsprintf(s, L"%s - %s: %d", APPNAME, localizedPin.c_str(), pinsUsed);
    return s;
}
