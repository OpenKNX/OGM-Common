# This script is responsible for all common tasks before a release build is executed

# get all definitions for this project
$settings = scripts/OpenKNX-Build-Settings.ps1 $args[0] $args[1] $args[2]

# Output current setting
Write-Host "Provided OpenKNX-Build-Settings:"
$settings
Write-Host "--------------------------------"

# check and cleanup working dir
if (Test-Path -Path release) {
  # clean working dir
  Remove-Item -Recurse release\*
}
else {
  New-Item -Path release -ItemType Directory | Out-Null
}

# create required directories
Copy-Item -Recurse lib/OGM-Common/scripts/setup/reusable/data release

# check for existance of OpenKNXProducer
$OpenKNXproducer = "~/bin/OpenKNXproducer.exe"

if ($IsMacOS -or $IsLinux) {
  $OpenKNXproducer = "/usr/local/bin/OpenKNXproducer"
}

if (Test-Path $OpenKNXproducer -PathType Leaf) {
  Write-Host "OpenKNXproducer found at $OpenKNXproducer"
}
else {
  $OpenKNXproducer = $null
  Write-Host "OpenKNXproducer not found at $OpenKNXproducer"
  Write-Host -ForegroundColor Yellow "
  OpenKNX-Tools are not Installed. Please install OpenKNX-Tools to Build KNX production file. 
  For more Informations visit: https://github.com/OpenKNX/OpenKNX/wiki/Installation-of-OpenKNX-toolscl
  
  "
  Start-Sleep -Seconds 2
}

if (-not ([string]::IsNullOrEmpty($OpenKNXproducer))) {
  Invoke-Expression "$OpenKNXproducer create --Debug --Output=""release/$($settings.targetName).knxprod"" --HeaderFileName=""include/knxprod.h"" ""src/$($settings.releaseName).xml"""
  Write-Host "Created release/$($settings.targetName).knxprod" -ForegroundColor Blue
  if (!$?) {
    Write-Host "Error in knxprod, Release was not built!" -ForegroundColor Red
    $prompt = Read-Host "Press 'y' to continue, any other key to cancel"
    if ($prompt -ne "y") {
      exit 1
    }
  }
}
else {
  Write-Host "OpenKNXproducer is not Installed. Skipping knxprod file creation." -ForegroundColor Yellow
}

if (Test-Path -Path "src/$($settings.releaseName).debug.xml") {
  Move-Item "src/$($settings.releaseName).debug.xml" "release/data/$($settings.targetName).xml"
}
if (Test-Path -Path "src/$($settings.releaseName).baggages") {
  Move-Item "src/$($settings.releaseName).baggages" "release/data/$($settings.targetName).baggages"
}

# write content.xml header
$releaseTarget = Join-Path $ProjectDir "release/data/content.xml"
# Create the directory structure if it doesn't exist
$directory = [System.IO.Path]::GetDirectoryName($releaseTarget)
if (-not (Test-Path -Path $directory)) {
  New-Item -ItemType Directory -Path $directory -Force
}
if (-not (Test-Path -Path $releaseTarget -PathType Leaf)) {
  New-Item -Path $releaseTarget -ItemType File | Out-Null
} 

if (Test-Path -Path $releaseTarget -PathType Leaf) {
  # write content.xml header
  Add-Content -Path $releaseTarget "<?xml version=""1.0"" encoding=""UTF-8""?>"
  Add-Content -Path $releaseTarget "<Content>"
  Add-Content -Path $releaseTarget "    <ETSapp Name=""$($settings.targetName)"" XmlFile=""$($settings.targetName).xml"" />"
  Add-Content -Path $releaseTarget "    <Products>"
  Write-Host "Created $releaseTarget" -ForegroundColor Blue 
}
else {
  Write-Host "Pre - ERROR: $releaseTarget could not be found!" -ForegroundColor Red
  exit 1
}
exit 0
