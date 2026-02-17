#include "kernel.h"

typedef struct {
    char name[32];
    char app_id[32];
    uint32_t color;
    int x, y;
} DesktopIcon;

static DesktopIcon icons[16];
static int icon_count = 0;
static bool sm_open = false;
static int sm_scroll = 0;

static uint32_t bg_color = COLOR_DESKTOP;
static int wallpaper_style = 1;

static bool ctx_open = false;
static int ctx_x, ctx_y;
static int ctx_hover = -1;

static bool boot_splash = true;

uint32_t desktop_get_color(void) { return bg_color; }
void desktop_set_color(uint32_t c) { bg_color = c; }
int desktop_get_wallpaper(void) { return wallpaper_style; }
void desktop_set_wallpaper(int s) { wallpaper_style = s; }

static void add_icon(const char* name, const char* id, uint32_t color, int x, int y) {
    if (icon_count >= 16) return;
    k_strcpy(icons[icon_count].name, name);
    k_strcpy(icons[icon_count].app_id, id);
    icons[icon_count].color = color;
    icons[icon_count].x = x;
    icons[icon_count].y = y;
    icon_count++;
}

void desktop_init(void) {
    icon_count = 0;
    int y = 20, dy = 90;
    add_icon("Files",    "explorer", 0xFFC832, 20, y); y += dy;
    add_icon("Notepad",  "notepad",  0x4FC3F7, 20, y); y += dy;
    add_icon("Terminal", "terminal", 0x333333, 20, y); y += dy;
    add_icon("Snake",    "snake",    0x4CAF50, 20, y); y += dy;
    add_icon("Tetris",   "tetris",   0xE040FB, 20, y); y += dy;
    add_icon("Paint",    "paint",    0xE91E63, 20, y); y += dy;
    add_icon("Calc",     "calc",     0xFF9800, 20, y); y += dy;
}

static void draw_wallpaper(void) {
    Framebuffer* f = fb_get();
    int w = f->width, h = f->height;

    if (wallpaper_style == 0) {
        fb_fill(0, 0, w, h, bg_color);
    } else if (wallpaper_style == 1) {
        int r1 = (bg_color >> 16) & 0xFF, g1 = (bg_color >> 8) & 0xFF, b1 = bg_color & 0xFF;
        int r2 = r1 / 3, g2 = g1 / 3, b2 = b1 / 3;
        for (int y = 0; y < h; y++) {
            int r = r1 + (r2 - r1) * y / h;
            int g = g1 + (g2 - g1) * y / h;
            int b = b1 + (b2 - b1) * y / h;
            fb_fill(0, y, w, 1, (r << 16) | (g << 8) | b);
        }
    } else {
        fb_fill(0, 0, w, h, bg_color);
        uint32_t line_color = ((bg_color >> 1) & 0x7F7F7F);
        for (int y = 0; y < h; y += 32) fb_fill(0, y, w, 1, line_color);
        for (int x = 0; x < w; x += 32) fb_fill(x, 0, 1, h, line_color);
    }
}

void desktop_draw(void) {
    Framebuffer* f = fb_get();

    if (boot_splash) {
        fb_fill(0, 0, f->width, f->height, 0x0C0C1A);
        int cx = f->width / 2, cy = f->height / 2;
        fb_fill(cx - 60, cy - 70, 120, 80, 0x0078D7);
        fb_text_t(cx - 28, cy - 40, "COS", COLOR_WHITE);
        fb_text_t(cx - 52, cy + 30, "Claude OS", COLOR_WHITE);
        fb_text_t(cx - 100, cy + 70, "Click or press any key to start", 0x666666);
        return;
    }

    draw_wallpaper();

    fb_text_t(f->width/2 - 60, f->height/2 - 40, "Claude OS", 0xFFFFFF40);
    fb_text_t(f->width/2 - 36, f->height/2 - 16, "v1.0", 0xFFFFFF40);

    for (int i = 0; i < icon_count; i++) {
        int ix = icons[i].x, iy = icons[i].y;
        fb_fill(ix + 16, iy, 48, 40, icons[i].color);
        fb_fill(ix + 12, iy + 8, 56, 32, icons[i].color);
        int tw = fb_text_width(icons[i].name);
        int tx = ix + 40 - tw/2;
        fb_text_t(tx+1, iy+50+1, icons[i].name, 0x00000080);
        fb_text_t(tx, iy+50, icons[i].name, COLOR_ICON_TEXT);
    }
}

#define CTX_ITEMS 5
static const char* ctx_labels[CTX_ITEMS] = {
    "New File", "New Folder", "Settings", "About", "Terminal"
};
static const char* ctx_actions[CTX_ITEMS] = {
    "", "", "settings", "about", "terminal"
};

