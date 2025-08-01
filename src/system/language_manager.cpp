#include "core/stdafx.h"
#include "system/language_manager.h"
#include "foundation/string_utils.h"
#include "foundation/file_utils.h"
#include "core/application.h"
#include "system/logger.h"
#include "resource.h"
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <algorithm>

// 移除硬编码的语言列表，改为动态扫描

LanguageManager::LanguageManager() : m_initialized(false) {
    initializeSupportedLanguages();
    scanLanguageFiles();
}

LanguageManager& LanguageManager::getInstance() {
    static LanguageManager instance;
    return instance;
}

bool LanguageManager::initialize() {
    if (m_initialized) {
        return true;
    }
    
    // 初始化控件映射
    initializeControlMappings();
    
    // 初始化字符串资源映射
    initializeStringResourceMappings();
    
    // 动态扫描可用语言文件
    scanLanguageFiles();
    
    if (m_availableLanguages.empty()) {
        // 如果没有找到任何语言文件，使用英文作为默认
        m_currentLanguage = L"en_US";
        m_initialized = true;
        return false;
    }
    
    m_initialized = true;
    return true;
}

std::wstring LanguageManager::getLanguageNameFromFile(const std::wstring& languageCode) const {
    std::wstring filePath = getLanguageFilePath(languageCode);
    
    // 检查文件是否存在
    if (GetFileAttributesW(filePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        LOG_WARNING(L"语言文件不存在: " + filePath);
        return L"";
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_WARNING(L"无法打开语言文件: " + filePath);
        return L"";
    }
    
    // 简单解析JSON文件，只查找language_info.name
    std::string line;
    bool inLanguageInfo = false;
    
    try {
        while (std::getline(file, line)) {
            trimString(line);
            
            if (line.empty()) {
                continue;
            }
            
            // 检查是否进入language_info对象
            if (line.find("\"language_info\"") != std::string::npos && line.back() == '{') {
                inLanguageInfo = true;
                continue;
            }
            
            // 如果在language_info对象中，查找name字段
            if (inLanguageInfo) {
                // 检查是否退出language_info对象
                if (line == "}" || line == "},") {
                    break; // 已经处理完language_info，可以退出
                }
                
                // 查找name字段
                if (line.find("\"name\"") != std::string::npos) {
                    size_t colonPos = line.find(':');
                    if (colonPos != std::string::npos) {
                        std::string value = extractJsonValue(line, colonPos);
                        
                        if (!value.empty()) {
                            return Foundation::StringUtils::utf8ToWide(value);
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR(L"解析语言文件时发生异常: " + filePath + L" - " + Foundation::StringUtils::utf8ToWide(e.what()));
        return L"";
    }
    
    LOG_WARNING(L"在语言文件中未找到language_info.name: " + filePath);
    return L"";
}

void LanguageManager::autoSelectLanguage() {
    std::wstring systemLang = getSystemLanguage();
    
    // 首先尝试完全匹配
    if (isLanguageAvailable(systemLang)) {
        setLanguage(systemLang);
        return;
    }
    
    // 如果是中文相关语言，使用中文
    if (systemLang.find(L"zh") == 0 && isLanguageAvailable(L"zh_CN")) {
        setLanguage(L"zh_CN");
        return;
    }
    
    // 默认使用英文
    if (isLanguageAvailable(L"en_US")) {
        setLanguage(L"en_US");
    } else if (!m_availableLanguages.empty()) {
        setLanguage(m_availableLanguages[0]);
    }
}

bool LanguageManager::setLanguage(const std::wstring& languageCode) {
    if (!isLanguageAvailable(languageCode)) {
        return false;
    }

    if (m_currentLanguage == languageCode) {
        return true;
    }

    if (loadLanguageFile(languageCode)) {
        m_currentLanguage = languageCode;
        
        // 设置Windows线程语言，让RC资源自动选择正确的语言版本
        setWindowsThreadLanguage(languageCode);
        
        return true;
    }

    return false;
}

std::wstring LanguageManager::getString(const std::wstring& key) const {
    // 首先尝试直接查找键
    auto it = m_strings.find(key);
    if (it != m_strings.end()) {
        return it->second;
    }
    
    // 如果没找到，尝试在strings前缀下查找
    std::wstring stringsKey = L"strings." + key;
    it = m_strings.find(stringsKey);
    if (it != m_strings.end()) {
        return it->second;
    }
    
    // 如果在语言文件中找不到，尝试从RC资源文件中加载
    // 首先检查原始键是否有对应的RC资源
    int resourceId = findStringResourceId(key);
    if (resourceId == 0) {
        // 如果原始键没有，检查带strings前缀的键
        resourceId = findStringResourceId(stringsKey);
    }
    
    if (resourceId > 0) {
        WCHAR buffer[Constants::MAX_CLASSNAME_LEN];
        if (LoadString(app.inst, resourceId, buffer, Constants::MAX_CLASSNAME_LEN) > 0) {
            return std::wstring(buffer);
        }
    }
    
    // 如果找不到，返回键名
    return key;
}



std::wstring LanguageManager::getControlText(int controlId, const std::wstring& dialogName) const {
    const ControlMapping* mapping = findControlMapping(controlId);
    if (mapping) {
        // 如果指定了对话框名称，优先使用指定的
        std::wstring targetDialog = dialogName.empty() ? mapping->dialogName : dialogName;
        
        // 根据对话框名称构建不同的键前缀
        std::wstring key;
        if (targetDialog == L"tray") {
            // 托盘菜单项
            key = L"tray." + mapping->elementName;
        } else if (targetDialog == L"strings") {
            // 字符串资源
            key = L"strings." + mapping->elementName;
        } else {
            // 对话框控件
            key = L"dialogs." + targetDialog + L"." + mapping->elementName;
        }
        
        auto it = m_strings.find(key);
        if (it != m_strings.end()) {
            return it->second;
        }
        
        // 如果没有找到翻译文本，提供回退机制
        // 对于字符串资源（如IDS_TRAYTIP），直接从RC资源文件中加载
        if (targetDialog == L"strings" && controlId > 0) {
            WCHAR buffer[Constants::MAX_CLASSNAME_LEN];
            if (LoadString(app.inst, controlId, buffer, Constants::MAX_CLASSNAME_LEN) > 0) {
                return std::wstring(buffer);
            }
        }
        
        // 对于其他控件，使用英文回退
        std::wstring fallback = getEnglishFallback(controlId, mapping->elementName);
        if (!fallback.empty()) {
            return fallback;
        }
    }
    
    // 如果没有找到映射或文本，返回空字符串
    return L"";
}



std::vector<std::pair<std::wstring, std::wstring>> LanguageManager::getAvailableLanguages() const {
    std::vector<std::pair<std::wstring, std::wstring>> result;
    
    for (const auto& lang : m_availableLanguages) {
        std::wstring name;
        
        // 首先检查缓存
        auto cacheIt = m_languageNameCache.find(lang);
        if (cacheIt != m_languageNameCache.end()) {
            name = cacheIt->second;
        } else {
            // 从对应的语言文件中读取本地化的语言名称
            name = getLanguageNameFromFile(lang);
            
            // 如果无法从文件中读取，使用配置中的回退名称
            if (name.empty()) {
                const LanguageConfig* config = findLanguageConfig(lang);
                if (config) {
                    name = config->displayName;
                } else {
                    name = lang; // 使用语言代码作为最终回退
                }
            }
            
            // 缓存结果
            m_languageNameCache[lang] = name;
        }
        
        result.emplace_back(lang, name);
    }
    
    return result;
}

void LanguageManager::clearLanguageNameCache() {
    m_languageNameCache.clear();
}

bool LanguageManager::isLanguageAvailable(const std::wstring& languageCode) const {
    return std::find(m_availableLanguages.begin(), m_availableLanguages.end(), languageCode) != m_availableLanguages.end();
}

std::wstring LanguageManager::getLanguageFileDescription(const std::wstring& path, const std::wstring& file) const {
    HINSTANCE inst = file.empty() ? app.inst : LoadLibrary((path + file).c_str());
    WCHAR buf[Constants::SMALL_BUFFER_SIZE];
    if (!inst || !LoadString(inst, IDS_LANG, buf, sizeof(buf)))
        *buf = L'\0';
    if (!file.empty() && inst)   // 如果我们必须加载它，则释放inst
        FreeLibrary(inst);
    return buf;
}

bool LanguageManager::loadLanguageFile(const std::wstring& languageCode) {
    std::wstring filePath = getLanguageFilePath(languageCode);
    return parseJsonFile(filePath);
}

bool LanguageManager::parseJsonFile(const std::wstring& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_ERROR(L"无法打开语言文件: " + filePath);
        return false;
    }
    
    // 简单的JSON解析器（仅支持我们需要的格式）
    m_strings.clear();
    std::string line;
    std::vector<std::wstring> keyStack;
    
    while (std::getline(file, line)) {
        trimString(line);
        
        if (line.empty() || line == "{") {
            continue;
        }
        
        // 处理对象结束
        if (line == "}" || line == "},") {
            if (!keyStack.empty()) {
                keyStack.pop_back();
            }
            continue;
        }
        
        // 处理对象开始
        if (line.back() == '{') {
            std::string key = line.substr(1, line.find('\"', 1) - 1);
            std::wstring wkey = Foundation::StringUtils::utf8ToWide(key);
            keyStack.push_back(wkey);
            continue;
        }
        
        // 处理键值对
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(1, line.find('\"', 1) - 1);
            std::string value = extractJsonValue(line, colonPos);
            
            // 构建完整键名
            std::wstring fullKey;
            if (!keyStack.empty()) {
                for (size_t i = 0; i < keyStack.size(); ++i) {
                    if (i > 0) fullKey += L".";
                    fullKey += keyStack[i];
                }
                fullKey += L"." + Foundation::StringUtils::utf8ToWide(key);
            } else {
                fullKey = Foundation::StringUtils::utf8ToWide(key);
            }
            
            // 转换值并处理转义字符
            std::wstring wvalue = Foundation::StringUtils::utf8ToWide(value);
            processEscapeSequences(wvalue);
            
            m_strings[fullKey] = wvalue;
        }
    }
    return true;
}

std::wstring LanguageManager::getSystemLanguage() const {
    LANGID langId = GetUserDefaultUILanguage();
    WORD primaryLang = PRIMARYLANGID(langId);
    WORD subLang = SUBLANGID(langId);
    
    // 遍历支持的语言配置，查找匹配的系统语言
    for (const auto& config : m_supportedLanguages) {
        if (config.primaryLang == primaryLang) {
            // 对于某些语言，还需要检查子语言
            if (config.primaryLang == LANG_CHINESE) {
                if (config.subLang == subLang) {
                    return config.code;
                }
            } else {
                // 对于其他语言，主语言匹配即可
                return config.code;
            }
        }
    }
    
    return L"en_US"; // 默认英文
}

std::wstring LanguageManager::getLanguageFilePath(const std::wstring& languageCode) const {
    WCHAR modulePath[MAX_PATH] = {0};
    GetModuleFileNameW(app.inst, modulePath, MAX_PATH);
    std::wstring exePath = modulePath;
    
    // 获取目录部分
    size_t lastSlash = exePath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        exePath = exePath.substr(0, lastSlash + 1);
    }
    
    return exePath + L"assets\\locales\\" + languageCode + L".json";
}

void LanguageManager::setWindowsThreadLanguage(const std::wstring& languageCode) {
    LANGID langId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US); // 默认英文
    
    // 使用统一的语言配置查找对应的Windows语言ID
    const LanguageConfig* config = findLanguageConfig(languageCode);
    if (config) {
        langId = MAKELANGID(config->primaryLang, config->subLang);
    } else {
        LOG_WARNING(L"未找到语言配置: " + languageCode + L"，使用默认英文");
    }
    
    // 设置线程UI语言，影响RC资源的选择
    SetThreadUILanguage(langId);
}

void LanguageManager::initializeControlMappings() {
    // 清空现有映射
    m_controlMappings.clear();
    
    // 对话框标题映射（使用ID=0作为特殊标识）
    m_controlMappings[0] = {0, L"", L"title"};  // 通用标题映射
    
    // 属性表选项卡标题映射
    m_controlMappings[IDC_TAB_PINS_TITLE] = {IDC_TAB_PINS_TITLE, L"pins", L"title"};
    m_controlMappings[IDC_TAB_AUTOPIN_TITLE] = {IDC_TAB_AUTOPIN_TITLE, L"autopin", L"title"};
    m_controlMappings[IDC_TAB_HOTKEYS_TITLE] = {IDC_TAB_HOTKEYS_TITLE, L"hotkeys", L"title"};
    m_controlMappings[IDC_TAB_LANGUAGE_TITLE] = {IDC_TAB_LANGUAGE_TITLE, L"language", L"title"};
    
    // 通用按钮映射
    m_controlMappings[IDOK] = {IDOK, L"options", L"ok"};
    m_controlMappings[IDCANCEL] = {IDCANCEL, L"options", L"cancel"};
    m_controlMappings[Constants::ID_APPLY_BUTTON] = {Constants::ID_APPLY_BUTTON, L"options", L"apply"};// Apply按钮
    
    // About对话框映射
    m_controlMappings[IDC_ABOUT_VERSION] = {IDC_ABOUT_VERSION, L"about", L"version"};
    m_controlMappings[IDC_ABOUT_PINS_LABEL] = {IDC_ABOUT_PINS_LABEL, L"about", L"pins_used"};
    m_controlMappings[IDC_ABOUT_EMAIL_LABEL] = {IDC_ABOUT_EMAIL_LABEL, L"about", L"email"};
    m_controlMappings[IDC_ABOUT_WEBSITE_LABEL] = {IDC_ABOUT_WEBSITE_LABEL, L"about", L"website"};
    m_controlMappings[IDC_MAIL] = {IDC_MAIL, L"about", L"email"};
    m_controlMappings[IDC_WEB] = {IDC_WEB, L"about", L"website"};
    
    // Edit Rule对话框映射
    m_controlMappings[IDC_RULE_GROUP] = {IDC_RULE_GROUP, L"edit_rule", L"rule_group"};
    m_controlMappings[IDC_RULE_DESC_LABEL] = {IDC_RULE_DESC_LABEL, L"edit_rule", L"description_label"};
    m_controlMappings[IDC_RULE_TITLE_LABEL] = {IDC_RULE_TITLE_LABEL, L"edit_rule", L"title_label"};
    m_controlMappings[IDC_RULE_CLASS_LABEL] = {IDC_RULE_CLASS_LABEL, L"edit_rule", L"class_label"};
    m_controlMappings[IDC_DESCR] = {IDC_DESCR, L"edit_rule", L"description"};
    m_controlMappings[IDC_TITLE] = {IDC_TITLE, L"edit_rule", L"window_title"};
    m_controlMappings[IDC_CLASS] = {IDC_CLASS, L"edit_rule", L"window_class"};
    
    // Pins选项页映射
    m_controlMappings[IDC_PINS_ICON_GROUP] = {IDC_PINS_ICON_GROUP, L"pins", L"icon_group"};
    m_controlMappings[IDC_PINS_COLOR_LABEL] = {IDC_PINS_COLOR_LABEL, L"pins", L"icon_file"};
    m_controlMappings[IDC_PINS_TRACKING_LABEL] = {IDC_PINS_TRACKING_LABEL, L"pins", L"tracking_label"};
    m_controlMappings[IDC_PINS_MS_LABEL1] = {IDC_PINS_MS_LABEL1, L"pins", L"ms_label"};
    m_controlMappings[IDC_PINS_ACTIVATION_GROUP] = {IDC_PINS_ACTIVATION_GROUP, L"pins", L"activation_group"};
    m_controlMappings[IDC_PINS_SYSTEM_GROUP] = {IDC_PINS_SYSTEM_GROUP, L"pins", L"system_group"};
    m_controlMappings[IDC_PIN_ICON_CHANGE] = {IDC_PIN_ICON_CHANGE, L"pins", L"change"};
    m_controlMappings[IDC_PIN_ICON_RESET] = {IDC_PIN_ICON_RESET, L"pins", L"reset_default"};
    m_controlMappings[IDC_POLL_RATE] = {IDC_POLL_RATE, L"pins", L"tracking_rate"};
    m_controlMappings[IDC_TRAY_SINGLE_CLICK] = {IDC_TRAY_SINGLE_CLICK, L"pins", L"single_click"};
    m_controlMappings[IDC_TRAY_DOUBLE_CLICK] = {IDC_TRAY_DOUBLE_CLICK, L"pins", L"double_click"};
    m_controlMappings[IDC_RUN_ON_STARTUP] = {IDC_RUN_ON_STARTUP, L"pins", L"run_on_startup"};
    
    // AutoPin选项页映射
    m_controlMappings[IDC_AUTOPIN_DELAY_LABEL] = {IDC_AUTOPIN_DELAY_LABEL, L"autopin", L"delay_label"};
    m_controlMappings[IDC_AUTOPIN_MS_LABEL] = {IDC_AUTOPIN_MS_LABEL, L"autopin", L"ms_label"};
    m_controlMappings[IDC_AUTOPIN_ON] = {IDC_AUTOPIN_ON, L"autopin", L"enable"};
    m_controlMappings[IDC_ADD] = {IDC_ADD, L"autopin", L"add"};
    m_controlMappings[IDC_REMOVE] = {IDC_REMOVE, L"autopin", L"remove"};
    m_controlMappings[IDC_EDIT] = {IDC_EDIT, L"autopin", L"edit"};
    m_controlMappings[IDC_UP] = {IDC_UP, L"autopin", L"move_up"};
    m_controlMappings[IDC_DOWN] = {IDC_DOWN, L"autopin", L"move_down"};
    m_controlMappings[IDC_RULE_DELAY] = {IDC_RULE_DELAY, L"autopin", L"delay"};
    
    // Hotkeys选项页映射
    m_controlMappings[IDC_HOTKEYS_PINMODE_LABEL] = {IDC_HOTKEYS_PINMODE_LABEL, L"hotkeys", L"pinmode_label"};
    m_controlMappings[IDC_HOTKEYS_TOGGLE_LABEL] = {IDC_HOTKEYS_TOGGLE_LABEL, L"hotkeys", L"toggle_label"};
    m_controlMappings[IDC_HOTKEYS_ON] = {IDC_HOTKEYS_ON, L"hotkeys", L"enable"};
    m_controlMappings[IDC_HOT_PINMODE] = {IDC_HOT_PINMODE, L"hotkeys", L"enter_pin_mode"};
    m_controlMappings[IDC_HOT_TOGGLEPIN] = {IDC_HOT_TOGGLEPIN, L"hotkeys", L"toggle_pin"};
    
    // Language选项页映射
    m_controlMappings[IDC_LANG_INTERFACE_GROUP] = {IDC_LANG_INTERFACE_GROUP, L"language", L"interface_group"};
    m_controlMappings[IDC_UILANG] = {IDC_UILANG, L"language", L"ui_language"};
    
    // 托盘菜单映射
    m_controlMappings[CM_NEWPIN] = {CM_NEWPIN, L"tray", L"pin_mode"};
    m_controlMappings[CM_REMOVEPINS] = {CM_REMOVEPINS, L"tray", L"remove_all_pins"};
    m_controlMappings[CM_OPTIONS] = {CM_OPTIONS, L"tray", L"options"};
    m_controlMappings[CM_ABOUT] = {CM_ABOUT, L"tray", L"about"};
    m_controlMappings[CM_CLOSE] = {CM_CLOSE, L"tray", L"exit"};
    
    // 字符串资源映射
    m_controlMappings[IDS_ERRBOXTTITLE] = {IDS_ERRBOXTTITLE, L"strings", L"error_box_title"};
    m_controlMappings[IDS_WRNBOXTTITLE] = {IDS_WRNBOXTTITLE, L"strings", L"warning_box_title"};
    m_controlMappings[IDS_OPTIONSTITLE] = {IDS_OPTIONSTITLE, L"strings", L"options_title"};
    m_controlMappings[IDS_WRN_UIRANGE] = {IDS_WRN_UIRANGE, L"strings", L"ui_range_warning"};
    m_controlMappings[IDS_NEWRULEDESCR] = {IDS_NEWRULEDESCR, L"strings", L"new_rule_description"};
    m_controlMappings[IDS_LANG] = {IDS_LANG, L"strings", L"language"};
    m_controlMappings[IDS_ERR_HOTKEYSSET] = {IDS_ERR_HOTKEYSSET, L"strings", L"hotkeys_set_error"};
    m_controlMappings[IDS_ERR_DLGCREATE] = {IDS_ERR_DLGCREATE, L"strings", L"dialog_create_error"};
    m_controlMappings[IDS_ERR_MUTEXFAILCONFIRM] = {IDS_ERR_MUTEXFAILCONFIRM, L"strings", L"mutex_fail_confirm"};
    m_controlMappings[IDS_ERR_WNDCLSREG] = {IDS_ERR_WNDCLSREG, L"strings", L"window_class_register_error"};
    m_controlMappings[IDS_ERR_SETPINPARENTFAIL] = {IDS_ERR_SETPINPARENTFAIL, L"strings", L"set_pin_parent_fail"};
    m_controlMappings[IDS_ERR_SETTOPMOSTFAIL] = {IDS_ERR_SETTOPMOSTFAIL, L"strings", L"set_topmost_fail"};
    m_controlMappings[IDS_ERR_COULDNOTFINDWND] = {IDS_ERR_COULDNOTFINDWND, L"strings", L"could_not_find_window"};
    m_controlMappings[IDS_ERR_ALREADYTOPMOST] = {IDS_ERR_ALREADYTOPMOST, L"strings", L"already_topmost"};
    m_controlMappings[IDS_ERR_CANNOTPINDESKTOP] = {IDS_ERR_CANNOTPINDESKTOP, L"strings", L"cannot_pin_desktop"};
    m_controlMappings[IDS_ERR_CANNOTPINTASKBAR] = {IDS_ERR_CANNOTPINTASKBAR, L"strings", L"cannot_pin_taskbar"};
    m_controlMappings[IDS_ERR_PINWND] = {IDS_ERR_PINWND, L"strings", L"pin_window_error"};
    m_controlMappings[IDS_ERR_PINCREATE] = {IDS_ERR_PINCREATE, L"strings", L"pin_create_error"};
    m_controlMappings[IDS_ERR_ALREADYRUNNING] = {IDS_ERR_ALREADYRUNNING, L"strings", L"already_running"};
    m_controlMappings[IDS_ERR_CCINIT] = {IDS_ERR_CCINIT, L"strings", L"common_controls_init_error"};
    m_controlMappings[IDS_ERR_HOOKDLL] = {IDS_ERR_HOOKDLL, L"strings", L"hook_dll_error"};
    m_controlMappings[IDS_ERR_TRAYSETWND] = {IDS_ERR_TRAYSETWND, L"strings", L"tray_set_window_error"};
    m_controlMappings[IDS_ERR_TRAYCREATE] = {IDS_ERR_TRAYCREATE, L"strings", L"tray_create_error"};
    m_controlMappings[IDS_TRAYTIP] = {IDS_TRAYTIP, L"strings", L"tray_tip"};
}

void LanguageManager::initializeStringResourceMappings() {
    // 字符串资源映射 - 将语言文件中的键映射到RC资源ID
    m_stringResourceMappings[L"strings.error_title"] = IDS_ERRBOXTTITLE;
    m_stringResourceMappings[L"strings.warning_title"] = IDS_WRNBOXTTITLE;
    m_stringResourceMappings[L"strings.options_title"] = IDS_OPTIONSTITLE;
    m_stringResourceMappings[L"strings.ui_range_warning"] = IDS_WRN_UIRANGE;
    m_stringResourceMappings[L"strings.new_rule_description"] = IDS_NEWRULEDESCR;
    m_stringResourceMappings[L"strings.language"] = IDS_LANG;
    m_stringResourceMappings[L"strings.hotkeys_set_error"] = IDS_ERR_HOTKEYSSET;
    m_stringResourceMappings[L"strings.dialog_create_error"] = IDS_ERR_DLGCREATE;
    m_stringResourceMappings[L"strings.mutex_fail_confirm"] = IDS_ERR_MUTEXFAILCONFIRM;
    m_stringResourceMappings[L"strings.window_class_register_error"] = IDS_ERR_WNDCLSREG;
    m_stringResourceMappings[L"strings.set_pin_parent_fail"] = IDS_ERR_SETPINPARENTFAIL;
    m_stringResourceMappings[L"strings.set_topmost_fail"] = IDS_ERR_SETTOPMOSTFAIL;
    m_stringResourceMappings[L"strings.could_not_find_window"] = IDS_ERR_COULDNOTFINDWND;
    m_stringResourceMappings[L"strings.already_topmost"] = IDS_ERR_ALREADYTOPMOST;
    m_stringResourceMappings[L"strings.cannot_pin_desktop"] = IDS_ERR_CANNOTPINDESKTOP;
    m_stringResourceMappings[L"strings.cannot_pin_taskbar"] = IDS_ERR_CANNOTPINTASKBAR;
    m_stringResourceMappings[L"strings.pin_window_error"] = IDS_ERR_PINWND;
    m_stringResourceMappings[L"strings.pin_create_error"] = IDS_ERR_PINCREATE;
    m_stringResourceMappings[L"strings.already_running"] = IDS_ERR_ALREADYRUNNING;
    m_stringResourceMappings[L"strings.common_controls_init_error"] = IDS_ERR_CCINIT;
    m_stringResourceMappings[L"strings.hook_dll_error"] = IDS_ERR_HOOKDLL;
    m_stringResourceMappings[L"strings.tray_set_window_error"] = IDS_ERR_TRAYSETWND;
    m_stringResourceMappings[L"strings.tray_create_error"] = IDS_ERR_TRAYCREATE;
    m_stringResourceMappings[L"strings.tray_tip"] = IDS_TRAYTIP;
}

const ControlMapping* LanguageManager::findControlMapping(int controlId) const {
    auto it = m_controlMappings.find(controlId);
    if (it != m_controlMappings.end()) {
        return &it->second;
    }
    return nullptr;
}

int LanguageManager::findStringResourceId(const std::wstring& key) const {
    auto it = m_stringResourceMappings.find(key);
    if (it != m_stringResourceMappings.end()) {
        return it->second;
    }
    return 0; // 返回0表示未找到
}

void LanguageManager::scanLanguageFiles() {
    m_availableLanguages.clear();
    m_languageNameCache.clear(); // 清空缓存，确保重新扫描时获取最新数据
    
    // 获取语言文件目录路径
    WCHAR modulePath[MAX_PATH] = {0};
    GetModuleFileNameW(app.inst, modulePath, MAX_PATH);
    std::wstring exePath = modulePath;
    
    // 获取目录部分
    size_t lastSlash = exePath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        exePath = exePath.substr(0, lastSlash + 1);
    }
    
    std::wstring langDir = exePath + L"assets\\locales\\";
    std::wstring searchPattern = langDir + L"*.json";
    
    // 使用FindFirstFile/FindNextFile扫描目录
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // 跳过目录
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                continue;
            }
            
            std::wstring fileName = findData.cFileName;
            
            // 检查文件扩展名是否为.json
            if (fileName.length() > 5 && 
                fileName.substr(fileName.length() - 5) == L".json") {
                
                // 提取语言代码（去掉.json扩展名）
                std::wstring langCode = fileName.substr(0, fileName.length() - 5);
                
                // 验证语言代码格式（例如：en_US, zh_CN, ja_JP等）
                if (isValidLanguageCode(langCode)) {
                    // 验证文件是否可以正常读取
                    std::wstring fullPath = langDir + fileName;
                    if (GetFileAttributesW(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        m_availableLanguages.push_back(langCode);
                    }
                }
            }
        } while (FindNextFileW(hFind, &findData));
        
        FindClose(hFind);
    }
    
    // 按语言代码排序，确保一致的显示顺序
    std::sort(m_availableLanguages.begin(), m_availableLanguages.end());
}

