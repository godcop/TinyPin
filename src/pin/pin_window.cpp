#include "core/stdafx.h"
#include "core/application.h"
#include "pin/pin_shape.h"
#include "pin/pin_window.h"
#include "window/window_cache.h"  // 添加窗口缓存支持
#include "resource.h"
#include "system/logger.h"
#include "system/language_manager.h"
#include <map>


LPCWSTR PinWnd::className = L"EFPinWnd";


ATOM PinWnd::registerClass()
{
    HCURSOR cursor = LoadCursor(app.inst, MAKEINTRESOURCE(IDC_REMOVEPIN));
    
    // 使用 WindowClassConfig 来设置图标
    Window::WindowClassConfig config = {};
    config.className = className;
    config.wndProc = proc;
    config.cursor = cursor;
    config.cbWndExtra = sizeof(void*);
    config.background = nullptr; // 不设置背景画刷，支持透明背景
    
    // 设置图标 - 使用 app.smIcon (PNG 图标)
    config.icon = app.smIcon;
    config.iconSm = app.smIcon;
    
    return Window::WindowRegistrar::registerWindowClass(config);
}


LRESULT CALLBACK PinWnd::proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_NCCREATE) {
        Data::create(wnd, app.mainWnd);
        return true;
    }
    else if (msg == WM_NCDESTROY) {
        Data::destroy(wnd);
        return 0;
    }
    Data* pd = Data::get(wnd);
    if (pd) {
        switch (msg) {
            case WM_CREATE:         return evCreate(wnd, *pd);
            case WM_DESTROY:        return evDestroy(wnd, *pd), 0;
            case WM_TIMER:          return evTimer(wnd, *pd, static_cast<UINT>(wparam)), 0;
            case WM_PAINT:          return evPaint(wnd, *pd), 0;
            case WM_LBUTTONDOWN:    return evLClick(wnd, *pd), 0;
            case WM_DPICHANGED:     return evDpiChanged(wnd, *pd, wparam, lparam), 0;
            case App::WM_PIN_RESETTIMER:   return evPinResetTimer(wnd, *pd, int(wparam)), 0;
            case App::WM_PIN_ASSIGNWND:    return evPinAssignWnd(wnd, *pd, HWND(wparam), int(lparam));
            case App::WM_PIN_GETPINNEDWND: return LRESULT(evGetPinnedWnd(wnd, *pd));
        }
    }
    return DefWindowProc(wnd, msg, wparam, lparam);
}


LRESULT PinWnd::evCreate(HWND wnd, Data& pd)
{
    // 发送'图钉已创建'通知
    PostMessage(pd.callbackWnd, App::WM_PINSTATUS, WPARAM(wnd), true);

    // 最初将图钉放置在屏幕中央；
    // 稍后它将被定位在被钉住窗口的标题栏上
    RECT rc;
    GetWindowRect(wnd, &rc);
    int wndW = rc.right - rc.left;
    int wndH = rc.bottom - rc.top;
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(wnd, nullptr, (scrW-wndW)/2, (scrH-wndH)/2, 0, 0,
        SWP_NOZORDER | SWP_NOSIZE);
    
    return 0;
}


