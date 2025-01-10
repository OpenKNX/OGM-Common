
$firmwareName = $args[0]


$currentDir = (Get-Item .).FullName
$firmwareWithPath = Join-Path $currentDir "data"
$firmwareWithPath = Join-Path $firmwareWithPath $firmwareName




$toolsExist = Test-Path -PathType Leaf ~/bin/esptools/esptool.exe
if ($toolsExist) {
    $helpText = ~/bin/esptools/esptool.exe version
    #$toolsExist = $helpText -match 'Version 1.7.0'

    #python -m esptool write_flash 0x0 $firmwareWithPath
    Write-Host "Now executing ~/bin/esptools/esptool.exe write_flash 0x0 $firmwareWithPath ..."
    Write-Host
    ~/bin/esptools/esptool.exe write_flash 0x0 $firmwareWithPath

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