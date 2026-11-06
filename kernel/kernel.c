#include "kernel.h"
#include "vga.h"
#include "serial.h"
#include "keyboard.h"
#include "ata.h"
#include "litefs.h"
#include "fs.h"
#include "shell.h"
#include "panic.h"
#include "fb.h"
#include "mouse.h"

void kernel_main(void) {
    serial_init();
    fb_init();
    vga_init();
    keyboard_init();
    mouse_init();

    vga_set_color(0x0A);
    vga_write("LiteC (Bumblebee) v0.3\n");
    vga_set_color(0x07);
    vga_write("Initializing...\n");

    fs_init();

    if (ata_init() == 0) {
        if (litefs_mount() == 0) {
            vga_write("  LiteFS mounted\n");
            
            /* Load keyboard layout */
            char cfg[16];
            if (fs_read("keymap.cfg", cfg, sizeof(cfg)) > 0) {
                if (cfg[0] == 'd' && cfg[1] == 'e') {
                    keyboard_set_layout(1);
                    vga_write("  Keyboard: DE\n");
                } else {
                    vga_write("  Keyboard: EN\n");
                }
            }
        }
    }
    shell_run();
    while (1);
}