#pragma once

// 前向声明，避免循环包含
struct App;

extern App app;

// 不可复制类，用于替代boost::noncopyable
class noncopyable {
protected:
    noncopyable() = default;
    ~noncopyable() = default;
    
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

// 常用常量定义
namespace Constants {
    // 缓冲区大小
    constexpr int MAX_CLASSNAME_LEN = 256;
    constexpr int MAX_WINDOWTEXT_LEN = 256;
    constexpr int SMALL_BUFFER_SIZE = 100;
    constexpr int MEDIUM_BUFFER_SIZE = 256;
    constexpr int LARGE_BUFFER_SIZE = 1000;
    
    // 时间相关
    constexpr int DEFAULT_BLINK_DELAY = 50;  // 毫秒
    constexpr int MIN_TRACK_RATE = 10;       // 毫秒
    constexpr int MAX_TRACK_RATE = 1000;     // 毫秒
    constexpr int DEFAULT_TRACK_RATE_NEW = 20;   // Win2000/XP
    constexpr int DEFAULT_TRACK_RATE_OLD = 100;  // 旧版Windows
    constexpr int MIN_AUTOPIN_DELAY = 100;   // 毫秒
    constexpr int MAX_AUTOPIN_DELAY = 10000; // 毫秒
    constexpr int DEFAULT_AUTOPIN_DELAY = 200; // 毫秒
    constexpr int TOP_STYLE_CHECK_INTERVAL = 500; // 毫秒，层级检查间隔
    constexpr int FIX_VISIBLE_INTERVAL = 100; // 毫秒，可见性修复间隔
    
    // UI相关
    constexpr int DEFAULT_LAYER_WND_POS = 100; // 层窗口默认位置
    
    // 控件ID常量
    constexpr int ID_APPLY_BUTTON = 0x3021;    // Apply按钮的ID
}