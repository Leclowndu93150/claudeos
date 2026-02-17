#include "kernel.h"

static Window windows[MAX_WINDOWS];
static int win_count = 0;
static int win_order[MAX_WINDOWS];
static int focused = -1;
static int next_id = 1;

void gui_init(void) {
    k_memset(windows, 0, sizeof(windows));
    win_count = 0;
    focused = -1;
}

Window** gui_get_windows(int* count) {
    static Window* ptrs[MAX_WINDOWS];
    int n = 0;
    for (int i = 0; i < win_count; i++)
        ptrs[n++] = &windows[win_order[i]];
    *count = n;
    return ptrs;
}

Window* gui_get_focused(void) {
    if (focused >= 0) return &windows[focused];
    return NULL;
}

Window* window_create(const char* title, int x, int y, int w, int h) {
    if (win_count >= MAX_WINDOWS) return NULL;
    int slot = -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == 0) { slot = i; break; }
    }
    if (slot < 0) return NULL;

    Window* win = &windows[slot];
    k_memset(win, 0, sizeof(Window));
    win->id = next_id++;
    k_strncpy(win->title, title, 127);
    win->x = x; win->y = y; win->w = w; win->h = h;
    win->focused_widget = -1;

    win_order[win_count] = slot;
    win_count++;
    window_focus(win);
    return win;
}

void window_destroy(Window* win) {
    for (int i = 0; i < win->widget_count; i++) {
        if (win->widgets[i].content) kfree(win->widgets[i].content);
    }
    if (win->app && win->app->on_close) win->app->on_close(win);
    if (win->user_data) kfree(win->user_data);

    int slot = (int)(win - windows);
    int oi = -1;
    for (int i = 0; i < win_count; i++) {
        if (win_order[i] == slot) { oi = i; break; }
    }
    if (oi >= 0) {
        for (int i = oi; i < win_count - 1; i++)
            win_order[i] = win_order[i+1];
        win_count--;
    }
    win->id = 0;

    if (focused == slot) {
        focused = win_count > 0 ? win_order[win_count-1] : -1;
        if (focused >= 0) windows[focused].active = true;
    }
}

void window_focus(Window* win) {
    int slot = (int)(win - windows);
    if (focused >= 0) windows[focused].active = false;
    focused = slot;
    win->active = true;

    int oi = -1;
    for (int i = 0; i < win_count; i++) {
        if (win_order[i] == slot) { oi = i; break; }
    }
    if (oi >= 0 && oi < win_count - 1) {
        for (int i = oi; i < win_count - 1; i++)
            win_order[i] = win_order[i+1];
        win_order[win_count-1] = slot;
    }
}

void window_toggle_max(Window* win) {
    Framebuffer* f = fb_get();
    if (win->maximized) {
        win->x = win->rx; win->y = win->ry;
        win->w = win->rw; win->h = win->rh;
        win->maximized = false;
    } else {
        win->rx = win->x; win->ry = win->y;
        win->rw = win->w; win->rh = win->h;
        win->x = 0; win->y = 0;
        win->w = f->width; win->h = f->height - TASKBAR_H;
        win->maximized = true;
    }
}

Widget* window_add_widget(Window* win, int type, int x, int y, int w, int h) {
    if (win->widget_count >= MAX_WIDGETS) return NULL;
    Widget* wg = &win->widgets[win->widget_count];
    k_memset(wg, 0, sizeof(Widget));
    wg->type = type;
    wg->x = x; wg->y = y; wg->w = w; wg->h = h;
    wg->visible = true; wg->enabled = true;
    wg->selected = -1; wg->last_click_idx = -1;
    if (type == W_TEXTAREA) {
        wg->content_cap = MAX_TEXT;
        wg->content = (char*)kmalloc(MAX_TEXT);
        if (wg->content) { wg->content[0] = 0; wg->content_len = 0; }
    }
    win->widget_count++;
    return wg;
}

