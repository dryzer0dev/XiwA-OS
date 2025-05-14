#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pcap.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <stdio.h>

#define MAX_TOOLS 100
#define TOOL_NAME_LENGTH 32
#define TOOL_DESC_LENGTH 128

typedef struct {
    char name[TOOL_NAME_LENGTH];
    char description[TOOL_DESC_LENGTH];
    void (*launch_function)(void);
    char category[20];
} Tool;

// Déclarations des fonctions de lancement des outils
void launch_nmap(void);
void launch_wireshark(void);
void launch_metasploit(void);
void launch_sqlmap(void);
void launch_aircrack(void);
void launch_maltego(void);
void launch_theharvester(void);
void launch_recon_ng(void);
void launch_osint_framework(void);
void launch_shodan(void);
void launch_hydra(void);
void launch_john(void);
void launch_hashcat(void);
void launch_cewl(void);
void launch_dirb(void);
void launch_nikto(void);
void launch_whatweb(void);
void launch_skipfish(void);
void launch_wapiti(void);
void launch_beef(void);
void launch_set(void);
void launch_weevely(void);
void launch_websploit(void);
void launch_armitage(void);
void launch_nessus(void);
void launch_openvas(void);
void launch_arachni(void);
void launch_zap(void);
void launch_burp(void);
void launch_ghidra(void);
void launch_radare2(void);
void launch_ida(void);
void launch_ollydbg(void);
void launch_volatility(void);
void launch_autopsy(void);
void launch_foremost(void);
void launch_scalpel(void);
void launch_testdisk(void);
void launch_photorec(void);
void launch_extundelete(void);
void launch_ddrescue(void);
void launch_airgeddon(void);
void launch_kismet(void);
void launch_reaver(void);
void launch_wifite(void);
void launch_fern(void);
void launch_wifiphisher(void);
void launch_bettercap(void);
void launch_ettercap(void);
void launch_responder(void);
void launch_impacket(void);
void launch_crackmapexec(void);
void launch_enum4linux(void);
void launch_smbmap(void);
void launch_ldapsearch(void);
void launch_kerbrute(void);
void launch_mimikatz(void);
void launch_bloodhound(void);
void launch_powerup(void);
void launch_powerless(void);
void launch_empire(void);
void launch_covenant(void);
void launch_sliver(void);
void launch_merlin(void);
void launch_pupy(void);
void launch_hoaxshell(void);
void launch_chameleon(void);
void launch_eviltwin(void);
void launch_wifijammer(void);
void launch_mdk3(void);
void launch_mdk4(void);
void launch_fluxion(void);
void launch_wifite2(void);
void launch_airgeddon2(void);
void launch_wifiphisher2(void);
void launch_aircrack_ng(void);
void launch_hashcat_gpu(void);
void launch_john_gpu(void);
void launch_oclhashcat(void);
void launch_cuda_hashcat(void);
void launch_oclhashcat_plus(void);
void launch_cuda_hashcat_plus(void);
void launch_hashcat_utils(void);
void launch_john_utils(void);
void launch_hashcat_plugins(void);
void launch_john_plugins(void);
void launch_hashcat_rules(void);
void launch_john_rules(void);
void launch_hashcat_masks(void);
void launch_john_masks(void);
void launch_hashcat_charsets(void);
void launch_john_charsets(void);
void launch_hashcat_benchmark(void);
void launch_john_benchmark(void);
void launch_hashcat_verify(void);
void launch_john_verify(void);
void launch_hashcat_show(void);
void launch_john_show(void);
void launch_hashcat_list(void);
void launch_john_list(void);
void launch_hashcat_help(void);
void launch_john_help(void);

