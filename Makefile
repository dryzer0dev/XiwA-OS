# Compilateur et options
CC = gcc
AS = nasm
LD = ld
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -c
ASFLAGS = -f elf32
LDFLAGS = -T link.ld -melf_i386

# Fichiers source
SRC_DIR = src
KERNEL_DIR = kernel
BOOT_DIR = boot
LIB_DIR = lib

# Fichiers objets
OBJ_DIR = obj
KERNEL_OBJ = $(OBJ_DIR)/kernel.o
BOOT_OBJ = $(OBJ_DIR)/boot.o
LIB_OBJS = $(patsubst $(LIB_DIR)/%.c,$(OBJ_DIR)/%.o,$(wildcard $(LIB_DIR)/*.c))

# Fichier image
IMAGE = ximonos.img

# Règles de compilation
all: $(IMAGE)

$(IMAGE): $(KERNEL_OBJ) $(BOOT_OBJ) $(LIB_OBJS)
	$(LD) $(LDFLAGS) -o $(OBJ_DIR)/kernel.bin $(KERNEL_OBJ) $(LIB_OBJS)
	dd if=/dev/zero of=$@ bs=512 count=2880
	dd if=$(BOOT_OBJ) of=$@ conv=notrunc
	dd if=$(OBJ_DIR)/kernel.bin of=$@ bs=512 seek=1 conv=notrunc

$(KERNEL_OBJ): $(KERNEL_DIR)/kernel.c
	$(CC) $(CFLAGS) -o $@ $<

$(BOOT_OBJ): $(BOOT_DIR)/boot.asm
	$(AS) $(ASFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: $(LIB_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

# Création des répertoires
$(shell mkdir -p $(OBJ_DIR))

# Nettoyage
clean:
	rm -rf $(OBJ_DIR) $(IMAGE)

# Exécution dans QEMU
run: $(IMAGE)
	qemu-system-i386 -drive format=raw,file=$(IMAGE)

.PHONY: all clean run 