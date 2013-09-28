//
// ported from tweak.asm
//
#include <stdint.h>
#include <string.h>
#include "../../u2-port.h"

// avaa 320x200 tweak tilan, 4 sivua, 4 planea

void pam_tw_opengraph(void)
{
	int10h(0x13);

	outport(0x03c4, 0x0604); // chain4 off

	outport(0x03c4, 0x0f02);
	memset(MK_FP(0xa000, 0), 0, 0x10000); // clear vmem

	outport(0x03d4, 0x0014); // crtc long off

	outport(0x03d4, 0xe317); // crtc byte on
}

// vaihtaa koko paletin

void pam_tw_setpalette(const uint8_t *pal)
{
	int i;

	for (i = 0; i < 0x300; i++)
		outportb(0x03c9, pal[i]);
}

// asettaa videomuistin alun

void pam_tw_setstart(int start)
{
	outport(0x03d4, (uint16_t)((start << 8) | 0x0d));
	outport(0x03d4, (uint16_t)((start & 0xff00) | 0x0c));
}

void pam_tw_waitvr(void)
{
	// TODO: is this timing sufficient?
	dis_waitb();
}
