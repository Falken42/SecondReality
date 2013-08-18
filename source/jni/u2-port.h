#ifndef U2_PORT
#define U2_PORT

#include <stdlib.h>
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "SecondReality", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "SecondReality", __VA_ARGS__))

// eliminate far keyword, make FP_SEG and FP_OFF pass-through
#define far
#define FP_SEG(x)			(x)
#define FP_OFF(x)			(x)

extern char *MK_FP(int seg, int off);
extern void outportb(unsigned short int port, unsigned char val);
extern void outport(unsigned short int port, unsigned short int val);

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
