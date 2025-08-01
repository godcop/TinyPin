#include "core/stdafx.h"
#include "window/window_helper.h"
#include "window/window_cache.h"  // 添加窗口缓存支持
#include "core/application.h"
#include "resource.h"  // 包含资源ID定义

// 安全获取窗口文本（公共函数）
std::wstring Window::getWindowText(HWND wnd) {
    if (!wnd) return L"";
    WCHAR windowText[Constants::MAX_WINDOWTEXT_LEN] = {0};
    GetWindowText(wnd, windowText, Constants::MAX_WINDOWTEXT_LEN);
    return std::wstring(windowText);
}

// 安全获取窗口类名（公共函数）
std::wstring Window::getWindowClassName(HWND wnd) {
    if (!wnd) return L"";
    WCHAR className[Constants::MAX_CLASSNAME_LEN] = {0};
    GetClassName(wnd, className, Constants::MAX_CLASSNAME_LEN);
    return std::wstring(className);
}

bool Window::isWndRectEmpty(HWND wnd)
{
    RECT rc;
    // 使用缓存的API减少系统调用
    return Window::Cached::getWindowRect(wnd, rc) && IsRectEmpty(&rc);
}

bool Window::isChild(HWND wnd)
{
    // 使用缓存的API减少系统调用
    return (Window::Cached::getWindowLong(wnd, GWL_STYLE) & WS_CHILD) != 0;
}

HWND Window::getNonChildParent(HWND wnd)
{
    while (isChild(wnd))
        wnd = GetParent(wnd);

    return wnd;
}

HWND Window::getTopParent(HWND wnd /*, bool mustBeVisible*/)
{
    // ------------------------------------------------------
    // 注意：'mustBeVisible'目前未使用
    // ------------------------------------------------------
    // 通过设置'mustBeVisible'标志，
    // 对最终父窗口搜索应用更多约束：
    //
    // #1：当父窗口不可见时停止（例如shell的显示属性
    //     它有一个来自RunDll的隐藏父窗口）
    // #2：当父窗口矩形为空时停止。这是VCL应用程序的情况
    //     其中创建了一个父窗口（TApplication类）作为代理
    //     用于应用程序功能（它有WS_VISIBLE，但宽度/高度为0）
    //
    // 图钉引擎处理不可见的窗口（通过代理机制）。
    // 'mustBeVisible'主要用于用户交互
    // （例如查看窗口的属性）
    // ------------------------------------------------------

    // 沿着窗口链向上查找最终的顶级窗口
    // 对于WS_CHILD窗口使用GetParent()
    // 对于其余的使用GetWindow(GW_OWNER)
    //
    for (;;) {
        HWND parent = isChild(wnd)
            ? GetParent(wnd)
            : GetWindow(wnd, GW_OWNER);

        if (!parent || parent == wnd) break;
        //if (mustBeVisible && !IsWindowVisible(parent) || IsWndRectEmpty(parent))
        //  break;
        wnd = parent;
    }

    return wnd;
}

