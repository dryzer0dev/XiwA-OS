#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FILENAME_LENGTH 255
#define MAX_PATH_LENGTH 4096
#define MAX_FILES_PER_DIR 1024
#define BLOCK_SIZE 4096
#define INODE_SIZE 256
#define MAX_BLOCKS_PER_INODE 12
#define MAX_INDIRECT_BLOCKS 1024

typedef struct {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t size;
    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;
    uint32_t blocks;
    uint32_t flags;
    uint32_t osd1;
    uint32_t block[15];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
    uint16_t i_extra_isize;
    uint16_t i_checksum_hi;
    uint32_t i_ctime_extra;
    uint32_t i_mtime_extra;
    uint32_t i_atime_extra;
    uint32_t i_crtime;
    uint32_t i_crtime_extra;
    uint32_t i_version_hi;
    uint32_t i_projid;
} inode_t;

typedef struct {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[MAX_FILENAME_LENGTH];
} directory_entry_t;

typedef struct {
    uint32_t magic;
    uint32_t inode_count;
    uint32_t block_count;
    uint32_t reserved_blocks;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t mount_time;
    uint32_t write_time;
    uint16_t mount_count;
    uint16_t max_mount_count;
    uint16_t magic_signature;
    uint16_t file_system_state;
    uint16_t error_behavior;
    uint16_t minor_revision_level;
    uint32_t last_check_time;
    uint32_t check_interval;
    uint32_t creator_os;
    uint32_t revision_level;
    uint16_t default_reserved_uid;
    uint16_t default_reserved_gid;
} superblock_t;

typedef struct {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t pad;
    uint32_t reserved[3];
} group_descriptor_t;

typedef struct {
    int fd;
    uint32_t inode;
    uint32_t position;
    uint32_t flags;
} file_handle_t;

typedef struct {
    char path[MAX_PATH_LENGTH];
    uint32_t device;
    uint32_t flags;
    superblock_t* superblock;
    group_descriptor_t* group_descriptors;
    inode_t* inodes;
    uint32_t* block_bitmap;
    uint32_t* inode_bitmap;
} mount_point_t;

typedef struct {
    mount_point_t* mount_points;
    uint32_t mount_point_count;
    file_handle_t* file_handles;
    uint32_t file_handle_count;
} file_system_t;

file_system_t file_system;

void init_file_system() {
    memset(&file_system, 0, sizeof(file_system_t));
}

bool mount_file_system(const char* path, uint32_t device) {
    if (file_system.mount_point_count >= 16) {
        return false;
    }

    mount_point_t* mount = &file_system.mount_points[file_system.mount_point_count++];
    strncpy(mount->path, path, sizeof(mount->path) - 1);
    mount->device = device;
    mount->flags = 0;

    // Lire le superblock
    mount->superblock = (superblock_t*)kmalloc(sizeof(superblock_t));
    if (!mount->superblock) {
        file_system.mount_point_count--;
        return false;
    }

    // Lire le superblock depuis le périphérique
    device_t* dev = find_device_by_id(device);
    if (!dev) {
        kfree(mount->superblock);
        file_system.mount_point_count--;
        return false;
    }

    if (read_device(dev, mount->superblock, sizeof(superblock_t), 1024) != sizeof(superblock_t)) {
        kfree(mount->superblock);
        file_system.mount_point_count--;
        return false;
    }

    // Vérifier la signature magique
    if (mount->superblock->magic != 0xEF53) {
        kfree(mount->superblock);
        file_system.mount_point_count--;
        return false;
    }

    // Allouer les descripteurs de groupe
    uint32_t group_count = (mount->superblock->block_count + mount->superblock->blocks_per_group - 1) /
                          mount->superblock->blocks_per_group;
    mount->group_descriptors = (group_descriptor_t*)kmalloc(sizeof(group_descriptor_t) * group_count);
    if (!mount->group_descriptors) {
        kfree(mount->superblock);
        file_system.mount_point_count--;
        return false;
    }

    // Lire les descripteurs de groupe
    if (read_device(dev, mount->group_descriptors,
                   sizeof(group_descriptor_t) * group_count,
                   1024 + sizeof(superblock_t)) != sizeof(group_descriptor_t) * group_count) {
        kfree(mount->group_descriptors);
        kfree(mount->superblock);
        file_system.mount_point_count--;
        return false;
    }

    // Allouer les inodes
    mount->inodes = (inode_t*)kmalloc(sizeof(inode_t) * mount->superblock->inode_count);
    if (!mount->inodes) {
        kfree(mount->group_descriptors);
        kfree(mount->superblock);
        file_system.mount_point_count--;
        return false;
    }

    // Allouer les bitmaps
    uint32_t block_bitmap_size = (mount->superblock->block_count + 31) / 32;
    uint32_t inode_bitmap_size = (mount->superblock->inode_count + 31) / 32;
    mount->block_bitmap = (uint32_t*)kmalloc(block_bitmap_size * sizeof(uint32_t));
    mount->inode_bitmap = (uint32_t*)kmalloc(inode_bitmap_size * sizeof(uint32_t));
    if (!mount->block_bitmap || !mount->inode_bitmap) {
        kfree(mount->inodes);
        kfree(mount->group_descriptors);
        kfree(mount->superblock);
        file_system.mount_point_count--;
        return false;
    }

    return true;
}

