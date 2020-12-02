ARCH	= arch/x86_64

AS	 	= nasm
CC 		= x86_64-pc-linux-gnu-gcc
LD 		= x86_64-elf-ld
GRUB 	= grub-mkrescue

SRC 		= ./src
OUT 		= ./dist
CONFIG		= ./config
ISO_DIR		= $(OUT)/release/x86_64/iso
TARGET		= $(OUT)/release/x86_64/kernel
TARGET_ISO 	= $(TARGET).iso

AS_FLAGS	= -f elf64
CC_FLAGS 	= -Wall -ffreestanding -m64
LD_FLAGS	= -n -m elf_x86_64 -T $(CONFIG)/x86_64/linker.ld -nostdlib


# config target objects
ARCH_ASM_OBJ	:= $(patsubst $(SRC)/$(ARCH)/%.asm, $(OUT)/$(ARCH)/%.o, $(shell find $(SRC)/$(ARCH) -name *.asm))
ARCH_C_OBJ		:= $(patsubst $(SRC)/$(ARCH)/%.c, $(OUT)/$(ARCH)/%.o, $(shell find $(SRC)/$(ARCH) -name *.c))
ARCH_OBJ		:= $(ARCH_ASM_OBJ) $(ARCH_C_OBJ)

KERNEL_OBJ		:= $(patsubst $(SRC)/kernel/%.c, $(OUT)/kernel/%.o, $(shell find $(SRC)/kernel -name *.c))

OBJS = $(ARCH_OBJ) $(KERNEL_OBJ)

$(ARCH_ASM_OBJ) : $(OUT)/$(ARCH)/%.o : $(SRC)/$(ARCH)/%.asm
	mkdir -p $(dir $@)
	$(AS) $(AS_FLAGS) $^ -o $@
$(ARCH_C_OBJ) : $(OUT)/$(ARCH)/%.o : $(SRC)/$(ARCH)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) -c -I $(SRC) $^ -o $@
$(KERNEL_OBJ) : $(OUT)/kernel/%.o : $(SRC)/kernel/%.c
	mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) -c -I $(SRC) $^ -o $@


.PHONY: x86_64 x86_64.iso all run clean

x86_64: $(OBJS)
	mkdir -p $(dir $(TARGET))
	$(LD) $(LD_FLAGS) -o $(TARGET) $^
	grub-file --is-x86-multiboot2 $(TARGET)

x86_64.iso: x86_64
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(CONFIG)/x86_64/grub.cfg $(ISO_DIR)/boot/grub/
	mv $(TARGET) $(ISO_DIR)/boot/
	$(GRUB) -o $(TARGET_ISO) $(ISO_DIR)
	sync

all: x86_64.iso

run: clean all
	qemu-system-x86_64 ./dist/release/x86_64/kernel.iso

clean:
	-rm -r $(OUT)