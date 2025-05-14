#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PROCESSES 256
#define MAX_THREADS_PER_PROCESS 32
#define STACK_SIZE 4096
#define PROCESS_PRIORITY_LOW 0
#define PROCESS_PRIORITY_NORMAL 1
#define PROCESS_PRIORITY_HIGH 2
#define THREAD_PRIORITY_LOW 0
#define THREAD_PRIORITY_NORMAL 1
#define THREAD_PRIORITY_HIGH 2

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
    uint32_t id;
    uint32_t process_id;
    uint32_t priority;
    bool running;
    cpu_state_t state;
    void* stack;
} thread_t;

typedef struct {
    uint32_t id;
    char name[32];
    uint32_t priority;
    bool running;
    uint32_t thread_count;
    thread_t* threads[MAX_THREADS_PER_PROCESS];
    void* code_segment;
    void* data_segment;
    void* heap;
    uint32_t heap_size;
} process_t;

typedef struct {
    process_t processes[MAX_PROCESSES];
    uint32_t process_count;
    thread_t* current_thread;
    uint32_t next_process_id;
    uint32_t next_thread_id;
} process_manager_t;

process_manager_t process_manager;

void init_process_manager() {
    memset(&process_manager, 0, sizeof(process_manager_t));
    process_manager.next_process_id = 1;
    process_manager.next_thread_id = 1;
}

uint32_t create_process(const char* name, uint32_t priority) {
    if (process_manager.process_count >= MAX_PROCESSES) {
        return 0;
    }

    process_t* process = &process_manager.processes[process_manager.process_count++];
    process->id = process_manager.next_process_id++;
    strncpy(process->name, name, sizeof(process->name) - 1);
    process->name[sizeof(process->name) - 1] = '\0';
    process->priority = priority;
    process->running = false;
    process->thread_count = 0;
    process->heap = NULL;
    process->heap_size = 0;

    return process->id;
}

void terminate_process(uint32_t process_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].id == process_id) {
            process_t* process = &process_manager.processes[i];

            // Terminer tous les threads
            for (uint32_t j = 0; j < process->thread_count; j++) {
                if (process->threads[j]) {
                    terminate_thread(process->threads[j]->id);
                }
            }

            // Libérer la mémoire
            if (process->code_segment) {
                vfree(process->code_segment);
            }
            if (process->data_segment) {
                vfree(process->data_segment);
            }
            if (process->heap) {
                vfree(process->heap);
            }

            // Supprimer le processus
            memmove(&process_manager.processes[i],
                    &process_manager.processes[i + 1],
                    (process_manager.process_count - i - 1) * sizeof(process_t));
            process_manager.process_count--;
            break;
        }
    }
}

uint32_t create_thread(uint32_t process_id, void (*entry)(void*), void* arg, uint32_t priority) {
    process_t* process = NULL;
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].id == process_id) {
            process = &process_manager.processes[i];
            break;
        }
    }

    if (!process || process->thread_count >= MAX_THREADS_PER_PROCESS) {
        return 0;
    }

    thread_t* thread = (thread_t*)kmalloc(sizeof(thread_t));
    if (!thread) {
        return 0;
    }

    thread->id = process_manager.next_thread_id++;
    thread->process_id = process_id;
    thread->priority = priority;
    thread->running = false;

    // Allouer la pile
    thread->stack = kmalloc_aligned(STACK_SIZE, PAGE_SIZE);
    if (!thread->stack) {
        kfree(thread);
        return 0;
    }

    // Initialiser l'état du CPU
    thread->state.esp = (uint32_t)thread->stack + STACK_SIZE;
    thread->state.eip = (uint32_t)entry;
    thread->state.cs = 0x08;
    thread->state.ss = 0x10;
    thread->state.eflags = 0x202;

    // Empiler l'argument
    thread->state.esp -= sizeof(void*);
    *(void**)thread->state.esp = arg;

    process->threads[process->thread_count++] = thread;
    return thread->id;
}

void terminate_thread(uint32_t thread_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        process_t* process = &process_manager.processes[i];
        for (uint32_t j = 0; j < process->thread_count; j++) {
            if (process->threads[j] && process->threads[j]->id == thread_id) {
                thread_t* thread = process->threads[j];

                // Libérer la pile
                if (thread->stack) {
                    kfree(thread->stack);
                }

                // Supprimer le thread
                memmove(&process->threads[j],
                        &process->threads[j + 1],
                        (process->thread_count - j - 1) * sizeof(thread_t*));
                process->thread_count--;

                kfree(thread);
                return;
            }
        }
    }
}

