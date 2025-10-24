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
# <binary-format>   currently supports "uf2" for RP2040, "bin" for SAMD and "esp32" for ESP32
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
if ($IsMacOS -or $IsLinux) { ~/.platformio/penv/bin/pio $buildMode -e $pioEnv }
else { ~/.platformio/penv/Scripts/pio.exe $buildMode -e $pioEnv }
if (!$?) {
  Write-Host "$pioEnv build failed, Firmware was not built!"
  exit 1
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
$withTP = $true;
if ($featureSet -eq "bin") {
  $processor = "SAMD"
  $binaryFormat = "bin"
} elseif ($featureSet -eq "esp32" -or $featureSet -eq "esp32-ip") {
  $binaryFormat = "factory.bin"
  $processor = "ESP32"
  $withIP = $true;
  $withTP = $false;
} elseif ($featureSet -eq "esp32-tp") {
  $binaryFormat = "factory.bin"
  $processor = "ESP32"
  $withIP = $false;
  $withTP = $false; # set to true as sooon as we support OTA via knx
} elseif ($featureSet -eq "esp32-tpip" -or $featureSet -eq "esp32-iptp") {
  $binaryFormat = "factory.bin"
  $processor = "ESP32"
  $withIP = $true;
  $withTP = $false; # set to true as sooon as we support OTA via knx
} elseif ($featureSet -eq "rp2040-ip" -or $featureSet -eq "rp2350-ip") {
  $withIP = $true;
  $withTP = $false;
} elseif ($featureSet -eq "rp2040-tpip" -or $featureSet -eq "rp2040-iptp" -or $featureSet -eq "rp2350-tpip" -or $featureSet -eq "rp2350-iptp") {
  $withIP = $true;
  $withTP = $true;
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

  # copy OTA image
  if ($withIP) {    
    $CopyItem2_Source = ".pio/build/$pioEnv/firmware.$OTAbinaryFormat"
    $CopyItem2_Target = Join-Path $CopyItem_Target_Dir "$firmwareName.$OTAbinaryFormat"
    Copy-Item $CopyItem2_Source $CopyItem2_Target
    if (!$?) {
      Write-Host "ERROR: Firmware could noch be copied!"
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
  if ($processor -eq "RP2040") {
    $espotaArgs = "'-p 2040'"
  }

  # Write the script file content to the file 
  $scriptContent = "../../data/OTA-Upload-Firmware-Generic.ps1 $firmwareName.$OTAbinaryFormat $espotaArgs"
  if (Test-Path $fileName) { Clear-Content -Path $fileName }
  Add-Content -Path $fileName -Value $scriptContent
  if (!$?) {
    Write-Host "ERROR: $fileName could not be created!"
    exit 1
  }
} 
# KNX-Upload
if ($withTP) {
  # create KNX-Upload-Firmware-<firmwarename>.ps1 script
  $fileName = "$CopyItem_Target_Dir/KNX-Upload-Firmware.ps1"

  # Write the script file content to the file 
  $scriptContent = "../../data/KNX-Upload-Firmware-Generic.ps1 $CopyItem_Target_Name"
  if (Test-Path $fileName) { Clear-Content -Path $fileName }
  Add-Content -Path $fileName -Value $scriptContent
  if (!$?) {
    Write-Host "ERROR: $fileName could not be created!"
    exit 1
  }
}

# USB-Upload
# create Upload-Firmware-<firmwarename>.ps1 script
$fileName = "$CopyItem_Target_Dir/USB-Upload-Firmware.ps1"

# Write the script file content to the file 
$scriptContent = "../../data/Upload-Firmware-Generic-$processor.ps1 $CopyItem_Target_Name"
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
