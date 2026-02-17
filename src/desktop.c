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

void desktop_init(void) {
    icon_count = 0;
    k_strcpy(icons[0].name, "Files");
    k_strcpy(icons[0].app_id, "explorer");
    icons[0].color = 0xFFC832;
    icons[0].x = 20; icons[0].y = 20;
    icon_count++;

    k_strcpy(icons[1].name, "Notepad");
    k_strcpy(icons[1].app_id, "notepad");
    icons[1].color = 0x4FC3F7;
    icons[1].x = 20; icons[1].y = 110;
    icon_count++;

    k_strcpy(icons[2].name, "Snake");
    k_strcpy(icons[2].app_id, "snake");
    icons[2].color = 0x4CAF50;
    icons[2].x = 20; icons[2].y = 200;
    icon_count++;

    k_strcpy(icons[3].name, "Tetris");
    k_strcpy(icons[3].app_id, "tetris");
    icons[3].color = 0xE040FB;
    icons[3].x = 20; icons[3].y = 290;
    icon_count++;
}

void desktop_draw(void) {
    Framebuffer* f = fb_get();
    fb_fill(0, 0, f->width, f->height, COLOR_DESKTOP);

    fb_text_t(f->width/2 - 60, f->height/2 - 40, "Claude OS", COLOR_WHITE);
    fb_text_t(f->width/2 - 36, f->height/2 - 16, "v1.0", COLOR_WHITE);

    for (int i = 0; i < icon_count; i++) {
        int ix = icons[i].x, iy = icons[i].y;
        fb_fill(ix + 16, iy, 48, 40, icons[i].color);
        fb_fill(ix + 12, iy + 8, 56, 32, icons[i].color);
        int tw = fb_text_width(icons[i].name);
        int tx = ix + 40 - tw/2;
        fb_text_t(tx+1, iy+50+1, icons[i].name, 0x002040);
        fb_text_t(tx, iy+50, icons[i].name, COLOR_ICON_TEXT);
    }
}

void desktop_event(Event* ev) {
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
}

static int tb_hovered = -1;

void taskbar_init(void) {}

void taskbar_draw(void) {
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
        int bw = 160;
        if (bx + bw > (int)f->width - 100) break;
        bool active = wins[i]->active && !wins[i]->minimized;
        fb_fill(bx, sy, bw, sh, active ? COLOR_TASKBAR_ACT : (tb_hovered == i ? COLOR_TASKBAR_HVR : COLOR_TASKBAR));
        if (active) fb_fill(bx, ty + TASKBAR_H - 3, bw, 3, COLOR_TITLE);

        char buf[24];
        k_strncpy(buf, wins[i]->title, 18);
        buf[18] = 0;
        fb_text_t(bx + 8, sy + (sh-FONT_H)/2, buf, COLOR_WHITE);
        bx += bw + 2;
    }

    uint32_t t = timer_ticks() / 100;
    int secs = t % 60;
    int mins = (t / 60) % 60;
    int hrs = (t / 3600) % 24;
    char time_buf[16];
    time_buf[0] = '0' + hrs/10; time_buf[1] = '0' + hrs%10;
    time_buf[2] = ':';
    time_buf[3] = '0' + mins/10; time_buf[4] = '0' + mins%10;
    time_buf[5] = ':';
    time_buf[6] = '0' + secs/10; time_buf[7] = '0' + secs%10;
    time_buf[8] = 0;
    fb_text_t(f->width - 80, ty + (TASKBAR_H-FONT_H)/2, time_buf, COLOR_WHITE);

    if (sm_open) {
        int smx = 0, smy = ty - 8 * MENU_ITEM_H - 10;
        int smw = START_MENU_W, smh = 8 * MENU_ITEM_H + 10;
        fb_fill(smx, smy, smw, smh, COLOR_MENU_BG);

        fb_text_t(smx + 16, smy + 12, "Claude OS", 0x0078D7);
        fb_fill(smx, smy + MENU_ITEM_H + 8, smw, 1, 0x555555);

        int app_count = 0;
        AppDef* apps = app_get_all(&app_count);
        for (int i = 0; i < app_count && i < 6; i++) {
            int iy = smy + MENU_ITEM_H + 12 + i * MENU_ITEM_H;
            MouseState* ms = mouse_state();
            if (ms->x >= smx && ms->x < smx+smw && ms->y >= iy && ms->y < iy+MENU_ITEM_H) {
                fb_fill(smx, iy, smw, MENU_ITEM_H, COLOR_MENU_HVR);
            }
            fb_fill(smx + 16, iy + 8, 20, 20, apps[i].icon_color);
            fb_text_t(smx + 48, iy + (MENU_ITEM_H-FONT_H)/2, apps[i].name, COLOR_MENU_TEXT);
        }

        int shut_y = smy + smh - MENU_ITEM_H - 4;
        fb_fill(smx, shut_y - 4, smw, 1, 0x555555);
        MouseState* ms2 = mouse_state();
        if (ms2->x >= smx && ms2->x < smx+smw && ms2->y >= shut_y && ms2->y < shut_y+MENU_ITEM_H) {
            fb_fill(smx, shut_y, smw, MENU_ITEM_H, COLOR_MENU_HVR);
        }
        fb_text_t(smx + 48, shut_y + (MENU_ITEM_H-FONT_H)/2, "Shutdown", COLOR_MENU_TEXT);
    }
}

void taskbar_event(Event* ev) {
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
                if (ev->mouse_x >= bx && ev->mouse_x < bx + 160) { tb_hovered = i; break; }
                bx += 162;
            }
        }
    }

    if (ev->type == EVENT_MOUSE_DOWN && ev->mouse_button == 0) {
        if (sm_open) {
            int smy = ty - 8 * MENU_ITEM_H - 10;
            int app_count = 0;
            AppDef* apps = app_get_all(&app_count);
            for (int i = 0; i < app_count && i < 6; i++) {
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
                if (ev->mouse_x >= bx && ev->mouse_x < bx + 160) {
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
                bx += 162;
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
