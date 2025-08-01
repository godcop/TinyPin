#pragma once


// 图钉图像和区域。
// 由图钉窗口用来塑造和绘制自己。
//
class PinShape {
public:
    // 禁止复制
    PinShape(const PinShape&) = delete;
    PinShape& operator=(const PinShape&) = delete;
public:
    PinShape();
    ~PinShape();

    bool initShape();
    bool initImage();
    bool initShapeForDpi(int dpi);
    bool initImageForDpi(int dpi);
    
    // 从指定文件路径加载图钉图像
    bool initImageFromPath(const std::wstring& imagePath);
    bool initShapeFromPath(const std::wstring& imagePath);

    HBITMAP getBmp() const { return bmp;  }
    HRGN    getRgn() const { return rgn;  }
    int     getW()   const { return sz.cx; }
    int     getH()   const { return sz.cy; }

protected:
    HBITMAP bmp;
    HRGN    rgn;
    SIZE    sz;
    
    // DPI缩放的辅助方法
    HBITMAP createScaledBitmap(HBITMAP srcBmp, int dpi);
    HRGN createScaledRegion(HRGN srcRgn, int dpi);
    
    // 固定尺寸缩放的辅助方法
    HBITMAP createFixedSizeBitmap(HBITMAP srcBmp, int width, int height);
    
    // 从文件路径加载位图的辅助方法
    HBITMAP loadBitmapFromPath(const std::wstring& imagePath);
};