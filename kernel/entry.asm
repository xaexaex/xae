; ==============================================================================
; KERNEL ENTRY POINT
; ==============================================================================
; WHAT: Simple 32-bit entry point for the kernel
; WHY: Bootloader handles the mode switch, we just set up and call C code
; HOW: Set up segments, stack, and call kernel_main

[BITS 32]
[EXTERN kernel_main]
[GLOBAL kernel_entry]

section .text
align 4

kernel_entry:
    ; Disable interrupts
    cli
    
    ; Set up segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up stack
    mov esp, 0x90000
    
    ; Call C kernel
    call kernel_main
    
    ; If we return, just halt
.halt:
    hlt
    jmp .halt


