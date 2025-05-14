# XiMonOS

XiMonOS est un système d'exploitation moderne et minimaliste, développé à des fins éducatives.

## Fonctionnalités

- Noyau 32 bits
- Gestion de la mémoire virtuelle
- Système de fichiers simple
- Interface graphique basique
- Gestion des processus
- Support du clavier et de la souris

## Prérequis

Pour compiler XiMonOS, vous aurez besoin de :

- GCC (version 4.8 ou supérieure)
- NASM (version 2.10 ou supérieure)
- Make
- QEMU (pour tester)

Sur Ubuntu/Debian :
```bash
sudo apt-get install gcc nasm make qemu-system-x86
```

Sur Windows :
- Installez MinGW-w64 pour GCC
- Installez NASM
- Installez Make
- Installez QEMU

## Compilation

1. Clonez le dépôt :
```bash
git clone https://github.com/votre-username/XiMonOS.git
cd XiMonOS
```

2. Compilez le projet :
```bash
cd build
make
```

3. Exécutez dans QEMU :
```bash
make run
```

## Structure du projet

```
XiMonOS/
├── boot/           # Code du bootloader
├── kernel/         # Code du noyau
│   ├── drivers/    # Pilotes matériels
│   ├── mm/         # Gestion de la mémoire
│   └── fs/         # Système de fichiers
├── gui/            # Interface graphique
│   └── assets/     # Ressources graphiques
├── apps/           # Applications
└── build/          # Fichiers de compilation
```

## Développement

Pour ajouter de nouvelles fonctionnalités :

1. Ajoutez vos fichiers source dans les dossiers appropriés
2. Mettez à jour le Makefile si nécessaire
3. Recompilez avec `make`

## Licence

Ce projet est sous licence MIT. Voir le fichier LICENSE pour plus de détails.

## Contribution

Les contributions sont les bienvenues ! N'hésitez pas à :

1. Fork le projet
2. Créer une branche pour votre fonctionnalité
3. Commiter vos changements
4. Pousser vers la branche
5. Ouvrir une Pull Request

## Contact

Pour toute question ou suggestion, n'hésitez pas à ouvrir une issue sur GitHub. 