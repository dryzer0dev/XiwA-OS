#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_KEYS 256
#define MAX_MOUSE_BUTTONS 8
#define MAX_JOYSTICKS 4
#define MAX_JOYSTICK_BUTTONS 32
#define MAX_JOYSTICK_AXES 8
#define MAX_TOUCH_POINTS 10

typedef struct {
    bool pressed;
    bool released;
    bool held;
    uint32_t timestamp;
} key_state_t;

typedef struct {
    bool pressed;
    bool released;
    bool held;
    uint32_t timestamp;
    uint32_t x;
    uint32_t y;
} mouse_button_state_t;

typedef struct {
    bool connected;
    bool buttons[MAX_JOYSTICK_BUTTONS];
    float axes[MAX_JOYSTICK_AXES];
    uint32_t button_count;
    uint32_t axis_count;
} joystick_state_t;

typedef struct {
    bool active;
    uint32_t id;
    uint32_t x;
    uint32_t y;
    float pressure;
    uint32_t timestamp;
} touch_point_t;

typedef struct {
    key_state_t keys[MAX_KEYS];
    mouse_button_state_t mouse_buttons[MAX_MOUSE_BUTTONS];
    uint32_t mouse_x;
    uint32_t mouse_y;
    int32_t mouse_dx;
    int32_t mouse_dy;
    int32_t mouse_wheel;
    joystick_state_t joysticks[MAX_JOYSTICKS];
    touch_point_t touch_points[MAX_TOUCH_POINTS];
    uint32_t touch_point_count;
} input_t;

static input_t input;

void init_input() {
    memset(&input, 0, sizeof(input_t));
}

void update_input() {
    for (uint32_t i = 0; i < MAX_KEYS; i++) {
        input.keys[i].pressed = false;
        input.keys[i].released = false;
    }

    for (uint32_t i = 0; i < MAX_MOUSE_BUTTONS; i++) {
        input.mouse_buttons[i].pressed = false;
        input.mouse_buttons[i].released = false;
    }

    input.mouse_dx = 0;
    input.mouse_dy = 0;
    input.mouse_wheel = 0;

    for (uint32_t i = 0; i < input.touch_point_count; i++) {
        if (!input.touch_points[i].active) {
            input.touch_points[i] = input.touch_points[--input.touch_point_count];
            i--;
        }
    }
}

void handle_key_event(uint32_t key, bool pressed) {
    if (key < MAX_KEYS) {
        if (pressed) {
            if (!input.keys[key].held) {
                input.keys[key].pressed = true;
            }
            input.keys[key].held = true;
        } else {
            if (input.keys[key].held) {
                input.keys[key].released = true;
            }
            input.keys[key].held = false;
        }
        input.keys[key].timestamp = get_timestamp();
    }
}

void handle_mouse_button_event(uint32_t button, bool pressed, uint32_t x, uint32_t y) {
    if (button < MAX_MOUSE_BUTTONS) {
        if (pressed) {
            if (!input.mouse_buttons[button].held) {
                input.mouse_buttons[button].pressed = true;
            }
            input.mouse_buttons[button].held = true;
        } else {
            if (input.mouse_buttons[button].held) {
                input.mouse_buttons[button].released = true;
            }
            input.mouse_buttons[button].held = false;
        }
        input.mouse_buttons[button].x = x;
        input.mouse_buttons[button].y = y;
        input.mouse_buttons[button].timestamp = get_timestamp();
    }
}

void handle_mouse_move_event(uint32_t x, uint32_t y) {
    input.mouse_dx = x - input.mouse_x;
    input.mouse_dy = y - input.mouse_y;
    input.mouse_x = x;
    input.mouse_y = y;
}

void handle_mouse_wheel_event(int32_t delta) {
    input.mouse_wheel = delta;
}

void handle_joystick_connect_event(uint32_t joystick_id, uint32_t button_count, uint32_t axis_count) {
    if (joystick_id < MAX_JOYSTICKS) {
        joystick_state_t* joystick = &input.joysticks[joystick_id];
        joystick->connected = true;
        joystick->button_count = button_count;
        joystick->axis_count = axis_count;
    }
}

