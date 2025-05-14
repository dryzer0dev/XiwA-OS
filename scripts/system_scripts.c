#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

void create_system_backup() {
    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));
    
    char cmd[512];
    printf("Création d'une sauvegarde système...\n");
    snprintf(cmd, sizeof(cmd), "sudo tar -czf /backup/system_%s.tar.gz /etc /var/lib", timestamp);
    system(cmd);
    printf("Sauvegarde créée: system_%s.tar.gz\n", timestamp);
}

void system_update() {
    printf("Mise à jour du système...\n");
    system("sudo apt-get update");
    system("sudo apt-get upgrade -y");
    system("sudo apt-get dist-upgrade -y");
    system("sudo apt-get autoremove -y");
    printf("Système mis à jour!\n");
}

void check_system_health() {
    printf("Vérification de la santé du système...\n");
    system("df -h");
    system("free -h");
    system("top -b -n 1");
    system("netstat -tuln");
    printf("Vérification terminée!\n");
}

void create_cron_job(const char *schedule, const char *command) {
    char cmd[512];
    printf("Création d'une tâche cron...\n");
    snprintf(cmd, sizeof(cmd), "(crontab -l 2>/dev/null; echo \"%s %s\") | crontab -", schedule, command);
    system(cmd);
    printf("Tâche cron créée!\n");
}

void system_cleanup() {
    printf("Nettoyage du système...\n");
    system("sudo apt-get clean");
    system("sudo apt-get autoremove -y");
    system("sudo journalctl --vacuum-time=3d");
    printf("Nettoyage terminé!\n");
} 