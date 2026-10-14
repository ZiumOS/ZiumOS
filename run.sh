#!/bin/bash
set -e

if [ ! -f "os.iso" ]; then
    echo "ISO not found. Building first..."
    ./build.sh
fi

if ! command -v qemu-system-i386 &> /dev/null; then
    echo "Error: qemu-system-i386 not found."
    echo "Please install QEMU: sudo apt-get install qemu-system-x86"
    exit 1
fi

echo "Running ZiumOS with QEMU..."
qemu-system-i386 -cdrom os.iso -boot d -m 512