void context_menu_draw(void) {
    if (!ctx_open) return;
    int mw = 160, mh = CTX_ITEMS * 28 + 4;
    fb_fill(ctx_x, ctx_y, mw, mh, COLOR_MENU_BG);
    fb_rect(ctx_x, ctx_y, mw, mh, 0x555555);
    for (int i = 0; i < CTX_ITEMS; i++) {
        int iy = ctx_y + 2 + i * 28;
        if (i == ctx_hover) fb_fill(ctx_x + 1, iy, mw - 2, 28, COLOR_MENU_HVR);
        fb_text_t(ctx_x + 12, iy + 6, ctx_labels[i], COLOR_MENU_TEXT);
    }
}

void desktop_event(Event* ev) {
    if (boot_splash) {
        if (ev->type == EVENT_MOUSE_DOWN || ev->type == EVENT_KEY_DOWN) {
            boot_splash = false;
        }
        return;
    }

    if (ctx_open) {
        if (ev->type == EVENT_MOUSE_MOVE) {
            ctx_hover = -1;
            int mw = 160;
            for (int i = 0; i < CTX_ITEMS; i++) {
                int iy = ctx_y + 2 + i * 28;
                if (ev->mouse_x >= ctx_x && ev->mouse_x < ctx_x + mw &&
                    ev->mouse_y >= iy && ev->mouse_y < iy + 28) {
                    ctx_hover = i;
                    break;
                }
            }
        }
        if (ev->type == EVENT_MOUSE_DOWN) {
            if (ctx_hover >= 0) {
                if (ctx_hover == 0) {
                    fs_mkfile("/home/Documents/new.txt", (const uint8_t*)"", 0);
                } else if (ctx_hover == 1) {
                    fs_mkdir("/home/NewFolder");
                } else if (ctx_actions[ctx_hover][0]) {
                    app_launch(ctx_actions[ctx_hover]);
                }
            }
            ctx_open = false;
            ctx_hover = -1;
            return;
        }
        return;
    }

    if (ev->type == EVENT_MOUSE_DOWN && ev->mouse_button == 0) {
        for (int i = 0; i < icon_count; i++) {
            int ix = icons[i].x, iy = icons[i].y;
            if (ev->mouse_x >= ix && ev->mouse_x < ix+80 &&
                ev->mouse_y >= iy && ev->mouse_y < iy+70) {
                app_launch(icons[i].app_id);
                return;
            }
        }
    }

    if (ev->type == EVENT_MOUSE_DOWN && ev->mouse_button == 1) {
        ctx_open = true;
        ctx_x = ev->mouse_x;
        ctx_y = ev->mouse_y;
        ctx_hover = -1;
        Framebuffer* f = fb_get();
        if (ctx_x + 160 > (int)f->width) ctx_x = f->width - 160;
        if (ctx_y + CTX_ITEMS * 28 + 4 > (int)f->height - TASKBAR_H)
            ctx_y = f->height - TASKBAR_H - CTX_ITEMS * 28 - 4;
    }
}

static int tb_hovered = -1;

void taskbar_init(void) {}

void taskbar_draw(void) {
    if (boot_splash) return;
    Framebuffer* f = fb_get();
    int ty = f->height - TASKBAR_H;

    fb_fill(0, ty, f->width, TASKBAR_H, COLOR_TASKBAR);
    fb_fill(0, ty, f->width, 1, 0x333333);

    int sx = 4, sy = ty + 4, sw = 80, sh = TASKBAR_H - 8;
    fb_fill(sx, sy, sw, sh, tb_hovered == -2 ? COLOR_TASKBAR_HVR : COLOR_TASKBAR);
    fb_text_t(sx + 8, sy + (sh-FONT_H)/2, "Claude", COLOR_WHITE);

    int wc = 0;
    Window** wins = gui_get_windows(&wc);
    int bx = 90;
    for (int i = 0; i < wc; i++) {
        int bw = 140;
        if (bx + bw > (int)f->width - 100) break;
        bool active = wins[i]->active && !wins[i]->minimized;
        fb_fill(bx, sy, bw, sh, active ? COLOR_TASKBAR_ACT : (tb_hovered == i ? COLOR_TASKBAR_HVR : COLOR_TASKBAR));
        if (active) fb_fill(bx, ty + TASKBAR_H - 3, bw, 3, COLOR_TITLE);
        char buf[20];
        k_strncpy(buf, wins[i]->title, 16);
        buf[16] = 0;
        fb_text_t(bx + 8, sy + (sh-FONT_H)/2, buf, COLOR_WHITE);
        bx += bw + 2;
    }

    uint32_t t = timer_ticks() / 100;
    int secs = t % 60, mins = (t / 60) % 60, hrs = (t / 3600) % 24;
    char time_buf[16];
    time_buf[0] = '0' + hrs/10; time_buf[1] = '0' + hrs%10;
    time_buf[2] = ':';
    time_buf[3] = '0' + mins/10; time_buf[4] = '0' + mins%10;
    time_buf[5] = ':';
    time_buf[6] = '0' + secs/10; time_buf[7] = '0' + secs%10;
    time_buf[8] = 0;
    fb_text_t(f->width - 80, ty + (TASKBAR_H-FONT_H)/2, time_buf, COLOR_WHITE);

    if (sm_open) {
        int app_count = 0;
        AppDef* apps = app_get_all(&app_count);
        int max_show = 14;
        if (app_count < max_show) max_show = app_count;
        int smh = (max_show + 2) * MENU_ITEM_H + 10;
        int smx = 0, smy = ty - smh;
        int smw = START_MENU_W;
        fb_fill(smx, smy, smw, smh, COLOR_MENU_BG);

        fb_text_t(smx + 16, smy + 12, "Claude OS", 0x0078D7);
        fb_fill(smx, smy + MENU_ITEM_H + 8, smw, 1, 0x555555);

        for (int i = 0; i < app_count && i < max_show; i++) {
            int iy = smy + MENU_ITEM_H + 12 + i * MENU_ITEM_H;
            MouseState* ms = mouse_state();
            if (ms->x >= smx && ms->x < smx+smw && ms->y >= iy && ms->y < iy+MENU_ITEM_H)
                fb_fill(smx, iy, smw, MENU_ITEM_H, COLOR_MENU_HVR);
            fb_fill(smx + 16, iy + 8, 20, 20, apps[i].icon_color);
            fb_text_t(smx + 48, iy + (MENU_ITEM_H-FONT_H)/2, apps[i].name, COLOR_MENU_TEXT);
        }

        int shut_y = smy + smh - MENU_ITEM_H - 4;
        fb_fill(smx, shut_y - 4, smw, 1, 0x555555);
        MouseState* ms2 = mouse_state();
        if (ms2->x >= smx && ms2->x < smx+smw && ms2->y >= shut_y && ms2->y < shut_y+MENU_ITEM_H)
            fb_fill(smx, shut_y, smw, MENU_ITEM_H, COLOR_MENU_HVR);
        fb_text_t(smx + 48, shut_y + (MENU_ITEM_H-FONT_H)/2, "Shutdown", COLOR_MENU_TEXT);
    }
}

