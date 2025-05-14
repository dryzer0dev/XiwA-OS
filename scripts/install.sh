#!/bin/bash

echo "Installation des dépendances système..."
sudo apt-get update
sudo apt-get install -y build-essential gcc make git

echo "Installation des outils de sécurité..."
sudo apt-get install -y hydra john hashcat dirb testdisk photorec theharvester shodan
sudo apt-get install -y snort iptables

echo "Installation des outils audio..."
sudo apt-get install -y alsa-utils mpg123 vorbis-tools ffmpeg
sudo apt-get install -y libasound2-dev

echo "Compilation des composants..."
gcc -o security/firewall security/firewall.c
gcc -o security/ids security/ids.c
gcc -o security/sandbox security/sandbox.c
gcc -o sound/driver sound/driver.c -lasound
gcc -o sound/player sound/player.c
gcc -o samples/sample_manager samples/sample_manager.c
gcc -o scripts/system_scripts scripts/system_scripts.c

echo "Création des répertoires nécessaires..."
mkdir -p ~/samples/{drums,synths,effects}
mkdir -p /backup

echo "Configuration des permissions..."
chmod +x security/*
chmod +x sound/*
chmod +x samples/*
chmod +x scripts/*

echo "Installation terminée!" 