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
    uint32_t esi, edi, ebp;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} cpu_state_t;

typedef struct {
    uint32_t thread_id;
    cpu_state_t cpu_state;
    uint8_t* stack;
    bool is_running;
    uint32_t priority;
    uint32_t quantum;
} thread_t;

typedef struct {
    uint32_t process_id;
    char name[32];
    uint32_t priority;
    thread_t threads[MAX_THREADS_PER_PROCESS];
    uint32_t thread_count;
    uint32_t memory_start;
    uint32_t memory_size;
    bool is_running;
    uint32_t parent_id;
    int exit_code;
} process_t;

typedef struct {
    process_t processes[MAX_PROCESSES];
    uint32_t process_count;
    uint32_t current_process;
    uint32_t current_thread;
    uint32_t next_process_id;
} process_manager_t;

process_manager_t process_manager;

void init_process_manager() {
    memset(&process_manager, 0, sizeof(process_manager_t));
    process_manager.next_process_id = 1;
}

uint32_t create_process(const char* name, uint32_t priority) {
    if (process_manager.process_count >= MAX_PROCESSES) {
        return 0;
    }

    process_t* process = &process_manager.processes[process_manager.process_count++];
    process->process_id = process_manager.next_process_id++;
    strncpy(process->name, name, 31);
    process->name[31] = '\0';
    process->priority = priority;
    process->thread_count = 0;
    process->is_running = true;
    process->parent_id = process_manager.current_process;
    process->exit_code = 0;

    // Allouer de la mémoire pour le processus
    process->memory_start = (uint32_t)vmalloc(STACK_SIZE * MAX_THREADS_PER_PROCESS);
    if (!process->memory_start) {
        process_manager.process_count--;
        return 0;
    }
    process->memory_size = STACK_SIZE * MAX_THREADS_PER_PROCESS;

    return process->process_id;
}

uint32_t create_thread(uint32_t process_id, void (*entry_point)(void*), void* arg) {
    process_t* process = NULL;
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].process_id == process_id) {
            process = &process_manager.processes[i];
            break;
        }
    }

    if (!process || process->thread_count >= MAX_THREADS_PER_PROCESS) {
        return 0;
    }

    thread_t* thread = &process->threads[process->thread_count++];
    thread->thread_id = process->thread_count;
    thread->stack = (uint8_t*)(process->memory_start + (thread->thread_id - 1) * STACK_SIZE);
    thread->is_running = true;
    thread->priority = process->priority;
    thread->quantum = 100; // 100ms par défaut

    // Initialiser l'état du CPU
    thread->cpu_state.esp = (uint32_t)thread->stack + STACK_SIZE;
    thread->cpu_state.eip = (uint32_t)entry_point;
    thread->cpu_state.cs = 0x08; // Segment de code noyau
    thread->cpu_state.ss = 0x10; // Segment de données noyau
    thread->cpu_state.eflags = 0x202; // IF=1, IOPL=0

    // Pousser l'argument sur la pile
    thread->cpu_state.esp -= 4;
    *(uint32_t*)thread->cpu_state.esp = (uint32_t)arg;

    return thread->thread_id;
}

void schedule() {
    if (process_manager.process_count == 0) {
        return;
    }

    // Trouver le prochain thread à exécuter
    uint32_t next_process = process_manager.current_process;
    uint32_t next_thread = process_manager.current_thread;
    uint32_t highest_priority = 0;

    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        process_t* process = &process_manager.processes[i];
        if (!process->is_running) continue;

        for (uint32_t j = 0; j < process->thread_count; j++) {
            thread_t* thread = &process->threads[j];
            if (!thread->is_running) continue;

            if (thread->priority > highest_priority) {
                highest_priority = thread->priority;
                next_process = i;
                next_thread = j;
            }
        }
    }

    // Sauvegarder l'état du thread courant
    if (process_manager.current_process < process_manager.process_count) {
        process_t* current_process = &process_manager.processes[process_manager.current_process];
        if (process_manager.current_thread < current_process->thread_count) {
            thread_t* current_thread = &current_process->threads[process_manager.current_thread];
            if (current_thread->is_running) {
                // Sauvegarder l'état du CPU
                asm volatile("movl %%eax, %0" : "=m"(current_thread->cpu_state.eax));
                asm volatile("movl %%ebx, %0" : "=m"(current_thread->cpu_state.ebx));
                asm volatile("movl %%ecx, %0" : "=m"(current_thread->cpu_state.ecx));
                asm volatile("movl %%edx, %0" : "=m"(current_thread->cpu_state.edx));
                asm volatile("movl %%esi, %0" : "=m"(current_thread->cpu_state.esi));
                asm volatile("movl %%edi, %0" : "=m"(current_thread->cpu_state.edi));
                asm volatile("movl %%ebp, %0" : "=m"(current_thread->cpu_state.ebp));
                asm volatile("movl %%esp, %0" : "=m"(current_thread->cpu_state.esp));
                asm volatile("movl (%%esp), %0" : "=r"(current_thread->cpu_state.eip));
            }
        }
    }

    // Restaurer l'état du nouveau thread
    process_manager.current_process = next_process;
    process_manager.current_thread = next_thread;

    process_t* next_process_ptr = &process_manager.processes[next_process];
    thread_t* next_thread_ptr = &next_process_ptr->threads[next_thread];

    // Restaurer l'état du CPU
    asm volatile("movl %0, %%eax" : : "m"(next_thread_ptr->cpu_state.eax));
    asm volatile("movl %0, %%ebx" : : "m"(next_thread_ptr->cpu_state.ebx));
    asm volatile("movl %0, %%ecx" : : "m"(next_thread_ptr->cpu_state.ecx));
    asm volatile("movl %0, %%edx" : : "m"(next_thread_ptr->cpu_state.edx));
    asm volatile("movl %0, %%esi" : : "m"(next_thread_ptr->cpu_state.esi));
    asm volatile("movl %0, %%edi" : : "m"(next_thread_ptr->cpu_state.edi));
    asm volatile("movl %0, %%ebp" : : "m"(next_thread_ptr->cpu_state.ebp));
    asm volatile("movl %0, %%esp" : : "m"(next_thread_ptr->cpu_state.esp));
    asm volatile("jmp *%0" : : "r"(next_thread_ptr->cpu_state.eip));
}

