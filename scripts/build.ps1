. (Join-Path $PSScriptRoot "common.ps1")

& (Join-Path $PSScriptRoot "generate-statecharts.ps1")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$gcc = Find-XpackExecutable -PackageName "arm-none-eabi-gcc" -Executable "arm-none-eabi-gcc.exe"
$cmake = Find-XpackExecutable -PackageName "cmake" -Executable "cmake.exe"
$ninja = Find-XpackExecutable -PackageName "ninja-build" -Executable "ninja.exe"
$gccRoot = Split-Path -Parent (Split-Path -Parent $gcc)
$buildDir = Join-Path $ProjectRoot "build"
$guiStack = "ON"
$localNow = Get-Date

& $cmake --fresh -S $ProjectRoot -B $buildDir -G Ninja `
    "-DARM_GCC_ROOT=$gccRoot" `
    "-DCMAKE_MAKE_PROGRAM=$ninja" `
    "-DCMAKE_BUILD_TYPE=Debug" `
    "-DWATCH_ENABLE_GUI_STACK=$guiStack" `
    "-DWATCH_INITIAL_HOUR=$($localNow.Hour)" `
    "-DWATCH_INITIAL_MINUTE=$($localNow.Minute)" `
    "-DWATCH_INITIAL_SECOND=$($localNow.Second)"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $cmake --build $buildDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& (Join-Path $PSScriptRoot "check-embedded-dependencies.ps1")
exit $LASTEXITCODE
