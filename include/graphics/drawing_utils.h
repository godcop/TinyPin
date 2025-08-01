#pragma once

#include "core/common.h"
#include <memory>

namespace Graphics {
namespace Drawing {
    // 基础图形操作函数
    HRGN makeRegionFromBmp(HBITMAP bmp, COLORREF clrMask);
    BOOL rectangle(HDC dc, const RECT& rc);
    bool getBmpSize(HBITMAP bmp, SIZE& sz);
    bool remapBmpColors(HBITMAP bmp, COLORREF clrs[][2], int cnt);
    bool rectContains(const RECT& rc1, const RECT& rc2);
    bool getBmpSizeAndBpp(HBITMAP bmp, SIZE& sz, int& bpp);
    
    // 区域辅助函数，用于替代s::Win::RgnH::create
    HRGN createRegionFromBitmap(HBITMAP bmp, COLORREF transparentColor);

    // 图像加载命名空间
    namespace Image {
        // 图像缩放辅助类
        class ImageScaler {
        public:
            // 使用双线性插值缩放RGBA数据
            static std::unique_ptr<unsigned char[]> scaleRGBA(
                const unsigned char* srcData, int srcWidth, int srcHeight,
                int dstWidth, int dstHeight);
        };

        // PNG图像加载功能
        HBITMAP LoadPngAsBitmap(LPCWSTR filename);
        HBITMAP LoadPngAsBitmap(LPCWSTR filename, int width, int height);
        HICON LoadPngAsIcon(LPCWSTR filename, int width = 0, int height = 0);

        // 从内存加载PNG图像
        HBITMAP LoadPngFromMemoryAsBitmap(const unsigned char* data, int len);
        HICON LoadPngFromMemoryAsIcon(const unsigned char* data, int len, int width = 0, int height = 0);

        // 通用图像加载功能（支持stb_image支持的所有格式）
        // 支持格式：JPEG, PNG, TGA, BMP, PSD, GIF, HDR, PIC, PNM
        HBITMAP LoadImageAsBitmap(LPCWSTR filename);
        HBITMAP LoadImageAsBitmap(LPCWSTR filename, int width, int height);
        HICON LoadImageAsIcon(LPCWSTR filename, int width = 0, int height = 0);

        // 辅助函数：RGBA数据转换
        HBITMAP CreateBitmapFromRGBA(const unsigned char* data, int width, int height);
        HICON CreateIconFromRGBA(const unsigned char* data, int width, int height);
    } // namespace Image
}
}