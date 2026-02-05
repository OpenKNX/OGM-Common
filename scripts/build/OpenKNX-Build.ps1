
param (
  [Parameter(Mandatory = $false, HelpMessage="Initiate debug build, -DebugBuild should be first odr last parameter")]
  [switch]$DebugBuild,

  [Parameter(Mandatory = $true)]
  [ValidateNotNullOrEmpty()]
  [string]$env,

  [Parameter(Mandatory = $false)]
  [string]$target
)

# pre build
if (Test-Path -Path scripts/OpenKNX-Pre-Build.ps1) {
    scripts/OpenKNX-Pre-Build.ps1
}
if (!$?) { exit 1 }

$buildMode = if ($DebugBuild) { "debug"} else { "run" }
if ($target) {
    if ($IsMacOS -or $IsLinux) { ~/.platformio/penv/bin/pio $buildMode -e $env --target $target } 
    else { ~/.platformio/penv/Scripts/pio.exe $buildMode -e $env --target $target }
} else {
    if ($IsMacOS -or $IsLinux) { ~/.platformio/penv/bin/pio $buildMode -e $env }
    else { ~/.platformio/penv/Scripts/pio.exe $buildMode -e $env } 
}

# post build
if (!$?) { exit 1 }

if (Test-Path -Path scripts/OpenKNX-Post-Build.ps1) {
    scripts/OpenKNX-Post-Build.ps1
}
