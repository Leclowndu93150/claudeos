#include "kernel.h"

typedef struct {
    int current_dir;
    char path[FS_PATH_LEN];
    Widget* path_label;
    Widget* list;
    Widget* up_btn;
} ExplorerState;

static ExplorerState* get_state(Window* win) {
    return (ExplorerState*)win->user_data;
}

static void refresh_list(Window* win) {
    ExplorerState* st = get_state(win);
    st->list->item_count = 0;

    int children[64];
    int count = fs_list(st->current_dir, children, 64);
    for (int i = 0; i < count && i < MAX_ITEMS; i++) {
        FSNode* node = fs_get(children[i]);
        if (!node) continue;
        if (node->is_dir) {
            char buf[128];
            k_strcpy(buf, "[DIR] ");
            k_strcat(buf, node->name);
            k_strncpy(st->list->items[st->list->item_count], buf, 127);
            st->list->item_icons[st->list->item_count] = 0xFFC832;
        } else {
            k_strncpy(st->list->items[st->list->item_count], node->name, 127);
            st->list->item_icons[st->list->item_count] = 0x90CAF9;
        }
        st->list->item_count++;
    }
    st->list->selected = -1;
    st->list->list_scroll = 0;
}

static void build_path(int dir_idx, char* out) {
    if (dir_idx == 0) {
        k_strcpy(out, "/");
        return;
    }
    char parts[16][FS_NAME_LEN];
    int depth = 0;
    int idx = dir_idx;
    while (idx > 0 && depth < 16) {
        k_strcpy(parts[depth++], fs_get(idx)->name);
        idx = fs_get(idx)->parent;
    }
    out[0] = '/';
    out[1] = 0;
    for (int d = depth - 1; d >= 0; d--) {
        k_strcat(out, parts[d]);
        if (d > 0) k_strcat(out, "/");
    }
}

static void on_up_click(Widget* self, Window* win) {
    (void)self;
    ExplorerState* st = get_state(win);
    FSNode* node = fs_get(st->current_dir);
    if (node && node->parent >= 0) {
        st->current_dir = node->parent;
        build_path(st->current_dir, st->path);
        k_strcpy(st->path_label->text, st->path);
        refresh_list(win);
    }
}

static void on_item_dblclick(Widget* self, Window* win, int idx) {
    (void)self;
    ExplorerState* st = get_state(win);

    int children[64];
    int count = fs_list(st->current_dir, children, 64);
    if (idx < 0 || idx >= count) return;

    FSNode* node = fs_get(children[idx]);
    if (!node) return;

    if (node->is_dir) {
        st->current_dir = children[idx];
        build_path(st->current_dir, st->path);
        k_strcpy(st->path_label->text, st->path);
        refresh_list(win);
    } else {
        char full_path[FS_PATH_LEN];
        build_path(st->current_dir, full_path);
        if (k_strcmp(full_path, "/") != 0) k_strcat(full_path, "/");
        k_strcat(full_path, node->name);
        app_launch_arg("notepad", full_path);
    }
}

static void explorer_start(Window* win) {
    win->user_data = kmalloc(sizeof(ExplorerState));
    if (!win->user_data) return;
    k_memset(win->user_data, 0, sizeof(ExplorerState));
    ExplorerState* st = get_state(win);
    st->current_dir = 0;
    k_strcpy(st->path, "/");

    st->up_btn = window_add_widget(win, W_BUTTON, 4, 4, 30, 24);
    k_strcpy(st->up_btn->text, "..");
    st->up_btn->on_click = on_up_click;

    st->path_label = window_add_widget(win, W_LABEL, 40, 8, 400, 20);
    k_strcpy(st->path_label->text, st->path);

    st->list = window_add_widget(win, W_LISTVIEW, 0, 34, win->w - 2, win->h - TITLE_H - 36);
    st->list->on_dblclick = on_item_dblclick;

    refresh_list(win);
}

static void explorer_draw(Window* win, int x, int y, int w, int h) {
    (void)x; (void)y;
    ExplorerState* st = get_state(win);
    st->list->w = w - 2;
    st->list->h = h - 36;
}

static void explorer_event(Window* win, Event* ev) {
    (void)win; (void)ev;
}

void file_explorer_register(void) {
    AppDef def;
    k_memset(&def, 0, sizeof(def));
    k_strcpy(def.id, "explorer");
    k_strcpy(def.name, "File Explorer");
    def.icon_color = 0xFFC832;
    def.def_w = 500;
    def.def_h = 400;
    def.on_start = explorer_start;
    def.on_draw = explorer_draw;
    def.on_event = explorer_event;
    app_register(&def);
}
