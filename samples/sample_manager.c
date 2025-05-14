#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

void create_sample_directory() {
    printf("Création du répertoire samples...\n");
    system("mkdir -p ~/samples");
    system("mkdir -p ~/samples/drums");
    system("mkdir -p ~/samples/synths");
    system("mkdir -p ~/samples/effects");
    printf("Répertoire samples créé!\n");
}

void download_sample_pack(const char *url, const char *category) {
    char cmd[512];
    printf("Téléchargement du pack de samples...\n");
    snprintf(cmd, sizeof(cmd), "wget -P ~/samples/%s %s", category, url);
    system(cmd);
    printf("Pack téléchargé dans ~/samples/%s\n", category);
}

void extract_sample_pack(const char *filename, const char *category) {
    char cmd[512];
    printf("Extraction du pack de samples...\n");
    
    if (strstr(filename, ".zip")) {
        snprintf(cmd, sizeof(cmd), "unzip ~/samples/%s/%s -d ~/samples/%s/", category, filename, category);
    } else if (strstr(filename, ".tar.gz")) {
        snprintf(cmd, sizeof(cmd), "tar xzf ~/samples/%s/%s -C ~/samples/%s/", category, filename, category);
    }
    
    system(cmd);
    printf("Pack extrait!\n");
}

void list_samples(const char *category) {
    printf("Liste des samples dans %s:\n", category);
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "ls -l ~/samples/%s", category);
    system(cmd);
}

void convert_sample_format(const char *input_file, const char *output_format) {
    char cmd[512];
    printf("Conversion du sample...\n");
    
    if (strcmp(output_format, "wav") == 0) {
        snprintf(cmd, sizeof(cmd), "ffmpeg -i %s %s.wav", input_file, input_file);
    } else if (strcmp(output_format, "mp3") == 0) {
        snprintf(cmd, sizeof(cmd), "ffmpeg -i %s %s.mp3", input_file, input_file);
    }
    
    system(cmd);
    printf("Sample converti!\n");
} 