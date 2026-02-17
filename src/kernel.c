#include "kernel.h"

struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

void idt_set_gate(int n, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[n].base_lo = base & 0xFFFF;
    idt[n].base_hi = (base >> 16) & 0xFFFF;
    idt[n].sel = sel;
    idt[n].zero = 0;
    idt[n].flags = flags;
}

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;
    k_memset(&idt, 0, sizeof(idt));

    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)isr34, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)isr35, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)isr36, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)isr37, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)isr38, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)isr39, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)isr40, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)isr41, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)isr42, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)isr43, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)isr44, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)isr45, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)isr46, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)isr47, 0x08, 0x8E);

    idt_load((uint32_t)&idtp);
}

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

void pic_init(void) {
    outb(PIC1_CMD, 0x11); outb(PIC2_CMD, 0x11); io_wait();
    outb(PIC1_DATA, 32);  outb(PIC2_DATA, 40);  io_wait();
    outb(PIC1_DATA, 4);   outb(PIC2_DATA, 2);   io_wait();
    outb(PIC1_DATA, 1);   outb(PIC2_DATA, 1);   io_wait();
    outb(PIC1_DATA, 0xF8);
    outb(PIC2_DATA, 0xEF);
}

void pic_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, 0x20);
    outb(PIC1_CMD, 0x20);
}

static volatile uint32_t tick_count = 0;

void timer_init(uint32_t freq) {
    uint32_t div = 1193180 / freq;
    outb(0x43, 0x36);
    outb(0x40, div & 0xFF);
    outb(0x40, (div >> 8) & 0xFF);
}

uint32_t timer_ticks(void) { return tick_count; }

static Event eq[EVENT_QUEUE_SIZE];
static volatile int eq_head = 0, eq_tail = 0;

void event_push(Event* e) {
    int next = (eq_head + 1) % EVENT_QUEUE_SIZE;
    if (next != eq_tail) {
        eq[eq_head] = *e;
        eq_head = next;
    }
}

bool event_poll(Event* e) {
    if (eq_head == eq_tail) return false;
    *e = eq[eq_tail];
    eq_tail = (eq_tail + 1) % EVENT_QUEUE_SIZE;
    return true;
}

void isr_handler(Registers* regs) {
    uint32_t n = regs->int_no;
    if (n == 32) {
        tick_count++;
        pic_eoi(0);
    } else if (n == 33) {
        keyboard_irq();
        pic_eoi(1);
    } else if (n == 44) {
        mouse_irq();
        pic_eoi(12);
    } else if (n >= 32) {
        pic_eoi(n - 32);
    }
}

static Framebuffer fb_info;

static void parse_multiboot(uint32_t* addr) {
    uint32_t total = *addr;
    uint8_t* tag = (uint8_t*)(addr + 2);
    while (tag < (uint8_t*)addr + total) {
        uint32_t type = *(uint32_t*)tag;
        uint32_t size = *(uint32_t*)(tag + 4);
        if (type == 0) break;
        if (type == 8) {
            uint32_t lo = *(uint32_t*)(tag + 8);
            fb_info.addr = (uint32_t*)(uint32_t)lo;
            fb_info.pitch = *(uint32_t*)(tag + 16);
            fb_info.width = *(uint32_t*)(tag + 20);
            fb_info.height = *(uint32_t*)(tag + 24);
            fb_info.bpp = *(uint8_t*)(tag + 28);
        }
        tag += (size + 7) & ~7;
    }
}

void kernel_main(uint32_t magic, uint32_t* mb_info) {
    if (magic != 0x36D76289) return;

    parse_multiboot(mb_info);
    idt_init();
    pic_init();
    timer_init(100);
    keyboard_init();
    mouse_init();

    mem_init(0x400000, 16 * 1024 * 1024);
    fb_init(fb_info.addr, fb_info.width, fb_info.height, fb_info.pitch, fb_info.bpp);

    fs_init();
    gui_init();
    desktop_init();
    taskbar_init();
    file_explorer_register();
    notepad_register();
    snake_register();
    tetris_register();
    taskmgr_register();

    asm volatile("sti");

    Event ev;
    while (1) {
        while (event_poll(&ev)) {
            gui_event(&ev);
        }
        gui_draw();
        gui_tick();
        asm volatile("hlt");
    }
}
