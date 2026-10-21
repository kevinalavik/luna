#include <gdt.h>

void load_gdt(uint64_t* gdt, int entries) {
    gdtr_t gdtr = {
        .base = (uintptr_t)gdt,
        .limit = (uint16_t)(entries * sizeof(uint64_t) - 1),
    };

    __asm__ volatile("lgdt %0" : : "m"(gdtr) : "memory");
}