#include "../commands.h"
#include "../vga.h"
#include "../fs.h"
#include "../lcs.h"
#include "../string.h"

/* ---- helpers -------------------------------------------------------- */

static void put_dec(unsigned int n) {
    char b[12]; int i = 0;
    if (!n) b[i++] = '0';
    while (n) { b[i++] = '0' + n % 10; n /= 10; }
    while (i--) vga_putchar(b[i]);
}

static int ends_with(const char *s, const char *suf) {
    int n = (int)strlen(s);
    int m = (int)strlen(suf);
    if (n < m) return 0;
    for (int i = 0; i < m; i++)
        if (s[n - m + i] != suf[i]) return 0;
    return 1;
}

/* list callback: only show files named app_*.lcs */
static int g_list_count;
static void list_cb(const char *name, int size) {
    if (strncmp(name, "app_", 4) != 0) return;
    if (!ends_with(name, ".lcs"))      return;

    g_list_count++;

    /* strip "app_" prefix and ".lcs" suffix for a clean display name */
    const char *n = name + 4;
    int len = (int)strlen(n) - 4;
    if (len < 0) len = 0;

    vga_write("  ");
    for (int i = 0; i < len; i++) vga_putchar(n[i]);

    /* pad to column 24 */
    int pad = 24 - len;
    while (pad-- > 0) vga_putchar(' ');

    put_dec((unsigned int)size);
    vga_write(" bytes\n");
}

/* ---- subcommands ---------------------------------------------------- */

static void do_list(void) {
    g_list_count = 0;
    vga_write("Installed LiteC software:\n");
    fs_list(list_cb);
    if (g_list_count == 0) vga_write("  (none)\n");
    else {
        vga_write("  ");
        put_dec((unsigned int)g_list_count);
        vga_write(" package(s) installed\n");
    }
}

static void do_run(const char *name) {
    int r = lcs_run(name);
    if (r == 0) { vga_write("ok\n"); return; }

    vga_write("failed: ");
    switch (r) {
        case -1: vga_write("no framebuffer (run 'desktop' first)\n"); break;
        case -2: vga_write("a program is already running\n");         break;
        case -3: vga_write("file not found\n");                       break;
        case -4: vga_write("bad magic or version\n");                 break;
        case -5: vga_write("payload too large\n");                    break;
        case -6: vga_write("file truncated\n");                       break;
        case -7: vga_write("program exited abnormally\n");            break;
        default: vga_write("unknown error\n");
    }
}

static void do_help(void) {
    vga_write("usage:\n");
    vga_write("  lcsm list            list installed .lcs packages\n");
    vga_write("  lcsm run <name>      run package (name without app_ / .lcs)\n");
    vga_write("  lcsm info <name>     show package size and location\n");
}

static void do_info(const char *name) {
    /* build the disk filename: app_<name>.lcs */
    char disk[64];
    int i = 0;
    const char *pfx = "app_";
    while (pfx[i]) { disk[i] = pfx[i]; i++; }
    int j = 0;
    while (name[j] && i < 58) disk[i++] = name[j++];
    const char *sfx = ".lcs";
    int k = 0;
    while (sfx[k]) disk[i++] = sfx[k++];
    disk[i] = 0;

    char buf[FS_MAX_SIZE];
    int n = fs_read(disk, buf, sizeof(buf));
    if (n < 0) { vga_write("no such package\n"); return; }

    vga_write("file:   "); vga_write(disk); vga_putchar('\n');
    vga_write("size:   "); put_dec((unsigned int)n); vga_write(" bytes\n");

    /* parse the 24-byte header for the display name */
    if (n >= 24 && memcmp(buf, "LCS1", 4) == 0) {
        unsigned int name_len = *(unsigned int*)(buf + 12);
        unsigned int paylen   = *(unsigned int*)(buf + 20);
        if (name_len > 0 && name_len <= 63 && 24 + (int)name_len <= n) {
            vga_write("title:  ");
            for (unsigned int p = 0; p < name_len; p++)
                vga_putchar(buf[24 + p]);
            vga_putchar('\n');
        }
        vga_write("payload: "); put_dec(paylen); vga_write(" bytes\n");
    } else {
        vga_write("header: invalid\n");
    }
}

/* ---- entry ---------------------------------------------------------- */

static void lcsm_fn(int argc, char **argv) {
    if (argc < 2) { do_help(); return; }

    if (strcmp(argv[1], "list") == 0) {
        do_list();
    } else if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) { vga_write("usage: lcsm run <name>\n"); return; }
        do_run(argv[2]);
    } else if (strcmp(argv[1], "info") == 0) {
        if (argc < 3) { vga_write("usage: lcsm info <name>\n"); return; }
        do_info(argv[2]);
    } else if (strcmp(argv[1], "help") == 0) {
        do_help();
    } else {
        vga_write("unknown subcommand: ");
        vga_write(argv[1]);
        vga_putchar('\n');
        do_help();
    }
}
REGISTER_COMMAND("lcsm", "LiteC Software Manager (list/run/info)", lcsm_fn)