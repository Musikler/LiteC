#ifndef MOUSE_H
#define MOUSE_H
void mouse_init(void);
void mouse_poll(void);              /* call every frame */
int  mouse_x(void);
int  mouse_y(void);
int  mouse_btn(void);               /* bit0=left, bit1=right, bit2=middle */
int  mouse_clicked(void);           /* 1 if a button was newly pressed this poll */
#endif