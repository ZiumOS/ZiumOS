; Boot.asm - Entry point untuk bootloader

[BITS 32]
global start
global kernel_early

extern kernel_entry  ; Fungsi dalam kernel.cpp

section .text
    ; Multiboot header untuk GRUB
    align 4
    dd 0x1BADB002               ; Magic number
    dd 0x00                     ; Flags
    dd - (0x1BADB002 + 0x00)    ; Checksum

start:
    cli                         ; Matikan interrupt
    mov esp, stack_space        ; Set stack pointer
    call kernel_entry           ; Panggil kernel
    hlt                         ; Halt CPU

kernel_early:
    ; Setup awal hardware
    ; Misalnya setup GDT, IDT, dsb.
    ret

section .bss
    resb 8192                   ; 8KB untuk stack
    stack_space: