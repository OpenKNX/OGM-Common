# This script is responsible for all common tasks before a release build is executed

$moduleName = 'Microsoft.PowerShell.Archive'
$minVersion = [version]'1.2.3.0'

$module = Get-Module -ListAvailable -Name $moduleName |
           Sort-Object Version -Descending |
           Select-Object -First 1

if ($null -eq $module -or $module.Version -lt $minVersion) {
    Write-Host ""
    Write-Host "❌ The module '$moduleName' is missing or outdated." -ForegroundColor Red
    if ($module) {
        Write-Host "   Found version: $($module.Version)"
    } else {
        Write-Host "   Module not found."
    }
    Write-Host "   Required minimum version: $minVersion"
    Write-Host ""
    Write-Host "➡️  You can install or update (as admin) the module using:"
    Write-Host "Install-Module $moduleName -MinimumVersion $minVersion -Repository PSGallery -Force -AllowClobber"
    Write-Host ""
    timeout /T 20
    exit 1
}

# get all definitions for this project
$settings = scripts/OpenKNX-Build-Settings.ps1 $args[0] $args[1] $args[2] $args[3]

# Output current setting
Write-Host "Provided OpenKNX-Build-Settings:"
$settings
Write-Host "--------------------------------"

# check and cleanup working dir
if (Test-Path -Path release) {
  # clean working dir
  Remove-Item -Recurse -Force release\*
}
else {
  New-Item -Path release -ItemType Directory | Out-Null
}

# create required directories
Copy-Item -Recurse lib/OGM-Common/scripts/setup/reusable/data release

