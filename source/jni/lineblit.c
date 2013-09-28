//
// ported from beg/asm.asm, techno/koea.asm, end/asm.asm
//
#include <stdint.h>
#include "u2-port.h"

void lineblit(char *buf, char *row)
{
	int zpl, zzz;

	for (zpl = 0; zpl < 4; zpl++)
	{
		outport(0x3C4, 0x02 + (0x100 << zpl));

		for (zzz = 0; zzz < 80; zzz += 2)
		{
			const uint8_t al = *(row + (zzz+0) * 4 + zpl);
			const uint8_t ah = *(row + (zzz+1) * 4 + zpl);
			*((uint16_t *)(buf + zzz)) = ((int)ah << 8) | al;
		}
	}
}
