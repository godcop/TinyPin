#include "core/stdafx.h"
#include "core/application.h"
#include "options/options.h"
#include "options/language_options.h"
#include "resource.h"
#include "system/language_manager.h"
#include "system/logger.h"

extern Options opt;

struct LanguageInfo {
    std::wstring code;
    std::wstring name;
    
    LanguageInfo(const std::wstring& c, const std::wstring& n) : code(c), name(n) {}
};

INT_PTR CALLBACK OptLang::dlgProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        evInitDialog(wnd);
        return TRUE;

    case WM_NOTIFY:
        {
            NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
            if (nmhdr->code == PSN_KILLACTIVE) {
                SetWindowLongPtr(wnd, DWLP_MSGRESULT, validate(wnd) ? FALSE : TRUE);
                return TRUE;
            }
            if (nmhdr->code == PSN_APPLY) {
                apply(wnd);
                return TRUE;
            }
        }
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            // Language selection changed
            SendMessage(GetParent(wnd), PSM_CHANGED, (WPARAM)wnd, 0);
        }
        break;

    case WM_DELETEITEM:
        {
            DELETEITEMSTRUCT* dis = reinterpret_cast<DELETEITEMSTRUCT*>(lParam);
            if (dis->itemData) {
                delete reinterpret_cast<LanguageInfo*>(dis->itemData);
            }
        }
        return TRUE;

    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT& mis = *reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
            int currentDpi = Graphics::DpiManager::getDpiForWindow(wnd);
            HFONT modernFont = Util::Font::FontHelper::getModernSystemFont(currentDpi);
            mis.itemHeight = Util::Font::FontHelper::getHeight(modernFont) + 
                           Util::Font::FontHelper::scaleSizeForDpi(2, currentDpi);
        }
        break;

    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT& dis = *reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            RECT& rc = dis.rcItem;
            HDC dc = dis.hDC;
            
            if (dis.itemID != -1) {
                bool sel = dis.itemState & ODS_SELECTED;
                FillRect(dc, &rc, GetSysColorBrush(sel ? COLOR_HIGHLIGHT : COLOR_WINDOW));

                if (dis.itemData) {
                    LanguageInfo& data = *reinterpret_cast<LanguageInfo*>(dis.itemData);

                    // 使用现代字体进行绘制
                    int currentDpi = Graphics::DpiManager::getDpiForWindow(wnd);
                    HFONT modernFont = Util::Font::FontHelper::getModernSystemFont(currentDpi);
                    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(dc, modernFont));

                    // 获取字体度量以正确计算垂直位置
                    TEXTMETRIC tm;
                    GetTextMetrics(dc, &tm);
                    int textHeight = tm.tmHeight;
                    int itemHeight = rc.bottom - rc.top;
                    int yPos = rc.top + (itemHeight - textHeight) / 2; // 垂直居中

                    UINT     orgAlign  = SetTextAlign(dc, TA_LEFT);
                    COLORREF orgTxtClr = SetTextColor(dc, GetSysColor(sel ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
                    int      orgBkMode = SetBkMode(dc, TRANSPARENT);

                    int margin = Util::Font::FontHelper::scaleSizeForDpi(2, currentDpi);
                    
                    // 绘制语言名称（左对齐）
                    TextOut(dc, rc.left + margin, yPos, data.name.c_str(), static_cast<int>(data.name.length()));
                    
                    // 绘制语言代码（右对齐，灰色）
                    if (!data.code.empty()) {
                        SetTextAlign(dc, TA_RIGHT);
                        if (!sel) SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
                        TextOut(dc, rc.right - margin, yPos, data.code.c_str(), static_cast<int>(data.code.length()));
                    }

                    SetBkMode(dc, orgBkMode);
                    SetTextColor(dc, orgTxtClr);
                    SetTextAlign(dc, orgAlign);
                    SelectObject(dc, oldFont);
                }
            }
        }
        break;
    }

    return FALSE;
}

