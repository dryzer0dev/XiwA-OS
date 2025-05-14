#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_DEVICES 64
#define MAX_DRIVERS 32
#define MAX_IRQ_HANDLERS 16
#define MAX_DMA_CHANNELS 8

typedef enum {
    DEVICE_TYPE_CHAR,
    DEVICE_TYPE_BLOCK,
    DEVICE_TYPE_NETWORK,
    DEVICE_TYPE_DISPLAY,
    DEVICE_TYPE_KEYBOARD,
    DEVICE_TYPE_MOUSE,
    DEVICE_TYPE_SOUND,
    DEVICE_TYPE_UNKNOWN
} device_type_t;

typedef enum {
    DEVICE_STATE_UNKNOWN,
    DEVICE_STATE_READY,
    DEVICE_STATE_BUSY,
    DEVICE_STATE_ERROR
} device_state_t;

typedef struct {
    uint32_t id;
    device_type_t type;
    char name[32];
    device_state_t state;
    void* driver_data;
    bool initialized;
} device_t;

typedef struct {
    char name[32];
    device_type_t type;
    bool (*init)(device_t* device);
    void (*deinit)(device_t* device);
    int (*read)(device_t* device, void* buffer, size_t size, size_t offset);
    int (*write)(device_t* device, const void* buffer, size_t size, size_t offset);
    int (*ioctl)(device_t* device, uint32_t request, void* arg);
} driver_t;

typedef struct {
    uint8_t irq;
    void (*handler)(void*);
    void* data;
} irq_handler_t;

typedef struct {
    device_t devices[MAX_DEVICES];
    driver_t drivers[MAX_DRIVERS];
    irq_handler_t irq_handlers[MAX_IRQ_HANDLERS];
    uint32_t device_count;
    uint32_t driver_count;
    uint32_t irq_handler_count;
} device_manager_t;

device_manager_t device_manager;

void init_device_manager() {
    memset(&device_manager, 0, sizeof(device_manager_t));
}

bool register_driver(const driver_t* driver) {
    if (device_manager.driver_count >= MAX_DRIVERS) {
        return false;
    }

    memcpy(&device_manager.drivers[device_manager.driver_count++], driver, sizeof(driver_t));
    return true;
}

bool register_device(device_t* device) {
    if (device_manager.device_count >= MAX_DEVICES) {
        return false;
    }

    device->id = device_manager.device_count;
    memcpy(&device_manager.devices[device_manager.device_count++], device, sizeof(device_t));
    return true;
}

void unregister_device(uint32_t device_id) {
    if (device_id >= device_manager.device_count) {
        return;
    }

    device_t* device = &device_manager.devices[device_id];
    if (device->initialized) {
        // Trouver le pilote correspondant
        for (uint32_t i = 0; i < device_manager.driver_count; i++) {
            if (device_manager.drivers[i].type == device->type) {
                device_manager.drivers[i].deinit(device);
                break;
            }
        }
    }

    // Supprimer le périphérique
    memmove(&device_manager.devices[device_id],
            &device_manager.devices[device_id + 1],
            (device_manager.device_count - device_id - 1) * sizeof(device_t));
    device_manager.device_count--;
}

device_t* find_device_by_id(uint32_t device_id) {
    if (device_id >= device_manager.device_count) {
        return NULL;
    }
    return &device_manager.devices[device_id];
}

device_t* find_device_by_type(device_type_t type) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].type == type) {
            return &device_manager.devices[i];
        }
    }
    return NULL;
}

bool initialize_device(device_t* device) {
    if (!device || device->initialized) {
        return false;
    }

    // Trouver le pilote correspondant
    for (uint32_t i = 0; i < device_manager.driver_count; i++) {
        if (device_manager.drivers[i].type == device->type) {
            if (device_manager.drivers[i].init(device)) {
                device->initialized = true;
                device->state = DEVICE_STATE_READY;
                return true;
            }
            return false;
        }
    }
    return false;
}

void deinitialize_device(device_t* device) {
    if (!device || !device->initialized) {
        return;
    }

    // Trouver le pilote correspondant
    for (uint32_t i = 0; i < device_manager.driver_count; i++) {
        if (device_manager.drivers[i].type == device->type) {
            device_manager.drivers[i].deinit(device);
            device->initialized = false;
            device->state = DEVICE_STATE_UNKNOWN;
            break;
        }
    }
}

int read_device(device_t* device, void* buffer, size_t size, size_t offset) {
    if (!device || !device->initialized || !buffer || size == 0) {
        return -1;
    }

    // Trouver le pilote correspondant
    for (uint32_t i = 0; i < device_manager.driver_count; i++) {
        if (device_manager.drivers[i].type == device->type) {
            return device_manager.drivers[i].read(device, buffer, size, offset);
        }
    }
    return -1;
}

