# -----------------------------------------------------------------------------
#    Open #
#    +----+    OpenKNX - minimal KNXnet/IP client for PowerShell
#    # KNX     Author: Erkan Colak
# -----------------------------------------------------------------------------

<#
.SYNOPSIS
    Reads KNX Interface Object Properties over KNXnet/IP - from the interface itself
    or from any device on the bus.

.DESCRIPTION
    A small, dependency free module. Two ways in, one result shape:

      Get-KnxProperty -InterfaceIp <ip> -PropertyId <n>            device management (the interface)
      Get-KnxProperty -InterfaceIp <ip> -Pa <a.b.c> -PropertyId <n> tunnel (a device on the bus)

    Both return the raw octets as [byte[]], or $null when the device has no such Property.

    Reference implementation, kept as small as the protocol allows. Windows PowerShell 5.1.
    KNX Standard v3.0.0: 03_08_02 (Core), 03_08_03 (Device Management), 03_08_04 (Tunnelling),
    03_06_03 (cEMI), 03_03_04 (Transport Layer), 03_03_07 (Application Layer).

.EXAMPLE
    Import-Module ./OpenKnx.Knx.psm1
    Get-KnxProperty -InterfaceIp 11.11.0.210 -PropertyId 15

.EXAMPLE
    Get-KnxPropertyText -InterfaceIp 11.11.0.210 -Pa 5.0.7 -PropertyId 2
#>

Set-StrictMode -Version 2.0

# --- frame helpers -----------------------------------------------------------

function New-KnxHeader([int] $Service, [int] $TotalLength) {
    return [byte[]] @(0x06, 0x10,
        [byte](($Service -shr 8) -band 0xFF), [byte]($Service -band 0xFF),
        [byte](($TotalLength -shr 8) -band 0xFF), [byte]($TotalLength -band 0xFF))
}

function New-KnxHpai([System.Net.IPEndPoint] $EndPoint) {
    $ip = $EndPoint.Address.GetAddressBytes()
    $p = [int] $EndPoint.Port
    return [byte[]] @(0x08, 0x01, $ip[0], $ip[1], $ip[2], $ip[3],
        [byte](($p -shr 8) -band 0xFF), [byte]($p -band 0xFF))
}

function ConvertTo-KnxAddress([string] $Pa) {
    if ($Pa -notmatch '^\s*(\d+)\.(\d+)\.(\d+)\s*$') { throw "Not a physical address: '$Pa' (expected a.b.c)" }
    return [int] ((([int] $Matches[1]) -shl 12) -bor (([int] $Matches[2]) -shl 8) -bor ([int] $Matches[3]))
}

# Waits for one datagram of the wanted service type, so an unrelated frame does not end the wait.
function Receive-KnxFrame([System.Net.Sockets.UdpClient] $Client, [int] $Service, [int] $TimeoutMs) {
    $deadline = (Get-Date).AddMilliseconds($TimeoutMs)
    while ((Get-Date) -lt $deadline) {
        if ($Client.Available -le 0) { Start-Sleep -Milliseconds 5; continue }
        $remote = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, 0)
        $data = $Client.Receive([ref] $remote)
        if ($data.Length -ge 6) {
            $svc = ((([int] $data[2]) -shl 8) -bor ([int] $data[3]))
            if ($svc -eq $Service) { return $data }
        }
    }
    return $null
}

# --- connection --------------------------------------------------------------

<#
.SYNOPSIS
    Opens a KNXnet/IP connection. Type 03h = device management, 04h = tunnelling.
