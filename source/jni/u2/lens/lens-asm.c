//
// ported from asm.asm
// note: setpalarea() resides in u2-port.c
//
#include <stdint.h>
#include <string.h>
#include "../../u2-port.h"

char *back;
char *rotpic;
char *rotpic90;

void dorow(int16_t *lens, int u, int y, int bits)
{
/*
DOWORD 	MACRO	diadd,siadd
	mov	bx,ds:[si+(siadd)]	;4
	mov	al,fs:[bx+di]		;3
	mov	bx,ds:[si+(siadd)+2]	;4
	mov	ah,fs:[bx+di]		;3
	or	ax,dx			;2
	mov	es:[bp+(diadd)],ax	;5
	ENDM				;=21

PUBLIC _dorow
_dorow	PROC FAR
	push 	bp
	mov 	bp,sp
	push 	si
	push	di
	push	ds
	
	mov	fs,cs:_back[2]
	mov	ax,0a000h
	mov	es,ax
	mov	ds,[bp+8]
	mov	dx,[bp+14]
	mov	dh,dl
	mov	di,[bp+10]
	mov	si,[bp+12]
	shl	si,2
	mov	cx,ds:[si+2]
	mov	si,ds:[si]
	cmp	cx,4
	jge	@@2
	jmp	@@0
@@2:	add	di,ds:[si]
	mov	bp,di
	add	si,2
	test	bp,1
	jz	@@1
	mov	bx,ds:[si]
	add	si,2
	mov	al,fs:[bx+di]
	or	ax,dx
	mov	es:[bp],al
	inc	bp
	dec	cx
@@1:	push	cx
	shr	cx,1
	sub	si,320
	sub	bp,320
	mov	ax,cx ;*1
	shl	cx,2
	add	ax,cx ;*4
	shl	cx,2
	add	ax,cx ;*16
	neg	ax
	;bx=-count*21
	add	ax,OFFSET @@l
	jmp	ax
	zzz=64
	REPT	64
	zzz=zzz-1
	DOWORD	320+zzz*2,320+zzz*4
	ENDM
@@l:	pop	cx
	test	cx,1
	jz	@@0
	and	cx,not 1
	add	bp,cx
	add	cx,cx
	add	si,cx
	mov	bx,ds:[si+320]
	mov	al,fs:[bx+di]
	or	ax,dx
	mov	es:[bp+320],al
@@0:	pop	ds
	pop	di
	pop	si
	pop 	bp
	ret
_dorow	ENDP
*/
}

void dorow2(int16_t *lens, int u, int y, int bits)
{
/*
PUBLIC _dorow2
_dorow2	PROC FAR
	push 	bp
	mov 	bp,sp
	push 	si
	push	di
	push	ds
	
	mov	fs,[bp+8]
	mov	si,[bp+12]
	shl	si,2
	mov	cx,fs:[si+2]
	mov	si,fs:[si]
	or	cx,cx
	jcxz	@@0
	mov	ds,cs:_back[2]
	mov	ax,0a000h
	mov	es,ax
	mov	dx,[bp+14]
	mov	dh,dl
	mov	di,[bp+10]
	add	di,fs:[si]
	mov	ax,di
	lea	bp,[si+2]
	mov	si,ax
	
@@3:	mov	bx,fs:[bp+2]
	mov	al,ds:[bx+di]
	or	al,dl
	mov	bx,fs:[bp]
	add	bp,4
	mov	es:[bx+si],al
	dec	cx
	jnz	@@3
	
@@0:	pop	ds
	pop	di
	pop	si
	pop 	bp
	ret
_dorow2	ENDP
*/
}

