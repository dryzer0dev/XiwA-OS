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

address_space_t kernel_space;
address_space_t* current_space = &kernel_space;

void init_virtual_memory() {
    // Initialiser l'espace d'adressage du noyau
    kernel_space.directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    memset(kernel_space.directory, 0, sizeof(page_directory_t));
    memset(kernel_space.tables, 0, sizeof(kernel_space.tables));
    memset(kernel_space.free_pages, 0, sizeof(kernel_space.free_pages));
    kernel_space.free_page_count = 0;

    // Mapper la mémoire du noyau
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

    // Mapper la mémoire vidéo
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

    // Activer la pagination
    asm volatile("movl %0, %%cr3" : : "r"(kernel_space.directory));
    uint32_t cr0;
    asm volatile("movl %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("movl %0, %%cr0" : : "r"(cr0));
}

address_space_t* create_address_space() {
    address_space_t* space = (address_space_t*)kmalloc(sizeof(address_space_t));
    if (!space) {
        return NULL;
    }

    space->directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    if (!space->directory) {
        kfree(space);
        return NULL;
    }

    memset(space->directory, 0, sizeof(page_directory_t));
    memset(space->tables, 0, sizeof(space->tables));
    memset(space->free_pages, 0, sizeof(space->free_pages));
    space->free_page_count = 0;

    // Copier les entrées du noyau
    for (uint32_t i = KERNEL_BASE / (PAGE_SIZE * PAGE_TABLE_ENTRIES); i < PAGE_DIRECTORY_ENTRIES; i++) {
        space->directory[i] = kernel_space.directory[i];
        space->tables[i] = kernel_space.tables[i];
    }

    return space;
}

void destroy_address_space(address_space_t* space) {
    if (!space || space == &kernel_space) {
        return;
    }

    // Libérer les tables de pages
    for (uint32_t i = 0; i < KERNEL_BASE / (PAGE_SIZE * PAGE_TABLE_ENTRIES); i++) {
        if (space->tables[i]) {
            kfree(space->tables[i]);
        }
    }

    kfree(space->directory);
    kfree(space);
}

void switch_address_space(address_space_t* space) {
    if (!space) {
        return;
    }

    current_space = space;
    asm volatile("movl %0, %%cr3" : : "r"(space->directory));
}

bool map_page(address_space_t* space, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    if (!space) {
        return false;
    }

    uint32_t page = virtual_addr / PAGE_SIZE;
    uint32_t table = page / PAGE_TABLE_ENTRIES;
    uint32_t entry = page % PAGE_TABLE_ENTRIES;

    if (!space->tables[table]) {
        space->tables[table] = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
        if (!space->tables[table]) {
            return false;
        }
        memset(space->tables[table], 0, sizeof(page_table_t));
        space->directory[table] = ((uint32_t)space->tables[table]) | PAGE_PRESENT | PAGE_WRITE;
    }

    space->tables[table][entry] = physical_addr | flags;
    return true;
}

bool unmap_page(address_space_t* space, uint32_t virtual_addr) {
    if (!space) {
        return false;
    }

    uint32_t page = virtual_addr / PAGE_SIZE;
    uint32_t table = page / PAGE_TABLE_ENTRIES;
    uint32_t entry = page % PAGE_TABLE_ENTRIES;

    if (!space->tables[table]) {
        return false;
    }

    space->tables[table][entry] = 0;

    // Vérifier si la table est vide
    bool empty = true;
    for (uint32_t i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        if (space->tables[table][i]) {
            empty = false;
            break;
        }
    }

    if (empty) {
        kfree(space->tables[table]);
        space->tables[table] = NULL;
        space->directory[table] = 0;
    }

    return true;
}

void* allocate_virtual_page(address_space_t* space, uint32_t flags) {
    if (!space) {
        return NULL;
    }

    // Chercher une page libre
    for (uint32_t i = 0; i < space->free_page_count; i++) {
        uint32_t page = space->free_pages[i];
        uint32_t table = page / PAGE_TABLE_ENTRIES;
        uint32_t entry = page % PAGE_TABLE_ENTRIES;

        if (!space->tables[table] || !space->tables[table][entry]) {
            // Allouer une page physique
            void* physical_page = kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);
            if (!physical_page) {
                return NULL;
            }

            // Mapper la page
            if (!map_page(space, page * PAGE_SIZE, (uint32_t)physical_page, flags)) {
                kfree(physical_page);
                return NULL;
            }

            // Retirer la page de la liste des pages libres
            memmove(&space->free_pages[i],
                    &space->free_pages[i + 1],
                    (space->free_page_count - i - 1) * sizeof(uint32_t));
            space->free_page_count--;

            return (void*)(page * PAGE_SIZE);
        }
    }

    return NULL;
}

void free_virtual_page(address_space_t* space, void* virtual_addr) {
    if (!space || !virtual_addr) {
        return;
    }

    uint32_t page = (uint32_t)virtual_addr / PAGE_SIZE;
    uint32_t table = page / PAGE_TABLE_ENTRIES;
    uint32_t entry = page % PAGE_TABLE_ENTRIES;

    if (!space->tables[table]) {
        return;
    }

    uint32_t physical_addr = space->tables[table][entry] & ~0xFFF;
    if (physical_addr) {
        kfree((void*)physical_addr);
    }

    unmap_page(space, (uint32_t)virtual_addr);

    // Ajouter la page à la liste des pages libres
    if (space->free_page_count < PAGE_DIRECTORY_ENTRIES) {
        space->free_pages[space->free_page_count++] = page;
    }
}

void invalidate_page(uint32_t virtual_addr) {
    asm volatile("invlpg (%0)" : : "r"(virtual_addr));
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