# copy global config files knxprod.h and hardware.h to search path before we compile the projekt
$env = $args[0]
$target = $args[1]

lib/OGM-Common/build-scripts/OpenKNX-Pre-Build.ps1
if (!$?) { exit 1 }

if ($target) {
    ~/.platformio/penv/Scripts/pio.exe run -e $env --target $target  
} else {
    ~/.platformio/penv/Scripts/pio.exe run -e $env
}
