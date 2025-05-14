#include <stdint.h>
#include <stddef.h>

// Définitions des couleurs VGA
#define VGA_BLACK 0
#define VGA_BLUE 1
#define VGA_GREEN 2
#define VGA_CYAN 3
#define VGA_RED 4
#define VGA_MAGENTA 5
#define VGA_BROWN 6
#define VGA_LIGHT_GREY 7
#define VGA_DARK_GREY 8
#define VGA_LIGHT_BLUE 9
#define VGA_LIGHT_GREEN 10
#define VGA_LIGHT_CYAN 11
#define VGA_LIGHT_RED 12
#define VGA_LIGHT_MAGENTA 13
#define VGA_LIGHT_BROWN 14
#define VGA_WHITE 15

// Adresse de la mémoire vidéo
volatile uint16_t* video_memory = (uint16_t*)0xB8000;

// Position du curseur
int cursor_x = 0;
int cursor_y = 0;

// Fonction pour mettre à jour le curseur matériel
void update_cursor() {
    uint16_t position = cursor_y * 80 + cursor_x;
    outb(0x3D4, 14);
    outb(0x3D5, position >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, position);
}

// Fonction pour effacer l'écran
void clear_screen() {
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i] = (VGA_BLACK << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

// Fonction pour afficher un caractère
void putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        video_memory[cursor_y * 80 + cursor_x] = (VGA_LIGHT_GREY << 8) | c;
        cursor_x++;
    }

    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= 25) {
        // Scroll
        for (int i = 0; i < 80 * 24; i++) {
            video_memory[i] = video_memory[i + 80];
        }
        for (int i = 80 * 24; i < 80 * 25; i++) {
            video_memory[i] = (VGA_BLACK << 8) | ' ';
        }
        cursor_y = 24;
    }

    update_cursor();
}

// Fonction pour afficher une chaîne
void print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        putchar(str[i]);
    }
}

// Point d'entrée du noyau
void kernel_main() {
    clear_screen();
    print("XiMonOS - Noyau initialise\n");
    print("Chargement du systeme...\n");
    
    // Boucle infinie
    while (1) {
        // Le noyau attend des interruptions
        __asm__("hlt");
    }
} 