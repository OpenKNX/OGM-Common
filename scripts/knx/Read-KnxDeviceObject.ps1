# -----------------------------------------------------------------------------
#    Open #
#    +----+    OpenKNX - Read the Device Object of an interface or a bus device
#    # KNX     Author: Erkan Colak
# -----------------------------------------------------------------------------

<#
.SYNOPSIS
    Prints the Device Object of a KNXnet/IP interface, or of any device on the bus.

.DESCRIPTION
    All protocol work lives in OpenKnx.Knx.psm1 next to this file - this script only chooses
    what to read and how to show it, so it doubles as an example of using that module.

    Without -Pa the addressed interface answers about itself over a device management
    connection. With -Pa the interface tunnels an A_PropertyValue_Read to that device, so any
    KNX device can be read as long as some interface is reachable.

.PARAMETER InterfaceIp
    IPv4 address of the KNXnet/IP interface or router, for example 11.11.0.210. Alias -i.

.PARAMETER Pa
    Physical address of the device to read, for example 5.0.7. Alias -pa.
    Without it the interface itself is read.

.PARAMETER Port
    KNXnet/IP control port. Default 3671.

.PARAMETER Property
    Property identifiers to read. Default: the descriptive set.

.PARAMETER TimeoutMs
    Milliseconds to wait for each answer. Default 2000.

.EXAMPLE
    .\Read-KnxDeviceObject.ps1 -i 11.11.0.210

.EXAMPLE
    .\Read-KnxDeviceObject.ps1 -i 11.11.0.210 -pa 5.0.7

.EXAMPLE
    .\Read-KnxDeviceObject.ps1 -i 11.11.0.210 -Property 2,19,205,206
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][Alias('i')][string] $InterfaceIp,
    [string] $Pa,
    [int] $Port = 3671,
    [int[]] $Property = @(2, 9, 15, 16, 19, 25, 52, 55, 78, 83, 202, 203, 204, 205, 206, 207),
    [int] $TimeoutMs = 2000
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'OpenKnx.Knx.psm1') -Force

# PID -> readable name and how to render the value. Anything unlisted prints as hex.
$KNOWN = @{
    2   = @{ Name = 'Object Name (Geraetename)'; Kind = 'Text'  }
    9   = @{ Name = 'Firmware Revision';         Kind = 'Byte'  }
    15  = @{ Name = 'Order Info';                Kind = 'Ascii' }
    16  = @{ Name = 'PEI Type';                  Kind = 'Byte'  }
    19  = @{ Name = 'Manufacturer Data (HW-ID)'; Kind = 'Text'  }
    25  = @{ Name = 'Version';                   Kind = 'Ver'   }
    52  = @{ Name = 'Max Retry Count';           Kind = 'Retry' }
    55  = @{ Name = 'Product ID';                Kind = 'Hex'   }
    78  = @{ Name = 'Hardware Type';             Kind = 'Hex'   }
    83  = @{ Name = 'Device Descriptor (Mask)';  Kind = 'Hex'   }
    202 = @{ Name = 'Struct Info (OpenKNX)';     Kind = 'Struct' }
    203 = @{ Name = 'Firmware Name (OpenKNX)';   Kind = 'Text'  }
    204 = @{ Name = 'Device Name ab Werk';       Kind = 'Text'  }
    205 = @{ Name = 'Build-Zeitstempel';         Kind = 'Stamp' }
    206 = @{ Name = 'Build-Version';             Kind = 'Text'  }
    207 = @{ Name = 'Chip (MCU)';                Kind = 'Text'  }
}