void handle_joystick_disconnect_event(uint32_t joystick_id) {
    if (joystick_id < MAX_JOYSTICKS) {
        input.joysticks[joystick_id].connected = false;
    }
}

void handle_joystick_button_event(uint32_t joystick_id, uint32_t button, bool pressed) {
    if (joystick_id < MAX_JOYSTICKS && button < MAX_JOYSTICK_BUTTONS) {
        input.joysticks[joystick_id].buttons[button] = pressed;
    }
}

void handle_joystick_axis_event(uint32_t joystick_id, uint32_t axis, float value) {
    if (joystick_id < MAX_JOYSTICKS && axis < MAX_JOYSTICK_AXES) {
        input.joysticks[joystick_id].axes[axis] = value;
    }
}

void handle_touch_event(uint32_t id, bool active, uint32_t x, uint32_t y, float pressure) {
    if (active) {
        if (input.touch_point_count < MAX_TOUCH_POINTS) {
            touch_point_t* point = &input.touch_points[input.touch_point_count++];
            point->active = true;
            point->id = id;
            point->x = x;
            point->y = y;
            point->pressure = pressure;
            point->timestamp = get_timestamp();
        }
    } else {
        for (uint32_t i = 0; i < input.touch_point_count; i++) {
            if (input.touch_points[i].id == id) {
                input.touch_points[i].active = false;
                break;
            }
        }
    }
}

bool is_key_pressed(uint32_t key) {
    return key < MAX_KEYS && input.keys[key].pressed;
}

bool is_key_released(uint32_t key) {
    return key < MAX_KEYS && input.keys[key].released;
}

bool is_key_held(uint32_t key) {
    return key < MAX_KEYS && input.keys[key].held;
}

bool is_mouse_button_pressed(uint32_t button) {
    return button < MAX_MOUSE_BUTTONS && input.mouse_buttons[button].pressed;
}

bool is_mouse_button_released(uint32_t button) {
    return button < MAX_MOUSE_BUTTONS && input.mouse_buttons[button].released;
}

bool is_mouse_button_held(uint32_t button) {
    return button < MAX_MOUSE_BUTTONS && input.mouse_buttons[button].held;
}

void get_mouse_position(uint32_t* x, uint32_t* y) {
    if (x) *x = input.mouse_x;
    if (y) *y = input.mouse_y;
}

void get_mouse_delta(int32_t* dx, int32_t* dy) {
    if (dx) *dx = input.mouse_dx;
    if (dy) *dy = input.mouse_dy;
}

int32_t get_mouse_wheel() {
    return input.mouse_wheel;
}

bool is_joystick_connected(uint32_t joystick_id) {
    return joystick_id < MAX_JOYSTICKS && input.joysticks[joystick_id].connected;
}

bool is_joystick_button_pressed(uint32_t joystick_id, uint32_t button) {
    return joystick_id < MAX_JOYSTICKS && button < MAX_JOYSTICK_BUTTONS &&
           input.joysticks[joystick_id].connected && input.joysticks[joystick_id].buttons[button];
}

float get_joystick_axis(uint32_t joystick_id, uint32_t axis) {
    return joystick_id < MAX_JOYSTICKS && axis < MAX_JOYSTICK_AXES &&
           input.joysticks[joystick_id].connected ? input.joysticks[joystick_id].axes[axis] : 0.0f;
}

uint32_t get_touch_point_count() {
    return input.touch_point_count;
}

void get_touch_point(uint32_t index, uint32_t* id, uint32_t* x, uint32_t* y, float* pressure) {
    if (index < input.touch_point_count) {
        touch_point_t* point = &input.touch_points[index];
        if (id) *id = point->id;
        if (x) *x = point->x;
        if (y) *y = point->y;
        if (pressure) *pressure = point->pressure;
    }
}

uint32_t get_timestamp() {
    return (uint32_t)(get_ticks() * 1000 / get_frequency());
} 