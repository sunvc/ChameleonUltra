#!/bin/bash
# Build the MAX fork application, package a signed DFU zip and flash it.
# Toolchain/nrfutil paths are overridable through environment variables.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP_DIR="$ROOT/firmware/application"
TOOLCHAIN="${GNU_INSTALL_ROOT:-/Users/lynn/armgnu/expanded/Payload/bin/}"
GNU_VERSION="${GNU_VERSION:-15.3.1}"
OUTPUT="${1:-$ROOT/build/max-custom.zip}"
PYTHON="${PYTHON:-python3}"

command -v nrfutil >/dev/null 2>&1 || { echo "nrfutil not found in PATH"; exit 1; }
[ -d "$TOOLCHAIN" ] || { echo "toolchain not found: $TOOLCHAIN (set GNU_INSTALL_ROOT)"; exit 1; }

echo "==> build"
cd "$APP_DIR"
make -j8 GNU_INSTALL_ROOT="$TOOLCHAIN" GNU_VERSION="$GNU_VERSION" CURRENT_DEVICE_TYPE=ultra

echo "==> package $OUTPUT"
mkdir -p "$(dirname "$OUTPUT")"
nrfutil pkg generate \
    --hw-version 0 \
    --sd-req 0x100 \
    --application "$ROOT/firmware/objects/application.bin" \
    --application-version 1 \
    --key-file "$ROOT/resource/dfu_key/chameleon.pem" \
    "$OUTPUT"

if ! nrfutil device list 2>&1 | grep -q "nordicDfu"; then
    echo "==> enter DFU"
    "$PYTHON" "$ROOT/tools/enter_dfu.py"
    for _ in $(seq 1 20); do
        if nrfutil device list 2>&1 | grep -q "nordicDfu"; then
            break
        fi
        sleep 0.5
    done
fi

if ! nrfutil device list 2>&1 | grep -q "nordicDfu"; then
    echo "device did not enter DFU. Manual: unplug USB, hold B while plugging in until LEDs 4/5 blink."
    exit 1
fi

echo "==> flash"
nrfutil device program --firmware "$OUTPUT" --traits nordicDfu

echo "==> wait for reboot"
for _ in $(seq 1 30); do
    if nrfutil device list 2>&1 | grep -q "fw_v515"; then
        break
    fi
    sleep 0.5
done
echo "done"
