#pragma once

#include "core/common.h"
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <list>

namespace Window {

    // 属性类型枚举，用于不同的缓存策略
    enum class PropertyType {
        FAST_CHANGING,    // 快速变化的属性（位置、可见性等）
        MEDIUM_CHANGING,  // 中等变化的属性（窗口状态等）
        SLOW_CHANGING     // 稳定属性（类名、样式等）
    };

    // 窗口状态缓存项
    struct WindowCacheEntry {
        std::wstring windowText;
        std::wstring className;
        RECT windowRect;
        bool isVisible;
        bool isIconic;
        bool isEnabled;
        bool isTopMost;
        bool isChild;
        HWND parent;
        HWND owner;
        LONG style;          // 窗口样式
        LONG exStyle;        // 扩展窗口样式
        std::chrono::steady_clock::time_point lastUpdate;
        
        WindowCacheEntry() : windowRect{0}, isVisible(false), isIconic(false), 
                           isEnabled(false), isTopMost(false), isChild(false),
                           parent(nullptr), owner(nullptr), style(0), exStyle(0) {}
    };

    // 窗口状态缓存管理器
    class WindowCache {
    public:
        // 缓存大小限制常量
        static constexpr size_t MAX_CACHE_SIZE = 50;
        
        // 获取单例实例
        static WindowCache& getInstance();

        // 获取窗口文本（带缓存）
        std::wstring getWindowText(HWND wnd, bool forceRefresh = false);
        
        // 获取窗口类名（带缓存）
        std::wstring getWindowClassName(HWND wnd, bool forceRefresh = false);
        
        // 获取窗口矩形（带缓存）
        bool getWindowRect(HWND wnd, RECT& rect, bool forceRefresh = false);
        
        // 检查窗口可见性（带缓存）
        bool isWindowVisible(HWND wnd, bool forceRefresh = false);
        
        // 检查窗口是否最小化（带缓存）
        bool isWindowIconic(HWND wnd, bool forceRefresh = false);
        
        // 检查窗口是否启用（带缓存）
        bool isWindowEnabled(HWND wnd, bool forceRefresh = false);
        
        // 检查窗口是否置顶（带缓存）
        bool isWindowTopMost(HWND wnd, bool forceRefresh = false);
        
        // 检查是否为子窗口（带缓存）
        bool isWindowChild(HWND wnd, bool forceRefresh = false);
        
        // 获取父窗口（带缓存）
        HWND getParentWindow(HWND wnd, bool forceRefresh = false);
        
        // 获取拥有者窗口（带缓存）
        HWND getOwnerWindow(HWND wnd, bool forceRefresh = false);
        
        // 获取窗口样式（带缓存）
        LONG getWindowLong(HWND wnd, int nIndex, bool forceRefresh = false);
        
        // 使指定窗口的缓存失效
        void invalidateWindow(HWND wnd);
        
        // 清理过期的缓存项
        void cleanupExpiredEntries();
        
        // 获取缓存统计信息
        struct CacheStats {
            size_t totalEntries;
            size_t hitCount;
            size_t missCount;
            double hitRatio;
        };
        CacheStats getStats() const;
        
        // 清空所有缓存
        void clearCache();

    private:
        WindowCache();
        ~WindowCache() = default;
        
        // 禁止复制和移动
        WindowCache(const WindowCache&) = delete;
        WindowCache& operator=(const WindowCache&) = delete;
        WindowCache(WindowCache&&) = delete;
        WindowCache& operator=(WindowCache&&) = delete;
        
        // 获取或创建缓存项
        WindowCacheEntry& getOrCreateEntry(HWND wnd);
        
        // 检查特定属性类型的缓存项是否过期
        bool isEntryExpiredForProperty(const WindowCacheEntry& entry, PropertyType type) const;
        
        // 更新缓存项的窗口信息
        void updateWindowInfo(HWND wnd, WindowCacheEntry& entry);
        
        // LRU缓存管理方法
        void moveToFront(HWND wnd);
        void evictLeastRecentlyUsed();
        
        // 成员变量
        mutable std::mutex m_mutex;
        std::unordered_map<HWND, WindowCacheEntry> m_cache;
        
        // LRU链表，用于跟踪访问顺序（最近访问的在前面）
        std::list<HWND> m_lruList;
        std::unordered_map<HWND, std::list<HWND>::iterator> m_lruIterators;
        
        // 统计信息
        mutable size_t m_hitCount;
        mutable size_t m_missCount;
    };

    // 便利函数，使用缓存的窗口API
    namespace Cached {
        std::wstring getWindowText(HWND wnd, bool forceRefresh = false);
        std::wstring getWindowClassName(HWND wnd, bool forceRefresh = false);
        bool getWindowRect(HWND wnd, RECT& rect, bool forceRefresh = false);
        bool isWindowVisible(HWND wnd, bool forceRefresh = false);
        bool isWindowIconic(HWND wnd, bool forceRefresh = false);
        bool isWindowEnabled(HWND wnd, bool forceRefresh = false);
        bool isWindowTopMost(HWND wnd, bool forceRefresh = false);
        bool isWindowChild(HWND wnd, bool forceRefresh = false);
        HWND getParentWindow(HWND wnd, bool forceRefresh = false);
        HWND getOwnerWindow(HWND wnd, bool forceRefresh = false);
        LONG getWindowLong(HWND wnd, int nIndex, bool forceRefresh = false);
    }

} // namespace Window