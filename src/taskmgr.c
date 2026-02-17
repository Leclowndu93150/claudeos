#include "kernel.h"

static void taskmgr_draw(Window* win, int x, int y, int w, int h) {
    (void)win;
    fb_fill(x, y, w, h, 0x1E1E1E);

    int wc = 0;
    Window** wins = gui_get_windows(&wc);
    char buf[128], num[16];

    fb_fill(x, y, w, 24, 0x333333);
    fb_text_t(x + 8, y + 4, "Task Manager", 0x00BFFF);

    int ly = y + 32;
    fb_text_t(x + 8, ly, "Name", 0xAAAAAA);
    fb_text_t(x + 200, ly, "Status", 0xAAAAAA);
    fb_fill(x + 4, ly + 18, w - 8, 1, 0x444444);
    ly += 24;

    for (int i = 0; i < wc && ly < y + h - 100; i++) {
        bool is_self = (wins[i] == win);
        uint32_t color = is_self ? 0x00BFFF : COLOR_WHITE;

        k_strncpy(buf, wins[i]->title, 24);
        buf[24] = 0;
        fb_text_t(x + 8, ly, buf, color);

        if (wins[i]->minimized) fb_text_t(x + 200, ly, "Minimized", 0xFFAA00);
        else if (wins[i]->active) fb_text_t(x + 200, ly, "Active", 0x4CAF50);
        else fb_text_t(x + 200, ly, "Running", 0xAAAAAA);

        ly += 20;
    }

    ly = y + h - 90;
    fb_fill(x + 4, ly, w - 8, 1, 0x444444);
    ly += 8;

    k_strcpy(buf, "Processes: ");
    k_itoa(wc, num, 10);
    k_strcat(buf, num);
    fb_text_t(x + 8, ly, buf, COLOR_WHITE);
    ly += 20;

    uint32_t total, used, free_mem;
    mem_stats(&total, &used, &free_mem);

    k_strcpy(buf, "Memory: ");
    k_itoa(used / 1024, num, 10);
    k_strcat(buf, num);
    k_strcat(buf, " KB / ");
    k_itoa(total / 1024, num, 10);
    k_strcat(buf, num);
    k_strcat(buf, " KB");
    fb_text_t(x + 8, ly, buf, COLOR_WHITE);
    ly += 20;

    int bar_w = w - 16;
    int fill_w = total ? (int)(used / (total / bar_w)) : 0;
    if (fill_w > bar_w) fill_w = bar_w;
    fb_fill(x + 8, ly, bar_w, 12, 0x333333);
    int pct = total ? (used / (total / 100)) : 0;
    uint32_t bar_color = (pct > 80) ? 0xF44336 : 0x4CAF50;
    fb_fill(x + 8, ly, fill_w, 12, bar_color);
    ly += 18;

    uint32_t t = timer_ticks() / 100;
    int secs = t % 60;
    int mins = (t / 60) % 60;
    int hrs = t / 3600;
    k_strcpy(buf, "Uptime: ");
    k_itoa(hrs, num, 10);
    k_strcat(buf, num);
    k_strcat(buf, "h ");
    k_itoa(mins, num, 10);
    k_strcat(buf, num);
    k_strcat(buf, "m ");
    k_itoa(secs, num, 10);
    k_strcat(buf, num);
    k_strcat(buf, "s");
    fb_text_t(x + 8, ly, buf, COLOR_WHITE);
}

static void taskmgr_event(Window* win, Event* ev) {
    (void)win; (void)ev;
}

void taskmgr_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "taskmgr");
    k_strcpy(def.name, "Task Manager");
    def.icon_color = 0x4CAF50;
    def.def_w = 320;
    def.def_h = 400;
    def.on_draw = taskmgr_draw;
    def.on_event = taskmgr_event;
    app_register(&def);
}
