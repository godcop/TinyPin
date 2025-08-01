; 微钉 Inno Setup 安装脚本
; 这是一个用于创建 TinyPin 应用程序安装包的 Inno Setup 脚本文件
;
; 构建说明：
; 1. 先编译项目（选择目标平台）：
;    - x64 平台：msbuild TinyPin.sln /p:Configuration=Release /p:Platform=x64 /p:OutDir="build\compile\Release\x64\"
;    - Win32 平台：msbuild TinyPin.sln /p:Configuration=Release /p:Platform=Win32 /p:OutDir="build\compile\Release\Win32\"
;    - ARM64 平台：msbuild TinyPin.sln /p:Configuration=Release /p:Platform=ARM64 /p:OutDir="build\compile\Release\ARM64\"
; 2. 再运行打包（通过命令行参数指定平台和可执行文件路径）：
;    - x64 平台：ISCC.exe TinyPin.iss /Dp=x64 /Ds="..\..\build\compile\Release\x64\TinyPin.exe" /Do="..\..\build\installer"
;    - Win32 平台：ISCC.exe TinyPin.iss /Dp=Win32 /Ds="..\..\build\compile\Release\Win32\TinyPin.exe" /Do="..\..\build\installer"
;    - ARM64 平台：ISCC.exe TinyPin.iss /Dp=arm64 /Ds="..\..\build\compile\Release\ARM64\TinyPin.exe" /Do="..\..\build\installer"
; 3. 输出路径：build\installer\TinyPin-1.0-{平台}-setup.exe
;
; 命令行参数机制：
; - 通过 /D 参数指定平台标识、可执行文件路径和版本号
; - p：平台标识（x64、Win32、arm64）
; - s：可执行文件的相对路径
; - v：版本号（如：1.0.1）
; - 安装包文件名会自动包含指定的平台标识和版本号
; - 架构要求会根据指定的平台自动设置
; - 如果未指定参数，会使用默认值（x64 平台，1.0.0 版本）
;
; 多平台支持详情：
; - x64 平台：生成 TinyPin-1.0-x64-setup.exe，只能在 64 位系统安装
; - Win32 平台：生成 TinyPin-1.0-Win32-setup.exe，可在 32 位和 64 位系统安装
; - ARM64 平台：生成 TinyPin-1.0-arm64-setup.exe，只能在 ARM64 系统安装
;
; 完整的多平台发布流程：
; 1. 编译 x64 版本：msbuild TinyPin.sln /p:Configuration=Release /p:Platform=x64 /p:OutDir="build\compile\Release\x64\"
; 2. 打包 x64 安装包：ISCC.exe TinyPin.iss /Dp=x64 /Ds="..\..\build\compile\Release\x64\TinyPin.exe" /Do="..\..\build\installer"
; 3. 编译 Win32 版本：msbuild TinyPin.sln /p:Configuration=Release /p:Platform=Win32 /p:OutDir="build\compile\Release\Win32\"
; 4. 打包 Win32 安装包：ISCC.exe TinyPin.iss /Dp=Win32 /Ds="..\..\build\compile\Release\Win32\TinyPin.exe" /Do="..\..\build\installer"
; 5. 编译 ARM64 版本：msbuild TinyPin.sln /p:Configuration=Release /p:Platform=ARM64 /p:OutDir="build\compile\Release\ARM64\"
; 6. 打包 ARM64 安装包：ISCC.exe TinyPin.iss /Dp=arm64 /Ds="..\..\build\compile\Release\ARM64\TinyPin.exe" /Do="..\..\build\installer"
;
; 项目结构：
; - 源代码：src\ 目录
; - 头文件：include\ 目录  
; - 资源文件：resources\ 目录（编译时使用，不打包到安装程序）
; - 应用资源：assets\ 目录（需要整体打包到安装程序）
; - 编译输出：build\compile\$(Configuration)\$(Platform)\ 目录
; - 安装包输出：build\installer\ 目录
;
; 文件编码：UTF-8 LF 无 BOM 格式

; ========== 平台自动识别定义 ==========
#define AppName "TinyPin"
; 版本号可通过命令行参数 /Dv 指定，如果未指定则使用默认值
#ifndef v
  #define v "1.0.0"
#endif
#define Version v
#define Publisher "godcop"
#define URL "https://github.com/godcop/TinyPin"

; ========== 命令行参数定义 ==========
; 通过命令行参数指定平台和可执行文件路径
; 如果未指定参数，使用默认值（x64 平台）
#ifndef p
  #define p "x64"
#endif
#define Platform p

#ifndef s
  #define s "..\..\build\compile\Release\x64\TinyPin.exe"
#endif
#define SourceExe s

; 验证指定的可执行文件是否存在
#if !FileExists(SourceExe)
  #error "指定的可执行文件不存在: " + SourceExe + "，请先编译项目或检查路径"
#endif

