#pragma once

#include <windows.h>
#include <vector>
#include <memory>

namespace Util {
namespace Font {

// 字体样式枚举
enum FontStyle {
    noStyle = 0,
    bold = 1,
    italic = 2,
    underline = 4
};

// 字体管理器类 - 负责字体生命周期管理
class FontManager {
private:
    std::vector<HFONT> m_fonts;

public:
    FontManager() = default;
    ~FontManager();
    
    // 禁用拷贝构造和拷贝赋值
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    
    // 支持移动语义
    FontManager(FontManager&& other) noexcept;
    FontManager& operator=(FontManager&& other) noexcept;
    
    // 创建字体并管理其生命周期
    HFONT createFont(const LOGFONTW& logFont);
    HFONT createFont(const WCHAR* faceName, int height, BYTE charSet = DEFAULT_CHARSET, int style = noStyle);
    
    // 创建DPI感知字体
    HFONT createDpiAwareFont(const WCHAR* faceName, int baseHeight, BYTE charSet = DEFAULT_CHARSET, int style = noStyle, int dpi = 96);
    HFONT createDpiAwareFont(HFONT baseFont, int style, int dpi);
    
    // 释放所有字体资源
    void releaseAllFonts();
    
    // 获取管理的字体数量
    size_t getFontCount() const { return m_fonts.size(); }
};

// 字体辅助类 - 提供静态字体创建方法
class FontHelper {
public:
    // 创建字体（不管理生命周期）
    static HFONT create(const WCHAR* faceName, int height, BYTE charSet = DEFAULT_CHARSET, int style = noStyle);
    static HFONT create(HFONT baseFont, int height = 0, int style = noStyle);
    
    // 创建DPI感知字体
    static HFONT createDpiAware(const WCHAR* faceName, int baseHeight, BYTE charSet = DEFAULT_CHARSET, int style = noStyle, int dpi = 96);
    static HFONT createDpiAware(HFONT baseFont, int style, int dpi);
    
    // 获取现代系统字体
    static HFONT getModernSystemFont(int dpi = 96);
    
    // 获取默认GUI字体
    static HFONT getStockDefaultGui();
    
    // 字体信息获取
    static int getHeight(HFONT font);
    
    // DPI缩放辅助
    static int scaleSizeForDpi(int baseSize, int dpi);
};

} // namespace Font
} // namespace Util

// RAII字体管理辅助类
namespace RAII {

class FontGuard {
private:
    HFONT m_font;
    bool m_shouldDelete;

public:
    explicit FontGuard(HFONT font, bool shouldDelete = true) 
        : m_font(font), m_shouldDelete(shouldDelete) {}
    
    ~FontGuard() {
        if (m_font && m_shouldDelete) {
            DeleteObject(m_font);
        }
    }
    
    // 禁用拷贝
    FontGuard(const FontGuard&) = delete;
    FontGuard& operator=(const FontGuard&) = delete;
    
    // 支持移动
    FontGuard(FontGuard&& other) noexcept 
        : m_font(other.m_font), m_shouldDelete(other.m_shouldDelete) {
        other.m_font = nullptr;
        other.m_shouldDelete = false;
    }
    
    FontGuard& operator=(FontGuard&& other) noexcept {
        if (this != &other) {
            if (m_font && m_shouldDelete) {
                DeleteObject(m_font);
            }
            m_font = other.m_font;
            m_shouldDelete = other.m_shouldDelete;
            other.m_font = nullptr;
            other.m_shouldDelete = false;
        }
        return *this;
    }
    
    HFONT get() const { return m_font; }
    HFONT release() {
        m_shouldDelete = false;
        return m_font;
    }
    
    operator HFONT() const { return m_font; }
    operator bool() const { return m_font != nullptr; }
};

} // namespace RAII