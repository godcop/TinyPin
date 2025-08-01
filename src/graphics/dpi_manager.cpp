#include "core/stdafx.h"
#include "graphics/dpi_manager.h"
#include "platform/library_manager.h"

namespace Graphics {
namespace DpiManager {

namespace {
    // 静态变量用于缓存DPI相关的函数指针和状态
    Platform::LibraryManager libraryManager_;
    
    using SetProcessDpiAwarenessContext_t = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    using SetProcessDpiAwareness_t = HRESULT(WINAPI*)(int);
    using GetDpiForWindow_t = UINT(WINAPI*)(HWND);
    using GetDpiForSystem_t = UINT(WINAPI*)(void);
    
    SetProcessDpiAwarenessContext_t SetProcessDpiAwarenessContext_ = nullptr;
    SetProcessDpiAwareness_t SetProcessDpiAwareness_ = nullptr;
    GetDpiForWindow_t GetDpiForWindow_ = nullptr;
    GetDpiForSystem_t GetDpiForSystem_ = nullptr;
    
    HMODULE user32Dll_ = nullptr;
    HMODULE shcoreDll_ = nullptr;
    int systemDpi_ = 96;
    bool initialized_ = false;
    
    void loadDpiApis() {
        if (initialized_) return;
        
        user32Dll_ = libraryManager_.loadLibrary(L"user32.dll");
        shcoreDll_ = libraryManager_.loadLibrary(L"shcore.dll");
        
        if (user32Dll_) {
            SetProcessDpiAwarenessContext_ = libraryManager_.getProcAddress<SetProcessDpiAwarenessContext_t>(
                user32Dll_, "SetProcessDpiAwarenessContext");
            GetDpiForWindow_ = libraryManager_.getProcAddress<GetDpiForWindow_t>(
                user32Dll_, "GetDpiForWindow");
            GetDpiForSystem_ = libraryManager_.getProcAddress<GetDpiForSystem_t>(
                user32Dll_, "GetDpiForSystem");
        }
        
        if (shcoreDll_) {
            SetProcessDpiAwareness_ = libraryManager_.getProcAddress<SetProcessDpiAwareness_t>(
                shcoreDll_, "SetProcessDpiAwareness");
        }
        
        initialized_ = true;
    }
}

bool initDpiAwareness() {
    loadDpiApis();
    
    // 尝试设置DPI感知（Windows 10 1703+）
    if (SetProcessDpiAwarenessContext_) {
        if (SetProcessDpiAwarenessContext_(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            systemDpi_ = getSystemDpi();
            return true;
        }
    }
    
    // 回退到旧API（Windows 8.1+）
    if (SetProcessDpiAwareness_) {
        if (SUCCEEDED(SetProcessDpiAwareness_(2))) { // PROCESS_PER_MONITOR_DPI_AWARE = 2
            systemDpi_ = getSystemDpi();
            return true;
        }
    }
    
    // 回退到基本DPI感知
    SetProcessDPIAware();
    systemDpi_ = getSystemDpi();
    return true;
}

int getDpiForWindow(HWND hwnd) {
    loadDpiApis();
    
    if (GetDpiForWindow_ && hwnd) {
        return GetDpiForWindow_(hwnd);
    }
    return getSystemDpi();
}

int getSystemDpi() {
    loadDpiApis();
    
    if (GetDpiForSystem_) {
        return GetDpiForSystem_();
    }
    
    HDC hdc = GetDC(nullptr);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(nullptr, hdc);
    return dpi;
}

double getDpiScale(HWND hwnd) {
    int dpi = hwnd ? getDpiForWindow(hwnd) : getSystemDpi();
    return static_cast<double>(dpi) / 96.0;
}

void scaleSizeForDpi(SIZE& size, int dpi) {
    double scale = static_cast<double>(dpi) / 96.0;
    size.cx = static_cast<LONG>(size.cx * scale);
    size.cy = static_cast<LONG>(size.cy * scale);
}

void scaleRectForDpi(RECT& rect, int dpi) {
    double scale = static_cast<double>(dpi) / 96.0;
    rect.left = static_cast<LONG>(rect.left * scale);
    rect.top = static_cast<LONG>(rect.top * scale);
    rect.right = static_cast<LONG>(rect.right * scale);
    rect.bottom = static_cast<LONG>(rect.bottom * scale);
}

} // namespace DpiManager
} // namespace Graphics