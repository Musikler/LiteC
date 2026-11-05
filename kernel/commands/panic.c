#include "../commands.h"
#include "../panic.h"
#include "../vga.h"

static void panic_fn(int argc, char **argv) {
    if (argc < 2) panic("Panic triggered by user (no reason given)");
    panic(argv[1]);
}
REGISTER_COMMAND("panic", "Trigger a kernel panic (for testing)", panic_fn)