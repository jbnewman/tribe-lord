# Builds Tribe Lord from PowerShell using the make inside MSYS2.
# Usage:  .\build.ps1          builds tribe-lord.gba
#         .\build.ps1 clean    deletes build files

$bash = 'C:\msys64\usr\bin\bash.exe'
if (-not (Test-Path $bash)) {
    Write-Host 'MSYS2 is not installed. Run install-windows.ps1 first.' -ForegroundColor Red
    exit 1
}

$env:DEVKITPRO = '/opt/devkitpro'
$env:DEVKITARM = '/opt/devkitpro/devkitARM'

$projectUnix = & $bash -lc "cygpath -u '$PSScriptRoot'"
& $bash -lc "cd '$projectUnix' && make $args"
exit $LASTEXITCODE
