#include "core/stdafx.h"
#include "foundation/file_utils.h"

// 文件系统操作功能实现

std::vector<std::wstring> Foundation::FileUtils::getFiles(std::wstring mask)
{
    std::vector<std::wstring> ret;
    
    // 提取路径和文件掩码
    std::wstring path;
    size_t lastSlash = mask.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        path = mask.substr(0, lastSlash + 1);
        mask = mask.substr(lastSlash + 1);
    }
    
    // 设置搜索数据
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((path + mask).c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // 跳过目录
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                ret.push_back(findData.cFileName);
            }
        } while (FindNextFileW(hFind, &findData));
        
        FindClose(hFind);
    }
    
    return ret;
}

bool Foundation::FileUtils::readFileBack(HANDLE file, void* buf, int bytes)
{
    DWORD read;
    return SetFilePointer(file, -bytes, 0, FILE_CURRENT) != -1
        && ReadFile(file, buf, bytes, &read, 0)
        && int(read) == bytes
        && SetFilePointer(file, -bytes, 0, FILE_CURRENT) != -1;
}

// 模块路径获取功能实现

// 获取模块路径
std::wstring Foundation::FileUtils::getModulePath(HINSTANCE hInstance)
{
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(hInstance, path, MAX_PATH) == 0) {
        return L"";
    }
    return std::wstring(path);
}

// 获取目录部分
std::wstring Foundation::FileUtils::getDirPath(const std::wstring& path)
{
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L"";
    }
    return path.substr(0, pos + 1);
}

// 获取文件名部分
std::wstring Foundation::FileUtils::getFileName(const std::wstring& path)
{
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return path;
    }
    return path.substr(pos + 1);
}