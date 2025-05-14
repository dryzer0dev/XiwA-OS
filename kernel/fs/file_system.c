#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FILENAME_LENGTH 255
#define MAX_PATH_LENGTH 4096
#define MAX_OPEN_FILES 256
#define MAX_MOUNT_POINTS 16
#define BLOCK_SIZE 4096
#define INODE_SIZE 256
#define MAX_BLOCKS_PER_FILE 1024
#define MAX_DIRECT_BLOCKS 12
#define MAX_INDIRECT_BLOCKS 256

typedef struct {
    uint32_t inode_number;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t size;
    uint32_t blocks;
    uint32_t direct_blocks[MAX_DIRECT_BLOCKS];
    uint32_t indirect_block;
    uint32_t double_indirect_block;
    uint32_t triple_indirect_block;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t access_time;
} inode_t;

typedef struct {
    char name[MAX_FILENAME_LENGTH];
    uint32_t inode_number;
} directory_entry_t;

typedef struct {
    uint32_t magic;
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t first_data_block;
    uint32_t state;
    uint32_t error_behavior;
    uint32_t last_check_time;
    uint32_t check_interval;
    uint32_t creator_os;
    uint32_t revision_level;
    uint32_t reserved[235];
} superblock_t;

typedef struct {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks;
    uint16_t free_inodes;
    uint16_t used_dirs;
    uint16_t reserved;
    uint32_t reserved2[3];
} group_descriptor_t;

typedef struct {
    char name[MAX_FILENAME_LENGTH];
    uint32_t inode_number;
    uint32_t mode;
    uint32_t size;
    uint32_t position;
    bool is_open;
} file_handle_t;

typedef struct {
    char mount_point[MAX_PATH_LENGTH];
    uint32_t device_id;
    superblock_t* superblock;
    group_descriptor_t* group_descriptors;
    inode_t* inode_table;
    uint8_t* block_bitmap;
    uint8_t* inode_bitmap;
} mount_point_t;

typedef struct {
    mount_point_t mount_points[MAX_MOUNT_POINTS];
    uint32_t mount_point_count;
    file_handle_t open_files[MAX_OPEN_FILES];
    uint32_t open_file_count;
    uint32_t current_directory_inode;
} file_system_t;

file_system_t file_system;

void init_file_system() {
    memset(&file_system, 0, sizeof(file_system_t));
    file_system.current_directory_inode = 1; // Inode racine
}

bool mount_file_system(const char* mount_point, uint32_t device_id) {
    if (file_system.mount_point_count >= MAX_MOUNT_POINTS) {
        return false;
    }

    mount_point_t* mp = &file_system.mount_points[file_system.mount_point_count++];
    strncpy(mp->mount_point, mount_point, MAX_PATH_LENGTH - 1);
    mp->mount_point[MAX_PATH_LENGTH - 1] = '\0';
    mp->device_id = device_id;

    // Lire le superblock
    device_t* device = find_device_by_id(device_id);
    if (!device) return false;

    mp->superblock = (superblock_t*)kmalloc(sizeof(superblock_t));
    if (!mp->superblock) return false;

    read_device(device, mp->superblock, sizeof(superblock_t));

    // Vérifier la signature du système de fichiers
    if (mp->superblock->magic != 0xEF53) { // ext4 signature
        kfree(mp->superblock);
        return false;
    }

    // Allouer et lire les descripteurs de groupe
    uint32_t group_count = (mp->superblock->total_blocks + mp->superblock->blocks_per_group - 1) /
                          mp->superblock->blocks_per_group;
    mp->group_descriptors = (group_descriptor_t*)kmalloc(sizeof(group_descriptor_t) * group_count);
    if (!mp->group_descriptors) {
        kfree(mp->superblock);
        return false;
    }

    read_device(device, mp->group_descriptors,
                sizeof(group_descriptor_t) * group_count);

    // Allouer et lire la table d'inodes
    mp->inode_table = (inode_t*)kmalloc(sizeof(inode_t) * mp->superblock->total_inodes);
    if (!mp->inode_table) {
        kfree(mp->group_descriptors);
        kfree(mp->superblock);
        return false;
    }

    read_device(device, mp->inode_table,
                sizeof(inode_t) * mp->superblock->total_inodes);

    // Allouer et lire les bitmaps
    uint32_t bitmap_size = (mp->superblock->blocks_per_group + 7) / 8;
    mp->block_bitmap = (uint8_t*)kmalloc(bitmap_size);
    mp->inode_bitmap = (uint8_t*)kmalloc(bitmap_size);
    if (!mp->block_bitmap || !mp->inode_bitmap) {
        kfree(mp->inode_table);
        kfree(mp->group_descriptors);
        kfree(mp->superblock);
        return false;
    }

    read_device(device, mp->block_bitmap, bitmap_size);
    read_device(device, mp->inode_bitmap, bitmap_size);

    return true;
}

