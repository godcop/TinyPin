#include "core/stdafx.h"
#include "foundation/string_utils.h"
#include <algorithm>

namespace Foundation {
namespace StringUtils {

// 移除加速键标记（&字符）
std::wstring remAccel(std::wstring s)
{
    std::wstring::size_type i = s.find_first_of(L"&");
    if (i != std::wstring::npos) s.erase(i, 1);
    return s;
}

// 返回字符串中最后一个分隔符之后的部分
// 示例: substrAfterLast("foobar", "oo") --> "bar"
// 错误时返回 ""
std::wstring substrAfterLast(const std::wstring& s, const std::wstring& delim)
{
    std::wstring::size_type i = s.find_last_of(delim);
    return i == std::wstring::npos ? L"" : s.substr(i + delim.length());
}

// 简单的通配符匹配函数，支持 * 和 ? 通配符
// * 匹配零个或多个字符
// ? 匹配任意单个字符
bool wildcardMatch(const wchar_t* pattern, const wchar_t* str)
{
    // 如果模式为空，字符串也必须为空才匹配
    if (!pattern || !*pattern)
        return !str || !*str;
        
    // 如果字符串为空，模式只能包含 * 才匹配
    if (!str || !*str) {
        while (*pattern == L'*') pattern++;
        return !*pattern;
    }
    
    // 处理 * 通配符
    if (*pattern == L'*') {
        // 跳过连续的 *
        while (*(pattern+1) == L'*') pattern++;
        
        // * 可以匹配零个或多个字符
        // 尝试匹配零个字符（跳过 *）或匹配当前字符并继续
        return wildcardMatch(pattern+1, str) || wildcardMatch(pattern, str+1);
    }
    
    // 处理 ? 通配符或精确匹配
    if (*pattern == L'?' || *pattern == *str)
        return wildcardMatch(pattern+1, str+1);
        
    return false;
}

// UTF-8 转换函数
std::wstring utf8ToWide(const std::string& utf8Str) {
    if (utf8Str.empty()) {
        return std::wstring();
    }
    
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (wideSize <= 0) {
        return std::wstring();
    }
    
    std::wstring wideStr(wideSize - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideSize);
    
    return wideStr;
}

std::string wideToUtf8(const std::wstring& wideStr) {
    if (wideStr.empty()) {
        return std::string();
    }
    
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8Size <= 0) {
        return std::string();
    }
    
    std::string utf8Str(utf8Size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Size, NULL, NULL);
    
    return utf8Str;
}

} // namespace StringUtils
} // namespace Foundation