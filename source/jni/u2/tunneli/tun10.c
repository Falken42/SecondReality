//
// ported from tunneli/tun10.pas
//
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "../../u2-port.h"

static const int veke = 1060; // frame count to exit

struct bc
{
	uint16_t x;
	uint16_t y;
};

struct rengas
{
	int x, y;
	int c;
};

static struct rengas putki[103];
static struct bc pcalc[138][64];

static int rows[201];

static int16_t sinit[4097];
static int16_t cosit[2049];
static int sade[103];

// $L u2-port.c
extern char tun[sizeof(pcalc)]; // [35328]
// $L u2-port.c
extern char sini[sizeof(sinit) + sizeof(cosit)]; // [12292]

static void setrgb(int c, int r, int g, int b)
{
	outportb(0x3c8, (uint8_t)c);
	outportb(0x3c9, (uint8_t)r);
	outportb(0x3c9, (uint8_t)g);
	outportb(0x3c9, (uint8_t)b);
}

static int waitr(void)
{
	setrgb(0, 0, 0, 0);
	return dis_waitb();
}

static uint16_t oldpos[7501];

static int pascal_round(float x)
{
	return (int)floor(x + ((x > 0.0f) ? 0.5f : -0.5f));
}

void tunneli_main(void)
{
	char far *const vram = MK_FP(0xa000, 0);
	int x, frame = 0;
	const int y = 0;
	uint16_t sx = 0, sy = 0;

	for (x = 0; x <= 200; x++) rows[x] = x * 320;
	memcpy(sinit, sini, 4097 * 2);
	memcpy(cosit, &sini[4097 * 2], 2048 * 2);
	memcpy(pcalc, tun, sizeof(pcalc));

	// int10h(0x13);

	dis_partstart();

	for (x = 0; x <= 64; x++) setrgb(64 + x, 64 - x, 64 - x, 64 - x);
	for (x = 0; x <= 64; x++) setrgb(128 + x, (64 - x) *3 / 4, (64 - x) *3 / 4, (64 - x) *3 / 4);

	setrgb(68, 0, 0, 0);
	setrgb(132, 0, 0, 0);
	setrgb(255, 0, 63, 0);

	memset(putki, 0, sizeof(putki));

	waitr();
	for (x = 0; x <= 100; x++) sade[x] = pascal_round(16384.0f / ((x * 7) + 95));

	for (;;)
	{
		const int frames = waitr();
		int ry = 0, sync;

		for (x = 80; x >= 4; x--)
		{
			const uint16_t bx = (uint16_t)(putki[x].x - putki[5].x);
			const uint16_t by = (uint16_t)(putki[x].y - putki[5].y);
			const uint8_t bbc = (uint8_t)(putki[x].c + pascal_round(x / 1.3f));
			if (bbc >= 64)
			{
				struct bc *pcp = &pcalc[(uint8_t)sade[x]][0];
				int cx = 64;
				while (cx--)
				{
					uint16_t di = bx + pcp->x;
					vram[oldpos[ry]] = 0;
					if (di <= 319)
					{
						const uint16_t row = by + pcp->y;
						if (row <= 199)
						{
							di += rows[row];
							vram[di] = bbc;
							oldpos[ry] = di;
						}
					}
					pcp++;
					ry++;
				}
			}
		}

		for (sync = 1; sync <= frames; sync++)
		{
			putki[100].x = cosit[sy & 2047] - sinit[(sy * 3) & 4095] - cosit[sx & 2047];
			putki[100].y = sinit[(sx * 2) & 4095] - cosit[sx & 2047] + sinit[y & 4095];
			memmove(&putki[0], &putki[1], sizeof(putki[0]) * 100);
			sy++;
			sx++;

			if ((sy & 15) > 7) putki[99].c = 128; else putki[99].c = 64;
			if (frame >= veke - 102) putki[99].c = 0;
			if (frame++ == veke) return;;

			if (dis_exit()) return;
		}
	}
}
