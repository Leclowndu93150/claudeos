#include "kernel.h"

#define TETRIS_W 10
#define TETRIS_H 20
#define TETRIS_TICK 30

static const uint8_t tetris_pieces[7][4][4][4] = {
    {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},{{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
     {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},{{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
    {{{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},{{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
     {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},{{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}},
    {{{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},{{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}}},
    {{{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},{{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},{{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}}},
    {{{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},{{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
     {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},{{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}}},
    {{{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},{{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}}},
    {{{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},{{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}},
     {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}},
};

static const uint32_t piece_colors[7] = {
    0x00BCD4, 0xFFEB3B, 0x9C27B0, 0x4CAF50, 0xF44336, 0xFF9800, 0x2196F3
};

typedef struct {
    uint8_t board[TETRIS_H][TETRIS_W];
    int piece, rot, px, py;
    int next_piece;
    int score, lines, level;
    bool game_over;
    uint32_t last_tick;
    uint32_t rand_state;
} TetrisState;

static TetrisState* get_state(Window* win) {
    return (TetrisState*)win->user_data;
}

static int tetris_random(TetrisState* st) {
    st->rand_state = st->rand_state * 1103515245 + 12345;
    return ((st->rand_state >> 16) & 0x7FFF) % 7;
}

static bool tetris_fits(TetrisState* st, int piece, int rot, int px, int py) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (!tetris_pieces[piece][rot][r][c]) continue;
            int bx = px + c, by = py + r;
            if (bx < 0 || bx >= TETRIS_W || by >= TETRIS_H) return false;
            if (by >= 0 && st->board[by][bx]) return false;
        }
    }
    return true;
}

static void tetris_lock(TetrisState* st) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (!tetris_pieces[st->piece][st->rot][r][c]) continue;
            int bx = st->px + c, by = st->py + r;
            if (by >= 0 && by < TETRIS_H && bx >= 0 && bx < TETRIS_W)
                st->board[by][bx] = st->piece + 1;
        }
    }

    int cleared = 0;
    for (int r = TETRIS_H - 1; r >= 0; r--) {
        bool full = true;
        for (int c = 0; c < TETRIS_W; c++) {
            if (!st->board[r][c]) { full = false; break; }
        }
        if (full) {
            cleared++;
            for (int rr = r; rr > 0; rr--)
                k_memcpy(st->board[rr], st->board[rr-1], TETRIS_W);
            k_memset(st->board[0], 0, TETRIS_W);
            r++;
        }
    }

    if (cleared > 0) {
        int points[] = {0, 100, 300, 500, 800};
        st->score += points[cleared] * (st->level + 1);
        st->lines += cleared;
        st->level = st->lines / 10;
    }
}

static void tetris_spawn(TetrisState* st) {
    st->piece = st->next_piece;
    st->next_piece = tetris_random(st);
    st->rot = 0;
    st->px = TETRIS_W / 2 - 2;
    st->py = -1;
    if (!tetris_fits(st, st->piece, st->rot, st->px, st->py))
        st->game_over = true;
}

static void tetris_reset(TetrisState* st) {
    k_memset(st->board, 0, sizeof(st->board));
    st->score = 0;
    st->lines = 0;
    st->level = 0;
    st->game_over = false;
    st->rand_state = timer_ticks();
    st->next_piece = tetris_random(st);
    tetris_spawn(st);
    st->last_tick = timer_ticks();
}

static void tetris_start(Window* win) {
    win->user_data = kmalloc(sizeof(TetrisState));
    if (!win->user_data) return;
    k_memset(win->user_data, 0, sizeof(TetrisState));
    tetris_reset(get_state(win));
}

static void tetris_draw(Window* win, int x, int y, int w, int h) {
    TetrisState* st = get_state(win);
    if (!st) return;
    fb_fill(x, y, w, h, 0x1A1A2E);

    int cell = (h - 20) / TETRIS_H;
    if (cell < 4) cell = 4;
    int board_w = TETRIS_W * cell;
    int ox = x + (w - board_w) / 2 - 40;
    int oy = y + 10;

    fb_fill(ox - 1, oy - 1, board_w + 2, TETRIS_H * cell + 2, 0x16213E);

    for (int r = 0; r < TETRIS_H; r++) {
        for (int c = 0; c < TETRIS_W; c++) {
            if (st->board[r][c]) {
                fb_fill(ox + c * cell + 1, oy + r * cell + 1, cell - 2, cell - 2,
                        piece_colors[st->board[r][c] - 1]);
            }
        }
    }

    if (!st->game_over) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (!tetris_pieces[st->piece][st->rot][r][c]) continue;
                int bx = st->px + c, by = st->py + r;
                if (by >= 0)
                    fb_fill(ox + bx * cell + 1, oy + by * cell + 1, cell - 2, cell - 2,
                            piece_colors[st->piece]);
            }
        }
    }

    int info_x = ox + board_w + 20;
    fb_text_t(info_x, oy, "NEXT:", COLOR_WHITE);
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (tetris_pieces[st->next_piece][0][r][c]) {
                fb_fill(info_x + c * 12, oy + 20 + r * 12, 10, 10,
                        piece_colors[st->next_piece]);
            }
        }
    }

    char buf[32], num[16];

    k_strcpy(buf, "Score:");
    fb_text_t(info_x, oy + 90, buf, COLOR_WHITE);
    k_itoa(st->score, num, 10);
    fb_text_t(info_x, oy + 108, num, COLOR_WHITE);

    k_strcpy(buf, "Lines:");
    fb_text_t(info_x, oy + 136, buf, COLOR_WHITE);
    k_itoa(st->lines, num, 10);
    fb_text_t(info_x, oy + 154, num, COLOR_WHITE);

    k_strcpy(buf, "Level:");
    fb_text_t(info_x, oy + 182, buf, COLOR_WHITE);
    k_itoa(st->level, num, 10);
    fb_text_t(info_x, oy + 200, num, COLOR_WHITE);

    if (st->game_over) {
        fb_text_t(ox + board_w/2 - 60, oy + TETRIS_H*cell/2 - 8, "GAME OVER", COLOR_WHITE);
        fb_text_t(ox + board_w/2 - 80, oy + TETRIS_H*cell/2 + 12, "Press ENTER to restart", COLOR_WHITE);
    }
}

static void tetris_event(Window* win, Event* ev) {
    TetrisState* st = get_state(win);
    if (!st || ev->type != EVENT_KEY_DOWN) return;

    if (st->game_over) {
        if (ev->key_char == '\n') tetris_reset(st);
        return;
    }

    if (ev->keycode == 0x4B) {
        if (tetris_fits(st, st->piece, st->rot, st->px - 1, st->py))
            st->px--;
    } else if (ev->keycode == 0x4D) {
        if (tetris_fits(st, st->piece, st->rot, st->px + 1, st->py))
            st->px++;
    } else if (ev->keycode == 0x48) {
        int new_rot = (st->rot + 1) % 4;
        if (tetris_fits(st, st->piece, new_rot, st->px, st->py))
            st->rot = new_rot;
    } else if (ev->keycode == 0x50) {
        if (tetris_fits(st, st->piece, st->rot, st->px, st->py + 1))
            st->py++;
    } else if (ev->key_char == ' ') {
        while (tetris_fits(st, st->piece, st->rot, st->px, st->py + 1))
            st->py++;
        tetris_lock(st);
        tetris_spawn(st);
    }
}

static void tetris_tick(Window* win) {
    TetrisState* st = get_state(win);
    if (!st || st->game_over) return;

    int speed = TETRIS_TICK - st->level * 2;
    if (speed < 5) speed = 5;

    uint32_t now = timer_ticks();
    if (now - st->last_tick < (uint32_t)speed) return;
    st->last_tick = now;

    if (tetris_fits(st, st->piece, st->rot, st->px, st->py + 1)) {
        st->py++;
    } else {
        tetris_lock(st);
        tetris_spawn(st);
    }
}

void tetris_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "tetris");
    k_strcpy(def.name, "Tetris");
    def.icon_color = 0xE040FB;
    def.def_w = 340;
    def.def_h = 500;
    def.on_start = tetris_start;
    def.on_draw = tetris_draw;
    def.on_event = tetris_event;
    def.on_tick = tetris_tick;
    app_register(&def);
}
