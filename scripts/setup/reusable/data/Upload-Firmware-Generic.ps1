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

.PARAMETER SerialPort
    Use this serial port directly instead of scanning and asking. Skips the device
    search and treats the port as the single found device. Aliases: -Port, -ComPort.
    Per platform:  COM7 (Windows)  ·  /dev/cu.usbmodemXXXX (macOS)  ·  /dev/ttyACM0 (Linux).
    For RP2040/RP2350 this is the running device's serial port (it is then reset into
    BOOTSEL); a BOOTSEL drive path is also accepted. For ESP32 it is the esptool port.

.PARAMETER Verify
    RP2040/RP2350 only: write the firmware with picotool and read it back for a byte-exact
    verify (picotool load -v -x), driven straight from BOOTSEL. Requires picotool (in PATH or
    ~/bin/). Without this switch the RP path uses the normal BOOTSEL drag-and-drop copy.
    ESP32 is always verified by esptool's own flash-hash check (shown after the flash).

.PARAMETER Ask
    Always show the confirm menu before flashing, even when exactly one device is found.
    By default (fast path) a single detected device is flashed directly without asking; pass
    -Ask to force the [J/A/X] prompt. Ignored in -Multi mode (which always confirms per device).

.PARAMETER Help
    Show the logo/header and full help (parameters + examples), then exit. Alias: -h.

.PARAMETER DebugSerial
    Print low-level serial debug output when reading device information.

.EXAMPLE
    .\Upload-Firmware-Generic.ps1 firmware.uf2

.EXAMPLE
    .\Upload-Firmware-Generic.ps1 firmware.bin -Port COM7

.EXAMPLE
    .\Upload-Firmware-Generic.ps1 firmware.uf2 -Port /dev/cu.usbmodem14201

.EXAMPLE
    .\Upload-Firmware-Generic.ps1 firmware.bin -Chip ESP32 -Lang EN -Multi

.EXAMPLE
    .\Upload-Firmware-Generic.ps1 firmware.uf2 -DebugSerial

.EXAMPLE
    .\Upload-Firmware-Generic.ps1 -Help
#>
param(
    [Parameter(Position=0)]
    [string]$FirmwareName,
    [string]$Chip = "",     # RP2040 | ESP32  (auto-detected from extension if empty)
    [string]$Lang = "",
    [switch]$Multi,         # flash multiple devices in a row
    [Alias('Port','ComPort')]
    [string]$SerialPort = "",        # use this serial port directly, skip search: COM7 | /dev/cu.* | /dev/ttyACM0
    [switch]$Verify,                 # RP2040/RP2350: write + read-back verify via picotool (needs picotool)
    [switch]$Ask,                    # force the confirm menu even when a single device is found (disables fast path)
    [Alias('h')]
    [switch]$Help,                   # show logo + full help and exit
    [switch]$DebugSerial = $false,   # show serial debug output when reading device info
    [switch]$AutoExit                # never pause on error; default pauses so the window stays readable
)

# Platform detection – ensures compatibility with PowerShell 5.1 on Windows
if ($null -eq (Get-Variable -Name 'IsMacOS' -ErrorAction SilentlyContinue)) {
    $IsMacOS  = $false
    $IsLinux  = $false
    $IsWindows = $true
}

# Render Unicode glyphs (box drawing, bullets •/▸, umlauts) correctly on the
# Windows console. Windows PowerShell 5.1 defaults to the OEM code page
# (e.g. 850/437) where these become "?"; UTF-8 output fixes it. Best-effort.
if ($IsWindows) {
    try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch {}
    # Enable ANSI/VT processing so escape sequences (e.g. bold) render in the Win10 console.
    try {
        $vtSig = '[DllImport("kernel32.dll")] public static extern bool GetConsoleMode(System.IntPtr h, out uint m);' +
                 '[DllImport("kernel32.dll")] public static extern bool SetConsoleMode(System.IntPtr h, uint m);' +
                 '[DllImport("kernel32.dll")] public static extern System.IntPtr GetStdHandle(int n);'
        $vt = Add-Type -MemberDefinition $vtSig -Name 'VtNative' -Namespace 'OpenKnxUpload' -PassThru -ErrorAction Stop
        $h = $vt::GetStdHandle(-11)   # STD_OUTPUT_HANDLE
        $mode = 0
        if ($vt::GetConsoleMode($h, [ref]$mode)) { $null = $vt::SetConsoleMode($h, $mode -bor 0x0004) }  # ENABLE_VIRTUAL_TERMINAL_PROCESSING
    } catch {}
}

# ─── Config (hier anpassbar) ─────────────────────────────────────────────────────
$VerifyEspChip       = $true   # vor dem ESP-Flash den Chip via esptool prüfen (true/false)
$VerifyRpFamily      = $true   # vor dem uf2-Flash prüfen, ob RP2040/RP2350 zum Laufwerk passt
$AbortOnChipMismatch = $true   # bei Chip/Firmware-Mismatch abbrechen (sonst nur warnen)
$FastSingleDevice    = $true   # genau EIN Gerät gefunden -> direkt flashen, Confirm-Menü überspringen (-Ask erzwingt Rückfrage)

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
        Way                = 'USB'
        WayHintUsb         = 'connect the device in bootsel mode'
        NoteUsb            = 'Check that the connected device is the one this firmware belongs to.'
        FirmwareFile       = "  File    :  {0}"
        FirmwarePath       = "  Path    :  {0}"
        FirmwareWarn       = "  Warning :  Make sure the connected device matches this firmware file!"
        FirmwareHint       = "Please make sure the connected device matches this firmware (wrong device = wrong firmware flashed)."
        FirmwareNotFound   = "Firmware file not found: {0}"
        PressEnter         = "Press Enter to exit"
        SearchDevices      = "Searching for devices..."
        NoDeviceFound      = "No device found."
        DeviceAutoSelected = "Device found: {0}"
        FastFlashNote      = "Only one device found - flashing directly (pass -Ask to confirm first)."
        MultipleDevices    = "Multiple devices found – please select:"
        SelectManual       = "Selection (1-{0})"
        InvalidInput       = "Invalid input. Selection (1-{0})"
        OptAgain           = "Auto-detect (plug in device, or unplug and replug)"
        OptRescan          = "Re-read / refresh the device list"
        CancelKey          = "[ESC/Q] Cancel"
        ChoicePrompt       = "Choice"
        IdleCountdown      = "No input - closing automatically in {0,2} s ... (press any key to stay)"
        IdleClose          = "No input - exited."
        DevTitle           = "Developer options"
        DevDebug           = "Debug output"
        DevMulti           = "Multi mode"
        DevAutoClose       = "Auto-close (timeout)"
        DevWipe            = "Wipe / Erase"
        DevInfo            = "Info / About"
        DevBack            = "Back"
        StateOn            = "ON "
        StateOff           = "OFF"
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
        RpMismatch         = "Firmware is for {0}, but the BOOTSEL device is {1}. Flash aborted (wrong variant would brick the device)."
        RpMismatchWarn     = "Warning: firmware is for {0}, device is {1} - flashing anyway!"
        RpFamilyOk         = "Firmware matches the device ({0})"
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
        FlashVerifiedEsp   = "Verified (flash hash matches the firmware)"
        FlashNotVerifiedEsp= "Flash written (esptool reported no integrity hash - not verified)"
        VerifyRpTitle      = "Writing + verifying flash via picotool..."
        VerifyRpOk         = "Verified (flash matches the firmware)"
        VerifyRpFail       = "VERIFY FAILED - flash does NOT match the firmware!"
        PicotoolNotFound   = "picotool not found - falling back to plain copy WITHOUT verify (install it or place it in ~/bin/)."
        VerifyChip         = "Checking chip type via esptool..."
        ChipOk             = "Detected: {0}"
        ChipMatch          = "Detected: {0}  (matches firmware)"
        ChipNotEsp         = "No ESP detected on this port (wrong port, or device not in bootloader). Flash aborted."
        ChipMismatch       = "Chip {0} does NOT match the firmware ({1}). Flash aborted."
        ChipMismatchWarn   = "Warning: chip {0} does not match the firmware ({1}) - flashing anyway."
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
        InfoUnavailableShort = "Device info not available"
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
        Way                = 'USB'
        WayHintUsb         = 'Gerät im Bootsel-Modus anstecken'
        NoteUsb            = 'Prüfen, dass das angeschlossene Gerät zu dieser Firmware gehört.'
        FirmwareFile       = "  Datei   :  {0}"
        FirmwarePath       = "  Pfad    :  {0}"
        FirmwareWarn       = "  Hinweis :  Sicherstellen, dass das angeschlossene Gerät zu dieser Firmware-Datei passt!"
        FirmwareHint       = "Bitte sicherstellen, dass das angeschlossene Gerät zu dieser Firmware passt (falsches Gerät = falsche Firmware geflasht)."
        FirmwareNotFound   = "Firmware-Datei nicht gefunden: {0}"
        PressEnter         = "Drücke Enter zum Beenden"
        SearchDevices      = "Suche Gerät..."
        NoDeviceFound      = "Kein Gerät gefunden."
        DeviceAutoSelected = "Gerät gefunden: {0}"
        FastFlashNote      = "Nur ein Gerät gefunden - wird direkt geflasht (mit -Ask erst bestätigen)."
        MultipleDevices    = "Mehrere Geräte gefunden – bitte auswählen:"
        SelectManual       = "Auswahl (1-{0})"
        InvalidInput       = "Ungültige Eingabe. Auswahl (1-{0})"
        OptAgain           = "Automatisch erkennen (Gerät einstecken oder aus-/einstecken)"
        OptRescan          = "Geräteliste neu einlesen / aktualisieren"
        CancelKey          = "[ESC/Q] Abbrechen"
        ChoicePrompt       = "Auswahl"
        IdleCountdown      = "Keine Eingabe - schließt automatisch in {0,2} s ... (Taste drücken zum Bleiben)"
        IdleClose          = "Keine Eingabe - beendet."
        DevTitle           = "Entwickleroptionen"
        DevDebug           = "Debug-Ausgabe"
        DevMulti           = "Multi-Modus"
        DevAutoClose       = "Auto-Schließen (Timeout)"
        DevWipe            = "Wipe / Erase"
        DevInfo            = "Info / About"
        DevBack            = "Zurück"
        StateOn            = "AN "
        StateOff           = "AUS"
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
        RpMismatch         = "Firmware ist für {0}, das Gerät im BOOTSEL ist {1}. Flash abgebrochen (falsche Variante würde das Gerät unbrauchbar machen)."
        RpMismatchWarn     = "Achtung: Firmware ist für {0}, Gerät ist {1} - wird trotzdem geflasht!"
        RpFamilyOk         = "Firmware passt zum Gerät ({0})"
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
        FlashVerifiedEsp   = "Verifiziert (Flash-Hash stimmt mit der Firmware überein)"
        FlashNotVerifiedEsp= "Flash geschrieben (esptool meldete keinen Integritäts-Hash - nicht verifiziert)"
        VerifyRpTitle      = "Schreibe + verifiziere Flash über picotool..."
        VerifyRpOk         = "Verifiziert (Flash stimmt mit der Firmware überein)"
        VerifyRpFail       = "VERIFY FEHLGESCHLAGEN - Flash stimmt NICHT mit der Firmware überein!"
        PicotoolNotFound   = "picotool nicht gefunden - normale Kopie OHNE Verify (installieren oder in ~/bin/ ablegen)."
        VerifyChip         = "Prüfe Chip-Typ via esptool..."
        ChipOk             = "Erkannt: {0}"
        ChipMatch          = "Erkannt: {0}  (passt zur Firmware)"
        ChipNotEsp         = "Kein ESP an diesem Port erkannt (falscher Port, oder Gerät nicht im Bootloader). Flash abgebrochen."
        ChipMismatch       = "Chip {0} passt NICHT zur Firmware ({1}). Flash abgebrochen."
        ChipMismatchWarn   = "Achtung: Chip {0} passt nicht zur Firmware ({1}) - wird trotzdem geflasht."
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
        InfoUnavailableShort = "Sysinfo konnte nicht ausgelesen werden"
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

