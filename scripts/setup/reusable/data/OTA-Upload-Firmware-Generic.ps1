#!/usr/bin/env pwsh
<#
Open ■
┬────┴  OTA-Upload-Firmware-Generic
■ KNX   2025 OpenKNX - Erkan Çolak
        wiki.openknx.de - forum.openknx.de

.SYNOPSIS
    Network OTA firmware update for IP-capable OpenKNX devices via espota.

.DESCRIPTION
    Works for both RP2040/RP2350 and ESP32:
      - RP2040/RP2350: gzips the .bin first (the uncompressed image does not fit the OTA area; the
        bootloader / PicoOTA-uzlib ungzips the staged .gz on the next boot).
      - ESP32: uploads the raw .bin (self-applies via Update, no gzip).
    Chip is auto-detected from the firmware extension (.factory.bin -> ESP32, else RP2040/RP2350) and can be
    forced with -Esp. Extra espota args (e.g. the port) are passed positionally, matching the caller in
    Build-Step.ps1. Cross-platform and Windows PowerShell 5.1 compatible.

.PARAMETER FirmwareName
    Path to the firmware image (.bin for RP, .factory.bin for ESP), relative to the current directory.

.PARAMETER EspotaArgs
    Additional espota arguments, e.g. "-p 2040". Passed through verbatim.

.PARAMETER ESP32
    Force ESP32 mode (raw .bin, no gzip; alias -Esp). Otherwise auto-detected from the .factory.bin extension.

.PARAMETER RP2040
    Mark the image as RP2040 (gzip for OTA); affects only the displayed chip type.

.PARAMETER RP2350
    Mark the image as RP2350 (gzip for OTA); affects only the displayed chip type.

.PARAMETER Lang
    Display language: DE or EN. Auto-detected from the system locale if omitted (default DE).

.PARAMETER Help
    Show this help and exit. Alias: -h.

.EXAMPLE
    ./OTA-Upload-Firmware-Generic.ps1 firmware.bin "-p 2040"
        RP2040: gzip firmware.bin -> firmware.bin.gz, then espota to the prompted IP.

.EXAMPLE
    ./OTA-Upload-Firmware-Generic.ps1 firmware.factory.bin "-p 3232"
        ESP32 (auto-detected): upload the raw .bin.

.NOTES
    AUTHOR : Erkan Çolak
#>
param(
    [Parameter(Position = 0)]
    [string]$FirmwareName,
    [Parameter(Position = 1)]
    [string]$EspotaArgs = "",
    [Alias('Esp')]
    [switch]$ESP32,
    [switch]$RP2040,
    [switch]$RP2350,
    [string]$Lang = "",
    [Alias('h')]
    [switch]$Help
)

# PowerShell 5.1 has no $IsWindows/$IsMacOS/$IsLinux -- define them.
if ($null -eq (Get-Variable -Name 'IsMacOS' -ErrorAction SilentlyContinue)) {
    $IsMacOS = $false; $IsLinux = $false; $IsWindows = $true
}
# UTF-8 output so box glyphs / umlauts render on the Windows console (best-effort).
if ($IsWindows) { try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch {} }

# ─── Language (default DE, else system locale) ─────────────────────────────────
if (-not $Lang) {
    $sysLang = if ($IsWindows) { (Get-Culture).TwoLetterISOLanguageName } else { "$env:LANG" }
    $Lang = if ($sysLang -match '^en') { 'EN' } else { 'DE' }
}
$_lang = $Lang.ToUpper()
if ($_lang -notin @('DE', 'EN')) { $_lang = 'DE' }

