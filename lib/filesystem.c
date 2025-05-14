#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FILES 1024
#define MAX_DIRS 256
#define MAX_MOUNTS 16
#define BLOCK_SIZE 4096
#define MAX_FILENAME 256
#define MAX_PATH 1024

typedef struct {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[MAX_FILENAME];
} ext4_dir_entry_t;

typedef struct {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t blocks;
    uint32_t flags;
    uint32_t osd1;
    uint32_t block[15];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
    uint8_t osd2[12];
} ext4_inode_t;

typedef struct {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t magic;
    uint16_t state;
    uint16_t errors;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t rev_level;
    uint16_t def_resuid;
    uint16_t def_resgid;
    uint32_t first_ino;
    uint16_t inode_size;
    uint16_t block_group_nr;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    uint8_t uuid[16];
    char volume_name[16];
    char last_mounted[64];
    uint32_t algorithm_usage_bitmap;
    uint8_t prealloc_blocks;
    uint8_t prealloc_dir_blocks;
    uint16_t reserved_gdt_blocks;
    uint8_t journal_uuid[16];
    uint32_t journal_inum;
    uint32_t journal_dev;
    uint32_t last_orphan;
    uint32_t hash_seed[4];
    uint8_t def_hash_version;
    uint8_t jnl_backup_type;
    uint16_t desc_size;
    uint32_t default_mount_opts;
    uint32_t first_meta_bg;
    uint32_t mkfs_time;
    uint32_t jnl_blocks[17];
} ext4_superblock_t;

typedef struct {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t pad;
    uint8_t reserved[12];
} ext4_group_desc_t;

typedef struct {
    uint32_t inode;
    uint32_t position;
    uint32_t size;
    uint32_t flags;
    ext4_inode_t inode_data;
} file_t;

typedef struct {
    uint32_t inode;
    char name[MAX_FILENAME];
    file_t* files;
    uint32_t file_count;
} directory_t;

typedef struct {
    char path[MAX_PATH];
    char device[MAX_FILENAME];
    ext4_superblock_t superblock;
    ext4_group_desc_t* group_descriptors;
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t group_count;
    bool mounted;
} mount_t;

typedef struct {
    file_t files[MAX_FILES];
    uint32_t file_count;
    directory_t directories[MAX_DIRS];
    uint32_t directory_count;
    mount_t mounts[MAX_MOUNTS];
    uint32_t mount_count;
} filesystem_t;

static filesystem_t filesystem;

void init_filesystem() {
    memset(&filesystem, 0, sizeof(filesystem_t));
}

bool mount_filesystem(const char* path, const char* device) {
    if (filesystem.mount_count >= MAX_MOUNTS) return false;

    mount_t* mount = &filesystem.mounts[filesystem.mount_count++];
    strncpy(mount->path, path, sizeof(mount->path) - 1);
    strncpy(mount->device, device, sizeof(mount->device) - 1);

    uint8_t* superblock_data = (uint8_t*)kmalloc(BLOCK_SIZE);
    if (!superblock_data) return false;

    if (read_device(0, superblock_data, BLOCK_SIZE) != BLOCK_SIZE) {
        kfree(superblock_data);
        return false;
    }

    memcpy(&mount->superblock, superblock_data, sizeof(ext4_superblock_t));
    kfree(superblock_data);

    if (mount->superblock.magic != 0xEF53) return false;

    mount->block_size = 1024 << mount->superblock.log_block_size;
    mount->inode_size = mount->superblock.inode_size;
    mount->blocks_per_group = mount->superblock.blocks_per_group;
    mount->inodes_per_group = mount->superblock.inodes_per_group;
    mount->group_count = (mount->superblock.blocks_count + mount->blocks_per_group - 1) / mount->blocks_per_group;

    mount->group_descriptors = (ext4_group_desc_t*)kmalloc(mount->group_count * sizeof(ext4_group_desc_t));
    if (!mount->group_descriptors) return false;

    uint32_t group_desc_block = mount->superblock.first_data_block + 1;
    uint32_t group_desc_size = mount->group_count * sizeof(ext4_group_desc_t);
    uint32_t group_desc_blocks = (group_desc_size + mount->block_size - 1) / mount->block_size;

    for (uint32_t i = 0; i < group_desc_blocks; i++) {
        uint8_t* block_data = (uint8_t*)kmalloc(mount->block_size);
        if (!block_data) {
            kfree(mount->group_descriptors);
            return false;
        }

        if (read_device(0, block_data, mount->block_size) != mount->block_size) {
            kfree(block_data);
            kfree(mount->group_descriptors);
            return false;
        }

        memcpy((uint8_t*)mount->group_descriptors + i * mount->block_size,
               block_data,
               i == group_desc_blocks - 1 ? group_desc_size % mount->block_size : mount->block_size);

        kfree(block_data);
    }

    mount->mounted = true;
    return true;
}

