param(
    [switch]$SkipTests
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$gadgetbridgeRoot = Join-Path $projectRoot 'external\Gadgetbridge'
$androidSdk = Join-Path $env:LOCALAPPDATA 'Android\Sdk'

if (-not (Test-Path -LiteralPath (Join-Path $gadgetbridgeRoot 'gradlew.bat'))) {
    throw "Gadgetbridge checkout is missing: $gadgetbridgeRoot"
}
if (-not (Test-Path -LiteralPath $androidSdk)) {
    throw "Android SDK is missing: $androidSdk"
}

$javaCandidates = @()
if ($env:JAVA_HOME) { $javaCandidates += $env:JAVA_HOME }
$javaCandidates += Get-ChildItem -LiteralPath 'D:\devtools\temurin21' -Directory `
    -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
$javaCandidates += Get-ChildItem -LiteralPath (Join-Path $projectRoot 'external\toolchains') `
    -Directory -Recurse -ErrorAction SilentlyContinue | Where-Object {
        Test-Path -LiteralPath (Join-Path $_.FullName 'bin\java.exe')
    } | Select-Object -ExpandProperty FullName

$javaHome = $javaCandidates | Where-Object {
    Test-Path -LiteralPath (Join-Path $_ 'bin\java.exe')
} | Select-Object -First 1
if (-not $javaHome) {
    throw 'JDK 21+ is required. Install portable Temurin under D:\devtools\temurin21.'
}

$releaseFile = Join-Path $javaHome 'release'
$javaVersion = Get-Content -LiteralPath $releaseFile -ErrorAction SilentlyContinue |
    Where-Object { $_ -match '^JAVA_VERSION=' } | Select-Object -First 1
if ($javaVersion -notmatch 'JAVA_VERSION="(2[1-9]|[3-9][0-9])') {
    throw "JDK 21+ is required; selected JDK is: $javaHome"
}

$env:JAVA_HOME = $javaHome
$env:ANDROID_HOME = $androidSdk
$env:ANDROID_SDK_ROOT = $androidSdk

Push-Location $gadgetbridgeRoot
try {
    $arguments = @('assembleMainlineDebug', '--no-daemon')
    if (-not $SkipTests) {
        $arguments = @('testMainlineDebugUnitTest', '--tests',
            'nodomain.freeyourgadget.gadgetbridge.service.devices.tsehowatch.TsehoLinkCodecTest') + $arguments
    }
    & .\gradlew.bat @arguments
    if ($LASTEXITCODE -ne 0) { throw "Gadgetbridge build failed ($LASTEXITCODE)." }
} finally {
    Pop-Location
}

$apk = Join-Path $gadgetbridgeRoot 'app\build\outputs\apk\mainline\debug\app-mainline-debug.apk'
if (-not (Test-Path -LiteralPath $apk)) { throw "APK was not created: $apk" }
Write-Host "Gadgetbridge APK: $apk"