# ─── the shared header ─────────────────────────────────────────────────────────────────────────────
# The same header the network and KNX scripts print. Optional here: this route needs neither ftc nor
# the shared file, so a release without it falls back to the previous title line rather than refusing.
$_uiPath = Join-Path $PSScriptRoot "OpenKNX-UI-Generic.ps1"
$_haveUi = (Test-Path -PathType Leaf $_uiPath)
if ($_haveUi) { . $_uiPath }

function OpenKNX_UsbTitle {
    <# @brief The unified title where the shared header is available, the previous one where it is not. #>
    if ($_haveUi) { OpenKNX_ShowTitle -Way $s.Way -Lang $_lang }
    else { OpenKNX_ShowLogo('OpenKNX - Generic Firmware Upload  -  RP2040/RP2350  &  ESP32') }
}

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

# ─── Help switch (-Help / -h) ────────────────────────────────────────────────────
# Shows the logo header, the auto-generated syntax, and the script's own comment-based
# help (synopsis, description, parameters, examples). Rendered directly from the file
# rather than via `Get-Help -Full`, because on some platforms / PS versions Get-Help on a
# script-by-path returns only the auto-syntax. Short-circuits before any device work.
if ($Help) {
    OpenKNX_UsbTitle
    $helpTarget = if ($PSCommandPath) { $PSCommandPath } else { $MyInvocation.MyCommand.Path }

    $syntaxLine = (Get-Command $helpTarget -Syntax | Out-String) -split "`r?`n" |
                  Where-Object { $_ -match '\[' } | Select-Object -Last 1
    if ($syntaxLine) {
        Write-Host "SYNTAX" -ForegroundColor Yellow
        Write-Host ("  " + ($syntaxLine -replace [regex]::Escape($helpTarget), [System.IO.Path]::GetFileName($helpTarget)).Trim())
    }

    $raw = Get-Content -LiteralPath $helpTarget -Raw
    $m   = [regex]::Match($raw, '(?s)<#(.*?)#>')
    if ($m.Success) {
        $started = $false
        foreach ($line in ($m.Groups[1].Value -split "`r?`n")) {
            if ($line -match '^\s*\.[A-Za-z]') { $started = $true }   # skip the ASCII banner above the first keyword
            if (-not $started) { continue }
            if     ($line -match '^\s*\.PARAMETER\s+(\S+)')                                             { Write-Host ''; Write-Host ("  -" + $Matches[1]) -ForegroundColor Cyan }
            elseif ($line -match '^\s*\.(SYNOPSIS|DESCRIPTION|EXAMPLE|NOTES|INPUTS|OUTPUTS|LINK)\b')     { Write-Host ''; Write-Host $Matches[1].ToUpper() -ForegroundColor Yellow }
            else                                                                                        { Write-Host $line }
        }
    }
    Write-Host
    exit 0
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

# Polls for BOOTSEL volumes until at least one appears. Returns the paths (empty on timeout).
function Wait-BootselPaths($timeoutSec = 15) {
    $deadline = (Get-Date).AddSeconds($timeoutSec)
    while ((Get-Date) -lt $deadline) {
        $found = @(ScanBootselPaths)
        if ($found.Count -gt 0) { return $found }
        Start-Sleep -Milliseconds 250
    }
    return @()
}

# Waits until a BOOTSEL volume is really usable. The mountpoint exists before the FAT is
# mounted, so a write can fail on a path that Test-Path already reports. INFO_UF2.TXT is
# readable only once the volume is up.
function Wait-BootselReady([string]$path, $timeoutSec = 10) {
    if ($IsWindows) { return $true }
    $deadline = (Get-Date).AddSeconds($timeoutSec)
    while ((Get-Date) -lt $deadline) {
        try { if ((Get-Item (Join-Path $path 'INFO_UF2.TXT') -ErrorAction Stop).Length -gt 0) { return $true } } catch {}
        Start-Sleep -Milliseconds 250
    }
    dbg "Wait-BootselReady timeout on $path"
    return $false
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
    # macOS / Linux: copy the uf2 ourselves in small blocks for a real, smooth progress
    # bar. The RP BOOTSEL drive reboots/ejects on the final block, so an error on the last
    # write/close (after all bytes were sent) counts as success.
    Write-Host "  $label" -ForegroundColor Yellow
    $total   = try { (Get-Item $sourcePath).Length } catch { 0 }
    $target  = Join-Path $devicePath ([System.IO.Path]::GetFileName($sourcePath))
    $bufSize = 16384
    $buf     = New-Object byte[] $bufSize
    $cells   = 24
    $ok      = $false
    # Retry: a freshly enumerated BOOTSEL volume can still reject the first open
    for ($attempt = 1; $attempt -le 3 -and -not $ok; $attempt++) {
        if ($attempt -gt 1) { Start-Sleep -Seconds 1; [void](Wait-BootselReady $devicePath 5) }
        $written = 0
        $lastPct = -1
        $src = $null; $dst = $null
        try { [Console]::CursorVisible = $false } catch {}
        try {
            $src = [System.IO.File]::OpenRead($sourcePath)
            $dst = [System.IO.File]::Create($target)
            while ($true) {
                $n = $src.Read($buf, 0, $bufSize)
                if ($n -le 0) { break }
                try { $dst.Write($buf, 0, $n); $dst.Flush() } catch { $written += $n; break }  # device ejected on last block
                $written += $n
                $pct = if ($total -gt 0) { [Math]::Min(100, [int](100 * $written / $total)) } else { 0 }
                if ($pct -ne $lastPct) {
                    $lastPct = $pct
                    $fill = [int]($cells * $pct / 100)
                    Write-Host "`r  [" -ForegroundColor DarkGray -NoNewline
                    Write-Host (([string][char]0x25A0 * $fill) + ([string][char]0x25A1 * ($cells - $fill))) -ForegroundColor Green -NoNewline
                    Write-Host ("]  {0,3} %" -f $pct) -ForegroundColor DarkGray -NoNewline
                }
            }
        } catch {
            dbg "Copy attempt $attempt failed: $($_.Exception.Message)"   # completion is decided by the bytes written
        } finally {
            if ($dst) { try { $dst.Close() } catch {} }
            if ($src) { try { $src.Close() } catch {} }
            try { [Console]::CursorVisible = $true } catch {}
        }
        $ok = ($total -gt 0 -and $written -ge $total)
        if (-not $ok) { Write-Host ("`r" + (' ' * ($cells + 14)) + "`r") -NoNewline }
    }
    if ($ok) {
        Write-Host "`r  [" -ForegroundColor DarkGray -NoNewline
        Write-Host ([string][char]0x25A0 * $cells) -ForegroundColor Green -NoNewline
        Write-Host "]  100 %" -ForegroundColor DarkGray
    } else {
        Write-Host ("`r" + (' ' * ($cells + 14)) + "`r") -NoNewline
    }
    return $ok
}

# ── ESP32 functions ────────────────────────────────────────────────────────────

# Reduces a chip name or firmware variant to a comparable model token:
#   "ESP32-D0WD-V3" / "ESP32" / "ESP32-PICO-D4"  -> "ESP32"   (classic)
#   "ESP32-S3" / "ESP32S3_V1"                     -> "ESP32S3" (also S2/C3/C5/C6/H2/P4)
function Get-EspModel([string]$chipOrName) {
    $n = ($chipOrName -replace '[-_ ]', '').ToUpper()
    $m = [regex]::Match($n, '^ESP32([SCHP]\d)?')
    if ($m.Success) {
        if ($m.Groups[1].Value) { return 'ESP32' + $m.Groups[1].Value }
        return 'ESP32'
    }
    return $n
}

# esptool v5 renamed subcommands to the hyphen form ("flash-id", "write-flash"); v4 uses
# the underscore form. Detect the generation once (cached) and map names accordingly, so
# v5 no longer prints the "deprecated" warning and v4 keeps working.
$script:EsptoolV5 = $null
function Test-EsptoolV5($esptool) {
    if ($null -eq $script:EsptoolV5) {
        $ver = try { & $esptool version 2>&1 | Out-String } catch { '' }
        $m = [regex]::Match($ver, 'v(\d+)\.')
        $script:EsptoolV5 = ($m.Success -and [int]$m.Groups[1].Value -ge 5)
        dbg "esptool v5+? $($script:EsptoolV5)  ($($ver.Trim()))"
    }
    return $script:EsptoolV5
}
# Maps an underscore subcommand ("flash_id" / "write_flash") to the hyphen form on v5.
function Get-EsptoolCmd($esptool, $name) {
    if (Test-EsptoolV5 $esptool) { return ($name -replace '_', '-') }
    return $name
}

# Runs esptool write-flash and renders OUR progress bar from esptool's own "(NN %)" output.
# esptool does the actual flashing (in a background job); we only read its captured output
# to drive the bar. Returns @{ Code = <exit>; Output = '<full esptool log>' }.
# Robust by design: the flash never depends on our parsing; if no % is seen we show an
# indeterminate indicator, and on failure the caller prints the full esptool log.
function Invoke-EsptoolWriteFlash($esptool, $port, $firmwarePath) {
    $wrCmd = Get-EsptoolCmd $esptool 'write_flash'
    $tmp   = New-TemporaryFile
    $job = Start-Job -ScriptBlock {
        param($e, $p, $cmd, $fw, $out)
        & $e --port $p --baud 460800 $cmd 0x0 $fw *>&1 | Tee-Object -FilePath $out | Out-Null
        return $LASTEXITCODE
    } -ArgumentList $esptool, $port, $wrCmd, $firmwarePath, $tmp.FullName

    $cells = 24; $lastPct = -1; $spin = 0
    try { [Console]::CursorVisible = $false } catch {}
    while ($job.State -eq 'Running') {
        $txt = try { Get-Content $tmp.FullName -Raw -ErrorAction SilentlyContinue } catch { '' }
        # esptool v5 prints percent as .1f ("  8.5%", "100.0%"); v4 printed integers ("(8 %)").
        # Capture the whole number incl. optional decimals — matching only \d{1,3} before '%'
        # grabs the fraction digit of "42.3%" (=> flickering 0-9). We drive the bar from the
        # numeric value and show esptool's own .1f string. Parse invariant (decimal point, not
        # the locale comma), so it stays correct on e.g. German systems.
        $pctStr = $null; $pctNum = $null
        if ($txt) {
            $ms = [regex]::Matches($txt, '(\d{1,3}(?:\.\d+)?)\s*%')
            if ($ms.Count -gt 0) {
                $pctStr = $ms[$ms.Count - 1].Groups[1].Value
                $pctNum = [double]::Parse($pctStr, [System.Globalization.CultureInfo]::InvariantCulture)
                if ($pctNum -gt 100) { $pctNum = 100; $pctStr = '100.0' }
            }
        }
        if ($null -ne $pctNum) {
            if ($pctNum -ne $lastPct) {
                $lastPct = $pctNum
                $fill = [int]($cells * $pctNum / 100)
                Write-Host "`r  [" -ForegroundColor DarkGray -NoNewline
                Write-Host (([string][char]0x25A0 * $fill) + ([string][char]0x25A1 * ($cells - $fill))) -ForegroundColor Green -NoNewline
                Write-Host ("]  {0,5} %" -f $pctStr) -ForegroundColor DarkGray -NoNewline
            }
        } else {
            Write-Host ("`r  [" + ([string][char]0x25A1 * $cells) + ']  ' + ('.' * (($spin % 3) + 1)) + '  ') -ForegroundColor DarkGray -NoNewline
        }
        $spin++
        Start-Sleep -Milliseconds 150
    }
    try { [Console]::CursorVisible = $true } catch {}

    $raw = Receive-Job $job
    Remove-Job $job
    $output = try { Get-Content $tmp.FullName -Raw -ErrorAction SilentlyContinue } catch { '' }
    Remove-Item $tmp.FullName -ErrorAction SilentlyContinue
    $exit = if ($raw -is [array]) { [int]($raw[-1]) } elseif ($null -ne $raw) { [int]$raw } else { 0 }

    if ($exit -eq 0) {
        Write-Host "`r  [" -ForegroundColor DarkGray -NoNewline
        Write-Host ([string][char]0x25A0 * $cells) -ForegroundColor Green -NoNewline
        Write-Host "]  100.0 %" -ForegroundColor DarkGray
    } else {
        Write-Host ("`r" + (' ' * ($cells + 16)) + "`r") -NoNewline
    }
    # esptool's stub reads the flash back and prints "Hash of data verified." on a match –
    # that is the built-in integrity check; surface it so the user sees the flash was verified.
    $verified = [bool]([regex]::IsMatch($output, '(?im)hash of data verified'))
    return [PSCustomObject]@{ Code = $exit; Output = "$output"; Verified = $verified }
}

# Probes a serial port with esptool. Returns @{ Chip='ESP32-S3'; Mac='..' } or $null.
# esptool v5 prints "Chip type:  ESP32-S3" + "MAC: ..", older versions "Chip is ESP32-S3".
# Retries once on a busy port (native-USB CDC needs longer to be released).
function Get-Esp32Info($esptool, $port) {
    $idCmd = Get-EsptoolCmd $esptool 'flash_id'
    for ($attempt = 1; $attempt -le 2; $attempt++) {
        Start-Sleep -Milliseconds ($attempt * 500)
        dbg "esptool chip-detect (try $attempt): `"$esptool`" --port $port $idCmd"
        $out = try { & $esptool --port $port $idCmd 2>&1 | Out-String } catch { "EXCEPTION: $_" }
        if ($script:DebugSerial) {
            foreach ($line in ($out -split "`r?`n")) { if ($line.Trim()) { Write-Host "  [DBG-ESPTOOL] $line" -ForegroundColor DarkMagenta } }
        }
        $cm = [regex]::Match($out, 'Chip (?:is|type:)\s+(ESP32\S*)')
        if ($cm.Success) {
            $macM = [regex]::Match($out, 'MAC:\s*([0-9A-Fa-f:]{17})')
            return [PSCustomObject]@{ Chip = $cm.Groups[1].Value; Mac = $(if ($macM.Success) { $macM.Groups[1].Value } else { $null }) }
        }
        if ($out -notmatch 'busy|temporarily unavailable|could not open|exclusively lock') { break }
        dbg "esptool: port busy -> retry"
    }
    dbg "esptool: no chip detected"
    return $null
}

# Extracts the expected ESP variant from a firmware file name (e.g.
# "...KNeoPiX-ESP32S3_V1.bin" -> "ESP32S3"), or $null if none is present.
function Get-EspFwVariant([string]$fileName) {
    $m = [regex]::Match($fileName, '(?i)ESP32[A-Z0-9]*')
    if ($m.Success) { return $m.Value }
    return $null
}

# Fallback chip identification when OpenKNX sysinfo could not be read.
# ESP-like ports (CH340/CP210x/Espressif/FTDI) are probed with esptool.
# Returns a short label like "ESP32-S3 (via esptool)" or $null if not identifiable.
function Get-ChipFallbackLabel($port, $esptool, $espPorts) {
    if ($esptool -and ($espPorts -contains $port)) {
        $info = Get-Esp32Info $esptool $port
        if ($info) {
            $label = $info.Chip
            if ($info.Mac) { $label += "  ·  MAC: $($info.Mac)" }
            return "$label  ·  via esptool"
        }
    }
    return $null
}

# Family of a BOOTSEL drive from its volume name: "RP2350" or "RP2040".
# On Windows the path is a drive id (e.g. "E:") -> re-query the volume name.
function Get-BootselFamily([string]$path) {
    $name = $path
    if ($IsWindows) {
        $vol = Get-CimInstance Win32_LogicalDisk -ErrorAction SilentlyContinue |
               Where-Object { $_.DeviceID -eq $path }
        if ($vol) { $name = $vol.VolumeName }
    }
    if ($name -match 'RP2350') { return 'RP2350' } else { return 'RP2040' }
}

# Reads the family ID from a .uf2 file's first block and maps it to "RP2040" / "RP2350".
# Returns $null for non-UF2 files or UF2s without a family ID. (Only the first 32 bytes
# are read.) Family IDs: RP2040 0xE48BFF56 ; RP2350 0xE48BFF59/5A/5B.
function Get-Uf2Family([string]$path) {
    $buf = New-Object byte[] 32
    try {
        $fs = [System.IO.File]::OpenRead($path)
        $read = $fs.Read($buf, 0, 32)
        $fs.Close(); $fs.Dispose()
    } catch { return $null }
    if ($read -lt 32) { return $null }
    # NOTE: 8-digit hex with the high bit set parses as a negative Int32 in PowerShell, and
    # casting that to [uint32] throws. Use the 'L' (Int64) suffix so the literals stay positive.
    if ([System.BitConverter]::ToUInt32($buf, 0) -ne 0x0A324655)  { return $null }   # magicStart0
    if ([System.BitConverter]::ToUInt32($buf, 4) -ne 0x9E5D5157L) { return $null }   # magicStart1
    if (-not ([System.BitConverter]::ToUInt32($buf, 8) -band 0x2000)) { return $null }  # familyID present?
    $fam = [System.BitConverter]::ToUInt32($buf, 28)
    if ($fam -eq 0xE48BFF56L) { return 'RP2040' }
    if ($fam -eq 0xE48BFF59L -or $fam -eq 0xE48BFF5AL -or $fam -eq 0xE48BFF5BL) { return 'RP2350' }
    return $null
}

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

# Returns the path to the picotool binary, or $null if not found. Same lookup order as
# FindEsptool: the OpenKNX tools bundle in ~/bin first, then PATH.
function FindPicotool() {
    if ($IsWindows) {
        $candidate = Join-Path $HOME "bin/picotool.exe"
        if (Test-Path -PathType Leaf $candidate) { return $candidate }
        foreach ($cmd in @("picotool.exe", "picotool")) {
            if (Get-Command $cmd -ErrorAction SilentlyContinue) { return $cmd }
        }
    } else {
        $candidate = Join-Path $HOME "bin/picotool"
        if (Test-Path -PathType Leaf $candidate) { return $candidate }
        if (Get-Command "picotool" -ErrorAction SilentlyContinue) { return "picotool" }
    }
    return $null
}

# Writes + verifies the firmware straight from BOOTSEL: "picotool load -v -x" loads the
# UF2/BIN, reads it back byte-for-byte (-v) and then executes/reboots into the app (-x).
# The device is already in BOOTSEL here, so no reset/re-entry dance is needed. Runs in a
# background job (indeterminate spinner) and returns @{ Code = <exit>; Output = '<log>' }.
function Invoke-PicotoolLoadVerify($picotool, $firmwarePath) {
    $tmp = New-TemporaryFile
    $job = Start-Job -ScriptBlock {
        param($pt, $fw, $out)
        & $pt load "$fw" -v -x *>&1 | Tee-Object -FilePath $out | Out-Null
        return $LASTEXITCODE
    } -ArgumentList $picotool, $firmwarePath, $tmp.FullName

    $spin = 0
    try { [Console]::CursorVisible = $false } catch {}
    while ($job.State -eq 'Running') {
        Write-Host ("`r  [" + ('.' * (($spin % 3) + 1)).PadRight(3) + ']  ') -ForegroundColor DarkGray -NoNewline
        $spin++
        Start-Sleep -Milliseconds 150
    }
    try { [Console]::CursorVisible = $true } catch {}

    $raw = Receive-Job $job
    Remove-Job $job
    $output = try { Get-Content $tmp.FullName -Raw -ErrorAction SilentlyContinue } catch { '' }
    Remove-Item $tmp.FullName -ErrorAction SilentlyContinue
    $exit = if ($raw -is [array]) { [int]($raw[-1]) } elseif ($null -ne $raw) { [int]$raw } else { 0 }
    Write-Host ("`r" + (' ' * 12) + "`r") -NoNewline
    return [PSCustomObject]@{ Code = $exit; Output = "$output" }
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
    if ($script:AutoExit) { return }
    if ($seconds -lt 0) {
        Read-Host $script:s.PressEnter
    } else {
        Start-Sleep -Seconds $seconds
    }
}

# ── Reusable activity panel (header/text/right/status) ───────────────────────
$script:ActivityPanelVisible = $false
$script:ActivityPanelTop     = -1
$script:ActivityPanelHeight  = 0

$script:WaitAnimFrameMs      = 250
$script:WaitAnimStatus       = ''
$script:WaitAnimHeader       = ''

function Get-WaitAnimFrame([int]$frame) {
    switch ($frame % 5) {
        0 { return [PSCustomObject]@{ TL='□'; TR='■'; BL='■'; BR='□' } } # F0
        1 { return [PSCustomObject]@{ TL=' '; TR='■'; BL='■'; BR='□' } } # F1 (1 ausgeblendet)
        2 { return [PSCustomObject]@{ TL='□'; TR=' '; BL='■'; BR='□' } } # F2 (2 ausgeblendet)
        3 { return [PSCustomObject]@{ TL='□'; TR='■'; BL=' '; BR='□' } } # F3 (3 ausgeblendet)
        default { return [PSCustomObject]@{ TL='□'; TR='■'; BL='■'; BR=' ' } } # F4 (4 ausgeblendet)
    }
}

function Format-WaitAnimPixel([string]$pixel) {
    $esc = [char]27
    if ($pixel -eq '■') { return "$esc[92m■$esc[0m" }
    if ($pixel -eq '□') { return "$esc[90m□$esc[0m" }
    return ' '
}

function Write-ActivityPanelLineAt([int]$y, [string]$line) {
    $width = [Math]::Max(1, [Console]::WindowWidth - 1)
    [Console]::SetCursorPosition(0, $y)
    [Console]::Write(' ' * $width)
    [Console]::SetCursorPosition(0, $y)
    [Console]::Write($line)
}

function Start-ActivityPanel([int]$height) {
    if ($script:ActivityPanelVisible) {
        if ($script:ActivityPanelHeight -eq $height) { return }
        Stop-ActivityPanel
    }
    for ($i = 0; $i -lt $height; $i++) { Write-Host }
    $script:ActivityPanelTop = [Math]::Max(0, [Console]::CursorTop - $height)
    $script:ActivityPanelHeight = $height
    $script:ActivityPanelVisible = $true
}

function Stop-ActivityPanel {
    if (-not $script:ActivityPanelVisible) { return }
    for ($i = 0; $i -lt $script:ActivityPanelHeight; $i++) {
        Write-ActivityPanelLineAt ($script:ActivityPanelTop + $i) ''
    }
    [Console]::SetCursorPosition(0, [Math]::Max(0, $script:ActivityPanelTop))
    $script:ActivityPanelVisible = $false
    $script:ActivityPanelTop = -1
    $script:ActivityPanelHeight = 0
}

function Render-ActivityPanel {
    param(
        [ValidateSet('corners','spinner')]
        [string]$Style,
        [int]$Frame,
        [string]$Header = '',
        [string]$Text = '',
        [string]$RightText = '',
        [string]$Status = ''
    )

    $esc = [char]27

    if ($Style -eq 'corners') {
        Start-ActivityPanel 5
        $f = Get-WaitAnimFrame $Frame
        $tl = Format-WaitAnimPixel $f.TL
        $tr = Format-WaitAnimPixel $f.TR
        $bl = Format-WaitAnimPixel $f.BL
        $br = Format-WaitAnimPixel $f.BR

        $line0 = if ($Header) { "  $Header" } else { '' }
        $line1 = "  $tl  $tr"
        $line2 = "  ${esc}[92m┬──┴${esc}[0m $Text"
        if ($RightText) { $line2 += "  $RightText" }
        $line3 = "  $bl  $br"
        $line4 = if ($Status) { "  ${esc}[90m$Status${esc}[0m" } else { '' }

        Write-ActivityPanelLineAt $script:ActivityPanelTop $line0
        Write-ActivityPanelLineAt ($script:ActivityPanelTop + 1) $line1
        Write-ActivityPanelLineAt ($script:ActivityPanelTop + 2) $line2
        Write-ActivityPanelLineAt ($script:ActivityPanelTop + 3) $line3
        Write-ActivityPanelLineAt ($script:ActivityPanelTop + 4) $line4
        [Console]::SetCursorPosition(0, $script:ActivityPanelTop + 4)
        return
    }

    Start-ActivityPanel 3
    $spinChars = @('-', '\', '|', '/')
    $spin = $spinChars[$Frame % $spinChars.Count]
    $line0 = if ($Header) { "  $Header" } else { '' }
    $line1 = "  $spin  $Text"
    if ($RightText) { $line1 += "  $RightText" }
    $line2 = if ($Status) { "  ${esc}[90m$Status${esc}[0m" } else { '' }

    Write-ActivityPanelLineAt $script:ActivityPanelTop $line0
    Write-ActivityPanelLineAt ($script:ActivityPanelTop + 1) $line1
    Write-ActivityPanelLineAt ($script:ActivityPanelTop + 2) $line2
    [Console]::SetCursorPosition(0, $script:ActivityPanelTop + 2)
}

function Write-WaitAnimFrame([int]$remaining, [int]$frame, [string]$waitTemplate) {
    $msg = ($waitTemplate -f $remaining.ToString().PadLeft(2)).Trim()
    Render-ActivityPanel -Style corners -Frame $frame -Header $script:WaitAnimHeader -Text $msg -Status $script:WaitAnimStatus
}

function Stop-WaitAnim {
    Stop-ActivityPanel
    $script:WaitAnimStatus = ''
    $script:WaitAnimHeader = ''
}

function Write-WaitAnimMessageBelow([string]$text, [string]$color = 'Gray') {
    if ($script:ActivityPanelVisible) {
        $script:WaitAnimStatus = $text
        return
    }
    Write-Host $text -ForegroundColor $color
}

# Waits up to 60s for EITHER a new device to appear OR an existing one to unplug+replug.
# Shows a single combined prompt. Returns the new/reappeared path, or $null on cancel/timeout.
function Wait-AutoDetect([string[]]$knownPaths, [scriptblock]$scanScript) {
    Write-Host $script:s.WaitReplug -ForegroundColor Yellow
    Write-Host "  $($script:s.CancelKey)" -ForegroundColor DarkGray
    $deadline     = (Get-Date).AddSeconds(60)
    $disappeared  = $null
    $afterPaths   = $knownPaths
    $frame        = 0
    $script:WaitAnimStatus = ''
    $script:WaitAnimHeader = $script:s.WaitReplug
    while ((Get-Date) -lt $deadline) {
        $remaining = [int]($deadline - (Get-Date)).TotalSeconds
        Write-WaitAnimFrame -remaining $remaining -frame $frame -waitTemplate $script:s.WaitingReplug
        $frame++
        Start-Sleep -Milliseconds $script:WaitAnimFrameMs
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                Stop-WaitAnim
                Write-Host $script:s.OptCancel -ForegroundColor DarkGray
                return $null
            }
        }
        $cur = @(& $scanScript)
        # Phase 1: check for new device appearing (not in known list)
        if (-not $disappeared) {
            $new = @($cur | Where-Object { $knownPaths -notcontains $_ })
            if ($new.Count -gt 0) {
                Stop-WaitAnim
                Write-Host ($script:s.DeviceDetected -f $new[0]) -ForegroundColor Green
                return $new[0]
            }
            # check for unplug of existing device
            $gone = @($knownPaths | Where-Object { $cur -notcontains $_ })
            if ($gone.Count -gt 0) {
                $disappeared = $gone[0]
                $afterPaths  = @($knownPaths | Where-Object { $_ -ne $disappeared })
                Write-WaitAnimMessageBelow ($script:s.DeviceUnplugged -f $disappeared) 'DarkGray'
            }
        } else {
            # Phase 2: waiting for replug after unplug
            $new = @($cur | Where-Object { $afterPaths -notcontains $_ })
            if ($new.Count -gt 0) {
                Stop-WaitAnim
                Write-Host ($script:s.DeviceDetected -f $new[0]) -ForegroundColor Green
                return $new[0]
            }
        }
    }
    Stop-WaitAnim
    Write-Host $script:s.NoReplug -ForegroundColor Red
    return $null
}

# Waits for a new device to appear (0-device case). Returns $true if found.
function WaitForAnyDevice($scanScript) {
    Write-Host $script:s.WaitReplug -ForegroundColor Yellow
    Write-Host "  $($script:s.CancelKey)" -ForegroundColor DarkGray
    $deadline = (Get-Date).AddSeconds(60)
    $frame = 0
    $script:WaitAnimHeader = $script:s.WaitReplug
    while ((Get-Date) -lt $deadline) {
        $remaining = [int]($deadline - (Get-Date)).TotalSeconds
        Write-WaitAnimFrame -remaining $remaining -frame $frame -waitTemplate $script:s.WaitingReplug
        $frame++
        Start-Sleep -Milliseconds $script:WaitAnimFrameMs
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                Stop-WaitAnim
                Write-Host $script:s.OptCancel -ForegroundColor DarkGray
                return $false
            }
        }
        if (@(& $scanScript).Count -gt 0) {
            Stop-WaitAnim
            return $true
        }
    }
    Stop-WaitAnim
    Write-Host $script:s.NoReplug -ForegroundColor Red
    return $false
}

