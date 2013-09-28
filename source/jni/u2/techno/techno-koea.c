//
// ported from koea.asm
//
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../../u2-port.h"

// in ../../u2-port.c
extern char circle[24000];
extern char circle2[8000];

static uint16_t _rows[200];
static uint16_t _blit16t[256];
static char *_vbufseg;
static uint16_t clipleft;

static unsigned polyisides;
static int polyixy[16][2];
static unsigned polysides;
static int polyxy[16][2];
#include "polyclip.c"

// in ../../sin1024.c
extern int sin1024[];

static void blitinit(void);
static void blit16(const char *vbuf, char *vram);
static void blit16b(const char *vbuf, char *vram);

void asminit(char *vbuf)
{
	_vbufseg = vbuf;
	blitinit();
}

void asmdoit(const char *vbuf, char *vram)
{
	blit16(vbuf, vram);
}

void asmdoit2(const char *vbuf, char *vram)
{
	blit16b(vbuf, vram);
}

static void blitinit(void)
{
	uint16_t *bx = _rows, cx;
	for (cx = 0; cx < 40 * 200; cx += 40)
		*bx++ = cx;
	bx = _blit16t;
	for (cx = 0; cx < 256; cx++)
	{
		uint8_t ah = 0, dh = 255, carry = 0x80;
		for (; carry; carry >>= 1)
		{
			if (cx & carry)
				ah ^= dh;
			dh >>= 1;
		}
		*bx++ = (uint16_t)(ah | ((ah & 1) << 15));
	}
}

static void blit16(const char *vbuf, char *vram)
{
	int cx;

	for (cx = 0; cx < 200; cx++)
	{
		int zzz;

		// dh = 0; // line starts black
		for (zzz = 0; zzz < 40; zzz += 2)
		{
/*
			bl = vbuf[zzz] ^ dh;
			ax = _blit16t[bl];
			bl = vbuf[1 + zzz] ^ ah;
			dx = _blit16t[bl];
			ah = dl;
			*(uint16_t *)&vram[zzz] = ax;
*/
		}
		vbuf += 40;
		vram += 40;
	}
}

static void blit16b(const char *vbuf, char *vram)
{
	int cx;

	for (cx = 0; cx < 200; cx++)
	{
		int zzz;

		// dh = 0; // line starts black
		for (zzz = 0; zzz < 40; zzz += 2)
		{
/*
			bl = vbuf[zzz] ^ dh;
			ax = _blit16t[bl];
			bl = vbuf[1 + zzz] ^ ah;
			dx = _blit16t[bl];
			ah = dl;
			*(uint16_t *)&vram[zzz] = ax;
*/
		}
		vbuf += 40;
		vram += 80;
	}
}

