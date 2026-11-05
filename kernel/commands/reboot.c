#include "../commands.h"
#include "../vga.h"

static inline void outb(unsigned short p, unsigned char v)
{ __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p)); }
static inline unsigned char inb(unsigned short p)
{ unsigned char r; __asm__ volatile("inb %1,%0" : "=a"(r) : "Nd"(p)); return r; }

static void reboot_fn(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_write("Rebooting...\n");
    while (inb(0x64) & 0x02) ;
    outb(0x64, 0xFE);
    /* if we get here, force a triple fault */
    __asm__ volatile("lidt (0); int3");
    for (;;) __asm__ volatile("hlt");
}
REGISTER_COMMAND("reboot", "Reboot the machine", reboot_fn)