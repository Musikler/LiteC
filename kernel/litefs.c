#include "litefs.h"
#include "ata.h"
#include "fs.h"
#include "string.h"

#define MAGIC        "LITEFS01"
#define FS_VERSION   2u
#define INODE_COUNT  64
#define LITEFS_BASE  192                        /* sector where superblock lives */
#define INODE_LBAS   8                          /* 8 sectors * 8 inodes = 64     */
#define DATA_START   (LITEFS_BASE + 1 + INODE_LBAS)  /* 201 */

typedef struct {
    char         magic[8];
    unsigned int version;
    unsigned int inode_count;
    unsigned int data_start_lba;
} __attribute__((packed)) superblock_t;

typedef struct {
    char         name[FS_MAX_NAME];             /* 32 bytes */
    unsigned int size;                          /*  4       */
    unsigned int first_lba;                     /*  4       */
    unsigned int block_count;                   /*  4       */
    unsigned int in_use;                        /*  4       */
    unsigned char pad[16];                      /* 16 -> 64 total */
} __attribute__((packed)) inode_t;

static superblock_t  sb;
static inode_t       inodes[INODE_COUNT];
static unsigned char sector_buf[512];

/* ------------------------------------------------------------ */
/*  inode table I/O                                             */
/* ------------------------------------------------------------ */

static int write_inodes(void) {
    for (int s = 0; s < INODE_LBAS; s++) {
        memset(sector_buf, 0, sizeof(sector_buf));
        memcpy(sector_buf, &inodes[s * 8], 8 * sizeof(inode_t));
        if (ata_write_lba(LITEFS_BASE + 1 + s, 1, sector_buf) < 0)
            return -1;
    }
    return 0;
}

static int read_inodes(void) {
    for (int s = 0; s < INODE_LBAS; s++) {
        if (ata_read_lba(LITEFS_BASE + 1 + s, 1, sector_buf) < 0)
            return -1;
        memcpy(&inodes[s * 8], sector_buf, 8 * sizeof(inode_t));
    }
    return 0;
}

/* ------------------------------------------------------------ */
/*  format: write empty superblock + zeroed inode table         */
/* ------------------------------------------------------------ */

int litefs_format(void) {
    if (!ata_present()) return -1;

    memset(&sb, 0, sizeof(sb));
    memcpy(sb.magic, MAGIC, 8);
    sb.version        = FS_VERSION;
    sb.inode_count    = INODE_COUNT;
    sb.data_start_lba = DATA_START;

    if (ata_write_lba(LITEFS_BASE, 1, &sb) < 0) return -1;

    memset(inodes, 0, sizeof(inodes));
    return write_inodes();
}

/* ------------------------------------------------------------ */
/*  mount: read superblock, verify, load inodes, replay into RAM */
/* ------------------------------------------------------------ */

int litefs_mount(void) {
    if (!ata_present()) return -1;

    if (ata_read_lba(LITEFS_BASE, 1, &sb) < 0) return -1;

    int need_format = 0;
    if (memcmp(sb.magic, MAGIC, 8) != 0)                need_format = 1;
    else if (sb.version != FS_VERSION)                  need_format = 1;
    else if (sb.inode_count != INODE_COUNT)             need_format = 1;
    else if (sb.data_start_lba != DATA_START)           need_format = 1;

    if (need_format) {
        if (litefs_format() < 0) return -1;
        return 0;                     /* empty FS, nothing to replay */
    }

    if (read_inodes() < 0) return -1;

    /* Replay inodes into the RAM FS. Skip empty slots and zero-length files. */
    unsigned char buf[FS_MAX_SIZE];
    for (int i = 0; i < INODE_COUNT; i++) {
        if (!inodes[i].in_use) continue;
        if (inodes[i].size == 0) {
            /* still recreate the file entry, but with no content */
            fs_create(inodes[i].name);
            continue;
        }
        int n = (int)inodes[i].size;
        if (n > FS_MAX_SIZE) n = FS_MAX_SIZE;
        if (inodes[i].first_lba < DATA_START) continue;   /* corrupt guard   */
        if (inodes[i].block_count == 0)       continue;   /* corrupt guard   */

        if (fs_create(inodes[i].name) != 0 &&
            fs_create(inodes[i].name) != -2)              /* -2 = exists     */
            continue;

        if (ata_read_lba(inodes[i].first_lba,
                         (unsigned char)inodes[i].block_count,
                         buf) == 0)
            fs_write(inodes[i].name, (const char*)buf, n);
    }
    return 0;
}

/* ------------------------------------------------------------ */
/*  sync: RAM FS -> disk                                        */
/* ------------------------------------------------------------ */

static unsigned int sync_next_lba;
static int          sync_idx;
static int          sync_err;

static void sync_one(const char *name, int size) {
    if (sync_idx >= INODE_COUNT) { sync_err = 1; return; }

    inode_t *in = &inodes[sync_idx++];
    memset(in, 0, sizeof(*in));

    /* name: copy but guarantee null-termination */
    int i = 0;
    for (; name[i] && i < FS_MAX_NAME - 1; i++) in->name[i] = name[i];
    in->name[i] = 0;

    in->size   = (unsigned int)size;
    in->in_use = 1;

    if (size == 0) {
        in->first_lba    = 0;
        in->block_count  = 0;
        return;
    }

    int blocks = (size + 511) / 512;
    in->first_lba    = sync_next_lba;
    in->block_count  = (unsigned int)blocks;

    static unsigned char buf[FS_MAX_SIZE];
    int n = fs_read(name, (char*)buf, sizeof(buf));
    if (n < 0) { sync_err = 1; return; }

    int padded = blocks * 512;
    if (padded > (int)sizeof(buf)) padded = (int)sizeof(buf);
    if (n < padded) memset(buf + n, 0, padded - n);

    if (ata_write_lba(in->first_lba, (unsigned char)blocks, buf) < 0) {
        sync_err = 1;
        return;
    }
    sync_next_lba += blocks;
}

int litefs_sync(void) {
    if (!ata_present()) return -1;

    /* Rewrite the superblock in case it was never written
       (e.g. very first sync after a raw disk was attached). */
    memcpy(sb.magic, MAGIC, 8);
    sb.version        = FS_VERSION;
    sb.inode_count    = INODE_COUNT;
    sb.data_start_lba = DATA_START;
    if (ata_write_lba(LITEFS_BASE, 1, &sb) < 0) return -1;

    memset(inodes, 0, sizeof(inodes));
    sync_next_lba = DATA_START;
    sync_idx      = 0;
    sync_err      = 0;

    fs_list(sync_one);
    if (sync_err) return -1;

    return write_inodes();
}