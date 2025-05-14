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
    page_directory_t* directory;
    page_table_t* tables[PAGE_DIRECTORY_ENTRIES];
    uint32_t* free_pages;
    uint32_t free_pages_count;
} address_space_t;

address_space_t kernel_space;
address_space_t* current_space;

void init_virtual_memory() {
    // Initialiser l'espace d'adressage du noyau
    kernel_space.directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    memset(kernel_space.directory, 0, sizeof(page_directory_t));
    memset(kernel_space.tables, 0, sizeof(kernel_space.tables));

    // Allouer les pages pour les tables de pages
    for (uint32_t i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        if (i >= (KERNEL_BASE >> 22)) {
            kernel_space.tables[i] = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
            memset(kernel_space.tables[i], 0, sizeof(page_table_t));
            kernel_space.directory->entries[i] = (uint32_t)kernel_space.tables[i] | PAGE_PRESENT | PAGE_WRITE;
        }
    }

    // Mapper la mémoire du noyau
    map_kernel_memory();

    // Mapper la mémoire vidéo
    map_video_memory();

    // Activer la pagination
    enable_paging();

    // Définir l'espace d'adressage actuel
    current_space = &kernel_space;
}

void map_kernel_memory() {
    // Mapper le noyau à 0xC0000000
    for (uint32_t i = 0; i < 1024; i++) {
        uint32_t physical_addr = i * PAGE_SIZE;
        uint32_t virtual_addr = KERNEL_BASE + (i * PAGE_SIZE);
        map_page(virtual_addr, physical_addr, PAGE_PRESENT | PAGE_WRITE);
    }
}

void map_video_memory() {
    // Mapper la mémoire vidéo à 0xB8000
    map_page(0xB8000, 0xB8000, PAGE_PRESENT | PAGE_WRITE);
}

void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t directory_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;

    // Vérifier si la table de pages existe
    if (!current_space->tables[directory_index]) {
        current_space->tables[directory_index] = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
        if (!current_space->tables[directory_index]) return;
        memset(current_space->tables[directory_index], 0, sizeof(page_table_t));
        current_space->directory->entries[directory_index] = (uint32_t)current_space->tables[directory_index] | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    // Mapper la page
    current_space->tables[directory_index]->entries[table_index] = physical_addr | flags;

    // Invalider la page dans le TLB
    invalidate_page(virtual_addr);
}

void unmap_page(uint32_t virtual_addr) {
    uint32_t directory_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;

    // Vérifier si la table de pages existe
    if (!current_space->tables[directory_index]) return;

    // Démaper la page
    current_space->tables[directory_index]->entries[table_index] = 0;

    // Invalider la page dans le TLB
    invalidate_page(virtual_addr);

    // Vérifier si la table de pages est vide
    bool empty = true;
    for (uint32_t i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        if (current_space->tables[directory_index]->entries[i]) {
            empty = false;
            break;
        }
    }

    // Libérer la table de pages si elle est vide
    if (empty) {
        kfree(current_space->tables[directory_index]);
        current_space->tables[directory_index] = NULL;
        current_space->directory->entries[directory_index] = 0;
    }
}

void invalidate_page(uint32_t virtual_addr) {
    asm volatile("invlpg (%0)":: "r"(virtual_addr));
}

void enable_paging() {
    // Charger le répertoire de pages
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

    // Allouer le répertoire de pages
    space->directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    if (!space->directory) {
        kfree(space);
        return NULL;
    }

    // Copier les entrées du noyau
    for (uint32_t i = (KERNEL_BASE >> 22); i < PAGE_DIRECTORY_ENTRIES; i++) {
        space->directory->entries[i] = kernel_space.directory->entries[i];
    }

    // Initialiser les tables de pages
    memset(space->tables, 0, sizeof(space->tables));
    for (uint32_t i = 0; i < (KERNEL_BASE >> 22); i++) {
        space->tables[i] = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
        if (!space->tables[i]) {
            // Nettoyer en cas d'échec
            for (uint32_t j = 0; j < i; j++) {
                kfree(space->tables[j]);
            }
            kfree(space->directory);
            kfree(space);
            return NULL;
        }
        memset(space->tables[i], 0, sizeof(page_table_t));
        space->directory->entries[i] = (uint32_t)space->tables[i] | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    // Initialiser la liste des pages libres
    space->free_pages = (uint32_t*)kmalloc(PAGE_DIRECTORY_ENTRIES * sizeof(uint32_t));
    if (!space->free_pages) {
        // Nettoyer en cas d'échec
        for (uint32_t i = 0; i < (KERNEL_BASE >> 22); i++) {
            kfree(space->tables[i]);
        }
        kfree(space->directory);
        kfree(space);
        return NULL;
    }
    space->free_pages_count = 0;

    return space;
}

void destroy_address_space(address_space_t* space) {
    if (!space || space == &kernel_space) return;

    // Libérer les tables de pages
    for (uint32_t i = 0; i < (KERNEL_BASE >> 22); i++) {
        if (space->tables[i]) {
            kfree(space->tables[i]);
        }
    }

    // Libérer le répertoire de pages
    kfree(space->directory);

    // Libérer la liste des pages libres
    kfree(space->free_pages);

    // Libérer l'espace d'adressage
    kfree(space);
}

void switch_address_space(address_space_t* space) {
    if (!space || space == current_space) return;

    // Charger le nouveau répertoire de pages
    asm volatile("movl %0, %%cr3":: "r"(space->directory));

    // Mettre à jour l'espace d'adressage actuel
    current_space = space;
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    void* ptr = kmalloc(size + alignment - 1);
    if (!ptr) return NULL;

    uint32_t addr = (uint32_t)ptr;
    uint32_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    uint32_t offset = aligned_addr - addr;

    if (offset > 0) {
        void* aligned_ptr = (void*)(aligned_addr);
        kfree(ptr);
        return aligned_ptr;
    }

    return ptr;
}

void* vmalloc(size_t size) {
    uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t* physical_pages = (uint32_t*)kmalloc(pages * sizeof(uint32_t));
    if (!physical_pages) return NULL;

    for (uint32_t i = 0; i < pages; i++) {
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
    for (uint32_t i = 0; i < pages; i++) {
        map_page(virtual_addr + (i * PAGE_SIZE),
                physical_pages[i],
                PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    kfree(physical_pages);
    return (void*)virtual_addr;
}

void vfree(void* ptr) {
    if (!ptr) return;

    uint32_t virtual_addr = (uint32_t)ptr;
    if (virtual_addr < USER_BASE) return;

    uint32_t directory_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;

    if (!current_space->tables[directory_index]) return;

    page_table_entry_t* entry = &current_space->tables[directory_index]->entries[table_index];
    while (*entry) {
        uint32_t physical_addr = *entry & ~0xFFF;
        kfree((void*)physical_addr);
        *entry = 0;
        entry++;
    }

    unmap_page(virtual_addr);
} 