#include "core/stdafx.h"
#include "options/options.h"
#include "core/application.h"
#include "system/logger.h"
#include "system/language_manager.h"
#include "foundation/string_utils.h"
#include <algorithm>
#include <fstream>


const HKEY Options::HKCU = HKEY_CURRENT_USER;

LPCWSTR Options::REG_PATH_EF      = L"Software\\godcop";
LPCWSTR Options::REG_APR_SUBPATH  = L"AutoPinRules";
LPCWSTR Options::REG_PATH_RUN     = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
LPCWSTR Options::REG_POLLRATE     = L"PollRate";
LPCWSTR Options::REG_TRAYDBLCLK   = L"TrayDblClk";
LPCWSTR Options::REG_AUTOPINON    = L"Enabled";
LPCWSTR Options::REG_AUTOPINDELAY = L"Delay";
LPCWSTR Options::REG_AUTOPINCOUNT = L"Count";
LPCWSTR Options::REG_AUTOPINRULE  = L"AutoPinRule%d";
LPCWSTR Options::REG_HOTKEYSON    = L"HotKeysOn";
LPCWSTR Options::REG_HOTNEWPIN    = L"HotKeyNewPin";
LPCWSTR Options::REG_HOTTOGGLEPIN = L"HotKeyTogglePin";
LPCWSTR Options::REG_LANGUAGE     = L"Language";


bool 
HotKey::load(HKEY key, LPCWSTR val)
{
    DWORD dw = 0;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    
    if (RegQueryValueExW(key, val, nullptr, &dwType, reinterpret_cast<LPBYTE>(&dw), &dwSize) != ERROR_SUCCESS || dwType != REG_DWORD)
        return false;

    vk  = LOBYTE(dw);
    mod = HIBYTE(dw);

    if (!vk)  // oops
        mod = 0;

    return true;
}


bool 
HotKey::save(HKEY key, LPCWSTR val) const
{
    DWORD dw = MAKEWORD(vk, mod);
    return RegSetValueExW(key, val, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dw), sizeof(DWORD)) == ERROR_SUCCESS;
}


bool 
AutoPinRule::match(HWND wnd) const
{
    if (!enabled) {
        return false;
    }
        
    // 使用WndHelper获取窗口信息
    Window::WndHelper helper(wnd);
    std::wstring windowTitle = helper.getText();
    std::wstring className = helper.getClassName();
    
    // 使用简单的通配符匹配
    bool titleMatch = Foundation::StringUtils::wildcardMatch(ttl.c_str(), windowTitle.c_str());
        bool classMatch = Foundation::StringUtils::wildcardMatch(cls.c_str(), className.c_str());
    
    return titleMatch && classMatch;
}


class NumFlagValueName {
    const int num;
public:
    NumFlagValueName(int num) : num(num) {}
    std::wstring operator()(WCHAR flag) {
        WCHAR buf[20];
        wsprintf(buf, L"%d%c", num, flag);
        return buf;
    }
};


namespace {
    // 辅助函数：读取注册表字符串值
    bool readRegString(HKEY key, LPCWSTR valueName, std::wstring& result) {
        WCHAR buffer[512] = {0};
        DWORD bufferSize = sizeof(buffer);
        DWORD type = REG_SZ;
        
        if (RegQueryValueExW(key, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufferSize) != ERROR_SUCCESS || type != REG_SZ)
            return false;
            
        result = buffer;
        return true;
    }
    
    // 辅助函数：写入注册表字符串值
    bool writeRegString(HKEY key, LPCWSTR valueName, const std::wstring& value) {
        return RegSetValueExW(key, valueName, 0, REG_SZ, 
                             reinterpret_cast<const BYTE*>(value.c_str()), 
                             (DWORD)(value.length() + 1) * sizeof(WCHAR)) == ERROR_SUCCESS;
    }
}

