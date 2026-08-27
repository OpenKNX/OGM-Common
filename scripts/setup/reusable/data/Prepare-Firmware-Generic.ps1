#!/usr/bin/env pwsh
<#
Open ■
┬────┴  Prepare-Firmware-Generic
■ KNX   2026 OpenKNX - Erkan Çolak

FILEPATH: OGM-Common/scripts/setup/reusable/data/Prepare-Firmware-Generic.ps1
   ships as: release/data/Prepare-Firmware-Generic.ps1

.SYNOPSIS
    Turns a released firmware package into the file you actually need — plain image, compressed for the
    KNX bus, or a difference to an older release.

.DESCRIPTION
    A release ships one file per device, and that file is a PACKAGE: a .uf2 wraps the image in 256-byte
    blocks, a .factory.bin puts a bootloader and a partition table in front of it. Neither is what a
    device's own updater takes, so this script writes out what is:

      <name>.app.bin      the plain application image — USB, ArduinoOTA, your own checksum
      <name>.app.bin.gz   the same image compressed — what goes over the KNX bus (knxOTA, ftc)
      <name>.okd          the difference to an older release — a fraction again, needs ftc

    None of the three upload scripts next to it calls this one. They send; this prepares — for a manual
    transfer through the knxOTA page of a router or interface, for another OTA tool, or for anyone who
    wants the files themselves.

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

.PARAMETER Gzip
    Also write <name>.app.bin.gz -- the form that goes over the KNX bus. The RP2040/RP2350 bootloader
    unpacks it on the next boot; an ESP32 target only if it was built with gzip support (its knxOTA
    page says so). Roughly a third of the size, and the bus is the slow part.

.PARAMETER Delta
    Build a difference against the firmware that is RUNNING on the target: a .uf2 / .factory.bin /
    .app.bin, or a folder to pick from. Needs ftc. A difference is a fraction of a full image, but the
    target refuses it unless the base matches exactly -- it checks length and checksum before it starts.

.PARAMETER All
    Image + gzip in one call.

.PARAMETER NoMenu
    Never ask. Without any of the switches above and with a user at the keyboard, the script offers a
    short menu; a pipeline that wants the plain image passes this.

.EXAMPLE
    ./Prepare-Firmware-Generic.ps1 ../Firmware
    # every package under Firmware/ is unwrapped in place

.EXAMPLE
    ./Prepare-Firmware-Generic.ps1 firmware-Device.factory.bin -OutDir ~/Desktop

.EXAMPLE
    ./Prepare-Firmware-Generic.ps1 -All
    # image + .gz -- the .gz is what you upload to a router/interface for a knxOTA transfer

