; boot.asm - ZiumOS Multiboot1 boot

[BITS 32]
global start
global mb_info_ptr
global mb_magic_val

extern kernel_main
extern __bss_start
extern __bss_end

section .text
    align 4
mb_header:
    ; Multiboot1 header (magic 0x1BADB002, flags=0, checksum makes sum=0)
    dd 0x1BADB002
    dd 0x00
    dd -(0x1BADB002 + 0x00)

start:
    mov [mb_magic_val], eax
    mov [mb_info_ptr], ebx
    cli
    mov esp, stack_top

    ; Zero .bss
    mov esi, __bss_start
    mov edi, __bss_end
    cmp esi, edi
    jae call_kernel
.zero:
    mov dword [esi], 0
    add esi, 4
    cmp esi, edi
    jb .zero

call_kernel:
    call kernel_main
    hlt
.hang:
    jmp .hang

section .bss
align 8
    mb_magic_val: resd 1
    mb_info_ptr:  resd 1
    stack_space:  resb 8192
    stack_top: