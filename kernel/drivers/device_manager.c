#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEVICES 100
#define MAX_DEVICE_NAME 64
#define MAX_DRIVER_NAME 64

typedef enum {
    DEVICE_TYPE_CPU,
    DEVICE_TYPE_GPU,
    DEVICE_TYPE_RAM,
    DEVICE_TYPE_DISK,
    DEVICE_TYPE_NETWORK,
    DEVICE_TYPE_AUDIO,
    DEVICE_TYPE_USB,
    DEVICE_TYPE_KEYBOARD,
    DEVICE_TYPE_MOUSE,
    DEVICE_TYPE_DISPLAY
} DeviceType;

typedef struct {
    char name[MAX_DEVICE_NAME];
    char driver[MAX_DRIVER_NAME];
    DeviceType type;
    int is_enabled;
    int is_plugged;
    void* driver_data;
} Device;

typedef struct {
    Device devices[MAX_DEVICES];
    int count;
} DeviceManager;

// Initialisation du gestionnaire de périphériques
DeviceManager* init_device_manager() {
    DeviceManager* dm = (DeviceManager*)malloc(sizeof(DeviceManager));
    if (!dm) return NULL;

    dm->count = 0;
    return dm;
}

// Ajouter un périphérique
int add_device(DeviceManager* dm, const char* name, DeviceType type, const char* driver) {
    if (dm->count >= MAX_DEVICES) return -1;

    Device* device = &dm->devices[dm->count];
    strncpy(device->name, name, MAX_DEVICE_NAME);
    strncpy(device->driver, driver, MAX_DRIVER_NAME);
    device->type = type;
    device->is_enabled = 1;
    device->is_plugged = 1;
    device->driver_data = NULL;

    dm->count++;
    return 0;
}

// Détecter les périphériques système
void detect_system_devices(DeviceManager* dm) {
    // CPU
    add_device(dm, "CPU Intel/AMD", DEVICE_TYPE_CPU, "cpu_driver");
    
    // GPU
    add_device(dm, "GPU NVIDIA/AMD", DEVICE_TYPE_GPU, "gpu_driver");
    
    // RAM
    add_device(dm, "RAM System", DEVICE_TYPE_RAM, "ram_driver");
    
    // Disque
    add_device(dm, "SSD/HDD System", DEVICE_TYPE_DISK, "disk_driver");
    
    // Réseau
    add_device(dm, "Network Card", DEVICE_TYPE_NETWORK, "network_driver");
    
    // Audio
    add_device(dm, "Audio Card", DEVICE_TYPE_AUDIO, "audio_driver");
    
    // USB
    add_device(dm, "USB Controller", DEVICE_TYPE_USB, "usb_driver");
    
    // Clavier
    add_device(dm, "Keyboard", DEVICE_TYPE_KEYBOARD, "keyboard_driver");
    
    // Souris
    add_device(dm, "Mouse", DEVICE_TYPE_MOUSE, "mouse_driver");
    
    // Écran
    add_device(dm, "Display", DEVICE_TYPE_DISPLAY, "display_driver");
}

// Lister les périphériques
void list_devices(DeviceManager* dm) {
    printf("\nPériphériques système:\n");
    printf("Nom\t\tType\t\tPilote\t\tÉtat\n");
    printf("--------------------------------------------------------\n");

    for (int i = 0; i < dm->count; i++) {
        Device* device = &dm->devices[i];
        printf("%s\t", device->name);
        
        switch (device->type) {
            case DEVICE_TYPE_CPU: printf("CPU\t\t"); break;
            case DEVICE_TYPE_GPU: printf("GPU\t\t"); break;
            case DEVICE_TYPE_RAM: printf("RAM\t\t"); break;
            case DEVICE_TYPE_DISK: printf("Disque\t\t"); break;
            case DEVICE_TYPE_NETWORK: printf("Réseau\t\t"); break;
            case DEVICE_TYPE_AUDIO: printf("Audio\t\t"); break;
            case DEVICE_TYPE_USB: printf("USB\t\t"); break;
            case DEVICE_TYPE_KEYBOARD: printf("Clavier\t\t"); break;
            case DEVICE_TYPE_MOUSE: printf("Souris\t\t"); break;
            case DEVICE_TYPE_DISPLAY: printf("Écran\t\t"); break;
        }
        
        printf("%s\t", device->driver);
        printf("%s\n", device->is_enabled ? "Activé" : "Désactivé");
    }
}

// Activer/Désactiver un périphérique
int toggle_device(DeviceManager* dm, const char* name, int enable) {
    for (int i = 0; i < dm->count; i++) {
        if (strcmp(dm->devices[i].name, name) == 0) {
            dm->devices[i].is_enabled = enable;
            return 0;
        }
    }
    return -1;
}

// Vérifier l'état d'un périphérique
int check_device_status(DeviceManager* dm, const char* name) {
    for (int i = 0; i < dm->count; i++) {
        if (strcmp(dm->devices[i].name, name) == 0) {
            return dm->devices[i].is_enabled && dm->devices[i].is_plugged;
        }
    }
    return -1;
} 