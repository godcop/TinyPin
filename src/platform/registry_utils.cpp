#include "core/stdafx.h"
#include "platform/registry_utils.h"
#include "system/logger.h"
#include <vector>

namespace Util {
namespace Registry {

// RegKeyHelper implementation
RegKeyHelper::RegKeyHelper(HKEY hKey) : m_hKey(hKey)
{
}

RegKeyHelper::~RegKeyHelper()
{
    if (m_hKey) {
        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }
}

RegKeyHelper RegKeyHelper::open(HKEY hKey, LPCWSTR subKey)
{
    HKEY resultKey = NULL;
    
    // 首先尝试只读权限
    LONG result = RegOpenKeyExW(hKey, subKey, 0, KEY_READ, &resultKey);
    if (result == ERROR_SUCCESS) {
        return RegKeyHelper(resultKey);
    }
    
    // 如果只读失败，尝试读写权限
    result = RegOpenKeyExW(hKey, subKey, 0, KEY_READ | KEY_WRITE, &resultKey);
    if (result == ERROR_SUCCESS) {
        return RegKeyHelper(resultKey);
    }
    
    return RegKeyHelper(NULL);
}

RegKeyHelper RegKeyHelper::create(HKEY hKey, LPCWSTR subKey)
{
    HKEY resultKey = NULL;
    DWORD disposition;
    if (RegCreateKeyExW(hKey, subKey, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &resultKey, &disposition) == ERROR_SUCCESS) {
        return RegKeyHelper(resultKey);
    }
    return RegKeyHelper(NULL);
}

bool RegKeyHelper::isValid() const
{
    return m_hKey != NULL;
}

HKEY RegKeyHelper::getHandle() const
{
    return m_hKey;
}

HKEY RegKeyHelper::release()
{
    HKEY handle = m_hKey;
    m_hKey = NULL;
    return handle;
}

bool RegKeyHelper::getDWord(LPCWSTR valueName, DWORD& value) const
{
    if (!m_hKey) return false;
    
    DWORD type = REG_DWORD;
    DWORD size = sizeof(DWORD);
    return RegQueryValueExW(m_hKey, valueName, NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS;
}

bool RegKeyHelper::setDWord(LPCWSTR valueName, DWORD value) const
{
    if (!m_hKey) return false;
    
    return RegSetValueExW(m_hKey, valueName, 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD)) == ERROR_SUCCESS;
}

bool RegKeyHelper::getString(LPCWSTR valueName, std::wstring& value) const
{
    if (!isValid()) {
        return false;
    }
    
    DWORD type = 0;
    DWORD size = 0;
    
    // 获取所需缓冲区大小
    LONG result1 = RegQueryValueExW(m_hKey, valueName, NULL, &type, NULL, &size);
    if (result1 != ERROR_SUCCESS) {
        return false;
    }
    
    // 检查类型是否为字符串类型
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        return false;
    }
    
    // 分配缓冲区并读取实际数据
    std::vector<WCHAR> buffer(size / sizeof(WCHAR) + 1, 0);
    LONG result2 = RegQueryValueExW(m_hKey, valueName, NULL, &type, (LPBYTE)&buffer[0], &size);
    if (result2 != ERROR_SUCCESS) {
        return false;
    }
    
    value = &buffer[0];
    return true;
}

bool RegKeyHelper::setString(LPCWSTR valueName, LPCWSTR value) const
{
    if (!m_hKey) return false;
    
    DWORD size = static_cast<DWORD>((wcslen(value) + 1) * sizeof(WCHAR));
    return RegSetValueExW(m_hKey, valueName, 0, REG_SZ, (LPBYTE)value, size) == ERROR_SUCCESS;
}

bool RegKeyHelper::deleteValue(LPCWSTR valueName) const
{
    if (!m_hKey) return false;
    
    return RegDeleteValueW(m_hKey, valueName) == ERROR_SUCCESS;
}

// AutoRegKeyHelper implementation
AutoRegKeyHelper::AutoRegKeyHelper(RegKeyHelper& key) : m_handle(key.release())
{
}

AutoRegKeyHelper::~AutoRegKeyHelper()
{
    if (m_handle) {
        RegCloseKey(m_handle);
        m_handle = NULL;
    }
}

bool AutoRegKeyHelper::getDWord(LPCWSTR valueName, DWORD& value) const
{
    if (!m_handle) return false;
    
    DWORD type = REG_DWORD;
    DWORD size = sizeof(DWORD);
    return RegQueryValueExW(m_handle, valueName, NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS;
}

bool AutoRegKeyHelper::setDWord(LPCWSTR valueName, DWORD value) const
{
    if (!m_handle) return false;
    
    return RegSetValueExW(m_handle, valueName, 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD)) == ERROR_SUCCESS;
}

bool AutoRegKeyHelper::getString(LPCWSTR valueName, std::wstring& value) const
{
    if (!isValid()) {
        return false;
    }
    
    DWORD type = 0;
    DWORD size = 0;
    
    // 获取所需缓冲区大小
    LONG result1 = RegQueryValueExW(m_handle, valueName, NULL, &type, NULL, &size);
    if (result1 != ERROR_SUCCESS) {
        return false;
    }
    
    // 检查类型是否为字符串类型
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        return false;
    }
    
    // 分配缓冲区并读取实际数据
    std::vector<WCHAR> buffer(size / sizeof(WCHAR) + 1, 0);
    LONG result2 = RegQueryValueExW(m_handle, valueName, NULL, &type, (LPBYTE)&buffer[0], &size);
    if (result2 != ERROR_SUCCESS) {
        return false;
    }
    
    value = &buffer[0];
    return true;
}

bool AutoRegKeyHelper::setString(LPCWSTR valueName, LPCWSTR value) const
{
    if (!m_handle) return false;
    
    DWORD size = static_cast<DWORD>((wcslen(value) + 1) * sizeof(WCHAR));
    return RegSetValueExW(m_handle, valueName, 0, REG_SZ, (LPBYTE)value, size) == ERROR_SUCCESS;
}

bool AutoRegKeyHelper::deleteValue(LPCWSTR valueName) const
{
    if (!m_handle) return false;
    
    return RegDeleteValueW(m_handle, valueName) == ERROR_SUCCESS;
}

bool AutoRegKeyHelper::isValid() const
{
    return m_handle != NULL;
}

HKEY AutoRegKeyHelper::getHandle() const
{
    return m_handle;
}

DWORD AutoRegKeyHelper::getValueType(LPCWSTR valueName) const
{
    if (!isValid()) {
        return REG_NONE;
    }
    
    DWORD type = REG_NONE;
    DWORD size = 0;
    LONG result = RegQueryValueExW(m_handle, valueName, NULL, &type, NULL, &size);
    
    // 只有在成功时才返回实际类型，失败时返回 REG_NONE
    if (result == ERROR_SUCCESS) {
        return type;
    } else {
        return REG_NONE;
    }
}

AutoRegKeyHelper::operator bool() const
{
    return isValid();
}

} // namespace Registry
} // namespace Util