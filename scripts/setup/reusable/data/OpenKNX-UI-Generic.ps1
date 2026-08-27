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


function OpenKNX_SelectInteractive {
    <#
    .SYNOPSIS
        Pick a row with the arrow keys. Returns the index, -1 on cancel, -2 for "one level up",
        and -3 when this console cannot do it -- then the caller falls back to numbers.
    .DESCRIPTION
        Up / Down move, Enter takes, Backspace or Left goes up a level, Esc cancels, a letter jumps to
        the next entry starting with it. Everything is drawn in place, so the screen does not fill up.
        Works in Windows PowerShell 5.1 and in PowerShell 7 -- ReadKey and SetCursorPosition are .NET,
        not a PowerShell feature. A redirected input has no keys at all, hence the -3.
    #>
    param([string[]]$Lines, [string]$Header = "", [int]$Selected = 0, [string]$Hint = "")
    if (-not $Lines -or $Lines.Count -eq 0) { return -1 }
    try { if ([Console]::IsInputRedirected) { return -3 } } catch { return -3 }

    $winH = 15
    try { $winH = [Math]::Max(5, [Math]::Min(20, [Console]::WindowHeight - 8)) } catch { }
    $count = $Lines.Count
    if ($Selected -lt 0 -or $Selected -ge $count) { $Selected = 0 }
    $off = 0
    $shown = [Math]::Min($winH, $count)
    $width = 100
    try { $width = [Math]::Max(40, [Console]::WindowWidth - 1) } catch { }

    # Print the block once, then derive the top from where the cursor ended up -- that way the anchor
    # is right even when the window scrolled while printing.
    $block = $shown + 2
    for ($i = 0; $i -lt $block; $i++) { Write-Host "" }
    $top = 0
    try { $top = [Console]::CursorTop - $block } catch { return -3 }
    if ($top -lt 0) { $top = 0 }

    # On the way out the block clears itself. Otherwise every level walked through stays on screen and
    # the display grows with each step -- this is ONE list that changes, not a stack of lists. finally
    # also catches the returns from inside the loop.
    try {

    while ($true) {
        if ($Selected -lt $off) { $off = $Selected }
        if ($Selected -ge $off + $shown) { $off = $Selected - $shown + 1 }
        try { [Console]::SetCursorPosition(0, $top) } catch { return -3 }

        $head = $Header
        if ($count -gt $shown) { $head = "$Header   ($($Selected + 1)/$count)" }
        Write-Host ($head.PadRight($width).Substring(0, $width)) -ForegroundColor DarkGray
        for ($i = 0; $i -lt $shown; $i++) {
            $idx = $off + $i
            $text = ""
            if ($idx -lt $count) { $text = $Lines[$idx] }
            $mark = "   "
            if ($idx -eq $Selected) { $mark = " > " }
            $row = ($mark + $text)
            if ($row.Length -gt $width) { $row = $row.Substring(0, $width) }
            if ($idx -eq $Selected) { Write-Host $row.PadRight($width) -ForegroundColor Black -BackgroundColor Green }
            else { Write-Host $row.PadRight($width) }
        }
        Write-Host ($Hint.PadRight($width).Substring(0, $width)) -ForegroundColor DarkGray

        $key = $null
        try { $key = [Console]::ReadKey($true) } catch { return -3 }
        switch ($key.Key) {
            "UpArrow"    { if ($Selected -gt 0) { $Selected-- } else { $Selected = $count - 1 }; break }
            "DownArrow"  { if ($Selected -lt $count - 1) { $Selected++ } else { $Selected = 0 }; break }
            "Home"       { $Selected = 0; break }
            "End"        { $Selected = $count - 1; break }
            "PageUp"     { $Selected = [Math]::Max(0, $Selected - $shown); break }
            "PageDown"   { $Selected = [Math]::Min($count - 1, $Selected + $shown); break }
            "Enter"      { return $Selected }
            "Spacebar"   { return $Selected }
            "RightArrow" { return $Selected }
            "Backspace"  { return -2 }
            "LeftArrow"  { return -2 }
            "Escape"     { return -1 }
            default {
                $ch = $key.KeyChar
                if ($ch -and [char]::IsLetterOrDigit($ch)) {
                    # Jump to the next entry starting with this letter, counting from the current row.
                    for ($k = 1; $k -le $count; $k++) {
                        $idx = ($Selected + $k) % $count
                        $t = $Lines[$idx].TrimStart()
                        if ($t.Length -gt 0 -and [char]::ToLower($t[0]) -eq [char]::ToLower($ch)) { $Selected = $idx; break }
                    }
                }
            }
        }
    }

    }
    finally {
        try {
            [Console]::SetCursorPosition(0, $top)
            for ($i = 0; $i -lt $block; $i++) { Write-Host ("".PadRight($width)) }
            [Console]::SetCursorPosition(0, $top)
        } catch { }
    }
}