/*
drawline PROC NEAR
	push	si
	push	di
	push	bp
@@vis:	movzx	ebx,bx
	cmp	bx,cx
	je	@@0
	jle	@@1
	xchg	bx,cx
	xchg	ax,dx
@@1:	sub	cx,bx
	mov	di,cx
	mov	si,cs:_rows[ebx*2]
	mov	bp,cs:clipleft
	or	bp,bp
	jz	@@nl
	push	si
	;left overflow fill
	jge	@@ndn
@@nup:	add	si,40
	xor	byte ptr ds:[si],080h
	inc	bp
	jnz	@@nup
	jmp	@@nl2
@@ndn:	sub	si,40
	xor	byte ptr ds:[si],080h
	dec	bp
	jnz	@@ndn
@@nl2:	pop	si
@@nl:	;
	jcxz	@@0
	movzx	ebp,ax
	shr	bp,3
	add	si,bp
	mov	bp,ax
	and	bp,7
	;go on
	cmp	ax,dx
	jl	@@r
@@l:	;=============== left
	neg	dx
	add	dx,ax
	mov	bx,di
	shr	bx,1
	neg	bx
	jmp	cs:_loffs[ebp*2]
ALIGN 16
_loffs	LABEL WORD
	dw	OFFSET @@l7
	dw	OFFSET @@l6
	dw	OFFSET @@l5
	dw	OFFSET @@l4
	dw	OFFSET @@l3
	dw	OFFSET @@l2
	dw	OFFSET @@l1
	dw	OFFSET @@l0
llinemacro MACRO mask,lbl1,lbl2,lbl3,lbl4,lbl5,lbl6,lbl7,lbl0
	local	l1,l2
	;ds:si=startpoint
	;di=ycnt
	;dx=xcnt
	;bx=counter
l1:	xor	byte ptr ds:[si],mask
	add	si,40
	dec	cx
	jz	@@0
	add	bx,dx
	jl	l1
l2:	IF lbl1 EQ @@l0
	dec	si
	ENDIF
	sub	bx,di
	jl	lbl1
	IF lbl2 EQ @@l0
	dec	si
	ENDIF
	sub	bx,di
	jl	lbl2
	IF lbl3 EQ @@l0
	dec	si
	ENDIF
	sub	bx,di
	jl	lbl3
	IF lbl4 EQ @@l0
	dec	si
	ENDIF
	sub	bx,di
	jl	lbl4
	IF lbl5 EQ @@l0
	dec	si
	ENDIF
	sub	bx,di
	jl	lbl5
	IF lbl6 EQ @@l0
	dec	si
	ENDIF
	sub	bx,di
	jl	lbl6
	IF lbl7 EQ @@l0
	dec	si
	ENDIF
	sub	bx,di
	jl	lbl7
	IF lbl0 EQ @@l0
	dec	si
	ENDIF
	sub	bx,di
	jl	l1
	jmp	l2
	ENDM
@@l7:	llinemacro 10000000b,@@l0,@@l1,@@l2,@@l3,@@l4,@@l5,@@l6,@@l7
@@l6:	llinemacro 01000000b,@@l7,@@l0,@@l1,@@l2,@@l3,@@l4,@@l5,@@l6
@@l5:	llinemacro 00100000b,@@l6,@@l7,@@l0,@@l1,@@l2,@@l3,@@l4,@@l5
@@l4:	llinemacro 00010000b,@@l5,@@l6,@@l7,@@l0,@@l1,@@l2,@@l3,@@l4
@@l3:	llinemacro 00001000b,@@l4,@@l5,@@l6,@@l7,@@l0,@@l1,@@l2,@@l3
@@l2:	llinemacro 00000100b,@@l3,@@l4,@@l5,@@l6,@@l7,@@l0,@@l1,@@l2
@@l1:	llinemacro 00000010b,@@l2,@@l3,@@l4,@@l5,@@l6,@@l7,@@l0,@@l1
@@l0:	llinemacro 00000001b,@@l1,@@l2,@@l3,@@l4,@@l5,@@l6,@@l7,@@l0
@@r:	;=============== right
	sub	dx,ax
	mov	bx,di
	shr	bx,1
	neg	bx
	jmp	cs:_roffs[ebp*2]
ALIGN 16
_roffs	LABEL WORD
	dw	OFFSET @@r7
	dw	OFFSET @@r6
	dw	OFFSET @@r5
	dw	OFFSET @@r4
	dw	OFFSET @@r3
	dw	OFFSET @@r2
	dw	OFFSET @@r1
	dw	OFFSET @@r0
rlinemacro MACRO mask,lbl1,lbl2,lbl3,lbl4,lbl5,lbl6,lbl7,lbl0
	local	l1,l2
	;ds:si=startpoint
	;di=ycnt
	;dx=xcnt
	;bx=counter
l1:	xor	byte ptr ds:[si],mask
	add	si,40
	dec	cx
	jz	@@0
	add	bx,dx
	jl	l1
l2:	IF lbl1 EQ @@r7
	inc	si
	ENDIF
	sub	bx,di
	jl	lbl1
	IF lbl2 EQ @@r7
	inc	si
	ENDIF
	sub	bx,di
	jl	lbl2
	IF lbl3 EQ @@r7
	inc	si
	ENDIF
	sub	bx,di
	jl	lbl3
	IF lbl4 EQ @@r7
	inc	si
	ENDIF
	sub	bx,di
	jl	lbl4
	IF lbl5 EQ @@r7
	inc	si
	ENDIF
	sub	bx,di
	jl	lbl5
	IF lbl6 EQ @@r7
	inc	si
	ENDIF
	sub	bx,di
	jl	lbl6
	IF lbl7 EQ @@r7
	inc	si
	ENDIF
	sub	bx,di
	jl	lbl7
	IF lbl0 EQ @@r7
	inc	si
	ENDIF
	sub	bx,di
	jl	l1
	jmp	l2
	ENDM
@@r7:	rlinemacro 10000000b,@@r6,@@r5,@@r4,@@r3,@@r2,@@r1,@@r0,@@r7
@@r6:	rlinemacro 01000000b,@@r5,@@r4,@@r3,@@r2,@@r1,@@r0,@@r7,@@r6
@@r5:	rlinemacro 00100000b,@@r4,@@r3,@@r2,@@r1,@@r0,@@r7,@@r6,@@r5
@@r4:	rlinemacro 00010000b,@@r3,@@r2,@@r1,@@r0,@@r7,@@r6,@@r5,@@r4
@@r3:	rlinemacro 00001000b,@@r2,@@r1,@@r0,@@r7,@@r6,@@r5,@@r4,@@r3
@@r2:	rlinemacro 00000100b,@@r1,@@r0,@@r7,@@r6,@@r5,@@r4,@@r3,@@r2
@@r1:	rlinemacro 00000010b,@@r0,@@r7,@@r6,@@r5,@@r4,@@r3,@@r2,@@r1
@@r0:	rlinemacro 00000001b,@@r7,@@r6,@@r5,@@r4,@@r3,@@r2,@@r1,@@r0
@@0:	pop	bp
	pop	di
	pop	si
	ret
drawline ENDP
*/