void LanguageManager::initializeSupportedLanguages() {
    // 初始化支持的语言配置
    // 这里集中管理所有语言的配置信息，避免硬编码分散在各处
    m_supportedLanguages = {
        LanguageConfig(L"zh_CN", L"简体中文", LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
        LanguageConfig(L"en_US", L"English", LANG_ENGLISH, SUBLANG_ENGLISH_US),
        LanguageConfig(L"ja_JP", L"日本語", LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN),
        LanguageConfig(L"fr_FR", L"Français", LANG_FRENCH, SUBLANG_FRENCH),
        LanguageConfig(L"de_DE", L"Deutsch", LANG_GERMAN, SUBLANG_GERMAN)
    };
}

const LanguageConfig* LanguageManager::findLanguageConfig(const std::wstring& languageCode) const {
    for (const auto& config : m_supportedLanguages) {
        if (config.code == languageCode) {
            return &config;
        }
    }
    return nullptr;
}

void LanguageManager::trimString(std::string& str) const {
    // 移除前后空白字符
    str.erase(0, str.find_first_not_of(" \t\r\n"));
    str.erase(str.find_last_not_of(" \t\r\n") + 1);
}

std::string LanguageManager::extractJsonValue(const std::string& line, size_t colonPos) const {
    std::string value = line.substr(colonPos + 1);
    
    // 移除值的引号、逗号和空白字符
    value.erase(0, value.find_first_not_of(" \t\""));
    if (!value.empty() && value.back() == ',') {
        value.pop_back();
    }
    if (!value.empty() && value.back() == '\"') {
        value.pop_back();
    }
    
    return value;
}

void LanguageManager::processEscapeSequences(std::wstring& str) const {
    // 处理 \\r\\n 转义
    size_t pos = 0;
    while ((pos = str.find(L"\\r\\n", pos)) != std::wstring::npos) {
        str.replace(pos, 4, L"\r\n");
        pos += 2;
    }
    // 处理单独的 \\n 转义
    pos = 0;
    while ((pos = str.find(L"\\n", pos)) != std::wstring::npos) {
        str.replace(pos, 2, L"\n");
        pos += 1;
    }
}

std::wstring LanguageManager::getEnglishFallback(int controlId, const std::wstring& elementName) const {
    // 基于控件ID的回退
    if (controlId == IDOK) {
        return L"OK";
    } else if (controlId == IDCANCEL) {
        return L"Cancel";
    } else if (controlId == Constants::ID_APPLY_BUTTON) {
        return L"Apply";
    }
    
    // 基于元素名称的回退
    if (elementName == L"ok") {
        return L"OK";
    } else if (elementName == L"cancel") {
        return L"Cancel";
    } else if (elementName == L"apply") {
        return L"Apply";
    } else if (elementName == L"yes") {
        return L"Yes";
    } else if (elementName == L"no") {
        return L"No";
    } else if (elementName == L"icon_file") {
        return L"Icon File";
    } else if (elementName == L"change") {
        return L"&Change";
    } else if (elementName == L"reset_default") {
        return L"Reset Default";
    }
    
    return L""; // 没有找到合适的回退
}

bool LanguageManager::isValidLanguageCode(const std::wstring& langCode) const {
    // 验证语言代码格式：xx_XX（如en_US, zh_CN, ja_JP等）
    if (langCode.length() != 5) {
        return false;
    }
    
    if (langCode[2] != L'_') {
        return false;
    }
    
    // 检查前两个字符是否为小写字母
    for (int i = 0; i < 2; i++) {
        if (langCode[i] < L'a' || langCode[i] > L'z') {
            return false;
        }
    }
    
    // 检查后两个字符是否为大写字母
    for (int i = 3; i < 5; i++) {
        if (langCode[i] < L'A' || langCode[i] > L'Z') {
            return false;
        }
    }
    
    return true;
}