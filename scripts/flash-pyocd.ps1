param(
    [string]$ProbeId = "",
    [string]$Frequency = "1m",
    [switch]$TrustCrc
)

. (Join-Path $PSScriptRoot "pyocd-common.ps1")

if (-not $ProbeId) {
    $ProbeId = Get-WatchProbeId
}

$image = Get-WatchFirmwareHex
$softDevice = Get-WatchSoftDeviceHex

if (-not (Test-Path -LiteralPath $image)) {
    throw "Firmware HEX is absent. Run npm run build first."
}
if (-not (Test-Path -LiteralPath $softDevice)) {
    throw "S140 SoftDevice is absent. Run npm run install:nrf5-sdk first."
}

$loadArgs = @(
    "load",
    "-u", $ProbeId,
    "-t", $script:DefaultTarget,
    "-f", $Frequency,
    "-M", "halt",
    "-e", "sector",
    "--no-reset"
)
if ($TrustCrc) {
    $loadArgs += "--trust-crc"
}
$loadArgs += @($softDevice, $image)

$result = Invoke-PyOcd -Arguments $loadArgs
if ($result -ne 0) {
    exit $result
}

$resetArgs = @(
    "reset",
    "-u", $ProbeId,
    "-t", $script:DefaultTarget,
    "-f", $Frequency,
    "-m", "sysresetreq"
)
exit (Invoke-PyOcd -Arguments $resetArgs)
