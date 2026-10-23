#include <ealloc.h>
#include <hhdm.h>
#include <limine.h>
#include <printf.h>
#include <asm.h>
#include <mem.h>
#include <align.h>
#include <stddef.h>
#include <stdint.h>
#include <lock.h>

__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0};

static uintptr_t base;
static uintptr_t max;
static spinlock_t lock = {0};

void ealloc_init() {
    if (!memmap_request.response) {
        printf("ealloc: Failed to get memory map!\n");
        halt();
    }

    if (!hhdm_present()) {
        printf("ealloc: Failed to get HHDM offset!\n");
        halt();
    }

    struct limine_memmap_response* memmap = memmap_request.response;
    uint64_t biggest = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE) {
            printf("ealloc: usable region: 0x%.16llx - 0x%.16llx (%llu bytes)\n",
                   (unsigned long long)e->base,
                   (unsigned long long)(e->base + e->length),
                   (unsigned long long)e->length);
            if (e->length > biggest) {
                base = (uintptr_t)e->base;
                max = (uintptr_t)(e->base + e->length);
                biggest = e->length;
            }
        }
    }

    if (biggest == 0) {
        printf("ealloc: no usable memory!\n");
        halt();
    }

    base = ALIGN_UP(base, PAGE_SIZE);
    max = ALIGN_DOWN(max, PAGE_SIZE);

    printf("ealloc: biggest usable region: 0x%.16llx - 0x%.16llx (%llu MiB)\n",
           base,
           max,
           ((max - base) / (1024 * 1024)));
}

void* early_alloc(size_t bytes /* rounded to PAGE_SIZE */) {
    spinlock_acquire(&lock);
    if (bytes == 0) {
        spinlock_release(&lock);
        return NULL;
    }

    bytes = ALIGN_UP(bytes, PAGE_SIZE);
    if (bytes > max - base) {
        printf("ealloc: can't allocate that much...\n"); // TODO: try the next fitting region
        spinlock_release(&lock);
        return NULL;
    }

    uintptr_t result = base;
    base += bytes;
    spinlock_release(&lock);
    return phys_to_virt(result);
}