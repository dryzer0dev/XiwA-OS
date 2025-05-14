[BITS 16]           ; Mode 16 bits
[ORG 0x7C00]        ; Adresse de chargement

; Configuration initiale
cli                 ; Désactive les interruptions
mov ax, 0x0000      ; Initialise les segments
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7C00      ; Initialise la pile
sti                 ; Réactive les interruptions

; Affiche le logo XiwA
mov ah, 0x0E        ; Fonction BIOS pour afficher un caractère
mov si, logo        ; Charge l'adresse du logo
print_logo:
    lodsb           ; Charge le caractère suivant
    test al, al     ; Vérifie si c'est la fin de la chaîne
    jz load_kernel  ; Si oui, charge le kernel
    int 0x10        ; Sinon, affiche le caractère
    jmp print_logo  ; Continue la boucle

; Charge le kernel
load_kernel:
    mov ah, 0x02    ; Fonction BIOS pour lire les secteurs
    mov al, 1       ; Nombre de secteurs à lire
    mov ch, 0       ; Cylindre 0
    mov cl, 2       ; Secteur 2 (après le bootloader)
    mov dh, 0       ; Tête 0
    mov dl, 0x80    ; Premier disque dur
    mov bx, 0x1000  ; Adresse de chargement du kernel
    int 0x13        ; Appel BIOS
    jc error        ; Si erreur, affiche message d'erreur

; Passe en mode protégé
cli                 ; Désactive les interruptions
lgdt [gdt_descriptor] ; Charge la GDT

; Active le mode protégé
mov eax, cr0
or eax, 1
mov cr0, eax

; Saut en mode protégé
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

; GDT (Global Descriptor Table)
gdt_start:
    ; Entrée nulle
    dd 0x0
    dd 0x0

    ; Segment de code
    dw 0xffff       ; Limite
    dw 0x0000       ; Base
    db 0x00         ; Base
    db 10011010b    ; Flags
    db 11001111b    ; Flags + Limite
    db 0x00         ; Base

    ; Segment de données
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; Données
logo:
    db "    ██╗  ██╗██╗██╗    ██╗ █████╗     ██████╗ ███████╗", 0x0A, 0x0D
    db "    ██║  ██║██║██║    ██║██╔══██╗    ██╔══██╗██╔════╝", 0x0A, 0x0D
    db "    ███████║██║██║    ██║███████║    ██║  ██║███████╗", 0x0A, 0x0D
    db "    ██╔══██║██║██║    ██║██╔══██║    ██║  ██║╚════██║", 0x0A, 0x0D
    db "    ██║  ██║██║██║    ██║██║  ██║    ██████╔╝███████║", 0x0A, 0x0D
    db "    ╚═╝  ╚═╝╚═╝╚═╝    ╚═╝╚═╝  ╚═╝    ╚═════╝ ╚══════╝", 0x0A, 0x0D
    db "    Chargement du systeme...", 0x0A, 0x0D, 0x00

error_msg:
    db "Erreur de chargement du kernel!", 0x0A, 0x0D, 0x00

error:
    mov ah, 0x0E
    mov si, error_msg
print_error:
    lodsb
    test al, al
    jz halt
    int 0x10
    jmp print_error

halt:
    cli
    hlt

; Padding et signature
times 510-($-$$) db 0
dw 0xAA55 