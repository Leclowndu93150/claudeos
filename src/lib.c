#include "kernel.h"

int k_strlen(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

int k_strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *(uint8_t*)a - *(uint8_t*)b;
}

int k_strncmp(const char* a, const char* b, int n) {
    while (n && *a && *a == *b) { a++; b++; n--; }
    return n ? *(uint8_t*)a - *(uint8_t*)b : 0;
}

char* k_strcpy(char* d, const char* s) {
    char* r = d;
    while ((*d++ = *s++));
    return r;
}

char* k_strncpy(char* d, const char* s, int n) {
    char* r = d;
    while (n && (*d++ = *s++)) n--;
    while (n-- > 0) *d++ = 0;
    return r;
}

char* k_strcat(char* d, const char* s) {
    char* r = d;
    while (*d) d++;
    while ((*d++ = *s++));
    return r;
}

void* k_memcpy(void* d, const void* s, uint32_t n) {
    uint8_t* dp = (uint8_t*)d;
    const uint8_t* sp = (const uint8_t*)s;
    while (n--) *dp++ = *sp++;
    return d;
}

void* k_memset(void* d, int v, uint32_t n) {
    uint8_t* dp = (uint8_t*)d;
    while (n--) *dp++ = (uint8_t)v;
    return d;
}

void* k_memmove(void* d, const void* s, uint32_t n) {
    uint8_t* dp = (uint8_t*)d;
    const uint8_t* sp = (const uint8_t*)s;
    if (dp < sp) {
        while (n--) *dp++ = *sp++;
    } else {
        dp += n; sp += n;
        while (n--) *--dp = *--sp;
    }
    return d;
}

void k_itoa(int val, char* buf, int base) {
    char tmp[32];
    int i = 0, neg = 0;
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    if (val < 0 && base == 10) { neg = 1; val = -val; }
    unsigned int uv = (unsigned int)val;
    while (uv) {
        int d = uv % base;
        tmp[i++] = d < 10 ? '0' + d : 'A' + d - 10;
        uv /= base;
    }
    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = 0;
}

int k_atoi(const char* s) {
    int r = 0, neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') { r = r * 10 + (*s - '0'); s++; }
    return neg ? -r : r;
}

char* k_strrchr(const char* s, int c) {
    const char* last = NULL;
    while (*s) { if (*s == c) last = s; s++; }
    return (char*)last;
}

typedef struct Block {
    uint32_t size;
    bool free;
    struct Block* next;
} Block;

static Block* heap_start = NULL;

void mem_init(uint32_t start, uint32_t size) {
    heap_start = (Block*)start;
    heap_start->size = size - sizeof(Block);
    heap_start->free = true;
    heap_start->next = NULL;
}

void* kmalloc(uint32_t size) {
    size = (size + 7) & ~7;
    Block* b = heap_start;
    while (b) {
        if (b->free && b->size >= size) {
            if (b->size > size + sizeof(Block) + 16) {
                Block* next = (Block*)((uint8_t*)b + sizeof(Block) + size);
                next->size = b->size - size - sizeof(Block);
                next->free = true;
                next->next = b->next;
                b->size = size;
                b->next = next;
            }
            b->free = false;
            return (uint8_t*)b + sizeof(Block);
        }
        b = b->next;
    }
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;
    Block* b = (Block*)((uint8_t*)ptr - sizeof(Block));
    b->free = true;
    Block* cur = heap_start;
    while (cur) {
        if (cur->free && cur->next && cur->next->free) {
            cur->size += sizeof(Block) + cur->next->size;
            cur->next = cur->next->next;
            continue;
        }
        cur = cur->next;
    }
}

void mem_stats(uint32_t* total, uint32_t* used, uint32_t* free_mem) {
    uint32_t t = 0, u = 0, f = 0;
    Block* b = heap_start;
    while (b) {
        t += b->size;
        if (b->free) f += b->size;
        else u += b->size;
        b = b->next;
    }
    if (total) *total = t;
    if (used) *used = u;
    if (free_mem) *free_mem = f;
}