void unmount_filesystem(const char* path) {
    for (uint32_t i = 0; i < filesystem.mount_count; i++) {
        if (strcmp(filesystem.mounts[i].path, path) == 0) {
            mount_t* mount = &filesystem.mounts[i];
            kfree(mount->group_descriptors);
            mount->mounted = false;
            filesystem.mounts[i] = filesystem.mounts[--filesystem.mount_count];
            break;
        }
    }
}

file_t* open_file(const char* path) {
    if (filesystem.file_count >= MAX_FILES) return NULL;

    char mount_path[MAX_PATH];
    char relative_path[MAX_PATH];
    mount_t* mount = NULL;

    for (uint32_t i = 0; i < filesystem.mount_count; i++) {
        if (strncmp(path, filesystem.mounts[i].path, strlen(filesystem.mounts[i].path)) == 0) {
            mount = &filesystem.mounts[i];
            strncpy(mount_path, mount->path, sizeof(mount_path) - 1);
            strncpy(relative_path, path + strlen(mount->path), sizeof(relative_path) - 1);
            break;
        }
    }

    if (!mount || !mount->mounted) return NULL;

    char* component = strtok(relative_path, "/");
    uint32_t current_inode = 2;

    while (component) {
        bool found = false;
        uint32_t group = (current_inode - 1) / mount->inodes_per_group;
        uint32_t index = (current_inode - 1) % mount->inodes_per_group;
        uint32_t block = mount->group_descriptors[group].inode_table + (index * mount->inode_size) / mount->block_size;
        uint32_t offset = (index * mount->inode_size) % mount->block_size;

        uint8_t* block_data = (uint8_t*)kmalloc(mount->block_size);
        if (!block_data) return NULL;

        if (read_device(0, block_data, mount->block_size) != mount->block_size) {
            kfree(block_data);
            return NULL;
        }

        ext4_inode_t* inode = (ext4_inode_t*)(block_data + offset);
        uint32_t block_count = (inode->size + mount->block_size - 1) / mount->block_size;

        for (uint32_t i = 0; i < block_count; i++) {
            uint32_t block_number;
            if (i < 12) {
                block_number = inode->block[i];
            } else if (i < 12 + mount->block_size / 4) {
                uint32_t* indirect_block = (uint32_t*)kmalloc(mount->block_size);
                if (!indirect_block) {
                    kfree(block_data);
                    return NULL;
                }

                if (read_device(0, indirect_block, mount->block_size) != mount->block_size) {
                    kfree(indirect_block);
                    kfree(block_data);
                    return NULL;
                }

                block_number = indirect_block[i - 12];
                kfree(indirect_block);
            } else {
                kfree(block_data);
                return NULL;
            }

            uint8_t* dir_block = (uint8_t*)kmalloc(mount->block_size);
            if (!dir_block) {
                kfree(block_data);
                return NULL;
            }

            if (read_device(0, dir_block, mount->block_size) != mount->block_size) {
                kfree(dir_block);
                kfree(block_data);
                return NULL;
            }

            ext4_dir_entry_t* entry = (ext4_dir_entry_t*)dir_block;
            while ((uint8_t*)entry < dir_block + mount->block_size) {
                if (entry->inode && entry->name_len == strlen(component) &&
                    strncmp(entry->name, component, entry->name_len) == 0) {
                    current_inode = entry->inode;
                    found = true;
                    break;
                }
                entry = (ext4_dir_entry_t*)((uint8_t*)entry + entry->rec_len);
            }

            kfree(dir_block);
            if (found) break;
        }

        kfree(block_data);
        if (!found) return NULL;

        component = strtok(NULL, "/");
    }

    file_t* file = &filesystem.files[filesystem.file_count++];
    file->inode = current_inode;
    file->position = 0;
    file->size = 0;
    file->flags = 0;

    uint32_t group = (current_inode - 1) / mount->inodes_per_group;
    uint32_t index = (current_inode - 1) % mount->inodes_per_group;
    uint32_t block = mount->group_descriptors[group].inode_table + (index * mount->inode_size) / mount->block_size;
    uint32_t offset = (index * mount->inode_size) % mount->block_size;

    uint8_t* block_data = (uint8_t*)kmalloc(mount->block_size);
    if (!block_data) return NULL;

    if (read_device(0, block_data, mount->block_size) != mount->block_size) {
        kfree(block_data);
        return NULL;
    }

    memcpy(&file->inode_data, block_data + offset, sizeof(ext4_inode_t));
    file->size = file->inode_data.size;

    kfree(block_data);
    return file;
}

