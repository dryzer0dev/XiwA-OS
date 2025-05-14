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
    DEVICE_TYPE_INPUT,
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
    uint32_t device_id;
    char name[32];
    device_type_t type;
    device_state_t state;
    uint32_t irq;
    uint32_t dma_channel;
    uint32_t io_base;
    uint32_t io_size;
    void* driver_data;
    bool is_initialized;
} device_t;

typedef struct {
    char name[32];
    device_type_t supported_types;
    bool (*probe)(device_t* device);
    bool (*init)(device_t* device);
    void (*cleanup)(device_t* device);
    int (*read)(device_t* device, void* buffer, size_t size, uint32_t offset);
    int (*write)(device_t* device, const void* buffer, size_t size, uint32_t offset);
    int (*ioctl)(device_t* device, uint32_t request, void* arg);
} driver_t;

typedef struct {
    uint32_t irq;
    void (*handler)(uint32_t irq, void* context);
    void* context;
} irq_handler_t;

typedef struct {
    device_t devices[MAX_DEVICES];
    uint32_t device_count;
    driver_t drivers[MAX_DRIVERS];
    uint32_t driver_count;
    irq_handler_t irq_handlers[MAX_IRQ_HANDLERS];
    uint32_t irq_handler_count;
    bool dma_channels[MAX_DMA_CHANNELS];
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

    // Trouver un driver compatible
    for (uint32_t i = 0; i < device_manager.driver_count; i++) {
        driver_t* driver = &device_manager.drivers[i];
        if ((driver->supported_types & (1 << device->type)) &&
            driver->probe(device)) {
            device->driver_data = NULL;
            if (driver->init(device)) {
                device->is_initialized = true;
                memcpy(&device_manager.devices[device_manager.device_count++], device, sizeof(device_t));
                return true;
            }
        }
    }

    return false;
}

void unregister_device(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            device_t* device = &device_manager.devices[i];
            
            // Trouver le driver
            for (uint32_t j = 0; j < device_manager.driver_count; j++) {
                driver_t* driver = &device_manager.drivers[j];
                if ((driver->supported_types & (1 << device->type)) &&
                    driver->probe(device)) {
                    driver->cleanup(device);
                    break;
                }
            }

            // Supprimer le périphérique
            memmove(&device_manager.devices[i],
                    &device_manager.devices[i + 1],
                    (device_manager.device_count - i - 1) * sizeof(device_t));
            device_manager.device_count--;
            break;
        }
    }
}

device_t* find_device_by_id(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            return &device_manager.devices[i];
        }
    }
    return NULL;
}

device_t* find_device_by_type(device_type_t type) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].type == type) {
            return &device_manager.devices[i];
        }
    }
    return NULL;
}

bool register_irq_handler(uint32_t irq, void (*handler)(uint32_t irq, void* context), void* context) {
    if (device_manager.irq_handler_count >= MAX_IRQ_HANDLERS) {
        return false;
    }

    irq_handler_t* irq_handler = &device_manager.irq_handlers[device_manager.irq_handler_count++];
    irq_handler->irq = irq;
    irq_handler->handler = handler;
    irq_handler->context = context;

    // Configurer l'IRQ dans le PIC
    outb(0x20, 0x11); // ICW1
    outb(0x21, irq);  // ICW2
    outb(0x21, 0x04); // ICW3
    outb(0x21, 0x01); // ICW4

    // Activer l'IRQ
    outb(0x21, inb(0x21) & ~(1 << irq));

    return true;
}

void unregister_irq_handler(uint32_t irq) {
    for (uint32_t i = 0; i < device_manager.irq_handler_count; i++) {
        if (device_manager.irq_handlers[i].irq == irq) {
            // Désactiver l'IRQ
            outb(0x21, inb(0x21) | (1 << irq));

            // Supprimer le handler
            memmove(&device_manager.irq_handlers[i],
                    &device_manager.irq_handlers[i + 1],
                    (device_manager.irq_handler_count - i - 1) * sizeof(irq_handler_t));
            device_manager.irq_handler_count--;
            break;
        }
    }
}

void handle_irq(uint32_t irq) {
    for (uint32_t i = 0; i < device_manager.irq_handler_count; i++) {
        if (device_manager.irq_handlers[i].irq == irq) {
            device_manager.irq_handlers[i].handler(irq, device_manager.irq_handlers[i].context);
            break;
        }
    }

    // Envoyer l'EOI au PIC
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

uint32_t allocate_dma_channel() {
    for (uint32_t i = 0; i < MAX_DMA_CHANNELS; i++) {
        if (!device_manager.dma_channels[i]) {
            device_manager.dma_channels[i] = true;
            return i;
        }
    }
    return (uint32_t)-1;
}

void free_dma_channel(uint32_t channel) {
    if (channel < MAX_DMA_CHANNELS) {
        device_manager.dma_channels[channel] = false;
    }
}

int read_device(device_t* device, void* buffer, size_t size, uint32_t offset) {
    if (!device || !device->is_initialized || !buffer) {
        return -1;
    }

    // Trouver le driver
    for (uint32_t i = 0; i < device_manager.driver_count; i++) {
        driver_t* driver = &device_manager.drivers[i];
        if ((driver->supported_types & (1 << device->type)) &&
            driver->probe(device)) {
            return driver->read(device, buffer, size, offset);
        }
    }

    return -1;
}

int write_device(device_t* device, const void* buffer, size_t size, uint32_t offset) {
    if (!device || !device->is_initialized || !buffer) {
        return -1;
    }

    // Trouver le driver
    for (uint32_t i = 0; i < device_manager.driver_count; i++) {
        driver_t* driver = &device_manager.drivers[i];
        if ((driver->supported_types & (1 << device->type)) &&
            driver->probe(device)) {
            return driver->write(device, buffer, size, offset);
        }
    }

    return -1;
}

int ioctl_device(device_t* device, uint32_t request, void* arg) {
    if (!device || !device->is_initialized) {
        return -1;
    }

    // Trouver le driver
    for (uint32_t i = 0; i < device_manager.driver_count; i++) {
        driver_t* driver = &device_manager.drivers[i];
        if ((driver->supported_types & (1 << device->type)) &&
            driver->probe(device)) {
            return driver->ioctl(device, request, arg);
        }
    }

    return -1;
}

// Fonctions d'aide pour les périphériques courants

device_t* get_keyboard_device() {
    return find_device_by_type(DEVICE_TYPE_CHAR);
}

device_t* get_mouse_device() {
    return find_device_by_type(DEVICE_TYPE_INPUT);
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