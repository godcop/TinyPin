#include "core/stdafx.h"
#include "pin/auto_pin_manager.h"
#include "core/application.h"
#include "options/options.h"
#include "pin/pin_manager.h"

void PendingWindows::add(HWND wnd) {
	if (!IsWindow(wnd)) return;

	// 获取窗口标题和类名
	Window::WndHelper helper(wnd);
	std::wstring title = helper.getText();
	std::wstring className = helper.getClassName();

	// 添加到队列
	m_wnds.push_back({ wnd, title, className, GetTickCount64() });
}

void PendingWindows::check(HWND wnd, const Options& opt)
{
    if (m_wnds.empty()) return;

    // 批量处理窗口，减少频繁的Sleep调用
    const int BATCH_SIZE = 5;
    int processedCount = 0;
    
    for (int n = static_cast<int>(m_wnds.size())-1; n >= 0; --n) {
        if (timeToChkWnd(m_wnds[n].time, opt)) {
            HWND targetWnd = m_wnds[n].wnd;
            
            if (checkWnd(targetWnd, opt)) {
                // 检查窗口是否仍然有效
                if (IsWindow(targetWnd)) {
                    // 检查窗口是否在黑名单中
                    if (!isInBlacklist(targetWnd)) {
                        // 检查是否为错误对话框
                        if (isErrorDialog(targetWnd)) {
                            addToBlacklist(targetWnd);
                        } else {
                            // 尝试图钉正常窗口
                            Pin::PinManager::pinWindow(wnd, targetWnd, opt.trackRate.value, true);
                            
                            // 给系统一点时间来创建图钉
                            Sleep(Constants::DEFAULT_BLINK_DELAY);
                            
                            // 检查图钉是否成功创建
                            if (!Pin::PinManager::hasPin(targetWnd)) {
                                addToBlacklist(targetWnd);
                            }
                        }
                    }
                }
            }
            m_wnds.erase(m_wnds.begin() + n);
            
            // 批量处理后再Sleep，减少频繁的Sleep调用
            if (++processedCount >= BATCH_SIZE) {
                Sleep(1); // 减少Sleep时间
                processedCount = 0;
            }
        }
    }
    
    // 清理过期的黑名单条目
    cleanupBlacklist();
}

bool PendingWindows::timeToChkWnd(ULONGLONG t, const Options& opt)
{
    // 使用GetTickCount64避免32位溢出问题
    return GetTickCount64() - t >= ULONGLONG(opt.autoPinDelay.value);
}

bool PendingWindows::checkWnd(HWND target, const Options& opt)
{
    for (int n = 0; n < int(opt.autoPinRules.size()); ++n)
        if (opt.autoPinRules[n].match(target))
            return true;
    return false;
}

bool PendingWindows::isErrorDialog(HWND wnd)
{
    if (!wnd || !IsWindow(wnd)) return false;
    
    // 获取窗口标题
    WCHAR windowTitle[Constants::MAX_WINDOWTEXT_LEN] = {0};
                GetWindowText(wnd, windowTitle, Constants::MAX_WINDOWTEXT_LEN);
    
    std::wstring title(windowTitle);
    
    // 主要检查是否为TinyPin自己的错误对话框
    // 只过滤明确的TinyPin错误对话框，让其他窗口都有机会被图钉
    if (title == App::APPNAME) {
        return true;
    }
    
    return false;
}

bool PendingWindows::isInBlacklist(HWND wnd)
{
    for (const auto& entry : m_blacklist) {
        if (entry.wnd == wnd) {
            return true;
        }
    }
    return false;
}

void PendingWindows::addToBlacklist(HWND wnd)
{
    // 检查是否已经在黑名单中
    if (!isInBlacklist(wnd)) {
        BlacklistEntry entry(wnd, GetTickCount64());
        m_blacklist.push_back(entry);
    }
}

void PendingWindows::cleanupBlacklist()
{
    const ULONGLONG BLACKLIST_TIMEOUT = 120000; // 2分钟超时
    ULONGLONG currentTime = GetTickCount64();
    
    // 使用remove_if算法，比逆向遍历更高效
    m_blacklist.erase(
        std::remove_if(m_blacklist.begin(), m_blacklist.end(),
            [currentTime, BLACKLIST_TIMEOUT](const BlacklistEntry& entry) {
                return !IsWindow(entry.wnd) || 
                       (currentTime - entry.time) >= BLACKLIST_TIMEOUT;
            }),
        m_blacklist.end()
    );
}