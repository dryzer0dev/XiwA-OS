#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BOOTLOADER_VERSION "1.0.0"
#define MAX_BOOT_OPTIONS 10
#define MAX_OS_NAME 256
#define MAX_OS_PATH 1024
#define LOGO_PATH "/boot/xiwa.png"

typedef struct {
    char name[MAX_OS_NAME];
    char path[MAX_OS_PATH];
    int is_default;
} BootOption;

typedef struct {
    BootOption options[MAX_BOOT_OPTIONS];
    int option_count;
    char logo_path[MAX_OS_PATH];
    int timeout;
} Bootloader;

void init_bootloader(Bootloader* bootloader) {
    bootloader->option_count = 0;
    bootloader->timeout = 5;
    strcpy(bootloader->logo_path, LOGO_PATH);
}

int add_boot_option(Bootloader* bootloader, const char* name, const char* path, int is_default) {
    if (bootloader->option_count >= MAX_BOOT_OPTIONS) {
        return -1;
    }

    BootOption* option = &bootloader->options[bootloader->option_count];
    strncpy(option->name, name, MAX_OS_NAME - 1);
    strncpy(option->path, path, MAX_OS_PATH - 1);
    option->is_default = is_default;
    bootloader->option_count++;

    return 0;
}

void load_boot_logo(const char* logo_path) {
    printf("\033[2J\033[H");
    printf("\033[1;37m");
    
    if (access(logo_path, F_OK) == -1) {
        printf("Erreur: Logo non trouvé à %s\n", logo_path);
        return;
    }
    
    printf("Chargement du logo...\n");
    usleep(500000);
    
    printf("\033[1;36m");
    printf("    ╔════════════════════════════════════════════════════════════╗\n");
    printf("    ║                                                            ║\n");
    printf("    ║                      XiwA-OS v%s                      ║\n", BOOTLOADER_VERSION);
    printf("    ║                                                            ║\n");
    printf("    ╚════════════════════════════════════════════════════════════╝\n");
    printf("\033[0m");
}

void show_boot_menu(Bootloader* bootloader) {
    load_boot_logo(bootloader->logo_path);
    
    printf("\n=== Menu de démarrage ===\n");
    for (int i = 0; i < bootloader->option_count; i++) {
        printf("%d. %s%s\n", i + 1, bootloader->options[i].name,
               bootloader->options[i].is_default ? " (par défaut)" : "");
    }
    
    printf("\nDémarrage automatique dans %d secondes...\n", bootloader->timeout);
}

void boot_system(const char* path) {
    printf("Démarrage de %s...\n", path);
    printf("Système démarré avec succès!\n");
}

void setup_bootloader(Bootloader* bootloader) {
    add_boot_option(bootloader, "XiwA-OS", "/boot/xiwa-os", 1);
    add_boot_option(bootloader, "XiwA-OS (Safe Mode)", "/boot/xiwa-os-safe", 0);
    add_boot_option(bootloader, "XiwA-OS (Recovery Mode)", "/boot/xiwa-os-recovery", 0);
}

int main() {
    Bootloader bootloader;
    init_bootloader(&bootloader);
    setup_bootloader(&bootloader);
    
    if (access("/etc/first_boot_done", F_OK) == -1) {
        printf("Premier démarrage détecté!\n");
        system("/usr/bin/first_boot");
        return 0;
    }
    
    show_boot_menu(&bootloader);
    
    int selection = 0;
    time_t start_time = time(NULL);
    
    while (time(NULL) - start_time < bootloader.timeout) {
        if (kbhit()) {
            char input = getchar();
            if (input >= '1' && input <= '0' + bootloader.option_count) {
                selection = input - '1';
                break;
            }
        }
        sleep(1);
    }
    
    if (selection == 0) {
        for (int i = 0; i < bootloader.option_count; i++) {
            if (bootloader.options[i].is_default) {
                boot_system(bootloader.options[i].path);
                break;
            }
        }
    } else {
        boot_system(bootloader.options[selection].path);
    }
    
    return 0;
}