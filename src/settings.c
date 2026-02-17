#include "kernel.h"

#define NUM_COLORS 12
#define NUM_WALLPAPERS 3

static const uint32_t bg_colors[NUM_COLORS] = {
    0x004E98, 0x0078D7, 0x1B5E20, 0x1A237E,
    0x4A148C, 0x880E4F, 0x3E2723, 0x263238,
    0x212121, 0x0D47A1, 0xBF360C, 0x006064,
};

static const char* bg_names[NUM_COLORS] = {
    "Blue", "Light Blue", "Green", "Navy",
    "Purple", "Pink", "Brown", "Slate",
    "Dark", "Royal", "Deep Orange", "Teal",
};

static const char* wp_names[NUM_WALLPAPERS] = {
    "Solid", "Gradient", "Grid",
};

typedef struct {
    int color_hover;
    int wp_hover;
} SettingsState;

static SettingsState* get_state(Window* win) {
    return (SettingsState*)win->user_data;
}

static void settings_start(Window* win) {
    win->user_data = kmalloc(sizeof(SettingsState));
    if (!win->user_data) return;
    k_memset(win->user_data, 0, sizeof(SettingsState));
    SettingsState* st = get_state(win);
    st->color_hover = -1;
    st->wp_hover = -1;
}

static void settings_draw(Window* win, int x, int y, int w, int h) {
    SettingsState* st = get_state(win);
    if (!st) return;

    fb_fill(x, y, w, h, 0xF3F3F3);

    fb_text_t(x + 12, y + 8, "Desktop Background", 0x333333);
    fb_fill(x + 12, y + 28, w - 24, 1, 0xCCCCCC);

    int sw = 48, sh = 36, pad = 8;
    int cols = (w - 24) / (sw + pad);
    if (cols < 1) cols = 1;

    uint32_t cur_color = desktop_get_color();

    for (int i = 0; i < NUM_COLORS; i++) {
        int col = i % cols, row = i / cols;
        int cx = x + 12 + col * (sw + pad);
        int cy = y + 40 + row * (sh + pad + 16);
        fb_fill(cx, cy, sw, sh, bg_colors[i]);
        if (bg_colors[i] == cur_color)
            fb_rect(cx - 2, cy - 2, sw + 4, sh + 4, 0xFF0000);
        else if (i == st->color_hover)
            fb_rect(cx - 1, cy - 1, sw + 2, sh + 2, 0x666666);
        fb_text_t(cx, cy + sh + 2, bg_names[i], COLOR_TEXT_DIM);
    }

    int wy = y + 40 + ((NUM_COLORS + cols - 1) / cols) * (sh + pad + 16) + 10;
    fb_text_t(x + 12, wy, "Wallpaper Style", 0x333333);
    fb_fill(x + 12, wy + 20, w - 24, 1, 0xCCCCCC);

    int cur_wp = desktop_get_wallpaper();
    for (int i = 0; i < NUM_WALLPAPERS; i++) {
        int bx = x + 12 + i * 100;
        int by = wy + 30;
        uint32_t bg = (i == cur_wp) ? 0x0078D7 : (i == st->wp_hover ? 0xCCCCCC : 0xE0E0E0);
        fb_fill(bx, by, 80, 28, bg);
        fb_rect(bx, by, 80, 28, 0xAAAAAA);
        fb_text_t(bx + 8, by + 6, wp_names[i], i == cur_wp ? COLOR_WHITE : COLOR_TEXT);
    }
}

static void settings_event(Window* win, Event* ev) {
    SettingsState* st = get_state(win);
    if (!st) return;

    int x = win->x + BORDER_W, y = win->y + TITLE_H;
    int w = win->w - 2 * BORDER_W;

    int sw = 48, sh = 36, pad = 8;
    int cols = (w - 24) / (sw + pad);
    if (cols < 1) cols = 1;

    if (ev->type == EVENT_MOUSE_MOVE || ev->type == EVENT_MOUSE_DOWN) {
        st->color_hover = -1;
        st->wp_hover = -1;

        for (int i = 0; i < NUM_COLORS; i++) {
            int col = i % cols, row = i / cols;
            int cx = x + 12 + col * (sw + pad);
            int cy = y + 40 + row * (sh + pad + 16);
            if (ev->mouse_x >= cx && ev->mouse_x < cx + sw &&
                ev->mouse_y >= cy && ev->mouse_y < cy + sh) {
                st->color_hover = i;
                if (ev->type == EVENT_MOUSE_DOWN) desktop_set_color(bg_colors[i]);
                return;
            }
        }

        int wy = y + 40 + ((NUM_COLORS + cols - 1) / cols) * (sh + pad + 16) + 10;
        for (int i = 0; i < NUM_WALLPAPERS; i++) {
            int bx = x + 12 + i * 100;
            int by = wy + 30;
            if (ev->mouse_x >= bx && ev->mouse_x < bx + 80 &&
                ev->mouse_y >= by && ev->mouse_y < by + 28) {
                st->wp_hover = i;
                if (ev->type == EVENT_MOUSE_DOWN) desktop_set_wallpaper(i);
                return;
            }
        }
    }
}

void settings_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "settings");
    k_strcpy(def.name, "Settings");
    def.icon_color = 0x78909C;
    def.def_w = 400;
    def.def_h = 400;
    def.on_start = settings_start;
    def.on_draw = settings_draw;
    def.on_event = settings_event;
    app_register(&def);
}
