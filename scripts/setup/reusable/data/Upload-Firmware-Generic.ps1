#!/usr/bin/env pwsh
<#
Open ■
┬────┴  Upload-Firmware-Generic
■ KNX   2025 OpenKNX - Erkan Çolak

FILEPATH: lib/OGM-Common/scripts/setup/reusable/data/Upload-Firmware-Generic.ps1

.SYNOPSIS
    Unified firmware uploader for RP2040/RP2350 and ESP32.

.DESCRIPTION
    Flashes firmware to a connected device. Chip type is auto-detected from the
    firmware file extension (.uf2 → RP2040/RP2350, .bin/.factory.bin → ESP32).
    Supports interactive device selection, auto-detect via plug/unplug, and
    flashing multiple devices in a row.

.PARAMETER FirmwareName
    Path to the firmware file (relative to current directory).

.PARAMETER Chip
    Override chip type: RP2040 or ESP32. Auto-detected from extension if omitted.

.PARAMETER Lang
    Display language: DE or EN. Auto-detected from system locale if omitted.

.PARAMETER Multi
    Flash multiple devices in a row without restarting the script.

.EXAMPLE
    .\Upload-Firmware-Generic.ps1 firmware.uf2

.EXAMPLE
    .\Upload-Firmware-Generic.ps1 firmware.bin -Chip ESP32 -Lang EN -Multi

.PARAMETER DebugSerial
    Print low-level serial debug output when reading device information.

.EXAMPLE
    .\Upload-Firmware-Generic.ps1 firmware.uf2 -DebugSerial
#>
param(
    [Parameter(Position=0)]
    [string]$FirmwareName,
    [string]$Chip = "",     # RP2040 | ESP32  (auto-detected from extension if empty)
    [string]$Lang = "",
    [switch]$Multi,         # flash multiple devices in a row
    [switch]$DebugSerial = $false    # show serial debug output when reading device info
)

# Platform detection – ensures compatibility with PowerShell 5.1 on Windows
if ($null -eq (Get-Variable -Name 'IsMacOS' -ErrorAction SilentlyContinue)) {
    $IsMacOS  = $false
    $IsLinux  = $false
    $IsWindows = $true
}

# ─── Chip auto-detection ───────────────────────────────────────────────────────
$chipWasAutoDetected = (-not $Chip)
if (-not $Chip) {
    if     ($FirmwareName -match '\.uf2$')              { $Chip = 'RP2040' }
    elseif ($FirmwareName -match '\.(bin|factory\.bin)$') { $Chip = 'ESP32'  }
}
$Chip = $Chip.ToUpper()

# ─── Language ──────────────────────────────────────────────────────────────────
if (-not $Lang) {
    $sysLang = if ($IsWindows) { (Get-Culture).TwoLetterISOLanguageName } else { "$env:LANG" }
    $Lang = if ($sysLang -match '^en') { 'EN' } else { 'DE' }
}
$_lang = $Lang.ToUpper()
if ($_lang -notin @('DE', 'EN')) { $_lang = 'DE' }

