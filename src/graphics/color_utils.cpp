#include "core/stdafx.h"
#include "graphics/color_utils.h"
#include <algorithm>
#include <vector>

namespace Graphics {
namespace Color {

namespace {
    // 内部辅助函数：RGB到HLS转换
    void RGBtoHLS(COLORREF clr, double& h, double& l, double& s) {
        double r = GetRValue(clr) / 255.0;
        double g = GetGValue(clr) / 255.0;
        double b = GetBValue(clr) / 255.0;
        
        double maxval = max(r, max(g, b));
        double minval = min(r, min(g, b));
        
        // 计算亮度
        l = (maxval + minval) / 2.0;
        
        if (maxval == minval) {
            // 灰度
            h = s = 0.0;
        } else {
            // 计算饱和度
            double delta = maxval - minval;
            s = (l <= 0.5) ? (delta / (maxval + minval)) : (delta / (2.0 - maxval - minval));
            
            // 计算色调
            if (r == maxval) {
                h = (g - b) / delta;
            } else if (g == maxval) {
                h = 2.0 + (b - r) / delta;
            } else {
                h = 4.0 + (r - g) / delta;
            }
            
            h /= 6.0;
            if (h < 0.0) h += 1.0;
        }
    }
    
    // 内部辅助函数：HLS到RGB转换
    COLORREF HLStoRGB(double h, double l, double s) {
        double r, g, b;
        
        if (s == 0.0) {
            // 灰度
            r = g = b = l;
        } else {
            double q = (l < 0.5) ? (l * (1.0 + s)) : (l + s - l * s);
            double p = 2.0 * l - q;
            
            double tr = h + 1.0/3.0;
            double tg = h;
            double tb = h - 1.0/3.0;
            
            if (tr > 1.0) tr -= 1.0;
            if (tb < 0.0) tb += 1.0;
            
            // HueToRGB lambda函数
            auto HueToRGB = [](double p, double q, double t) {
                if (t < 0.0) t += 1.0;
                if (t > 1.0) t -= 1.0;
                
                if (t < 1.0/6.0) return p + (q - p) * 6.0 * t;
                if (t < 1.0/2.0) return q;
                if (t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6.0;
                return p;
            };
            
            r = HueToRGB(p, q, tr);
            g = HueToRGB(p, q, tg);
            b = HueToRGB(p, q, tb);
        }
        
        return RGB(static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255));
    }
}

COLORREF light(COLORREF clr)
{
    double h, l, s;
    RGBtoHLS(clr, h, l, s);
    
    // 增加亮度
    l = min(1.0, l + 0.25);
    
    return HLStoRGB(h, l, s);
}

COLORREF dark(COLORREF clr)
{
    double h, l, s;
    RGBtoHLS(clr, h, l, s);
    
    // 降低亮度
    l = max(0.0, l - 0.25);
    
    return HLStoRGB(h, l, s);
}

// 将COLORREF颜色值转换为RGB格式字符串 (例如: "255,0,111")
std::wstring colorToRgbString(COLORREF color)
{
    int r = GetRValue(color);
    int g = GetGValue(color);
    int b = GetBValue(color);
    
    return std::to_wstring(r) + L"," + std::to_wstring(g) + L"," + std::to_wstring(b);
}

// 将RGB格式字符串转换为COLORREF颜色值
COLORREF rgbStringToColor(const std::wstring& rgbStr)
{
    if (rgbStr.empty()) {
        return RGB(255, 0, 0); // 默认红色
    }
    
    // 检查是否为RGB格式 (包含逗号)
    if (!isRgbFormat(rgbStr)) {
        // 如果不是RGB格式，尝试作为整数值解析
        return static_cast<COLORREF>(_wtoi(rgbStr.c_str()));
    }
    
    // 解析RGB格式字符串
    std::vector<int> values;
    std::wstring temp = rgbStr;
    size_t pos = 0;
    
    // 分割字符串
    while ((pos = temp.find(L',')) != std::wstring::npos) {
        std::wstring token = temp.substr(0, pos);
        values.push_back(_wtoi(token.c_str()));
        temp.erase(0, pos + 1);
    }
    // 添加最后一个值
    if (!temp.empty()) {
        values.push_back(_wtoi(temp.c_str()));
    }
    
    // 确保有3个值
    if (values.size() != 3) {
        return RGB(255, 0, 0); // 默认红色
    }
    
    // 限制值在0-255范围内
    int r = max(0, min(255, values[0]));
    int g = max(0, min(255, values[1]));
    int b = max(0, min(255, values[2]));
    
    return RGB(r, g, b);
}

// 检查字符串是否为RGB格式
bool isRgbFormat(const std::wstring& str)
{
    // RGB格式应该包含逗号
    return str.find(L',') != std::wstring::npos;
}

} // namespace Color
} // namespace Graphics