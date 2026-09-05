# This script builds the release variant of the firmware.
# Call it with the following parameters:
#    scripts/Build-Step.ps1 <pio-environment> <firmware-name> <binary-format> [<product-name>] <project-dir>
#    scripts/Build-Step.ps1 -pioEnv <pio-environment> -firmwareName <firmware-name> -binaryFormat <binary-format> [-productName <product-name>] -projectDir <project-dir>
# 
# <pio-environment> is the env: entry from platformio.ini, which defines how to build the firmware. 
#                   For [env:RP_2040], use "RP_2040" as the <pio-environment>.
# <firmware-name>   is the name of the firmware file that will be created. It is important to have
#                   different firmware names for different hardware builds within the same release.
#                   There should always be an "Upload-Firmware-<firmwarename>.ps1" script delivered,
#                   which installs this firmware.
# <binary-format>   is interpreted as an enum with some depricated values (for compatibility):
#                       bin (deprecated)   - old SAMD processor (deprecated)
#                       uf2 (deprecated)   - RP2040 without OTA
#                       esp32 (deprecated) - ESP32 with OTA
#                       esp32-ip (new)     - ESP32 with OTA
#                       esp32-tp (new)     - ESP32 with KNX
#                       esp32-tpip (new)   - ESP32 with KNX and OTA
#                       esp32-iptp (new)   - ESP32 with KNX and OTA
#                       rp2040-ip (new)    - RP2040 with OTA
#                       rp2040-tp (new)    - RP2040 with KNX
#                       rp2040-tpip (new)  - RP2040 with KNX and OTA
#                       rp2040-iptp (new)  - RP2040 with KNX and OTA
#                       rp2350-ip (new)    - RP2350 with OTA
#                       rp2350-tp (new)    - RP2350 with KNX
#                       rp2350-tpip (new)  - RP2350 with KNX and OTA
#                       rp2350-iptp (new)  - RP2350 with KNX and OTA
# <product-name>    (optional) is the name of the product. If not provided, the firmware name without
#                   the "firmware-" prefix will be used.
# <project-dir>     is the directory path of the project.
#
# This file does not require any changes and is project-independent.

param (
  [Parameter(Mandatory = $false, HelpMessage="Initiate debug build, -DebugBuild should be first odr last parameter")]
  [switch]$DebugBuild,

  [Parameter(Mandatory = $true)]
  [ValidateNotNullOrEmpty()]
  [string]$pioEnv,

  [Parameter(Mandatory = $true)]
  [ValidateNotNullOrEmpty()]
  [string]$firmwareName,

  [Parameter(Mandatory = $true)]
  [ValidateNotNullOrEmpty()]
  [string]$featureSet,
  
  [Parameter(Mandatory = $false)]
  [string]$productName,

  [Parameter(Mandatory = $false)]
  [string]$ProjectDir
)

$buildMode = if ($DebugBuild) { "debug"} else { "run" }

# Try to find PlatformIO executable
# Priority:
# 1. In PATH (most common - IDE extension or standalone installer)
# 2. In Python virtual environment (pip install platformio)

$pioPath = $null

# Try to find in PATH first (works for IDE extension and standalone)
$pioCmd = Get-Command pio, pio.exe -ErrorAction SilentlyContinue | Select-Object -First 1
# Windows PowerShell 5.1 defines neither $IsMacOS nor $IsLinux; reading them there yields $null, so the
# Windows branch is taken for the right reason rather than by accident (and it survives Set-StrictMode).
if ($null -eq (Get-Variable -Name 'IsMacOS' -ErrorAction SilentlyContinue)) {
    $IsMacOS = $false; $IsLinux = $false; $IsWindows = $true
}