void taskbar_event(Event* ev) {
    if (boot_splash) return;
    Framebuffer* f = fb_get();
    int ty = f->height - TASKBAR_H;

    if (ev->type == EVENT_MOUSE_MOVE) {
        tb_hovered = -1;
        if (ev->mouse_x >= 4 && ev->mouse_x < 84) tb_hovered = -2;
        else {
            int wc = 0;
            gui_get_windows(&wc);
            int bx = 90;
            for (int i = 0; i < wc; i++) {
                if (ev->mouse_x >= bx && ev->mouse_x < bx + 140) { tb_hovered = i; break; }
                bx += 142;
            }
        }
    }

    if (ev->type == EVENT_MOUSE_DOWN && ev->mouse_button == 0) {
        if (sm_open) {
            int app_count = 0;
            AppDef* apps = app_get_all(&app_count);
            int max_show = 14;
            if (app_count < max_show) max_show = app_count;
            int smh = (max_show + 2) * MENU_ITEM_H + 10;
            int smy = ty - smh;
            for (int i = 0; i < app_count && i < max_show; i++) {
                int iy = smy + MENU_ITEM_H + 12 + i * MENU_ITEM_H;
                if (ev->mouse_x >= 0 && ev->mouse_x < START_MENU_W &&
                    ev->mouse_y >= iy && ev->mouse_y < iy + MENU_ITEM_H) {
                    app_launch(apps[i].id);
                    sm_open = false;
                    return;
                }
            }
            sm_open = false;
            return;
        }

        if (ev->mouse_y >= ty) {
            if (ev->mouse_x >= 4 && ev->mouse_x < 84) {
                sm_open = !sm_open;
                return;
            }
            int wc = 0;
            Window** wins = gui_get_windows(&wc);
            int bx = 90;
            for (int i = 0; i < wc; i++) {
                if (ev->mouse_x >= bx && ev->mouse_x < bx + 140) {
                    if (wins[i]->minimized) {
                        wins[i]->minimized = false;
                        window_focus(wins[i]);
                    } else if (wins[i]->active) {
                        wins[i]->minimized = true;
                    } else {
                        window_focus(wins[i]);
                    }
                    return;
                }
                bx += 142;
            }
        }
    }
}

bool start_menu_is_open(void) { return sm_open; }
void start_menu_close(void) { sm_open = false; }

void cursor_draw(int x, int y) {
    for (int i = 0; i < 12; i++) {
        fb_pixel(x, y+i, COLOR_WHITE);
        fb_pixel(x+1, y+i, COLOR_BLACK);
    }
    for (int i = 0; i < 8; i++) {
        fb_pixel(x+i, y+i, COLOR_WHITE);
        fb_pixel(x+i+1, y+i, COLOR_BLACK);
    }
    fb_pixel(x+1, y+8, COLOR_WHITE);
    fb_pixel(x+2, y+9, COLOR_WHITE);
    fb_pixel(x+3, y+10, COLOR_WHITE);
    fb_pixel(x+2, y+8, COLOR_BLACK);
    fb_pixel(x+3, y+9, COLOR_BLACK);
    fb_pixel(x+4, y+10, COLOR_BLACK);
}
