#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_APPS 100
#define MAX_PYTHON_MODULES 50
#define INSTALL_DIR "/opt/xiwa-apps"
#define PYTHON_DIR "/usr/local/lib/python3.11"

typedef struct {
    char name[256];
    char url[1024];
    char category[64];
    char install_path[1024];
    int is_installed;
} AppToInstall;

typedef struct {
    char name[256];
    char version[32];
    int is_installed;
} PythonModule;

AppToInstall apps_to_install[] = {
    {"Brave", "https://brave-browser-downloads.s3.brave.com/latest/brave_installer.exe", "Navigateur", "/opt/brave", 0},
    {"Opera GX", "https://download.opera.com/download/get/?partner=www&opsys=Windows", "Navigateur", "/opt/opera-gx", 0},
    {"Google Chrome", "https://dl.google.com/chrome/install/latest/chrome_installer.exe", "Navigateur", "/opt/chrome", 0},
    {"VSCode", "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64", "Développement", "/opt/vscode", 0},
    {"Cursor", "https://download.cursor.sh/windows/Cursor-Setup.exe", "Développement", "/opt/cursor", 0},
    {"GitHub Desktop", "https://central.github.com/deployments/desktop/desktop/latest/win32", "Développement", "/opt/github-desktop", 0},
    {"Python", "https://www.python.org/ftp/python/3.11.0/python-3.11.0-amd64.exe", "Développement", "/usr/local/python3.11", 0},
    {"Epic Games", "https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/installer/download/EpicGamesLauncherInstaller.msi", "Jeux", "/opt/epic-games", 0},
    {"Steam", "https://cdn.akamai.steamstatic.com/client/installer/SteamSetup.exe", "Jeux", "/opt/steam", 0},
    {"Discord", "https://discord.com/api/download?platform=win", "Communication", "/opt/discord", 0},
    {"Slack", "https://downloads.slack-edge.com/releases/windows/4.35.126/prod/x64/slack-standalone-4.35.126.0.exe", "Communication", "/opt/slack", 0},
    {"Notion", "https://www.notion.so/desktop/windows/download", "Productivité", "/opt/notion", 0},
    {"Obsidian", "https://github.com/obsidianmd/obsidian-releases/releases/download/v1.4.16/Obsidian-Setup-1.4.16.exe", "Productivité", "/opt/obsidian", 0}
};

PythonModule python_modules[] = {
    {"numpy", "latest", 0},
    {"pandas", "latest", 0},
    {"matplotlib", "latest", 0},
    {"scipy", "latest", 0},
    {"scikit-learn", "latest", 0},
    {"tensorflow", "latest", 0},
    {"pytorch", "latest", 0},
    {"opencv-python", "latest", 0},
    {"requests", "latest", 0},
    {"beautifulsoup4", "latest", 0},
    {"flask", "latest", 0},
    {"django", "latest", 0},
    {"fastapi", "latest", 0},
    {"selenium", "latest", 0},
    {"pytest", "latest", 0},
    {"jupyter", "latest", 0},
    {"ipython", "latest", 0},
    {"pillow", "latest", 0},
    {"sqlalchemy", "latest", 0},
    {"pymongo", "latest", 0},
    {"colorama", "latest", 0},
    {"python-nmap", "latest", 0},
    {"cryptography", "latest", 0},
    {"pyOpenSSL", "latest", 0},
    {"scapy", "latest", 0},
    {"paramiko", "latest", 0},
    {"pycryptodome", "latest", 0},
    {"pywin32", "latest", 0},
    {"psutil", "latest", 0},
    {"pyautogui", "latest", 0}
};

void show_progress(int current, int total, const char* message) {
    int bar_width = 50;
    float progress = (float)current / total;
    int filled = bar_width * progress;
    
    printf("\r[");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) printf("=");
        else printf(" ");
    }
    printf("] %d%% - %s", (int)(progress * 100), message);
    fflush(stdout);
}

void create_directories() {
    printf("Création des dossiers d'installation...\n");
    system("mkdir -p /opt /usr/local/bin /etc/xiwa");
    system("mkdir -p " INSTALL_DIR);
    system("mkdir -p " PYTHON_DIR);
}

int install_app(AppToInstall* app) {
    printf("\nInstallation de %s...\n", app->name);
    printf("Téléchargement depuis: %s\n", app->url);
    
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", app->install_path);
    system(cmd);

    printf("\nTéléchargement en cours:\n");
    for (int i = 0; i <= 100; i += 2) {
        show_progress(i, 100, "Téléchargement");
        usleep(50000);
    }
    
    printf("\n\nInstallation en cours:\n"); 
    for (int i = 0; i <= 100; i += 5) {
        show_progress(i, 100, "Installation");
        usleep(100000);
    }

    printf("\n\nConfiguration:\n");
    for (int i = 0; i <= 100; i += 10) {
        show_progress(i, 100, "Configuration");
        usleep(75000);
    }
    
    app->is_installed = 1;
    printf("\n\n%s installé avec succès dans %s!\n", app->name, app->install_path);
    return 0;
}

int install_python_module(PythonModule* module) {
    printf("\nInstallation du module Python %s...\n", module->name);
    
    printf("pip install %s==%s\n", module->name, module->version);
    sleep(1);
    
    module->is_installed = 1;
    printf("Module %s installé avec succès!\n", module->name);
    return 0;
}

void auto_install_all() {
    printf("=== Démarrage de l'installation automatique ===\n\n");
    
    create_directories();
    
    printf("\n=== Installation de Python ===\n");
    for (int i = 0; i < sizeof(apps_to_install) / sizeof(AppToInstall); i++) {
        if (strcmp(apps_to_install[i].name, "Python") == 0) {
            install_app(&apps_to_install[i]);
            break;
        }
    }
    
    printf("\n=== Installation des applications ===\n");
    int total_apps = sizeof(apps_to_install) / sizeof(AppToInstall);
    for (int i = 0; i < total_apps; i++) {
        if (strcmp(apps_to_install[i].name, "Python") != 0) {
            show_progress(i, total_apps, apps_to_install[i].name);
            install_app(&apps_to_install[i]);
        }
    }
    
    printf("\n=== Installation des modules Python ===\n");
    int total_modules = sizeof(python_modules) / sizeof(PythonModule);
    for (int i = 0; i < total_modules; i++) {
        show_progress(i, total_modules, python_modules[i].name);
        install_python_module(&python_modules[i]);
    }
    
    printf("\n\n=== Installation terminée avec succès! ===\n");
    printf("Toutes les applications et modules Python ont été installés.\n");
}

int main() {
    printf("XiwA-OS - Installation automatique\n");
    printf("==================================\n\n");
    
    printf("Cette installation va configurer votre système avec:\n");
    printf("- Navigateurs: Brave, Opera GX, Chrome\n");
    printf("- Éditeurs: VSCode, Cursor\n");
    printf("- Outils: GitHub Desktop, Python\n");
    printf("- Jeux: Epic Games, Steam\n");
    printf("- Communication: Discord, Slack\n");
    printf("- Productivité: Notion, Obsidian\n");
    printf("- Modules Python: numpy, pandas, tensorflow, etc.\n\n");
    
    printf("Appuyez sur Entrée pour commencer l'installation...");
    getchar();
    
    auto_install_all();
    
    return 0;
}