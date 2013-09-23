//
// ported from koeb.asm
//
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../../u2-port.h"

// in ../../u2-port.c
extern char circle[24000];
extern char circle2[8000];

// in ../../sin1024.c
extern int sin1024[];

// ################################################################

static uint16_t sizefade;
static uint16_t rotspeed;
static uint16_t palfader;
static uint8_t zumplane = 0x11;

static const uint8_t flip8[] =
{
	0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240, 8, 136, 72, 200,
	40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248, 4, 132, 68, 196, 36, 164, 100,
	228, 20, 148, 84, 212, 52, 180, 116, 244, 12, 140, 76, 204, 44, 172, 108, 236, 28, 156,
	92, 220, 60, 188, 124, 252, 2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178,
	114, 242, 10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250, 6,
	134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246, 14, 142, 78, 206,
	46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254, 1, 129, 65, 193, 33, 161, 97, 225,
	17, 145, 81, 209, 49, 177, 113, 241, 9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217,
	57, 185, 121, 249, 5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
	13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253, 3, 131, 67, 195,
	35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243, 11, 139, 75, 203, 43, 171, 107, 235,
	27, 155, 91, 219, 59, 187, 123, 251, 7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215,
	55, 183, 119, 247, 15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255
};

static uint8_t *circles[8];

static uint8_t pal[32 * 3];

static const uint8_t pal0[] =
{
	// pal0
	0, 30, 40,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 30, 40,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	// pal1
	0, 0 * 7 / 9, 0,
	10, 10 * 7 / 9, 10,
	20, 20 * 7 / 9, 20,
	30, 30 * 7 / 9, 30,
	40, 40 * 7 / 9, 40,
	50, 50 * 7 / 9, 50,
	60, 60 * 7 / 9, 60,
	30, 30 * 7 / 9, 30,
	0, 0 * 7 / 9, 0,
	10, 10 * 7 / 9, 10,
	20, 20 * 7 / 9, 20,
	30, 30 * 7 / 9, 30,
	40, 40 * 7 / 9, 40,
	50, 50 * 7 / 9, 50,
	60, 60 * 7 / 9, 60,
	30, 30 * 7 / 9, 30,
	// pal2
	50, 50 * 6 / 9, 50,
	60, 60 * 6 / 9, 60,
	30, 30 * 6 / 9, 30,
	0, 0 * 6 / 9, 0,
	10, 10 * 6 / 9, 10,
	20, 20 * 6 / 9, 20,
	30, 30 * 6 / 9, 30,
	40, 40 * 6 / 9, 40,
	50, 50 * 6 / 9, 50,
	60, 60 * 6 / 9, 60,
	30, 30 * 6 / 9, 30,
	0, 0 * 6 / 9, 0,
	10, 10 * 6 / 9, 10,
	20, 20 * 6 / 9, 20,
	30, 30 * 6 / 9, 30,
	40, 40 * 6 / 9, 40
};

static const uint8_t sinuspower = 0;
char power1[256 * 16];

static void bltline(char *dest, const char *src, uint8_t start_at_plane, int copy_planes)
{
	outportb(0x3c4, 2);
	while (copy_planes--)
	{
		int zzz;
		outportb(0x3c5, start_at_plane);
		for (zzz = 0; zzz < 40; zzz++)
			dest[zzz] = src[zzz];
		src += 40;
		start_at_plane <<= 1;
	}
}

static void bltlinerev(char *dest, const char *src, uint8_t start_at_plane, int copy_planes)
{
	outportb(0x3c4, 2);
	while (copy_planes--)
	{
		int zzz;
		outportb(0x3c5, start_at_plane);
		for (zzz = 0; zzz < 40; zzz++)
			dest[zzz] = flip8[src[39 - zzz]];
		src += 40;
		start_at_plane <<= 1;
	}
}

static void resetmode13(void)
{
	int i;

	dis_waitb();
	outportb(0x3c8, 0);
	for (i = 0; i < 768; i++)
		outportb(0x3c9, 0);
	dis_waitb();
	int10h(0x13);
	inportb(0x3da);
	for (i = 0; i < 16; i++)
	{
		outportb(0x3c0, (uint8_t)i);
		outportb(0x3c0, (uint8_t)i);
	}
	outportb(0x3c0, 0x11);
	outportb(0x3c0, 255);
	outportb(0x3c0, 32);
	// clear pal
	outportb(0x3c8, 0);
	for (i = 0; i < 768; i++)
		outportb(0x3c9, 0);
}

