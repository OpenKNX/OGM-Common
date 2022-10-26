# This script is responsible for all common tasks before a release build is executed

# set product names, allows mapping of (devel) name in Project to a more consistent name in release
$sourceName=$args[0]
$targetName=$args[1]

# Release indication, set according names for Release or Beta
$releaseIndication = $args[2]
if ($releaseIndication) {
    $releaseName="$sourceName-$releaseIndication"
} else {
    $releaseName="$sourceName"
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
~/bin/OpenKNXproducer.exe create --Debug --Output="release/$targetName.knxprod" --HeaderFileName="src/$sourceName.h" "src/$releaseName.xml"
if (!$?) {
    Write-Host "Error in knxprod, Release was not built!"
    exit 1
}
Move-Item "src/$releaseName.debug.xml" "release/data/$targetName.xml"

# copy generated headerfile and according hardware file to according directory
$hardwareName = $sourceName + "Hardware"
Write-Host "OpenKNX pre-build-steps:"
Write-Host "  - Copying '$sourceName' into search path 'lib/OGM-Common/include/knxprod.h'"
Copy-Item src/$sourceName.h lib/OGM-Common/include/knxprod.h
if (!$?) { exit 1 }
Write-Host "  - Copying '$hardwareName' into search path 'lib/OGM-Common/include/hardware.h'"
Copy-Item src/$hardwareName.h lib/OGM-Common/include/hardware.h
if (!$?) { exit 1 }

exit 0
