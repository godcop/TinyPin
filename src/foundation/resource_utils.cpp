#include "core/stdafx.h"
#include "foundation/resource_utils.h"
#include "core/application.h"

namespace {
    // 检查最后的错误是否为资源未找到
    bool isLastErrResNotFound()
    {
        DWORD e = GetLastError();
        return e == ERROR_RESOURCE_DATA_NOT_FOUND ||
            e == ERROR_RESOURCE_TYPE_NOT_FOUND ||
            e == ERROR_RESOURCE_NAME_NOT_FOUND ||
            e == ERROR_RESOURCE_LANG_NOT_FOUND;
    }
}

// 本地化菜单加载实现
HMENU Util::Res::LoadLocalizedMenu(LPCTSTR lpMenuName)
{
    if (app.resMod) {
        HMENU ret = LoadMenu(app.resMod, lpMenuName);
        if (ret)
            return ret;
    }
    return LoadMenu(app.inst, lpMenuName);
}

HMENU Util::Res::LoadLocalizedMenu(WORD id)
{
    return Util::Res::LoadLocalizedMenu(MAKEINTRESOURCE(id));
}

// 本地化对话框操作实现
int Util::Res::LocalizedDialogBoxParam(LPCTSTR lpTemplate, HWND hParent, DLGPROC lpDialogFunc, LPARAM dwInit)
{
    if (app.resMod) {
        int ret = static_cast<int>(DialogBoxParam(app.resMod, lpTemplate, hParent, lpDialogFunc, dwInit));
        if (ret != -1 || !isLastErrResNotFound())
            return ret;
    }
    return static_cast<int>(DialogBoxParam(app.inst, lpTemplate, hParent, lpDialogFunc, dwInit));
}

int Util::Res::LocalizedDialogBoxParam(WORD id, HWND hParent, DLGPROC lpDialogFunc, LPARAM dwInit)
{
    return Util::Res::LocalizedDialogBoxParam(MAKEINTRESOURCE(id), hParent, lpDialogFunc, dwInit);
}

HWND Util::Res::CreateLocalizedDialog(LPCTSTR lpTemplate, HWND hParent, DLGPROC lpDialogFunc)
{
    if (app.resMod) {
        HWND ret = CreateDialog(app.resMod, lpTemplate, hParent, lpDialogFunc);
        if (ret || !isLastErrResNotFound())
            return ret;
    }
    return CreateDialog(app.inst, lpTemplate, hParent, lpDialogFunc);
}

HWND Util::Res::CreateLocalizedDialog(WORD id, HWND hParent, DLGPROC lpDialogFunc)
{
    return Util::Res::CreateLocalizedDialog(MAKEINTRESOURCE(id), hParent, lpDialogFunc);
}

// 资源字符串处理实现
void Util::Res::ResStr::init(UINT id, size_t bufLen, DWORD_PTR* params) {
    str = std::make_unique<WCHAR[]>(bufLen);
    if (!app.resMod || !LoadString(app.resMod, id, str.get(), static_cast<int>(bufLen)))
        LoadString(app.inst, id, str.get(), static_cast<int>(bufLen));
    if (params) {
        DWORD flags = FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY;
        va_list* va = reinterpret_cast<va_list*>(params);
        if (!FormatMessage(flags, std::wstring(str.get()).c_str(), 0, 0, str.get(), static_cast<DWORD>(bufLen), va))
            str.get()[0] = 0;
    }
}