void OptLang::evInitDialog(HWND wnd)
{
    // 获取当前DPI并现代化对话框
    int currentDpi = Graphics::DpiManager::getDpiForWindow(wnd);
    Util::Dialog::modernizeDialog(wnd, currentDpi);
    
    // 本地化对话框文本
    Util::Dialog::localizeDialog(wnd, L"language");
    
    // 加载可用语言
    loadLanguages(GetDlgItem(wnd, IDC_UILANG), opt.language);
}

void OptLang::loadLanguages(HWND combo, const std::wstring& currentLang)
{
    // Clear existing items
    SendMessage(combo, CB_RESETCONTENT, 0, 0);
    
    // Add auto-detect option
    std::wstring autoDetectText = LANG_MGR.getString(L"dialogs.language.auto_detect");
    if (autoDetectText.empty() || autoDetectText == L"dialogs.language.auto_detect") {
        autoDetectText = LANG_MGR.getString(L"language.auto_detect");
        if (autoDetectText.empty() || autoDetectText == L"language.auto_detect") {
            // 根据当前语言提供回退文本
            autoDetectText = (LANG_MGR.getCurrentLanguage() == L"en_US") ? L"Auto Detect" : L"自动检测";
        }
    }
    LanguageInfo* autoInfo = new LanguageInfo(L"", autoDetectText);
    int index = static_cast<int>(SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(autoInfo->name.c_str())));
    SendMessage(combo, CB_SETITEMDATA, index, reinterpret_cast<LPARAM>(autoInfo));
    
    // Add available languages
    std::vector<std::pair<std::wstring, std::wstring>> languages = LANG_MGR.getAvailableLanguages();
    
    for (const auto& langPair : languages) {
        LanguageInfo* info = new LanguageInfo(langPair.first, langPair.second);
        index = static_cast<int>(SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(info->name.c_str())));
        SendMessage(combo, CB_SETITEMDATA, index, reinterpret_cast<LPARAM>(info));
    }
    
    // Select current language
    int count = static_cast<int>(SendMessage(combo, CB_GETCOUNT, 0, 0));
    
    // Use current language from language manager if empty
    std::wstring langToSelect = currentLang.empty() ? LANG_MGR.getCurrentLanguage() : currentLang;
    
    bool found = false;
    for (int i = 0; i < count; i++) {
        LanguageInfo* info = reinterpret_cast<LanguageInfo*>(SendMessage(combo, CB_GETITEMDATA, i, 0));
        if (info && info->code == langToSelect) {
            SendMessage(combo, CB_SETCURSEL, i, 0);
            found = true;
            break;
        }
    }
    
    // If no selection made, select auto-detect (first item)
    if (!found) {
        SendMessage(combo, CB_SETCURSEL, 0, 0);
    }
}

std::wstring OptLang::getComboSel(HWND combo)
{
    int sel = static_cast<int>(SendMessage(combo, CB_GETCURSEL, 0, 0));
    if (sel == CB_ERR) {
        return L"";
    }
    
    LanguageInfo* info = reinterpret_cast<LanguageInfo*>(SendMessage(combo, CB_GETITEMDATA, sel, 0));
    return info ? info->code : L"";
}

bool OptLang::validate(HWND wnd)
{
    // No validation needed for language selection
    return true;
}

void OptLang::apply(HWND wnd)
{
    std::wstring newLang = getComboSel(GetDlgItem(wnd, IDC_UILANG));
    
    if (opt.language != newLang) {
        // Update the language setting
        opt.language = newLang;
        
        // 立即保存所有设置到INI文件
        opt.saveImmediately();
        
        // Apply language change to language manager
        if (newLang.empty()) {
            LANG_MGR.autoSelectLanguage();
        } else {
            LANG_MGR.setLanguage(newLang);
        }
        
        // Get the actual current language after setting
        std::wstring currentLang = LANG_MGR.getCurrentLanguage();
    
    }
}