if($settings.compileWith -eq "openknxproducer")
{
  # DEVELOPER side: this runs on the developer's machine during the release build and turns the
  # module XML templates (src/*.share.xml etc.) into the full XML, the .knxprod and include/knxprod.h
  # (producer command "create"). Requirements of new template features (new ETS elements, op:
  # directives, ...) belong HERE - deliberately only an existence check, no version check.
  #
  # Do not confuse with the END-USER side in Build-knxprod.ps1, which ships inside the release
  # package and builds a .knxprod from the already preprocessed XML (command "knxprod"). Its
  # $checkVersion forces every user to update their tools, so bump it only when the shipped XML
  # or the "knxprod" command itself actually needs a newer producer.
  # Windows PowerShell 5.1 defines neither $IsMacOS nor $IsLinux; reading them there yields $null, so
  # the Windows branch is taken for the right reason rather than by accident (survives Set-StrictMode).
  if ($null -eq (Get-Variable -Name 'IsMacOS' -ErrorAction SilentlyContinue)) {
    $IsMacOS = $false; $IsLinux = $false; $IsWindows = $true
  }

  # OPENKNX_SKIP_KNXPROD=1 (set by an OAM's Build-Release.ps1 -SkipKnxprod): no producer run. include/knxprod.h
  # and the ETS data below stay from the last run; what gets packed is reported instead of taken silently.
  $skipKnxprod = ($env:OPENKNX_SKIP_KNXPROD -eq "1")

  # Hardware profile (OGM-HardwareConfig HW-Profiler): an OAM with HardwareProfile.xml (project folder, else src/) gets its
  # generated ETS parts and headers before the producer reads them; a lib without the generator stops here instead of failing
  # later in the producer. One manifest serves the Dev and the Release product XML.
  $hwManifests = @(@("HardwareProfile.xml", "src/HardwareProfile.xml") | Where-Object { Test-Path -Path $_ -PathType Leaf })
  if ($hwManifests.Count -gt 1) {
    Write-Host "Two hardware manifests: HardwareProfile.xml and src/HardwareProfile.xml - keep one (its .slots file belongs next to it)" -ForegroundColor Red
    exit 1
  }
  $hwGenerator = "lib/OGM-HardwareConfig/generator/Build-HardwareProfile.ps1"
  if (($hwManifests.Count -eq 1) -and -not $skipKnxprod) {
    $hwManifest = $hwManifests[0]
    if (-not (Test-Path -Path $hwGenerator -PathType Leaf)) {
      Write-Host "$hwManifest found, but $hwGenerator is missing: lib/OGM-HardwareConfig needs a version with the HW-Profiler (generator/)" -ForegroundColor Red
      exit 1
    }
    # extra parameters from the OAM's Build-Release.ps1: $global:OpenKNXHwProfilerArgs = @{ Detailed = $true } (splatted; the
    # OAM removes the variable again in a finally, as with OPENKNX_SKIP_KNXPROD); any dictionary, also [ordered]
    $hwArgs = @{}
    if ($global:OpenKNXHwProfilerArgs -is [System.Collections.IDictionary]) {
      foreach ($k in $global:OpenKNXHwProfilerArgs.Keys) { $hwArgs["$k"] = $global:OpenKNXHwProfilerArgs[$k] }
    }
    elseif ($null -ne $global:OpenKNXHwProfilerArgs) {
      Write-Host "OpenKNXHwProfilerArgs must be a hashtable, e.g. @{ Detailed = `$true }" -ForegroundColor Red
      exit 1
    }
    if ($hwArgs.ContainsKey("Manifest")) {
      Write-Host "OpenKNXHwProfilerArgs must not set Manifest: the build takes HardwareProfile.xml (project folder, else src/)" -ForegroundColor Red
      exit 1
    }
    Write-Host "Generating hardware files ($hwManifest)..." -ForegroundColor Cyan
    $global:LASTEXITCODE = 0
    & $hwGenerator -Manifest $hwManifest @hwArgs
    if ($global:LASTEXITCODE -ne 0) {
      Write-Host "Hardware generation ($hwGenerator) failed with exit code $global:LASTEXITCODE" -ForegroundColor Red
      exit 1
    }
  }

  # check for existance of OpenKNXProducer
  $OpenKNXproducer = "~/bin/OpenKNXproducer.exe"

  if ($IsMacOS -or $IsLinux) {
    $OpenKNXproducer = "/usr/local/bin/OpenKNXproducer"
  }

  if ($skipKnxprod) {
    $OpenKNXproducer = $null
    Write-Host "Skipping knxprod creation (SkipKnxprod): include/knxprod.h and the ETS data stay from the last OpenKNXproducer run" -ForegroundColor Yellow
  }
  elseif (Test-Path $OpenKNXproducer -PathType Leaf) {
    Write-Host "OpenKNXproducer found at $OpenKNXproducer"
  }
  else {
    $OpenKNXproducer = $null
    Write-Host "OpenKNXproducer not found at $OpenKNXproducer"
    Write-Host -ForegroundColor Yellow "
    OpenKNX-Tools are not Installed. Please install OpenKNX-Tools to Build KNX production file. 
    For more Informations visit: https://github.com/OpenKNX/OpenKNX/wiki/Installation-of-OpenKNX-toolscl
    
    "
    Start-Sleep -Seconds 2
  }

  if (-not ([string]::IsNullOrEmpty($OpenKNXproducer))) {
    $expr = "$OpenKNXproducer create --Debug --Output=""release/$($settings.releaseName).knxprod"" --HeaderFileName=""include/knxprod.h"" ""src/$($settings.releaseName).xml"""
    $expr += '; $success=$?'
    Invoke-Expression $expr
    if (!$success) {
        exit 1
    }
    Write-Host "Created release/$($settings.releaseName).knxprod" -ForegroundColor Blue
  }
  elseif (-not $skipKnxprod) {
    Write-Host "OpenKNXproducer is not Installed. Skipping knxprod file creation." -ForegroundColor Yellow
  }
  if ($skipKnxprod) {
    $etsXml = "src/$($settings.releaseName).debug.xml"
    $etsBag = "src/$($settings.releaseName).baggages"
    if (Test-Path -Path $etsXml) { Write-Host "  ETS XML packed from the last run: $etsXml ($((Get-Item $etsXml).LastWriteTime))" -ForegroundColor Yellow }
    else { Write-Host "  no $etsXml - the package has no ETS application" -ForegroundColor Red }
    if (-not (Test-Path -Path $etsBag)) { Write-Host "  no $etsBag - none in this package (a producer run moves them into its own package)" -ForegroundColor Yellow }
  }
  if (Test-Path -Path "src/$($settings.releaseName).debug.xml") {
    Copy-Item "src/$($settings.releaseName).debug.xml" "release/data/$($settings.targetName).xml"
  }
  if (Test-Path -Path "src/$($settings.releaseName).baggages") {
    Move-Item "src/$($settings.releaseName).baggages" "release/data/$($settings.targetName).baggages"
  }
}
elseif($settings.compileWith -eq "kaenxcreator")
{
  Write-Host "Using Kaenx-Creator!"
  $KaenxCreator = "~/bin/Kaenx.Creator.Console.exe"

  if ($IsMacOS -or $IsLinux) {
    $KaenxCreator = "/usr/local/bin/Kaenx.Creator.Console"
  }

  if (Test-Path $KaenxCreator -PathType Leaf) {
    Write-Host "KaenxCreatorConsole found at $KaenxCreator"
  }
  else {
    $KaenxCreator = $null
    Write-Host "KaenxCreator not found at $KaenxCreator"
    Write-Host -ForegroundColor Red "
    KaenxCreator not Installed. Please install KaenxCreator to Build KNX production file. 
    You can find the latest release here: https://github.com/OpenKNX/Kaenx-Creator-Console/releases/"
    Start-Sleep -Seconds 2
    exit 1
  }
  if (-not ([string]::IsNullOrEmpty($KaenxCreator))) {
    $currentDirectory = (Get-Location).Path
    $expr = "$KaenxCreator release $($currentDirectory)\$($settings.targetName).ae-manu"
    $expr += '; $success=$?'
    Invoke-Expression $expr
    if (!$success) {
        exit 1
    }
    Write-Host "Created release/data/$($settings.targetName).xml" -ForegroundColor Blue
  }
}
else {
  Write-Host "No KNX-Tool selected. Skipping knxprod file creation." -ForegroundColor Yellow
}




