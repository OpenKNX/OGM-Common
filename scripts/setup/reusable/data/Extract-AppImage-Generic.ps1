#!/usr/bin/env pwsh
<#
Open ■
┬────┴  Extract-AppImage-Generic
■ KNX   2026 OpenKNX - Erkan Çolak

FILEPATH: OGM-Common/scripts/setup/reusable/data/Extract-AppImage-Generic.ps1
   ships as: release/data/Extract-AppImage-Generic.ps1

.SYNOPSIS
    Pulls the raw application image out of a released firmware package — without ftc, without any tool
    beyond PowerShell itself.

.DESCRIPTION
    A release ships one file per device, and that file is a PACKAGE: a .uf2 wraps the image in 256-byte
    blocks, a .factory.bin puts a bootloader and a partition table in front of it. Both carry the plain
    image inside them, and this script writes it out as <name>.app.bin.

    You need that image if you build firmware differences yourself, feed another OTA tool, or want to
    checksum what is actually flashed. ftc does the same unwrapping internally; this exists so the files
    are reachable for anyone who does not want to run ftc at all.

    Where the build wrote a <name>.image.txt next to the package, this script reads it: the exact image
    length and, for an ESP bundle, the offset. That matters because a .uf2 pads its last block, so
    unwrapping alone yields a few bytes too many -- and a firmware difference is checked against the
    exact length. Without the facts file the script still works, and says that the length is not certain.

    Nothing is downloaded, nothing is installed, and the source file is never modified.

.PARAMETER Path
    A .uf2 or .factory.bin, or a folder — a folder is searched and every package in it is unwrapped.

.PARAMETER OutDir
    Where to write. Default: next to the source file.

.PARAMETER Force
    Overwrite an existing .app.bin instead of leaving it alone.

.EXAMPLE
    ./Extract-AppImage-Generic.ps1 ../Firmware
    # every package under Firmware/ is unwrapped in place

.EXAMPLE
    ./Extract-AppImage-Generic.ps1 firmware-Device.factory.bin -OutDir ~/Desktop

.NOTES
    AUTHOR : Erkan Çolak
    Runs on Windows PowerShell 5.1 and PowerShell 7+, on Windows, macOS, Linux and Raspberry Pi.

.LINK
    https://wiki.openknx.de
#>

