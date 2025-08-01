# 微钉 (TinyPin)

<div align="center">
  <img src="assets/images/TinyPin.png" alt="TinyPin" width="64" height="64">
</div>

一个轻量级的 Windows 窗口置顶工具，让您可以将任何窗口固定在桌面最前端，提高工作效率。

## 📋 项目简介

微钉 (TinyPin) 是一个免费开源的 Windows 应用程序，专为需要多窗口操作的用户设计。它可以将应用程序窗口置顶显示，让您在工作时始终保持重要窗口可见，无需频繁切换窗口。

> 这个项目参考了 DeskPins，在 DeskPins 的基础上作了大量重构。如您更喜欢 DeskPins，或需要在 Windows 7 及以下系统使用，可点击<a href="https://efotinis.neocities.org/deskpins/" target="_blank" style="color:rgb(53, 171, 255); font-weight: bold;">这里</a>前往 DeskPins 官网下载。

### ✨ 主要特性

- **🎯 精准置顶**：支持将窗口固定在最前端（部分系统级窗口不支持）
- **🔥 热键支持**：快速进入图钉模式或切换活动窗口置顶状态
- **🤖 自动图钉**：根据自定义规则自动置顶特定窗口
- **🎨 可视化操作**：直观的图钉图标和拖拽操作
- **🌍 多语言支持**：支持中文、英文、德语、法语、日语
- **⚡ 轻量高效**：占用资源极少，运行稳定
- **🔧 高度可定制**：丰富的配置选项满足不同需求

### 🎯 适用场景

- **开发编程**：保持文档、终端或调试窗口始终可见
- **数据录入**：参考资料窗口置顶，提高录入效率
- **视频会议**：聊天窗口或笔记应用保持在前端
- **学习办公**：重要信息窗口固定显示
- **游戏辅助**：攻略、地图等辅助窗口置顶

## 🚀 快速开始

### 基本使用

1. **启动程序**：运行 TinyPin.exe，程序将在系统托盘中显示图钉图标
2. **进入图钉模式**：
   - 右键点击托盘图标，选择"进入图钉模式"
   - 或使用默认热键 `Ctrl + F11`
3. **置顶窗口**：在图钉模式下，点击要置顶的窗口即可
4. **取消置顶**：再次点击已置顶的窗口上的图钉图标

### 高级功能

#### 🔥 热键操作

- **进入图钉模式**：`Ctrl + F11`（可自定义）
- **切换活动窗口置顶**：`Ctrl + F12`（可自定义）

#### 🤖 自动图钉

1. 打开选项对话框（右键托盘图标 → 选项）
2. 切换到"自动"标签页
3. 添加自动图钉规则：
   - 设置窗口标题匹配规则
   - 设置窗口类名匹配规则
   - 配置延迟时间
4. 启用自动图钉功能

#### ⚙️ 个性化设置

- **图钉图标**：自定义图钉的外观样式
- **跟踪速率**：调整图钉跟随窗口移动的响应速度
- **托盘激活**：设置单击或双击托盘图标的行为
- **开机启动**：设置程序随系统启动
- **界面语言**：选择您偏好的界面语言

## 🛠️ 编译和打包

### 系统要求

- **操作系统**：Windows 7 SP1 或更高版本 (未测试 Windows 7，请自行测试)
- **开发环境**：Visual Studio 2022 (v143 工具集)
- **Windows SDK**：Windows 10 SDK
- **打包工具**：Inno Setup 6.0+ (可选，用于创建安装包)

### 编译步骤

#### 1. 克隆项目

```bash
git clone https://github.com/godcop/TinyPin.git
cd TinyPin
```

#### 2. 编译项目

使用 MSBuild 命令行编译：

```bash
# Debug 版本 (x64)
msbuild TinyPin.sln /p:Configuration=Debug /p:Platform=x64 /p:OutDir="build\compile\Debug\x64\"

# Release 版本 (x64)
msbuild TinyPin.sln /p:Configuration=Release /p:Platform=x64 /p:OutDir="build\compile\Release\x64\"

# 32位版本
msbuild TinyPin.sln /p:Configuration=Release /p:Platform=Win32 /p:OutDir="build\compile\Release\Win32\"

# ARM64版本
msbuild TinyPin.sln /p:Configuration=Release /p:Platform=ARM64 /p:OutDir="build\compile\Release\ARM64\"
```

或者在 Visual Studio 中打开 `TinyPin.sln` 解决方案文件直接编译。

#### 3. 编译输出

编译完成后，可执行文件将生成在：

```
build/compile/Release/x64/TinyPin.exe      # x64版本
build/compile/Release/Win32/TinyPin.exe    # 32位版本
build/compile/Release/ARM64/TinyPin.exe    # ARM64版本
```

### 创建安装包

如果您需要创建安装包进行分发，可以使用 Inno Setup：

#### 1. 安装 Inno Setup