#>
function Open-KnxConnection {
    param(
        [Parameter(Mandatory = $true)][string] $InterfaceIp,
        [int] $Port = 3671,
        [ValidateSet('Management', 'Tunnel')][string] $Type = 'Management',
        [int] $TimeoutMs = 2000
    )

    $client = New-Object System.Net.Sockets.UdpClient(0)
    $client.Connect($InterfaceIp, $Port)
    $hpai = New-KnxHpai ([System.Net.IPEndPoint] $client.Client.LocalEndPoint)

    # CRI: management is 2 octets, tunnelling carries the layer (02h = link layer) plus one reserved
    $cri = if ($Type -eq 'Tunnel') { [byte[]] @(0x04, 0x04, 0x02, 0x00) } else { [byte[]] @(0x02, 0x03) }
    $body = [byte[]] ($hpai + $hpai + $cri)
    $frame = [byte[]] ((New-KnxHeader 0x0205 (6 + $body.Length)) + $body)
    [void] $client.Send($frame, $frame.Length)

    $resp = Receive-KnxFrame $client 0x0206 $TimeoutMs
    if ($null -eq $resp) { $client.Close(); throw "No CONNECT_RESPONSE from $InterfaceIp." }
    if ($resp[7] -ne 0) {
        $client.Close()
        # 24h no more connections, 28h not authorised (03_08_02 7.8.2)
        throw ("Connect refused, status 0x{0:X2}." -f $resp[7])
    }

    $own = 0
    if ($Type -eq 'Tunnel' -and $resp.Length -ge 20) { $own = ((([int] $resp[18]) -shl 8) -bor [int] $resp[19]) }

    return [pscustomobject]@{
        Client    = $client
        Hpai      = $hpai
        Channel   = [int] $resp[6]
        Type      = $Type
        Seq       = 0    # KNXnet/IP connection header sequence
        OwnPa     = $own # the individual address the interface lent us
        TimeoutMs = $TimeoutMs
    }
}

<#
.SYNOPSIS
    Closes a connection opened with Open-KnxConnection. Always call it, also on an error.
#>
function Close-KnxConnection {
    param([Parameter(Mandatory = $true)] $Connection)
    if ($null -eq $Connection) { return }
    try {
        $body = [byte[]] (@([byte] $Connection.Channel, 0x00) + $Connection.Hpai)
        $frame = [byte[]] ((New-KnxHeader 0x0209 (6 + $body.Length)) + $body)
        [void] $Connection.Client.Send($frame, $frame.Length)
    }
    catch { }
    $Connection.Client.Close()
}

# --- device management path (the interface itself) ---------------------------

function Send-KnxCemi($Connection, [byte[]] $Cemi, [int] $Service) {
    $conn = [byte[]] @(0x04, [byte] $Connection.Channel, [byte] ($Connection.Seq -band 0xFF), 0x00)
    $body = [byte[]] ($conn + $Cemi)
    $frame = [byte[]] ((New-KnxHeader $Service (6 + $body.Length)) + $body)
    [void] $Connection.Client.Send($frame, $frame.Length)
    $Connection.Seq = ($Connection.Seq + 1) -band 0xFF
}

function Confirm-KnxFrame($Connection, [byte[]] $Frame, [int] $AckService) {
    $ack = [byte[]] ((New-KnxHeader $AckService 10) + @(0x04, [byte] $Connection.Channel, $Frame[8], 0x00))
    [void] $Connection.Client.Send($ack, $ack.Length)
}

function Read-KnxPropertyLocal($Connection, [int] $PropertyId, [int] $Elements, [int] $StartIndex) {
    # M_PropRead.req on Interface Object Type 0, instance 1 (03_06_03 4.1.7.3)
    $cemi = [byte[]] @(0xFC, 0x00, 0x00, 0x01, [byte] $PropertyId,
        [byte] (((($Elements -band 0x0F) -shl 4)) -bor (($StartIndex -shr 8) -band 0x0F)),
        [byte] ($StartIndex -band 0xFF))
    Send-KnxCemi $Connection $cemi 0x0310
    [void] (Receive-KnxFrame $Connection.Client 0x0311 $Connection.TimeoutMs)
    $con = Receive-KnxFrame $Connection.Client 0x0310 $Connection.TimeoutMs
    if ($null -eq $con -or $con.Length -lt 17) { return $null }
    Confirm-KnxFrame $Connection $con 0x0311
    if ($con[10] -ne 0xFB) { return $null }
    if ((([int] $con[15]) -shr 4) -eq 0) { return $null }   # element count 0 -> the octet after is an error code
    return , ([byte[]] $con[17..($con.Length - 1)])
}

