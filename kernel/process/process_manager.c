#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PROCESSES 256
#define MAX_THREADS_PER_PROCESS 16
#define STACK_SIZE 4096
#define PROCESS_PRIORITY_LOW 0
#define PROCESS_PRIORITY_NORMAL 1
#define PROCESS_PRIORITY_HIGH 2

typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cs, ds, es, fs, gs, ss;
} CPUState;

typedef struct {
    uint32_t thread_id;
    CPUState state;
    uint32_t stack[STACK_SIZE];
    bool is_running;
    uint32_t priority;
    uint32_t quantum;
} Thread;

typedef struct {
    uint32_t process_id;
    char name[64];
    uint32_t priority;
    Thread threads[MAX_THREADS_PER_PROCESS];
    uint32_t thread_count;
    uint32_t memory_start;
    uint32_t memory_size;
    bool is_running;
    uint32_t parent_id;
    uint32_t exit_code;
} Process;

typedef struct {
    Process processes[MAX_PROCESSES];
    uint32_t process_count;
    uint32_t current_process;
    uint32_t current_thread;
    uint32_t next_process_id;
} ProcessManager;

ProcessManager pm;

void init_process_manager() {
    memset(&pm, 0, sizeof(ProcessManager));
    pm.next_process_id = 1;
}

Process* create_process(const char* name, uint32_t priority) {
    if (pm.process_count >= MAX_PROCESSES) {
        return NULL;
    }

    Process* process = &pm.processes[pm.process_count++];
    process->process_id = pm.next_process_id++;
    strncpy(process->name, name, 63);
    process->name[63] = '\0';
    process->priority = priority;
    process->thread_count = 0;
    process->is_running = true;
    process->parent_id = pm.current_process;
    process->exit_code = 0;

    // Allouer de la mémoire pour le processus
    process->memory_start = kmalloc(1024 * 1024); // 1MB par défaut
    process->memory_size = 1024 * 1024;

    return process;
}

Thread* create_thread(Process* process, void* entry_point) {
    if (process->thread_count >= MAX_THREADS_PER_PROCESS) {
        return NULL;
    }

    Thread* thread = &process->threads[process->thread_count++];
    thread->thread_id = process->thread_count;
    thread->is_running = true;
    thread->priority = process->priority;
    thread->quantum = 100; // 100ms par défaut

    // Initialiser l'état du thread
    memset(&thread->state, 0, sizeof(CPUState));
    thread->state.eip = (uint32_t)entry_point;
    thread->state.esp = (uint32_t)&thread->stack[STACK_SIZE - 1];
    thread->state.eflags = 0x202; // IF=1, IOPL=0

    return thread;
}

void schedule() {
    // Trouver le prochain thread à exécuter
    uint32_t next_process = pm.current_process;
    uint32_t next_thread = pm.current_thread;
    uint32_t highest_priority = 0;

    for (uint32_t i = 0; i < pm.process_count; i++) {
        Process* process = &pm.processes[i];
        if (!process->is_running) continue;

        for (uint32_t j = 0; j < process->thread_count; j++) {
            Thread* thread = &process->threads[j];
            if (!thread->is_running) continue;

            if (thread->priority > highest_priority) {
                highest_priority = thread->priority;
                next_process = i;
                next_thread = j;
            }
        }
    }

    // Effectuer le changement de contexte si nécessaire
    if (next_process != pm.current_process || next_thread != pm.current_thread) {
        if (pm.current_process < pm.process_count) {
            Thread* current_thread = &pm.processes[pm.current_process].threads[pm.current_thread];
            save_cpu_state(&current_thread->state);
        }

        pm.current_process = next_process;
        pm.current_thread = next_thread;

        Thread* next_thread_ptr = &pm.processes[next_process].threads[next_thread];
        load_cpu_state(&next_thread_ptr->state);
    }
}

void terminate_process(Process* process) {
    if (!process) return;

    process->is_running = false;
    kfree((void*)process->memory_start);

    // Si c'est le processus courant, planifier un nouveau processus
    if (process == &pm.processes[pm.current_process]) {
        schedule();
    }
}

void terminate_thread(Thread* thread) {
    if (!thread) return;

    thread->is_running = false;

    // Si c'est le thread courant, planifier un nouveau thread
    if (thread == &pm.processes[pm.current_process].threads[pm.current_thread]) {
        schedule();
    }
}

void set_process_priority(Process* process, uint32_t priority) {
    if (!process) return;

    process->priority = priority;
    for (uint32_t i = 0; i < process->thread_count; i++) {
        process->threads[i].priority = priority;
    }
}

void set_thread_priority(Thread* thread, uint32_t priority) {
    if (!thread) return;
    thread->priority = priority;
}

void sleep_process(Process* process, uint32_t milliseconds) {
    if (!process) return;

    process->is_running = false;
    // Ajouter le processus à la liste des processus en attente
    add_to_sleep_queue(process, milliseconds);
    schedule();
}

void wake_process(Process* process) {
    if (!process) return;

    process->is_running = true;
    schedule();
}

void yield() {
    // Forcer un changement de contexte
    schedule();
}

void save_cpu_state(CPUState* state) {
    // Sauvegarder l'état du CPU
    asm volatile(
        "movl %%eax, 0(%%edi)\n"
        "movl %%ebx, 4(%%edi)\n"
        "movl %%ecx, 8(%%edi)\n"
        "movl %%edx, 12(%%edi)\n"
        "movl %%esi, 16(%%edi)\n"
        "movl %%edi, 20(%%edi)\n"
        "movl %%ebp, 24(%%edi)\n"
        "movl %%esp, 28(%%edi)\n"
        "movl $1f, 32(%%edi)\n"
        "pushfl\n"
        "popl 36(%%edi)\n"
        "movl %%cs, 40(%%edi)\n"
        "movl %%ds, 44(%%edi)\n"
        "movl %%es, 48(%%edi)\n"
        "movl %%fs, 52(%%edi)\n"
        "movl %%gs, 56(%%edi)\n"
        "movl %%ss, 60(%%edi)\n"
        "1:\n"
        : : "D" (state)
    );
}

void load_cpu_state(CPUState* state) {
    // Charger l'état du CPU
    asm volatile(
        "movl 0(%%esi), %%eax\n"
        "movl 4(%%esi), %%ebx\n"
        "movl 8(%%esi), %%ecx\n"
        "movl 12(%%esi), %%edx\n"
        "movl 16(%%esi), %%esi\n"
        "movl 20(%%edi), %%edi\n"
        "movl 24(%%edi), %%ebp\n"
        "movl 28(%%edi), %%esp\n"
        "pushl 36(%%edi)\n"
        "popfl\n"
        "movl 40(%%edi), %%cs\n"
        "movl 44(%%edi), %%ds\n"
        "movl 48(%%edi), %%es\n"
        "movl 52(%%edi), %%fs\n"
        "movl 56(%%edi), %%gs\n"
        "movl 60(%%edi), %%ss\n"
        "jmp *32(%%edi)\n"
        : : "S" (state)
    );
} 