#!/bin/bash

echo "Creation de l'ISO XiwA OS..."

# Créer la structure de l'ISO
mkdir -p iso/{boot,usr/local/bin,usr/local/share/xiwa}

# Copier les fichiers nécessaires
cp boot/init iso/usr/local/bin/
cp gui/window_manager iso/usr/local/bin/
cp gui/style.css iso/usr/local/share/xiwa/
cp boot/xiwa.service iso/usr/local/bin/

# Créer le fichier de démarrage
cat > iso/boot/grub/grub.cfg << EOF
set timeout=5
set default=0

menuentry "XiwA OS" {
    linux /boot/vmlinuz root=/dev/sda1 quiet splash
    initrd /boot/initrd.img
}

menuentry "XiwA OS (Mode sans échec)" {
    linux /boot/vmlinuz root=/dev/sda1 single
    initrd /boot/initrd.img
}
EOF

# Créer l'ISO
grub-mkrescue -o xiwa.iso iso/

echo "ISO cree avec succes: xiwa.iso" 