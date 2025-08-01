#pragma once

#include "core/common.h"

namespace Platform {
namespace SystemInfo {

    // 操作系统版本检测
    DWORD packVer(DWORD major, DWORD minor);
    int getOsMajorVersion();
    DWORD getOsVersion();
    bool isWin8orGreater();
    
    // 屏幕信息获取
    bool getScrSize(SIZE& sz);

} // namespace SystemInfo
} // namespace Platform