// Liste des outils disponibles
Tool tools[MAX_TOOLS] = {
    {"Nmap", "Scanner de ports et d'hôtes réseau", launch_nmap, "Network"},
    {"Wireshark", "Analyseur de trafic réseau", launch_wireshark, "Network"},
    {"Hydra", "Cracker de mots de passe", launch_hydra, "Password"},
    {"John", "Cracker de hachages", launch_john, "Password"},
    {"Hashcat", "Cracker de hachages GPU", launch_hashcat, "Password"},
    {"Cewl", "Générateur de wordlists", launch_cewl, "Password"},
    {"Dirb", "Scanner de répertoires web", launch_dirb, "Web"},
    {"Nikto", "Scanner de vulnérabilités web", launch_nikto, "Web"},
    {"WhatWeb", "Scanner de technologies web", launch_whatweb, "Web"},
    {"Skipfish", "Scanner de sécurité web", launch_skipfish, "Web"},
    {"Wapiti", "Scanner de vulnérabilités web", launch_wapiti, "Web"},
    {"BeEF", "Framework d'exploitation de navigateur", launch_beef, "Exploitation"},
    {"SET", "Social Engineering Toolkit", launch_set, "Exploitation"},
    {"Weevely", "Web shell", launch_weevely, "Exploitation"},
    {"WebSploit", "Framework d'exploitation web", launch_websploit, "Exploitation"},
    {"Armitage", "Interface graphique pour Metasploit", launch_armitage, "Exploitation"},
    {"Nessus", "Scanner de vulnérabilités", launch_nessus, "Vulnerability"},
    {"OpenVAS", "Scanner de vulnérabilités", launch_openvas, "Vulnerability"},
    {"Arachni", "Scanner de vulnérabilités web", launch_arachni, "Vulnerability"},
    {"ZAP", "Proxy d'analyse de sécurité", launch_zap, "Vulnerability"},
    {"Burp", "Suite d'outils de sécurité web", launch_burp, "Vulnerability"},
    {"Ghidra", "Framework de reverse engineering", launch_ghidra, "Reverse"},
    {"Radare2", "Framework d'analyse binaire", launch_radare2, "Reverse"},
    {"IDA", "Désassembleur et débogueur", launch_ida, "Reverse"},
    {"OllyDbg", "Débogueur Windows", launch_ollydbg, "Reverse"},
    {"Volatility", "Analyse de mémoire", launch_volatility, "Forensics"},
    {"Autopsy", "Plateforme d'analyse forensique", launch_autopsy, "Forensics"},
    {"Foremost", "Récupération de fichiers", launch_foremost, "Forensics"},
    {"Scalpel", "Récupération de fichiers", launch_scalpel, "Forensics"},
    {"TestDisk", "Récupération de partitions", launch_testdisk, "Forensics"},
    {"PhotoRec", "Récupération de fichiers", launch_photorec, "Forensics"},
    {"Extundelete", "Récupération de fichiers ext", launch_extundelete, "Forensics"},
    {"DDRescue", "Récupération de données", launch_ddrescue, "Forensics"},
    {"Airgeddon", "Suite d'outils WiFi", launch_airgeddon, "Wireless"},
    {"Kismet", "Détecteur de réseaux sans fil", launch_kismet, "Wireless"},
    {"Reaver", "Cracker de WPS", launch_reaver, "Wireless"},
    {"Wifite", "Cracker de réseaux WiFi", launch_wifite, "Wireless"},
    {"Fern", "Cracker de réseaux WiFi", launch_fern, "Wireless"},
    {"Wifiphisher", "Attaques de phishing WiFi", launch_wifiphisher, "Wireless"},
    {"Bettercap", "Framework d'attaque réseau", launch_bettercap, "Network"},
    {"Ettercap", "Intercepteur de trafic réseau", launch_ettercap, "Network"},
    {"Responder", "Attaques de réseau local", launch_responder, "Network"},
    {"Impacket", "Collection d'outils réseau", launch_impacket, "Network"},
    {"CrackMapExec", "Outils d'énumération réseau", launch_crackmapexec, "Network"},
    {"Enum4linux", "Énumération SMB", launch_enum4linux, "Network"},
    {"SMBMap", "Exploration de partages SMB", launch_smbmap, "Network"},
    {"LDAPSearch", "Recherche LDAP", launch_ldapsearch, "Network"},
    {"Kerbrute", "Attaques Kerberos", launch_kerbrute, "Network"},
    {"Mimikatz", "Extraction de credentials", launch_mimikatz, "Exploitation"},
    {"BloodHound", "Analyse de domaine Active Directory", launch_bloodhound, "Exploitation"},
    {"PowerUp", "Privilege Escalation Windows", launch_powerup, "Exploitation"},
    {"PowerLess", "Privilege Escalation Windows", launch_powerless, "Exploitation"},
    {"Empire", "Framework post-exploitation", launch_empire, "Exploitation"},
    {"Covenant", "Framework C2", launch_covenant, "Exploitation"},
    {"Sliver", "Framework C2", launch_sliver, "Exploitation"},
    {"Merlin", "Framework C2", launch_merlin, "Exploitation"},
    {"Pupy", "Framework post-exploitation", launch_pupy, "Exploitation"},
    {"HoaxShell", "Shell inversé", launch_hoaxshell, "Exploitation"},
    {"Chameleon", "Framework d'évasion", launch_chameleon, "Exploitation"},
    {"EvilTwin", "Attaques de réseau WiFi", launch_eviltwin, "Wireless"},
    {"WifiJammer", "Brouilleur WiFi", launch_wifijammer, "Wireless"},
    {"MDK3", "Outils d'attaque WiFi", launch_mdk3, "Wireless"},
    {"MDK4", "Outils d'attaque WiFi", launch_mdk4, "Wireless"},
    {"Fluxion", "Attaques de réseau WiFi", launch_fluxion, "Wireless"},
    {"Wifite2", "Cracker de réseaux WiFi", launch_wifite2, "Wireless"},
    {"Airgeddon2", "Suite d'outils WiFi", launch_airgeddon2, "Wireless"},
    {"Wifiphisher2", "Attaques de phishing WiFi", launch_wifiphisher2, "Wireless"},
    {"Aircrack-ng", "Suite d'outils WiFi", launch_aircrack_ng, "Wireless"},
    {"Hashcat GPU", "Cracker de hachages GPU", launch_hashcat_gpu, "Password"},
    {"John GPU", "Cracker de hachages GPU", launch_john_gpu, "Password"},
    {"OCLHashcat", "Cracker de hachages OpenCL", launch_oclhashcat, "Password"},
    {"CUDA Hashcat", "Cracker de hachages CUDA", launch_cuda_hashcat, "Password"},
    {"OCLHashcat Plus", "Cracker de hachages OpenCL", launch_oclhashcat_plus, "Password"},
    {"CUDA Hashcat Plus", "Cracker de hachages CUDA", launch_cuda_hashcat_plus, "Password"},
    {"Hashcat Utils", "Utilitaires Hashcat", launch_hashcat_utils, "Password"},
    {"John Utils", "Utilitaires John", launch_john_utils, "Password"},
    {"Hashcat Plugins", "Plugins Hashcat", launch_hashcat_plugins, "Password"},
    {"John Plugins", "Plugins John", launch_john_plugins, "Password"},
    {"Hashcat Rules", "Règles Hashcat", launch_hashcat_rules, "Password"},
    {"John Rules", "Règles John", launch_john_rules, "Password"},
    {"Hashcat Masks", "Masques Hashcat", launch_hashcat_masks, "Password"},
    {"John Masks", "Masques John", launch_john_masks, "Password"},
    {"Hashcat Charsets", "Jeux de caractères Hashcat", launch_hashcat_charsets, "Password"},
    {"John Charsets", "Jeux de caractères John", launch_john_charsets, "Password"},
    {"Hashcat Benchmark", "Benchmark Hashcat", launch_hashcat_benchmark, "Password"},
    {"John Benchmark", "Benchmark John", launch_john_benchmark, "Password"},
    {"Hashcat Verify", "Vérification Hashcat", launch_hashcat_verify, "Password"},
    {"John Verify", "Vérification John", launch_john_verify, "Password"},
    {"Hashcat Show", "Affichage Hashcat", launch_hashcat_show, "Password"},
    {"John Show", "Affichage John", launch_john_show, "Password"},
    {"Hashcat List", "Liste Hashcat", launch_hashcat_list, "Password"},
    {"John List", "Liste John", launch_john_list, "Password"},
    {"Hashcat Help", "Aide Hashcat", launch_hashcat_help, "Password"},
    {"John Help", "Aide John", launch_john_help, "Password"}
};

