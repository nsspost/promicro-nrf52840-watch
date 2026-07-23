$ErrorActionPreference = "Stop"

$version = "0.22.2"
$expectedSha256 = "235B05A2E3C948426D35455753FBC5543ACD0B789E84F5E3EB1A011D498DDB0C"
$projectRoot = Split-Path -Parent $PSScriptRoot
$toolDir = Join-Path $projectRoot "tools\statesmith"
$archive = Join-Path $toolDir "statesmith-win-x64.zip"
$executable = Join-Path $toolDir "ss.cli.exe"
$url = "https://github.com/StateSmith/StateSmith/releases/download/v$version/statesmith-win-x64.zip"

if (Test-Path -LiteralPath $executable) {
    & $executable --version
    exit $LASTEXITCODE
}

New-Item -ItemType Directory -Force -Path $toolDir | Out-Null
Invoke-WebRequest -Uri $url -OutFile $archive

$actualSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash
if ($actualSha256 -ne $expectedSha256) {
    throw "StateSmith archive SHA-256 mismatch: $actualSha256"
}

Expand-Archive -LiteralPath $archive -DestinationPath $toolDir -Force
& $executable --version
exit $LASTEXITCODE
