#pragma once

#include "core/common.h"
#include <memory>
#include <string>

namespace Util {

    // RAII资源管理辅助类
    namespace RAII {
        // 删除器函数对象
        struct DCDeleter {
            void operator()(HDC dc) const { if (dc) DeleteDC(dc); }
        };
        
        struct ObjectDeleter {
            void operator()(HGDIOBJ obj) const { if (obj) DeleteObject(obj); }
        };
        
        struct HandleDeleter {
            void operator()(HANDLE h) const { if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h); }
        };
        
        // 通用RAII包装器模板
        template<typename T, typename Deleter>
        class ResourceGuard {
        private:
            T resource;
            Deleter deleter;
            bool valid;
            
        public:
            explicit ResourceGuard(T res, Deleter del = Deleter()) 
                : resource(res), deleter(del), valid(res != nullptr) {}
            
            ~ResourceGuard() {
                if (valid && resource) {
                    deleter(resource);
                }
            }
            
            // 禁止复制
            ResourceGuard(const ResourceGuard&) = delete;
            ResourceGuard& operator=(const ResourceGuard&) = delete;
            
            // 允许移动
            ResourceGuard(ResourceGuard&& other) noexcept 
                : resource(other.resource), deleter(std::move(other.deleter)), valid(other.valid) {
                other.valid = false;
            }
            
            ResourceGuard& operator=(ResourceGuard&& other) noexcept {
                if (this != &other) {
                    // 清理当前资源
                    if (valid && resource) {
                        deleter(resource);
                    }
                    // 移动新资源
                    resource = other.resource;
                    deleter = std::move(other.deleter);
                    valid = other.valid;
                    other.valid = false;
                }
                return *this;
            }
            
            T get() const { return resource; }
            bool isValid() const { return valid; }
            
            // 释放所有权
            T release() {
                valid = false;
                return resource;
            }
        };
        
        // 常用的RAII包装器
        using DCGuard = ResourceGuard<HDC, DCDeleter>;
        using RegionGuard = ResourceGuard<HRGN, ObjectDeleter>;
        using BitmapGuard = ResourceGuard<HBITMAP, ObjectDeleter>;
        using FontGuard = ResourceGuard<HFONT, ObjectDeleter>;
        using BrushGuard = ResourceGuard<HBRUSH, ObjectDeleter>;
        using HandleGuard = ResourceGuard<HANDLE, HandleDeleter>;
        
        // 便利函数模板，减少重复代码
        template<typename T, typename Deleter>
        inline ResourceGuard<T, Deleter> makeGuard(T resource) {
            return ResourceGuard<T, Deleter>(resource);
        }
        
        // 特化的便利函数
        inline DCGuard makeDCGuard(HDC dc) { return makeGuard<HDC, DCDeleter>(dc); }
        inline RegionGuard makeRegionGuard(HRGN rgn) { return makeGuard<HRGN, ObjectDeleter>(rgn); }
        inline BitmapGuard makeBitmapGuard(HBITMAP bmp) { return makeGuard<HBITMAP, ObjectDeleter>(bmp); }
        inline FontGuard makeFontGuard(HFONT font) { return makeGuard<HFONT, ObjectDeleter>(font); }
        inline BrushGuard makeBrushGuard(HBRUSH brush) { return makeGuard<HBRUSH, ObjectDeleter>(brush); }
        inline HandleGuard makeHandleGuard(HANDLE handle) { return makeGuard<HANDLE, HandleDeleter>(handle); }
        
        // 特殊的Paint RAII包装器
        class PaintGuard {
        private:
            HWND hwnd;
            PAINTSTRUCT& ps;
            
        public:
            PaintGuard(HWND h, PAINTSTRUCT& p) : hwnd(h), ps(p) {}
            ~PaintGuard() { EndPaint(hwnd, &ps); }
            
            // 禁止复制和移动
            PaintGuard(const PaintGuard&) = delete;
            PaintGuard& operator=(const PaintGuard&) = delete;
            PaintGuard(PaintGuard&&) = delete;
            PaintGuard& operator=(PaintGuard&&) = delete;
        };
        
        inline PaintGuard makePaintGuard(HWND hwnd, PAINTSTRUCT& ps) {
            return PaintGuard(hwnd, ps);
        }
    }

    // 本地化资源管理
    namespace Res {
        // 本地化菜单加载
        HMENU LoadLocalizedMenu(LPCTSTR lpMenuName);
        HMENU LoadLocalizedMenu(WORD id);
        
        // 本地化对话框操作
        int   LocalizedDialogBoxParam(LPCTSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
        int   LocalizedDialogBoxParam(WORD id, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
        HWND  CreateLocalizedDialog(LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc);
        HWND  CreateLocalizedDialog(WORD id, HWND hWndParent, DLGPROC lpDialogFunc);

        // 资源字符串加载和格式化
        // 自动使用已加载的语言DLL（如果有的话）
        class ResStr {
        public:
            ResStr(UINT id, size_t bufLen = Constants::MAX_CLASSNAME_LEN) {
                init(id, bufLen); }
            ResStr(UINT id, size_t bufLen, DWORD_PTR p1) {
                init(id, bufLen, &p1); }
            ResStr(UINT id, size_t bufLen, DWORD_PTR p1, DWORD_PTR p2) {
                DWORD_PTR params[] = {p1,p2}; init(id, bufLen, params); }
            ResStr(UINT id, size_t bufLen, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3) {
                DWORD_PTR params[] = {p1,p2,p3}; init(id, bufLen, params); }
            ResStr(UINT id, size_t bufLen, DWORD_PTR* params) {
                init(id, bufLen, params); }
            
            // 默认析构函数，智能指针自动管理内存
            ~ResStr() = default;
            
            // 禁止复制
            ResStr(const ResStr&) = delete;
            ResStr& operator=(const ResStr&) = delete;
            
            // 允许移动
            ResStr(ResStr&& other) noexcept : str(std::move(other.str)) {}
            ResStr& operator=(ResStr&& other) noexcept {
                if (this != &other) {
                    str = std::move(other.str);
                }
                return *this;
            }
            
            operator LPCWSTR() const { return str.get(); }
            
        private:
            std::unique_ptr<WCHAR[]> str;
            void init(UINT id, size_t bufLen, DWORD_PTR* params = nullptr);
        };
    }

} // namespace Util