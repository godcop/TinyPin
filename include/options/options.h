#pragma once

#include "foundation/error_handler.h"
#include "resource.h"
#include "system/language_manager.h"

struct HotKey;


// Hotkey item.
// Manages its own activation, persistence and UI interaction.
//
struct HotKey {
    int  id;
    UINT vk;
    UINT mod;

    explicit HotKey(int id_, UINT vk_ = 0, UINT mod_ = 0)
        : id(id_), vk(vk_), mod(mod_) {}

    bool operator==(const HotKey& other) const {
        return vk == other.vk && mod == other.mod;
    }
    bool operator!=(const HotKey& other) const {
        return !(*this == other);
    }

    bool set(HWND wnd) const
    {
        return !!RegisterHotKey(wnd, id, mod, vk);
    }

    bool unset(HWND wnd) const
    {
        return !!UnregisterHotKey(wnd, id);
    }

    bool load(HKEY key, LPCWSTR val);
    bool save(HKEY key, LPCWSTR val) const;

    void getUI(HWND wnd, int id)
    {
        LRESULT res = SendDlgItemMessage(wnd, id, HKM_GETHOTKEY, 0, 0);
        vk  = LOBYTE(res);
        mod = HIBYTE(res);
        if (!vk)
            mod = 0;
    }

    void setUI(HWND wnd, int id) const
    {
        SendDlgItemMessage(wnd, id, HKM_SETHOTKEY, MAKEWORD(vk, mod), 0);
    }

};


// Autopin rule.
// Manages its own persistence.
//
struct AutoPinRule {
    std::wstring descr;
    std::wstring ttl;
    std::wstring cls;
    bool enabled;

    AutoPinRule(const std::wstring& d = L"New Rule", 
        const std::wstring& t = L"", 
        const std::wstring& c = L"", 
        bool b = true) : descr(d), ttl(t), cls(c), enabled(b) {}

    bool match(HWND wnd) const;

    bool load(HKEY key, int i);
    bool save(HKEY key, int i) const;
    static void remove(HKEY key, int i);
};


// Simple scalar option.
// Manages its own range and UI interaction.
//
template <typename T>
struct ScalarOption {
    T value, minV, maxV, step;
    ScalarOption(T value_, T min_, T max_, T step_ = T(1)) : 
    value(value_), minV(min_), maxV(max_), step(step_)
    {
        if (maxV < minV)
            maxV = minV;
        value = clamp(value);
    }

    T clamp(T n) const
    {
        return n < minV ? minV : n > maxV ? maxV : n;
    }

    bool inRange(T t) const
    {
        return minV <= t && t <= maxV;
    }

    ScalarOption& operator=(T t)
    {
        value = clamp(t);
        return *this;
    }

    bool operator!=(const ScalarOption& other) const
    {
        return value != other.value;
    }

    // get value from ctrl (use min val on error)
    // making sure it's in range
    T getUI(HWND wnd, int id)
    {
        if (!std::numeric_limits<T>::is_integer) {
            // only integers are supported for now
            return minV;
        }

        BOOL xlated;
        T t = static_cast<T>(GetDlgItemInt(wnd, id, &xlated, 
            std::numeric_limits<T>::is_signed));
        if (!xlated || t < minV)
            t = minV;
        else if (t > maxV)
            t = maxV;

        return t;
    }

