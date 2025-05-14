#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>

void create_sandbox_env() {
    printf("Création de l'environnement sandbox...\n");
    system("mkdir -p /tmp/sandbox");
    system("chmod 700 /tmp/sandbox");
    printf("Environnement sandbox créé!\n");
}

void run_in_sandbox(const char *program) {
    printf("Exécution de %s dans le sandbox...\n", program);
    
    // Création d'un environnement isolé
    if (chroot("/tmp/sandbox") != 0) {
        perror("chroot failed");
        return;
    }
    
    // Restriction des capacités
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        perror("prctl failed");
        return;
    }
    
    // Exécution du programme
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "chroot /tmp/sandbox %s", program);
    system(cmd);
}

void cleanup_sandbox() {
    printf("Nettoyage du sandbox...\n");
    system("rm -rf /tmp/sandbox/*");
    printf("Sandbox nettoyé!\n");
}

void monitor_sandbox() {
    printf("Surveillance du sandbox:\n");
    system("ps aux | grep sandbox");
    system("lsof | grep sandbox");
} 