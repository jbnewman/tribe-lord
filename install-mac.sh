#!/bin/bash
# Installs everything needed to build Tribe Lord on macOS (Apple Silicon like the M3, or Intel):
#   Xcode command line tools (make), devkitPro GBA tools (devkitARM + libtonc), and the mGBA emulator.
# Run from Terminal:  bash install-mac.sh
# Safe to run again; it skips anything already installed.

set -euo pipefail

step() {
    printf '\n\033[36m==> %s\033[0m\n' "$1"
}

step "Checking Xcode command line tools"
if ! xcode-select -p >/dev/null 2>&1; then
    xcode-select --install
    echo "Finish the install window that just opened, then run this script again."
    exit 1
fi

step "Installing devkitPro pacman"
if [ ! -x /usr/local/bin/dkp-pacman ]; then
    pkg_dir="$(mktemp -d)"
    curl -fL -o "$pkg_dir/devkitpro-pacman-installer.pkg" \
        https://github.com/devkitPro/pacman/releases/latest/download/devkitpro-pacman-installer.pkg
    sudo installer -pkg "$pkg_dir/devkitpro-pacman-installer.pkg" -target /
    rm -rf "$pkg_dir"
fi

step "Installing devkitPro GBA tools"
sudo /usr/local/bin/dkp-pacman -Syu --noconfirm
sudo /usr/local/bin/dkp-pacman -S --needed --noconfirm gba-dev libtonc

step "Setting DEVKITPRO and DEVKITARM environment variables"
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
if ! grep -q 'DEVKITPRO=' "$HOME/.zprofile" 2>/dev/null; then
    {
        echo ''
        echo '# devkitPro (Game Boy Advance tools)'
        echo 'export DEVKITPRO=/opt/devkitpro'
        echo 'export DEVKITARM=$DEVKITPRO/devkitARM'
    } >> "$HOME/.zprofile"
fi

step "Installing mGBA emulator"
if command -v brew >/dev/null 2>&1; then
    brew list --cask mgba >/dev/null 2>&1 || brew install --cask mgba
else
    echo "Homebrew not found. Download mGBA from https://mgba.io/downloads.html"
fi

step "Checking the compiler"
"$DEVKITARM/bin/arm-none-eabi-gcc" --version | head -n 1

printf '\n\033[32mAll done!\033[0m Open a new Terminal window, then build with:\n'
echo "  cd \"$(cd "$(dirname "$0")" && pwd)\" && make"
echo "Then open tribe-lord.gba in mGBA."