$_strings = @{
    EN = @{
        Platform_macOS     = "Platform: macOS detected"
        Platform_Linux     = "Platform: Linux detected"
        Platform_Windows   = "Platform: Windows detected"
        ChipUnknown        = "Unknown chip type. Use -Chip RP2040 or -Chip ESP32, or pass a .uf2 / .bin firmware file."
        Firmware           = "Firmware: {0}"
        FirmwareFile       = "  File    :  {0}"
        FirmwarePath       = "  Path    :  {0}"
        FirmwareWarn       = "  Warning :  Make sure the connected device matches this firmware file!"
        FirmwareHint       = "Please make sure the connected device matches this firmware (wrong device = wrong firmware flashed)."
        FirmwareNotFound   = "Firmware file not found: {0}"
        PressEnter         = "Press Enter to exit"
        SearchDevices      = "Searching for devices..."
        NoDeviceFound      = "No device found."
        DeviceAutoSelected = "Device found: {0}"
        MultipleDevices    = "Multiple devices found – please select:"
        SelectManual       = "Selection (1-{0})"
        InvalidInput       = "Invalid input. Selection (1-{0})"
        OptAgain           = "Auto-detect (plug in device, or unplug and replug)"
        CancelKey          = "[ESC/Q] Cancel"
        ChoicePrompt       = "Choice"
        WaitUnplug         = "Please UNPLUG the device NOW..."
        WaitingUnplug      = "  Waiting for unplug... {0}s  "
        NoUnplug           = "No device unplugged detected - manual selection required."
        DeviceUnplugged    = "Device unplugged: {0}"
        WaitReplug         = "Please PLUG IN a new device  –or–  unplug and replug an existing one..."
        WaitingReplug      = "  Waiting for device... {0}s  "
        DeviceDetected     = "Device detected: {0}"
        NoReplug           = "Device not detected in time."
        FlashAnother       = "Flash another device? [Y/X]"
        FlashedCount       = "{0} device(s) flashed so far."
        FlashedTotal       = "Total: {0} device(s) flashed."
        # RP2040-specific
        DeviceInBootsel    = "(BOOTSEL mode – ready to flash)"
        DeviceSerial       = "(running – will be put into BOOTSEL mode)"
        MultiBootsel       = "Multiple BOOTSEL volumes found - please select:"
        PortFound          = "Port found: {0}"
        NotifyFirmware     = "Sending update signal to device..."
        AttemptBootsel     = "Attempting to set port {0} into BOOTSEL mode..."
        Installing         = "Device found, installing firmware `"{0}`"..."
        FlashError         = "Error writing firmware! Device may not be mounted correctly."
        Done               = "Done!"
        ManualInstrTitle   = "Tip: Put the RP2040/RP2350 into BOOTSEL mode manually"
        ManualInstr        = @(
            "With reset button:",
            "  Hold BOOTSEL  →  press RESET  →  release both.",
            "",
            "Without reset button:",
            "  Unplug USB and KNX  →  hold BOOTSEL  →  reconnect USB  →  release BOOTSEL.",
            "",
            "The device will appear as a drive. Then run this script again."
        )
        Esp32InstrTitle    = "Tip: ESP32 not found? Check connection"
        Esp32Instr         = @(
            "Make sure the device is connected via USB.",
            "",
            "If esptool cannot connect, try manual flash mode:",
            "  Hold BOOT  →  press RESET  →  release both.",
            "",
            "Then run this script again."
        )
        # ESP32-specific
        DeviceEsp32Serial  = "(ESP32 serial port)"
        EsptoolNotFound    = @(
            "esptool not found. Please install it or place it in ~/bin/:",
            "  pip install esptool",
            "Or download the OpenKNX tools package:",
            "  https://github.com/OpenKNX/OpenKNXproducer/releases"
        )
        FlashingEsp32      = "Flashing firmware via esptool..."
        FlashDoneEsp32     = "Done!"
        FlashErrorEsp32    = "esptool returned an error. Flash may have failed."
        # Console / device info
        OptCancel          = "Cancel"
        OptFlashBootsel    = "Flash this device (will be put into BOOTSEL mode)"
        OptFlashEsp32      = "Flash this device"
        OptInfo            = "Read device information"
        OptSkip            = "Skip"
        InfoWaitPort       = "  Waiting for device port... {0}s  "
        InfoNoPort         = "No serial port found after flash."
        InfoConnecting     = "Connecting to {0}..."
        InfoTimeout        = "No response from device (timeout)."
        InfoError          = "Serial port error: {0}"
        InfoHeader         = "─── Device Information ──────────────────────────────────────────"
        InfoFooter         = "────────────────────────────────────────────────────────────────"
        InfoReadingDevices  = "Reading device info for serial devices..."
        ConfirmFlash        = "Flash this device? [Y/N]"
        PsVersionRequired   = "PowerShell 5.1 or higher required."
        PsVersionFound      = "Found: PowerShell {0}"
        PsVersionInstall    = "Please install PowerShell 5.1 or PowerShell 7+."
        # File picker
        SelectFwTitle       = "Select Firmware File"
        SelectFwFolder      = "Folder: {0}"
        SelectFwNoFiles     = "No subdirectories or firmware files (.uf2 / .bin) found."
        SelectFwLabelUp     = "..  (go up)"
        SelectFwHint        = "Up/Down = navigate   Enter = select / enter folder   Backspace = up   ESC = cancel"
        SelectFwCancel      = "Cancel"
        # Wipe / Erase (Dev only)
        WipeTitle           = "Wipe / Erase  -  [DEV]"
        WipeDevOnly         = "Only visible in developer mode!"
        WipeSelectDev       = "Select device:"
        WipeSelectedDev     = "Selected device:"
        WipeSelectCmd       = "Select erase command:"
        WipeEraseKnx        = "erase knx       -  Erase KNX parameters"
        WipeEraseOknx       = "erase openknx   -  Erase OpenKNX module data"
        WipeEraseFiles      = "erase files     -  Erase filesystem"
        WipeEraseAll        = "erase all       -  Erase EVERYTHING!"
        WipeInfo           = @(
            "This menu erases data on a connected OpenKNX device via the serial console.",
            "The device must be running and reachable via serial port.",
            "Four erase levels are available: KNX parameters, OpenKNX module data,",
            "filesystem, or everything at once."
        )
        WipeWarning         = "CAUTION: Erased data cannot be recovered! Use with care."
        WipeSearching       = "Searching for serial devices..."
        WipeConfirmAll      = "!! erase all -- ALL data will be permanently lost!"
        WipeConfirmPrompt   = "Are you sure? This cannot be undone! [J/N]"
        WipeSending         = "Sending '{0}' to {1}..."
        WipeWaiting         = "  Waiting for response...  {0}s  "
        WipeDone            = "Erase complete!"
        WipeError           = "No response from device or command failed."
        WipeRespHeader      = "--- Device Response -------------------------------------------"
        WipeRespFooter      = "---------------------------------------------------------------"
        WipeNoDevices       = "No serial devices found. Please connect a device first."
        OptBack             = "Back"
    }
    DE = @{
        Platform_macOS     = "Plattform: macOS erkannt"
        Platform_Linux     = "Plattform: Linux erkannt"
        Platform_Windows   = "Plattform: Windows erkannt"
        ChipUnknown        = "Unbekannter Chip-Typ. Bitte -Chip RP2040 oder -Chip ESP32 angeben, oder eine .uf2 / .bin Datei übergeben."
        Firmware           = "Firmware: {0}"
        FirmwareFile       = "  Datei   :  {0}"
        FirmwarePath       = "  Pfad    :  {0}"
        FirmwareWarn       = "  Hinweis :  Sicherstellen, dass das angeschlossene Gerät zu dieser Firmware-Datei passt!"
        FirmwareHint       = "Bitte sicherstellen, dass das angeschlossene Gerät zu dieser Firmware passt (falsches Gerät = falsche Firmware geflasht)."
        FirmwareNotFound   = "Firmware-Datei nicht gefunden: {0}"
        PressEnter         = "Drücke Enter zum Beenden"
        SearchDevices      = "Suche Gerät..."
        NoDeviceFound      = "Kein Gerät gefunden."
        DeviceAutoSelected = "Gerät gefunden: {0}"
        MultipleDevices    = "Mehrere Geräte gefunden – bitte auswählen:"
        SelectManual       = "Auswahl (1-{0})"
        InvalidInput       = "Ungültige Eingabe. Auswahl (1-{0})"
        OptAgain           = "Automatisch erkennen (Gerät einstecken oder aus-/einstecken)"
        CancelKey          = "[ESC/Q] Abbrechen"
        ChoicePrompt       = "Auswahl"
        WaitUnplug         = "Bitte das Gerät JETZT AUSSTECKEN..."
        WaitingUnplug      = "  Warte auf Ausstecken... {0}s  "
        NoUnplug           = "Kein Gerät ausgesteckt erkannt – manuelle Auswahl erforderlich."
        DeviceUnplugged    = "Gerät ausgesteckt: {0}"
        WaitReplug         = "Neues Gerät EINSTECKEN  –oder–  vorhandenes AUS-/EINSTECKEN..."
        WaitingReplug      = "  Warte auf Gerät... {0}s  "
        DeviceDetected     = "Gerät erkannt: {0}"
        NoReplug           = "Gerät nicht rechtzeitig erkannt."
        FlashAnother       = "Weiteres Gerät flashen? [J/X]"
        FlashedCount       = "Bisher {0} Gerät(e) geflasht."
        FlashedTotal       = "Insgesamt {0} Gerät(e) geflasht."
        # RP2040-spezifisch
        DeviceInBootsel    = "(BOOTSEL-Modus – bereit zum Flashen)"
        DeviceSerial       = "(läuft – wird in BOOTSEL-Modus versetzt)"
        MultiBootsel       = "Mehrere BOOTSEL-Laufwerke gefunden – bitte auswählen:"
        PortFound          = "Schnittstelle gefunden: {0}"
        NotifyFirmware     = "Sende Update-Signal an das Gerät..."
        AttemptBootsel     = "Versuche über Port {0} in den BOOTSEL-Modus zu versetzen..."
        Installing         = "Gerät gefunden, installiere Firmware `"{0}`"..."
        FlashError         = "Fehler beim Schreiben der Firmware! Gerät ggf. nicht korrekt gemountet."
        Done               = "Fertig!"
        ManualInstrTitle   = "Tipp: RP2040/RP2350 manuell in den BOOTSEL-Modus versetzen"
        ManualInstr        = @(
            "Mit Reset-Taste:",
            "  BOOTSEL halten  →  RESET drücken  →  beide loslassen.",
            "",
            "Ohne Reset-Taste:",
            "  USB und KNX trennen  →  BOOTSEL halten  →  USB verbinden  →  BOOTSEL loslassen.",
            "",
            "Das Gerät erscheint dann als Laufwerk. Skript anschließend erneut starten."
        )
        Esp32InstrTitle    = "Tipp: ESP32 nicht gefunden? Verbindung prüfen"
        Esp32Instr         = @(
            "Sicherstellen, dass das Gerät per USB verbunden ist.",
            "",
            "Falls esptool keine Verbindung bekommt, manuellen Flash-Modus versuchen:",
            "  BOOT halten  →  RESET drücken  →  beide loslassen.",
            "",
            "Danach Skript erneut starten."
        )
        # ESP32-spezifisch
        DeviceEsp32Serial  = "(ESP32 serieller Port)"
        EsptoolNotFound    = @(
            "esptool nicht gefunden. Bitte installieren oder in ~/bin/ ablegen:",
            "  pip install esptool",
            "Oder das OpenKNX-Tools-Paket herunterladen:",
            "  https://github.com/OpenKNX/OpenKNXproducer/releases"
        )
        FlashingEsp32      = "Flashe Firmware über esptool..."
        FlashDoneEsp32     = "Fertig!"
        FlashErrorEsp32    = "esptool meldet einen Fehler. Flash möglicherweise fehlgeschlagen."
        # Konsole / Geräteinformationen
        OptCancel          = "Abbrechen"
        OptFlashBootsel    = "Dieses Gerät flashen (wird in BOOTSEL-Modus versetzt)"
        OptFlashEsp32      = "Dieses Gerät flashen"
        OptInfo            = "Geräteinformationen abrufen"
        OptSkip            = "Überspringen"
        InfoWaitPort       = "  Warte auf Gerät-Port... {0}s  "
        InfoNoPort         = "Kein serieller Port nach dem Flash gefunden."
        InfoConnecting     = "Verbinde mit {0}..."
        InfoTimeout        = "Keine Antwort vom Gerät (Timeout)."
        InfoError          = "Serieller Port Fehler: {0}"
        InfoHeader         = "─── Geräteinformationen ─────────────────────────────────────────"
        InfoFooter         = "────────────────────────────────────────────────────────────────"
        InfoReadingDevices  = "Lese Geräteinformationen der seriellen Geräte..."
        ConfirmFlash        = "Dieses Gerät flashen? [J/N]"
        PsVersionRequired   = "PowerShell 5.1 oder höher erforderlich."
        PsVersionFound      = "Gefunden: PowerShell {0}"
        PsVersionInstall    = "Bitte PowerShell 5.1 oder PowerShell 7+ installieren."
        # Dateiauswahl
        SelectFwTitle       = "Firmware-Datei auswählen"
        SelectFwFolder      = "Ordner: {0}"
        SelectFwNoFiles     = "Keine Unterordner oder Firmware-Dateien (.uf2 / .bin) gefunden."
        SelectFwLabelUp     = "..  (hoch)"
        SelectFwHint        = "Hoch/Runter = navigieren   Enter = auswählen/öffnen   Backspace = hoch   ESC = abbrechen"
        SelectFwCancel      = "Abbrechen"
        # Wipe / Erase (nur Dev)
        WipeTitle           = "Wipe / Erase  -  [DEV]"
        WipeDevOnly         = "Nur im Entwicklermodus sichtbar!"
        WipeSelectDev       = "Gerät auswählen:"
        WipeSelectedDev     = "Ausgewähltes Gerät:"
        WipeSelectCmd       = "Erase-Befehl auswählen:"
        WipeEraseKnx        = "erase knx       -  KNX-Parameter löschen"
        WipeEraseOknx       = "erase openknx   -  OpenKNX Moduldaten löschen"
        WipeEraseFiles      = "erase files     -  Dateisystem löschen"
        WipeEraseAll        = "erase all       -  ALLES löschen!!"
        WipeInfo           = @(
            "Dieses Menü löscht Daten auf einem angeschlossenen OpenKNX-Gerät über die serielle Konsole.",
            "Das Gerät muss laufen und über einen seriellen Port erreichbar sein.",
            "Es stehen vier Löschstufen zur Verfügung: KNX-Parameter, OpenKNX-Moduldaten,",
            "Dateisystem oder alles auf einmal."
        )
        WipeWarning         = "ACHTUNG: Gelöschte Daten können NICHT wiederhergestellt werden! Mit Vorsicht verwenden."
        WipeSearching       = "Suche serielle Geräte..."
        WipeConfirmAll      = "!! erase all -- Alle Daten werden unwiderruflich gelöscht!"
        WipeConfirmPrompt   = "Wirklich fortfahren? Nicht rückgängig machbar! [J/N]"
        WipeSending         = "Sende '{0}' an {1}..."
        WipeWaiting         = "  Warte auf Antwort...  {0}s  "
        WipeDone            = "Erase abgeschlossen!"
        WipeError           = "Keine Antwort vom Gerät oder Befehl fehlgeschlagen."
        WipeRespHeader      = "--- Gerät-Antwort ---------------------------------------------"
        WipeRespFooter      = "---------------------------------------------------------------"
        WipeNoDevices       = "Keine seriellen Geräte gefunden. Bitte Gerät anschließen."
        OptBack             = "Zurück"
    }
}
$s = $_strings[$_lang]

# ─── Minimum PowerShell version check (requires PS 5.1 / Windows 10+) ─────────
if ($PSVersionTable.PSVersion.Major -lt 5 -or
    ($PSVersionTable.PSVersion.Major -eq 5 -and $PSVersionTable.PSVersion.Minor -lt 1)) {
    Write-Host $s.PsVersionRequired -ForegroundColor Red
    Write-Host ($s.PsVersionFound -f $PSVersionTable.PSVersion) -ForegroundColor Red
    Write-Host $s.PsVersionInstall -ForegroundColor DarkRed
    Read-Host $s.PressEnter
    exit 1
}
# ───────────────────────────────────────────────────────────────────────────────

function OpenKNX_ShowLogo($AddCustomText = $null) {
    Write-Host ""
    Write-Host "Open " -NoNewline
    Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -ForegroundColor Green
    $unicodeString = "$( [char]::ConvertFromUtf32(0x252C) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2534) ) "
    if ($AddCustomText) { Write-Host "$($unicodeString) $($AddCustomText)" -ForegroundColor Green }
    else                { Write-Host "$($unicodeString)"                    -ForegroundColor Green }
    Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -NoNewline -ForegroundColor Green
    Write-Host " KNX"
    Write-Host ""
}

