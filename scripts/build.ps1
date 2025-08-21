# TinyDB Build Script for Windows PowerShell
# Usage: .\scripts\build.ps1 [Debug|Release]

param(
    [string]$BuildType = "Debug",
    [switch]$Clean = $false
)

# Set GCC path (adjust if needed)
$gccPath = "C:\Users\8arry\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.MSVCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"
$env:PATH = "$gccPath;" + $env:PATH

Write-Host "Building TinyDB..." -ForegroundColor Green
Write-Host "Build Type: $BuildType" -ForegroundColor Yellow

# Create build directory if it doesn't exist
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build"
}

# Clean build directory if requested
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item "build\*" -Recurse -Force
}

# Set compiler flags based on build type
$commonFlags = "-std=c++23 -Wall -Wextra -I."
if ($BuildType -eq "Debug") {
    $flags = "$commonFlags -g -O0 -DDEBUG"
} else {
    $flags = "$commonFlags -O3 -DNDEBUG"
}

Write-Host "Compiler flags: $flags" -ForegroundColor Cyan

try {
    # Build the core library test
    Write-Host "Building tests..." -ForegroundColor Yellow
    g++ $flags.Split(' ') libcore/database/value.cpp tests/test_simple.cpp -o build/test_simple.exe
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Build successful!" -ForegroundColor Green
        Write-Host "Executables created in build/ directory" -ForegroundColor Green
        
        # List created files
        Get-ChildItem "build" -Filter "*.exe" | ForEach-Object {
            Write-Host "  📁 $($_.Name)" -ForegroundColor Cyan
        }
    } else {
        throw "Compilation failed"
    }
} catch {
    Write-Host "❌ Build failed: $_" -ForegroundColor Red
    exit 1
}

Write-Host "🎉 Build completed successfully!" -ForegroundColor Green