//
// ported from copper.asm
//
#include <stdint.h>
#include "../../u2-port.h"

unsigned alku_frame_count;
uint8_t *alku_cop_pal;
unsigned alku_do_pal;
unsigned alku_cop_start;
unsigned alku_cop_scrl;

static void copper1(void);
static void copper2(void);
static void copper3(void);

void alku_close_copper(void)
{
	dis_setcopper(0, NULL);
	dis_setcopper(1, NULL);
	dis_setcopper(2, NULL);
}

void alku_init_copper(void)
{
	dis_setcopper(1, copper1);
	dis_setcopper(2, copper2);
	dis_setcopper(0, copper3);
}

static void copper1(void)
{
	outport(0x03d4, (uint16_t)((alku_cop_start << 8) | 0x0d));
	outport(0x03d4, (uint16_t)((alku_cop_start & 0xff00) | 0x0c));

	outportb(0x3c0, 0x33);
	outportb(0x3c0, (uint8_t)alku_cop_scrl);
}

static void copper2(void)
{
	alku_frame_count++;

	if (alku_do_pal)
	{
		int i;

		outportb(0x3c8, 0);
		for (i = 0; i < 768; i++)
			outportb(0x3c9, alku_cop_pal[i]);
		alku_do_pal = 0;
	}
}

unsigned alku_cop_dofade;

uint8_t alku_fadepal[768 * 2];

uint16_t *alku_cop_fadepal;

static void copper3(void)
{
	if (alku_cop_dofade)
	{
		unsigned di = 0, si = 0;

		alku_cop_dofade--;

		alku_cop_pal = alku_fadepal;
		alku_do_pal = 1;

		while (di < 768)
		{
			const unsigned ax = alku_cop_fadepal[si], sum = alku_fadepal[di + 768] + (uint8_t)ax;
			alku_fadepal[di + 768] = (uint8_t)sum;
			alku_fadepal[di++] += (uint8_t)((ax >> 8) + (sum >> 8));
			si += 2;
		}
	}
}
