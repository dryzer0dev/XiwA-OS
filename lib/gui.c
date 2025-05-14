#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define BPP 32
#define MAX_WINDOWS 32
#define MAX_WIDGETS 256
#define MAX_ANIMATIONS 64

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t color;
    bool visible;
    bool focused;
    void (*draw)(void* widget);
    void (*update)(void* widget);
    void (*handle_event)(void* widget, uint32_t event);
} widget_t;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    char title[64];
    bool visible;
    bool focused;
    widget_t* widgets;
    uint32_t widget_count;
    void (*draw)(void* window);
    void (*update)(void* window);
    void (*handle_event)(void* window, uint32_t event);
} window_t;

typedef struct {
    uint32_t start_x;
    uint32_t start_y;
    uint32_t end_x;
    uint32_t end_y;
    uint32_t duration;
    uint32_t elapsed;
    void (*update)(void* animation);
    void (*complete)(void* animation);
} animation_t;

typedef struct {
    uint32_t* framebuffer;
    window_t windows[MAX_WINDOWS];
    uint32_t window_count;
    animation_t animations[MAX_ANIMATIONS];
    uint32_t animation_count;
    uint32_t focused_window;
    uint32_t mouse_x;
    uint32_t mouse_y;
    bool mouse_down;
} gui_t;

static gui_t gui;

void init_gui() {
    memset(&gui, 0, sizeof(gui_t));
    gui.framebuffer = (uint32_t*)kmalloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
}

void draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        gui.framebuffer[y * SCREEN_WIDTH + x] = color;
    }
}

void draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            draw_pixel(x + j, y + i, color);
        }
    }
}

void draw_circle(uint32_t x, uint32_t y, uint32_t radius, uint32_t color) {
    for (uint32_t i = 0; i < radius * 2; i++) {
        for (uint32_t j = 0; j < radius * 2; j++) {
            uint32_t dx = i - radius;
            uint32_t dy = j - radius;
            if (dx * dx + dy * dy <= radius * radius) {
                draw_pixel(x + dx, y + dy, color);
            }
        }
    }
}

void draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    int32_t ux = (dx > 0) ? 1 : -1;
    int32_t uy = (dy > 0) ? 1 : -1;
    int32_t x = x1;
    int32_t y = y1;
    int32_t eps;

    eps = 0;
    dx = abs(dx);
    dy = abs(dy);

    if (dx > dy) {
        for (x = x1; x != x2; x += ux) {
            draw_pixel(x, y, color);
            eps += dy;
            if ((eps << 1) >= dx) {
                y += uy;
                eps -= dx;
            }
        }
    } else {
        for (y = y1; y != y2; y += uy) {
            draw_pixel(x, y, color);
            eps += dx;
            if ((eps << 1) >= dy) {
                x += ux;
                eps -= dy;
            }
        }
    }
}

void draw_text(uint32_t x, uint32_t y, const char* text, uint32_t color) {
    for (uint32_t i = 0; text[i]; i++) {
        uint8_t c = text[i];
        for (uint32_t j = 0; j < 8; j++) {
            for (uint32_t k = 0; k < 8; k++) {
                if (font_data[c * 8 + j] & (1 << k)) {
                    draw_pixel(x + i * 8 + k, y + j, color);
                }
            }
        }
    }
}

uint32_t create_window(const char* title, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (gui.window_count >= MAX_WINDOWS) return 0;

    window_t* window = &gui.windows[gui.window_count++];
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    strncpy(window->title, title, sizeof(window->title) - 1);
    window->visible = true;
    window->focused = false;
    window->widgets = (widget_t*)kmalloc(MAX_WIDGETS * sizeof(widget_t));
    window->widget_count = 0;

    return gui.window_count;
}

void destroy_window(uint32_t window_id) {
    if (window_id > 0 && window_id <= gui.window_count) {
        window_t* window = &gui.windows[window_id - 1];
        kfree(window->widgets);
        gui.windows[window_id - 1] = gui.windows[--gui.window_count];
    }
}

uint32_t add_widget(uint32_t window_id, widget_t* widget) {
    if (window_id > 0 && window_id <= gui.window_count) {
        window_t* window = &gui.windows[window_id - 1];
        if (window->widget_count < MAX_WIDGETS) {
            window->widgets[window->widget_count++] = *widget;
            return window->widget_count;
        }
    }
    return 0;
}

void remove_widget(uint32_t window_id, uint32_t widget_id) {
    if (window_id > 0 && window_id <= gui.window_count) {
        window_t* window = &gui.windows[window_id - 1];
        if (widget_id > 0 && widget_id <= window->widget_count) {
            window->widgets[widget_id - 1] = window->widgets[--window->widget_count];
        }
    }
}

