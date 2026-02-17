#include "kernel.h"

static void about_draw(Window* win, int x, int y, int w, int h) {
    (void)win;
    fb_fill(x, y, w, h, 0x1A1A2E);

    int cx = x + w / 2;

    fb_fill(cx - 60, y + 20, 120, 80, 0x0078D7);
    fb_fill(cx - 50, y + 30, 100, 60, 0x0078D7);
    fb_text_t(cx - 28, y + 50, "COS", COLOR_WHITE);

    fb_text_t(cx - 52, y + 115, "Claude OS", COLOR_WHITE);
    fb_text_t(cx - 36, y + 138, "v1.0.0", 0x888888);

    int ly = y + 170;
    int lx = x + 20;
    int rw = w - 40;

    fb_fill(lx, ly, rw, 1, 0x333333);
    ly += 12;

    fb_text_t(lx, ly, "Architecture:", 0x888888);
    fb_text_t(lx + 130, ly, "x86-32 (i386)", COLOR_WHITE);
    ly += 22;

    fb_text_t(lx, ly, "Boot:", 0x888888);
    fb_text_t(lx + 130, ly, "Multiboot2 / GRUB2", COLOR_WHITE);
    ly += 22;

    fb_text_t(lx, ly, "Display:", 0x888888);
    fb_text_t(lx + 130, ly, "1024x768x32bpp VESA", COLOR_WHITE);
    ly += 22;

    fb_text_t(lx, ly, "Timer:", 0x888888);
    fb_text_t(lx + 130, ly, "PIT @ 100Hz", COLOR_WHITE);
    ly += 22;

    fb_text_t(lx, ly, "Input:", 0x888888);
    fb_text_t(lx + 130, ly, "PS/2 Keyboard + Mouse", COLOR_WHITE);
    ly += 22;

    fb_text_t(lx, ly, "Memory:", 0x888888);
    uint32_t total, used, free_mem;
    mem_stats(&total, &used, &free_mem);
    char buf[64], num[16];
    k_itoa(total / 1024, num, 10);
    k_strcpy(buf, num);
    k_strcat(buf, " KB heap");
    fb_text_t(lx + 130, ly, buf, COLOR_WHITE);
    ly += 22;

    fb_text_t(lx, ly, "Uptime:", 0x888888);
    uint32_t t = timer_ticks() / 100;
    k_itoa(t / 3600, num, 10);
    k_strcpy(buf, num);
    k_strcat(buf, "h ");
    k_itoa((t / 60) % 60, num, 10);
    k_strcat(buf, num);
    k_strcat(buf, "m ");
    k_itoa(t % 60, num, 10);
    k_strcat(buf, num);
    k_strcat(buf, "s");
    fb_text_t(lx + 130, ly, buf, COLOR_WHITE);
    ly += 30;

    fb_fill(lx, ly, rw, 1, 0x333333);
    ly += 12;

    fb_text_t(lx, ly, "Built from scratch in C and x86 assembly.", 0x666666);
    ly += 18;
    fb_text_t(lx, ly, "No libc. No stdlib. Just bare metal.", 0x666666);
}

static void about_event(Window* win, Event* ev) {
    (void)win; (void)ev;
}

void about_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "about");
    k_strcpy(def.name, "About");
    def.icon_color = 0x5C6BC0;
    def.def_w = 380;
    def.def_h = 440;
    def.on_draw = about_draw;
    def.on_event = about_event;
    app_register(&def);
}
