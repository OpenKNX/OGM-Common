
$firmwareName = $args[0]


$currentDir = (Get-Item .).FullName
$firmwareWithPath = Join-Path $currentDir "data"
$firmwareWithPath = Join-Path $firmwareWithPath $firmwareName


Write-Host "Executing python -m esptool write_flash 0x0 $firmwareWithPath"

python -m esptool write_flash 0x0 $firmwareWithPath



Write-Host "Done" -ForegroundColor Green
Write-Host 
timeout /T 60     