. (Join-Path $PSScriptRoot "common.ps1")

$server = Get-JLinkGdbServer

Write-Host "SEGGER GDB server: localhost:2331 (Ctrl+C to stop)"
& $server -noGui -device NRF52840_XXAA -if SWD -speed $JLinkSpeedKhz `
    -endian little -port 2331 -swoport 2332 -telnetport 2333
exit $LASTEXITCODE
