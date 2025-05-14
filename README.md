# XiMonOS

XiMonOS est un système d'exploitation éducatif développé en C et assembleur.

## Prérequis

- GCC (version 4.8 ou supérieure)
- NASM (version 2.10 ou supérieure)
- Make
- QEMU (pour l'émulation)

## Structure du projet

```
ximonos/
├── boot/           # Code du bootloader
├── kernel/         # Code du kernel
├── lib/            # Bibliothèques système
├── obj/            # Fichiers objets générés
├── Makefile        # Configuration de compilation
├── link.ld         # Script de liaison
└── README.md       # Documentation
```

## Compilation

Pour compiler le projet :

```bash
make
```

Pour nettoyer les fichiers générés :

```bash
make clean
```

## Exécution

Pour exécuter le système dans QEMU :

```bash
make run
```

## Fonctionnalités

- Gestion de la mémoire
- Gestion des processus
- Système de fichiers
- Interface graphique
- Gestion des périphériques
- Support réseau
- Gestion audio
- Gestion des entrées (clavier, souris)

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