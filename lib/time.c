#include <stdint.h>
#include <stdbool.h>

#define MAX_TIMERS 32
#define MAX_ALARMS 16
#define MAX_CALLBACKS 64

typedef struct {
    uint32_t id;
    char name[64];
    uint64_t start_time;
    uint64_t duration;
    bool running;
    bool repeating;
    void (*callback)(void*);
    void* user_data;
} timer_t;

typedef struct {
    uint32_t id;
    char name[64];
    uint64_t time;
    bool enabled;
    void (*callback)(void*);
    void* user_data;
} alarm_t;

typedef struct {
    uint32_t id;
    char name[64];
    uint64_t interval;
    uint64_t last_time;
    bool running;
    void (*callback)(void*);
    void* user_data;
} callback_t;

typedef struct {
    uint64_t frequency;
    uint64_t start_time;
    timer_t timers[MAX_TIMERS];
    uint32_t timer_count;
    alarm_t alarms[MAX_ALARMS];
    uint32_t alarm_count;
    callback_t callbacks[MAX_CALLBACKS];
    uint32_t callback_count;
} time_t;

static time_t time;

void init_time() {
    memset(&time, 0, sizeof(time_t));
    time.frequency = get_frequency();
    time.start_time = get_ticks();
}

uint64_t get_current_time() {
    return (get_ticks() - time.start_time) * 1000000 / time.frequency;
}

uint64_t get_current_time_ms() {
    return get_current_time() / 1000;
}

uint64_t get_current_time_us() {
    return get_current_time();
}

uint32_t create_timer(const char* name, uint64_t duration, bool repeating,
                     void (*callback)(void*), void* user_data) {
    if (time.timer_count >= MAX_TIMERS) return 0;

    timer_t* timer = &time.timers[time.timer_count++];
    timer->id = time.timer_count;
    strncpy(timer->name, name, sizeof(timer->name) - 1);
    timer->start_time = get_current_time();
    timer->duration = duration;
    timer->running = true;
    timer->repeating = repeating;
    timer->callback = callback;
    timer->user_data = user_data;

    return timer->id;
}

void destroy_timer(uint32_t timer_id) {
    if (timer_id > 0 && timer_id <= time.timer_count) {
        time.timers[timer_id - 1] = time.timers[--time.timer_count];
    }
}

void start_timer(uint32_t timer_id) {
    if (timer_id > 0 && timer_id <= time.timer_count) {
        time.timers[timer_id - 1].running = true;
        time.timers[timer_id - 1].start_time = get_current_time();
    }
}

void stop_timer(uint32_t timer_id) {
    if (timer_id > 0 && timer_id <= time.timer_count) {
        time.timers[timer_id - 1].running = false;
    }
}

void reset_timer(uint32_t timer_id) {
    if (timer_id > 0 && timer_id <= time.timer_count) {
        time.timers[timer_id - 1].start_time = get_current_time();
    }
}

bool is_timer_running(uint32_t timer_id) {
    return timer_id > 0 && timer_id <= time.timer_count && time.timers[timer_id - 1].running;
}

uint64_t get_timer_elapsed(uint32_t timer_id) {
    if (timer_id > 0 && timer_id <= time.timer_count) {
        return get_current_time() - time.timers[timer_id - 1].start_time;
    }
    return 0;
}

uint64_t get_timer_remaining(uint32_t timer_id) {
    if (timer_id > 0 && timer_id <= time.timer_count) {
        timer_t* timer = &time.timers[timer_id - 1];
        if (timer->running) {
            uint64_t elapsed = get_current_time() - timer->start_time;
            return elapsed < timer->duration ? timer->duration - elapsed : 0;
        }
    }
    return 0;
}

uint32_t create_alarm(const char* name, uint64_t time,
                     void (*callback)(void*), void* user_data) {
    if (time.alarm_count >= MAX_ALARMS) return 0;

    alarm_t* alarm = &time.alarms[time.alarm_count++];
    alarm->id = time.alarm_count;
    strncpy(alarm->name, name, sizeof(alarm->name) - 1);
    alarm->time = time;
    alarm->enabled = true;
    alarm->callback = callback;
    alarm->user_data = user_data;

    return alarm->id;
}

