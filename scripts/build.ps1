param(
    [switch]$Gui
)

. (Join-Path $PSScriptRoot "common.ps1")

$gcc = Find-XpackExecutable -PackageName "arm-none-eabi-gcc" -Executable "arm-none-eabi-gcc.exe"
$cmake = Find-XpackExecutable -PackageName "cmake" -Executable "cmake.exe"
$ninja = Find-XpackExecutable -PackageName "ninja-build" -Executable "ninja.exe"
$gccRoot = Split-Path -Parent (Split-Path -Parent $gcc)
$buildDir = Join-Path $ProjectRoot "build"
$guiStack = if ($Gui) { "ON" } else { "OFF" }

& $cmake --fresh -S $ProjectRoot -B $buildDir -G Ninja `
    "-DARM_GCC_ROOT=$gccRoot" `
    "-DCMAKE_MAKE_PROGRAM=$ninja" `
    "-DCMAKE_BUILD_TYPE=Debug" `
    "-DWATCH_ENABLE_GUI_STACK=$guiStack"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $cmake --build $buildDir
exit $LASTEXITCODE
