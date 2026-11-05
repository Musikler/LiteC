#include "gfx.h"
#include "fb.h"
#include "font.h"
#include "bootinfo.h"

/* ---------- forward declaration for internal helper ---------- */
static void fb_set_blend_rect_safe(int x, int y, int w, int h,
                                   unsigned int rgb, int a);

/* ---------- framebuffer access ---------- */
static inline unsigned int *fb_base(void) {
    return (unsigned int*)(unsigned long)bootinfo()->fb_addr;
}
static inline int fb_pitch_px(void) { return (int)(bootinfo()->fb_pitch / 4); }
static inline int fb_w(void)        { return (int)bootinfo()->fb_w; }
static inline int fb_h(void)        { return (int)bootinfo()->fb_h; }

static inline unsigned int fb_get(int x, int y) {
    if (x < 0 || y < 0 || x >= fb_w() || y >= fb_h()) return 0;
    return fb_base()[y * fb_pitch_px() + x];
}
static inline void fb_set_blend(int x, int y, unsigned int rgb, int a) {
    if (x < 0 || y < 0 || x >= fb_w() || y >= fb_h()) return;
    unsigned int *p = &fb_base()[y * fb_pitch_px() + x];
    *p = gfx_blend(*p, rgb, a);
}

/* ---------- blending ---------- */
unsigned int gfx_blend(unsigned int dst, unsigned int src, int a) {
    if (a <= 0)   return dst;
    if (a >= 255) return src;
    unsigned int dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
    unsigned int sr = (src >> 16) & 0xFF, sg = (src >> 8) & 0xFF, sb = src & 0xFF;
    unsigned int r = (sr * a + dr * (255 - a)) / 255;
    unsigned int g = (sg * a + dg * (255 - a)) / 255;
    unsigned int b = (sb * a + db * (255 - a)) / 255;
    return (r << 16) | (g << 8) | b;
}

void gfx_fill_rect_alpha(int x, int y, int w, int h, unsigned int rgb, int a) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            fb_set_blend(x + i, y + j, rgb, a);
}

/* ---------- gradients ---------- */
static unsigned int lerp(unsigned int a, unsigned int b, int t) {
    unsigned int ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
    unsigned int br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
    unsigned int r  = (ar * (255 - t) + br * t) / 255;
    unsigned int g  = (ag * (255 - t) + bg * t) / 255;
    unsigned int bl = (ab * (255 - t) + bb * t) / 255;
    return (r << 16) | (g << 8) | bl;
}

void gfx_gradient_v(int x, int y, int w, int h,
                    unsigned int top, unsigned int bot) {
    for (int j = 0; j < h; j++) {
        int t = (h > 1) ? (j * 255 / (h - 1)) : 0;
        unsigned int c = lerp(top, bot, t);
        fb_fill_rect(x, y + j, w, 1, c);
    }
}

/* ---------- rounded rectangles ---------- */
static int corner_coverage(int radius, int dx, int dy) {
    int hits = 0, R2 = radius * radius;
    for (int sy = 0; sy < 2; sy++)
        for (int sx = 0; sx < 2; sx++) {
            int px = 2 * dx + sx, py = 2 * dy + sy;
            int d2 = px * px + py * py;
            if (d2 <= 4 * R2) hits++;
        }
    return hits * 255 / 4;
}

static void fb_set_blend_rect_safe(int x, int y, int w, int h,
                                   unsigned int rgb, int a) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            fb_set_blend(x + i, y + j, rgb, a);
}

void gfx_rounded_rect(int x, int y, int w, int h, int radius,
                      unsigned int rgb, int alpha) {
    if (radius * 2 > w) radius = w / 2;
    if (radius * 2 > h) radius = h / 2;

    for (int j = 0; j < h; j++) {
        if (j < radius) {
            for (int i = 0; i < radius; i++) {
                int cov = corner_coverage(radius, radius - i - 1, radius - j - 1);
                int a = cov * alpha / 255;
                fb_set_blend(x + i,             y + j, rgb, a);
                fb_set_blend(x + w - 1 - i,     y + j, rgb, a);
            }
            fb_set_blend_rect_safe(x + radius, y + j, w - 2 * radius, 1, rgb, alpha);
        } else if (j >= h - radius) {
            int jj = h - j - 1;
            for (int i = 0; i < radius; i++) {
                int cov = corner_coverage(radius, radius - i - 1, radius - jj - 1);
                int a = cov * alpha / 255;
                fb_set_blend(x + i,         y + j, rgb, a);
                fb_set_blend(x + w - 1 - i, y + j, rgb, a);
            }
            fb_set_blend_rect_safe(x + radius, y + j, w - 2 * radius, 1, rgb, alpha);
        } else {
            fb_set_blend_rect_safe(x, y + j, w, 1, rgb, alpha);
        }
    }
}

/* ---------- text ---------- */
static void draw_glyph(int x, int y, unsigned char ch,
                       unsigned int fg, unsigned int bg, int has_bg) {
    if (ch >= FONT_COUNT) ch = '?';
    const unsigned char *g = &font_data[(int)ch * FONT_H];
    for (int j = 0; j < FONT_H; j++) {
        unsigned char row = g[j];
        for (int i = 0; i < FONT_W; i++) {
            int on = (row >> (7 - i)) & 1;
            int px = x + i, py = y + j;
            if (on)          fb_put_pixel(px, py, fg);
            else if (has_bg) fb_put_pixel(px, py, bg);
        }
    }
}

void gfx_text(int x, int y, const char *s, unsigned int rgb) {
    int cx = x;
    while (*s) {
        if (*s == '\n') { y += FONT_H; cx = x; s++; continue; }
        draw_glyph(cx, y, (unsigned char)*s, rgb, 0, 0);
        cx += FONT_W;
        s++;
    }
}

void gfx_text_bg(int x, int y, const char *s, unsigned int fg, unsigned int bg) {
    int cx = x;
    while (*s) {
        if (*s == '\n') { y += FONT_H; cx = x; s++; continue; }
        draw_glyph(cx, y, (unsigned char)*s, fg, bg, 1);
        cx += FONT_W;
        s++;
    }
}

int gfx_text_w(const char *s) {
    int n = 0;
    while (*s) { if (*s == '\n') return n * FONT_W; n++; s++; }
    return n * FONT_W;
}