function OpenKNX_ListRoots {
    <#
    .SYNOPSIS
        Where "up" ends: the drives of this machine.
    .DESCRIPTION
        Windows has several roots, Unix has one -- but a firmware often sits on a stick, and that is
        mounted under /Volumes (macOS) or /media, /run/media, /mnt (Linux). Those are listed too, so
        walking up leads to the stick instead of a dead end at "/".
    #>
    $out = @()
    if (OpenKNX_UI_OnWindows) {
        foreach ($d in (Get-PSDrive -PSProvider FileSystem -ErrorAction SilentlyContinue)) {
            if ($d.Root) { $out += $d.Root }
        }
        return $out
    }
    $out += "/"
    $mounts = @()
    if (OpenKNX_UI_OnMac) { $mounts = @("/Volumes") }
    else { $mounts = @("/media/$env:USER", "/run/media/$env:USER", "/media", "/mnt") }
    foreach ($m in $mounts) {
        if (-not (Test-Path -PathType Container $m)) { continue }
        foreach ($v in (Get-ChildItem -LiteralPath $m -Directory -ErrorAction SilentlyContinue)) {
            $out += $v.FullName
        }
    }
    return ($out | Select-Object -Unique)
}

function OpenKNX_PickFileNative {
    <#
    .SYNOPSIS
        The system's own file chooser, where there is one.
    .DESCRIPTION
        Windows gets the common dialog, macOS the Finder chooser, Linux zenity or kdialog if installed.
        Every one of them can be absent or refuse to open (no desktop session, PowerShell running MTA);
        that is not an error here -- the caller falls back to walking the folders in the terminal.
        Returns "" when nothing was chosen.
    #>
    param([string]$Start = ".", [string[]]$Include = @("*"))
    $start = (Resolve-Path -ErrorAction SilentlyContinue $Start)
    if ($start) { $start = $start.Path } else { $start = (Get-Location).Path }

    if (OpenKNX_UI_OnWindows) {
        try {
            Add-Type -AssemblyName System.Windows.Forms -ErrorAction Stop
            $dlg = New-Object System.Windows.Forms.OpenFileDialog
            $dlg.InitialDirectory = $start
            $pat = ($Include -join ';')
            $dlg.Filter = "Firmware ($pat)|$pat|Alle Dateien (*.*)|*.*"
            $dlg.Multiselect = $false
            if ($dlg.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) { return $dlg.FileName }
        } catch { }
        return ""
    }
    if (OpenKNX_UI_OnMac) {
        # `choose file` returns an alias; POSIX path makes it usable. A cancel raises -128 -> empty.
        $script = 'try' + "`n" +
                  'set f to choose file with prompt "OpenKNX: Firmware auswaehlen" default location POSIX file "' + $start + '"' + "`n" +
                  'POSIX path of f' + "`n" + 'end try'
        try {
            $out = (& osascript -e $script 2>$null)
            if ($LASTEXITCODE -eq 0 -and $out) { return ($out | Select-Object -First 1).Trim() }
        } catch { }
        return ""
    }
    foreach ($tool in @("zenity", "kdialog")) {
        $cmd = Get-Command $tool -ErrorAction SilentlyContinue
        if (-not $cmd) { continue }
        try {
            if ($tool -eq "zenity") { $out = (& $cmd --file-selection --filename "$start/" 2>$null) }
            else { $out = (& $cmd --getopenfilename "$start" 2>$null) }
            if ($LASTEXITCODE -eq 0 -and $out) { return ($out | Select-Object -First 1).Trim() }
        } catch { }
    }
    return ""
}

