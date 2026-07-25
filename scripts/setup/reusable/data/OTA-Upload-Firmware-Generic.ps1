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
    [string]$Ip = "",
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
        ChooseTarget   = "How do you want to select the target?"
        OptEnterIp     = "Enter IP address manually"
        OptScan        = "Search the network (mDNS)"
        MenuChoice     = "Selection (1-2)"
        Scanning       = "Searching for OpenKNX devices (mDNS)..."
        ScanHint       = "Only devices with mDNS enabled appear (default). If mDNS is disabled on the OpenKNX device, enter the IP manually."
        ScanNone       = "No OpenKNX devices found via mDNS."
        ScanFound      = "Found {0} device(s):"
        ScanPick       = "Select device (1-{0}), or 0 to enter an IP manually"
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
        ChooseTarget   = "Wie möchtest du das Ziel auswählen?"
        OptEnterIp     = "IP-Adresse manuell eingeben"
        OptScan        = "Netzwerk durchsuchen (mDNS)"
        MenuChoice     = "Auswahl (1-2)"
        Scanning       = "Suche OpenKNX-Geräte (mDNS)..."
        ScanHint       = "Es erscheinen nur Geräte mit aktiviertem mDNS (Standard). Ist mDNS am OpenKNX Gerät deaktiviert, bitte IP manuell eingeben."
        ScanNone       = "Keine OpenKNX-Geräte per mDNS gefunden."
        ScanFound      = "{0} Gerät(e) gefunden:"
        ScanPick       = "Gerät auswählen (1-{0}), oder 0 für manuelle IP-Eingabe"
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

# ─── mDNS / DNS-SD discovery of OpenKNX devices (service _openknx._tcp) ─────────
# Pure .NET/UDP so it works identically on Windows (PS 5.1), macOS and Linux - no dns-sd/avahi needed.
# One PTR query for _openknx._tcp.local is sent with the QU bit (unicast response); answers are collected
# on an ephemeral port for a short window, then PTR->SRV->A->TXT are correlated. The OTA port and chip
# come from the TXT "ota=" item (3232 = ESP32, 2040 = RP2040/RP2350), not from the placeholder SRV port.

# Reads a (possibly compressed) DNS name from $bytes at $offset. Returns @{Name; Next}; Next is the
# offset right after the name in the record stream (a compression jump does not advance it).
function Read-DnsName([byte[]]$bytes, [int]$offset) {
    $labels = @(); $jumped = $false; $next = $offset; $safety = 0
    while ($true) {
        if ($safety++ -gt 128 -or $offset -ge $bytes.Length) { break }   # malformed / loop guard
        $len = $bytes[$offset]
        if ($len -eq 0) { if (-not $jumped) { $next = $offset + 1 }; break }
        if (($len -band 0xC0) -eq 0xC0) {                                 # compression pointer
            if ($offset + 1 -ge $bytes.Length) { break }
            $ptr = (($len -band 0x3F) -shl 8) -bor $bytes[$offset + 1]
            if (-not $jumped) { $next = $offset + 2 }
            $jumped = $true; $offset = $ptr; continue
        }
        $offset++
        if ($offset + $len -gt $bytes.Length) { break }
        $labels += [System.Text.Encoding]::UTF8.GetString($bytes, $offset, $len)
        $offset += $len
    }
    return @{ Name = ($labels -join '.'); Next = $next }
}

# Builds a single mDNS PTR query for $service (QU bit set for a unicast response).
function Build-MdnsQuery([string]$service) {
    $ms = New-Object System.IO.MemoryStream
    $w  = New-Object System.IO.BinaryWriter($ms)
    foreach ($v in @(0, 0, 1, 0, 0, 0)) { $w.Write([byte]($v -shr 8)); $w.Write([byte]($v -band 0xFF)) } # header, qd=1
    foreach ($label in $service.Split('.')) {
        $b = [System.Text.Encoding]::UTF8.GetBytes($label)
        $w.Write([byte]$b.Length); $w.Write($b)
    }
    $w.Write([byte]0)                                    # root label
    $w.Write([byte]0); $w.Write([byte]12)                # QTYPE PTR
    $w.Write([byte]0x80); $w.Write([byte]0x01)           # QCLASS IN + QU (unicast response)
    $w.Flush()
    return $ms.ToArray()
}

