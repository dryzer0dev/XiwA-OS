#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void configure_firewall() {
    printf("Configuration du pare-feu...\n");
    system("sudo iptables -F");
    system("sudo iptables -P INPUT DROP");
    system("sudo iptables -P FORWARD DROP");
    system("sudo iptables -P OUTPUT ACCEPT");
    system("sudo iptables -A INPUT -i lo -j ACCEPT");
    system("sudo iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT");
    system("sudo iptables -A INPUT -p tcp --dport 22 -j ACCEPT");
    printf("Pare-feu configuré avec succès!\n");
}

void show_firewall_rules() {
    printf("Règles actuelles du pare-feu:\n");
    system("sudo iptables -L -v -n");
}

void add_firewall_rule(const char *rule) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sudo iptables %s", rule);
    system(cmd);
    printf("Règle ajoutée: %s\n", rule);
}

void save_firewall_rules() {
    printf("Sauvegarde des règles du pare-feu...\n");
    system("sudo iptables-save > /etc/iptables/rules.v4");
    printf("Règles sauvegardées!\n");
} 