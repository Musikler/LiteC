#ifndef COMMANDS_H
#define COMMANDS_H
typedef struct {
    const char *name;
    const char *help;
    void (*func)(int argc, char **argv);
} command_t;
extern const command_t __start_commands[];
extern const command_t __stop_commands[];
#define REGISTER_COMMAND(_name,_help,_func) \
    __attribute__((section(".commands"), used)) \
    static const command_t _cmd_##_func = { _name, _help, _func };
#endif
