#include <kcon.h>
#include <flanterm.h>
#include <flanterm_backends/fb.h>

console_t kcon = {0};

int kcon_write(console_t* self, const uint8_t* buf, size_t len) {
    flanterm_write((struct flanterm_context*)self->private, (const char*)buf, len);
    return len; // this is wack, we just hope that flanterm printed everything :^)
}

console_t* kcon_init(struct limine_framebuffer* fb) {
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
    return &kcon;
}