void unmount_file_system(const char* path) {
    for (uint32_t i = 0; i < file_system.mount_point_count; i++) {
        if (strcmp(file_system.mount_points[i].path, path) == 0) {
            mount_point_t* mount = &file_system.mount_points[i];

            // Libérer les ressources
            kfree(mount->inode_bitmap);
            kfree(mount->block_bitmap);
            kfree(mount->inodes);
            kfree(mount->group_descriptors);
            kfree(mount->superblock);

            // Supprimer le point de montage
            memmove(&file_system.mount_points[i],
                    &file_system.mount_points[i + 1],
                    (file_system.mount_point_count - i - 1) * sizeof(mount_point_t));
            file_system.mount_point_count--;
            break;
        }
    }
}

int open_file(const char* path, uint32_t flags) {
    if (file_system.file_handle_count >= 256) {
        return -1;
    }

    // Trouver le point de montage
    mount_point_t* mount = NULL;
    for (uint32_t i = 0; i < file_system.mount_point_count; i++) {
        if (strncmp(path, file_system.mount_points[i].path,
                   strlen(file_system.mount_points[i].path)) == 0) {
            mount = &file_system.mount_points[i];
            break;
        }
    }

    if (!mount) {
        return -1;
    }

    // Trouver l'inode
    uint32_t inode = find_inode_by_path(mount, path + strlen(mount->path));
    if (!inode) {
        return -1;
    }

    // Créer un nouveau descripteur de fichier
    file_handle_t* handle = &file_system.file_handles[file_system.file_handle_count++];
    handle->fd = file_system.file_handle_count;
    handle->inode = inode;
    handle->position = 0;
    handle->flags = flags;

    return handle->fd;
}

void close_file(int fd) {
    if (fd <= 0 || fd > file_system.file_handle_count) {
        return;
    }

    // Supprimer le descripteur de fichier
    memmove(&file_system.file_handles[fd - 1],
            &file_system.file_handles[fd],
            (file_system.file_handle_count - fd) * sizeof(file_handle_t));
    file_system.file_handle_count--;
}

int read_file(int fd, void* buffer, size_t size) {
    if (fd <= 0 || fd > file_system.file_handle_count || !buffer) {
        return -1;
    }

    file_handle_t* handle = &file_system.file_handles[fd - 1];
    mount_point_t* mount = NULL;

    // Trouver le point de montage
    for (uint32_t i = 0; i < file_system.mount_point_count; i++) {
        if (file_system.mount_points[i].inodes[handle->inode - 1].block[0]) {
            mount = &file_system.mount_points[i];
            break;
        }
    }

    if (!mount) {
        return -1;
    }

    inode_t* inode = &mount->inodes[handle->inode - 1];
    if (handle->position >= inode->size) {
        return 0;
    }

    size_t bytes_to_read = min(size, inode->size - handle->position);
    size_t bytes_read = 0;

    while (bytes_read < bytes_to_read) {
        uint32_t block_index = handle->position / BLOCK_SIZE;
        uint32_t block_offset = handle->position % BLOCK_SIZE;
        uint32_t block_number = get_block_number(mount, inode, block_index);

        if (!block_number) {
            break;
        }

        size_t bytes_in_block = min(bytes_to_read - bytes_read, BLOCK_SIZE - block_offset);
        device_t* dev = find_device_by_id(mount->device);
        if (!dev) {
            break;
        }

        if (read_device(dev, (uint8_t*)buffer + bytes_read,
                       bytes_in_block,
                       block_number * BLOCK_SIZE + block_offset) != bytes_in_block) {
            break;
        }

        bytes_read += bytes_in_block;
        handle->position += bytes_in_block;
    }

    return bytes_read;
}

