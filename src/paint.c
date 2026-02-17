#include "kernel.h"

#define PAINT_CW 512
#define PAINT_CH 384
#define TOOLBAR_H 36
#define PAL_COUNT 16

static const uint32_t palette[PAL_COUNT] = {
    0x000000, 0xFFFFFF, 0xFF0000, 0x00FF00,
    0x0000FF, 0xFFFF00, 0xFF00FF, 0x00FFFF,
    0x800000, 0x008000, 0x000080, 0x808000,
    0x800080, 0x008080, 0xC0C0C0, 0x808080,
};

typedef struct {
    uint32_t* canvas;
    int cw, ch;
    uint32_t color;
    int brush;
    bool drawing;
    int last_cx, last_cy;
    int pal_sel;
    int pal_hover;
    int brush_hover;
} PaintState;

static PaintState* get_state(Window* win) {
    return (PaintState*)win->user_data;
}

static void paint_set_pixel(PaintState* st, int cx, int cy) {
    int r = st->brush;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int px = cx + dx, py = cy + dy;
            if (px >= 0 && px < st->cw && py >= 0 && py < st->ch)
                st->canvas[py * st->cw + px] = st->color;
        }
    }
}

static void paint_line(PaintState* st, int x0, int y0, int x1, int y1) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (1) {
        paint_set_pixel(st, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

static void paint_start(Window* win) {
    win->user_data = kmalloc(sizeof(PaintState));
    if (!win->user_data) return;
    k_memset(win->user_data, 0, sizeof(PaintState));
    PaintState* st = get_state(win);
    st->cw = PAINT_CW;
    st->ch = PAINT_CH;
    st->color = 0x000000;
    st->brush = 1;
    st->pal_sel = 0;
    st->pal_hover = -1;
    st->brush_hover = -1;
    st->canvas = (uint32_t*)kmalloc(st->cw * st->ch * 4);
    if (st->canvas) {
        for (int i = 0; i < st->cw * st->ch; i++)
            st->canvas[i] = 0xFFFFFF;
    }
}

static void paint_draw(Window* win, int x, int y, int w, int h) {
    PaintState* st = get_state(win);
    if (!st || !st->canvas) return;

    fb_fill(x, y, w, TOOLBAR_H, 0xF0F0F0);
    fb_fill(x, y + TOOLBAR_H - 1, w, 1, 0xCCCCCC);

    int px = x + 4;
    for (int i = 0; i < PAL_COUNT; i++) {
        int ps = 18;
        fb_fill(px, y + 4, ps, ps, palette[i]);
        if (i == st->pal_sel)
            fb_rect(px - 1, y + 3, ps + 2, ps + 2, 0xFF0000);
        else if (i == st->pal_hover)
            fb_rect(px - 1, y + 3, ps + 2, ps + 2, 0x666666);
        px += ps + 3;
    }

    int bx = px + 10;
    char* sizes[] = {"S", "M", "L"};
    for (int i = 0; i < 3; i++) {
        uint32_t bg = (st->brush == i) ? 0x0078D7 : (st->brush_hover == i ? 0xCCCCCC : 0xE0E0E0);
        fb_fill(bx, y + 4, 24, 20, bg);
        fb_rect(bx, y + 4, 24, 20, 0x999999);
        fb_text_t(bx + 8, y + 6, sizes[i], st->brush == i ? COLOR_WHITE : COLOR_TEXT);
        bx += 28;
    }

    int canvas_x = x + (w - st->cw) / 2;
    int canvas_y = y + TOOLBAR_H + (h - TOOLBAR_H - st->ch) / 2;
    if (canvas_x < x) canvas_x = x;
    if (canvas_y < y + TOOLBAR_H) canvas_y = y + TOOLBAR_H;

    fb_fill(x, y + TOOLBAR_H, w, h - TOOLBAR_H, 0x808080);

    int draw_w = st->cw, draw_h = st->ch;
    if (canvas_x + draw_w > x + w) draw_w = x + w - canvas_x;
    if (canvas_y + draw_h > y + h) draw_h = y + h - canvas_y;

    for (int row = 0; row < draw_h; row++) {
        uint32_t* src = st->canvas + row * st->cw;
        for (int col = 0; col < draw_w; col++) {
            fb_pixel(canvas_x + col, canvas_y + row, src[col]);
        }
    }
}

static void paint_event(Window* win, Event* ev) {
    PaintState* st = get_state(win);
    if (!st || !st->canvas) return;

    int x = win->x + BORDER_W, y = win->y + TITLE_H;
    int w = win->w - 2 * BORDER_W, h = win->h - TITLE_H - BORDER_W;

    int canvas_x = x + (w - st->cw) / 2;
    int canvas_y = y + TOOLBAR_H + (h - TOOLBAR_H - st->ch) / 2;
    if (canvas_x < x) canvas_x = x;
    if (canvas_y < y + TOOLBAR_H) canvas_y = y + TOOLBAR_H;

    if (ev->type == EVENT_MOUSE_MOVE) {
        st->pal_hover = -1;
        st->brush_hover = -1;
        int px = x + 4;
        for (int i = 0; i < PAL_COUNT; i++) {
            if (ev->mouse_x >= px && ev->mouse_x < px + 18 &&
                ev->mouse_y >= y + 4 && ev->mouse_y < y + 22) {
                st->pal_hover = i;
                break;
            }
            px += 21;
        }
        int bx = x + 4 + PAL_COUNT * 21 + 10;
        for (int i = 0; i < 3; i++) {
            if (ev->mouse_x >= bx && ev->mouse_x < bx + 24 &&
                ev->mouse_y >= y + 4 && ev->mouse_y < y + 24) {
                st->brush_hover = i;
                break;
            }
            bx += 28;
        }
    }

    if (ev->type == EVENT_MOUSE_DOWN && ev->mouse_button == 0) {
        int px = x + 4;
        for (int i = 0; i < PAL_COUNT; i++) {
            if (ev->mouse_x >= px && ev->mouse_x < px + 18 &&
                ev->mouse_y >= y + 4 && ev->mouse_y < y + 22) {
                st->pal_sel = i;
                st->color = palette[i];
                return;
            }
            px += 21;
        }
        int bx = x + 4 + PAL_COUNT * 21 + 10;
        for (int i = 0; i < 3; i++) {
            if (ev->mouse_x >= bx && ev->mouse_x < bx + 24 &&
                ev->mouse_y >= y + 4 && ev->mouse_y < y + 24) {
                st->brush = i;
                return;
            }
            bx += 28;
        }

        int cx = ev->mouse_x - canvas_x;
        int cy = ev->mouse_y - canvas_y;
        if (cx >= 0 && cx < st->cw && cy >= 0 && cy < st->ch) {
            st->drawing = true;
            st->last_cx = cx;
            st->last_cy = cy;
            paint_set_pixel(st, cx, cy);
        }
    }

    if (ev->type == EVENT_MOUSE_MOVE && st->drawing) {
        int cx = ev->mouse_x - canvas_x;
        int cy = ev->mouse_y - canvas_y;
        if (cx < 0) cx = 0; if (cx >= st->cw) cx = st->cw - 1;
        if (cy < 0) cy = 0; if (cy >= st->ch) cy = st->ch - 1;
        paint_line(st, st->last_cx, st->last_cy, cx, cy);
        st->last_cx = cx;
        st->last_cy = cy;
    }

    if (ev->type == EVENT_MOUSE_UP) {
        st->drawing = false;
    }
}

static void paint_close(Window* win) {
    PaintState* st = get_state(win);
    if (st && st->canvas) kfree(st->canvas);
}

void paint_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "paint");
    k_strcpy(def.name, "Paint");
    def.icon_color = 0xE91E63;
    def.def_w = 600;
    def.def_h = 480;
    def.on_start = paint_start;
    def.on_draw = paint_draw;
    def.on_event = paint_event;
    def.on_close = paint_close;
    app_register(&def);
}
