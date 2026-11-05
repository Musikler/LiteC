#include "../commands.h"
#include "../vga.h"
#include "../fb.h"
#include "../gfx.h"
#include "../font.h"
#include "../string.h"

extern int desktop_run(void);
extern char keyboard_getchar(void);

static void put_dec(unsigned int n) {
    char b[12]; int i = 0;
    if (!n) b[i++] = '0';
    while (n) { b[i++] = '0' + n % 10; n /= 10; }
    while (i--) vga_putchar(b[i]);
}

/* ---- primitive test scene (unchanged from before) ---- */
static void demo_run(void) {
    gfx_gradient_v(0, 0, fb_width(), fb_height(), 0x1A1A2E, 0x2A2A45);

    int card_w = 260, card_h = 160, gap = 20;
    int total = card_w * 3 + gap * 2;
    int cx = (fb_width() - total) / 2;
    int cy = 80;

    gfx_rounded_rect(cx,                       cy, card_w, card_h,  8, 0x24243A, 255);
    gfx_rounded_rect(cx + card_w + gap,        cy, card_w, card_h, 16, 0x353553, 255);
    gfx_rounded_rect(cx + (card_w + gap) * 2,  cy, card_w, card_h, 24, 0x89B4FA, 255);

    gfx_text(cx + 20,                      cy + 20, "radius 8",  0xCDD6F4);
    gfx_text(cx + card_w + gap + 20,       cy + 20, "radius 16", 0xCDD6F4);
    gfx_text(cx + (card_w + gap) * 2 + 20, cy + 20, "radius 24", 0x1A1A2E);

    gfx_fill_rect_alpha(0, fb_height() - 60, fb_width(), 60, 0x000000, 160);
    gfx_text(20, fb_height() - 45,
             "graphics-launch demo: alpha + rounded corners + gradient",
             0xCDD6F4);
    gfx_text(20, 20, "LiteC graphics subsystem", 0x89B4FA);
}

/* ---- entry ---- */
static void graphics_launch_fn(int argc, char **argv) {
    if (!fb_available()) {
        vga_write("No framebuffer available.\n");
        vga_write("Bootloader did not set a VESA mode.\n");
        return;
    }

    /* `graphics-launch demo` → primitive showcase */
    if (argc >= 2 && strcmp(argv[1], "demo") == 0) {
        demo_run();
        vga_write("Demo shown. Press any key to return.\n");
        keyboard_getchar();
        vga_clear();
        return;
    }

    /* default: launch the real desktop */
    vga_write("Launching desktop... (ESC returns to shell)\n");
    int r = desktop_run();
    vga_clear();
    vga_set_color(0x07);
    if (r == 0) vga_write("Desktop closed.\n");
    else        vga_write("Desktop unavailable.\n");
}
REGISTER_COMMAND("graphics-launch", "Launch the graphical desktop",
                 graphics_launch_fn)