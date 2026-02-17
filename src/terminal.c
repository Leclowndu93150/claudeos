#include "kernel.h"

#define TERM_BUF 16384
#define TERM_INPUT_MAX 256
#define TERM_HIST_MAX 32

typedef struct {
    char buf[TERM_BUF];
    int buf_len;
    int scroll;
    char input[TERM_INPUT_MAX];
    int input_len;
    int cursor;
    char hist[TERM_HIST_MAX][TERM_INPUT_MAX];
    int hist_count;
    int hist_pos;
    int cwd_idx;
} TermState;

static TermState* get_state(Window* win) {
    return (TermState*)win->user_data;
}

static void term_print(TermState* st, const char* s) {
    while (*s && st->buf_len < TERM_BUF - 1)
        st->buf[st->buf_len++] = *s++;
    st->buf[st->buf_len] = 0;
}

static void term_print_num(TermState* st, int val) {
    char num[16];
    k_itoa(val, num, 10);
    term_print(st, num);
}

static void build_cwd_path(TermState* st, char* out) {
    if (st->cwd_idx == 0) { k_strcpy(out, "/"); return; }
    char parts[16][FS_NAME_LEN];
    int depth = 0, idx = st->cwd_idx;
    while (idx > 0 && depth < 16) {
        k_strcpy(parts[depth++], fs_get(idx)->name);
        idx = fs_get(idx)->parent;
    }
    k_strcpy(out, "/");
    for (int d = depth - 1; d >= 0; d--) {
        k_strcat(out, parts[d]);
        if (d > 0) k_strcat(out, "/");
    }
}

static void build_full_path(TermState* st, const char* name, char* out) {
    build_cwd_path(st, out);
    if (k_strcmp(out, "/") != 0) k_strcat(out, "/");
    k_strcat(out, name);
}

static void term_prompt(TermState* st) {
    term_print(st, "root@claudeos:");
    char path[FS_PATH_LEN];
    build_cwd_path(st, path);
    term_print(st, path);
    term_print(st, "$ ");
}

static void cmd_neofetch(TermState* st) {
    term_print(st, "\n");
    term_print(st, "    ___ _             _        \n");
    term_print(st, "   / __| | __ _ _  _ | |___    root@claudeos\n");
    term_print(st, "  | (  | |/ _` || || / _` |   ---------------\n");
    term_print(st, "   \\___|_|\\__,_| \\_,_\\__,_|   OS: Claude OS 1.0\n");
    term_print(st, "    ___  ___                   Kernel: x86-32 custom\n");
    term_print(st, "   / _ \\/ __|                  Shell: claude-sh\n");
    term_print(st, "  | (_) \\__ \\                  Resolution: 1024x768x32\n");
    term_print(st, "   \\___/|___/                  ");

    term_print(st, "Uptime: ");
    uint32_t t = timer_ticks() / 100;
    term_print_num(st, t / 3600); term_print(st, "h ");
    term_print_num(st, (t / 60) % 60); term_print(st, "m ");
    term_print_num(st, t % 60); term_print(st, "s\n");

    term_print(st, "                               Memory: ");
    uint32_t total, used, free_mem;
    mem_stats(&total, &used, &free_mem);
    term_print_num(st, used / 1024); term_print(st, "K / ");
    term_print_num(st, total / 1024); term_print(st, "K\n\n");
}

