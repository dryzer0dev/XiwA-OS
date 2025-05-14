#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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

// Déclarations externes des fonctions d'initialisation
extern void init_core();
extern void init_memory();
extern void init_process_manager();
extern void init_device_manager();
extern void init_filesystem();
extern void init_network_manager();
extern void init_gui();
extern void init_audio();
extern void init_input();
extern void init_time();

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

// Fonction principale du kernel
void kernel_main() {
    // Initialisation des composants du système
    init_core();
    init_memory();
    init_process_manager();
    init_device_manager();
    init_filesystem();
    init_network_manager();
    init_gui();
    init_audio();
    init_input();
    init_time();

    // Boucle principale du kernel
    while (1) {
        // Mise à jour des composants
        update_time();
        update_input();
        update_windows();
        process_audio();
        
        // Gestion des processus
        schedule();
        
        // Attente d'interruption
        asm volatile("hlt");
    }
}

// Gestionnaire d'interruptions
void handle_interrupt(uint32_t interrupt_number, uint32_t error_code) {
    switch (interrupt_number) {
        case 0: // Division par zéro
            panic("Division par zéro");
            break;
        case 1: // Débogage
            // Gestion du débogage
            break;
        case 2: // Interruption non masquable
            panic("Interruption non masquable");
            break;
        case 3: // Point d'arrêt
            // Gestion du point d'arrêt
            break;
        case 4: // Débordement
            // Gestion du débordement
            break;
        case 5: // Limite de segment
            panic("Limite de segment dépassée");
            break;
        case 6: // Instruction invalide
            panic("Instruction invalide");
            break;
        case 7: // Coprocesseur non disponible
            panic("Coprocesseur non disponible");
            break;
        case 8: // Double faute
            panic("Double faute");
            break;
        case 9: // Coprocesseur segment overrun
            panic("Coprocesseur segment overrun");
            break;
        case 10: // TSS invalide
            panic("TSS invalide");
            break;
        case 11: // Segment non présent
            panic("Segment non présent");
            break;
        case 12: // Faute de pile
            panic("Faute de pile");
            break;
        case 13: // Protection générale
            panic("Protection générale");
            break;
        case 14: // Faute de page
            handle_page_fault(error_code);
            break;
        case 15: // Réservé
            panic("Interruption réservée");
            break;
        case 16: // Erreur de coprocesseur
            panic("Erreur de coprocesseur");
            break;
        default:
            // Gestion des autres interruptions
            break;
    }
}

// Gestionnaire de faute de page
void handle_page_fault(uint32_t error_code) {
    uint32_t faulting_address;
    asm volatile("movl %%cr2, %0" : "=r" (faulting_address));
    
    // Vérifie si la faute est due à une page non présente
    if (!(error_code & 1)) {
        // Alloue une nouvelle page
        if (allocate_page(faulting_address)) {
            return;
        }
    }
    
    panic("Faute de page irrécupérable");
}

// Fonction de panique
void panic(const char* message) {
    // Désactive les interruptions
    asm volatile("cli");
    
    // Affiche le message d'erreur
    // TODO: Implémenter l'affichage sur l'écran
    
    // Boucle infinie
    while (1) {
        asm volatile("hlt");
    }
} 