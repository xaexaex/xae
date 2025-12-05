; ==============================================================================
; BOOTLOADER - The First Code That Runs!
; ==============================================================================
; WHAT: This is a bootloader written in x86 assembly
; WHY: When your computer starts, the BIOS loads the first 512 bytes from
;      your disk into memory at address 0x7C00 and jumps to it. This is that code!
; HOW: We set up the CPU to 16-bit mode, load our kernel from disk, then jump to it

[BITS 16]           ; Tell assembler we're in 16-bit real mode (old DOS-style mode)
[ORG 0x7C00]        ; BIOS loads us at memory address 0x7C00

start:
    ; STEP 1: Set up segment registers
    ; WHY: In 16-bit mode, memory is accessed using segment:offset pairs
    cli                 ; Disable interrupts while we set up
    xor ax, ax          ; Set AX to 0
    mov ds, ax          ; Data Segment = 0
    mov es, ax          ; Extra Segment = 0
    mov ss, ax          ; Stack Segment = 0
    mov sp, 0x7C00      ; Stack grows downward from our bootloader
    sti                 ; Re-enable interrupts

    ; STEP 2: Display a boot message so we know it's working!
    mov si, msg_boot    ; SI = pointer to our message string
    call print_string   ; Call our print function

    ; STEP 3: Load the kernel from disk into memory
    ; WHY: Our kernel is too big for 512 bytes, it's stored in the next sectors
    mov bx, 0x1000      ; Load kernel at memory address 0x1000:0000
    mov es, bx
    xor bx, bx          ; ES:BX = 0x1000:0x0000 (physical address 0x10000)

    mov ah, 0x02        ; BIOS function: Read Sectors
    mov al, 30          ; Read 30 sectors (about 15KB for our kernel with shell)
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Start at sector 2 (sector 1 is this bootloader)
    mov dh, 0           ; Head 0
    mov dl, 0x80        ; Drive 0x80 = first hard drive
    int 0x13            ; Call BIOS disk interrupt
    
    jc disk_error       ; If carry flag set, there was an error

    ; STEP 4: Switch to protected mode and jump to kernel
    mov si, msg_kernel
    call print_string
    
    cli                     ; Disable interrupts
    lgdt [gdt_descriptor]   ; Load GDT
    
    ; Enable protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Far jump to 32-bit code
    jmp 0x08:protected_mode

[BITS 32]
protected_mode:
    ; Set up segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    ; Jump to kernel (flat address 0x10000)
    jmp 0x08:0x10000

disk_error:
    mov si, msg_error
    call print_string
    hlt                 ; Halt the CPU

; ==============================================================================
; FUNCTION: print_string
; INPUT: SI = pointer to null-terminated string
; OUTPUT: Prints string to screen using BIOS
; ==============================================================================
print_string:
    pusha               ; Save all registers
.loop:
    lodsb               ; Load byte from [SI] into AL, increment SI
    or al, al           ; Check if AL is 0 (end of string)
    jz .done            ; If zero, we're done
    mov ah, 0x0E        ; BIOS teletype function
    mov bh, 0           ; Page 0
    int 0x10            ; Call BIOS video interrupt
    jmp .loop
.done:
    popa                ; Restore all registers
    ret

; Messages
msg_boot:   db 'XAE OS Booting...', 13, 10, 0
msg_kernel: db 'Loading kernel...', 13, 10, 0
msg_error:  db 'Disk read error!', 13, 10, 0

; ==============================================================================
; GDT for Protected Mode
; ==============================================================================
align 4
gdt_start:
    ; Null descriptor
    dq 0x0

    ; Code segment (0x08)
    dw 0xFFFF       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 10011010b    ; Access: present, ring 0, code, executable, readable
    db 11001111b    ; Flags: 4KB granularity, 32-bit
    db 0x00         ; Base high

    ; Data segment (0x10)
    dw 0xFFFF       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 10010010b    ; Access: present, ring 0, data, writable
    db 11001111b    ; Flags: 4KB granularity, 32-bit
    db 0x00         ; Base high

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ==============================================================================
; BOOT SIGNATURE
; WHY: The BIOS looks for 0xAA55 at the end of sector 1 to know it's bootable
; ==============================================================================
times 510-($-$$) db 0   ; Pad with zeros to byte 510
dw 0xAA55               ; Boot signature (must be at bytes 511-512)
