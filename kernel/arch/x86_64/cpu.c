#include <cpu.h>

__attribute__((noreturn)) void halt(void) {
    for (;;) {
        asm("hlt");
    }
}

void pause(void) {
    __asm__ volatile("pause");
}