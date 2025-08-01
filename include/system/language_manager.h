#pragma once

#include <string>
#include <unordered_map>
#include <memory>

// 控件映射结构
struct ControlMapping {
    int controlId;
    std::wstring dialogName;
    std::wstring elementName;
};

// 语言管理器类
// 负责加载和管理多语言资源
// 语言配置结构体
struct LanguageConfig {
    std::wstring code;          // 语言代码 (如 "zh_CN")
    std::wstring displayName;   // 显示名称 (如 "简体中文")
    WORD primaryLang;           // Windows主语言ID
    WORD subLang;               // Windows子语言ID
    
    LanguageConfig(const std::wstring& c, const std::wstring& d, WORD p, WORD s)
        : code(c), displayName(d), primaryLang(p), subLang(s) {}
};

class LanguageManager {
public:
    // 获取单例实例
    static LanguageManager& getInstance();
    
    // 初始化语言管理器
    bool initialize();
    
    // 根据系统语言自动选择语言
    void autoSelectLanguage();
    
    // 手动设置语言
    bool setLanguage(const std::wstring& languageCode);
    
    // 获取当前语言代码
    const std::wstring& getCurrentLanguage() const { return m_currentLanguage; }
    
    // 获取本地化字符串
    std::wstring getString(const std::wstring& key) const;
    
    // 根据控件ID获取本地化文本
    std::wstring getControlText(int controlId, const std::wstring& dialogName = L"") const;
    
    // 获取可用语言列表
    std::vector<std::pair<std::wstring, std::wstring>> getAvailableLanguages() const;
    
    // 清理语言名称缓存（当语言文件发生变化时使用）
    void clearLanguageNameCache();
    
    // 检查语言是否可用
    bool isLanguageAvailable(const std::wstring& languageCode) const;
    
    // 获取语言文件描述
    // path: 语言文件路径
    // file: 语言文件名（可选，为空时使用当前实例）
    // 返回: 语言文件的描述字符串
    std::wstring getLanguageFileDescription(const std::wstring& path, const std::wstring& file = L"") const;

private:
    LanguageManager();
    ~LanguageManager() = default;
    LanguageManager(const LanguageManager&) = delete;
    LanguageManager& operator=(const LanguageManager&) = delete;
    
    // 加载语言文件
    bool loadLanguageFile(const std::wstring& languageCode);
    
    // 解析JSON文件
    bool parseJsonFile(const std::wstring& filePath);
    
    // 获取系统语言
    std::wstring getSystemLanguage() const;
    
    // 获取语言文件路径
    std::wstring getLanguageFilePath(const std::wstring& languageCode) const;
    
    // 设置Windows线程语言
    void setWindowsThreadLanguage(const std::wstring& languageCode);
    
    // 初始化控件映射
    void initializeControlMappings();
    
    // 根据控件ID查找映射
    const ControlMapping* findControlMapping(int controlId) const;
    
    // 初始化字符串键到RC资源ID的映射
    void initializeStringResourceMappings();
    
    // 根据字符串键查找对应的RC资源ID
    int findStringResourceId(const std::wstring& key) const;

private:
    std::wstring m_currentLanguage;
    std::unordered_map<std::wstring, std::wstring> m_strings;
    std::vector<std::wstring> m_availableLanguages;
    std::unordered_map<int, ControlMapping> m_controlMappings;
    std::unordered_map<std::wstring, int> m_stringResourceMappings; // 字符串键到RC资源ID的映射
    bool m_initialized = false;
    mutable std::unordered_map<std::wstring, std::wstring> m_languageNameCache; // 语言名称缓存
    std::vector<LanguageConfig> m_supportedLanguages; // 支持的语言配置
    
    // 扫描语言文件目录
    void scanLanguageFiles();
    
    // 验证语言代码格式
    bool isValidLanguageCode(const std::wstring& langCode) const;
    
    // 从指定语言文件中读取语言名称
    std::wstring getLanguageNameFromFile(const std::wstring& languageCode) const;
    
    // 初始化支持的语言配置
    void initializeSupportedLanguages();
    
    // 查找语言配置
    const LanguageConfig* findLanguageConfig(const std::wstring& languageCode) const;
    
    // 字符串清理辅助方法
    void trimString(std::string& str) const;
    std::string extractJsonValue(const std::string& line, size_t colonPos) const;
    void processEscapeSequences(std::wstring& str) const;
    std::wstring getEnglishFallback(int controlId, const std::wstring& elementName) const;
};

// 便捷宏定义
#define LANG_MGR LanguageManager::getInstance()
#define LANG_STR(key) LANG_MGR.getString(L##key)