function OpenKNX_WalkFile {
    <#
    .SYNOPSIS
        Step through folders in the terminal -- the fallback that always works.
    .DESCRIPTION
        Over SSH, in a build console and on a machine without a desktop there is no dialog. Arrow keys
        move, Enter takes, Backspace goes up, Esc cancels; a console that cannot do keys (redirected
        input) gets the numbered list instead. Walking up past the root lists the drives, so a firmware
        on a stick is reachable. Returns "" when nothing was chosen.
    #>
    param([string]$Start = ".", [string[]]$Include = @("*"), [string]$Prompt = "Auswahl")
    $cur = (Resolve-Path -ErrorAction SilentlyContinue $Start)
    if ($cur) { $cur = $cur.Path } else { $cur = (Get-Location).Path }
    $hint = "  Pfeile bewegen . Enter waehlt . Backspace eine Ebene hoch . Esc bricht ab"

    while ($true) {
        $labels = @()
        $entries = @()

        if ($cur -eq "@roots") {
            foreach ($r in (OpenKNX_ListRoots)) {
                $labels += $r
                $entries += @{ Kind = "dir"; Path = $r }
            }
            $head = "  Laufwerke"
        }
        else {
            $upTxt = ".."
            if (-not (Split-Path -Parent $cur)) { $upTxt = ".. (Laufwerke)" }
            $labels += $upTxt
            $entries += @{ Kind = "up" }
            if ($script:OpenKNX_HaveNativePicker) {
                $labels += "Dateidialog des Systems oeffnen ..."
                $entries += @{ Kind = "native" }
            }

            foreach ($d in (Get-ChildItem -LiteralPath $cur -Directory -ErrorAction SilentlyContinue | Sort-Object Name)) {
                $labels += ($d.Name + "/")
                $entries += @{ Kind = "dir"; Path = $d.FullName }
            }
            # -Include beisst ohne -Recurse nicht zuverlaessig; deshalb selbst filtern.
            $files = @(Get-ChildItem -LiteralPath $cur -File -ErrorAction SilentlyContinue |
                       Where-Object { $n = $_.Name; @($Include | Where-Object { $n -like $_ }).Count -gt 0 } |
                       Sort-Object LastWriteTime -Descending)
            foreach ($f in $files) {
                $labels += ("{0,-46} {1,10:N0} B   {2:dd.MM. HH:mm}" -f $f.Name, $f.Length, $f.LastWriteTime)
                $entries += @{ Kind = "file"; Path = $f.FullName }
            }
            $head = "  " + $cur
        }

        $pick = OpenKNX_SelectInteractive -Lines $labels -Header $head -Hint $hint
        if ($pick -eq -3) {
            # No keys (redirected input): fall back to the numbered list.
            Write-Host ""
            Write-Host $head -ForegroundColor DarkGray
            for ($i = 0; $i -lt $labels.Count; $i++) { Write-Host ("  [{0}] {1}" -f ($i + 1), $labels[$i]) }
            Write-Host ""
            $answer = Read-Host ("  $Prompt (Nummer, Pfad, leer = abbrechen)")
            if (-not $answer) { return "" }
            $answer = $answer.Trim()
            $n = 0
            if ([int]::TryParse($answer, [ref]$n) -and $n -ge 1 -and $n -le $entries.Count) { $pick = $n - 1 }
            else {
                $cand = $answer
                if (-not [System.IO.Path]::IsPathRooted($cand)) { $cand = Join-Path $cur $cand }
                if (Test-Path -PathType Leaf $cand) { return (Resolve-Path $cand).Path }
                if (Test-Path -PathType Container $cand) { $cur = (Resolve-Path $cand).Path; continue }
                Write-Host ("  nicht gefunden: {0}" -f $answer) -ForegroundColor Yellow
                continue
            }
        }
        if ($pick -eq -1) { return "" }
        if ($pick -eq -2) {
            if ($cur -eq "@roots") { return "" }
            $up = Split-Path -Parent $cur
            if ($up) { $cur = $up } else { $cur = "@roots" }
            continue
        }
        $e = $entries[$pick]
        if ($e.Kind -eq "up") {
            $up = Split-Path -Parent $cur
            if ($up) { $cur = $up } else { $cur = "@roots" }
            continue
        }
        if ($e.Kind -eq "native") {
            $n = OpenKNX_PickFileNative -Start $cur -Include $Include
            if ($n -and (Test-Path -PathType Leaf $n)) { return $n }
            continue                                  # cancelled: back to the list
        }
        if ($e.Kind -eq "dir") { $cur = $e.Path; continue }
        return $e.Path
    }
}

function OpenKNX_PickFile {
    <#
    .SYNOPSIS
        Choose a file: arrow keys in the terminal, with the system dialog one row away.
    .DESCRIPTION
        The keyboard leads -- it works in every console, over SSH and without a desktop. Where a native
        chooser exists it is offered as a row in the list, so nobody has to leave the terminal for it and
        nobody gets a window popped at them who did not ask. "" = nothing chosen.
    #>
    param([string]$Start = ".", [string[]]$Include = @("*"), [string]$Prompt = "Auswahl")
    $script:OpenKNX_HaveNativePicker = $false
    try {
        if (-not [Console]::IsInputRedirected) {
            if (OpenKNX_UI_OnWindows) { $script:OpenKNX_HaveNativePicker = $true }
            elseif (OpenKNX_UI_OnMac) { $script:OpenKNX_HaveNativePicker = $true }
            elseif ((Get-Command zenity -ErrorAction SilentlyContinue) -or
                    (Get-Command kdialog -ErrorAction SilentlyContinue)) { $script:OpenKNX_HaveNativePicker = $true }
        }
    } catch { }
    return (OpenKNX_WalkFile -Start $Start -Include $Include -Prompt $Prompt)
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
