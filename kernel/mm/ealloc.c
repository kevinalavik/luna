#include <ealloc.h>
#include <mem.h>
#include <limine.h>
#include <printf.h>
#include <asm.h>
#include <align.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#if ARCH == x86_64
#include <paging.h>
#endif
#include <lock.h>

#define EALLOC_MAX_REGIONS 128
#define EALLOC_ALIGNMENT 8

typedef struct ealloc_region {
    uintptr_t start;
    uintptr_t cur;
    uintptr_t end;
} ealloc_region_t;

static ealloc_region_t regions[EALLOC_MAX_REGIONS];
static size_t region_count;
static bool disabled;
static spinlock_t lock = {0};

void ealloc_init(void) {
    mem_init();

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
               (end - start) / 1024);
    }

    if (region_count == 0) {
        printf("ealloc: no usable memory!\n");
        halt();
    }

    printf("ealloc: %llu regions, %llu MiB total\n", region_count, (total / (1024 * 1024)));
}

void* early_alloc(size_t bytes) {
    if (bytes == 0)
        return NULL;

    if (bytes > SIZE_MAX - (EALLOC_ALIGNMENT - 1))
        return NULL;

    bytes = ALIGN_UP(bytes, EALLOC_ALIGNMENT);
    void* ret = NULL;

    spinlock_acquire(&lock);
    if (!disabled) {
        for (size_t i = 0; i < region_count; i++) {
            ealloc_region_t* r = &regions[i];

            if (r->cur > r->end || bytes > r->end - r->cur)
                continue;

            uintptr_t phys = r->cur;
            r->cur += bytes;

            ret = phys_to_virt(phys);
            break;
        }
    }
    spinlock_release(&lock);

    if (!ret)
        printf("ealloc: allocation failed (%llu bytes)\n", bytes);

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
    if (!out)
        return false;

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
