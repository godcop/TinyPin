# GitHub Actions 工作流说明

本项目使用 GitHub Actions 实现自动化构建、测试和发布流程。

## 工作流概述

### 1. 持续集成 (CI) - `ci.yml`

**触发条件**: 
- 推送到 `main` 分支
- 向 `main` 分支提交 Pull Request

**功能**:
- 多平台构建测试 (x64, Win32, ARM64)
- 代码质量检查 (编码格式、项目结构)
- 构建产物上传 (保留7天)

**状态检查**:
- ✅ 所有平台构建成功
- ✅ 代码格式符合规范 (UTF-8 LF 无BOM)
- ✅ 项目结构完整

### 2. 自动发布 - `release.yml`

**触发条件**:
- 推送到 `release` 分支
- 向 `release` 分支提交 Pull Request (仅构建测试)

**功能**:
- 自动构建所有平台的 Release 版本
- 生成多平台安装包
- 自动创建 GitHub Release
- 上传安装包到 Releases

**版本号规则**:
- 优先使用 Git 标签 (格式: `v1.0.0`)
- 如无标签则使用日期版本 (格式: `1.0.yyyyMMdd`)

### 3. 手动发布 - `manual-release.yml`

**触发条件**: 
- 手动触发 (GitHub Actions 页面)

**功能**:
- 指定版本号手动发布
- 支持预发布版本标记
- 完整的构建和发布流程

## 使用流程

### 日常开发流程

1. **开发阶段**: 在 `main` 分支进行开发
   ```bash
   git checkout main
   git add .
   git commit -m "feat: 添加新功能"
   git push origin main
   ```
   - 自动触发 CI 检查
   - 确保代码质量和构建成功

2. **准备发布**: 将代码合并到 `release` 分支
   ```bash
   git checkout release
   git merge main
   git push origin release
   ```
   - 自动触发构建和发布流程
   - 生成安装包并发布到 Releases

### 版本管理

#### 自动版本 (推荐)
使用 Git 标签指定版本号：
```bash
git tag v1.0.1
git push origin v1.0.1
git checkout release
git merge main
git push origin release
```

#### 手动发布
1. 访问 GitHub Actions 页面
2. 选择 "手动发布" 工作流
3. 点击 "Run workflow"
4. 输入版本号 (如: `1.0.1`)
5. 选择是否为预发布版本

## 构建产物

### 安装包命名规则
- `TinyPin-{版本号}-x64-setup.exe` - 64位系统
- `TinyPin-{版本号}-Win32-setup.exe` - 32位/64位系统通用
- `TinyPin-{版本号}-arm64-setup.exe` - ARM64系统

### 系统要求
- **x64**: 仅64位Windows系统
- **Win32**: 32位和64位Windows系统
- **ARM64**: 仅ARM64 Windows系统
- **最低版本**: Windows 7 SP1

## 故障排除

### 常见问题

1. **构建失败**
   - 检查代码编译错误
   - 确保所有依赖文件存在
   - 验证项目配置正确

2. **安装包创建失败**
   - 检查 Inno Setup 脚本语法
   - 确保可执行文件路径正确
   - 验证资源文件完整

3. **发布失败**
   - 检查 GitHub Token 权限
   - 确保版本号格式正确
   - 验证分支权限设置

### 调试方法

1. **查看工作流日志**
   - GitHub → Actions → 选择失败的工作流
   - 展开失败的步骤查看详细日志

2. **本地测试构建**
   ```bash
   # 测试编译
   msbuild TinyPin.sln /p:Configuration=Release /p:Platform=x64
   
   # 测试安装包创建
   tools\Scripts\build.bat
   ```

3. **验证文件格式**
   ```powershell
   # 检查文件编码
   Get-Content file.cpp -Encoding Byte | Select-Object -First 3
   
   # 检查行尾格式
   (Get-Content file.cpp -Raw) -match "`r`n"
   ```

## 配置说明

### 环境变量
- `BUILD_CONFIGURATION`: 构建配置 (Debug/Release)
- `PROJECT_NAME`: 项目名称

### 密钥要求
- `GITHUB_TOKEN`: 自动提供，用于创建 Releases

### 权限设置
确保 GitHub Actions 具有以下权限：
- Contents: Write (创建 Releases)
- Actions: Write (运行工作流)

## 最佳实践

1. **代码规范**
   - 使用 UTF-8 LF 无BOM 编码
   - 遵循项目代码风格
   - 添加适当的中文注释

2. **版本管理**
   - 使用语义化版本号 (x.y.z)
   - 重要版本使用 Git 标签
   - 预发布版本标记清楚

3. **发布流程**
   - main 分支用于日常开发
   - release 分支用于正式发布
   - 充分测试后再合并到 release

4. **文档维护**
   - 及时更新 README.md
   - 记录重要变更
   - 维护发布说明