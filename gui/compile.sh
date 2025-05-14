#!/bin/bash

echo "Installation des dépendances..."
sudo apt-get update
sudo apt-get install -y build-essential
sudo apt-get install -y libgtk-3-dev
sudo apt-get install -y libglib2.0-dev
sudo apt-get install -y pkg-config

echo "Compilation de l'interface graphique..."
gcc -o window_manager window_manager.c $(pkg-config --cflags --libs gtk+-3.0)

echo "Installation des icônes..."
sudo mkdir -p /usr/share/icons/hicolor/scalable/apps
sudo cp icons/* /usr/share/icons/hicolor/scalable/apps/

echo "Installation du thème..."
sudo mkdir -p /usr/share/themes/xiwa
sudo cp style.css /usr/share/themes/xiwa/

echo "Création du lanceur..."
echo "[Desktop Entry]
Name=XiwA OS
Comment=Interface graphique XiwA OS
Exec=/usr/local/bin/window_manager
Icon=xiwa-logo
Terminal=false
Type=Application
Categories=System;" > xiwa.desktop

sudo mv xiwa.desktop /usr/share/applications/
sudo cp window_manager /usr/local/bin/

echo "Configuration des permissions..."
sudo chmod +x /usr/local/bin/window_manager
sudo chmod +x /usr/share/applications/xiwa.desktop

echo "Installation terminée!" 