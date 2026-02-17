#include "kernel.h"

static AppDef app_defs[MAX_APPS];
static int app_count = 0;
static int launch_offset = 0;
static char pending_arg[FS_PATH_LEN];

void app_register(AppDef* def) {
    if (app_count < MAX_APPS) app_defs[app_count++] = *def;
}

AppDef* app_get_all(int* count) { *count = app_count; return app_defs; }

const char* app_pending_arg(void) { return pending_arg; }

void app_launch(const char* id) { app_launch_arg(id, NULL); }

void app_launch_arg(const char* id, const char* arg) {
    for (int i = 0; i < app_count; i++) {
        if (k_strcmp(app_defs[i].id, id) == 0) {
            int ox = 80 + (launch_offset % 6) * 30;
            int oy = 40 + (launch_offset % 6) * 30;
            launch_offset++;
            Window* win = window_create(app_defs[i].name, ox, oy,
                                        app_defs[i].def_w, app_defs[i].def_h);
            if (!win) return;
            win->app = &app_defs[i];
            if (arg) k_strncpy(pending_arg, arg, FS_PATH_LEN-1);
            else pending_arg[0] = 0;
            if (app_defs[i].on_start) app_defs[i].on_start(win);
            return;
        }
    }
}

static FSNode fs_nodes[FS_MAX_FILES];
static int fs_count = 0;

void fs_init(void) {
    k_memset(fs_nodes, 0, sizeof(fs_nodes));
    fs_count = 0;

    k_strcpy(fs_nodes[0].name, "/");
    fs_nodes[0].is_dir = true; fs_nodes[0].parent = -1;
    fs_count++;

    fs_mkdir("/home");
    fs_mkdir("/home/Documents");
    fs_mkdir("/home/Pictures");

    const char* welcome = "Welcome to Claude OS!\n\nThis is a real operating system\nbuilt from scratch in C and x86\nassembly. It boots directly on\nhardware via GRUB.\n\nFeatures:\n- Window manager with drag/resize\n- File Explorer\n- Notepad text editor\n- Snake & Tetris games\n- Extensible .capp app format\n\nDouble-click desktop icons or\nuse the Start menu to launch apps.";
    fs_mkfile("/home/Documents/welcome.txt", (const uint8_t*)welcome, k_strlen(welcome));

    const char* readme = "Claude OS Application Format (.capp)\n\nTo create a new app:\n1. Create a new .c file in src/apps/\n2. Define a static struct for your state\n3. Implement on_start, on_draw, on_event\n4. Write a register function with AppDef\n5. Call app_register(&def)\n6. Declare your register function in kernel.h\n7. Call it from kernel_main\n8. Add the .c file to the Makefile\n\nEach app gets its own file, its own\nprivate data struct, and hooks into\nthe window system via function pointers.\nThis is C's version of polymorphism.";
    fs_mkfile("/home/Documents/readme.txt", (const uint8_t*)readme, k_strlen(readme));

    const char* notes = "My Notes\n--------\nClaude OS is cool!";
    fs_mkfile("/home/Documents/notes.txt", (const uint8_t*)notes, k_strlen(notes));
}

int fs_find(const char* path) {
    if (k_strcmp(path, "/") == 0) return 0;
    for (int i = 0; i < fs_count; i++) {
        char full[FS_PATH_LEN] = {0};
        int idx = i;
        char parts[16][FS_NAME_LEN];
        int depth = 0;
        while (idx > 0 && depth < 16) {
            k_strcpy(parts[depth++], fs_nodes[idx].name);
            idx = fs_nodes[idx].parent;
        }
        full[0] = '/';
        for (int d = depth - 1; d >= 0; d--) {
            k_strcat(full, parts[d]);
            if (d > 0) k_strcat(full, "/");
        }
        if (k_strcmp(full, path) == 0) return i;
    }
    return -1;
}

int fs_list(int dir, int* out, int max) {
    int n = 0;
    for (int i = 0; i < fs_count && n < max; i++) {
        if (fs_nodes[i].parent == dir && fs_nodes[i].name[0])
            out[n++] = i;
    }
    return n;
}

FSNode* fs_get(int idx) {
    if (idx < 0 || idx >= fs_count) return NULL;
    return &fs_nodes[idx];
}

int fs_mkdir(const char* path) {
    if (fs_count >= FS_MAX_FILES) return -1;
    char parent_path[FS_PATH_LEN] = {0};
    char name[FS_NAME_LEN] = {0};
    k_strcpy(parent_path, path);
    char* slash = k_strrchr(parent_path + 1, '/');
    if (slash) { k_strcpy(name, slash + 1); *slash = 0; }
    else { k_strcpy(name, path + 1); parent_path[0] = '/'; parent_path[1] = 0; }

    int pi = fs_find(parent_path);
    if (pi < 0) return -1;

    int idx = fs_count++;
    k_strcpy(fs_nodes[idx].name, name);
    fs_nodes[idx].is_dir = true;
    fs_nodes[idx].parent = pi;
    return idx;
}

int fs_mkfile(const char* path, const uint8_t* data, uint32_t size) {
    if (fs_count >= FS_MAX_FILES) return -1;
    char parent_path[FS_PATH_LEN] = {0};
    char name[FS_NAME_LEN] = {0};
    k_strcpy(parent_path, path);
    char* slash = k_strrchr(parent_path + 1, '/');
    if (slash) { k_strcpy(name, slash + 1); *slash = 0; }
    else { k_strcpy(name, path + 1); parent_path[0] = '/'; parent_path[1] = 0; }

    int pi = fs_find(parent_path);
    if (pi < 0) return -1;

    int idx = fs_count++;
    k_strcpy(fs_nodes[idx].name, name);
    fs_nodes[idx].is_dir = false;
    fs_nodes[idx].parent = pi;
    fs_nodes[idx].size = size;
    fs_nodes[idx].data = (uint8_t*)kmalloc(size + 1);
    if (fs_nodes[idx].data) {
        k_memcpy(fs_nodes[idx].data, data, size);
        fs_nodes[idx].data[size] = 0;
    }
    return idx;
}

int fs_write(int idx, const uint8_t* data, uint32_t size) {
    if (idx < 0 || idx >= fs_count) return -1;
    if (fs_nodes[idx].data) kfree(fs_nodes[idx].data);
    fs_nodes[idx].data = (uint8_t*)kmalloc(size + 1);
    if (fs_nodes[idx].data) {
        k_memcpy(fs_nodes[idx].data, data, size);
        fs_nodes[idx].data[size] = 0;
    }
    fs_nodes[idx].size = size;
    return 0;
}
