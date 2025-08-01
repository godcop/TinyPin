#pragma once

#include "core/stdafx.h"
#include <string>

namespace Util {
namespace Registry {

// 注册表键辅助类，用于替代ef::Win::RegKeyH
class RegKeyHelper {
public:
    // 打开现有注册表键
    static RegKeyHelper open(HKEY hKey, LPCWSTR subKey);
    
    // 创建或打开注册表键
    static RegKeyHelper create(HKEY hKey, LPCWSTR subKey);
    
    // 析构函数，关闭注册表键
    ~RegKeyHelper();
    
    // 获取DWORD值
    bool getDWord(LPCWSTR valueName, DWORD& value) const;
    
    // 设置DWORD值
    bool setDWord(LPCWSTR valueName, DWORD value) const;
    
    // 获取字符串值
    bool getString(LPCWSTR valueName, std::wstring& value) const;
    
    // 设置字符串值
    bool setString(LPCWSTR valueName, LPCWSTR value) const;
    
    // 删除值
    bool deleteValue(LPCWSTR valueName) const;
    
    // 检查键是否有效
    bool isValid() const;
    
    // 获取原始HKEY句柄
    HKEY getHandle() const;
    
    // 释放句柄所有权（调用者负责关闭句柄）
    HKEY release();
    
private:
    RegKeyHelper(HKEY hKey);
    
    HKEY m_hKey;
};

// 自动关闭的注册表键辅助类，用于替代s::Win::AutoRegKeyH
class AutoRegKeyHelper {
public:
    AutoRegKeyHelper(RegKeyHelper& key);  // 注意：不再是 const 引用
    ~AutoRegKeyHelper();
    
    // 禁用拷贝构造和赋值操作
    AutoRegKeyHelper(const AutoRegKeyHelper&) = delete;
    AutoRegKeyHelper& operator=(const AutoRegKeyHelper&) = delete;
    
    // 获取DWORD值
    bool getDWord(LPCWSTR valueName, DWORD& value) const;
    
    // 设置DWORD值
    bool setDWord(LPCWSTR valueName, DWORD value) const;
    
    // 获取字符串值
    bool getString(LPCWSTR valueName, std::wstring& value) const;
    
    // 设置字符串值
    bool setString(LPCWSTR valueName, LPCWSTR value) const;
    
    // 删除值
    bool deleteValue(LPCWSTR valueName) const;
    
    // 检查键是否有效
    bool isValid() const;
    
    // 获取原始HKEY句柄
    HKEY getHandle() const;
    
    // 获取值类型
    DWORD getValueType(LPCWSTR valueName) const;
    
    // 转换为bool（检查是否有效）
    operator bool() const;
    
private:
    HKEY m_handle;  // 直接持有句柄，避免拷贝问题
};

} // namespace Registry
} // namespace Util