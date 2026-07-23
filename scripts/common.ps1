$ErrorActionPreference = "Stop"

$script:ProjectRoot = Split-Path -Parent $PSScriptRoot
$script:XpacksRoot = Join-Path $ProjectRoot "xpacks"
$script:JLinkSpeedKhz = 100

function Find-XpackExecutable {
    param(
        [Parameter(Mandatory = $true)][string]$PackageName,
        [Parameter(Mandatory = $true)][string]$Executable
    )

    $package = Join-Path $XpacksRoot "@xpack-dev-tools/$PackageName"
    if (-not (Test-Path $package)) {
        throw "Tool package '$PackageName' is absent. Run: npx xpm install"
    }

    $match = Get-ChildItem -Path (Join-Path $package ".content/bin") -File -Filter $Executable |
        Select-Object -First 1
    if (-not $match) {
        throw "'$Executable' was not found under '$package'."
    }
    return $match.FullName
}

function Get-OpenOcd {
    Find-XpackExecutable -PackageName "openocd" -Executable "openocd.exe"
}

function Get-Gdb {
    Find-XpackExecutable -PackageName "arm-none-eabi-gcc" -Executable "arm-none-eabi-gdb.exe"
}

function Find-SeggerExecutable {
    param(
        [Parameter(Mandatory = $true)][string[]]$Names
    )

    foreach ($name in $Names) {
        $command = Get-Command $name -ErrorAction SilentlyContinue
        if ($command) { return $command.Source }
    }

    $seggerRoots = @(
        (Join-Path $env:ProgramFiles "SEGGER"),
        (Join-Path ${env:ProgramFiles(x86)} "SEGGER")
    ) | Where-Object { $_ -and (Test-Path $_) }

    foreach ($root in $seggerRoots) {
        foreach ($name in $Names) {
            $match = Get-ChildItem -Path $root -Recurse -File -Filter $name `
                -ErrorAction SilentlyContinue |
                Sort-Object FullName -Descending |
                Select-Object -First 1
            if ($match) { return $match.FullName }
        }
    }

    throw @"
SEGGER J-Link Software is not installed.
Run 'npm run install-jlink', accept SEGGER's Terms of Use in the browser,
install the 64-bit package, then repeat this command.
"@
}

function Get-JLinkCommander {
    Find-SeggerExecutable -Names @("JLink.exe", "JLinkExe.exe")
}

function Get-JLinkGdbServer {
    Find-SeggerExecutable -Names @("JLinkGDBServerCL.exe", "JLinkGDBServerCLExe.exe")
}

function Invoke-JLinkCommandFile {
    param(
        [Parameter(Mandatory = $true)][string[]]$Commands
    )

    $jlink = Get-JLinkCommander
    $commandFile = [System.IO.Path]::GetTempFileName()
    try {
        Set-Content -LiteralPath $commandFile -Value $Commands -Encoding ASCII
        $output = & $jlink -NoGui 1 -ExitOnError 1 -Device NRF52840_XXAA `
            -If SWD -Speed $JLinkSpeedKhz -AutoConnect 1 `
            -CommandFile $commandFile 2>&1
        $exitCode = $LASTEXITCODE
        $output | Out-Host
        return $exitCode
    }
    finally {
        Remove-Item -LiteralPath $commandFile -Force -ErrorAction SilentlyContinue
    }
}
