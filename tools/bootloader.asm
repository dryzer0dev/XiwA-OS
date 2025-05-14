[BITS 16]           ; Mode 16 bits
[ORG 0x7C00]        ; Point d'entrée du bootloader

; Configuration initiale
mov ax, 0x07C0      ; Segment de données
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7C00      ; Pile

; Message de démarrage
mov si, message
call print_string

; Chargement du noyau
mov ah, 0x02        ; Fonction de lecture disque
mov al, 1           ; Nombre de secteurs à lire
mov ch, 0           ; Cylindre 0
mov cl, 2           ; Secteur 2
mov dh, 0           ; Tête 0
mov dl, 0x80        ; Disque dur
mov bx, 0x1000      ; Adresse de chargement
int 0x13

; Passage en mode protégé
cli                 ; Désactive les interruptions
lgdt [gdt_descriptor] ; Charge la GDT

; Active le mode protégé
mov eax, cr0
or eax, 1
mov cr0, eax

; Saut en mode protégé
jmp 0x08:protected_mode

; Sous-routine d'affichage
print_string:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

; Données
message: db 'XiMonOS - Chargement...', 0

; GDT (Global Descriptor Table)
gdt_start:
    ; Entrée nulle
    dd 0x0
    dd 0x0

    ; Segment de code
    dw 0xFFFF       ; Limite
    dw 0x0000       ; Base
    db 0x00         ; Base
    db 10011010b    ; Flags
    db 11001111b    ; Flags + Limite
    db 0x00         ; Base

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

[BITS 32]
protected_mode:
    ; Configuration des segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Saut vers le noyau
    jmp 0x08:0x1000

; Padding et signature
times 510-($-$$) db 0
dw 0xAA55 