# Discovers OpenKNX devices via mDNS. Returns objects {Name, Ip, Pa, Version, OtaPort, Chip}.
function Get-OpenKnxDevices([int]$timeoutMs = 2500) {
    $query = Build-MdnsQuery "_openknx._tcp.local"
    $udp = New-Object System.Net.Sockets.UdpClient
    $out = @{}
    try {
        $udp.Client.SetSocketOption([System.Net.Sockets.SocketOptionLevel]::Socket, [System.Net.Sockets.SocketOptionName]::ReuseAddress, $true)
        $udp.Client.Bind((New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, 0)))
        $udp.Client.ReceiveTimeout = 350
        $mcast = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Parse("224.0.0.251"), 5353)
        [void]$udp.Send($query, $query.Length, $mcast)

        $ptr = @{}; $srv = @{}; $addr = @{}; $txt = @{}
        $deadline = (Get-Date).AddMilliseconds($timeoutMs)
        while ((Get-Date) -lt $deadline) {
            $remote = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, 0)
            $buf = $null
            try { $buf = $udp.Receive([ref]$remote) } catch { continue }  # ReceiveTimeout -> loop to deadline
            if (-not $buf -or $buf.Length -lt 12) { continue }
            $qd = ($buf[4] -shl 8) -bor $buf[5]
            $rr = (($buf[6] -shl 8) -bor $buf[7]) + (($buf[8] -shl 8) -bor $buf[9]) + (($buf[10] -shl 8) -bor $buf[11])
            $pos = 12
            for ($i = 0; $i -lt $qd; $i++) { $n = Read-DnsName $buf $pos; $pos = $n.Next + 4 }  # skip questions
            for ($i = 0; $i -lt $rr; $i++) {
                if ($pos + 10 -gt $buf.Length) { break }
                $nm = Read-DnsName $buf $pos; $pos = $nm.Next; $name = $nm.Name
                $type  = ($buf[$pos] -shl 8) -bor $buf[$pos + 1]
                $rdlen = ($buf[$pos + 8] -shl 8) -bor $buf[$pos + 9]
                $rdpos = $pos + 10
                if ($rdpos + $rdlen -gt $buf.Length) { break }
                switch ($type) {
                    12 { $t = Read-DnsName $buf $rdpos; $ptr[$t.Name] = $true }                         # PTR -> instance
                    33 { $srv[$name] = @{ Port = (($buf[$rdpos + 4] -shl 8) -bor $buf[$rdpos + 5]);      # SRV
                                          Target = (Read-DnsName $buf ($rdpos + 6)).Name } }
                    1  { if ($rdlen -ge 4) { $addr[$name] = "$($buf[$rdpos]).$($buf[$rdpos+1]).$($buf[$rdpos+2]).$($buf[$rdpos+3])" } } # A
                    16 { $kv = @{}; $tp = $rdpos; $end = $rdpos + $rdlen                                 # TXT
                         while ($tp -lt $end) {
                             $l = $buf[$tp]; $tp++
                             if ($l -eq 0 -or $tp + $l -gt $end) { break }
                             $item = [System.Text.Encoding]::UTF8.GetString($buf, $tp, $l); $tp += $l
                             $eq = $item.IndexOf('='); if ($eq -ge 0) { $kv[$item.Substring(0, $eq)] = $item.Substring($eq + 1) }
                         }
                         $txt[$name] = $kv }
                }
                $pos = $rdpos + $rdlen
            }
        }
        foreach ($inst in @($ptr.Keys + $srv.Keys | Sort-Object -Unique)) {
            $targetHost = if ($srv.ContainsKey($inst)) { $srv[$inst].Target } else { $null }
            $ip = if ($targetHost -and $addr.ContainsKey($targetHost)) { $addr[$targetHost] } else { $targetHost }
            if (-not $ip) { continue }
            $tx = if ($txt.ContainsKey($inst)) { $txt[$inst] } else { @{} }
            $otaPort = $tx['ota']
            $chip = if ($otaPort -eq '3232') { 'ESP32' } elseif ($otaPort -eq '2040') { 'RP2040/RP2350' } else { '' }
            $out[$inst] = [PSCustomObject]@{
                Name = ($inst -replace '\._openknx\._tcp\.local\.?$', '')
                Ip = $ip; Pa = $tx['address']; Version = $tx['version']; OtaPort = $otaPort; Chip = $chip
                Configured = $tx['configured']; Firmware = $tx['firmware']; Serial = $tx['serial']; OtaMode = $tx['otamode']
            }
        }
    } catch {
        # discovery is best-effort; on any socket/parse error just return what we have
    } finally {
        $udp.Close()
    }
    return @($out.Values | Sort-Object Name)
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

