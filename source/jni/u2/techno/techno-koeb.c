//
// ported from koeb.asm
//
#include <stdint.h>
#include "../../u2-port.h"

// in ../../u2-port.c
extern char circle[24000];
extern char circle2[8000];

// ################################################################

static unsigned palfader;

static const uint8_t flip8[] =
{
	0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240, 8, 136, 72, 200,
	40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248, 4, 132, 68, 196, 36, 164, 100,
	228, 20, 148, 84, 212, 52, 180, 116, 244, 12, 140, 76, 204, 44, 172, 108, 236, 28, 156,
	92, 220, 60, 188, 124, 252, 2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178,
	114, 242, 10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250, 6,
	134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246, 14, 142, 78, 206,
	46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254, 1, 129, 65, 193, 33, 161, 97, 225,
	17, 145, 81, 209, 49, 177, 113, 241, 9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217,
	57, 185, 121, 249, 5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
	13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253, 3, 131, 67, 195,
	35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243, 11, 139, 75, 203, 43, 171, 107, 235,
	27, 155, 91, 219, 59, 187, 123, 251, 7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215,
	55, 183, 119, 247, 15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255
};

static uint8_t pal[32 * 3];

static const uint8_t pal0[] =
{
	// pal0
	0, 30, 40,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 30, 40,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	// pal1
	0, 0 * 7 / 9, 0,
	10, 10 * 7 / 9, 10,
	20, 20 * 7 / 9, 20,
	30, 30 * 7 / 9, 30,
	40, 40 * 7 / 9, 40,
	50, 50 * 7 / 9, 50,
	60, 60 * 7 / 9, 60,
	30, 30 * 7 / 9, 30,
	0, 0 * 7 / 9, 0,
	10, 10 * 7 / 9, 10,
	20, 20 * 7 / 9, 20,
	30, 30 * 7 / 9, 30,
	40, 40 * 7 / 9, 40,
	50, 50 * 7 / 9, 50,
	60, 60 * 7 / 9, 60,
	30, 30 * 7 / 9, 30,
	// pal2
	50, 50 * 6 / 9, 50,
	60, 60 * 6 / 9, 60,
	30, 30 * 6 / 9, 30,
	0, 0 * 6 / 9, 0,
	10, 10 * 6 / 9, 10,
	20, 20 * 6 / 9, 20,
	30, 30 * 6 / 9, 30,
	40, 40 * 6 / 9, 40,
	50, 50 * 6 / 9, 50,
	60, 60 * 6 / 9, 60,
	30, 30 * 6 / 9, 30,
	0, 0 * 6 / 9, 0,
	10, 10 * 6 / 9, 10,
	20, 20 * 6 / 9, 20,
	30, 30 * 6 / 9, 30,
	40, 40 * 6 / 9, 40
};

char power1[256 * 16];

static void bltline(char *dest, const char *src, uint8_t start_at_plane, int copy_planes)
{
	outportb(0x3c4, 2);
	while (copy_planes--)
	{
		int zzz;
		outportb(0x3c5, start_at_plane);
		for (zzz = 0; zzz < 40; zzz++)
			dest[zzz] = src[zzz];
		src += 40;
		start_at_plane <<= 1;
	}
}

static void bltlinerev(char *dest, const char *src, uint8_t start_at_plane, int copy_planes)
{
	outportb(0x3c4, 2);
	while (copy_planes--)
	{
		int zzz;
		outportb(0x3c5, start_at_plane);
		for (zzz = 0; zzz < 40; zzz++)
			dest[zzz] = flip8[src[39 - zzz]];
		src += 40;
		start_at_plane <<= 1;
	}
}

static void resetmode13(void)
{
	unsigned i;

	dis_waitb();

	outportb(0x3c8, 0);
	for (i = 0; i < 768; i++)
		outportb(0x3c9, 0);

	dis_waitb();
	int10h(0x13);

	inportb(0x3da);
	for (i = 0; i < 16; i++)
	{
		outportb(0x3c0, (uint8_t)i);
		outportb(0x3c0, (uint8_t)i);
	}

	outportb(0x3c0, 0x11);
	outportb(0x3c0, 255);
	outportb(0x3c0, 32);

	// clear pal
	outportb(0x3c8, 0);
	for (i = 0; i < 768; i++)
		outportb(0x3c9, 0);
}

static void mixpal(uint8_t *dest, const uint8_t *src, unsigned bytes, unsigned palfader)
{
	if (palfader <= 256)
		while (bytes--)
			*dest++ = (uint8_t)((*src++ * palfader) >> 8);
	else
	{
		const unsigned bx = palfader - 256;
		while (bytes--)
		{
			const unsigned ax = *src++ + bx;
			*dest++ = (uint8_t)((ax < 64) ? ax : 63);
		}
	}
}

static void outpal(uint8_t start, const uint8_t *src, unsigned bytes)
{
	outportb(0x3c8, start);
	while (bytes--)
		outportb(0x3c9, *src++);
}

static unsigned framecount;
static int palanimc;
static int scrnpos;
static int scrnposl;
static int scrnx;
static int scrny;
static const int patdir = -3;

static void init_interference(void)
{
	char *const vram = MK_FP(0x0a000, 0), *di, *bp;
	const char *si;
	int zzz;

	outport(0x3d4, 0x2813);

	si = circle2;
	di = vram;
	bp = vram + 80 * 399;
	for (zzz = 0; zzz < 200; zzz++)
	{
		bltline(di, si, 4, 1);
		bltlinerev(di + 40, si, 4, 1);
		bltline(bp, si, 4, 1);
		bltlinerev(bp + 40, si, 4, 1);
		di += 80;
		bp -= 80;
		si += 40;
	}

	outport(0x3ce, 0x204);

	si = circle;
	di = vram;
	bp = vram + 80 * 399;
	for (zzz = 0; zzz < 200; zzz++)
	{
		bltline(di, si, 1, 3); // start at plane 1, copy 3 planes
		bltlinerev(di + 40, si, 1, 3); // start at plane 1, copy 3 planes
		bltline(bp, si, 1, 3); // start at plane 1, copy 3 planes
		bltlinerev(bp + 40, si, 1, 3); // start at plane 1, copy 3 planes
		di += 80;
		bp -= 80;
		si += 40 * 3;
	}

	framecount = 0;
}

static void do_interference(void)
{
	do
	{
		dis_waitb();
		outportb(0x3c0, 0x13);
		outportb(0x3c0, (uint8_t)scrnposl);
		outportb(0x3c0, 32);

		palanimc += patdir;
		if (palanimc < 0)
			palanimc = 8 * 3 - 3;
		if (palanimc >= 8 * 3)
			palanimc = 0;

		palfader += 2;
		if (palfader >= 512)
			palfader = 512;

		mixpal(pal, pal0 + palanimc, 8 * 3, palfader);
		mixpal(pal + 8 * 3, pal0 + palanimc, 8 * 3, palfader);

		outpal(0, pal, 16 * 3);

		scrnx = 160;
		scrny = 100;

		scrnposl = scrnx & 7;
		scrnpos = 80 * scrny + (scrnx >> 3);

		outport(0x3d4, (uint16_t)((scrnpos & 0xff00) | 0x0c));
		outport(0x3d4, (uint16_t)((scrnpos << 8) | 0x0d));
	}
	while ((++framecount < 256) && !dis_exit());
}

void dointerference2(void)
{
	resetmode13();
	init_interference();
	do_interference();
}
