#include <hhdm.h>
#include <limine.h>
#include <stddef.h>

__attribute__((
    used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID, .revision = 0};

bool hhdm_present(void) {
    return hhdm_request.response != NULL;
}

uintptr_t hhdm_offset(void) {
    if (!hhdm_request.response)
        return 0;

    return (uintptr_t)hhdm_request.response->offset;
}

void* phys_to_virt(uint64_t phys) {
    return (void*)(phys + hhdm_offset());
}

uint64_t virt_to_phys(const void* virt) {
    return (uint64_t)virt - hhdm_offset();
}