int write_device(device_t* device, const void* buffer, size_t size, size_t offset) {
    if (!device || !device->initialized || !buffer || size == 0) {
        return -1;
    }

    // Trouver le pilote correspondant
    for (uint32_t i = 0; i < device_manager.driver_count; i++) {
        if (device_manager.drivers[i].type == device->type) {
            return device_manager.drivers[i].write(device, buffer, size, offset);
        }
    }
    return -1;
}

int ioctl_device(device_t* device, uint32_t request, void* arg) {
    if (!device || !device->initialized) {
        return -1;
    }

    // Trouver le pilote correspondant
    for (uint32_t i = 0; i < device_manager.driver_count; i++) {
        if (device_manager.drivers[i].type == device->type) {
            return device_manager.drivers[i].ioctl(device, request, arg);
        }
    }
    return -1;
}

bool register_irq_handler(uint8_t irq, void (*handler)(void*), void* data) {
    if (device_manager.irq_handler_count >= MAX_IRQ_HANDLERS) {
        return false;
    }

    irq_handler_t* irq_handler = &device_manager.irq_handlers[device_manager.irq_handler_count++];
    irq_handler->irq = irq;
    irq_handler->handler = handler;
    irq_handler->data = data;

    // Configurer le PIC
    if (irq < 8) {
        outb(0x21, inb(0x21) & ~(1 << irq));
    } else {
        outb(0xA1, inb(0xA1) & ~(1 << (irq - 8)));
    }

    return true;
}

void unregister_irq_handler(uint8_t irq) {
    for (uint32_t i = 0; i < device_manager.irq_handler_count; i++) {
        if (device_manager.irq_handlers[i].irq == irq) {
            // Désactiver l'IRQ dans le PIC
            if (irq < 8) {
                outb(0x21, inb(0x21) | (1 << irq));
            } else {
                outb(0xA1, inb(0xA1) | (1 << (irq - 8)));
            }

            // Supprimer le gestionnaire
            memmove(&device_manager.irq_handlers[i],
                    &device_manager.irq_handlers[i + 1],
                    (device_manager.irq_handler_count - i - 1) * sizeof(irq_handler_t));
            device_manager.irq_handler_count--;
            break;
        }
    }
}

void handle_device_interrupt(uint8_t irq) {
    for (uint32_t i = 0; i < device_manager.irq_handler_count; i++) {
        if (device_manager.irq_handlers[i].irq == irq) {
            device_manager.irq_handlers[i].handler(device_manager.irq_handlers[i].data);
            break;
        }
    }

    // Envoyer l'EOI au PIC
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

// Fonctions utilitaires pour les périphériques courants
device_t* get_keyboard_device() {
    return find_device_by_type(DEVICE_TYPE_KEYBOARD);
}

device_t* get_mouse_device() {
    return find_device_by_type(DEVICE_TYPE_MOUSE);
}

device_t* get_display_device() {
    return find_device_by_type(DEVICE_TYPE_DISPLAY);
}

device_t* get_disk_device() {
    return find_device_by_type(DEVICE_TYPE_BLOCK);
}

device_t* get_network_device() {
    return find_device_by_type(DEVICE_TYPE_NETWORK);
}

device_t* get_sound_device() {
    return find_device_by_type(DEVICE_TYPE_SOUND);
}

// Gestionnaire de ressources pour les périphériques

typedef struct {
    uint32_t start;
    uint32_t end;
    uint32_t type;
    bool is_used;
} resource_t;

#define MAX_RESOURCES 32
#define RESOURCE_TYPE_IO 1
#define RESOURCE_TYPE_MEMORY 2
#define RESOURCE_TYPE_IRQ 3
#define RESOURCE_TYPE_DMA 4

resource_t resources[MAX_RESOURCES];
uint32_t resource_count = 0;

void register_resource(uint32_t start, uint32_t end, uint32_t type) {
    if (resource_count >= MAX_RESOURCES) return;

    resources[resource_count].start = start;
    resources[resource_count].end = end;
    resources[resource_count].type = type;
    resources[resource_count].is_used = false;
    resource_count++;
}

bool allocate_resource(uint32_t type, uint32_t size, uint32_t* start) {
    for (uint32_t i = 0; i < resource_count; i++) {
        if (resources[i].type == type && !resources[i].is_used) {
            uint32_t available_size = resources[i].end - resources[i].start;
            if (available_size >= size) {
                resources[i].is_used = true;
                *start = resources[i].start;
                return true;
            }
        }
    }
    return false;
}

void free_resource(uint32_t start) {
    for (uint32_t i = 0; i < resource_count; i++) {
        if (resources[i].start == start) {
            resources[i].is_used = false;
            break;
        }
    }
} 