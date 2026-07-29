. (Join-Path $PSScriptRoot "common.ps1")

$jlink = Get-JLinkCommander
$commandFile = [System.IO.Path]::GetTempFileName()
try {
    Set-Content -LiteralPath $commandFile `
        -Value @(
            "erase",
            "q"
        ) -Encoding ASCII
    & $jlink -NoGui 1 -ExitOnError 1 -Device NRF52840_XXAA `
        -If SWD -Speed 100 -CommandFile $commandFile
    exit $LASTEXITCODE
}
finally {
    Remove-Item -LiteralPath $commandFile -Force `
        -ErrorAction SilentlyContinue
}
