#!/usr/bin/env pwsh
<#
Open ■
┬────┴  Test-PowerShell51-Compat
■ KNX   2026 OpenKNX - Erkan Çolak

FILEPATH: OGM-Common/scripts/setup/reusable/Test-PowerShell51-Compat.ps1

.SYNOPSIS
    Checks OpenKNX PowerShell scripts for constructs that Windows PowerShell 5.1 cannot run, and for
    encoding that the Windows console cannot render.

.DESCRIPTION
    Release scripts have to run unchanged on a plain Windows 10 with Windows PowerShell 5.1. That
    version has no ternary operator, no ?? / ??=, no && / || between commands, and a number of cmdlet
    parameters that arrived with PowerShell 6 or 7. A script using one of them fails at PARSE time, so
    it does not fail halfway -- it fails before the first line runs, on the user's machine, with a
    syntax error.

    The syntax checks walk the abstract syntax tree rather than matching text: the PowerShell 7 parser
    reports a ternary as a TernaryExpressionAst and a && chain as a PipelineChainAst, so they are found
    exactly, without false hits inside strings, comments or regular expressions.

    The encoding check reads bytes: a file holding umlauts or box-drawing glyphs needs a UTF-8 BOM, or
    Windows PowerShell 5.1 reads it in the ANSI code page and prints "Ger?t".

.PARAMETER Path
    Files or folders to check. Folders are searched recursively for *.ps1. Default: the current folder.

.PARAMETER Quiet
    Print only findings and the summary line.

.EXAMPLE
    ./Test-PowerShell51-Compat.ps1 ./data
    # checks every script in the data folder

.EXAMPLE
    ./Test-PowerShell51-Compat.ps1 ./data/KNX-Upload-Firmware-Generic.ps1
    # checks one file

.NOTES
    AUTHOR : Erkan Çolak
    Exit code 0 = clean, 1 = at least one finding.

.LINK
    https://wiki.openknx.de
#>