# write content.xml header
$releaseTarget = "release/data/content.xml"
if (![string]::IsNullOrEmpty($ProjectDir)) {
  $releaseTarget = Join-Path $ProjectDir $releaseTarget
}

# Create the directory structure if it doesn't exist
$directory = [System.IO.Path]::GetDirectoryName($releaseTarget)
if (-not (Test-Path -Path $directory)) {
  New-Item -ItemType Directory -Path $directory -Force
}
if (-not (Test-Path -Path $releaseTarget -PathType Leaf)) {
  New-Item -Path $releaseTarget -ItemType File | Out-Null
} 

if (Test-Path -Path $releaseTarget -PathType Leaf) {
  # write content.xml header
  Add-Content -Path $releaseTarget "<?xml version=""1.0"" encoding=""UTF-8""?>"
  Add-Content -Path $releaseTarget "<Content>"
  Add-Content -Path $releaseTarget "    <ETSapp Name=""$($settings.targetName)"" XmlFile=""$($settings.targetName).xml"" />"
  Add-Content -Path $releaseTarget "    <Products>"
  Write-Host "Created $releaseTarget" -ForegroundColor Blue 
}
else {
  Write-Host "Pre - ERROR: $releaseTarget could not be found!" -ForegroundColor Red
  exit 1
}

# --- module release hooks ------------------------------------------------------------------------
# Counterpart of the Post.ps1 hooks in Build-Release-Postprocess.ps1, same contract, run before the
# firmware build. Present so the convention is symmetric: a module shipping scripts/release/Pre.ps1
# must not be silently ignored. Found by convention -- NO module is named here.
$releaseRoot = (Resolve-Path "release" -ErrorAction SilentlyContinue).Path
if ($releaseRoot) {
  foreach ($hook in @(Get-ChildItem -Path "lib/*/scripts/release/Pre.ps1" -ErrorAction SilentlyContinue | Sort-Object FullName)) {
    $moduleName = Split-Path -Leaf (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $hook.FullName)))
    Write-Host "Release hook (pre): $moduleName" -ForegroundColor Blue
    try   { & $hook.FullName -ReleaseRoot $releaseRoot -BuildParam $args[0] }
    catch { Write-Host "  hook $moduleName failed: $($_.Exception.Message) -- continuing" -ForegroundColor DarkYellow }
  }
}

exit 0
