#!/usr/bin/env pwsh
<#
Open ■
┬────┴  Build-knxprod-Generic
■ KNX   2026 OpenKNX - Erkan Çolak

.SYNOPSIS
    Builds a .knxprod file from the application XML using OpenKNXproducer.

.DESCRIPTION
    This is the real build engine. It lives in the release "data" folder and is
    normally invoked by the thin ETS-Applikation/Build-knxprod.ps1 wrapper, which
    bakes in the XML name and the application version at release build time.
    It also works stand-alone (auto-discovers the XML next to itself).

    Cross-platform: builds the .knxprod on Windows, macOS and Linux (OpenKNXproducer
    now runs on all three). If the producer is not installed, a friendly hint is shown.

    Targets Windows PowerShell 5.1 (Win10 default) and PowerShell 7+ (macOS/Linux).

.PARAMETER Lang
    UI language: DE (default) or EN. Auto-detected from culture/LANG when omitted.

.PARAMETER Xml
    Application XML file name (located in this script's folder). When omitted the
    script auto-discovers the single *.xml (other than content.xml).

.PARAMETER OutDir
    Directory the <app>.knxprod is written to. Defaults to this script's folder.

.PARAMETER AppVersion
    Application version string, shown in the UI only (the wrapper injects it).

.PARAMETER Detailed
    Show the full OpenKNXproducer output and the exact command line (verbose view).

.PARAMETER Help
    Show usage information and exit.

.EXAMPLE
    .\Build-knxprod-Generic.ps1
    .\Build-knxprod-Generic.ps1 -Xml NeoPixel.xml -AppVersion 4.3.7 -OutDir ..\ETS-Applikation

.LINK
    https://github.com/OpenKNX/OpenKNXproducer
#>
param(
    [string]$Lang = "",
    [string]$Xml = "",               # application XML name in this folder (else auto-discover)
    [string]$OutDir = "",            # where to write <app>.knxprod (else next to this script)
    [string]$AppVersion = "",        # application version, display only (wrapper injects it)
    [string]$OutName = "",           # explicit output basename (wrapper injects e.g. IP-Interface-Dev-v1.0.0); else the XML basename
    [switch]$Detailed,               # show the full OpenKNXproducer output + the exact command
    [Alias("h")]
    [switch]$Help
)

# ── Config (hier anpassbar) ──────────────────────────────────────────────────────
$BuilderVersion = '0.0.2'        # eigene Version dieses Builders (im Titel angezeigt)
$ShowWarnings = $true            # OpenKNXproducer-Warnungen in der Fertig-Box anzeigen (true/false)
$MaxWarnings  = 10               # max. angezeigte Warnzeilen (Rest als "… und N weitere")
$ExitTimeout  = 60               # Sekunden bis zum Auto-Schließen der Abschluss-Box (Zeit zum Lesen des Hinweises)
# Hinweis: KEIN eigener Versions-Floor mehr. Die Mindest-Producer-Version steht in der
# XML (minOpenKNXproducerVersion); OpenKNXproducer prüft sie selbst und bricht mit klarer
# Meldung ab. Wir prüfen nur noch, ob der Producer überhaupt vorhanden/lauffähig ist.

# ── Platform detection (PowerShell 5.1 has no automatic $IsWindows) ──────────────
if ($null -eq (Get-Variable -Name 'IsMacOS' -ErrorAction SilentlyContinue)) {
    $IsMacOS = $false; $IsLinux = $false; $IsWindows = $true
}

# Render Unicode glyphs (box drawing, bullets, umlauts) on the Windows console.
# Windows PowerShell 5.1 defaults to the OEM code page (850/437) -> "?"; UTF-8 fixes it.
if ($IsWindows) {
    try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch {}
}

# ── Language (consistent with Upload-Firmware-Generic.ps1; default DE) ───────────
if (-not $Lang) {
    $sysLang = if ($IsWindows) { (Get-Culture).TwoLetterISOLanguageName } else { "$env:LANG" }
    $Lang = if ($sysLang -match '^en') { 'EN' } else { 'DE' }
}
$Lang = $Lang.ToUpper()
if ($Lang -notin @('DE', 'EN')) { $Lang = 'DE' }

$L = @{
    DE = @{
        Title          = 'Knxprod Builder - OpenKNX.de'
        TargetApp      = 'Ziel-App'
        Missing        = 'nicht gefunden'
        Found          = 'gefunden'
        ProducerName   = 'OpenKNXproducer'
        SearchVer      = 'Suche Producer:'
        ToolsMissTitle = 'OpenKNX-Tools fehlen'
        ToolsMiss1     = 'Bitte das neuste OpenKNX-Tools-Paket herunterladen:'
        ToolsMiss2     = 'entpacken und das Readme befolgen. Weitere Infos im OpenKNX-Wiki:'
        ToolsMiss3     = 'Danach dieses Skript erneut starten.'
        BrowserWarn    = 'Hinweis: Browser warnen ggf. vor dem Paket (es enthält ausführbare Programme + Skript).'
        LinksTitle     = 'Links'
        NoXmlTitle     = 'Keine Applikations-XML gefunden'
        NoXml1         = 'In {0} liegt keine passende .xml (außer content.xml).'
        SelectTitle    = 'Mehrere XML-Dateien gefunden – bitte auswählen:'
        SelectPrompt   = 'Auswahl'
        Cancel         = 'Abbrechen'
        Building       = 'OpenKNXproducer erzeugt {0}.knxprod – bitte warten …'
        BuildDone      = 'OpenKNXproducer erzeugt {0}.knxprod – FERTIG!'
        DoneText       = '{0}.knxprod wurde erfolgreich erstellt.'
        DoneFile       = 'Datei'
        DoneSize       = 'Größe'
        DoneVer        = 'Version'
        NextTitle      = 'Nächste Schritte'
        Next1          = 'Die .knxprod in der ETS über den Katalog importieren (ETS 5.7.7 oder neuer, ETS 6).'
        Next2          = 'Dann Physikalische Adresse und - nach der Parametrierung - die Applikation programmieren.'
        Next3          = 'Update-Hinweise (Firmware- und/oder ETS-Update) siehe Applikationsbeschreibung.'
        NoteTitle      = '⚠︎ Wichtiger Hinweis'
        NoteBody       = @(
            'OpenKNX ist ein unabhängiges Open-Source-Projekt. Wir stellen offenen Quellcode bereit – die Produktdatenbank hast du',
            'dir daraus soeben selbst erzeugt. Sie ist von niemandem geprüft, registriert oder freigegeben.',
            '',
            'Die Software wird unentgeltlich und ohne Gewährleistung bereitgestellt; für die Haftung gelten die Regelungen der',
            'Lizenz (GPL-3.0, §§ 15-16).',
            ''
        )
        NoteOwn        = @(
            'Die von dir erzeugte Datei und ihr Einsatz liegen allein in deiner Verantwortung und setzen Fachkenntnis voraus.'
        )
        Hints          = 'Hinweise ({0}):'
        HintsMore      = '… und {0} weitere'
        ProducerLog    = 'Producer-Ausgabe (Aufruf + komplettes Protokoll)'
        ErrTitle       = 'Fehler beim Erstellen der knxprod'
        ExitHint       = '[Enter] Beenden – schließt automatisch in {0,2} s'
    }
    EN = @{
        Title          = 'Knxprod Builder - OpenKNX.de'
        TargetApp      = 'Target App'
        Missing        = 'not found'
        Found          = 'found'
        ProducerName   = 'OpenKNXproducer'
        SearchVer      = 'Looking for producer:'
        ToolsMissTitle = 'OpenKNX tools are missing'
        ToolsMiss1     = 'Please download the latest OpenKNX tools package:'
        ToolsMiss2     = 'extract it and follow the readme. More info in the OpenKNX wiki:'
        ToolsMiss3     = 'Then run this script again.'
        BrowserWarn    = 'Note: browsers may warn about the package (it contains executables + a script).'
        LinksTitle     = 'Links'
        NoXmlTitle     = 'No application XML found'
        NoXml1         = 'No matching .xml (other than content.xml) in {0}.'
        SelectTitle    = 'Multiple XML files found – please select:'
        SelectPrompt   = 'Choice'
        Cancel         = 'Cancel'
        Building       = 'OpenKNXproducer is creating {0}.knxprod – please wait …'
        BuildDone      = 'OpenKNXproducer is creating {0}.knxprod – DONE!'
        DoneText       = '{0}.knxprod created successfully.'
        DoneFile       = 'File'
        DoneSize       = 'Size'
        DoneVer        = 'Version'
        NextTitle      = 'Next steps'
        Next1          = 'Import the .knxprod into ETS via the catalog (ETS 5.7.7 or newer, ETS 6).'
        Next2          = 'Then assign the physical address and - after parametrisation - program the application.'
        Next3          = 'See the application description for update notes (firmware and/or ETS update).'
        NoteTitle      = '⚠︎ Important note'
        NoteBody       = @(
            'OpenKNX is an independent open-source project. We publish source code only – you generated this product database from',
            'it yourself, just now. It has not been checked, registered or approved by anyone.',
            '',
            'The software is provided free of charge and without warranty; liability is governed by the license (GPL-3.0, §§ 15-16).',
            ''
        )
        NoteOwn        = @(
            'The file you generated, and how you use it, is entirely your own responsibility and requires the corresponding',
            'expertise.'
        )
        Hints          = 'Notes ({0}):'
        HintsMore      = '… and {0} more'
        ProducerLog    = 'Producer output (command + full log)'
        ErrTitle       = 'Error while creating the knxprod'
        ExitHint       = '[Enter] Exit – closes automatically in {0,2} s'
    }
}
$s = if ($L.ContainsKey($Lang)) { $L[$Lang] } else { $L.DE }

$LINK_TOOLS = 'https://github.com/OpenKNX/OpenKNXproducer/releases'
$LINK_WIKI  = 'https://github.com/OpenKNX/OpenKNX/wiki/Installation-of-OpenKNX-tools'

# ── Presentation ─────────────────────────────────────────────────────────────────
$W      = 62                          # rule width (matches Upload-Firmware-Generic)
$COL_MK = '     '                     # marker / bullet column (indent 5)
# Show a real check on terminals that can draw it: Windows Terminal (sets $env:WT_SESSION)
# and macOS/Linux. The legacy Windows console (conhost + Consolas) has no ✓ glyph -> "[OK]".
$modernTerm = (-not $IsWindows) -or [bool]$env:WT_SESSION
$TICK   = if ($modernTerm) { [char]::ConvertFromUtf32(0x2713) } else { '[OK]' }   # ✓ / [OK]
# Brand line incl. the builder's own version (language-neutral product name).
$brandLine = "Knxprod Builder v$BuilderVersion - OpenKNX.de"

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

# Light-gray rule (─), exactly like the Upload-Firmware-Generic separators.
function Show-Rule { Write-Host ('  ' + ([string][char]0x2500 * $W)) -ForegroundColor DarkGray }

# Section: rule / title / rule. Title colour carries severity (no » marker, no cyan).
function Show-Head($title, [ConsoleColor]$color = [ConsoleColor]::White) {
    Show-Rule
    Write-Host ('  ' + $title) -ForegroundColor $color
    Show-Rule
}

# Bullet item:  • text   (grey bullet in the marker column).
function Show-Item($text, [ConsoleColor]$color = [ConsoleColor]::White) {
    Write-Host ($COL_MK + [char]0x2022 + '  ') -ForegroundColor DarkGray -NoNewline                   # •
    Write-Host $text -ForegroundColor $color
}

# Plain indented text line (no marker); an empty string prints a blank separator line.
function Show-Text($text, [ConsoleColor]$color = [ConsoleColor]::DarkGray) {
    if ([string]::IsNullOrEmpty($text)) { Write-Host "" } else { Write-Host ('  ' + $text) -ForegroundColor $color }
}

# Key/value detail line:  Label  value   (label padded so values align).
function Show-Field($label, $value) {
    Write-Host ('  ' + ([string]$label).PadRight(9)) -ForegroundColor DarkGray -NoNewline
    Write-Host $value -ForegroundColor DarkGray
}

# Reads a single choice from a fixed set (case-insensitive). Returns the upper-cased match.
function Read-Choice([string]$prompt, [string[]]$valid) {
    while ($true) {
        $v = (Read-Host $prompt).Trim().ToUpper()
        if ($v -in $valid) { return $v }
    }
}

# [Enter] to exit, or auto-close after $seconds. Shows a live countdown.
function Wait-EnterOrTimeout([int]$seconds) {
    $end = (Get-Date).AddSeconds($seconds)
    while ($true) {
        $rem = [int][Math]::Ceiling(($end - (Get-Date)).TotalSeconds)
        if ($rem -lt 0) { $rem = 0 }
        Write-Host ("`r  " + ($s.ExitHint -f $rem) + '   ') -ForegroundColor DarkGray -NoNewline
        if ($rem -le 0) { break }
        try {
            if ([Console]::KeyAvailable) {
                if (([Console]::ReadKey($true)).Key -eq 'Enter') { break }
            }
        } catch {
            break   # no interactive console (redirected / CI) – nothing to wait for
        }
        Start-Sleep -Milliseconds 100
    }
    Write-Host ""
}

function Show-Help {
    OpenKNX_ShowLogo $brandLine
    Write-Host "USAGE:" -ForegroundColor Yellow
    Write-Host "  .\Build-knxprod-Generic.ps1 [-Lang DE|EN] [-Xml <name>] [-OutDir <dir>] [-AppVersion <v>] [-Help]"
    Write-Host ""
    Write-Host "OPTIONS:" -ForegroundColor Yellow
    Write-Host "  -Lang DE|EN     " -NoNewline -ForegroundColor White; Write-Host "UI language (default: auto, DE)" -ForegroundColor DarkGray
    Write-Host "  -Xml <name>     " -NoNewline -ForegroundColor White; Write-Host "XML in this folder (default: auto-discover)" -ForegroundColor DarkGray
    Write-Host "  -OutDir <dir>   " -NoNewline -ForegroundColor White; Write-Host "Output folder for the .knxprod (default: here)" -ForegroundColor DarkGray
    Write-Host "  -AppVersion <v> " -NoNewline -ForegroundColor White; Write-Host "Application version, shown in the UI" -ForegroundColor DarkGray
    Write-Host "  -Detailed       " -NoNewline -ForegroundColor White; Write-Host "Show full OpenKNXproducer output + the exact command" -ForegroundColor DarkGray
    Write-Host "  -Help, -h       " -NoNewline -ForegroundColor White; Write-Host "Show this help" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "Builds <app>.knxprod from the application XML via OpenKNXproducer (Windows only)." -ForegroundColor DarkGray
    Write-Host "Normally called by the ETS-Applikation\Build-knxprod.ps1 wrapper." -ForegroundColor DarkGray
    Write-Host ""
}

# ══════════════════════════════════════════════════════════════════════════════════
# Main
# ══════════════════════════════════════════════════════════════════════════════════
if ($Help) { Show-Help; exit 0 }

Clear-Host
# Title:  Knxprod Builder v0.0.2 - OpenKNX.de  (Ziel-App: NeoPixel.xml - v0.3.0)
$titleLine = $brandLine
$tgt = @()
if ($Xml)        { $tgt += $Xml }
if ($AppVersion) { $tgt += "v$AppVersion" }
if ($tgt.Count -gt 0) { $titleLine += "  ($($s.TargetApp): " + ($tgt -join ' - ') + ")" }
OpenKNX_ShowLogo $titleLine

# ── Tool detection (cross-platform: OpenKNXproducer now builds on Windows + macOS/Linux) ──
# Mirror Build-Release-Preprocess.ps1: Windows -> ~/bin/OpenKNXproducer.exe,
# macOS/Linux -> /usr/local/bin/OpenKNXproducer, plus a PATH fallback so a producer
# installed elsewhere is still found.
if ($IsWindows) {
    $producer = Join-Path (Join-Path $HOME 'bin') 'OpenKNXproducer.exe'
} else {
    $producer = '/usr/local/bin/OpenKNXproducer'
}
if (-not (Test-Path -PathType Leaf $producer)) {
    $onPath = Get-Command 'OpenKNXproducer' -ErrorAction SilentlyContinue
    if ($onPath) { $producer = $onPath.Source }
}
$producerOk  = Test-Path -PathType Leaf $producer   # only "is it there?" – the producer self-checks the XML's required version
$producerVer = $null
if ($producerOk) {
    # best-effort version read for display only (no [Version] cast -> tolerates suffixes
    # like 4.3.10-beta); a parse hiccup must NOT block the build
    try {
        $verOut = (& $producer version 2>$null | Select-Object -First 1)
        if ($verOut -match '\d+\.\d+[0-9A-Za-z.\-]*') { $producerVer = $matches[0] }
    } catch { $producerVer = $null }
}

Write-Host ('  ' + $s.SearchVer + '  ') -ForegroundColor DarkGray -NoNewline
if ($producerOk) {
    $verLabel = if ($producerVer) { "Version $producerVer" } else { $s.Found }
    Write-Host "$($s.ProducerName) - $verLabel" -ForegroundColor Green -NoNewline
    Write-Host "  $TICK" -ForegroundColor Green
} else {
    Write-Host "$($s.ProducerName) - $($s.Missing)" -ForegroundColor Red
}
Write-Host ""

if (-not $producerOk) {
    Show-Head $s.ToolsMissTitle DarkYellow
    Show-Item $s.ToolsMiss1 Yellow
    Show-Item $LINK_TOOLS Cyan
    Show-Item $s.ToolsMiss2 White
    Show-Item $LINK_WIKI  Cyan
    Show-Item $s.ToolsMiss3 Yellow
    Write-Host ""
    Show-Item $s.BrowserWarn DarkYellow
    Show-Rule
    Write-Host ""
    Wait-EnterOrTimeout 30
    exit 1
}

# ── Locate application XML (this script lives in the release "data" folder) ───────
# The wrapper passes -Xml '<targetName>.xml'. If that name is missing (or none was
# passed), fall back to auto-discovery so this works out-of-the-box for every OAM.
$dataDir = $PSScriptRoot
$xmls = @()
if ($Xml) {
    $explicit = Join-Path $dataDir $Xml
    if (Test-Path -PathType Leaf $explicit) { $xmls = @(Get-Item $explicit) }
}
if ($xmls.Count -eq 0) {
    $xmls = @(Get-ChildItem (Join-Path $dataDir '*.xml') -Exclude 'content.xml' -ErrorAction SilentlyContinue)
}

if ($xmls.Count -eq 0) {
    Show-Head $s.NoXmlTitle DarkYellow
    Show-Item ($s.NoXml1 -f $dataDir) Red
    Show-Rule
    Write-Host ""
    Wait-EnterOrTimeout 30
    exit 1
}

if ($xmls.Count -eq 1) {
    $xmlFile = $xmls[0]
} else {
    Show-Head $s.SelectTitle
    $idxW = ([string]$xmls.Count).Length
    for ($i = 0; $i -lt $xmls.Count; $i++) {
        Write-Host ($COL_MK + '[' + ([string]($i + 1)).PadLeft($idxW) + ']  ') -ForegroundColor DarkGray -NoNewline
        Write-Host $xmls[$i].Name -ForegroundColor White
    }
    Write-Host ($COL_MK + '[' + 'X'.PadLeft($idxW) + ']  ') -ForegroundColor DarkGray -NoNewline
    Write-Host $s.Cancel -ForegroundColor DarkGray
    Show-Rule
    Write-Host ""
    $valid  = @('X') + (1..$xmls.Count | ForEach-Object { "$_" })
    $choice = Read-Choice "  $($s.SelectPrompt) [1-$($xmls.Count)/X]" $valid
    if ($choice -eq 'X') { Write-Host ""; exit 0 }
    $xmlFile = $xmls[[int]$choice - 1]
}

$filename = [System.IO.Path]::GetFileNameWithoutExtension($xmlFile)
# The wrapper may inject an explicit output name (versioned / variant-tagged, e.g. IP-Interface-Dev-v1.0.0);
# fall back to the XML basename when the engine is called directly.
$baseName = if ($OutName) { $OutName } else { $filename }
$outDir   = if ($OutDir) { $OutDir } else { $PSScriptRoot }
$outFile  = Join-Path $outDir "$baseName.knxprod"

# ── Build (OpenKNXproducer as background job) + bouncing ■ spinner on one line ───-
# Start-Process -PassThru + redirect is unreliable on PS 5.1 (.HasExited never flips
# -> hang), so run the producer as a Start-Job and poll its state instead.
$job = Start-Job -ScriptBlock {
    param($prod, $out, $xmlPath)
    $text = & $prod knxprod --NoXsd "--Output=$out" $xmlPath 2>&1 | Out-String
    [PSCustomObject]@{ Code = $LASTEXITCODE; Output = $text }
} -ArgumentList $producer, $outFile, $xmlFile.FullName

try { [Console]::CursorVisible = $false } catch {}   # hide the blinking caret during the spinner
$cells = 6; $pos = 0; $dir = 1
while ($job.State -eq 'Running') {
    $bar = (' ' * $cells).ToCharArray()
    $bar[$pos] = [char]0x25A0   # ■ (console-safe)
    Write-Host ("`r  [") -ForegroundColor DarkGray -NoNewline
    Write-Host (-join $bar) -ForegroundColor Green   -NoNewline
    Write-Host ']  '        -ForegroundColor DarkGray -NoNewline
    Write-Host ($s.Building -f $baseName) -ForegroundColor White -NoNewline
    if ($pos -ge $cells - 1) { $dir = -1 } elseif ($pos -le 0) { $dir = 1 }
    $pos += $dir
    Start-Sleep -Milliseconds 120
}
try { [Console]::CursorVisible = $true } catch {}

$res         = Receive-Job $job
Remove-Job $job
$code        = $res.Code
$producerOut = "$($res.Output)"
# Clear the whole spinner+message line: the "… bitte warten …" text is often longer
# than a fixed width (long versioned base names), so wipe the full console line width
# (fallback for a redirected console with no window) or leftover tail chars remain.
$clearW = try { [Console]::WindowWidth - 1 } catch { 100 }
Write-Host ("`r" + (' ' * $clearW) + "`r") -NoNewline   # clear the spinner+message line

# ── Result ───────────────────────────────────────────────────────────────────────
if ($code -eq 0 -and (Test-Path -PathType Leaf $outFile)) {
    # Final build line: full green bar + FERTIG!
    Write-Host ('  [') -ForegroundColor DarkGray -NoNewline
    Write-Host ([string][char]0x25A0 * $cells) -ForegroundColor Green -NoNewline
    Write-Host ']  ' -ForegroundColor DarkGray -NoNewline
    Write-Host ($s.BuildDone -f $baseName) -ForegroundColor Green
    Write-Host ""
    Show-Rule
    Write-Host ""
    Write-Host ('  ' + ($s.DoneText -f $baseName)) -ForegroundColor Green
    Write-Host ""
    $sizeKb = [Math]::Round((Get-Item $outFile).Length / 1KB, 1)
    Show-Field $s.DoneFile $outFile
    Show-Field $s.DoneSize "$sizeKb KB"
    if ($AppVersion) { Show-Field $s.DoneVer $AppVersion }
    if ($ShowWarnings) {
        $warns = @($producerOut -split "`r?`n" | Where-Object { $_ -match '(?i)warn' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
        if ($warns.Count -gt 0) {
            Write-Host ('  ' + ($s.Hints -f $warns.Count)) -ForegroundColor DarkYellow
            foreach ($warnLine in ($warns | Select-Object -First $MaxWarnings)) {
                Write-Host ('    ' + [char]0x2022 + ' ' + $warnLine) -ForegroundColor DarkYellow
            }
            if ($warns.Count -gt $MaxWarnings) {
                Write-Host ('    ' + ($s.HintsMore -f ($warns.Count - $MaxWarnings))) -ForegroundColor DarkYellow
            }
        }
    }
    Write-Host ""
    Show-Head $s.NextTitle
    Show-Item $s.Next1 Cyan
    Show-Item $s.Next2
    Show-Item $s.Next3 DarkGray
    Show-Rule
    # Disclaimer - shown on success only, i.e. exactly when a product database was produced.
    Write-Host ""
    Show-Head $s.NoteTitle DarkYellow
    foreach ($noteLine in $s.NoteBody) { Show-Text $noteLine }
    foreach ($noteLine in $s.NoteOwn) { Show-Text $noteLine White }
    Show-Rule
    if ($Detailed) {
        Write-Host ""
        Show-Head $s.ProducerLog
        Show-Item ("$producer knxprod --NoXsd --Output=`"$outFile`" `"$($xmlFile.FullName)`"") DarkGray
        Show-Rule
        foreach ($logLine in ($producerOut -split "`r?`n")) {
            if ($logLine.TrimEnd()) { Write-Host ($COL_MK + $logLine.TrimEnd()) -ForegroundColor DarkGray }
        }
        Show-Rule
    }
} else {
    Show-Head $s.ErrTitle Red
    foreach ($line in ($producerOut -split "`r?`n")) {
        if ($line.Trim()) { Write-Host ($COL_MK + $line.Trim()) -ForegroundColor Red }
    }
    Show-Rule
}

Write-Host ""
Wait-EnterOrTimeout $ExitTimeout
if ($code -ne 0) { exit 1 }