void close_file(file_t* file) {
    if (!file) return;
    for (uint32_t i = 0; i < filesystem.file_count; i++) {
        if (&filesystem.files[i] == file) {
            filesystem.files[i] = filesystem.files[--filesystem.file_count];
            break;
        }
    }
}

int32_t read_file(file_t* file, void* buffer, uint32_t size) {
    if (!file || !buffer || !size) return -1;

    mount_t* mount = NULL;
    for (uint32_t i = 0; i < filesystem.mount_count; i++) {
        if (filesystem.mounts[i].mounted) {
            mount = &filesystem.mounts[i];
            break;
        }
    }

    if (!mount) return -1;

    uint32_t bytes_read = 0;
    while (bytes_read < size && file->position < file->size) {
        uint32_t block_index = file->position / mount->block_size;
        uint32_t block_offset = file->position % mount->block_size;
        uint32_t bytes_to_read = mount->block_size - block_offset;
        if (bytes_to_read > size - bytes_read) bytes_to_read = size - bytes_read;
        if (bytes_to_read > file->size - file->position) bytes_to_read = file->size - file->position;

        uint32_t block_number;
        if (block_index < 12) {
            block_number = file->inode_data.block[block_index];
        } else if (block_index < 12 + mount->block_size / 4) {
            uint32_t* indirect_block = (uint32_t*)kmalloc(mount->block_size);
            if (!indirect_block) return bytes_read;

            if (read_device(0, indirect_block, mount->block_size) != mount->block_size) {
                kfree(indirect_block);
                return bytes_read;
            }

            block_number = indirect_block[block_index - 12];
            kfree(indirect_block);
        } else {
            return bytes_read;
        }

        uint8_t* block_data = (uint8_t*)kmalloc(mount->block_size);
        if (!block_data) return bytes_read;

        if (read_device(0, block_data, mount->block_size) != mount->block_size) {
            kfree(block_data);
            return bytes_read;
        }

        memcpy((uint8_t*)buffer + bytes_read, block_data + block_offset, bytes_to_read);
        kfree(block_data);

        bytes_read += bytes_to_read;
        file->position += bytes_to_read;
    }

    return bytes_read;
}

int32_t write_file(file_t* file, const void* buffer, uint32_t size) {
    if (!file || !buffer || !size) return -1;

    mount_t* mount = NULL;
    for (uint32_t i = 0; i < filesystem.mount_count; i++) {
        if (filesystem.mounts[i].mounted) {
            mount = &filesystem.mounts[i];
            break;
        }
    }

    if (!mount) return -1;

    uint32_t bytes_written = 0;
    while (bytes_written < size) {
        uint32_t block_index = file->position / mount->block_size;
        uint32_t block_offset = file->position % mount->block_size;
        uint32_t bytes_to_write = mount->block_size - block_offset;
        if (bytes_to_write > size - bytes_written) bytes_to_write = size - bytes_written;

        uint32_t block_number;
        if (block_index < 12) {
            if (file->inode_data.block[block_index] == 0) {
                uint32_t group = (file->inode - 1) / mount->inodes_per_group;
                uint32_t* block_bitmap = (uint32_t*)kmalloc(mount->block_size);
                if (!block_bitmap) return bytes_written;

                if (read_device(0, block_bitmap, mount->block_size) != mount->block_size) {
                    kfree(block_bitmap);
                    return bytes_written;
                }

                for (uint32_t i = 0; i < mount->block_size * 8; i++) {
                    if (!(block_bitmap[i / 32] & (1 << (i % 32)))) {
                        block_bitmap[i / 32] |= 1 << (i % 32);
                        file->inode_data.block[block_index] = mount->group_descriptors[group].block_bitmap * mount->block_size + i;
                        break;
                    }
                }

                if (write_device(0, block_bitmap, mount->block_size) != mount->block_size) {
                    kfree(block_bitmap);
                    return bytes_written;
                }

                kfree(block_bitmap);
            }
            block_number = file->inode_data.block[block_index];
        } else if (block_index < 12 + mount->block_size / 4) {
            uint32_t* indirect_block = (uint32_t*)kmalloc(mount->block_size);
            if (!indirect_block) return bytes_written;

            if (read_device(0, indirect_block, mount->block_size) != mount->block_size) {
                kfree(indirect_block);
                return bytes_written;
            }

            if (indirect_block[block_index - 12] == 0) {
                uint32_t group = (file->inode - 1) / mount->inodes_per_group;
                uint32_t* block_bitmap = (uint32_t*)kmalloc(mount->block_size);
                if (!block_bitmap) {
                    kfree(indirect_block);
                    return bytes_written;
                }

                if (read_device(0, block_bitmap, mount->block_size) != mount->block_size) {
                    kfree(block_bitmap);
                    kfree(indirect_block);
                    return bytes_written;
                }

                for (uint32_t i = 0; i < mount->block_size * 8; i++) {
                    if (!(block_bitmap[i / 32] & (1 << (i % 32)))) {
                        block_bitmap[i / 32] |= 1 << (i % 32);
                        indirect_block[block_index - 12] = mount->group_descriptors[group].block_bitmap * mount->block_size + i;
                        break;
                    }
                }

                if (write_device(0, block_bitmap, mount->block_size) != mount->block_size) {
                    kfree(block_bitmap);
                    kfree(indirect_block);
                    return bytes_written;
                }

                kfree(block_bitmap);
            }
            block_number = indirect_block[block_index - 12];

            if (write_device(0, indirect_block, mount->block_size) != mount->block_size) {
                kfree(indirect_block);
                return bytes_written;
            }

            kfree(indirect_block);
        } else {
            return bytes_written;
        }

        uint8_t* block_data = (uint8_t*)kmalloc(mount->block_size);
        if (!block_data) return bytes_written;

        if (read_device(0, block_data, mount->block_size) != mount->block_size) {
            kfree(block_data);
            return bytes_written;
        }

        memcpy(block_data + block_offset, (uint8_t*)buffer + bytes_written, bytes_to_write);

        if (write_device(0, block_data, mount->block_size) != mount->block_size) {
            kfree(block_data);
            return bytes_written;
        }

        kfree(block_data);

        bytes_written += bytes_to_write;
        file->position += bytes_to_write;
        if (file->position > file->size) {
            file->size = file->position;
            file->inode_data.size = file->size;
        }
    }

    return bytes_written;
}

