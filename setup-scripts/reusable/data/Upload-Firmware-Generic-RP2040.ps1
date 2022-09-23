Import-Module BitsTransfer

$firmwareName = $args[0]

Write-Host Suche RP2040 als Disk-Laufwerk...
$device=$(Get-WmiObject Win32_LogicalDisk | Where-Object { $_.VolumeName -match "RPI-RP2" })
if (!$device)
{
    Write-Host Nicht gefunden, Suche COM-Port fuer RP2040...
    $portList = get-pnpdevice -class Ports
    if ($portList) {
        foreach($usbDevice in $portList) {
            if ($usbDevice.Present) {
                $isCom = $usbDevice.Name -match "USB.*\(COM(\d{1,3})\)"
                if($isCom)
                {
                    Write-Host Gefunden $port
                    $port = $Matches[0]
                    break
                }
            }
        }
        if($port)
        {
            Write-Host Verwende $port zum neustart vom RP2040
            $serial = new-Object System.IO.Ports.SerialPort $port,1200,None,8,1
            try { $serial.Open()} catch {}
            $serial.Close()
            # mode ${port}: BAUD=1200 parity=N data=8 stop=1 | Out-Null
            Start-Sleep -s 1
            # ./rp2040load.exe -v -D firmware
            $device=$(Get-WmiObject Win32_LogicalDisk | Where-Object { $_.VolumeName -match "RPI-RP2" })
        }
    }
}
if ($device)
{
    Write-Host Installiere firmware...
    Start-BitsTransfer -Source data/$firmwareName -Destination $device.DeviceID.ToString() -Description "Installiere" -DisplayName "Installiere Firmware..."
    # Copy-Item data/firmware.uf2 $device.DeviceID.ToString()
    Write-Host Fertig!
    timeout /T 20 
}
else 
{
    Write-Host 
    Write-Host "Kein RP2040 gefunden!"
    Write-Host 
    Write-Host "Versuche bitte die alternative Setup-Methode: Den RP2040 im BOOTSEL-Modus zu starten"
    Write-Host "Falls die Hardware eine Reset-Taste hat, dann erst die BOOTSEL-Taste drücken und halten,"
    Write-Host "und dann zusaetzlich die Reset-Taste druecken. Dann beide Tasten loslassen."
    Write-Host "Ohen Reset-Taste das Geraet stromlos machen (USB-Stecker ziehen und vom KNX trennen),"
    Write-Host "Danach die BOOTSEL-Taste druecken und gleichzeitig USB mit dem Recner verbinden."
    Write-Host "Jetzt befindet sich das Geraet im Bootmodus, die BOOTSEL-Taste kann jetzt losgelassen werden."
    Write-Host "Anschliessend das Skript erneut starten."
    timeout /T 60     
}
