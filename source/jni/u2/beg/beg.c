#include "../../u2-port.h"

#include "readp.c"

extern char pic[];

char	pal2[768];
char	palette[768];
char	rowbuf[640];

beg_main()
{
    char    far *vram=MK_FP(0x0a000,0);
	int	a,b,c,y;
	unsigned char al;

	dis_partstart();
	outp(0x3c4,2);
	outp(0x3c5,15);
	memset(vram,15,32768);
	memset(vram+32768,15,32768);
	//_asm mov ax,80h+13h
	//_asm int 10h
	for(a=0;a<32;a++) dis_waitb();
	outp(0x3c8,0);
	for(a=0;a<255;a++)
	{
		outp(0x3c9,63);
		outp(0x3c9,63);
		outp(0x3c9,63);
	}
	outp(0x3c9,0);
	outp(0x3c9,0);
	outp(0x3c9,0);
	inp(0x3da);
	outp(0x3c0,0x11);
	outp(0x3c0,255);
	outp(0x3c0,0x20);
	//inittwk();

	outport(0x3D4, 0x000C);
	outport(0x3D4, 0x000D);
	outp(0x3D4, 9);
	al = inp(0x3D5);
	al &= ~0x80;
	al &= ~31;
	outp(0x3D5, al);
	outp(0x3C0, 0x11);
	outp(0x3C0, 0);
	outp(0x3C0, 32);

	outp(0x3C0, 0x11);
	outp(0x3C0, 255);
	outp(0x3C0, 0x20);

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
		setpalarea(pal2,0,254);
	}
	setpalarea(palette,0,254);
}
