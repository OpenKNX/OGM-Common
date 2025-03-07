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

OpenKNX_ShowLogo -AddCustomText "USB Firmware Upload (ESP32)"


$firmwareName = $args[0]


$currentDir = (Get-Item .).FullName
$firmwareWithPath = Join-Path $currentDir "data"
$firmwareWithPath = Join-Path $firmwareWithPath $firmwareName




$toolsExist = Test-Path -PathType Leaf ~/bin/esptool.exe
if ($toolsExist) {
    $helpText = ~/bin/esptool.exe version
    #$toolsExist = $helpText -match 'Version 1.7.0'

    #python -m esptool write_flash 0x0 $firmwareWithPath
    Write-Host "Now executing ~/bin/esptool.exe write_flash 0x0 $firmwareWithPath ..."
    Write-Host
    ~/bin/esptool.exe write_flash 0x0 $firmwareWithPath

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