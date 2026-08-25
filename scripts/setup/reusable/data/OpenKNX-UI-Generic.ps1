#!/usr/bin/env pwsh
<#
Open ■
┬────┴  OpenKNX-UI-Generic
■ KNX   2026 OpenKNX - Erkan Çolak

FILEPATH: OGM-Common/scripts/setup/reusable/data/OpenKNX-UI-Generic.ps1
   ships as: release/data/OpenKNX-UI-Generic.ps1

.SYNOPSIS
    The shared header for the OpenKNX firmware upload scripts — USB, network and KNX bus.

.DESCRIPTION
    Three routes lead to the same device, so they greet the user the same way. This file holds the one
    header they all print and the few helpers behind it, dot-sourced by each upload script.

    The header answers, in this order and always with the same keys:

      Gerät      which device this firmware belongs to
      Firmware   which version, and for which processor
      Datei      the file itself and its size
      Pfad       where it lies
      Weg        how it is going to get there — the only route-specific line
      System     what it is running on

    The device name comes from the firmware folder and is printed unchanged: a prettified name that is
    wrong once is worse than a plain one that is always right. Version and processor are read out of
    the firmware file by ftc; where ftc is absent the line falls back to what the caller detected.

    Nothing here talks to a device, opens a port or writes a file.

.NOTES
    AUTHOR : Erkan Çolak
    Runs on Windows PowerShell 5.1 and PowerShell 7+, on Windows, macOS, Linux and Raspberry Pi.

.LINK
    https://wiki.openknx.de
#>

# Windows PowerShell 5.1 has no $IsWindows/$IsMacOS/$IsLinux. Defining them here would collide with the
# host script, which defines them too -- so this file only READS them, through Get-Variable, and never
# assigns. 5.1 exists on Windows only, which is what makes the fallback correct rather than convenient.
function OpenKNX_UI_OnWindows {
    $v = Get-Variable -Name 'IsWindows' -ErrorAction SilentlyContinue
    if ($v) { return [bool]$v.Value }
    return $true
}
function OpenKNX_UI_OnMac {
    $v = Get-Variable -Name 'IsMacOS' -ErrorAction SilentlyContinue
    if ($v) { return [bool]$v.Value }
    return $false
}

$script:OpenKNX_UI_Strings = @{
    DE = @{
        TitleFmt = 'OpenKNX · {1} · {0}'
        KindUpdate = 'Firmware-Update'
        KindPackage = 'Firmware-Paket'
        Device   = 'Gerät'
        Firmware = 'Firmware'
        File     = 'Datei'
        Path     = 'Pfad'
        Way      = 'Weg'
        System   = 'System'
        Unknown  = 'unbekannt'
    }
    EN = @{
        TitleFmt = 'OpenKNX · {1} · {0}'
        KindUpdate = 'Firmware update'
        KindPackage = 'firmware package'
        Device   = 'Device'
        Firmware = 'Firmware'
        File     = 'File'
        Path     = 'Path'
        Way      = 'Route'
        System   = 'System'
        Unknown  = 'unknown'
    }
}

function OpenKNX_UI_Lang {
    <# @brief Normalise a language tag to DE or EN, defaulting to DE. #>
    param([string]$Lang = "")
    $l = "$Lang".ToUpper()
    if ($l -eq 'EN') { return 'EN' }
    return 'DE'
}

function OpenKNX_ShowLogo {
    <# @brief The OpenKNX logo block with one title line. Identical in every script by design. #>
    param($AddCustomText = $null)
    Write-Host ""
    Write-Host "Open " -NoNewline
    Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -ForegroundColor Green
    $bar = "$( [char]::ConvertFromUtf32(0x252C) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2500) )$( [char]::ConvertFromUtf32(0x2534) ) "
    if ($AddCustomText) { Write-Host "$bar $AddCustomText" -ForegroundColor Green }
    else { Write-Host $bar -ForegroundColor Green }
    Write-Host "$( [char]::ConvertFromUtf32(0x25A0) )" -NoNewline -ForegroundColor Green
    Write-Host " KNX"
    Write-Host ""
}