int write_file(int fd, const void* buffer, size_t size) {
    if (fd <= 0 || fd > file_system.file_handle_count || !buffer) {
        return -1;
    }

    file_handle_t* handle = &file_system.file_handles[fd - 1];
    mount_point_t* mount = NULL;

    // Trouver le point de montage
    for (uint32_t i = 0; i < file_system.mount_point_count; i++) {
        if (file_system.mount_points[i].inodes[handle->inode - 1].block[0]) {
            mount = &file_system.mount_points[i];
            break;
        }
    }

    if (!mount) {
        return -1;
    }

    inode_t* inode = &mount->inodes[handle->inode - 1];
    size_t bytes_written = 0;

    while (bytes_written < size) {
        uint32_t block_index = handle->position / BLOCK_SIZE;
        uint32_t block_offset = handle->position % BLOCK_SIZE;
        uint32_t block_number = get_block_number(mount, inode, block_index);

        if (!block_number) {
            // Allouer un nouveau bloc
            block_number = allocate_block(mount);
            if (!block_number) {
                break;
            }

            if (!set_block_number(mount, inode, block_index, block_number)) {
                free_block(mount, block_number);
                break;
            }
        }

        size_t bytes_in_block = min(size - bytes_written, BLOCK_SIZE - block_offset);
        device_t* dev = find_device_by_id(mount->device);
        if (!dev) {
            break;
        }

        if (write_device(dev, (uint8_t*)buffer + bytes_written,
                        bytes_in_block,
                        block_number * BLOCK_SIZE + block_offset) != bytes_in_block) {
            break;
        }

        bytes_written += bytes_in_block;
        handle->position += bytes_in_block;

        if (handle->position > inode->size) {
            inode->size = handle->position;
        }
    }

    return bytes_written;
}

uint32_t find_inode_by_path(mount_point_t* mount, const char* path) {
    if (!path || path[0] != '/') {
        return 0;
    }

    uint32_t current_inode = 2; // Inode racine
    char* path_copy = strdup(path);
    char* token = strtok(path_copy, "/");

    while (token) {
        bool found = false;
        inode_t* inode = &mount->inodes[current_inode - 1];

        // Lire le répertoire
        uint8_t* buffer = (uint8_t*)kmalloc(BLOCK_SIZE);
        if (!buffer) {
            kfree(path_copy);
            return 0;
        }

        for (uint32_t i = 0; i < MAX_BLOCKS_PER_INODE; i++) {
            if (!inode->block[i]) {
                continue;
            }

            device_t* dev = find_device_by_id(mount->device);
            if (!dev) {
                kfree(buffer);
                kfree(path_copy);
                return 0;
            }

            if (read_device(dev, buffer, BLOCK_SIZE,
                           inode->block[i] * BLOCK_SIZE) != BLOCK_SIZE) {
                kfree(buffer);
                kfree(path_copy);
                return 0;
            }

            directory_entry_t* entry = (directory_entry_t*)buffer;
            while ((uint8_t*)entry < buffer + BLOCK_SIZE) {
                if (entry->inode && strncmp(entry->name, token, entry->name_len) == 0) {
                    current_inode = entry->inode;
                    found = true;
                    break;
                }
                entry = (directory_entry_t*)((uint8_t*)entry + entry->rec_len);
            }

            if (found) {
                break;
            }
        }

        kfree(buffer);
        if (!found) {
            kfree(path_copy);
            return 0;
        }

        token = strtok(NULL, "/");
    }

    kfree(path_copy);
    return current_inode;
}

