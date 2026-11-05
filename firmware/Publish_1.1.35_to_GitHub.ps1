# Publish ESP32 Radar Tracker 1.1.35 source folder. Keep this script outside the firmware folder.
$ErrorActionPreference = 'Stop'
$RepoRoot = (Get-Location).Path
$FirmwarePath = 'firmware/ESP32_Radar_Tracker_1.1.35'
if (-not (Test-Path (Join-Path $RepoRoot $FirmwarePath))) { throw "Firmware folder not found: $FirmwarePath" }
git -C $RepoRoot add -- $FirmwarePath
git -C $RepoRoot commit -m 'Publish ESP32 Radar Tracker firmware 1.1.35'
if ($LASTEXITCODE -ne 0) { throw 'Git commit failed.' }
git -C $RepoRoot push origin main
if ($LASTEXITCODE -ne 0) { throw 'GitHub push failed. Keep the ZIP backup.' }
Write-Host 'Published firmware 1.1.35.'
