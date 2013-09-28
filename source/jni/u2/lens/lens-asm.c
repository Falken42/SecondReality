//
// ported from asm.asm
//
#include <stdint.h>
#include <string.h>
#include "../../u2-port.h"

char *back;
char *rotpic;
char *rotpic90;

void dorow(const uint16_t *lens, int u, int y, int bits)
{
	int cx = lens[(y << 1) + 1];
	if (cx >= 4)
	{
		char *const vram = MK_FP(0xa000, 0);
		int si = lens[y << 1] >> 1;
		const int di = lens[si++] + u;
		int bp = di, zzz;
		if (bp & 1)
		{
			vram[bp++] = back[(uint16_t)(lens[si++] + di)] | bits;
			cx--;
		}
		si -= 160;
		bp -= 320;
		zzz = cx >> 1;
		zzz = (zzz + (zzz << 2) + (zzz << 4)) / 21;
		while (zzz-- > 0)
		{
			const int diadd = 320 + zzz * 2, siadd = 160 + zzz * 2;
			vram[(uint16_t)(bp + diadd)] = back[(uint16_t)(lens[si + siadd] + di)] | bits;
			vram[(uint16_t)(bp + diadd + 1)] = back[(uint16_t)(lens[si + siadd + 1] + di)] | bits;
		}
		if (cx & 1)
		{
			cx &= ~1;
			vram[(uint16_t)(bp + cx + 320)] = back[(uint16_t)(lens[si + cx + 160] + di)] | bits;
		}
	}
}

void dorow2(const uint16_t *lens, int u, int y, int bits)
{
	int cx = lens[(y << 1) + 1];
	if (cx)
	{
		char *const vram = MK_FP(0xa000, 0);
		int si = lens[y << 1] >> 1;
		const int di = lens[si++] + u;
		do
		{
			vram[(uint16_t)(lens[si] + di)] = back[(uint16_t)(lens[si + 1] + di)] | bits;
			si += 2;
		}
		while (--cx);
	}
}

void dorow3(const uint16_t *lens, int u, int y, int bits)
{
	int cx = lens[(y << 1) + 1];
	if (cx)
	{
		char *const vram = MK_FP(0xa000, 0);
		int si = lens[y << 1] >> 1;
		const int di = lens[si] + u;
		do
		{
			si++;
			vram[(uint16_t)(lens[si] + di)] = back[(uint16_t)(lens[si] + di)];
		}
		while (--cx);
	}
}

void lens_inittwk(void)
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

	esi = edi = 0;
	for (rept = 0; rept < ZOOMXW / 4; rept++)
	{
		esi += yadd;
		edi -= xadd;
		moda2[rept] = ((edi >> 8) & 0xff00) | ((esi >> 16) & 0x00ff);
		esi += yadd;
		edi -= xadd;
		modb2[rept] = ((edi >> 8) & 0xff00) | ((esi >> 16) & 0x00ff);
		esi += yadd;
		edi -= xadd;
		moda6[rept] = ((edi >> 8) & 0xff00) | ((esi >> 16) & 0x00ff);
		esi += yadd;
		edi -= xadd;
		modb6[rept] = ((edi >> 8) & 0xff00) | ((esi >> 16) & 0x00ff);
	}

	// aspect ratio
	xadd = (307 * xadd) >> 8;
	yadd = (307 * yadd) >> 8;

	edi = -0x1000;
	for (ecx = ZOOMYW; ecx; ecx--)
	{
		xpos += xadd;
		ypos += yadd;
		esi = ((ypos >> 8) & 0xff00) | ((xpos >> 16) & 0x00ff);

		outport(0x3c4, 0x0302);
		zzz = 0x1000;
		for (rept = 0; rept < ZOOMXW / 4; rept++)
		{
			es[edi + zzz++] = ds[(uint16_t)(esi + moda2[rept])];
			es[edi + zzz++] = ds[(uint16_t)(esi + moda6[rept])];
		}

		outport(0x3c4, 0x0c02);
		zzz = 0x1000;
		for (rept = 0; rept < ZOOMXW / 4; rept++)
		{
			es[edi + zzz++] = ds[(uint16_t)(esi + modb2[rept])];
			es[edi + zzz++] = ds[(uint16_t)(esi + modb6[rept])];
		}

		edi += 80;
	}
}
