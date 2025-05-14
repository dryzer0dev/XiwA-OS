#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_DEVICES 64
#define DEVICE_TYPE_KEYBOARD 1
#define DEVICE_TYPE_MOUSE 2
#define DEVICE_TYPE_DISK 3
#define DEVICE_TYPE_DISPLAY 4
#define DEVICE_TYPE_NETWORK 5
#define DEVICE_TYPE_SOUND 6

typedef struct {
    uint32_t device_id;
    uint32_t device_type;
    char name[32];
    bool is_initialized;
    void* driver_data;
    void (*init)(void*);
    void (*deinit)(void*);
    int (*read)(void*, void*, size_t);
    int (*write)(void*, const void*, size_t);
    int (*ioctl)(void*, uint32_t, void*);
} device_t;

typedef struct {
    device_t devices[MAX_DEVICES];
    uint32_t device_count;
    uint32_t next_device_id;
} device_manager_t;

device_manager_t device_manager;

void init_device_manager() {
    memset(&device_manager, 0, sizeof(device_manager_t));
    device_manager.next_device_id = 1;
}

device_t* register_device(const char* name, uint32_t type,
                         void (*init)(void*),
                         void (*deinit)(void*),
                         int (*read)(void*, void*, size_t),
                         int (*write)(void*, const void*, size_t),
                         int (*ioctl)(void*, uint32_t, void*)) {
    if (device_manager.device_count >= MAX_DEVICES) {
        return NULL;
    }

    device_t* device = &device_manager.devices[device_manager.device_count++];
    device->device_id = device_manager.next_device_id++;
    device->device_type = type;
    strncpy(device->name, name, 31);
    device->name[31] = '\0';
    device->is_initialized = false;
    device->driver_data = NULL;
    device->init = init;
    device->deinit = deinit;
    device->read = read;
    device->write = write;
    device->ioctl = ioctl;

    return device;
}

void unregister_device(device_t* device) {
    if (!device) return;

    if (device->is_initialized && device->deinit) {
        device->deinit(device->driver_data);
    }

    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (&device_manager.devices[i] == device) {
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

device_t* find_device_by_type(uint32_t type) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_type == type) {
            return &device_manager.devices[i];
        }
    }
    return NULL;
}

bool initialize_device(device_t* device) {
    if (!device || device->is_initialized) return false;

    if (device->init) {
        device->driver_data = kmalloc(1024); // 1KB pour les données du pilote
        if (!device->driver_data) return false;

        device->init(device->driver_data);
        device->is_initialized = true;
        return true;
    }
    return false;
}

void deinitialize_device(device_t* device) {
    if (!device || !device->is_initialized) return;

    if (device->deinit) {
        device->deinit(device->driver_data);
    }

    kfree(device->driver_data);
    device->driver_data = NULL;
    device->is_initialized = false;
}

int read_device(device_t* device, void* buffer, size_t size) {
    if (!device || !device->is_initialized || !device->read) {
        return -1;
    }
    return device->read(device->driver_data, buffer, size);
}

int write_device(device_t* device, const void* buffer, size_t size) {
    if (!device || !device->is_initialized || !device->write) {
        return -1;
    }
    return device->write(device->driver_data, buffer, size);
}

int ioctl_device(device_t* device, uint32_t request, void* arg) {
    if (!device || !device->is_initialized || !device->ioctl) {
        return -1;
    }
    return device->ioctl(device->driver_data, request, arg);
}

// Fonctions d'aide pour les périphériques courants

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
    return find_device_by_type(DEVICE_TYPE_DISK);
}

device_t* get_network_device() {
    return find_device_by_type(DEVICE_TYPE_NETWORK);
}

device_t* get_sound_device() {
    return find_device_by_type(DEVICE_TYPE_SOUND);
}

// Gestionnaire d'interruptions pour les périphériques

void handle_device_interrupt(uint32_t irq) {
    // Trouver le périphérique associé à cette IRQ
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        device_t* device = &device_manager.devices[i];
        if (device->is_initialized) {
            // Vérifier si le périphérique a une fonction de gestion d'interruption
            if (device->ioctl) {
                device->ioctl(device->driver_data, IOCTL_HANDLE_IRQ, (void*)irq);
            }
        }
    }
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