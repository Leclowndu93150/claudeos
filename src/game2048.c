#include "kernel.h"

typedef struct {
    int grid[4][4];
    int score;
    bool game_over;
    bool won;
    uint32_t rand_state;
} State2048;

static State2048* get_state(Window* win) {
    return (State2048*)win->user_data;
}

static int rand2048(State2048* st) {
    st->rand_state = st->rand_state * 1103515245 + 12345;
    return (st->rand_state >> 16) & 0x7FFF;
}

static void spawn_tile(State2048* st) {
    int empty[16], n = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (st->grid[r][c] == 0) empty[n++] = r * 4 + c;
    if (n == 0) return;
    int idx = empty[rand2048(st) % n];
    st->grid[idx / 4][idx % 4] = (rand2048(st) % 10 < 9) ? 2 : 4;
}

static bool can_move(State2048* st) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            if (st->grid[r][c] == 0) return true;
            if (c < 3 && st->grid[r][c] == st->grid[r][c+1]) return true;
            if (r < 3 && st->grid[r][c] == st->grid[r+1][c]) return true;
        }
    return false;
}

static bool slide_row(int row[4], int* score) {
    bool moved = false;
    int out[4] = {0};
    int pos = 0;
    for (int i = 0; i < 4; i++) {
        if (row[i] == 0) continue;
        if (pos > 0 && out[pos - 1] == row[i]) {
            out[pos - 1] *= 2;
            *score += out[pos - 1];
            moved = true;
        } else {
            if (pos != i) moved = true;
            out[pos++] = row[i];
        }
    }
    k_memcpy(row, out, sizeof(out));
    return moved;
}

static bool do_move(State2048* st, int dir) {
    bool moved = false;
    int temp[4];
    for (int i = 0; i < 4; i++) {
        switch (dir) {
        case 0:
            for (int j = 0; j < 4; j++) temp[j] = st->grid[j][i];
            if (slide_row(temp, &st->score)) { moved = true; for (int j = 0; j < 4; j++) st->grid[j][i] = temp[j]; }
            break;
        case 1:
            for (int j = 0; j < 4; j++) temp[j] = st->grid[3-j][i];
            if (slide_row(temp, &st->score)) { moved = true; for (int j = 0; j < 4; j++) st->grid[3-j][i] = temp[j]; }
            break;
        case 2:
            if (slide_row(st->grid[i], &st->score)) moved = true;
            break;
        case 3:
            for (int j = 0; j < 4; j++) temp[j] = st->grid[i][3-j];
            if (slide_row(temp, &st->score)) { moved = true; for (int j = 0; j < 4; j++) st->grid[i][3-j] = temp[j]; }
            break;
        }
    }
    return moved;
}

static void reset_2048(State2048* st) {
    k_memset(st->grid, 0, sizeof(st->grid));
    st->score = 0;
    st->game_over = false;
    st->won = false;
    st->rand_state = timer_ticks();
    spawn_tile(st);
    spawn_tile(st);
}

static void game2048_start(Window* win) {
    win->user_data = kmalloc(sizeof(State2048));
    if (!win->user_data) return;
    k_memset(win->user_data, 0, sizeof(State2048));
    reset_2048(get_state(win));
}

static uint32_t tile_color(int val) {
    switch (val) {
    case 2:    return 0xEEE4DA;
    case 4:    return 0xEDE0C8;
    case 8:    return 0xF2B179;
    case 16:   return 0xF59563;
    case 32:   return 0xF67C5F;
    case 64:   return 0xF65E3B;
    case 128:  return 0xEDCF72;
    case 256:  return 0xEDCC61;
    case 512:  return 0xEDC850;
    case 1024: return 0xEDC53F;
    case 2048: return 0xEDC22E;
    default:   return 0x3C3A32;
    }
}

static uint32_t tile_text_color(int val) {
    return val <= 4 ? 0x776E65 : 0xF9F6F2;
}

static void game2048_draw(Window* win, int x, int y, int w, int h) {
    State2048* st = get_state(win);
    if (!st) return;

    fb_fill(x, y, w, h, 0xFAF8EF);

    char buf[32], num[16];
    k_strcpy(buf, "Score: ");
    k_itoa(st->score, num, 10);
    k_strcat(buf, num);
    fb_text_t(x + 8, y + 6, buf, 0x776E65);

    int pad = 8;
    int board_s = w - 16;
    int board_h_max = h - 36;
    if (board_h_max < board_s) board_s = board_h_max;
    int cell = (board_s - pad * 5) / 4;
    int board_w = cell * 4 + pad * 5;
    int bx = x + (w - board_w) / 2;
    int by = y + 30;

    fb_fill(bx, by, board_w, board_w, 0xBBADA0);

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int cx = bx + pad + c * (cell + pad);
            int cy = by + pad + r * (cell + pad);
            int val = st->grid[r][c];
            fb_fill(cx, cy, cell, cell, val ? tile_color(val) : 0xCDC1B4);
            if (val) {
                k_itoa(val, num, 10);
                int tw = fb_text_width(num);
                fb_text_t(cx + (cell - tw) / 2, cy + (cell - FONT_H) / 2, num, tile_text_color(val));
            }
        }
    }

    if (st->game_over) {
        fb_text_t(bx + board_w / 2 - 60, by + board_w / 2 - 8, "Game Over!", 0x776E65);
        fb_text_t(bx + board_w / 2 - 80, by + board_w / 2 + 12, "Press ENTER to restart", 0x776E65);
    }
    if (st->won) {
        fb_text_t(bx + board_w / 2 - 36, by + board_w / 2 - 8, "You Win!", 0xEDC22E);
    }
}

static void game2048_event(Window* win, Event* ev) {
    State2048* st = get_state(win);
    if (!st || ev->type != EVENT_KEY_DOWN) return;

    if (st->game_over) {
        if (ev->key_char == '\n') reset_2048(st);
        return;
    }

    int dir = -1;
    if (ev->keycode == 0x48) dir = 0;
    else if (ev->keycode == 0x50) dir = 1;
    else if (ev->keycode == 0x4B) dir = 2;
    else if (ev->keycode == 0x4D) dir = 3;

    if (dir >= 0) {
        if (do_move(st, dir)) {
            spawn_tile(st);
            for (int r = 0; r < 4; r++)
                for (int c = 0; c < 4; c++)
                    if (st->grid[r][c] == 2048) st->won = true;
            if (!can_move(st)) st->game_over = true;
        }
    }
}

void game2048_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "2048");
    k_strcpy(def.name, "2048");
    def.icon_color = 0xEDC22E;
    def.def_w = 340;
    def.def_h = 400;
    def.on_start = game2048_start;
    def.on_draw = game2048_draw;
    def.on_event = game2048_event;
    app_register(&def);
}
