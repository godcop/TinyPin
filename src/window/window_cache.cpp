#include "core/stdafx.h"
#include "window/window_cache.h"
#include "window/window_helper.h"
#include "options/options.h"

// 引用全局选项对象
extern Options opt;

namespace Window {

// 缓存超时时间常量
namespace CacheTimeouts {
    // 快速变化的属性使用用户自定义的跟踪速率
    std::chrono::milliseconds getFastChanging() {
        return std::chrono::milliseconds(opt.trackRate.value);
    }
    constexpr auto MEDIUM_CHANGING = std::chrono::milliseconds(100); // 窗口状态等中等变化的属性  
    constexpr auto SLOW_CHANGING = std::chrono::milliseconds(1000);  // 类名、样式等稳定属性
    constexpr auto DEFAULT_TIMEOUT = std::chrono::milliseconds(100); // 默认超时时间
}

// WindowCache 实现

WindowCache::WindowCache() 
    : m_hitCount(0), m_missCount(0) {
}

WindowCache& WindowCache::getInstance() {
    static WindowCache instance;
    return instance;
}

Window::WindowCacheEntry& WindowCache::getOrCreateEntry(HWND wnd) {
    auto it = m_cache.find(wnd);
    if (it == m_cache.end()) {
        // 检查缓存大小是否超过限制
        if (m_cache.size() >= MAX_CACHE_SIZE) {
            evictLeastRecentlyUsed();
        }
        
        // 创建新的缓存项
        auto result = m_cache.emplace(wnd, WindowCacheEntry());
        
        // 添加到LRU链表前端
        m_lruList.push_front(wnd);
        m_lruIterators[wnd] = m_lruList.begin();
        
        return result.first->second;
    } else {
        // 移动到LRU链表前端（标记为最近使用）
        moveToFront(wnd);
        return it->second;
    }
}

bool WindowCache::isEntryExpiredForProperty(const WindowCacheEntry& entry, PropertyType type) const {
    auto age = std::chrono::steady_clock::now() - entry.lastUpdate;
    
    switch (type) {
        case PropertyType::FAST_CHANGING:
            return age > CacheTimeouts::getFastChanging();
        case PropertyType::MEDIUM_CHANGING:
            return age > CacheTimeouts::MEDIUM_CHANGING;
        case PropertyType::SLOW_CHANGING:
            return age > CacheTimeouts::SLOW_CHANGING;
        default:
            return age > CacheTimeouts::DEFAULT_TIMEOUT;
    }
}

void WindowCache::updateWindowInfo(HWND wnd, WindowCacheEntry& entry) {
    if (!wnd || !IsWindow(wnd)) {
        return;
    }
    
    // 更新窗口文本
    WCHAR windowText[Constants::MAX_WINDOWTEXT_LEN] = {0};
    GetWindowText(wnd, windowText, Constants::MAX_WINDOWTEXT_LEN);
    entry.windowText = windowText;
    
    // 更新窗口类名
    WCHAR className[Constants::MAX_CLASSNAME_LEN] = {0};
    GetClassName(wnd, className, Constants::MAX_CLASSNAME_LEN);
    entry.className = className;
    
    // 更新窗口矩形
    GetWindowRect(wnd, &entry.windowRect);
    
    // 更新窗口状态
    entry.isVisible = !!IsWindowVisible(wnd);
    entry.isIconic = !!IsIconic(wnd);
    entry.isEnabled = !!IsWindowEnabled(wnd);
    
    // 更新窗口样式
    entry.style = GetWindowLong(wnd, GWL_STYLE);
    entry.exStyle = GetWindowLong(wnd, GWL_EXSTYLE);
    
    // 从样式中推导其他属性
    entry.isTopMost = !!(entry.exStyle & WS_EX_TOPMOST);
    entry.isChild = !!(entry.style & WS_CHILD);
    
    // 更新父窗口和拥有者窗口
    entry.parent = GetParent(wnd);
    entry.owner = GetWindow(wnd, GW_OWNER);
    
    // 更新时间戳
    entry.lastUpdate = std::chrono::steady_clock::now();
}

void WindowCache::moveToFront(HWND wnd) {
    auto iterIt = m_lruIterators.find(wnd);
    if (iterIt != m_lruIterators.end()) {
        // 从当前位置移除
        m_lruList.erase(iterIt->second);
        // 添加到前端
        m_lruList.push_front(wnd);
        // 更新迭代器
        iterIt->second = m_lruList.begin();
    }
}

void WindowCache::evictLeastRecentlyUsed() {
    if (m_lruList.empty()) {
        return;
    }
    
    // 获取最少使用的窗口（链表末尾）
    HWND lruWindow = m_lruList.back();
    
    // 从缓存中移除
    m_cache.erase(lruWindow);
    
    // 从LRU链表中移除
    m_lruList.pop_back();
    
    // 从迭代器映射中移除
    m_lruIterators.erase(lruWindow);
}

std::wstring WindowCache::getWindowText(HWND wnd, bool forceRefresh) {
    if (!wnd) return L"";
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::MEDIUM_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    return entry.windowText;
}

std::wstring WindowCache::getWindowClassName(HWND wnd, bool forceRefresh) {
    if (!wnd) return L"";
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::SLOW_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    return entry.className;
}

bool WindowCache::getWindowRect(HWND wnd, RECT& rect, bool forceRefresh) {
    if (!wnd) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::FAST_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    rect = entry.windowRect;
    return true;
}

bool WindowCache::isWindowVisible(HWND wnd, bool forceRefresh) {
    if (!wnd) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::FAST_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    return entry.isVisible;
}

bool WindowCache::isWindowIconic(HWND wnd, bool forceRefresh) {
    if (!wnd) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::MEDIUM_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    return entry.isIconic;
}

bool WindowCache::isWindowEnabled(HWND wnd, bool forceRefresh) {
    if (!wnd) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::MEDIUM_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    return entry.isEnabled;
}

bool WindowCache::isWindowTopMost(HWND wnd, bool forceRefresh) {
    if (!wnd) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::MEDIUM_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    return entry.isTopMost;
}

bool WindowCache::isWindowChild(HWND wnd, bool forceRefresh) {
    if (!wnd) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::SLOW_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    return entry.isChild;
}

HWND WindowCache::getParentWindow(HWND wnd, bool forceRefresh) {
    if (!wnd) return nullptr;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::SLOW_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    return entry.parent;
}

HWND WindowCache::getOwnerWindow(HWND wnd, bool forceRefresh) {
    if (!wnd) return nullptr;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::SLOW_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    return entry.owner;
}

LONG WindowCache::getWindowLong(HWND wnd, int nIndex, bool forceRefresh) {
    if (!wnd || !IsWindow(wnd)) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& entry = getOrCreateEntry(wnd);
    if (forceRefresh || isEntryExpiredForProperty(entry, PropertyType::SLOW_CHANGING)) {
        updateWindowInfo(wnd, entry);
        m_missCount++;
    } else {
        m_hitCount++;
    }
    
    // 根据索引返回相应的值
    switch (nIndex) {
        case GWL_STYLE:
            return entry.style;
        case GWL_EXSTYLE:
            return entry.exStyle;
        default:
            // 对于其他索引，直接调用系统API
            return ::GetWindowLong(wnd, nIndex);
    }
}

void WindowCache::invalidateWindow(HWND wnd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_cache.find(wnd);
    if (it != m_cache.end()) {
        m_cache.erase(it);
        
        // 从LRU链表中移除
        auto iterIt = m_lruIterators.find(wnd);
        if (iterIt != m_lruIterators.end()) {
            m_lruList.erase(iterIt->second);
            m_lruIterators.erase(iterIt);
        }
    }
}

void WindowCache::cleanupExpiredEntries() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto now = std::chrono::steady_clock::now();
    auto it = m_cache.begin();
    
