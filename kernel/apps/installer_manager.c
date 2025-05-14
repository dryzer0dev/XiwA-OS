#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_INSTALLERS 100
#define MAX_INSTALL_PATH 1024

typedef struct {
    char name[256];
    char installer_url[1024];
    char install_path[MAX_INSTALL_PATH];
    char executable_path[MAX_INSTALL_PATH];
    int is_installed;
    time_t install_date;
} Installer;

typedef struct {
    Installer installers[MAX_INSTALLERS];
    int count;
} InstallerManager;

// Initialisation du gestionnaire d'installation
InstallerManager* init_installer_manager() {
    InstallerManager* im = (InstallerManager*)malloc(sizeof(InstallerManager));
    if (!im) return NULL;
    im->count = 0;
    return im;
}

// Ajouter un installateur
int add_installer(InstallerManager* im, const char* name, const char* url) {
    if (im->count >= MAX_INSTALLERS) return -1;

    Installer* installer = &im->installers[im->count];
    strncpy(installer->name, name, 256);
    strncpy(installer->installer_url, url, 1024);
    snprintf(installer->install_path, MAX_INSTALL_PATH, "/opt/%s", name);
    snprintf(installer->executable_path, MAX_INSTALL_PATH, "/opt/%s/%s.exe", name, name);
    installer->is_installed = 0;

    im->count++;
    return 0;
}

// Installer une application
int install_application(InstallerManager* im, const char* name) {
    for (int i = 0; i < im->count; i++) {
        if (strcmp(im->installers[i].name, name) == 0) {
            Installer* installer = &im->installers[i];
            
            printf("Téléchargement de %s depuis %s...\n", name, installer->installer_url);
            printf("Installation dans %s...\n", installer->install_path);
            
            // Simuler l'installation
            installer->is_installed = 1;
            installer->install_date = time(NULL);
            
            printf("Installation de %s terminée avec succès!\n", name);
            return 0;
        }
    }
    return -1;
}

// Lancer une application installée
int launch_installed_app(InstallerManager* im, const char* name) {
    for (int i = 0; i < im->count; i++) {
        if (strcmp(im->installers[i].name, name) == 0 && im->installers[i].is_installed) {
            printf("Lancement de %s depuis %s...\n", name, im->installers[i].executable_path);
            return 0;
        }
    }
    return -1;
}

// Configuration des installateurs populaires
void setup_popular_installers(InstallerManager* im) {
    // Epic Games
    add_installer(im, "Epic Games",
                 "https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/installer/download/EpicGamesLauncherInstaller.msi");

    // Visual Studio Code
    add_installer(im, "VSCode",
                 "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64");

    // Discord
    add_installer(im, "Discord",
                 "https://discord.com/api/download?platform=win");

    // Steam
    add_installer(im, "Steam",
                 "https://cdn.akamai.steamstatic.com/client/installer/SteamSetup.exe");

    // Google Chrome
    add_installer(im, "Chrome",
                 "https://dl.google.com/chrome/install/latest/chrome_installer.exe");
}

// Exemple d'utilisation
void example_usage() {
    InstallerManager* im = init_installer_manager();
    setup_popular_installers(im);

    // Installer Epic Games
    install_application(im, "Epic Games");
    
    // Lancer Epic Games
    launch_installed_app(im, "Epic Games");
    
    // Installer et lancer VSCode
    install_application(im, "VSCode");
    launch_installed_app(im, "VSCode");
    
    // Installer et lancer Discord
    install_application(im, "Discord");
    launch_installed_app(im, "Discord");
} 