static void term_exec(TermState* st, const char* cmd) {
    while (*cmd == ' ') cmd++;
    if (cmd[0] == 0) return;

    if (st->hist_count < TERM_HIST_MAX)
        k_strcpy(st->hist[st->hist_count++], cmd);
    st->hist_pos = st->hist_count;

    if (k_strcmp(cmd, "help") == 0) {
        term_print(st, "Commands:\n");
        term_print(st, "  help      - Show this help\n");
        term_print(st, "  clear     - Clear screen\n");
        term_print(st, "  echo TEXT - Print text\n");
        term_print(st, "  ls        - List directory\n");
        term_print(st, "  cd DIR    - Change directory\n");
        term_print(st, "  cat FILE  - Print file contents\n");
        term_print(st, "  pwd       - Print working directory\n");
        term_print(st, "  mkdir DIR - Create directory\n");
        term_print(st, "  touch FILE- Create empty file\n");
        term_print(st, "  whoami    - Current user\n");
        term_print(st, "  date      - Show time\n");
        term_print(st, "  uptime    - Show uptime\n");
        term_print(st, "  neofetch  - System info\n");
    } else if (k_strcmp(cmd, "clear") == 0 || k_strcmp(cmd, "cls") == 0) {
        st->buf_len = 0; st->buf[0] = 0; st->scroll = 0;
        return;
    } else if (k_strncmp(cmd, "echo ", 5) == 0) {
        term_print(st, cmd + 5);
        term_print(st, "\n");
    } else if (k_strcmp(cmd, "echo") == 0) {
        term_print(st, "\n");
    } else if (k_strcmp(cmd, "ls") == 0 || k_strncmp(cmd, "ls ", 3) == 0) {
        int dir = st->cwd_idx;
        int children[64];
        int count = fs_list(dir, children, 64);
        for (int i = 0; i < count; i++) {
            FSNode* node = fs_get(children[i]);
            if (!node) continue;
            term_print(st, node->name);
            if (node->is_dir) term_print(st, "/");
            term_print(st, "  ");
        }
        if (count > 0) term_print(st, "\n");
    } else if (k_strncmp(cmd, "cd ", 3) == 0) {
        const char* arg = cmd + 3;
        while (*arg == ' ') arg++;
        if (k_strcmp(arg, "..") == 0) {
            FSNode* n = fs_get(st->cwd_idx);
            if (n && n->parent >= 0) st->cwd_idx = n->parent;
        } else if (k_strcmp(arg, "/") == 0) {
            st->cwd_idx = 0;
        } else {
            int children[64];
            int count = fs_list(st->cwd_idx, children, 64);
            bool found = false;
            for (int i = 0; i < count; i++) {
                FSNode* n = fs_get(children[i]);
                if (n && n->is_dir && k_strcmp(n->name, arg) == 0) {
                    st->cwd_idx = children[i];
                    found = true;
                    break;
                }
            }
            if (!found) {
                term_print(st, "cd: no such directory: ");
                term_print(st, arg); term_print(st, "\n");
            }
        }
    } else if (k_strncmp(cmd, "cat ", 4) == 0) {
        const char* fname = cmd + 4;
        while (*fname == ' ') fname++;
        int children[64];
        int count = fs_list(st->cwd_idx, children, 64);
        bool found = false;
        for (int i = 0; i < count; i++) {
            FSNode* n = fs_get(children[i]);
            if (n && !n->is_dir && k_strcmp(n->name, fname) == 0) {
                if (n->data) { term_print(st, (const char*)n->data); term_print(st, "\n"); }
                found = true; break;
            }
        }
        if (!found) {
            term_print(st, "cat: "); term_print(st, fname);
            term_print(st, ": No such file\n");
        }
    } else if (k_strcmp(cmd, "pwd") == 0) {
        char path[FS_PATH_LEN];
        build_cwd_path(st, path);
        term_print(st, path); term_print(st, "\n");
    } else if (k_strncmp(cmd, "mkdir ", 6) == 0) {
        const char* name = cmd + 6;
        while (*name == ' ') name++;
        char full[FS_PATH_LEN];
        build_full_path(st, name, full);
        if (fs_mkdir(full) >= 0) { term_print(st, "Created: "); term_print(st, name); term_print(st, "\n"); }
        else { term_print(st, "mkdir: failed\n"); }
    } else if (k_strncmp(cmd, "touch ", 6) == 0) {
        const char* name = cmd + 6;
        while (*name == ' ') name++;
        char full[FS_PATH_LEN];
        build_full_path(st, name, full);
        if (fs_mkfile(full, (const uint8_t*)"", 0) >= 0) {
            term_print(st, "Created: "); term_print(st, name); term_print(st, "\n");
        } else { term_print(st, "touch: failed\n"); }
    } else if (k_strcmp(cmd, "whoami") == 0) {
        term_print(st, "root\n");
    } else if (k_strcmp(cmd, "date") == 0 || k_strcmp(cmd, "uptime") == 0) {
        uint32_t t = timer_ticks() / 100;
        term_print(st, "Up ");
        term_print_num(st, t / 3600); term_print(st, "h ");
        term_print_num(st, (t / 60) % 60); term_print(st, "m ");
        term_print_num(st, t % 60); term_print(st, "s\n");
    } else if (k_strcmp(cmd, "neofetch") == 0) {
        cmd_neofetch(st);
        return;
    } else if (k_strcmp(cmd, "exit") == 0) {
        term_print(st, "Use the X button to close.\n");
    } else {
        term_print(st, cmd); term_print(st, ": command not found\n");
    }
}

static void terminal_start(Window* win) {
    win->user_data = kmalloc(sizeof(TermState));
    if (!win->user_data) return;
    k_memset(win->user_data, 0, sizeof(TermState));
    TermState* st = get_state(win);
    st->cwd_idx = 0;
    term_print(st, "Claude OS Terminal v1.0\n");
    term_print(st, "Type 'help' for commands, 'neofetch' for system info.\n\n");
    term_prompt(st);
}