bool 
AutoPinRule::load(HKEY key, int i)
{
    NumFlagValueName val(i);
    
    // 读取描述、标题匹配模式和类名匹配模式
    if (!readRegString(key, val(L'D').c_str(), descr) ||
        !readRegString(key, val(L'T').c_str(), ttl) ||
        !readRegString(key, val(L'C').c_str(), cls))
        return false;
    
    // 读取启用状态
    DWORD enabled_dw = 0;
    DWORD dwSize = sizeof(DWORD);
    DWORD type = REG_DWORD;
    if (RegQueryValueExW(key, val(L'E').c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(&enabled_dw), &dwSize) != ERROR_SUCCESS || type != REG_DWORD)
        return false;
    
    enabled = enabled_dw != 0;
    return true;
}


bool 
AutoPinRule::save(HKEY key, int i) const
{
    NumFlagValueName val(i);
    
    // 保存描述、标题匹配模式和类名匹配模式
    if (!writeRegString(key, val(L'D').c_str(), descr) ||
        !writeRegString(key, val(L'T').c_str(), ttl) ||
        !writeRegString(key, val(L'C').c_str(), cls))
        return false;
    
    // 保存启用状态
    DWORD enabled_dw = enabled ? 1 : 0;
    if (RegSetValueExW(key, val(L'E').c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&enabled_dw), sizeof(DWORD)) != ERROR_SUCCESS)
        return false;
    
    return true;
}


void 
AutoPinRule::remove(HKEY key, int i)
{
    NumFlagValueName val(i);
    RegDeleteValueW(key, val(L'D').c_str());
    RegDeleteValueW(key, val(L'T').c_str());
    RegDeleteValueW(key, val(L'C').c_str());
    RegDeleteValueW(key, val(L'E').c_str());
}


Options::Options() : 
    pinImagePath(L"assets\\images\\TinyPin.png"),  // 默认使用原始图钉文件
    trackRate(Constants::DEFAULT_TRACK_RATE_OLD, Constants::MIN_TRACK_RATE, Constants::MAX_TRACK_RATE, Constants::MIN_TRACK_RATE),
    dblClkTray(false),
    runOnStartup(false),
    hotkeysOn(true),
    hotEnterPin(App::HOTID_ENTERPINMODE, VK_F11, MOD_CONTROL),
    hotTogglePin(App::HOTID_TOGGLEPIN, VK_F12, MOD_CONTROL),
    autoPinOn(false),
    autoPinDelay(Constants::DEFAULT_AUTOPIN_DELAY, Constants::MIN_AUTOPIN_DELAY, Constants::MAX_AUTOPIN_DELAY, Constants::SMALL_BUFFER_SIZE),
    language(L"")    // empty means auto-detect
{
    // 为Win2K+设置更高的跟踪频率（更高的WM_TIMER分辨率）
    // Windows 2000的主版本号为5
    if (Platform::SystemInfo::getOsMajorVersion() >= 5)
        trackRate.value = Constants::DEFAULT_TRACK_RATE_NEW;
}


Options::~Options()
{
}


bool
Options::save() const
{
    // 使用UTF-8兼容的格式化保存方法，避免中文乱码
    if (!saveFormattedSettingsToIni()) {
        LOG_ERROR(L"保存设置到INI文件失败");
        return false;
    }
    // 注意：开机启动设置已在 saveImmediately() 中立即处理，
    // 此处不再重复处理，避免重复的注册表操作
    return true;
}


bool
Options::load()
{
    // 从INI文件加载大部分设置
    if (!loadSettingsFromIni()) {
        // 使用默认设置
    }
    
    // 从注册表读取开机启动状态
    std::wstring fullRegPath = L"HKEY_CURRENT_USER\\" + std::wstring(REG_PATH_RUN);
    
    // 先创建 RegKeyHelper 对象，然后再创建 AutoRegKeyHelper
    Util::Registry::RegKeyHelper regKey = Util::Registry::RegKeyHelper::open(HKCU, REG_PATH_RUN);
    if (regKey.isValid()) {
        // 创建 AutoRegKeyHelper，它会获取句柄的所有权
        Util::Registry::AutoRegKeyHelper runKey(regKey);
        
        // 获取值类型
        DWORD valueType = runKey.getValueType(App::APPNAME);
        
        // 判断开机启动状态
        runOnStartup = (valueType == REG_SZ);
    } else {
        LOG_WARNING(L"无法打开注册表键读取开机启动状态: " + fullRegPath);
        runOnStartup = false; // 默认为禁用
    }
    
    return true;
}


