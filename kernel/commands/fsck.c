#include "../commands.h"
#include "../vga.h"
#include "../ata.h"

typedef struct {
    char magic[8];
    unsigned int version;
    unsigned int inode_count;
    unsigned int data_start_lba;
} __attribute__((packed)) sb_t;

typedef struct {
    char name[32];
    unsigned int size;
    unsigned int first_lba;
    unsigned int block_count;
    unsigned int in_use;
    unsigned char pad[16];
} __attribute__((packed)) ino_t;

static void put_dec(unsigned int n) {
    char b[12]; int i = 0;
    if (!n) b[i++] = '0';
    while (n) { b[i++] = '0' + n % 10; n /= 10; }
    while (i--) vga_putchar(b[i]);
}

static void fsck_fn(int argc, char **argv) {
    (void)argc; (void)argv;
    if (!ata_present()) { vga_write("no disk\n"); return; }
    sb_t sb;
    if (ata_read_lba(0, 1, &sb) < 0) { vga_write("read err\n"); return; }
    vga_write("magic: ");
    for (int i = 0; i < 8; i++) vga_putchar(sb.magic[i]);
    vga_putchar('\n');
    ino_t ino[8];
    unsigned int used = 0;
    for (int s = 0; s < 8; s++) {
        if (ata_read_lba(1 + s, 1, ino) < 0) { vga_write("read err\n"); return; }
        for (int i = 0; i < 8; i++) {
            if (!ino[i].in_use) continue;
            used++;
            vga_write("  "); vga_write(ino[i].name);
            vga_write("  size="); put_dec(ino[i].size);
            vga_write(" lba=");   put_dec(ino[i].first_lba);
            vga_write(" blocks="); put_dec(ino[i].block_count);
            vga_putchar('\n');
        }
    }
    if (!used) vga_write("(no inodes in use)\n");
}
REGISTER_COMMAND("fsck", "Dump raw on-disk inodes", fsck_fn)