#pragma once

class Options;

// 创建窗口的自动图钉检查。
// 记住每个窗口添加的时间，因此在检查时
// 只处理那些已经通过自动图钉延迟的窗口。
//
class PendingWindows {
public:
    void add(HWND wnd);
    void check(HWND wnd, const Options& opt);

protected:
    struct Entry {
        HWND wnd;
        std::wstring title;
        std::wstring className;
        ULONGLONG time;
        Entry(HWND h = 0, const std::wstring& t = L"", const std::wstring& c = L"", ULONGLONG tm = 0) 
            : wnd(h), title(t), className(c), time(tm) {}
    };
    std::vector<Entry> m_wnds;

    struct BlacklistEntry {
        HWND wnd;
        ULONGLONG time;
        BlacklistEntry(HWND h = 0, ULONGLONG t = 0) : wnd(h), time(t) {}
    };
    std::vector<BlacklistEntry> m_blacklist;

    bool timeToChkWnd(ULONGLONG t, const Options& opt);
    bool checkWnd(HWND target, const Options& opt);
    bool isErrorDialog(HWND wnd);
    bool isInBlacklist(HWND wnd);
    void addToBlacklist(HWND wnd);
    void cleanupBlacklist();
};