bool Window::isTopMost(HWND wnd)
{
    // 使用缓存的API减少系统调用
    return (Window::Cached::getWindowLong(wnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}

bool Window::getVisibleWindowRect(HWND wnd, RECT& rect)
{
    if (!wnd || !IsWindow(wnd)) {
        return false;
    }
    
    // 首先尝试使用DWM API获取实际可视边框
    // 这需要动态加载dwmapi.dll，因为不是所有系统都支持
    typedef HRESULT (WINAPI *DwmGetWindowAttributeFunc)(HWND, DWORD, PVOID, DWORD);
    
    HMODULE dwmapi = LoadLibrary(L"dwmapi.dll");
    if (dwmapi) {
        DwmGetWindowAttributeFunc dwmGetWindowAttribute = 
            (DwmGetWindowAttributeFunc)GetProcAddress(dwmapi, "DwmGetWindowAttribute");
        
        if (dwmGetWindowAttribute) {
            // DWMWA_EXTENDED_FRAME_BOUNDS = 9
            HRESULT hr = dwmGetWindowAttribute(wnd, 9, &rect, sizeof(RECT));
            FreeLibrary(dwmapi);
            
            if (SUCCEEDED(hr)) {
                return true;
            }
        } else {
            FreeLibrary(dwmapi);
        }
    }
    
    // 回退到标准GetWindowRect
    return GetWindowRect(wnd, &rect) != FALSE;
}

BOOL Window::moveWindow(HWND wnd, const RECT& rc, BOOL repaint)
{
    return MoveWindow(wnd, rc.left, rc.top, 
        rc.right-rc.left, rc.bottom-rc.top, repaint);
}

bool Window::psChanged(HWND page)
{
    return !!PropSheet_Changed(GetParent(page), page);
}

// WndHelper实现
Window::WndHelper::WndHelper(HWND hwnd) : m_hwnd(hwnd) {}

std::wstring Window::WndHelper::getText() const {
    if (!m_hwnd) return L"";
    
    int len = GetWindowTextLength(m_hwnd);
    if (len <= 0) return L"";
    
    std::vector<wchar_t> buffer(len + 1);
    GetWindowText(m_hwnd, &buffer[0], len + 1);
    return std::wstring(&buffer[0]);
}

void Window::WndHelper::setText(const std::wstring& text) const {
    if (m_hwnd) {
        SetWindowText(m_hwnd, text.c_str());
    }
}

std::wstring Window::WndHelper::getClassName() const {
    // 使用缓存的API减少系统调用
    return Window::Cached::getWindowClassName(m_hwnd);
}

LONG Window::WndHelper::getStyle() const {
    // 使用缓存的API减少系统调用
    return m_hwnd ? Window::Cached::getWindowLong(m_hwnd, GWL_STYLE) : 0;
}

LONG Window::WndHelper::getExStyle() const {
    // 使用缓存的API减少系统调用
    return m_hwnd ? Window::Cached::getWindowLong(m_hwnd, GWL_EXSTYLE) : 0;
}

void Window::WndHelper::modifyExStyle(LONG exStyleToRemove, LONG exStyleToAdd) const {
    if (m_hwnd) {
        // 使用缓存的API减少系统调用
        LONG exStyle = Window::Cached::getWindowLong(m_hwnd, GWL_EXSTYLE);
        exStyle &= ~exStyleToRemove;
        exStyle |= exStyleToAdd;
        SetWindowLong(m_hwnd, GWL_EXSTYLE, exStyle);
        // 窗口样式改变后，使缓存失效
        Window::WindowCache::getInstance().invalidateWindow(m_hwnd);
    }
}

void Window::WndHelper::update() const {
    if (m_hwnd) {
        UpdateWindow(m_hwnd);
    }
}

// WindowRegistrar 实现
ATOM Window::WindowRegistrar::registerWindowClass(const WindowClassConfig& config)
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = config.style;
    wcex.lpfnWndProc = config.wndProc;
    wcex.cbClsExtra = config.cbClsExtra;
    wcex.cbWndExtra = config.cbWndExtra;
    wcex.hInstance = app.inst;
    wcex.hIcon = config.icon;
    wcex.hCursor = config.cursor ? config.cursor : LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = config.background;
    wcex.lpszMenuName = config.menuName;
    wcex.lpszClassName = config.className;
    wcex.hIconSm = config.iconSm;
    
    return RegisterClassExW(&wcex);
}

ATOM Window::WindowRegistrar::registerSimpleClass(LPCWSTR className, WNDPROC wndProc, 
                                                  HCURSOR cursor, int cbWndExtra)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = app.inst;
    wc.hCursor = cursor ? cursor : LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = className;
    wc.cbWndExtra = cbWndExtra;
    
    return RegisterClass(&wc);
}

void Window::enableGroup(HWND wnd, int id, bool mode)
{
    HWND group = GetDlgItem(wnd, id);
    if (!group) return;
    
    RECT rc1;
    GetWindowRect(group, &rc1);
    
    // 定义传递给回调函数的数据结构
    struct EnableData {
        RECT groupRect;
        bool enable;
        HWND groupWnd;
        HWND parentWnd;
    };
    
    EnableData data = { rc1, mode, group, wnd };
    
    // 枚举所有子控件，启用/禁用在组框内的控件
    EnumChildWindows(wnd, [](HWND child, LPARAM lParam) -> BOOL {
        EnableData* data = reinterpret_cast<EnableData*>(lParam);
        
        // 跳过组框本身
        if (child == data->groupWnd) {
            return TRUE;
        }
        
        // 获取控件ID，跳过控制复选框
        int childId = GetDlgCtrlID(child);
        
        // 跳过自动功能的启用复选框和热键功能的启用复选框
        // 这些复选框应该始终保持可用状态，因为它们是用来控制整个组的
        if (childId == IDC_AUTOPIN_ON || childId == IDC_HOTKEYS_ON) {
            return TRUE;
        }
        
        RECT childRect;
        GetWindowRect(child, &childRect);
        
        // 检查子控件是否在组框内
        if (childRect.left >= data->groupRect.left &&
            childRect.top >= data->groupRect.top &&
            childRect.right <= data->groupRect.right &&
            childRect.bottom <= data->groupRect.bottom) {
            EnableWindow(child, data->enable);
        }
        
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data));
}