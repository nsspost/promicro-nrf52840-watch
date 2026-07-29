$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$apk = Join-Path $projectRoot 'external\Gadgetbridge\app\build\outputs\apk\mainline\debug\app-mainline-debug.apk'
$adb = Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'

if (-not (Test-Path -LiteralPath $apk)) { throw "APK is missing: $apk" }
if (-not (Test-Path -LiteralPath $adb)) { throw "adb is missing: $adb" }
& $adb install -r $apk
if ($LASTEXITCODE -ne 0) { throw "APK installation failed ($LASTEXITCODE)." }
