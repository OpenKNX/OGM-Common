#!/usr/bin/env pwsh
<#
Open ■
┬────┴  Build-knxprod
■ KNX   2026 OpenKNX

.SYNOPSIS
    Builds a .knxprod file from the application XML using OpenKNXproducer.

.DESCRIPTION
    Cross-platform aware:
      - Windows        : full build (OpenKNXproducer needs ETS / Falcon SDK -> Windows only)
      - macOS / Linux  : shows a friendly notice + download links, then exits (no ETS)

    Targets Windows PowerShell 5.1 (Win10 default) and PowerShell 7+ (macOS/Linux).

.PARAMETER Lang
    UI language: DE (default) or EN. Auto-detected from culture/LANG when omitted.

.PARAMETER Help
    Show usage information and exit.

.EXAMPLE
    .\Build-knxprod.ps1
    .\Build-knxprod.ps1 -Lang EN

.LINK
    https://github.com/OpenKNX/OpenKNXproducer
#>
param(
    [string]$Lang = "",
    [Alias("h")]
    [switch]$Help
)

# ── Config (hier anpassbar) ──────────────────────────────────────────────────────
$ShowWarnings = $true       # OpenKNXproducer-Warnungen in der Fertig-Box anzeigen (true/false)
$MaxWarnings  = 10          # max. angezeigte Warnzeilen (Rest als "… und N weitere")
$checkVersion = '3.4.0'     # minimal benötigte OpenKNXproducer-Version

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
        Missing        = 'nicht gefunden'
        Outdated       = 'veraltet'
        ProducerName   = 'OpenKNXproducer'
        SearchVer      = 'Suche Version:'
        WinOnlyTitle   = 'knxprod-Erstellung nur unter Windows'
        WinOnly1       = 'OpenKNXproducer benötigt die ETS bzw. das Falcon SDK – das gibt es nur für Windows.'
        WinOnly2       = 'Unter macOS / Linux kann die .knxprod daher nicht erstellt werden.'
        WhatCanTitle   = 'Was du tun kannst'
        WhatCan1       = 'Die Applikations-XML liegt bereit – auf einem Windows-PC bauen.'
        WhatCan2       = 'Oder einen Windows-Runner / CI für den knxprod-Schritt verwenden.'
        ToolsMissTitle = 'OpenKNX-Tools fehlen oder sind veraltet'
        ToolsMiss1     = 'Bitte das neuste Tools-Paket herunterladen (mindestens Version {0}):'
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
        Hints          = 'Hinweise ({0}):'
        HintsMore      = '… und {0} weitere'
        ErrTitle       = 'Fehler beim Erstellen der knxprod'
        ExitHint       = '[Enter] Beenden – schließt automatisch in {0,2} s'
    }
    EN = @{
        Title          = 'Knxprod Builder - OpenKNX.de'
        Missing        = 'not found'
        Outdated       = 'outdated'
        ProducerName   = 'OpenKNXproducer'
        SearchVer      = 'Checking version:'
        WinOnlyTitle   = 'Building knxprod is Windows-only'
        WinOnly1       = 'OpenKNXproducer requires ETS / the Falcon SDK, which exist for Windows only.'
        WinOnly2       = 'On macOS / Linux the .knxprod therefore cannot be created.'
        WhatCanTitle   = 'What you can do'
        WhatCan1       = 'The application XML is ready – build it on a Windows PC.'
        WhatCan2       = 'Or use a Windows runner / CI for the knxprod step.'
        ToolsMissTitle = 'OpenKNX tools are missing or outdated'
        ToolsMiss1     = 'Please download the latest tools package (at least version {0}):'
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
        Hints          = 'Notes ({0}):'
        HintsMore      = '… and {0} more'
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
$TICK   = [char]0x2713                 # ✓ (light check; if Consolas shows a box, swap for 'OK')

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

# Key/value detail line:  Label  value   (label padded so values align).
function Show-Field($label, $value) {
    Write-Host ('  ' + ([string]$label).PadRight(7)) -ForegroundColor DarkGray -NoNewline
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
    OpenKNX_ShowLogo $s.Title
    Write-Host "USAGE:" -ForegroundColor Yellow
    Write-Host "  .\Build-knxprod.ps1 [-Lang DE|EN] [-Help]"
    Write-Host ""
    Write-Host "OPTIONS:" -ForegroundColor Yellow
    Write-Host "  -Lang DE|EN   " -NoNewline -ForegroundColor White; Write-Host "UI language (default: auto, DE)" -ForegroundColor DarkGray
    Write-Host "  -Help, -h     " -NoNewline -ForegroundColor White; Write-Host "Show this help" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "Builds <app>.knxprod from the XML in ../data via OpenKNXproducer (Windows only)." -ForegroundColor DarkGray
    Write-Host ""
}

# ══════════════════════════════════════════════════════════════════════════════════
# Main
# ══════════════════════════════════════════════════════════════════════════════════
if ($Help) { Show-Help; exit 0 }

Clear-Host
OpenKNX_ShowLogo $s.Title

# ── macOS / Linux: ETS not available -> notice + links, then exit ────────────────-
if (-not $IsWindows) {
    Show-Head $s.WinOnlyTitle DarkYellow
    Show-Item $s.WinOnly1 DarkYellow
    Show-Item $s.WinOnly2 DarkYellow
    Write-Host ""
    Show-Head $s.WhatCanTitle
    Show-Item $s.WhatCan1
    Show-Item $s.WhatCan2
    Write-Host ""
    Show-Head $s.LinksTitle
    Show-Item $LINK_TOOLS Cyan
    Show-Item $LINK_WIKI  Cyan
    Show-Rule
    Write-Host ""
    Wait-EnterOrTimeout 30
    exit 2
}

# ── Tool detection (Windows) ─────────────────────────────────────────────────────
$producer    = Join-Path (Join-Path $HOME 'bin') 'OpenKNXproducer.exe'
$producerOk  = Test-Path -PathType Leaf $producer
$producerVer = $null
if ($producerOk) {
    try {
        $verOut = (& $producer version 2>$null | Select-Object -First 1)
        $producerVer = [System.Version](($verOut -split ' ')[1])
        if ($producerVer -lt [System.Version]$checkVersion) { $producerOk = $false }
    } catch { $producerOk = $false }
}

Write-Host ('  ' + $s.SearchVer + '  ') -ForegroundColor DarkGray -NoNewline
if ($producerOk) {
    Write-Host "$($s.ProducerName) - Version $producerVer" -ForegroundColor Green -NoNewline
    Write-Host "  $TICK" -ForegroundColor Green
} else {
    $pInfo = if ($producerVer) { "$($s.Outdated) (v$producerVer)" } else { $s.Missing }
    Write-Host "$($s.ProducerName) - $pInfo" -ForegroundColor Red
}
Write-Host ""

if (-not $producerOk) {
    Show-Head $s.ToolsMissTitle DarkYellow
    Show-Item ($s.ToolsMiss1 -f $checkVersion) Yellow
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

# ── Locate application XML ───────────────────────────────────────────────────────
$dataDir = Join-Path (Join-Path $PSScriptRoot '..') 'data'
$xmls = @(Get-ChildItem (Join-Path $dataDir '*.xml') -Exclude 'content.xml' -ErrorAction SilentlyContinue)

if ($xmls.Count -eq 0) {
    Show-Head $s.NoXmlTitle DarkYellow
    Show-Item ($s.NoXml1 -f $dataDir) Red
    Show-Rule
    Write-Host ""
    Wait-EnterOrTimeout 30
    exit 1
}

if ($xmls.Count -eq 1) {
    $xml = $xmls[0]
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
    $xml = $xmls[[int]$choice - 1]
}

$filename = [System.IO.Path]::GetFileNameWithoutExtension($xml)
$outFile  = Join-Path $PSScriptRoot "$filename.knxprod"

# ── Build (OpenKNXproducer as background job) + bouncing ■ spinner on one line ───-
# Start-Process -PassThru + redirect is unreliable on PS 5.1 (.HasExited never flips
# -> hang), so run the producer as a Start-Job and poll its state instead.
$job = Start-Job -ScriptBlock {
    param($prod, $out, $xmlPath)
    $text = & $prod knxprod --NoXsd "--Output=$out" $xmlPath 2>&1 | Out-String
    [PSCustomObject]@{ Code = $LASTEXITCODE; Output = $text }
} -ArgumentList $producer, $outFile, $xml.FullName

try { [Console]::CursorVisible = $false } catch {}   # hide the blinking caret during the spinner
$cells = 6; $pos = 0; $dir = 1
while ($job.State -eq 'Running') {
    $bar = (' ' * $cells).ToCharArray()
    $bar[$pos] = [char]0x25A0   # ■ (console-safe)
    Write-Host ("`r  [") -ForegroundColor DarkGray -NoNewline
    Write-Host (-join $bar) -ForegroundColor Green   -NoNewline
    Write-Host ']  '        -ForegroundColor DarkGray -NoNewline
    Write-Host ($s.Building -f $filename) -ForegroundColor White -NoNewline
    if ($pos -ge $cells - 1) { $dir = -1 } elseif ($pos -le 0) { $dir = 1 }
    $pos += $dir
    Start-Sleep -Milliseconds 120
}
try { [Console]::CursorVisible = $true } catch {}

$res         = Receive-Job $job
Remove-Job $job
$code        = $res.Code
$producerOut = "$($res.Output)"
Write-Host ("`r" + (' ' * ($W + 8)) + "`r") -NoNewline   # clear the spinner+message line

# ── Result ───────────────────────────────────────────────────────────────────────
if ($code -eq 0 -and (Test-Path -PathType Leaf $outFile)) {
    # Final build line: full green bar + FERTIG!
    Write-Host ('  [') -ForegroundColor DarkGray -NoNewline
    Write-Host ([string][char]0x25A0 * $cells) -ForegroundColor Green -NoNewline
    Write-Host ']  ' -ForegroundColor DarkGray -NoNewline
    Write-Host ($s.BuildDone -f $filename) -ForegroundColor Green
    Write-Host ""
    Show-Rule
    Write-Host ""
    Write-Host ('  ' + ($s.DoneText -f $filename)) -ForegroundColor Green
    Write-Host ""
    $sizeKb = [Math]::Round((Get-Item $outFile).Length / 1KB, 1)
    Show-Field $s.DoneFile $outFile
    Show-Field $s.DoneSize "$sizeKb KB"
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
    Show-Rule
} else {
    Show-Head $s.ErrTitle Red
    foreach ($line in ($producerOut -split "`r?`n")) {
        if ($line.Trim()) { Write-Host ($COL_MK + $line.Trim()) -ForegroundColor Red }
    }
    Show-Rule
}

Write-Host ""
Wait-EnterOrTimeout 30
if ($code -ne 0) { exit 1 }
