$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$edge = "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe"
if (-not (Test-Path -LiteralPath $edge)) {
    $edge = "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe"
}
if (-not (Test-Path -LiteralPath $edge)) {
    throw "Microsoft Edge is required for deterministic Roboto host rendering."
}

$source = Join-Path $projectRoot "tools/strict-context-host/home-round.html"
$reference = Join-Path $projectRoot `
    "docs/tsehosense-watch-graphics-spec-v1/reference/screens/preview-round.png"
$outputDir = Join-Path $projectRoot "build/strict-context-host"
$actual = Join-Path $outputDir "home-round.png"
$difference = Join-Path $outputDir "home-round-diff.png"
$comparison = Join-Path $outputDir "home-round-comparison.png"
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

$sourceUri = ([System.Uri]$source).AbsoluteUri
& $edge --headless=new --disable-gpu --hide-scrollbars `
    --force-device-scale-factor=1 --window-size=256,256 `
    --run-all-compositor-stages-before-draw --virtual-time-budget=1000 `
    "--screenshot=$actual" $sourceUri | Out-Null
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $actual)) {
    throw "Strict Context host screenshot failed."
}

Add-Type -AssemblyName System.Drawing
$expectedBitmap = [System.Drawing.Bitmap]::FromFile($reference)
$actualBitmap = [System.Drawing.Bitmap]::FromFile($actual)
if ($expectedBitmap.Width -ne 256 -or $expectedBitmap.Height -ne 256 -or
    $actualBitmap.Width -ne 256 -or $actualBitmap.Height -ne 256) {
    throw "Golden and actual images must both be 256x256."
}

$diffBitmap = New-Object System.Drawing.Bitmap 256,256
$sideBitmap = New-Object System.Drawing.Bitmap 768,256
$same = 0
$total = 256 * 256
for ($y = 0; $y -lt 256; $y++) {
    for ($x = 0; $x -lt 256; $x++) {
        $expectedPixel = $expectedBitmap.GetPixel($x, $y)
        $actualPixel = $actualBitmap.GetPixel($x, $y)
        if ($expectedPixel.ToArgb() -eq $actualPixel.ToArgb()) {
            $same++
            $diffBitmap.SetPixel($x, $y, [System.Drawing.Color]::Black)
        } else {
            $delta = [Math]::Min(
                255,
                [Math]::Abs($expectedPixel.R - $actualPixel.R) +
                [Math]::Abs($expectedPixel.G - $actualPixel.G) +
                [Math]::Abs($expectedPixel.B - $actualPixel.B))
            $diffBitmap.SetPixel(
                $x, $y, [System.Drawing.Color]::FromArgb(255, $delta, 0, 0))
        }
    }
}

$graphics = [System.Drawing.Graphics]::FromImage($sideBitmap)
$graphics.DrawImageUnscaled($expectedBitmap, 0, 0)
$graphics.DrawImageUnscaled($actualBitmap, 256, 0)
$graphics.DrawImageUnscaled($diffBitmap, 512, 0)
$graphics.Dispose()
$diffBitmap.Save($difference, [System.Drawing.Imaging.ImageFormat]::Png)
$sideBitmap.Save($comparison, [System.Drawing.Imaging.ImageFormat]::Png)
$expectedBitmap.Dispose()
$actualBitmap.Dispose()
$diffBitmap.Dispose()
$sideBitmap.Dispose()

$matchPercent = 100.0 * $same / $total
Write-Host ("Strict Context host render: {0:N2}% exact pixels." -f $matchPercent)
Write-Host "Actual:     $actual"
Write-Host "Difference: $difference"
Write-Host "Comparison: $comparison"
