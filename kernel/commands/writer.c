#include "../commands.h"
#include "../vga.h"
#include "../fs.h"
#include "../keyboard.h"
#include "../string.h"
#include "../sound.h"

#define MAX_LINES 100
#define MAX_COLS  79

static char text_buf[MAX_LINES][MAX_COLS + 1];
static int num_lines = 0;
static int cx = 0, cy = 0;
static int scroll_y = 0;

static void redraw(const char *filename) {
    vga_clear();
    vga_set_color(0x1F); vga_write(" LiteC Writer ");
    vga_write("  File: "); vga_write(filename);
    vga_set_color(0x17);
    vga_putchar('\n');
    for (int i = 0; i < 23; i++) {
        int line = i + scroll_y;
        if (line < num_lines) vga_write(text_buf[line]);
        vga_putchar('\n');
    }
    vga_set_color(0x70); 
    vga_write(" Ctrl+S: Save & Exit | ESC: Exit ");
    vga_set_color(0x07);
}

static void writer_fn(int argc, char **argv) {
    if (argc < 2) { vga_write("usage: writer <file>\n"); return; }
    const char *fname = argv[1];

    num_lines = 0; cx = 0; cy = 0; scroll_y = 0;
    for (int i = 0; i < MAX_LINES; i++) text_buf[i][0] = 0;

    char disk_buf[FS_MAX_SIZE];
    int n = fs_read(fname, disk_buf, sizeof(disk_buf));
    if (n > 0) {
        int line = 0, col = 0;
        for (int i = 0; i < n; i++) {
            if (disk_buf[i] == '\n' || col >= MAX_COLS) {
                text_buf[line][col] = 0;
                line++; col = 0;
                if (line >= MAX_LINES) break;
            } else {
                text_buf[line][col++] = disk_buf[i];
            }
        }
        if (col > 0) text_buf[line][col] = 0;
        num_lines = line + 1;
    }
    if (num_lines == 0) num_lines = 1;

    for (;;) {
        redraw(fname);
        // Render cursor
        if (cy >= scroll_y && cy < scroll_y + 23) {
            // VGA hardware cursor not implemented, just a visual block
            int px = (cx < MAX_COLS) ? cx : MAX_COLS;
            // Note: vga_write doesn't expose direct memory here, so we skip drawing the cursor block for simplicity
        }

        int k = keyboard_getkey();

        if (k == 27) { break; } // ESC
        if (k == 19) { // Ctrl+S
            char out[FS_MAX_SIZE]; int len = 0;
            for (int i = 0; i < num_lines; i++) {
                for (int j = 0; text_buf[i][j]; j++) {
                    if (len < FS_MAX_SIZE - 2) out[len++] = text_buf[i][j];
                }
                if (len < FS_MAX_SIZE - 2) out[len++] = '\n';
            }
            fs_write(fname, out, len);
            vga_write("Saved!\n"); speaker_beep(1500, 50);
            break;
        }
        
        if (k == KEY_UP) { if (cy > 0) cy--; if (cy < scroll_y) scroll_y--; }
        else if (k == KEY_DOWN) { if (cy < num_lines - 1) cy++; if (cy >= scroll_y + 23) scroll_y++; }
        else if (k == KEY_LEFT) { if (cx > 0) cx--; }
        else if (k == KEY_RIGHT) { if (text_buf[cy][cx]) cx++; }
        else if (k == '\n') {
            if (num_lines < MAX_LINES) {
                for (int i = num_lines; i > cy + 1; i--) {
                    for (int j = 0; j < MAX_COLS; j++) text_buf[i][j] = text_buf[i-1][j];
                }
                int old_len = 0; while (text_buf[cy][old_len]) old_len++;
                int new_len = old_len - cx;
                for (int i = 0; i < new_len; i++) text_buf[cy+1][i] = text_buf[cy][cx+i];
                text_buf[cy+1][new_len] = 0;
                text_buf[cy][cx] = 0;
                num_lines++;
                cy++; cx = 0;
            }
        }
        else if (k == '\b') {
            if (cx > 0) {
                int len = 0; while (text_buf[cy][len]) len++;
                for (int i = cx - 1; i < len; i++) text_buf[cy][i] = text_buf[cy][i+1];
                cx--;
            } else if (cy > 0) {
                int prev_len = 0; while (text_buf[cy-1][prev_len]) prev_len++;
                int this_len = 0; while (text_buf[cy][this_len]) this_len++;
                if (prev_len + this_len < MAX_COLS) {
                    for (int i = 0; i <= this_len; i++) text_buf[cy-1][prev_len + i] = text_buf[cy][i];
                    for (int i = cy; i < num_lines - 1; i++) {
                        for (int j = 0; j < MAX_COLS; j++) text_buf[i][j] = text_buf[i+1][j];
                    }
                    num_lines--; cy--; cx = prev_len;
                }
            }
        }
        else if (k >= 32 && k < 127) {
            int len = 0; while (text_buf[cy][len]) len++;
            if (len < MAX_COLS) {
                for (int i = len; i >= cx; i--) text_buf[cy][i+1] = text_buf[cy][i];
                text_buf[cy][cx] = (char)k;
                cx++;
            }
        }
    }
    vga_clear();
}
REGISTER_COMMAND("writer", "Simple text editor", writer_fn)