function OpenKNX_ShowTitle {
    <#
    .SYNOPSIS
        Logo plus the unified title: product first, what it is, then the route.
    .PARAMETER Kind
        What this is about. Defaults to a firmware update, because most of these scripts are one --
        the extractor is not, and calling its screen an update would be a small lie in large letters.
    #>
    param([string]$Way, [string]$Lang = "", [string]$Kind = "")
    $s = $script:OpenKNX_UI_Strings[(OpenKNX_UI_Lang $Lang)]
    $k = $Kind
    if (-not $k) { $k = $s.KindUpdate }
    OpenKNX_ShowLogo ($s.TitleFmt -f $Way, $k)
}

function OpenKNX_ShortenPath {
    <#
    .SYNOPSIS
        Keep the tail of a path -- the part that identifies it -- cut on a separator.
    .DESCRIPTION
        Dropping whole components keeps every remaining name readable; cutting by character count would
        leave a half word at the front that looks like a different folder.

        Split via the -split operator rather than String.Split(): passing a string array plus
        StringSplitOptions binds to an overload that returns the whole path as one element, which looked
        exactly like a path that did not need shortening.
    #>
    param([string]$Path, [int]$Max = 62)
    if (-not $Path) { return "" }
    if ($Path.Length -le $Max) { return $Path }
    $ell = [char]::ConvertFromUtf32(0x2026)
    $sep = [System.IO.Path]::DirectorySeparatorChar
    $parts = @($Path -split '[\\/]+' | Where-Object { $_ })
    if ($parts.Count -eq 0) { return $ell + $Path.Substring($Path.Length - ($Max - 1)) }
    $tail = @()
    foreach ($i in ($parts.Count - 1)..0) {
        $cand = @($parts[$i]) + $tail
        if ((($cand -join $sep).Length + 2) -gt $Max -and $tail.Count -gt 0) { break }
        $tail = $cand
    }
    return $ell + $sep + ($tail -join $sep)
}

function OpenKNX_FormatBytes {
    <#
    .SYNOPSIS
        A byte count grouped in the chosen language, not in the machine's locale.
    .DESCRIPTION
        "{0:N0}" follows the current culture, so the same release printed 2.181.632 on one machine and
        2,181,632 on the next. The header already commits to a language; the number follows that.
    #>
    param([long]$Bytes, [string]$Lang = "")
    $ci = [System.Globalization.CultureInfo]::GetCultureInfo('de-DE')
    if ((OpenKNX_UI_Lang $Lang) -eq 'EN') { $ci = [System.Globalization.CultureInfo]::GetCultureInfo('en-US') }
    return $Bytes.ToString('N0', $ci)
}

function OpenKNX_ShowKv {
    <# @brief One key/value line in the shared 12-column gutter. #>
    param([string]$Key, [string]$Value, [string]$Dim = "")
    Write-Host ("  {0,-12} " -f $Key) -NoNewline -ForegroundColor DarkGray
    Write-Host $Value -NoNewline
    if ($Dim) { Write-Host "  $Dim" -ForegroundColor DarkGray }
    else { Write-Host "" }
}

