<#
.SYNOPSIS
    Setup script for the Bobbycar-Steering cross-platform ESP32 project.
    Initialises git submodules (ESP-IDF, LVGL), installs ESP-IDF tools,
    and prepares the development environment.

.DESCRIPTION
    Run this once before building:
        powershell -ExecutionPolicy Bypass -File .\setup.ps1

    The script will:
        1. Install Python (via winget) if not found
        2. Initialise git submodules (ESP-IDF, LVGL)
        3. Run ESP-IDF install script
        4. Install Python dependencies
#>

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

# -----------------------------------------------------------------------
# Helper: refresh PATH from system + user environment variables
# -----------------------------------------------------------------------
function Refresh-EnvPath {
    $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "Machine") + ";" +
                [System.Environment]::GetEnvironmentVariable("PATH", "User") + ";" + $env:PATH
}

# -----------------------------------------------------------------------
# Helper: test if a command is a real executable (not Windows App Alias stub)
# -----------------------------------------------------------------------
function Test-RealCommand {
    param([string]$Name)
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -eq $cmd) { return $false }
    if ($cmd.Source -like "*WindowsApps*") { return $false }
    return $true
}

# =======================================================================
# 1. Python
# =======================================================================
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  1. Python"
Write-Host "============================================" -ForegroundColor Cyan

$pythonOk = Test-RealCommand "python"
if (-not $pythonOk) {
    Write-Host "[INST] Installing Python 3.13 via winget ..."
    winget install --id Python.Python.3.13 --accept-source-agreements --accept-package-agreements --silent
    Refresh-EnvPath
    $pyDirs = @(
        "$env:LOCALAPPDATA\Programs\Python\Python313",
        "$env:LOCALAPPDATA\Programs\Python\Python312",
        "C:\Python313",
        "C:\Python312"
    )
    foreach ($d in $pyDirs) {
        if (Test-Path "$d\python.exe") {
            $env:PATH = "$d;$d\Scripts;" + $env:PATH
            break
        }
    }
    $pythonOk = Test-RealCommand "python"
}
if ($pythonOk) {
    $v = & python --version 2>&1
    Write-Host "[OK]   Python found: $v" -ForegroundColor Green
} else {
    Write-Host "[WARN] Python still not on PATH. You may need to restart your terminal." -ForegroundColor Yellow
    Write-Host "       Manual install: https://www.python.org/downloads/"
}

# =======================================================================
# 2. Git Submodules
# =======================================================================
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  2. Git Submodules (ESP-IDF + LVGL)"
Write-Host "============================================" -ForegroundColor Cyan

if (Test-Path ".gitmodules") {
    Write-Host "[GIT]  Initialising submodules ..."
    git submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) { throw "git submodule update failed" }
    Write-Host "[OK]   Submodules initialised." -ForegroundColor Green
} else {
    Write-Host "[WARN] No .gitmodules file found — submodules may not be configured." -ForegroundColor Yellow
}

# =======================================================================
# 3. ESP-IDF Setup
# =======================================================================
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  3. ESP-IDF Setup"
Write-Host "============================================" -ForegroundColor Cyan

$idfPath = Join-Path $PSScriptRoot "sdk\esp-idf"
if (Test-Path "$idfPath\install.ps1") {
    Write-Host "[IDF]  Running ESP-IDF install script ..."
    Push-Location $idfPath

    # Install for all three targets
    & .\install.ps1 esp32c3,esp32h2,esp32c5

    Pop-Location
    Write-Host "[OK]   ESP-IDF tools installed." -ForegroundColor Green
} else {
    Write-Host "[WARN] ESP-IDF not found at $idfPath" -ForegroundColor Yellow
    Write-Host "       Run:  git submodule update --init --recursive"
}

# =======================================================================
# 4. Python Dependencies
# =======================================================================
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  4. Python Dependencies"
Write-Host "============================================" -ForegroundColor Cyan

$pythonOk = Test-RealCommand "python"
if ($pythonOk) {
    & python -m pip install --upgrade pip --quiet 2>$null
    & python -m pip install pyserial --quiet 2>$null
    Write-Host "[OK]   Python dependencies installed." -ForegroundColor Green
} else {
    Write-Host "[WARN] Python not found. Run:  pip install pyserial" -ForegroundColor Yellow
}

# =======================================================================
# Done
# =======================================================================
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Setup complete!"
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  # Activate ESP-IDF environment (every new terminal session):"
Write-Host "  sdk\esp-idf\export.ps1"
Write-Host ""
Write-Host "  # Set target and build:"
Write-Host "  idf.py set-target esp32c3"
Write-Host "  idf.py build"
Write-Host ""
Write-Host "  # Flash and monitor:"
Write-Host "  idf.py -p COM<N> flash monitor"
Write-Host ""
Write-Host "NOTE: If tools were just installed you may need to restart your terminal" -ForegroundColor Yellow
Write-Host "      or open a new PowerShell session for PATH changes to take effect." -ForegroundColor Yellow
Write-Host ""
