#include "installer.h"
#include "vga.h"
#include "keyboard.h"
#include "ata.h"
#include "litefs.h"
#include "fs.h"
#include "string.h"
#include "boothdd_bytes.h"
#include "stage2hdd_bytes.h"
#ifndef KERNEL_SECTORS
#define KERNEL_SECTORS 128
#endif

extern unsigned char boot_boothdd_bin[];
extern unsigned int   boot_boothdd_bin_len;
extern unsigned char boot_stage2hdd_bin[];
extern unsigned int   boot_stage2hdd_bin_len;

#define KERNEL_LOAD  0x10000
#define KERNEL_LBA   5
#define STAGE2_LBA   1
#define BOOT_LBA     0

#define SCR_W 80
#define SCR_H 25
#define VGA   ((unsigned short*)0xB8000)

/* attr = (bg<<4)|fg */
#define C_BORDER  0x3F   /* white on cyan    */
#define C_TITLE   0x1F   /* white on blue    */
#define C_TEXT    0x17   /* light gray on blue */
#define C_DIM     0x18   /* dark gray on blue  */
#define C_ACCENT  0x1E   /* yellow on blue   */
#define C_OK      0x1A   /* green on blue    */
#define C_ERR     0x1C   /* red on blue      */

static void scr_fill(unsigned char attr) {
    unsigned short v = ((unsigned short)attr << 8) | ' ';
    for (int i = 0; i < SCR_W * SCR_H; i++) VGA[i] = v;
}
static void put(int r, int c, char ch, unsigned char attr) {
    if (r < 0 || r >= SCR_H || c < 0 || c >= SCR_W) return;
    VGA[r * SCR_W + c] = ((unsigned short)attr << 8) | (unsigned char)ch;
}
static void str_at(int r, int c, const char *s, unsigned char attr) {
    while (*s && c < SCR_W - 1) put(r, c++, *s++, attr);
}
static void str_center(int r, const char *s, unsigned char attr) {
    int n = 0; while (s[n]) n++;
    str_at(r, (SCR_W - n) / 2, s, attr);
}
static void box(void) {
    put(0, 0, 0xC9, C_BORDER);
    for (int c = 1; c < SCR_W - 1; c++) put(0, c, 0xCD, C_BORDER);
    put(0, SCR_W - 1, 0xBB, C_BORDER);
    for (int r = 1; r < SCR_H - 1; r++) {
        put(r, 0, 0xBA, C_BORDER);
        put(r, SCR_W - 1, 0xBA, C_BORDER);
    }
    put(SCR_H - 1, 0, 0xC8, C_BORDER);
    for (int c = 1; c < SCR_W - 1; c++) put(SCR_H - 1, c, 0xCD, C_BORDER);
    put(SCR_H - 1, SCR_W - 1, 0xBC, C_BORDER);
}

static void draw_logo(void) {
    str_center(3, " _     ___ _____ _____ ____ ", C_ACCENT);
    str_center(4, "| |   |_ _|_   _| ____/ ___|", C_ACCENT);
    str_center(5, "| |    | |  | | |  _|| |    ", C_ACCENT);
    str_center(6, "| |___ | |  | | | |__| |___ ", C_ACCENT);
    str_center(7, "|_____|___| |_| |_____\\____|", C_ACCENT);
    str_center(9, "(Bumblebee)", C_DIM);
}

static int screen_welcome(void) {
    scr_fill(C_TEXT);
    box();
    draw_logo();
    str_center(12, "Welcome to the LiteC Installer", C_TITLE);
    str_center(14, "This will install LiteC to your hard disk.", C_TEXT);
    str_center(15, "All existing data on the target disk will be erased.", C_ERR);
    str_center(20, "[ENTER] continue", C_OK);
    str_center(21, "[ESC]   cancel",   C_DIM);
    for (;;) {
        char c = keyboard_getchar();
        if (c == '\n') return 1;
        if (c == 27)   return 0;
    }
}

static void put_dec(int r, int c, unsigned int n, unsigned char attr) {
    char b[12]; int i = 0;
    if (!n) b[i++] = '0';
    while (n) { b[i++] = '0' + n % 10; n /= 10; }
    while (i--) put(r, c++, b[i], attr);
}

static int screen_disk(void) {
    scr_fill(C_TEXT);
    box();
    str_center(2, "Select installation target", C_TITLE);

    str_at(6, 3, "Detected disk:", C_ACCENT);
    str_at(7, 5, "/dev/hda  (primary IDE master)", C_TEXT);
    str_at(8, 5, "QEMU HARDDISK (emulated ATA)", C_DIM);

    str_center(14, "The installer will write to this disk.", C_TEXT);
    str_center(15, "Make sure it is the right one.", C_DIM);

    str_center(20, "[ENTER] continue", C_OK);
    str_center(21, "[ESC]   back",     C_DIM);
    for (;;) {
        char c = keyboard_getchar();
        if (c == '\n') return 1;
        if (c == 27)   return 0;
    }
}