if ($pioCmd) {
  $pioPath = $pioCmd.Source
  Write-Host "Found PlatformIO in PATH: $pioPath" -ForegroundColor DarkGray
}
else {
  # Fallback: Try Python virtual environment location
  if ($IsMacOS -or $IsLinux) { 
    $pioPath = Join-Path $HOME (Join-Path ".platformio" (Join-Path "penv" (Join-Path "bin" "pio")))
  }
  else { 
    $pioPath = Join-Path $HOME (Join-Path ".platformio" (Join-Path "penv" (Join-Path "Scripts" "pio.exe")))
  }
  
  if (Test-Path $pioPath) {
    Write-Host "Found PlatformIO in venv: $pioPath" -ForegroundColor DarkGray
  }
  else {
    Write-Host "ERROR: PlatformIO not found!" -ForegroundColor Red
    Write-Host "  Searched in PATH and at: $pioPath" -ForegroundColor Yellow
    Write-Host "  Please install PlatformIO: https://platformio.org/install/cli" -ForegroundColor Yellow
    exit 1
  }
}

# --------------------------------------------------------------------------- #
# Layer 1: Static IDF build detection -- quick check for "custom_idf_build = true" in pio project config.
# Runs BEFORE 'pio run' and gives an early warning for long builds.
# --------------------------------------------------------------------------- #
$isIdfBuild = $false
try {
    $pioConfigOutput = & $pioExe project config -e $pioEnv 2>$null | Out-String
    if ($pioConfigOutput -match "custom_idf_build\s*=\s*(true|yes|1)") {
        Write-Host ""
        $isIdfBuild = $true
        Write-Host "[$pioEnv] IDF build detected (custom_idf_build = true)"
        Write-Host "[$pioEnv]   Phase 2 (Arduino app compile): fast after successful Phase 1, otherwise also takes while"
        Write-Host "[$pioEnv]   Phase 1 (IDF libs from source): takes while on first build or after clean"
        Write-Host ""
    }
} catch {
    Write-Host "[$pioEnv] WARNING: pio project config query failed -- proceeding without early IDF detection"
}

& $pioPath $buildMode -e $pioEnv
if (!$?) {
  Write-Host "$pioEnv build failed, Firmware was not built!"
  exit 1
}

# --------------------------------------------------------------------------- #
# Layer 3: Marker file check for IDF build and sdkconfig cache hit -- 
#         gives detailed info about IDF version and whether Phase 1 was a cache hit or rebuild.
# idf_generate_crt_asm.py (pre-script).  Exists only if the IDF build was actually executed, so it is a more reliable indicator than Layer 1's static config check.
#         Also gives a warning if Layer 1 detected an IDF build but the marker file is missing, which may indicate that the pre-script did not run and the IDF build was not
# IDF-Build war UND das pre-script erfolgreich durchlief.
# --------------------------------------------------------------------------- #
$markerRelPath = ".pio/build/$pioEnv/idf_build.marker"
$markerPath = if (![string]::IsNullOrEmpty($ProjectDir)) {
    Join-Path $ProjectDir $markerRelPath
} else {
    $markerRelPath
}
if (Test-Path $markerPath) {
    # key=value Paare parsen (Kommentarzeilen ueberspringen)
    $markerData = @{}
    Get-Content $markerPath | Where-Object { $_ -match "^[^#]+=.+" } | ForEach-Object {
        $kv = $_ -split "=", 2
        $markerData[$kv[0].Trim()] = $kv[1].Trim()
    }
    $isIdfBuild = $true
    $cacheHit   = $markerData["idf_cache_hit"] -eq "true"
    $idfVer     = $markerData["idf_version"]
    Write-Host ""
    Write-Host "[$pioEnv] IDF build summary:"
    Write-Host "[$pioEnv]   ESP-IDF version : $idfVer"
    if ($cacheHit) {
        Write-Host "[$pioEnv]   Phase 1 status  : SKIPPED  (sdkconfig.defaults cache hit)"
    } else {
        Write-Host "[$pioEnv]   Phase 1 status  : REBUILT from source"
    }
    Write-Host ""
} elseif ($isIdfBuild) {
    # Layer 1 erkannte IDF, aber Marker fehlt -> pre-script lief moeglicherweise nicht
    Write-Host "[$pioEnv] WARNING: IDF build detected but marker file not found."
    Write-Host "[$pioEnv]   Expected: $markerPath"
    Write-Host "[$pioEnv]   Possible cause: idf_generate_crt_asm.py fehlt in extra_scripts"
}

