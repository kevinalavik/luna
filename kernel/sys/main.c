#include <cpu.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <console.h>
#include <kcon.h>
#include <printf.h>

__attribute__((used, section(".limine_requests"))) static volatile uint64_t limine_base_revision[] =
    LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

__attribute__((
    used,
    section(".limine_requests_start"))) static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used, section(".limine_requests_end"))) static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false ||
        framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        halt();
    }

    struct limine_framebuffer* framebuffer = framebuffer_request.response->framebuffers[0];
    console_t* kcon = kcon_init(framebuffer);
    console_set_current(kcon);
    printf("boot: Luna kernel v0.1.0\n");
    init_interrupts();
    halt();
}
