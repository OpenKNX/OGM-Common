# This script builds on release variant
# call it with
#    scripts/Build-Step.ps1 <pio-environment> <firmware-name> <binary-format>
# 
# <pio-environment> is the env: entry from platformio.ini, which defines how to build the firmware. 
#                   for [env:RP_2040] use "RP_2040" as <pio-environment>
# <firmware-name>   is the name of the firmware file, which is created. Important, if there are
#                   different firmwares for differen hardware build in one release
#                   There should be always an "Upload-Firmware-<firmwarename>.ps1" script delivered,
#                   which installs this firmware.
# <binary-format>   currently just "uf2" for RP2040 and "bin" for SAMD
#
# This file needs no changes and is project independent 

$pioEnv = $args[0]
$firmwareName = $args[1]
$binaryFormat = $args[2]
$productName = $args[3]

~/.platformio/penv/Scripts/pio.exe run -e $pioEnv
if (!$?) {
    Write-Host "$pioEnv build failed, Release was not built!"
    exit 1
}
Copy-Item ".pio/build/$pioEnv/firmware.$binaryFormat" "release/data/$firmwareName.$binaryFormat"
# create Upload-Firmware-xxx.ps1 file
if (!$productName) {
    $productName = $firmwareName.Replace("firmware-", "")
}
$processor = "RP2040"
if ($binaryFormat -eq "bin") {
    $processor = "SAMD"
}
$fileName = "release/Upload-Firmware-$productName.ps1"
"./data/Upload-Firmware-Generic-$processor.ps1 $firmwareName.$binaryFormat" >$fileName

# add entry to content.xml
"        <Product Name=""$productName"" Firmware=""$firmwareName.$binaryFormat"" Processor=""$processor"" />" >>release/data/content.xml
exit 0