param(
    [Parameter(Position = 0)][string]$Path = ".",
    [string]$Lang = "",
    [string]$OutDir = "",
    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($null -eq (Get-Variable -Name 'IsMacOS' -ErrorAction SilentlyContinue)) {
    $IsMacOS = $false; $IsLinux = $false; $IsWindows = $true
}
if ($IsWindows) { try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch {} }

# ─── Language (default DE, else system locale) ─────────────────────────────────────────────────────
if (-not $Lang) {
    if ($IsWindows) { $sysLang = (Get-Culture).TwoLetterISOLanguageName } else { $sysLang = "$env:LANG" }
    if ($sysLang -match '^en') { $Lang = 'EN' } else { $Lang = 'DE' }
}
$_lang = $Lang.ToUpper()
if ($_lang -ne 'EN') { $_lang = 'DE' }
$_strings = @{
    EN = @{ Way = 'extract'; NotFound = '  not found: {0}'; NoPkg = '  no .uf2 and no .factory.bin found.'
            NoImg = 'no image inside it'; Same = 'already there, identical'
            Differs = 'already there, DIFFERS - overwrite with -Force'
            Wrote = '  {0} image(s) written.'
            What = '  The application image is what runs on the device - no bootloader, no wrapper.' }
    DE = @{ Way = 'auspacken'; NotFound = '  nicht gefunden: {0}'; NoPkg = '  keine .uf2 und keine .factory.bin gefunden.'
            NoImg = 'kein Image darin gefunden'; Same = 'liegt bereits vor, identisch'
            Differs = 'liegt bereits vor, WEICHT AB - mit -Force überschreiben'
            Wrote = '  {0} Image(s) geschrieben.'
            What = '  Das Anwendungsimage ist das, was auf dem Gerät läuft - ohne Bootloader, ohne Verpackung.' }
}
$s = $_strings[$_lang]

# ─── the shared header ─────────────────────────────────────────────────────────────────────────────
# The same header the upload scripts print. A private copy of the logo here is exactly how the three
# of them drifted apart before; this one is not going to start that again.
$_uiPath = Join-Path $PSScriptRoot "OpenKNX-UI-Generic.ps1"
$_haveUi = (Test-Path -PathType Leaf $_uiPath)
if ($_haveUi) { . $_uiPath }
else {
    function OpenKNX_ShowLogo($AddCustomText = $null) {
        Write-Host ""
        Write-Host "Open " -NoNewline
        Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -ForegroundColor Green
        $bar = "$( [char]::ConvertFromUtf32(0x252C) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2534) ) "
        if ($AddCustomText) { Write-Host "$bar $AddCustomText" -ForegroundColor Green }
        else { Write-Host $bar -ForegroundColor Green }
        Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -NoNewline -ForegroundColor Green
        Write-Host " KNX"
        Write-Host ""
    }
}

# The unwrapping itself lives in one place, because three things in a release need it: this script,
# the network upload, and the difference builder.
$imgLib = Join-Path $PSScriptRoot "OpenKNX-Image-Generic.ps1"
if (-not (Test-Path -PathType Leaf $imgLib)) {
    Write-Host ""
    Write-Host "  OpenKNX-Image-Generic.ps1 fehlt neben diesem Skript." -ForegroundColor Red
    Write-Host "  $imgLib" -ForegroundColor DarkGray
    Write-Host ""
    exit 1
}
. $imgLib

# ─── run ──────────────────────────────────────────────────────────────────────────────────────────
if ($_haveUi) {
    $_ui = $script:OpenKNX_UI_Strings[(OpenKNX_UI_Lang $_lang)]
    OpenKNX_ShowTitle -Way $s.Way -Lang $_lang -Kind $_ui.KindPackage
}
else { OpenKNX_ShowLogo "OpenKNX" }

if (-not (Test-Path $Path)) {
    Write-Host ($s.NotFound -f $Path) -ForegroundColor Red
    exit 1
}
$files = @()
if (Test-Path -PathType Container $Path) {
    $files += Get-ChildItem -Path $Path -Include *.uf2, *.factory.bin -File -Recurse |
              ForEach-Object { $_.FullName }
}
else { $files += (Resolve-Path $Path).Path }

if ($files.Count -eq 0) {
    Write-Host $s.NoPkg -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

$done = 0
foreach ($f in ($files | Sort-Object)) {
    $item = Get-Item $f
    $name = $item.Name
    $stem = $name -replace '\.factory\.bin$', '' -replace '\.uf2$', ''
    $dir = $OutDir
    if (-not $dir) { $dir = $item.DirectoryName }
    if (-not (Test-Path -PathType Container $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    $target = Join-Path $dir ($stem + ".app.bin")

    $img = OpenKNX_GetAppImage -Path $item.FullName
    if ($null -eq $img) {
        Write-Host ("  {0,-52} {1}" -f $name, $s.NoImg) -ForegroundColor Yellow
        continue
    }
    if ((Test-Path -PathType Leaf $target) -and -not $Force) {
        # An existing image is not overwritten by default -- but it IS compared, because a mismatch is
        # worth knowing about and costs nothing to check.
        $old = [System.IO.File]::ReadAllBytes($target)
        $same = ($old.Length -eq $img.Length)
        if ($same) {
            for ($i = 0; $i -lt $img.Length; $i++) { if ($old[$i] -ne $img[$i]) { $same = $false; break } }
        }
        if ($same) { Write-Host ("  {0,-52} {1}" -f $name, $s.Same) -ForegroundColor DarkGray }
        else { Write-Host ("  {0,-52} {1}" -f $name, $s.Differs) -ForegroundColor Yellow }
        continue
    }
    [System.IO.File]::WriteAllBytes($target, $img)
    Write-Host ("  {0,-52} {1,10:N0} B  ->  {2}" -f $name, $img.Length, (Split-Path -Leaf $target)) -ForegroundColor Green
    $done++
}

Write-Host ""
if ($done -gt 0) { Write-Host ($s.Wrote -f $done) -ForegroundColor Green }
Write-Host $s.What -ForegroundColor DarkGray
Write-Host ""
