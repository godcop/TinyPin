#include "core/stdafx.h"
#include "window/window_monitor.h"

HWINEVENTHOOK EventHookWindowCreationMonitor::hook = nullptr;
HWND EventHookWindowCreationMonitor::wnd = nullptr;
int EventHookWindowCreationMonitor::msgId = 0;

bool EventHookWindowCreationMonitor::init(HWND wnd, int msgId)
{
    if (!hook) {
        hook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, 
            nullptr, proc, 0, 0, WINEVENT_OUTOFCONTEXT);
        if (hook) {
            this->wnd = wnd;
            this->msgId = msgId;
        }
    }
    return hook != nullptr;
}

bool EventHookWindowCreationMonitor::term()
{
    if (hook && UnhookWinEvent(hook)) {
        hook = nullptr;
    }
    return !hook;
}

VOID CALLBACK EventHookWindowCreationMonitor::proc(HWINEVENTHOOK hook, DWORD event,
    HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (hook == EventHookWindowCreationMonitor::hook &&
        event == EVENT_OBJECT_CREATE &&
        idObject == OBJID_WINDOW)
    {
        PostMessage(wnd, msgId, (WPARAM)hwnd, 0);
    }
}