. (Join-Path $PSScriptRoot "common.ps1")

$image = Join-Path $ProjectRoot "build/watch_firmware.hex"
$backupDir = Join-Path $ProjectRoot "backups"
$flashBackup = Join-Path $backupDir "flash-before-first-write.bin"
$uicrBackup = Join-Path $backupDir "uicr-before-first-write.bin"

if (-not (Test-Path $image)) {
    throw "Firmware is not built. Run scripts/build.ps1 first."
}

if (-not (Test-Path $flashBackup)) {
    New-Item -ItemType Directory -Path $backupDir -Force | Out-Null
    Write-Host "Saving one-time backup of Flash and UICR..."
    $backupResult = Invoke-JLinkCommandFile -Commands @(
        "r",
        "h",
        "savebin $flashBackup, 0x00000000, 0x00100000",
        "savebin $uicrBackup, 0x10001000, 0x00000100",
        "q"
    )
    if ($backupResult -ne 0) {
        throw "Backup failed; refusing to write Flash."
    }
}

$result = Invoke-JLinkCommandFile -Commands @(
    "r",
    "h",
    "loadfile $image",
    "r",
    "g",
    "q"
)
exit $result
