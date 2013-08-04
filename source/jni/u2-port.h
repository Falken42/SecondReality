#ifndef U2_PORT
#define U2_PORT

#include <stdlib.h>

// eliminate far keyword, make FP_SEG and FP_OFF pass-through
#define far
#define FP_SEG(x)			(x)
#define FP_OFF(x)			(x)

extern char *MK_FP(int seg, int off);

extern int  outline(char *f, char *t);

extern void dis_partstart();
extern int  dis_sync();
extern int  dis_exit();

extern int  init_copper();

extern void tw_opengraph();
extern void tw_putpixel(int x, int y, int color);
extern int  tw_getpixel(int x, int y);
extern void tw_setpalette(void *pal);

#endif // U2_PORT

