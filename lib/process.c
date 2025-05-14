#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PROCESSES 256
#define MAX_THREADS_PER_PROCESS 32
#define KERNEL_STACK_SIZE 4096
#define USER_STACK_SIZE 16384

typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
} cpu_state_t;

typedef struct {
    uint32_t id;
    cpu_state_t state;
    uint32_t stack;
    uint32_t priority;
    bool active;
} thread_t;

typedef struct {
    uint32_t id;
    char name[32];
    uint32_t page_directory;
    thread_t threads[MAX_THREADS_PER_PROCESS];
    uint32_t thread_count;
    uint32_t priority;
    bool active;
} process_t;

typedef struct {
    process_t processes[MAX_PROCESSES];
    uint32_t process_count;
    uint32_t current_process;
    uint32_t current_thread;
    uint32_t next_process_id;
    uint32_t next_thread_id;
} process_manager_t;

static process_manager_t process_manager;

void init_process_manager() {
    memset(&process_manager, 0, sizeof(process_manager_t));
    process_manager.next_process_id = 1;
    process_manager.next_thread_id = 1;
}

uint32_t create_process(const char* name, uint32_t priority) {
    if (process_manager.process_count >= MAX_PROCESSES) return 0;

    uint32_t process_id = process_manager.next_process_id++;
    process_t* process = &process_manager.processes[process_manager.process_count++];

    process->id = process_id;
    strncpy(process->name, name, sizeof(process->name) - 1);
    process->page_directory = create_address_space();
    process->thread_count = 0;
    process->priority = priority;
    process->active = true;

    return process_id;
}

void terminate_process(uint32_t process_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].id == process_id) {
            process_t* process = &process_manager.processes[i];
            for (uint32_t j = 0; j < process->thread_count; j++) {
                if (process->threads[j].active) {
                    kfree((void*)process->threads[j].stack);
                }
            }
            destroy_address_space(process->page_directory);
            process->active = false;
            break;
        }
    }
}

uint32_t create_thread(uint32_t process_id, void* entry_point, uint32_t priority) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].id == process_id) {
            process_t* process = &process_manager.processes[i];
            if (process->thread_count >= MAX_THREADS_PER_PROCESS) return 0;

            thread_t* thread = &process->threads[process->thread_count++];
            thread->id = process_manager.next_thread_id++;
            thread->stack = (uint32_t)kmalloc_aligned(KERNEL_STACK_SIZE, PAGE_SIZE);
            thread->priority = priority;
            thread->active = true;

            thread->state.esp = thread->stack + KERNEL_STACK_SIZE;
            thread->state.eip = (uint32_t)entry_point;
            thread->state.eflags = 0x202;
            thread->state.cs = 0x08;
            thread->state.ds = thread->state.es = thread->state.fs = thread->state.gs = thread->state.ss = 0x10;

            return thread->id;
        }
    }
    return 0;
}

void terminate_thread(uint32_t process_id, uint32_t thread_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].id == process_id) {
            process_t* process = &process_manager.processes[i];
            for (uint32_t j = 0; j < process->thread_count; j++) {
                if (process->threads[j].id == thread_id) {
                    if (process->threads[j].active) {
                        kfree((void*)process->threads[j].stack);
                    }
                    process->threads[j].active = false;
                    break;
                }
            }
            break;
        }
    }
}

void schedule() {
    if (process_manager.process_count == 0) return;

    uint32_t next_process = process_manager.current_process;
    uint32_t next_thread = process_manager.current_thread;
    uint32_t highest_priority = 0;

    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        process_t* process = &process_manager.processes[i];
        if (!process->active) continue;

        for (uint32_t j = 0; j < process->thread_count; j++) {
            thread_t* thread = &process->threads[j];
            if (!thread->active) continue;

            uint32_t priority = process->priority + thread->priority;
            if (priority > highest_priority) {
                highest_priority = priority;
                next_process = i;
                next_thread = j;
            }
        }
    }

    if (next_process != process_manager.current_process ||
        next_thread != process_manager.current_thread) {
        process_t* current_process = &process_manager.processes[process_manager.current_process];
        thread_t* current_thread = &current_process->threads[process_manager.current_thread];

        asm volatile("movl %%esp, %0" : "=m"(current_thread->state.esp));
        asm volatile("movl %%ebp, %0" : "=m"(current_thread->state.ebp));
        asm volatile("pushfl; popl %0" : "=m"(current_thread->state.eflags));

        process_manager.current_process = next_process;
        process_manager.current_thread = next_thread;

        process_t* next_process_ptr = &process_manager.processes[next_process];
        thread_t* next_thread_ptr = &next_process_ptr->threads[next_thread];

        switch_address_space(next_process_ptr->page_directory);

        asm volatile("movl %0, %%esp" : : "m"(next_thread_ptr->state.esp));
        asm volatile("movl %0, %%ebp" : : "m"(next_thread_ptr->state.ebp));
        asm volatile("pushl %0; popfl" : : "m"(next_thread_ptr->state.eflags));
    }
}

void sleep_thread(uint32_t process_id, uint32_t thread_id, uint32_t milliseconds) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].id == process_id) {
            process_t* process = &process_manager.processes[i];
            for (uint32_t j = 0; j < process->thread_count; j++) {
                if (process->threads[j].id == thread_id) {
                    process->threads[j].active = false;
                    schedule();
                    break;
                }
            }
            break;
        }
    }
}

void wake_thread(uint32_t process_id, uint32_t thread_id) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.processes[i].id == process_id) {
            process_t* process = &process_manager.processes[i];
            for (uint32_t j = 0; j < process->thread_count; j++) {
                if (process->threads[j].id == thread_id) {
                    process->threads[j].active = true;
                    break;
                }
            }
            break;
        }
    }
} 