function OpenKNX_ShowContext {
    <#
    .SYNOPSIS
        The six-line context block. A line whose value is empty is left out, never faked.
    #>
    param(
        [hashtable]$Facts,
        [string]$Way = "",
        [string]$WayHint = "",
        [string]$Note = "",
        [string]$Lang = ""
    )
    $s = $script:OpenKNX_UI_Strings[(OpenKNX_UI_Lang $Lang)]

    if ($Facts.Device) { OpenKNX_ShowKv $s.Device $Facts.Device }

    # Version and processor share one line: together they say what is going onto the device. Either may
    # be missing -- an unstamped build has no version, a .gz says nothing about its processor.
    $fw = ""
    if ($Facts.Version) { $fw = $Facts.Version }
    if ($Facts.Mcu) {
        if ($fw) { $fw = $fw + "  ·  " + $Facts.Mcu }
        else { $fw = $Facts.Mcu }
    }
    if ($fw) { OpenKNX_ShowKv $s.Firmware $fw }

    if ($Facts.File) {
        $size = ""
        if ($Facts.Size) { $size = (OpenKNX_FormatBytes -Bytes $Facts.Size -Lang $Lang) + " B" }
        OpenKNX_ShowKv $s.File $Facts.File $size
    }
    if ($Facts.Dir) { OpenKNX_ShowKv $s.Path (OpenKNX_ShortenPath $Facts.Dir) }
    if ($Way) {
        # The hint belongs to the route, so it is joined to it rather than set beside it as a second value.
        $hint = ""
        if ($WayHint) { $hint = "·  " + $WayHint }
        OpenKNX_ShowKv $s.Way $Way $hint
    }

    $psv = "$($PSVersionTable.PSVersion)"
    $os = "Windows"
    if (OpenKNX_UI_OnMac) { $os = "macOS" }
    elseif (-not (OpenKNX_UI_OnWindows)) { $os = "Linux" }
    OpenKNX_ShowKv $s.System "$os  ·  PowerShell $psv"

    if ($Note) {
        # Trimmed: the callers' existing strings carry their own two-space indent, which would land
        # behind the "!" marker and read as a second level.
        Write-Host ""
        Write-Host ("  !  " + $Note.Trim()) -ForegroundColor Yellow
    }
    Write-Host ""
}

function OpenKNX_ReadChoice {
    <# @brief One prompt, one trimmed answer, an empty answer falling back to the default. #>
    param([string]$Prompt, [string]$Default = "")
    $suffix = ""
    if ($Default) { $suffix = " [$Default]" }
    $answer = Read-Host "  $Prompt$suffix"
    if (-not $answer) { return $Default }
    return $answer.Trim()
}

function OpenKNX_GetFtcVersion {
    <#
    .SYNOPSIS
        ftc's own version, or "" when the binary cannot be asked.
    .DESCRIPTION
        Asking is also the executability test. A release ships one binary per platform, and picking the
        wrong one is not a theoretical mistake: a Windows build sitting in Tools/ answers nothing on a
        Mac. Running it and reading the answer settles both questions at once.
    #>
    param([string]$Exe)
    if (-not $Exe) { return "" }
    try {
        $out = & $Exe --version 2>&1 | Out-String
        if ($out -match '(?m)^\s*ftc\s+([0-9]+\.[0-9]+\.[0-9]+)') { return $Matches[1] }
    }
    catch { }
    return ""
}

function OpenKNX_GetArch {
    <# @brief x64 | arm64 | armhf | x86. 64-bit patterns are tested first: "x86_64" contains "x86". #>
    $a = ""
    try { $a = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString() } catch { }
    if (-not $a) { $a = "$env:PROCESSOR_ARCHITECTURE" }
    $a = $a.ToLower()
    if ($a -match 'arm64|aarch64') { return 'arm64' }
    if ($a -match 'amd64|x86_64|x64') { return 'x64' }
    if ($a -match 'arm') { return 'armhf' }
    if ($a -match 'x86|i386|i686') { return 'x86' }
    return 'x64'
}

function OpenKNX_FtcCandidates {
    <# @brief The binary names for this platform, most specific first, the unsuffixed one last. #>
    $arch = OpenKNX_GetArch
    if (OpenKNX_UI_OnWindows) {
        $names = @("ftc-windows-$arch.exe")
        if ($arch -ne 'x64') { $names += "ftc-windows-x64.exe" }
        $names += "ftc.exe"
        return $names
    }
    if (OpenKNX_UI_OnMac) {
        $names = @("ftc-macos-$arch")
        if ($arch -ne 'x64') { $names += "ftc-macos-x64" }
        $names += "ftc"
        return $names
    }
    $names = @("ftc-linux-$arch")
    $names += "ftc"
    return $names
}

