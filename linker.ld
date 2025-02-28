ENTRY(start)

SECTIONS {
    . = 1M;  /* Kernel dimulai dari alamat 1MB */
    
    .text : ALIGN(4K) {
        *(.text)
    }
    
    .rodata : ALIGN(4K) {
        *(.rodata)
    }
    
    .data : ALIGN(4K) {
        *(.data)
    }
    
    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}