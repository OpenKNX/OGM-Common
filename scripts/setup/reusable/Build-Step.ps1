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
  [Parameter(Mandatory = $true)]
  [ValidateNotNullOrEmpty()]
  [string]$pioEnv,

  [Parameter(Mandatory = $true)]
  [ValidateNotNullOrEmpty()]
  [string]$firmwareName,

  [Parameter(Mandatory = $true)]
  [ValidateNotNullOrEmpty()]
  [string]$binaryFormat,
  
  [Parameter(Mandatory = $false)]
  [string]$productName,

  [Parameter(Mandatory = $false)]
  [string]$ProjectDir
)

if ($IsMacOS -or $IsLinux) { ~/.platformio/penv/bin/pio run -e $pioEnv }
else { ~/.platformio/penv/Scripts/pio.exe run -e $pioEnv }
if (!$?) {
  Write-Host "$pioEnv build failed, Release was not built!"
  exit 1
}

# binaryFormat uf2 means rp2040 without OTA
$processor = "RP2040"
$withOTA = $false;
if ($binaryFormat -eq "bin") {
  $processor = "SAMD"
}
elseif ($binaryFormat -eq "esp32") {
  $binaryFormat = "bin"
  $processor = "ESP32"
  $withOTA = $true;
}
elseif ($binaryFormat -eq "rp2040") {
  $binaryFormat = "uf2"
  $withOTA = $true;
}

# if no product name is given, use firmware name without "firmware-" prefix
if (!$productName) {
  $productName = $firmwareName.Replace("firmware-", "")
}

# Create source and target path for firmware
$CopyItem_Source = ".pio/build/$pioEnv/firmware.$binaryFormat"
$CopyItem_Target_Data = "release/data"
$CopyItem_Target_Device = "Device-$productName"
$CopyItem_Target_Dir = "release/$CopyItem_Target_Device"
$CopyItem_Target_Name = "$firmwareName.$binaryFormat"
if (![string]::IsNullOrEmpty($ProjectDir)) {
  $CopyItem_Source = Join-Path $ProjectDir $CopyItem_Source
  $CopyItem_Target_Data = Join-Path $ProjectDir $CopyItem_Target_Data
  $CopyItem_Target_Dir = Join-Path $ProjectDir $CopyItem_Target_Dir
}
$CopyItem_Target = Join-Path $CopyItem_Target_Dir "$CopyItem_Target_Name"

# Check if firmware is available and copy it to release
Write-Host "The $PioEnv firmware is available as $CopyItem_Source"
if ( Test-Path $CopyItem_Source ) {
  Write-Host "Copy-Item: $CopyItem_Source to $CopyItem_Target"
  # create target directories if not exists
  if (!(Test-Path -Path $CopyItem_Target_Data)) {
    New-Item -ItemType Directory -Force -Path $CopyItem_Target_Data
  }
  if (!(Test-Path -Path $CopyItem_Target_Dir)) {
    New-Item -ItemType Directory -Force -Path $CopyItem_Target_Dir
  }
  
  # copy firmware to release
  Copy-Item $CopyItem_Source $CopyItem_Target 
  Write-Host "DEBUG: nach Copy-Item $CopyItem_Target"

  # copy OTA image
  if ($withOTA) {    
    if ($processor -eq "ESP32") {
      $CopyItem2_Source = ".pio/build/$pioEnv/firmware.factory.$binaryFormat"
      $CopyItem2_Target_Dir = Join-Path $CopyItem_Target_Dir "$firmwareName.factory.$binaryFormat"
      Copy-Item $CopyItem2_Source $CopyItem2_Target_Dir
    }
    elseif ($processor -eq "RP2040") {
      $CopyItem2_Source = ".pio/build/$pioEnv/firmware.bin"
      $CopyItem2_Target_Dir = Join-Path $CopyItem_Target_Dir "$firmwareName.bin"
      Copy-Item $CopyItem2_Source $CopyItem2_Target_Dir
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
if ($processor -eq "RP2040") {
  # create KNX-Upload-Firmware-<firmwarename>.ps1 script
  $fileName = "$CopyItem_Target_Dir/KNX-Upload-Firmware.ps1"

  # Write the script file content to the file 
  $scriptContent = "../data/KNX-Upload-Firmware-Generic.ps1 $CopyItem_Target_Name"
  if (Test-Path $fileName) { Clear-Content -Path $fileName }
  Add-Content -Path $fileName -Value $scriptContent
  if (!$?) {
    Write-Host "ERROR: $fileName could not be created!"
    exit 1
  }
}

# OTA
if ($withOTA) {
  # create OTA-Upload-Firmware-<firmwarename>.ps1 script
  $fileName = "$CopyItem_Target_Dir/OTA-Upload-Firmware.ps1"
  $OTAbinaryFormat = "bin"
  if ($processor -eq "RP2040") {
    $espotaArgs = "'-p 2040'"
  }

  # Write the script file content to the file 
  $scriptContent = "../data/OTA-Upload-Firmware-Generic.ps1 $firmwareName.$OTAbinaryFormat $espotaArgs"
  if (Test-Path $fileName) { Clear-Content -Path $fileName }
  Add-Content -Path $fileName -Value $scriptContent
  if (!$?) {
    Write-Host "ERROR: $fileName could not be created!"
    exit 1
  }
}

# create Upload-Firmware-<firmwarename>.ps1 script
$fileName = "$CopyItem_Target_Dir/USB-Upload-Firmware.ps1"

# Write the script file content to the file 
$scriptContent = "../data/Upload-Firmware-Generic-$processor.ps1 $CopyItem_Target_Name"
if ( $processor -eq "ESP32") {
  $scriptContent = "../data/Upload-Firmware-Generic-$processor.ps1 $firmwareName.factory.$binaryFormat"
}
if (Test-Path $fileName) { Clear-Content -Path $fileName }
Add-Content -Path $fileName -Value $scriptContent
if (!$?) {
  Write-Host "ERROR: $fileName could not be created!"
  exit 1
}

#check if file exists content.xml exists then add the closing tags
$releaseTarget = "$CopyItem_Target_Data/content.xml"

#check if file exists content.xml exists if not create it
if ((Test-Path -Path $releaseTarget -PathType Leaf)) {
  # Add entry to content.xml. If entry already exists, do nothing. If not, add it. If file does not exist, create it.
  $XMLContent = "         <Product Name=""$productName"" Firmware=""../$productName/$CopyItem_Target_Name"" Processor=""$processor"" />"
  $lineExists = Select-String -Path $fileName -Pattern $XMLContent -Quiet
  if (-not $lineExists) { Add-Content -Path $releaseTarget -Value $XMLContent }
}
else {
  Write-Host "ERROR - Buildstep: $releaseTarget could not be found!" -ForegroundColor Red
  exit 1
}

exit 0