# --- tunnel path (a device on the bus) ---------------------------------------

# Sends one complete TPDU (TPCI already merged into its first octet).
# cEMI L_Data.req: MC | AddIL | CTRL1 | CTRL2 | SA | DA | LEN | TPDU...
# LEN counts the TPDU octets minus one (03_06_03 4.1.5.3.1).
function Send-KnxTpdu($Connection, [int] $Dest, [byte[]] $Tpdu) {
    $cemi = [byte[]] (@(0x11, 0x00, 0xBC, 0x60,
            [byte] (($Connection.OwnPa -shr 8) -band 0xFF), [byte] ($Connection.OwnPa -band 0xFF),
            [byte] (($Dest -shr 8) -band 0xFF), [byte] ($Dest -band 0xFF),
            [byte] ($Tpdu.Length - 1)) + $Tpdu)
    Send-KnxCemi $Connection $cemi 0x0420
}

# One receive loop for everything the interface sends. Waiting for the tunnelling ACK in a separate
# place would throw the answer away, because a datagram that does not match is discarded.
# Every TUNNELLING_REQUEST from the interface is acknowledged - an unacknowledged one is repeated
# and then ends the connection.
function Invoke-KnxTunnelPump($Connection, [int] $TimeoutMs, [scriptblock] $Match) {
    $deadline = (Get-Date).AddMilliseconds($TimeoutMs)
    while ((Get-Date) -lt $deadline) {
        if ($Connection.Client.Available -le 0) { Start-Sleep -Milliseconds 5; continue }
        $remote = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, 0)
        $f = $Connection.Client.Receive([ref] $remote)
        if ($f.Length -lt 6) { continue }
        $svc = ((([int] $f[2]) -shl 8) -bor [int] $f[3])
        if ($svc -eq 0x0421) { continue }                       # our own frame was acknowledged
        if ($svc -ne 0x0420 -or $f.Length -lt 16) { continue }
        Confirm-KnxFrame $Connection $f 0x0421
        $cemi = [byte[]] $f[10..($f.Length - 1)]
        if ($cemi[0] -ne 0x29) { continue }                     # L_Data.con and others carry no answer
        if ($null -eq $Match -or (& $Match $cemi)) { return $cemi }
    }
    return $null
}

function Read-KnxPropertyRemote($Connection, [int] $Dest, [int] $PropertyId, [int] $Elements, [int] $StartIndex) {
    # CONNECTIONLESS. 03_03_07 3.4.4.1 passes A_PropertyValue_Read with T_Data_Individual, so the TPCI
    # is 00h and there is no T_Connect, no sequence numbering and no T_Ack to keep track of. That is
    # not only less code - a device that answers connectionless but refuses a transport connection
    # (seen on a foreign interface) stays readable.
    # APCI 3D5h: its top two bits sit in the low two bits of the first octet, next to the TPCI.
    $tpdu = [byte[]] @(0x03, 0xD5, 0x00, [byte] $PropertyId,
        [byte] (((($Elements -band 0x0F) -shl 4)) -bor (($StartIndex -shr 8) -band 0x0F)),
        [byte] ($StartIndex -band 0xFF))
    Send-KnxTpdu $Connection $Dest $tpdu

    # L_Data.ind: 0:MC 1:AddIL 2:CTRL1 3:CTRL2 4-5:SA 6-7:DA 8:LEN 9:TPCI+APCIhi 10:APCIlo
    #             11:objIdx 12:PID 13-14:count|start 15..:data
    $answer = Invoke-KnxTunnelPump $Connection $Connection.TimeoutMs {
        param($c)
        ($c.Length -ge 15) -and
        (((($c[9] -band 0x03) -shl 8) -bor $c[10]) -eq 0x3D6) -and
        ($c[12] -eq $PropertyId)
    }
    # Drain what is still in flight - above all the L_Data.con of our own request. KNXnet/IP tunnelling
    # allows exactly ONE outstanding frame (03_08_04 2.6); leaving one unacknowledged makes the
    # interface repeat it and ignore the next request, so only the first read of a session succeeds.
    [void] (Invoke-KnxTunnelPump $Connection 120 { param($c) $false })
    if ($null -eq $answer) { return $null }

    if ((([int] $answer[13]) -shr 4) -eq 0) { return $null }   # element count 0 -> absent or refused
    if ($answer.Length -le 15) { return $null }
    return , ([byte[]] $answer[15..($answer.Length - 1)])
}

