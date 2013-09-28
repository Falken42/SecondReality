//
// ported from tweak.asm
//
#include <stdint.h>
#include <string.h>
#include "../../u2-port.h"

static const uint8_t planetau[] = {1, 2, 4, 8};

// avaa 320x200 tweak tilan, 4 sivua, 4 planea

void alku_tw_opengraph()
{
	int10h(0x13);

	outport(0x03c4, 0x0604); // chain4 off

	outport(0x03c4, 0x0f02);
	memset(MK_FP(0xa000, 0), 0, 0x10000); // clear vmem

	outport(0x03d4, 0x0014); // crtc long off

	outport(0x03d4, 0xe317); // crtc byte on

	outport(0x03d4, 0x0009); // 400

	outport(0x03d4, 0x5813); // 640 wide

	outport(0x03d4, 0x7018); // linecompare
	outport(0x03d4, 0x1f07); // 8th bit
}

// piirtт pisteen ruudulle

void alku_tw_putpixel(int x, int y, int color)
{
	outport(0x03c4, (planetau[x & 0x03] << 8) | 0x02); // select plane

	*MK_FP(0xa000, (x >> 2) + (y << 4) + (y << 5) + (y << 7)) = (char)color;
}

int alku_tw_getpixel(int x, int y)
{
	outport(0x03ce, ((x & 0x03) << 8) | 0x04); // select plane

	return *MK_FP(0xa000, (x >> 2) + (y << 4) + (y << 5) + (y << 7));
}

// vaihtaa koko paletin

void alku_tw_setpalette(const uint8_t *pal)
{
	int i;

	for (i = 0; i < 0x300; i++)
		outportb(0x03c9, pal[i]);
}
