#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"

#define SCREEN_W 1024
#define SCREEN_H 768

#define COLOR_BLACK       0x000000
#define COLOR_WHITE       0xFFFFFF
#define COLOR_DESKTOP     0x004E98
#define COLOR_TITLE       0x0078D7
#define COLOR_TITLE_OFF   0x888888
#define COLOR_WINDOW_BG   0xF3F3F3
#define COLOR_TASKBAR     0x181818
#define COLOR_TASKBAR_HVR 0x323232
#define COLOR_TASKBAR_ACT 0x3C3C3C
#define COLOR_BUTTON_BG   0xE1E1E1
#define COLOR_BUTTON_HVR  0xCCE4F7
#define COLOR_BORDER      0xADADAD
#define COLOR_INPUT_BG    0xFFFFFF
#define COLOR_INPUT_BRD   0x7A7A7A
#define COLOR_INPUT_FOC   0x0078D7
#define COLOR_SELECTION   0x0078D7
#define COLOR_SEL_TEXT    0xFFFFFF
#define COLOR_CLOSE_HVR   0xE81123
#define COLOR_MENU_BG     0x2B2B2B
#define COLOR_MENU_HVR    0x414141
#define COLOR_MENU_TEXT   0xFFFFFF
#define COLOR_TEXT        0x000000
#define COLOR_TEXT_DIM    0x646464
#define COLOR_ICON_TEXT   0xFFFFFF
#define COLOR_SHADOW      0x00000040
#define COLOR_TOOLBAR     0xF0F0F0
#define COLOR_TOOLBAR_BRD 0xD9D9D9
#define COLOR_SCROLLBAR   0xF0F0F0
#define COLOR_SCROLL_THM  0xCDCDCD

#define TITLE_H       30
#define TASKBAR_H     44
#define BORDER_W      1
#define WIN_BTN_W     46
#define FONT_W        8
#define FONT_H        16
#define SCROLL_W      16
#define MENU_ITEM_H   36
#define START_MENU_W  260

#define MAX_WINDOWS   32
#define MAX_WIDGETS   48
#define MAX_TEXT      4096
#define MAX_ITEMS     256
#define MAX_APPS      32

#define EVENT_QUEUE_SIZE 256

typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
} __attribute__((packed)) Registers;

typedef struct {
    uint32_t* addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
} Framebuffer;

#define EVENT_NONE       0
#define EVENT_MOUSE_MOVE 1
#define EVENT_MOUSE_DOWN 2
#define EVENT_MOUSE_UP   3
#define EVENT_KEY_DOWN   4
#define EVENT_KEY_UP     5

typedef struct {
    int type;
    int mouse_x, mouse_y;
    int mouse_button;
    uint8_t keycode;
    char key_char;
    bool shift, ctrl, alt;
} Event;

typedef struct {
    int x, y;
    bool left, right, middle;
    uint8_t cycle;
    uint8_t packet[3];
    bool shift, ctrl, alt, caps;
} MouseState;

typedef struct {
    bool shift, ctrl, alt, caps;
} KeyboardState;

#define W_LABEL     0
#define W_BUTTON    1
#define W_TEXTINPUT 2
#define W_TEXTAREA  3
#define W_LISTVIEW  4

typedef struct Widget Widget;
typedef struct Window Window;

struct Widget {
    int type;
    int x, y, w, h;
    bool visible, enabled, focused, hovered, pressed;

    char text[256];
    uint32_t color;

    void (*on_click)(Widget* self, Window* win);

    int cursor_pos;
    int scroll_x;
    void (*on_submit)(Widget* self, Window* win);
    void (*on_change)(Widget* self, Window* win);

    char* content;
    int content_len, content_cap;
    int cur_line, cur_col;
    int scroll_y;

    char items[MAX_ITEMS][128];
    uint32_t item_icons[MAX_ITEMS];
    int item_count;
    int selected;
    int list_scroll;
    void (*on_select)(Widget* self, Window* win, int idx);
    void (*on_dblclick)(Widget* self, Window* win, int idx);
    uint32_t last_click_time;
    int last_click_idx;
};

typedef void (*AppFunc)(Window*);
typedef void (*AppDrawFunc)(Window*, int, int, int, int);
typedef void (*AppEventFunc)(Window*, Event*);

typedef struct {
    char id[32];
    char name[64];
    uint32_t icon_color;
    int def_w, def_h;
    AppFunc on_start;
    AppDrawFunc on_draw;
    AppEventFunc on_event;
    AppFunc on_close;
    AppFunc on_tick;
} AppDef;