void unmount_file_system(const char* mount_point) {
    for (uint32_t i = 0; i < file_system.mount_point_count; i++) {
        if (strcmp(file_system.mount_points[i].mount_point, mount_point) == 0) {
            mount_point_t* mp = &file_system.mount_points[i];

            // Écrire les modifications sur le disque
            device_t* device = find_device_by_id(mp->device_id);
            if (device) {
                write_device(device, mp->superblock, sizeof(superblock_t));
                write_device(device, mp->group_descriptors,
                           sizeof(group_descriptor_t) * mp->superblock->total_blocks /
                           mp->superblock->blocks_per_group);
                write_device(device, mp->inode_table,
                           sizeof(inode_t) * mp->superblock->total_inodes);
            }

            // Libérer la mémoire
            kfree(mp->block_bitmap);
            kfree(mp->inode_bitmap);
            kfree(mp->inode_table);
            kfree(mp->group_descriptors);
            kfree(mp->superblock);

            // Supprimer le point de montage
            memmove(&file_system.mount_points[i],
                    &file_system.mount_points[i + 1],
                    (file_system.mount_point_count - i - 1) * sizeof(mount_point_t));
            file_system.mount_point_count--;
            break;
        }
    }
}

file_handle_t* open_file(const char* path, uint32_t mode) {
    if (file_system.open_file_count >= MAX_OPEN_FILES) {
        return NULL;
    }

    // Trouver le point de montage
    mount_point_t* mp = NULL;
    const char* relative_path = path;
    for (uint32_t i = 0; i < file_system.mount_point_count; i++) {
        if (strncmp(path, file_system.mount_points[i].mount_point,
                   strlen(file_system.mount_points[i].mount_point)) == 0) {
            mp = &file_system.mount_points[i];
            relative_path = path + strlen(mp->mount_point);
            break;
        }
    }

    if (!mp) return NULL;

    // Trouver l'inode du fichier
    uint32_t inode_number = find_inode_by_path(mp, relative_path);
    if (inode_number == 0) return NULL;

    inode_t* inode = &mp->inode_table[inode_number - 1];

    // Créer le handle de fichier
    file_handle_t* handle = &file_system.open_files[file_system.open_file_count++];
    strncpy(handle->name, path, MAX_FILENAME_LENGTH - 1);
    handle->name[MAX_FILENAME_LENGTH - 1] = '\0';
    handle->inode_number = inode_number;
    handle->mode = mode;
    handle->size = inode->size;
    handle->position = 0;
    handle->is_open = true;

    return handle;
}

void close_file(file_handle_t* handle) {
    if (!handle || !handle->is_open) return;

    handle->is_open = false;

    // Trouver et supprimer le handle
    for (uint32_t i = 0; i < file_system.open_file_count; i++) {
        if (&file_system.open_files[i] == handle) {
            memmove(&file_system.open_files[i],
                    &file_system.open_files[i + 1],
                    (file_system.open_file_count - i - 1) * sizeof(file_handle_t));
            file_system.open_file_count--;
            break;
        }
    }
}

