#ifndef U2_PORT
#define U2_PORT

#include "platform.h"

// eliminate far keyword, and pass-through segment and offset macros
#define far
#define FP_SEG(x)			(x)
#define FP_OFF(x)			(x)

// MSVC defines inp() and outp() as intrinsic functions, so work around their usage
#define inp					inportb
#define outp				outportb

extern char *MK_FP(int seg, int off);

unsigned char inportb(unsigned short int port);
extern void outportb(unsigned short int port, unsigned char val);
extern void outport(unsigned short int port, unsigned short int val);

extern int  dis_indemo();
extern int  dis_musplus();
extern void dis_partstart();
extern int  dis_waitb();
extern int  dis_sync();
extern int  dis_exit();

extern void tw_opengraph();
extern void tw_putpixel(int x, int y, int color);
extern int  tw_getpixel(int x, int y);
extern void tw_setpalette(char *pal);
extern void tw_setstart(int s);

// functions which need wrapping because of C library differences
extern int fcrand();

#endif // U2_PORT

