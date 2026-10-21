#include <cpu.h>
#include <gdt.h>
#include <printf.h>
#include <idt.h>

uint64_t gdt[6] = {
    GDT_ENTRY(0, 0, 0, 0),           // NULL
    GDT_ENTRY(0, 0xFFFF, 0x9A, 0xA), // KCODE
    GDT_ENTRY(0, 0xFFFF, 0x92, 0xC), // KDATA
    GDT_ENTRY(0, 0xFFFF, 0xF2, 0xC), // UDATA
    GDT_ENTRY(0, 0xFFFF, 0xFA, 0xA), // UCODE
    GDT_ENTRY(0, 0, 0x89, 0),        // TSS, base = &tss, limit = sizeof(tss).
};

__attribute__((noreturn)) void halt(void) {
    for (;;) {
        asm("hlt");
    }
}

void pause(void) {
    __asm__ volatile("pause");
}

void init_interrupts() {
    load_gdt(gdt, sizeof(gdt) / sizeof(gdt[0]));
    printf("arch: GDT ok\n");
    idt_init();
    __asm__ volatile("sti");
    printf("arch: IDT ok\n");
}