#$checkVersion = "0.2.1"
$toolsExist = Test-Path -PathType Leaf ~/bin/esptools/espota.exe
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

    $validIpAddress = $false
    while (-not $validIpAddress) {
        $ipAddress = Read-Host "IP-Addresse des Gerätes eingeben dass aktualisiert werden soll:"
        $validIpAddress = [System.Net.IPAddress]::TryParse($ipAddress, [ref]$null)
        if (-not $validIpAddress) {
            Write-Host "Ungültige IP-Addresse. Bitte erneut eingeben."
        }
    }
    Write-Host "~/bin/esptools/espota.exe -i $ipAddress -r -f $firmwareName"
    ~/bin/esptools/espota.exe -i $ipAddress -r -f $firmwareName
	
    timeout /T -1
}
