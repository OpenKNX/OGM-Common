<#
Open ■
┬────┴  OTA-Upload-Firmware-Generic.ps1
■ KNX   2025 OpenKNX - Ing-Dom

This script is designed to perform a console based OTA-Update of IP-capable OpenKNX-Devices
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

OpenKNX_ShowLogo -AddCustomText "OTA Firmware Update"

#$checkVersion = "0.2.1"
$toolsExist = Test-Path -PathType Leaf ~/bin/espota.exe
if ($toolsExist) {
    #$versionLine = ~/bin/espota.exe version | findstr /R /C:"Version Client: *\d*.\d*.\d*"
    #$splitted = $versionLine.split(' ')
    #$toolsExist = [System.Version]$splitted[$splitted.length-1] -ge [System.Version]$checkVersion
}
if (!$toolsExist) {
    Write-Host "
        Fuer das OTA-Update fehlt das notwendige espota.exe oder es ist veraltet..
        Bitte das neuste Paket herunterladen

        https://github.com/OpenKNX/OpenKNXproducer/releases
        
        entpacken und das Readme befolgen. Weitere Informationen hierzu gibt es im OpenKNX-Wiki

            https://github.com/OpenKNX/OpenKNX/wiki/Installation-of-OpenKNX-tools

        Danach bitte dieses Script erneut starten.

        ACHTUNG: Heutige Browser warnen vor dem Inhalt des OpenKNX-Tools Pakets, 
                 weil es ausfuehrbare Programme und ein PowerShell-Skript enthaellt.
    "
    timeout /T -1
}

if ($toolsExist) {
    $firmwareName = (Resolve-Path "./data/$($args[0])").Path

    Write-Host "Hinweis: auf dem Gerät muss OTA ggf. durch aktivieren des Programmiermodus erlaubt werden"

    $validIpAddress = $false
    while (-not $validIpAddress) {
        $ipAddress = Read-Host "IP-Addresse des Update-Ziels eingeben"
        $validIpAddress = [System.Net.IPAddress]::TryParse($ipAddress, [ref]$null)
        if (-not $validIpAddress) {
            Write-Host "Ungueltige IP-Addresse. Bitte erneut eingeben."
        }
    }
    #Write-Host "~/bin/espota.exe -i $ipAddress -r $args[1] -f $firmwareName"
    ~/bin/espota.exe -i $ipAddress -r $args[1] -f $firmwareName
	
    timeout /T -1
}
