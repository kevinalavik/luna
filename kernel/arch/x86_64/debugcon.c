#include <debugcon.h>
#include <portio.h>

static console_t debugcon = {0};

int debugcon_write(console_t* self, const uint8_t* buf, size_t len) {
    uint16_t port = (uint16_t)self->private;

    for (size_t i = 0; i < len; i++)
        outb(port, buf[i]);

    return (int)len;
}

console_t* debugcon_init(uint16_t port) {
    debugcon.private = port;
    debugcon.write = debugcon_write;
    return &debugcon;
}