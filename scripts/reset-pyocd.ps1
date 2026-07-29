param(
    [string]$ProbeId = "",
    [string]$Frequency = "1m"
)

. (Join-Path $PSScriptRoot "pyocd-common.ps1")

if (-not $ProbeId) {
    $ProbeId = Get-WatchProbeId
}

$resetArgs = @(
    "reset",
    "-u", $ProbeId,
    "-t", $script:DefaultTarget,
    "-f", $Frequency,
    "-m", "sysresetreq"
)
exit (Invoke-PyOcd -Arguments $resetArgs)
