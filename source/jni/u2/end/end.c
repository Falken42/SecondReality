#include <stdio.h>
#include "../../u2-port.h"

#include "../../readp.h"

extern char pic[];

#if 0 // unused
char *scroller[]={
"      S e c o n d  R e a l i t y     ",
"                                     ",
"     has entirely been created by    ",
"       the Future Crew in 1993       ",
"                                     ",
"         First presented at          ",
"       Assembly 1993 - Finland       ",
/*"                                     ",
"                                     ",
"          credits...                 ",
"                                     ",
"      (this is surely the best       ",
"       endscroller ever!)            ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
*/};
#endif

static char	pal2[768];
static char	palette[768];
static char	rowbuf[640];

end_main()
{
	char far *vram=MK_FP(0x0a000,0);
	int	a,b,c,y;
	dis_partstart();
	outp(0x3c0,0x11);
	outp(0x3c0,255);
	outp(0x3c0,0x20);
	dis_waitb();
	outp(0x3c8,0);
	for(a=0;a<768-3;a++) outp(0x3c9,63);
//	_asm mov ax,13h+80h
//	_asm int 10h	
//	inittwk();
	outport(0x3d4,0x000c);
	outport(0x3d4,0x000d);
	outp(0x3d4,9);
	outp(0x3d5, (inp(0x3d5) & ~0x80) & ~31);
	outp(0x3c0,0x11);
	outp(0x3c0,0);
	outp(0x3c0,32);
	dis_waitb();
	outp(0x3c8,0);
	for(a=0;a<768-3;a++) outp(0x3c9,63);
	for(a=0;a<32;a++) dis_waitb();

	readp(palette,-1,pic);
	for(y=0;y<400;y++)
	{
		readp(rowbuf,y,pic);
		lineblit(vram+(unsigned)y*80U,rowbuf);
	}
	
	for(c=0;c<=128;c++)
	{
		for(a=0;a<768-3;a++) pal2[a]=((128-c)*63+palette[a]*c)/128;
		dis_waitb();
		setpalarea(pal2,0,255);
	}
	for(a=0;a<500 && !dis_exit();a++) // TODO: correctly implement dis_musplus() to make this work with "a<5000"
	{
		dis_waitb();
		if(dis_musplus()>-16) break;
	}
	for(c=63;c>=0;c--)
	{
		for(a=0;a<768-3;a++) pal2[a]=(palette[a]*c)/64;
		dis_waitb();
		setpalarea(pal2,0,255);
	}
	/*
	_asm mov ax,4
	_asm int 10h
	outp(0x3c8,0); outp(0x3c9,0); outp(0x3c9,0); outp(0x3c9,0);
	for(a=0;a<768-3;a++) outp(0x3c9,40);
	for(b=0;b<25;b++) printf("\n");
	for(a=0;a<24 && !kbhit();a++)
	{
		printf("%s\n",scroller[a]);
		for(b=0;b<20;b++) dis_waitb();
	}
	*/
}
