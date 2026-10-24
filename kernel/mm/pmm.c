#include <pmm.h>
#include <align.h>
#include <ealloc.h>
#include <printf.h>
#include <vmemmap.h>
#include <mem.h>
#include <lock.h>

static struct list_node free_list[MAX_ORDER];
static uintptr_t nr_free;
static spinlock_t pmm_lock;
static _Atomic bool ready;

static void block_add(struct page* p, unsigned order) {
    p->order = order;
    page_set(p, PG_BUDDY);
    list_add(&free_list[order], &p->buddy);
}

static void block_remove(struct page* p) {
    list_del(&p->buddy);
    page_clear(p, PG_BUDDY);
}

static void buddy_free(struct page* p, unsigned order) {
    uintptr_t pfn = page_to_pfn(p);

    nr_free += 1UL << order;

    while (order < MAX_ORDER - 1) {
        struct page* b = pfn_to_page(pfn ^ (1UL << order));

        if (!page_buddy(b) || b->order != order)
            break;

        block_remove(b);
        pfn &= ~(1UL << order);
        order++;
    }

    block_add(pfn_to_page(pfn), order);
}

struct page* alloc_pages(unsigned order) {
    struct page* ret = NULL;

    if (!atomic_load(&ready) || order >= MAX_ORDER)
        return NULL;

    spinlock_acquire(&pmm_lock);

    for (unsigned o = order; o < MAX_ORDER; o++) {
        if (list_empty(&free_list[o]))
            continue;

        struct page* p = container_of(free_list[o].next, struct page, buddy);
        block_remove(p);

        while (o > order) {
            o--;
            block_add(p + (1UL << o), o);
        }

        atomic_store(&p->refcount, 1);
        nr_free -= 1UL << order;
        ret = p;
        break;
    }

    spinlock_release(&pmm_lock);
    return ret;
}

void free_pages(struct page* p, unsigned order) {
    bool bad = false;

    if (!p)
        return;

    if (!atomic_load(&ready)) {
        printf("pmm: free before init\n");
        return;
    }

    spinlock_acquire(&pmm_lock);

    if (order >= MAX_ORDER || page_buddy(p) || page_reserved(p) || atomic_load(&p->refcount) != 1) {
        bad = true;
    } else {
        atomic_store(&p->refcount, 0);
        buddy_free(p, order);
    }

    spinlock_release(&pmm_lock);

    if (bad)
        printf("pmm: bad free of pfn %lu\n", page_to_pfn(p));
}

uintptr_t pmm_free_count(void) {
    spinlock_acquire(&pmm_lock);
    uintptr_t n = nr_free;
    spinlock_release(&pmm_lock);
    return n;
}

bool pmm_is_ready(void) {
    return atomic_load(&ready);
}

static void free_run(uintptr_t pfn, uintptr_t end) {
    while (pfn < end) {
        unsigned o = MAX_ORDER - 1;

        while (o && ((pfn & ((1UL << o) - 1)) || pfn + (1UL << o) > end))
            o--;

        for (uintptr_t i = 0; i < (1UL << o); i++) {
            struct page* p = pfn_to_page(pfn + i);
            page_clear(p, PG_RESERVED);
            atomic_store(&p->refcount, 0);
        }

        buddy_free(pfn_to_page(pfn), o);
        pfn += 1UL << o;
    }
}

static void reserve_used_ranges(void) {
    size_t n = ealloc_used_count();

    for (size_t i = 0; i < n; i++) {
        ealloc_range_t r;
        if (!ealloc_get_used(i, &r))
            break;

        uintptr_t start = ALIGN_DOWN(r.base, PAGE_SIZE) / PAGE_SIZE;
        uintptr_t end = ALIGN_UP(r.base + r.length, PAGE_SIZE) / PAGE_SIZE;

        for (uintptr_t pfn = start; pfn < end; pfn++) {
            struct page* p = pfn_to_page(pfn);
            page_set(p, PG_RESERVED);
            atomic_store(&p->refcount, 1);
        }
    }
}

static void free_usable(uintptr_t start, uintptr_t end) {
    uintptr_t cur = start;
    size_t n = ealloc_used_count();

    for (size_t i = 0; i < n; i++) {
        ealloc_range_t r;
        if (!ealloc_get_used(i, &r))
            break;

        uintptr_t rs = r.base / PAGE_SIZE;
        uintptr_t re = ALIGN_UP(r.base + r.length, PAGE_SIZE) / PAGE_SIZE;

        if (re <= cur || rs >= end)
            continue;

        if (rs > cur)
            free_run(cur, rs);

        cur = re;
    }

    if (cur < end)
        free_run(cur, end);
}

void pmm_init(void) {
    spinlock_init(&pmm_lock);

    for (int o = 0; o < MAX_ORDER; o++)
        list_init(&free_list[o]);

    ealloc_disable();
    reserve_used_ranges();

    spinlock_acquire(&pmm_lock);

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* e = memmap->entries[i];

        if (e->type != LIMINE_MEMMAP_USABLE)
            continue;

        uintptr_t start = ALIGN_UP(e->base, PAGE_SIZE) / PAGE_SIZE;
        uintptr_t end = ALIGN_DOWN(e->base + e->length, PAGE_SIZE) / PAGE_SIZE;

        if (start == 0)
            start = 1;

        if (start < end)
            free_usable(start, end);
    }

    uintptr_t free_now = nr_free;
    spinlock_release(&pmm_lock);

    atomic_store(&ready, true);

    printf("pmm: %lu pages free (%lu MiB)\n", free_now, free_now * PAGE_SIZE / (1024 * 1024));
}