static int screen_confirm(void) {
    scr_fill(C_TEXT);
    box();
    str_center(2, "WARNING: ALL DATA ON THE DISK WILL BE ERASED", C_ERR);
    str_at(5, 3, "The following will be written:", C_ACCENT);
    str_at(6, 5, "Boot sector      LBA 0",         C_TEXT);
    str_at(7, 5, "Stage 2 loader   LBA 1-4",       C_TEXT);
    str_at(8, 5, "Kernel           LBA 5-132",     C_TEXT);
    str_at(9, 5, "LiteFS filesystem LBA 192+",     C_TEXT);

    str_at(13, 3, "Type  YES  to confirm, or press ESC to cancel:", C_ACCENT);

    char buf[4] = {0,0,0,0};
    int n = 0;
    for (;;) {
        put(15, 3, '>', C_ACCENT);
        put(15, 5, ' ', C_TEXT); put(15, 6, ' ', C_TEXT);
        put(15, 7, ' ', C_TEXT);
        str_at(15, 5, buf, C_OK);
        put(15, 5 + n, '_', C_OK);

        char c = keyboard_getchar();
        if (c == 27) return 0;
        if (c == '\n') {
            if (n == 3 && buf[0] == 'Y' && buf[1] == 'E' && buf[2] == 'S')
                return 1;
            return 0;
        }
        if (c == '\b' && n > 0) { buf[--n] = 0; continue; }
        if ((c == 'y' || c == 'Y' || c == 'e' || c == 'E' || c == 's' || c == 'S')
            && n < 3) {
            /* normalize to uppercase */
            if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
            buf[n++] = c; buf[n] = 0;
        }
    }
}

static void progress_bar(int r, int pct, const char *label) {
    int barw = 40;
    int fill = barw * pct / 100;
    put(r, 3, '[', C_TEXT);
    for (int i = 0; i < barw; i++)
        put(r, 4 + i, i < fill ? '#' : '-',
            i < fill ? C_OK : C_DIM);
    put(r, 4 + barw, ']', C_TEXT);
    put_dec(r, 4 + barw + 2, (unsigned int)pct, C_ACCENT);
    put(r, 4 + barw + 2 + (pct >= 100 ? 4 : (pct >= 10 ? 3 : 2)), '%', C_ACCENT);
    str_at(r + 2, 3, "                                        ", C_TEXT);
    str_at(r + 2, 3, label, C_TEXT);
}

static int install_to_disk(void) {
    progress_bar(7, 3, "Writing boot sector...");
    if (ata_write_lba(BOOT_LBA, 1, boot_boothdd_bin) < 0) return -1;

    progress_bar(7, 8, "Writing stage 2 loader...");
    int st2_sectors = (int)(boot_stage2hdd_bin_len / 512);
    for (int i = 0; i < st2_sectors; i++) {
        if (ata_write_lba(STAGE2_LBA + i, 1, boot_stage2hdd_bin + i * 512) < 0)
            return -2;
    }

    progress_bar(7, 15, "Writing kernel...");
    const unsigned char *k = (const unsigned char*)KERNEL_LOAD;
    for (int i = 0; i < KERNEL_SECTORS; i++) {
        if (ata_write_lba(KERNEL_LBA + i, 1, k + i * 512) < 0) return -3;
        if ((i & 7) == 0)
            progress_bar(7, 15 + (i * 65) / KERNEL_SECTORS, "Writing kernel...");
    }

    progress_bar(7, 85, "Formatting filesystem...");
    if (litefs_format() < 0) return -4;

    progress_bar(7, 100, "Installation complete.");
    return 0;
}

static int screen_progress_and_install(void) {
    scr_fill(C_TEXT);
    box();
    str_center(2, "Installing LiteC (Bumblebee)", C_TITLE);
    int rc = install_to_disk();
    if (rc != 0) {
        str_center(20, "INSTALL FAILED", C_ERR);
        str_at(21, 3, "Check the data disk is attached and try again.", C_TEXT);
        str_center(23, "[ENTER] return to shell", C_DIM);
        while (keyboard_getchar() != '\n') ;
    }
    return rc;
}

static int screen_complete(void) {
    scr_fill(C_TEXT);
    box();
    str_center(4,  "                  _",      C_OK);
    str_center(5,  "                 (_)",     C_OK);
    str_center(7,  "Installation complete!", C_TITLE);
    str_center(10, "LiteC has been installed to /dev/hda.", C_TEXT);
    str_center(12, "Remove the floppy image from your QEMU command line,", C_DIM);
    str_center(13, "then reboot to start LiteC from the hard disk.", C_DIM);
    str_center(19, "[ENTER] reboot now", C_OK);
    for (;;) {
        char c = keyboard_getchar();
        if (c == '\n') return 1;
    }
}

int installer_run(void) {
    if (!ata_present()) {
        scr_fill(C_TEXT);
        box();
        str_center(10, "ERROR: no hard disk detected", C_ERR);
        str_center(12, "Attach a disk to the IDE controller and retry.", C_TEXT);
        str_center(20, "[ENTER] return to shell", C_DIM);
        while (keyboard_getchar() != '\n') ;
        return -1;
    }

    if (!screen_welcome())       return 0;
    if (!screen_disk())          return 0;
    if (!screen_confirm())       return 0;

    int rc = screen_progress_and_install();
    if (rc == 0) screen_complete();
    return rc;
}