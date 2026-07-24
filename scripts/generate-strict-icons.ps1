$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$edge = "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe"
if (-not (Test-Path -LiteralPath $edge)) {
    $edge = "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe"
}
if (-not (Test-Path -LiteralPath $edge)) {
    throw "Microsoft Edge is required to rasterize Phosphor SVG assets."
}

$names = @(
    "BATTERY", "CLOUD", "CARET_RIGHT", "CHAT", "ECOSYSTEM",
    "CARET_LEFT", "LIST", "SLIDERS", "LOCK", "STOP",
    "WARNING", "CHECK", "ERROR", "SPINNER"
)
$source = Join-Path $projectRoot "tools/strict-context-host/icon-atlas.html"
$outputDir = Join-Path $projectRoot "build/strict-context-assets"
$atlas = Join-Path $outputDir "icon-atlas.png"
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

& $edge --headless=new --disable-gpu --hide-scrollbars `
    --force-device-scale-factor=1 --window-size=448,32 `
    --run-all-compositor-stages-before-draw --virtual-time-budget=1000 `
    "--screenshot=$atlas" ([System.Uri]$source).AbsoluteUri | Out-Null
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $atlas)) {
    throw "Phosphor icon atlas rendering failed."
}

Add-Type -AssemblyName System.Drawing
$bitmap = [System.Drawing.Bitmap]::FromFile($atlas)
if ($bitmap.Width -ne 448 -or $bitmap.Height -ne 32) {
    throw "Icon atlas must be 448x32."
}

$header = @(
    "#ifndef WATCH_STRICT_CONTEXT_ICONS_H",
    "#define WATCH_STRICT_CONTEXT_ICONS_H",
    "",
    "#include <gno/gno.h>",
    "",
    "typedef enum {"
)
for ($index = 0; $index -lt $names.Count; $index++) {
    $comma = if ($index -lt ($names.Count - 1)) { "," } else { "" }
    $header += "    WATCH_STRICT_ICON_$($names[$index])$comma"
}
$header += @(
    "} watch_strict_icon_t;",
    "",
    "void watch_strict_draw_icon(gno_context_t *graphics,",
    "                            int x,",
    "                            int y,",
    "                            watch_strict_icon_t icon,",
    "                            gno_color_t color);",
    "",
    "#endif"
)

$sourceLines = @(
    "#include `"watch/strict_context_icons.h`"",
    "",
    "#include <stdbool.h>",
    "#include <stddef.h>",
    "#include <stdint.h>",
    ""
)

for ($icon = 0; $icon -lt $names.Count; $icon++) {
    $sourceLines += "static const uint8_t icon_$($names[$icon].ToLower())[128] = {"
    for ($y = 0; $y -lt 32; $y++) {
        $bytes = @()
        for ($byte = 0; $byte -lt 4; $byte++) {
            $value = 0
            for ($bit = 0; $bit -lt 8; $bit++) {
                $pixel = $bitmap.GetPixel(($icon * 32) + ($byte * 8) + $bit, $y)
                if (($pixel.R + $pixel.G + $pixel.B) -ge 384) {
                    $value = $value -bor (0x80 -shr $bit)
                }
            }
            $bytes += ("0x{0:X2}u" -f $value)
        }
        $sourceLines += "    $($bytes -join ', '),"
    }
    $sourceLines += "};"
    $sourceLines += ""
}

$sourceLines += @(
    "static const uint8_t *const icon_data[] = {"
)
foreach ($name in $names) {
    $sourceLines += "    icon_$($name.ToLower()),"
}
$sourceLines += @(
    "};",
    "",
    "void watch_strict_draw_icon(gno_context_t *graphics,",
    "                            int x,",
    "                            int y,",
    "                            watch_strict_icon_t icon,",
    "                            gno_color_t color)",
    "{",
    "    if (graphics == NULL || (uint32_t)icon >=",
    "        (sizeof(icon_data) / sizeof(icon_data[0]))) {",
    "        return;",
    "    }",
    "    const gno_mono_bitmap_t bitmap = {",
    "        .width = 32u,",
    "        .height = 32u,",
    "        .row_stride_bytes = 4u,",
    "        .pixels = icon_data[(uint32_t)icon]",
    "    };",
    "    gno_draw_mono_bitmap(graphics, x, y, &bitmap,",
    "                         color, GNO_RGB(0, 0, 0), false);",
    "}"
)

$bitmap.Dispose()
$headerPath = Join-Path $projectRoot "include/watch/strict_context_icons.h"
$sourcePath = Join-Path $projectRoot "src/gui/strict_context_icons.c"
Set-Content -LiteralPath $headerPath -Value $header -Encoding ASCII
Set-Content -LiteralPath $sourcePath -Value $sourceLines -Encoding ASCII
Write-Host "Generated 14 bounded Phosphor mono masks."
