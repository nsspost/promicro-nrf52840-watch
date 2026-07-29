. (Join-Path $PSScriptRoot "common.ps1")

$nm = Find-XpackExecutable `
    -PackageName "arm-none-eabi-gcc" `
    -Executable "arm-none-eabi-nm.exe"
$elf = Join-Path $ProjectRoot "build/watch_firmware.elf"
$gnoArchive = Join-Path $ProjectRoot "build/external/NOG_C/libgno.a"

if (-not (Test-Path -LiteralPath $elf)) {
    throw "Firmware ELF is absent. Run npm run build first."
}

$firmwareSymbols = & $nm $elf
$wideDivision = $firmwareSymbols | Where-Object {
    $_ -match '\b(__aeabi_uldivmod|__aeabi_ldivmod|__divdi3|__udivdi3)\b'
}

if ($wideDivision) {
    throw "64-bit division runtime entered the firmware:`n$($wideDivision -join "`n")"
}
Write-Host "Embedded dependency check passed: no 64-bit division. newlib-nano memory routines are allowed."
