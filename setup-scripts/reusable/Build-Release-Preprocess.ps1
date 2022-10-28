# This script is responsible for all common tasks before a release build is executed

# get all definitions for this project
$settings = scripts/OpenKNX-Build-Settings.ps1
# $knxprod=$args[0]
# $hardware=$args[1]
# $sourceName=$args[2]
# $targetName=$args[3]

# Release indication, set according names for Release or Beta
$releaseIndication = $args[0]
if ($releaseIndication) {
    $releaseName="$($settings.sourceName)-$releaseIndication"
} else {
    $releaseName="$($settings.sourceName)"
}

# check and cleanup working dir
if (Test-Path -Path release) {
    # clean working dir
    Remove-Item -Recurse release\*
} else {
    New-Item -Path release -ItemType Directory | Out-Null
}

# create required directories
Copy-Item -Recurse ../OGM-Common/setup-scripts/reusable/data release

# get xml for kxnprod, always first step which also generates headerfile for release
~/bin/OpenKNXproducer.exe create --Debug --Output="release/$($settings.targetName).knxprod" --HeaderFileName="src/$($settings.sourceName).h" "src/$releaseName.xml"
if (!$?) {
    Write-Host "Error in knxprod, Release was not built!"
    exit 1
}
Move-Item "src/$releaseName.debug.xml" "release/data/$($settings.targetName).xml"

# copy generated headerfile and according hardware file to according directory
lib/OGM-Common/build-scripts/OpenKNX-Pre-Build.ps1 $releaseIndication
if (!$?) { exit 1 }

exit 0
