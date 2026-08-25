#!/usr/bin/env pwsh
<#
Open ■
┬────┴  KNX-Upload-Firmware-Generic
■ KNX   2026 OpenKNX - Erkan Çolak

FILEPATH: OGM-Common/scripts/setup/reusable/data/KNX-Upload-Firmware-Generic.ps1
   ships as: release/data/KNX-Upload-Firmware-Generic.ps1

.SYNOPSIS
    Updates an OpenKNX device over the KNX bus, using ftc — the native OpenKNX file-transfer client.

.DESCRIPTION
    This script does the two things ftc cannot do for itself, and nothing else:

      it finds ftc — the copy shipped with this release and the one already installed, picks the one
      that runs on this machine, and offers to install the newer of the two;

      it says what is about to happen, in the same header the USB and network scripts print.

    Everything after that belongs to "ftc knxota": finding the interface, picking the device, checking
    that the firmware really belongs to it, deciding between the full image and a difference to the
    release the device is running, asking the one confirmation and reading the version back afterwards.
    None of it is repeated here. Two places asking the same questions drift apart, and then the answers
    stop matching.

.PARAMETER FirmwareFile
    Firmware to install, relative to the calling script. Passed in by the per-device wrapper.

.PARAMETER Ip
    KNXnet/IP interface. Omitted: knxota searches the bus and offers what it finds.

.PARAMETER Pa
    Individual address of the device, e.g. 1.1.5. Omitted: knxota asks, or searches the line.

.PARAMETER From
    A previous release — its folder or the <name>.app.bin inside it. Given: only the difference to that
    release is sent. Omitted: knxota decides, and asks if it can save the time.

.PARAMETER NoDelta
    Always send the whole image, even where a difference would do.

.PARAMETER Lang
    DE or EN. Omitted: taken from the system locale, defaulting to DE.

.EXAMPLE
    ./KNX-Upload-Firmware.ps1
    # knxota asks for what it cannot find out by itself

.EXAMPLE
    ./KNX-Upload-Firmware.ps1 -Ip 192.168.1.50 -Pa 1.1.5 -From ../../../OpenKNX-NeoPixel-0.7.0
    # sends only the difference to 0.7.0, without a single question

.NOTES
    AUTHOR : Erkan Çolak
    Runs on Windows PowerShell 5.1 and PowerShell 7+, on Windows, macOS, Linux and Raspberry Pi.

.LINK
    https://wiki.openknx.de
#>

param(
    [Parameter(Position = 0)][string]$FirmwareFile,
    [string]$Ip = "",
    [string]$Pa = "",
    [string]$From = "",
    [switch]$NoDelta,
    [string]$Lang = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Windows PowerShell 5.1 has no $IsWindows/$IsMacOS/$IsLinux -- define them. 5.1 exists on Windows only,
# which is what makes the fallback correct rather than merely convenient.
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
    EN = @{
        Way            = 'KNX bus'
        WayHint        = 'via ftc {0}'
        Note           = 'The device is out of reach for about 30 seconds right at the end.'
        FtcInstalled   = '  ftc {0}   installed at {1}'
        FtcNone        = '  ftc is not installed'
        FtcShipped     = '  ftc {0}   shipped with this release'
        FtcNewer       = '  The copy in this release is newer than the installed one.'
        FtcRebuilt     = '  Same version, but the copy in this release was built later ({0} vs {1}).'
        FtcBuilt       = '   built {0}'
        FtcInstallAsk  = 'install it? (y/n)'
        FtcNotInstAsk  = 'ftc is not installed. install it now? (y/n)'
        FtcMissing     = '  ftc was not found.'
        FtcMissing1    = '    An update over the KNX bus is driven by ftc, the OpenKNX file-transfer client.'
        FtcMissing2    = '    It is a separate tool and is not part of every firmware release.'
        FtcMissing3    = '    Get it from the OpenKNX project (see wiki.openknx.de), put it anywhere on your PATH,'
        FtcMissing4    = "    or run 'ftc install' once and it will place itself there."
        FtcMissing5    = '    Until then this device can be updated over USB (USB-Upload-Firmware.ps1)'
        FtcMissing6    = '    or, if it is on the network, with ArduinoOTA.'
        NoFirmware     = '  no firmware given - this script is normally started by KNX-Upload-Firmware.ps1'
        NotFound       = '  firmware not found: {0}'
        HandOver       = '  handing over to knxota ...'
        Done           = '  done.'
        Cancelled      = '  cancelled - nothing was written to the device.'
        Refused        = '  this firmware does not belong to this device - nothing was written.'
        NoWrite        = '  the device is not accepting writes - nothing was written.'
        NotComplete    = '  the transfer did not complete - the reason is in the result box above.'
        NoReturn       = '  the device has not answered since the restart - see the output above.'
        Failed         = '  the update did not complete - see the output above.'
    }
    DE = @{
        Way            = 'KNX-Bus'
        WayHint        = 'über ftc {0}'
        Note           = 'Das Gerät ist ganz am Ende etwa 30 Sekunden nicht erreichbar.'
        FtcInstalled   = '  ftc {0}   installiert unter {1}'
        FtcNone        = '  ftc ist nicht installiert'
        FtcShipped     = '  ftc {0}   in diesem Release enthalten'
        FtcNewer       = '  Die Fassung in diesem Release ist neuer als die installierte.'
        FtcRebuilt     = '  Gleiche Version, aber die Fassung im Release ist später gebaut ({0} statt {1}).'
        FtcBuilt       = '   gebaut {0}'
        FtcInstallAsk  = 'installieren? (j/n)'
        FtcNotInstAsk  = 'ftc ist nicht installiert. jetzt installieren? (j/n)'
        FtcMissing     = '  ftc wurde nicht gefunden.'
        FtcMissing1    = '    Ein Update über den KNX-Bus läuft über ftc, den OpenKNX-Dateitransfer-Client.'
        FtcMissing2    = '    Das ist ein eigenständiges Werkzeug und nicht in jedem Firmware-Release enthalten.'
        FtcMissing3    = '    Vom OpenKNX-Projekt holen (siehe wiki.openknx.de), irgendwo in den PATH legen,'
        FtcMissing4    = "    oder einmal 'ftc install' aufrufen - dann legt es sich selbst dorthin."
        FtcMissing5    = '    Bis dahin lässt sich dieses Gerät über USB aktualisieren (USB-Upload-Firmware.ps1)'
        FtcMissing6    = '    oder, wenn es am Netzwerk hängt, mit ArduinoOTA.'
        NoFirmware     = '  keine Firmware angegeben - dieses Skript startet normalerweise KNX-Upload-Firmware.ps1'
        NotFound       = '  Firmware nicht gefunden: {0}'
        HandOver       = '  Übergabe an knxota ...'
        Done           = '  fertig.'
        Cancelled      = '  abgebrochen - es wurde nichts auf das Gerät geschrieben.'
        Refused        = '  diese Firmware gehört nicht zu diesem Gerät - es wurde nichts geschrieben.'
        NoWrite        = '  das Gerät nimmt keine Schreibzugriffe an - es wurde nichts geschrieben.'
        NotComplete    = '  die Übertragung wurde nicht abgeschlossen - der Grund steht im Ergebnis-Kasten oben.'
        NoReturn       = '  das Gerät hat sich seit dem Neustart nicht gemeldet - siehe die Ausgabe oben.'
        Failed         = '  das Update wurde nicht abgeschlossen - siehe die Ausgabe oben.'
    }
}
$s = $_strings[$_lang]

