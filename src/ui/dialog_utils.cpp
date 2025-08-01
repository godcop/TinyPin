#include "core/stdafx.h"
#include "ui/dialog_utils.h"
#include "core/application.h"
#include "graphics/font_utils.h"
#include "system/language_manager.h"
#include "window/window_cache.h"  // 添加窗口缓存支持

namespace Util {
namespace Dialog {

// 现代化对话框的通用函数
void modernizeDialog(HWND dlg, int dpi)
{
    if (!dlg || !IsWindow(dlg)) return;
    
    // 获取现代系统字体
    HFONT modernFont = Font::FontHelper::getModernSystemFont(dpi);
    if (!modernFont) return;
    
    // 获取字体高度用于调整控件大小
    HDC dc = GetDC(dlg);
    HFONT oldFont = (HFONT)SelectObject(dc, modernFont);
    TEXTMETRIC tm;
    GetTextMetrics(dc, &tm);
    int fontHeight = tm.tmHeight;
    SelectObject(dc, oldFont);
    ReleaseDC(dlg, dc);
    
    // 定义传递给回调函数的数据结构
    struct FontData {
        HFONT modernFont;
        int fontHeight;
        int dpi;
    };
    
    FontData fontData = { modernFont, fontHeight, dpi };
    
    // 为对话框中的所有控件设置现代字体
    EnumChildWindows(dlg, [](HWND child, LPARAM lParam) -> BOOL {
        FontData* data = (FontData*)lParam;
        
        // 获取控件类名
        // 使用缓存的API减少系统调用
        std::wstring className = Window::Cached::getWindowClassName(child);
        if (className.empty()) {
            return TRUE; // 继续枚举
        }
        
        // 为标准控件设置现代字体
        if (className == L"Static" || 
            className == L"Button" ||
            className == L"Edit" ||
            className == L"ComboBox" ||
            className == L"ListBox" ||
            className == L"SysListView32" ||
            className == L"msctls_hotkey32" ||
            className == L"msctls_updown32") {
            
            SendMessage(child, WM_SETFONT, (WPARAM)data->modernFont, TRUE);
            
            // 为ComboBox、Edit和Static控件调整高度
            if (className == L"ComboBox" || className == L"Edit" || className == L"Static") {
                RECT rc;
                GetWindowRect(child, &rc);
                HWND parent = GetParent(child);
                ScreenToClient(parent, (POINT*)&rc.left);
                ScreenToClient(parent, (POINT*)&rc.right);
                
                int newHeight;
                if (className == L"Static") {
                    // 对于Static控件（标签），使用字体高度加少量边距
                    newHeight = data->fontHeight + Font::FontHelper::scaleSizeForDpi(2, data->dpi);
                } else {
                    // 对于ComboBox和Edit控件，使用更多边距
                    newHeight = data->fontHeight + Font::FontHelper::scaleSizeForDpi(8, data->dpi);
                }
                
                // 只有当新高度明显不同时才调整
                int currentHeight = rc.bottom - rc.top;
                if (abs(newHeight - currentHeight) > 2) {
                    SetWindowPos(child, NULL, 0, 0, 
                               rc.right - rc.left, newHeight,
                               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }
        }
        
        return TRUE; // 继续枚举
    }, (LPARAM)&fontData);
    
    // 强制重绘对话框
    InvalidateRect(dlg, NULL, TRUE);
    UpdateWindow(dlg);
}

void localizeDialog(HWND dlg, const std::wstring& dialogName) {
    if (!dlg || !IsWindow(dlg)) return;
    
    // 本地化对话框标题
    localizeDialogTitle(dlg, dialogName);
    
    // 特殊处理属性表对话框
    if (dialogName == L"options") {
        localizePropertySheetButtons(dlg, dialogName);
    }
    
    // 本地化所有子控件
    EnumChildWindows(dlg, [](HWND child, LPARAM lParam) -> BOOL {
        const std::wstring* baseDialogName = reinterpret_cast<const std::wstring*>(lParam);
        
        int controlId = GetDlgCtrlID(child);
        
        // 使用新的控件ID映射系统获取本地化文本
        std::wstring text = LANG_MGR.getControlText(controlId, *baseDialogName);
        
        if (!text.empty()) {
            SetWindowText(child, text.c_str());
        }
        
        return TRUE;
    }, reinterpret_cast<LPARAM>(&dialogName));
}

void localizePropertySheetButtons(HWND propSheet, const std::wstring& dialogName) {
    if (!propSheet || !IsWindow(propSheet)) return;
    
    // 属性表的标准按钮ID
    const int buttonIds[] = { IDOK, IDCANCEL, Constants::ID_APPLY_BUTTON }; // Apply按钮的ID
    
    for (int i = 0; i < 3; i++) {
        HWND button = GetDlgItem(propSheet, buttonIds[i]);
        if (button) {
            // 使用新的控件ID映射系统获取本地化文本
            std::wstring text = LANG_MGR.getControlText(buttonIds[i], dialogName);
            
            if (!text.empty()) {
                SetWindowText(button, text.c_str());
            }
        }
    }
}

void localizeDialogTitle(HWND dlg, const std::wstring& dialogName) {
    if (!dlg || !IsWindow(dlg)) return;
    
    // 使用新的控件ID映射系统获取对话框标题，使用0作为对话框标题的特殊ID
    std::wstring title = LANG_MGR.getControlText(0, dialogName);
    if (!title.empty()) {
        SetWindowText(dlg, title.c_str());
    }
}

void localizeControl(HWND dlg, int controlId, const std::wstring& dialogName, const std::wstring& controlName) {
    if (!dlg || !IsWindow(dlg)) return;
    
    HWND control = GetDlgItem(dlg, controlId);
    if (!control) return;
    
    // 使用新的控件ID映射系统获取本地化文本
    std::wstring text = LANG_MGR.getControlText(controlId, dialogName);
    if (!text.empty()) {
        SetWindowText(control, text.c_str());
    }
}

void localizeMenu(HMENU menu, const std::wstring& menuName) {
    if (!menu) return;
    
    int itemCount = GetMenuItemCount(menu);
    for (int i = 0; i < itemCount; i++) {
        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_STRING;
        mii.dwTypeData = nullptr;
        mii.cch = 0;
        
        // 获取菜单项信息
        if (GetMenuItemInfo(menu, i, TRUE, &mii)) {
            if (mii.hSubMenu) {
                // 递归处理子菜单
                localizeMenu(mii.hSubMenu, menuName);
            } else if (mii.wID > 0) {
                // 使用新的控件ID映射系统本地化菜单项
                localizeMenuItem(menu, mii.wID, menuName, L"");
            }
        }
    }
}

void localizeMenuItem(HMENU menu, UINT itemId, const std::wstring& menuName, const std::wstring& itemName) {
    if (!menu) return;
    
    // 使用新的控件ID映射系统获取菜单项文本
    std::wstring text = LANG_MGR.getControlText(itemId, menuName);
    
    if (!text.empty()) {
        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_STRING;
        mii.dwTypeData = const_cast<LPWSTR>(text.c_str());
        mii.cch = static_cast<UINT>(text.length());
        
        SetMenuItemInfo(menu, itemId, FALSE, &mii);
    }
}

} // namespace Dialog

} // namespace Util