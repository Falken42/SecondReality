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
static nedata *nl[NROWS] = { NULL };

static int16_t yrow = 0, yrowad = 0;
static nedata **siend = NULL;

// from adata.asm
static int16_t newdata1[0xFFF0 / 2] = { 0 };		// accessed using 16-bit words

extern char bgpic[65535];

static void ng_init()
{
	int16_t ax;

	nec  = &ne[0];
	ax   = ndp0;
	ax  ^= 0x8000;
	ndp0 = ax;
	ndp  = ax;

	for (ax = 0; ax < NROWS; ax++)
		nep[ax] = NULL;

	if (newdata1[0] == 0)
		newdata1[0] = -1;

	if (newdata1[0x4000] == 0)
		newdata1[0x4000] = -1;
}

static void ng_pass2()
{
	nedata *bx, *dx;
	nedata **bp, **si, **di, **sbp, **sdi;
	int cnt, ax, cx;

	bp	   = &nep[0];
	di	   = &nl[0];
	cnt	   = 200;
	yrow   = 0;
	yrowad = 0;

	// di=pointer to this row list end
	// bp=pointer to nep

label1:
	// add new items this row
	bx = *bp;
	while (bx != NULL)
	{
		*di = bx;
		di++;
		bx = bx->next;
	}

	// sort this row (insertion sort)
	sbp = bp;
	sdi = di;
	bp  = di;
	si  = &nl[1];
	goto label4;

label5:
	// for(k=1;k<listc;k++) {	// k=SI
	bx = *si;					// bx=list[k]
	dx = bx;					// dx=i
	ax = bx->x;					// eax=x
	di = si;
	di--;						// di=j
	goto label6;

label9:
	// for(j=k-1;j>=0 && x<e[list[j]].x;j--) {
	di[1] = bx;					// bx=cs:di
	di--;

label6:
	if (di >= &nl[0])
	{
		bx = *di;
		if (ax < bx->x)
			goto label9;
	}

	di[1] = dx;
	si++;

label4:
	if (si < bp)
		goto label5;

	di = sdi;

	// bp=nl end
	
	// process list & kill finished lines
	siend = bp;
	cx    = 0x8000;
	si	  = &nl[0];
	di	  = si;
	goto label10;

label11:
	bx = *si;
	if (yrow < bx->y2)
	{
		*di = bx;
		di++;
		ax = bx->x;
		bx->x += bx->dx;
		ax >>= 16;

		// clip X
		if (ax > 319)
			ax = 319;
		else if (ax < 1)
			ax = 1;

		if (cx == ax)
		{
			// same x pos
			ax = bx->color;
			newdata1[ndp - 1] ^= ax;
		}
		else
		{
			// new x pos
			ax += yrowad;
			newdata1[ndp    ] = ax;
			ax = bx->color;
			newdata1[ndp + 1] = ax;
			ndp += 2;
			cx = ax;
		}
	}

	si++;

label10:
	if (si < siend)
		goto label11;

	bp  = sbp;
	yrow++;
	yrowad += 320;
	bp++;
	
	cnt--;
	if (cnt > 0)
		goto label1;

	newdata1[ndp    ] = 63999;
	newdata1[ndp + 1] = 0;
	newdata1[ndp + 2] = -1;
	newdata1[ndp + 3] = 0;
	ndp += 4;
}

static __inline void fillmacro(uint8_t *vram, const uint8_t *bg, uint8_t color, int count)
{
	while (count--)
		*vram++ = *bg++ | color;
}

static void ng_pass3()
{
	uint8_t *vram = MK_FP(0xA000, 0x0000);
	const uint8_t *bg = bgpic;
	int16_t bx, si;
	int di, al, ah, dhx, dx, chx, cx;

	di  = 0;
	si  = ndp0;
	bx  = si;
	bx ^= 0x4000;
	al  = ah = 0;

	//si=new     bx=last
	//cx=newpos  dx=lastpos
	//al=newcol  ah=lastcol
	dhx = newdata1[bx    ];
	dx  = newdata1[bx + 1];
	bx += 2;
	chx = newdata1[si    ];
	cx  = newdata1[si + 1];
	si += 2;

label21:
	if (dx < cx)
		goto label23;
	else if (dx == cx)
		goto label22;

	// cx<dx
	if (al != ah)
		fillmacro(vram + di, bg + di, al, cx - di);

	di  = cx;
	cx  = chx;
	al ^= cx & 0xFF;
	chx = newdata1[si    ];
	cx  = newdata1[si + 1];
	si += 2;
	goto label21;

label22:
	// cx=dx
	if (cx == -1)
		return;

label23:
	// dx<cx
	if (al != ah)
		fillmacro(vram + di, bg + di, al, cx - di);

	di  = dx;
	dx  = dhx;
	ah ^= dx & 0xFF;
	dhx = newdata1[bx    ];
	dx  = newdata1[bx + 1];
	bx += 2;
	goto label21;
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