void asmbox(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
/*
	push 	bp
	mov	bp,sp
	push	si
	push	di
	push	ds
	
	mov	eax,[bp+6]
	mov	dword ptr cs:polyixy[0],eax
	mov	eax,[bp+10]
	mov	dword ptr cs:polyixy[4],eax
	mov	eax,[bp+14]
	mov	dword ptr cs:polyixy[8],eax
	mov	eax,[bp+18]
	mov	dword ptr cs:polyixy[12],eax
	mov	cs:polyisides,4
	call	clipanypoly

	mov	ds,cs:_vbufseg
	mov	si,OFFSET polyxy
	mov	di,cs:polysides
	or	di,di
	jz	@@0
	dec	di
	jz	@@2

@@1:	mov	ax,cs:[si+0]
	mov	bx,cs:[si+2]
	mov	dx,cs:[si+4]
	mov	cx,cs:[si+6]
	call	drawline
	add	si,4
	dec	di
	jnz	@@1

@@2:	mov	ax,cs:[si+0]
	mov	bx,cs:[si+2]
	mov	dx,cs:polyxy[0]
	mov	cx,cs:polyxy[2]
	call	drawline

@@0:	pop	ds
	pop	di
	pop	si
	pop	bp
	ret
*/
}

// ################################################################

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

static const uint8_t pal2[] =
{
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
	30, 30 * 7 / 9, 30
};