.EXAMPLE
    ./Prepare-Firmware-Generic.ps1 -Delta ../KNeoPix-0.6
    # difference from the 0.6 release to this one, as <name>.okd

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
    [switch]$Force,
    [switch]$Gzip,
    [string]$Delta = "",
    [switch]$All,
    [switch]$NoMenu
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
            What = '  The application image is what runs on the device - no bootloader, no wrapper.'
            MenuTitle = '  What do you need?'
            Menu1 = '  [1] application image      .app.bin   USB, ArduinoOTA, checksums'
            Menu2 = '  [2] compressed for the bus .app.bin.gz  knxOTA, ftc - about a third of the size'
            Menu3 = '  [3] difference to an older release  .okd   knxOTA, ftc - a fraction again'
            Menu4 = '  [4] image + compressed'
            MenuAsk = 'choice'
            GzWrote = '  {0,-52} {1,10:N0} B  ->  {2}  ({3} %)'
            GzSkip = '  {0,-52} .gz already there - overwrite with -Force'
            DeltaPick = '  Difference to WHICH version? It has to be the one RUNNING on the target.'
            DeltaOther = '  [{0}] another path ...'
            DeltaAsk = 'old firmware'
            DeltaNone = '  no older firmware found next to this folder - give a path with -Delta'
            DeltaNoFtc = '  ftc not found - it builds the difference. Install it, or use -Gzip.'
            DeltaBase = '  base: {0}'
            DeltaOk = '  {0,-52} {1,10:N0} B  ->  {2}  ({3} % of the image)'
            DeltaFail = '  ftc could not build the difference: {0}'
            DeltaWarn = '  The target refuses a difference whose base is not EXACTLY what it runs.' }
    DE = @{ Way = 'auspacken'; NotFound = '  nicht gefunden: {0}'; NoPkg = '  keine .uf2 und keine .factory.bin gefunden.'
            NoImg = 'kein Image darin gefunden'; Same = 'liegt bereits vor, identisch'
            Differs = 'liegt bereits vor, WEICHT AB - mit -Force überschreiben'
            Wrote = '  {0} Image(s) geschrieben.'
            What = '  Das Anwendungsimage ist das, was auf dem Gerät läuft - ohne Bootloader, ohne Verpackung.'
            MenuTitle = '  Was brauchen Sie?'
            Menu1 = '  [1] Anwendungs-Abbild      .app.bin   USB, ArduinoOTA, Prüfsummen'
            Menu2 = '  [2] komprimiert für den Bus .app.bin.gz  knxOTA, ftc - etwa ein Drittel der Größe'
            Menu3 = '  [3] Differenz zu einer Vorversion  .okd   knxOTA, ftc - nochmal ein Bruchteil'
            Menu4 = '  [4] Abbild + komprimiert'
            MenuAsk = 'Auswahl'
            GzWrote = '  {0,-52} {1,10:N0} B  ->  {2}  ({3} %)'
            GzSkip = '  {0,-52} .gz liegt bereits vor - mit -Force überschreiben'
            DeltaPick = '  Differenz zu WELCHER Fassung? Es muss die sein, die auf dem Ziel LÄUFT.'
            DeltaOther = '  [{0}] anderer Pfad ...'
            DeltaAsk = 'alte Firmware'
            DeltaNone = '  keine ältere Firmware neben diesem Ordner gefunden - Pfad mit -Delta angeben'
            DeltaNoFtc = '  ftc nicht gefunden - es baut die Differenz. Installieren, oder -Gzip nutzen.'
            DeltaBase = '  Vorlage: {0}'
            DeltaOk = '  {0,-52} {1,10:N0} B  ->  {2}  ({3} % des Abbilds)'
            DeltaFail = '  ftc konnte die Differenz nicht bauen: {0}'
            DeltaWarn = '  Das Ziel lehnt eine Differenz ab, deren Vorlage nicht GENAU die laufende ist.' }
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
    function OpenKNX_ReadChoice {
        param([string]$Prompt, [string]$Default = "")
        $suffix = ""
        if ($Default) { $suffix = " [$Default]" }
        $answer = Read-Host "  $Prompt$suffix"
        if (-not $answer) { return $Default }
        return $answer.Trim()
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


# ─── gzip + difference ─────────────────────────────────────────────────────────────────────────────
# Both answer the same question in different sizes: what travels over the bus. The image itself is
# what runs; these two are transport forms of it.

function OpenKNX_WriteGzip {
    <# @brief Gzip a file with .NET only -- no external tool, same result on every platform. #>
    param([string]$Source, [string]$Target)
    $in = [System.IO.File]::ReadAllBytes($Source)
    $fs = [System.IO.File]::Create($Target)
    try {
        $gz = New-Object System.IO.Compression.GZipStream($fs, [System.IO.Compression.CompressionLevel]::Optimal)
        try { $gz.Write($in, 0, $in.Length) } finally { $gz.Dispose() }   # Dispose writes CRC32 + length
    } finally { $fs.Dispose() }
    $out = [System.IO.File]::ReadAllBytes($Target)
    # The target decides by the first two bytes, not by the name -- so verify them here.
    if ($out.Length -lt 3 -or $out[0] -ne 0x1F -or $out[1] -ne 0x8B) { return $null }
    return $out.Length
}

function OpenKNX_FindOlderFirmware {
    <# @brief Firmware packages lying next to this one -- candidates for the base of a difference. #>
    param([string]$Dir, [string]$Exclude)
    $roots = @($Dir)
    $up = Split-Path -Parent $Dir
    if ($up -and (Test-Path -PathType Container $up)) { $roots += $up }
    $hits = @()
    foreach ($r in $roots) {
        $depth = if ($r -eq $Dir) { 0 } else { 2 }
        Get-ChildItem -Path $r -Include *.app.bin, *.uf2, *.factory.bin -File -Recurse -ErrorAction SilentlyContinue |
            ForEach-Object {
                if ($_.FullName -eq $Exclude) { return }
                $hits += $_
            }
    }
    return ($hits | Sort-Object LastWriteTime -Descending | Select-Object -First 12)
}

# ─── run ──────────────────────────────────────────────────────────────────────────────────────────
if ($_haveUi) {
    $_ui = $script:OpenKNX_UI_Strings[(OpenKNX_UI_Lang $_lang)]
    OpenKNX_ShowTitle -Way $s.Way -Lang $_lang -Kind $_ui.KindPackage
}
else { OpenKNX_ShowLogo "OpenKNX" }

# No switches and someone at the keyboard: ask what is needed. A pipeline passes its switches and never
# sees the menu -- which is why this hangs off the switches, not off a prompt.
$_asked = $false
# UserInteractive alone is not enough: it stays true even when input comes from a pipe -- Read-Host then
# blocks or silently takes the default. What matters is whether anyone CAN type.
$_canAsk = [Environment]::UserInteractive
try { if ([Console]::IsInputRedirected) { $_canAsk = $false } } catch { }
if (-not $NoMenu -and -not $Gzip -and -not $All -and -not $Delta -and $_canAsk) {
    Write-Host $s.MenuTitle -ForegroundColor Yellow
    Write-Host ""
    Write-Host $s.Menu1
    Write-Host $s.Menu2
    Write-Host $s.Menu3
    Write-Host $s.Menu4
    Write-Host ""
    $c = OpenKNX_ReadChoice $s.MenuAsk "1"
    $_asked = $true
    switch ($c) {
        "2" { $Gzip = $true }
        "3" { $Delta = "?" }        # ? = not settled yet, asked for below
        "4" { $All = $true }
        default { }
    }
}
if ($All) { $Gzip = $true }

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
$images = @()      # jedes vorhandene .app.bin, egal ob eben geschrieben oder schon dagewesen
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
        $images += $target
        continue
    }
    [System.IO.File]::WriteAllBytes($target, $img)
    Write-Host ("  {0,-52} {1,10:N0} B  ->  {2}" -f $name, $img.Length, (Split-Path -Leaf $target)) -ForegroundColor Green
    $images += $target
    $done++
}

