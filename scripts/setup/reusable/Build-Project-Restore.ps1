Write-Output "Build project restore"
$restoreExist = Test-Path -PathType Container "restore"
if (!$restoreExist) {
    New-Item -Name "restore" -ItemType "directory" | Out-Null
}
Copy-Item "lib/OGM-Common/scripts/restore/*" "restore/"
if (!$?) {
    return 1
}
Write-Output "... Newest project restore is now available"

