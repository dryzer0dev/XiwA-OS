#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_DEVICES 64
#define MAX_DRIVERS 32
#define MAX_IRQ_HANDLERS 16
#define MAX_DMA_CHANNELS 8

typedef enum {
    DEVICE_CHAR,
    DEVICE_BLOCK,
    DEVICE_NETWORK,
    DEVICE_DISPLAY,
    DEVICE_SOUND,
    DEVICE_INPUT
} device_type_t;

typedef enum {
    DEVICE_READY,
    DEVICE_BUSY,
    DEVICE_ERROR,
    DEVICE_OFFLINE
} device_state_t;

typedef struct {
    uint32_t id;
    char name[32];
    device_type_t type;
    device_state_t state;
    uint32_t base_address;
    uint32_t irq;
    uint32_t dma_channel;
    void* driver_data;
} device_t;

typedef struct {
    uint32_t id;
    char name[32];
    device_type_t type;
    bool (*init)(device_t* device);
    void (*deinit)(device_t* device);
    int32_t (*read)(device_t* device, void* buffer, uint32_t size);
    int32_t (*write)(device_t* device, const void* buffer, uint32_t size);
    int32_t (*ioctl)(device_t* device, uint32_t command, void* arg);
} driver_t;

typedef struct {
    uint32_t irq;
    void (*handler)(void);
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

static device_manager_t device_manager;

void init_device_manager() {
    memset(&device_manager, 0, sizeof(device_manager_t));
}

uint32_t register_driver(const char* name, device_type_t type,
                        bool (*init)(device_t*),
                        void (*deinit)(device_t*),
                        int32_t (*read)(device_t*, void*, uint32_t),
                        int32_t (*write)(device_t*, const void*, uint32_t),
                        int32_t (*ioctl)(device_t*, uint32_t, void*)) {
    if (device_manager.driver_count >= MAX_DRIVERS) return 0;

    driver_t* driver = &device_manager.drivers[device_manager.driver_count++];
    driver->id = device_manager.driver_count;
    strncpy(driver->name, name, sizeof(driver->name) - 1);
    driver->type = type;
    driver->init = init;
    driver->deinit = deinit;
    driver->read = read;
    driver->write = write;
    driver->ioctl = ioctl;

    return driver->id;
}

void unregister_driver(uint32_t driver_id) {
    for (uint32_t i = 0; i < device_manager.driver_count; i++) {
        if (device_manager.drivers[i].id == driver_id) {
            for (uint32_t j = 0; j < device_manager.device_count; j++) {
                if (device_manager.devices[j].driver_data) {
                    device_manager.drivers[i].deinit(&device_manager.devices[j]);
                }
            }
            device_manager.drivers[i] = device_manager.drivers[--device_manager.driver_count];
            break;
        }
    }
}

uint32_t register_device(const char* name, device_type_t type,
                        uint32_t base_address, uint32_t irq) {
    if (device_manager.device_count >= MAX_DEVICES) return 0;

    device_t* device = &device_manager.devices[device_manager.device_count++];
    device->id = device_manager.device_count;
    strncpy(device->name, name, sizeof(device->name) - 1);
    device->type = type;
    device->state = DEVICE_OFFLINE;
    device->base_address = base_address;
    device->irq = irq;
    device->dma_channel = 0xFFFFFFFF;
    device->driver_data = NULL;

    return device->id;
}

void unregister_device(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].id == device_id) {
            if (device_manager.devices[i].driver_data) {
                for (uint32_t j = 0; j < device_manager.driver_count; j++) {
                    if (device_manager.drivers[j].type == device_manager.devices[i].type) {
                        device_manager.drivers[j].deinit(&device_manager.devices[i]);
                        break;
                    }
                }
            }
            if (device_manager.devices[i].dma_channel != 0xFFFFFFFF) {
                device_manager.dma_channels[device_manager.devices[i].dma_channel] = false;
            }
            device_manager.devices[i] = device_manager.devices[--device_manager.device_count];
            break;
        }
    }
}

bool init_device(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].id == device_id) {
            device_t* device = &device_manager.devices[i];
            for (uint32_t j = 0; j < device_manager.driver_count; j++) {
                if (device_manager.drivers[j].type == device->type) {
                    if (device_manager.drivers[j].init(device)) {
                        device->state = DEVICE_READY;
                        return true;
                    }
                    device->state = DEVICE_ERROR;
                    return false;
                }
            }
            return false;
        }
    }
    return false;
}

void deinit_device(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].id == device_id) {
            device_t* device = &device_manager.devices[i];
            if (device->driver_data) {
                for (uint32_t j = 0; j < device_manager.driver_count; j++) {
                    if (device_manager.drivers[j].type == device->type) {
                        device_manager.drivers[j].deinit(device);
                        break;
                    }
                }
            }
            device->state = DEVICE_OFFLINE;
            break;
        }
    }
}

int32_t read_device(uint32_t device_id, void* buffer, uint32_t size) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].id == device_id) {
            device_t* device = &device_manager.devices[i];
            if (device->state != DEVICE_READY) return -1;

            for (uint32_t j = 0; j < device_manager.driver_count; j++) {
                if (device_manager.drivers[j].type == device->type) {
                    return device_manager.drivers[j].read(device, buffer, size);
                }
            }
            return -1;
        }
    }
    return -1;
}

int32_t write_device(uint32_t device_id, const void* buffer, uint32_t size) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].id == device_id) {
            device_t* device = &device_manager.devices[i];
            if (device->state != DEVICE_READY) return -1;

            for (uint32_t j = 0; j < device_manager.driver_count; j++) {
                if (device_manager.drivers[j].type == device->type) {
                    return device_manager.drivers[j].write(device, buffer, size);
                }
            }
            return -1;
        }
    }
    return -1;
}

int32_t ioctl_device(uint32_t device_id, uint32_t command, void* arg) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].id == device_id) {
            device_t* device = &device_manager.devices[i];
            if (device->state != DEVICE_READY) return -1;

            for (uint32_t j = 0; j < device_manager.driver_count; j++) {
                if (device_manager.drivers[j].type == device->type) {
                    return device_manager.drivers[j].ioctl(device, command, arg);
                }
            }
            return -1;
        }
    }
    return -1;
}

void register_irq_handler(uint32_t irq, void (*handler)(void)) {
    if (device_manager.irq_handler_count >= MAX_IRQ_HANDLERS) return;

    irq_handler_t* irq_handler = &device_manager.irq_handlers[device_manager.irq_handler_count++];
    irq_handler->irq = irq;
    irq_handler->handler = handler;
}

void unregister_irq_handler(uint32_t irq) {
    for (uint32_t i = 0; i < device_manager.irq_handler_count; i++) {
        if (device_manager.irq_handlers[i].irq == irq) {
            device_manager.irq_handlers[i] = device_manager.irq_handlers[--device_manager.irq_handler_count];
            break;
        }
    }
}

uint32_t allocate_dma_channel() {
    for (uint32_t i = 0; i < MAX_DMA_CHANNELS; i++) {
        if (!device_manager.dma_channels[i]) {
            device_manager.dma_channels[i] = true;
            return i;
        }
    }
    return 0xFFFFFFFF;
}

void free_dma_channel(uint32_t channel) {
    if (channel < MAX_DMA_CHANNELS) {
        device_manager.dma_channels[channel] = false;
    }
}

device_t* find_device_by_id(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].id == device_id) {
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