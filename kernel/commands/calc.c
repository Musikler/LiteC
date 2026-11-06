#include "../commands.h"
#include "../vga.h"
#include "../string.h"

static int atoi_simple(const char *s) {
    int res = 0; int sign = 1;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') { res = res * 10 + (*s - '0'); s++; }
    return res * sign;
}

static void put_int(int n) {
    char b[12]; int i = 0;
    if (n == 0) { vga_putchar('0'); return; }
    if (n < 0) { vga_putchar('-'); n = -n; }
    while (n) { b[i++] = '0' + n % 10; n /= 10; }
    while (i--) vga_putchar(b[i]);
}

static void calc_fn(int argc, char **argv) {
    if (argc != 4) {
        vga_write("usage: calc <num1> <op> <num2>\n");
        vga_write("ops: + - * /\n");
        return;
    }
    int a = atoi_simple(argv[1]);
    int b = atoi_simple(argv[3]);
    char op = argv[2][0];
    int res = 0;

    if (op == '+') res = a + b;
    else if (op == '-') res = a - b;
    else if (op == '*') res = a * b;
    else if (op == '/') {
        if (b == 0) { vga_write("error: div by zero\n"); return; }
        res = a / b;
    } else { vga_write("unknown operator\n"); return; }

    put_int(a); vga_putchar(' '); vga_putchar(op); vga_putchar(' ');
    put_int(b); vga_write(" = "); put_int(res); vga_putchar('\n');
}
REGISTER_COMMAND("calc", "Simple calculator", calc_fn)