# ─── the shared header ─────────────────────────────────────────────────────────────────────────────
# Dot-sourced, not copied: the three upload scripts print the same header, and a copy per script is how
# they drifted apart in the first place. It also holds the ftc lookup, so it is required rather than
# optional -- a fallback that cannot find ftc would leave this route dead while looking like it works.
$uiPath = Join-Path $PSScriptRoot "OpenKNX-UI-Generic.ps1"
if (-not (Test-Path -PathType Leaf $uiPath)) {
    Write-Host ""
    Write-Host "  OpenKNX-UI-Generic.ps1 is missing next to this script." -ForegroundColor Red
    Write-Host "  $uiPath" -ForegroundColor DarkGray
    Write-Host "  It ships in the same folder; this release package is incomplete." -ForegroundColor DarkGray
    Write-Host ""
    exit 1
}
. $uiPath

# ─── the firmware ──────────────────────────────────────────────────────────────────────────────────
if ($MyInvocation.PSCommandPath) { $callerDir = Split-Path -Parent $MyInvocation.PSCommandPath }
else { $callerDir = $PWD.Path }

if (-not $FirmwareFile) {
    OpenKNX_ShowLogo $s.Way
    Write-Host $s.NoFirmware -ForegroundColor Red
    exit 1
}
if (Test-Path -PathType Leaf $FirmwareFile) { $fw = (Resolve-Path $FirmwareFile).Path }
else { $fw = Join-Path $callerDir $FirmwareFile }
if (-not (Test-Path -PathType Leaf $fw)) {
    OpenKNX_ShowLogo $s.Way
    Write-Host ($s.NotFound -f $FirmwareFile) -ForegroundColor Red
    exit 1
}

# ─── ftc: find it before the header, because the header quotes its version ─────────────────────────
# Searched quietly first so the header comes out in one piece; what was found is reported right below it.
$roots = @()
foreach ($rel in @("../Tools", "../../Tools", "../../../Tools", ".")) {
    $roots += (Join-Path $PSScriptRoot $rel)
}
$found = OpenKNX_FindFtc -SearchDirs $roots
$ftc = ""
if ($found.Installed) { $ftc = $found.Installed }
elseif ($found.Shipped) { $ftc = $found.Shipped }
$ftcVer = ""
if ($found.InstalledVersion) { $ftcVer = $found.InstalledVersion }
elseif ($found.ShippedVersion) { $ftcVer = $found.ShippedVersion }

