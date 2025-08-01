#include "core/stdafx.h"
#include "core/application.h"
#include "pin/pin_shape.h"
#include "resource.h"
#include "system/logger.h"

// 外部全局选项对象
extern Options opt;


PinShape::PinShape() : bmp(nullptr), rgn(nullptr)
{
    sz.cx = sz.cy = 1;
}


PinShape::~PinShape()
{
    DeleteObject(bmp);
    DeleteObject(rgn);
}


namespace {
    // 获取默认图钉图像路径的辅助函数
    std::wstring getDefaultPinImagePath() {
        static std::wstring defaultPath;
        if (defaultPath.empty()) {
            std::wstring moduleDir = Foundation::FileUtils::getDirPath(Foundation::FileUtils::getModulePath(app.inst));
            defaultPath = moduleDir + L"assets\\images\\TinyPin.png";
        }
        return defaultPath;
    }
}

bool PinShape::initImage()
{
    if (bmp) {
        DeleteObject(bmp);
        bmp = nullptr;
    }

    // 从配置中获取图钉图像路径
    const std::wstring& imagePath = opt.pinImagePath;
    
    // 尝试从配置的路径加载图钉图像
    bmp = loadBitmapFromPath(imagePath);
    
    if (!bmp) {
        // 如果配置的路径加载失败，尝试加载默认图标
        bmp = loadBitmapFromPath(getDefaultPinImagePath());
        
        // 注意：不再回退到IDB_PIN资源（pin.bmp），确保始终使用TinyPin.png
    }
    
    return bmp != nullptr;
}


bool PinShape::initShape()
{
    return initShapeForDpi(96); // Default DPI
}


bool PinShape::initShapeForDpi(int dpi)
{
    if (rgn) {
        DeleteObject(rgn);
        rgn = nullptr;
        sz.cx = sz.cy = 1;
    }

    HBITMAP bmp = nullptr;
    
    // 从配置中获取图钉图像路径
    const std::wstring& imagePath = opt.pinImagePath;
    
    // 尝试从配置的路径加载图钉图像
    bmp = loadBitmapFromPath(imagePath);
    
    // 如果配置的路径加载失败，尝试加载默认图标
    if (!bmp) {
        bmp = loadBitmapFromPath(getDefaultPinImagePath());
        
        // 注意：不再回退到IDB_PIN资源（pin.bmp），确保始终使用TinyPin.png
    }
    
    if (bmp) {
        // 强制设置图钉尺寸为 32×32 像素
        sz.cx = 32;
        sz.cy = 32;
        
        // 如果原始位图尺寸不是 32×32，需要缩放
        SIZE originalSize;
        if (Graphics::Drawing::getBmpSize(bmp, originalSize)) {
            if (originalSize.cx != 32 || originalSize.cy != 32) {
                // 创建 32×32 的缩放位图
                HBITMAP scaledBmp = createFixedSizeBitmap(bmp, 32, 32);
                if (scaledBmp) {
                    DeleteObject(bmp);
                    bmp = scaledBmp;
                }
            }
        }
        
        // 从位图创建区域
        HRGN originalRgn = Graphics::Drawing::createRegionFromBitmap(bmp, RGB(255,0,255));
        if (originalRgn) {
            rgn = originalRgn;
        }
        
        DeleteObject(bmp);
    }

    return rgn != nullptr;
}


bool PinShape::initImageForDpi(int dpi)
{
    if (bmp) {
        DeleteObject(bmp);
        bmp = nullptr;
    }

    HBITMAP originalBmp = nullptr;
    
    // 从配置中获取图钉图像路径
    const std::wstring& imagePath = opt.pinImagePath;
    
    // 尝试从配置的路径加载图钉图像，直接缩放到32x32
    originalBmp = loadBitmapFromPath(imagePath);
    
    // 如果配置的路径加载失败，尝试加载默认图标
    if (!originalBmp) {
        originalBmp = loadBitmapFromPath(getDefaultPinImagePath());
        
        // 注意：不再回退到IDB_PIN资源（pin.bmp），确保始终使用TinyPin.png
    }
    
    if (originalBmp) {
        // 确保图像尺寸为32x32
        SIZE originalSize;
        if (Graphics::Drawing::getBmpSize(originalBmp, originalSize)) {
            if (originalSize.cx != 32 || originalSize.cy != 32) {
                HBITMAP scaledBmp = createFixedSizeBitmap(originalBmp, 32, 32);
                if (scaledBmp) {
                    DeleteObject(originalBmp);
                    originalBmp = scaledBmp;
                }
            }
        }
    }
    
    if (!originalBmp) return false;
    
    bmp = originalBmp;
    
    return bmp != nullptr;
}


