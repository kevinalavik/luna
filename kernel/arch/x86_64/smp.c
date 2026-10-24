#include <smp.h>
#include <limine.h>
#include <printf.h>
#include <cpu.h>
#include <tss.h>
#include <gdt.h>
#include <idt.h>
#include <string.h>
#include <stdbool.h>
#include <arch.h>
#include <asm.h>
#include <kcon.h>
#include <paging.h>

__attribute__((used,
               section(".limine_requests"))) static volatile struct limine_mp_request mp_request = {
    .id = LIMINE_MP_REQUEST_ID, .revision = 0};

static cpu_t cpus[MAX_CPUS];
static uint32_t cpus_ready = 0;

static void cpu_local_init(cpu_t* cpu) {
    tss_init(&cpu->tss);

    uint64_t tss_base = (uint64_t)&cpu->tss;
    cpu->gdt[5] = GDT_ENTRY(tss_base, sizeof(tss_t) - 1, 0x89, 0);
    cpu->gdt[6] = tss_base >> 32;

    tss_load();
}

void cpu_early_init(void) {
    cpu_t* cpu = &cpus[0];

    cpu->self = cpu;
    cpu->id = 0;

    memcpy(cpu->gdt, gdt_template, sizeof(gdt_template));
    gdt_load(cpu->gdt, 7);

    wrmsr(MSR_GS_BASE, (uint64_t)cpu);
}

void cpu_init(struct limine_mp_info* mp_info) {
    ptable_load(kernel_ptable);
    cpu_t* cpu = (cpu_t*)mp_info->extra_argument;

    memcpy(cpu->gdt, gdt_template, sizeof(gdt_template));
    gdt_load(cpu->gdt, 7);
    wrmsr(MSR_GS_BASE, (uint64_t)cpu);
    idt_init();
    cpu_local_init(cpu);

    printf("smp: hello from CPU %u!\n", cpu->id);
    __atomic_add_fetch(&cpus_ready, 1, __ATOMIC_RELEASE);
    __asm__ volatile("sti");
    halt();
}

void smp_init() {
    if (!mp_request.response) {
        printf("smp: Failed to get mp_request\n");
        halt();
    }

    struct limine_mp_response* mp_response = mp_request.response;
    printf("smp: %lu CPU(s) available\n", (unsigned long)mp_response->cpu_count);

    uint32_t next_id = 1;
    uint32_t started = 0;
    for (uint64_t i = 0; i < mp_response->cpu_count; i++) {
        struct limine_mp_info* info = mp_response->cpus[i];

        if (info->lapic_id == mp_response->bsp_lapic_id) {
            cpus[0].lapic_id = info->lapic_id;
            cpu_local_init(&cpus[0]);
            continue;
        }

        if (next_id >= MAX_CPUS) {
            printf("smp: you got to many cores bro, please give me some :(\n");
            break;
        }

        cpu_t* cpu = &cpus[next_id];
        cpu->self = cpu;
        cpu->id = next_id++;
        cpu->lapic_id = info->lapic_id;

        __atomic_store_n(&info->extra_argument, (uint64_t)cpu, __ATOMIC_RELEASE);
        __atomic_store_n(&info->goto_address, cpu_init, __ATOMIC_SEQ_CST);
        started++;
    }

    while (__atomic_load_n(&cpus_ready, __ATOMIC_ACQUIRE) < started) {
        pause();
    }

    printf("smp: all %u AP(s) online\n", started);
}