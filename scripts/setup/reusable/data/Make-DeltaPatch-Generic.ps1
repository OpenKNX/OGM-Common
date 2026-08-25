#!/usr/bin/env pwsh
<#
Open ■
┬────┴  Make-DeltaPatch-Generic
■ KNX   2026 OpenKNX - Erkan Çolak

FILEPATH: OGM-Common/scripts/setup/reusable/data/Make-DeltaPatch-Generic.ps1
   ships as: release/data/Make-DeltaPatch-Generic.ps1

.SYNOPSIS
    Builds delta patches between two OpenKNX releases — one per device, in one call.

.DESCRIPTION
    A delta update sends only the difference between the firmware a device is running and the one it
    should run. Building that difference needs BOTH images, so it is done here, on a PC, and not on the
    device: point this script at the release the devices currently have and at the release they should
    get, and it writes one patch per device.

    A release ships one file per device -- the .uf2 or the .factory.bin -- and the raw application image
    is derived from it, using the <name>.image.txt the build wrote alongside. Older releases that still
    carry an <firmware>.app.bin are used as they are.

    A patch only fits the release it was built from. The device checks that before accepting anything,
    so a patch handed to the wrong device is refused rather than misapplied — but naming the output
    after both versions saves everyone the confusion.

.PARAMETER From
    Release folder the devices are running now (the one containing "Firmware").

.PARAMETER To
    Release folder they should end up with.

.PARAMETER Out
    Where the patches are written. Default: "Delta-<from>-to-<to>" next to the newer release.

.PARAMETER Ftc
    The ftc tool. Default: looked up next to this script, then in the release's Tools folder, then in PATH.

.PARAMETER Raw
    Write uncompressed patches. Smaller devices unpack a compressed patch to a file first; if one of
    yours is short on filesystem space, this trades transfer time for room.

.EXAMPLE
    ./Make-DeltaPatch-Generic.ps1 -From ../OpenKNX-NeoPixel-0.7.0 -To ../OpenKNX-NeoPixel-0.8.0

.EXAMPLE
    ./Make-DeltaPatch-Generic.ps1 -From ./old -To ./new -Out ./patches -Raw
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$From,
    [Parameter(Mandatory = $true)][string]$To,
    [string]$Out = "",
    [string]$Ftc = "",
    [switch]$Raw
)

Set-StrictMode -Version Latest

# The image unwrapping lives in one place; this script only needs the result.
$imgLib = Join-Path $PSScriptRoot "OpenKNX-Image-Generic.ps1"
if (Test-Path -PathType Leaf $imgLib) { . $imgLib }

$ErrorActionPreference = 'Stop'

# Bus throughput, measured: an OpenKNX IP-Interface reaches about the lower figure, an MDT in its fast
# mode the upper one. Shown as a span so nobody reads a single number as a promise.
$BusSlow, $BusFast = 480, 630

# Windows PowerShell 5.1 has no $IsWindows; PowerShell 6+ has it and makes it read-only. Reading it
# through Get-Variable covers both without ever assigning to an automatic variable -- and 5.1 only runs
# on Windows, which is what makes the fallback correct rather than merely convenient.
$onWindows = if (Get-Variable -Name 'IsWindows' -ErrorAction SilentlyContinue) { $IsWindows } else { $true }

function Resolve-Ftc {
    param([string]$Hint)
    $exe = if ($onWindows) { "ftc.exe" } else { "ftc" }
    $candidates = @()
    if ($Hint) { $candidates += $Hint }
    $candidates += (Join-Path $PSScriptRoot $exe)
    $candidates += (Join-Path $PSScriptRoot "../Tools/$exe")
    $candidates += (Join-Path $To "Tools/$exe")
    foreach ($c in $candidates) {
        if ($c -and (Test-Path $c)) { return (Resolve-Path $c).Path }
    }
    $inPath = Get-Command $exe -ErrorAction SilentlyContinue
    if ($inPath) { return $inPath.Source }
    throw "ftc not found. Pass -Ftc <path>; it ships in the release's Tools folder."
}

