#pragma once

#include <stdint.h>

#define GDT_ENTRY(base, limit, access, flags)                                                      \
    (((uint64_t)(limit) & 0xFFFF) | (((uint64_t)(base) & 0xFFFF) << 16) |                          \
     (((uint64_t)(base) >> 16 & 0xFF) << 32) | ((uint64_t)(access) << 40) |                        \
     (((uint64_t)(limit) >> 16 & 0xF) << 48) | (((uint64_t)(flags) & 0xF) << 52) |                 \
     (((uint64_t)(base) >> 24 & 0xFF) << 56))

typedef struct gdtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdtr_t;

void load_gdt(uint64_t* gdt, int entries);