function OpenKNX_GetFtcBuild {
    <#
    .SYNOPSIS
        When an ftc binary was built, as a DateTime, or $null.
    .DESCRIPTION
        Two builds of the same version report the same version -- so the version alone cannot tell a
        release copy from a months-old installed one, and the newer copy would never be offered. ftc
        prints its build stamp, and that settles it. Matched by the SHAPE of the stamp rather than by
        its label, because the label is translated and the stamp is not.

        Not the file's timestamp: copying a release rewrites that, so it says when the file arrived,
        never when the program was built.
    #>
    param([string]$Exe)
    if (-not $Exe) { return $null }
    try {
        $out = & $Exe --version 2>&1 | Out-String
        if ($out -match '(?m)^\s*\S+\s+([A-Z][a-z]{2}\s+\d{1,2}\s+\d{4}\s+\d{2}:\d{2}:\d{2})\s*$') {
            # __DATE__ pads a single-digit day with a space, so the run is collapsed before parsing.
            $stamp = ($Matches[1] -replace '\s+', ' ')
            return [datetime]::ParseExact($stamp, 'MMM d yyyy HH:mm:ss',
                                          [System.Globalization.CultureInfo]::InvariantCulture)
        }
    }
    catch { }
    return $null
}

function OpenKNX_FindFtc {
    <#
    .SYNOPSIS
        A usable ftc: the one shipped with this release, and the one already installed.
    .DESCRIPTION
        Both are reported, because which one to use is a decision worth showing rather than making
        silently. Every candidate is verified by asking it for its version -- a name alone proves
        nothing about whether the binary runs here.
    #>
    param([string[]]$SearchDirs = @())

    $shipped = ""
    $shippedVer = ""
    $shippedBuild = $null
    foreach ($d in $SearchDirs) {
        if (-not $d) { continue }
        if (-not (Test-Path -PathType Container $d)) { continue }
        foreach ($n in (OpenKNX_FtcCandidates)) {
            $p = Join-Path $d $n
            if (-not (Test-Path -PathType Leaf $p)) { continue }
            $v = OpenKNX_GetFtcVersion $p
            if ($v) { $shipped = (Resolve-Path $p).Path; $shippedVer = $v; $shippedBuild = OpenKNX_GetFtcBuild $p; break }
        }
        if ($shipped) { break }
    }

    $installed = ""
    $installedVer = ""
    $installedBuild = $null
    $onPathName = "ftc"
    if (OpenKNX_UI_OnWindows) { $onPathName = "ftc.exe" }
    $cmd = Get-Command $onPathName -ErrorAction SilentlyContinue
    if ($cmd) {
        $v = OpenKNX_GetFtcVersion $cmd.Source
        if ($v) { $installed = $cmd.Source; $installedVer = $v; $installedBuild = OpenKNX_GetFtcBuild $cmd.Source }
    }
    if (-not $installed) {
        foreach ($d in @("$HOME/.local/bin", "$HOME/bin")) {
            $p = Join-Path $d $onPathName
            if (-not (Test-Path -PathType Leaf $p)) { continue }
            $v = OpenKNX_GetFtcVersion $p
            if ($v) { $installed = $p; $installedVer = $v; $installedBuild = OpenKNX_GetFtcBuild $p; break }
        }
    }
    return @{ Shipped = $shipped; ShippedVersion = $shippedVer; ShippedBuild = $shippedBuild
              Installed = $installed; InstalledVersion = $installedVer; InstalledBuild = $installedBuild }
}

function OpenKNX_CompareVersion {
    <# @brief -1 / 0 / 1. An unreadable version counts as older, so a working copy always wins. #>
    param([string]$A, [string]$B)
    if (-not $A) { return -1 }
    if (-not $B) { return 1 }
    try { return ([version]$A).CompareTo([version]$B) } catch { return 0 }
}

