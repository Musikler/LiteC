#include "../commands.h"
#include "../vga.h"
#include "../litefs.h"
#include "../fs.h"
static void format_fn(int argc, char **argv) {
    (void)argc; (void)argv;
    if (litefs_format() == 0) { fs_init(); vga_write("disk formatted\n"); }
    else                      vga_write("format failed (no disk?)\n");
}
REGISTER_COMMAND("format", "Format data disk (erases files)", format_fn)