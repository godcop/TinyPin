#include "core/stdafx.h"
#include "platform/library_manager.h"
#include <algorithm>

// 动态库管理器实现

Platform::LibraryManager::~LibraryManager() {
    freeAllLibraries();
}

Platform::LibraryManager::LibraryManager(LibraryManager&& other) noexcept 
    : m_loadedLibraries(std::move(other.m_loadedLibraries)) {
    other.m_loadedLibraries.clear();
}

Platform::LibraryManager& Platform::LibraryManager::operator=(LibraryManager&& other) noexcept {
    if (this != &other) {
        freeAllLibraries();
        m_loadedLibraries = std::move(other.m_loadedLibraries);
        other.m_loadedLibraries.clear();
    }
    return *this;
}

HMODULE Platform::LibraryManager::loadLibrary(const std::wstring& libraryName) {
    HMODULE hModule = LoadLibraryW(libraryName.c_str());
    if (hModule) {
        m_loadedLibraries.push_back(hModule);
    }
    return hModule;
}

void Platform::LibraryManager::freeLibrary(HMODULE hModule) {
    if (hModule) {
        auto it = std::find(m_loadedLibraries.begin(), m_loadedLibraries.end(), hModule);
        if (it != m_loadedLibraries.end()) {
            FreeLibrary(hModule);
            m_loadedLibraries.erase(it);
        }
    }
}

void Platform::LibraryManager::freeAllLibraries() {
    for (HMODULE hModule : m_loadedLibraries) {
        if (hModule) {
            FreeLibrary(hModule);
        }
    }
    m_loadedLibraries.clear();
}