static const uint8_t pal1[] =
{
	30, 30 * 8 / 9, 30,
	60, 60 * 8 / 9, 60,
	50, 50 * 8 / 9, 50,
	40, 40 * 8 / 9, 40,
	30, 30 * 8 / 9, 30,
	20, 20 * 8 / 9, 20,
	10, 10 * 8 / 9, 10,
	0, 0 * 8 / 9, 0,
	30, 30 * 8 / 9, 30,
	60, 60 * 8 / 9, 60,
	50, 50 * 8 / 9, 50,
	40, 40 * 8 / 9, 40,
	30, 30 * 8 / 9, 30,
	20, 20 * 8 / 9, 20,
	10, 10 * 8 / 9, 10,
	0, 0 * 8 / 9, 0
};
	
static unsigned sinuspower;
static unsigned powercnt;
char power0[256 * 16];

#define PLANE(pl) outport(0x3c4, 0x0002 + (pl) * 0x100)

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

static void outpal(uint8_t start, const uint8_t *src, unsigned bytes)
{
	outportb(0x3c8, start);
	while (bytes--)
		outportb(0x3c9, *src++);
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

static unsigned framecount;
static int palanimc;
static int scrnpos;
static int scrnposl;
static int scrnx;
static int scrny;
static int scrnrot;
static int sinurot;
static int overrot = 211;
static int overx;
static int overya;
static int patdir;

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
	char *const vram = MK_FP(0x0a000, 0);

	do
	{
		int si, di, bp, row;

		dis_waitb();
		outportb(0x3c0, 0x13);
		outportb(0x3c0, (uint8_t)scrnposl);
		outportb(0x3c0, 32);

		palanimc += patdir;
		if (palanimc < 0)
			palanimc = 8 * 3 - 3;
		if (palanimc >= 8 * 3)
			palanimc = 0;

		outpal(0, pal1 + palanimc, 8 * 3);
		outpal(8, pal2 + palanimc, 8 * 3);

		PLANE(8);
		si = 0;
		di = scrnpos;
		bp = sinurot = (sinurot + 7) & 1023;
		for (row = 0; row < 200; row++)
		{
			int ax;

			bp = (bp + 9) & 1023;
			ax = ((const int8_t *)power0)[(sinuspower << 8) | (uint8_t)(sin1024[bp] >> 3)] - scrnposl + overx;
			memcpy(&vram[di], &circles[-(ax & 7) + 7][si + (ax >> 3) + overya], 40 + 4);
			di += 80;
			si += 80;
		}

		// MOVE

		row = dis_musrow() & 7;
		if (!row || (row == 4))
			patdir = -3;

		scrnrot = (scrnrot + 5) & 1023;
		scrnx = (sin1024[scrnrot] >> 2) + 160;
		scrny = (sin1024[(scrnrot + 256) & 1023] >> 2) + 100;

		overrot = (overrot + 7) & 1023;
		overx = (sin1024[overrot] >> 2) + 160;
		overya = ((sin1024[(overrot + 256) & 1023] >> 2) + 100) * 80;

		scrnposl = scrnx & 7;
		scrnpos = 80 * scrny + (scrnx >> 3);

		outport(0x3d4, (uint16_t)((scrnpos & 0xff00) | 0x0c));
		outport(0x3d4, (uint16_t)((scrnpos << 8) | 0x0d));

		if ((framecount++ >= 70 * 5) && (++powercnt >= 16))
		{
			powercnt = 0;
			if (sinuspower < 15)
				sinuspower++;
		}
	}
	while ((dis_getmframe() < 925) && !dis_exit());
}

void initinterference(void)
{
	init_interference();
}

void dointerference(void)
{
	do_interference();
	free(memseg);
}

void techno_inittwk(void)
{
	int i;

	// clear palette
	outportb(0x3c8, 0);
	for (i = 0; i < 768; i++)
		outportb(0x3c9, 0);

	// 400 rows
	outport(0x3d4, 0x0009);

	// tweak
	outport(0x3d4, 0x0014);
	outport(0x3d4, 0xe317);
	outport(0x3c4, 0x0604);

	outport(0x3c4, 0x0f02);
	memset(MK_FP(0x0a000, 0), 0, 65536);
}
