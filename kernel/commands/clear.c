#include "../commands.h"
#include "../vga.h"
static void clear_fn(int argc,char**argv){(void)argc;(void)argv;vga_clear();}
REGISTER_COMMAND("clear","Clear screen",clear_fn)
