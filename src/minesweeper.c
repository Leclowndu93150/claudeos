#include "kernel.h"

#define MINE_W 16
#define MINE_H 16
#define MINE_COUNT 40
#define CELL_HIDDEN 0
#define CELL_REVEALED 1
#define CELL_FLAGGED 2

typedef struct {
    uint8_t mines[MINE_H][MINE_W];
    uint8_t state[MINE_H][MINE_W];
    uint8_t adj[MINE_H][MINE_W];
    bool game_over;
    bool won;
    bool started;
    int revealed;
    int flags;
    uint32_t start_time;
    uint32_t rand_state;
} MineState;

static MineState* get_state(Window* win) {
    return (MineState*)win->user_data;
}

static int mine_rand(MineState* st) {
    st->rand_state = st->rand_state * 1103515245 + 12345;
    return (st->rand_state >> 16) & 0x7FFF;
}

static void count_adjacent(MineState* st) {
    for (int r = 0; r < MINE_H; r++) {
        for (int c = 0; c < MINE_W; c++) {
            int n = 0;
            for (int dr = -1; dr <= 1; dr++) {
                for (int dc = -1; dc <= 1; dc++) {
                    int nr = r + dr, nc = c + dc;
                    if (nr >= 0 && nr < MINE_H && nc >= 0 && nc < MINE_W && st->mines[nr][nc])
                        n++;
                }
            }
            st->adj[r][c] = n;
        }
    }
}

static void place_mines(MineState* st, int safe_r, int safe_c) {
    k_memset(st->mines, 0, sizeof(st->mines));
    int placed = 0;
    while (placed < MINE_COUNT) {
        int r = mine_rand(st) % MINE_H;
        int c = mine_rand(st) % MINE_W;
        if (st->mines[r][c]) continue;
        if (r >= safe_r - 1 && r <= safe_r + 1 && c >= safe_c - 1 && c <= safe_c + 1) continue;
        st->mines[r][c] = 1;
        placed++;
    }
    count_adjacent(st);
}

static void flood_reveal(MineState* st, int r, int c) {
    if (r < 0 || r >= MINE_H || c < 0 || c >= MINE_W) return;
    if (st->state[r][c] != CELL_HIDDEN) return;
    st->state[r][c] = CELL_REVEALED;
    st->revealed++;
    if (st->adj[r][c] == 0) {
        for (int dr = -1; dr <= 1; dr++)
            for (int dc = -1; dc <= 1; dc++)
                flood_reveal(st, r + dr, c + dc);
    }
}

static void mine_reset(MineState* st) {
    k_memset(st->mines, 0, sizeof(st->mines));
    k_memset(st->state, 0, sizeof(st->state));
    k_memset(st->adj, 0, sizeof(st->adj));
    st->game_over = false;
    st->won = false;
    st->started = false;
    st->revealed = 0;
    st->flags = 0;
    st->rand_state = timer_ticks();
}

static void mine_start(Window* win) {
    win->user_data = kmalloc(sizeof(MineState));
    if (!win->user_data) return;
    k_memset(win->user_data, 0, sizeof(MineState));
    MineState* st = get_state(win);
    st->rand_state = timer_ticks();
    mine_reset(st);
}

static const uint32_t num_colors[] = {
    0, 0x0000FF, 0x008000, 0xFF0000, 0x000080,
    0x800000, 0x008080, 0x000000, 0x808080
};

