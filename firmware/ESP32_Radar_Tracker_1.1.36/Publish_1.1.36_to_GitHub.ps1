# Publish ESP32 Radar Tracker 1.1.36 to GitHub without overwriting older releases.
$ErrorActionPreference = 'Stop'
$repoUrl = 'https://github.com/alisatsyplyarova-art/ESP32-Radar-Tracker.git'
$repoDir = Join-Path $env:TEMP 'ESP32-Radar-Tracker-publish'
$versionName = 'ESP32_Radar_Tracker_1.1.36'
$relativePath = "firmware/$versionName"
$sourceDir = $PSScriptRoot
$scriptName = 'Publish_1.1.36_to_GitHub.ps1'
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'Git не найден. Установите Git for Windows.' }
if (-not (Test-Path (Join-Path $repoDir '.git'))) { if (Test-Path $repoDir) { Remove-Item $repoDir -Recurse -Force }; git clone $repoUrl $repoDir; if ($LASTEXITCODE -ne 0) { throw 'Не удалось клонировать репозиторий.' } }
git -C $repoDir checkout main
if ($LASTEXITCODE -ne 0) { throw 'Не удалось переключиться на main.' }
git -C $repoDir pull --ff-only origin main
if ($LASTEXITCODE -ne 0) { throw 'Не удалось обновить main.' }
$targetDir = Join-Path $repoDir $relativePath
if (Test-Path $targetDir) { throw "Папка $relativePath уже существует. Старую версию скрипт не перезаписывает." }
New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
Get-ChildItem -LiteralPath $sourceDir -Force | Where-Object { $_.Name -ne $scriptName } | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $targetDir -Recurse -Force }
git -C $repoDir add -- $relativePath
git -C $repoDir commit -m 'Publish ESP32 Radar Tracker firmware 1.1.36'
if ($LASTEXITCODE -ne 0) { throw 'Не удалось создать commit. Проверьте git user.name / git user.email.' }
git -C $repoDir push origin main
if ($LASTEXITCODE -ne 0) { throw 'Отправка не удалась. Проверьте авторизацию GitHub.' }
Write-Host "Опубликовано: https://github.com/alisatsyplyarova-art/ESP32-Radar-Tracker/tree/main/$relativePath" -ForegroundColor Green
