param(
    [string]$ProbeId = "",
    [string]$Frequency = "1m",
    [int]$DebugWords = 40
)

. (Join-Path $PSScriptRoot "pyocd-common.ps1")

if (-not $ProbeId) {
    $ProbeId = Get-WatchProbeId
}

$debugState = Get-WatchSymbolAddress -Symbol "watch_debug_state"
$bleStatus = Get-WatchSymbolAddress -Symbol "watch_ble_status"

Write-Host ("watch_debug_state = {0}" -f (Format-WatchAddress $debugState))
Write-Host ("watch_ble_status   = {0}" -f (Format-WatchAddress $bleStatus))

$commands = @(
    "commander",
    "-u", $ProbeId,
    "-t", $script:DefaultTarget,
    "-f", $Frequency,
    "-M", "attach",
    "-c", ("read32 {0} {1}" -f (Format-WatchAddress $debugState), $DebugWords),
    "-c", ("read32 {0} 8" -f (Format-WatchAddress $bleStatus)),
    "-c", "continue"
)

exit (Invoke-PyOcd -Arguments $commands)
