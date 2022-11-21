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
} else {
    New-Item -Path release -ItemType Directory | Out-Null
}

# create required directories
Copy-Item -Recurse lib/OGM-Common/setup-scripts/reusable/data release

# get xml for kxnprod, always first step which also generates headerfile for release
~/bin/OpenKNXproducer.exe create --Debug --Output="release/$($settings.targetName).knxprod" --HeaderFileName="src/$($settings.sourceName).h" "src/$($settings.releaseName).xml"
if (!$?) {
    Write-Host "Error in knxprod, Release was not built!"
    exit 1
}
Move-Item "src/$($settings.releaseName).debug.xml" "release/data/$($settings.targetName).xml"

# copy generated headerfile and according hardware file to according directory
lib/OGM-Common/build-scripts/OpenKNX-Pre-Build.ps1
if (!$?) { exit 1 }

exit 0