# Watches existing devices, waits for unplug then replug. Returns the new device path, or $null.
function WaitUnplugReplug($allPaths, $scanScript) {
    Write-Host $script:s.WaitUnplug -ForegroundColor Yellow
    Write-Host "  $($script:s.CancelKey)" -ForegroundColor DarkGray
    $disappeared = $null
    $deadline = (Get-Date).AddSeconds(30)
    $frame = 0
    $unplugPrefix = (($script:s.WaitingUnplug -split '\{0\}', 2)[0]).Trim()
    while ((Get-Date) -lt $deadline) {
        $remaining = [int]($deadline - (Get-Date)).TotalSeconds
        Render-ActivityPanel -Style spinner -Frame $frame -Header $script:s.WaitUnplug -Text $unplugPrefix -RightText ("{0}s" -f $remaining)
        $frame++
        Start-Sleep -Milliseconds $script:WaitAnimFrameMs
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                Stop-ActivityPanel
                Write-Host $script:s.OptCancel -ForegroundColor DarkGray
                return $null
            }
        }
        $cur  = @(& $scanScript)
        $gone = @($allPaths | Where-Object { $cur -notcontains $_ })
        if ($gone.Count -gt 0) { $disappeared = $gone[0]; break }
    }
    Stop-ActivityPanel
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
    $frame = 0
    $script:WaitAnimHeader = $script:s.WaitReplug
    while ((Get-Date) -lt $deadline) {
        $remaining = [int]($deadline - (Get-Date)).TotalSeconds
        Write-WaitAnimFrame -remaining $remaining -frame $frame -waitTemplate $script:s.WaitingReplug
        $frame++
        Start-Sleep -Milliseconds $script:WaitAnimFrameMs
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                Stop-WaitAnim
                Write-Host $script:s.OptCancel -ForegroundColor DarkGray
                return $null
            }
        }
        $cur = @(& $scanScript)
        $new = @($cur | Where-Object { $afterPaths -notcontains $_ })
        if ($new.Count -gt 0) { $appearedPath = $new[0]; break }
    }
    Stop-WaitAnim
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
    $frame = 0
    $prefix = (($script:s.InfoWaitPort -split '\{0\}', 2)[0]).Trim()
    while ((Get-Date) -lt $deadline) {
        $remaining = [int]($deadline - (Get-Date)).TotalSeconds
        Render-ActivityPanel -Style spinner -Frame $frame -Header $script:s.SearchDevices -Text $prefix -RightText ("{0}s" -f $remaining)
        $frame++
        Start-Sleep -Milliseconds $script:WaitAnimFrameMs
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if ($key.Key -eq 'Escape' -or $key.KeyChar -match '^[Qq]$') {
                Stop-ActivityPanel
                return $null
            }
        }
        $current = @(& $scanScript)
        $new = @($current | Where-Object { $knownPorts -notcontains $_ })
        if ($new.Count -gt 0) { Stop-ActivityPanel; return $new[0] }
    }
    Stop-ActivityPanel
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
        $sp.Dispose()   # release the OS handle so esptool can open the port right after
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
        try { $sp.Dispose() } catch {}
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
        $spinIdx      = 0
        $wipePrefix   = (($script:s.WipeWaiting -split '\{0\}', 2)[0]).Trim()

        while ((Get-Date) -lt $deadline) {
            $remaining = [int]($deadline - (Get-Date)).TotalSeconds
            Render-ActivityPanel -Style spinner -Frame $spinIdx -Header $script:s.WipeTitle -Text $wipePrefix -RightText ("{0}s" -f $remaining)
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
                    Stop-ActivityPanel
                    break
                }
            }
        }
        Stop-ActivityPanel
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

