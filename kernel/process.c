#include <stdint.h>
#include <stddef.h>

#define MAX_PROCESSES 256

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

process_t* processes[MAX_PROCESSES];
uint32_t current_pid = 0;

#define PROCESS_RUNNING 0
#define PROCESS_READY 1
#define PROCESS_BLOCKED 2
#define PROCESS_TERMINATED 3

process_t* create_process(const char* name, void* entry_point) {
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == 0) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        return 0;
    }

    process_t* process = (process_t*)kmalloc(sizeof(process_t));
    if (!process) {
        return 0;
    }

    process->pid = current_pid++;
    process->state = PROCESS_READY;
    strcpy(process->name, name);
    process->eip = (uint32_t)entry_point;
    process->page_directory = create_page_directory();
    process->stack = alloc_stack();

    processes[slot] = process;

    return process;
}

void terminate_process(process_t* process) {
    if (!process) {
        return;
    }

    free_page_directory(process->page_directory);
    free_stack(process->stack);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == process) {
            processes[i] = 0;
            break;
        }
    }

    kfree(process);
}

void set_process_state(process_t* process, uint8_t state) {
    if (!process) {
        return;
    }
    process->state = state;
}

process_t* get_current_process() {
    return processes[current_pid];
}

void init_process_manager() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i] = 0;
    }

    create_process("idle", idle_process);
} 