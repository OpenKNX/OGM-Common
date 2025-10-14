# This script is responsible for all common tasks after a release build is executed


# get all definitions for this project
$settings = scripts/OpenKNX-Build-Settings.ps1 $args[0]


#check if file exists content.xml exists then add the closing tags
$releaseTarget = "release/data/content.xml"

if (Test-Path -Path $releaseTarget -PathType Leaf) {
  Add-Content -Path $releaseTarget -Value "    </Products>"
  Add-Content -Path $releaseTarget -Value "</Content>"
}
else {
  Write-Host "ERROR: $releaseTarget could not be found!"
}

# add necessary scripts, but allow project local versions
if (Test-Path -Path scripts/Readme-Release.txt -PathType Leaf) {
  Copy-Item scripts/Readme-Release.txt release/
}
else {
  Copy-Item lib/OGM-Common/scripts/setup/reusable/Readme-Release.txt release/
}
# Create ETS-Applikation directory and copy Build-knxprod.ps1
if (!(Test-Path -Path release/ETS-Applikation -PathType Container)) {
  New-Item -ItemType Directory -Force -Path release/ETS-Applikation | Out-Null
}
if (Test-Path -Path scripts/Build-knxprod.ps1 -PathType Leaf) {
  Copy-Item scripts/Build-knxprod.ps1 release/ETS-Applikation/
}
else {
  Copy-Item lib/OGM-Common/scripts/setup/reusable/Build-knxprod.ps1 release/ETS-Applikation/
}
# Copy-Item scripts/Upload-Firmware*.ps1 release/

# here we might need a better switch in future
# if ($($settings.releaseIndication) -eq "Big") 
# {
#     Remove-Item release/Upload-Firmware-*SAMD*.ps1
# }

# add optional files
if (Test-Path -Path scripts/Readme-Hardware.html -PathType Leaf) {
  Copy-Item scripts/Readme-Hardware.html release/
}

# cleanup
if (Test-Path -Path "release/$($settings.targetName).knxprod" -PathType Leaf) {
  Remove-Item "release/$($settings.targetName).knxprod"
}

# calculate version string
$appVersion = Select-String -Path "include/knxprod.h" -Pattern "#define MAIN_ApplicationVersion"
$appVersion = $appVersion.ToString().Split()[-1]
$appMajor = [math]::Floor($appVersion / 16)
$appMinor = $appVersion % 16

$appRev = Select-String -Path "include/knxprod.h" -Pattern "#define MAIN_FirmwareRevision"
if ($appRev) {
  $appRev = $appRev.ToString().Split()[-1]
}
else {
  $appRev = 0
  if (Test-Path -Path "src/main.cpp" -PathType Leaf) {
    $appRev = Select-String -Path src/main.cpp -Pattern "const uint8_t firmwareRevision"
    $appRev = $appRev.ToString().Split()[-1].Replace(";", "")
  }
}

$appVersion = "$appMajor.$appMinor"
$appVersion = "$appVersion.$appRev"


# create dependency file
if (Test-Path -Path dependencies.txt -PathType Leaf) {
  Remove-Item dependencies.txt
}
lib/OGM-Common/scripts/setup/reusable/Build-Dependencies.ps1
Get-Content dependencies.txt

# (re-)create restore directory
lib/OGM-Common/scripts/setup/reusable/Build-Project-Restore.ps1

# create package
$releaseTemp = "Release.zip"
# if Release.zip exist, remove it
if (Test-Path -Path $releaseTemp) {
  Remove-Item $releaseTemp
}
# create Release.zip
Compress-Archive -Path release/* -DestinationPath $releaseTemp -Verbose
#Check if Release.zip is created
if (Test-Path -Path $releaseTemp -PathType Leaf ) {
  # remove all files and directories in release directory
  Remove-Item -Recurse release/*
  # move Release.zip to release directory
  Move-Item $releaseTemp "release/$($settings.targetName)-$($settings.appRelease)-$appVersion.zip"
  Write-Host "Release $($settings.targetName)-$($settings.appRelease)-$appVersion successfully created!" -ForegroundColor Green
}
else {
  Write-Host "ERROR: $($settings.targetName)-$($settings.appRelease)-$appVersion.zip could not be created!"
}