# Shows a numbered list and lets the user pick one. Returns the chosen string, or $null if empty.
function Select-FromList($prompt, [string[]]$items) {
    if ($items.Count -eq 0) { return $null }
    if ($items.Count -eq 1) { return $items[0] }

    Write-Host
    Write-Host "$prompt" -ForegroundColor Yellow
    for ($i = 0; $i -lt $items.Count; $i++) {
        Write-Host "  [$($i+1)] $($items[$i])"
    }
    Write-Host

    $input = Read-Host ($script:s.SelectManual -f $items.Count)
    $index = $null
    do {
        $index = $input -as [int]
        if ($index -lt 1 -or $index -gt $items.Count) {
            $input = Read-Host ($script:s.InvalidInput -f $items.Count)
        }
    } while ($index -lt 1 -or $index -gt $items.Count)
    return $items[$index - 1]
}

# ── RP2040 functions ───────────────────────────────────────────────────────────

function ScanBootselPaths() {
    if ($IsMacOS) {
        $found = @()
        foreach ($vol in @("RPI-RP2", "RP2350")) {
            $path = "/Volumes/$vol"
            if (Test-Path $path) { $found += $path }
        }
        return $found
    } elseif ($IsLinux) {
        $found = @()
        foreach ($vol in @("RPI-RP2", "RP2350")) {
            foreach ($base in @("/media/$env:USER", "/run/media/$env:USER", "/mnt")) {
                $path = "$base/$vol"
                if (Test-Path $path) { $found += $path }
            }
        }
        return $found
    } else {
        # Get-CimInstance works without admin and is supported on PS3+
        return @(Get-CimInstance Win32_LogicalDisk -ErrorAction SilentlyContinue |
            Where-Object { $_.VolumeName -match 'RPI-RP2|RP2350' } |
            ForEach-Object { $_.DeviceID })
    }
}

function ScanPicoPorts() {
    if ($IsMacOS) {
        return @(Get-ChildItem /dev/cu.usbmodem* -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
    } elseif ($IsLinux) {
        return @(Get-ChildItem /dev/ttyACM* -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
    } else {
        $ports = @()
        try {
            # Get-PnpDevice may not exist on older Windows – catch and fall back
            $portList = Get-PnpDevice -Class Ports -ErrorAction Stop
            foreach ($usbDevice in $portList) {
                if ($usbDevice.Present) {
                    $isPico = $usbDevice.InstanceId.StartsWith('USB\VID_2E8A')
                    $isCom  = $usbDevice.Name -match 'COM\d{1,3}'
                    if ($isPico -and $isCom) { $ports += $Matches[0] }
                }
            }
        } catch {
            # Fallback: return all COM ports (no VID filter possible without PnpDevice)
            $ports = @([System.IO.Ports.SerialPort]::GetPortNames())
        }
        return $ports
    }
}

# Copies firmware with animated spinner. Returns $true on success.
function Copy-FirmwareWithSpinner($label, $sourcePath, $devicePath) {
    if ($IsWindows) {
        # Shell.Application.CopyHere is async – no spinner possible; Shell shows its own progress dialog
        Write-Host "  $label" -ForegroundColor Yellow
        $shell = New-Object -ComObject 'Shell.Application'
        $shell.NameSpace($devicePath).CopyHere($sourcePath, 0)
        # Wait until the file appears on the target volume (CopyHere returns immediately)
        $deadline = (Get-Date).AddSeconds(30)
        $fileName = [System.IO.Path]::GetFileName($sourcePath)
        while ((Get-Date) -lt $deadline) {
            if (Test-Path (Join-Path $devicePath $fileName)) { break }
            Start-Sleep -Milliseconds 300
        }
        return $true
    }
    # macOS / Linux: cp is synchronous – run in background job and show spinner
    $spinChars = @('-', '\', '|', '/')
    $job = Start-Job -ScriptBlock {
        param($src, $dst)
        & /bin/cp $src $dst
        return ($LASTEXITCODE -eq 0)
    } -ArgumentList $sourcePath, $devicePath

    $i = 0
    while ($job.State -eq 'Running') {
        [Console]::Write("`r  $($spinChars[$i % 4])  $label  ")
        $i++
        Start-Sleep -Milliseconds 150
    }
    [Console]::Write("`r$(' ' * ($label.Length + 8))`r")  # clear spinner line

    $result = Receive-Job $job -Wait -ErrorAction SilentlyContinue
    Remove-Job $job
    return ($result -eq $true)
}

# ── ESP32 functions ────────────────────────────────────────────────────────────

# Returns the path to the esptool binary, or $null if not found.
function FindEsptool() {
    if ($IsWindows) {
        # 1. OpenKNX tools bundle location
        $candidate = Join-Path $HOME "bin/esptool.exe"
        if (Test-Path -PathType Leaf $candidate) { return $candidate }
        # 2. pip install esptool on Windows puts it in PATH
        foreach ($cmd in @("esptool.exe", "esptool")) {
            if (Get-Command $cmd -ErrorAction SilentlyContinue) { return $cmd }
        }
    } else {
        $candidate = Join-Path $HOME "bin/esptool"
        if (Test-Path -PathType Leaf $candidate) { return $candidate }
        foreach ($cmd in @("esptool", "esptool.py")) {
            if (Get-Command $cmd -ErrorAction SilentlyContinue) { return $cmd }
        }
    }
    return $null
}

# Returns all current ESP32-like serial ports.
# VIDs covered: CH340 (1A86), CP210x (10C4), Espressif native USB (303A), FTDI (0403)
function ScanEsp32Ports() {
    if ($IsMacOS) {
        return @(Get-ChildItem /dev/cu.wchusbserial*, /dev/cu.usbserial-*, /dev/cu.SLAB_USBtoUART*, /dev/cu.usbmodem* `
            -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
    } elseif ($IsLinux) {
        return @(Get-ChildItem /dev/ttyUSB*, /dev/ttyACM* -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
    } else {
        $ports = @()
        try {
            $portList = Get-PnpDevice -Class Ports -ErrorAction Stop
            foreach ($usbDevice in $portList) {
                if ($usbDevice.Present) {
                    $isEsp = $usbDevice.InstanceId -match 'USB\\VID_(1A86|10C4|303A|0403)'
                    $isCom = $usbDevice.Name -match 'COM\d{1,3}'
                    if ($isEsp -and $isCom) { $ports += $Matches[0] }
                }
            }
        } catch {
            $ports = @([System.IO.Ports.SerialPort]::GetPortNames())
        }
        return $ports
    }
}

# ── Shared ─────────────────────────────────────────────────────────────────────

function WaitOrPause($seconds = -1) {
    if ($seconds -lt 0) {
        Read-Host $script:s.PressEnter
    } else {
        Start-Sleep -Seconds $seconds
    }
}

# Waits up to 60s for EITHER a new device to appear OR an existing one to unplug+replug.
# Shows a single combined prompt. Returns the new/reappeared path, or $null on cancel/timeout.
function Wait-AutoDetect([string[]]$knownPaths, [scriptblock]$scanScript) {
    Write-Host $script:s.WaitReplug -ForegroundColor Yellow
    Write-Host "  $($script:s.CancelKey)" -ForegroundColor DarkGray
    $deadline     = (Get-Date).AddSeconds(60)
    $disappeared  = $null
    $afterPaths   = $knownPaths
    while ((Get-Date) -lt $deadline) {
        $remaining = [int]($deadline - (Get-Date)).TotalSeconds
        $label = if ($disappeared) { $script:s.WaitingReplug } else { $script:s.WaitingReplug }
        [Console]::Write("`r$($label -f $remaining.ToString().PadLeft(2))")
        Start-Sleep -Milliseconds 400
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                [Console]::WriteLine()
                Write-Host $script:s.OptCancel -ForegroundColor DarkGray
                return $null
            }
        }
        $cur = @(& $scanScript)
        # Phase 1: check for new device appearing (not in known list)
        if (-not $disappeared) {
            $new = @($cur | Where-Object { $knownPaths -notcontains $_ })
            if ($new.Count -gt 0) {
                [Console]::WriteLine()
                Write-Host ($script:s.DeviceDetected -f $new[0]) -ForegroundColor Green
                return $new[0]
            }
            # check for unplug of existing device
            $gone = @($knownPaths | Where-Object { $cur -notcontains $_ })
            if ($gone.Count -gt 0) {
                $disappeared = $gone[0]
                $afterPaths  = @($knownPaths | Where-Object { $_ -ne $disappeared })
                [Console]::WriteLine()
                Write-Host ($script:s.DeviceUnplugged -f $disappeared) -ForegroundColor DarkGray
                [Console]::Write("`r$($script:s.WaitingReplug -f $remaining.ToString().PadLeft(2))")
            }
        } else {
            # Phase 2: waiting for replug after unplug
            $new = @($cur | Where-Object { $afterPaths -notcontains $_ })
            if ($new.Count -gt 0) {
                [Console]::WriteLine()
                Write-Host ($script:s.DeviceDetected -f $new[0]) -ForegroundColor Green
                return $new[0]
            }
        }
    }
    [Console]::WriteLine()
    Write-Host $script:s.NoReplug -ForegroundColor Red
    return $null
}

# Waits for a new device to appear (0-device case). Returns $true if found.
function WaitForAnyDevice($scanScript) {
    Write-Host $script:s.WaitReplug -ForegroundColor Yellow
    Write-Host "  $($script:s.CancelKey)" -ForegroundColor DarkGray
    $deadline = (Get-Date).AddSeconds(60)
    while ((Get-Date) -lt $deadline) {
        $remaining = [int]($deadline - (Get-Date)).TotalSeconds
        [Console]::Write("`r$($script:s.WaitingReplug -f $remaining.ToString().PadLeft(2))")
        Start-Sleep -Milliseconds 500
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                [Console]::WriteLine()
                Write-Host $script:s.OptCancel -ForegroundColor DarkGray
                return $false
            }
        }
        if (@(& $scanScript).Count -gt 0) {
            [Console]::WriteLine()
            return $true
        }
    }
    [Console]::WriteLine()
    Write-Host $script:s.NoReplug -ForegroundColor Red
    return $false
}