HBITMAP PinShape::createScaledBitmap(HBITMAP srcBmp, int dpi)
{
    if (!srcBmp) return nullptr;
    
    SIZE srcSize;
    if (!Graphics::Drawing::getBmpSize(srcBmp, srcSize)) return nullptr;
    
    SIZE scaledSize = srcSize;
    Graphics::DpiManager::scaleSizeForDpi(scaledSize, dpi);
    
    // If no scaling needed, return copy of original
    if (scaledSize.cx == srcSize.cx && scaledSize.cy == srcSize.cy) {
        return (HBITMAP)CopyImage(srcBmp, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    }
    
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcSrc = CreateCompatibleDC(hdcScreen);
    HDC hdcDst = CreateCompatibleDC(hdcScreen);
    
    HBITMAP scaledBmp = CreateCompatibleBitmap(hdcScreen, scaledSize.cx, scaledSize.cy);
    
    if (scaledBmp) {
        HGDIOBJ oldSrcBmp = SelectObject(hdcSrc, srcBmp);
        HGDIOBJ oldDstBmp = SelectObject(hdcDst, scaledBmp);
        
        // Use high-quality scaling
        SetStretchBltMode(hdcDst, HALFTONE);
        SetBrushOrgEx(hdcDst, 0, 0, nullptr);
        
        StretchBlt(hdcDst, 0, 0, scaledSize.cx, scaledSize.cy,
                   hdcSrc, 0, 0, srcSize.cx, srcSize.cy, SRCCOPY);
        
        SelectObject(hdcSrc, oldSrcBmp);
        SelectObject(hdcDst, oldDstBmp);
    }
    
    DeleteDC(hdcSrc);
    DeleteDC(hdcDst);
    ReleaseDC(nullptr, hdcScreen);
    
    return scaledBmp;
}


HBITMAP PinShape::createFixedSizeBitmap(HBITMAP srcBmp, int width, int height)
{
    if (!srcBmp) return nullptr;
    
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcSrc = CreateCompatibleDC(hdcScreen);
    HDC hdcDst = CreateCompatibleDC(hdcScreen);
    
    HBITMAP scaledBmp = CreateCompatibleBitmap(hdcScreen, width, height);
    
    if (scaledBmp) {
        HGDIOBJ oldSrcBmp = SelectObject(hdcSrc, srcBmp);
        HGDIOBJ oldDstBmp = SelectObject(hdcDst, scaledBmp);
        
        // 获取源位图尺寸
        SIZE srcSize;
        if (!Graphics::Drawing::getBmpSize(srcBmp, srcSize)) {
            srcSize.cx = srcSize.cy = 1;
        }
        
        // 使用高质量缩放
        SetStretchBltMode(hdcDst, HALFTONE);
        SetBrushOrgEx(hdcDst, 0, 0, nullptr);
        
        StretchBlt(hdcDst, 0, 0, width, height,
                   hdcSrc, 0, 0, srcSize.cx, srcSize.cy, SRCCOPY);
        
        SelectObject(hdcSrc, oldSrcBmp);
        SelectObject(hdcDst, oldDstBmp);
    }
    
    DeleteDC(hdcSrc);
    DeleteDC(hdcDst);
    ReleaseDC(nullptr, hdcScreen);
    
    return scaledBmp;
}


HRGN PinShape::createScaledRegion(HRGN srcRgn, int dpi)
{
    if (!srcRgn) return nullptr;
    
    double scale = (double)dpi / 96.0;
    
    // Get region data
    DWORD dataSize = GetRegionData(srcRgn, 0, nullptr);
    if (dataSize == 0) return nullptr;
    
    std::vector<BYTE> buffer(dataSize);
    RGNDATA* rgnData = (RGNDATA*)buffer.data();
    
    if (GetRegionData(srcRgn, dataSize, rgnData) == 0) return nullptr;
    
    // Scale all rectangles
    RECT* rects = (RECT*)rgnData->Buffer;
    for (DWORD i = 0; i < rgnData->rdh.nCount; i++) {
        rects[i].left = (LONG)(rects[i].left * scale);
        rects[i].top = (LONG)(rects[i].top * scale);
        rects[i].right = (LONG)(rects[i].right * scale);
        rects[i].bottom = (LONG)(rects[i].bottom * scale);
    }
    
    // Update header
    rgnData->rdh.rcBound.left = (LONG)(rgnData->rdh.rcBound.left * scale);
    rgnData->rdh.rcBound.top = (LONG)(rgnData->rdh.rcBound.top * scale);
    rgnData->rdh.rcBound.right = (LONG)(rgnData->rdh.rcBound.right * scale);
    rgnData->rdh.rcBound.bottom = (LONG)(rgnData->rdh.rcBound.bottom * scale);
    
    return ExtCreateRegion(nullptr, dataSize, rgnData);
}


bool PinShape::initImageFromPath(const std::wstring& imagePath)
{
    if (bmp) {
        DeleteObject(bmp);
        bmp = nullptr;
    }

    // 从指定路径加载图钉图像
    bmp = loadBitmapFromPath(imagePath);
    
    if (!bmp) {
        // 如果指定路径加载失败，尝试加载默认图标
        std::wstring moduleDir = Foundation::FileUtils::getDirPath(Foundation::FileUtils::getModulePath(app.inst));
        std::wstring defaultPath = moduleDir + L"assets\\images\\TinyPin.png";
        
        bmp = loadBitmapFromPath(defaultPath);
        
        // 注意：不再回退到IDB_PIN资源（pin.bmp），确保始终使用TinyPin.png
        // 如果TinyPin.png也不存在，bmp将保持为nullptr，这样可以避免显示不一致的图标
    }
    
    return bmp != nullptr;
}


bool PinShape::initShapeFromPath(const std::wstring& imagePath)
{
    if (rgn) {
        DeleteObject(rgn);
        rgn = nullptr;
        sz.cx = sz.cy = 1;
    }

    HBITMAP bmp = nullptr;
    
    // 从指定路径加载图钉图像
    bmp = loadBitmapFromPath(imagePath);
    
    // 如果指定路径加载失败，尝试加载默认图标
    if (!bmp) {
        std::wstring moduleDir = Foundation::FileUtils::getDirPath(Foundation::FileUtils::getModulePath(app.inst));
        std::wstring defaultPath = moduleDir + L"assets\\images\\TinyPin.png";
        
        bmp = loadBitmapFromPath(defaultPath);
        
        // 注意：不再回退到IDB_PIN资源（pin.bmp），确保始终使用TinyPin.png
        // 如果TinyPin.png也不存在，将无法创建形状区域
    }
    
    if (bmp) {
        // 强制设置图钉尺寸为 32×32 像素
        sz.cx = 32;
        sz.cy = 32;
        
        // 如果原始位图尺寸不是 32×32，需要缩放
        SIZE originalSize;
        if (Graphics::Drawing::getBmpSize(bmp, originalSize)) {
            if (originalSize.cx != 32 || originalSize.cy != 32) {
                // 创建 32×32 的缩放位图
                HBITMAP scaledBmp = createFixedSizeBitmap(bmp, 32, 32);
                if (scaledBmp) {
                    DeleteObject(bmp);
                    bmp = scaledBmp;
                }
            }
        }
        
        // 从位图创建区域
        HRGN originalRgn = Graphics::Drawing::createRegionFromBitmap(bmp, RGB(255,0,255));
        if (originalRgn) {
            rgn = originalRgn;
        }
        
        DeleteObject(bmp);
    }

    return rgn != nullptr;
}


HBITMAP PinShape::loadBitmapFromPath(const std::wstring& imagePath)
{
    if (imagePath.empty()) {
        return nullptr;
    }
    
    // 检查路径是否为相对路径，如果是则转换为绝对路径
    std::wstring fullPath = imagePath;
    if (PathIsRelative(imagePath.c_str())) {
        std::wstring moduleDir = Foundation::FileUtils::getDirPath(Foundation::FileUtils::getModulePath(app.inst));
        fullPath = moduleDir + imagePath;
    }
    
    // 检查文件是否存在
    if (!PathFileExists(fullPath.c_str())) {
        return nullptr;
    }
    
    // 使用通用图像加载函数，支持stb_image支持的所有格式
    // 支持格式：JPEG, PNG, TGA, BMP, PSD, GIF, HDR, PIC, PNM
    HBITMAP bitmap = Graphics::Drawing::Image::LoadImageAsBitmap(fullPath.c_str());
    
    if (bitmap) {
        return bitmap;
    }
    
    // 如果stb_image加载失败，尝试使用Windows原生API加载（兼容性回退）
    std::wstring ext = PathFindExtension(fullPath.c_str());
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    
    if (ext == L".bmp") {
        // 加载BMP格式图像
        return (HBITMAP)LoadImage(nullptr, fullPath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    } else if (ext == L".ico") {
        // 加载ICO格式图像并转换为位图
        HICON icon = (HICON)LoadImage(nullptr, fullPath.c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
        if (icon) {
            ICONINFO iconInfo;
            if (GetIconInfo(icon, &iconInfo)) {
                HBITMAP result = (HBITMAP)CopyImage(iconInfo.hbmColor, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
                DeleteObject(iconInfo.hbmColor);
                DeleteObject(iconInfo.hbmMask);
                DestroyIcon(icon);
                return result;
            }
            DestroyIcon(icon);
        }
    }
    
    return nullptr;
}
