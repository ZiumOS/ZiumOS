#!/bin/bash
set -e

echo "Building ZiumOS in release mode..."
# Override optimization flags for release
make all CXXFLAGS="-m32 -fno-use-cxa-atexit -fno-rtti -fno-exceptions -nostdlib -ffreestanding -fno-stack-protector -O3 -Wall -Wextra -DNDEBUG"
echo "Release build complete. ISO: os.iso"