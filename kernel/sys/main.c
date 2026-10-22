#include <cpu.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <console.h>
#include <kcon.h>
#include <printf.h>
#include <debugcon.h>
#include <ealloc.h>

__attribute__((used, section(".limine_requests"))) static volatile uint64_t limine_base_revision[] =
    LIMINE_BASE_REVISION(6);

__attribute__((
    used,
    section(".limine_requests_start"))) static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used, section(".limine_requests_end"))) static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        halt();
    }

    console_t* kcon = kcon_init();
    console_set_current(kcon);
    printf("boot: Luna kernel \033[95m<3\033[0m v0.1.0\n");
    init_interrupts();

    ealloc_init();
    void* a = early_alloc(1); // waste a single page for testing :^)
    if (!a) {
        printf("boot: ealloc is broken :(\n");
        halt();
    }
    printf("boot: ealloc ok (allocated single page: %p)\n", a);

    halt();
}
