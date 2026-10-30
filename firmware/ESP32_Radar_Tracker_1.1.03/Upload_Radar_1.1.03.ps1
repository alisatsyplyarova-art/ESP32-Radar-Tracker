
# ESP32 Radar Tracker 1.1.03
# Complete GitHub uploader

$ErrorActionPreference = 'Stop'

$Repo = 'alisatsyplyarova-art/ESP32-Radar-Tracker'
$Branch = 'main'
$Target = 'firmware/ESP32_Radar_Tracker_1.1.03'

# Folder containing this uploader and the firmware subfolder
$Source = $PSScriptRoot

$Work = Join-Path $env:TEMP 'Radar_GitHub_Upload'
$RepoDir = Join-Path $Work 'repo'
$TargetDir = Join-Path $RepoDir ($Target -replace '/', '\')

try {
    Write-Host 'ESP32 Radar Tracker 1.1.03 upload' -ForegroundColor Cyan

    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw 'Git is not installed.'
    }

    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
        throw 'GitHub CLI is not installed.'
    }

    # Find firmware recursively
    $Firmware = Get-ChildItem -LiteralPath $Source `
        -Filter 'ESP32_Radar_Tracker_1.1.03.ino' `
        -Recurse -File | Select-Object -First 1

    if (-not $Firmware) {
        throw 'Firmware INO file was not found.'
    }

    Write-Host "Firmware found: $($Firmware.FullName)" -ForegroundColor Green

    # Check GitHub authentication
    & gh auth status
    if ($LASTEXITCODE -ne 0) {
        throw 'Run gh auth login first.'
    }

    # Prepare temporary workspace
    if (Test-Path $RepoDir) {
        Remove-Item -LiteralPath $RepoDir -Recurse -Force
    }

    New-Item -ItemType Directory -Path $Work -Force | Out-Null

    # Clone repository
    Write-Host 'Cloning repository...' -ForegroundColor Yellow

    & git clone --branch $Branch "https://github.com/$Repo.git" $RepoDir

    if ($LASTEXITCODE -ne 0) {
        throw 'Git clone failed.'
    }

    # Create destination
    New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null

    # Copy the complete package, including this script
    Write-Host 'Copying complete firmware package...' -ForegroundColor Yellow

    & robocopy $Source $TargetDir /E /R:2 /W:2 /XD '.git' /XF 'desktop.ini'

    $RoboCode = $LASTEXITCODE

    if ($RoboCode -ge 8) {
        throw "Robocopy failed with code $RoboCode."
    }

    # Commit and push
    Push-Location $RepoDir

    try {
        & git add --all -- $Target

        if ($LASTEXITCODE -ne 0) {
            throw 'Git add failed.'
        }

        & git diff --cached --quiet
        $DiffCode = $LASTEXITCODE

        if ($DiffCode -eq 0) {
            Write-Host 'No changes to upload.' -ForegroundColor Yellow
        }
        elseif ($DiffCode -eq 1) {
            & git -c user.name="ESP32 Radar Tracker uploader" `
                -c user.email="uploader@users.noreply.github.com" `
                commit -m "Update ESP32 Radar Tracker 1.1.03"

            if ($LASTEXITCODE -ne 0) {
                throw 'Git commit failed.'
            }

            Write-Host 'Uploading to GitHub...' -ForegroundColor Yellow

            & git push origin $Branch

            if ($LASTEXITCODE -ne 0) {
                throw 'Git push failed.'
            }
        }
        else {
            throw 'Git diff check failed.'
        }
    }
    finally {
        Pop-Location
    }

    $GitHubUrl = "https://github.com/$Repo/tree/$Branch/$Target"

    Write-Host ''
    Write-Host 'UPLOAD COMPLETED!' -ForegroundColor Green
    Write-Host $GitHubUrl -ForegroundColor Green
}
catch {
    Write-Host ''
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Read-Host 'Press Enter to exit' | Out-Null
}
