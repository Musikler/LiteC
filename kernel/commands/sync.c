#include "../commands.h"
#include "../vga.h"
#include "../litefs.h"
static void sync_fn(int argc, char **argv) {
    (void)argc; (void)argv;
    if (litefs_sync() == 0) vga_write("synced\n");
    else                    vga_write("sync failed (no disk?)\n");
}
REGISTER_COMMAND("sync", "Flush files to disk", sync_fn)