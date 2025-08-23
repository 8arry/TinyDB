# TinyDB Clean Build Script
# 这个脚本会清理并重新构建整个项目

Write-Host "=== TinyDB Clean Build ===" -ForegroundColor Green

# 切换到项目根目录
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

# 删除旧的build目录
if (Test-Path "build") {
    Write-Host "Cleaning old build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force build
}

# 创建新的build目录
Write-Host "Creating build directory..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path "build" | Out-Null
Set-Location build

# 配置CMake
Write-Host "Configuring with CMake..." -ForegroundColor Yellow
cmake -G "Ninja" ..

# 构建项目
Write-Host "Building project..." -ForegroundColor Yellow
cmake --build .

# 运行测试
Write-Host "Running tests..." -ForegroundColor Yellow
ctest --output-on-failure

Write-Host "=== Build Complete ===" -ForegroundColor Green
Write-Host "Executables are in: build/build/" -ForegroundColor Cyan
