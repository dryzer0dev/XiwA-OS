#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define KERNEL_BASE 0xC0000000
#define PAGE_SIZE 4096
#define MAX_CPUS 16
#define MAX_IRQS 256

typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
} cpu_state_t;

typedef struct {
    uint32_t id;
    bool active;
    cpu_state_t state;
    void* stack;
    uint32_t priority;
} cpu_t;

typedef struct {
    void (*handler)(void*);
    void* data;
    uint32_t count;
    uint32_t mask;
} irq_handler_t;

static cpu_t cpus[MAX_CPUS];
static irq_handler_t irq_handlers[MAX_IRQS];
static uint32_t cpu_count = 0;
static uint32_t current_cpu = 0;

void init_core() {
    memset(cpus, 0, sizeof(cpus));
    memset(irq_handlers, 0, sizeof(irq_handlers));
    cpu_count = 1;
    cpus[0].active = true;
}

void register_irq(uint32_t irq, void (*handler)(void*), void* data) {
    if (irq >= MAX_IRQS) return;
    irq_handlers[irq].handler = handler;
    irq_handlers[irq].data = data;
    irq_handlers[irq].mask = 1 << irq;
}

void handle_irq(uint32_t irq) {
    if (irq >= MAX_IRQS) return;
    irq_handler_t* handler = &irq_handlers[irq];
    if (handler->handler) {
        handler->handler(handler->data);
        handler->count++;
    }
}

void switch_context(cpu_state_t* old_state, cpu_state_t* new_state) {
    asm volatile("movl %0, %%esp" : : "r"(new_state->esp));
    asm volatile("movl %0, %%eax" : : "r"(new_state->eax));
    asm volatile("movl %0, %%ebx" : : "r"(new_state->ebx));
    asm volatile("movl %0, %%ecx" : : "r"(new_state->ecx));
    asm volatile("movl %0, %%edx" : : "r"(new_state->edx));
    asm volatile("movl %0, %%esi" : : "r"(new_state->esi));
    asm volatile("movl %0, %%edi" : : "r"(new_state->edi));
    asm volatile("movl %0, %%ebp" : : "r"(new_state->ebp));
    asm volatile("pushl %0" : : "r"(new_state->eflags));
    asm volatile("popfl");
    asm volatile("jmp *%0" : : "r"(new_state->eip));
}

void enable_interrupts() {
    asm volatile("sti");
}

void disable_interrupts() {
    asm volatile("cli");
}

void halt() {
    asm volatile("hlt");
}

void reboot() {
    uint8_t good = 0x02;
    while (good & 0x02) good = inb(0x64);
    outb(0x64, 0xFE);
    halt();
}

void panic(const char* msg) {
    disable_interrupts();
    while (1) halt();
} 