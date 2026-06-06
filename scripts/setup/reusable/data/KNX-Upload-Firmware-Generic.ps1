#!/usr/bin/env pwsh
<#
Open ■
┬────┴  KNX-Upload-Firmware-Generic
■ KNX   2025 OpenKNX

Uploads firmware to an OpenKNX device via KNX bus using KnxFileTransferClient.
Cross-platform: Windows, macOS, Linux.
#>

# Platform detection
if ($null -eq (Get-Variable -Name 'IsMacOS' -ErrorAction SilentlyContinue)) {
    $IsMacOS = $false; $IsLinux = $false; $IsWindows = $true
}

function OpenKNX_ShowLogo($AddCustomText = $null) {
    Write-Host ""
    Write-Host "Open " -NoNewline
    Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -ForegroundColor Green
    $u = "$( [char]::ConvertFromUtf32(0x252C) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2534) ) "
    if ($AddCustomText) { Write-Host "$u $AddCustomText" -ForegroundColor Green }
    else                { Write-Host $u                  -ForegroundColor Green }
    Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -NoNewline -ForegroundColor Green
    Write-Host " KNX"
    Write-Host ""
}

# Find KnxFileTransferClient binary (cross-platform)
function Find-KnxClient {
    # 1. ~/bin/ — preferred install location
    foreach ($name in @('KnxFileTransferClient', 'KnxFileTransferClient.exe')) {
        $p = Join-Path ([System.IO.Path]::GetFullPath((Join-Path $HOME 'bin'))) $name
        if (Test-Path -PathType Leaf $p) { return $p }
    }
    # 2. PATH
    foreach ($name in @('KnxFileTransferClient', 'KnxFileTransferClient.exe')) {
        if (Get-Command $name -ErrorAction SilentlyContinue) { return $name }
    }
    return $null
}

# Check version (>= required)
function Test-KnxClientVersion([string]$bin, [string]$required) {
    try {
        $out = & $bin version 2>&1 | Out-String
        if ($out -match 'Version Client[:\s]+([\d.]+)') {
            return ([System.Version]$Matches[1]) -ge ([System.Version]$required)
        }
    } catch {}
    return $false
}

$checkVersion = '0.2.8'
OpenKNX_ShowLogo 'KNX Bus Firmware Upload'

$bin = Find-KnxClient
$toolsOk = $bin -and (Test-KnxClientVersion $bin $checkVersion)

if (-not $toolsOk) {
    Write-Host ""
    Write-Host "  KnxFileTransferClient nicht gefunden oder veraltet (mind. v$checkVersion)." -ForegroundColor Red
    Write-Host ""
    Write-Host "  Download:  https://github.com/OpenKNX/KnxFileTransferClient/releases" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Installation (Windows, macOS, Linux):" -ForegroundColor White
    Write-Host "    1. ZIP entpacken" -ForegroundColor White
    Write-Host "    2. Install-OpenKNX-Tools.ps1 mit PowerShell ausfuehren" -ForegroundColor White
    Write-Host "       (kopiert die Tools automatisch nach ~/bin/)" -ForegroundColor DarkGray
    Write-Host ""
    Read-Host '  Druecke Enter zum Beenden'
    exit 1
}

$firmwareName = (Resolve-Path "./$($args[0])").Path
Write-Host "  Firmware : $firmwareName" -ForegroundColor Cyan
Write-Host ""

# KnxFileTransferClient.ps1 wrapper takes precedence (some installs ship it)
$ps1Wrapper = Join-Path ([System.IO.Path]::GetFullPath((Join-Path $HOME 'bin'))) 'KnxFileTransferClient.ps1'
if (Test-Path -PathType Leaf $ps1Wrapper) {
    & $ps1Wrapper "$firmwareName"
} else {
    & $bin fwupdate --connect Search "$firmwareName"
}

Read-Host '  Druecke Enter zum Beenden'
