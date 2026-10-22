#include <lock.h>
#include <asm.h>

void spinlock_init(spinlock_t* lock) {
    if (!lock)
        return;

    lock->l = (atomic_flag)ATOMIC_FLAG_INIT;
}

void spinlock_acquire(spinlock_t* lock) {
    if (!lock)
        return;

    while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
        pause();
    }
}

void spinlock_release(spinlock_t* lock) {
    if (!lock)
        return;

    atomic_flag_clear_explicit(lock, memory_order_release);
}