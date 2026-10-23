#pragma once

#include <stdint.h>

__attribute__((noreturn)) static inline void halt(void) {
    for (;;)
        __asm__ volatile("hlt");
}

static inline void pause(void) {
    __asm__ volatile("pause");
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    __asm__ volatile("wrmsr" ::"c"(msr), "a"((uint32_t)val), "d"((uint32_t)(val >> 32)) : "memory");
}

static inline uint64_t read_cr3(void) {
    uint64_t v;
    __asm__ volatile("mov %%cr3, %0" : "=r"(v));
    return v;
}

static inline void invlpg(uint64_t virt) {
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}