// INI文件处理方法
std::wstring Options::getIniFilePath() const
{
    WCHAR exePath[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    
    // 获取exe文件所在目录
    std::wstring path = exePath;
    size_t lastSlash = path.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        path = path.substr(0, lastSlash + 1);
    }
    
    // 添加配置文件名
    path += L"TinyPin.ini";
    return path;
}

void Options::loadLanguageFromIni()
{
    std::wstring iniPath = getIniFilePath();
    
    std::wstring value = readUtf8IniValue(iniPath, L"Settings", L"Language", L"");
    
    if (!value.empty()) {
        language = value;
    } else {
        language = L"";  // 空字符串表示自动检测
    }
}

bool Options::loadSettingsFromIni()
{
    std::wstring iniPath = getIniFilePath();
    
    // 检查文件是否存在
    if (GetFileAttributesW(iniPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    
    // 加载语言设置
    loadLanguageFromIni();
    
    // 加载图钉设置
    std::wstring value;
    
    value = readUtf8IniValue(iniPath, L"Pins", L"PinImagePath", L"");
    if (!value.empty()) {
        pinImagePath = value;
    }
    
    value = readUtf8IniValue(iniPath, L"Pins", L"TrackRate", L"");
    if (!value.empty()) {
        int rate = _wtoi(value.c_str());
        if (trackRate.inRange(rate)) {
            trackRate = rate;
        }
    }
    
    value = readUtf8IniValue(iniPath, L"Pins", L"TrayDblClick", L"");
    if (!value.empty()) {
        dblClkTray = (_wtoi(value.c_str()) != 0);
    }
    
    // 加载热键设置
    value = readUtf8IniValue(iniPath, L"Hotkeys", L"Enabled", L"");
    if (!value.empty()) {
        hotkeysOn = (_wtoi(value.c_str()) != 0);
    }
    
    loadHotKeyFromIni(hotEnterPin, L"EnterPin");
    loadHotKeyFromIni(hotTogglePin, L"TogglePin");
    
    // 加载自动图钉设置
    value = readUtf8IniValue(iniPath, L"AutoPin", L"Enabled", L"");
    if (!value.empty()) {
        autoPinOn = (_wtoi(value.c_str()) != 0);
    }
    
    value = readUtf8IniValue(iniPath, L"AutoPin", L"Delay", L"");
    if (!value.empty()) {
        int delay = _wtoi(value.c_str());
        if (autoPinDelay.inRange(delay)) {
            autoPinDelay = delay;
        }
    }
    
    loadAutoPinRulesFromIni();
    
    return true;
}

bool Options::loadHotKeyFromIni(HotKey& hotkey, LPCWSTR keyName)
{
    std::wstring iniPath = getIniFilePath();
    
    // 加载虚拟键码
    std::wstring vkKeyName = std::wstring(keyName) + L"_VK";
    std::wstring value = readUtf8IniValue(iniPath, L"Hotkeys", vkKeyName, L"");
    if (!value.empty()) {
        hotkey.vk = static_cast<UINT>(_wtoi(value.c_str()));
    }
    
    // 加载修饰键
    std::wstring modKeyName = std::wstring(keyName) + L"_MOD";
    value = readUtf8IniValue(iniPath, L"Hotkeys", modKeyName, L"");
    if (!value.empty()) {
        hotkey.mod = static_cast<UINT>(_wtoi(value.c_str()));
    }
    
    // 如果没有虚拟键码，清除修饰键
    if (!hotkey.vk) {
        hotkey.mod = 0;
    }
    
    return true;
}

// UTF-8兼容的INI读取辅助函数
std::wstring Options::readUtf8IniValue(const std::wstring& iniPath, const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue) const
{
    try {
        // 使用utilities中的转换函数
        std::string utf8Path = Foundation::StringUtils::wideToUtf8(iniPath);
        
        // 读取整个文件内容
        std::ifstream file(utf8Path, std::ios::binary);
        if (!file.is_open()) {
            return defaultValue;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        // 使用utilities中的转换函数
        std::wstring wideContent = Foundation::StringUtils::utf8ToWide(content);
        
        // 查找目标section
        std::wstring sectionHeader = L"[" + section + L"]";
        size_t sectionPos = wideContent.find(sectionHeader);
        if (sectionPos == std::wstring::npos) {
            return defaultValue;
        }
        
        // 从section开始位置查找key
        size_t searchStart = sectionPos + sectionHeader.length();
        size_t nextSectionPos = wideContent.find(L"[", searchStart);
        if (nextSectionPos == std::wstring::npos) {
            nextSectionPos = wideContent.length();
        }
        
        std::wstring sectionContent = wideContent.substr(searchStart, nextSectionPos - searchStart);
        
        // 查找key=value行
        std::wstring keyPattern = key + L"=";
        size_t keyPos = sectionContent.find(keyPattern);
        if (keyPos == std::wstring::npos) {
            return defaultValue;
        }
        
        // 确保找到的是行的开始（前面是换行符或空白）
        if (keyPos > 0) {
            wchar_t prevChar = sectionContent[keyPos - 1];
            if (prevChar != L'\n' && prevChar != L'\r' && prevChar != L' ' && prevChar != L'\t') {
                // 继续查找下一个匹配
                size_t nextPos = keyPos + keyPattern.length();
                while (nextPos < sectionContent.length()) {
                    keyPos = sectionContent.find(keyPattern, nextPos);
                    if (keyPos == std::wstring::npos) {
                        return defaultValue;
                    }
                    if (keyPos == 0 || sectionContent[keyPos - 1] == L'\n' || sectionContent[keyPos - 1] == L'\r') {
                        break;
                    }
                    nextPos = keyPos + keyPattern.length();
                }
                if (keyPos == std::wstring::npos) {
                    return defaultValue;
                }
            }
        }
        
        // 提取值
        size_t valueStart = keyPos + keyPattern.length();
        size_t lineEnd = sectionContent.find(L'\n', valueStart);
        if (lineEnd == std::wstring::npos) {
            lineEnd = sectionContent.length();
        }
        
        std::wstring value = sectionContent.substr(valueStart, lineEnd - valueStart);
        
        // 移除行尾的回车符
        if (!value.empty() && value.back() == L'\r') {
            value.pop_back();
        }
        
        return value;
    }
    catch (const std::exception&) {
        return defaultValue;
    }
}

bool Options::loadAutoPinRulesFromIni()
{
    std::wstring iniPath = getIniFilePath();
    
    // 清空现有规则
    autoPinRules.clear();
    
    // 加载规则数量
    std::wstring ruleCountStr = readUtf8IniValue(iniPath, L"AutoPin", L"RuleCount", L"0");
    if (ruleCountStr.empty() || ruleCountStr == L"0") {
        return true; // 没有规则，这是正常的
    }
    
    int ruleCount = _wtoi(ruleCountStr.c_str());
    
    // 加载每个规则
    for (int i = 0; i < ruleCount; ++i) {
        std::wstring sectionName = L"AutoPinRule" + std::to_wstring(i);
        AutoPinRule rule;
        
        // 加载描述
        rule.descr = readUtf8IniValue(iniPath, sectionName, L"Description", L"");
        
        // 加载标题匹配模式
        rule.ttl = readUtf8IniValue(iniPath, sectionName, L"Title", L"");
        
        // 加载类名匹配模式
        rule.cls = readUtf8IniValue(iniPath, sectionName, L"Class", L"");
        
        // 加载启用状态
        std::wstring enabledStr = readUtf8IniValue(iniPath, sectionName, L"Enabled", L"1");
        rule.enabled = (_wtoi(enabledStr.c_str()) != 0);
        
        autoPinRules.push_back(rule);
    }
    
    return true;
}

// 立即保存设置到INI文件（用于设置更改时立即保存）
bool Options::saveImmediately() const
{
    // 使用格式化的INI文件保存方法
    if (!saveFormattedSettingsToIni()) {
        return false;
    }
    
    // 立即处理开机启动注册表操作
    Util::Registry::RegKeyHelper regKey = Util::Registry::RegKeyHelper::create(HKCU, REG_PATH_RUN);
    if (regKey.isValid()) {
        // 创建 AutoRegKeyHelper，它会获取句柄的所有权
        Util::Registry::AutoRegKeyHelper runKey(regKey);
        
        if (runOnStartup) {
            WCHAR fileName[MAX_PATH] = {0};
            GetModuleFileNameW(nullptr, fileName, MAX_PATH);
            runKey.setString(App::APPNAME, fileName);
        }
        else {
            runKey.deleteValue(App::APPNAME);
        }
    }
    
    return true;
}

// 格式化INI文件写入方法
bool Options::saveFormattedSettingsToIni() const
{
    std::wstring iniPath = getIniFilePath();
    
    try {
        // 使用utilities中的转换函数
        std::string utf8Path = Foundation::StringUtils::wideToUtf8(iniPath);
        
        // 使用普通文件流写入UTF-8编码的INI内容
        std::ofstream file(utf8Path, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // 辅助函数：将宽字符串转换为UTF-8字符串
        auto toUtf8 = [](const std::wstring& wstr) -> std::string {
            return Foundation::StringUtils::wideToUtf8(wstr);
        };
        
        // 写入文件头注释
        file << "; 微钉 配置文件\n";
        file << "; 此文件包含所有 微钉 设置\n";
        file << "; 自动生成 - 支持手动编辑\n";
        file << "\n";
        
        // [Settings] 部分 - 语言设置
        file << "[Settings]\n";
        file << "; 语言设置 (例如：zh_CN, en_US, 空值表示自动检测)\n";
        file << "Language=" << toUtf8(language) << "\n";
        file << "\n";
        
        // [Pins] 部分 - 图钉设置
        file << "[Pins]\n";
        file << "; 图钉图像文件路径 (相对于程序目录)\n";
        file << "PinImagePath=" << toUtf8(pinImagePath) << "\n";
        file << "; 窗口跟踪频率，单位毫秒 (10-1000)\n";
        file << "TrackRate=" << trackRate.value << "\n";
        file << "; 托盘图标双击行为 (0=单击, 1=双击)\n";
        file << "TrayDblClick=" << (dblClkTray ? 1 : 0) << "\n";
        file << "\n";
        
        // [Hotkeys] 部分 - 热键设置
        file << "[Hotkeys]\n";
        file << "; 启用热键 (0=禁用, 1=启用)\n";
        file << "Enabled=" << (hotkeysOn ? 1 : 0) << "\n";
        file << "; 进入图钉模式热键 (VK=虚拟键码, MOD=修饰键)\n";
        file << "; 修饰键: 1=Alt, 2=Ctrl, 4=Shift, 8=Win\n";
        file << "EnterPin_VK=" << hotEnterPin.vk << "\n";
        file << "EnterPin_MOD=" << hotEnterPin.mod << "\n";
        file << "; 切换图钉热键\n";
        file << "TogglePin_VK=" << hotTogglePin.vk << "\n";
        file << "TogglePin_MOD=" << hotTogglePin.mod << "\n";
        file << "\n";
        
        // [AutoPin] 部分 - 自动图钉设置
        file << "[AutoPin]\n";
        file << "; 启用自动图钉 (0=禁用, 1=启用)\n";
        file << "Enabled=" << (autoPinOn ? 1 : 0) << "\n";
        file << "; 自动图钉延迟时间，单位毫秒 (100-10000)\n";
        file << "Delay=" << autoPinDelay.value << "\n";
        file << "; 自动图钉规则数量\n";
        file << "RuleCount=" << autoPinRules.size() << "\n";
        file << "\n";
        
        // 自动图钉规则部分
        for (size_t i = 0; i < autoPinRules.size(); ++i) {
            const AutoPinRule& rule = autoPinRules[i];
            file << "[AutoPinRule" << i << "]\n";
            file << "; 规则描述\n";
            file << "Description=" << toUtf8(rule.descr) << "\n";
            file << "; 窗口标题匹配模式 (* 表示通配符)\n";
            file << "Title=" << toUtf8(rule.ttl) << "\n";
            file << "; 窗口类名匹配模式\n";
            file << "Class=" << toUtf8(rule.cls) << "\n";
            file << "; 规则启用状态 (0=禁用, 1=启用)\n";
            file << "Enabled=" << (rule.enabled ? 1 : 0) << "\n";
            
            // 在规则之间添加空行（除了最后一个）
            if (i < autoPinRules.size() - 1) {
                file << "\n";
            }
        }
        
        file.close();
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}