void schedule() {
    if (!process_manager.current_thread) {
        // Trouver le premier thread prêt
        for (uint32_t i = 0; i < process_manager.process_count; i++) {
            process_t* process = &process_manager.processes[i];
            if (!process->running) {
                continue;
            }

            for (uint32_t j = 0; j < process->thread_count; j++) {
                if (process->threads[j] && process->threads[j]->running) {
                    process_manager.current_thread = process->threads[j];
                    return;
                }
            }
        }
        return;
    }

    // Sauvegarder l'état du thread actuel
    cpu_state_t* current_state = &process_manager.current_thread->state;
    asm volatile("movl %%eax, %0" : "=m"(current_state->eax));
    asm volatile("movl %%ebx, %0" : "=m"(current_state->ebx));
    asm volatile("movl %%ecx, %0" : "=m"(current_state->ecx));
    asm volatile("movl %%edx, %0" : "=m"(current_state->edx));
    asm volatile("movl %%esi, %0" : "=m"(current_state->esi));
    asm volatile("movl %%edi, %0" : "=m"(current_state->edi));
    asm volatile("movl %%ebp, %0" : "=m"(current_state->ebp));
    asm volatile("movl %%esp, %0" : "=m"(current_state->esp));
    asm volatile("pushfl");
    asm volatile("popl %0" : "=m"(current_state->eflags));

    // Trouver le prochain thread à exécuter
    thread_t* next_thread = NULL;
    uint32_t highest_priority = 0;

    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        process_t* process = &process_manager.processes[i];
        if (!process->running) {
            continue;
        }

        for (uint32_t j = 0; j < process->thread_count; j++) {
            thread_t* thread = process->threads[j];
            if (thread && thread->running && thread != process_manager.current_thread) {
                uint32_t priority = process->priority + thread->priority;
                if (priority > highest_priority) {
                    highest_priority = priority;
                    next_thread = thread;
                }
            }
        }
    }

    if (next_thread) {
        process_manager.current_thread = next_thread;
    }

    // Restaurer l'état du nouveau thread
    cpu_state_t* new_state = &process_manager.current_thread->state;
    asm volatile("movl %0, %%esp" : : "m"(new_state->esp));
    asm volatile("pushl %0" : : "m"(new_state->eflags));
    asm volatile("popfl");
    asm volatile("movl %0, %%eax" : : "m"(new_state->eax));
    asm volatile("movl %0, %%ebx" : : "m"(new_state->ebx));
    asm volatile("movl %0, %%ecx" : : "m"(new_state->ecx));
    asm volatile("movl %0, %%edx" : : "m"(new_state->edx));
    asm volatile("movl %0, %%esi" : : "m"(new_state->esi));
    asm volatile("movl %0, %%edi" : : "m"(new_state->edi));
    asm volatile("movl %0, %%ebp" : : "m"(new_state->ebp));
    asm volatile("jmp *%0" : : "m"(new_state->eip));
}

void set_process_priority(uint32_t process_id, uint32_t priority) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].id == process_id) {
            process_manager.processes[i].priority = priority;
            break;
        }
    }
}

void set_thread_priority(uint32_t thread_id, uint32_t priority) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        process_t* process = &process_manager.processes[i];
        for (uint32_t j = 0; j < process->thread_count; j++) {
            if (process->threads[j] && process->threads[j]->id == thread_id) {
                process->threads[j]->priority = priority;
                return;
            }
        }
    }
}

void sleep_process(uint32_t process_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].id == process_id) {
            process_manager.processes[i].running = false;
            break;
        }
    }
}

void wake_process(uint32_t process_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].id == process_id) {
            process_manager.processes[i].running = true;
            break;
        }
    }
}

void sleep_thread(uint32_t thread_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        process_t* process = &process_manager.processes[i];
        for (uint32_t j = 0; j < process->thread_count; j++) {
            if (process->threads[j] && process->threads[j]->id == thread_id) {
                process->threads[j]->running = false;
                return;
            }
        }
    }
}

void wake_thread(uint32_t thread_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        process_t* process = &process_manager.processes[i];
        for (uint32_t j = 0; j < process->thread_count; j++) {
            if (process->threads[j] && process->threads[j]->id == thread_id) {
                process->threads[j]->running = true;
                return;
            }
        }
    }
}

void yield() {
    schedule();
} 