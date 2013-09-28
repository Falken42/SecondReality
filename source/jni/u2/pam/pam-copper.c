//
// ported from copper.asm
//
#include <stdint.h>
#include "../../u2-port.h"

unsigned pam_frame_count;
uint8_t *pam_cop_pal;
unsigned pam_do_pal;

static void copper2(void);
static void copper3(void);

void pam_close_copper(void)
{
	dis_setcopper(0, NULL);
	dis_setcopper(1, NULL);
	dis_setcopper(2, NULL);
}

void pam_init_copper(void)
{
	dis_setcopper(1, copper3);
	dis_setcopper(2, copper2);
	dis_setcopper(0, copper3);
}

static void copper2(void)
{
	pam_frame_count++;

	if (pam_do_pal)
	{
		int i;

		outportb(0x3c8, 0);
		for (i = 0; i < 768; i++)
			outportb(0x3c9, pam_cop_pal[i]);
		pam_do_pal = 0;
	}
}

static void copper3(void) {}
