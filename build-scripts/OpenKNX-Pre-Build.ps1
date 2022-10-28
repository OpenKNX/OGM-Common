# copy global config files knxprod.h and hardware.h to search path before we compile the projekt

# get all definitions for this project
$settings = scripts/OpenKNX-Build-Settings.ps1
# $knxprod = $args[0]
# $hardware = $args[1]

Write-Host "OpenKNX pre-build-steps:"
$headerExists = Test-Path -PathType Leaf $settings.knxprod
if (!$headerExists) {
    Write-Host -ForegroundColor Red "Missing $($settings.knxprod), ensure to create according knxprod-headerfile with 'Run Test Task'."
    exit 1
}
$hardwareExists = Test-Path -PathType Leaf $settings.hardware
if (!$hardwareExists) {
    Write-Host -ForegroundColor Red "Missing $($settings.hardware), you cannot continue without according hardware definition."
    exit 1
}

Write-Host "  - Copying '$($settings.knxprod)' into search path 'lib/OGM-Common/include/knxprod.h'"
Copy-Item $settings.knxprod lib/OGM-Common/include/knxprod.h 
if (!$?) { exit 1 }
Write-Host "  - Copying '$($settings.hardware)' into search path 'lib/OGM-Common/include/hardware.h'"
Copy-Item $settings.hardware lib/OGM-Common/include/hardware.h 
if (!$?) { exit 1 }
