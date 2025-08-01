#pragma once

#include "core/common.h"
#include <string>
#include <vector>

namespace Foundation {
namespace FileUtils {

    // 文件系统操作
    std::vector<std::wstring> getFiles(std::wstring mask);
    bool readFileBack(HANDLE file, void* buf, int bytes);
    
    // 模块路径获取
    std::wstring getModulePath(HINSTANCE hInstance);
    std::wstring getDirPath(const std::wstring& path);
    std::wstring getFileName(const std::wstring& path);

} // namespace FileUtils
} // namespace Foundation