void PinWnd::evDestroy(HWND wnd, Data& pd)
{
    if (pd.topMostWnd) {
        SetWindowPos(pd.topMostWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        pd.topMostWnd = nullptr;
        pd.proxyWnd = nullptr;
        pd.proxyMode = false;
    }

    // 发送'图钉已销毁'通知
    PostMessage(pd.callbackWnd, App::WM_PINSTATUS, WPARAM(wnd), false);
}


// 用于收集线程的所有非子窗口的辅助类。
//
class ThreadWnds {
public:
    ThreadWnds(HWND wnd) : appWnd(wnd) {}
    int count() const { return static_cast<int>(wndList.size()); }
    HWND operator[](int n) const { return wndList[n]; }

    bool collect()
    {
        DWORD threadId = GetWindowThreadProcessId(appWnd, nullptr);
        return !!EnumThreadWindows(threadId, (WNDENUMPROC)enumProc, LPARAM(this));
    }

protected:
    HWND appWnd;
    std::vector<HWND> wndList;

    static BOOL CALLBACK enumProc(HWND wnd, LPARAM param)
    {
        ThreadWnds& obj = *reinterpret_cast<ThreadWnds*>(param);
        if (wnd == obj.appWnd || GetWindow(wnd, GW_OWNER) == obj.appWnd)
            obj.wndList.push_back(wnd);
        return true;  // 继续枚举
    }

};


// - 获取可见的顶级窗口
// - 如果任何启用的窗口在任何禁用的窗口之后
//   获取最后一个启用的窗口；否则退出
// - 将所有禁用的窗口（从最底部的开始）
//   移动到最后一个启用的窗口之后
//
void PinWnd::fixPopupZOrder(HWND appWnd)
{
    // - 找到最可见的、禁用的、顶级窗口（窗口X）
    // - 找到所有非禁用的顶级窗口并将它们
    //   放置在窗口X之上

    // 获取所有非子窗口
    ThreadWnds threadWnds(appWnd);
    if (!threadWnds.collect())
        return;

    // 黑客方法：这里我假设EnumThreadWindows按照
    // z-order返回HWND（这在任何地方有文档记录吗？）

    // 如果任何禁用的窗口在任何启用的窗口之上，
    // 则需要重新排序。
    bool needReordering = false;
    for (int n = 1; n < threadWnds.count(); ++n) {
        if (!IsWindowEnabled(threadWnds[n-1]) && IsWindowEnabled(threadWnds[n])) {
            needReordering = true;
            break;
        }
    }

    if (!needReordering)
        return;

    // 找到最后一个启用的
    HWND lastEnabled = nullptr;
    for (int n = threadWnds.count()-1; n >= 0; --n) {
        if (IsWindowEnabled(threadWnds[n])) {
            lastEnabled = threadWnds[n];
            break;
        }
    }

    // 没有启用的？退出
    if (!lastEnabled)
        return;

    // 将所有禁用的（从最后一个开始）移动到最后一个启用的之后
    for (int n = threadWnds.count()-1; n >= 0; --n) {
        if (!IsWindowEnabled(threadWnds[n])) {
            SetWindowPos(threadWnds[n], lastEnabled, 0,0,0,0, 
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
        }
    }

}


void PinWnd::evTimer(HWND wnd, Data& pd, UINT id)
{
    if (id != 1) return;

    // 应用窗口是否仍然存在？
    if (!IsWindow(pd.topMostWnd)) {
        pd.topMostWnd = nullptr;
        pd.proxyWnd = nullptr;
        pd.proxyMode = false;
        DestroyWindow(wnd);
        return;
    }

    // 静态变量用于减少频繁的层级调整
    static std::map<HWND, DWORD> lastTopStyleCheck;
    static std::map<HWND, bool> lastMinimizedState;
    
    DWORD currentTick = GetTickCount();
    HWND targetWnd = pd.getPinOwner();
    if (!targetWnd) {
        targetWnd = pd.topMostWnd;
    }

    // 对于现代Windows应用，添加额外的状态检测
    if (Window::isModernWindowsApp(pd.topMostWnd)) {
        // 检查主窗口是否发生了状态变化（如最小化、恢复等）
        bool currentMinimized = IsIconic(pd.topMostWnd);
        bool lastMinimized = lastMinimizedState[pd.topMostWnd];
        
        if (currentMinimized != lastMinimized) {
            lastMinimizedState[pd.topMostWnd] = currentMinimized;
            if (currentMinimized) {
                // 现代应用窗口最小化，图钉将隐藏
            } else {
                // 现代应用窗口恢复，图钉将重新显示并重新定位
                // 窗口恢复时，可能需要重新查找代理窗口
                if (pd.proxyMode) {
                    Data* pdMutable = &pd;
                    pdMutable->proxyWnd = nullptr; // 清除旧的代理窗口
                }
            }
        }
        
        // 检查代理窗口是否仍然有效
        if (pd.proxyMode && pd.proxyWnd) {
            if (!IsWindow(pd.proxyWnd) || !IsWindowVisible(pd.proxyWnd)) {
                Data* pdMutable = &pd;
                pdMutable->proxyWnd = nullptr;
            }
        }
    }

    // 处理代理模式
    if (pd.proxyMode
        && (!pd.proxyWnd || !IsWindowVisible(pd.proxyWnd))
        && !selectProxy(wnd, pd)) {
        return;
    }

    // 检查可见性
    if (!fixVisible(wnd, pd)) {
        return;
    }

    if (pd.proxyMode && !IsWindowEnabled(pd.topMostWnd)) {
        fixPopupZOrder(pd.topMostWnd);
    }

    // 减少频繁的层级检查 - 每500ms检查一次即可
    bool needTopStyleCheck = (currentTick - lastTopStyleCheck[targetWnd]) > Constants::TOP_STYLE_CHECK_INTERVAL;
    if (needTopStyleCheck) {
        lastTopStyleCheck[targetWnd] = currentTick;
        
        // 只在必要时调用fixTopStyle，避免频繁的层级切换
        LONG targetExStyle = GetWindowLong(targetWnd, GWL_EXSTYLE);
        if (!(targetExStyle & WS_EX_TOPMOST)) {
            fixTopStyle(targetWnd, pd);
        }
    }
    
    // 更新图钉位置
    placeOnCaption(wnd, pd);
    
    // 优化的层级管理策略 - 减少不必要的SetWindowPos调用
    LONG pinExStyle = GetWindowLong(wnd, GWL_EXSTYLE);
    bool pinNeedsTopmost = !(pinExStyle & WS_EX_TOPMOST);
    
    if (pinNeedsTopmost) {
        SetWindowLong(wnd, GWL_EXSTYLE, pinExStyle | WS_EX_TOPMOST);
        
        // 只在图钉样式改变时才调整层级
        SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}


void PinWnd::evPaint(HWND wnd, Data& pd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(wnd, &ps);
    if (!dc) {
        return;
    }
    
    // RAII管理Paint结束
    auto paintGuard = Util::RAII::makePaintGuard(wnd, ps);
    
    if (!app.pinShape.getBmp()) {
        return;
    }
    
    // 首先用黑色填充整个窗口背景（黑色会被设置为透明）
    RECT clientRect;
    GetClientRect(wnd, &clientRect);
    auto brushGuard = Util::RAII::makeBrushGuard(CreateSolidBrush(RGB(0, 0, 0)));
    if (brushGuard.isValid()) {
        FillRect(dc, &clientRect, brushGuard.get());
    }
    
    HDC memDC = CreateCompatibleDC(dc);
    if (!memDC) {
        return;
    }
    
    // RAII管理内存DC
    auto dcGuard = Util::RAII::makeDCGuard(memDC);
    
    HBITMAP orgBmp = static_cast<HBITMAP>(SelectObject(memDC, app.pinShape.getBmp()));
    if (orgBmp) {
        // 获取位图信息
        BITMAP bm;
        GetObject(app.pinShape.getBmp(), sizeof(bm), &bm);
        
        // 使用TransparentBlt，将黑色作为透明色
        TransparentBlt(dc, 0, 0, app.pinShape.getW(), app.pinShape.getH(), 
                      memDC, 0, 0, bm.bmWidth, bm.bmHeight, 
                      RGB(0, 0, 0)); // 黑色作为透明色
        
        SelectObject(memDC, orgBmp);
    }
}


void PinWnd::evLClick(HWND wnd, Data& pd)
{
    DestroyWindow(wnd);
}


void PinWnd::evDpiChanged(HWND wnd, Data& pd, WPARAM wparam, LPARAM lparam)
{
    int newDpi = HIWORD(wparam);
    RECT* newRect = (RECT*)lparam;
    
    // 如果建议，更新窗口位置
    if (newRect) {
        SetWindowPos(wnd, nullptr, 
            newRect->left, newRect->top,
            newRect->right - newRect->left,
            newRect->bottom - newRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
    
    // 根据新DPI更新图钉大小
    SetWindowPos(wnd, 0, 0, 0, app.pinShape.getW(), app.pinShape.getH(), 
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    
    // 为新DPI更新窗口区域
    if (app.pinShape.getRgn()) {
        auto rgnGuard = Util::RAII::makeRegionGuard(CreateRectRgn(0, 0, 0, 0));
        if (rgnGuard.isValid() && CombineRgn(rgnGuard.get(), app.pinShape.getRgn(), 0, RGN_COPY) != ERROR) {
            if (!SetWindowRgn(wnd, rgnGuard.get(), false)) {
                // SetWindowRgn失败时，系统不会接管区域的所有权，所以RAII会自动清理
            } else {
                // SetWindowRgn成功时，系统接管了区域的所有权，释放RAII的所有权
                rgnGuard.release();
            }
        }
    }
    
    // 重新定位标题栏上的图钉
    placeOnCaption(wnd, pd);
    
    // 强制重绘
    InvalidateRect(wnd, nullptr, TRUE);
}


bool PinWnd::evPinAssignWnd(HWND wnd, Data& pd, HWND target, int pollRate)
{
    // 这不应该发生；这意味着图钉已经被使用
    if (pd.topMostWnd) {
        return false;
    }

    pd.topMostWnd = target;

    // 决定代理模式 - 使用改进的检测逻辑
    if (Window::needsProxyMode(target)) {
        // 设置代理模式标志；我们稍后会找到代理窗口
        pd.proxyMode = true;
    }

    // 只有在非代理模式或已找到有效代理窗口时才设置父窗口关系
    HWND pinOwner = pd.getPinOwner();
    if (pinOwner) {
        // 文档说不要设置GWL_HWNDPARENT，但它仍然有效
        // （SetParent()只适用于同一进程的窗口）
        SetLastError(0);
        if (!SetWindowLongPtr(wnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(pinOwner)) && GetLastError()) {
            // 对于某些现代Windows应用（如设置、计算器等），SetWindowLongPtr可能会失败
            // 但这不影响图钉的基本功能（置顶），所以只记录警告而不显示错误弹窗
            LOG_WARNING(std::wstring(L"无法设置图钉的父窗口关系，但图钉功能仍然正常。目标窗口句柄: 0x") + 
                       std::to_wstring(reinterpret_cast<uintptr_t>(pinOwner)));
        }
    } else if (pd.proxyMode) {
        // 在代理模式下，代理窗口会在后续的定时器中通过selectProxy函数查找
        // 这是正常流程，不需要显示错误
    }

    if (!SetWindowPos(pd.topMostWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE)) {
        // 置顶失败时，清理数据并返回失败
        // 错误消息由上层的pinWindow函数统一处理，避免重复显示
        pd.topMostWnd = nullptr;
        pd.proxyMode = false;
        pd.proxyWnd = nullptr;
        return false;
    }

    // 设置窗口区域
    if (app.pinShape.getRgn()) {
        auto rgnGuard = Util::RAII::makeRegionGuard(CreateRectRgn(0,0,0,0));
        if (rgnGuard.isValid() && CombineRgn(rgnGuard.get(), app.pinShape.getRgn(), 0, RGN_COPY) != ERROR) {
            if (!SetWindowRgn(wnd, rgnGuard.get(), false)) {
                // SetWindowRgn失败时，系统不会接管区域的所有权，所以RAII会自动清理
            } else {
                // SetWindowRgn成功时，系统接管了区域的所有权，释放RAII的所有权
                rgnGuard.release();
            }
        }
    }

    // 设置图钉大小
    SetWindowPos(wnd, 0, 0, 0, app.pinShape.getW(), app.pinShape.getH(), 
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    
    // 计算并设置图钉位置
    placeOnCaption(wnd, pd);
    
    // 显示图钉窗口
    ShowWindow(wnd, SW_SHOW);
    
    // 统一的层级设置策略 - 修复多图钉显示问题
    // 确保图钉具有TOPMOST属性
    LONG exStyle = GetWindowLong(wnd, GWL_EXSTYLE);
    if (!(exStyle & WS_EX_TOPMOST)) {
        SetWindowLong(wnd, GWL_EXSTYLE, exStyle | WS_EX_TOPMOST);
    }
    
    // 使用HWND_TOP确保图钉在所有TOPMOST窗口的最前面
    // 这样可以避免多个图钉之间的层级冲突
    SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0, 
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    SetForegroundWindow(pd.topMostWnd);

    // 启动轮询定时器
    SetTimer(wnd, 1, pollRate, 0);

    return true;
}


HWND PinWnd::evGetPinnedWnd(HWND wnd, Data& pd)
{
    return pd.topMostWnd;
}


void PinWnd::evPinResetTimer(HWND wnd, Data& pd, int pollRate)
{
    // 只有在有被钉住的窗口时才设置它
    if (pd.topMostWnd) {
        SetTimer(wnd, 1, pollRate, 0);
    }
}


// VCL应用程序的补丁（所有者的所有者问题）。
// 如果被钉住的窗口和图钉窗口的可见性状态不同
// 将图钉状态更改为被钉住窗口状态。
// 返回图钉是否可见
// （如果是，调用者应该执行进一步的调整）
bool PinWnd::fixVisible(HWND wnd, const Data& pd)
{
    // 合理性检查
    if (!IsWindow(pd.topMostWnd)) return false;

    HWND pinOwner = pd.getPinOwner();
    if (!pinOwner) return false;

    // 对于现代Windows应用，需要特殊的最小化检测逻辑
    bool ownerVisible;
    if (Window::isModernWindowsApp(pd.topMostWnd)) {
        // 对于现代Windows应用，检查主窗口和代理窗口的状态
        // 使用缓存的API减少系统调用
        bool mainWndVisible = Window::Cached::isWindowVisible(pd.topMostWnd) && 
                             !Window::Cached::isWindowIconic(pd.topMostWnd);
        bool proxyWndVisible = Window::Cached::isWindowVisible(pinOwner) && 
                              !Window::Cached::isWindowIconic(pinOwner);
        
        // 如果主窗口最小化，图钉应该隐藏
        if (Window::Cached::isWindowIconic(pd.topMostWnd)) {
            ownerVisible = false;
        }
        // 如果主窗口不可见，图钉也应该隐藏
        else if (!Window::Cached::isWindowVisible(pd.topMostWnd)) {
            ownerVisible = false;
        }
        // 如果代理窗口存在且状态正常，使用代理窗口状态
        else if (pd.proxyWnd && IsWindow(pd.proxyWnd)) {
            ownerVisible = proxyWndVisible;
        }
        // 否则使用主窗口状态
        else {
            ownerVisible = mainWndVisible;
        }
        
        // 如果代理窗口失效，尝试重新查找
        if (pd.proxyMode && pd.proxyWnd && !IsWindow(pd.proxyWnd)) {
            Data* pdMutable = const_cast<Data*>(&pd);
            pdMutable->proxyWnd = nullptr;
            // 重新查找代理窗口的逻辑会在evTimer中的selectProxy调用中处理
        }
    } else {
        // 传统应用的处理逻辑
        // IsIconic()至关重要；没有它我们无法通过点击任务栏按钮来恢复最小化的窗口
        // 使用缓存的API减少系统调用
        ownerVisible = Window::Cached::isWindowVisible(pinOwner) && 
                      !Window::Cached::isWindowIconic(pinOwner);
    }
    
    bool pinVisible = !!Window::Cached::isWindowVisible(wnd);
    if (ownerVisible != pinVisible) {
        ShowWindow(wnd, ownerVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
        // 窗口状态改变后，使缓存失效
        Window::WindowCache::getInstance().invalidateWindow(wnd);
    }

    // 返回图钉现在是否可见
    return ownerVisible;
}


// VCL应用程序的补丁（清除WS_EX_TOPMOST位）
void PinWnd::fixTopStyle(HWND wnd, const Data& pd)
{
    // 合理性检查
    if (!IsWindow(pd.topMostWnd)) return;

    HWND pinOwner = pd.getPinOwner();
    if (!pinOwner) return;

    // 使用缓存的API获取窗口样式
    LONG ownerStyle = Window::Cached::getWindowLong(pinOwner, GWL_EXSTYLE);
    LONG pinStyle = Window::Cached::getWindowLong(wnd, GWL_EXSTYLE);
    
    bool ownerTopMost = !!(ownerStyle & WS_EX_TOPMOST);
    bool pinTopMost = !!(pinStyle & WS_EX_TOPMOST);

    if (ownerTopMost != pinTopMost) {
        SetWindowPos(wnd, ownerTopMost ? HWND_TOPMOST : HWND_NOTOPMOST, 
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        // 窗口样式改变后，使缓存失效
        Window::WindowCache::getInstance().invalidateWindow(wnd);
    }
}


void PinWnd::placeOnCaption(HWND wnd, const Data& pd)
{
    HWND pinOwner = pd.getPinOwner();
    
    // 在代理模式下，如果代理窗口还未找到，使用原始目标窗口作为参考
    if (!pinOwner && pd.proxyMode) {
        pinOwner = pd.topMostWnd;
    }
    
    if (!pinOwner) {
        return;
    }

    // 对于现代Windows应用，检查窗口状态
    if (Window::isModernWindowsApp(pd.topMostWnd)) {
        // 如果主窗口最小化，不更新图钉位置（图钉应该已经隐藏）
        // 使用缓存的API减少系统调用
        if (Window::Cached::isWindowIconic(pd.topMostWnd)) {
            return;
        }
        
        // 如果主窗口不可见，也不更新位置
        if (!Window::Cached::isWindowVisible(pd.topMostWnd)) {
            return;
        }
        
        // 如果代理窗口存在但不可见，尝试使用主窗口
        if (pd.proxyWnd && !Window::Cached::isWindowVisible(pd.proxyWnd)) {
            pinOwner = pd.topMostWnd;
        }
    }

    // 获取窗口矩形 - 对于现代Windows应用使用可视边框
    RECT pinned;
    if (Window::isModernWindowsApp(pd.topMostWnd)) {
        // 对于现代Windows应用，使用特殊的矩形获取方法
        if (!Window::getVisibleWindowRect(pinOwner, pinned)) {
            if (!Window::Cached::getWindowRect(pinOwner, pinned)) {
                LOG_WARNING(L"无法获取现代应用窗口矩形");
                return;
            }
        }
        
        // 检查窗口矩形是否有效
        if (pinned.right <= pinned.left || pinned.bottom <= pinned.top) {
            LOG_WARNING(L"现代应用窗口矩形无效，跳过位置更新");
            return;
        }
        
        // 对于现代Windows应用，可能需要调整位置计算
        // 因为它们的标题栏可能有不同的布局
        int windowWidth = pinned.right - pinned.left;
        int pinWidth = app.pinShape.getW();
        int x = pinned.left + (windowWidth - pinWidth) / 2;
        
        // 对于现代应用，图钉位置可能需要更靠近窗口顶部
        int y = pinned.top + 15;  // 减少偏移量，更靠近顶部
        
        // 确保图钉不会超出屏幕边界
        RECT screenRect;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
        
        if (x < screenRect.left) x = screenRect.left;
        if (x + pinWidth > screenRect.right) x = screenRect.right - pinWidth;
        if (y < screenRect.top) y = screenRect.top;
        
        SetWindowPos(wnd, NULL, x, y, 0, 0, 
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        // 窗口位置改变后，使缓存失效
        Window::WindowCache::getInstance().invalidateWindow(wnd);
    } else {
        // 传统应用的位置计算
        if (!Window::Cached::getWindowRect(pinOwner, pinned)) {
            return;
        }
        
        int windowWidth = pinned.right - pinned.left;
        int pinWidth = app.pinShape.getW();
        int x = pinned.left + (windowWidth - pinWidth) / 2;
        int y = pinned.top + 20;
        
        SetWindowPos(wnd, NULL, x, y, 0, 0, 
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        // 窗口位置改变后，使缓存失效
        Window::WindowCache::getInstance().invalidateWindow(wnd);
    }
}


bool PinWnd::selectProxy(HWND wnd, const Data& pd)
{
    HWND appWnd = pd.topMostWnd;
    if (!IsWindow(appWnd)) return false;

    DWORD thread = GetWindowThreadProcessId(appWnd, 0);
    
    // 对于现代Windows应用，使用增强的代理窗口查找策略
    if (Window::isModernWindowsApp(appWnd)) {
        // 首先尝试标准的枚举方式
        bool found = EnumThreadWindows(thread, (WNDENUMPROC)enumThreadWndProc, 
            LPARAM(wnd)) && pd.getPinOwner();
        
        if (!found) {
            // 如果标准方式失败，尝试查找子窗口
            Data* pdMutable = const_cast<Data*>(&pd);
            EnumChildWindows(appWnd, (WNDENUMPROC)enumChildWndProc, LPARAM(wnd));
            found = pd.getPinOwner() != nullptr;
        }
        
        return found;
    } else {
        // 对于传统应用，使用原有的查找方式
        return EnumThreadWindows(thread, (WNDENUMPROC)enumThreadWndProc, 
            LPARAM(wnd)) && pd.getPinOwner();
    }
}


BOOL CALLBACK PinWnd::enumThreadWndProc(HWND wnd, LPARAM param)
{
    HWND pin = HWND(param);
    Data* pd = Data::get(pin);
    if (!pd)
        return false;

    if (GetWindow(wnd, GW_OWNER) == pd->topMostWnd) {
        // 使用缓存的API减少系统调用
        if (Window::Cached::isWindowVisible(wnd) && 
            !Window::Cached::isWindowIconic(wnd) && 
            !Window::isWndRectEmpty(wnd)) {
            pd->proxyWnd = wnd;
            
            // 设置代理窗口为图钉的父窗口
            SetLastError(0);
            if (!SetWindowLongPtr(pin, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(wnd)) && GetLastError()) {
                // 对于某些现代Windows应用，设置父窗口关系可能失败，但不影响基本功能
                LOG_WARNING(std::wstring(L"无法设置代理窗口的父子关系，但图钉功能仍然正常。代理窗口句柄: 0x") + 
                           std::to_wstring(reinterpret_cast<uintptr_t>(wnd)));
            }
            
            // 重新计算图钉位置，因为现在有了有效的代理窗口
            placeOnCaption(pin, *pd);
            
            // 层级管理由evTimer统一处理，这里不做层级调整
            // 只确保图钉具有TOPMOST属性
            LONG exStyle = Window::Cached::getWindowLong(pin, GWL_EXSTYLE);
            if (!(exStyle & WS_EX_TOPMOST)) {
                SetWindowLong(pin, GWL_EXSTYLE, exStyle | WS_EX_TOPMOST);
                // 窗口样式改变后，使缓存失效
                Window::WindowCache::getInstance().invalidateWindow(pin);
            }
            
            return false;   // 停止枚举
        }
    }

    return true;  // 继续枚举
}


// 新增：子窗口枚举回调函数，用于现代Windows应用
BOOL CALLBACK PinWnd::enumChildWndProc(HWND wnd, LPARAM param)
{
    HWND pin = HWND(param);
    Data* pd = Data::get(pin);
    if (!pd)
        return false;

    // 对于现代Windows应用，查找合适的子窗口作为代理
    // 使用缓存的API减少系统调用
    if (Window::Cached::isWindowVisible(wnd) && 
        !Window::Cached::isWindowIconic(wnd) && 
        !Window::isWndRectEmpty(wnd)) {
        // 获取窗口类名进行进一步筛选
        std::wstring className = Window::Cached::getWindowClassName(wnd);
        
        // 优先选择这些类型的窗口作为代理
        if (className == L"Windows.UI.Core.CoreWindow" ||
            className == L"DirectUIHWND" ||
            className.find(L"Chrome") != std::wstring::npos ||
            className.find(L"Content") != std::wstring::npos) {
            
            pd->proxyWnd = wnd;
            
            // 设置代理窗口为图钉的父窗口
            SetLastError(0);
            if (!SetWindowLongPtr(pin, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(wnd)) && GetLastError()) {
                LOG_WARNING(std::wstring(L"无法设置子窗口代理的父子关系，但图钉功能仍然正常。代理窗口句柄: 0x") + 
                           std::to_wstring(reinterpret_cast<uintptr_t>(wnd)));
            }
            
            // 重新计算图钉位置
            placeOnCaption(pin, *pd);
            
            // 层级管理由evTimer统一处理，这里不做层级调整
            // 只确保图钉具有TOPMOST属性
            LONG exStyle = Window::Cached::getWindowLong(pin, GWL_EXSTYLE);
            if (!(exStyle & WS_EX_TOPMOST)) {
                SetWindowLong(pin, GWL_EXSTYLE, exStyle | WS_EX_TOPMOST);
                // 窗口样式改变后，使缓存失效
                Window::WindowCache::getInstance().invalidateWindow(pin);
            }
            
            return false;   // 停止枚举
        }
    }

    return true;  // 继续枚举
}
