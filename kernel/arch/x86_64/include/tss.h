#pragma once

#include <stdint.h>

#define TSS_OFFSET 0x28 // offset from gdt base

typedef struct tss {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb;
} __attribute__((packed)) tss_t;

void tss_init(tss_t* tss);
void tss_load(void);