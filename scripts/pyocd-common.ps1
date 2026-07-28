$ErrorActionPreference = "Stop"

$script:ProjectRoot = Split-Path -Parent $PSScriptRoot
$script:DefaultProbeId = "368D32883435"
$script:DefaultTarget = "nrf52840"
$script:DefaultFrequency = "1m"

function Get-WatchProbeId {
    if ($env:WATCH_PROBE_ID) {
        return $env:WATCH_PROBE_ID
    }
    return $script:DefaultProbeId
}

function Get-WatchAdb {
    $adb = Join-Path $env:LOCALAPPDATA "Android\Sdk\platform-tools\adb.exe"
    if (Test-Path -LiteralPath $adb) {
        return $adb
    }

    $command = Get-Command adb -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    throw "adb was not found. Install Android platform-tools or add adb to PATH."
}

function Get-PyOcdCommand {
    $pyocd = Get-Command pyocd -ErrorAction SilentlyContinue
    if ($pyocd) {
        return @{
            Executable = $pyocd.Source
            Prefix = @()
            Label = "pyocd"
        }
    }

    $uvx = Get-Command uvx -ErrorAction SilentlyContinue
    if ($uvx) {
        return @{
            Executable = $uvx.Source
            Prefix = @("--python", "3.13", "pyocd")
            Label = "uvx pyocd"
        }
    }

    throw "pyocd was not found. Install it directly or install uv/uvx."
}

function Invoke-PyOcd {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $command = Get-PyOcdCommand
    Write-Host ("Using {0}" -f $command.Label)
    $savedErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = & $command.Executable @($command.Prefix + $Arguments) 2>&1
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $savedErrorActionPreference
    }
    $output |
        ForEach-Object {
            if ($_ -is [System.Management.Automation.ErrorRecord]) {
                $_.Exception.Message
            } else {
                $_
            }
        } |
        Out-Host
    return $exitCode
}

function Get-WatchSoftDeviceHex {
    return Join-Path $ProjectRoot `
        "external\nRF5_SDK_17.1.0_ddde560\components\softdevice\s140\hex\s140_nrf52_7.2.0_softdevice.hex"
}

function Get-WatchFirmwareHex {
    return Join-Path $ProjectRoot "build\watch_firmware.hex"
}

function Get-WatchFirmwareElf {
    return Join-Path $ProjectRoot "build\watch_firmware.elf"
}

function Get-WatchNm {
    $nm = Join-Path $ProjectRoot `
        "xpacks\@xpack-dev-tools\arm-none-eabi-gcc\.content\bin\arm-none-eabi-nm.exe"
    if (Test-Path -LiteralPath $nm) {
        return $nm
    }
    $command = Get-Command arm-none-eabi-nm -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }
    throw "arm-none-eabi-nm was not found. Run npx xpm install first."
}

function Get-WatchSymbolAddress {
    param(
        [Parameter(Mandatory = $true)][string]$Symbol
    )

    $elf = Get-WatchFirmwareElf
    if (-not (Test-Path -LiteralPath $elf)) {
        throw "Firmware ELF is absent. Run npm run build first."
    }

    $nm = Get-WatchNm
    $symbolLine = & $nm -n $elf |
        Where-Object { $_ -match ("\b" + [regex]::Escape($Symbol) + "$") } |
        Select-Object -First 1
    if (-not $symbolLine -or $symbolLine -notmatch "^([0-9A-Fa-f]+)\s") {
        throw "$Symbol was not found in '$elf'."
    }

    return [Convert]::ToUInt32($Matches[1], 16)
}

function Format-WatchAddress {
    param([uint32]$Value)
    return ("0x{0:X8}" -f $Value)
}
