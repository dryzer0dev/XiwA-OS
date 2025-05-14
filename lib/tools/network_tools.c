#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Nmap
void launch_nmap(void) {
    clear_screen();
    print("=== Nmap Scanner ===\n\n");
    print("Options disponibles :\n");
    print("1. Scan rapide (-F)\n");
    print("2. Scan complet (-p-)\n");
    print("3. Scan de vulnérabilités (-sV)\n");
    print("4. Scan OS (-O)\n");
    print("\nEntrez l'adresse IP à scanner : ");
    
    char ip[16];
    gets(ip);
    
    print("\nLancement du scan...\n");
    // TODO: Implémenter le scan réel
    print("Scan terminé !\n");
}

// Wireshark
void launch_wireshark(void) {
    clear_screen();
    print("=== Wireshark ===\n\n");
    print("Options disponibles :\n");
    print("1. Capturer sur l'interface par défaut\n");
    print("2. Capturer sur une interface spécifique\n");
    print("3. Analyser un fichier de capture\n");
    print("\nChoisissez une option : ");
    
    char option = getchar();
    
    switch (option) {
        case '1':
            print("\nDémarrage de la capture...\n");
            // TODO: Implémenter la capture
            break;
        case '2':
            print("\nEntrez le nom de l'interface : ");
            char interface[32];
            gets(interface);
            print("\nDémarrage de la capture sur ");
            print(interface);
            print("...\n");
            // TODO: Implémenter la capture
            break;
        case '3':
            print("\nEntrez le chemin du fichier : ");
            char filepath[256];
            gets(filepath);
            print("\nAnalyse du fichier...\n");
            // TODO: Implémenter l'analyse
            break;
    }
} 