static void draw_widget(Widget* wg, int ox, int oy) {
    int x = ox + wg->x, y = oy + wg->y;
    int w = wg->w, h = wg->h;

    switch (wg->type) {
    case W_LABEL:
        fb_text_t(x, y + (h - FONT_H)/2, wg->text, wg->color ? wg->color : COLOR_TEXT);
        break;

    case W_BUTTON:
        fb_fill(x, y, w, h, wg->pressed ? COLOR_BUTTON_HVR : (wg->hovered ? COLOR_BUTTON_HVR : COLOR_BUTTON_BG));
        fb_rect(x, y, w, h, COLOR_BORDER);
        fb_text_t(x + (w - fb_text_width(wg->text))/2, y + (h - FONT_H)/2, wg->text, COLOR_TEXT);
        break;

    case W_TEXTINPUT: {
        fb_fill(x, y, w, h, COLOR_INPUT_BG);
        fb_rect(x, y, w, h, wg->focused ? COLOR_INPUT_FOC : COLOR_INPUT_BRD);
        fb_clip_set(x+4, y+2, w-8, h-4);
        int tx = x + 4 - wg->scroll_x;
        char* txt = wg->text[0] ? wg->text : NULL;
        if (txt) fb_text_t(tx, y + (h-FONT_H)/2, txt, COLOR_TEXT);
        if (wg->focused) {
            int cx = tx + wg->cursor_pos * FONT_W;
            if (cx >= x+4 && cx < x+w-4) {
                for (int r = y+4; r < y+h-4; r++) fb_pixel(cx, r, COLOR_TEXT);
            }
        }
        fb_clip_reset();
        break;
    }

    case W_TEXTAREA: {
        fb_fill(x, y, w, h, COLOR_INPUT_BG);
        fb_rect(x, y, w, h, wg->focused ? COLOR_INPUT_FOC : COLOR_INPUT_BRD);
        if (!wg->content) break;
        fb_clip_set(x+2, y+2, w-4-SCROLL_W, h-4);
        int lh = FONT_H + 2;
        int vis = h / lh + 1;
        int start = wg->scroll_y / lh;
        char* p = wg->content;
        int line = 0, lines = 0;
        char* line_starts[4096];
        int line_lens[4096];
        line_starts[0] = p;
        while (*p) {
            if (*p == '\n') {
                line_lens[lines] = (int)(p - line_starts[lines]);
                lines++;
                line_starts[lines] = p + 1;
            }
            p++;
        }
        line_lens[lines] = (int)(p - line_starts[lines]);
        lines++;
        wg->cur_line = wg->cur_line < 0 ? 0 : (wg->cur_line >= lines ? lines-1 : wg->cur_line);

        for (int i = start; i < lines && i < start + vis; i++) {
            int ty = y + 2 + i * lh - wg->scroll_y;
            char buf[512];
            int len = line_lens[i] < 511 ? line_lens[i] : 511;
            k_memcpy(buf, line_starts[i], len);
            buf[len] = 0;
            fb_text_t(x+4, ty, buf, COLOR_TEXT);
            if (wg->focused && i == wg->cur_line) {
                int cc = wg->cur_col < len ? wg->cur_col : len;
                int cx = x + 4 + cc * FONT_W;
                if ((timer_ticks() / 50) % 2 == 0) {
                    for (int r = ty; r < ty+FONT_H && r < y+h-2; r++)
                        fb_pixel(cx, r, COLOR_TEXT);
                }
            }
        }
        fb_clip_reset();

        int total_h = lines * lh;
        if (total_h > h - 4) {
            int sbx = x + w - SCROLL_W;
            fb_fill(sbx, y, SCROLL_W, h, COLOR_SCROLLBAR);
            int th = (h * h) / total_h;
            if (th < 20) th = 20;
            int ty2 = y + (wg->scroll_y * (h - th)) / (total_h - h + 4);
            fb_fill(sbx, ty2, SCROLL_W, th, COLOR_SCROLL_THM);
        }
        break;
    }

    case W_LISTVIEW: {
        fb_fill(x, y, w, h, COLOR_INPUT_BG);
        fb_rect(x, y, w, h, COLOR_INPUT_BRD);
        fb_clip_set(x+1, y+1, w-2, h-2);
        int ih = 24;
        for (int i = 0; i < wg->item_count; i++) {
            int iy = y + 1 + i * ih - wg->list_scroll;
            if (iy + ih < y || iy > y + h) continue;
            if (i == wg->selected) {
                fb_fill(x+1, iy, w-2, ih, COLOR_SELECTION);
                fb_text_t(x+28, iy+(ih-FONT_H)/2, wg->items[i], COLOR_SEL_TEXT);
            } else {
                fb_text_t(x+28, iy+(ih-FONT_H)/2, wg->items[i], COLOR_TEXT);
            }
            uint32_t ic = wg->item_icons[i] ? wg->item_icons[i] : COLOR_TEXT_DIM;
            fb_fill(x+4, iy+2, 20, 20, ic);
        }
        fb_clip_reset();
        break;
    }
    }
}

