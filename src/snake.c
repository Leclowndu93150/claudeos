#include "kernel.h"

#define SNAKE_GRID_W 30
#define SNAKE_GRID_H 20
#define SNAKE_MAX    600
#define SNAKE_TICK   8

typedef struct {
    int sx[SNAKE_MAX], sy[SNAKE_MAX];
    int len;
    int dx, dy;
    int food_x, food_y;
    int score;
    bool game_over;
    uint32_t last_tick;
} SnakeState;

static SnakeState* get_state(Window* win) {
    return (SnakeState*)win->user_data;
}

static uint32_t snake_rand(uint32_t seed) {
    return seed * 1103515245 + 12345;
}

static void place_food(SnakeState* st) {
    uint32_t r = snake_rand(timer_ticks() ^ (uint32_t)(st->score * 7 + st->len));
    for (int attempts = 0; attempts < 100; attempts++) {
        int fx = (r >> 4) % SNAKE_GRID_W;
        int fy = (r >> 12) % SNAKE_GRID_H;
        bool on_snake = false;
        for (int i = 0; i < st->len; i++) {
            if (st->sx[i] == fx && st->sy[i] == fy) { on_snake = true; break; }
        }
        if (!on_snake) {
            st->food_x = fx;
            st->food_y = fy;
            return;
        }
        r = snake_rand(r);
    }
    st->food_x = 0;
    st->food_y = 0;
}

static void snake_reset(SnakeState* st) {
    st->len = 3;
    st->dx = 1; st->dy = 0;
    st->score = 0;
    st->game_over = false;
    for (int i = 0; i < st->len; i++) {
        st->sx[i] = 5 - i;
        st->sy[i] = SNAKE_GRID_H / 2;
    }
    place_food(st);
    st->last_tick = timer_ticks();
}

static void snake_start(Window* win) {
    win->user_data = kmalloc(sizeof(SnakeState));
    if (!win->user_data) return;
    k_memset(win->user_data, 0, sizeof(SnakeState));
    snake_reset(get_state(win));
}

static void snake_draw(Window* win, int x, int y, int w, int h) {
    SnakeState* st = get_state(win);
    if (!st) return;
    int cell_w = w / SNAKE_GRID_W;
    int cell_h = (h - 30) / SNAKE_GRID_H;
    int cell = cell_w < cell_h ? cell_w : cell_h;
    if (cell < 2) cell = 2;
    int ox = x + (w - SNAKE_GRID_W * cell) / 2;
    int oy = y + 24;

    fb_fill(x, y, w, h, 0x1B5E20);
    fb_fill(ox - 1, oy - 1, SNAKE_GRID_W * cell + 2, SNAKE_GRID_H * cell + 2, 0x2E7D32);

    for (int i = 0; i < st->len; i++) {
        uint32_t color = (i == 0) ? 0x76FF03 : 0x4CAF50;
        fb_fill(ox + st->sx[i] * cell + 1, oy + st->sy[i] * cell + 1, cell - 2, cell - 2, color);
    }

    fb_fill(ox + st->food_x * cell + 1, oy + st->food_y * cell + 1, cell - 2, cell - 2, 0xFF1744);

    char score_buf[32] = "Score: ";
    char num[16];
    k_itoa(st->score, num, 10);
    k_strcat(score_buf, num);
    fb_text_t(x + 8, y + 4, score_buf, COLOR_WHITE);

    if (st->game_over) {
        fb_text_t(x + w/2 - 60, y + h/2 - 8, "GAME OVER", COLOR_WHITE);
        fb_text_t(x + w/2 - 80, y + h/2 + 12, "Press ENTER to restart", COLOR_WHITE);
    }
}

static void snake_event(Window* win, Event* ev) {
    SnakeState* st = get_state(win);
    if (!st || ev->type != EVENT_KEY_DOWN) return;

    if (st->game_over) {
        if (ev->key_char == '\n') snake_reset(st);
        return;
    }

    if (ev->keycode == 0x48 && st->dy != 1) { st->dx = 0; st->dy = -1; }
    else if (ev->keycode == 0x50 && st->dy != -1) { st->dx = 0; st->dy = 1; }
    else if (ev->keycode == 0x4B && st->dx != 1) { st->dx = -1; st->dy = 0; }
    else if (ev->keycode == 0x4D && st->dx != -1) { st->dx = 1; st->dy = 0; }
}

static void snake_tick(Window* win) {
    SnakeState* st = get_state(win);
    if (!st || st->game_over) return;

    uint32_t now = timer_ticks();
    if (now - st->last_tick < SNAKE_TICK) return;
    st->last_tick = now;

    int nx = st->sx[0] + st->dx;
    int ny = st->sy[0] + st->dy;

    if (nx < 0 || nx >= SNAKE_GRID_W || ny < 0 || ny >= SNAKE_GRID_H) {
        st->game_over = true;
        return;
    }

    for (int i = 0; i < st->len; i++) {
        if (st->sx[i] == nx && st->sy[i] == ny) {
            st->game_over = true;
            return;
        }
    }

    bool ate = (nx == st->food_x && ny == st->food_y);

    if (!ate) {
        for (int i = st->len - 1; i > 0; i--) {
            st->sx[i] = st->sx[i-1];
            st->sy[i] = st->sy[i-1];
        }
    } else {
        if (st->len < SNAKE_MAX) {
            for (int i = st->len; i > 0; i--) {
                st->sx[i] = st->sx[i-1];
                st->sy[i] = st->sy[i-1];
            }
            st->len++;
        }
        st->score += 10;
        place_food(st);
    }

    st->sx[0] = nx;
    st->sy[0] = ny;
}

void snake_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "snake");
    k_strcpy(def.name, "Snake");
    def.icon_color = 0x4CAF50;
    def.def_w = 500;
    def.def_h = 420;
    def.on_start = snake_start;
    def.on_draw = snake_draw;
    def.on_event = snake_event;
    def.on_tick = snake_tick;
    app_register(&def);
}