# featureSet replaces the old binaryFormat setting in a compatible way
# it is interpreted as an enum with some depricated values (for compatibility)
# bin (deprecated) - old SAMD processor
# uf2 (deprecated) - RP2040 without OTA
# esp32 - (deprecated) ESP32 with OTA
# esp32-ip (new) esp32 with OTA
# esp32-tp (new) esp32 with KNX
# rp2040-ip (new) rp2040 with OTA
# rp2040-tp (new) rp2040 with KNX
# rp2350-ip (new) rp2350 with OTA
# rp2350-tp (new) rp2350 with KNX
# inherent logic: 
# - a device with OTA does not need a KNX-Upload
# - esp is always IP and OTA is always possible
# - RP2040/2350 needs to distinguish 


# binaryFormat uf2 means rp2040 without OTA
$binaryFormat = "uf2"
$OTAbinaryFormat = "bin"
$processor = "RP2040"
$withIP = $false;
if ($featureSet -eq "bin") {
  $processor = "SAMD"
  $binaryFormat = "bin"
} elseif ($featureSet -eq "esp32" -or $featureSet -eq "esp32-ip") {
  $binaryFormat = "factory.bin"
  $processor = "ESP32"
  $withIP = $true;
} elseif ($featureSet -eq "esp32-tp") {
  $binaryFormat = "factory.bin"
  $processor = "ESP32"
  $withIP = $false;
} elseif ($featureSet -eq "esp32-tpip" -or $featureSet -eq "esp32-iptp") {
  $binaryFormat = "factory.bin"
  $processor = "ESP32"
  $withIP = $true;
} elseif ($featureSet -eq "rp2040-ip" -or $featureSet -eq "rp2350-ip") {
  $withIP = $true;
} elseif ($featureSet -eq "rp2040-tpip" -or $featureSet -eq "rp2040-iptp" -or $featureSet -eq "rp2350-tpip" -or $featureSet -eq "rp2350-iptp") {
  $withIP = $true;
} elseif ($featureSet -eq "uf2" -or $featureSet -eq "rp2040-tp" -or $featureSet -eq "rp2350-tp") {
  $withIP = $false;
} else {
  Write-Host "ERROR: Wrong featureset $featureSet in Build-Step!"
  exit 1
}

# if no product name is given, use firmware name without "firmware-" prefix
if (!$productName) {
  $productName = $firmwareName.Replace("firmware-", "")
}


# ─────────────────────────────────────────────────────────────────────────────────────────────────
# The facts a tool would otherwise have to assume about a firmware package, written down by the build
# that actually knows them. Everything that reads a release -- ftc, the extractor script, anything
# third-party -- reads this instead of hard-coding an offset or guessing where an image ends.
#
# Derived AND VERIFIED here: the slice this file describes is compared against the real application
# image, so a layout change breaks the BUILD visibly instead of breaking a user's tool silently.
# ─────────────────────────────────────────────────────────────────────────────────────────────────
# Order the identity block is written in -- grouped by what a reader looks for first.
$ExtraOrder = @("orderNumber", "firmwareName", "firmwareVersion", "mcu",
                "openKnxId", "appNumber", "appVersion",
                "env", "buildDate", "mainVersion")

# One #define out of a generated header. Returns the token with quotes stripped, or "".
function Get-DefineValue([string]$File, [string]$Name) {
    if (-not (Test-Path -PathType Leaf $File)) { return "" }
    foreach ($line in (Get-Content -LiteralPath $File)) {
        if ($line -match ('^\s*#define\s+' + [regex]::Escape($Name) + '\s+(.+?)\s*$')) {
            return $Matches[1].Trim().Trim('"')
        }
    }
    return ""
}

