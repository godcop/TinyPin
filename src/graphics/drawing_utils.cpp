#include "core/stdafx.h"
#include "graphics/drawing_utils.h"
#include "core/common.h"
#include "foundation/string_utils.h"
#include <algorithm>
#include <unordered_map>
#include <cstring>

// PNG图像加载功能实现
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

namespace Graphics {
namespace Drawing {

bool rectContains(const RECT& rc1, const RECT& rc2)
{
    return rc2.left   >= rc1.left
        && rc2.top    >= rc1.top
        && rc2.right  <= rc1.right
        && rc2.bottom <= rc1.bottom;
}

BOOL rectangle(HDC dc, const RECT& rc)
{
    return Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
}

bool getBmpSize(HBITMAP bmp, SIZE& sz)
{
    BITMAP bm;
    if (!GetObject(bmp, sizeof(bm), &bm))
        return false;
    sz.cx = bm.bmWidth;
    sz.cy = abs(bm.bmHeight);
    return true;
}

bool getBmpSizeAndBpp(HBITMAP bmp, SIZE& sz, int& bpp)
{
    BITMAP bm;
    if (!GetObject(bmp, sizeof(bm), &bm))
        return false;
    sz.cx = bm.bmWidth;
    sz.cy = abs(bm.bmHeight);
    bpp   = bm.bmBitsPixel;
    return true;
}

// 从位图创建区域，排除透明色区域
HRGN createRegionFromBitmap(HBITMAP bmp, COLORREF transparentColor)
{
    HRGN rgn = NULL;
    SIZE sz;
    if (!getBmpSize(bmp, sz))
        return NULL;

    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = sz.cx;
    bi.bmiHeader.biHeight = sz.cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    DWORD* bits = 0;
    if (HDC dc = CreateCompatibleDC(0)) {
        HBITMAP dib = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, (void**)&bits, 0, 0);
        if (dib) {
            HBITMAP oldBmp = (HBITMAP)SelectObject(dc, dib);
            if (HDC memDC = CreateCompatibleDC(dc)) {
                HBITMAP oldMemBmp = (HBITMAP)SelectObject(memDC, bmp);
                BitBlt(dc, 0,0,sz.cx,sz.cy, memDC, 0,0, SRCCOPY);
                SelectObject(memDC, oldMemBmp);
                DeleteDC(memDC);
            }
            SelectObject(dc, oldBmp);

            // 创建区域
            auto fullRgnGuard = Util::RAII::makeRegionGuard(CreateRectRgn(0, 0, sz.cx, sz.cy));
            auto tmpRgnGuard = Util::RAII::makeRegionGuard(CreateRectRgn(0, 0, 0, 0));
            if (fullRgnGuard.isValid() && tmpRgnGuard.isValid()) {
                for (int y = 0; y < sz.cy; y++) {
                    for (int x = 0; x < sz.cx; x++) {
                        if (bits[y * sz.cx + x] == transparentColor) {
                            SetRectRgn(tmpRgnGuard.get(), x, y, x+1, y+1);
                            CombineRgn(fullRgnGuard.get(), fullRgnGuard.get(), tmpRgnGuard.get(), RGN_DIFF);
                        }
                    }
                }
                rgn = fullRgnGuard.release(); // 释放所有权，返回给调用者
            }

            DeleteObject(dib);
        }
        DeleteDC(dc);
    }

