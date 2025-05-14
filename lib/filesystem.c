#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FILENAME_LENGTH 255
#define MAX_PATH_LENGTH 4096
#define MAX_FILES_PER_DIR 1024

typedef struct {
    char name[MAX_FILENAME_LENGTH];
    uint32_t size;
    uint32_t attributes;
    uint64_t creation_time;
    uint64_t last_modified;
    uint32_t permissions;
    bool is_directory;
    uint32_t parent_inode;
    uint32_t inode;
} FileEntry;

typedef struct {
    FileEntry entries[MAX_FILES_PER_DIR];
    uint32_t entry_count;
    uint32_t inode;
} Directory;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint64_t total_blocks;
    uint64_t free_blocks;
    uint32_t root_inode;
    char fs_type[8];
} FileSystemHeader;

FileSystemHeader fs_header;
Directory current_directory;

void init_filesystem() {
    strcpy(fs_header.fs_type, "XFS");
    fs_header.magic = 0x58465300;
    fs_header.version = 1;
    fs_header.block_size = 4096;
    fs_header.total_blocks = 1024 * 1024;
    fs_header.free_blocks = fs_header.total_blocks;
    fs_header.root_inode = 1;
    
    current_directory.inode = 1;
    current_directory.entry_count = 0;
}

bool create_file(const char* name, bool is_directory) {
    if (current_directory.entry_count >= MAX_FILES_PER_DIR) {
        return false;
    }
    
    FileEntry* entry = &current_directory.entries[current_directory.entry_count];
    strncpy(entry->name, name, MAX_FILENAME_LENGTH - 1);
    entry->name[MAX_FILENAME_LENGTH - 1] = '\0';
    entry->size = 0;
    entry->attributes = 0;
    entry->creation_time = get_current_time();
    entry->last_modified = entry->creation_time;
    entry->permissions = 0755;
    entry->is_directory = is_directory;
    entry->parent_inode = current_directory.inode;
    entry->inode = generate_new_inode();
    
    current_directory.entry_count++;
    return true;
}

bool delete_file(const char* name) {
    for (uint32_t i = 0; i < current_directory.entry_count; i++) {
        if (strcmp(current_directory.entries[i].name, name) == 0) {
            for (uint32_t j = i; j < current_directory.entry_count - 1; j++) {
                current_directory.entries[j] = current_directory.entries[j + 1];
            }
            current_directory.entry_count--;
            return true;
        }
    }
    return false;
}

FileEntry* find_file(const char* name) {
    for (uint32_t i = 0; i < current_directory.entry_count; i++) {
        if (strcmp(current_directory.entries[i].name, name) == 0) {
            return &current_directory.entries[i];
        }
    }
    return NULL;
}

bool change_directory(const char* name) {
    FileEntry* entry = find_file(name);
    if (entry && entry->is_directory) {
        current_directory.inode = entry->inode;
        return true;
    }
    return false;
}

// Fonction pour lire un fichier
uint32_t read_file(FileEntry* file, void* buffer, uint32_t size) {
    if (!file || !buffer) {
        return 0;
    }

    uint32_t bytes_read = 0;
    uint32_t current_block = file->inode;
    
    while (bytes_read < size && bytes_read < file->size) {
        uint32_t block_offset = bytes_read % fs_header.block_size;
        uint32_t bytes_to_read = min(fs_header.block_size - block_offset, 
                                   min(size - bytes_read, file->size - bytes_read));
        
        memcpy((char*)buffer + bytes_read,
               (char*)get_block_address(current_block) + block_offset,
               bytes_to_read);
        
        bytes_read += bytes_to_read;
        current_block = get_next_block(current_block);
    }

    return bytes_read;
}

// Fonction pour écrire dans un fichier
uint32_t write_file(FileEntry* file, const void* buffer, uint32_t size) {
    if (!file || !buffer) {
        return 0;
    }

    uint32_t bytes_written = 0;
    uint32_t current_block = file->inode;
    
    while (bytes_written < size) {
        uint32_t block_offset = bytes_written % fs_header.block_size;
        uint32_t bytes_to_write = min(fs_header.block_size - block_offset, size - bytes_written);
        
        memcpy((char*)get_block_address(current_block) + block_offset,
               (char*)buffer + bytes_written,
               bytes_to_write);
        
        bytes_written += bytes_to_write;
        
        if (bytes_written > file->size) {
            file->size = bytes_written;
        }
        
        if (bytes_written < size) {
            uint32_t next_block = get_next_block(current_block);
            if (next_block == 0) {
                next_block = alloc_block();
                set_next_block(current_block, next_block);
            }
            current_block = next_block;
        }
    }

    return bytes_written;
} 
} 