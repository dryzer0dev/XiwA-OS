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

typedef uint32_t page_directory_entry_t;
typedef uint32_t page_table_entry_t;

typedef struct {
    page_directory_entry_t entries[PAGE_DIRECTORY_ENTRIES];
} page_directory_t;

typedef struct {
    page_table_entry_t entries[PAGE_TABLE_ENTRIES];
} page_table_t;

typedef struct {
    uint32_t virtual_address;
    uint32_t physical_address;
    uint32_t flags;
    bool is_present;
} page_mapping_t;

typedef struct {
    page_directory_t* directory;
    page_mapping_t* mappings;
    uint32_t mapping_count;
    uint32_t max_mappings;
} address_space_t;

address_space_t kernel_space;
address_space_t* current_space;

void init_virtual_memory() {
    kernel_space.directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    kernel_space.mappings = (page_mapping_t*)kmalloc(sizeof(page_mapping_t) * 1024);
    kernel_space.mapping_count = 0;
    kernel_space.max_mappings = 1024;

    map_kernel_memory();

    map_video_memory();

    enable_paging();

    current_space = &kernel_space;
}

void map_kernel_memory() {
    uint32_t kernel_start = (uint32_t)&_kernel_start;
    uint32_t kernel_end = (uint32_t)&_kernel_end;
    uint32_t kernel_size = kernel_end - kernel_start;

    for (uint32_t i = 0; i < kernel_size; i += PAGE_SIZE) {
        map_page(kernel_space.directory,
                KERNEL_BASE + i,
                kernel_start + i,
                PAGE_PRESENT | PAGE_WRITE);
    }
}

void map_video_memory() {
    // Mapper la mémoire vidéo à 0xB8000
    map_page(kernel_space.directory,
            0xB8000,
            0xB8000,
            PAGE_PRESENT | PAGE_WRITE);
}

void map_page(page_directory_t* directory, uint32_t virtual_address, uint32_t physical_address, uint32_t flags) {
    uint32_t directory_index = virtual_address >> 22;
    uint32_t table_index = (virtual_address >> 12) & 0x3FF;

    if (!(directory->entries[directory_index] & PAGE_PRESENT)) {
        // Créer une nouvelle table de pages
        page_table_t* table = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
        if (!table) return;

        memset(table, 0, sizeof(page_table_t));

        directory->entries[directory_index] = (uint32_t)table | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    page_table_t* table = (page_table_t*)(directory->entries[directory_index] & ~0xFFF);

    if (table->entries[table_index] & PAGE_PRESENT) {
        return;
    }

    table->entries[table_index] = physical_address | flags;

    invalidate_page(virtual_address);
}

void unmap_page(page_directory_t* directory, uint32_t virtual_address) {
    uint32_t directory_index = virtual_address >> 22;
    uint32_t table_index = (virtual_address >> 12) & 0x3FF;

    if (!(directory->entries[directory_index] & PAGE_PRESENT)) {
        return;
    }

    page_table_t* table = (page_table_t*)(directory->entries[directory_index] & ~0xFFF);

    if (!(table->entries[table_index] & PAGE_PRESENT)) {
        return;
    }

    // Démaper la page
    table->entries[table_index] = 0;

    invalidate_page(virtual_address);

    bool is_empty = true;
    for (uint32_t i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        if (table->entries[i] & PAGE_PRESENT) {
            is_empty = false;
            break;
        }
    }

    if (is_empty) {
        kfree(table);
        directory->entries[directory_index] = 0;
    }
}

void invalidate_page(uint32_t virtual_address) {
    asm volatile("invlpg (%0)":: "r"(virtual_address));
}

void enable_paging() {
    asm volatile("movl %0, %%cr3":: "r"(kernel_space.directory));

    // Activer la pagination
    uint32_t cr0;
    asm volatile("movl %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("movl %0, %%cr0":: "r"(cr0));
}

address_space_t* create_address_space() {
    address_space_t* space = (address_space_t*)kmalloc(sizeof(address_space_t));
    if (!space) return NULL;

    space->directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    if (!space->directory) {
        kfree(space);
        return NULL;
    }

    space->mappings = (page_mapping_t*)kmalloc(sizeof(page_mapping_t) * 1024);
    if (!space->mappings) {
        kfree(space->directory);
        kfree(space);
        return NULL;
    }

    space->mapping_count = 0;
    space->max_mappings = 1024;

    // Copier les entrées du noyau
    for (uint32_t i = KERNEL_BASE / PAGE_SIZE; i < PAGE_DIRECTORY_ENTRIES; i++) {
        space->directory->entries[i] = kernel_space.directory->entries[i];
    }

    return space;
}

void destroy_address_space(address_space_t* space) {
    if (!space) return;

    for (uint32_t i = 0; i < space->mapping_count; i++) {
        page_mapping_t* mapping = &space->mappings[i];
        if (mapping->is_present) {
            unmap_page(space->directory, mapping->virtual_address);
        }
    }

    kfree(space->mappings);
    kfree(space->directory);
    kfree(space);
}

void switch_address_space(address_space_t* space) {
    if (!space) return;

    current_space = space;
    asm volatile("movl %0, %%cr3":: "r"(space->directory));
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    void* ptr = kmalloc(size + alignment - 1);
    if (!ptr) return NULL;

    uint32_t addr = (uint32_t)ptr;
    uint32_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);

    return (void*)aligned_addr;
}

void* vmalloc(size_t size) {
    uint32_t virtual_address = USER_BASE;
    bool found = false;

    while (virtual_address < KERNEL_BASE) {
        bool is_free = true;
        for (uint32_t i = 0; i < current_space->mapping_count; i++) {
            page_mapping_t* mapping = &current_space->mappings[i];
            if (mapping->is_present &&
                mapping->virtual_address <= virtual_address &&
                mapping->virtual_address + PAGE_SIZE > virtual_address) {
                is_free = false;
                break;
            }
        }

        if (is_free) {
            found = true;
            break;
        }

        virtual_address += PAGE_SIZE;
    }

    if (!found) return NULL;

    for (uint32_t i = 0; i < size; i += PAGE_SIZE) {
        void* physical_page = kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);
        if (!physical_page) {
            for (uint32_t j = 0; j < i; j += PAGE_SIZE) {
                unmap_page(current_space->directory, virtual_address + j);
            }
            return NULL;
        }

        if (!map_page(current_space->directory,
                     virtual_address + i,
                     (uint32_t)physical_page,
                     PAGE_PRESENT | PAGE_WRITE | PAGE_USER)) {
            kfree(physical_page);
            for (uint32_t j = 0; j < i; j += PAGE_SIZE) {
                unmap_page(current_space->directory, virtual_address + j);
            }
            return NULL;
        }

        if (current_space->mapping_count < current_space->max_mappings) {
            page_mapping_t* mapping = &current_space->mappings[current_space->mapping_count++];
            mapping->virtual_address = virtual_address + i;
            mapping->physical_address = (uint32_t)physical_page;
            mapping->flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
            mapping->is_present = true;
        }
    }

    return (void*)virtual_address;
}

void vfree(void* ptr) {
    if (!ptr) return;

    uint32_t virtual_address = (uint32_t)ptr;

    for (uint32_t i = 0; i < current_space->mapping_count; i++) {
        page_mapping_t* mapping = &current_space->mappings[i];
        if (mapping->is_present && mapping->virtual_address == virtual_address) {
            unmap_page(current_space->directory, virtual_address);
            kfree((void*)mapping->physical_address);
            mapping->is_present = false;
            break;
        }
    }
} 