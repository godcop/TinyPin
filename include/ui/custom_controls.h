#pragma once

#include "core/common.h"
#include <memory>
#include <string>

namespace Util {

    namespace CustomControls {
        // 链接控件类，用于创建可点击的超链接
        class LinkCtrl {
        public:
            // 静态工厂方法，用于子类化现有控件
            static LinkCtrl* subclass(HWND parent, int id);
            
            // 智能指针版本的工厂方法
            static std::unique_ptr<LinkCtrl> subclassUnique(HWND parent, int id);
            
            // 设置不同状态下的字体
            void setFonts(HFONT normal, HFONT hover, HFONT visited);
            
            // 设置不同状态下的颜色
            void setColors(COLORREF normal, COLORREF hover, COLORREF visited);
            
            // 设置链接URL
            void setUrl(LPCWSTR url);
            
            // 析构函数，确保正确清理
            ~LinkCtrl();
            
            // 构造函数（仅供内部使用，请使用静态工厂方法）
            LinkCtrl(HWND parent, int id);
            
        private:
            HWND m_hwnd;                // 控件窗口句柄
            HWND m_parent;              // 父窗口句柄
            int m_id;                   // 控件ID
            std::wstring m_url;         // 链接URL
            HFONT m_normalFont;         // 正常状态字体
            HFONT m_hoverFont;          // 悬停状态字体
            HFONT m_visitedFont;        // 访问过状态字体
            COLORREF m_normalColor;     // 正常状态颜色
            COLORREF m_hoverColor;      // 悬停状态颜色
            COLORREF m_visitedColor;    // 访问过状态颜色
            bool m_isHover;             // 是否处于悬停状态
            WNDPROC m_originalProc;     // 原始窗口过程
            
            // 禁止复制和移动
            LinkCtrl(const LinkCtrl&) = delete;
            LinkCtrl& operator=(const LinkCtrl&) = delete;
            LinkCtrl(LinkCtrl&&) = delete;
            LinkCtrl& operator=(LinkCtrl&&) = delete;
            
            // 链接控件的窗口过程
            static LRESULT CALLBACK linkProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        };
    }

}