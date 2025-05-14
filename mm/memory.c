#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 1024

// Structure pour représenter une page de mémoire
typedef struct {
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t unused : 7;
    uint32_t frame : 20;
} page_t;

// Structure pour représenter une table de pages
typedef struct {
    page_t pages[1024];
} page_table_t;

// Structure pour représenter un répertoire de pages
typedef struct {
    page_table_t* tables[1024];
    uint32_t tables_physical[1024];
    uint32_t physical_addr;
} page_directory_t;

// Répertoire de pages actuel
page_directory_t* current_directory = 0;

// Bitmap pour suivre les pages libres
uint32_t* frames;
uint32_t nframes;

// Fonction pour définir un bit dans le bitmap
void set_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    frames[idx] |= (0x1 << off);
}

// Fonction pour effacer un bit dans le bitmap
void clear_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    frames[idx] &= ~(0x1 << off);
}

// Fonction pour trouver la première page libre
uint32_t first_free_frame() {
    for (uint32_t i = 0; i < nframes / 32; i++) {
        if (frames[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t to_test = 0x1 << j;
                if (!(frames[i] & to_test)) {
                    return i * 32 + j;
                }
            }
        }
    }
    return (uint32_t)-1;
}

// Fonction pour allouer une page
void alloc_frame(page_t* page, int is_kernel, int is_writeable) {
    if (page->frame != 0) {
        return;
    } else {
        uint32_t idx = first_free_frame();
        if (idx == (uint32_t)-1) {
            // PANIC: pas de pages libres
            return;
        }
        set_frame(idx * PAGE_SIZE);
        page->present = 1;
        page->rw = (is_writeable) ? 1 : 0;
        page->user = (is_kernel) ? 0 : 1;
        page->frame = idx;
    }
}

// Fonction pour libérer une page
void free_frame(page_t* page) {
    if (!page->frame) {
        return;
    } else {
        clear_frame(page->frame * PAGE_SIZE);
        page->frame = 0;
    }
}

// Initialisation du gestionnaire de mémoire
void init_memory() {
    // Calcul du nombre de frames nécessaires
    nframes = 1024 * 1024 * 32 / PAGE_SIZE;
    frames = (uint32_t*)kmalloc(nframes / 32);
    memset(frames, 0, nframes / 32);
} 