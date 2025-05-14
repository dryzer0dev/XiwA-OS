#include <kernel/apps/package_manager.h>
#include <kernel/fs/fs.h>
#include <kernel/memory/memory.h>
#include <kernel/system/system.h>
#include <libc/string.h>

#define MAX_PACKAGES 100
#define MAX_NAME_LENGTH 50
#define MAX_VERSION_LENGTH 20

typedef struct {
    char name[MAX_NAME_LENGTH];
    char version[MAX_VERSION_LENGTH];
    bool installed;
} Package;

static Package packages[MAX_PACKAGES];
static int package_count = 0;

int init_package_manager() {
    package_count = 0;
    return 0;
}

int install_package(const char* name, const char* version) {
    if (package_count >= MAX_PACKAGES) {
        return -1; 
    }

    for (int i = 0; i < package_count; i++) {
        if (strcmp(packages[i].name, name) == 0) {
            if (packages[i].installed) {
                return -2; 
            }
            packages[i].installed = true;
            return 0;
        }
    }

    strncpy(packages[package_count].name, name, MAX_NAME_LENGTH - 1);
    strncpy(packages[package_count].version, version, MAX_VERSION_LENGTH - 1);
    packages[package_count].installed = true;
    package_count++;

    return 0;
}

int uninstall_package(const char* name) {
    for (int i = 0; i < package_count; i++) {
        if (strcmp(packages[i].name, name) == 0) {
            if (!packages[i].installed) {
                return -1; 
            }
            packages[i].installed = false;
            return 0;
        }
    }
    return -2; 
}

int list_packages(Package** package_list, int* count) {
    *package_list = packages;
    *count = package_count;
    return 0;
}

bool is_package_installed(const char* name) {
    for (int i = 0; i < package_count; i++) {
        if (strcmp(packages[i].name, name) == 0) {
            return packages[i].installed;
        }
    }
    return false;
}

int update_package(const char* name, const char* new_version) {
    for (int i = 0; i < package_count; i++) {
        if (strcmp(packages[i].name, name) == 0) {
            if (!packages[i].installed) {
                return -1; 
            }
            strncpy(packages[i].version, new_version, MAX_VERSION_LENGTH - 1);
            return 0;
        }
    }
    return -2; 
}