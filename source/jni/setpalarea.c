//
// ported from beg/asm.asm, lens/asm.asm, end/asm.asm
//
#include "u2-port.h"

void setpalarea(char *pal, int start, int cnt)
{
	outportb(0x3C8, start);
	cnt = (cnt << 1) + cnt;
	while (cnt--)
		outportb(0x3C9, *pal++);
}