param(
    [Parameter(Position = 0, ValueFromRemainingArguments = $true)]
    [string[]]$Path = @("."),
    [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function OpenKNX_ShowLogo($AddCustomText = $null) {
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

# Cmdlet parameters that do not exist in Windows PowerShell 5.1. Each entry is command -> parameters.
# Kept deliberately short: only the ones that actually turn up in build and upload scripts.
$script:PS6Params = @{
    'join-path'        = @('AdditionalChildPath')
    'get-content'      = @('AsByteStream')
    'set-content'      = @('AsByteStream')
    'convertfrom-json' = @('AsHashtable', 'NoEnumerate')
    'split-path'       = @('LeafBase')
    'foreach-object'   = @('Parallel', 'ThrottleLimit')
    'test-connection'  = @('TargetName', 'MtuSize')
    'invoke-restmethod'= @('SkipHttpErrorCheck', 'Resume')
    'invoke-webrequest'= @('SkipHttpErrorCheck', 'Resume')
    'start-process'    = @('WhatIf')
    'new-item'         = @('AsPlainText')
    'out-file'         = @('AsByteStream')
    'export-csv'       = @('UseQuotes', 'QuoteFields')
    'import-csv'       = @('UseQuotes')
    'sort-object'      = @('Stable', 'Bottom', 'Top')
    'select-string'    = @('NoEmphasis', 'Raw')
    'get-error'        = @()
}
# Commands that do not exist at all before PowerShell 6/7.
$script:PS6Commands = @('Get-Error', 'Test-Json', 'Join-String', 'ConvertFrom-Markdown', 'Get-Uptime',
                        'Remove-Alias', 'Set-Clipboard-Core', 'Invoke-DscResource-Core')

function Test-OneFile {
    param([string]$File)
    $findings = @()

    # --- encoding -------------------------------------------------------------------------------
    $bytes = [System.IO.File]::ReadAllBytes($File)
    $hasBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
    $nonAscii = $false
    foreach ($b in $bytes) { if ($b -gt 0x7F) { $nonAscii = $true; break } }
    if ($nonAscii -and -not $hasBom) {
        $findings += @{ Line = 1; Kind = 'encoding'
                        Text = 'non-ASCII characters without a UTF-8 BOM - Windows PowerShell 5.1 reads this in the ANSI code page' }
    }

    # --- syntax ---------------------------------------------------------------------------------
    $tokens = $null
    $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile($File, [ref]$tokens, [ref]$errors)
    if ($errors -and $errors.Count -gt 0) {
        foreach ($e in $errors) {
            $findings += @{ Line = $e.Extent.StartLineNumber; Kind = 'parse'; Text = $e.Message }
        }
        return $findings
    }

    # A ternary is a node of its own, so it is found exactly -- no regex can tell "$a ? $b : $c" from a
    # question mark inside a regular expression.
    $ternary = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.TernaryExpressionAst] }, $true)
    foreach ($n in $ternary) {
        $findings += @{ Line = $n.Extent.StartLineNumber; Kind = 'ternary'
                        Text = 'ternary operator ? : - use if/else, 5.1 has no ternary' }
    }
    $chain = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.PipelineChainAst] }, $true)
    foreach ($n in $chain) {
        $findings += @{ Line = $n.Extent.StartLineNumber; Kind = 'chain'
                        Text = '&& or || between commands - use if ($LASTEXITCODE -eq 0)' }
    }
    $binary = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.BinaryExpressionAst] }, $true)
    foreach ($n in $binary) {
        if ("$($n.Operator)" -eq 'QuestionQuestion') {
            $findings += @{ Line = $n.Extent.StartLineNumber; Kind = 'coalesce'
                            Text = 'null-coalescing ?? - use if (-not $x)' }
        }
    }
    $assign = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.AssignmentStatementAst] }, $true)
    foreach ($n in $assign) {
        if ("$($n.Operator)" -eq 'QuestionQuestionEquals') {
            $findings += @{ Line = $n.Extent.StartLineNumber; Kind = 'coalesce'
                            Text = 'null-coalescing assignment ??= - use if (-not $x) { $x = ... }' }
        }
    }
    # $x?.y / $x?[0] -- the null-conditional member access, PowerShell 7 only.
    foreach ($t in $tokens) {
        if ($t.Kind -eq 'QuestionDot' -or $t.Kind -eq 'QuestionLBracket') {
            $findings += @{ Line = $t.Extent.StartLineNumber; Kind = 'nullcond'
                            Text = 'null-conditional ?. or ?[ - guard with if instead' }
        }
    }

    # --- cmdlets and parameters ------------------------------------------------------------------
    $cmds = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.CommandAst] }, $true)
    foreach ($c in $cmds) {
        $name = $c.GetCommandName()
        if (-not $name) { continue }
        $lower = $name.ToLower()
        if ($script:PS6Commands -contains $name) {
            $findings += @{ Line = $c.Extent.StartLineNumber; Kind = 'cmdlet'
                            Text = "$name does not exist in Windows PowerShell 5.1" }
        }
        if (-not $script:PS6Params.ContainsKey($lower)) { continue }
        foreach ($el in $c.CommandElements) {
            if (-not ($el -is [System.Management.Automation.Language.CommandParameterAst])) { continue }
            foreach ($bad in $script:PS6Params[$lower]) {
                if ($el.ParameterName -like "$bad*") {
                    $findings += @{ Line = $el.Extent.StartLineNumber; Kind = 'param'
                                    Text = "$name -$($el.ParameterName) needs PowerShell 6 or newer" }
                }
            }
        }
        # Join-Path with three or more path arguments is the 7-only form even without the named parameter.
        if ($lower -eq 'join-path') {
            $positional = @($c.CommandElements | Where-Object {
                    -not ($_ -is [System.Management.Automation.Language.CommandParameterAst]) })
            if ($positional.Count -ge 4) {
                $findings += @{ Line = $c.Extent.StartLineNumber; Kind = 'param'
                                Text = 'Join-Path with three path parts - 5.1 takes only two' }
            }
        }
    }

    # --- automatic variables that 5.1 does not define ---------------------------------------------
    # Reading $IsWindows on 5.1 yields $null, so a plain "if ($IsWindows)" silently takes the wrong
    # branch. Only flag a READ that is not inside a Get-Variable guard and not an assignment.
    $guarded = (Get-Content -Raw -LiteralPath $File) -match "Get-Variable\s+-Name\s+'(IsWindows|IsMacOS|IsLinux)'"
    if (-not $guarded) {
        $vars = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.VariableExpressionAst] }, $true)
        $seen = @{}
        foreach ($v in $vars) {
            $vn = $v.VariablePath.UserPath
            if ($vn -notin @('IsWindows', 'IsMacOS', 'IsLinux', 'IsCoreCLR')) { continue }
            if ($seen.ContainsKey($vn)) { continue }
            $seen[$vn] = $true
            $findings += @{ Line = $v.Extent.StartLineNumber; Kind = 'autovar'
                            Text = "`$$vn is not defined in 5.1 - define it first, or read it via Get-Variable" }
        }
    }

    return $findings
}

# ─── run ──────────────────────────────────────────────────────────────────────────────────────────
if (-not $Quiet) { OpenKNX_ShowLogo "PowerShell 5.1 compatibility check" }

$files = @()
foreach ($p in $Path) {
    if (Test-Path -PathType Container $p) {
        $files += (Get-ChildItem -Path $p -Filter *.ps1 -File -Recurse | ForEach-Object { $_.FullName })
    }
    elseif (Test-Path -PathType Leaf $p) { $files += (Resolve-Path $p).Path }
    else { Write-Host "  not found: $p" -ForegroundColor Red }
}
$files = @($files | Sort-Object -Unique)

$total = 0
foreach ($f in $files) {
    $found = @(Test-OneFile -File $f)
    if ($found.Count -eq 0) {
        if (-not $Quiet) { Write-Host ("  ok   " + (Split-Path -Leaf $f)) -ForegroundColor DarkGray }
        continue
    }
    $total += $found.Count
    Write-Host ("  " + (Split-Path -Leaf $f)) -ForegroundColor Yellow
    foreach ($x in ($found | Sort-Object { $_.Line })) {
        Write-Host ("      {0,5}  {1,-9} {2}" -f $x.Line, $x.Kind, $x.Text) -ForegroundColor Red
    }
}

Write-Host ""
if ($total -eq 0) {
    Write-Host ("  {0} file(s) checked, nothing found." -f $files.Count) -ForegroundColor Green
    exit 0
}
Write-Host ("  {0} file(s) checked, {1} finding(s)." -f $files.Count, $total) -ForegroundColor Red
exit 1
