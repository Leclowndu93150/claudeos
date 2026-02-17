#include "kernel.h"

typedef struct {
    Widget* textarea;
    Widget* save_btn;
    char file_path[FS_PATH_LEN];
    int file_idx;
} NotepadState;

static NotepadState* get_state(Window* win) {
    return (NotepadState*)win->user_data;
}

static void on_save_click(Widget* self, Window* win) {
    (void)self;
    NotepadState* st = get_state(win);
    if (st->file_idx >= 0 && st->textarea->content) {
        fs_write(st->file_idx, (const uint8_t*)st->textarea->content, st->textarea->content_len);
    }
}

static void notepad_start(Window* win) {
    win->user_data = kmalloc(sizeof(NotepadState));
    if (!win->user_data) return;
    k_memset(win->user_data, 0, sizeof(NotepadState));
    NotepadState* st = get_state(win);
    st->file_idx = -1;
    st->file_path[0] = 0;

    st->save_btn = window_add_widget(win, W_BUTTON, 4, 4, 60, 24);
    k_strcpy(st->save_btn->text, "Save");
    st->save_btn->on_click = on_save_click;

    st->textarea = window_add_widget(win, W_TEXTAREA, 0, 34, win->w - 2, win->h - TITLE_H - 36);
    st->textarea->focused = true;

    const char* arg = app_pending_arg();
    if (arg && arg[0]) {
        k_strncpy(st->file_path, arg, FS_PATH_LEN - 1);
        st->file_idx = fs_find(arg);
        if (st->file_idx >= 0) {
            FSNode* node = fs_get(st->file_idx);
            if (node && node->data && st->textarea->content) {
                int len = node->size;
                if (len >= MAX_TEXT) len = MAX_TEXT - 1;
                k_memcpy(st->textarea->content, node->data, len);
                st->textarea->content[len] = 0;
                st->textarea->content_len = len;
            }
            k_strcpy(win->title, "Notepad - ");
            k_strcat(win->title, node->name);
        }
    }
}

static void notepad_draw(Window* win, int x, int y, int w, int h) {
    (void)x; (void)y;
    NotepadState* st = get_state(win);
    st->textarea->w = w - 2;
    st->textarea->h = h - 36;
}

static void notepad_event(Window* win, Event* ev) {
    (void)win; (void)ev;
}

static void notepad_close(Window* win) {
    (void)win;
}

void notepad_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "notepad");
    k_strcpy(def.name, "Notepad");
    def.icon_color = 0x4FC3F7;
    def.def_w = 500;
    def.def_h = 400;
    def.on_start = notepad_start;
    def.on_draw = notepad_draw;
    def.on_event = notepad_event;
    def.on_close = notepad_close;
    app_register(&def);
}