# Target selection: an explicit -Ip wins; otherwise a menu (manual IP entry / mDNS network search).
$ipAddress = $null
$pickedPort = $null   # OTA port reported by a scan-picked device (overrides the caller's -p)
if ($Ip -and [System.Net.IPAddress]::TryParse($Ip.Trim(), [ref]$null)) { $ipAddress = $Ip.Trim() }
while (-not $ipAddress) {
    Write-Host ""
    Write-Host $s.ChooseTarget -ForegroundColor Yellow
    Write-Host "  [1] $($s.OptEnterIp)"
    Write-Host "  [2] $($s.OptScan)"
    $mode = (Read-Host $s.MenuChoice).Trim()
    if ($mode -eq '2') {
        Write-Host ""
        Write-Host $s.Scanning -ForegroundColor DarkGray
        Write-Host "  $($s.ScanHint)" -ForegroundColor DarkGray
        $found = @(Get-OpenKnxDevices)
        if ($found.Count -eq 0) { Write-Host "  $($s.ScanNone)" -ForegroundColor Yellow; continue }
        Write-Host ""
        Write-Host ($s.ScanFound -f $found.Count) -ForegroundColor Yellow
        for ($i = 0; $i -lt $found.Count; $i++) {
            $d = $found[$i]
            $line = "  [{0}] {1,-18} {2,-15}" -f ($i + 1), $d.Name, $d.Ip
            if ($d.Pa)         { $line += "  PA $($d.Pa)" }
            if ($d.OtaPort)    { $line += "  OTA $($d.OtaPort)"; if ($d.OtaMode) { $line += " ($($d.OtaMode))" } }
            if ($d.Version)    { $line += "  v$($d.Version)" }
            if ($d.Configured) { $line += "  cfg=$($d.Configured)" }
            Write-Host $line
        }
        Write-Host ""
        $sel = (Read-Host ($s.ScanPick -f $found.Count)).Trim()
        $idx = $sel -as [int]
        if ($null -ne $idx -and $idx -ge 1 -and $idx -le $found.Count) {
            $ipAddress  = $found[$idx - 1].Ip
            $pickedPort = $found[$idx - 1].OtaPort   # use the port the device itself reports
        }
        continue
    }
    # manual IP entry (option 1 / default)
    $entered = (Read-Host $s.AskIp).Trim()
    if ([System.Net.IPAddress]::TryParse($entered, [ref]$null)) { $ipAddress = $entered }
    else { Write-Host $s.BadIp -ForegroundColor Red }
}

# extra espota args: a scan-picked device reports its own OTA port (authoritative); otherwise use the
# port the caller passed (e.g. "-p 2040" / "-p 3232").
$extra = @()
if ($pickedPort) { $extra = @('-p', "$pickedPort") }
elseif (-not [string]::IsNullOrWhiteSpace($EspotaArgs)) { $extra = $EspotaArgs.Trim().Trim("'`"").Split(' ') }

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