int read_file(file_handle_t* handle, void* buffer, size_t size) {
    if (!handle || !handle->is_open || !buffer) return -1;

    // Trouver le point de montage
    mount_point_t* mp = find_mount_point_by_path(handle->name);
    if (!mp) return -1;

    inode_t* inode = &mp->inode_table[handle->inode_number - 1];
    if (handle->position >= inode->size) return 0;

    size_t bytes_to_read = min(size, inode->size - handle->position);
    size_t bytes_read = 0;

    while (bytes_read < bytes_to_read) {
        uint32_t block_number = get_block_number(inode, handle->position + bytes_read);
        if (block_number == 0) break;

        size_t block_offset = (handle->position + bytes_read) % BLOCK_SIZE;
        size_t bytes_in_block = min(BLOCK_SIZE - block_offset,
                                  bytes_to_read - bytes_read);

        device_t* device = find_device_by_id(mp->device_id);
        if (!device) break;

        uint8_t block_buffer[BLOCK_SIZE];
        read_device(device, block_buffer, BLOCK_SIZE);

        memcpy((uint8_t*)buffer + bytes_read,
               block_buffer + block_offset,
               bytes_in_block);

        bytes_read += bytes_in_block;
    }

    handle->position += bytes_read;
    return bytes_read;
}

int write_file(file_handle_t* handle, const void* buffer, size_t size) {
    if (!handle || !handle->is_open || !buffer) return -1;

    // Trouver le point de montage
    mount_point_t* mp = find_mount_point_by_path(handle->name);
    if (!mp) return -1;

    inode_t* inode = &mp->inode_table[handle->inode_number - 1];
    size_t bytes_written = 0;

    while (bytes_written < size) {
        uint32_t block_number = get_block_number(inode, handle->position + bytes_written);
        if (block_number == 0) {
            // Allouer un nouveau bloc
            block_number = allocate_block(mp);
            if (block_number == 0) break;

            set_block_number(inode, handle->position + bytes_written, block_number);
        }

        size_t block_offset = (handle->position + bytes_written) % BLOCK_SIZE;
        size_t bytes_in_block = min(BLOCK_SIZE - block_offset,
                                  size - bytes_written);

        device_t* device = find_device_by_id(mp->device_id);
        if (!device) break;

        uint8_t block_buffer[BLOCK_SIZE];
        if (block_offset > 0 || bytes_in_block < BLOCK_SIZE) {
            read_device(device, block_buffer, BLOCK_SIZE);
        }

        memcpy(block_buffer + block_offset,
               (uint8_t*)buffer + bytes_written,
               bytes_in_block);

        write_device(device, block_buffer, BLOCK_SIZE);

        bytes_written += bytes_in_block;
    }

    handle->position += bytes_written;
    if (handle->position > inode->size) {
        inode->size = handle->position;
        inode->modification_time = get_current_time();
    }

    return bytes_written;
}

uint32_t find_inode_by_path(mount_point_t* mp, const char* path) {
    if (!path || path[0] != '/') return 0;

    uint32_t current_inode = 1; // Inode racine
    char* path_copy = strdup(path);
    char* component = strtok(path_copy, "/");

    while (component) {
        inode_t* inode = &mp->inode_table[current_inode - 1];
        if (!(inode->mode & 0x4000)) { // Vérifier si c'est un répertoire
            free(path_copy);
            return 0;
        }

        bool found = false;
        uint32_t block_number = 0;
        uint32_t block_offset = 0;

        while (get_next_block(inode, &block_number, &block_offset)) {
            device_t* device = find_device_by_id(mp->device_id);
            if (!device) continue;

            uint8_t block_buffer[BLOCK_SIZE];
            read_device(device, block_buffer, BLOCK_SIZE);

            directory_entry_t* entry = (directory_entry_t*)block_buffer;
            while ((uint8_t*)entry < block_buffer + BLOCK_SIZE) {
                if (entry->inode_number != 0 &&
                    strcmp(entry->name, component) == 0) {
                    current_inode = entry->inode_number;
                    found = true;
                    break;
                }
                entry = (directory_entry_t*)((uint8_t*)entry + entry->name_length + 8);
            }

            if (found) break;
        }

        if (!found) {
            free(path_copy);
            return 0;
        }

        component = strtok(NULL, "/");
    }

    free(path_copy);
    return current_inode;
}

