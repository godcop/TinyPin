#pragma once

#include "core/common.h"
#include <string>
#include <vector>

namespace Platform {

    // 动态库管理器，用于统一管理库的加载和释放
    class LibraryManager {
    public:
        LibraryManager() = default;
        ~LibraryManager();
        
        // 禁止复制
        LibraryManager(const LibraryManager&) = delete;
        LibraryManager& operator=(const LibraryManager&) = delete;
        
        // 允许移动
        LibraryManager(LibraryManager&& other) noexcept;
        LibraryManager& operator=(LibraryManager&& other) noexcept;
        
        // 加载库
        HMODULE loadLibrary(const std::wstring& libraryName);
        
        // 获取函数地址
        template<typename T>
        T getProcAddress(HMODULE hModule, const std::string& procName) {
            return reinterpret_cast<T>(::GetProcAddress(hModule, procName.c_str()));
        }
        
        // 释放指定库
        void freeLibrary(HMODULE hModule);
        
        // 释放所有库
        void freeAllLibraries();
        
    private:
        std::vector<HMODULE> m_loadedLibraries;
    };

} // namespace Platform