#include "../commands.h"
#include "../vga.h"
#include "../string.h"
static void help_fn(int argc,char**argv){
    (void)argc;(void)argv;
    vga_write("Commands:\n");
    for(const command_t*c=__start_commands;c<__stop_commands;c++){
        vga_write("  ");vga_write(c->name);
        int pad=10-(int)strlen(c->name);while(pad-->0)vga_putchar(' ');
        vga_write(c->help);vga_putchar('\n');
    }
}
REGISTER_COMMAND("help","Show this help",help_fn)