[Setup]
; ========== 应用程序基本信息 ==========
; 应用程序名称，使用自动定义的变量
AppName={#AppName}
; 应用程序版本号，使用自动定义的变量
AppVersion={#Version}
; 应用程序完整版本名称，包含名称和版本号
AppVerName={#AppName} {#Version}
; 发布者名称，使用自动定义的变量
AppPublisher={#Publisher}
; 发布者官方网站 URL，使用自动定义的变量
AppPublisherURL={#URL}
; 技术支持网站 URL，用户可通过此链接获取帮助
AppSupportURL={#URL}/issues
; 软件更新检查 URL，用于检查新版本
AppUpdatesURL={#URL}/releases
; 版权信息，显示在程序属性中
AppCopyright=Copyright © 2025 {#Publisher}

; ========== 安装目录和文件设置 ==========
; 默认安装目录，{autopf} 表示 Program Files 文件夹
DefaultDirName={autopf}\{#AppName}
; 开始菜单中的程序组名称
DefaultGroupName={#AppName}
; 允许用户选择不创建开始菜单图标
AllowNoIcons=yes

; ========== 安装包输出设置 ==========
; 生成的安装包输出目录，默认为 build\installer 目录，可通过 /Do 参数指定
#ifndef o
  #define o "..\..\build\installer"
#endif
OutputDir={#o}
; 生成的安装包文件名（不含扩展名），平台自动识别
OutputBaseFilename={#AppName}-{#Version}-{#Platform}-setup
; 安装包的图标文件路径
SetupIconFile=..\..\resources\app.ico

; ========== 压缩设置 ==========
; 使用 LZMA 压缩算法，提供最佳压缩率
Compression=lzma
; 启用固实压缩，进一步减小安装包大小
SolidCompression=yes

; ========== 系统要求 ==========
; 最低支持的 Windows 版本（6.1sp1 = Windows 7 SP1）
MinVersion=6.1sp1

; 根据检测到的平台自动设置架构要求
#if Platform == "x64"
  ; 只允许在 64 位兼容系统上安装
  ArchitecturesAllowed=x64compatible
  ; 在 64 位系统上以 64 位模式安装
  ArchitecturesInstallIn64BitMode=x64compatible
#elif Platform == "Win32"
  ; 允许在 32 位和 64 位系统上安装
  ArchitecturesAllowed=x86 x64compatible
  ; 在 64 位系统上以 32 位模式安装
  ArchitecturesInstallIn64BitMode=
#elif Platform == "arm64"
  ; 只允许在 ARM64 系统上安装
  ArchitecturesAllowed=arm64
  ; 在 ARM64 系统上以 64 位模式安装
  ArchitecturesInstallIn64BitMode=arm64
#endif

; 使用最低权限安装，避免不必要的管理员权限
PrivilegesRequired=lowest

; ========== 安装向导界面设置 ==========
; 使用现代风格的安装向导界面
WizardStyle=modern
; 禁用窗口大小调整，保持固定尺寸
WizardResizable=no
; 禁用程序组选择页面，使用默认设置
DisableProgramGroupPage=yes
; 启用"准备安装"页面，让用户确认安装设置
DisableReadyPage=no

; ========== 许可证文件 ==========
; 许可证文件路径，安装时会显示给用户
LicenseFile=..\..\LICENSE.txt

; ========== 卸载程序设置 ==========
; 卸载程序在控制面板中显示的图标
UninstallDisplayIcon={app}\{#AppName}.exe
; 卸载程序在控制面板中显示的名称
UninstallDisplayName={#AppName}

[Languages]
; ========== 安装程序支持的语言 ==========
; 英语界面，使用默认的英语消息文件
Name: "english"; MessagesFile: "compiler:Default.isl"
; 简体中文界面，使用内置的简体中文消息文件
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
; ========== 可选安装任务 ==========
; 创建桌面图标任务，默认不选中
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
; 开机自启动任务，默认不选中
Name: "startup"; Description: "开机自动启动 微钉"; GroupDescription: "启动选项:"; Flags: unchecked

[Files]
; ========== 主程序文件 ==========
; 使用自动检测到的可执行文件路径
Source: "{#SourceExe}"; DestDir: "{app}"; Flags: ignoreversion

; ========== 资源文件 ==========
; 许可证文件
Source: "..\..\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion

; 说明文档
Source: "..\..\README.md"; DestDir: "{app}"; Flags: ignoreversion

; ========== 资源目录 ==========
; 整个 assets 目录（包含图片、语言包等）
Source: "..\..\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; ========== 创建的快捷方式图标 ==========
; 开始菜单图标
; 在开始菜单程序组中创建应用快捷方式
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppName}.exe"
; 在开始菜单程序组中创建卸载程序快捷方式
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
; 桌面图标（可选）
; 在桌面创建快捷方式，仅当用户选择了桌面图标任务时
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Tasks: desktopicon

[Registry]
; ========== 注册表项设置 ==========
; 应用程序安装路径
; 在当前用户注册表中记录安装路径，避免需要管理员权限，卸载时删除整个注册表键
Root: HKCU; Subkey: "SOFTWARE\{#Publisher}\{#AppName}"; ValueType: string; ValueName: "Install_Dir"; ValueData: "{app}"; Flags: uninsdeletekey

; 开机自启动（可选）
; 在当前用户的启动项中添加应用，仅当用户选择了开机自启动任务时，卸载时删除此值
Root: HKCU; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "{#AppName}"; ValueData: "{app}\{#AppName}.exe"; Flags: uninsdeletevalue; Tasks: startup

[Run]
; ========== 安装完成后执行的操作 ==========
; 安装完成后运行程序（可选）
; 安装完成后询问用户是否立即运行程序，不等待程序结束，静默安装时跳过
Filename: "{app}\{#AppName}.exe"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; ========== 卸载时需要删除的文件和目录 ==========
; 清理用户配置文件
; 删除用户应用数据目录中的应用文件夹及其所有内容
Type: filesandordirs; Name: "{userappdata}\{#AppName}"
; 清理应用资源目录
; 删除安装目录中的 assets 文件夹及其所有内容
Type: filesandordirs; Name: "{app}\assets"
; 清理临时文件
; 删除应用程序目录中的所有日志文件
Type: files; Name: "{app}\*.log"

[Code]
procedure InitializeWizard();
begin
  { 设置安装向导窗口大小为 400x300 像素（明显比默认的 563x440 小） }
  WizardForm.ClientWidth := ScaleX(380);
  WizardForm.ClientHeight := ScaleY(250);
  { 确保窗口居中显示 }
  WizardForm.Position := poScreenCenter;
end;