static void mixpal(uint8_t *dest, const uint8_t *src, int bytes, int dx)
{
	if (dx <= 256)
		while (bytes--)
			*dest++ = (uint8_t)((*src++ * dx) >> 8);
	else
	{
		const int bx = dx - 256;
		while (bytes--)
		{
			const int ax = *src++ + bx;
			*dest++ = (uint8_t)((ax < 64) ? ax : 63);
		}
	}
}

static void outpal(uint8_t start, const uint8_t *src, int bytes)
{
	outportb(0x3c8, start);
	while (bytes--)
		outportb(0x3c9, *src++);
}

static void waitb(void)
{
	dis_waitb();
}

static void rotate1(uint8_t *dest, const uint8_t *src)
{
	int si = 0, carry = 0, cx = 32000 / 32 - 2, zzz = 32;
	for (;;)
	{
		si += zzz;
		if (!--cx)
			break;
		for (zzz = 0; zzz < 32; zzz++)
		{
			const int al = src[si + zzz], old_carry = carry;
			carry = al & 1;
			dest[si + zzz] = (uint8_t)((old_carry << 7) | (al >> 1));
		}
	}
}

static uint16_t framecount;
static uint16_t palanimc;
static uint16_t palanimc2;
static uint16_t scrnpos;
static uint16_t scrnposl;
static uint16_t scrnx;
static uint16_t scrny;
static uint16_t scrnrot;
static uint16_t sinurot;
static uint16_t overrot;
static uint16_t overx;
static uint16_t overya;
static uint16_t patdir = (uint16_t)-3;

static uint8_t *memseg;

static void init_interference(void)
{
	char *const vram = MK_FP(0x0a000, 0);
	char *di, *bp;
	const char *si;
	int zzz;

	outport(0x3d4, 0x2813);

	// get mem for circles
	memseg = (uint8_t *)malloc(32768 * 8);
	for (zzz = 0; zzz < 8; zzz++)
		circles[zzz] = memseg + (zzz << 15);

	si = circle2;
	di = vram;
	bp = vram + 80 * 399;
	for (zzz = 0; zzz < 200; zzz++)
	{
		bltline(di, si, 4, 1);
		bltlinerev(di + 40, si, 4, 1);
		bltline(bp, si, 4, 1);
		bltlinerev(bp + 40, si, 4, 1);
		di += 80;
		bp -= 80;
		si += 40;
	}

	outport(0x3ce, 0x204);

	memcpy(circles[0], vram, 32000);
	for (zzz = 0; zzz < 7; zzz++)
		rotate1(circles[zzz + 1], circles[zzz]);

	si = circle;
	di = vram;
	bp = vram + 80 * 399;
	for (zzz = 0; zzz < 200; zzz++)
	{
		bltline(di, si, 1, 3); // start at plane 1, copy 3 planes
		bltlinerev(di + 40, si, 1, 3); // start at plane 1, copy 3 planes
		bltline(bp, si, 1, 3); // start at plane 1, copy 3 planes
		bltlinerev(bp + 40, si, 1, 3); // start at plane 1, copy 3 planes
		di += 80;
		bp -= 80;
		si += 40 * 3;
	}

	framecount = 0;
}

