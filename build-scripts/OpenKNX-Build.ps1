# copy global config files knxprod.h and hardware.h to search path before we compile the projekt
$knxprod = $args[0]
$hardware = $args[1]
$env = $args[2]
$target = $args[3]

Write-Host "OpenKNX pre-build-steps:"
if (!$knxprod) {
    $knxprod = "knxprod.h"
}
$headerExists = Test-Path -PathType Leaf src/$knxprod
if (!$headerExists) {
    Write-Host -ForegroundColor Red "Missing $knxprod, ensure to create according knxprod-headerfile with 'Run Test Task'."
    exit 1
}
$hardwareExists = Test-Path -PathType Leaf src/$hardware
if (!$hardwareExists) {
    Write-Host -ForegroundColor Red "Missing $hardware, you cannot continue without according hardware definition."
    exit 1
}

Write-Host "  - Copying '$knxprod' into search path 'lib/OGM-Common/include/knxprod.h'"
Copy-Item src/$knxprod lib/OGM-Common/include/knxprod.h 
Write-Host "  - Copying '$hardware' into search path 'lib/OGM-Common/include/hardware.h'"
Copy-Item src/$hardware lib/OGM-Common/include/hardware.h 
if ($target) {
    ~/.platformio/penv/Scripts/pio.exe run -e $env --target $target  
} else {
    ~/.platformio/penv/Scripts/pio.exe run -e $env
}
