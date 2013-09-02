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

int gravity;

int dotnum;

int gravityd = 16;

int rows[200];

struct s_dot // ripped from main.c
{
	int x;
	int y;
	int z;
	int old1;
	int old2;
	int old3; // unused
	int old4; // unused
	int yadd;
};
struct s_dot dot[MAXDOTS];

int rotsin;
int rotcos;

char *bgpic; // expects at least [320*202]

char depthtable1[512];
char depthtable2[512];
char depthtable3[512];

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

static void store16(char far *dest, uint16_t value)
{
	const uint8_t *src = (const uint8_t *)&value;
	*dest++ = *src++;
	*dest = *src;
}

static void store32(char far *dest, uint32_t value)
{
	const uint8_t *src = (const uint8_t *)&value;
	*dest++ = *src++;
	*dest++ = *src++;
	*dest++ = *src++;
	*dest = *src;
}

void drawdots(void)
{
	char far *const vram = MK_FP(0xa000, 0);

	struct s_dot *si = dot, *const end = si + dotnum;
	for (; si < end; si++)
	{
		uint32_t eax;
		const int z = ((si->z * rotcos - si->x * rotsin) >> 16) + 9000;
		const int x = ((si->z * rotsin + si->x * rotcos) >> 8);
		const int column = (uint16_t)(((x >> 3) + x) / z + 160);
		if (column <= 319)
		{
			int row = (uint16_t)((8 << 16) / z + 100);
			if (row <= 199)
			{
				// shadow

				const int shadow_offset = rows[row] + column;

				store16(&vram[si->old1], load16le(&bgpic[si->old1]));
				store16(&vram[shadow_offset], 87 + 87 * 256);
				si->old1 = shadow_offset;

				// ball

				si->yadd += gravity;
				si->y = si->y + si->yadd;
				if (si->y >= gravitybottom)
				{
					si->yadd = (-si->yadd * gravityd) >> 4;
					si->y += si->yadd;
				}

				row = (uint16_t)((si->y << 6) / z + 100);
				if (row <= 199)
				{
					const int ball_offset = rows[row] + column;
					const int bp = ((uint16_t)z >> 6) & ~3; // [0,512)

					store32(&vram[si->old2], load32le(&bgpic[si->old2]));
					store32(&vram[si->old2 + 320], load32le(&bgpic[si->old2 + 320]));
					store32(&vram[si->old2 + 640], load32le(&bgpic[si->old2 + 640]));
					store16(&vram[ball_offset + 1], load16le(&depthtable1[bp]));
					store32(&vram[ball_offset + 320], load32le(&depthtable2[bp]));
					store16(&vram[ball_offset + 641], load16le(&depthtable3[bp]));
					si->old2 = ball_offset;

					continue;
				}
			}
		}

		store32(&vram[si->old2], load32le(&bgpic[si->old2]));
		store32(&vram[si->old2 + 320], load32le(&bgpic[si->old2 + 320]));
		store32(&vram[si->old2 + 640], eax = load32le(&bgpic[si->old2 + 640]));
		store16(&vram[si->old1], load16le(&bgpic[si->old1]));
		si->old1 = (uint16_t)eax;
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
	2248, -2306, 0, // from face.inc
	30000, 30000, 30000
};
