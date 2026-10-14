; isr.asm — ZiumOS IRQ dispatcher stub
; isr_stub is installed for IRQs 0x20-0x2F in the IDT.
[BITS 32]
global isr_stub
extern handle_pic

section .text
isr_stub:
    cli
    pushad
    pushfd

    ; Determine active IRQ by reading ISR from PICs
    mov dx, 0x20
    in al, dx               ; master ISR
    xor ecx, ecx
    mov ebx, 1
.find_master:
    test al, bl
    jnz .found_master
    shl bl, 1
    inc ecx
    cmp ecx, 8
    jl .find_master

    mov dx, 0xA0
    in al, dx               ; slave ISR
    xor ecx, ecx
    mov ebx, 1
.find_slave:
    test al, bl
    jnz .found_slave
    shl bl, 1
    inc ecx
    cmp ecx, 8
    jl .find_slave
    jmp .done

.found_master:
    push ecx
    call handle_pic
    add esp, 4
    mov al, 0x20
    out 0x20, al          ; EOI master
    jmp .done

.found_slave:
    lea ecx, [ecx + 8]
    push ecx
    call handle_pic
    add esp, 4
    mov al, 0x20
    out 0xA0, al          ; EOI slave
    out 0x20, al          ; EOI master (cascade)

.done:
    popfd
    popad
    sti
    iret