void destroy_alarm(uint32_t alarm_id) {
    if (alarm_id > 0 && alarm_id <= time.alarm_count) {
        time.alarms[alarm_id - 1] = time.alarms[--time.alarm_count];
    }
}

void enable_alarm(uint32_t alarm_id) {
    if (alarm_id > 0 && alarm_id <= time.alarm_count) {
        time.alarms[alarm_id - 1].enabled = true;
    }
}

void disable_alarm(uint32_t alarm_id) {
    if (alarm_id > 0 && alarm_id <= time.alarm_count) {
        time.alarms[alarm_id - 1].enabled = false;
    }
}

bool is_alarm_enabled(uint32_t alarm_id) {
    return alarm_id > 0 && alarm_id <= time.alarm_count && time.alarms[alarm_id - 1].enabled;
}

uint32_t create_callback(const char* name, uint64_t interval,
                        void (*callback)(void*), void* user_data) {
    if (time.callback_count >= MAX_CALLBACKS) return 0;

    callback_t* cb = &time.callbacks[time.callback_count++];
    cb->id = time.callback_count;
    strncpy(cb->name, name, sizeof(cb->name) - 1);
    cb->interval = interval;
    cb->last_time = get_current_time();
    cb->running = true;
    cb->callback = callback;
    cb->user_data = user_data;

    return cb->id;
}

void destroy_callback(uint32_t callback_id) {
    if (callback_id > 0 && callback_id <= time.callback_count) {
        time.callbacks[callback_id - 1] = time.callbacks[--time.callback_count];
    }
}

void start_callback(uint32_t callback_id) {
    if (callback_id > 0 && callback_id <= time.callback_count) {
        time.callbacks[callback_id - 1].running = true;
        time.callbacks[callback_id - 1].last_time = get_current_time();
    }
}

void stop_callback(uint32_t callback_id) {
    if (callback_id > 0 && callback_id <= time.callback_count) {
        time.callbacks[callback_id - 1].running = false;
    }
}

bool is_callback_running(uint32_t callback_id) {
    return callback_id > 0 && callback_id <= time.callback_count && time.callbacks[callback_id - 1].running;
}

void update_time() {
    uint64_t current_time = get_current_time();

    for (uint32_t i = 0; i < time.timer_count; i++) {
        timer_t* timer = &time.timers[i];
        if (timer->running) {
            uint64_t elapsed = current_time - timer->start_time;
            if (elapsed >= timer->duration) {
                if (timer->callback) {
                    timer->callback(timer->user_data);
                }
                if (timer->repeating) {
                    timer->start_time = current_time;
                } else {
                    timer->running = false;
                }
            }
        }
    }

    for (uint32_t i = 0; i < time.alarm_count; i++) {
        alarm_t* alarm = &time.alarms[i];
        if (alarm->enabled && current_time >= alarm->time) {
            if (alarm->callback) {
                alarm->callback(alarm->user_data);
            }
            alarm->enabled = false;
        }
    }

    for (uint32_t i = 0; i < time.callback_count; i++) {
        callback_t* cb = &time.callbacks[i];
        if (cb->running) {
            uint64_t elapsed = current_time - cb->last_time;
            if (elapsed >= cb->interval) {
                if (cb->callback) {
                    cb->callback(cb->user_data);
                }
                cb->last_time = current_time;
            }
        }
    }
}

void sleep(uint64_t milliseconds) {
    uint64_t start_time = get_current_time_ms();
    while (get_current_time_ms() - start_time < milliseconds) {
        asm volatile("pause");
    }
}

void sleep_us(uint64_t microseconds) {
    uint64_t start_time = get_current_time_us();
    while (get_current_time_us() - start_time < microseconds) {
        asm volatile("pause");
    }
} 