# ─── the header ────────────────────────────────────────────────────────────────────────────────────
$facts = OpenKNX_GetFirmwareFacts -FirmwarePath $fw -FtcExe $ftc -Lang $_lang
$hint = ""
if ($ftcVer) { $hint = ($s.WayHint -f $ftcVer) }
OpenKNX_ShowTitle -Way $s.Way -Lang $_lang
OpenKNX_ShowContext -Facts $facts -Way $s.Way -WayHint $hint -Note $s.Note -Lang $_lang

# ─── ftc: report, and offer the newer one ──────────────────────────────────────────────────────────
# The build stamp is shown alongside the version, because two builds of the same version are the normal
# case between releases and the version alone cannot tell them apart.
function Format-Build($d) {
    if ($null -eq $d) { return "" }
    return ($s.FtcBuilt -f $d.ToString('yyyy-MM-dd HH:mm'))
}
if ($found.Installed) {
    Write-Host ($s.FtcInstalled -f $found.InstalledVersion, $found.Installed) -NoNewline -ForegroundColor DarkGray
    Write-Host (Format-Build $found.InstalledBuild) -ForegroundColor DarkGray
}
else { Write-Host $s.FtcNone -ForegroundColor DarkYellow }
if ($found.Shipped) {
    Write-Host ($s.FtcShipped -f $found.ShippedVersion) -NoNewline -ForegroundColor DarkGray
    Write-Host (Format-Build $found.ShippedBuild) -ForegroundColor DarkGray
}

if (OpenKNX_ShippedIsNewer $found) {
    Write-Host ""
    if ((OpenKNX_CompareVersion $found.ShippedVersion $found.InstalledVersion) -eq 0 -and $found.Installed) {
        $newB = ""
        $oldB = ""
        if ($found.ShippedBuild) { $newB = $found.ShippedBuild.ToString('yyyy-MM-dd HH:mm') }
        if ($found.InstalledBuild) { $oldB = $found.InstalledBuild.ToString('yyyy-MM-dd HH:mm') }
        Write-Host ($s.FtcRebuilt -f $newB, $oldB) -ForegroundColor Yellow
    }
    else { Write-Host $s.FtcNewer -ForegroundColor Yellow }
    if ((OpenKNX_ReadChoice $s.FtcInstallAsk "j") -match '^(y|j)') {
        & $found.Shipped install
        $found = OpenKNX_FindFtc -SearchDirs $roots
        if ($found.Installed) { $ftc = $found.Installed } else { $ftc = $found.Shipped }
    }
    else { $ftc = $found.Shipped }
}
elseif (-not $ftc -and $found.Shipped) {
    Write-Host ""
    if ((OpenKNX_ReadChoice $s.FtcNotInstAsk "j") -match '^(y|j)') {
        & $found.Shipped install
        $found = OpenKNX_FindFtc -SearchDirs $roots
    }
    if ($found.Installed) { $ftc = $found.Installed } else { $ftc = $found.Shipped }
}

if (-not $ftc) {
    # ftc is a separate OpenKNX tool with its own release cycle. A firmware release may well ship without
    # it, so this is a normal situation and not a defect -- say where to get it instead of failing blankly.
    Write-Host ""
    Write-Host $s.FtcMissing -ForegroundColor Yellow
    Write-Host $s.FtcMissing1
    Write-Host $s.FtcMissing2
    Write-Host ""
    Write-Host $s.FtcMissing3 -ForegroundColor DarkGray
    Write-Host $s.FtcMissing4 -ForegroundColor DarkGray
    Write-Host ""
    Write-Host $s.FtcMissing5 -ForegroundColor DarkGray
    Write-Host $s.FtcMissing6 -ForegroundColor DarkGray
    Write-Host ""
    exit 1
}

# ─── hand over ─────────────────────────────────────────────────────────────────────────────────────
# No password is passed: knxota asks for one itself when the target turns out to be protected, at the
# moment it is needed. Handing it in up front would put it in the shell history for a device that may
# not even want it.
Write-Host ""
Write-Host $s.HandOver -ForegroundColor Cyan
Write-Host ""

$callArgs = @()
if ($Ip) { $callArgs += @('--ip', $Ip) }
if ($Pa) { $callArgs += $Pa }
$callArgs += @('knxota', $fw)
if ($From) { $callArgs += @('--from', $From) }
if ($NoDelta) { $callArgs += '--no-delta' }
$callArgs += @('--lang', $_lang.ToLower())

& $ftc @callArgs
$rc = $LASTEXITCODE

Write-Host ""
# knxota says WHY it stopped, and a deliberate cancel is not a failure. Reporting every non-zero code as
# "the update did not complete" told someone who had just pressed q that something had gone wrong.
switch ($rc) {
    0 { Write-Host $s.Done -ForegroundColor Green }
    130 { }  # knxota already said it was cancelled, and said it more precisely. One line is enough.
    1 { Write-Host $s.Refused -ForegroundColor Yellow }
    3 { Write-Host $s.NoWrite -ForegroundColor Yellow }
    4 { Write-Host $s.NotComplete -ForegroundColor Yellow }
    6 { Write-Host $s.NoReturn -ForegroundColor Red }
    default { Write-Host $s.Failed -ForegroundColor Red }
}
exit $rc
