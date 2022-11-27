# copy global config files knxprod.h and hardware.h to search path before we compile the projekt
$env = $args[0]
$target = $args[1]


if (Test-Path -Path scripts/OpenKNX-Pre-Build.ps1) {
    scripts/OpenKNX-Pre-Build.ps1
} else {
    lib/OGM-Common/scripts/build/OpenKNX-Pre-Build.ps1
}
if (!$?) { exit 1 }

if ($target) {
    ~/.platformio/penv/Scripts/pio.exe run -e $env --target $target  
} else {
    ~/.platformio/penv/Scripts/pio.exe run -e $env
}

if (!$?) { exit 1 }

if (Test-Path -Path scripts/OpenKNX-Post-Build.ps1) {
    scripts/OpenKNX-Post-Build.ps1
}
