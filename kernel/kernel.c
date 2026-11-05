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
        vga_write("  ATA disk found\n");
        if (litefs_mount() != 0)
            panic("LiteFS mount failed: disk present but unreadable");
        vga_write("  LiteFS mounted\n");
    } else {
        vga_write("  No ATA disk -- filesystem is RAM-only\n");
    }

    vga_write("Type 'help' for commands.\n\n");
    shell_run();
    while (1);
}