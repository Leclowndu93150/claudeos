#include "kernel.h"

typedef struct {
    char display[32];
    int acc;
    int current;
    int op;
    bool new_input;
    int hover_r, hover_c;
} CalcState;

static CalcState* get_state(Window* win) {
    return (CalcState*)win->user_data;
}

#define CALC_COLS 4
#define CALC_ROWS 5
#define DISP_H 50
#define BTN_PAD 3

static const char btn_text[CALC_ROWS][CALC_COLS][3] = {
    {"C",  "<",  "/",  "x"},
    {"7",  "8",  "9",  "-"},
    {"4",  "5",  "6",  "+"},
    {"1",  "2",  "3",  "="},
    {"0",  "",   "",   ""},
};

static const uint32_t btn_bg[CALC_ROWS][CALC_COLS] = {
    {0xD32F2F, 0x555555, 0x666633, 0x666633},
    {0x424242, 0x424242, 0x424242, 0x666633},
    {0x424242, 0x424242, 0x424242, 0x666633},
    {0x424242, 0x424242, 0x424242, 0xFF8F00},
    {0x424242, 0,        0,        0},
};

static void calc_update_display(CalcState* st) {
    k_itoa(st->current, st->display, 10);
}

static void calc_apply(CalcState* st) {
    switch (st->op) {
    case 1: st->acc += st->current; break;
    case 2: st->acc -= st->current; break;
    case 3: st->acc *= st->current; break;
    case 4: if (st->current) st->acc /= st->current; break;
    default: st->acc = st->current; break;
    }
    st->current = st->acc;
    calc_update_display(st);
}

static void calc_press(CalcState* st, int r, int c) {
    if (r == 0 && c == 0) {
        st->acc = 0; st->current = 0; st->op = 0;
        st->new_input = true;
        calc_update_display(st);
        return;
    }
    if (r == 0 && c == 1) {
        st->current /= 10;
        calc_update_display(st);
        return;
    }
    if ((r == 0 && c == 2)) { calc_apply(st); st->op = 4; st->new_input = true; return; }
    if ((r == 0 && c == 3)) { calc_apply(st); st->op = 3; st->new_input = true; return; }
    if (r == 1 && c == 3) { calc_apply(st); st->op = 2; st->new_input = true; return; }
    if (r == 2 && c == 3) { calc_apply(st); st->op = 1; st->new_input = true; return; }
    if (r == 3 && c == 3) { calc_apply(st); st->op = 0; st->new_input = true; return; }

    int digit = -1;
    if (r == 4 && c == 0) digit = 0;
    else if (r >= 1 && r <= 3 && c <= 2) {
        int map[3][3] = {{7,8,9},{4,5,6},{1,2,3}};
        digit = map[r-1][c];
    }
    if (digit >= 0) {
        if (st->new_input) { st->current = digit; st->new_input = false; }
        else st->current = st->current * 10 + digit;
        calc_update_display(st);
    }
}

static void calculator_start(Window* win) {
    win->user_data = kmalloc(sizeof(CalcState));
    if (!win->user_data) return;
    k_memset(win->user_data, 0, sizeof(CalcState));
    CalcState* st = get_state(win);
    st->new_input = true;
    st->hover_r = -1;
    k_strcpy(st->display, "0");
}

static uint32_t lighten(uint32_t c, int amt) {
    int r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
    r += amt; g += amt; b += amt;
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    return (r << 16) | (g << 8) | b;
}

static void calculator_draw(Window* win, int x, int y, int w, int h) {
    CalcState* st = get_state(win);
    if (!st) return;

    fb_fill(x, y, w, h, 0x1E1E1E);

    fb_fill(x + 4, y + 4, w - 8, DISP_H, 0x2D2D2D);
    int tw = fb_text_width(st->display);
    fb_text_t(x + w - 12 - tw, y + 4 + (DISP_H - FONT_H) / 2, st->display, COLOR_WHITE);

    int bay = y + DISP_H + 8;
    int bah = h - DISP_H - 12;
    int bw = (w - 8 - BTN_PAD * (CALC_COLS - 1)) / CALC_COLS;
    int bh = (bah - BTN_PAD * (CALC_ROWS - 1)) / CALC_ROWS;

    for (int r = 0; r < CALC_ROWS; r++) {
        for (int c = 0; c < CALC_COLS; c++) {
            if (btn_text[r][c][0] == 0) continue;
            int bx = x + 4 + c * (bw + BTN_PAD);
            int by = bay + r * (bh + BTN_PAD);
            int cw = (r == 4 && c == 0) ? w - 8 : bw;
            uint32_t bg = btn_bg[r][c];
            if (r == st->hover_r && c == st->hover_c) bg = lighten(bg, 40);
            fb_fill(bx, by, cw, bh, bg);
            int ttw = fb_text_width(btn_text[r][c]);
            fb_text_t(bx + (cw - ttw) / 2, by + (bh - FONT_H) / 2, btn_text[r][c], COLOR_WHITE);
        }
    }
}

static void calculator_event(Window* win, Event* ev) {
    CalcState* st = get_state(win);
    if (!st) return;

    int x = win->x + BORDER_W, y = win->y + TITLE_H;
    int w = win->w - 2 * BORDER_W, h = win->h - TITLE_H - BORDER_W;
    int bay = y + DISP_H + 8;
    int bah = h - DISP_H - 12;
    int bw = (w - 8 - BTN_PAD * (CALC_COLS - 1)) / CALC_COLS;
    int bh = (bah - BTN_PAD * (CALC_ROWS - 1)) / CALC_ROWS;

    if (ev->type == EVENT_MOUSE_MOVE || ev->type == EVENT_MOUSE_DOWN) {
        st->hover_r = -1; st->hover_c = -1;
        for (int r = 0; r < CALC_ROWS; r++) {
            for (int c = 0; c < CALC_COLS; c++) {
                if (btn_text[r][c][0] == 0) continue;
                int bx = x + 4 + c * (bw + BTN_PAD);
                int by = bay + r * (bh + BTN_PAD);
                int cw = (r == 4 && c == 0) ? w - 8 : bw;
                if (ev->mouse_x >= bx && ev->mouse_x < bx + cw &&
                    ev->mouse_y >= by && ev->mouse_y < by + bh) {
                    st->hover_r = r; st->hover_c = c;
                    if (ev->type == EVENT_MOUSE_DOWN) calc_press(st, r, c);
                    return;
                }
            }
        }
    }

    if (ev->type == EVENT_KEY_DOWN) {
        char ch = ev->key_char;
        if (ch >= '0' && ch <= '9') {
            int d = ch - '0';
            if (d == 0) calc_press(st, 4, 0);
            else { calc_press(st, 3 - (d - 1) / 3, (d - 1) % 3); }
        } else if (ch == '+') calc_press(st, 2, 3);
        else if (ch == '-') calc_press(st, 1, 3);
        else if (ch == '*') calc_press(st, 0, 3);
        else if (ch == '/') calc_press(st, 0, 2);
        else if (ch == '\n') calc_press(st, 3, 3);
        else if (ch == '\b') calc_press(st, 0, 1);
        else if (ch == 27) calc_press(st, 0, 0);
    }
}

void calculator_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "calc");
    k_strcpy(def.name, "Calculator");
    def.icon_color = 0xFF9800;
    def.def_w = 260;
    def.def_h = 380;
    def.on_start = calculator_start;
    def.on_draw = calculator_draw;
    def.on_event = calculator_event;
    app_register(&def);
}
