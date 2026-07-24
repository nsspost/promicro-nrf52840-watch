. (Join-Path $PSScriptRoot "common.ps1")

$elf = Join-Path $ProjectRoot "build/watch_firmware.elf"
if (-not (Test-Path -LiteralPath $elf)) {
    throw "Firmware ELF is absent. Run scripts/build.ps1 first."
}

$nm = Find-XpackExecutable `
    -PackageName "arm-none-eabi-gcc" `
    -Executable "arm-none-eabi-nm.exe"
$symbols = & $nm -n $elf

function Get-FirmwareSymbolAddress {
    param([Parameter(Mandatory = $true)][string]$Name)

    $line = $symbols |
        Where-Object { $_ -match "\b$([regex]::Escape($Name))$" } |
        Select-Object -First 1
    if (-not $line -or $line -notmatch '^([0-9A-Fa-f]+)\s') {
        throw "Firmware symbol '$Name' was not found in '$elf'."
    }
    return "0x$($Matches[1])"
}

$commandAddress = Get-FirmwareSymbolAddress "watch_ui_test_command"
$xAddress = Get-FirmwareSymbolAddress "watch_ui_test_x"
$yAddress = Get-FirmwareSymbolAddress "watch_ui_test_y"
$resultAddress = Get-FirmwareSymbolAddress "watch_ui_test_result"

function New-UiStep {
    param(
        [string]$Name,
        [uint32]$Command,
        [uint32]$X,
        [uint32]$Y,
        [uint32]$Expected
    )

    [pscustomobject]@{
        Name = $Name
        Command = $Command
        X = $X
        Y = $Y
        Expected = $Expected
    }
}

# Command result: 0x600D0000 | critical-phase<<12 |
# command-phase<<8 | skin<<7 | current-screen. Phases are reported only on
# their own modal screen, making every expected value independent of old
# modal state.
$steps = @(
    (New-UiStep "home -> devices"             1  60 200 0x600D0001),
    (New-UiStep "devices -> PUMP-2"            1 100  65 0x600D0002),
    (New-UiStep "overview -> parameters"       1  60 200 0x600D0003),
    (New-UiStep "parameters -> metric"         1 100  65 0x600D0004),
    (New-UiStep "metric -> parameters"         1  40  25 0x600D0003),
    (New-UiStep "parameters -> overview"       1  40  25 0x600D0002),
    (New-UiStep "overview -> controls"         1 170 200 0x600D0005),
    (New-UiStep "guarded command confirm"      1 100  80 0x600D0106),
    (New-UiStep "guarded command hold"         2 120 195 0x600D0206),
    (New-UiStep "guarded command success"      3   0   0 0x600D0306),
    (New-UiStep "next command case"            1 120 195 0x600D0106),
    (New-UiStep "second command hold"          2 120 195 0x600D0206),
    (New-UiStep "guarded command rejected"     3   0   0 0x600D0406),
    (New-UiStep "next command case 2"          1 120 195 0x600D0106),
    (New-UiStep "third command hold"           2 120 195 0x600D0206),
    (New-UiStep "guarded command timeout"      3   0   0 0x600D0506),
    (New-UiStep "reset command demo outcome"   1 120 195 0x600D0106),
    (New-UiStep "command -> controls"          1  40  25 0x600D0005),
    (New-UiStep "critical request confirm"     1 100 145 0x600D1007),
    (New-UiStep "critical request hold"        2 120 195 0x600D2007),
    (New-UiStep "critical request approved"    3   0   0 0x600D3007),
    (New-UiStep "next critical case"           1 120 195 0x600D1007),
    (New-UiStep "blocking event preemption"    4   0   0 0x600D0009),
    (New-UiStep "event -> pending request"      1  40  25 0x600D1007),
    (New-UiStep "second critical hold"         2 120 195 0x600D2007),
    (New-UiStep "critical request denied"      3   0   0 0x600D4007),
    (New-UiStep "reset critical demo outcome"  1 120 195 0x600D1007),
    (New-UiStep "request -> controls"          1  40  25 0x600D0005),
    (New-UiStep "controls -> overview"         1  40  25 0x600D0002),
    (New-UiStep "overview -> devices"          1  40  25 0x600D0001),
    (New-UiStep "devices -> offline detector"  1 100 114 0x600D0002),
    (New-UiStep "detector -> devices"           1  40  25 0x600D0001),
    (New-UiStep "devices -> local watch"        1 100 163 0x600D0002),
    (New-UiStep "watch -> parameters"           1  60 200 0x600D0003),
    (New-UiStep "watch parameter -> detail"     1 100  65 0x600D0004),
    (New-UiStep "watch metric -> parameters"    1  40  25 0x600D0003),
    (New-UiStep "watch parameters -> overview"  1  40  25 0x600D0002),
    (New-UiStep "watch overview -> devices"     1  40  25 0x600D0001),
    (New-UiStep "devices -> home"              1  40  25 0x600D0000),
    (New-UiStep "home -> event journal"        1 170 200 0x600D0008),
    (New-UiStep "journal -> event detail"      1 100  65 0x600D0009),
    (New-UiStep "acknowledge blocking event"   1 120 195 0x600D0008),
    (New-UiStep "journal -> home"              1  40  25 0x600D0000),
    (New-UiStep "home -> diagnostic"           1 120  20 0x600D000A),
    (New-UiStep "diagnostic -> strict skin"    1 120 178 0x600D008A),
    (New-UiStep "strict diagnostic -> home"    1 120 210 0x600D0080),
    (New-UiStep "strict home -> devices"       1 120 215 0x600D0081),
    (New-UiStep "strict devices -> PUMP-2"     1 100  85 0x600D0082),
    (New-UiStep "strict overview -> params"    1  70 200 0x600D0083),
    (New-UiStep "strict params -> metric"      1 100  85 0x600D0084),
    (New-UiStep "strict metric -> params"      1  30  25 0x600D0083),
    (New-UiStep "strict params -> overview"    1  30  25 0x600D0082),
    (New-UiStep "strict overview -> controls"  1 170 200 0x600D0085),
    (New-UiStep "strict command confirm"       1 100 140 0x600D0186),
    (New-UiStep "strict command hold"          2 120 195 0x600D0286),
    (New-UiStep "strict command success"       3   0   0 0x600D0386),
    (New-UiStep "strict command -> controls"   1  30  25 0x600D0085),
    (New-UiStep "strict critical confirm"      1 100 200 0x600D1087),
    (New-UiStep "strict critical hold"         2 120 195 0x600D2087),
    (New-UiStep "strict critical approved"     3   0   0 0x600D3087),
    (New-UiStep "strict request -> controls"   1  30  25 0x600D0085),
    (New-UiStep "strict controls -> overview"  1  30  25 0x600D0082),
    (New-UiStep "strict overview -> devices"   1  30  25 0x600D0081),
    (New-UiStep "strict devices -> home"       1  30  25 0x600D0080),
    (New-UiStep "strict home -> journal"       1 190 215 0x600D0088),
    (New-UiStep "strict journal -> detail"     1 100  85 0x600D0089),
    (New-UiStep "strict acknowledge event"     1 120 205 0x600D0088),
    (New-UiStep "strict journal -> home"       1  30  25 0x600D0080)
)

