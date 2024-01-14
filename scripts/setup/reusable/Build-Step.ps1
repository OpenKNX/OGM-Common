# This script builds the release variant of the firmware.
# Call it with the following parameters:
#    scripts/Build-Step.ps1 <pio-environment> <firmware-name> <binary-format> [<product-name>] <project-dir>
# 
# <pio-environment> is the env: entry from platformio.ini, which defines how to build the firmware. 
#                   For [env:RP_2040], use "RP_2040" as the <pio-environment>.
# <firmware-name>   is the name of the firmware file that will be created. It is important to have
#                   different firmware names for different hardware builds within the same release.
#                   There should always be an "Upload-Firmware-<firmwarename>.ps1" script delivered,
#                   which installs this firmware.
# <binary-format>   currently supports "uf2" for RP2040 and "bin" for SAMD.
# <product-name>    (optional) is the name of the product. If not provided, the firmware name without
#                   the "firmware-" prefix will be used.
# <project-dir>     is the directory path of the project.
#
# This file does not require any changes and is project-independent.

param (
  [Parameter(Mandatory=$true)]
  [ValidateNotNullOrEmpty()]
  [string]$pioEnv,

  [Parameter(Mandatory=$true)]
  [ValidateNotNullOrEmpty()]
  [string]$firmwareName,

  [Parameter(Mandatory=$true)]
  [ValidateNotNullOrEmpty()]
  [string]$binaryFormat,
  
  [Parameter(Mandatory=$false)]
  [string]$productName,

  [Parameter(Mandatory=$true)]
  [string]$ProjectDir
)


if ($IsMacOS -or $IsLinux) { ~/.platformio/penv/bin/pio run -e $pioEnv }
else { ~/.platformio/penv/Scripts/pio.exe run -e $pioEnv }
if (!$?) {
    Write-Host "$pioEnv build failed, Release was not built!"
    exit 1
}

# Create source and target path for firmware
$CopyItem_Source= Join-Path $ProjectDir ".pio/build/$pioEnv/firmware.$binaryFormat"
$CopyItem_Target_Dir= Join-Path $ProjectDir "release/data"
$CopyItem_Target= Join-Path $CopyItem_Target_Dir "$firmwareName.$binaryFormat"

# Check if firmware is available and copy it to release/data
Write-Host "The $PioEnv firmware is available as $CopyItem_Source"
if( Test-Path $CopyItem_Source ) {
  Write-Host "Copy-Item: $CopyItem_Source to $CopyItem_Target"
  # create target directory if not exists
  if (!(Test-Path -Path $CopyItem_Target_Dir)) {
    New-Item -ItemType Directory -Force -Path $CopyItem_Target_Dir
  }
  # copy firmware to release/data
  Copy-Item $CopyItem_Source $CopyItem_Target
} else {
  # firmware not found
  Write-Host "ERROR: $CopyItem_Source not found!"
  exit 1
}

# if no product name is given, use firmware name without "firmware-" prefix
if (!$productName) {
    $productName = $firmwareName.Replace("firmware-", "")
}
# create Upload-Firmware-<firmwarename>.ps1 script
$processor = "RP2040"
if ($binaryFormat -eq "bin") {
    $processor = "SAMD"
}

# create Upload-Firmware-<firmwarename>.ps1 script
$fileName = Join-Path $ProjectDir "release/Upload-Firmware-$productName.ps1"
# Write the script file content to the file 
$scriptContent = "./data/Upload-Firmware-Generic-$processor.ps1 $firmwareName.$binaryFormat"
if (Test-Path $fileName) { Clear-Content -Path $fileName }
Add-Content -Path $fileName -Value $scriptContent
if (!$?) {
    Write-Host "ERROR: $fileName could not be created!"
    exit 1
}

$releaseTarget = Join-Path $ProjectDir "release/data/content.xml"
#check if file exists content.xml exists if not create it
if ((Test-Path -Path $releaseTarget -PathType Leaf)) {
  # Add entry to content.xml. If entry already exists, do nothing. If not, add it. If file does not exist, create it.
  $XMLContent ="         <Product Name=""$productName"" Firmware=""$firmwareName.$binaryFormat"" Processor=""$processor"" />"
  $lineExists = Select-String -Path $fileName -Pattern $XMLContent -Quiet
  if (-not $lineExists) { Add-Content -Path $releaseTarget -Value $XMLContent }
} else {
  Write-Host "ERROR - Buildstep: $releaseTarget could not be found!" -ForegroundColor Red
  exit 1
}

exit 0
