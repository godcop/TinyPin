# TinyPin 多平台安装包构建指南

## 概述

TinyPin 安装脚本现在支持通过命令行参数指定目标平台和可执行文件路径，提供更灵活的构建控制。

## 支持的平台

| 平台 | 安装包文件名 | 系统要求 | 说明 |
|------|-------------|----------|------|
| x64 | `TinyPin-1.0-x64-setup.exe` | 仅 64 位系统 | 标准 64 位版本 |
| Win32 | `TinyPin-1.0-Win32-setup.exe` | 32 位和 64 位系统 | 兼容性最好的版本 |
| ARM64 | `TinyPin-1.0-arm64-setup.exe` | 仅 ARM64 系统 | 适用于 ARM64 设备 |

## 命令行参数机制

TinyPin 的 Inno Setup 脚本支持通过命令行参数指定平台和可执行文件路径：

### 支持的参数

- `/Dp=平台标识`：指定目标平台（x64、Win32、arm64）
- `/Ds="可执行文件路径"`：指定可执行文件的相对路径

### 支持的平台

| 平台标识 | 描述 | 输出文件名 | 系统要求 |
|---------|------|-----------|----------|
| `x64` | 64位平台 | `TinyPin-1.0-x64-setup.exe` | 仅64位系统 |
| `Win32` | 32位平台 | `TinyPin-1.0-Win32-setup.exe` | 32位和64位系统 |
| `arm64` | ARM64平台 | `TinyPin-1.0-arm64-setup.exe` | 仅ARM64系统 |

### 使用方法

###### 默认行为

如果未指定命令行参数，脚本将使用默认值：
- `p = "x64"`
- `s = "..\..\build\Release\x64\TinyPin.exe"`

```bash
# 使用默认参数（等同于 x64 平台）
ISCC.exe TinyPin.iss
```

```bash
# 构建 x64 平台
ISCC.exe TinyPin.iss /Dp=x64 /Ds="..\..\build\Release\x64\TinyPin.exe"

# 构建 Win32 平台  
ISCC.exe TinyPin.iss /Dp=Win32 /Ds="..\..\build\Release\Win32\TinyPin.exe"

# 构建 ARM64 平台
ISCC.exe TinyPin.iss /Dp=arm64 /Ds="..\..\build\Release\arm64\TinyPin.exe"
```

## 手动构建单个平台

### 1. 编译项目
```bash
# x64 平台
msbuild TinyPin.sln /p:Configuration=Release /p:Platform=x64

# Win32 平台
msbuild TinyPin.sln /p:Configuration=Release /p:Platform=Win32

# ARM64 平台
msbuild TinyPin.sln /p:Configuration=Release /p:Platform=ARM64
```

### 2. 生成安装包
```bash
cd tools\Scripts
ISCC.exe TinyPin.iss
```

## 自动构建所有平台

### 使用批处理脚本（Windows）
```bash
cd tools\Scripts
build.bat
```

### 使用 PowerShell 脚本（推荐）
```powershell
cd tools\Scripts
.\build.ps1
```

#### PowerShell 脚本参数
```powershell
# 构建指定平台
.\build.ps1 -Platforms @("x64", "Win32")

# 使用 Debug 配置
.\build.ps1 -Configuration Debug

# 只构建 x64 平台
.\build.ps1 -Platforms @("x64")
```

## 架构要求自动设置

脚本会根据检测到的平台自动设置以下架构要求：

### x64 平台
```ini
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
```
- 只能在 64 位系统上安装
- 以 64 位模式运行

### Win32 平台
```ini
ArchitecturesAllowed=x86 x64compatible
ArchitecturesInstallIn64BitMode=
```
- 可以在 32 位和 64 位系统上安装
- 在 64 位系统上以 32 位模式运行

### ARM64 平台
```ini
ArchitecturesAllowed=arm64
ArchitecturesInstallIn64BitMode=arm64
```
- 只能在 ARM64 系统上安装
- 以 64 位模式运行

## 发布流程建议

1. **开发阶段**：主要使用 x64 平台进行开发和测试
2. **测试阶段**：构建 Win32 版本进行兼容性测试
3. **发布阶段**：构建所有支持的平台版本

```bash
# 完整发布流程
.\build.ps1
```

## 故障排除

### 常见问题

1. **找不到 MSBuild**
   - 确保已安装 Visual Studio 或 Build Tools
   - 在 Developer Command Prompt 中运行

2. **找不到 Inno Setup**
   - 项目已内置 Inno Setup 编译器，位于 `tools\InnoSetup6\ISCC.exe`
   - 如果使用系统安装的版本，检查安装路径是否为 `C:\Program Files\Inno Setup 6\`

3. **ARM64 编译失败**
   - 确保 Visual Studio 已安装 ARM64 构建工具
   - 某些第三方库可能不支持 ARM64

4. **权限错误**
   - 以管理员身份运行 PowerShell
   - 或者设置执行策略：`Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser`

### 调试技巧

1. **查看详细编译输出**
   ```bash
   msbuild TinyPin.sln /p:Configuration=Release /p:Platform=x64 /verbosity:detailed
   ```

2. **检查生成的文件**
   ```bash
   dir build\Release\*\TinyPin.exe
   ```

3. **手动运行 Inno Setup**
   ```bash
   "C:\Program Files\Inno Setup 6\ISCC.exe" TinyPin.iss
   ```

## 文件结构

```
tools/Scripts/
├── TinyPin.iss                 # 主安装脚本
├── build.bat                   # 批处理构建脚本
├── build.ps1                   # PowerShell 构建脚本
├── README.md                   # 本说明文档
└── TinyPin-1.0-*-setup.exe     # 生成的安装包
```