static bool widget_event(Widget* wg, Window* win, Event* ev, int ox, int oy) {
    int x = ox + wg->x, y = oy + wg->y;
    int w = wg->w, h = wg->h;
    int mx = ev->mouse_x, my = ev->mouse_y;
    bool inside = mx >= x && mx < x+w && my >= y && my < y+h;

    if (wg->type == W_BUTTON) {
        if (ev->type == EVENT_MOUSE_MOVE) { wg->hovered = inside; }
        if (ev->type == EVENT_MOUSE_DOWN && inside) { wg->pressed = true; return true; }
        if (ev->type == EVENT_MOUSE_UP && wg->pressed) {
            wg->pressed = false;
            if (inside && wg->on_click) wg->on_click(wg, win);
            return true;
        }
    }

    if (wg->type == W_TEXTINPUT) {
        if (ev->type == EVENT_MOUSE_DOWN) {
            wg->focused = inside;
            if (inside) {
                int rel = mx - x - 4 + wg->scroll_x;
                wg->cursor_pos = rel / FONT_W;
                int len = k_strlen(wg->text);
                if (wg->cursor_pos > len) wg->cursor_pos = len;
                if (wg->cursor_pos < 0) wg->cursor_pos = 0;
                return true;
            }
        }
        if (ev->type == EVENT_KEY_DOWN && wg->focused) {
            int len = k_strlen(wg->text);
            if (ev->key_char == '\b') {
                if (wg->cursor_pos > 0) {
                    k_memmove(wg->text + wg->cursor_pos - 1, wg->text + wg->cursor_pos, len - wg->cursor_pos + 1);
                    wg->cursor_pos--;
                    if (wg->on_change) wg->on_change(wg, win);
                }
            } else if (ev->key_char == '\n') {
                if (wg->on_submit) wg->on_submit(wg, win);
            } else if (ev->keycode == 0x4B) {
                if (wg->cursor_pos > 0) wg->cursor_pos--;
            } else if (ev->keycode == 0x4D) {
                if (wg->cursor_pos < len) wg->cursor_pos++;
            } else if (ev->key_char >= 32 && ev->key_char <= 126 && len < 254) {
                k_memmove(wg->text + wg->cursor_pos + 1, wg->text + wg->cursor_pos, len - wg->cursor_pos + 1);
                wg->text[wg->cursor_pos] = ev->key_char;
                wg->cursor_pos++;
                if (wg->on_change) wg->on_change(wg, win);
            }
            return true;
        }
    }

    if (wg->type == W_TEXTAREA && wg->content) {
        if (ev->type == EVENT_MOUSE_DOWN) {
            wg->focused = inside;
            if (inside) return true;
        }
        if (ev->type == EVENT_KEY_DOWN && wg->focused) {
            int len = wg->content_len;
            int pos = 0, line = 0, col = 0;
            char* p = wg->content;
            while (pos < len) {
                if (line == wg->cur_line && col == wg->cur_col) break;
                if (p[pos] == '\n') { line++; col = 0; } else { col++; }
                pos++;
            }

            if (ev->key_char == '\b') {
                if (pos > 0) {
                    k_memmove(wg->content + pos - 1, wg->content + pos, len - pos + 1);
                    wg->content_len--;
                    if (wg->cur_col > 0) wg->cur_col--;
                    else if (wg->cur_line > 0) {
                        wg->cur_line--;
                        int cl = 0;
                        char* lp = wg->content;
                        int ln = 0;
                        while (*lp && ln < wg->cur_line) { if (*lp == '\n') ln++; lp++; }
                        while (*lp && *lp != '\n') { cl++; lp++; }
                        wg->cur_col = cl;
                    }
                    if (wg->on_change) wg->on_change(wg, win);
                }
            } else if (ev->key_char == '\n') {
                if (len < wg->content_cap - 1) {
                    k_memmove(wg->content + pos + 1, wg->content + pos, len - pos + 1);
                    wg->content[pos] = '\n';
                    wg->content_len++;
                    wg->cur_line++; wg->cur_col = 0;
                    if (wg->on_change) wg->on_change(wg, win);
                }
            } else if (ev->keycode == 0x48) {
                if (wg->cur_line > 0) wg->cur_line--;
            } else if (ev->keycode == 0x50) {
                wg->cur_line++;
            } else if (ev->keycode == 0x4B) {
                if (wg->cur_col > 0) wg->cur_col--;
                else if (wg->cur_line > 0) { wg->cur_line--; wg->cur_col = 9999; }
            } else if (ev->keycode == 0x4D) {
                wg->cur_col++;
            } else if (ev->key_char >= 32 && ev->key_char <= 126 && len < wg->content_cap - 1) {
                k_memmove(wg->content + pos + 1, wg->content + pos, len - pos + 1);
                wg->content[pos] = ev->key_char;
                wg->content_len++;
                wg->cur_col++;
                if (wg->on_change) wg->on_change(wg, win);
            }

            int lh = FONT_H + 2;
            int cy = wg->cur_line * lh;
            if (cy < wg->scroll_y) wg->scroll_y = cy;
            if (cy + lh > wg->scroll_y + wg->h - 4)
                wg->scroll_y = cy + lh - wg->h + 4;

            return true;
        }
    }

    if (wg->type == W_LISTVIEW) {
        int ih = 24;
        if (ev->type == EVENT_MOUSE_DOWN && inside) {
            int idx = (my - y - 1 + wg->list_scroll) / ih;
            if (idx >= 0 && idx < wg->item_count) {
                uint32_t now = timer_ticks();
                if (idx == wg->last_click_idx && now - wg->last_click_time < 40) {
                    if (wg->on_dblclick) wg->on_dblclick(wg, win, idx);
                    wg->last_click_time = 0;
                } else {
                    wg->selected = idx;
                    wg->last_click_time = now;
                    wg->last_click_idx = idx;
                    if (wg->on_select) wg->on_select(wg, win, idx);
                }
            }
            return true;
        }
    }

    return false;
}