# --- public surface ----------------------------------------------------------

<#
.SYNOPSIS
    Reads one Property of the Device Object. Returns the raw octets, or $null if it is absent.
.PARAMETER Pa
    Physical address of a device on the bus. Without it the interface itself is read.
#>
function Get-KnxProperty {
    param(
        [Parameter(Mandatory = $true)][Alias('i')][string] $InterfaceIp,
        [Parameter(Mandatory = $true)][int] $PropertyId,
        [string] $Pa,
        [int] $Port = 3671,
        [int] $Elements = 1,
        [int] $StartIndex = 1,
        [int] $TimeoutMs = 2000,
        $Connection
    )

    $own = $null
    if ($null -eq $Connection) {
        $type = if ($Pa) { 'Tunnel' } else { 'Management' }
        $own = Open-KnxConnection -InterfaceIp $InterfaceIp -Port $Port -Type $type -TimeoutMs $TimeoutMs
        $Connection = $own
    }
    try {
        if ($Pa) { return Read-KnxPropertyRemote $Connection (ConvertTo-KnxAddress $Pa) $PropertyId $Elements $StartIndex }
        return Read-KnxPropertyLocal $Connection $PropertyId $Elements $StartIndex
    }
    finally { if ($null -ne $own) { Close-KnxConnection $own } }
}

<#
.SYNOPSIS
    Reads a text Property (one ISO 8859-1 octet per element) and returns it as a string.
.DESCRIPTION
    Start index 0 answers the current element count. Over device management the count field
    allows blocks of 15; a device on the bus answers in an L_Data_Standard Frame, which carries
    at most 9 data octets (03_03_07 Fig. 47, 03_02_02 2.2.4), so -Pa reads in blocks of 9.
#>
function Get-KnxPropertyText {
    param(
        [Parameter(Mandatory = $true)][Alias('i')][string] $InterfaceIp,
        [Parameter(Mandatory = $true)][int] $PropertyId,
        [string] $Pa,
        [int] $Port = 3671,
        [int] $TimeoutMs = 2000,
        $Connection
    )

    $own = $null
    if ($null -eq $Connection) {
        $type = if ($Pa) { 'Tunnel' } else { 'Management' }
        $own = Open-KnxConnection -InterfaceIp $InterfaceIp -Port $Port -Type $type -TimeoutMs $TimeoutMs
        $Connection = $own
    }
    try {
        $len = Get-KnxProperty -InterfaceIp $InterfaceIp -PropertyId $PropertyId -Pa $Pa -StartIndex 0 -Connection $Connection
        if ($null -eq $len -or $len.Length -lt 2) { return $null }
        $count = ((([int] $len[0]) -shl 8) -bor ([int] $len[1]))
        if ($count -le 0) { return $null }

        $all = New-Object System.Collections.Generic.List[byte]
        $index = 1
        $maxBlock = if ($Pa) { 9 } else { 15 }
        while ($index -le $count) {
            $block = [Math]::Min($maxBlock, $count - $index + 1)
            $part = Get-KnxProperty -InterfaceIp $InterfaceIp -PropertyId $PropertyId -Pa $Pa `
                -Elements $block -StartIndex $index -Connection $Connection
            if ($null -eq $part -or $part.Length -eq 0) { break }
            foreach ($b in $part) { [void] $all.Add($b) }
            $index += $block
        }
        if ($all.Count -eq 0) { return $null }
        return ([System.Text.Encoding]::GetEncoding(28591)).GetString($all.ToArray())
    }
    finally { if ($null -ne $own) { Close-KnxConnection $own } }
}

Export-ModuleMember -Function Open-KnxConnection, Close-KnxConnection, Get-KnxProperty, Get-KnxPropertyText, ConvertTo-KnxAddress
