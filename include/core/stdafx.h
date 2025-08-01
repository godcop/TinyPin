#pragma once

#define STRICT
#define WIN32_LEAN_AND_MEAN

// 现代Windows API设置
#undef _WIN32_WINNT
#define WINVER        0x0A00  // Windows 10
#define _WIN32_WINNT  0x0A00  // Windows 10
#define _WIN32_IE     0x0A00  // Internet Explorer 10

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <windowsx.h>
#include <prsht.h>
#include <commdlg.h>

// C++标准库
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>

// Windows API
#include <shlwapi.h>
#include <shlobj.h>

// DPI感知定义
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

// 项目头文件
#include "common.h"

// 工具模块聚合 - 提供统一的工具模块访问入口
#include "foundation/error_handler.h"
#include "foundation/string_utils.h"
#include "foundation/resource_utils.h"
#include "platform/system_info.h"
#include "platform/library_manager.h"
#include "platform/process_manager.h"
#include "platform/registry_utils.h"
#include "foundation/file_utils.h"
#include "graphics/font_utils.h"
#include "graphics/drawing_utils.h"
#include "graphics/color_utils.h"
#include "graphics/geometry_utils.h"
#include "graphics/dpi_manager.h"

#include "ui/dialog_utils.h"
#include "ui/custom_controls.h"
#include "window/window_helper.h"
#include "window/window_detector.h"
#include "window/window_monitor.h"
#include "graphics/window_highlighter.h"
