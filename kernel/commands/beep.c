#include "../commands.h"
#include "../vga.h"
#include "../sound.h"

static void beep_fn(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_write("Beep!\n");
    speaker_beep(1000, 200);
}
REGISTER_COMMAND("beep", "Play a test beep", beep_fn)