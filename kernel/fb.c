#include "fb.h"

static unsigned int *fb = 0;
static int W, H, PITCH;

int fb_init(void) {
    volatile bootinfo_t *bi = bootinfo();
    if (bi->magic != 0x424F4F54) return -1;
    if (bi->fb_bpp != 32)        return -1;
    fb    = (unsigned int*)(unsigned long)bi->fb_addr;
    W     = (int)bi->fb_w;
    H     = (int)bi->fb_h;
    PITCH = (int)(bi->fb_pitch / 4);
    return 0;
}
int fb_available(void) { return fb != 0; }
int fb_width(void)  { return W; }
int fb_height(void) { return H; }

void fb_put_pixel(int x, int y, unsigned int rgb) {
    if (x < 0 || y < 0 || x >= W || y >= H) return;
    fb[y * PITCH + x] = rgb;
}

void fb_fill_rect(int x, int y, int w, int h, unsigned int rgb) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > W) w = W - x;
    if (y + h > H) h = H - y;
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) {
        unsigned int *row = fb + (y + j) * PITCH + x;
        for (int i = 0; i < w; i++) row[i] = rgb;
    }
}

void fb_clear(unsigned int rgb) { fb_fill_rect(0, 0, W, H, rgb); }