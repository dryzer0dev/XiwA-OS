#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FIRST_BOOT_FILE "/etc/first_boot_done"
#define WELCOME_MESSAGE "\
╔════════════════════════════════════════════════════════════╗\n\
║                    Bienvenue sur XiwA-OS                    ║\n\
║                                                            ║\n\
║  Nous allons configurer votre système avec les meilleures   ║\n\
║  applications pour une expérience optimale.                 ║\n\
║                                                            ║\n\
║  Cette installation peut prendre quelques minutes.         ║\n\
║  Veuillez ne pas éteindre votre ordinateur.               ║\n\
╚════════════════════════════════════════════════════════════╝\n"

// Vérifier si c'est le premier démarrage
int is_first_boot() {
    FILE* file = fopen(FIRST_BOOT_FILE, "r");
    if (file) {
        fclose(file);
        return 0;
    }
    return 1;
}

// Marquer le premier démarrage comme terminé
void mark_first_boot_done() {
    FILE* file = fopen(FIRST_BOOT_FILE, "w");
    if (file) {
        fprintf(file, "First boot completed on %s", ctime(time(NULL)));
        fclose(file);
    }
}

// Afficher la progression
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

// Fonction principale de premier démarrage
int first_boot_setup() {
    if (!is_first_boot()) {
        return 0; // Pas le premier démarrage
    }

    // Afficher le message de bienvenue
    printf("%s\n", WELCOME_MESSAGE);
    printf("Démarrage de l'installation automatique dans 5 secondes...\n");
    sleep(5);

    // Créer les dossiers nécessaires
    printf("Création des dossiers système...\n");
    system("mkdir -p /opt /usr/bin /etc");

    // Lancer l'installation automatique
    printf("Lancement de l'installation des applications...\n");
    system("/usr/bin/auto_installer");

    // Marquer le premier démarrage comme terminé
    mark_first_boot_done();

    printf("\n\n=== Installation terminée avec succès! ===\n");
    printf("Votre système est maintenant prêt à être utilisé.\n");
    printf("Redémarrage dans 10 secondes...\n");
    sleep(10);
    system("reboot");

    return 0;
}

// Fonction principale
int main() {
    return first_boot_setup();
} 