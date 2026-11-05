#include "desktop.h"
#include "fb.h"
#include "gfx.h"
#include "font.h"
#include "mouse.h"
#include "bootinfo.h"
#include "serial.h"
#include "string.h"

/* weak fallback so this links without lcs.c */
__attribute__((weak)) int lcs_run(const char *name) {
    (void)name;
    return -1;
}

/* ---- palette ---- */
#define BG_TOP       0x141423
#define ACCENT       0x89B4FA
#define ACCENT_HI    0xA6C8FF
#define WIN_BG       0x24243A
#define TITLE_TOP    0x353553
#define TITLE_BOT    0x2A2A45
#define TEXT         0xCDD6F4
#define TEXT_DIM     0x7F849C
#define TASKBAR_BG   0x12121E
#define TASKBAR_BORD 0x313244
#define TASKBAR_H    48
#define WIN_RADIUS   12

/* ---- helpers ---- */
static inline unsigned char inb(unsigned short p) {
    unsigned char r;
    __asm__ volatile("inb %1,%0" : "=a"(r) : "Nd"(p));
    return r;
}

static int point_in(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && py >= ry && px < rx + rw && py < ry + rh;
}

static void put_dec_into(char *buf, int *n, unsigned int v) {
    char tmp[12]; int t = 0;
    if (!v) tmp[t++] = '0';
    while (v) { tmp[t++] = '0' + v % 10; v /= 10; }
    while (t) buf[(*n)++] = tmp[--t];
}

/* ---- state ---- */
static int win_x, win_y, win_w, win_h;
static int win_visible;
static int drag_active, drag_ox, drag_oy;
static const char *status_msg;
static int click_count;             

/* Track previous positions for dirty-rectangle rendering */
static int last_mx = -1, last_my = -1;
static int last_win_x = -1, last_win_y = -1, last_win_visible = -1;
static int last_taskbar_hover = -1;

/* ---- cursor bitmap ---- */
#define CURSOR_W 8
#define CURSOR_H 12
static const unsigned char cursor_bmp[] = {
    1,0,0,0,0,0,0,0,
    1,1,0,0,0,0,0,0,
    1,2,1,0,0,0,0,0,
    1,2,2,1,0,0,0,0,
    1,2,2,2,1,0,0,0,
    1,2,2,2,2,1,0,0,
    1,2,2,2,2,2,1,0,
    1,2,2,2,2,2,2,1,
    1,2,2,2,2,2,2,1,
    1,2,2,2,1,1,1,1,
    1,2,1,2,1,0,0,0,
    1,1,0,1,2,1,0,0,
};

static void draw_cursor(int x, int y) {
    for (int j = 0; j < CURSOR_H; j++)
        for (int i = 0; i < CURSOR_W; i++) {
            unsigned char c = cursor_bmp[j * CURSOR_W + i];
            if (c == 0) continue;
            fb_put_pixel(x + i, y + j, c == 1 ? 0x000000 : 0xFFFFFF);
        }
}

/* Restore a rectangle to the default wallpaper color */
static void restore_rect(int x, int y, int w, int h) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > fb_width()) w = fb_width() - x;
    if (y + h > fb_height()) h = fb_height() - y;
    if (w <= 0 || h <= 0) return;
    fb_fill_rect(x, y, w, h, BG_TOP);
}