# ─── compressed for the bus ───────────────────────────────────────────────────────────────────────
if ($Gzip) {
    foreach ($img in $images) {
        $gzPath = "$img.gz"
        $nm = Split-Path -Leaf $img
        if ((Test-Path -PathType Leaf $gzPath) -and -not $Force) {
            Write-Host ($s.GzSkip -f $nm) -ForegroundColor DarkGray
            continue
        }
        $raw = (Get-Item $img).Length
        $len = OpenKNX_WriteGzip -Source $img -Target $gzPath
        if ($null -eq $len) {
            Write-Host ("  {0,-52} gzip?" -f $nm) -ForegroundColor Red
            continue
        }
        $pct = [math]::Round($len * 100.0 / [math]::Max($raw, 1))
        Write-Host ($s.GzWrote -f $nm, $len, (Split-Path -Leaf $gzPath), $pct) -ForegroundColor Green
    }
}


# ─── difference to an older release ────────────────────────────────────────────────────────────────
# The patch is built here, not on the device: that needs BOTH images. Before applying, the target
# checks the base's length and checksum -- if they do not match, it refuses.
if ($Delta) {
    $new = $images | Select-Object -First 1
    if ($images.Count -ne 1) {
        Write-Host ""
        Write-Host ("  {0}" -f $s.DeltaWarn) -ForegroundColor Yellow
    }
    if (-not $new) {
        Write-Host $s.NoPkg -ForegroundColor Yellow
        exit 1
    }
    $newDir = Split-Path -Parent $new

    # Settle the base: given on the command line, or picked from what lies next to this folder.
    $old = ""
    if ($Delta -ne "?" -and (Test-Path $Delta)) {
        if (Test-Path -PathType Container $Delta) {
            $cand = OpenKNX_FindOlderFirmware -Dir $Delta -Exclude $new
        } else { $old = (Resolve-Path $Delta).Path }
    } else {
        $cand = OpenKNX_FindOlderFirmware -Dir $newDir -Exclude $new
    }

    if (-not $old) {
        if (-not $cand -or $cand.Count -eq 0) {
            Write-Host ""
            Write-Host $s.DeltaNone -ForegroundColor Yellow
            Write-Host ""
            exit 1
        }
        Write-Host ""
        Write-Host $s.DeltaPick -ForegroundColor Yellow
        Write-Host ""
        $i = 1
        foreach ($c in $cand) {
            Write-Host ("  [{0}] {1,-46} {2,10:N0} B   {3:dd.MM. HH:mm}" -f $i, $c.Name, $c.Length, $c.LastWriteTime)
            $i++
        }
        Write-Host ($s.DeltaOther -f $i)
        Write-Host ""
        $pick = OpenKNX_ReadChoice $s.DeltaAsk "1"
        $n = 0
        if ([int]::TryParse($pick, [ref]$n) -and $n -ge 1 -and $n -le $cand.Count) {
            $old = $cand[$n - 1].FullName
        } else {
            # "another path": the system dialog where there is one -- otherwise walk the folders, up
            # to the drives. Nobody has to type a long path.
            if (Get-Command OpenKNX_PickFile -ErrorAction SilentlyContinue) {
                $old = OpenKNX_PickFile -Start $newDir -Include @("*.app.bin", "*.uf2", "*.factory.bin", "*.bin") -Prompt $s.DeltaAsk
            } else {
                $old = OpenKNX_ReadChoice $s.DeltaAsk ""
            }
            if (-not $old -or -not (Test-Path -PathType Leaf $old)) {
                Write-Host ($s.NotFound -f $old) -ForegroundColor Red
                exit 1
            }
            $old = (Resolve-Path $old).Path
        }
    }

    # A package as the base works too -- it is unwrapped first, otherwise ftc would compare block
    # frames instead of firmware.
    if ($old -notmatch '\.app\.bin$') {
        $tmpImg = OpenKNX_GetAppImage -Path $old
        if ($null -eq $tmpImg) {
            Write-Host ("  {0,-52} {1}" -f (Split-Path -Leaf $old), $s.NoImg) -ForegroundColor Yellow
            exit 1
        }
        $tmp = Join-Path ([System.IO.Path]::GetTempPath()) ((Split-Path -Leaf $old) + ".app.bin")
        [System.IO.File]::WriteAllBytes($tmp, $tmpImg)
        $old = $tmp
    }

    $ftc = ""
    if (Get-Command OpenKNX_FindFtc -ErrorAction SilentlyContinue) {
        $found = OpenKNX_FindFtc -SearchDirs @($PSScriptRoot, (Split-Path -Parent $PSScriptRoot), $newDir)
        if ($found.Installed) { $ftc = $found.Installed } elseif ($found.Shipped) { $ftc = $found.Shipped }
    }
    if (-not $ftc) {
        $c = Get-Command "ftc" -ErrorAction SilentlyContinue
        if ($c) { $ftc = $c.Source }
    }
    if (-not $ftc) {
        Write-Host ""
        Write-Host $s.DeltaNoFtc -ForegroundColor Yellow
        Write-Host ""
        exit 1
    }

    $out = [System.IO.Path]::ChangeExtension($new, $null).TrimEnd('.') -replace '\.app$', ''
    $out = "$out.okd"
    Write-Host ""
    Write-Host ($s.DeltaBase -f (Split-Path -Leaf $old)) -ForegroundColor DarkGray
    & $ftc delta make $old $new $out 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -PathType Leaf $out)) {
        Write-Host ($s.DeltaFail -f $LASTEXITCODE) -ForegroundColor Red
        Write-Host ""
        exit 1
    }
    $len = (Get-Item $out).Length
    $pct = [math]::Round($len * 100.0 / [math]::Max((Get-Item $new).Length, 1))
    Write-Host ($s.DeltaOk -f (Split-Path -Leaf $new), $len, (Split-Path -Leaf $out), $pct) -ForegroundColor Green
    Write-Host $s.DeltaWarn -ForegroundColor DarkGray
}

Write-Host ""
if ($done -gt 0) { Write-Host ($s.Wrote -f $done) -ForegroundColor Green }
Write-Host $s.What -ForegroundColor DarkGray
Write-Host ""
