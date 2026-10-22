#include <tss.h>
#include <ealloc.h>
#include <printf.h>
#include <cpu.h>
#include <string.h>
#include <asm.h>

#define STACK_SIZE 16384

void tss_init(tss_t* tss) {
    memset(tss, 0, sizeof(*tss));

    void* kstack = early_alloc(STACK_SIZE);
    if (!kstack) {
        printf("tss: out of memory for stack\n");
        halt();
    }

    tss->rsp[0] = (uint64_t)kstack + STACK_SIZE;
    tss->iopb = sizeof(tss_t);
}

void tss_load(void) {
    uint16_t sel = TSS_OFFSET;
    __asm__ volatile("ltr %0" ::"r"(sel));
}