/* ---- window ---- */
static void draw_window(void) {
    /* body */
    gfx_rounded_rect(win_x, win_y, win_w, win_h, WIN_RADIUS, WIN_BG, 255);

    /* title bar */
    int tbx = win_x + 4;
    int tby = win_y + 4;
    int tbw = win_w - 8;
    int tbh = 30;
    gfx_gradient_v(tbx, tby, tbw, tbh, TITLE_TOP, TITLE_BOT);

    /* branding */
    gfx_text(tbx + 12, tby + (tbh - FONT_H) / 2, "LiteC Bumblebee", TEXT);

    /* close button */
    int cbx = tbx + tbw - 20;
    int cby = tby + (tbh - 12) / 2;
    int cb_hover = point_in(mouse_x(), mouse_y(), cbx - 2, cby - 2, 16, 16);
    gfx_rounded_rect(cbx, cby, 12, 12, 6,
                     cb_hover ? 0xFF8080 : 0xFF5555, 255);

    /* body content */
    int bx = win_x + 20;
    int by = win_y + 50;
    gfx_text(bx, by,      "Welcome to LiteC Bumblebee!", TEXT);
    gfx_text(bx, by + 22, "Drag the title bar to move this window.", TEXT_DIM);
    gfx_text(bx, by + 44, "Click the red dot to close.", TEXT_DIM);
    gfx_text(bx, by + 66, "Click 'Launch Hello' to run a .lcs app.", TEXT_DIM);

    /* launch button */
    int lbx = win_x + 20;
    int lby = win_y + 140;
    int lb_hover = point_in(mouse_x(), mouse_y(), lbx, lby, 160, 34);
    gfx_rounded_rect(lbx, lby, 160, 34, 8,
                     lb_hover ? ACCENT_HI : ACCENT, 255);
    gfx_text(lbx + 16, lby + (34 - FONT_H) / 2, "Launch Hello", 0x0A0A15);

    /* status line */
    if (status_msg) {
        gfx_text(win_x + 16, win_y + win_h - 26, status_msg, 0xFFAA55);
    }
}

/* ---- taskbar ---- */
static void draw_taskbar(void) {
    int ty = fb_height() - TASKBAR_H;
    fb_fill_rect(0, ty, fb_width(), TASKBAR_H, TASKBAR_BG);
    fb_fill_rect(0, ty, fb_width(), 1, TASKBAR_BORD);

    /* brand */
    gfx_text(16, ty + (TASKBAR_H - FONT_H) / 2, "LiteC Bumblebee", ACCENT);

    /* divider */
    fb_fill_rect(180, ty + 10, 1, TASKBAR_H - 20, TASKBAR_BORD);

    /* window task button */
    int tx = 200, tw = 170, th = 32;
    int thy = ty + (TASKBAR_H - th) / 2;
    if (win_visible) {
        int thover = point_in(mouse_x(), mouse_y(), tx, thy, tw, th);
        gfx_rounded_rect(tx, thy, tw, th, 6,
                         thover ? 0x2A2A45 : 0x1A1A2E, 255);
        fb_fill_rect(tx, thy + 6, 3, th - 12, ACCENT);
        gfx_text(tx + 12, thy + (th - FONT_H) / 2, "LiteC Demo", TEXT);
    }

    /* right side: mouse coords + click counter */
    char buf[80]; int n = 0;
    const char *s1 = "x:";
    while (*s1) buf[n++] = *s1++;
    put_dec_into(buf, &n, (unsigned int)mouse_x());
    buf[n++] = ' ';
    buf[n++] = 'y'; buf[n++] = ':';
    put_dec_into(buf, &n, (unsigned int)mouse_y());
    buf[n++] = ' '; buf[n++] = ' ';
    const char *s2 = "clicks:";
    while (*s2) buf[n++] = *s2++;
    put_dec_into(buf, &n, (unsigned int)click_count);
    buf[n] = 0;
    int w = gfx_text_w(buf);
    gfx_text(fb_width() - w - 16, ty + (TASKBAR_H - FONT_H) / 2, buf, TEXT_DIM);
}

