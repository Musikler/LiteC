#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_UP    0x100
#define KEY_DOWN  0x101
#define KEY_LEFT  0x102
#define KEY_RIGHT 0x103

void keyboard_init(void);
void keyboard_set_layout(int is_german);
char keyboard_getchar(void);
int keyboard_getkey(void); /* Returns char or KEY_* macros */

#endif