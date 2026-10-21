#include <console.h>

console_t* current = NULL;

int console_write(console_t* console, const uint8_t* buf, size_t len) {
    if (!console || !console->write || !buf || len == 0)
        return -1;

    spinlock_acquire(&console->lock);
    int rv = console->write(console, buf, len);
    spinlock_release(&console->lock);
    return rv;
}

int console_read(console_t* console, const uint8_t* buf, size_t len) {
    if (!console || !console->read || !buf || len == 0)
        return -1;

    spinlock_acquire(&console->lock);
    int rv = console->read(console, buf, len);
    spinlock_release(&console->lock);
    return rv;
}

void console_set_current(console_t* console) {
    current = console;
}

console_t* console_get_current() {
    return current;
}