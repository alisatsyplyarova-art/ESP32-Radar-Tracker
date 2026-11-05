# Публикация ESP32 Radar Tracker 1.1.34 в GitHub
# Запускать из PowerShell прямо из распакованной папки прошивки.
# Требуется установленный Git for Windows и доступ к репозиторию через Git Credential Manager/SSH.

$ErrorActionPreference = 'Stop'
$RepoUrl = 'https://github.com/alisatsyplyarova-art/ESP32-Radar-Tracker.git'
$Branch = 'main'
$VersionFolder = 'ESP32_Radar_Tracker_1.1.34'
$RepoTarget = "firmware/$VersionFolder"
$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$Sketch = Join-Path $ScriptRoot 'ESP32_Radar_Tracker_1.1.34.ino'

function Invoke-Git {
    param([string[]]$GitArgs)
    & git @GitArgs
    if ($LASTEXITCODE -ne 0) { throw "Команда git завершилась с ошибкой (код $LASTEXITCODE): git $($GitArgs -join ' ')" }
}

try {
    if (-not (Test-Path $Sketch)) { throw 'Не найден файл ESP32_Radar_Tracker_1.1.34.ino. Запускайте скрипт из исходной папки версии 1.1.34.' }
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'Git не найден. Установите Git for Windows: https://git-scm.com/download/win' }

    $TempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('RadarTracker_1.1.34_' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $TempRoot | Out-Null
    $ClonePath = Join-Path $TempRoot 'ESP32-Radar-Tracker'

    Write-Host 'Клонирую репозиторий и получаю актуальную ветку main...' -ForegroundColor Cyan
    Invoke-Git @('clone', '--branch', $Branch, '--single-branch', $RepoUrl, $ClonePath)

    $Destination = Join-Path $ClonePath ($RepoTarget -replace '/', [IO.Path]::DirectorySeparatorChar)
    if (Test-Path $Destination) { throw "Папка $RepoTarget уже существует в GitHub. Ничего не перезаписано; проверьте публикацию версии." }

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    Get-ChildItem -LiteralPath $ScriptRoot -Force | Where-Object { $_.Name -ne '.git' } | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $Destination -Recurse -Force
    }

    Push-Location $ClonePath
    try {
        Invoke-Git @('add', '--', $RepoTarget)
        & git diff --cached --quiet
        if ($LASTEXITCODE -eq 0) { throw 'Нет изменений для публикации. Коммит не создан.' }
        if ($LASTEXITCODE -gt 1) { throw 'Не удалось проверить подготовленные изменения.' }
        $CommitMessage = 'Publish ESP32 Radar Tracker firmware 1.1.34'
        Invoke-Git @('commit', '-m', $CommitMessage)
        Write-Host 'Отправляю коммит в origin/main...' -ForegroundColor Cyan
        Invoke-Git @('push', 'origin', $Branch)
        Write-Host ''
        Write-Host 'ПУБЛИКАЦИЯ УСПЕШНА: папка версии 1.1.34 отправлена в GitHub.' -ForegroundColor Green
        Write-Host "https://github.com/alisatsyplyarova-art/ESP32-Radar-Tracker/tree/main/$RepoTarget"
    }
    finally { Pop-Location }
}
catch {
    Write-Host "ОШИБКА ПУБЛИКАЦИИ: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host 'Исходники на компьютере не удалялись. Проверьте сообщение выше и повторите запуск.' -ForegroundColor Yellow
    exit 1
}
finally {
    if ($TempRoot -and (Test-Path $TempRoot)) { Remove-Item -LiteralPath $TempRoot -Recurse -Force -ErrorAction SilentlyContinue }
}