# Builds the compact one-line device label parts from a parsed info object:
#   "Device Name"  ·  Firmware vX.Y.Z  ·  SN: ....
# The hardware id is intentionally NOT included here (too wide for lists);
# it is shown on its own line in the pre-flash confirm screen instead.
function Format-DeviceLabelParts($info) {
    $parts = @()
    if ($info.DeviceName)          { $parts += "`"$($info.DeviceName)`"" }
    if ($info.FirmwareName -and $info.FirmwareVersion) { $parts += "$($info.FirmwareName) v$($info.FirmwareVersion)" }
    elseif ($info.FirmwareName)    { $parts += $info.FirmwareName }
    if ($info.DeviceSerial)        { $parts += "SN: $($info.DeviceSerial)" }
    return ($parts -join '  ·  ')
}

# Prints the device header used in the pre-flash confirm screens:
#   "Device Name"   (or the port as fallback)
#   [Hardware-Id (HW ID: 0x....)]   (own line, only if present)
#   Firmware: Name  vX.Y.Z          (only if present)
#   SN: ....                        (only if present; serial is normally always there)
function Show-DeviceConfirmHeader($info, $fallbackPath) {
    $namePart = if ($info.DeviceName) { "`"$($info.DeviceName)`"" } else { $fallbackPath }
    Write-Host "  $namePart" -ForegroundColor Cyan
    if ($info.DeviceId) {
        Write-Host "  [$($info.DeviceId)]" -ForegroundColor DarkCyan
    }
    if ($info.FirmwareName) {
        $verPart = if ($info.FirmwareVersion) { "  v$($info.FirmwareVersion)" } else { '' }
        Write-Host "  Firmware: $($info.FirmwareName)$verPart" -ForegroundColor DarkCyan
    }
    if ($info.DeviceSerial) {
        $esc = [char]27
        Write-Host ("  " + $esc + "[1mSN: " + $info.DeviceSerial + $esc + "[0m") -ForegroundColor DarkCyan   # bold
    }
}

# Returns the per-device columns (Port / Name / Fw / Sn) used to render the
# selection list as a left-aligned table. Falls back to the plain Label suffix
# for devices without parsed info (BOOTSEL drives, failed reads).
function Get-DeviceListRow($d) {
    $port = $d.Path
    $info = if ($d.PSObject.Properties['DeviceInfo']) { $d.DeviceInfo } else { $null }
    if ($info) {
        $name = if ($info.DeviceName) { "`"$($info.DeviceName)`"" } else { '' }
        $fw   = if ($info.FirmwareName -and $info.FirmwareVersion) { "$($info.FirmwareName) v$($info.FirmwareVersion)" }
                elseif ($info.FirmwareName) { $info.FirmwareName } else { '' }
        $sn   = if ($info.DeviceSerial) { "SN: $($info.DeviceSerial)" } else { '' }
        return [PSCustomObject]@{ Port = $port; Name = $name; Fw = $fw; Sn = $sn; HasInfo = $true }
    }
    $rest = $d.Label
    $pfx  = "$port  "
    if ($rest -and $rest.StartsWith($pfx)) { $rest = $rest.Substring($pfx.Length) }
    return [PSCustomObject]@{ Port = $port; Name = $rest; Fw = ''; Sn = ''; HasInfo = $false }
}

# Tries to read + parse device info from a serial port.
# Returns a compact one-line label string, or $null if unavailable.
function Get-SerialDeviceLabel($port) {
    $raw = Read-DeviceInfo $port
    if (-not $raw) { return $null }
    $info = Parse-DeviceInfo $raw
    if (-not $info) { return $null }
    $label = Format-DeviceLabelParts $info
    if (-not $label) { return $null }
    return $label
}

# Reads sysinfo for one serial device and updates it in place (Label / DeviceInfo /
# InfoReadFailed); prints "ok", the esptool chip fallback, or "unavailable". Shared by
# both main loops so the enrichment logic lives in exactly one place.
function Update-DeviceLabel($device, $espPorts, $esptool) {
    $p       = $device.Path
    $rawText = Read-DeviceInfo $p
    $parsed  = if ($rawText) { Parse-DeviceInfo $rawText } else { $null }
    $device.DeviceInfo = $parsed
    if ($parsed) {
        $label = Format-DeviceLabelParts $parsed
        if ($label) { $device.Label = "$p  $label" }
        Write-Host "ok" -ForegroundColor DarkGray
    } else {
        $device.InfoReadFailed = $true
        $chipLbl = Get-ChipFallbackLabel $p $esptool $espPorts
        if ($chipLbl) {
            $device.Label = "$p  $chipLbl"
            Write-Host $chipLbl -ForegroundColor DarkYellow
        } else {
            $device.Label = "$p  $($script:s.InfoUnavailableShort)"
            Write-Host $script:s.InfoUnavailableShort -ForegroundColor DarkYellow
        }
    }
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
    $w    = 64
    $sep  = [string][char]0x2550 * $w   # ═  double rule
    $sep2 = [string][char]0x2500 * $w   # ─  light rule
    $tri  = [char]0x25B8                 # ▸  section marker
    $dot  = [char]0x2022                 # •  bullet

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

    # Small helpers for consistent, aligned rows
    function Meta($label, $value, [ConsoleColor]$valColor = [ConsoleColor]::White) {
        Write-Host ("    {0,-9}" -f $label) -ForegroundColor DarkGray -NoNewline
        Write-Host $value -ForegroundColor $valColor
    }
    function Head($text) {
        Write-Host "  $sep2" -ForegroundColor DarkGray
        Write-Host "  $tri $text" -ForegroundColor Cyan
        Write-Host "  $sep2" -ForegroundColor DarkGray
    }
    function Item($text, [ConsoleColor]$color = [ConsoleColor]::White) {
        Write-Host "     $dot  " -ForegroundColor DarkCyan -NoNewline
        Write-Host $text -ForegroundColor $color
    }

    Clear-Host
    OpenKNX_ShowLogo('Generic Firmware Upload  -  About')

    Write-Host "  $sep" -ForegroundColor DarkCyan
    Write-Host "  Upload-Firmware-Generic" -ForegroundColor Cyan -NoNewline
    Write-Host "   RP2040 / RP2350  &  ESP32" -ForegroundColor DarkGray
    Write-Host "  $sep" -ForegroundColor DarkCyan
    Write-Host
    Meta 'Projekt'  'OpenKNX  /  OGM-Common  /  2026'
    Meta 'Autor'    'Erkan Çolak'
    Meta 'GitHub'   'github.com/GeminiServer'      Cyan
    Meta 'Version'  "$gitHash  /  $gitDate"        DarkYellow
    Write-Host

    Head 'Was dieses Tool kann'
    Item 'Auto-Erkennung RP2040/RP2350 und ESP32'
    Item 'Gerät einstecken oder aus-/einstecken wird automatisch erkannt'
    Item 'Multi-Flash - mehrere Geräte in einem Durchlauf (-Multi)'
    Item 'Geräteinformationen auslesen (seriell / -DebugSerial)'
    Item 'Sprache Deutsch / English - automatisch oder via -Lang'
    Item 'macOS / Linux / Windows'
    Write-Host

    Head 'Entwickler-Features  (?? an jedem Prompt)'
    Item 'Wipe / Erase  -  Firmware-Daten löschen via OpenKNX-Console' DarkCyan
    Write-Host "          erase knx / erase openknx / erase files / erase all" -ForegroundColor DarkGray
    Item 'Debug-Ausgabe  (serieller Traffic sichtbar)' DarkCyan
    Item 'Multi-Modus  (zur Laufzeit umschaltbar)'     DarkCyan
    Write-Host

    Head 'Geplante Features'
    Item 'Online-Update-Suche  (coming soon)' DarkGray
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
        Write-Host "  $($script:s.DevTitle)" -ForegroundColor Cyan
        Write-Host "  $sep" -ForegroundColor DarkCyan
        Write-Host
        $on  = $script:s.StateOn
        $off = $script:s.StateOff
        $dState = if ($script:DebugSerial)     { "[X] $on" } else { "[ ] $off" }
        $mState = if ($script:MultiMode)       { "[X] $on" } else { "[ ] $off" }
        $tState = if (-not $script:NoTimeout)  { "[X] $on" } else { "[ ] $off" }   # auto-close active?
        Write-Host ("  [D]  {0,-26}$dState" -f $script:s.DevDebug)     -ForegroundColor White
        Write-Host ("  [M]  {0,-26}$mState" -f $script:s.DevMulti)     -ForegroundColor White
        Write-Host ("  [T]  {0,-26}$tState" -f $script:s.DevAutoClose) -ForegroundColor White
        Write-Host "  [W]  $($script:s.DevWipe)" -ForegroundColor DarkRed
        Write-Host "  [I]  $($script:s.DevInfo)" -ForegroundColor DarkCyan
        Write-Host "  [X]  $($script:s.DevBack)" -ForegroundColor DarkGray
        Write-Host
        $c = (Read-Host "  [D/M/T/W/I/X]").Trim().ToUpper()
        if     ($c -eq 'D') { $script:DebugSerial = -not [bool]$script:DebugSerial }
        elseif ($c -eq 'M') { $script:MultiMode   = -not [bool]$script:MultiMode }
        elseif ($c -eq 'T') { $script:NoTimeout   = -not [bool]$script:NoTimeout }
        elseif ($c -eq 'W') { Show-WipeMenu }
        elseif ($c -eq 'I') { Show-About }
    } while ($c -ne 'X')
    Write-Host
}

# Reads a validated choice from the user. Typing ?? opens the dev settings menu.
# Optional $Reprint ScriptBlock is called after the dev menu to redisplay context.
function Read-Choice([string]$prompt, [string[]]$valid, [scriptblock]$Reprint = $null) {
    $idleSec      = 5     # grace before the countdown starts
    $countdownSec = 30    # then count down and close the tool
    while ($true) {
        # Use interactive key polling with idle auto-close; fall back to Read-Host when
        # there is no real console (redirected / CI) so behaviour stays unchanged there.
        $consoleOk = $true
        try { $null = [Console]::KeyAvailable } catch { $consoleOk = $false }

        # Menus whose options are all single letters ([I]/[X], [J]/[A]/[X], ...) act on the keystroke
        # itself -- no Enter. Lists with double-digit entries ("10") still need Enter, and so does the
        # hidden '??', so those keep the typed-buffer path.
        $singleKey = -not ($valid | Where-Object { $_.Length -ne 1 })

        if (-not $consoleOk -or $script:NoTimeout) {
            $v = (Read-Host $prompt).Trim()
        } else {
            $promptText = $prompt + ': '
            Write-Host $promptText -NoNewline
            $buffer  = ''
            $lastKey = Get-Date          # any keypress resets this -> resets the close timer
            $cd      = $false            # countdown currently shown on the line?
            $shown   = -1
            $v       = $null
            while ($true) {
                if ([Console]::KeyAvailable) {
                    $key = [Console]::ReadKey($true)
                    if ($cd) {           # a key cancels the countdown -> restore the prompt
                        Write-Host ("`r" + (' ' * 78) + "`r") -NoNewline
                        Write-Host ($promptText + $buffer) -NoNewline
                        $cd = $false; $shown = -1
                    }
                    $lastKey = Get-Date
                    if ($key.Key -eq 'Enter') { $v = $buffer.Trim(); Write-Host ''; break }
                    elseif ($key.Key -eq 'Backspace') {
                        if ($buffer.Length -gt 0) { $buffer = $buffer.Substring(0, $buffer.Length - 1); Write-Host "`b `b" -NoNewline }
                    } elseif ([int][char]$key.KeyChar -ge 32) {
                        $ch = [string]$key.KeyChar
                        if ($singleKey -and $buffer -eq '' -and $ch.ToUpper() -in $valid) {
                            Write-Host $ch                 # echo the key, then act on it
                            $v = $ch
                            break
                        }
                        $buffer += $ch; Write-Host $ch -NoNewline
                    }
                } else {
                    # Idle close runs whether or not something was typed -- a half-typed entry used to
                    # disable it entirely and the tool then waited for Enter forever.
                    $idle = ((Get-Date) - $lastKey).TotalSeconds
                    if ($idle -ge $idleSec) {
                        $remain = [int][Math]::Ceiling(($idleSec + $countdownSec) - $idle)
                        if ($remain -le 0) {
                            Write-Host ''
                            Write-Host ('  ' + $script:s.IdleClose) -ForegroundColor DarkGray
                            exit 0
                        }
                        if ($remain -ne $shown) {
                            $shown = $remain
                            Write-Host ("`r  " + ($script:s.IdleCountdown -f $remain) + '   ') -ForegroundColor DarkGray -NoNewline
                            $cd = $true
                        }
                    }
                    Start-Sleep -Milliseconds 150
                }
            }
        }

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
        OpenKNX_UsbTitle
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
        Write-Host "  $($script:s.SelectFwHint)" -ForegroundColor DarkGray

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