static void draw_window(Window* win) {
    if (win->minimized) return;
    int x = win->x, y = win->y, w = win->w, h = win->h;

    fb_rect(x, y, w, h, win->active ? COLOR_TITLE : COLOR_TITLE_OFF);

    uint32_t tc = win->active ? COLOR_TITLE : COLOR_TITLE_OFF;
    fb_fill(x+BORDER_W, y+BORDER_W, w-2*BORDER_W, TITLE_H-BORDER_W, tc);
    fb_text_t(x+10, y+(TITLE_H-FONT_H)/2, win->title, COLOR_WHITE);

    int bx = x + w - WIN_BTN_W;
    fb_fill(bx, y, WIN_BTN_W, TITLE_H, win->close_hov ? COLOR_CLOSE_HVR : tc);
    fb_text_t(bx + (WIN_BTN_W-FONT_W)/2, y+(TITLE_H-FONT_H)/2, "X", COLOR_WHITE);

    bx -= WIN_BTN_W;
    if (win->max_hov) fb_fill(bx, y, WIN_BTN_W, TITLE_H, COLOR_TASKBAR_HVR);
    char* ms = win->maximized ? "=" : "#";
    fb_text_t(bx + (WIN_BTN_W-FONT_W)/2, y+(TITLE_H-FONT_H)/2, ms, COLOR_WHITE);

    bx -= WIN_BTN_W;
    if (win->min_hov) fb_fill(bx, y, WIN_BTN_W, TITLE_H, COLOR_TASKBAR_HVR);
    fb_text_t(bx + (WIN_BTN_W-FONT_W)/2, y+(TITLE_H-FONT_H)/2, "_", COLOR_WHITE);

    int cx = x + BORDER_W;
    int cy = y + TITLE_H;
    int cw = w - 2 * BORDER_W;
    int ch = h - TITLE_H - BORDER_W;
    fb_fill(cx, cy, cw, ch, COLOR_WINDOW_BG);

    if (win->app && win->app->on_draw) {
        fb_clip_set(cx, cy, cw, ch);
        win->app->on_draw(win, cx, cy, cw, ch);
        fb_clip_reset();
    }

    fb_clip_set(cx, cy, cw, ch);
    for (int i = 0; i < win->widget_count; i++) {
        if (win->widgets[i].visible) draw_widget(&win->widgets[i], cx, cy);
    }
    fb_clip_reset();
}

