#include <stdint.h>

// Deklarasi fungsi assembly
extern "C" void kernel_early();
extern "C" void kernel_main();

// Fungsi yang dipanggil dari bootloader
extern "C" void kernel_entry() {
    // Panggil setup awal hardware
    kernel_early();
    
    // Panggil fungsi kernel utama
    kernel_main();
    
    // Jika kernel_main selesai, hentikan CPU
    while(1) {
        asm volatile("hlt");
    }
}

// Fungsi untuk output ke port VGA text mode
void vga_write(const char* str) {
    volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
    int i = 0;
    
    while(str[i] != '\0') {
        // Format: 0xABCD dimana AB adalah atribut warna dan CD adalah karakter ASCII
        vga_buffer[i] = (uint16_t)str[i] | 0x0700;
        i++;
    }
}

// Fungsi utama kernel
extern "C" void kernel_main() {
    // Tampilkan pesan selamat datang
    vga_write("SimpleLiveCPP OS loaded successfully!");
    
    // Inisialisasi layanan sistem lainnya
    // ...
}