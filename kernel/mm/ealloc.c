#include <ealloc.h>
#include <hhdm.h>
#include <limine.h>
#include <printf.h>
#include <asm.h>
#include <align.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#if ARCH == x86_64
#include <paging.h> // ugly way to do this but sure, idc.
#endif
#include <lock.h>

#define EALLOC_MAX_REGIONS 128

__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0};

typedef struct ealloc_region {
    uintptr_t start; // original aligned start (never changes)
    uintptr_t cur;   // bump pointer, start+cur has been handed out
    uintptr_t end;   // aligned end
} ealloc_region_t;

static ealloc_region_t regions[EALLOC_MAX_REGIONS];
static size_t region_count;
static bool disabled;
static spinlock_t lock = {0};

void ealloc_init(void) {
    if (!memmap_request.response) {
        printf("ealloc: Failed to get memory map!\n");
        halt();
    }

    hhdm_init();

    struct limine_memmap_response* memmap = memmap_request.response;
    uint64_t total = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* e = memmap->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE)
            continue;

        uintptr_t start = ALIGN_UP((uintptr_t)e->base, PAGE_SIZE);
        uintptr_t end = ALIGN_DOWN((uintptr_t)(e->base + e->length), PAGE_SIZE);

        if (end <= start)
            continue;

        if (region_count >= EALLOC_MAX_REGIONS) {
            printf("ealloc: too many usable regions, ignoring 0x%.16llx\n", start);
            continue;
        }

        regions[region_count++] = (ealloc_region_t){
            .start = start,
            .cur = start,
            .end = end,
        };
        total += end - start;

        printf("ealloc: usable region: 0x%.16llx - 0x%.16llx (%llu KiB)\n",
               start,
               end,
               ((end - start) / 1024));
    }

    if (region_count == 0) {
        printf("ealloc: no usable memory!\n");
        halt();
    }

    printf("ealloc: %llu regions, %llu MiB total\n", region_count, (total / (1024 * 1024)));
}

void* early_alloc(size_t bytes /* rounded up to PAGE_SIZE */) {
    if (bytes == 0)
        return NULL;

    if (bytes > SIZE_MAX - (PAGE_SIZE - 1))
        return NULL;
    bytes = ALIGN_UP(bytes, PAGE_SIZE);

    printf("ealloc: Allocating %d bytes!\n", bytes);

    void* ret = NULL;

    spinlock_acquire(&lock);

    if (!disabled) {
        for (size_t i = 0; i < region_count; i++) {
            ealloc_region_t* r = &regions[i];
            if (r->end - r->cur >= bytes) {
                uintptr_t phys = r->cur;
                r->cur += bytes;
                ret = phys_to_virt(phys);
                break;
            }
        }

        if (!ret)
            printf("ealloc: out of memory (requested %llu bytes)\n", bytes);
    }

    spinlock_release(&lock);
    return ret;
}

size_t ealloc_used_count(void) {
    size_t n = 0;
    spinlock_acquire(&lock);
    for (size_t i = 0; i < region_count; i++)
        if (regions[i].cur > regions[i].start)
            n++;
    spinlock_release(&lock);
    return n;
}

bool ealloc_get_used(size_t index, ealloc_range_t* out) {
    bool found = false;
    spinlock_acquire(&lock);
    for (size_t i = 0, n = 0; i < region_count; i++) {
        if (regions[i].cur == regions[i].start)
            continue;
        if (n++ == index) {
            out->base = regions[i].start;
            out->length = regions[i].cur - regions[i].start;
            found = true;
            break;
        }
    }
    spinlock_release(&lock);
    return found;
}

void ealloc_disable(void) {
    spinlock_acquire(&lock);
    disabled = true;
    spinlock_release(&lock);
}