int num_tools = 100; // Nombre d'outils actuellement définis

// Fonction pour afficher le menu des outils
void show_tools_menu(void) {
    clear_screen();
    print("=== XiMonOS Tools Launcher ===\n\n");
    
    // Affiche les catégories
    print("Catégories disponibles :\n");
    print("1. Network\n");
    print("2. Password\n");
    print("3. Web\n");
    print("4. Exploitation\n");
    print("5. Vulnerability\n");
    print("6. Reverse\n");
    print("7. Forensics\n");
    print("8. Wireless\n");
    print("\nAppuyez sur le numéro de la catégorie ou 'q' pour quitter\n");
}

// Fonction pour afficher les outils d'une catégorie
void show_category_tools(const char* category) {
    clear_screen();
    print("=== Outils ");
    print(category);
    print(" ===\n\n");
    
    int tool_num = 1;
    for (int i = 0; i < num_tools; i++) {
        if (strcmp(tools[i].category, category) == 0) {
            print(tool_num);
            print(". ");
            print(tools[i].name);
            print(" - ");
            print(tools[i].description);
            print("\n");
            tool_num++;
        }
    }
    
    print("\nAppuyez sur le numéro de l'outil ou 'b' pour retour\n");
}

// Fonction pour lancer un outil
void launch_tool(int tool_index) {
    if (tool_index >= 0 && tool_index < num_tools) {
        tools[tool_index].launch_function();
    }
}