void dorow3(int16_t *lens, int u, int y, int bits)
{
/*
PUBLIC _dorow3
_dorow3	PROC FAR
	push 	bp
	mov 	bp,sp
	push 	si
	push	di
	push	ds
	
	mov	fs,[bp+8]
	mov	si,[bp+12]
	shl	si,2
	mov	cx,fs:[si+2]
	mov	si,fs:[si]
	or	cx,cx
	jcxz	@@0
	mov	ds,cs:_back[2]
	mov	ax,0a000h
	mov	es,ax
	mov	di,[bp+10]
	add	di,fs:[si]
	add	si,2
	
@@3:	mov	bx,fs:[si]
	add	si,2
	mov	al,ds:[bx+di]
	mov	es:[bx+di],al
	dec	cx
	jnz	@@3
	
@@0:	pop	ds
	pop	di
	pop	si
	pop 	bp
	ret
_dorow3	ENDP
*/
}

void inittwk(void)
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

	// set 4 x vertical
	outportb(0x3d4, 9);
	outportb(0x3d5, (inportb(0x3d5) & ~31) | 3);
}

#define ZOOMXW 160
#define ZOOMYW 100

static void xchg(int *a, int *b)
{
	const int c = *a;
	*a = *b;
	*b = c;
}

void rotate(int x, int y, int xa, int ya)
{
	static int moda2[ZOOMXW / 4], moda6[ZOOMXW / 4];
	static int modb2[ZOOMXW / 4], modb6[ZOOMXW / 4];

	int xpos = x << 16, ypos = y << 16, xadd, yadd;
	int eax = xa << 6, ebx = ya << 6, ecx = (eax < 0) ? -eax : eax, edx = (ebx < 0) ? -ebx : ebx;
	int esi, edi, zzz, rept;
	char *ds = rotpic, *const es = MK_FP(0x0a000, 0);

	if (ecx > edx)
	{
		ds = rotpic90;
		xchg(&eax, &ebx);
		eax = -eax;
		ecx = xpos;
		edx = ypos;
		xchg(&ecx, &edx);
		ecx = -ecx;
		xpos = ecx;
		ypos = edx;
	}

	xadd = eax;
	yadd = ebx;

/*
	xor	ax,ax
	mov	cx,LOWORD(yadd)
	mov	bl,byte ptr cs:yadd[2]
	mov	dx,LOWORD(xadd)
	neg	dx
	mov	bh,byte ptr cs:xadd[2]
	neg	bh
	sbb	bh,0
si = 0;
di = 0;
	for (rept = 0; rept < ZOOMXW / 4; rept++)
	{
		add	si,cx
		adc	al,bl
		add	di,dx
		adc	ah,bh
		moda2[rept] = ax;
		add	si,cx
		adc	al,bl
		add	di,dx
		adc	ah,bh
		modb2[rept] = ax;
		add	si,cx
		adc	al,bl
		add	di,dx
		adc	ah,bh
		moda6[rept] = ax;
		add	si,cx
		adc	al,bl
		add	di,dx
		adc	ah,bh
		modb6[rept] = ax;
	}
	
	// aspect ratio
	mov	eax,307
	mul	dword ptr cs:xadd
	sar	eax,8
	mov	cs:xadd,eax
	mov	eax,307
	mul	dword ptr cs:yadd
	sar	eax,8
	mov	cs:yadd,eax
*/

	edi = -0x1000;
	for (ecx = ZOOMYW; ecx; ecx--)
	{
		xpos += xadd;
		ypos += yadd;
		esi = ((ypos >> 8) & 0xf0) | ((xpos >> 16) & 0x0f);

		outport(0x3c4, 0x0302);
		zzz = 0x1000;
		for (rept = 0; rept < ZOOMXW / 4; rept++)
		{
			es[edi + zzz++] = ds[esi + moda2[rept]];
			es[edi + zzz++] = ds[esi + moda6[rept]];
		}

		outport(0x3c4, 0x0c02);
		zzz = 0x1000;
		for (rept = 0; rept < ZOOMXW / 4; rept++)
		{
			es[edi + zzz++] = ds[esi + modb2[rept]];
			es[edi + zzz++] = ds[esi + modb6[rept]];
		}

		edi += 80;
	}
}