# Displays device list + [A]/[R]/[X] options. Returns: chosen $device object, 'A', 'R', or 'X'.
function Select-Device([object[]]$devices) {
    if ($devices.Count -eq 1) {
        # Nur den Port anzeigen – Label-Suffix (z.B. BOOTSEL-Hint) kommt erst beim Confirm
        Write-Host ($script:s.DeviceAutoSelected -f $devices[0].Path) -ForegroundColor Green
        Write-Host
        return $devices[0]
    }
    $validChoices = @('A','R','X') + (1..$devices.Count | ForEach-Object { "$_" })
    $promptParts  = (1..$devices.Count | ForEach-Object { "$_" }) + @('A','R','X')
    $printList = {
        Write-Host
        Write-Host $script:s.MultipleDevices -ForegroundColor Cyan
        # Build a left-aligned table: Port / Name / Firmware / SN (HW id omitted here).
        $rows  = @(foreach ($d in $devices) { Get-DeviceListRow $d })
        # Column widths are driven only by rows with real device info; BOOTSEL/
        # failed-read fallback rows may overflow (no further columns follow them).
        $wPort = 0; $wName = 0; $wFw = 0
        foreach ($r in $rows) {
            if ($r.Port.Length -gt $wPort) { $wPort = $r.Port.Length }
            if (-not $r.HasInfo) { continue }
            if ($r.Name.Length -gt $wName) { $wName = $r.Name.Length }
            if ($r.Fw.Length   -gt $wFw)   { $wFw   = $r.Fw.Length }
        }
        $idxW = ([string]$devices.Count).Length
        for ($i = 0; $i -lt $devices.Count; $i++) {
            $entryColor = 'Green'
            if ($devices[$i].PSObject.Properties['InfoReadFailed'] -and $devices[$i].InfoReadFailed) {
                $entryColor = 'DarkYellow'
            }
            $r    = $rows[$i]
            $num  = ([string]($i + 1)).PadLeft($idxW)
            $line = ("{0}  {1}  {2}  {3}" -f $r.Port.PadRight($wPort), $r.Name.PadRight($wName), $r.Fw.PadRight($wFw), $r.Sn).TrimEnd()
            Write-Host "  [$num] $line" -ForegroundColor $entryColor
        }
        Write-Host "  [A]  $($script:s.OptAgain)"
        Write-Host "  [R]  $($script:s.OptRescan)" -ForegroundColor DarkCyan
        Write-Host "  [X]  $($script:s.OptCancel)" -ForegroundColor DarkGray
        Write-Host
    }
    & $printList
    $choice = Read-Choice "$($script:s.ChoicePrompt) [$(($promptParts -join '/'))]" $validChoices $printList
    if ($choice -match '^\d+$') { return $devices[[int]$choice - 1] }
    return $choice   # 'A', 'R' or 'X'
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

OpenKNX_UsbTitle
if (-not $_haveUi) {
    Write-Host "  Target MCU:  $chipDisplay$chipAutoLabel" -ForegroundColor DarkGray
    Write-Host "  Platform  :  $platformDisplay$platformAutoLabel" -ForegroundColor DarkGray
    Write-Host "  PowerShell:  $($PSVersionTable.PSVersion)" -ForegroundColor DarkGray
    Write-Host
}

if ($Chip -notin @('RP2040', 'ESP32')) {
    Write-Host $s.ChipUnknown -ForegroundColor Red
    WaitOrPause
    exit 1
}

$currentDir   = (Get-Item .).FullName
if ([System.IO.Path]::IsPathRooted($FirmwareName)) {
    $firmwarePath = $FirmwareName
} else {
    # Resolve a relative firmware name against the CWD first, then against the CALLING wrapper's directory:
    # release/Firmware/<target>/ holds the .bin next to its USB-Upload-Firmware.ps1, which is usually NOT the CWD.
    $firmwarePath = Join-Path $currentDir $FirmwareName
    if (-not (Test-Path $firmwarePath)) {
        $callerScript = (Get-PSCallStack | Select-Object -Skip 1 | Where-Object ScriptName | Select-Object -First 1).ScriptName
        if ($callerScript) {
            $alt = Join-Path (Split-Path -Parent $callerScript) $FirmwareName
            if (Test-Path $alt) { $firmwarePath = $alt }
        }
    }
}

$fwFileName = [System.IO.Path]::GetFileName($firmwarePath)
$fwDir      = [System.IO.Path]::GetDirectoryName($firmwarePath)
if ($_haveUi) {
    # The context block names the device the firmware belongs to, so the warning about matching device
    # and file is one the reader can act on rather than merely worry about.
    $_ftcExe = ""
    $_roots = @()
    foreach ($_rel in @("../Tools", "../../Tools", "../../../Tools", ".")) { $_roots += (Join-Path $PSScriptRoot $_rel) }
    $_ftc = OpenKNX_FindFtc -SearchDirs $_roots
    if ($_ftc.Installed) { $_ftcExe = $_ftc.Installed } elseif ($_ftc.Shipped) { $_ftcExe = $_ftc.Shipped }
    $_facts = OpenKNX_GetFirmwareFacts -FirmwarePath $firmwarePath -FtcExe $_ftcExe -Mcu $chipDisplay -Lang $_lang
    OpenKNX_ShowContext -Facts $_facts -Way $s.Way -WayHint $s.WayHintUsb -Note $s.NoteUsb -Lang $_lang
}
else {
    $sep = [string][char]0x2500 * 62
    Write-Host "  $sep" -ForegroundColor DarkGray
    Write-Host ($s.FirmwareFile -f $fwFileName) -ForegroundColor White
    Write-Host ($s.FirmwarePath -f $fwDir)      -ForegroundColor DarkGray
    Write-Host ($s.FirmwareWarn)                -ForegroundColor DarkYellow
    Write-Host "  $sep" -ForegroundColor DarkGray
    Write-Host
}

if (-not (Test-Path $firmwarePath)) {
    Write-Host ($s.FirmwareNotFound -f $firmwarePath) -ForegroundColor Red
    WaitOrPause
    exit 1
}

# Runtime-toggles (können über ?? Menü zur Laufzeit geändert werden)
$script:MultiMode = [bool]$Multi
$script:NoTimeout = $false   # disables the idle auto-close countdown in Read-Choice
# Fast path: flash a single detected device without the confirm menu. On by default
# ($FastSingleDevice); -Ask forces the prompt. Never active in Multi mode (deliberate per-device flow).
$script:FastFlash = ($FastSingleDevice -and -not $Ask)

# ══════════════════════════════════════════════════════════════════════════════
# RP2040 main loop
# ══════════════════════════════════════════════════════════════════════════════
if ($Chip -eq 'RP2040') {

    $flashCount = 0
    $continueFlashing = $true
    $esptool = FindEsptool   # optional: used only as a chip-id fallback when sysinfo fails

    while ($continueFlashing) {

        # ── Scan all devices ─────────────────────────────────────────────────
        if ($SerialPort) {
            # -SerialPort/-Port/-ComPort given: skip the search, use this port as if it were
            # the single device found. Classify as a BOOTSEL drive or a running serial port.
            $devices = @()
            if (@(ScanBootselPaths) -contains $SerialPort) {
                $devices += [PSCustomObject]@{ Type='bootsel'; Path=$SerialPort; Label="$SerialPort  $(Get-BootselFamily $SerialPort)  $($s.DeviceInBootsel)"; InfoReadFailed=$false }
            } else {
                $devices += [PSCustomObject]@{ Type='serial'; Path=$SerialPort; Label="$SerialPort  $($s.DeviceSerial)"; DeviceInfo=$null; InfoReadFailed=$false }
            }
        } else {
            Write-Host $s.SearchDevices
            $bootselPaths = @(ScanBootselPaths)
            $serialPorts  = @(ScanPicoPorts)

            $devices = @()
            foreach ($p in $bootselPaths) {
                $fam = Get-BootselFamily $p
                $devices += [PSCustomObject]@{ Type='bootsel'; Path=$p; Label="$p  $fam  $($s.DeviceInBootsel)"; InfoReadFailed=$false }
            }
            foreach ($p in $serialPorts) {
                $devices += [PSCustomObject]@{ Type='serial'; Path=$p; Label="$p  $($s.DeviceSerial)"; DeviceInfo=$null; InfoReadFailed=$false }
            }
        }

        # ── Enrich serial labels with device info (only when multiple devices) ──
        if ($devices.Count -gt 1 -and $serialPorts.Count -gt 0) {
            Write-Host $s.InfoReadingDevices -ForegroundColor DarkGray
            $espPorts = @(ScanEsp32Ports)
            $totalSerial = $serialPorts.Count
            $sIdx = 0
            for ($i = 0; $i -lt $devices.Count; $i++) {
                if ($devices[$i].Type -ne 'serial') { continue }
                $sIdx++
                Write-Host "  [$sIdx/$totalSerial] $($devices[$i].Path)... " -NoNewline
                Update-DeviceLabel $devices[$i] $espPorts $esptool
            }
        }

        if ($devices.Count -eq 0) {
            Write-Host $s.NoDeviceFound
            Write-Host
            Write-Host "  [A]  $($s.OptAgain)"
            Write-Host "  [R]  $($s.OptRescan)" -ForegroundColor DarkCyan
            Write-Host "  [X]  $($s.OptCancel)" -ForegroundColor DarkGray
            Write-Host
            $choice = ""
            $choice = Read-Choice "$($s.ChoicePrompt) [A/R/X]" @('A','R','X')
            if ($choice -eq 'X') {
                Show-ManualInstr
                $continueFlashing = $false
                continue
            } elseif ($choice -eq 'R') {
                continue   # rescan device list
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
        } elseif ($devResult -eq 'R') {
            Write-Host
            continue   # rescan device list
        } elseif ($devResult -eq 'A') {
            $allPaths     = @($devices | ForEach-Object { $_.Path })
            $appearedPath = Wait-AutoDetect $allPaths { @(ScanBootselPaths) + @(ScanPicoPorts) }
            if (-not $appearedPath) { Write-Host; continue }
            $isBootsel = @(ScanBootselPaths) -contains $appearedPath
            $selected  = [PSCustomObject]@{
                Type       = if ($isBootsel) { 'bootsel' } else { 'serial' }
                Path       = $appearedPath
                Label      = if ($isBootsel) { "$appearedPath  $(Get-BootselFamily $appearedPath)  $($s.DeviceInBootsel)" } else { "$appearedPath  $($s.DeviceSerial)" }
                DeviceInfo = $null
                    InfoReadFailed = $false
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
                Show-DeviceConfirmHeader $preInfo $port
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
            } else {
                # Sysinfo nicht lesbar -> Fallback: Chip-Typ via esptool (ESP-artige Ports)
                Write-Host
                $chipLbl = Get-ChipFallbackLabel $port $esptool (@(ScanEsp32Ports))
                if ($chipLbl) { Write-Host "  $chipLbl" -ForegroundColor DarkYellow }
                else          { Write-Host "  $($s.InfoUnavailableShort)" -ForegroundColor DarkYellow }
                Write-Host
            }

            # ── J/A/N Confirm-Menü (Fast-Path: genau EIN Gerät -> ohne Rückfrage) ─
            $confirm = 'J'
            if ($script:FastFlash -and -not $script:MultiMode -and $devices.Count -eq 1) {
                Write-Host "  $($s.FastFlashNote)" -ForegroundColor DarkGray
            } else {
                Write-Host "  [J]  $($s.OptFlashBootsel)" -ForegroundColor Green
                Write-Host "  [A]  $($s.OptAgain)"
                Write-Host "  [X]  $($s.OptCancel)"
                Write-Host
                $confirm = Read-Choice "$($s.ChoicePrompt) [J/A/X]" @('J','Y','A','X')
            }
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
                    InfoReadFailed = $false
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
            $newBootsel = @(Wait-BootselPaths 15)   # poll for the mount instead of a fixed sleep
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
        [void](Wait-BootselReady $devicePath 10)   # FAT must be readable before writing

        # ── Verify uf2 family matches the BOOTSEL device (RP2040 vs RP2350) ────
        if ($VerifyRpFamily) {
            $fwFam  = Get-Uf2Family $firmwarePath
            $devFam = Get-BootselFamily $devicePath
            if ($fwFam -and $devFam -and $fwFam -ne $devFam) {
                Write-Host
                if ($AbortOnChipMismatch) {
                    Write-Host ("  " + ($s.RpMismatch -f $fwFam, $devFam)) -ForegroundColor Red
                    if (-not $script:MultiMode) { $continueFlashing = $false }
                    WaitOrPause 10
                    continue
                }
                Write-Host ("  " + ($s.RpMismatchWarn -f $fwFam, $devFam)) -ForegroundColor DarkYellow
            } elseif ($fwFam -and $devFam) {
                Write-Host ("  " + ($s.RpFamilyOk -f $devFam)) -ForegroundColor Green
            }
        }

        # ── Flash ─────────────────────────────────────────────────────────────
        $serialBefore = @(ScanPicoPorts)   # snapshot before copy; device will reboot as serial
        Write-Host
        $fwBaseName = [System.IO.Path]::GetFileName($firmwarePath)

        # -Verify (opt-in): write + read-back verify via picotool straight from BOOTSEL.
        # Falls back to the normal drag-and-drop copy if picotool is not installed.
        $picotool = if ($Verify) { FindPicotool } else { $null }
        if ($Verify -and -not $picotool) {
            Write-Host ("  " + $s.PicotoolNotFound) -ForegroundColor DarkYellow
        }

        if ($picotool) {
            Write-Host ("  " + ($s.Installing -f $fwBaseName)) -ForegroundColor Yellow
            Write-Host ("  " + $s.VerifyRpTitle) -ForegroundColor DarkGray
            $pres = Invoke-PicotoolLoadVerify $picotool $firmwarePath
            if ($pres.Code -ne 0) {
                Write-Host ("  " + $s.VerifyRpFail) -ForegroundColor Red
                foreach ($line in ($pres.Output -split "`r?`n")) { if ($line.Trim()) { Write-Host "     $line" -ForegroundColor Red } }
                $continueFlashing = $false
                WaitOrPause 10
                continue
            }
            $flashCount++
            Write-Host ("  " + $s.VerifyRpOk) -ForegroundColor Green
        } else {
            $flashOk = Copy-FirmwareWithSpinner ($s.Installing -f $fwBaseName) $firmwarePath $devicePath
            if (-not $flashOk) {
                Write-Host $s.FlashError -ForegroundColor Red
                $continueFlashing = $false
                WaitOrPause 10
                continue
            }
            $flashCount++
            Write-Host $s.Done -ForegroundColor Green
        }

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
            WaitOrPause 1
        }
    }

    if ($script:MultiMode -and $flashCount -gt 0) {
        Write-Host
        Write-Host ($s.FlashedTotal -f $flashCount) -ForegroundColor Green
        WaitOrPause 1
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
        if ($SerialPort) {
            # -SerialPort/-Port/-ComPort given: skip the search, use this port as if it were
            # the single device found.
            $devices = @([PSCustomObject]@{ Type='esp32'; Path=$SerialPort; Label="$SerialPort  $($s.DeviceEsp32Serial)"; DeviceInfo=$null; InfoReadFailed=$false })
        } else {
            Write-Host $s.SearchDevices
            $ports = @(ScanEsp32Ports)

            $devices = @()
            foreach ($p in $ports) {
                $devices += [PSCustomObject]@{ Type='esp32'; Path=$p; Label="$p  $($s.DeviceEsp32Serial)"; DeviceInfo=$null; InfoReadFailed=$false }
            }
        }

        # ── Enrich serial labels with device info (only when multiple devices) ──
        if ($devices.Count -gt 1) {
            Write-Host $s.InfoReadingDevices -ForegroundColor DarkGray
            $totalSerial = $devices.Count
            for ($i = 0; $i -lt $devices.Count; $i++) {
                Write-Host "  [$($i+1)/$totalSerial] $($devices[$i].Path)... " -NoNewline
                Update-DeviceLabel $devices[$i] $ports $esptool
            }
        }

        if ($devices.Count -eq 0) {
            Write-Host $s.NoDeviceFound
            Write-Host
            Write-Host "  [A]  $($s.OptAgain)"
            Write-Host "  [R]  $($s.OptRescan)" -ForegroundColor DarkCyan
            Write-Host "  [X]  $($s.OptCancel)" -ForegroundColor DarkGray
            Write-Host
            $choice = ""
            $choice = Read-Choice "$($s.ChoicePrompt) [A/R/X]" @('A','R','X')
            if ($choice -eq 'X') {
                Show-Esp32Instr
                $continueFlashing = $false
                continue
            } elseif ($choice -eq 'R') {
                continue   # rescan device list
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
        } elseif ($devResult -eq 'R') {
            Write-Host
            continue   # rescan device list
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
        $espInfo = if ($selected.PSObject.Properties['DeviceInfo']) { $selected.DeviceInfo } else { $null }
        if ($espInfo) {
            Show-DeviceConfirmHeader $espInfo $selected.Path
            Write-Host
            Write-Host "  $($selected.Path)  $($s.DeviceEsp32Serial)" -ForegroundColor DarkGray
        } else {
            Write-Host "  $($selected.Path)  $($s.DeviceEsp32Serial)" -ForegroundColor Cyan
        }
        Write-Host
        # Fast path: exactly one device found -> flash directly (default on; -Ask / Multi force the menu).
        $confirmEsp = 'J'
        if ($script:FastFlash -and -not $script:MultiMode -and $devices.Count -eq 1) {
            Write-Host "  $($s.FastFlashNote)" -ForegroundColor DarkGray
        } else {
            Write-Host "  [J]  $($s.OptFlashEsp32)" -ForegroundColor Green
            Write-Host "  [A]  $($s.OptAgain)"
            Write-Host "  [X]  $($s.OptCancel)"
            Write-Host
            $confirmEsp = Read-Choice "$($s.ChoicePrompt) [J/A/X]" @('J','Y','A','X')
        }
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

        # ── Verify chip type via esptool (safety gate, reuses esptool) ─────────
        if ($VerifyEspChip) {
            Write-Host
            Write-Host "  $($s.VerifyChip)" -ForegroundColor DarkGray
            $chipInfo = Get-Esp32Info $esptool $selected.Path
            $detectedChip = if ($chipInfo) { $chipInfo.Chip } else { $null }
            if (-not $detectedChip) {
                Write-Host "  $($s.ChipNotEsp)" -ForegroundColor Red
                Show-Esp32Instr
                if (-not $script:MultiMode) { $continueFlashing = $false; WaitOrPause }
                continue
            }
            $fwVariant = Get-EspFwVariant $fwFileName
            if ($fwVariant -and (Get-EspModel $detectedChip) -ne (Get-EspModel $fwVariant)) {
                if ($AbortOnChipMismatch) {
                    Write-Host ("  " + ($s.ChipMismatch -f $detectedChip, $fwVariant)) -ForegroundColor Red
                    if (-not $script:MultiMode) { $continueFlashing = $false }
                    WaitOrPause 10
                    continue
                }
                Write-Host ("  " + ($s.ChipMismatchWarn -f $detectedChip, $fwVariant)) -ForegroundColor DarkYellow
            } elseif ($fwVariant) {
                Write-Host ("  " + ($s.ChipMatch -f $detectedChip)) -ForegroundColor Green
            } else {
                Write-Host ("  " + ($s.ChipOk -f $detectedChip)) -ForegroundColor Green
            }
        }

        # ── Flash via esptool (our progress bar, fed by esptool's own % output) ─
        Write-Host
        Write-Host "  $($s.FlashingEsp32)" -ForegroundColor Cyan
        if ($script:DebugSerial) {
            Write-Host "  $esptool --port $($selected.Path) --baud 460800 $(Get-EsptoolCmd $esptool 'write_flash') 0x0 $firmwarePath" -ForegroundColor DarkGray
        }
        $flashRes = Invoke-EsptoolWriteFlash $esptool $selected.Path $firmwarePath

        if ($flashRes.Code -ne 0) {
            Write-Host "  $($s.FlashErrorEsp32)" -ForegroundColor Red
            foreach ($line in ($flashRes.Output -split "`r?`n")) { if ($line.Trim()) { Write-Host "     $line" -ForegroundColor Red } }
            $continueFlashing = $false
            WaitOrPause 10
            continue
        }

        $flashCount++
        Write-Host "  $($s.FlashDoneEsp32)" -ForegroundColor Green
        # esptool already read the flash back and hashed it – show that it was verified.
        if ($flashRes.Verified) {
            Write-Host "  $($s.FlashVerifiedEsp)" -ForegroundColor Green
        } else {
            Write-Host "  $($s.FlashNotVerifiedEsp)" -ForegroundColor DarkGray
        }

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
        #WaitOrPause 5
    }
}
