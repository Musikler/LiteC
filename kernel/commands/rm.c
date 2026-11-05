#include "../commands.h"
#include "../vga.h"
#include "../fs.h"
#include "../litefs.h"

static void rm_fn(int argc, char **argv) {
    if (argc < 2) { vga_write("usage: rm <file>\n"); return; }
    if (fs_delete(argv[1]) < 0) { vga_write("no such file\n"); return; }
    vga_write("deleted\n");
    litefs_sync();
}
REGISTER_COMMAND("rm", "Delete file", rm_fn)