directory_t* open_directory(const char* path) {
    if (filesystem.directory_count >= MAX_DIRS) return NULL;

    char mount_path[MAX_PATH];
    char relative_path[MAX_PATH];
    mount_t* mount = NULL;

    for (uint32_t i = 0; i < filesystem.mount_count; i++) {
        if (strncmp(path, filesystem.mounts[i].path, strlen(filesystem.mounts[i].path)) == 0) {
            mount = &filesystem.mounts[i];
            strncpy(mount_path, mount->path, sizeof(mount_path) - 1);
            strncpy(relative_path, path + strlen(mount->path), sizeof(relative_path) - 1);
            break;
        }
    }

    if (!mount || !mount->mounted) return NULL;

    char* component = strtok(relative_path, "/");
    uint32_t current_inode = 2;

    while (component) {
        bool found = false;
        uint32_t group = (current_inode - 1) / mount->inodes_per_group;
        uint32_t index = (current_inode - 1) % mount->inodes_per_group;
        uint32_t block = mount->group_descriptors[group].inode_table + (index * mount->inode_size) / mount->block_size;
        uint32_t offset = (index * mount->inode_size) % mount->block_size;

        uint8_t* block_data = (uint8_t*)kmalloc(mount->block_size);
        if (!block_data) return NULL;

        if (read_device(0, block_data, mount->block_size) != mount->block_size) {
            kfree(block_data);
            return NULL;
        }

        ext4_inode_t* inode = (ext4_inode_t*)(block_data + offset);
        uint32_t block_count = (inode->size + mount->block_size - 1) / mount->block_size;

        for (uint32_t i = 0; i < block_count; i++) {
            uint32_t block_number;
            if (i < 12) {
                block_number = inode->block[i];
            } else if (i < 12 + mount->block_size / 4) {
                uint32_t* indirect_block = (uint32_t*)kmalloc(mount->block_size);
                if (!indirect_block) {
                    kfree(block_data);
                    return NULL;
                }

                if (read_device(0, indirect_block, mount->block_size) != mount->block_size) {
                    kfree(indirect_block);
                    kfree(block_data);
                    return NULL;
                }

                block_number = indirect_block[i - 12];
                kfree(indirect_block);
            } else {
                kfree(block_data);
                return NULL;
            }

            uint8_t* dir_block = (uint8_t*)kmalloc(mount->block_size);
            if (!dir_block) {
                kfree(block_data);
                return NULL;
            }

            if (read_device(0, dir_block, mount->block_size) != mount->block_size) {
                kfree(dir_block);
                kfree(block_data);
                return NULL;
            }

            ext4_dir_entry_t* entry = (ext4_dir_entry_t*)dir_block;
            while ((uint8_t*)entry < dir_block + mount->block_size) {
                if (entry->inode && entry->name_len == strlen(component) &&
                    strncmp(entry->name, component, entry->name_len) == 0) {
                    current_inode = entry->inode;
                    found = true;
                    break;
                }
                entry = (ext4_dir_entry_t*)((uint8_t*)entry + entry->rec_len);
            }

            kfree(dir_block);
            if (found) break;
        }

        kfree(block_data);
        if (!found) return NULL;

        component = strtok(NULL, "/");
    }

    directory_t* dir = &filesystem.directories[filesystem.directory_count++];
    dir->inode = current_inode;
    strncpy(dir->name, path, sizeof(dir->name) - 1);
    dir->files = (file_t*)kmalloc(MAX_FILES * sizeof(file_t));
    dir->file_count = 0;

    uint32_t group = (current_inode - 1) / mount->inodes_per_group;
    uint32_t index = (current_inode - 1) % mount->inodes_per_group;
    uint32_t block = mount->group_descriptors[group].inode_table + (index * mount->inode_size) / mount->block_size;
    uint32_t offset = (index * mount->inode_size) % mount->block_size;

    uint8_t* block_data = (uint8_t*)kmalloc(mount->block_size);
    if (!block_data) return NULL;

    if (read_device(0, block_data, mount->block_size) != mount->block_size) {
        kfree(block_data);
        return NULL;
    }

    ext4_inode_t* inode = (ext4_inode_t*)(block_data + offset);
    uint32_t block_count = (inode->size + mount->block_size - 1) / mount->block_size;

    for (uint32_t i = 0; i < block_count; i++) {
        uint32_t block_number;
        if (i < 12) {
            block_number = inode->block[i];
        } else if (i < 12 + mount->block_size / 4) {
            uint32_t* indirect_block = (uint32_t*)kmalloc(mount->block_size);
            if (!indirect_block) {
                kfree(block_data);
                return NULL;
            }

            if (read_device(0, indirect_block, mount->block_size) != mount->block_size) {
                kfree(indirect_block);
                kfree(block_data);
                return NULL;
            }

            block_number = indirect_block[i - 12];
            kfree(indirect_block);
        } else {
            kfree(block_data);
            return NULL;
        }

        uint8_t* dir_block = (uint8_t*)kmalloc(mount->block_size);
        if (!dir_block) {
            kfree(block_data);
            return NULL;
        }

        if (read_device(0, dir_block, mount->block_size) != mount->block_size) {
            kfree(dir_block);
            kfree(block_data);
            return NULL;
        }

        ext4_dir_entry_t* entry = (ext4_dir_entry_t*)dir_block;
        while ((uint8_t*)entry < dir_block + mount->block_size) {
            if (entry->inode && entry->name_len > 0) {
                file_t* file = &dir->files[dir->file_count++];
                file->inode = entry->inode;
                file->position = 0;
                file->size = 0;
                file->flags = 0;

                uint32_t file_group = (entry->inode - 1) / mount->inodes_per_group;
                uint32_t file_index = (entry->inode - 1) % mount->inodes_per_group;
                uint32_t file_block = mount->group_descriptors[file_group].inode_table + (file_index * mount->inode_size) / mount->block_size;
                uint32_t file_offset = (file_index * mount->inode_size) % mount->block_size;

                uint8_t* file_block_data = (uint8_t*)kmalloc(mount->block_size);
                if (!file_block_data) {
                    kfree(dir_block);
                    kfree(block_data);
                    return NULL;
                }

                if (read_device(0, file_block_data, mount->block_size) != mount->block_size) {
                    kfree(file_block_data);
                    kfree(dir_block);
                    kfree(block_data);
                    return NULL;
                }

                memcpy(&file->inode_data, file_block_data + file_offset, sizeof(ext4_inode_t));
                file->size = file->inode_data.size;

                kfree(file_block_data);
            }
            entry = (ext4_dir_entry_t*)((uint8_t*)entry + entry->rec_len);
        }

        kfree(dir_block);
    }

    kfree(block_data);
    return dir;
}

void close_directory(directory_t* dir) {
    if (!dir) return;
    for (uint32_t i = 0; i < filesystem.directory_count; i++) {
        if (&filesystem.directories[i] == dir) {
            kfree(dir->files);
            filesystem.directories[i] = filesystem.directories[--filesystem.directory_count];
            break;
        }
    }
} 