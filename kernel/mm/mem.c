#include <mem.h>
#include <limine.h>
#include <stddef.h>

__attribute__((
    used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID, .revision = 0};

__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0};

uintptr_t hhdm_offset = 0;
struct limine_memmap_response* memmap = NULL;

void mem_init() {
    if (hhdm_request.response)
        hhdm_offset = hhdm_request.response->offset;

    if (memmap_request.response)
        memmap = memmap_request.response;
}

void* phys_to_virt(uint64_t phys) {
    return (void*)(phys + hhdm_offset);
}

uint64_t virt_to_phys(const void* virt) {
    return (uint64_t)virt - hhdm_offset;
}