# Watches existing devices, waits for unplug then replug. Returns the new device path, or $null.
function WaitUnplugReplug($allPaths, $scanScript) {
    Write-Host $script:s.WaitUnplug -ForegroundColor Yellow
    Write-Host "  $($script:s.CancelKey)" -ForegroundColor DarkGray
    $disappeared = $null
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline) {
        $remaining = [int]($deadline - (Get-Date)).TotalSeconds
        [Console]::Write("`r$($script:s.WaitingUnplug -f $remaining.ToString().PadLeft(2))")
        Start-Sleep -Milliseconds 500
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                [Console]::WriteLine()
                Write-Host $script:s.OptCancel -ForegroundColor DarkGray
                return $null
            }
        }
        $cur  = @(& $scanScript)
        $gone = @($allPaths | Where-Object { $cur -notcontains $_ })
        if ($gone.Count -gt 0) { $disappeared = $gone[0]; break }
    }
    [Console]::WriteLine()
    if (-not $disappeared) {
        Write-Host $script:s.NoUnplug -ForegroundColor Red
        return $null
    }
    Write-Host ($script:s.DeviceUnplugged -f $disappeared) -ForegroundColor DarkGray
    Write-Host $script:s.WaitReplug -ForegroundColor Yellow
    Write-Host "  $($script:s.CancelKey)" -ForegroundColor DarkGray
    $afterPaths  = @($allPaths | Where-Object { $_ -ne $disappeared })
    $deadline    = (Get-Date).AddSeconds(30)
    $appearedPath = $null
    while ((Get-Date) -lt $deadline) {
        $remaining = [int]($deadline - (Get-Date)).TotalSeconds
        [Console]::Write("`r$($script:s.WaitingReplug -f $remaining.ToString().PadLeft(2))")
        Start-Sleep -Milliseconds 500
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                [Console]::WriteLine()
                Write-Host $script:s.OptCancel -ForegroundColor DarkGray
                return $null
            }
        }
        $cur = @(& $scanScript)
        $new = @($cur | Where-Object { $afterPaths -notcontains $_ })
        if ($new.Count -gt 0) { $appearedPath = $new[0]; break }
    }
    [Console]::WriteLine()
    if (-not $appearedPath) {
        Write-Host $script:s.NoReplug -ForegroundColor Red
        return $null
    }
    Write-Host ($script:s.DeviceDetected -f $appearedPath) -ForegroundColor Green
    return $appearedPath
}

# Waits up to $timeoutSec for a port not in $knownPorts to appear. Returns path or $null.
function Wait-ForSerialPort($scanScript, $timeoutSec, [string[]]$knownPorts) {
    $deadline = (Get-Date).AddSeconds($timeoutSec)
    while ((Get-Date) -lt $deadline) {
        $remaining = [int]($deadline - (Get-Date)).TotalSeconds
        [Console]::Write("`r$($script:s.InfoWaitPort -f $remaining.ToString().PadLeft(2))")
        Start-Sleep -Milliseconds 500
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                [Console]::WriteLine()
                return $null
            }
        }
        $current = @(& $scanScript)
        $new = @($current | Where-Object { $knownPorts -notcontains $_ })
        if ($new.Count -gt 0) { [Console]::WriteLine(); return $new[0] }
    }
    [Console]::WriteLine()
    return $null
}

function dbg($msg) { if ($script:DebugSerial) { Write-Host "  [DBG] $msg" -ForegroundColor Magenta } }

# Opens $port @ 115200, sends "i", polls up to 5s for a response.
# Returns the raw received string, or $null on timeout/error.
function Read-DeviceInfo($port) {
    $sp = New-Object System.IO.Ports.SerialPort $port, 115200, 'None', 8, 1
    $sp.WriteTimeout = 2000
    $sp.DtrEnable    = $true    # USB-CDC: signals "terminal connected" so firmware sends/receives
    $sp.Encoding     = [System.Text.Encoding]::UTF8
    try {
        dbg "Opening port $port"
        $sp.Open()
        dbg "Port opened. Waiting 500ms for firmware init..."
        Start-Sleep -Milliseconds 500
        dbg "Sending Enter to clear firmware prompt state..."
        $sp.Write("`r`n")
        Start-Sleep -Milliseconds 800
        dbg "BytesToRead before flush: $($sp.BytesToRead)"
        $sp.DiscardInBuffer()
        dbg "Buffer flushed. Sending 'i'..."
        $sp.WriteLine('i')
        # Poll for response (up to 5s). Stop when end separator arrives or buffer drains.
        $received = ''
        $deadline = (Get-Date).AddSeconds(5)
        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 200
            $btr = $sp.BytesToRead
            dbg "Poll: BytesToRead=$btr  received.Length=$($received.Length)"
            if ($btr -gt 0) {
                $chunk = $sp.ReadExisting()
                $received += $chunk
                dbg "Read $($chunk.Length) chars. Total=$($received.Length)"
                if ($received -match '-{20,}[\r\n]*$') { dbg "End separator found – done."; break }
            } elseif ($received.Length -gt 20) {
                dbg "Buffer empty + data received – final drain..."
                Start-Sleep -Milliseconds 400
                $chunk = $sp.ReadExisting()
                $received += $chunk
                dbg "Final drain: $($chunk.Length) chars"
                break
            }
        }
        $sp.Close()
        dbg "Port closed. Total received: $($received.Length) chars"
        if ($script:DebugSerial -and $received) {
            Write-Host "  [DBG] Raw output:" -ForegroundColor Magenta
            $received -split "`r?`n" | ForEach-Object { Write-Host "  [DBG]   |$_|" -ForegroundColor DarkMagenta }
        }
        if (-not $received) { dbg "Nothing received – returning null"; return $null }
        return $received
    } catch {
        dbg "Exception: $_"
        if ($sp.IsOpen) { try { $sp.Close() } catch {} }
        return $null
    }
}

# Sends a single command to $port via serial, streams back the response with a spinner.
# Returns the raw response string, or $null on error/timeout.
function Invoke-SerialCommand([string]$port, [string]$command, [int]$timeoutSec = 30) {
    $sp = New-Object System.IO.Ports.SerialPort $port, 115200, 'None', 8, 1
    $sp.WriteTimeout = 2000
    $sp.DtrEnable    = $true
    $sp.Encoding     = [System.Text.Encoding]::UTF8
    try {
        dbg "Invoke-SerialCommand: Opening $port  cmd='$command'"
        $sp.Open()
        Start-Sleep -Milliseconds 500
        $sp.Write("`r`n")
        Start-Sleep -Milliseconds 600
        $sp.DiscardInBuffer()
        $sp.WriteLine($command)

        $received     = ''
        $deadline     = (Get-Date).AddSeconds($timeoutSec)
        $lastActivity = Get-Date
        $silenceMs    = 2500   # stop after 2.5s silence once data received
        $spinChars    = @('-', '\', '|', '/')
        $spinIdx      = 0

        while ((Get-Date) -lt $deadline) {
            $remaining = [int]($deadline - (Get-Date)).TotalSeconds
            [Console]::Write("`r  $($spinChars[$spinIdx % 4])  $($script:s.WipeWaiting -f $remaining.ToString().PadLeft(2))")
            $spinIdx++
            Start-Sleep -Milliseconds 200

            $btr = $sp.BytesToRead
            if ($btr -gt 0) {
                $chunk        = $sp.ReadExisting()
                $received    += $chunk
                $lastActivity = Get-Date
                dbg "Invoke-SerialCommand: +$($chunk.Length) chars  total=$($received.Length)"
            } elseif ($received.Length -gt 0) {
                if (((Get-Date) - $lastActivity).TotalMilliseconds -gt $silenceMs) {
                    dbg "Invoke-SerialCommand: Silence $($silenceMs)ms - done"
                    break
                }
            }

            if ([Console]::KeyAvailable) {
                $key = [Console]::ReadKey($true)
                if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                    [Console]::WriteLine(); break
                }
            }
        }
        [Console]::Write("`r$(' ' * 55)`r")  # clear spinner
        $sp.Close()
        dbg "Invoke-SerialCommand: Finished. Total=$($received.Length) chars"
        if ($received.Trim()) { return $received } else { return $null }
    } catch {
        dbg "Invoke-SerialCommand exception: $_"
        if ($sp.IsOpen) { try { $sp.Close() } catch {} }
        return $null
    }
}

# Reads and prints device info from $port. $port may be $null.
# Returns $true on success, $false on timeout/error (caller may retry).
function Show-DeviceInfo($port) {
    if (-not $port) { Write-Host $script:s.InfoNoPort -ForegroundColor DarkYellow; return $false }
    Write-Host ($script:s.InfoConnecting -f $port) -ForegroundColor DarkGray
    $raw = Read-DeviceInfo $port
    if ($null -eq $raw) { Write-Host $script:s.InfoTimeout -ForegroundColor DarkYellow; return $false }
    Write-Host
    Write-Host $script:s.InfoHeader -ForegroundColor Cyan
    foreach ($line in ($raw -split "`r?`n")) {
        $display = $line -replace '^\S+\s+\S+:\s*', '' -replace '\|.*$', ''
        if ($display.Trim()) { Write-Host "  $display" }
    }
    Write-Host $script:s.InfoFooter -ForegroundColor Cyan
    Write-Host
    return $true
}

