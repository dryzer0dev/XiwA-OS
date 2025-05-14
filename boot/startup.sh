#!/bin/bash

# Configuration du système
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export HOME="/root"
export DISPLAY=":0"

# Fonction pour afficher le logo de démarrage
show_splash() {
    clear
    echo "
    ██╗  ██╗██╗██╗    ██╗ █████╗     ██████╗ ███████╗
    ██║  ██║██║██║    ██║██╔══██╗    ██╔══██╗██╔════╝
    ███████║██║██║    ██║███████║    ██║  ██║███████╗
    ██╔══██║██║██║    ██║██╔══██║    ██║  ██║╚════██║
    ██║  ██║██║██║    ██║██║  ██║    ██████╔╝███████║
    ╚═╝  ╚═╝╚═╝╚═╝    ╚═╝╚═╝  ╚═╝    ╚═════╝ ╚══════╝
    "
    sleep 2
}

# Fonction pour vérifier les dépendances
check_dependencies() {
    echo "Vérification des dépendances..."
    
    # Vérifier les packages essentiels
    for pkg in build-essential gcc make git libgtk-3-dev libglib2.0-dev pkg-config; do
        if ! dpkg -l | grep -q "^ii  $pkg "; then
            echo "Installation de $pkg..."
            apt-get install -y $pkg
        fi
    done
    
    echo "Dépendances vérifiées!"
}

# Fonction pour compiler les composants
compile_components() {
    echo "Compilation des composants..."
    
    # Compiler l'interface graphique
    cd /usr/local/src/xiwa/gui
    gcc -o window_manager window_manager.c $(pkg-config --cflags --libs gtk+-3.0)
    
    # Compiler le système d'initialisation
    cd /usr/local/src/xiwa/boot
    gcc -o init init.c
    
    echo "Compilation terminée!"
}

# Fonction pour configurer le système
setup_system() {
    echo "Configuration du système..."
    
    # Créer les répertoires nécessaires
    mkdir -p /usr/local/bin
    mkdir -p /usr/local/share/xiwa
    mkdir -p /var/log/xiwa
    
    # Copier les fichiers
    cp /usr/local/src/xiwa/gui/window_manager /usr/local/bin/
    cp /usr/local/src/xiwa/boot/init /usr/local/bin/
    cp /usr/local/src/xiwa/gui/style.css /usr/local/share/xiwa/
    
    # Configurer les permissions
    chmod +x /usr/local/bin/window_manager
    chmod +x /usr/local/bin/init
    
    echo "Configuration terminée!"
}

# Fonction principale
main() {
    # Afficher le splash screen
    show_splash
    
    # Vérifier les dépendances
    check_dependencies
    
    # Compiler les composants
    compile_components
    
    # Configurer le système
    setup_system
    
    # Démarrer le système
    echo "Démarrage de XiwA OS..."
    /usr/local/bin/init
}

# Exécuter la fonction principale
main 