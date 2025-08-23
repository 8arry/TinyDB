# Basic code formatting script
# This script performs basic formatting to simulate clang-format

Write-Host "Applying basic C++ code formatting..."

# Get all C++ source files
$sourceFiles = Get-ChildItem -Path . -Include "*.cpp", "*.hpp", "*.h" -Recurse -Exclude "build/*", "tests/catch.hpp"

foreach ($file in $sourceFiles) {
    Write-Host "Processing: $($file.Name)"
    
    # Read file content
    $content = Get-Content $file.FullName -Raw
    
    # Basic formatting fixes
    # Remove trailing whitespace
    $content = $content -replace '\s+$', ''
    
    # Ensure files end with newline
    if (-not $content.EndsWith("`n")) {
        $content = $content + "`n"
    }
    
    # Normalize line endings
    $content = $content -replace '\r\n', "`n"
    $content = $content -replace '\r', "`n"
    
    # Write back to file with UTF-8 encoding (no BOM)
    [System.IO.File]::WriteAllText($file.FullName, $content, [System.Text.UTF8Encoding]::new($false))
}

Write-Host "Basic formatting complete!"
Write-Host ""
Write-Host "Note: For full clang-format compliance, please install clang-format"
Write-Host "and run: clang-format -i (find . -name '*.cpp' -o -name '*.hpp')"