# Parses the raw serial output from Read-DeviceInfo into a structured object.
# Strategy: ANSI-strip → normalize line endings → IndexOf-based timestamp+pipe strip
#           → collect lines only after "=== Information ===" header (no-regex path).
# Works with \r\n, \r\r\n, bare \r, bare \n and devices that emit ANSI escape codes.
# Returns PSCustomObject with DeviceName/Id/Serial/FirmwareName/FirmwareVersion, or $null.
function Parse-DeviceInfo([string]$rawText) {
    if (-not $rawText) { return $null }

    # 1. Strip ANSI escape sequences (ESC[2K = clear line, ESC[33m = color, etc.)
    $rawText = $rawText -replace '\x1B\[[0-9;]*[A-Za-z]', ''

    # 2. Normalize all CR/LF variants (\r\n, \r\r\n, bare \r) to \n
    $normalized = [regex]::Replace($rawText, "`r`n|`r|`n", "`n")

    # blocks: jeder "=== BlockName ===" Header erzeugt einen Eintrag
    #   $r.Blocks['Information']['Device']['Name']
    #   $r.Blocks['Versions']['(flat)']['This Firmware']
    #   $r.Blocks['Memory']['(flat)']['Free memory']
    $blocks  = @{}
    $block   = $null
    $section = $null

    foreach ($line in ($normalized -split "`n")) {
        # 3. Timestamp-strip: "0d 01:23:13: Content|"  →  "Content|"
        $idx     = $line.IndexOf(': ')
        $content = if ($idx -ge 0) { $line.Substring($idx + 2) } else { $line }

        # 4. Pipe-strip: "Content|" → "Content"
        $pipeIdx = $content.IndexOf('|')
        if ($pipeIdx -ge 0) { $content = $content.Substring(0, $pipeIdx) }
        $content = $content.Trim()
        if (-not $content) { continue }

        # 5. Block-Header: "=== BlockName ===" → neuer Block
        if ($content -match '^={4,}\s+(\S+(?:\s+\S+)*?)\s+={4,}$') {
            $block   = $matches[1].Trim()
            $section = '(flat)'
            if (-not $blocks.ContainsKey($block))           { $blocks[$block] = @{} }
            if (-not $blocks[$block].ContainsKey($section)) { $blocks[$block][$section] = @{} }
            continue
        }
        if (-not $block) { continue }

        # 6. Section-Headers: einzelne Wörter ohne ':', '=', '-' (z.B. Device, Firmware)
        if ($content.Length -le 20 -and $content.IndexOf(':') -lt 0 -and
            $content.IndexOf('=') -lt 0 -and $content.IndexOf('-') -lt 0 -and
            $content -match '^[A-Za-z][A-Za-z ]*$') {
            $section = $content
            if (-not $blocks[$block].ContainsKey($section)) { $blocks[$block][$section] = @{} }
            continue
        }

        # 7. Key: value
        $kvIdx = $content.IndexOf(':')
        if ($kvIdx -le 0) { continue }
        $key = $content.Substring(0, $kvIdx).Trim()
        $val = $content.Substring($kvIdx + 1).Trim()
        if (-not $val) { continue }
        $blocks[$block][$section][$key] = $val
    }

    if ($blocks.Count -eq 0) { return $null }

    $info = if ($blocks.ContainsKey('Information')) { $blocks['Information'] } else { @{} }
    $dev  = if ($info.ContainsKey('Device'))        { $info['Device']        } else { @{} }
    $fw   = if ($info.ContainsKey('Firmware'))      { $info['Firmware']      } else { @{} }

    $r = [PSCustomObject]@{
        # Direkt-Shortcuts (Rückwärtskompatibilität)
        DeviceName      = $dev['Name']
        DeviceId        = $dev['ID']
        DeviceSerial    = $dev['Serial number']
        FirmwareName    = $fw['Name']
        FirmwareVersion = $fw['Version']
        # Alle Blöcke: $r.Blocks['Information']['Runtime']['Watchdog']
        #              $r.Blocks['Versions']['(flat)']['This Firmware']
        Blocks          = $blocks
        # Shortcut auf den Information-Block (Rückwärtskompatibilität)
        Sections        = $info
    }

    if ($script:DebugSerial) {
        Write-Host "  [DBG-PARSE] DeviceName='$($r.DeviceName)'  DeviceId='$($r.DeviceId)'" -ForegroundColor Magenta
        Write-Host "  [DBG-PARSE] FirmwareName='$($r.FirmwareName)'  Version='$($r.FirmwareVersion)'" -ForegroundColor Magenta
    }

    if (-not $r.DeviceName -and -not $r.FirmwareName) { return $null }
    return $r
}

# Tries to read + parse device info from a serial port.
# Returns a compact one-line label string, or $null if unavailable.
function Get-SerialDeviceLabel($port) {
    $raw = Read-DeviceInfo $port
    if (-not $raw) { return $null }
    $info = Parse-DeviceInfo $raw
    if (-not $info) { return $null }
    $parts = @()
    if ($info.DeviceName) { $parts += $info.DeviceName }
    if ($info.FirmwareName -and $info.FirmwareVersion) { $parts += "$($info.FirmwareName) v$($info.FirmwareVersion)" }
    elseif ($info.FirmwareName)    { $parts += $info.FirmwareName }
    if ($info.DeviceSerial)        { $parts += "SN: $($info.DeviceSerial)" }
    if ($parts.Count -eq 0) { return $null }
    return $parts -join '  ·  '
}

# Wipe / Erase menu – only accessible from Dev Settings ([W]).
# Scans serial devices, lets user pick a device + erase command, confirms, then sends via serial.
function Show-WipeMenu {
    $sep  = [string][char]0x2550 * 52
    $sep2 = [string][char]0x2500 * 52

    Clear-Host
    Write-Host
    Write-Host "  $sep" -ForegroundColor DarkRed
    Write-Host "  $($script:s.WipeTitle)" -ForegroundColor Red
    Write-Host "  $sep" -ForegroundColor DarkRed
    Write-Host "  $($script:s.WipeDevOnly)" -ForegroundColor DarkYellow
    Write-Host
    Write-Host "  $sep2" -ForegroundColor DarkGray
    foreach ($infoLine in $script:s.WipeInfo) {
        Write-Host "  $infoLine" -ForegroundColor Gray
    }
    Write-Host "  $sep2" -ForegroundColor DarkGray
    Write-Host
    Write-Host "  $($script:s.WipeWarning)" -ForegroundColor Red
    Write-Host

    # ── Scan all serial devices (RP2040 + ESP32) ─────────────────────────────
    Write-Host "  $($script:s.WipeSearching)" -ForegroundColor DarkGray
    $ports = @(@(ScanPicoPorts) + @(ScanEsp32Ports) | Select-Object -Unique)

    if ($ports.Count -eq 0) {
        Write-Host "  $($script:s.WipeNoDevices)" -ForegroundColor DarkYellow
        Write-Host
        Read-Host "  [Enter] $($script:s.OptCancel)" | Out-Null
        return
    }

    # Build device labels
    $devices = @()
    foreach ($p in $ports) {
        $lbl = Get-SerialDeviceLabel $p
        $devices += [PSCustomObject]@{ Path = $p; Label = if ($lbl) { "$p  $lbl" } else { $p } }
    }

    # ── Select device ────────────────────────────────────────────────────────
    Write-Host "  $($script:s.WipeSelectDev)" -ForegroundColor Yellow
    Write-Host
    for ($i = 0; $i -lt $devices.Count; $i++) {
        Write-Host "  [$($i+1)] $($devices[$i].Label)" -ForegroundColor Cyan
    }
    Write-Host "  [A]  $($script:s.OptAgain)"
    Write-Host "  [X]  $($script:s.OptCancel)" -ForegroundColor DarkGray
    Write-Host

    $validDev  = @('A','X') + (1..$devices.Count | ForEach-Object { "$_" })
    $promptDev = (1..$devices.Count | ForEach-Object { "$_" }) + @('A','X')
    $devChoice = Read-Choice "$($script:s.ChoicePrompt) [$(($promptDev -join '/'))]" $validDev
    if ($devChoice -eq 'X') { return }

    $selectedPort = $null
    if ($devChoice -eq 'A') {
        $allPaths  = @($devices | ForEach-Object { $_.Path })
        $appeared  = Wait-AutoDetect $allPaths { @(ScanPicoPorts) + @(ScanEsp32Ports) | Select-Object -Unique }
        if (-not $appeared) { return }
        $selectedPort = $appeared
    } else {
        $selectedPort = $devices[[int]$devChoice - 1].Path
    }
    $selectedLabel = ($devices | Where-Object { $_.Path -eq $selectedPort } | Select-Object -First 1).Label
    if (-not $selectedLabel) { $selectedLabel = (Get-SerialDeviceLabel $selectedPort); if ($selectedLabel) { $selectedLabel = "$selectedPort  $selectedLabel" } else { $selectedLabel = $selectedPort } }

    # ── Select erase command ─────────────────────────────────────────────────
    Write-Host
    Write-Host "  $($script:s.WipeSelectedDev)  $selectedLabel" -ForegroundColor Cyan
    Write-Host
    Write-Host "  $sep2" -ForegroundColor DarkGray
    Write-Host "  $($script:s.WipeSelectCmd)" -ForegroundColor Yellow
    Write-Host "  $sep2" -ForegroundColor DarkGray
    Write-Host
    Write-Host "  [1]  $($script:s.WipeEraseKnx)"   -ForegroundColor White
    Write-Host "  [2]  $($script:s.WipeEraseOknx)"  -ForegroundColor White
    Write-Host "  [3]  $($script:s.WipeEraseFiles)" -ForegroundColor White
    Write-Host "  [4]  $($script:s.WipeEraseAll)"   -ForegroundColor Red
    Write-Host "  [X]  $($script:s.OptCancel)"      -ForegroundColor DarkGray
    Write-Host

    $cmdChoice = Read-Choice "$($script:s.ChoicePrompt) [1/2/3/4/X]" @('1','2','3','4','X')
    if ($cmdChoice -eq 'X') { return }

    $eraseCmd = switch ($cmdChoice) {
        '1' { 'erase knx'     }
        '2' { 'erase openknx' }
        '3' { 'erase files'   }
        '4' { 'erase all'     }
    }

    # ── Confirmation ─────────────────────────────────────────────────────────
    Write-Host
    if ($cmdChoice -eq '4') {
        Write-Host "  $($script:s.WipeConfirmAll)" -ForegroundColor Red
        Write-Host
    }
    $confirm = Read-Choice "$($script:s.WipeConfirmPrompt)" @('J','Y','N')
    if ($confirm -eq 'N') { Write-Host; return }

    # ── Execute ───────────────────────────────────────────────────────────────
    Write-Host
    Write-Host ($script:s.WipeSending -f $eraseCmd, $selectedPort) -ForegroundColor Yellow
    Write-Host

    $response = Invoke-SerialCommand $selectedPort $eraseCmd 30

    # ── Show response ────────────────────────────────────────────────────────
    Write-Host
    if ($response) {
        Write-Host "  $($script:s.WipeRespHeader)" -ForegroundColor Cyan
        $response -split "`r?`n" | ForEach-Object {
            $line    = $_ -replace '\x1B\[[0-9;]*[A-Za-z]', ''              # strip ANSI
            $display = $line -replace '^\S+\s+\S+:\s*', '' -replace '\|.*$', ''  # strip timestamp
            if ($display.Trim()) { Write-Host "    $display" -ForegroundColor White }
        }
        Write-Host "  $($script:s.WipeRespFooter)" -ForegroundColor Cyan
        Write-Host
        Write-Host "  $($script:s.WipeDone)" -ForegroundColor Green
    } else {
        Write-Host "  $($script:s.WipeError)" -ForegroundColor DarkYellow
    }

    Write-Host
    Read-Host "  [Enter] $($script:s.OptBack)" | Out-Null
}

