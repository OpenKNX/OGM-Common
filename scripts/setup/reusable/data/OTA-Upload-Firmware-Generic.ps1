#!/usr/bin/env pwsh
<#
Open ■
┬────┴  OTA-Upload-Firmware-Generic.ps1
■ KNX   2025 OpenKNX

OTA firmware update for IP-capable OpenKNX devices via espota.
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

# Find espota binary (cross-platform: ~/bin/, PATH, pip-installed)
function Find-Espota {
    # 1. ~/bin/
    foreach ($name in @('espota', 'espota.exe', 'espota.py')) {
        $p = Join-Path ([System.IO.Path]::GetFullPath((Join-Path $HOME 'bin'))) $name
        if (Test-Path -PathType Leaf $p) { return $p }
    }
    # 2. PATH (pip install esptool installs espota on all platforms)
    foreach ($name in @('espota', 'espota.exe', 'espota.py')) {
        if (Get-Command $name -ErrorAction SilentlyContinue) { return $name }
    }
    return $null
}

OpenKNX_ShowLogo 'OTA Firmware Update'

$espota = Find-Espota
if (-not $espota) {
    Write-Host ""
    Write-Host "  espota nicht gefunden." -ForegroundColor Red
    Write-Host ""
    Write-Host "  Option 1 — OpenKNX-Tools (empfohlen, Windows/macOS/Linux):" -ForegroundColor Yellow
    Write-Host "    https://github.com/OpenKNX/OpenKNXproducer/releases" -ForegroundColor White
    Write-Host "    ZIP entpacken → Install-OpenKNX-Tools.ps1 ausfuehren" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "  Option 2 — pip:" -ForegroundColor Yellow
    Write-Host "    pip install esptool" -ForegroundColor White
    Write-Host ""
    Read-Host '  Druecke Enter zum Beenden'
    exit 1
}

$firmwareName = (Resolve-Path "./$($args[0])").Path
Write-Host "  Firmware : $firmwareName" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Hinweis: OTA muss auf dem Geraet ggf. durch den Programmiermodus aktiviert werden." -ForegroundColor DarkYellow
Write-Host ""

# IP input with validation
$validIp = $false
while (-not $validIp) {
    $ipAddress = (Read-Host "  IP-Adresse des Update-Ziels").Trim()
    $validIp   = [System.Net.IPAddress]::TryParse($ipAddress, [ref]$null)
    if (-not $validIp) { Write-Host "  Ungueltige IP-Adresse. Bitte erneut eingeben." -ForegroundColor Red }
}

Write-Host ""
Write-Host "  $espota -i $ipAddress -r $($args[1]) -f $firmwareName" -ForegroundColor DarkGray
Write-Host ""

& $espota -i $ipAddress -r $args[1] -f $firmwareName

Write-Host ""
Read-Host '  Druecke Enter zum Beenden'
