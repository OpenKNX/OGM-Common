#!/usr/bin/env pwsh
<#
Open ■
┬────┴  OpenKNX-Image-Generic
■ KNX   2026 OpenKNX - Erkan Çolak

FILEPATH: OGM-Common/scripts/setup/reusable/data/OpenKNX-Image-Generic.ps1
   ships as: release/data/OpenKNX-Image-Generic.ps1

.SYNOPSIS
    The application image inside a firmware package — one place, dot-sourced by everything that needs it.

.DESCRIPTION
    A release ships ONE file per device, and that file is a package: a .uf2 wraps the image in 256-byte
    blocks, a .factory.bin puts a bootloader and a partition table in front of it. Both carry the plain
    application image, and three things in a release want it — the network upload, the difference
    builder, and anyone extracting it by hand.

    Those three used to be served by shipping the image a second time. They are served from here now,
    which is why the release holds one image instead of three copies of it.

    Two things a package does not state about itself are read from <name>.image.txt, written by the
    build: where the image starts, and where it ENDS. The end is the sharp one — a .uf2 pads its last
    block, so unwrapping alone gives up to 255 bytes too many, and a firmware difference is checked
    against the exact length.

.NOTES
    AUTHOR : Erkan Çolak
    Runs on Windows PowerShell 5.1 and PowerShell 7+, on Windows, macOS, Linux and Raspberry Pi.

.LINK
    https://wiki.openknx.de
#>

function OpenKNX_LE32([byte[]]$b, [int]$o) {
    # Multiplication, not a shift chain: in PowerShell a byte shifted left by 8 stays a byte and yields
    # 0. Multiplying promotes the operand, which is the only reason this reads the way it does.
    return [uint32]$b[$o] + ([uint32]$b[$o + 1] * 256) + ([uint32]$b[$o + 2] * 65536) + ([uint32]$b[$o + 3] * 16777216)
}

function OpenKNX_ImageFacts([string]$PackagePath) {
    <# @brief The facts file the build wrote next to a package, or $null. #>
    $name = [System.IO.Path]::GetFileName($PackagePath)
    $stem = $name -replace '\.factory\.bin$', '' -replace '\.app\.bin$', '' -replace '\.uf2$', '' -replace '\.bin$', ''
    $f = Join-Path ([System.IO.Path]::GetDirectoryName($PackagePath)) ($stem + ".image.txt")
    if (-not (Test-Path -PathType Leaf $f)) { return $null }
    $o = @{}
    foreach ($line in (Get-Content -LiteralPath $f)) {
        $t = $line.Trim()
        if (-not $t -or $t.StartsWith("#")) { continue }
        $eq = $t.IndexOf("=")
        if ($eq -lt 1) { continue }
        $o[$t.Substring(0, $eq).Trim()] = $t.Substring($eq + 1).Trim()
    }
    if (-not $o.ContainsKey("appLength")) { return $null }
    return $o
}

function OpenKNX_Sha256([byte[]]$b) {
    return [System.BitConverter]::ToString(
        [System.Security.Cryptography.SHA256]::Create().ComputeHash($b)).Replace("-", "").ToLower()
}

function OpenKNX_ExpandUf2([byte[]]$data) {
    <# @brief The image inside a .uf2: 512-byte blocks, ordered by target address. #>
    $BLK = 512
    if ($data.Length -lt $BLK) { return $null }
    $chunks = New-Object System.Collections.ArrayList
    $base = [uint32]::MaxValue
    $end = [uint32]0
    for ($o = 0; $o + $BLK -le $data.Length; $o += $BLK) {
        # The L suffix is not decoration: 0x9E5D5157 has bit 31 set, so the bare literal parses as a
        # NEGATIVE [int] while the computed value is a positive [long]. The two print identically and
        # compare as different. The other two magics are well under 2^31 and unaffected.
        if ((OpenKNX_LE32 $data $o) -ne 0x0A324655L) { continue }
        if ((OpenKNX_LE32 $data ($o + 4)) -ne 0x9E5D5157L) { continue }
        if ((OpenKNX_LE32 $data ($o + 508)) -ne 0x0AB16F30L) { continue }
        if (((OpenKNX_LE32 $data ($o + 8)) -band 0x00000001) -ne 0) { continue }  # not a main-flash block
        $addr = OpenKNX_LE32 $data ($o + 12)
        $len = OpenKNX_LE32 $data ($o + 16)
        if ($len -gt 476) { continue }
        [void]$chunks.Add(@{ Addr = $addr; Off = $o + 32; Len = $len })
        if ($addr -lt $base) { $base = $addr }
        if (($addr + $len) -gt $end) { $end = $addr + $len }
    }
    if ($chunks.Count -eq 0) { return $null }
    $out = New-Object byte[] ($end - $base)
    foreach ($c in $chunks) { [Array]::Copy($data, $c.Off, $out, ($c.Addr - $base), $c.Len) }
    return $out
}

