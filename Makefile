CC = gcc
AS = nasm
LD = ld

CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Isrc -nostdlib -fno-exceptions -fno-stack-protector -fno-pic -fno-pie
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

ASM_SRC = src/boot.asm
C_SRC = src/kernel.c src/lib.c src/drivers.c src/gui.c src/desktop.c src/apps.c \
        src/file_explorer.c src/notepad.c src/snake.c src/tetris.c src/taskmgr.c \
        src/calculator.c src/paint.c src/minesweeper.c src/game2048.c \
        src/settings.c src/about.c src/terminal.c

ASM_OBJ = $(ASM_SRC:.asm=.o)
C_OBJ = $(C_SRC:.c=.o)
OBJ = $(ASM_OBJ) $(C_OBJ)

KERNEL = iso/boot/claudeos.bin
ISO = claudeos.iso

all: $(ISO)

$(ISO): $(KERNEL)
	grub-mkrescue -o $(ISO) iso/

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

src/boot.o: src/boot.asm
	$(AS) -f elf32 $< -o $@

src/%.o: src/%.c src/kernel.h src/types.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(KERNEL) $(ISO)

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -m 128M

.PHONY: all clean run
