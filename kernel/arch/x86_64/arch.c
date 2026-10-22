#include <arch.h>
#include <smp.h>
#include <idt.h>

void interrupt_init() {
    idt_init();
    __asm__ volatile("sti");
}

void mp_init() {
    smp_init();
}