static bool window_event(Window* win, Event* ev) {
    int x = win->x, y = win->y, w = win->w, h = win->h;
    int mx = ev->mouse_x, my = ev->mouse_y;

    if (ev->type == EVENT_MOUSE_MOVE) {
        int bx = x + w - WIN_BTN_W;
        win->close_hov = mx >= bx && mx < bx+WIN_BTN_W && my >= y && my < y+TITLE_H;
        bx -= WIN_BTN_W;
        win->max_hov = mx >= bx && mx < bx+WIN_BTN_W && my >= y && my < y+TITLE_H;
        bx -= WIN_BTN_W;
        win->min_hov = mx >= bx && mx < bx+WIN_BTN_W && my >= y && my < y+TITLE_H;

        if (win->dragging) {
            win->x = mx - win->drag_ox;
            win->y = my - win->drag_oy;
            return true;
        }
    }

    if (ev->type == EVENT_MOUSE_DOWN && ev->mouse_button == 0) {
        if (my >= y && my < y + TITLE_H && mx >= x && mx < x + w) {
            int bx = x + w - WIN_BTN_W;
            if (mx >= bx) return (int)'c';
            bx -= WIN_BTN_W;
            if (mx >= bx) { window_toggle_max(win); return true; }
            bx -= WIN_BTN_W;
            if (mx >= bx) { win->minimized = true; return true; }

            win->dragging = true;
            win->drag_ox = mx - x;
            win->drag_oy = my - y;
            return true;
        }
    }

    if (ev->type == EVENT_MOUSE_UP) {
        win->dragging = false;
    }

    int cx = x + BORDER_W;
    int cy = y + TITLE_H;
    int cw = w - 2*BORDER_W;
    int ch = h - TITLE_H - BORDER_W;

    if (ev->type == EVENT_KEY_DOWN || (ev->type >= EVENT_MOUSE_DOWN && mx >= cx && mx < cx+cw && my >= cy && my < cy+ch)) {
        for (int i = 0; i < win->widget_count; i++) {
            if (win->widgets[i].visible && win->widgets[i].enabled) {
                if (widget_event(&win->widgets[i], win, ev, cx, cy)) return true;
            }
        }
        if (win->app && win->app->on_event) {
            win->app->on_event(win, ev);
            return true;
        }
    }

    return false;
}

void gui_event(Event* ev) {
    Framebuffer* f = fb_get();

    if (start_menu_is_open()) {
        taskbar_event(ev);
        if (ev->type == EVENT_MOUSE_DOWN) start_menu_close();
        return;
    }

    if (ev->type == EVENT_MOUSE_DOWN || ev->type == EVENT_MOUSE_MOVE || ev->type == EVENT_MOUSE_UP) {
        if (ev->mouse_y >= (int)f->height - TASKBAR_H) {
            taskbar_event(ev);
            return;
        }
    }

    if (ev->type == EVENT_MOUSE_DOWN) {
        for (int i = win_count - 1; i >= 0; i--) {
            Window* w = &windows[win_order[i]];
            if (w->minimized) continue;
            if (ev->mouse_x >= w->x && ev->mouse_x < w->x + w->w &&
                ev->mouse_y >= w->y && ev->mouse_y < w->y + w->h) {
                if (!w->active) window_focus(w);
                int r = window_event(w, ev);
                if (r == (int)'c') window_destroy(w);
                return;
            }
        }
        desktop_event(ev);
        return;
    }

    if (ev->type == EVENT_MOUSE_MOVE || ev->type == EVENT_MOUSE_UP) {
        if (focused >= 0) window_event(&windows[focused], ev);
        return;
    }

    if (ev->type == EVENT_KEY_DOWN) {
        if (focused >= 0) window_event(&windows[focused], ev);
        return;
    }
}

void gui_draw(void) {
    desktop_draw();
    for (int i = 0; i < win_count; i++) {
        draw_window(&windows[win_order[i]]);
    }
    taskbar_draw();
    MouseState* m = mouse_state();
    cursor_draw(m->x, m->y);
    fb_swap();
}

void gui_tick(void) {
    for (int i = 0; i < win_count; i++) {
        Window* w = &windows[win_order[i]];
        if (w->app && w->app->on_tick) w->app->on_tick(w);
    }
}
