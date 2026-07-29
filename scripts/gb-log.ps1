param(
    [int]$Lines = 500,
    [string]$Pattern = "Tseho|TsehoWatch|settime|BtLEQueue|DeviceCommunicationService|7a5c000|GATT_SUCCESS"
)

. (Join-Path $PSScriptRoot "pyocd-common.ps1")

$adb = Get-WatchAdb
& $adb devices
Write-Host ""
& $adb shell date
Write-Host ""
& $adb logcat -d -t $Lines |
    Select-String -Pattern $Pattern
exit $LASTEXITCODE
