# Claude OS

A bare-metal x86 operating system with a graphical desktop environment, built from scratch in C and x86 assembly. Boots directly on hardware via GRUB2.

![Claude OS](https://img.shields.io/badge/arch-x86--32-blue) ![License](https://img.shields.io/badge/license-MIT-green)

## Features

- **Window Manager** - Draggable, resizable windows with minimize, maximize, and close buttons
- **Desktop** - Clickable icons, start menu with app launcher, taskbar with window switching and clock
- **File Explorer** - Navigate the in-memory filesystem, double-click files to open in Notepad
- **Notepad** - Text editor with save support, blinking cursor, scrolling
- **Snake** - Classic snake game with arrow key controls and score tracking
- **Tetris** - Full Tetris with rotation, line clearing, levels, next piece preview, and hard drop
- **Task Manager** - Shows running processes, memory usage bar, and system uptime
- **Extensible App Format** - Add new apps by creating a single .c file with function pointer callbacks

## Architecture

- Multiboot2 boot protocol with GRUB2 bootloader
- VESA framebuffer graphics (1024x768x32bpp) with double buffering
- Custom GDT, IDT with 48 ISR stubs for CPU exceptions and hardware IRQs
- PIC remapping, PIT timer at 100Hz
- PS/2 keyboard driver (scancode set 1, extended keys for arrows)
- PS/2 mouse driver (3-byte packet protocol)
- Free-list heap allocator (kmalloc/kfree)
- In-memory ramdisk filesystem
- Event-driven GUI with widget system (labels, buttons, text inputs, text areas, list views)
- Per-window app state via heap allocation

## Building

### Requirements

- `nasm` - assembler
- `gcc` with 32-bit support (`gcc-multilib`)
- `ld` - GNU linker
- `grub-mkrescue`, `xorriso`, `mtools` - ISO generation

### On Linux / WSL

```bash
./build.sh
```

### On Windows

```
build.bat
```

This uses WSL under the hood.

### Manual build

```bash
make        # Build claudeos.iso
make run    # Build and launch in QEMU
make clean  # Remove build artifacts
```

## Running

### QEMU

```bash
qemu-system-i386 -cdrom claudeos.iso -m 128M
```

### VirtualBox

1. Create a new VM (Type: Other, Version: Other/Unknown)
2. Give it at least 128MB RAM and 32MB+ video memory
3. Attach `claudeos.iso` as a CD/DVD
4. Boot

### Real Hardware

Write `claudeos.iso` to a USB drive with [Rufus](https://rufus.ie/) or [Etcher](https://etcher.balena.io/), then boot from USB.

## Creating a New App

Each app is a single .c file with a private state struct and function pointer callbacks (C's version of polymorphism).

1. Create `src/myapp.c`
2. Define your state struct
3. Implement `on_start`, `on_draw`, `on_event` (and optionally `on_tick`, `on_close`)
4. Write a register function that fills an `AppDef` and calls `app_register()`
5. Declare the register function in `src/kernel.h`
6. Call it from `kernel_main()` in `src/kernel.c`
7. Add the .c file to `C_SRC` in the `Makefile`

Example skeleton:

```c
#include "kernel.h"

typedef struct {
    int my_data;
} MyAppState;

static void myapp_start(Window* win) {
    win->user_data = kmalloc(sizeof(MyAppState));
    k_memset(win->user_data, 0, sizeof(MyAppState));
}

static void myapp_draw(Window* win, int x, int y, int w, int h) {
    MyAppState* st = (MyAppState*)win->user_data;
    fb_fill(x, y, w, h, COLOR_WINDOW_BG);
    fb_text_t(x + 8, y + 8, "Hello from my app!", COLOR_TEXT);
}

static void myapp_event(Window* win, Event* ev) {
    MyAppState* st = (MyAppState*)win->user_data;
}

void myapp_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "myapp");
    k_strcpy(def.name, "My App");
    def.icon_color = 0xFF5722;
    def.def_w = 400;
    def.def_h = 300;
    def.on_start = myapp_start;
    def.on_draw = myapp_draw;
    def.on_event = myapp_event;
    app_register(&def);
}
```

## Project Structure

```
claudeos/
  src/
    boot.asm         - Multiboot2 header, GDT, ISR stubs
    kernel.c          - IDT, PIC, timer, event loop, main
    kernel.h          - All types, structs, function declarations
    types.h           - Basic types, port I/O
    lib.c             - String functions, heap allocator
    drivers.c         - Keyboard, mouse, framebuffer, bitmap font
    gui.c             - Window manager, widget system, compositor
    desktop.c         - Desktop icons, taskbar, start menu, cursor
    apps.c            - App registry, in-memory filesystem
    file_explorer.c   - File browser app
    notepad.c         - Text editor app
    snake.c           - Snake game
    tetris.c          - Tetris game
    taskmgr.c         - Task manager
  iso/boot/grub/
    grub.cfg          - GRUB bootloader config
  linker.ld           - Linker script (kernel at 1MB)
  Makefile            - Build system
  build.sh            - Linux/WSL build script
  build.bat           - Windows build wrapper
```

## License

MIT
