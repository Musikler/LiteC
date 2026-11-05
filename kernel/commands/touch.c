#include "../commands.h"
#include "../vga.h"
#include "../fs.h"
#include "../litefs.h"

static void touch_fn(int argc, char **argv) {
    if (argc < 2) { vga_write("usage: touch <file>\n"); return; }
    int r = fs_create(argv[1]);
    if (r == 0) {
        vga_write("created\n");
        litefs_sync();
    } else if (r == -2) {
        vga_write("exists\n");
    } else {
        vga_write("error\n");
    }
}
REGISTER_COMMAND("touch", "Create empty file", touch_fn)