#include "core/stdafx.h"
#include "graphics/font_utils.h"
#include <algorithm>

namespace Util {
namespace Font {

// FontManager 实现
FontManager::~FontManager() {
    releaseAllFonts();
}

FontManager::FontManager(FontManager&& other) noexcept 
    : m_fonts(std::move(other.m_fonts)) {
    other.m_fonts.clear();
}

FontManager& FontManager::operator=(FontManager&& other) noexcept {
    if (this != &other) {
        releaseAllFonts();
        m_fonts = std::move(other.m_fonts);
        other.m_fonts.clear();
    }
    return *this;
}

HFONT FontManager::createFont(const LOGFONTW& logFont) {
    HFONT font = CreateFontIndirectW(&logFont);
    if (font) {
        m_fonts.push_back(font);
    }
    return font;
}

HFONT FontManager::createFont(const WCHAR* faceName, int height, BYTE charSet, int style) {
    LOGFONTW lf = {0};
    lf.lfHeight = height;
    lf.lfCharSet = charSet;
    lf.lfWeight = (style & bold) ? FW_BOLD : FW_NORMAL;
    lf.lfItalic = (style & italic) ? TRUE : FALSE;
    lf.lfUnderline = (style & underline) ? TRUE : FALSE;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, faceName);
    
    return createFont(lf);
}

HFONT FontManager::createDpiAwareFont(const WCHAR* faceName, int baseHeight, BYTE charSet, int style, int dpi) {
    LOGFONTW lf = {0};
    lf.lfHeight = -MulDiv(baseHeight, dpi, 120);
    lf.lfCharSet = charSet;
    lf.lfWeight = (style & bold) ? FW_BOLD : FW_NORMAL;
    lf.lfItalic = (style & italic) ? TRUE : FALSE;
    lf.lfUnderline = (style & underline) ? TRUE : FALSE;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, faceName);
    
    return createFont(lf);
}

HFONT FontManager::createDpiAwareFont(HFONT baseFont, int style, int dpi) {
    LOGFONTW lf = {0};
    GetObjectW(baseFont, sizeof(LOGFONTW), &lf);
    
    int baseSize = abs(lf.lfHeight);
    if (baseSize > 0) {
        lf.lfHeight = -MulDiv(baseSize, dpi, 120);
    }
    
    if (style & bold) {
        lf.lfWeight = FW_BOLD;
    }
    
    if (style & italic) {
        lf.lfItalic = TRUE;
    }
    
    if (style & underline) {
        lf.lfUnderline = TRUE;
    }
    
    lf.lfQuality = CLEARTYPE_QUALITY;
    
    return createFont(lf);
}

void FontManager::releaseAllFonts() {
    for (HFONT font : m_fonts) {
        if (font) {
            DeleteObject(font);
        }
    }
    m_fonts.clear();
}

// FontHelper 实现
HFONT FontHelper::create(const WCHAR* faceName, int height, BYTE charSet, int style) {
    LOGFONTW lf = {0};
    lf.lfHeight = height;
    lf.lfCharSet = charSet;
    lf.lfWeight = (style & bold) ? FW_BOLD : FW_NORMAL;
    lf.lfItalic = (style & italic) ? TRUE : FALSE;
    lf.lfUnderline = (style & underline) ? TRUE : FALSE;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, faceName);
    
    return CreateFontIndirectW(&lf);
}

HFONT FontHelper::create(HFONT baseFont, int height, int style) {
    LOGFONTW lf = {0};
    GetObjectW(baseFont, sizeof(LOGFONTW), &lf);
    
    if (height != 0) {
        lf.lfHeight = height;
    }
    
    if (style & bold) {
        lf.lfWeight = FW_BOLD;
    }
    
    if (style & italic) {
        lf.lfItalic = TRUE;
    }
    
    if (style & underline) {
        lf.lfUnderline = TRUE;
    }
    
    lf.lfQuality = CLEARTYPE_QUALITY;
    
    return CreateFontIndirectW(&lf);
}

HFONT FontHelper::createDpiAware(const WCHAR* faceName, int baseHeight, BYTE charSet, int style, int dpi) {
    LOGFONTW lf = {0};
    lf.lfHeight = -MulDiv(baseHeight, dpi, 120);
    lf.lfCharSet = charSet;
    lf.lfWeight = (style & bold) ? FW_BOLD : FW_NORMAL;
    lf.lfItalic = (style & italic) ? TRUE : FALSE;
    lf.lfUnderline = (style & underline) ? TRUE : FALSE;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, faceName);
    
    return CreateFontIndirectW(&lf);
}

HFONT FontHelper::createDpiAware(HFONT baseFont, int style, int dpi) {
    LOGFONTW lf = {0};
    GetObjectW(baseFont, sizeof(LOGFONTW), &lf);
    
    int baseSize = abs(lf.lfHeight);
    if (baseSize > 0) {
        lf.lfHeight = -MulDiv(baseSize, dpi, 120);
    }
    
    if (style & bold) {
        lf.lfWeight = FW_BOLD;
    }
    
    if (style & italic) {
        lf.lfItalic = TRUE;
    }
    
    if (style & underline) {
        lf.lfUnderline = TRUE;
    }
    
    lf.lfQuality = CLEARTYPE_QUALITY;
    
    return CreateFontIndirectW(&lf);
}

HFONT FontHelper::getModernSystemFont(int dpi) {
    NONCLIENTMETRICSW ncm = {0};
    ncm.cbSize = sizeof(NONCLIENTMETRICSW);
    
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
        int baseFontSize = 9;
        ncm.lfMessageFont.lfHeight = -MulDiv(baseFontSize, dpi, 72);
        ncm.lfMessageFont.lfQuality = CLEARTYPE_QUALITY;
        ncm.lfMessageFont.lfWeight = FW_NORMAL;
        return CreateFontIndirectW(&ncm.lfMessageFont);
    }
    
    return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

HFONT FontHelper::getStockDefaultGui() {
    return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

int FontHelper::getHeight(HFONT font) {
    LOGFONTW lf = {0};
    GetObjectW(font, sizeof(LOGFONTW), &lf);
    return abs(lf.lfHeight);
}

int FontHelper::scaleSizeForDpi(int baseSize, int dpi) {
    return MulDiv(baseSize, dpi, 96);
}

} // namespace Font
} // namespace Util