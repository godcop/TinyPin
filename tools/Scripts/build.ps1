# 微钉 TinyPin 多平台安装包构建脚本 (PowerShell)
# 支持 x64、Win32、ARM64 平台的自动化构建

param(
    [string]$Configuration = "Release",
    [switch]$SkipBuild = $false,
    [switch]$Verbose = $false
)

# 设置控制台编码为 GBK
[Console]::OutputEncoding = [System.Text.Encoding]::GetEncoding("GBK")

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "微钉 TinyPin 多平台安装包构建脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 定义路径和文件
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$SolutionFile = Join-Path $ProjectRoot "TinyPin.sln"
$InstallerScript = "TinyPin.iss"
$InstallerDir = Join-Path $ProjectRoot "tools\Scripts"

# 定义支持的平台
$Platforms = @("x64", "Win32", "ARM64")

# 构建结果统计
$BuildResults = @()

function Write-Step {
    param([string]$Message, [string]$Color = "Yellow")
    Write-Host $Message -ForegroundColor $Color
}

function Write-Success {
    param([string]$Message)
    Write-Host "[√] $Message" -ForegroundColor Green
}

function Write-Error {
    param([string]$Message)
    Write-Host "[×] $Message" -ForegroundColor Red
}

function Test-Command {
    param([string]$Command)
    try {
        Get-Command $Command -ErrorAction Stop | Out-Null
        return $true
    }
    catch {
        return $false
    }
}

# 检查必要工具
Write-Step "检查构建环境..."

$MSBuildPath = Get-Command "msbuild.exe" -ErrorAction SilentlyContinue
if (-not $MSBuildPath) {
    Write-Error "找不到 MSBuild，请确保已安装 Visual Studio 或 Build Tools"
    exit 1
}

$ISCCPath = Join-Path $ProjectRoot "tools\InnoSetup6\ISCC.exe"
if (-not (Test-Path $ISCCPath)) {
    Write-Error "找不到 Inno Setup 编译器: $ISCCPath"
    Write-Error "请确保 Inno Setup 编译器已放置在项目的 tools\InnoSetup6\ 目录中"
    exit 1
}

Write-Success "构建环境检查通过"
Write-Host ""

# 开始构建各平台
$TotalPlatforms = $Platforms.Count
$CurrentPlatform = 0

foreach ($Platform in $Platforms) {
    $CurrentPlatform++
    Write-Step "[$CurrentPlatform/$TotalPlatforms] 构建 $Platform 平台..." "Cyan"
    
    $BuildSuccess = $true
    $ErrorMessage = ""
    
    try {
        # 编译项目
        if (-not $SkipBuild) {
            Write-Host "  编译 $Platform 版本..."
            Set-Location $ProjectRoot
            
            $OutDir = Join-Path $ProjectRoot "build\compile\$Configuration\$Platform\"
            $BuildArgs = @(
                $SolutionFile,
                "/p:Configuration=$Configuration",
                "/p:Platform=$Platform",
                "/p:OutDir=$OutDir"
            )
            
            if (-not $Verbose) {
                $BuildArgs += "/verbosity:minimal"
            }
            
            $BuildResult = & msbuild @BuildArgs
            if ($LASTEXITCODE -ne 0) {
                throw "编译失败，退出代码: $LASTEXITCODE"
            }
            
            Write-Success "编译完成"
        }
        
        # 检查可执行文件是否存在
        $ExePath = Join-Path $ProjectRoot "build\compile\$Configuration\$Platform\TinyPin.exe"
        if (-not (Test-Path $ExePath)) {
            throw "找不到编译输出: $ExePath"
        }
        
        # 打包安装程序
        Write-Host "  打包 $Platform 安装包..."
        Set-Location $InstallerDir
        
        $SourcePath = Join-Path $ProjectRoot "build\compile\$Configuration\$Platform\TinyPin.exe"
        $OutputPath = Join-Path $ProjectRoot "build\installer"
        $PackageResult = & $ISCCPath $InstallerScript "/Dp=$Platform" "/Ds=$SourcePath" "/Do=$OutputPath"
        if ($LASTEXITCODE -ne 0) {
            throw "打包失败，退出代码: $LASTEXITCODE"
        }
        
        # 检查生成的安装包
        $SetupFile = Join-Path $OutputPath "TinyPin-1.0-$Platform-setup.exe"
        if (Test-Path $SetupFile) {
            $FileSize = [math]::Round((Get-Item $SetupFile).Length / 1MB, 2)
            $SetupFileName = Split-Path $SetupFile -Leaf
            Write-Host "  [√] $Platform 安装包生成成功: $SetupFileName ($FileSize MB)" -ForegroundColor Green
            $BuildSuccess = $true
        } else {
            throw "找不到生成的安装包: $SetupFile"
        }
        
        Write-Success "$Platform 平台构建完成"
    }
    catch {
        $BuildSuccess = $false
        $ErrorMessage = $_.Exception.Message
        Write-Error "$Platform 平台构建失败: $ErrorMessage"
    }
    
    # 记录构建结果
    $BuildResults += @{
        Platform = $Platform
        Success = $BuildSuccess
        Error = $ErrorMessage
    }
    
    Write-Host ""
}

# 显示构建摘要
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "构建摘要" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$SuccessCount = 0
$FailureCount = 0

foreach ($Result in $BuildResults) {
    if ($Result.Success) {
        Write-Success "$($Result.Platform) 平台构建成功"
        $SuccessCount++
    }
    else {
        Write-Error "$($Result.Platform) 平台构建失败: $($Result.Error)"
        $FailureCount++
    }
}

Write-Host ""
Write-Host "成功: $SuccessCount 个平台" -ForegroundColor Green
Write-Host "失败: $FailureCount 个平台" -ForegroundColor Red

# 显示生成的安装包
if ($SuccessCount -gt 0) {
    Write-Host ""
    Write-Host "生成的安装包:" -ForegroundColor Yellow
    $InstallerOutputDir = Join-Path $ProjectRoot "build\installer"
    $SetupFiles = Get-ChildItem -Path $InstallerOutputDir -Name "TinyPin-1.0-*-setup.exe" -ErrorAction SilentlyContinue
    if ($SetupFiles) {
        foreach ($File in $SetupFiles) {
            $FilePath = Join-Path $InstallerOutputDir $File
            $FileInfo = Get-Item $FilePath
            $SizeMB = [math]::Round($FileInfo.Length / 1MB, 2)
            Write-Host "  $File ($SizeMB MB)" -ForegroundColor White
        }
    }
    else {
        Write-Host "  未找到安装包文件" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "编译文件保存在: $(Join-Path $ProjectRoot 'build\compile')" -ForegroundColor Green
Write-Host "安装包保存在: $(Join-Path $ProjectRoot 'build\installer')" -ForegroundColor Green
Write-Host "构建完成时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor Gray

# 设置退出代码
if ($FailureCount -gt 0) {
    exit 1
}
else {
    exit 0
}