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

typedef uint32_t page_directory_t[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
typedef uint32_t page_table_t[PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

typedef struct {
    page_directory_t* directory;
    page_table_t* tables[PAGE_DIRECTORY_ENTRIES];
    uint32_t* free_pages;
    uint32_t free_page_count;
} address_space_t;

address_space_t kernel_space;
address_space_t* current_space;

void init_virtual_memory() {
    // Initialiser l'espace d'adressage du noyau
    kernel_space.directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    memset(kernel_space.directory, 0, sizeof(page_directory_t));

    // Allouer les tables de pages pour le noyau
    for (uint32_t i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        kernel_space.tables[i] = NULL;
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
    // Mapper les 4 premiers Mo pour le noyau
    for (uint32_t i = 0; i < 1024; i++) {
        uint32_t physical_addr = i * PAGE_SIZE;
        uint32_t virtual_addr = KERNEL_BASE + physical_addr;
        map_page(&kernel_space, virtual_addr, physical_addr,
                PAGE_PRESENT | PAGE_WRITE);
    }
}

void map_video_memory() {
    // Mapper la mémoire vidéo à 0xB8000
    map_page(&kernel_space, 0xB8000, 0xB8000,
            PAGE_PRESENT | PAGE_WRITE);
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
    if (!space) {
        return NULL;
    }

    // Allouer le répertoire de pages
    space->directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    if (!space->directory) {
        kfree(space);
        return NULL;
    }

    // Copier les entrées du noyau
    for (uint32_t i = KERNEL_BASE / PAGE_SIZE / PAGE_TABLE_ENTRIES;
         i < PAGE_DIRECTORY_ENTRIES; i++) {
        space->directory[i] = kernel_space.directory[i];
    }

    // Initialiser les tables de pages
    for (uint32_t i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        space->tables[i] = NULL;
    }

    // Allouer la table des pages libres
    space->free_pages = (uint32_t*)kmalloc(PAGE_DIRECTORY_ENTRIES * sizeof(uint32_t));
    if (!space->free_pages) {
        kfree(space->directory);
        kfree(space);
        return NULL;
    }

    space->free_page_count = 0;
    return space;
}

void destroy_address_space(address_space_t* space) {
    if (!space || space == &kernel_space) {
        return;
    }

    // Libérer les tables de pages
    for (uint32_t i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        if (space->tables[i] && i < KERNEL_BASE / PAGE_SIZE / PAGE_TABLE_ENTRIES) {
            kfree(space->tables[i]);
        }
    }

    // Libérer le répertoire de pages
    kfree(space->directory);
    kfree(space->free_pages);
    kfree(space);
}

void switch_address_space(address_space_t* space) {
    if (!space) {
        return;
    }

    current_space = space;
    asm volatile("movl %0, %%cr3":: "r"(space->directory));
}

bool map_page(address_space_t* space, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    if (!space) {
        return false;
    }

    uint32_t directory_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;

    // Vérifier si la table de pages existe
    if (!space->tables[directory_index]) {
        // Allouer une nouvelle table de pages
        space->tables[directory_index] = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
        if (!space->tables[directory_index]) {
            return false;
        }

        memset(space->tables[directory_index], 0, sizeof(page_table_t));
        space->directory[directory_index] = (uint32_t)space->tables[directory_index] | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    // Mapper la page
    space->tables[directory_index][table_index] = physical_addr | flags;
    invalidate_page(virtual_addr);

    return true;
}

bool unmap_page(address_space_t* space, uint32_t virtual_addr) {
    if (!space) {
        return false;
    }

    uint32_t directory_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;

    // Vérifier si la table de pages existe
    if (!space->tables[directory_index]) {
        return false;
    }

    // Démapper la page
    space->tables[directory_index][table_index] = 0;
    invalidate_page(virtual_addr);

    // Vérifier si la table est vide
    bool table_empty = true;
    for (uint32_t i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        if (space->tables[directory_index][i]) {
            table_empty = false;
            break;
        }
    }

    // Libérer la table si elle est vide
    if (table_empty && directory_index < KERNEL_BASE / PAGE_SIZE / PAGE_TABLE_ENTRIES) {
        kfree(space->tables[directory_index]);
        space->tables[directory_index] = NULL;
        space->directory[directory_index] = 0;
    }

    return true;
}

void invalidate_page(uint32_t virtual_addr) {
    asm volatile("invlpg (%0)":: "r"(virtual_addr));
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    void* ptr = kmalloc(size + alignment - 1);
    if (!ptr) {
        return NULL;
    }

    uint32_t addr = (uint32_t)ptr;
    uint32_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    return (void*)aligned_addr;
}

void* vmalloc(size_t size) {
    if (!current_space) {
        return NULL;
    }

    // Aligner la taille sur la taille de page
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint32_t page_count = size / PAGE_SIZE;

    // Allouer des pages physiques
    uint32_t* physical_pages = (uint32_t*)kmalloc(page_count * sizeof(uint32_t));
    if (!physical_pages) {
        return NULL;
    }

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

    // Trouver une plage d'adresses virtuelles libre
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

    // Mapper les pages
    for (uint32_t i = 0; i < page_count; i++) {
        if (!map_page(current_space, virtual_addr + i * PAGE_SIZE,
                     physical_pages[i], PAGE_PRESENT | PAGE_WRITE | PAGE_USER)) {
            // Démapper les pages déjà mappées
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
    if (!ptr || !current_space) {
        return;
    }

    uint32_t virtual_addr = (uint32_t)ptr;
    if (virtual_addr < USER_BASE || virtual_addr >= KERNEL_BASE) {
        return;
    }

    // Trouver la table de pages
    uint32_t directory_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;

    if (!current_space->tables[directory_index]) {
        return;
    }

    // Libérer la page physique
    uint32_t physical_addr = current_space->tables[directory_index][table_index] & ~0xFFF;
    kfree((void*)physical_addr);

    // Démapper la page
    unmap_page(current_space, virtual_addr);
} 