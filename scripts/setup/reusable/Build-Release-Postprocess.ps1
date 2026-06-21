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
# The ETS-Applikation/Build-knxprod.ps1 WRAPPER is generated further below, once the
# application version is known (it gets baked into the wrapper). The real build engine
# ships as release/data/Build-knxprod-Generic.ps1 (copied from reusable/data above).
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
  #Remove-Item "release/$($settings.targetName).knxprod"
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


# --- Generate the thin ETS-Applikation/Build-knxprod.ps1 wrapper --------------------
# (this file has no BOM -> keep it pure ASCII so Windows PowerShell 5.1 reads it right)
# It just calls the real engine in ../data with the XML name + app version baked in.
# Forward slashes in the path work on Windows PowerShell 5.1 and on macOS/Linux.
if (!(Test-Path -Path release/ETS-Applikation -PathType Container)) {
  New-Item -ItemType Directory -Force -Path release/ETS-Applikation | Out-Null
}
if (Test-Path -Path scripts/Build-knxprod.ps1 -PathType Leaf) {
  # project-local full builder override (legacy): keep copying it verbatim
  Copy-Item scripts/Build-knxprod.ps1 release/ETS-Applikation/
  Write-Host "Copied project-local scripts/Build-knxprod.ps1 to release/ETS-Applikation/" -ForegroundColor Blue
}
else {
  $wrapperXml = "$($settings.targetName).xml"
  $author     = 'Erkan ' + [char]0x00C7 + 'olak'   # 0x00C7 = C-cedilla; built here so the source stays ASCII
  $wrapper = @"
#!/usr/bin/env pwsh
<#
  Build-knxprod (wrapper) - OpenKNX - $author
  Auto-generated at release build time. Do NOT edit by hand.
  Calls the real engine ../data/Build-knxprod-Generic.ps1 with the XML name
  and application version baked in.
#>
param([string]`$Lang = "", [switch]`$Detailed, [Alias("h")][switch]`$Help)
& "`$PSScriptRoot/../data/Build-knxprod-Generic.ps1" -Xml '$wrapperXml' -AppVersion '$appVersion' -OutDir `$PSScriptRoot -Lang `$Lang -Detailed:`$Detailed -Help:`$Help
exit `$LASTEXITCODE
"@
  $etsDir      = (Resolve-Path 'release/ETS-Applikation').Path
  $wrapperPath = Join-Path $etsDir 'Build-knxprod.ps1'
  $encBom      = New-Object System.Text.UTF8Encoding $true      # Windows PS 5.1 needs the BOM
  [System.IO.File]::WriteAllText($wrapperPath, $wrapper, $encBom)
  Write-Host "Generated release/ETS-Applikation/Build-knxprod.ps1 (-> data/Build-knxprod-Generic.ps1, XML $wrapperXml, app $appVersion)" -ForegroundColor Blue
}


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
  # Remove-Item -Recurse -Force release/*
  # move Release.zip to release directory
  Move-Item $releaseTemp "release/$($settings.targetName)-$($settings.appRelease)-$appVersion.zip"
  Write-Host "Release $($settings.targetName)-$($settings.appRelease)-$appVersion successfully created!" -ForegroundColor Green
}
else {
  Write-Host "ERROR: $($settings.targetName)-$($settings.appRelease)-$appVersion.zip could not be created!"
}
