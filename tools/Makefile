# Compilateur et options
CC = gcc
AS = nasm
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -c
ASFLAGS = -f elf32
LDFLAGS = -T link.ld -melf_i386

# Dossiers
SRC_DIR = ..
BOOT_DIR = $(SRC_DIR)/boot
KERNEL_DIR = $(SRC_DIR)/kernel
GUI_DIR = $(SRC_DIR)/gui
APPS_DIR = $(SRC_DIR)/apps

# Fichiers sources
BOOT_SRC = $(BOOT_DIR)/bootloader.asm
KERNEL_SRC = $(KERNEL_DIR)/kernel.c \
             $(KERNEL_DIR)/mm/memory.c \
             $(KERNEL_DIR)/process.c \
             $(KERNEL_DIR)/fs/filesystem.c
GUI_SRC = $(GUI_DIR)/window_manager.cpp

# Fichiers objets
BOOT_OBJ = $(BOOT_DIR)/bootloader.o
KERNEL_OBJ = $(KERNEL_DIR)/kernel.o \
             $(KERNEL_DIR)/mm/memory.o \
             $(KERNEL_DIR)/process.o \
             $(KERNEL_DIR)/fs/filesystem.o
GUI_OBJ = $(GUI_DIR)/window_manager.o

# Cible principale
all: XiMonOS.img

# Compilation du bootloader
$(BOOT_OBJ): $(BOOT_SRC)
	$(AS) $(ASFLAGS) $< -o $@

# Compilation du noyau
$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

# Compilation de l'interface graphique
$(GUI_DIR)/%.o: $(GUI_DIR)/%.cpp
	$(CC) $(CFLAGS) $< -o $@

# Création de l'image disque
XiMonOS.img: $(BOOT_OBJ) $(KERNEL_OBJ) $(GUI_OBJ)
	$(CC) $(LDFLAGS) $(KERNEL_OBJ) $(GUI_OBJ) -o kernel.bin
	dd if=/dev/zero of=$@ bs=512 count=2880
	dd if=$(BOOT_OBJ) of=$@ conv=notrunc
	dd if=kernel.bin of=$@ bs=512 seek=1 conv=notrunc

# Nettoyage
clean:
	rm -f $(BOOT_OBJ) $(KERNEL_OBJ) $(GUI_OBJ) kernel.bin XiMonOS.img

# Exécution dans QEMU
run: XiMonOS.img
	qemu-system-i386 -drive format=raw,file=$<

.PHONY: all clean run 