// Fonction principale du lanceur d'outils
void tools_launcher_main(void) {
    bool running = true;
    char input;
    
    while (running) {
        show_tools_menu();
        input = getchar();
        
        switch (input) {
            case '1':
                show_category_tools("Network");
                break;
            case '2':
                show_category_tools("Password");
                break;
            case '3':
                show_category_tools("Web");
                break;
            case '4':
                show_category_tools("Exploitation");
                break;
            case '5':
                show_category_tools("Vulnerability");
                break;
            case '6':
                show_category_tools("Reverse");
                break;
            case '7':
                show_category_tools("Forensics");
                break;
            case '8':
                show_category_tools("Wireless");
                break;
            case 'q':
                running = false;
                break;
        }
    }
}

void launch_hydra(void) {
    // Vérifier/installer hydra
    if (system("which hydra > /dev/null 2>&1") != 0) {
        printf("Installation de Hydra...\n");
        system("sudo apt-get install -y hydra");
    }

    char ip[64], user[32], pass[32];
    printf("IP cible: "); scanf("%s", ip);
    printf("Utilisateur: "); scanf("%s", user);
    printf("Mot de passe: "); scanf("%s", pass);
    printf("Tentative brute-force sur %s avec %s:%s\n", ip, user, pass);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "hydra -l %s -p %s %s ssh", user, pass, ip);
    system(cmd);
}

void launch_john(void) {
    // Vérifier/installer john
    if (system("which john > /dev/null 2>&1") != 0) {
        printf("Installation de John the Ripper...\n");
        system("sudo apt-get install -y john");
    }

    char hash[256];
    printf("Hash à cracker: "); scanf("%s", hash);
    printf("Cracking...\n");
    
    FILE *tmp = fopen("/tmp/hash.txt", "w");
    fprintf(tmp, "%s", hash);
    fclose(tmp);
    
    system("john /tmp/hash.txt");
}

void launch_hashcat(void) {
    // Vérifier/installer hashcat
    if (system("which hashcat > /dev/null 2>&1") != 0) {
        printf("Installation de Hashcat...\n");
        system("sudo apt-get install -y hashcat");
    }

    char hash[256], wordlist[256];
    printf("Hash: "); scanf("%s", hash);
    printf("Wordlist: "); scanf("%s", wordlist);
    printf("Cracking GPU...\n");
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "hashcat -m 0 %s %s", hash, wordlist);
    system(cmd);
}

void launch_dirb(void) {
    // Vérifier/installer dirb
    if (system("which dirb > /dev/null 2>&1") != 0) {
        printf("Installation de Dirb...\n");
        system("sudo apt-get install -y dirb");
    }

    char url[128];
    printf("URL cible: "); scanf("%s", url);
    printf("Scan des répertoires cachés...\n");
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "dirb %s", url);
    system(cmd);
}

void launch_testdisk(void) {
    // Vérifier/installer testdisk
    if (system("which testdisk > /dev/null 2>&1") != 0) {
        printf("Installation de TestDisk...\n");
        system("sudo apt-get install -y testdisk");
    }

    printf("Scan des partitions perdues...\n");
    system("testdisk");
}

void launch_photorec(void) {
    // Vérifier/installer photorec (inclus dans testdisk)
    if (system("which photorec > /dev/null 2>&1") != 0) {
        printf("Installation de PhotoRec...\n");
        system("sudo apt-get install -y testdisk");
    }

    printf("Récupération de fichiers supprimés...\n");
    system("photorec");
}

void launch_theharvester(void) {
    // Vérifier/installer theharvester
    if (system("which theHarvester > /dev/null 2>&1") != 0) {
        printf("Installation de TheHarvester...\n");
        system("sudo apt-get install -y theharvester");
    }

    char domaine[128];
    printf("Domaine: "); scanf("%s", domaine);
    printf("Recherche d'emails et sous-domaines...\n");
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "theHarvester -d %s -b all", domaine);
    system(cmd);
}

void launch_shodan(void) {
    // Vérifier/installer shodan
    if (system("which shodan > /dev/null 2>&1") != 0) {
        printf("Installation de Shodan CLI...\n");
        system("sudo apt-get install -y python3-pip && pip3 install shodan");
    }

    char query[128];
    printf("Recherche Shodan: "); scanf("%s", query);
    printf("Résultats:\n");
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "shodan search %s", query);
    system(cmd);
}