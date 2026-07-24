. (Join-Path $PSScriptRoot "common.ps1")

$sdkVersion = "nRF5_SDK_17.1.0_ddde560"
$sdkDirectory = Join-Path $ProjectRoot "external/$sdkVersion"
$archive = Join-Path $ProjectRoot "external/$sdkVersion.zip"
$downloadUrl =
    "https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v17.x.x/$sdkVersion.zip"
$expectedLength = 131838843

if (Test-Path (Join-Path $sdkDirectory `
        "components/softdevice/s140/headers/ble.h")) {
    Write-Host "nRF5 SDK 17.1.0 is already installed."
    exit 0
}

New-Item -ItemType Directory -Path (Split-Path $archive) -Force |
    Out-Null
Write-Host "Downloading the official Nordic nRF5 SDK 17.1.0..."
Invoke-WebRequest -Uri $downloadUrl -OutFile $archive

if ((Get-Item -LiteralPath $archive).Length -ne $expectedLength) {
    throw "Downloaded SDK archive has an unexpected size."
}

Expand-Archive -LiteralPath $archive `
    -DestinationPath (Split-Path $sdkDirectory) -Force
if (-not (Test-Path (Join-Path $sdkDirectory `
        "components/softdevice/s140/headers/ble.h"))) {
    throw "The extracted SDK is incomplete."
}
Write-Host "Installed nRF5 SDK 17.1.0 in external/."
