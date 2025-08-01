#pragma once

#include "core/common.h"
#include <string>

namespace Foundation {
namespace StringUtils {
    // 字符串比较函数
    inline bool strmatch(LPCWSTR s1, LPCWSTR s2) {
        return wcscmp(s1, s2) == 0;
    }
    
    inline bool strimatch(LPCWSTR s1, LPCWSTR s2) {
        return _wcsicmp(s1, s2) == 0;
    }
    
    // 字符串操作函数
    std::wstring remAccel(std::wstring s);
    std::wstring substrAfterLast(const std::wstring& s, const std::wstring& delim);
    
    // 通配符匹配函数
    bool wildcardMatch(const wchar_t* pattern, const wchar_t* str);
    
    // UTF-8 转换函数
    std::wstring utf8ToWide(const std::string& utf8Str);
    std::string wideToUtf8(const std::wstring& wideStr);

} // namespace StringUtils
} // namespace Foundation