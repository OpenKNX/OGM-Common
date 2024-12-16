$checkVersion = "0.2.1"
$toolsExist = Test-Path -PathType Leaf ~/bin/KnxFileTransferClient.exe
if ($toolsExist) {
    $versionLine = ~/bin/KnxFileTransferClient.exe version | findstr /R /C:"Version Client: *\d*.\d*.\d*"
    $splitted = $versionLine.split(' ')
    $toolsExist = [System.Version]$splitted[$splitted.length-1] -ge [System.Version]$checkVersion
}
if (!$toolsExist) {
    Write-Host "
        Fuer das Update ueber den KNX-Bus fehlt der notwendige KnxFileTransferClient oder er ist veraltet..
        Bitte das neuste Paket herunterladen (mindestens Version $checkVersion)

            https://github.com/OpenKNX/KnxFileTransferClient/releases
        
        entpacken und die Datei Install-KnxFileTransferClient.ps1 mit der Powershell ausführen.

        Danach bitte dieses Script erneut starten.

        ACHTUNG: Heutige Browser warnen vor dem Inhalt des OpenKNX-Tools Pakets, 
                 weil es ausfuehrbare Programme und ein PowerShell-Skript enthaellt.
                 Falls jemand dem Paketinhalt nicht traut, kann er sich KnxFileTransferClient
                 selber kompilieren und entsprechend installieren.
    "
    timeout /T -1
}

if ($toolsExist) {
    $firmwareName = (Resolve-Path "./data/$($args[0])").Path
    if (Test-Path -PathType Leaf ~/bin/KnxFileTransferClient.ps1) {
        ~/bin/KnxFileTransferClient.ps1 "$firmwareName"
    } else {
        Write-Output $fileName
        ~/bin/KnxFileTransferClient.exe fwupdate --connect Search "$firmwareName"
    }
    timeout /T -1
}
