#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_RUNNING_APPS 50
#define MAX_APP_ARGS 10

typedef struct {
    char name[256];
    char path[1024];
    int pid;
    time_t start_time;
    int is_running;
    char args[MAX_APP_ARGS][256];
    int arg_count;
} RunningApp;

typedef struct {
    RunningApp apps[MAX_RUNNING_APPS];
    int count;
} AppRunner;

// Initialisation du gestionnaire d'applications
AppRunner* init_app_runner() {
    AppRunner* runner = (AppRunner*)malloc(sizeof(AppRunner));
    if (!runner) return NULL;
    runner->count = 0;
    return runner;
}

// Lancer une application
int launch_app(AppRunner* runner, const char* name, const char* path, char** args, int arg_count) {
    if (runner->count >= MAX_RUNNING_APPS) return -1;

    RunningApp* app = &runner->apps[runner->count];
    strncpy(app->name, name, 256);
    strncpy(app->path, path, 1024);
    app->pid = runner->count + 1000; // Simuler un PID
    app->start_time = time(NULL);
    app->is_running = 1;
    app->arg_count = arg_count;

    // Copier les arguments
    for (int i = 0; i < arg_count && i < MAX_APP_ARGS; i++) {
        strncpy(app->args[i], args[i], 256);
    }

    printf("Lancement de %s (PID: %d)...\n", name, app->pid);
    runner->count++;
    return app->pid;
}

// Arrêter une application
int stop_app(AppRunner* runner, int pid) {
    for (int i = 0; i < runner->count; i++) {
        if (runner->apps[i].pid == pid) {
            printf("Arrêt de %s (PID: %d)...\n", runner->apps[i].name, pid);
            runner->apps[i].is_running = 0;
            return 0;
        }
    }
    return -1;
}

// Lister les applications en cours d'exécution
void list_running_apps(AppRunner* runner) {
    printf("\nApplications en cours d'exécution:\n");
    printf("Nom\t\tPID\t\tTemps d'exécution\tÉtat\n");
    printf("----------------------------------------------------------------------\n");

    for (int i = 0; i < runner->count; i++) {
        RunningApp* app = &runner->apps[i];
        time_t runtime = time(NULL) - app->start_time;
        printf("%s\t\t%d\t\t%ld secondes\t\t%s\n",
               app->name, app->pid, runtime,
               app->is_running ? "En cours" : "Arrêté");
    }
}

// Exemple d'utilisation
void example_usage() {
    AppRunner* runner = init_app_runner();
    
    // Lancer Epic Games
    char* epic_args[] = {"--no-sandbox"};
    launch_app(runner, "Epic Games", "/usr/bin/epic-games", epic_args, 1);
    
    // Lancer Google Chrome
    char* chrome_args[] = {"--new-window", "https://www.google.com"};
    launch_app(runner, "Google Chrome", "/usr/bin/google-chrome", chrome_args, 2);
    
    // Afficher les applications en cours d'exécution
    list_running_apps(runner);
    
    // Arrêter une application
    stop_app(runner, 1000);
    
    // Afficher à nouveau
    list_running_apps(runner);
} 