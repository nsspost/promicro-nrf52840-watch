. (Join-Path $PSScriptRoot "common.ps1")

Write-Host "Reading watch_debug_state twice; heartbeat is the third word."
$result = Invoke-JLinkCommandFile -Commands @(
    "r",
    "g",
    "sleep 1000",
    "h",
    "mem32 0x20000000 4",
    "g",
    "sleep 1000",
    "h",
    "mem32 0x20000000 4",
    "g",
    "q"
)
exit $result