从 [Inno Setup 官网](https://jrsoftware.org/isinfo.php) 下载并安装最新版本。

#### 2. 编译安装包

```bash
cd tools/installer

# 创建 x64 安装包
ISCC.exe TinyPin.iss /Dp=x64 /Ds="..\..\build\compile\Release\x64\TinyPin.exe" /Do="..\..\build\installer"

# 创建 32位 安装包
ISCC.exe TinyPin.iss /Dp=Win32 /Ds="..\..\build\compile\Release\Win32\TinyPin.exe" /Do="..\..\build\installer"

# 创建 ARM64 安装包
ISCC.exe TinyPin.iss /Dp=arm64 /Ds="..\..\build\compile\Release\ARM64\TinyPin.exe" /Do="..\..\build\installer"
```

#### 3. 批量构建

使用提供的批处理脚本一键构建所有平台：

```bash
cd tools/installer
build_all_platforms.bat
```

或使用 PowerShell 脚本（推荐）：

```bash
cd tools/installer
.\build_all_platforms.ps1
```

生成的安装包将保存在 `build/installer/` 目录下：

- `TinyPin-1.0-x64-setup.exe`
- `TinyPin-1.0-Win32-setup.exe`
- `TinyPin-1.0-arm64-setup.exe`

### 项目结构

```
TinyPin/
├── src/                    # 源代码文件
│   ├── core/              # 核心模块
│   ├── ui/                # 用户界面
│   ├── pin/               # 图钉功能
│   ├── options/           # 选项设置
│   ├── platform/          # 平台相关
│   ├── foundation/        # 基础工具
│   ├── graphics/          # 图形处理
│   ├── system/            # 系统功能
│   └── window/            # 窗口管理
├── include/               # 头文件
├── resources/             # 编译资源（图标、清单等）
├── assets/                # 运行时资源（语言包、图片等）
├── tools/installer/       # 安装包构建脚本
└── build/                 # 构建输出目录
    ├── compile/           # 编译产生的文件
    │   ├── Debug/         # Debug版本输出
    │   └── Release/       # Release版本输出
    └── installer/         # 安装包输出目录
```

## ❓ 常见问题

### Q: 为什么某些窗口无法置顶？

**A:** 部分系统级窗口（如任务栏、桌面、任务管理器、某些安全软件窗口）由于系统权限限制无法被置顶。部分 UWP 应用不能在窗口上显示图钉图标，但实际已经置顶。这是正常现象，属于 Windows 系统的安全机制。

### Q: 热键不生效怎么办？

**A:** 请检查以下几点：

- 确认热键没有被其他软件占用
- 在选项对话框中重新设置热键
- 以管理员权限运行程序（某些情况下需要）
- 重启程序后再次尝试

### Q: 程序启动后托盘图标不显示？

**A:** 可能的解决方案：

- 检查系统托盘设置，确保允许显示 TinyPin 图标
- 重启 Windows 资源管理器（explorer.exe）
- 以管理员权限运行程序
- 检查是否被杀毒软件误报并添加信任

### Q: 自动图钉功能不工作？

**A:** 请确认：

- 已在"自动"选项页中启用了自动图钉功能
- 规则设置正确（窗口标题或类名匹配）
- 延迟时间设置合理（建议 500-2000 毫秒）
- 目标程序确实创建了新窗口（如果是后台窗口，则要重启目标程序）

### Q: 图钉图标显示异常或消失？

**A:** 尝试以下解决方法：

- 在选项中重新设置图钉图标
- 检查图钉跟踪速率设置（建议 50-200 毫秒）
- 重启程序
- 确保目标窗口没有被其他程序遮挡

### Q: 程序占用内存过高？

**A:** TinyPin 设计为轻量级应用，正常内存占用应在 10-30MB 范围内。如果占用过高：

- 检查是否有过多的自动图钉规则
- 降低图钉跟踪速率
- 重启程序清理内存
- 检查是否有内存泄漏并反馈给开发者

### Q: 支持哪些 Windows 版本？

**A:**

- **官方支持**：Windows 10 和 Windows 11
- **理论支持**：Windows 7 SP1 及以上版本（未充分测试）
- **架构支持**：x86、x64、ARM64

### Q: 程序被杀毒软件误报怎么办？

**A:** 这是开源软件常见问题：

- 将程序添加到杀毒软件的信任列表
- 从官方 GitHub 仓库下载确保安全性
- 查看 <a href="docs/AntivirusIssues.md" style="color: #ff4757; font-weight: bold;">软件报毒问题说明</a>
- 可以自行编译源代码确保安全

## 📄 许可证

本项目采用 MIT 许可证开源。详细信息请查看 [LICENSE.txt](LICENSE.txt) 文件。

## 🤝 贡献

欢迎提交 Issue 和 Pull Request 来帮助改进这个项目！

## 📞 联系方式

- **项目主页**：https://github.com/godcop/TinyPin
- **问题反馈**：https://github.com/godcop/TinyPin/issues
- **作者**：godcop

---

**微钉 (TinyPin)** - 让窗口管理更简单高效！
