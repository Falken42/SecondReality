//
// ported from glenz/asm.asm
//

#include "../../u2-port.h"

void testasm()
{
	// hackhackhack
	outport(0x3C4, 0x0408);
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