# Shows the about/info screen with author, version (git hash) and feature overview.
function Show-About {
    $sep  = [string][char]0x2550 * 54
    $sep2 = [string][char]0x2500 * 54
    # Retrieve git hash + date of this script
    $scriptDir = Split-Path $PSCommandPath -Parent
    $gitHash = try {
        $h = & git -C $scriptDir rev-parse --short HEAD 2>&1
        if ($LASTEXITCODE -eq 0) { $h.Trim() } else { 'n/a' }
    } catch { 'n/a' }
    $gitDate = try {
        $d = & git -C $scriptDir log -1 --format='%ci' HEAD 2>&1
        if ($LASTEXITCODE -eq 0 -and $d -match '\d{4}-\d{2}-\d{2}') { $Matches[0] } else { 'n/a' }
    } catch { 'n/a' }
    Clear-Host
    Write-Host
    Write-Host "  $sep" -ForegroundColor DarkCyan
    Write-Host "  Upload-Firmware-Generic  -  About" -ForegroundColor Cyan
    Write-Host "  $sep" -ForegroundColor DarkCyan
    Write-Host
    Write-Host "  OpenKNX  /  OGM-Common  /  2026" -ForegroundColor Cyan
    Write-Host "  Author   Erkan Colak" -ForegroundColor White
    Write-Host "  GitHub   github.com/GeminiServer" -ForegroundColor DarkCyan
    Write-Host
    Write-Host "  $sep2" -ForegroundColor DarkGray
    Write-Host "  Version  $gitHash  /  $gitDate" -ForegroundColor DarkYellow
    Write-Host "  $sep2" -ForegroundColor DarkGray
    Write-Host
    Write-Host "  Was dieses Tool kann:" -ForegroundColor White
    Write-Host "    -  Auto-Erkennung RP2040/RP2350 und ESP32" -ForegroundColor White
    Write-Host "    -  Gerät einstecken oder aus-/einstecken wird automatisch erkannt" -ForegroundColor White
    Write-Host "    -  Multi-Flash - mehrere Geräte in einem Durchlauf (-Multi)" -ForegroundColor White
    Write-Host "    -  Geräteinformationen auslesen (seriell / -DebugSerial)" -ForegroundColor White
    Write-Host "    -  Sprache Deutsch / English - automatisch oder via -Lang" -ForegroundColor White
    Write-Host "    -  macOS / Linux / Windows" -ForegroundColor White
    Write-Host
    Write-Host "  $sep2" -ForegroundColor DarkGray
    Write-Host "  Entwickler-Features  (?? an jedem Prompt):" -ForegroundColor DarkCyan
    Write-Host "    -  Wipe / Erase  -  Firmware-Daten löschen via OpenKNX-Console" -ForegroundColor DarkCyan
    Write-Host "         erase knx / erase openknx / erase files / erase all" -ForegroundColor DarkGray
    Write-Host "    -  Debug-Ausgabe  (serieller Traffic sichtbar)" -ForegroundColor DarkCyan
    Write-Host "    -  Multi-Modus  (zur Laufzeit umschaltbar)" -ForegroundColor DarkCyan
    Write-Host
    Write-Host "  $sep2" -ForegroundColor DarkGray
    Write-Host "  Geplante Features:" -ForegroundColor DarkGray
    Write-Host "    -  Online-Update-Suche  (coming soon)" -ForegroundColor DarkGray
    Write-Host "  $sep2" -ForegroundColor DarkGray
    Write-Host
    Read-Host "  [Enter] Zurueck / Back" | Out-Null
    Write-Host
}

# Shows the developer settings menu (hidden – type ?? at any prompt).
function Show-DevSettings {
    $sep = [string][char]0x2500 * 42
    do {
        Clear-Host
        Write-Host
        Write-Host "  $sep" -ForegroundColor DarkCyan
        Write-Host "  Entwickleroptionen / Dev Settings" -ForegroundColor Cyan
        Write-Host "  $sep" -ForegroundColor DarkCyan
        Write-Host
        $dState = if ($script:DebugSerial) { "[X] AN " } else { "[ ] AUS" }
        $mState = if ($script:MultiMode)   { "[X] AN " } else { "[ ] AUS" }
        Write-Host "  [D]  Debug-Ausgabe    $dState" -ForegroundColor White
        Write-Host "  [M]  Multi-Modus      $mState" -ForegroundColor White
        Write-Host "  [W]  Wipe / Erase" -ForegroundColor DarkRed
        Write-Host "  [I]  Info / About" -ForegroundColor DarkCyan
        Write-Host "  [X]  Zurück / Back" -ForegroundColor DarkGray
        Write-Host
        $c = (Read-Host "  [D/M/W/I/X]").Trim().ToUpper()
        if     ($c -eq 'D') { $script:DebugSerial = -not [bool]$script:DebugSerial }
        elseif ($c -eq 'M') { $script:MultiMode   = -not [bool]$script:MultiMode }
        elseif ($c -eq 'W') { Show-WipeMenu }
        elseif ($c -eq 'I') { Show-About }
    } while ($c -ne 'X')
    Write-Host
}

# Reads a validated choice from the user. Typing ?? opens the dev settings menu.
# Optional $Reprint ScriptBlock is called after the dev menu to redisplay context.
function Read-Choice([string]$prompt, [string[]]$valid, [scriptblock]$Reprint = $null) {
    while ($true) {
        $v = (Read-Host $prompt).Trim()
        if ($v -eq '??') {
            Show-DevSettings
            if ($Reprint) { & $Reprint }
            continue
        }
        $v = $v.ToUpper()
        if ($v -in $valid) { return $v }
    }
}

# Shows the manual BOOTSEL instructions nicely formatted.
function Show-ManualInstr {
    $sep = [string][char]0x2500 * 60
    Write-Host
    Write-Host "  $sep" -ForegroundColor DarkYellow
    Write-Host "  $($script:s.ManualInstrTitle)" -ForegroundColor Yellow
    Write-Host "  $sep" -ForegroundColor DarkYellow
    Write-Host
    foreach ($text in $script:s.ManualInstr) {
        if ($text) { Write-Host "    $text" -ForegroundColor White }
        else        { Write-Host }
    }
    Write-Host
}

# Shows ESP32 connection/flash-mode tip.
function Show-Esp32Instr {
    $sep = [string][char]0x2500 * 60
    Write-Host
    Write-Host "  $sep" -ForegroundColor DarkCyan
    Write-Host "  $($script:s.Esp32InstrTitle)" -ForegroundColor Cyan
    Write-Host "  $sep" -ForegroundColor DarkCyan
    Write-Host
    foreach ($text in $script:s.Esp32Instr) {
        if ($text) { Write-Host "    $text" -ForegroundColor White }
        else        { Write-Host }
    }
    Write-Host
}

# TUI file browser to pick a .uf2 or .bin firmware file. Cross-platform (no GUI).
# Returns absolute path of selected file, or $null if cancelled.
# Interactive arrow-key file browser. Cross-platform (Windows/macOS/Linux).
# Up/Down = navigate, Enter = select/enter dir, Backspace = go up, ESC/Q = cancel.
function Select-FirmwareFile {
    $dir    = (Get-Item .).FullName
    $cursor = 0

    while ($true) {
        # ── Build entries ──────────────────────────────────────────────────
        $entries = [System.Collections.Generic.List[PSCustomObject]]::new()
        $parent  = Split-Path $dir -Parent
        if ($parent -and $parent -ne $dir) {
            $entries.Add([PSCustomObject]@{ Display = $script:s.SelectFwLabelUp; Path = $parent; IsDir = $true })
        }
        try {
            Get-ChildItem -LiteralPath $dir -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name | ForEach-Object {
                    $entries.Add([PSCustomObject]@{ Display = "$($_.Name)/"; Path = $_.FullName; IsDir = $true })
                }
        } catch {}
        try {
            Get-ChildItem -LiteralPath $dir -File -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -match '\.(uf2|bin)$' } |
                Sort-Object Name | ForEach-Object {
                    $tag = if ($_.Name -match '\.uf2$') { '  [RP2040/RP2350]' } else { '  [ESP32]' }
                    $entries.Add([PSCustomObject]@{ Display = "$($_.Name)$tag"; Path = $_.FullName; IsDir = $false })
                }
        } catch {}
        if ($cursor -ge $entries.Count) { $cursor = [Math]::Max(0, $entries.Count - 1) }

        # ── Render ─────────────────────────────────────────────────────────
        Clear-Host
        $sep = [string][char]0x2500 * 62
        Write-Host
        Write-Host "  $sep" -ForegroundColor DarkCyan
        Write-Host "  $($script:s.SelectFwTitle)" -ForegroundColor Cyan
        Write-Host "  $sep" -ForegroundColor DarkCyan
        Write-Host "  $($script:s.SelectFwFolder -f $dir)" -ForegroundColor DarkGray
        Write-Host "  $($script:s.SelectFwHint)" -ForegroundColor DarkGray
        Write-Host

        if ($entries.Count -eq 0) {
            Write-Host "  $($script:s.SelectFwNoFiles)" -ForegroundColor DarkGray
            Write-Host
            Write-Host "  [ESC]  $($script:s.SelectFwCancel)" -ForegroundColor DarkGray
            [Console]::ReadKey($true) | Out-Null
            return $null
        }

        for ($i = 0; $i -lt $entries.Count; $i++) {
            if ($i -eq $cursor) {
                $color  = if ($entries[$i].IsDir) { 'White' } else { 'White' }
                $prefix = '  >> '
                Write-Host "$prefix$($entries[$i].Display)" -ForegroundColor $color -BackgroundColor DarkCyan
            } else {
                $color  = if ($entries[$i].IsDir) { 'Yellow' } else { 'Green' }
                Write-Host "     $($entries[$i].Display)" -ForegroundColor $color
            }
        }

        Write-Host
        Write-Host "  Hoch/Runter = navigieren  |  Enter = auswählen  |  ESC = abbrechen" -ForegroundColor DarkGray

        # ── Input ──────────────────────────────────────────────────────────
        $key = [Console]::ReadKey($true)
        switch ($key.Key) {
            'UpArrow'   { if ($cursor -gt 0) { $cursor-- } }
            'DownArrow' { if ($cursor -lt $entries.Count - 1) { $cursor++ } }
            'Backspace' {
                if ($parent -and $parent -ne $dir) { $dir = $parent; $cursor = 0 }
            }
            'Enter' {
                $sel = $entries[$cursor]
                if ($sel.IsDir) { $dir = $sel.Path; $cursor = 0 }
                else            { Clear-Host; return $sel.Path }
            }
            'Escape' { Clear-Host; return $null }
            default  { if ($key.KeyChar -match '^[Qq]$') { Clear-Host; return $null } }
        }
    }
}

