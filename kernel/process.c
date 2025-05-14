#include <stdint.h>
#include <stddef.h>

#define MAX_PROCESSES 256

// Structure pour représenter un processus
typedef struct {
    uint32_t pid;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t page_directory;
    uint32_t stack;
    uint8_t state;
    char name[32];
} process_t;

// Table des processus
process_t* processes[MAX_PROCESSES];
uint32_t current_pid = 0;

// État d'un processus
#define PROCESS_RUNNING 0
#define PROCESS_READY 1
#define PROCESS_BLOCKED 2
#define PROCESS_TERMINATED 3

// Fonction pour créer un nouveau processus
process_t* create_process(const char* name, void* entry_point) {
    // Trouver un slot libre
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == 0) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        return 0; // Plus de place
    }

    // Allouer la structure du processus
    process_t* process = (process_t*)kmalloc(sizeof(process_t));
    if (!process) {
        return 0;
    }

    // Initialiser le processus
    process->pid = current_pid++;
    process->state = PROCESS_READY;
    strcpy(process->name, name);
    process->eip = (uint32_t)entry_point;
    process->page_directory = create_page_directory();
    process->stack = alloc_stack();

    // Sauvegarder le processus
    processes[slot] = process;

    return process;
}

// Fonction pour terminer un processus
void terminate_process(process_t* process) {
    if (!process) {
        return;
    }

    // Libérer les ressources
    free_page_directory(process->page_directory);
    free_stack(process->stack);

    // Retirer le processus de la table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == process) {
            processes[i] = 0;
            break;
        }
    }

    // Libérer la structure
    kfree(process);
}

// Fonction pour changer l'état d'un processus
void set_process_state(process_t* process, uint8_t state) {
    if (!process) {
        return;
    }
    process->state = state;
}

// Fonction pour obtenir le processus courant
process_t* get_current_process() {
    return processes[current_pid];
}

// Fonction pour initialiser le gestionnaire de processus
void init_process_manager() {
    // Initialiser la table des processus
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i] = 0;
    }

    // Créer le processus idle
    create_process("idle", idle_process);
} 