static void do_interference(void)
{
/*
@@aga:
	waitb();
	mov	dx,3c0h
	mov	al,13h
	out	dx,al
	mov	al,byte ptr cs:scrnposl
	out	dx,al
	mov	al,32
	out	dx,al
	
	xor	al,al
	mov	si,OFFSET pal
	mov	cx,16*3
	outpal(al, si, cx);

	mov	si,cs:palanimc
	add	si,cs:patdir
	cmp	si,0
	jge	@@a11
	mov	si,8*3-3
@@a11:
	cmp	si,8*3
	jb	@@a1
	xor	si,si
@@a1:
	mov	cs:palanimc,si
	mov	cs:palanimc2,si

	mov	dx,cs:palfader
	add	dx,2
	cmp	dx,512
	jb	@@pf1
	mov	dx,512
@@pf1:
	mov	cs:palfader,dx

	mov	si,cs:palanimc
	add	si,OFFSET pal0
	mov	di,OFFSET pal
	mov	cx,8*3
	mixpal(di, si, cx, dx);
	mov	si,cs:palanimc
	add	si,OFFSET pal0
	mov	di,OFFSET pal+8*3
	mov	cx,8*3
	mixpal(di, si, cx, dx);

	mov	si,OFFSET pal
	mov	al,0
	mov	cx,16*3
	outpal(al, si, cx);

	jmp	@@OVER3
	mov	dx,3c4h
	mov	al,2
	mov	ah,cs:zumplane
	rol	ah,1
	mov	cs:zumplane,ah
	out	dx,ax

	mov	ax,0a000h
	mov	es,ax
	xor	si,si
	mov	di,cs:scrnpos
	mov	bp,cs:sinurot
	add	bp,7*2
	and	bp,2047
	mov	cs:sinurot,bp
	mov	cx,200
@@cp1:
	zzz=0
	push	si
	add	bp,9*2
	and	bp,2047
	mov	bx,cs:_sin1024[bp]
	sar	bx,3
	mov	bh,cs:sinuspower
	movsx	ax,byte ptr cs:power1[bx]
	sub	ax,cs:scrnposl
	add	ax,cs:overx
	mov	bx,ax
	and	bx,7
	shl	bx,1
	neg	bx
	mov	ds,cs:circles[bx+7*2]
	sar	ax,3
	add	si,ax
	add	si,cs:overya
	REPT	40/4+1
	mov	eax,ds:[si+zzz]
	mov	es:[di+zzz],eax
	zzz=zzz+4
	ENDM
	pop	si
	add	di,80
	add	si,80
	dec	cx
	jz	@@cp0
	jmp	@@cp1
@@cp0:
@@OVER3:
	;MOVE
	mov	bx,6
	int	0fch
	;bx=row
	and	bx,7
	cmp	bx,0
	jne	@@m1
	mov	cs:patdir,-3
@@m1:
	cmp	bx,4
	jne	@@m2
	mov	cs:patdir,-3 ;-3
@@m2:
	mov	bx,cs:scrnrot
	add	bx,5
	and	bx,1023
	mov	cs:scrnrot,bx

	cmp	cs:framecount,64
	jb	@@szf1
	inc	cs:rotspeed
	mov	ax,cs:sizefade
	cmp	ax,16834
	jge	@@1
@@1:
	mov	cs:sizefade,ax
@@szf1:
	shl	bx,1
	mov	ax,cs:_sin1024[bx]
	imul	cs:sizefade
	mov	ax,dx
	add	ax,160
	mov	cs:scrnx,ax
	add	bx,256*2
	and	bx,1024*2-1
	mov	ax,cs:_sin1024[bx]
	imul	cs:sizefade
	mov	ax,dx
	add	ax,100
	mov	cs:scrny,ax

	mov	bx,cs:overrot
	add	bx,cs:rotspeed
	and	bx,1023
	mov	cs:overrot,bx

	shl	bx,1
	mov	ax,cs:_sin1024[bx]
	sar	ax,2
	imul	cs:sizefade
	mov	ax,dx
	add	ax,160
	mov	cs:overx,ax
	add	bx,256*2
	and	bx,1024*2-1
	mov	ax,cs:_sin1024[bx]
	sar	ax,2
	imul	cs:sizefade
	mov	ax,dx
	add	ax,100
	mov	bx,80
	mul	bx
	mov	cs:overya,ax

	mov	ax,cs:scrnx
	mov	bx,ax
	and	ax,7
	mov	cs:scrnposl,ax
	mov	ax,80
	mul	cs:scrny
	sar	bx,3
	add	ax,bx
	mov	cs:scrnpos,ax

	mov	bx,cs:scrnpos
	mov	dx,3d4h
	mov	al,0ch
	mov	ah,bh
	out	dx,ax
	inc	al
	mov	ah,bl
	out	dx,ax

	cmp	cs:framecount,192
	jb	@@asd2
	test	cs:framecount,3
	jnz	@@asd2
@@asd2:
@@p1:
	inc	cs:framecount
	cmp	cs:framecount,256
	je	@@xx
	mov	bx,2
	int	0fch
	or	ax,ax
	jz	@@aga
@@xx:
	ret
*/
}

void dointerference2(void)
{
	resetmode13();
	init_interference();
	do_interference();
	free(memseg);
}
