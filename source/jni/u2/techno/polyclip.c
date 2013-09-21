//
// ported from polyclip.asm
// to be included from techno-koea.c
//
// variables below come from techno-koea.c:
// - static unsigned polyisides;
// - static int16_t polyixy[16][2];
// - static unsigned polysides;
// - static int16_t polyxy[16][2];
//
// and the only function externalized from this file is clipanypoly()
//
#include <stdint.h>

static int16_t WMINY;
static int16_t WMAXY = 199;
static int16_t WMINX;
static int16_t WMAXX = 319;

static int16_t clip_x1;
static int16_t clip_y1;
static int16_t clip_x2;
static int16_t clip_y2;
static int16_t clipxy2[32][2]; // tmp storage for polyclip

static unsigned cliplinex(void);
static unsigned clipliney(void);

static void clipanypoly_cap2r(void)
{
	if (!clipliney() && !cliplinex())
	{
		polyxy[0][0] = clip_x1;
		polyxy[0][1] = clip_y1;
		polyxy[1][0] = clip_x2;
		polyxy[1][1] = clip_y2;
		polysides = ((clip_x1 == clip_x2) && (clip_y1 == clip_y2)) ? 1 : 2;
	}
}

static void clipanypoly(void) // polyisides/polyixy =>polysides/polyxy
{
	unsigned ax, cx, di;
	int16_t xxx, yyy;
	if (polyisides <= 2)
	{
		polysides = 0;
		if (polyisides == 1) // dot
		{
			if ((polyixy[0][0] >= WMINX) && (polyixy[0][0] <= WMAXX) && (polyixy[0][1] >= WMINY) && (polyixy[0][1] <= WMAXY))
			{
				polyxy[0][0] = polyixy[0][0];
				polyxy[0][1] = polyixy[0][1];
				polysides = 1;
			}
		}
		else if (polyisides) // line
		{
			clip_x1 = polyixy[0][0];
			clip_y1 = polyixy[0][1];
			clip_x2 = polyixy[1][0];
			clip_y2 = polyixy[1][1];
			clipanypoly_cap2r();
		}
		return;
	}
	// polygon, first clip y, then x
	clip_x1 = polyixy[polyisides - 1][0];
	clip_y1 = polyixy[polyisides - 1][1];
	clip_x2 = polyixy[0][0];
	clip_y2 = polyixy[0][1];
	ax = clipliney();
	cx = polyisides;
	di = 0;
	polysides = 0;
	xxx = yyy = 0x8000; // TODO:
	while (cx)
	{
		if (!ax)
		{
			if ((clip_x1 != xxx) || (clip_y1 != yyy))
			{
				clipxy2[polysides][0] = clip_x1;
				clipxy2[polysides][1] = clip_y1;
				xxx = clip_x1;
				yyy = clip_y1;
				polysides++;
			}
			if ((clip_x2 != xxx) || (clip_y2 != yyy))
			{
				clipxy2[polysides][0] = clip_x2;
				clipxy2[polysides][1] = clip_y2;
				xxx = clip_x2;
				yyy = clip_y2;
				polysides++;
			}
		}
		di++;
		if (--cx)
		{
			clip_x1 = polyixy[di - 1][0];
			clip_y1 = polyixy[di - 1][1];
			clip_x2 = polyixy[di][0];
			clip_y2 = polyixy[di][1];
			ax = clipliney();
		}
	}
	if ((clipxy2[0][0] == xxx) && (clipxy2[0][1] == yyy))
		polysides--;
	if (polysides <= 2)
	{
		if (polysides) // reclip the remaining line
		{
			polysides = 0;
			clip_x1 = clipxy2[0][0];
			clip_y1 = clipxy2[0][1];
			clip_x2 = clipxy2[1][0];
			clip_y2 = clipxy2[1][1];
			clipanypoly_cap2r();
		}
		return;
	}
	clip_x1 = clipxy2[polysides - 1][0];
	clip_y1 = clipxy2[polysides - 1][1];
	clip_x2 = clipxy2[0][0];
	clip_y2 = clipxy2[0][1];
	ax = cliplinex();
	cx = polysides;
	di = 0;
	polysides = 0;
	xxx = yyy = 0x8000; // TODO:
	while (cx)
	{
		if (!ax)
		{
			if ((clip_x1 != xxx) || (clip_y1 != yyy))
			{
				polyxy[polysides][0] = clip_x1;
				polyxy[polysides][1] = clip_y1;
				xxx = clip_x1;
				yyy = clip_y1;
				polysides++;
			}
			if ((clip_x2 != xxx) || (clip_y2 != yyy))
			{
				polyxy[polysides][0] = clip_x2;
				polyxy[polysides][1] = clip_y2;
				xxx = clip_x2;
				yyy = clip_y2;
				polysides++;
			}
		}
		di++;
		if (--cx)
		{
			clip_x1 = clipxy2[di - 1][0];
			clip_y1 = clipxy2[di - 1][1];
			clip_x2 = clipxy2[di][0];
			clip_y2 = clipxy2[di][1];
			ax = cliplinex();
		}
	}
	if ((polyxy[0][0] == xxx) && (polyxy[0][1] == yyy))
		polysides--;
}

static unsigned clipcheck(int reg, int min, int max, unsigned flagmin, unsigned flagmax)
{
	unsigned flagreg = 0;
	if (reg < min)
		flagreg |= flagmin;
	if (reg > max)
		flagreg |= flagmax;
	return flagreg;
}

static void clipmacro(int16_t *v1, int16_t v2, int16_t *w1, int16_t w2, int16_t wl)
{
	const int cx = w2 - *w1;
	if (!cx)
		*w1 = wl;
	else
	{
		*v1 += (int16_t)((v2 - *v1) * (wl - *w1) / cx);
		*w1 = wl;
	}
}

static unsigned cliplinex(void)
{
	const unsigned bl = clipcheck(clip_x1, WMINX, WMAXX, 1, 2);
	const unsigned bh = clipcheck(clip_x2, WMINX, WMAXX, 1, 2);
	const unsigned al = bl & bh;
	if (!al)
	{
		if (bl & 1)
			clipmacro(&clip_y1, clip_y2, &clip_x1, clip_x2, WMINX);
		if (bl & 2)
			clipmacro(&clip_y1, clip_y2, &clip_x1, clip_x2, WMAXX);
		if (bh & 1)
			clipmacro(&clip_y2, clip_y1, &clip_x2, clip_x1, WMINX);
		if (bh & 2)
			clipmacro(&clip_y2, clip_y1, &clip_x2, clip_x1, WMAXX);
	}
	return al;
}

static unsigned clipliney(void)
{
	const unsigned bl = clipcheck(clip_y1, WMINY, WMAXY, 4, 8);
	const unsigned bh = clipcheck(clip_y2, WMINY, WMAXY, 4, 8);
	const unsigned al = bl & bh;
	if (!al)
	{
		if (bl & 4)
			clipmacro(&clip_x1, clip_x2, &clip_y1, clip_y2, WMINY);
		if (bl & 8)
			clipmacro(&clip_x1, clip_x2, &clip_y1, clip_y2, WMAXY);
		if (bh & 4)
			clipmacro(&clip_x2, clip_x1, &clip_y2, clip_y1, WMINY);
		if (bh & 8)
			clipmacro(&clip_x2, clip_x1, &clip_y2, clip_y1, WMAXY);
	}
	return al;
}
