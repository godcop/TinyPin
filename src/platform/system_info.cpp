#include "core/stdafx.h"
#include "platform/system_info.h"

// 操作系统版本检测功能实现

// 将主版本号和次版本号打包为单个整数值用于版本比较
DWORD Platform::SystemInfo::packVer(DWORD major, DWORD minor)
{
    return (major << 16) | minor;
}

// 获取操作系统主版本号
int Platform::SystemInfo::getOsMajorVersion()
{
    // 使用VerifyVersionInfo替代已弃用的GetVersionEx
    // 检查是否为Windows 10或更高版本 (10.0)
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    osvi.dwMajorVersion = 10;
    DWORDLONG dwlConditionMask = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION, dwlConditionMask)) {
        return 10;
    }
    
    // 检查是否为Windows 8.1 (6.3)
    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 3;
    dwlConditionMask = VerSetConditionMask(
        VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
        VER_MINORVERSION, VER_GREATER_EQUAL);
    if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask)) {
        return 6;
    }
    
    // 检查是否为Windows 7 (6.1)
    osvi.dwMinorVersion = 1;
    if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask)) {
        return 6;
    }
    
    // 检查是否为Windows Vista (6.0)
    osvi.dwMinorVersion = 0;
    if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask)) {
        return 6;
    }
    
    // 默认返回5 (Windows XP/2000)
    return 5;
}

// 获取操作系统版本号（主版本号和次版本号打包在一起）
DWORD Platform::SystemInfo::getOsVersion()
{
    // 使用VerifyVersionInfo替代已弃用的GetVersionEx
    // 检查是否为Windows 10或更高版本 (10.0)
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    osvi.dwMajorVersion = 10;
    osvi.dwMinorVersion = 0;
    DWORDLONG dwlConditionMask = VerSetConditionMask(
        VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
        VER_MINORVERSION, VER_GREATER_EQUAL);
    if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask)) {
        return packVer(10, 0);
    }
    
    // 检查是否为Windows 8.1 (6.3)
    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 3;
    dwlConditionMask = VerSetConditionMask(
        VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
        VER_MINORVERSION, VER_GREATER_EQUAL);
    if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask)) {
        return packVer(6, 3);
    }
    
    // 检查是否为Windows 8 (6.2)
    osvi.dwMinorVersion = 2;
    if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask)) {
        return packVer(6, 2);
    }
    
    // 检查是否为Windows 7 (6.1)
    osvi.dwMinorVersion = 1;
    if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask)) {
        return packVer(6, 1);
    }
    
    // 检查是否为Windows Vista (6.0)
    osvi.dwMinorVersion = 0;
    if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask)) {
        return packVer(6, 0);
    }
    
    // 默认返回Windows XP (5.1)
    return packVer(5, 1);
}

bool Platform::SystemInfo::isWin8orGreater()
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    DWORDLONG const dwlConditionMask = VerSetConditionMask(
        VerSetConditionMask(
            0, VER_MAJORVERSION, VER_GREATER_EQUAL),
        VER_MINORVERSION, VER_GREATER_EQUAL);

    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 2; // Windows 8是6.2

    return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) != FALSE;
}

// 屏幕信息获取功能实现

bool Platform::SystemInfo::getScrSize(SIZE& sz)
{
    return ((sz.cx = GetSystemMetrics(SM_CXSCREEN)) != 0 &&
        (sz.cy = GetSystemMetrics(SM_CYSCREEN)) != 0);
}