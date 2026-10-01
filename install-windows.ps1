# Installs everything needed to build Tribe Lord on Windows:
#   MSYS2 (Linux-style shell and make), devkitPro GBA tools (devkitARM + libtonc), and the mGBA emulator.
# Run from PowerShell:  powershell -ExecutionPolicy Bypass -File .\install-windows.ps1
# Safe to run again; it skips anything already installed.

$ErrorActionPreference = 'Stop'

$msysRoot = 'C:\msys64'
$bash = Join-Path $msysRoot 'usr\bin\bash.exe'

function Write-Step($message) {
    Write-Host "`n==> $message" -ForegroundColor Cyan
}

function Install-WingetPackage($id) {
    # winget returns an error code when the app is already installed, so don't stop on it
    winget install --id $id --exact --silent --accept-package-agreements --accept-source-agreements
}

function Invoke-Msys($command) {
    & $bash -lc $command
    if ($LASTEXITCODE -ne 0) {
        throw "MSYS2 command failed: $command"
    }
}

if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
    throw 'winget was not found. Install "App Installer" from the Microsoft Store, then run this script again.'
}

Write-Step 'Installing MSYS2'
if (-not (Test-Path $bash)) {
    Install-WingetPackage 'MSYS2.MSYS2'
}
if (-not (Test-Path $bash)) {
    throw "MSYS2 did not install to $msysRoot."
}

Write-Step 'Updating MSYS2'
Invoke-Msys 'true'
# The first update can close itself after updating core files, so it runs twice
& $bash -lc 'pacman -Syuu --noconfirm'
Invoke-Msys 'pacman -Syuu --noconfirm'

Write-Step 'Installing devkitPro GBA tools'
$setupScript = @'
set -e
if ! grep -q '^\[dkp-libs\]' /etc/pacman.conf; then
    pacman-key --recv BC26F752D25B92CE272E0F44F7FD5492264BB9D0 --keyserver keyserver.ubuntu.com
    pacman-key --lsign BC26F752D25B92CE272E0F44F7FD5492264BB9D0
    pacman -U --noconfirm https://pkg.devkitpro.org/devkitpro-keyring.pkg.tar.zst
    pacman-key --populate devkitpro
    printf '\n[dkp-libs]\nServer = https://pkg.devkitpro.org/packages\n\n[dkp-windows]\nServer = https://pkg.devkitpro.org/packages/windows/$arch/\n' >> /etc/pacman.conf
fi
pacman -Syu --noconfirm
pacman -S --needed --noconfirm make gba-dev libtonc
'@
# bash needs Unix line endings
$setupPath = Join-Path $env:TEMP 'tribe-lord-devkitpro-setup.sh'
[System.IO.File]::WriteAllText($setupPath, $setupScript.Replace("`r", ''), (New-Object System.Text.UTF8Encoding $false))
$setupPathUnix = & $bash -lc "cygpath -u '$setupPath'"
Invoke-Msys "bash '$setupPathUnix'"
Remove-Item $setupPath

Write-Step 'Setting DEVKITPRO and DEVKITARM environment variables'
$devkitPro = '/opt/devkitpro'
$devkitArm = '/opt/devkitpro/devkitARM'
[Environment]::SetEnvironmentVariable('DEVKITPRO', $devkitPro, 'User')
[Environment]::SetEnvironmentVariable('DEVKITARM', $devkitArm, 'User')
$env:DEVKITPRO = $devkitPro
$env:DEVKITARM = $devkitArm

Write-Step 'Installing mGBA emulator'
Install-WingetPackage 'JeffreyPfau.mGBA'

Write-Step 'Checking the compiler'
Invoke-Msys '/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc --version | head -n 1'

Write-Host "`nAll done! To build the game, run:" -ForegroundColor Green
Write-Host '  .\build.ps1'
Write-Host 'Then open tribe-lord.gba in mGBA.'
