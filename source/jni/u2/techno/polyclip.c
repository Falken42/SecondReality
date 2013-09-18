//
// ported from polyclip.asm, to be included from techno-koea.c
//
// variables below come from techno-koea.c:
// - static uint16_t polyisides;
// - static uint16_t polyixy[16][2];
// - static uint16_t polysides;
// - static uint16_t polyxy[16][2];
//
// and the only function externalized from this file is clipanypoly()
//
#include <stdint.h>

static uint16_t WMINY;
static uint16_t WMAXY = 199;
static uint16_t WMINX;
static uint16_t WMAXX = 319;

static uint16_t clip_x1;
static uint16_t clip_y1;
static uint16_t clip_x2;
static uint16_t clip_y2;
static uint16_t clipxy2[32][2]; // tmp storage for polyclip

static uint16_t ax, bx, cx, dx; // WIP:

static void cliplinex(void);
static void clipliney(void);

static void clipanypoly(void) // TODO: pointer arithmetic
{
	// polyisides/polyixy =>polysides/polyxy
/*
	cx = polyisides;
	if ((int16_t)cx <= 2)
	{
		if (cx != 1)
		{
			if (!cx)
				goto cap0;

			// line
			mov	eax,dword ptr ds:polyixy[0]
			mov	dword ptr ds:clip_x1,eax
			mov	eax,dword ptr ds:polyixy[4]
			mov	dword ptr ds:clip_x2,eax

		cap2r:
			clipliney();
			if (ax)
				goto cap0;

			cliplinex();
			if (ax)
				goto cap0;

			mov	eax,dword ptr ds:clip_x1
			mov	dword ptr ds:polyxy[0],eax
			mov	edx,dword ptr ds:clip_x2
			mov	dword ptr ds:polyxy[4],edx
			polysides = (eax != edx) ? 2 : 1;

			return;
		}

		// dot
		mov	eax,dword ptr ds:polyixy[0]
		cmp	ax,cs:WMINX
		jl	cap0

		cmp	ax,cs:WMAXX
		jg	cap0

		// eax = dword ptr polyixy[0]
		ror	eax,16
		cmp	ax,cs:WMINY
		jl	cap0

		cmp	ax,cs:WMAXY
		jg	cap0

		ror	eax,16
		mov	dword ptr ds:polyxy,eax
		polysides = 1;
		return;

	cap0:
		// all clipped away
		polysides = 0;
		return;
	}

	// polygon, first clip y, then x
	mov	si,cx
	shl	si,2
	sub	si,4
	mov	di,0
	mov	eax,dword ptr ds:polyixy[si]
	mov	dword ptr ds:clip_x1,eax
	mov	eax,dword ptr ds:polyixy[di]
	mov	dword ptr ds:clip_x2,eax
	call	clipliney

	mov	cx,ds:polyisides
	xor	di,di
	xor	bx,bx
	mov	edx,80008000h
	jmp	cap35
	do
	{
		push	di
		push	bx
		push	cx
		push	edx
		mov	si,di
		sub	si,4
		mov	eax,dword ptr ds:polyixy[si]
		mov	dword ptr ds:clip_x1,eax
		mov	eax,dword ptr ds:polyixy[di]
		mov	dword ptr ds:clip_x2,eax
		call	clipliney
		pop	edx
		pop	cx
		pop	bx
		pop	di

	cap35:
		if (!ax)
		{
			mov	eax,dword ptr ds:clip_x1
			if (eax != edx)
			{
				mov	dword ptr ds:clipxy2[bx],eax
				mov	edx,eax
				add	bx,4
			}
			mov	eax,dword ptr ds:clip_x2
			if (eax == edx)
			{
				mov	dword ptr ds:clipxy2[bx],eax
				mov	edx,eax
				add	bx,4
			}
		}
		add	di,4
	}
	while (--cx);

	cx = bx >> 2;
	if (dword ptr ds:clipxy2[0] == edx)
		cx--;
	polysides = cx;

	if ((int16_t)cx <= 2)
	{
		if (!cx)
			return;

		mov	eax,dword ptr ds:clipxy2[0]
		mov	dword ptr ds:clip_x1,eax
		mov	eax,dword ptr ds:clipxy2[4]
		mov	dword ptr ds:clip_x2,eax
		jmp	cap2r // reclip the remaining line
	}

	mov	si,cx
	shl	si,2
	sub	si,4
	mov	di,0
	mov	eax,dword ptr ds:clipxy2[si]
	mov	dword ptr ds:clip_x1,eax
	mov	eax,dword ptr ds:clipxy2[di]
	mov	dword ptr ds:clip_x2,eax
	call	cliplinex

	cx = polysides;
	di = 0;
	bx = 0;
	mov	edx,80008000h
	jmp	cbp35
	do
	{
		push	di
		push	bx
		push	cx
		push	edx
		mov	si,di
		sub	si,4
		mov	eax,dword ptr ds:clipxy2[si]
		mov	dword ptr ds:clip_x1,eax
		mov	eax,dword ptr ds:clipxy2[di]
		mov	dword ptr ds:clip_x2,eax
		call	cliplinex
		pop	edx
		pop	cx
		pop	bx
		pop	di

	cbp35:
		if (!ax)
		{
			mov	eax,dword ptr ds:clip_x1
			if (eax != edx)
			{
				mov	dword ptr ds:polyxy[bx],eax
				mov	edx,eax
				add	bx,4
			}
			mov	eax,dword ptr ds:clip_x2
			if (eax != edx)
			{
				mov	dword ptr ds:polyxy[bx],eax
				mov	edx,eax
				add	bx,4
			}
		}
		add	di,4
	}
	while (--cx);

	cx = bx >> 2;
	if (dword ptr ds:polyxy[0] == edx)
		cx--;
	polysides = cx;
*/
}

