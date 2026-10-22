#include <gdt.h>
#include <printf.h>

const uint64_t gdt_template[7] = {
    GDT_ENTRY(0, 0, 0, 0),
    GDT_ENTRY(0, 0xFFFF, 0x9A, 0xA),
    GDT_ENTRY(0, 0xFFFF, 0x92, 0xC),
    GDT_ENTRY(0, 0xFFFF, 0xF2, 0xC),
    GDT_ENTRY(0, 0xFFFF, 0xFA, 0xA),
    GDT_ENTRY(0, 0, 0, 0),
    GDT_ENTRY(0, 0, 0, 0),
};

void gdt_load(uint64_t* gdt, int entries) {
    gdtr_t gdtr = {
        .base = (uintptr_t)gdt,
        .limit = (uint16_t)(entries * sizeof(uint64_t) - 1),
    };

    __asm__ volatile("lgdt %0\n"
                     "pushq $0x08\n"
                     "leaq 1f(%%rip), %%rax\n"
                     "pushq %%rax\n"
                     "lretq\n"
                     "1:\n"
                     "movw $0x10, %%ax\n"
                     "movw %%ax, %%ds\n"
                     "movw %%ax, %%es\n"
                     "movw %%ax, %%ss\n"
                     :
                     : "m"(gdtr)
                     : "rax", "memory");

    printf("gdt: Loaded GDT, base=0x%.16llx, limit=0x%.4x\n", gdtr.base, gdtr.limit);
}