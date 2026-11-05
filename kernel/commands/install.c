#include "../commands.h"
#include "../vga.h"
#include "../installer.h"

static void install_fn(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_clear();
    installer_run();
    vga_clear();
    vga_set_color(0x07);
    vga_write("Back at shell.\n");
}
REGISTER_COMMAND("install", "Install LiteC to hard disk", install_fn)