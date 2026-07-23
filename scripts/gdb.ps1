. (Join-Path $PSScriptRoot "common.ps1")

$gdb = Get-Gdb
$elf = Join-Path $ProjectRoot "build/watch_firmware.elf"

if (-not (Test-Path $elf)) {
    throw "Firmware is not built. Run scripts/build.ps1 first."
}

& $gdb $elf `
    -ex "target extended-remote localhost:2331" `
    -ex "monitor reset halt" `
    -ex "break main"
exit $LASTEXITCODE
