#pragma once

// New language options dialog
// Uses JSON-based language files instead of DLL resources
class OptLang {
public:
    static INT_PTR CALLBACK dlgProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    static bool validate(HWND wnd);
    static void apply(HWND wnd);
    static void evInitDialog(HWND wnd);
    static void loadLanguages(HWND combo, const std::wstring& currentLang);
    static std::wstring getComboSel(HWND combo);
};