# Displays device list + [A]/[X] options. Returns: chosen $device object, 'A', or 'X'.
function Select-Device([object[]]$devices) {
    if ($devices.Count -eq 1) {
        # Nur den Port anzeigen – Label-Suffix (z.B. BOOTSEL-Hint) kommt erst beim Confirm
        Write-Host ($script:s.DeviceAutoSelected -f $devices[0].Path) -ForegroundColor Green
        Write-Host
        return $devices[0]
    }
    $validChoices = @('A','X') + (1..$devices.Count | ForEach-Object { "$_" })
    $promptParts  = (1..$devices.Count | ForEach-Object { "$_" }) + @('A','X')
    $printList = {
        Write-Host
        Write-Host $script:s.MultipleDevices -ForegroundColor Yellow
        for ($i = 0; $i -lt $devices.Count; $i++) {
            Write-Host "  [$($i+1)] $($devices[$i].Label)" -ForegroundColor Green
        }
        Write-Host "  [A]  $($script:s.OptAgain)"
        Write-Host "  [X]  $($script:s.OptCancel)"
        Write-Host
    }
    & $printList
    $choice = Read-Choice "$($script:s.ChoicePrompt) [$(($promptParts -join '/'))]" $validChoices $printList
    if ($choice -match '^\d+$') { return $devices[[int]$choice - 1] }
    return $choice   # 'A' or 'X'
}

# ── File picker (wenn keine Firmware-Datei angegeben) ─────────────────────────

if (-not $FirmwareName) {
    $picked = Select-FirmwareFile
    if (-not $picked) { exit 0 }
    $FirmwareName = $picked
    # Chip aus Dateiendung ableiten falls noch nicht gesetzt
    if (-not $Chip) {
        if     ($FirmwareName -match '\.uf2$')               { $Chip = 'RP2040' }
        elseif ($FirmwareName -match '\.(bin|factory\.bin)$') { $Chip = 'ESP32'  }
    }
    $Chip = $Chip.ToUpper()
}

# ── Header ─────────────────────────────────────────────────────────────────────

$chipDisplay    = if ($Chip -eq 'RP2040') { 'RP2040/RP2350' } elseif ($Chip -eq 'ESP32') { 'ESP32' } else { $Chip }
$platformDisplay = if ($IsMacOS) { 'macOS' } elseif ($IsLinux) { 'Linux' } else { 'Windows' }
$chipAutoLabel     = if ($chipWasAutoDetected) { '  (detected from selected firmware)' } else { '' }
$platformAutoLabel = '  (detected)'

OpenKNX_ShowLogo('OpenKNX - Generic Firmware Upload  -  RP2040/RP2350  &  ESP32')
Write-Host "  Target MCU:  $chipDisplay$chipAutoLabel" -ForegroundColor DarkGray
Write-Host "  Platform  :  $platformDisplay$platformAutoLabel" -ForegroundColor DarkGray
Write-Host "  PowerShell:  $($PSVersionTable.PSVersion)" -ForegroundColor DarkGray
Write-Host

if ($Chip -notin @('RP2040', 'ESP32')) {
    Write-Host $s.ChipUnknown -ForegroundColor Red
    WaitOrPause
    exit 1
}

$currentDir   = (Get-Item .).FullName
$firmwarePath = if ([System.IO.Path]::IsPathRooted($FirmwareName)) { $FirmwareName } else { Join-Path $currentDir $FirmwareName }

$fwFileName = [System.IO.Path]::GetFileName($firmwarePath)
$fwDir      = [System.IO.Path]::GetDirectoryName($firmwarePath)
$sep = [string][char]0x2500 * 62
Write-Host "  $sep" -ForegroundColor DarkGray
Write-Host ($s.FirmwareFile -f $fwFileName) -ForegroundColor White
Write-Host ($s.FirmwarePath -f $fwDir)      -ForegroundColor DarkGray
Write-Host ($s.FirmwareWarn)                -ForegroundColor DarkYellow
Write-Host "  $sep" -ForegroundColor DarkGray
Write-Host

if (-not (Test-Path $firmwarePath)) {
    Write-Host ($s.FirmwareNotFound -f $firmwarePath) -ForegroundColor Red
    WaitOrPause
    exit 1
}

# Runtime-toggles (können über ?? Menü zur Laufzeit geändert werden)
$script:MultiMode = [bool]$Multi

