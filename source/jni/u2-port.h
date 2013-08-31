#ifndef U2_PORT
#define U2_PORT

#include "platform.h"

// eliminate far keyword, and pass-through segment and offset macros
#define far
#define FP_SEG(x)			(x)
#define FP_OFF(x)			(x)

extern char *MK_FP(int seg, int off);
extern void outportb(unsigned short int port, unsigned char val);
extern void outport(unsigned short int port, unsigned short int val);

extern void dis_partstart();
extern int  dis_sync();
extern int  dis_exit();

extern void tw_opengraph();
extern void tw_putpixel(int x, int y, int color);
extern int  tw_getpixel(int x, int y);
extern void tw_setpalette(char *pal);
extern void tw_setstart(int s);

#endif // U2_PORT