    return rgn;
}

// makeRegionFromBmp 作为 createRegionFromBitmap 的别名
HRGN makeRegionFromBmp(HBITMAP bmp, COLORREF clrMask)
{
    return createRegionFromBitmap(bmp, clrMask);
}

// 转换位图的白色/浅灰/深灰为指定颜色的阴影
// 注意：位图不能被任何DC选中才能成功
bool remapBmpColors(HBITMAP bmp, COLORREF clrs[][2], int cnt)
{
    bool ok = false;
    SIZE sz;
    int bpp;
    if (getBmpSizeAndBpp(bmp, sz, bpp)) {
        if (HDC dc = CreateCompatibleDC(0)) {
            if (HBITMAP orgBmp = HBITMAP(SelectObject(dc, bmp))) {
                if (bpp == 16) {
                    // 在16位色彩模式下颜色会发生变化
                    // 例如浅灰色 (0xC0C0C0) 变成 0xC6C6C6
                    // GetNearestColor() 只适用于调色板模式
                    // 所以我们需要手动修正源颜色
                    // 16位色彩模式的颜色转换函数
                    auto Nearest16BppColor = [](COLORREF clr) -> COLORREF {
                        // 提取RGB分量
                        int r = GetRValue(clr);
                        int g = GetGValue(clr);
                        int b = GetBValue(clr);
                        
                        // 在16位色彩模式下，通常使用5位R，6位G，5位B
                        // 将8位颜色分量转换为对应位数，然后再转回8位
                        r = (r & 0xF8); // 5位，保留高5位
                        g = (g & 0xFC); // 6位，保留高6位
                        b = (b & 0xF8); // 5位，保留高5位
                        
                        return RGB(r, g, b);
                    };
                    
                    for (int n = 0; n < cnt; ++n)
                        clrs[n][0] = Nearest16BppColor(clrs[n][0]);
                }
                
                // 优化：预先计算颜色映射表以减少循环中的比较
                std::unordered_map<COLORREF, COLORREF> colorMap;
                for (int n = 0; n < cnt; ++n) {
                    colorMap[clrs[n][0]] = clrs[n][1];
                }
                
                // 优化：按行处理，减少函数调用开销
                for (int y = 0; y < sz.cy; ++y) {
                    for (int x = 0; x < sz.cx; ++x) {
                        COLORREF clr = GetPixel(dc, x, y);
                        auto it = colorMap.find(clr);
                        if (it != colorMap.end()) {
                            SetPixelV(dc, x, y, it->second);
                        }
                    }
                }
                ok = true;
                SelectObject(dc, orgBmp);
            }
            DeleteDC(dc);
        }
    }

    return ok;
}

// 图像加载命名空间实现
namespace Image {

// 图像缩放辅助类实现
std::unique_ptr<unsigned char[]> ImageScaler::scaleRGBA(
    const unsigned char* srcData, int srcWidth, int srcHeight,
    int dstWidth, int dstHeight) {
    
    if (!srcData || srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0) {
        return nullptr;
    }
    
    auto scaledData = std::make_unique<unsigned char[]>(dstWidth * dstHeight * 4);
    
    // 优化：如果尺寸相同，直接复制
    if (srcWidth == dstWidth && srcHeight == dstHeight) {
        std::memcpy(scaledData.get(), srcData, srcWidth * srcHeight * 4);
        return scaledData;
    }
    
    // 使用像素中心采样，确保边界像素正确映射
    float xRatio = static_cast<float>(srcWidth - 1) / (dstWidth - 1);
    float yRatio = static_cast<float>(srcHeight - 1) / (dstHeight - 1);
    
    // 优化：预计算一些常用值
    const int srcWidthMinus1 = srcWidth - 1;
    const int srcHeightMinus1 = srcHeight - 1;
    
    for (int y = 0; y < dstHeight; y++) {
        for (int x = 0; x < dstWidth; x++) {
            float srcX = (dstWidth == 1) ? 0 : x * xRatio;
            float srcY = (dstHeight == 1) ? 0 : y * yRatio;
            
            int x1 = static_cast<int>(srcX);
            int y1 = static_cast<int>(srcY);
            int x2 = (x1 + 1 < srcWidth) ? (x1 + 1) : srcWidthMinus1;
            int y2 = (y1 + 1 < srcHeight) ? (y1 + 1) : srcHeightMinus1;
            
            float xWeight = srcX - x1;
            float yWeight = srcY - y1;
            
            int dstIndex = (y * dstWidth + x) * 4;
            
            // 优化：预计算源像素索引
            int srcIndex1 = (y1 * srcWidth + x1) * 4;
            int srcIndex2 = (y1 * srcWidth + x2) * 4;
            int srcIndex3 = (y2 * srcWidth + x1) * 4;
            int srcIndex4 = (y2 * srcWidth + x2) * 4;
            
            // 双线性插值计算每个颜色通道
            for (int c = 0; c < 4; c++) {
                float p1 = srcData[srcIndex1 + c] * (1 - xWeight) + 
                          srcData[srcIndex2 + c] * xWeight;
                float p2 = srcData[srcIndex3 + c] * (1 - xWeight) + 
                          srcData[srcIndex4 + c] * xWeight;
                
                scaledData[dstIndex + c] = static_cast<unsigned char>(
                    p1 * (1 - yWeight) + p2 * yWeight + 0.5f);
            }
        }
    }
    
    return scaledData;
}

// 从PNG文件加载为HBITMAP
HBITMAP LoadPngAsBitmap(LPCWSTR filename) {
    // 将宽字符文件名转换为UTF-8
    std::string utf8Filename = Foundation::StringUtils::wideToUtf8(filename);
    
    int width, height, channels;
    unsigned char* data = stbi_load(utf8Filename.c_str(), &width, &height, &channels, 4); // 强制RGBA
    
    if (!data) {
        return nullptr;
    }
    
    HBITMAP bitmap = CreateBitmapFromRGBA(data, width, height);
    stbi_image_free(data);
    
    return bitmap;
}

// 从PNG文件加载为指定尺寸的HBITMAP（使用双线性插值缩放）
HBITMAP LoadPngAsBitmap(LPCWSTR filename, int width, int height) {
    // 将宽字符文件名转换为UTF-8
    std::string utf8Filename = Foundation::StringUtils::wideToUtf8(filename);
    
    int imgWidth, imgHeight, channels;
    unsigned char* data = stbi_load(utf8Filename.c_str(), &imgWidth, &imgHeight, &channels, 4); // 强制RGBA
    
    if (!data) {
        return nullptr;
    }
    
    HBITMAP bitmap = nullptr;
    
    // 如果指定了尺寸且与原始尺寸不同，需要缩放
    if (width > 0 && height > 0 && (width != imgWidth || height != imgHeight)) {
        auto scaledData = ImageScaler::scaleRGBA(data, imgWidth, imgHeight, width, height);
        if (scaledData) {
            bitmap = CreateBitmapFromRGBA(scaledData.get(), width, height);
        }
    } else {
        bitmap = CreateBitmapFromRGBA(data, imgWidth, imgHeight);
    }
    
    stbi_image_free(data);
    return bitmap;
}

// 从PNG文件加载为HICON
HICON LoadPngAsIcon(LPCWSTR filename, int width, int height) {
    // 将宽字符文件名转换为UTF-8
    std::string utf8Filename = Foundation::StringUtils::wideToUtf8(filename);
    
    int imgWidth, imgHeight, channels;
    unsigned char* data = stbi_load(utf8Filename.c_str(), &imgWidth, &imgHeight, &channels, 4); // 强制RGBA
    
    if (!data) {
        return nullptr;
    }
    
    HICON icon = nullptr;
    
    // 如果指定了尺寸且与原始尺寸不同，需要缩放
    if (width > 0 && height > 0 && (width != imgWidth || height != imgHeight)) {
        auto scaledData = ImageScaler::scaleRGBA(data, imgWidth, imgHeight, width, height);
        if (scaledData) {
            icon = CreateIconFromRGBA(scaledData.get(), width, height);
        }
    } else {
        icon = CreateIconFromRGBA(data, imgWidth, imgHeight);
    }
    
    stbi_image_free(data);
    return icon;
}

// 从PNG数据加载为HBITMAP
HBITMAP LoadPngFromMemoryAsBitmap(const unsigned char* data, int len) {
    int width, height, channels;
    unsigned char* imageData = stbi_load_from_memory(data, len, &width, &height, &channels, 4); // 强制RGBA
    
    if (!imageData) {
        return nullptr;
    }
    
    HBITMAP bitmap = CreateBitmapFromRGBA(imageData, width, height);
    stbi_image_free(imageData);
    
    return bitmap;
}

// 从PNG数据加载为HICON
HICON LoadPngFromMemoryAsIcon(const unsigned char* data, int len, int width, int height) {
    int imgWidth, imgHeight, channels;
    unsigned char* imageData = stbi_load_from_memory(data, len, &imgWidth, &imgHeight, &channels, 4); // 强制RGBA
    
    if (!imageData) {
        return nullptr;
    }
    
    HICON icon = nullptr;
    
    // 如果指定了尺寸且与原始尺寸不同，需要缩放
    if (width > 0 && height > 0 && (width != imgWidth || height != imgHeight)) {
        auto scaledData = ImageScaler::scaleRGBA(imageData, imgWidth, imgHeight, width, height);
        if (scaledData) {
            icon = CreateIconFromRGBA(scaledData.get(), width, height);
        }
    } else {
        icon = CreateIconFromRGBA(imageData, imgWidth, imgHeight);
    }
    
    stbi_image_free(imageData);
    return icon;
}

// 通用图像加载功能（支持stb_image支持的所有格式）
HBITMAP LoadImageAsBitmap(LPCWSTR filename) {
    return LoadPngAsBitmap(filename);
}

HBITMAP LoadImageAsBitmap(LPCWSTR filename, int width, int height) {
    return LoadPngAsBitmap(filename, width, height);
}

HICON LoadImageAsIcon(LPCWSTR filename, int width, int height) {
    return LoadPngAsIcon(filename, width, height);
}

// 辅助函数：将RGBA数据转换为HBITMAP
HBITMAP CreateBitmapFromRGBA(const unsigned char* data, int width, int height) {
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // 负值表示自上而下的位图
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* bits = nullptr;
    HDC hdc = GetDC(nullptr);
    HBITMAP bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdc);
    
