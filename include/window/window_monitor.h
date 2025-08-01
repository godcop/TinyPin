#pragma once

// 用于监控系统窗口创建的抽象基类。
// 每当创建窗口时向客户端窗口发送消息。
//
class WindowCreationMonitor {
public:
    virtual ~WindowCreationMonitor() {}
    virtual bool init(HWND wnd, int msgId) = 0;
    virtual bool term() = 0;
};

// 使用SetWinEventHook()的窗口创建监视器。
//
class EventHookWindowCreationMonitor : public WindowCreationMonitor, ::noncopyable {
public:
    EventHookWindowCreationMonitor() {}
    ~EventHookWindowCreationMonitor() { term(); }

    bool init(HWND wnd, int msgId);
    bool term();

private:
    static HWINEVENTHOOK hook;
    static HWND wnd;
    static int msgId;

    static VOID CALLBACK proc(HWINEVENTHOOK hook, DWORD event,
        HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
};