# ══════════════════════════════════════════════════════════════════════════════
# RP2040 main loop
# ══════════════════════════════════════════════════════════════════════════════
if ($Chip -eq 'RP2040') {

    $flashCount = 0
    $continueFlashing = $true

    while ($continueFlashing) {

        # ── Scan all devices ─────────────────────────────────────────────────
        Write-Host $s.SearchDevices
        $bootselPaths = @(ScanBootselPaths)
        $serialPorts  = @(ScanPicoPorts)

        $devices = @()
        foreach ($p in $bootselPaths) {
            $devices += [PSCustomObject]@{ Type='bootsel'; Path=$p; Label="$p  $($s.DeviceInBootsel)" }
        }
        foreach ($p in $serialPorts) {
            $devices += [PSCustomObject]@{ Type='serial'; Path=$p; Label="$p  $($s.DeviceSerial)"; DeviceInfo=$null }
        }

        # ── Enrich serial labels with device info (only when multiple devices) ──
        if ($devices.Count -gt 1 -and $serialPorts.Count -gt 0) {
            Write-Host $s.InfoReadingDevices -ForegroundColor DarkGray
            $totalSerial = $serialPorts.Count
            $sIdx = 0
            for ($i = 0; $i -lt $devices.Count; $i++) {
                if ($devices[$i].Type -ne 'serial') { continue }
                $sIdx++
                $p = $devices[$i].Path
                [Console]::Write("`r  [$sIdx/$totalSerial] $p...")
                $rawText = Read-DeviceInfo $p
                $parsed  = if ($rawText) { Parse-DeviceInfo $rawText } else { $null }
                $devices[$i].DeviceInfo = $parsed
                if ($parsed) {
                    $parts = @()
                    if ($parsed.DeviceName) { $parts += $parsed.DeviceName }
                    if ($parsed.FirmwareName -and $parsed.FirmwareVersion) { $parts += "$($parsed.FirmwareName) v$($parsed.FirmwareVersion)" }
                    elseif ($parsed.FirmwareName) { $parts += $parsed.FirmwareName }
                    if ($parsed.DeviceSerial) { $parts += "SN: $($parsed.DeviceSerial)" }
                    if ($parts.Count -gt 0) { $devices[$i].Label = "$p  $($parts -join '  ·  ')" }
                }
            }
            [Console]::WriteLine()
        }

        if ($devices.Count -eq 0) {
            Write-Host $s.NoDeviceFound
            Write-Host
            Write-Host "  [A]  $($s.OptAgain)"
            Write-Host "  [X]  $($s.OptCancel)"
            Write-Host
            $choice = ""
            $choice = Read-Choice "$($s.ChoicePrompt) [A/X]" @('A','X')
            if ($choice -eq 'X') {
                Show-ManualInstr
                $continueFlashing = $false
                continue
            } else {
                WaitForAnyDevice { @(ScanBootselPaths) + @(ScanPicoPorts) } | Out-Null
            }
            continue
        }

        # ── Select device ────────────────────────────────────────────────────
        $devResult = Select-Device $devices
        $selected  = $null
        if ($devResult -eq 'X') {
            Show-ManualInstr
            $continueFlashing = $false
            continue
        } elseif ($devResult -eq 'A') {
            $allPaths     = @($devices | ForEach-Object { $_.Path })
            $appearedPath = Wait-AutoDetect $allPaths { @(ScanBootselPaths) + @(ScanPicoPorts) }
            if (-not $appearedPath) { Write-Host; continue }
            $isBootsel = @(ScanBootselPaths) -contains $appearedPath
            $selected  = [PSCustomObject]@{
                Type       = if ($isBootsel) { 'bootsel' } else { 'serial' }
                Path       = $appearedPath
                Label      = if ($isBootsel) { "$appearedPath  $($s.DeviceInBootsel)" } else { "$appearedPath  $($s.DeviceSerial)" }
                DeviceInfo = $null
            }
        } else {
            $selected = $devResult
        }

        # ── Serial → BOOTSEL via 1200-baud trick ─────────────────────────────
        $devicePath = $null
        if ($selected.Type -eq 'serial') {
            $port = $selected.Path

            # ── Pre-flash device info + confirm ───────────────────────────────
            $preInfo = $selected.DeviceInfo
            if ($null -eq $preInfo) {
                Write-Host $s.InfoReadingDevices -ForegroundColor DarkGray
                $rawText = Read-DeviceInfo $port
                dbg "Pre-flash Read-DeviceInfo: $(if ($null -eq $rawText) { 'NULL' } else { "$($rawText.Length) chars" })"
                if ($rawText) {
                    $preInfo = Parse-DeviceInfo $rawText
                    dbg "Pre-flash Parse-DeviceInfo: $(if ($null -eq $preInfo) { 'NULL' } else { "DeviceName='$($preInfo.DeviceName)'  FirmwareName='$($preInfo.FirmwareName)'" })"
                }
            }
            if ($preInfo) {
                Write-Host
                $namePart = if ($preInfo.DeviceName) { "`"$($preInfo.DeviceName)`"" } else { $port }
                $idPart   = if ($preInfo.DeviceId)   { "  [$($preInfo.DeviceId)]"   } else { '' }
                Write-Host "  $namePart$idPart" -ForegroundColor Cyan
                if ($preInfo.FirmwareName) {
                    $verPart = if ($preInfo.FirmwareVersion) { "  v$($preInfo.FirmwareVersion)" } else { '' }
                    Write-Host "  Firmware: $($preInfo.FirmwareName)$verPart" -ForegroundColor DarkCyan
                }
                Write-Host
                Write-Host "  $($s.DeviceSerial)" -ForegroundColor DarkYellow
                Write-Host
            } elseif ($rawText) {
                # Parsed info unavailable – show raw output
                Write-Host
                Write-Host $s.InfoHeader -ForegroundColor Cyan
                foreach ($line in ($rawText -split "`r?`n")) {
                    $display = $line -replace '^\S+\s+\S+:\s*', '' -replace '\|.*$', ''
                    if ($display.Trim()) { Write-Host "  $display" }
                }
                Write-Host $s.InfoFooter -ForegroundColor Cyan
                Write-Host
                Write-Host "  $($s.DeviceSerial)" -ForegroundColor DarkYellow
                Write-Host
            }

            # ── J/A/N Confirm-Menü ────────────────────────────────────────────
            Write-Host "  [J]  $($s.OptFlashBootsel)" -ForegroundColor Green
            Write-Host "  [A]  $($s.OptAgain)"
            Write-Host "  [X]  $($s.OptCancel)"
            Write-Host
            $confirm = Read-Choice "$($s.ChoicePrompt) [J/A/X]" @('J','Y','A','X')
            if ($confirm -eq 'X') {
                Write-Host
                if (-not $script:MultiMode) { $continueFlashing = $false }
                continue
            }
            if ($confirm -eq 'A') {
                # Anderes Gerät: kombiniertes Warten (einstecken oder aus-/einstecken)
                $allPaths     = @($selected.Path)
                $appearedPath = Wait-AutoDetect $allPaths { @(ScanBootselPaths) + @(ScanPicoPorts) }
                if (-not $appearedPath) { Write-Host; continue }
                $isBootsel = @(ScanBootselPaths) -contains $appearedPath
                $selected  = [PSCustomObject]@{
                    Type       = if ($isBootsel) { 'bootsel' } else { 'serial' }
                    Path       = $appearedPath
                    Label      = $appearedPath
                    DeviceInfo = $null
                }
                $port = $appearedPath
                continue  # nächste Loop-Iteration: wieder Info lesen + J/A/N
            }
            Write-Host

            Write-Host ($s.PortFound -f $port) -ForegroundColor Green
            Write-Host
            Write-Host $s.NotifyFirmware
            $serial = New-Object System.IO.Ports.SerialPort $port, 115200, 'None', 8, 1
            try { $serial.Open(); $serial.Write([byte[]] (7), 0, 1); Start-Sleep -Seconds 1 }
            catch {}
            finally { if ($serial.IsOpen) { $serial.Close() } }
            Write-Host
            Write-Host ($s.AttemptBootsel -f $port)
            $serial = New-Object System.IO.Ports.SerialPort $port, 1200, 'None', 8, 1
            try { $serial.Open() }
            catch {}
            finally { if ($serial.IsOpen) { $serial.Close() } }
            Start-Sleep -Seconds 2
            $newBootsel = @(ScanBootselPaths)
            if ($newBootsel.Count -eq 0) {
                Write-Host $s.NoDeviceFound -ForegroundColor Red
                Write-Host
                continue
            }
            $devicePath = if ($newBootsel.Count -eq 1) { $newBootsel[0] } else {
                Select-FromList $s.MultiBootsel $newBootsel
            }
        } else {
            $devicePath = $selected.Path
        }

        if (-not $devicePath) { Write-Host; continue }

        # ── Flash ─────────────────────────────────────────────────────────────
        $serialBefore = @(ScanPicoPorts)   # snapshot before copy; device will reboot as serial
        Write-Host
        $fwBaseName = [System.IO.Path]::GetFileName($firmwarePath)
        $flashOk    = Copy-FirmwareWithSpinner ($s.Installing -f $fwBaseName) $firmwarePath $devicePath
        if (-not $flashOk) {
            Write-Host $s.FlashError -ForegroundColor Red
            $continueFlashing = $false
            WaitOrPause 10
            continue
        }

        $flashCount++
        Write-Host $s.Done -ForegroundColor Green

        # ── Info option ──────────────────────────────────────────────────────
        $lastPort  = Wait-ForSerialPort { ScanPicoPorts } 15 $serialBefore
        $showInfo  = $true
        while ($showInfo) {
            Write-Host
            Write-Host "  [I]  $($s.OptInfo)"
            Write-Host "  [X]  $($s.OptSkip)"
            Write-Host
            $ic = Read-Choice "$($s.ChoicePrompt) [I/X]" @('I','X')
            if ($ic -eq 'X') { $showInfo = $false }
            else { $showInfo = -not (Show-DeviceInfo $lastPort) }  # retry on timeout
        }

        if ($script:MultiMode) {
            Write-Host ($s.FlashedCount -f $flashCount) -ForegroundColor DarkGray
            Write-Host
            $answer = Read-Choice $s.FlashAnother @('J','Y','X')
            $continueFlashing = $answer -in @('J','Y')
            if ($continueFlashing) { Write-Host }
        } else {
            $continueFlashing = $false
            WaitOrPause 10
        }
    }

    if ($script:MultiMode -and $flashCount -gt 0) {
        Write-Host
        Write-Host ($s.FlashedTotal -f $flashCount) -ForegroundColor Green
        WaitOrPause 5
    }
}

# ══════════════════════════════════════════════════════════════════════════════
# ESP32 main loop
# ══════════════════════════════════════════════════════════════════════════════
elseif ($Chip -eq 'ESP32') {

    $esptool = FindEsptool
    if (-not $esptool) {
        foreach ($line in $s.EsptoolNotFound) { Write-Host $line }
        WaitOrPause
        exit 1
    }

    $flashCount = 0
    $continueFlashing = $true

    while ($continueFlashing) {

        # ── Scan serial ports ────────────────────────────────────────────────
        Write-Host $s.SearchDevices
        $ports = @(ScanEsp32Ports)

        $devices = @()
        foreach ($p in $ports) {
            $devices += [PSCustomObject]@{ Type='esp32'; Path=$p; Label="$p  $($s.DeviceEsp32Serial)" }
        }

        if ($devices.Count -eq 0) {
            Write-Host $s.NoDeviceFound
            Write-Host
            Write-Host "  [A]  $($s.OptAgain)"
            Write-Host "  [X]  $($s.OptCancel)"
            Write-Host
            $choice = ""
            $choice = Read-Choice "$($s.ChoicePrompt) [A/X]" @('A','X')
            if ($choice -eq 'X') {
                Show-Esp32Instr
                $continueFlashing = $false
                continue
            } else {
                WaitForAnyDevice { ScanEsp32Ports } | Out-Null
            }
            continue
        }

        # ── Select device ────────────────────────────────────────────────────
        $devResult = Select-Device $devices
        $selected  = $null
        if ($devResult -eq 'X') {
            Show-Esp32Instr
            $continueFlashing = $false
            continue
        } elseif ($devResult -eq 'A') {
            $allPaths     = @($devices | ForEach-Object { $_.Path })
            $appearedPath = Wait-AutoDetect $allPaths { ScanEsp32Ports }
            if (-not $appearedPath) { Write-Host; continue }
            $selected = [PSCustomObject]@{ Type='esp32'; Path=$appearedPath; Label="$appearedPath  $($s.DeviceEsp32Serial)" }
        } else {
            $selected = $devResult
        }

        # ── Confirm-Menü ─────────────────────────────────────────────────────
        Write-Host
        Write-Host "  $($selected.Path)  $($s.DeviceEsp32Serial)" -ForegroundColor Cyan
        Write-Host
        Write-Host "  [J]  $($s.OptFlashEsp32)" -ForegroundColor Green
        Write-Host "  [A]  $($s.OptAgain)"
        Write-Host "  [X]  $($s.OptCancel)"
        Write-Host
        $confirmEsp = Read-Choice "$($s.ChoicePrompt) [J/A/X]" @('J','Y','A','X')
        if ($confirmEsp -eq 'X') {
            Write-Host
            if (-not $script:MultiMode) { $continueFlashing = $false }
            continue
        }
        if ($confirmEsp -eq 'A') {
            $allPaths     = @($devices | ForEach-Object { $_.Path })
            $appearedPath = WaitUnplugReplug $allPaths { ScanEsp32Ports }
            if (-not $appearedPath) { Write-Host; continue }
            $selected = [PSCustomObject]@{ Type='esp32'; Path=$appearedPath; Label="$appearedPath  $($s.DeviceEsp32Serial)" }
        }

        # ── Flash via esptool ─────────────────────────────────────────────────
        Write-Host
        Write-Host $s.FlashingEsp32 -ForegroundColor Yellow
        Write-Host "  $esptool --port $($selected.Path) --baud 460800 write_flash 0x0 $firmwarePath" -ForegroundColor DarkGray
        Write-Host

        & $esptool --port $selected.Path --baud 460800 write_flash 0x0 $firmwarePath

        if ($LASTEXITCODE -ne 0) {
            Write-Host $s.FlashErrorEsp32 -ForegroundColor Red
            $continueFlashing = $false
            WaitOrPause 10
            continue
        }

        $flashCount++
        Write-Host $s.FlashDoneEsp32 -ForegroundColor Green

        # ── Info option ──────────────────────────────────────────────────────
        $showInfo = $true
        while ($showInfo) {
            Write-Host
            Write-Host "  [I]  $($s.OptInfo)"
            Write-Host "  [X]  $($s.OptSkip)"
            Write-Host
            $ic = Read-Choice "$($s.ChoicePrompt) [I/X]" @('I','X')
            if ($ic -eq 'X') { $showInfo = $false }
            else { $showInfo = -not (Show-DeviceInfo $selected.Path) }  # retry on timeout
        }

        if ($script:MultiMode) {
            Write-Host ($s.FlashedCount -f $flashCount) -ForegroundColor DarkGray
            Write-Host
            $answer = Read-Choice $s.FlashAnother @('J','Y','X')
            $continueFlashing = $answer -in @('J','Y')
            if ($continueFlashing) { Write-Host }
        } else {
            $continueFlashing = $false
            WaitOrPause 10
        }
    }

    if ($script:MultiMode -and $flashCount -gt 0) {
        Write-Host
        Write-Host ($s.FlashedTotal -f $flashCount) -ForegroundColor Green
        WaitOrPause 5
    }
}
