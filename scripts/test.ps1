# TinyDB Test Runner Script for Windows PowerShell
# Usage: .\scripts\test.ps1

# Set GCC path (adjust if needed)
$gccPath = "C:\Users\8arry\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.MSVCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"
$env:PATH = "$gccPath;" + $env:PATH

Write-Host "🧪 Running TinyDB Tests..." -ForegroundColor Green

# Build tests first if they don't exist
if (-not (Test-Path "build/test_simple.exe")) {
    Write-Host "Tests not found, building first..." -ForegroundColor Yellow
    & .\scripts\build.ps1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Build failed, cannot run tests" -ForegroundColor Red
        exit 1
    }
}

$testsPassed = 0
$testsFailed = 0

Write-Host "`n📋 Test Results:" -ForegroundColor Cyan
Write-Host "=================" -ForegroundColor Cyan

# Run Value class tests
Write-Host "`n🔍 Running Value class tests..." -ForegroundColor Yellow
try {
    $output = & .\build\test_simple.exe
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Value class tests: PASSED" -ForegroundColor Green
        $testsPassed++
        # Show test output
        $output | ForEach-Object {
            Write-Host "   $_" -ForegroundColor Gray
        }
    } else {
        Write-Host "❌ Value class tests: FAILED" -ForegroundColor Red
        $testsFailed++
        $output | ForEach-Object {
            Write-Host "   $_" -ForegroundColor Red
        }
    }
} catch {
    Write-Host "❌ Value class tests: ERROR - $_" -ForegroundColor Red
    $testsFailed++
}

# Summary
Write-Host "`n📊 Test Summary:" -ForegroundColor Cyan
Write-Host "=================" -ForegroundColor Cyan
Write-Host "✅ Passed: $testsPassed" -ForegroundColor Green
Write-Host "❌ Failed: $testsFailed" -ForegroundColor Red
Write-Host "📈 Total:  $($testsPassed + $testsFailed)" -ForegroundColor Cyan

if ($testsFailed -eq 0) {
    Write-Host "`n🎉 All tests passed!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "`n💥 Some tests failed!" -ForegroundColor Red
    exit 1
}