uint32_t get_block_number(mount_point_t* mount, inode_t* inode, uint32_t block_index) {
    if (block_index < MAX_BLOCKS_PER_INODE) {
        return inode->block[block_index];
    }

    block_index -= MAX_BLOCKS_PER_INODE;
    if (block_index < MAX_INDIRECT_BLOCKS) {
        uint32_t* indirect_blocks = (uint32_t*)kmalloc(BLOCK_SIZE);
        if (!indirect_blocks) {
            return 0;
        }

        device_t* dev = find_device_by_id(mount->device);
        if (!dev) {
            kfree(indirect_blocks);
            return 0;
        }

        if (read_device(dev, indirect_blocks, BLOCK_SIZE,
                       inode->block[12] * BLOCK_SIZE) != BLOCK_SIZE) {
            kfree(indirect_blocks);
            return 0;
        }

        uint32_t block_number = indirect_blocks[block_index];
        kfree(indirect_blocks);
        return block_number;
    }

    return 0;
}

bool set_block_number(mount_point_t* mount, inode_t* inode, uint32_t block_index, uint32_t block_number) {
    if (block_index < MAX_BLOCKS_PER_INODE) {
        inode->block[block_index] = block_number;
        return true;
    }

    block_index -= MAX_BLOCKS_PER_INODE;
    if (block_index < MAX_INDIRECT_BLOCKS) {
        if (!inode->block[12]) {
            inode->block[12] = allocate_block(mount);
            if (!inode->block[12]) {
                return false;
            }
        }

        uint32_t* indirect_blocks = (uint32_t*)kmalloc(BLOCK_SIZE);
        if (!indirect_blocks) {
            return false;
        }

        device_t* dev = find_device_by_id(mount->device);
        if (!dev) {
            kfree(indirect_blocks);
            return false;
        }

        if (read_device(dev, indirect_blocks, BLOCK_SIZE,
                       inode->block[12] * BLOCK_SIZE) != BLOCK_SIZE) {
            kfree(indirect_blocks);
            return false;
        }

        indirect_blocks[block_index] = block_number;

        if (write_device(dev, indirect_blocks, BLOCK_SIZE,
                        inode->block[12] * BLOCK_SIZE) != BLOCK_SIZE) {
            kfree(indirect_blocks);
            return false;
        }

        kfree(indirect_blocks);
        return true;
    }

    return false;
}

uint32_t allocate_block(mount_point_t* mount) {
    for (uint32_t i = 0; i < mount->superblock->block_count; i++) {
        uint32_t word_index = i / 32;
        uint32_t bit_index = i % 32;

        if (!(mount->block_bitmap[word_index] & (1 << bit_index))) {
            mount->block_bitmap[word_index] |= (1 << bit_index);
            mount->superblock->free_blocks--;

            // Écrire le bitmap mis à jour
            device_t* dev = find_device_by_id(mount->device);
            if (!dev) {
                return 0;
            }

            if (write_device(dev, mount->block_bitmap,
                           (mount->superblock->block_count + 31) / 32 * sizeof(uint32_t),
                           mount->group_descriptors[0].block_bitmap * BLOCK_SIZE) !=
                (mount->superblock->block_count + 31) / 32 * sizeof(uint32_t)) {
                return 0;
            }

            return i + mount->superblock->first_data_block;
        }
    }

    return 0;
}

void free_block(mount_point_t* mount, uint32_t block_number) {
    if (block_number < mount->superblock->first_data_block ||
        block_number >= mount->superblock->first_data_block + mount->superblock->block_count) {
        return;
    }

    uint32_t index = block_number - mount->superblock->first_data_block;
    uint32_t word_index = index / 32;
    uint32_t bit_index = index % 32;

    mount->block_bitmap[word_index] &= ~(1 << bit_index);
    mount->superblock->free_blocks++;

    // Écrire le bitmap mis à jour
    device_t* dev = find_device_by_id(mount->device);
    if (!dev) {
        return;
    }

    write_device(dev, mount->block_bitmap,
                (mount->superblock->block_count + 31) / 32 * sizeof(uint32_t),
                mount->group_descriptors[0].block_bitmap * BLOCK_SIZE);
} 