    while (it != m_cache.end()) {
        // 检查窗口是否仍然有效，或者缓存项是否过期太久（超过10秒）
        if (!IsWindow(it->first) || (now - it->second.lastUpdate) > std::chrono::seconds(10)) {
            HWND wnd = it->first;
            
            // 从LRU链表中移除
            auto iterIt = m_lruIterators.find(wnd);
            if (iterIt != m_lruIterators.end()) {
                m_lruList.erase(iterIt->second);
                m_lruIterators.erase(iterIt);
            }
            
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

WindowCache::CacheStats WindowCache::getStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    CacheStats stats;
    stats.totalEntries = m_cache.size();
    stats.hitCount = m_hitCount;
    stats.missCount = m_missCount;
    
    size_t totalAccess = m_hitCount + m_missCount;
    stats.hitRatio = totalAccess > 0 ? static_cast<double>(m_hitCount) / totalAccess : 0.0;
    
    return stats;
}

void WindowCache::clearCache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_lruList.clear();
    m_lruIterators.clear();
    m_hitCount = 0;
    m_missCount = 0;
}

// 便利函数实现
namespace Cached {

std::wstring getWindowText(HWND wnd, bool forceRefresh) {
    return WindowCache::getInstance().getWindowText(wnd, forceRefresh);
}

std::wstring getWindowClassName(HWND wnd, bool forceRefresh) {
    return WindowCache::getInstance().getWindowClassName(wnd, forceRefresh);
}

bool getWindowRect(HWND wnd, RECT& rect, bool forceRefresh) {
    return WindowCache::getInstance().getWindowRect(wnd, rect, forceRefresh);
}

bool isWindowVisible(HWND wnd, bool forceRefresh) {
    return WindowCache::getInstance().isWindowVisible(wnd, forceRefresh);
}

bool isWindowIconic(HWND wnd, bool forceRefresh) {
    return WindowCache::getInstance().isWindowIconic(wnd, forceRefresh);
}

bool isWindowEnabled(HWND wnd, bool forceRefresh) {
    return WindowCache::getInstance().isWindowEnabled(wnd, forceRefresh);
}

bool isWindowTopMost(HWND wnd, bool forceRefresh) {
    return WindowCache::getInstance().isWindowTopMost(wnd, forceRefresh);
}

bool isWindowChild(HWND wnd, bool forceRefresh) {
    return WindowCache::getInstance().isWindowChild(wnd, forceRefresh);
}

HWND getParentWindow(HWND wnd, bool forceRefresh) {
    return WindowCache::getInstance().getParentWindow(wnd, forceRefresh);
}

HWND getOwnerWindow(HWND wnd, bool forceRefresh) {
    return WindowCache::getInstance().getOwnerWindow(wnd, forceRefresh);
}

LONG getWindowLong(HWND wnd, int nIndex, bool forceRefresh) {
    return WindowCache::getInstance().getWindowLong(wnd, nIndex, forceRefresh);
}

} // namespace Cached

} // namespace Window