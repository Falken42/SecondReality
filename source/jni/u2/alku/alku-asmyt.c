//
// ported from asmyt.asm
//
#include <stdint.h>
#include "../../u2-port.h"

static const uint16_t mmask[] = {0x0102, 0x0202, 0x0402, 0x0802, 0x0102, 0x0202, 0x0402, 0x0802};
static const uint16_t rmask[] = {0x0004, 0x0104, 0x0204, 0x0304, 0x0004, 0x0104, 0x0204, 0x0304};

void ascrolltext(int scrl, const int *text)
{
	char *const vvmem = MK_FP(0xa000, 0);
	int m;

	for (m = 0; m < 4; m++)
	{
		const int aa = (scrl + m) >> 2;

		outport(0x3c4, mmask[(scrl + m) & 3]);
		outport(0x3ce, rmask[(scrl + m) & 3]);

		while (*text != -1)
		{
			const int a = *text++;
			const int c = *text++;
			vvmem[a + aa] ^= c;
		}

		text += 2;
	}
}

void outline(char *src, char *dest)
{
	int mrol = 0x08, cnt, ccc;

	for (cnt = 4; cnt > 0; cnt--)
	{
		const char *si = src + cnt - 1;
		char *di = dest;

		outport(0x3C4, (mrol << 8) | 0x02);

		di[-352      ] = 0;
		di[-352 + 176] = 0;

		for (ccc = 0; ccc < 75; ccc++)
		{
			const char al = si[ccc * 640];
			di[ccc * 352] = al;
			di[ccc * 352 + 176] = al;
		}

		si += 75*40 * 16;		// scale by 16 for segment offset

		for (ccc = 0; ccc < 75; ccc++)
		{
			const char al = si[ccc * 640];
			di[ccc * 352 + 75 * 352] = al;
			di[ccc * 352 + 75 * 352 + 176] = al;
		}

		mrol >>= 1;
	}
}
