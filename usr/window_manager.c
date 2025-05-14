#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_WINDOWS 32
#define WINDOW_TITLE_HEIGHT 32
#define WINDOW_BORDER_WIDTH 1
#define WINDOW_MIN_WIDTH 200
#define WINDOW_MIN_HEIGHT 150

typedef struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    char title[64];
    bool is_active;
    bool is_minimized;
    bool is_maximized;
    uint32_t window_id;
    void* buffer;
    uint32_t flags;
} Window;

typedef struct {
    Window windows[MAX_WINDOWS];
    uint32_t window_count;
    Window* active_window;
    uint32_t next_window_id;
} WindowManager;

WindowManager wm;

void init_window_manager() {
    memset(&wm, 0, sizeof(WindowManager));
    wm.next_window_id = 1;
}

Window* create_window(const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (wm.window_count >= MAX_WINDOWS) {
        return NULL;
    }

    Window* window = &wm.windows[wm.window_count++];
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    strncpy(window->title, title, 63);
    window->title[63] = '\0';
    window->is_active = false;
    window->is_minimized = false;
    window->is_maximized = false;
    window->window_id = wm.next_window_id++;
    window->buffer = kmalloc(width * height * 4);
    window->flags = 0;

    return window;
}

void draw_window(Window* window) {
    if (!window || !window->buffer) return;

    // Dessiner la bordure
    draw_rect(window->x - WINDOW_BORDER_WIDTH, 
             window->y - WINDOW_TITLE_HEIGHT - WINDOW_BORDER_WIDTH,
             window->width + 2 * WINDOW_BORDER_WIDTH,
             window->height + WINDOW_TITLE_HEIGHT + 2 * WINDOW_BORDER_WIDTH,
             0x2C2C2C);

    // Dessiner la barre de titre
    draw_rect(window->x, window->y - WINDOW_TITLE_HEIGHT,
             window->width, WINDOW_TITLE_HEIGHT,
             window->is_active ? 0x3C3C3C : 0x2C2C2C);

    // Dessiner le titre
    draw_text(window->x + 10, window->y - WINDOW_TITLE_HEIGHT + 8,
             window->title, 0xFFFFFF);

    // Dessiner les boutons de contrôle
    draw_window_controls(window);

    // Dessiner le contenu de la fenêtre
    draw_window_content(window);
}

void draw_window_controls(Window* window) {
    int button_size = 16;
    int button_spacing = 4;
    int start_x = window->x + window->width - (button_size + button_spacing) * 3;

    // Bouton Minimiser
    draw_rect(start_x, window->y - WINDOW_TITLE_HEIGHT + 8,
             button_size, button_size, 0x4CAF50);

    // Bouton Maximiser
    draw_rect(start_x + button_size + button_spacing,
             window->y - WINDOW_TITLE_HEIGHT + 8,
             button_size, button_size, 0xFFC107);

    // Bouton Fermer
    draw_rect(start_x + (button_size + button_spacing) * 2,
             window->y - WINDOW_TITLE_HEIGHT + 8,
             button_size, button_size, 0xF44336);
}

void draw_window_content(Window* window) {
    if (window->is_minimized) return;

    // Copier le buffer de la fenêtre vers l'écran
    memcpy(get_framebuffer() + (window->y * SCREEN_WIDTH + window->x) * 4,
           window->buffer,
           window->width * window->height * 4);
}

void handle_window_event(Window* window, uint32_t event_type, int32_t x, int32_t y) {
    switch (event_type) {
        case EVENT_MOUSE_DOWN:
            if (is_in_title_bar(window, x, y)) {
                start_window_drag(window, x, y);
            } else if (is_in_control_buttons(window, x, y)) {
                handle_control_button_click(window, x, y);
            }
            break;

        case EVENT_MOUSE_MOVE:
            if (window->is_dragging) {
                move_window(window, x - window->drag_offset_x,
                          y - window->drag_offset_y);
            }
            break;

        case EVENT_MOUSE_UP:
            window->is_dragging = false;
            break;
    }
}

bool is_in_title_bar(Window* window, int32_t x, int32_t y) {
    return x >= window->x && x < window->x + window->width &&
           y >= window->y - WINDOW_TITLE_HEIGHT && y < window->y;
}

void start_window_drag(Window* window, int32_t x, int32_t y) {
    window->is_dragging = true;
    window->drag_offset_x = x - window->x;
    window->drag_offset_y = y - window->y;
    set_active_window(window);
}

void move_window(Window* window, int32_t new_x, int32_t new_y) {
    window->x = new_x;
    window->y = new_y;
    request_redraw();
}

void set_active_window(Window* window) {
    if (wm.active_window) {
        wm.active_window->is_active = false;
    }
    window->is_active = true;
    wm.active_window = window;
    request_redraw();
}

void minimize_window(Window* window) {
    window->is_minimized = true;
    request_redraw();
}

void maximize_window(Window* window) {
    window->is_maximized = !window->is_maximized;
    if (window->is_maximized) {
        window->prev_x = window->x;
        window->prev_y = window->y;
        window->prev_width = window->width;
        window->prev_height = window->height;
        window->x = 0;
        window->y = WINDOW_TITLE_HEIGHT;
        window->width = SCREEN_WIDTH;
        window->height = SCREEN_HEIGHT - WINDOW_TITLE_HEIGHT;
    } else {
        window->x = window->prev_x;
        window->y = window->prev_y;
        window->width = window->prev_width;
        window->height = window->prev_height;
    }
    request_redraw();
}

void close_window(Window* window) {
    for (uint32_t i = 0; i < wm.window_count; i++) {
        if (&wm.windows[i] == window) {
            kfree(window->buffer);
            memmove(&wm.windows[i], &wm.windows[i + 1],
                   (wm.window_count - i - 1) * sizeof(Window));
            wm.window_count--;
            if (wm.active_window == window) {
                wm.active_window = NULL;
            }
            request_redraw();
            break;
        }
    }
} 