/* ---- input ---- */
static void handle_input(void) {
    int mx = mouse_x(), my = mouse_y();
    int clicked = mouse_clicked();
    int holding = mouse_btn() & 1;

    if (clicked) click_count++;     /* debug counter */

    /* taskbar: restore hidden window */
    if (clicked && !win_visible) {
        int ty = fb_height() - TASKBAR_H;
        int tx = 200, tw = 170, th = 32;
        int thy = ty + (TASKBAR_H - th) / 2;
        if (point_in(mx, my, tx, thy, tw, th)) {
            win_visible = 1;
            win_x = (fb_width()  - win_w) / 2;
            win_y = (fb_height() - TASKBAR_H - win_h) / 2;
            status_msg = 0;
            return;
        }
    }

    if (!win_visible) {
        if (!holding) drag_active = 0;
        return;
    }

    /* close button */
    if (clicked) {
        int tbx = win_x + 4;
        int tby = win_y + 4;
        int tbw = win_w - 8;
        int tbh = 30;
        int cbx = tbx + tbw - 20;
        int cby = tby + (tbh - 12) / 2;
        if (point_in(mx, my, cbx - 2, cby - 2, 16, 16)) {
            win_visible = 0;
            drag_active = 0;
            status_msg = 0;
            return;
        }
    }

    /* launch button */
    if (clicked) {
        int lbx = win_x + 20;
        int lby = win_y + 140;
        if (point_in(mx, my, lbx, lby, 160, 34)) {
            int r = lcs_run("hello");
            if      (r == 0)  status_msg = "Launched hello.lcs";
            else if (r == -3) status_msg = "No hello.lcs on disk";
            else              status_msg = "Launch error";
            return;
        }
    }

    /* title-bar drag */
    if (clicked && !drag_active &&
        my >= win_y && my < win_y + 34 &&
        mx >= win_x && mx < win_x + win_w) {
        drag_active = 1;
        drag_ox = mx - win_x;
        drag_oy = my - win_y;
    }
    if (!holding) drag_active = 0;

    if (drag_active) {
        win_x = mx - drag_ox;
        win_y = my - drag_oy;
        if (win_x < 0) win_x = 0;
        if (win_y < 0) win_y = 0;
        if (win_x + win_w > fb_width()) win_x = fb_width() - win_w;
        
        /* Standard clamp */
        if (win_y + win_h > fb_height() - TASKBAR_H)
            win_y = fb_height() - TASKBAR_H - win_h;
    }
}

/* ---- frame ---- */
static void frame(void) {
    mouse_poll();
    handle_input();

    int mouse_moved = (mouse_x() != last_mx) || (mouse_y() != last_my);
    int window_moved = (win_x != last_win_x) || (win_y != last_win_y) || (win_visible != last_win_visible);
    
    int ty = fb_height() - TASKBAR_H;
    int hover_taskbar = point_in(mouse_x(), mouse_y(), 200, ty + 8, 170, 32);
    int taskbar_changed = (hover_taskbar != last_taskbar_hover);

    if (mouse_moved || window_moved || mouse_clicked()) {
        // 1. Erase old window area
        if (last_win_visible) {
            restore_rect(last_win_x, last_win_y, win_w, win_h);
        }
        
        // 2. Erase old cursor area
        restore_rect(last_mx, last_my, CURSOR_W, CURSOR_H);

        // 3. Redraw taskbar if it might have been erased or hover changed
        if (window_moved || mouse_moved || taskbar_changed) {
            draw_taskbar();
            last_taskbar_hover = hover_taskbar;
        }

        // 4. Draw new window
        if (win_visible) {
            draw_window();
        }

        // 5. Draw new cursor
        draw_cursor(mouse_x(), mouse_y());

        // Save state for next frame
        last_mx = mouse_x();
        last_my = mouse_y();
        last_win_x = win_x;
        last_win_y = win_y;
        last_win_visible = win_visible;
    }
}

/* ---- entry ---- */
int desktop_run(void) {
    if (!fb_available()) return -1;
    if (fb_width() < 640 || fb_height() < 480) return -1;

    win_w = 480;
    win_h = 280;
    win_x = (fb_width()  - win_w) / 2;
    win_y = (fb_height() - TASKBAR_H - win_h) / 2;
    win_visible = 1;
    drag_active = 0;
    status_msg  = 0;
    click_count = 0;

    /* Initial full draw */
    fb_fill_rect(0, 0, fb_width(), fb_height(), BG_TOP);
    draw_taskbar();
    draw_window();
    
    last_mx = mouse_x();
    last_my = mouse_y();
    last_win_x = win_x;
    last_win_y = win_y;
    last_win_visible = win_visible;
    last_taskbar_hover = 0;
    
    draw_cursor(last_mx, last_my);

    for (;;) {
        frame();

        /* Throttle to prevent QEMU tearing */
        //for (volatile unsigned int spin = 0; spin < 1000000; spin++)
        //    __asm__ volatile("pause");

        unsigned char st = inb(0x64);
        if ((st & 1) && !(st & 0x20)) {
            unsigned char k = inb(0x60);
            if (k == 0x01) return 0;
        }
    }
}