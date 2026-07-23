. (Join-Path $PSScriptRoot "common.ps1")

$result = Invoke-JLinkCommandFile -Commands @(
    "r",
    "h",
    "mem32 0x10000100 4",
    "q"
)
exit $result
