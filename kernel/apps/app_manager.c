#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_APPS 100
#define MAX_APP_NAME 64
#define MAX_APP_PATH 256
#define MAX_APP_ICON 1024

typedef struct {
    char name[MAX_APP_NAME];
    char path[MAX_APP_PATH];
    char icon[MAX_APP_ICON];
    char version[16];
    char description[256];
    char category[32];
    int is_installed;
    time_t install_date;
    size_t size;
} Application;

typedef struct {
    Application apps[MAX_APPS];
    int count;
} AppManager;

// Initialisation du gestionnaire d'applications
AppManager* init_app_manager() {
    AppManager* am = (AppManager*)malloc(sizeof(AppManager));
    if (!am) return NULL;

    am->count = 0;
    return am;
}

// Ajouter une application
int add_app(AppManager* am, const char* name, const char* path, const char* icon,
           const char* version, const char* description, const char* category) {
    if (am->count >= MAX_APPS) return -1;

    Application* app = &am->apps[am->count];
    strncpy(app->name, name, MAX_APP_NAME);
    strncpy(app->path, path, MAX_APP_PATH);
    strncpy(app->icon, icon, MAX_APP_ICON);
    strncpy(app->version, version, 16);
    strncpy(app->description, description, 256);
    strncpy(app->category, category, 32);
    app->is_installed = 1;
    app->install_date = time(NULL);
    app->size = 0; // À calculer lors de l'installation

    am->count++;
    return 0;
}

// Installer les applications système
void install_system_apps(AppManager* am) {
    // Explorateur de fichiers
    add_app(am, "Explorateur", "/usr/bin/explorer", 
           "📁", "1.0.0", "Gestionnaire de fichiers", "Système");

    // Terminal
    add_app(am, "Terminal", "/usr/bin/terminal",
           "💻", "1.0.0", "Terminal système", "Utilitaires");

    // Navigateur web
    add_app(am, "Navigateur", "/usr/bin/browser",
           "🌐", "1.0.0", "Navigateur web", "Internet");

    // Éditeur de texte
    add_app(am, "Éditeur", "/usr/bin/editor",
           "📝", "1.0.0", "Éditeur de texte", "Utilitaires");

    // Calculatrice
    add_app(am, "Calculatrice", "/usr/bin/calculator",
           "🔢", "1.0.0", "Calculatrice", "Utilitaires");

    // Horloge
    add_app(am, "Horloge", "/usr/bin/clock",
           "🕒", "1.0.0", "Horloge et calendrier", "Utilitaires");

    // Paramètres
    add_app(am, "Paramètres", "/usr/bin/settings",
           "⚙️", "1.0.0", "Paramètres système", "Système");

    // Gestionnaire de paquets
    add_app(am, "Gestionnaire de paquets", "/usr/bin/package-manager",
           "📦", "1.0.0", "Gestion des applications", "Système");

    // Lecteur multimédia
    add_app(am, "Lecteur", "/usr/bin/media-player",
           "🎵", "1.0.0", "Lecteur multimédia", "Multimédia");

    // Gestionnaire de tâches
    add_app(am, "Gestionnaire de tâches", "/usr/bin/task-manager",
           "📊", "1.0.0", "Gestion des processus", "Système");
}

// Lister les applications
void list_apps(AppManager* am) {
    printf("\nApplications installées:\n");
    printf("Icône\tNom\t\tVersion\t\tCatégorie\t\tDescription\n");
    printf("----------------------------------------------------------------------\n");

    for (int i = 0; i < am->count; i++) {
        Application* app = &am->apps[i];
        printf("%s\t", app->icon);
        printf("%s\t\t", app->name);
        printf("%s\t\t", app->version);
        printf("%s\t\t", app->category);
        printf("%s\n", app->description);
    }
}

// Rechercher une application
Application* find_app(AppManager* am, const char* name) {
    for (int i = 0; i < am->count; i++) {
        if (strcmp(am->apps[i].name, name) == 0) {
            return &am->apps[i];
        }
    }
    return NULL;
}

// Désinstaller une application
int uninstall_app(AppManager* am, const char* name) {
    for (int i = 0; i < am->count; i++) {
        if (strcmp(am->apps[i].name, name) == 0) {
            am->apps[i].is_installed = 0;
            return 0;
        }
    }
    return -1;
}

// Mettre à jour une application
int update_app(AppManager* am, const char* name, const char* new_version) {
    Application* app = find_app(am, name);
    if (app) {
        strncpy(app->version, new_version, 16);
        app->install_date = time(NULL);
        return 0;
    }
    return -1;
} 