$_strings = @{
    EN = @{
        Title          = 'OTA Firmware Update'
        Firmware       = "  Firmware : {0}"
        Target         = "  Target   : {0}:{1}"
        TypeRp         = "  Type     : {0} (gzip for OTA)"
        TypeEsp        = "  Type     : {0} (raw .bin, no gzip)"
        Hint           = "  Note: OTA may need to be enabled on the device via programming mode."
        NotFound       = "Firmware file not found: {0}"
        Compressing    = "  Compressing image (gzip)..."
        CompressFail   = "  Compression failed."
        AskIp          = "  IP address of the update target"
        BadIp          = "  Invalid IP address. Please try again."
        EspotaMissing  = "espota not found."
        EspotaOpt1     = "  Option 1 - OpenKNX tools (recommended):"
        EspotaOpt1b    = "    https://github.com/OpenKNX/OpenKNXproducer/releases -> Install-OpenKNX-Tools"
        EspotaOpt2     = "  Option 2 - pip:  pip install esptool"
        Done           = "  Done."
        Failed         = "  espota returned an error. Upload may have failed."
        PressEnter     = "  Press Enter to exit"
    }
    DE = @{
        Title          = 'OTA Firmware-Update'
        Firmware       = "  Firmware : {0}"
        Target         = "  Ziel     : {0}:{1}"
        TypeRp         = "  Typ      : {0} (gzip für OTA)"
        TypeEsp        = "  Typ      : {0} (rohe .bin, kein gzip)"
        Hint           = "  Hinweis: OTA muss auf dem Gerät ggf. über den Programmiermodus aktiviert werden."
        NotFound       = "Firmware-Datei nicht gefunden: {0}"
        Compressing    = "  Komprimiere Image (gzip)..."
        CompressFail   = "  Komprimierung fehlgeschlagen."
        AskIp          = "  IP-Adresse des Update-Ziels"
        BadIp          = "  Ungültige IP-Adresse. Bitte erneut eingeben."
        EspotaMissing  = "espota nicht gefunden."
        EspotaOpt1     = "  Option 1 - OpenKNX-Tools (empfohlen):"
        EspotaOpt1b    = "    https://github.com/OpenKNX/OpenKNXproducer/releases -> Install-OpenKNX-Tools"
        EspotaOpt2     = "  Option 2 - pip:  pip install esptool"
        Done           = "  Fertig."
        Failed         = "  espota meldet einen Fehler. Upload möglicherweise fehlgeschlagen."
        PressEnter     = "  Drücke Enter zum Beenden"
    }
}
$s = $_strings[$_lang]

function OpenKNX_ShowLogo($AddCustomText = $null) {
    Write-Host ""
    Write-Host "Open " -NoNewline
    Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -ForegroundColor Green
    $u = "$( [char]::ConvertFromUtf32(0x252C) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2534) ) "
    if ($AddCustomText) { Write-Host "$u $AddCustomText" -ForegroundColor Green } else { Write-Host $u -ForegroundColor Green }
    Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -NoNewline -ForegroundColor Green
    Write-Host " KNX"
    Write-Host ""
}

if ($Help) { Get-Help $MyInvocation.MyCommand.Path -Detailed; exit 0 }

# Find espota: ~/bin (OpenKNX tools), PATH, or the arduino-pico bundled tool.
function Find-Espota {
    foreach ($name in @('espota', 'espota.exe', 'espota.py')) {
        $p = Join-Path (Join-Path $HOME 'bin') $name
        if (Test-Path -PathType Leaf $p) { return $p }
    }
    foreach ($name in @('espota', 'espota.exe', 'espota.py')) {
        if (Get-Command $name -ErrorAction SilentlyContinue) { return (Get-Command $name).Source }
    }
    $pico = Join-Path $HOME ".platformio/packages/framework-arduinopico/tools/espota.py"
    if (Test-Path -PathType Leaf $pico) { return $pico }
    return $null
}

# gzip a file (adds the CRC32 + ISIZE trailer the RP bootloader needs). Returns the .gz path or $null.
function Compress-Gzip([string]$inPath) {
    $outPath = "$inPath.gz"
    try {
        $raw = [System.IO.File]::ReadAllBytes($inPath)
        $outFs = [System.IO.File]::Create($outPath)
        $levelName = if ([enum]::GetNames([System.IO.Compression.CompressionLevel]) -contains 'SmallestSize') { 'SmallestSize' } else { 'Optimal' }
        $level = [System.IO.Compression.CompressionLevel]::$levelName
        $gz = New-Object System.IO.Compression.GZipStream($outFs, $level)
        try { $gz.Write($raw, 0, $raw.Length) } finally { $gz.Dispose() } # flush + gzip trailer
        $outFs.Dispose()
        return $outPath
    } catch {
        if ($outFs) { try { $outFs.Dispose() } catch {} }
        return $null
    }
}

