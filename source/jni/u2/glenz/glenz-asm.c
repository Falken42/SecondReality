//
// ported from glenz/asm.asm
//

#include "../../u2-port.h"

void testasm()
{
	// hackhackhack
	outport(0x3C4, 0x0804);		// disable chain-4
	outport(0x3C4, 0x0102);		// write to plane 1
	outport(0x3D4, 0xBF06);		// 200 scanlines (see: http://wiki.osdev.org/VGA_Hardware#Port_0x3C4.2C_0x3CE.2C_0x3D4 )
	outport(0x3D4, 0x1F07);
}

//
// ported from glenz/vec.asm
//

void cglenzinit()
{
}

void cglenzdone()
{
}

int crotlist(long *src, long *dest)
{
}

int cprojlist(long *src, long *dest)
{
}

int ceasypolylist(int *polylist, int *polys, long *points3)
{
}

void cglenzpolylist()
{
}

int ccliplist(long *pointlist)
{
}

int csetmatrix(int *mtx, long xadd, long yadd, long zadd)
{
}

//
// ported from glenz/math.asm
//

int cmatrix_yxz(int rotx, int roty, int rotz, int *mtx)
{
}

//
// ported from glenz/zoom.asm
//

void zoom(char *to, char *from, int factor)
{
	// unused?
}

