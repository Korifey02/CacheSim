<#
.SYNOPSIS
    Запускает все тесты CacheSim и сравнивает вывод с эталонными результатами.
.DESCRIPTION
    Для каждого .c файла в tests/ ищет соответствующий .expected файл в tests/expected/,
    запускает CacheSim, фильтрует строку "Time is ..." и сравнивает остаток с эталоном.
.EXAMPLE
    .\run_tests.ps1
    .\run_tests.ps1 -SkipBuild
    .\run_tests.ps1 -TestFilter "matmul"
#>

param(
    [switch]$SkipBuild,
    [string]$TestFilter = ""
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$exe = Join-Path $projectRoot "CacheSim.exe"
$testsDir = Join-Path $projectRoot "tests"
$expectedDir = Join-Path $testsDir "expected"
$cmake = "C:\Program Files\JetBrains\CLion 2025.1.3\bin\cmake\win\x64\bin\cmake.exe"

# --- Сборка ---
if (-not $SkipBuild) {
    Write-Host "=== Building CacheSim ===" -ForegroundColor Cyan
    & $cmake --build $projectRoot --target CacheSim -j 6 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "BUILD FAILED" -ForegroundColor Red
        exit 1
    }
    Write-Host ""
}

if (-not (Test-Path $exe)) {
    Write-Host "ERROR: $exe not found. Build the project first." -ForegroundColor Red
    exit 1
}

# --- Тесты ---
$testFiles = Get-ChildItem -Path $testsDir -Filter "*.c" | Sort-Object Name
if ($TestFilter -ne "") {
    $testFiles = $testFiles | Where-Object { $_.BaseName -like "*$TestFilter*" }
}

$totalTests = 0
$passedTests = 0
$failedTests = @()

foreach ($testFile in $testFiles) {
    $testName = $testFile.BaseName
    $expectedFile = Join-Path $expectedDir "$testName.expected"
    $totalTests++

    Write-Host -NoNewline "  $testName ... "

    if (-not (Test-Path $expectedFile)) {
        Write-Host "SKIP (no .expected file)" -ForegroundColor Yellow
        continue
    }

    # Запуск теста
    try {
        $rawOutput = & $exe $testFile.FullName 2>&1 | Out-String
    }
    catch {
        Write-Host "CRASH" -ForegroundColor Red
        $failedTests += "$testName (crashed)"
        continue
    }

    # Фильтруем строку с временем и пустые строки в начале/конце
    $actualLines = ($rawOutput -split "`r?`n") |
        Where-Object { $_ -notmatch '^\s*Time is' } |
        Where-Object { $_ -notmatch '^\s*$' }

    $expectedLines = (Get-Content $expectedFile -Raw) -split "`r?`n" |
        Where-Object { $_ -notmatch '^\s*$' }

    # Сравнение
    $actual = ($actualLines -join "`n").Trim()
    $expected = ($expectedLines -join "`n").Trim()

    if ($actual -eq $expected) {
        Write-Host "PASSED" -ForegroundColor Green
        $passedTests++
    }
    else {
        Write-Host "FAILED" -ForegroundColor Red
        $failedTests += $testName

        # Показываем различия
        $maxLines = [Math]::Max($actualLines.Count, $expectedLines.Count)
        for ($i = 0; $i -lt $maxLines; $i++) {
            $a = if ($i -lt $actualLines.Count) { $actualLines[$i] } else { "<missing>" }
            $e = if ($i -lt $expectedLines.Count) { $expectedLines[$i] } else { "<missing>" }
            if ($a -ne $e) {
                Write-Host "    line $($i+1):" -ForegroundColor DarkGray
                Write-Host "      expected: $e" -ForegroundColor DarkGreen
                Write-Host "      actual:   $a" -ForegroundColor DarkRed
            }
        }
    }
}

# --- Итоги ---
Write-Host ""
Write-Host "=== Results: $passedTests/$totalTests passed ===" -ForegroundColor $(if ($failedTests.Count -eq 0) { "Green" } else { "Red" })
if ($failedTests.Count -gt 0) {
    Write-Host "Failed tests:" -ForegroundColor Red
    foreach ($f in $failedTests) {
        Write-Host "  - $f" -ForegroundColor Red
    }
    exit 1
}
exit 0
