#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

void install_snort() {
    printf("Installation de Snort...\n");
    system("sudo apt-get update");
    system("sudo apt-get install -y snort");
    printf("Snort installé avec succès!\n");
}

void configure_snort() {
    printf("Configuration de Snort...\n");
    system("sudo cp /etc/snort/snort.conf /etc/snort/snort.conf.backup");
    system("sudo sed -i 's/var HOME_NET any/var HOME_NET 192.168.1.0\\/24/' /etc/snort/snort.conf");
    printf("Snort configuré!\n");
}

void start_snort() {
    printf("Démarrage de Snort en mode IDS...\n");
    system("sudo snort -dev -l /var/log/snort");
}

void check_snort_logs() {
    printf("Analyse des logs Snort:\n");
    system("sudo tail -n 50 /var/log/snort/alert");
}

void update_snort_rules() {
    printf("Mise à jour des règles Snort...\n");
    system("sudo snort -c /etc/snort/snort.conf -T");
    printf("Règles mises à jour!\n");
} 