static void terminal_draw(Window* win, int x, int y, int w, int h) {
    TermState* st = get_state(win);
    if (!st) return;

    fb_fill(x, y, w, h, 0x0C0C1A);

    int lh = FONT_H + 2;
    int max_vis = h / lh;

    char* lines[1024];
    int line_lens[1024];
    int line_count = 0;
    char* p = st->buf;
    lines[0] = p;
    while (*p && line_count < 1023) {
        if (*p == '\n') {
            line_lens[line_count] = (int)(p - lines[line_count]);
            line_count++;
            lines[line_count] = p + 1;
        }
        p++;
    }
    line_lens[line_count] = (int)(p - lines[line_count]);
    if (line_lens[line_count] > 0 || line_count == 0) line_count++;

    if (st->scroll > line_count - max_vis) st->scroll = line_count - max_vis;
    if (st->scroll < 0) st->scroll = 0;

    int start = line_count - max_vis - st->scroll;
    if (start < 0) start = 0;

    fb_clip_set(x, y, w, h);
    for (int i = start; i < line_count && (i - start) < max_vis; i++) {
        int ty = y + (i - start) * lh;
        char buf[512];
        int len = line_lens[i] < 510 ? line_lens[i] : 510;
        k_memcpy(buf, lines[i], len);
        buf[len] = 0;

        bool is_last = (i == line_count - 1);
        if (is_last && st->input_len > 0) {
            int remain = 510 - len;
            int ilen = st->input_len < remain ? st->input_len : remain;
            k_memcpy(buf + len, st->input, ilen);
            buf[len + ilen] = 0;
        }

        uint32_t color = 0xCCCCCC;
        if (buf[0] == 'r' && k_strncmp(buf, "root@", 5) == 0) color = 0x4CAF50;
        fb_text_t(x + 4, ty, buf, color);

        if (is_last && (timer_ticks() / 50) % 2 == 0) {
            int cx = x + 4 + (len + st->cursor) * FONT_W;
            for (int r = ty; r < ty + FONT_H; r++)
                fb_pixel(cx, r, 0x4CAF50);
        }
    }
    fb_clip_reset();
}

static void terminal_event(Window* win, Event* ev) {
    TermState* st = get_state(win);
    if (!st || ev->type != EVENT_KEY_DOWN) return;

    if (ev->keycode == 0x48) {
        if (st->hist_pos > 0) {
            st->hist_pos--;
            k_strcpy(st->input, st->hist[st->hist_pos]);
            st->input_len = k_strlen(st->input);
            st->cursor = st->input_len;
        }
        return;
    }
    if (ev->keycode == 0x50) {
        if (st->hist_pos < st->hist_count - 1) {
            st->hist_pos++;
            k_strcpy(st->input, st->hist[st->hist_pos]);
            st->input_len = k_strlen(st->input);
            st->cursor = st->input_len;
        } else {
            st->hist_pos = st->hist_count;
            st->input[0] = 0; st->input_len = 0; st->cursor = 0;
        }
        return;
    }
    if (ev->keycode == 0x4B) {
        if (st->cursor > 0) st->cursor--;
        return;
    }
    if (ev->keycode == 0x4D) {
        if (st->cursor < st->input_len) st->cursor++;
        return;
    }

    if (ev->key_char == '\n') {
        term_print(st, st->input);
        term_print(st, "\n");
        term_exec(st, st->input);
        st->input[0] = 0; st->input_len = 0; st->cursor = 0;
        if (!(st->buf_len > 0 && st->buf[st->buf_len-1] == ' ' && st->buf_len >= 2 && st->buf[st->buf_len-2] == '$'))
            term_prompt(st);
        st->scroll = 0;
        return;
    }

    if (ev->key_char == '\b') {
        if (st->cursor > 0) {
            k_memmove(st->input + st->cursor - 1, st->input + st->cursor, st->input_len - st->cursor + 1);
            st->input_len--;
            st->cursor--;
        }
        return;
    }

    if (ev->key_char >= 32 && ev->key_char <= 126 && st->input_len < TERM_INPUT_MAX - 1) {
        k_memmove(st->input + st->cursor + 1, st->input + st->cursor, st->input_len - st->cursor + 1);
        st->input[st->cursor] = ev->key_char;
        st->input_len++;
        st->cursor++;
    }
}

void terminal_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "terminal");
    k_strcpy(def.name, "Terminal");
    def.icon_color = 0x333333;
    def.def_w = 640;
    def.def_h = 420;
    def.on_start = terminal_start;
    def.on_draw = terminal_draw;
    def.on_event = terminal_event;
    app_register(&def);
}