struct Window {
    int id;
    int x, y, w, h;
    char title[128];
    bool active, minimized, maximized;
    int rx, ry, rw, rh;
    bool dragging;
    int drag_ox, drag_oy;
    bool close_hov, max_hov, min_hov;

    Widget widgets[MAX_WIDGETS];
    int widget_count;
    int focused_widget;

    AppDef* app;
    void* user_data;
};

#define FS_MAX_FILES 128
#define FS_NAME_LEN  64
#define FS_PATH_LEN  256

typedef struct {
    char name[FS_NAME_LEN];
    uint32_t size;
    uint8_t* data;
    bool is_dir;
    int parent;
} FSNode;

void idt_init(void);
void pic_init(void);
void pic_eoi(uint8_t irq);
void timer_init(uint32_t freq);
uint32_t timer_ticks(void);
void event_push(Event* e);
bool event_poll(Event* e);

void keyboard_init(void);
void keyboard_irq(void);
bool keyboard_has_key(void);
int keyboard_getchar(void);
KeyboardState* keyboard_state(void);

void mouse_init(void);
void mouse_irq(void);
MouseState* mouse_state(void);

void fb_init(uint32_t* addr, uint32_t w, uint32_t h, uint32_t pitch, uint8_t bpp);
void fb_pixel(int x, int y, uint32_t c);
void fb_fill(int x, int y, int w, int h, uint32_t c);
void fb_rect(int x, int y, int w, int h, uint32_t c);
void fb_char(int x, int y, char ch, uint32_t fg, uint32_t bg);
void fb_text(int x, int y, const char* s, uint32_t fg, uint32_t bg);
void fb_text_t(int x, int y, const char* s, uint32_t fg);
int fb_text_width(const char* s);
void fb_swap(void);
void fb_clip_set(int x, int y, int w, int h);
void fb_clip_reset(void);
Framebuffer* fb_get(void);

void mem_init(uint32_t start, uint32_t size);
void* kmalloc(uint32_t size);
void kfree(void* ptr);
void mem_stats(uint32_t* total, uint32_t* used, uint32_t* free_mem);

int k_strlen(const char* s);
int k_strcmp(const char* a, const char* b);
int k_strncmp(const char* a, const char* b, int n);
char* k_strcpy(char* d, const char* s);
char* k_strncpy(char* d, const char* s, int n);
char* k_strcat(char* d, const char* s);
void* k_memcpy(void* d, const void* s, uint32_t n);
void* k_memset(void* d, int v, uint32_t n);
void* k_memmove(void* d, const void* s, uint32_t n);
void k_itoa(int val, char* buf, int base);
int k_atoi(const char* s);
char* k_strrchr(const char* s, int c);

void gui_init(void);
void gui_event(Event* e);
void gui_draw(void);
void gui_tick(void);
Window* window_create(const char* title, int x, int y, int w, int h);
void window_destroy(Window* win);
void window_focus(Window* win);
void window_toggle_max(Window* win);
Widget* window_add_widget(Window* win, int type, int x, int y, int w, int h);
Window** gui_get_windows(int* count);
Window* gui_get_focused(void);

void desktop_init(void);
void desktop_draw(void);
void desktop_event(Event* e);
void taskbar_init(void);
void taskbar_draw(void);
void taskbar_event(Event* e);
bool start_menu_is_open(void);
void start_menu_close(void);
void cursor_draw(int x, int y);

void app_register(AppDef* def);
void app_launch(const char* id);
void app_launch_arg(const char* id, const char* arg);
AppDef* app_get_all(int* count);
const char* app_pending_arg(void);
void file_explorer_register(void);
void notepad_register(void);
void snake_register(void);
void tetris_register(void);
void taskmgr_register(void);

void fs_init(void);
int fs_find(const char* path);
int fs_list(int dir, int* out, int max);
FSNode* fs_get(int idx);
int fs_mkdir(const char* path);
int fs_mkfile(const char* path, const uint8_t* data, uint32_t size);
int fs_write(int idx, const uint8_t* data, uint32_t size);

extern void idt_load(uint32_t);
extern void gdt_flush(uint32_t);

extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);
extern void isr32(void); extern void isr33(void); extern void isr34(void);
extern void isr35(void); extern void isr36(void); extern void isr37(void);
extern void isr38(void); extern void isr39(void); extern void isr40(void);
extern void isr41(void); extern void isr42(void); extern void isr43(void);
extern void isr44(void); extern void isr45(void); extern void isr46(void);
extern void isr47(void);

#endif
