#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_FILES 1000
#define MAX_FILENAME 256
#define MAX_PATH 1024

typedef struct {
    char name[MAX_FILENAME];
    char path[MAX_PATH];
    size_t size;
    time_t created;
    time_t modified;
    int is_directory;
    int is_hidden;
    int is_system;
    char permissions[10];
    char owner[32];
    char group[32];
} FileEntry;

typedef struct {
    FileEntry entries[MAX_FILES];
    int count;
    char mount_point[MAX_PATH];
    char fs_type[10]; // "NTFS" ou "ext4"
} FileSystem;

// Initialisation du système de fichiers
FileSystem* init_filesystem(const char* mount_point, const char* fs_type) {
    FileSystem* fs = (FileSystem*)malloc(sizeof(FileSystem));
    if (!fs) return NULL;

    strncpy(fs->mount_point, mount_point, MAX_PATH);
    strncpy(fs->fs_type, fs_type, 10);
    fs->count = 0;

    // Créer les dossiers système
    create_directory(fs, "/home");
    create_directory(fs, "/etc");
    create_directory(fs, "/var");
    create_directory(fs, "/usr");
    create_directory(fs, "/bin");
    create_directory(fs, "/sbin");
    create_directory(fs, "/opt");
    create_directory(fs, "/tmp");

    return fs;
}

// Créer un fichier
int create_file(FileSystem* fs, const char* path, const char* name) {
    if (fs->count >= MAX_FILES) return -1;

    FileEntry* entry = &fs->entries[fs->count];
    strncpy(entry->name, name, MAX_FILENAME);
    strncpy(entry->path, path, MAX_PATH);
    entry->size = 0;
    entry->created = time(NULL);
    entry->modified = entry->created;
    entry->is_directory = 0;
    entry->is_hidden = 0;
    entry->is_system = 0;
    strcpy(entry->permissions, "rw-r--r--");
    strcpy(entry->owner, "root");
    strcpy(entry->group, "root");

    fs->count++;
    return 0;
}

// Créer un dossier
int create_directory(FileSystem* fs, const char* path) {
    if (fs->count >= MAX_FILES) return -1;

    FileEntry* entry = &fs->entries[fs->count];
    strncpy(entry->name, basename(path), MAX_FILENAME);
    strncpy(entry->path, dirname(path), MAX_PATH);
    entry->size = 0;
    entry->created = time(NULL);
    entry->modified = entry->created;
    entry->is_directory = 1;
    entry->is_hidden = 0;
    entry->is_system = 0;
    strcpy(entry->permissions, "rwxr-xr-x");
    strcpy(entry->owner, "root");
    strcpy(entry->group, "root");

    fs->count++;
    return 0;
}

// Lister les fichiers d'un dossier
void list_directory(FileSystem* fs, const char* path) {
    printf("\nContenu de %s:\n", path);
    printf("Permissions\tTaille\tPropriétaire\tGroupe\t\tModifié\t\tNom\n");
    printf("----------------------------------------------------------------------\n");

    for (int i = 0; i < fs->count; i++) {
        if (strcmp(fs->entries[i].path, path) == 0) {
            printf("%s\t", fs->entries[i].permissions);
            printf("%zu\t", fs->entries[i].size);
            printf("%s\t\t", fs->entries[i].owner);
            printf("%s\t\t", fs->entries[i].group);
            
            char time_str[20];
            strftime(time_str, 20, "%Y-%m-%d %H:%M", localtime(&fs->entries[i].modified));
            printf("%s\t", time_str);
            
            if (fs->entries[i].is_directory) {
                printf("\033[1;34m%s/\033[0m\n", fs->entries[i].name); // Bleu pour les dossiers
            } else {
                printf("%s\n", fs->entries[i].name);
            }
        }
    }
}

// Fonction utilitaire pour obtenir le nom du fichier depuis le chemin
const char* basename(const char* path) {
    const char* last_slash = strrchr(path, '/');
    return last_slash ? last_slash + 1 : path;
}

// Fonction utilitaire pour obtenir le chemin du dossier
const char* dirname(const char* path) {
    static char dir[MAX_PATH];
    strncpy(dir, path, MAX_PATH);
    char* last_slash = strrchr(dir, '/');
    if (last_slash) *last_slash = '\0';
    return dir;
} 