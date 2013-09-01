#include <stdio.h>
#include "u2-port.h"

static const uint8_t *aseg;

static uint8_t ntaulu[4] = { 0x1, 0x3, 0x7, 0xF };
static uint8_t taulu[4]  = { 0xF, 0xE, 0xC, 0x8 };

static uint8_t ptaulu[4] = { 1, 2, 4, 8 };

// from pam/asmyt.asm
void init_uframe(int seg)
{
	aseg = (const uint8_t *)seg;
}

void ulosta_frame(int start)
{
	const uint8_t *src = aseg;
	uint8_t *dest = MK_FP(start, 0);
	uint16_t di = 0, bx, cx, _sdi, _scx;
	int8_t al, cl, ch;
	int16_t ax;

//	LOGI("--- frame: start=%04Xh ---", start);

looppi:
	al = *src++;
//	LOGI("code = %d, offs = %d", al, di);
	if (al > 0)
		goto outtaa;
	if (al == 0)
		goto over;

skip:
//	LOGI("* skip, offs = %d", di);
	ax = al;
	di -= ax;
	al = *src++;
	if (al < 0)
		goto skip;
	if (al == 0)
		goto over;

outtaa:
//	LOGI("* outtaa, offs = %d", di);
	if (--al == 0)
		goto single;

	ax = al;
	bx = di;
	bx &= 3;
	bx += ax;
	bx >>= 2;
	if (bx == 0)
		goto samebyte;
	if (bx == 1)
		goto twobytes;

	cx = ax;
	bx = di;
	bx &= 3;
	outport(0x3C4, ((int)taulu[bx] << 8) | 0x02);
	
	al = *src;
	bx = di;
	bx >>= 2;
//	LOGI("1: dest[%d] = %d", bx, al);
	dest[bx] = al;

	_sdi = di;
	_scx = cx;
	
	ax = di;
	cx += ax;
	cx >>= 2;
	cx -= bx;
	cx--;
	di >>= 2;
	di++;

	outport(0x3C4, 0x0F02);
	al = *src;
//	LOGI("** outtaa: offs = %d, count = %d", di, cx);
	while (cx--)
	{
//	LOGI("2: dest[%d] = %d", di, al);
		dest[di] = al;
		di++;
	}

	cx = _scx;
	di = _sdi;

	di += cx;
	bx = di;
	bx &= 3;
	outport(0x3C4, ((int)ntaulu[bx] << 8) | 0x02);
	
	al = *src++;
	bx = di;
	bx >>= 2;
//	LOGI("3: dest[%d] = %d", bx, al);
	dest[bx] = al;

	di++;

	al = *src++;
	if (al < 0)
		goto skip;
	if (al > 0)
		goto outtaa;
	goto over;

twobytes:
//	LOGI("* twobytes, offs = %d", di);
	bx = di;
	bx &= 3;
	cl = taulu[bx];
	di += ax;
	bx = di;
	bx &= 3;
	ch = ntaulu[bx];
	outport(0x3C4, ((int)cl << 8) | 0x02);
	cl = *src;
	bx = di;
	bx >>= 2;
//	LOGI("4: dest[%d] = %d", bx - 1, cl);
	dest[bx - 1] = cl;
	outport(0x3C4, ((int)ch << 8) | 0x02);
//	LOGI("5: dest[%d] = %d", bx, cl);
	dest[bx] = cl;
	di++;
	src++;

	al = *src++;
	if (al < 0)
		goto skip;
	if (al > 0)
		goto outtaa;
	goto over;

samebyte:
//	LOGI("* samebyte, offs = %d", di);
	bx = di;
	bx &= 3;
	cl = taulu[bx];
	di += ax;
	bx = di;
	bx &= 3;
	cl &= ntaulu[bx];
	outport(0x3C4, ((int)cl << 8) | 0x02);
	
	al = *src++;
	bx = di;
	bx >>= 2;
//	LOGI("6: dest[%d] = %d", bx, al);
	dest[bx] = al;
	di++;

	al = *src++;
	if (al < 0)
		goto skip;
	if (al > 0)
		goto outtaa;
	goto over;

single:
//	LOGI("* single, offs = %d", di);
	bx = di;
	bx &= 3;
	outport(0x3C4, ((int)ptaulu[bx] << 8) | 0x02);
	al = *src++;
	bx = di;
	bx >>= 2;
//	LOGI("7: dest[%d] = %d", bx, al);
	dest[bx] = al;
	di++;

	al = *src++;
	if (al < 0)
		goto skip;
	if (al > 0)
		goto outtaa;

over:
	aseg = src;
}

