[BITS 32]
[ORG 0x7C00]

; Configuration initiale
cli                     ; Désactive les interruptions
mov ax, 0x0000         ; Initialise les segments
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7C00         ; Initialise le pointeur de pile

; Charge le kernel
mov ah, 0x02           ; Fonction de lecture disque
mov al, 1              ; Nombre de secteurs à lire
mov ch, 0              ; Cylindre 0
mov cl, 2              ; Secteur 2 (après le bootloader)
mov dh, 0              ; Tête 0
mov dl, 0x80           ; Premier disque dur
mov bx, 0x1000         ; Adresse de chargement
int 0x13               ; Interruption BIOS pour lecture disque

; Passe en mode protégé
lgdt [gdt_descriptor]  ; Charge le GDT
mov eax, cr0           ; Active le mode protégé
or eax, 1
mov cr0, eax

; Saut vers le code 32 bits
jmp 0x08:protected_mode

[BITS 32]
protected_mode:
    ; Initialise les segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Saut vers le kernel
    jmp 0x1000

; GDT
gdt_start:
    ; Entrée nulle
    dd 0x0
    dd 0x0

    ; Segment de code
    dw 0xFFFF          ; Limite
    dw 0x0000          ; Base
    db 0x00            ; Base
    db 10011010b       ; Flags
    db 11001111b       ; Flags + Limite
    db 0x00            ; Base

    ; Segment de données
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

times 510-($-$$) db 0  ; Remplit le reste du secteur avec des zéros
dw 0xAA55              ; Signature de boot 