function Get-AppImages {
    <# One entry per device: the product folder name and its raw application image. #>
    param([string]$ReleaseDir)
    $fwRoot = Join-Path $ReleaseDir "Firmware"
    if (-not (Test-Path $fwRoot)) { throw "no Firmware folder in $ReleaseDir" }
    $map = @{}
    foreach ($dir in Get-ChildItem $fwRoot -Directory) {
        # A release ships ONE file per device now. The raw image is derived from it -- an .app.bin is
        # still taken when an older release happens to carry one.
        $app = Get-ChildItem $dir.FullName -Filter "*.app.bin" -File | Select-Object -First 1
        if (-not $app) {
            $pkg = Get-ChildItem $dir.FullName -Include *.uf2, *.factory.bin -File | Select-Object -First 1
            if ($pkg) {
                $tmp = Join-Path ([System.IO.Path]::GetTempPath()) ($pkg.BaseName + ".app.bin")
                if (OpenKNX_SaveAppImage -Path $pkg.FullName -Target $tmp) { $app = Get-Item $tmp }
            }
        }
        if ($app) { $map[$dir.Name] = $app.FullName }
    }
    return $map
}

function Format-Span {
    param([int]$Bytes)
    $slow = $Bytes / $BusSlow
    $fast = $Bytes / $BusFast
    if ($slow -lt 90) { return ("{0:N0}-{1:N0} s" -f $fast, $slow) }
    return ("{0:N1}-{1:N1} min" -f ($fast / 60), ($slow / 60))
}

if (-not (Test-Path $From)) { throw "release not found: $From" }
if (-not (Test-Path $To)) { throw "release not found: $To" }
$ftcExe = Resolve-Ftc -Hint $Ftc

$fromName = (Get-Item $From).Name
$toName = (Get-Item $To).Name
if (-not $Out) { $Out = Join-Path (Split-Path (Resolve-Path $To)) "Delta-$fromName-to-$toName" }
New-Item -ItemType Directory -Force -Path $Out | Out-Null

$old = Get-AppImages $From
$new = Get-AppImages $To

Write-Host ""
Write-Host "  delta patches  $fromName -> $toName" -ForegroundColor Cyan
Write-Host ("  {0,-42}{1,10}{2,10}{3,14}" -f "device", "patch", "of image", "over the bus") -ForegroundColor DarkGray

$made = 0
$skipped = @()
foreach ($device in ($new.Keys | Sort-Object)) {
    if (-not $old.ContainsKey($device)) {
        $skipped += "$device (not in $fromName)"
        continue
    }
    $ext = if ($Raw) { "okd" } else { "okdz" }
    $target = Join-Path $Out "$device.$ext"
    $callArgs = @('delta', 'make', $old[$device], $new[$device], $target)
    if (-not $Raw) { $callArgs += '--pack' }
    & $ftcExe @callArgs *> $null
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $target)) {
        $skipped += "$device (patch could not be built)"
        continue
    }
    $size = (Get-Item $target).Length
    $image = (Get-Item $new[$device]).Length
    Write-Host ("  {0,-42}{1,10}{2,8:N1} %{3,14}" -f $device, $size, (100.0 * $size / $image), (Format-Span $size)) -ForegroundColor Green
    $made++
}

foreach ($s in $skipped) { Write-Host "  skipped  $s" -ForegroundColor DarkYellow }

Write-Host ""
if ($made -eq 0) {
    Write-Host "  no patch could be built - do both releases carry a firmware package?" -ForegroundColor Red
    exit 1
}
Write-Host "  $made patch(es) in $Out" -ForegroundColor Green
Write-Host "  send one with:  ftc -i <interface> <pa> delta <patch>" -ForegroundColor DarkGray
Write-Host ""
