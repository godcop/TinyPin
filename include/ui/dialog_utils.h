#pragma once

#include "core/common.h"
#include "system/language_manager.h"
#include "graphics/font_utils.h"
#include "window/window_helper.h"
#include <string>

namespace Util {

    // 对话框现代化和本地化工具
    namespace Dialog {
        // 现代化对话框的通用函数
        void modernizeDialog(HWND dlg, int dpi = 96);
        
        // 对话框本地化功能
        void localizeDialog(HWND dlg, const std::wstring& dialogName);
        void localizeDialogTitle(HWND dlg, const std::wstring& dialogName);
        void localizePropertySheetButtons(HWND propSheet, const std::wstring& dialogName);
        void localizeControl(HWND dlg, int controlId, const std::wstring& dialogName, const std::wstring& controlName);
        void localizeMenu(HMENU menu, const std::wstring& menuName);
        void localizeMenuItem(HMENU menu, UINT itemId, const std::wstring& menuName, const std::wstring& itemName);
    }

    // 窗口控件组操作（从 window_utils 中移动到这里，因为主要用于对话框）
    namespace Wnd {
        void enableGroup(HWND wnd, int id, bool mode);
    }

}