#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct console {
    void* private;
    int (*write)(struct console* self, const uint8_t* buf, size_t len);
    int (*read)(struct console* self, const uint8_t* buf, size_t len);
} console_t;

int console_write(console_t* console, const uint8_t* buf, size_t len);
int console_read(console_t* console, const uint8_t* buf, size_t len);

void console_set_current(console_t* console);
console_t* console_get_current(); // wack