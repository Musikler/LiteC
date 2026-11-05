#include "../commands.h"
#include "../vga.h"
#include "../fs.h"
#include "../litefs.h"
#include "../string.h"

static void write_fn(int argc, char **argv) {
    if (argc < 3) { vga_write("usage: write <file> <text...>\n"); return; }
    static char buf[FS_MAX_SIZE];
    int len = 0;
    for (int i = 2; i < argc; i++) {
        if (i > 2) buf[len++] = ' ';
        size_t l = strlen(argv[i]);
        if (len + (int)l >= (int)sizeof(buf) - 1) break;
        memcpy(buf + len, argv[i], l); len += l;
    }
    buf[len++] = '\n';
    if (fs_write(argv[1], buf, len) < 0) { vga_write("error\n"); return; }
    vga_write("ok\n");
    litefs_sync();
}
REGISTER_COMMAND("write", "Write text to file", write_fn)