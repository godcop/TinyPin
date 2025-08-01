#include "core/stdafx.h"
#include "graphics/window_highlighter.h"
#include "core/application.h"
#include "system/logger.h"

void Graphics::markWindow(HWND wnd, bool mode)
{
    const int blinkDelay = Constants::DEFAULT_BLINK_DELAY;  // 毫秒
    // 高亮边框的厚度
    const int width = 3;
    // 第一个值可以变化；第二个应该为零
    const int flashes = mode ? 1 : 0;
    // 如果窗口最大化时的收缩量，以使高亮可见
    const int zoomFix = IsZoomed(wnd) ? GetSystemMetrics(SM_CXFRAME) : 0;

    // 当启用合成时，禁止在玻璃框架上绘制
    // （GetWindowDC()返回一个裁剪框架的DC）；在这种情况下，
    // 我们使用屏幕DC并绘制一个简单的矩形（因为GetWindowRgn()
    // 返回ERROR）；这个矩形与目标前面的其他窗口重叠，
    // 但总比没有好
    const bool composition = app.dwm.isCompositionEnabled();

    HDC dc = composition ? GetDC(0) : GetWindowDC(wnd);
    if (dc) {
        int orgRop2 = SetROP2(dc, R2_XORPEN);

        auto rgnGuard = Util::RAII::makeRegionGuard(CreateRectRgn(0,0,0,0));
        bool hasRgn = rgnGuard.isValid() && GetWindowRgn(wnd, rgnGuard.get()) != ERROR;
        const int loops = flashes*2+1;

        if (hasRgn) {
            for (int m = 0; m < loops; ++m) {
                FrameRgn(dc, rgnGuard.get(), HBRUSH(GetStockObject(WHITE_BRUSH)), width, width);
                GdiFlush();
                if (mode && m < loops-1)
                    Sleep(blinkDelay);
            }
        }
        else {
            RECT rc;
            GetWindowRect(wnd, &rc);
            if (!composition)
                OffsetRect(&rc, -rc.left, -rc.top);
            InflateRect(&rc, -zoomFix, -zoomFix);

            HGDIOBJ orgPen = SelectObject(dc, GetStockObject(WHITE_PEN));
            HGDIOBJ orgBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));

            RECT tmp;
            for (int m = 0; m < loops; ++m) {
                CopyRect(&tmp, &rc);
                for (int n = 0; n < width; ++n) {
                    Rectangle(dc, tmp.left, tmp.top, tmp.right, tmp.bottom);
                    InflateRect(&tmp, -1, -1);
                }
                GdiFlush();
                if (mode && m < loops-1)
                    Sleep(blinkDelay);
            }
            SelectObject(dc, orgBrush);
            SelectObject(dc, orgPen);
        }

        SetROP2(dc, orgRop2);
        ReleaseDC(composition ? 0 : wnd, dc);
    }
}