$commands = [System.Collections.Generic.List[string]]::new()
$commands.Add("r")
$commands.Add("h")
$commands.Add("w4 0xE0002000, 0x00000002")
$commands.Add("g")
$commands.Add("sleep 1200")
$commands.Add("h")

foreach ($step in $steps) {
    $commands.Add(("w4 {0}, 0x{1:X8}" -f $xAddress, $step.X))
    $commands.Add(("w4 {0}, 0x{1:X8}" -f $yAddress, $step.Y))
    $commands.Add(("w4 {0}, 0x{1:X8}" -f $commandAddress, $step.Command))
    $commands.Add("g")
    $commands.Add("sleep 600")
    $commands.Add("h")
    $commands.Add("mem32 $resultAddress 1")
}
$commands.Add("g")
$commands.Add("q")

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
    throw "J-Link UI verification failed with exit code $exitCode."
}

$bareResultAddress = $resultAddress.Substring(2)
$pattern = "(?im)^\s*$([regex]::Escape($bareResultAddress))\s*=\s*([0-9A-Fa-f]{8})"
$matches = [regex]::Matches(($output -join "`n"), $pattern)
if ($matches.Count -ne $steps.Count) {
    $output | Out-Host
    throw "Expected $($steps.Count) UI results, received $($matches.Count)."
}

$failures = [System.Collections.Generic.List[string]]::new()
for ($i = 0; $i -lt $steps.Count; $i++) {
    $actual = [Convert]::ToUInt32($matches[$i].Groups[1].Value, 16)
    $expected = [uint32]$steps[$i].Expected
    if ($actual -eq $expected) {
        Write-Host ("PASS {0,-34} 0x{1:X8}" -f $steps[$i].Name, $actual)
    } else {
        $failures.Add(
            ("FAIL {0}: expected 0x{1:X8}, got 0x{2:X8}" -f
                $steps[$i].Name, $expected, $actual))
    }
}

if ($failures.Count -ne 0) {
    $failures | ForEach-Object { Write-Error $_ }
    throw "$($failures.Count) UI verification step(s) failed."
}

Write-Host "UI verification passed: $($steps.Count) semantic interaction steps."