void terminate_process(uint32_t process_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].process_id == process_id) {
            process_t* process = &process_manager.processes[i];

            // Terminer tous les threads
            for (uint32_t j = 0; j < process->thread_count; j++) {
                process->threads[j].is_running = false;
            }

            // Libérer la mémoire
            vfree((void*)process->memory_start);

            // Supprimer le processus
            memmove(&process_manager.processes[i],
                    &process_manager.processes[i + 1],
                    (process_manager.process_count - i - 1) * sizeof(process_t));
            process_manager.process_count--;

            // Si c'était le processus courant, planifier un nouveau
            if (i == process_manager.current_process) {
                schedule();
            }
            break;
        }
    }
}

void terminate_thread(uint32_t process_id, uint32_t thread_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].process_id == process_id) {
            process_t* process = &process_manager.processes[i];

            for (uint32_t j = 0; j < process->thread_count; j++) {
                if (process->threads[j].thread_id == thread_id) {
                    process->threads[j].is_running = false;

                    // Supprimer le thread
                    memmove(&process->threads[j],
                            &process->threads[j + 1],
                            (process->thread_count - j - 1) * sizeof(thread_t));
                    process->thread_count--;

                    // Si c'était le thread courant, planifier un nouveau
                    if (i == process_manager.current_process &&
                        j == process_manager.current_thread) {
                        schedule();
                    }
                    break;
                }
            }
            break;
        }
    }
}

void set_process_priority(uint32_t process_id, uint32_t priority) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].process_id == process_id) {
            process_t* process = &process_manager.processes[i];
            process->priority = priority;

            // Mettre à jour la priorité de tous les threads
            for (uint32_t j = 0; j < process->thread_count; j++) {
                process->threads[j].priority = priority;
            }
            break;
        }
    }
}

void set_thread_priority(uint32_t process_id, uint32_t thread_id, uint32_t priority) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].process_id == process_id) {
            process_t* process = &process_manager.processes[i];

            for (uint32_t j = 0; j < process->thread_count; j++) {
                if (process->threads[j].thread_id == thread_id) {
                    process->threads[j].priority = priority;
                    break;
                }
            }
            break;
        }
    }
}

void sleep_process(uint32_t process_id, uint32_t milliseconds) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].process_id == process_id) {
            process_t* process = &process_manager.processes[i];
            process->is_running = false;

            // Planifier un nouveau processus
            schedule();

            // Attendre le temps spécifié
            uint32_t start_time = get_current_time();
            while (get_current_time() - start_time < milliseconds) {
                // Attendre
            }

            process->is_running = true;
            break;
        }
    }
}

void wake_process(uint32_t process_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].process_id == process_id) {
            process_manager.processes[i].is_running = true;
            break;
        }
    }
}

void yield() {
    // Sauvegarder l'état du thread courant
    if (process_manager.current_process < process_manager.process_count) {
        process_t* current_process = &process_manager.processes[process_manager.current_process];
        if (process_manager.current_thread < current_process->thread_count) {
            thread_t* current_thread = &current_process->threads[process_manager.current_thread];
            if (current_thread->is_running) {
                // Sauvegarder l'état du CPU
                asm volatile("movl %%eax, %0" : "=m"(current_thread->cpu_state.eax));
                asm volatile("movl %%ebx, %0" : "=m"(current_thread->cpu_state.ebx));
                asm volatile("movl %%ecx, %0" : "=m"(current_thread->cpu_state.ecx));
                asm volatile("movl %%edx, %0" : "=m"(current_thread->cpu_state.edx));
                asm volatile("movl %%esi, %0" : "=m"(current_thread->cpu_state.esi));
                asm volatile("movl %%edi, %0" : "=m"(current_thread->cpu_state.edi));
                asm volatile("movl %%ebp, %0" : "=m"(current_thread->cpu_state.ebp));
                asm volatile("movl %%esp, %0" : "=m"(current_thread->cpu_state.esp));
                asm volatile("movl (%%esp), %0" : "=r"(current_thread->cpu_state.eip));
            }
        }
    }

    // Planifier un nouveau thread
    schedule();
} 