void focus_window(uint32_t window_id) {
    if (window_id > 0 && window_id <= gui.window_count) {
        gui.windows[gui.focused_window].focused = false;
        gui.focused_window = window_id - 1;
        gui.windows[gui.focused_window].focused = true;
    }
}

void move_window(uint32_t window_id, uint32_t x, uint32_t y) {
    if (window_id > 0 && window_id <= gui.window_count) {
        window_t* window = &gui.windows[window_id - 1];
        window->x = x;
        window->y = y;
    }
}

void resize_window(uint32_t window_id, uint32_t width, uint32_t height) {
    if (window_id > 0 && window_id <= gui.window_count) {
        window_t* window = &gui.windows[window_id - 1];
        window->width = width;
        window->height = height;
    }
}

void show_window(uint32_t window_id) {
    if (window_id > 0 && window_id <= gui.window_count) {
        gui.windows[window_id - 1].visible = true;
    }
}

void hide_window(uint32_t window_id) {
    if (window_id > 0 && window_id <= gui.window_count) {
        gui.windows[window_id - 1].visible = false;
    }
}

uint32_t create_animation(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y,
                         uint32_t duration, void (*update)(void*), void (*complete)(void*)) {
    if (gui.animation_count >= MAX_ANIMATIONS) return 0;

    animation_t* animation = &gui.animations[gui.animation_count++];
    animation->start_x = start_x;
    animation->start_y = start_y;
    animation->end_x = end_x;
    animation->end_y = end_y;
    animation->duration = duration;
    animation->elapsed = 0;
    animation->update = update;
    animation->complete = complete;

    return gui.animation_count;
}

void update_animations() {
    for (uint32_t i = 0; i < gui.animation_count; i++) {
        animation_t* animation = &gui.animations[i];
        animation->elapsed++;
        if (animation->update) {
            animation->update(animation);
        }
        if (animation->elapsed >= animation->duration) {
            if (animation->complete) {
                animation->complete(animation);
            }
            gui.animations[i] = gui.animations[--gui.animation_count];
            i--;
        }
    }
}

void draw_windows() {
    for (uint32_t i = 0; i < gui.window_count; i++) {
        window_t* window = &gui.windows[i];
        if (window->visible) {
            draw_rect(window->x, window->y, window->width, window->height, 0xFFFFFF);
            draw_rect(window->x, window->y, window->width, 20, 0x0000FF);
            draw_text(window->x + 5, window->y + 5, window->title, 0xFFFFFF);

            for (uint32_t j = 0; j < window->widget_count; j++) {
                widget_t* widget = &window->widgets[j];
                if (widget->visible) {
                    if (widget->draw) {
                        widget->draw(widget);
                    }
                }
            }
        }
    }
}

void update_windows() {
    for (uint32_t i = 0; i < gui.window_count; i++) {
        window_t* window = &gui.windows[i];
        if (window->visible) {
            for (uint32_t j = 0; j < window->widget_count; j++) {
                widget_t* widget = &window->widgets[j];
                if (widget->visible && widget->update) {
                    widget->update(widget);
                }
            }
            if (window->update) {
                window->update(window);
            }
        }
    }
}

void handle_mouse_event(uint32_t x, uint32_t y, bool down) {
    gui.mouse_x = x;
    gui.mouse_y = y;
    gui.mouse_down = down;

    for (uint32_t i = 0; i < gui.window_count; i++) {
        window_t* window = &gui.windows[i];
        if (window->visible) {
            if (x >= window->x && x < window->x + window->width &&
                y >= window->y && y < window->y + window->height) {
                focus_window(i + 1);
                for (uint32_t j = 0; j < window->widget_count; j++) {
                    widget_t* widget = &window->widgets[j];
                    if (widget->visible) {
                        if (x >= window->x + widget->x && x < window->x + widget->x + widget->width &&
                            y >= window->y + widget->y && y < window->y + widget->y + widget->height) {
                            if (widget->handle_event) {
                                widget->handle_event(widget, down ? 1 : 2);
                            }
                        }
                    }
                }
                if (window->handle_event) {
                    window->handle_event(window, down ? 1 : 2);
                }
                break;
            }
        }
    }
}

void handle_keyboard_event(uint32_t key) {
    if (gui.focused_window < gui.window_count) {
        window_t* window = &gui.windows[gui.focused_window];
        if (window->handle_event) {
            window->handle_event(window, key);
        }
    }
}

void render() {
    memset(gui.framebuffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    update_animations();
    update_windows();
    draw_windows();
} 