    if (bitmap && bits) {
        // 将RGBA数据转换为BGRA（Windows位图格式）
        unsigned char* bitmapData = static_cast<unsigned char*>(bits);
        for (int i = 0; i < width * height; i++) {
            bitmapData[i * 4 + 0] = data[i * 4 + 2]; // B
            bitmapData[i * 4 + 1] = data[i * 4 + 1]; // G
            bitmapData[i * 4 + 2] = data[i * 4 + 0]; // R
            bitmapData[i * 4 + 3] = data[i * 4 + 3]; // A
        }
    }
    
    return bitmap;
}

// 辅助函数：将RGBA数据转换为HICON
HICON CreateIconFromRGBA(const unsigned char* data, int width, int height) {
    HBITMAP colorBitmap = CreateBitmapFromRGBA(data, width, height);
    if (!colorBitmap) {
        return nullptr;
    }
    
    // 创建掩码位图（基于alpha通道）
    HBITMAP maskBitmap = CreateBitmap(width, height, 1, 1, nullptr);
    if (!maskBitmap) {
        DeleteObject(colorBitmap);
        return nullptr;
    }
    
    HDC maskDC = CreateCompatibleDC(nullptr);
    HBITMAP oldMaskBitmap = (HBITMAP)SelectObject(maskDC, maskBitmap);
    
    // 根据alpha通道设置掩码
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char alpha = data[(y * width + x) * 4 + 3];
            SetPixel(maskDC, x, y, alpha < 128 ? RGB(255, 255, 255) : RGB(0, 0, 0));
        }
    }
    
    SelectObject(maskDC, oldMaskBitmap);
    DeleteDC(maskDC);
    
    // 创建图标
    ICONINFO iconInfo = {0};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = colorBitmap;
    iconInfo.hbmMask = maskBitmap;
    
    HICON icon = CreateIconIndirect(&iconInfo);
    
    DeleteObject(colorBitmap);
    DeleteObject(maskBitmap);
    
    return icon;
}

} // namespace Image

} // namespace Drawing
} // namespace Graphics