function OpenKNX_ReadIdentityInto {
    <#
    .SYNOPSIS
        Fill Version and Mcu from what ftc reads out of one firmware file. Silent on refusal.
    .DESCRIPTION
        Capturing ftc's output is what keeps it offline: with --check and no interface it stops at the
        file only when its output is NOT a terminal. A refused file simply leaves the fields as they were.
    #>
    param([hashtable]$Facts, [string]$File, [string]$FtcExe, [string]$Lang = "")
    if (-not $FtcExe) { return $Facts }
    try {
        $lang = "de"
        if ((OpenKNX_UI_Lang $Lang) -eq 'EN') { $lang = "en" }
        $out = & $FtcExe --lang $lang knxota $File --check 2>&1 | Out-String
        if ($out -match '(?m)^\s*Version\s+([0-9]+\.[0-9]+\.[0-9]+)') { $Facts.Version = $Matches[1] }
        if ($out -match '(?m)^\s*Hardware\s+(\S+)') { $Facts.Mcu = $Matches[1] }
    }
    catch { }
    return $Facts
}

function OpenKNX_ShippedIsNewer {
    <#
    .SYNOPSIS
        Is the copy in this release newer than the installed one?
    .DESCRIPTION
        Version first. On a tie the build stamp decides, which is the case that matters in practice: a
        rebuilt tool keeps its version number, so "equal versions" would otherwise hide every fix made
        since the installed copy was put there.
    #>
    param([hashtable]$Found)
    if (-not $Found.Shipped) { return $false }
    if (-not $Found.Installed) { return $true }
    $cmp = OpenKNX_CompareVersion $Found.ShippedVersion $Found.InstalledVersion
    if ($cmp -gt 0) { return $true }
    if ($cmp -lt 0) { return $false }
    if ($null -eq $Found.ShippedBuild) { return $false }
    if ($null -eq $Found.InstalledBuild) { return $true }
    return ($Found.ShippedBuild -gt $Found.InstalledBuild)
}

function OpenKNX_GetFirmwareFacts {
    <#
    .SYNOPSIS
        What the header needs about one firmware file.
    .DESCRIPTION
        The device name is the firmware folder's name, printed unchanged. Version and processor are read
        out of the file by ftc, whose output is captured -- and capturing it is what keeps ftc offline:
        with --check and no interface it stops at the file only when its output is NOT a terminal.
        Without ftc the two fields stay empty and the caller's own detection fills the processor in.
    #>
    param([string]$FirmwarePath, [string]$FtcExe = "", [string]$Mcu = "", [string]$Lang = "")

    $facts = @{ Device = ""; Version = ""; Mcu = $Mcu; File = ""; Size = 0; Dir = "" }
    if (-not $FirmwarePath) { return $facts }
    if (-not (Test-Path -PathType Leaf $FirmwarePath)) { return $facts }

    $item = Get-Item $FirmwarePath
    $facts.File = $item.Name
    $facts.Size = $item.Length
    $facts.Dir = $item.DirectoryName
    $facts.Device = Split-Path -Leaf $item.DirectoryName

    if ($FtcExe) {
        $facts = OpenKNX_ReadIdentityInto -Facts $facts -File $item.FullName -FtcExe $FtcExe -Lang $Lang
        # Some files in a release cannot state an identity: a raw RP image has none, and an ESP
        # .factory.bin carries the bootloader and the partition table in front of the image, so nothing
        # at offset 0 says what it is. Their siblings can -- the .uf2 for an RP, the application image
        # for an ESP -- and every one of them comes out of the same build in the same folder.
        foreach ($pattern in @('*.uf2', '*.app.bin')) {
            if ($facts.Version) { break }
            $sib = Get-ChildItem -LiteralPath $facts.Dir -Filter $pattern -File -ErrorAction SilentlyContinue |
                   Select-Object -First 1
            if ($sib) {
                $facts = OpenKNX_ReadIdentityInto -Facts $facts -File $sib.FullName -FtcExe $FtcExe -Lang $Lang
            }
        }
    }
    return $facts
}
