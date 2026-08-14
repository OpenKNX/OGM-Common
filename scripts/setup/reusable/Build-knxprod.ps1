$checkVersion = "4.3.11"
$toolsExist = Test-Path -PathType Leaf ~/bin/OpenKNXproducer.exe
if ($toolsExist) {
    # Auf vier Komponenten auffuellen: der Producer meldet die Version dreiteilig,
    # [System.Version] setzt Revision dann auf -1 und "4.3.11" gilt gegen "4.3.11.0" als kleiner.
    function Get-PaddedVersion($version) {
        $parts = @($version -split '\.') + @('0', '0', '0', '0')
        return [System.Version]($parts[0..3] -join '.')
    }
    $toolsExist = (Get-PaddedVersion ((~/bin/OpenKNXproducer version) -split ' ')[1]) -ge (Get-PaddedVersion $checkVersion)
}
if ($toolsExist) {
    $toolsExist = Test-Path -PathType Leaf ~/bin/bossac.exe
}
if (!$toolsExist) {
    Write-Host "
        Fuer das Setup fehlen die notwendigen OpenKNX-Tools oder sie sind veraltet..
        Bitte das neuste Paket herunterladen (mindestens Version $checkVersion)

            https://github.com/OpenKNX/OpenKNXproducer/releases
        
        entpacken und das Readme befolgen. Weitere Informationen hierzu gibt es im OpenKNX-Wiki

            https://github.com/OpenKNX/OpenKNX/wiki/Installation-of-OpenKNX-tools

        Danach bitte dieses Script erneut starten.

        ACHTUNG: Heutige Browser warnen vor dem Inhalt des OpenKNX-Tools Pakets, 
                 weil es ausfuehrbare Programme und ein PowerShell-Skript enthaellt.
                 Falls jemand dem Paketinhalt nicht traut, kann er sich OpenKNXproducer
                 und bossac selber kompilieren und entsprechend installieren.
    "
    timeout /T -1
}

if ($toolsExist) {
    $xml = Get-ChildItem ../data/*.xml -Exclude content.xml
    $filename = [System.IO.Path]::GetFileNameWithoutExtension($xml)
    ~/bin/OpenKNXproducer.exe knxprod --NoXsd --Output="./$filename.knxprod" "../data/$filename.xml"
    timeout /T 20 
}
