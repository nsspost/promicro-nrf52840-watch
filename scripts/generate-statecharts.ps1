$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$stateSmith = Join-Path $projectRoot "tools\statesmith\ss.cli.exe"
$diagram = Join-Path $projectRoot "statecharts\GuiSm.plantuml"

if (-not (Test-Path -LiteralPath $stateSmith)) {
    & (Join-Path $PSScriptRoot "install-statesmith.ps1")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

& $stateSmith run $diagram --lang C99 --no-sim-gen --no-csx --no-ask --rebuild
exit $LASTEXITCODE