# The MCU as a tool needs it: the .uf2 family id says RP2040 vs RP2350 exactly, a .factory.bin is ESP32.
# Derived from the package, not from the file extension -- the extension is what tools guess today.
function Get-PackageMcu([string]$Package, [string]$Format) {
    if ($Format -eq "factory") { return "ESP32" }
    if ($Format -ne "uf2" -or -not (Test-Path -PathType Leaf $Package)) { return "" }
    try {
        $fs = [System.IO.File]::OpenRead($Package)
        $b = New-Object byte[] 32
        $n = $fs.Read($b, 0, 32); $fs.Close()
        if ($n -lt 32) { return "" }
        $fam = [BitConverter]::ToUInt32($b, 28)
        # The L suffix matters: without it PowerShell parses 0xE48BFF56 as a NEGATIVE Int32 and the
        # comparison against the UInt32 read from the file is never true.
        if ($fam -eq 0xE48BFF56L) { return "RP2040" }
        if ($fam -eq 0xE48BFF59L -or $fam -eq 0xE48BFF5AL -or $fam -eq 0xE48BFF5BL) { return "RP2350" }
    } catch { }
    return ""
}

function Write-ImageFacts {
    param(
        [string]$AppImage,   # the raw application image (.pio/build/<env>/firmware.bin)
        [string]$Package,    # the .uf2 / .factory.bin the release ships
        [string]$Format,     # uf2 | factory | raw
        [string]$Target,     # where to write the facts file
        [hashtable]$Extra = @{}  # identity/provenance, in the order given by $ExtraOrder
    )
    if (-not (Test-Path -PathType Leaf $AppImage)) { return $false }
    $app = [System.IO.File]::ReadAllBytes($AppImage)
    $sha = [System.BitConverter]::ToString(
        [System.Security.Cryptography.SHA256]::Create().ComputeHash($app)).Replace("-", "").ToLower()

    $appOffset = 0
    if ($Format -eq "factory" -and (Test-Path -PathType Leaf $Package)) {
        # Read from the partition table rather than assume 0x10000: if the layout ever moves, this
        # follows it, and the check below catches it if it does not.
        $pkg = [System.IO.File]::ReadAllBytes($Package)
        $PT = 0x8000
        for ($e = $PT; ($e + 32) -le $pkg.Length -and $e -lt ($PT + 0x1000); $e += 32) {
            if ($pkg[$e] -ne 0xAA -or $pkg[$e + 1] -ne 0x50) { break }
            if ($pkg[$e + 2] -ne 0) { continue }                       # type 0 = app
            $off = [uint32]$pkg[$e + 4] + ([uint32]$pkg[$e + 5] * 256) + `
                   ([uint32]$pkg[$e + 6] * 65536) + ([uint32]$pkg[$e + 7] * 16777216)
            $sub = $pkg[$e + 3]
            if ($sub -eq 0) { $appOffset = [int]$off; break }
            if ($appOffset -eq 0 -and $sub -ge 0x10 -and $sub -le 0x1F) { $appOffset = [int]$off }
        }
        if ($appOffset -eq 0 -or ($appOffset + $app.Length) -gt $pkg.Length) {
            Write-Host "ERROR: the application image does not fit where the partition table says" -ForegroundColor Red
            return $false
        }
        # The stated slice MUST be the application image. If it is not, the assumption is already wrong
        # and the build should say so now, not a user's tool three months from now.
        for ($i = 0; $i -lt 4096; $i++) {
            if ($pkg[$appOffset + $i] -ne $app[$i]) {
                Write-Host "ERROR: the package does not carry the application image at offset $appOffset" -ForegroundColor Red
                return $false
            }
        }
    }

    $nl = [Environment]::NewLine
    $txt = "# OpenKNX firmware image facts - written by the build, read by tools." + $nl +
           "# appLength is what a .uf2 cannot state: it is block-padded, so its payload is longer." + $nl +
           "format          = $Format" + $nl +
           "package         = " + (Split-Path -Leaf $Package) + $nl +
           "appOffset       = $appOffset" + $nl +
           "appLength       = " + $app.Length + $nl +
           "appSha256       = $sha" + $nl
    if ($Extra.Count -gt 0) {
        # Identity and provenance: what the package cannot say about itself. Tools compare on
        # orderNumber (the KNX order info, what ETS shows) -- firmwareName carries a build suffix.
        $txt += $nl + "# identity and provenance" + $nl
        foreach ($k in $ExtraOrder) {
            if ($Extra.ContainsKey($k) -and "$($Extra[$k])" -ne "") {
                $txt += ($k.PadRight(15) + " = " + $Extra[$k]) + $nl
            }
        }
    }
    [System.IO.File]::WriteAllText($Target, $txt, (New-Object System.Text.UTF8Encoding $false))
    return $true
}

# Create source and target path for firmware
$CopyItem_Source = ".pio/build/$pioEnv/firmware.$binaryFormat"
$CopyItem_Target_Root = "release"
$CopyItem_Target_Data = "$CopyItem_Target_Root/data"
$CopyItem_Target_Device = "$productName"
$CopyItem_Target_Dir = "$CopyItem_Target_Root/Firmware/$CopyItem_Target_Device"
$CopyItem_Target_Name = "$firmwareName.$binaryFormat"
if (![string]::IsNullOrEmpty($ProjectDir)) {
  $CopyItem_Source = Join-Path $ProjectDir $CopyItem_Source
  $CopyItem_Target_Data = Join-Path $ProjectDir $CopyItem_Target_Data
  $CopyItem_Target_Dir = Join-Path $ProjectDir $CopyItem_Target_Dir
}
$CopyItem_Target = Join-Path $CopyItem_Target_Dir $CopyItem_Target_Name

# Check if firmware is available and copy it to release
Write-Host "The $PioEnv firmware is available as $CopyItem_Source"
if ( Test-Path $CopyItem_Source ) {
  Write-Host "Copy-Item: $CopyItem_Source to $CopyItem_Target"
  # create target directories if not exists
  if (!(Test-Path -Path $CopyItem_Target_Dir)) {
    New-Item -ItemType Directory -Force -Path $CopyItem_Target_Dir | Out-Null
  }
  if (!(Test-Path -Path $CopyItem_Target_Data)) {
    New-Item -ItemType Directory -Force -Path $CopyItem_Target_Data | Out-Null
  }
  
  # copy firmware to release
  Copy-Item $CopyItem_Source $CopyItem_Target 
  if (!$?) {
    Write-Host "ERROR: Firmware could noch be copied!"
    exit 1
  }

  # NOT copied any more: the OTA image and the "application image" were the same bytes as the package
  # already carries, shipped twice more. A release now holds ONE file per device; everything that needs
  # the raw image derives it, guided by the <name>.image.txt written below.

  # Raw application image. Everything else in a release is a WRAPPER: .uf2 carries the image in 256-byte
  # blocks, .factory.bin prepends bootloader and partition table. A delta update needs the image itself,
  # because that is what the device is running and what a patch is computed against. Without this file a
  # user would have to unwrap a release by hand before a patch could be built for it -- so it ships, and
  # the next release can be reached from this one.
  $CopyItem3_Source = ".pio/build/$pioEnv/firmware.bin"
  if (![string]::IsNullOrEmpty($ProjectDir)) {
    $CopyItem3_Source = Join-Path $ProjectDir ".pio/build/$pioEnv/firmware.bin"
  }
  if (Test-Path $CopyItem3_Source) {

    # Alongside it: what the package does NOT say about itself.
    $factsFormat = "raw"
    if ($binaryFormat -eq "uf2") { $factsFormat = "uf2" }
    elseif ($binaryFormat -eq "factory.bin") { $factsFormat = "factory" }
    $factsFile = Join-Path $CopyItem_Target_Dir "$firmwareName.image.txt"

    # Identity straight out of the generated headers -- the same numbers the firmware reports at runtime,
    # so a tool can compare the two without unpacking anything. The ETS version is "major.minor.revision":
    # major/minor are the two nibbles of MAIN_ApplicationVersion, the revision is its own define.
    $hdrDir  = if ([string]::IsNullOrEmpty($ProjectDir)) { "include" } else { Join-Path $ProjectDir "include" }
    $knxprod = Join-Path $hdrDir "knxprod.h"
    $vers    = Join-Path $hdrDir "versions.h"
    $appVerRaw = Get-DefineValue $knxprod "MAIN_ApplicationVersion"
    $fwVersion = ""
    if ($appVerRaw -match '^\d+$') {
        $av = [int]$appVerRaw
        $rev = Get-DefineValue $knxprod "MAIN_FirmwareRevision"
        if ($rev -notmatch '^\d+$') { $rev = "0" }
        $fwVersion = "{0}.{1}.{2}" -f (($av -band 0xF0) -shr 4), ($av -band 0x0F), $rev
    }
    $extra = @{
        orderNumber     = Get-DefineValue $knxprod "MAIN_OrderNumber"
        firmwareName    = Get-DefineValue $knxprod "MAIN_FirmwareName"
        firmwareVersion = $fwVersion
        mcu             = Get-PackageMcu $CopyItem_Target $factsFormat
        openKnxId       = Get-DefineValue $knxprod "MAIN_OpenKnxId"
        appNumber       = Get-DefineValue $knxprod "MAIN_ApplicationNumber"
        appVersion      = $appVerRaw
        env             = $pioEnv
        buildDate       = (Get-Date -Format "yyyy-MM-dd")
        mainVersion     = Get-DefineValue $vers "MAIN_Version"
    }
    if (Write-ImageFacts -AppImage $CopyItem3_Source -Package $CopyItem_Target -Format $factsFormat -Target $factsFile -Extra $extra) {
      Write-Host "Wrote: $factsFile  ($factsFormat, $((Get-Item $CopyItem3_Source).Length) B)"
    }
    else {
      Write-Host "ERROR: image facts could not be written for $firmwareName" -ForegroundColor Red
      exit 1
    }
  }
}
else {
  # firmware not found
  Write-Host "ERROR: $CopyItem_Source not found!"
  exit 1
}

# create Upload-Firmware-<firmwarename>.ps1 script

# need to do this BEFORE the USB Upload-File is created
# OTA-Upload
if ($withIP) {
  # create OTA-Upload-Firmware-<firmwarename>.ps1 script
  $fileName = "$CopyItem_Target_Dir/OTA-Upload-Firmware.ps1"
  # Chip-specific OTA args: explicit espota port + chip flag, derived from the featureSet so RP2040 and
  # RP2350 are distinguished. ESP32 uploads the raw .bin; RP gzips it -- the OTA-Upload script decides via
  # the -ESP32 / -RP2040 / -RP2350 flag. Ports: ESP32 = 3232, RP2040/RP2350 = 2040 (PicoOTA).
  $espotaArgs = ""; $chipArg = ""
  if     ($featureSet -match "esp32")  { $espotaArgs = "'-p 3232'"; $chipArg = "-ESP32" }
  elseif ($featureSet -match "rp2350") { $espotaArgs = "'-p 2040'"; $chipArg = "-RP2350" }
  elseif ($featureSet -match "rp2040") { $espotaArgs = "'-p 2040'"; $chipArg = "-RP2040" }

  # Write the script file content to the file
  $scriptContent = "& `"`$PSScriptRoot/../../data/OTA-Upload-Firmware-Generic.ps1`" $CopyItem_Target_Name $espotaArgs $chipArg"
  if (Test-Path $fileName) { Clear-Content -Path $fileName }
  Add-Content -Path $fileName -Value $scriptContent
  if (!$?) {
    Write-Host "ERROR: $fileName could not be created!"
    exit 1
  }
} 
# KNX-Upload
# Every processor that runs the file-transfer module can be updated over the bus -- an ESP32 just as much
# as an RP. The script used to be tied to $withTP, which is about the KNX MEDIUM, not about whether the
# device can take an update this way; a device declared "ip" was left without the script even though it
# sits on the same bus. SAMD is the one exception: no file-transfer module, no bus update.
if ($processor -ne "SAMD") {
  # create KNX-Upload-Firmware-<firmwarename>.ps1 script
  $fileName = "$CopyItem_Target_Dir/KNX-Upload-Firmware.ps1"

  # NOT $CopyItem_Target_Name: that is the USB format, and for an ESP32 it is the .factory.bin -- an
  # esptool package with the bootloader and the partition table in front of the image. The device's own
  # updater writes an APPLICATION image, and a .factory.bin does not even state its identity at offset 0,
  # so knxOTA refused it outright. Over the bus an ESP32 therefore gets the raw application image, which
  # is copied for every target anyway; an RP keeps the .uf2, which is the only RP file carrying identity.
  # The package, for both families: ftc reads the application image out of it, guided by image.txt.
  $knxUploadName = $CopyItem_Target_Name
  # Write the script file content to the file. @args forwards -Ip / -Pa / -From / -NoDelta, so the same
  # wrapper serves both the interactive run and a scripted one.
  $scriptContent = "& `"`$PSScriptRoot/../../data/KNX-Upload-Firmware-Generic.ps1`" $knxUploadName @args"
  if (Test-Path $fileName) { Clear-Content -Path $fileName }
  Add-Content -Path $fileName -Value $scriptContent
  if (!$?) {
    Write-Host "ERROR: $fileName could not be created!"
    exit 1
  }
}

# Prepare-Firmware
# The release ships ONE file per device -- a package. Everything that needs the raw application image
# derives it (ftc for the bus, the OTA script for the network); this is the same derivation for a person
# who wants the files themselves: the plain image for USB or their own checksum, the gzipped one for a
# knxOTA transfer, or a difference to an older release. The three upload scripts next to it send; this
# one prepares. None of them calls it.
$fileName = "$CopyItem_Target_Dir/Prepare-Firmware.ps1"
$scriptContent = "& `"`$PSScriptRoot/../../data/Prepare-Firmware-Generic.ps1`" `"`$PSScriptRoot/$CopyItem_Target_Name`" @args"
if (Test-Path $fileName) { Clear-Content -Path $fileName }
Add-Content -Path $fileName -Value $scriptContent
if (!$?) {
  Write-Host "ERROR: $fileName could not be created!"
  exit 1
}

# USB-Upload
# create Upload-Firmware-<firmwarename>.ps1 script
$fileName = "$CopyItem_Target_Dir/USB-Upload-Firmware.ps1"

# Write the script file content to the file 
$scriptContent = "& `"`$PSScriptRoot/../../data/Upload-Firmware-Generic.ps1`" $CopyItem_Target_Name"
if (Test-Path $fileName) { Clear-Content -Path $fileName }
Add-Content -Path $fileName -Value $scriptContent
if (!$?) {
  Write-Host "ERROR: $fileName could not be created!"
  exit 1
}

#check if file exists content.xml exists then add the closing tags
$releaseTarget = "$CopyItem_Target_Data/content.xml"

#check if file exists content.xml exists if not create it
if (Test-Path -Path $releaseTarget -PathType Leaf) {
  # Add entry to content.xml. If entry already exists, do nothing. If not, add it. If file does not exist, create it.
  $XMLContent = "         <Product Name=""$productName"" Firmware=""../Firmware/$CopyItem_Target_Device/$CopyItem_Target_Name"" Processor=""$processor"" />"
  $lineExists = Select-String -Path $fileName -Pattern $XMLContent -Quiet
  if (-not $lineExists) { Add-Content -Path $releaseTarget -Value $XMLContent }
}
else {
  Write-Host "ERROR - Buildstep: $releaseTarget could not be found!" -ForegroundColor Red
  exit 1
}

exit 0