function Format-Value([hashtable] $Info, [byte[]] $Bytes) {
    switch ($Info.Kind) {
        'Ascii' { return ((($Bytes | Where-Object { $_ -ge 0x20 }) | ForEach-Object { [char] $_ }) -join '') }
        'Byte'  { return [string] ([int] $Bytes[0]) }
        'Ver'   {
            # DPT_Version: 5 bit revision | 5 bit major | 6 bit minor - the revision is capped at 31 here
            $v = ((([int] $Bytes[0]) -shl 8) -bor [int] $Bytes[1])
            return "{0}.{1} rev {2}" -f (($v -shr 6) -band 0x1F), ($v -band 0x3F), (($v -shr 11) -band 0x1F)
        }
        'Struct' {
            # 9 octets: "OpenKNX", layout version, bit n = PID 202 + n present
            if ($Bytes.Length -lt 9) { return (($Bytes | ForEach-Object { "{0:X2}" -f $_ }) -join ' ') }
            $tag = (($Bytes[0..6] | ForEach-Object { [char] $_ }) -join '')
            if ($tag -cne 'OpenKNX') { return "kein OpenKNX: " + (($Bytes | ForEach-Object { "{0:X2}" -f $_ }) -join ' ') }
            $pids = @()
            for ($bit = 0; $bit -lt 8; $bit++) { if (($Bytes[8] -shr $bit) -band 1) { $pids += (202 + $bit) } }
            return "OpenKNX  Version {0}  PIDs {1}" -f $Bytes[7], ($pids -join ', ')
        }
        'Retry' { return "0x{0:X2}  (Busy {1} / Nak {2})" -f $Bytes[0], ((($Bytes[0] -shr 4) -band 7)), ($Bytes[0] -band 7) }
        'Stamp' {
            # 4 octets, seconds since the epoch. Everything else about the version lives in PID 25 and 9.
            $ts = 0
            for ($k = 0; $k -lt 4; $k++) { $ts = ($ts * 256) + [int] $Bytes[$k] }
            if ($ts -le 0) { return 'nicht gesetzt' }
            return ([datetimeoffset]::FromUnixTimeSeconds($ts)).LocalDateTime.ToString('yyyy-MM-dd HH:mm:ss')
        }
        default { return (($Bytes | ForEach-Object { "{0:X2}" -f $_ }) -join ' ') }
    }
}

$type = if ($Pa) { 'Tunnel' } else { 'Management' }
$cn = Open-KnxConnection -InterfaceIp $InterfaceIp -Port $Port -Type $type -TimeoutMs $TimeoutMs
try {
    $target = if ($Pa) { "Geraet $Pa  ueber $InterfaceIp" } else { "Interface $InterfaceIp" }
    Write-Host ""
    Write-Host ("  Device Object von {0}" -f $target)
    Write-Host "  ---------------------------------------------------------------"

    foreach ($propId in $Property) {
        $info = if ($KNOWN.ContainsKey($propId)) { $KNOWN[$propId] } else { @{ Name = 'unbekannt'; Kind = 'Hex' } }

        if ($info.Kind -eq 'Text') {
            $text = Get-KnxPropertyText -InterfaceIp $InterfaceIp -PropertyId $propId -Pa $Pa -Connection $cn
            if ([string]::IsNullOrEmpty($text)) {
                Write-Host ("  PID {0,3}  {1,-26} -" -f $propId, $info.Name) -ForegroundColor DarkGray
            }
            else {
                Write-Host ("  PID {0,3}  {1,-26} {2}" -f $propId, $info.Name, $text)
            }
            continue
        }

        $bytes = Get-KnxProperty -InterfaceIp $InterfaceIp -PropertyId $propId -Pa $Pa -Connection $cn
        if ($null -eq $bytes -or $bytes.Length -eq 0) {
            Write-Host ("  PID {0,3}  {1,-26} -" -f $propId, $info.Name) -ForegroundColor DarkGray
            continue
        }
        Write-Host ("  PID {0,3}  {1,-26} {2}" -f $propId, $info.Name, (Format-Value $info $bytes))
    }
    Write-Host ""
}
finally {
    # Always disconnect, also on an error: the interface allows only a few connections and a leaked
    # one blocks the next start with E_NO_MORE_CONNECTIONS (24h) until it times out on its own.
    Close-KnxConnection $cn
}
