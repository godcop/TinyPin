@echo off
chcp 65001 >nul
echo ========================================
echo 微钉 TinyPin 多平台安装包构建脚本
echo ========================================
echo.

REM 获取脚本所在目录并设置项目根目录
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%..\.."
set "SOLUTION_FILE=%PROJECT_ROOT%\TinyPin.sln"
set "INSTALLER_SCRIPT=%SCRIPT_DIR%TinyPin.iss"

echo 开始构建多平台安装包...
echo.

REM 检查 MSBuild 是否可用
where msbuild >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误：找不到 MSBuild，请确保已安装 Visual Studio 或 Build Tools
    exit /b 1
)

REM 检查 Inno Setup 编译器是否可用
set "ISCC_PATH=%PROJECT_ROOT%\tools\InnoSetup6\ISCC.exe"
if not exist "%ISCC_PATH%" (
    echo 错误：找不到 ISCC.exe，请确保 Inno Setup 编译器已放置在项目中
    echo 预期路径：%ISCC_PATH%
    exit /b 1
)

REM 构建 x64 平台
echo [1/3] 构建 x64 平台...
echo 编译 x64 版本...
msbuild "%SOLUTION_FILE%" /p:Configuration=Release /p:Platform=x64 /nologo /verbosity:minimal
if %errorlevel% neq 0 (
    echo 错误：x64 平台编译失败
    goto :error
)

echo 打包 x64 安装包...
"%ISCC_PATH%" "%INSTALLER_SCRIPT%" /Dp=x64 /Ds="%PROJECT_ROOT%\build\compile\Release\x64\TinyPin.exe" /Do="%PROJECT_ROOT%\build\installer"
if %errorlevel% neq 0 (
    echo 错误：x64 安装包打包失败
    goto :error
)
echo x64 平台构建完成
echo.

REM 构建 Win32 平台
echo [2/3] 构建 Win32 平台...
echo 编译 Win32 版本...
msbuild "%SOLUTION_FILE%" /p:Configuration=Release /p:Platform=Win32 /nologo /verbosity:minimal
if %errorlevel% neq 0 (
    echo 错误：Win32 平台编译失败
    goto :error
)

echo 打包 Win32 安装包...
"%ISCC_PATH%" "%INSTALLER_SCRIPT%" /Dp=Win32 /Ds="%PROJECT_ROOT%\build\compile\Release\Win32\TinyPin.exe" /Do="%PROJECT_ROOT%\build\installer"
if %errorlevel% neq 0 (
    echo 错误：Win32 安装包打包失败
    goto :error
)
echo Win32 平台构建完成
echo.

REM 构建 ARM64 平台
echo [3/3] 构建 ARM64 平台...
echo 编译 ARM64 版本...
msbuild "%SOLUTION_FILE%" /p:Configuration=Release /p:Platform=ARM64 /nologo /verbosity:minimal
if %errorlevel% neq 0 (
    echo 错误：ARM64 平台编译失败
    goto :error
)

echo 打包 ARM64 安装包...
"%ISCC_PATH%" "%INSTALLER_SCRIPT%" /Dp=arm64 /Ds="%PROJECT_ROOT%\build\compile\Release\ARM64\TinyPin.exe" /Do="%PROJECT_ROOT%\build\installer"
if %errorlevel% neq 0 (
    echo 错误：ARM64 安装包打包失败
    goto :error
)
echo ARM64 平台构建完成
echo.

echo ========================================
echo 所有平台构建完成！
echo ========================================
echo 生成的安装包：
dir /b "%PROJECT_ROOT%\build\installer\TinyPin-1.0-*-setup.exe" 2>nul
echo.
echo 编译文件位置：%PROJECT_ROOT%\build\compile\
echo 安装包位置：%PROJECT_ROOT%\build\installer\
echo 构建完成时间：%date% %time%
exit /b 0

:error
echo.
echo ========================================
echo 构建失败！
echo ========================================
exit /b 1