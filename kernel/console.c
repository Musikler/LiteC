/* console.c */
#include "console.h"
#include "fb.h"
#include "gfx.h"
#include "font.h"
#include "serial.h"

#define COLS 128
#define ROWS 48

static int fb_mode;
static int cx, cy;
static unsigned char attr = 0x07;

/* VGA 16-color palette */
static const unsigned int pal[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
    0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
    0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
};

static unsigned int fg_color(void) { return pal[attr & 0x0F]; }
static unsigned int bg_color(void) { return pal[(attr >> 4) & 0x0F]; }

void console_init(void) {
    fb_mode = fb_available();
    console_clear();
}

static void fb_draw_char(int col, int row, char c) {
    gfx_text_bg(col * FONT_W, row * FONT_H, (char[2]){c, 0}, fg_color(), bg_color());
}

static void fb_clear_row(int row) {
    fb_fill_rect(0, row * FONT_H, fb_width(), FONT_H, bg_color());
}

static void fb_scroll(void) {
    /* copy everything up by FONT_H pixels */
    int pitch = fb_width();
    unsigned int *base = (unsigned int*)(unsigned long)0;
    (void)base; (void)pitch;
    /* use FB fill trick: memmove via per-row copy */
    /* Simpler: use framebuffer memmove */
}

void console_clear(void) {
    cx = cy = 0;
    if (fb_mode) {
        fb_clear(bg_color());
    } else {
        unsigned short *vga = (unsigned short*)0xB8000;
        unsigned short v = ((unsigned short)attr << 8) | ' ';
        for (int i = 0; i < 80 * 25; i++) vga[i] = v;
    }
}

static void fb_scroll_simple(void) {
    /* re-render is easier: since we don't keep a text buffer, just clear bottom row
       and let cursor stay. For a cleaner look we do a real scroll. */
    unsigned int *p = (unsigned int*)(unsigned long)0;
    (void)p;
}

void console_putchar(char c) {
    serial_putchar(c);

    if (!fb_mode) {
        unsigned short *vga = (unsigned short*)0xB8000;
        if (c == '\n') { cx = 0; cy++; }
        else if (c == '\r') { cx = 0; }
        else if (c == '\b') {
            if (cx > 0) cx--;
            vga[cy * 80 + cx] = ((unsigned short)attr << 8) | ' ';
        } else {
            vga[cy * 80 + cx] = ((unsigned short)attr << 8) | (unsigned char)c;
            if (++cx >= 80) { cx = 0; cy++; }
        }
        if (cy >= 25) {
            for (int i = 0; i < 24 * 80; i++) vga[i] = vga[i + 80];
            for (int i = 24 * 80; i < 25 * 80; i++)
                vga[i] = ((unsigned short)attr << 8) | ' ';
            cy = 24;
        }
        return;
    }

    /* FB mode */
    if (c == '\n') { cx = 0; cy++; }
    else if (c == '\r') { cx = 0; }
    else if (c == '\b') {
        if (cx > 0) cx--;
        fb_draw_char(cx, cy, ' ');
    } else {
        fb_draw_char(cx, cy, c);
        if (++cx >= COLS) { cx = 0; cy++; }
    }
    if (cy >= ROWS) {
        /* scroll: memmove up one text row (FONT_H pixels) */
        int bytes_per_row = fb_width() * 4;
        /* GOOD */
        unsigned char *base = (unsigned char*)(unsigned long)bootinfo()->fb_addr;
        int total_rows = ROWS * FONT_H;
        for (int r = 1; r < total_rows; r++) {
            unsigned char *src = base + r * bytes_per_row;
            unsigned char *dst = base + (r - 1) * bytes_per_row;
            for (int b = 0; b < bytes_per_row; b++) dst[b] = src[b];
        }
        fb_fill_rect(0, (ROWS - 1) * FONT_H, fb_width(), FONT_H, bg_color());
        cy = ROWS - 1;
    }
}

void console_write(const char *s) { while (*s) console_putchar(*s++); }
void console_set_color(unsigned char v) { attr = v; }