static void mine_draw(Window* win, int x, int y, int w, int h) {
    MineState* st = get_state(win);
    if (!st) return;

    fb_fill(x, y, w, h, 0xC0C0C0);

    int cell = w / MINE_W;
    int cell_h = (h - 30) / MINE_H;
    if (cell_h < cell) cell = cell_h;
    if (cell < 4) cell = 4;

    int ox = x + (w - MINE_W * cell) / 2;
    int oy = y + 28;

    char buf[32], num[16];
    k_strcpy(buf, "Mines: ");
    k_itoa(MINE_COUNT - st->flags, num, 10);
    k_strcat(buf, num);
    fb_text_t(x + 8, y + 6, buf, COLOR_TEXT);

    if (st->game_over || st->won) {
        fb_text_t(x + w / 2 - 40, y + 6, st->won ? "YOU WIN!" : "BOOM!", st->won ? 0x008000 : 0xFF0000);
        fb_text_t(x + w - 120, y + 6, "ENTER=Restart", COLOR_TEXT_DIM);
    }

    for (int r = 0; r < MINE_H; r++) {
        for (int c = 0; c < MINE_W; c++) {
            int cx = ox + c * cell, cy = oy + r * cell;

            if (st->state[r][c] == CELL_REVEALED) {
                fb_fill(cx, cy, cell - 1, cell - 1, 0xE0E0E0);
                if (st->mines[r][c]) {
                    fb_fill(cx + 2, cy + 2, cell - 5, cell - 5, 0x000000);
                } else if (st->adj[r][c] > 0) {
                    char n[2] = { '0' + st->adj[r][c], 0 };
                    fb_text_t(cx + (cell - FONT_W) / 2, cy + (cell - FONT_H) / 2, n,
                              num_colors[st->adj[r][c]]);
                }
            } else if (st->state[r][c] == CELL_FLAGGED) {
                fb_fill(cx, cy, cell - 1, cell - 1, 0xB0B0B0);
                fb_text_t(cx + (cell - FONT_W) / 2, cy + (cell - FONT_H) / 2, "F", 0xFF0000);
            } else {
                fb_fill(cx, cy, cell - 1, cell - 1, 0xB0B0B0);
                if (st->game_over && st->mines[r][c]) {
                    fb_fill(cx + 2, cy + 2, cell - 5, cell - 5, 0x000000);
                }
            }
        }
    }
}

static void mine_event(Window* win, Event* ev) {
    MineState* st = get_state(win);
    if (!st) return;

    if (ev->type == EVENT_KEY_DOWN && ev->key_char == '\n' && (st->game_over || st->won)) {
        mine_reset(st);
        return;
    }

    if (st->game_over || st->won) return;

    int x = win->x + BORDER_W, y = win->y + TITLE_H;
    int w = win->w - 2 * BORDER_W, h = win->h - TITLE_H - BORDER_W;
    int cell = w / MINE_W;
    int cell_h = (h - 30) / MINE_H;
    if (cell_h < cell) cell = cell_h;
    if (cell < 4) cell = 4;
    int ox = x + (w - MINE_W * cell) / 2;
    int oy = y + 28;

    if (ev->type == EVENT_MOUSE_DOWN) {
        int c = (ev->mouse_x - ox) / cell;
        int r = (ev->mouse_y - oy) / cell;
        if (r < 0 || r >= MINE_H || c < 0 || c >= MINE_W) return;

        if (ev->mouse_button == 1) {
            if (st->state[r][c] == CELL_HIDDEN) {
                st->state[r][c] = CELL_FLAGGED;
                st->flags++;
            } else if (st->state[r][c] == CELL_FLAGGED) {
                st->state[r][c] = CELL_HIDDEN;
                st->flags--;
            }
            return;
        }

        if (st->state[r][c] != CELL_HIDDEN) return;

        if (!st->started) {
            st->started = true;
            st->start_time = timer_ticks();
            place_mines(st, r, c);
        }

        if (st->mines[r][c]) {
            st->game_over = true;
            for (int rr = 0; rr < MINE_H; rr++)
                for (int cc = 0; cc < MINE_W; cc++)
                    if (st->mines[rr][cc]) st->state[rr][cc] = CELL_REVEALED;
            return;
        }

        flood_reveal(st, r, c);

        if (st->revealed == MINE_W * MINE_H - MINE_COUNT)
            st->won = true;
    }
}

void minesweeper_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "mines");
    k_strcpy(def.name, "Minesweeper");
    def.icon_color = 0x607D8B;
    def.def_w = 420;
    def.def_h = 480;
    def.on_start = mine_start;
    def.on_draw = mine_draw;
    def.on_event = mine_event;
    app_register(&def);
}