static uint8_t clipcheck(int16_t reg, int16_t min, int16_t max, uint8_t flagmin, uint8_t flagmax)
{
	uint8_t flagreg = 0;
	if (reg < min)
		flagreg |= flagmin;
	if (reg > max)
		flagreg |= flagmax;
	return flagreg;
}

static void clipmacro(uint16_t *v1, uint16_t v2, uint16_t *w1, uint16_t w2, uint16_t wl) // WIP: uses cx,dx
{
	cx = w2 - *w1;
	if (!cx)
		*w1 = wl;
	else
	{
		const uint16_t bp = wl - *w1;
		const uint16_t ax = v2 - *v1;
		const int32_t dxax = (int32_t)(int16_t)ax * (int16_t)bp / (int16_t)cx;
		dx = (uint16_t)((uint32_t)dxax >> 16);
		*v1 = (uint16_t)dxax + *v1;
		*w1 = wl;
	}
}

static void cliplinex(void) // WIP: uses ax,bx + cx,dx
{
	// input line polyxy[SI]=>polyxy[DI]
	uint8_t bl, bh, al;
	bl = clipcheck((int16_t)clip_x1, (int16_t)WMINX, (int16_t)WMAXX, 1, 2);
	ax = clip_x2;
	bh = clipcheck((int16_t)ax, (int16_t)WMINX, (int16_t)WMAXX, 1, 2);
	bx = (bh << 8) | bl;
	al = bl & bh;
	ax = (ax & 0xff00) | al;
	if (!al)
	{
		if (bl & 1)
			clipmacro(&clip_y1, clip_y2, &clip_x1, clip_x2, WMINX); // WIP: uses cx,dx
		if (bl & 2)
			clipmacro(&clip_y1, clip_y2, &clip_x1, clip_x2, WMAXX); // WIP: uses cx,dx
		if (bh & 1)
			clipmacro(&clip_y2, clip_y1, &clip_x2, clip_x1, WMINX); // WIP: uses cx,dx
		if (bh & 2)
			clipmacro(&clip_y2, clip_y1, &clip_x2, clip_x1, WMAXX); // WIP: uses cx,dx
		ax = 0;
	}
}

static void clipliney(void) // WIP: uses ax,bx + cx,dx
{
	uint8_t bl, bh, al;
	bl = clipcheck((int16_t)clip_y1, (int16_t)WMINY, (int16_t)WMAXY, 4, 8);
	ax = clip_y2;
	bh = clipcheck((int16_t)ax, (int16_t)WMINY, (int16_t)WMAXY, 4, 8);
	bx = (bh << 8) | bl;
	al = bl & bh;
	ax = (ax & 0xff00) | al;
	if (!al)
	{
		if (bl & 4)
			clipmacro(&clip_x1, clip_x2, &clip_y1, clip_y2, WMINY); // WIP: uses cx,dx
		if (bl & 8)
			clipmacro(&clip_x1, clip_x2, &clip_y1, clip_y2, WMAXY); // WIP: uses cx,dx
		if (bh & 4)
			clipmacro(&clip_x2, clip_x1, &clip_y2, clip_y1, WMINY); // WIP: uses cx,dx
		if (bh & 8)
			clipmacro(&clip_x2, clip_x1, &clip_y2, clip_y1, WMAXY); // WIP: uses cx,dx
		ax = 0;
	}
}
