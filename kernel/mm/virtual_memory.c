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
    uint32_t size;
    uint32_t flags;
} memory_mapping_t;

typedef struct {
    memory_mapping_t mappings[256];
    uint32_t mapping_count;
    page_directory_t* page_directory;
} address_space_t;

address_space_t kernel_space;
address_space_t* current_space;

void init_virtual_memory() {
    // Initialiser l'espace d'adressage du noyau
    kernel_space.page_directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    memset(kernel_space.page_directory, 0, sizeof(page_directory_t));
    kernel_space.mapping_count = 0;

    // Mapper le noyau
    map_kernel_memory();

    // Mapper la mémoire vidéo
    map_video_memory();

    // Activer la pagination
    enable_paging();
}

void map_kernel_memory() {
    // Mapper le noyau à 0xC0000000
    uint32_t kernel_physical = 0x100000;
    uint32_t kernel_virtual = KERNEL_BASE;
    uint32_t kernel_size = 0x1000000; // 16MB

    for (uint32_t i = 0; i < kernel_size; i += PAGE_SIZE) {
        map_page(kernel_virtual + i, kernel_physical + i,
                PAGE_PRESENT | PAGE_WRITE);
    }

    // Ajouter le mapping à l'espace d'adressage du noyau
    memory_mapping_t* mapping = &kernel_space.mappings[kernel_space.mapping_count++];
    mapping->virtual_address = kernel_virtual;
    mapping->physical_address = kernel_physical;
    mapping->size = kernel_size;
    mapping->flags = PAGE_PRESENT | PAGE_WRITE;
}

void map_video_memory() {
    // Mapper la mémoire vidéo à 0xB8000
    uint32_t video_physical = 0xB8000;
    uint32_t video_virtual = 0xB8000;
    uint32_t video_size = 0x8000; // 32KB

    for (uint32_t i = 0; i < video_size; i += PAGE_SIZE) {
        map_page(video_virtual + i, video_physical + i,
                PAGE_PRESENT | PAGE_WRITE);
    }

    // Ajouter le mapping à l'espace d'adressage du noyau
    memory_mapping_t* mapping = &kernel_space.mappings[kernel_space.mapping_count++];
    mapping->virtual_address = video_virtual;
    mapping->physical_address = video_physical;
    mapping->size = video_size;
    mapping->flags = PAGE_PRESENT | PAGE_WRITE;
}

void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    page_directory_t* pd = current_space->page_directory;
    page_table_t* pt;

    // Vérifier si la table de pages existe
    if (!(pd->entries[pd_index] & PAGE_PRESENT)) {
        // Créer une nouvelle table de pages
        pt = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
        memset(pt, 0, sizeof(page_table_t));

        // Ajouter la table de pages au répertoire de pages
        pd->entries[pd_index] = ((uint32_t)pt) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    } else {
        pt = (page_table_t*)(pd->entries[pd_index] & ~0xFFF);
    }

    // Ajouter la page à la table de pages
    pt->entries[pt_index] = physical_addr | flags;

    // Invalider le TLB
    invalidate_page(virtual_addr);
}

void unmap_page(uint32_t virtual_addr) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    page_directory_t* pd = current_space->page_directory;
    if (!(pd->entries[pd_index] & PAGE_PRESENT)) {
        return;
    }

    page_table_t* pt = (page_table_t*)(pd->entries[pd_index] & ~0xFFF);
    pt->entries[pt_index] = 0;

    // Invalider le TLB
    invalidate_page(virtual_addr);
}

void invalidate_page(uint32_t virtual_addr) {
    asm volatile("invlpg (%0)" : : "r" (virtual_addr) : "memory");
}

void enable_paging() {
    // Charger le répertoire de pages
    asm volatile("movl %0, %%cr3" : : "r" (current_space->page_directory));

    // Activer la pagination
    uint32_t cr0;
    asm volatile("movl %%cr0, %0" : "=r" (cr0));
    cr0 |= 0x80000000;
    asm volatile("movl %0, %%cr0" : : "r" (cr0));
}

address_space_t* create_address_space() {
    address_space_t* space = (address_space_t*)kmalloc(sizeof(address_space_t));
    space->page_directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    memset(space->page_directory, 0, sizeof(page_directory_t));
    space->mapping_count = 0;

    // Copier les mappings du noyau
    for (uint32_t i = 0; i < kernel_space.mapping_count; i++) {
        space->mappings[i] = kernel_space.mappings[i];
    }
    space->mapping_count = kernel_space.mapping_count;

    return space;
}

void switch_address_space(address_space_t* space) {
    current_space = space;
    asm volatile("movl %0, %%cr3" : : "r" (space->page_directory));
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    void* ptr = kmalloc(size + alignment);
    if (!ptr) return NULL;

    uint32_t addr = (uint32_t)ptr;
    uint32_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    
    if (aligned_addr != addr) {
        // Libérer la mémoire non alignée
        kfree(ptr);
        ptr = kmalloc(size + alignment);
        if (!ptr) return NULL;
        addr = (uint32_t)ptr;
        aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    }

    return (void*)aligned_addr;
}

void* vmalloc(size_t size) {
    // Trouver une région libre dans l'espace d'adressage
    uint32_t virtual_addr = USER_BASE;
    bool found = false;

    while (virtual_addr < KERNEL_BASE) {
        bool region_free = true;
        for (uint32_t i = 0; i < current_space->mapping_count; i++) {
            memory_mapping_t* mapping = &current_space->mappings[i];
            if (virtual_addr >= mapping->virtual_address &&
                virtual_addr < mapping->virtual_address + mapping->size) {
                region_free = false;
                virtual_addr = mapping->virtual_address + mapping->size;
                break;
            }
        }

        if (region_free) {
            found = true;
            break;
        }
    }

    if (!found) return NULL;

    // Allouer la mémoire physique
    void* physical_addr = kmalloc_aligned(size, PAGE_SIZE);
    if (!physical_addr) return NULL;

    // Mapper la mémoire
    for (uint32_t i = 0; i < size; i += PAGE_SIZE) {
        map_page(virtual_addr + i, (uint32_t)physical_addr + i,
                PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    // Ajouter le mapping à l'espace d'adressage
    memory_mapping_t* mapping = &current_space->mappings[current_space->mapping_count++];
    mapping->virtual_address = virtual_addr;
    mapping->physical_address = (uint32_t)physical_addr;
    mapping->size = size;
    mapping->flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    return (void*)virtual_addr;
}

void vfree(void* addr) {
    uint32_t virtual_addr = (uint32_t)addr;
    memory_mapping_t* mapping = NULL;
    uint32_t mapping_index = 0;

    // Trouver le mapping
    for (uint32_t i = 0; i < current_space->mapping_count; i++) {
        if (virtual_addr == current_space->mappings[i].virtual_address) {
            mapping = &current_space->mappings[i];
            mapping_index = i;
            break;
        }
    }

    if (!mapping) return;

    // Démaper la mémoire
    for (uint32_t i = 0; i < mapping->size; i += PAGE_SIZE) {
        unmap_page(virtual_addr + i);
    }

    // Libérer la mémoire physique
    kfree((void*)mapping->physical_address);

    // Supprimer le mapping
    memmove(&current_space->mappings[mapping_index],
            &current_space->mappings[mapping_index + 1],
            (current_space->mapping_count - mapping_index - 1) * sizeof(memory_mapping_t));
    current_space->mapping_count--;
} 