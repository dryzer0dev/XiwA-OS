#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

void init_audio() {
    printf("Initialisation du système audio...\n");
    system("sudo modprobe snd-pcm");
    system("sudo modprobe snd-mixer-oss");
    system("sudo modprobe snd-seq-oss");
    printf("Système audio initialisé!\n");
}

void test_audio() {
    printf("Test du système audio...\n");
    system("speaker-test -t wav -c 2");
}

void set_volume(int volume) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "amixer set Master %d%%", volume);
    system(cmd);
    printf("Volume réglé à %d%%\n", volume);
}

void list_audio_devices() {
    printf("Liste des périphériques audio:\n");
    system("aplay -l");
    system("arecord -l");
}

void configure_audio() {
    printf("Configuration du système audio...\n");
    system("alsactl restore");
    printf("Configuration audio restaurée!\n");
} 