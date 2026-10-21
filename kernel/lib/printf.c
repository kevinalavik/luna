#include <printf.h>
#include <stdarg.h>
#include <console.h>

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#define NANOPRINTF_IMPLEMENTATION
#include <nanoprintf.h>

void putc(int c, void*) {
    if (c == '\n')
        putc('\r', NULL);
    const uint8_t s[] = {(uint8_t)c};
    console_write(console_get_current(), s, 1);
}

int printf(const char* fmt, ...) {
    va_list vlist;
    va_start(vlist, fmt);
    int rv = npf_vpprintf(putc, NULL, fmt, vlist);
    va_end(vlist);
    return rv;
}