OpenKNX_ShowLogo $s.Title

# resolve firmware
if ([string]::IsNullOrWhiteSpace($FirmwareName) -or -not (Test-Path $FirmwareName)) {
    Write-Host ($s.NotFound -f $FirmwareName) -ForegroundColor Red
    Read-Host $s.PressEnter
    exit 1
}
$firmwarePath = (Resolve-Path $FirmwareName).Path

# chip: ESP if forced (-ESP32) or a .factory.bin, else RP (specific type from -RP2040/-RP2350 if given)
$isEsp = $ESP32 -or ($FirmwareName -match '\.factory\.bin$')
$chip  = if ($isEsp) { 'ESP32' } elseif ($RP2350) { 'RP2350' } elseif ($RP2040) { 'RP2040' } else { 'RP2040/RP2350' }

# espota
$espota = Find-Espota
if (-not $espota) {
    Write-Host ""
    Write-Host "  $($s.EspotaMissing)" -ForegroundColor Red
    Write-Host ""
    Write-Host $s.EspotaOpt1 -ForegroundColor Yellow
    Write-Host $s.EspotaOpt1b -ForegroundColor DarkGray
    Write-Host $s.EspotaOpt2 -ForegroundColor Yellow
    Write-Host ""
    Read-Host $s.PressEnter
    exit 1
}
# espota.py needs a python interpreter; a bare espota/espota.exe is run directly
$runner = @()
if ($espota -match '\.py$') {
    if ($IsWindows) { $py = Join-Path $HOME ".platformio\penv\Scripts\python.exe" }
    else            { $py = Join-Path $HOME ".platformio/penv/bin/python" }
    if (-not (Test-Path $py)) {
        $pyCmd = Get-Command python -ErrorAction SilentlyContinue
        if ($pyCmd) { $py = $pyCmd.Source } else { $py = 'python' }
    }
    $runner = @($py, $espota)
} else {
    $runner = @($espota)
}

# RP: gzip; ESP: raw
if ($isEsp) {
    Write-Host ($s.TypeEsp -f $chip) -ForegroundColor DarkGray
    $upload = $firmwarePath
} else {
    Write-Host ($s.TypeRp -f $chip) -ForegroundColor DarkGray
    Write-Host $s.Compressing -ForegroundColor DarkGray
    $upload = Compress-Gzip $firmwarePath
    if (-not $upload) { Write-Host $s.CompressFail -ForegroundColor Red; Read-Host $s.PressEnter; exit 1 }
}

Write-Host ($s.Firmware -f $upload) -ForegroundColor Cyan
Write-Host ""
Write-Host $s.Hint -ForegroundColor DarkYellow
Write-Host ""

# IP input with validation
$ipAddress = $null
$validIp = $false
while (-not $validIp) {
    $ipAddress = (Read-Host $s.AskIp).Trim()
    $validIp = [System.Net.IPAddress]::TryParse($ipAddress, [ref]$null)
    if (-not $validIp) { Write-Host $s.BadIp -ForegroundColor Red }
}

# extra espota args (e.g. "-p 2040") split into tokens
$extra = @()
if (-not [string]::IsNullOrWhiteSpace($EspotaArgs)) { $extra = $EspotaArgs.Trim().Trim("'`"").Split(' ') }

Write-Host ""
Write-Host ("  " + ($runner -join ' ') + " -i $ipAddress $($extra -join ' ') -f $upload") -ForegroundColor DarkGray
Write-Host ""

& $runner[0] ($runner[1..($runner.Count - 1)]) -i $ipAddress $extra -f $upload
$code = $LASTEXITCODE

Write-Host ""
if ($code -eq 0) { Write-Host $s.Done -ForegroundColor Green } else { Write-Host $s.Failed -ForegroundColor Red }
Write-Host ""
Read-Host $s.PressEnter
exit $code
