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

if ($IsMacOS -or $IsLinux) { Write-Host -ForegroundColor Yellow "
  This OS does not support OpenKNX-Tools. Please use Windows to Build KNX production file. 
  For more Informations visit: https://github.com/OpenKNX/OpenKNX/wiki/Installation-of-OpenKNX-toolscl
  
  " 
  Start-Sleep -Seconds 2 
} 
else {
  # get xml for kxnprod, always first step which also generates headerfile for release
  ~/bin/OpenKNXproducer.exe create --Debug --Output="release/$($settings.targetName).knxprod" --HeaderFileName="include/knxprod.h" "src/$($settings.releaseName).xml"
  if (!$?) {
      Write-Host "Error in knxprod, Release was not built!"
      $prompt = Read-Host "Press 'y' to continue, any other key to cancel"
      if ($prompt -ne "y") {
          exit 1
      }
  }
}
if (Test-Path -Path "src/$($settings.releaseName).debug.xml") {
  Move-Item "src/$($settings.releaseName).debug.xml" "release/data/$($settings.targetName).debug.xml"
}
if (Test-Path -Path "src/$($settings.releaseName).baggages") {
    Move-Item "src/$($settings.releaseName).baggages" "release/data/$($settings.targetName).baggages"
}

# write content.xml header
$releaseTarget = "release/data/content.xml"
"<?xml version=""1.0"" encoding=""UTF-8""?>" >$releaseTarget
"<Content>" >>$releaseTarget
"    <ETSapp Name=""$($settings.targetName)"" XmlFile=""$($settings.targetName).xml"" />" >>$releaseTarget
"    <Products>" >>$releaseTarget
exit 0
