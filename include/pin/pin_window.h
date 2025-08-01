#pragma once


// 图钉形状的小弹出窗口。
// 完全负责使目标窗口保持在最前面。
//
class PinWnd {
public:
    static ATOM registerClass();
    static LRESULT CALLBACK proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static LPCWSTR className;

protected:
    // 窗口数据对象。
    //
    class Data {
    public:
        // 附加到图钉窗口并返回对象，出错时返回0。
        static Data* create(HWND pin, HWND callback) {
            Data* p = new Data(callback);
            SetLastError(0);
            if (!SetWindowLongPtr(pin, 0, reinterpret_cast<LONG_PTR>(p)) && GetLastError()) {
                delete p;
                p = 0;
            }
            return p;
        }
        // 获取对象，出错时返回0。
        static Data* get(HWND pin) {
            return reinterpret_cast<Data*>(GetWindowLongPtr(pin, 0));
        }
        // 从图钉窗口分离并删除。
        static void destroy(HWND pin) {
            Data* p = get(pin);
            if (!p)
                return;
            SetLastError(0);
            if (!SetWindowLongPtr(pin, 0, 0) && GetLastError())
                return;
            delete p;
        }

        HWND callbackWnd;

        bool proxyMode;
        HWND topMostWnd;
        HWND proxyWnd;

        HWND getPinOwner() const {
            return proxyMode ? proxyWnd : topMostWnd;
        }

    private:
        Data(HWND wnd) : callbackWnd(wnd), proxyMode(false), topMostWnd(0), proxyWnd(0) {}
    };

    static BOOL CALLBACK enumThreadWndProc(HWND wnd, LPARAM param);
    static BOOL CALLBACK enumChildWndProc(HWND wnd, LPARAM param);
    static bool selectProxy(HWND wnd, const Data& pd);
    static void fixTopStyle(HWND wnd, const Data& pd);
    static void placeOnCaption(HWND wnd, const Data& pd);
    static bool fixVisible(HWND wnd, const Data& pd);
    static void fixPopupZOrder(HWND appWnd);

    static LRESULT evCreate(HWND wnd, Data& pd);
    static void evDestroy(HWND wnd, Data& pd);
    static void evTimer(HWND wnd, Data& pd, UINT id);
    static void evPaint(HWND wnd, Data& pd);
    static void evLClick(HWND wnd, Data& pd);
    static void evDpiChanged(HWND wnd, Data& pd, WPARAM wparam, LPARAM lparam);
    static bool evPinAssignWnd(HWND wnd, Data& pd, HWND target, int pollRate);
    static HWND evGetPinnedWnd(HWND wnd, Data& pd);
    static void evPinResetTimer(HWND wnd, Data& pd, int pollRate);
};