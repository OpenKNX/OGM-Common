<#
Open ■
┬────┴  Upload-Firmware-Generic-ESP32.ps1
■ KNX   2025 OpenKNX - Ing-Dom

This script is designed to perform a console based Firmware-Upload over USB to ESP-based OpenKNX-Devices
#>

function OpenKNX_ShowLogo($AddCustomText = $null) {
    Write-Host ""
    Write-Host "Open " -NoNewline
    #Write-Host "■" -ForegroundColor Green
    Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -ForegroundColor Green
    $unicodeString = "$( [char]::ConvertFromUtf32(0x252C) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2534) ) "
  
    if ($AddCustomText) { 
      #Write-Host "┬────┴  $AddCustomText" -ForegroundColor Green
      Write-Host "$($unicodeString) $($AddCustomText)"  -ForegroundColor Green
    }
    else {
      #Write-Host "┬────┴" -ForegroundColor Green
      Write-Host "$($unicodeString)"  -ForegroundColor Green
    }
  
    #Write-Host "■" -NoNewline -ForegroundColor Green
    Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -NoNewline -ForegroundColor Green
    Write-Host " KNX"
    Write-Host ""
  }

function SearchComPortForDevice($name, $vid) {
    Write-Host "Searching COM-Port for $name (VID: $vid)"
    $portList = get-pnpdevice -class Ports
    if ($portList) {
        foreach ($usbDevice in $portList) {
            if ($usbDevice.Present) {
                $isDevice = $usbDevice.InstanceId.StartsWith("USB\VID_$vid")
                $isCom = $usbDevice.Name -match "COM\d{1,3}"
                if ($isDevice -and $isCom) {
                    # $port = $Matches[1]
                    $port = $Matches[0]
                    Write-Host "COM-Port found: $port" -ForegroundColor Green
                    return $port
                }
            }
        }
    }
    return $null
}

OpenKNX_ShowLogo -AddCustomText "USB Firmware Upload (ESP32)"


$firmwareName = $args[0]


$currentDir = (Get-Item .).FullName
$firmwareWithPath = Join-Path $currentDir $firmwareName




$toolsExist = Test-Path -PathType Leaf ~/bin/esptool.exe
if ($toolsExist) {
    $port = SearchComPortForDevice "ESP32" "1A86"
    $helpText = ~/bin/esptool.exe version
    #$toolsExist = $helpText -match 'Version 1.7.0'

    #python -m esptool write_flash 0x0 $firmwareWithPath
    if ($port) {
      Write-Host "Uploading with:"
      Write-Host "~/bin/esptool.exe --port $port --baud 460800 write_flash 0x0 $firmwareWithPath ..."
      Write-Host
      ~/bin/esptool.exe --port "$port" --baud 460800 write_flash 0x0 $firmwareWithPath
    } else {
      Write-Host "Not found, trying alternative (slower) scan with:" -ForegroundColor Yellow
      Write-Host "~/bin/esptool.exe --baud 460800 write_flash 0x0 $firmwareWithPath ..."
      Write-Host
      ~/bin/esptool.exe --baud 460800 write_flash 0x0 $firmwareWithPath
    }

    timeout /T -1
}
else {
    Write-Host "
        Fuer das Setup fehlen die notwendigen OpenKNX-Tools oder sie sind veraltet..
        Bitte das neuste Paket herunterladen.

            https://github.com/OpenKNX/OpenKNXproducer/releases
        
        entpacken und das Readme befolgen. Weitere Informationen hierzu gibt es im OpenKNX-Wiki

            https://github.com/OpenKNX/OpenKNX/wiki/Installation-of-OpenKNX-tools

        Danach bitte dieses Script erneut starten.

        ACHTUNG: Heutige Browser warnen vor dem Inhalt des OpenKNX-Tools Pakets, 
                 weil es ausfuehrbare Programme und ein PowerShell-Skript enthaellt.
    "
    timeout /T -1
}