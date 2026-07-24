. (Join-Path $PSScriptRoot "common.ps1")

$elf = Join-Path $ProjectRoot "build/watch_firmware.elf"
if (-not (Test-Path -LiteralPath $elf)) {
    throw "Firmware ELF is absent. Run scripts/build.ps1 first."
}

$nm = Find-XpackExecutable `
    -PackageName "arm-none-eabi-gcc" `
    -Executable "arm-none-eabi-nm.exe"
$symbolLine = & $nm -n $elf |
    Where-Object { $_ -match '\bwatch_debug_state$' } |
    Select-Object -First 1
if (-not $symbolLine -or $symbolLine -notmatch '^([0-9A-Fa-f]+)\s') {
    throw "watch_debug_state was not found in '$elf'."
}

$stateValue = [Convert]::ToUInt32($Matches[1], 16)
function Format-Address {
    param([uint32]$Value)
    return ("0x{0:X8}" -f $Value)
}

$stateAddress = Format-Address $stateValue
$touchAddress = Format-Address ($stateValue + 16)
$busAddress = Format-Address ($stateValue + 52)
$guiAddress = Format-Address ($stateValue + 68)
$tailAddress = Format-Address ($stateValue + 84)
$bleAddress = Format-Address ($stateValue + 96)

$commands = @(
    "r",
    # A terminated GDB session can leave Cortex-M FPB comparators enabled.
    "w4 0xE0002000, 0x00000002",
    "g",
    "sleep 1000",
    "h",
    "mem32 $stateAddress 4",
    "g",
    "sleep 1000",
    "h",
    "mem32 $stateAddress 4",
    "mem32 $touchAddress 4",
    "mem32 $busAddress 4",
    "mem32 $guiAddress 4",
    "mem32 $tailAddress 2",
    "mem32 $bleAddress 8",
    "g",
    "q"
)

$jlink = Get-JLinkCommander
$commandFile = [System.IO.Path]::GetTempFileName()
try {
    Set-Content -LiteralPath $commandFile -Value $commands -Encoding ASCII
    $output = & $jlink -NoGui 1 -ExitOnError 1 -Device NRF52840_XXAA `
        -If SWD -Speed $JLinkSpeedKhz -AutoConnect 1 `
        -CommandFile $commandFile 2>&1
    $exitCode = $LASTEXITCODE
}
finally {
    Remove-Item -LiteralPath $commandFile -Force -ErrorAction SilentlyContinue
}

if ($exitCode -ne 0) {
    $output | Out-Host
    throw "J-Link runtime verification failed with exit code $exitCode."
}

function Get-MemoryRows {
    param(
        [Parameter(Mandatory = $true)][string]$Address,
        [Parameter(Mandatory = $true)][int]$ExpectedCount
    )

    $bare = $Address.Substring(2)
    $pattern =
        "(?im)^\s*$([regex]::Escape($bare))\s*=\s*" +
        "([0-9A-Fa-f]{8}(?:\s+[0-9A-Fa-f]{8}){0,3})\s*$"
    $rows = [regex]::Matches(($output -join "`n"), $pattern)
    if ($rows.Count -ne $ExpectedCount) {
        $output | Out-Host
        throw "Expected $ExpectedCount memory row(s) at $Address, received $($rows.Count)."
    }
    return $rows
}

function Convert-MemoryWords {
    param([Parameter(Mandatory = $true)]$Match)

    return @($Match.Groups[1].Value -split '\s+' |
        ForEach-Object { [Convert]::ToUInt32($_, 16) })
}

$baseRows = Get-MemoryRows $stateAddress 2
$first = Convert-MemoryWords $baseRows[0]
$second = Convert-MemoryWords $baseRows[1]
$touch = Convert-MemoryWords (Get-MemoryRows $touchAddress 1)[0]
$bus = Convert-MemoryWords (Get-MemoryRows $busAddress 1)[0]
$gui = Convert-MemoryWords (Get-MemoryRows $guiAddress 1)[0]
$tail = Convert-MemoryWords (Get-MemoryRows $tailAddress 1)[0]
$ble = Convert-MemoryWords (Get-MemoryRows $bleAddress 1)[0]

$failures = [System.Collections.Generic.List[string]]::new()
if ($second[0] -ne 0x57415443) {
    $failures.Add("debug magic is invalid")
}
if ($second[2] -le $first[2]) {
    $failures.Add("heartbeat did not increase")
}
if ($second[3] -ge 0xE000) {
    $failures.Add(("firmware stopped at error checkpoint 0x{0:X}" -f $second[3]))
}
if ($touch[0] -ne 1 -or $touch[1] -ne 0xB6) {
    $failures.Add(
        ("CST816D identity mismatch: present={0}, chip=0x{1:X}" -f
            $touch[0], $touch[1]))
}
if ($bus[0] -ne 0) {
    $failures.Add("touch I2C error counter is non-zero")
}
if ($bus[1] -ge 24 -or $bus[2] -ge 60 -or $bus[3] -ge 60) {
    $failures.Add("RTC time fields are outside civil-time ranges")
}
if ($gui[0] -ne 0) {
    $failures.Add("GUI error counter is non-zero")
}
if ($gui[1] -ne 0) {
    $failures.Add("GUI did not return to HOME_CONTEXT after reset")
}
if ($gui[2] -ge 8) {
    $failures.Add("RTC subsecond is outside the 8 Hz range")
}
if ($gui[3] -ne 0 -or $tail[0] -ne 0) {
    $failures.Add("command/request lifecycle is unexpectedly active")
}
if ($tail[1] -ne 1842) {
    $failures.Add("test View Model revision is not 1842")
}
if ($ble[0] -lt 1 -or $ble[0] -eq 0xFF) {
    $failures.Add(("BLE is not advertising/connected: state={0}" -f $ble[0]))
}
if ($ble[1] -ne 0) {
    $failures.Add(("BLE error is 0x{0:X8}" -f $ble[1]))
}

if ($failures.Count -ne 0) {
    $failures | ForEach-Object { Write-Error $_ }
    throw "$($failures.Count) runtime verification check(s) failed."
}

$summary =
    "Runtime verification passed: heartbeat {0}->{1}, " +
    "CST816D=0x{2:X2}, GUI errors={3}, revision={4}, BLE state={5}."
Write-Host ($summary -f
    $first[2], $second[2], $touch[1], $gui[0], $tail[1], $ble[0])
