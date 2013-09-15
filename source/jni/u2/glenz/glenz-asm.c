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
// ported from glenz/new.asm
//

#define NROWS		256
#define MAXLINES	512

typedef struct _nedata
{
	int32_t			x;
	int16_t			y1, y2;
	int16_t			color;
	struct _nedata *next;
	int32_t			dx;
} nedata;

static int16_t ndp0 = 0, ndp = 0;
static nedata *nec = NULL;
static nedata *nep[NROWS] = { NULL };
static nedata  ne[MAXLINES] = { 0 };
static int8_t  nl[256] = { 0 };

static void ng_init()
{
}

static void ng_pass2()
{
}

static void ng_pass3()
{
}

static void newgroup(int cmd, int *polylist)
{
	if (cmd == 0)
		ng_init();
	else if (cmd == 1)
	{
		int ax, bx, cx, dx, tmp, cnt, color, *firstvx, *thisvx, *nextvx;
		nedata *nax;

		// add polygons to list
		while ((cnt = polylist[0]) > 0)
		{
			color   = polylist[1];
			firstvx = &polylist[2];

			while (cnt > 0)
			{
				polylist += 2;
				thisvx = &polylist[0];
				nextvx = &polylist[2];
				if (cnt == 1)
					nextvx = firstvx;

				nec->color = color;
				bx = thisvx[2];			// y1
				cx = nextvx[2];			// y2
				if (bx < cx)
				{
					ax = thisvx[0];		// x1
					dx = nextvx[0];		// x2
				}
				else
				{
					tmp = bx; bx = cx; cx = tmp;
					ax = nextvx[0];		// x1
					dx = thisvx[0];		// x2
				}

				// ax,bx=xy1 dx,cx=xy2
				nec->y1 = bx;
				nec->y2 = cx;
				nec->x  = ax;

				ax   = -ax;
				ax  += dx;
				ax <<= 16;
				cx  -= bx;

				// cx=y2-y1,ax=(x2-x1)<<16
				if (cx > 0)				// skip horizontal lines
				{
					ax /= cx;
					nec->dx = ax;

					// if y1<0, clip
					if (bx < 0)
					{
						if (nec->y2 < 0)
							goto next;

						bx  = -bx;
						ax *= bx;
						nec->x = ax;

						bx = 0;
						nec->y1 = bx;
					}

					nax = nep[bx];
					if (nax == NULL)
					{
						// first on this row
						nep[bx] = nec;
						nec->next = NULL;
					}
					else
					{
						// add to this row
						// scan if already exists
						do
						{
							if ((nec->y2 == nax->y2) && (nec->x == nax->x) && (nec->dx == nax->dx))
							{
								// duplicate line nax
								nax->color ^= nec->color;
								goto next;
							}

							// *** NOTE ***
							// there is something odd in the logic with this code in new.asm (lines 382-399).  line 398 checks if
							// the next element in nax is null or not, but it isn't loaded until after the comparison and jump
							// back to line 382!  in fact, the label "@@h1" doesn't even need to be at line 382, and can be placed
							// after the comparison.  but if next does become null, then that means that there is at least one
							// pass where the values with nax are read from while null.  we'll work around this for now.
							nax = nax->next;
						} while (nax != NULL);

						nax->next = nep[bx];
						nep[bx] = nax;
					}

					nec++;
				}

next:
				cnt--;
			}

			polylist += 2;
		}
	}
	else
	{
		ng_pass2();
		ng_pass3();
	}
}

//
// ported from glenz/vec.asm
//

void cglenzinit()
{
	newgroup(0, NULL);
}

void cglenzdone()
{
	newgroup(2, NULL);
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

void cglenzpolylist(int *polylist)
{
	newgroup(1, polylist);
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