function OpenKNX_ExpandFactory([byte[]]$d) {
    <#
    .SYNOPSIS
        The image inside a .factory.bin, located through the partition table rather than a fixed offset.
    #>
    $PT = 0x8000
    if ($d.Length -le 0x9000) { return $null }
    $appOff = 0
    for ($e = $PT; ($e + 32) -le $d.Length -and $e -lt ($PT + 0x1000); $e += 32) {
        if ($d[$e] -ne 0xAA -or $d[$e + 1] -ne 0x50) { break }
        if ($d[$e + 2] -ne 0) { continue }                              # type 0 = app
        $off = OpenKNX_LE32 $d ($e + 4)
        $sub = $d[$e + 3]
        if ($sub -eq 0) { $appOff = [int]$off; break }
        if ($appOff -eq 0 -and $sub -ge 0x10 -and $sub -le 0x1F) { $appOff = [int]$off }
    }
    if ($appOff -eq 0 -or ($appOff + 24) -ge $d.Length) { return $null }
    if ($d[$appOff] -ne 0xE9) { return $null }
    $segs = $d[$appOff + 1]
    $hash = ($d[$appOff + 23] -eq 1)
    $p = [int]$appOff + 24
    for ($i = 0; $i -lt $segs; $i++) {
        if (($p + 8) -gt $d.Length) { return $null }
        $p += 8 + [int](OpenKNX_LE32 $d ($p + 4))
        if ($p -gt $d.Length) { return $null }
    }
    $end = $p + (15 - ($p % 16)) + 1                                    # through the padded checksum byte
    if ($hash) { $end += 32 }                                           # ...and the appended SHA-256
    if ($end -gt $d.Length) { return $null }
    $out = New-Object byte[] ($end - $appOff)
    [Array]::Copy($d, $appOff, $out, 0, $out.Length)
    return $out
}

function OpenKNX_GetAppImage {
    <#
    .SYNOPSIS
        The application image behind whatever was pointed at. $null when it cannot be had.
    .PARAMETER Path
        A package (.uf2 / .factory.bin) or an image (.app.bin / .bin) — an image is returned unchanged.
    .PARAMETER Quiet
        Say nothing about what was done.
    #>
    param([string]$Path, [switch]$Quiet)
    if (-not (Test-Path -PathType Leaf $Path)) { return $null }
    $name = [System.IO.Path]::GetFileName($Path)
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $facts = OpenKNX_ImageFacts $Path

    if ($name -like "*.app.bin" -or ($name -like "*.bin" -and $name -notlike "*.factory.bin")) {
        return $bytes                                                   # already the image
    }

    $img = $null
    if ($null -ne $facts -and $facts.ContainsKey("appOffset") -and [int]$facts.appOffset -gt 0) {
        # The build said where. No partition table to walk, no offset to assume.
        $off = [int]$facts.appOffset
        $len = [int]$facts.appLength
        if (($off + $len) -le $bytes.Length) {
            $img = New-Object byte[] $len
            [Array]::Copy($bytes, $off, $img, 0, $len)
        }
    }
    elseif ($name -like "*.uf2") { $img = OpenKNX_ExpandUf2 $bytes }
    elseif ($name -like "*.factory.bin") { $img = OpenKNX_ExpandFactory $bytes }
    if ($null -eq $img) { return $null }

    if ($null -ne $facts) {
        $want = [int]$facts.appLength
        if ($img.Length -lt $want) { return $null }                     # shorter than promised: wrong
        if ($img.Length -gt $want) {
            $trim = New-Object byte[] $want
            [Array]::Copy($img, 0, $trim, 0, $want)
            $img = $trim
        }
        if ($facts.ContainsKey("appSha256") -and (OpenKNX_Sha256 $img) -ne $facts.appSha256) { return $null }
    }
    elseif (-not $Quiet) {
        Write-Host "  $name : ohne .image.txt - die Länge ist nicht gesichert" -ForegroundColor DarkYellow
    }
    return $img
}

function OpenKNX_SaveAppImage {
    <# @brief Write the application image next to (or wherever) — returns the path, or $null. #>
    param([string]$Path, [string]$Target, [switch]$Quiet)
    $img = OpenKNX_GetAppImage -Path $Path -Quiet:$Quiet
    if ($null -eq $img) { return $null }
    [System.IO.File]::WriteAllBytes($Target, $img)
    return $Target
}
