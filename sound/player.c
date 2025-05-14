#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void play_audio_file(const char *filename) {
    char cmd[512];
    printf("Lecture de %s...\n", filename);
    
    // Détection du type de fichier
    if (strstr(filename, ".mp3")) {
        snprintf(cmd, sizeof(cmd), "mpg123 %s", filename);
    } else if (strstr(filename, ".wav")) {
        snprintf(cmd, sizeof(cmd), "aplay %s", filename);
    } else if (strstr(filename, ".ogg")) {
        snprintf(cmd, sizeof(cmd), "ogg123 %s", filename);
    } else {
        printf("Format non supporté!\n");
        return;
    }
    
    system(cmd);
}

void install_audio_players() {
    printf("Installation des lecteurs audio...\n");
    system("sudo apt-get install -y mpg123 vorbis-tools");
    printf("Lecteurs audio installés!\n");
}

void list_playlist(const char *directory) {
    printf("Liste des fichiers audio dans %s:\n", directory);
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "find %s -type f \\( -name \"*.mp3\" -o -name \"*.wav\" -o -name \"*.ogg\" \\)", directory);
    system(cmd);
}

void create_playlist(const char *directory, const char *playlist_file) {
    printf("Création de la playlist...\n");
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "find %s -type f \\( -name \"*.mp3\" -o -name \"*.wav\" -o -name \"*.ogg\" \\) > %s", 
             directory, playlist_file);
    system(cmd);
    printf("Playlist créée dans %s\n", playlist_file);
} 