//
// ported from asm.asm
// some public symbols may conflict with other parts
//
#include "../../u2-port.h"

// should be placed in u2-port.h?
extern void outp(unsigned short port, unsigned char val);

#define MAXDOTS 1024

#define BOTTOM 8000

int gravitybottom = BOTTOM;

int bpmin = 30000;
int bpmax = -30000;

int gravity;

int dotnum;

int gravityd = 16;

int rows[200];

struct s_dot // ripped from main.c
{
	int	x;
	int	y;
	int	z;
	int	old1;
	int	old2;
	int	old3;
	int	old4;
	int	yadd;
};
struct s_dot dot[MAXDOTS];

int rotsin;
int rotcos;

char *bgpic;

long depthtable1[128];
long depthtable2[128];
long depthtable3[128];
long depthtable4[128];

static uint16_t load16le(const void *src)
{
	const uint8_t *s = (const uint8_t *)src;
	return s[0] | (s[1] << 8);
}

static uint32_t load32le(const void *src)
{
	const uint8_t *s = (const uint8_t *)src;
	return s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);
}

static uint16_t store16(char far *dest, uint16_t value)
{
	const uint8_t *src = (const uint8_t *)&value;
	*dest++ = *src++;
	*dest++ = *src++;
	return value;
}

static uint32_t store32(char far *dest, uint32_t value)
{
	const uint8_t *src = (const uint8_t *)&value;
	*dest++ = *src++;
	*dest++ = *src++;
	*dest++ = *src++;
	*dest++ = *src++;
	return value;
}

static uint32_t plot(char far *vram, const struct s_dot *si)
{
	const int di0 = si->old2, di1 = di0 + 320, di2 = di1 + 320;
	store32(&vram[di0], load32le(&bgpic[di0]));
	store32(&vram[di1], load32le(&bgpic[di1]));
	return store32(&vram[di2], load32le(&bgpic[di2]));
}

void drawdots(void)
{
	char far *const vram = MK_FP(0xa000, 0);
	const struct s_dot *const end = &dot[dotnum];
	struct s_dot *si = dot;
	for (; si < end; si++)
	{
		int bp = ((si->z * rotcos - si->x * rotsin) >> 16) + 9000;
		int temp = (si->z * rotsin + si->x * rotcos) >> 8;
		const int column = (uint16_t)(((temp >> 3) + temp) / bp + 160);
		if (column <= 319)
		{
			int row = (uint16_t)((8 << 16) / bp + 100);
			if (row <= 199)
			{
				// shadow

				store16(&vram[si->old1], load16le(&bgpic[si->old1]));
				si->old1 = rows[row] + column;
				store16(&vram[si->old1], 87 + 87 * 256);

				// ball

				si->yadd += gravity;
				si->y = si->y + si->yadd;
				if (si->y >= gravitybottom)
				{
					si->yadd = (int16_t)(-si->yadd * gravityd) >> 4;
					si->y += si->yadd;
				}

				row = (uint16_t)((si->y << 6) / bp + 100);
				if (row <= 199)
				{
					plot(vram, si);

					bp = ((uint16_t)bp >> 6) & ~3;
					if (bp < bpmin)
						bpmin = bp;
					if (bp > bpmax)
						bpmax = bp;

					temp = rows[row] + column;
					store16(&vram[temp + 1], load16le((const char *)depthtable1 + bp));
					store32(&vram[temp + 320], load32le((const char *)depthtable2 + bp));
					store16(&vram[temp + 641], load16le((const char *)depthtable3 + bp));
					si->old2 = temp;

					continue;
				}
			}
		}
		temp = si->old1;
		si->old1 = (uint16_t)plot(vram, si);
		store16(&vram[temp], load16le(&bgpic[temp]));
	}
}

void setpalette(const uint8_t *pal)
{
	int cx = 768;
	outp(0x3c8, 0);
	while (cx--)
		outp(0x3c9, *pal++);
}

int face[] =
{
	2248,-2306,0, // from face.inc
	30000,30000,30000
};
