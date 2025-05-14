#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2
#define PAGE_USER 0x4
#define PAGE_SIZE_4MB 0x80
#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024
#define KERNEL_BASE 0xC0000000
#define USER_BASE 0x40000000

typedef uint32_t page_directory_t[PAGE_DIRECTORY_ENTRIES];
typedef uint32_t page_table_t[PAGE_TABLE_ENTRIES];

typedef struct {
    page_directory_t* directory;
    page_table_t* tables[PAGE_DIRECTORY_ENTRIES];
    uint32_t free_pages[PAGE_DIRECTORY_ENTRIES];
    uint32_t free_page_count;
} address_space_t;

static address_space_t kernel_space;
static address_space_t* current_space = &kernel_space;

void init_memory() {
    kernel_space.directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    memset(kernel_space.directory, 0, sizeof(page_directory_t));
    memset(kernel_space.tables, 0, sizeof(kernel_space.tables));
    memset(kernel_space.free_pages, 0, sizeof(kernel_space.free_pages));
    kernel_space.free_page_count = 0;

    for (uint32_t i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        uint32_t virtual_addr = KERNEL_BASE + (i * PAGE_SIZE * PAGE_TABLE_ENTRIES);
        uint32_t physical_addr = virtual_addr - KERNEL_BASE;

        page_table_t* table = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
        memset(table, 0, sizeof(page_table_t));

        for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            (*table)[j] = (physical_addr + (j * PAGE_SIZE)) | PAGE_PRESENT | PAGE_WRITE;
        }

        kernel_space.tables[i] = table;
        kernel_space.directory[i] = ((uint32_t)table) | PAGE_PRESENT | PAGE_WRITE;
    }

    uint32_t video_addr = 0xB8000;
    uint32_t video_page = video_addr / PAGE_SIZE;
    uint32_t video_table = video_page / PAGE_TABLE_ENTRIES;
    uint32_t video_entry = video_page % PAGE_TABLE_ENTRIES;

    if (!kernel_space.tables[video_table]) {
        kernel_space.tables[video_table] = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
        memset(kernel_space.tables[video_table], 0, sizeof(page_table_t));
        kernel_space.directory[video_table] = ((uint32_t)kernel_space.tables[video_table]) | PAGE_PRESENT | PAGE_WRITE;
    }

    kernel_space.tables[video_table][video_entry] = video_addr | PAGE_PRESENT | PAGE_WRITE;

    asm volatile("movl %0, %%cr3" : : "r"(kernel_space.directory));
    uint32_t cr0;
    asm volatile("movl %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("movl %0, %%cr0" : : "r"(cr0));
}

void* kmalloc(size_t size) {
    return kmalloc_aligned(size, 4);
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    void* ptr = kmalloc(size + alignment - 1);
    if (!ptr) return NULL;
    uint32_t addr = (uint32_t)ptr;
    uint32_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    return (void*)aligned_addr;
}

void kfree(void* ptr) {
    if (!ptr) return;
    uint32_t addr = (uint32_t)ptr;
    if (addr < KERNEL_BASE) return;
    uint32_t page = addr / PAGE_SIZE;
    uint32_t table = page / PAGE_TABLE_ENTRIES;
    uint32_t entry = page % PAGE_TABLE_ENTRIES;
    if (!kernel_space.tables[table]) return;
    uint32_t physical_addr = kernel_space.tables[table][entry] & ~0xFFF;
    if (physical_addr) {
        kfree((void*)physical_addr);
    }
    kernel_space.tables[table][entry] = 0;
    if (kernel_space.free_page_count < PAGE_DIRECTORY_ENTRIES) {
        kernel_space.free_pages[kernel_space.free_page_count++] = page;
    }
}

void* vmalloc(size_t size) {
    if (!current_space) return NULL;
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint32_t page_count = size / PAGE_SIZE;
    uint32_t* physical_pages = (uint32_t*)kmalloc(page_count * sizeof(uint32_t));
    if (!physical_pages) return NULL;

    for (uint32_t i = 0; i < page_count; i++) {
        physical_pages[i] = (uint32_t)kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);
        if (!physical_pages[i]) {
            for (uint32_t j = 0; j < i; j++) {
                kfree((void*)physical_pages[j]);
            }
            kfree(physical_pages);
            return NULL;
        }
    }

    uint32_t virtual_addr = USER_BASE;
    bool found = false;

    while (virtual_addr < KERNEL_BASE) {
        bool range_free = true;
        for (uint32_t i = 0; i < page_count; i++) {
            uint32_t directory_index = (virtual_addr + i * PAGE_SIZE) >> 22;
            uint32_t table_index = ((virtual_addr + i * PAGE_SIZE) >> 12) & 0x3FF;
            if (current_space->tables[directory_index] &&
                current_space->tables[directory_index][table_index]) {
                range_free = false;
                break;
            }
        }
        if (range_free) {
            found = true;
            break;
        }
        virtual_addr += PAGE_SIZE;
    }

    if (!found) {
        for (uint32_t i = 0; i < page_count; i++) {
            kfree((void*)physical_pages[i]);
        }
        kfree(physical_pages);
        return NULL;
    }

    for (uint32_t i = 0; i < page_count; i++) {
        if (!map_page(current_space, virtual_addr + i * PAGE_SIZE,
                     physical_pages[i], PAGE_PRESENT | PAGE_WRITE | PAGE_USER)) {
            for (uint32_t j = 0; j < i; j++) {
                unmap_page(current_space, virtual_addr + j * PAGE_SIZE);
                kfree((void*)physical_pages[j]);
            }
            kfree(physical_pages);
            return NULL;
        }
    }

    kfree(physical_pages);
    return (void*)virtual_addr;
}

void vfree(void* ptr) {
    if (!ptr || !current_space) return;
    uint32_t virtual_addr = (uint32_t)ptr;
    if (virtual_addr < USER_BASE || virtual_addr >= KERNEL_BASE) return;
    uint32_t directory_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;
    if (!current_space->tables[directory_index]) return;
    uint32_t physical_addr = current_space->tables[directory_index][table_index] & ~0xFFF;
    kfree((void*)physical_addr);
    unmap_page(current_space, virtual_addr);
} 