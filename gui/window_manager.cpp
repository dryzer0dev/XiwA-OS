#include <stdint.h>
#include <stddef.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define MAX_WINDOWS 32

typedef struct {
    int x, y;
    int width, height;
    char title[64];
    uint32_t* buffer;
    bool is_visible;
    bool is_focused;
    void (*draw_callback)(void*);
    void* user_data;
} window_t;

window_t* windows[MAX_WINDOWS];
int current_window = 0;

uint32_t* screen_buffer;

window_t* create_window(const char* title, int x, int y, int width, int height) {
    int slot = -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == 0) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        return 0;
    }

    // Allouer la structure de la fenêtre
    window_t* window = (window_t*)kmalloc(sizeof(window_t));
    if (!window) {
        return 0;
    }

    // Initialiser la fenêtre
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    strcpy(window->title, title);
    window->buffer = (uint32_t*)kmalloc(width * height * sizeof(uint32_t));
    window->is_visible = true;
    window->is_focused = false;
    window->draw_callback = 0;
    window->user_data = 0;

    // Sauvegarder la fenêtre
    windows[slot] = window;

    return window;
}

// Fonction pour dessiner une fenêtre
void draw_window(window_t* window) {
    if (!window || !window->is_visible) {
        return;
    }

    // Dessiner le fond
    for (int y = 0; y < window->height; y++) {
        for (int x = 0; x < window->width; x++) {
            window->buffer[y * window->width + x] = 0xFFFFFF; // Blanc
        }
    }

    // Dessiner la barre de titre
    for (int x = 0; x < window->width; x++) {
        window->buffer[x] = window->is_focused ? 0x0000FF : 0x808080; // Bleu si focus, gris sinon
    }

    // Dessiner le titre
    draw_text(window->buffer, window->width, 5, 5, window->title, 0xFFFFFF);

    // Appeler le callback de dessin si présent
    if (window->draw_callback) {
        window->draw_callback(window->user_data);
    }

    // Copier la fenêtre sur l'écran
    for (int y = 0; y < window->height; y++) {
        for (int x = 0; x < window->width; x++) {
            if (window->x + x >= 0 && window->x + x < SCREEN_WIDTH &&
                window->y + y >= 0 && window->y + y < SCREEN_HEIGHT) {
                screen_buffer[(window->y + y) * SCREEN_WIDTH + (window->x + x)] = 
                    window->buffer[y * window->width + x];
            }
        }
    }
}

// Fonction pour mettre à jour l'écran
void update_screen() {
    // Effacer l'écran
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        screen_buffer[i] = 0x000000; // Noir
    }

    // Dessiner toutes les fenêtres visibles
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] && windows[i]->is_visible) {
            draw_window(windows[i]);
        }
    }

    // Mettre à jour l'affichage
    update_display(screen_buffer);
}

// Fonction pour gérer les événements de la souris
void handle_mouse_event(int x, int y, int buttons) {
    // Trouver la fenêtre sous le curseur
    window_t* target = 0;
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        if (windows[i] && windows[i]->is_visible) {
            if (x >= windows[i]->x && x < windows[i]->x + windows[i]->width &&
                y >= windows[i]->y && y < windows[i]->y + windows[i]->height) {
                target = windows[i];
                break;
            }
        }
    }

    // Mettre à jour le focus
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i]) {
            windows[i]->is_focused = (windows[i] == target);
        }
    }

    // Mettre à jour l'affichage
    update_screen();
}

// Fonction pour initialiser le gestionnaire de fenêtres
void init_window_manager() {
    // Allouer le buffer de l'écran
    screen_buffer = (uint32_t*)kmalloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));

    // Initialiser la table des fenêtres
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i] = 0;
    }

    // Créer la fenêtre de bureau
    create_window("Bureau", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
} 