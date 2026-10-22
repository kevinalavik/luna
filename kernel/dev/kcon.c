#include <kcon.h>
#include <flanterm.h>
#include <flanterm_backends/fb.h>
#include <limine.h>

#if ARCH == x86_64
#include <debugcon.h> // route to debugcon aswell on x86_64
static console_t* debugcon = NULL;
#endif

__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

static console_t kcon = {0};

int kcon_write(console_t* self, const uint8_t* buf, size_t len) {
    flanterm_write((struct flanterm_context*)self->private, (const char*)buf, len);
#if ARCH == x86_64
    console_write(debugcon, buf, len);
#endif
    return (int)len; // this is wack, we just hope that flanterm printed everything :^)
}

console_t* kcon_init() {
    if (!framebuffer_request.response)
        return NULL;
    struct limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];
    struct flanterm_context* ft_ctx = flanterm_fb_init(NULL,
                                                       NULL,
                                                       fb->address,
                                                       fb->width,
                                                       fb->height,
                                                       fb->pitch,
                                                       fb->red_mask_size,
                                                       fb->red_mask_shift,
                                                       fb->green_mask_size,
                                                       fb->green_mask_shift,
                                                       fb->blue_mask_size,
                                                       fb->blue_mask_shift,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       0,
                                                       0,
                                                       1,
                                                       0,
                                                       0,
                                                       0,
                                                       0);
    kcon.private = ft_ctx;
    kcon.write = kcon_write;
    spinlock_init(&kcon.lock);
#if ARCH == x86_64
    debugcon = debugcon_init(0xe9);
#endif
    return &kcon;
}