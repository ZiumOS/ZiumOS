ENTRY(start)

SECTIONS {
    . = 1M;
    __bss_start = .;
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
    __bss_end = .;
}
