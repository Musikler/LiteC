#include "../commands.h"
#include "../vga.h"

static inline void outw(unsigned short p, unsigned short v) {
    __asm__ volatile("outw %0,%1" :: "a"(v), "Nd"(p));
}

static void shutdown_fn(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_write("Shutting down...\n");

    /* QEMU / KVM */
    outw(0x604, 0x2000);
    /* Bochs / older QEMU */
    outw(0xB004, 0x2000);
    /* VirtualBox */
    outw(0x4004, 0x3400);

    /* If all else fails, halt the CPU */
    vga_write("ACPI shutdown failed. System halted.\n");
    __asm__ volatile("cli");
    for (;;) __asm__ volatile("hlt");
}
REGISTER_COMMAND("shutdown", "Power off the machine", shutdown_fn)