uint32_t get_block_number(inode_t* inode, uint32_t offset) {
    uint32_t block_index = offset / BLOCK_SIZE;

    if (block_index < MAX_DIRECT_BLOCKS) {
        return inode->direct_blocks[block_index];
    }

    block_index -= MAX_DIRECT_BLOCKS;
    if (block_index < MAX_INDIRECT_BLOCKS) {
        uint32_t indirect_block[BLOCK_SIZE / 4];
        device_t* device = find_device_by_id(current_device_id);
        if (!device) return 0;

        read_device(device, indirect_block, BLOCK_SIZE);
        return indirect_block[block_index];
    }

    block_index -= MAX_INDIRECT_BLOCKS;
    if (block_index < MAX_INDIRECT_BLOCKS * MAX_INDIRECT_BLOCKS) {
        uint32_t double_indirect_block[BLOCK_SIZE / 4];
        uint32_t indirect_block[BLOCK_SIZE / 4];
        device_t* device = find_device_by_id(current_device_id);
        if (!device) return 0;

        read_device(device, double_indirect_block, BLOCK_SIZE);
        read_device(device, indirect_block, BLOCK_SIZE);
        return indirect_block[block_index / MAX_INDIRECT_BLOCKS];
    }

    return 0;
}

void set_block_number(inode_t* inode, uint32_t offset, uint32_t block_number) {
    uint32_t block_index = offset / BLOCK_SIZE;

    if (block_index < MAX_DIRECT_BLOCKS) {
        inode->direct_blocks[block_index] = block_number;
        return;
    }

    block_index -= MAX_DIRECT_BLOCKS;
    if (block_index < MAX_INDIRECT_BLOCKS) {
        if (inode->indirect_block == 0) {
            inode->indirect_block = allocate_block(current_mount_point);
        }

        uint32_t indirect_block[BLOCK_SIZE / 4];
        device_t* device = find_device_by_id(current_device_id);
        if (!device) return;

        read_device(device, indirect_block, BLOCK_SIZE);
        indirect_block[block_index] = block_number;
        write_device(device, indirect_block, BLOCK_SIZE);
        return;
    }

    block_index -= MAX_INDIRECT_BLOCKS;
    if (block_index < MAX_INDIRECT_BLOCKS * MAX_INDIRECT_BLOCKS) {
        if (inode->double_indirect_block == 0) {
            inode->double_indirect_block = allocate_block(current_mount_point);
        }

        uint32_t double_indirect_block[BLOCK_SIZE / 4];
        uint32_t indirect_block[BLOCK_SIZE / 4];
        device_t* device = find_device_by_id(current_device_id);
        if (!device) return;

        read_device(device, double_indirect_block, BLOCK_SIZE);
        if (double_indirect_block[block_index / MAX_INDIRECT_BLOCKS] == 0) {
            double_indirect_block[block_index / MAX_INDIRECT_BLOCKS] = allocate_block(current_mount_point);
            write_device(device, double_indirect_block, BLOCK_SIZE);
        }

        read_device(device, indirect_block, BLOCK_SIZE);
        indirect_block[block_index % MAX_INDIRECT_BLOCKS] = block_number;
        write_device(device, indirect_block, BLOCK_SIZE);
    }
}

uint32_t allocate_block(mount_point_t* mp) {
    for (uint32_t group = 0; group < mp->superblock->total_blocks / mp->superblock->blocks_per_group; group++) {
        group_descriptor_t* gd = &mp->group_descriptors[group];
        if (gd->free_blocks == 0) continue;

        for (uint32_t i = 0; i < mp->superblock->blocks_per_group; i++) {
            if (!(mp->block_bitmap[group * mp->superblock->blocks_per_group / 8 + i / 8] & (1 << (i % 8)))) {
                mp->block_bitmap[group * mp->superblock->blocks_per_group / 8 + i / 8] |= (1 << (i % 8));
                gd->free_blocks--;
                mp->superblock->free_blocks--;
                return group * mp->superblock->blocks_per_group + i + mp->superblock->first_data_block;
            }
        }
    }
    return 0;
}

void free_block(mount_point_t* mp, uint32_t block_number) {
    uint32_t group = (block_number - mp->superblock->first_data_block) / mp->superblock->blocks_per_group;
    uint32_t index = (block_number - mp->superblock->first_data_block) % mp->superblock->blocks_per_group;

    mp->block_bitmap[group * mp->superblock->blocks_per_group / 8 + index / 8] &= ~(1 << (index % 8));
    mp->group_descriptors[group].free_blocks++;
    mp->superblock->free_blocks++;
} 