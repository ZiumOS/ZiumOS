#!/bin/bash
# Script init untuk menyiapkan environment di Gitpod

# Pastikan skrip berhenti jika ada error
set -e

# Fungsi untuk menampilkan pesan status
function log_info() {
    echo -e "\e[1;34m[INFO]\e[0m $1"
}

# Install dependencies
log_info "Menginstall dependencies yang diperlukan..."
sudo apt-get update
sudo apt-get install -y build-essential nasm grub-common xorriso grub-pc-bin

# Buat direktori untuk persistent storage
log_info "Menyiapkan persistent storage..."
STORAGE_DIR="/workspace/persistent-storage"

if [ ! -d "$STORAGE_DIR" ]; then
    mkdir -p "$STORAGE_DIR"
    log_info "Direktori persistent storage dibuat di $STORAGE_DIR"
else
    log_info "Direktori persistent storage sudah ada di $STORAGE_DIR"
fi

# Buat symlink untuk package dan tools yang diinstall
log_info "Menyiapkan symlink untuk persistent packages..."
mkdir -p "$STORAGE_DIR/packages"

# Tambahkan PATH kustom ke .bashrc
if ! grep -q "PERSISTENT_PATH" ~/.bashrc; then
    echo 'export PERSISTENT_PATH="/workspace/persistent-storage/packages/bin"' >> ~/.bashrc
    echo 'export PATH="$PERSISTENT_PATH:$PATH"' >> ~/.bashrc
    log_info "PATH kustom ditambahkan ke .bashrc"
fi

# Function untuk menginstall package ke persistent storage
function install_to_persistent() {
    local package=$1
    local install_dir="$STORAGE_DIR/packages/$package"
    
    if [ ! -d "$install_dir" ]; then
        log_info "Menginstall $package ke persistent storage..."
        mkdir -p "$install_dir"
        # Contoh instalasi package (sesuaikan dengan kebutuhan)
        # wget ... atau git clone ... ke $install_dir
    else
        log_info "$package sudah terinstall di persistent storage"
    fi
}

# Install beberapa package contoh
# install_to_persistent "custom-package1"
# install_to_persistent "custom-package2"

# Siapkan environment untuk membangun OS
log_info "Menyiapkan environment untuk build OS..."
if [ ! -f "boot.asm" ] || [ ! -f "kernel.cpp" ]; then
    # Copy file template jika belum ada
    cp -n "$STORAGE_DIR/templates/boot.asm" . 2>/dev/null || log_info "File boot.asm sudah ada"
    cp -n "$STORAGE_DIR/templates/kernel.cpp" . 2>/dev/null || log_info "File kernel.cpp sudah ada"
    cp -n "$STORAGE_DIR/templates/Makefile" . 2>/dev/null || log_info "File Makefile sudah ada"
    cp -n "$STORAGE_DIR/templates/linker.ld" . 2>/dev/null || log_info "File linker.ld sudah ada"
    cp -n "$STORAGE_DIR/templates/grub.cfg" . 2>/dev/null || log_info "File grub.cfg sudah ada"
fi

# Berikan izin eksekusi pada script
chmod +x *.sh 2>/dev/null || true

log_info "Environment setup selesai! Anda bisa mulai bekerja pada OS."
log_info "Untuk membangun OS, jalankan 'make all'"
log_info "Untuk membersihkan file sementara, jalankan 'make clean'"