    // Check control for out-of-range values.
    // If clamp is false, show warning and move focus to control.
    // If clamp is true, set control to a proper value.
    // Return whether final value is valid.
    //
    bool validateUI(HWND wnd, int id, bool clampValue)
    {
        if (!std::numeric_limits<T>::is_integer) {
            // only integers are supported for now
            return false;
        }

        const bool isSigned = std::numeric_limits<T>::is_signed;
        BOOL xlated;
        T t = static_cast<T>(GetDlgItemInt(wnd, id, &xlated, isSigned));
        if (xlated && inRange(t))
            return true;

        if (clampValue) {
            T newValue = clamp(t);
            SetDlgItemInt(wnd, id, static_cast<UINT>(newValue), isSigned);
            return true;
        }
        else {
            // 使用本地化的错误处理
            std::wstring errorMsg = LANG_MGR.getString(L"strings.ui_range_warning");
            WCHAR buffer[Constants::MAX_CLASSNAME_LEN];
            
            if (!errorMsg.empty() && errorMsg != L"strings.ui_range_warning") {
                // 使用本地化的格式化字符串 - 注意：%1 是占位符，%2!d! 和 %3!d! 是数值
                // 但由于swprintf_s不支持这种格式，我们需要替换为标准格式
                std::wstring formattedMsg = errorMsg;
                // 简单替换为标准格式
                size_t pos = formattedMsg.find(L"%1");
                if (pos != std::wstring::npos) {
                    formattedMsg.replace(pos, 2, L"Value");
                }
                pos = formattedMsg.find(L"%2!d!");
                if (pos != std::wstring::npos) {
                    formattedMsg.replace(pos, 5, L"%d");
                }
                pos = formattedMsg.find(L"%3!d!");
                if (pos != std::wstring::npos) {
                    formattedMsg.replace(pos, 5, L"%d");
                }
                swprintf_s(buffer, formattedMsg.c_str(), minV, maxV);
            } else {
                // 回退到英文
                swprintf_s(buffer, L"Value must be between %d and %d", minV, maxV);
            }
            
            Foundation::ErrorHandler::warning(wnd, buffer);
            SetFocus(GetDlgItem(wnd, id));
            return false;
        }
    }

};

typedef ScalarOption<int>        IntOption;
typedef std::vector<AutoPinRule> AutoPinRules;


// Program options.
// Provides default values and manages persistence.
//
class Options {
public:
    // pins
    std::wstring  pinImagePath;  // 图钉图像文件路径（相对于程序目录）
    IntOption     trackRate;
    bool          dblClkTray;
    bool          runOnStartup;
    // hotkeys
    bool          hotkeysOn;
    HotKey        hotEnterPin;
    HotKey        hotTogglePin;
    // autopin
    bool          autoPinOn;
    AutoPinRules  autoPinRules;
    IntOption     autoPinDelay;
    // lang
    std::wstring  language;   // language code like "zh_CN", "en_US", empty means auto-detect

    Options();
    ~Options();

    bool save() const;
    bool load();
    
    // 立即保存设置到INI文件（用于设置更改时立即保存）
    bool saveImmediately() const;
    
    // INI file methods
    std::wstring getIniFilePath() const;
    void loadLanguageFromIni();
    
    // UTF-8兼容的INI文件操作方法
    bool loadSettingsFromIni();
    bool loadHotKeyFromIni(HotKey& hotkey, LPCWSTR keyName);
    bool loadAutoPinRulesFromIni();
    
    // 格式化INI文件写入方法（UTF-8兼容）
    bool saveFormattedSettingsToIni() const;
    
    // UTF-8兼容的INI读取辅助函数
    std::wstring readUtf8IniValue(const std::wstring& iniPath, const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue) const;

protected:
    // constants
    static const HKEY   HKCU;

    static LPCWSTR REG_PATH_EF;
    static LPCWSTR REG_APR_SUBPATH;
    static LPCWSTR REG_PATH_RUN;

    static LPCWSTR REG_POLLRATE;
    static LPCWSTR REG_TRAYDBLCLK;
    static LPCWSTR REG_AUTOPINON;
    static LPCWSTR REG_AUTOPINDELAY;
    static LPCWSTR REG_AUTOPINCOUNT;
    static LPCWSTR REG_AUTOPINRULE;
    static LPCWSTR REG_HOTKEYSON;
    static LPCWSTR REG_HOTNEWPIN;
    static LPCWSTR REG_HOTTOGGLEPIN;
    static LPCWSTR REG_LANGUAGE;

    // utilities
    bool REGOK(DWORD err) { return err == ERROR_SUCCESS; }
};


class WindowCreationMonitor;


// Used by options dialog to pass data to the various tabs.
//
struct OptionsPropSheetData {
    Options& opt;
    WindowCreationMonitor& winCreMon;
};
