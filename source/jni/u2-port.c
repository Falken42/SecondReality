#include <stdio.h>
#include "u2-port.h"

// Second Reality externed variables
// part 1: alku
char  hzpic[65535 + 63250];		// sizes calced from parsed .inc data
char  font[32][1500];			// incremented to 32 to handle out-of-bounds access in alku
int   frame_count;
char *cop_pal;
int   do_pal;
int   cop_start;
int   cop_scrl;
int   cop_dofade;
char *cop_fadepal;
char  fadepal[768*2];

// part3: pam
char  pal[769 + (768 * 64)];			// size from pal.inc plus 768*64 buffer in pam/include.asm
char  memblock[(65535 * 4) + 62486];	// sizes calced from parsed out.in[0-4] data

// part4: beg
char  pic[44575];				// size of srtitle.up

// internal variables (for timing, vga emulation, etc)
static int last_frame_time;
static int dis_sync_val, dis_sync_time, dis_partid = 0;
static unsigned int vga_width, vga_height, vga_stride, vga_start;
static uint8_t vga_pal[768], *vga_plane[4], *vga_buffer;
static uint8_t vga_pal_index, vga_pal_comp, vga_adr_reg, vga_attr_reg, vga_cur_plane, vga_chain4, vga_horiz_pan;

// some prototypes
void outp(unsigned short int port, unsigned char val);

// round value to next power of two (or return same if already a power of two)
static int nextPow2(int val)
{
	int next = 1;
	
	while (next < val)
		next <<= 1;
	
	return next;
}

// bit counter (hamming weight)
// source: http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
static int bitcount(int val)
{
	val = val - ((val >> 1) & 0x55555555);
	val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
	return (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

// integer log2, used to determine highest bit set in a value
static int ilog2(int val)
{
	int res = 0;

	while (val >>= 1)
		++res;

	return res;
}

static void demo_init()
{
	int t;

	// allocate the emulated direct access VGA memory area
	vga_buffer = (uint8_t *)malloc(65536);
	memset(vga_buffer, 0, 65536);

	// allocate space for all four VGA memory planes
	for (t = 0; t < 4; t++)
	{
		vga_plane[t] = (uint8_t *)malloc(65536);
		memset(vga_plane[t], 0, 65536);
	}

	// init some VGA register defaults
	vga_chain4	  = 0x08;
	vga_attr_reg  = 0;
	vga_start	  = 0;
	vga_cur_plane = 0;
	vga_horiz_pan = 0;

	// initialize demo state
	frame_count     = 0;
	last_frame_time = platform_get_usec();
	dis_sync_val    = 0;
	dis_sync_time   = last_frame_time;
}

static void demo_set_video_mode(int width, int height, int stride)
{
	vga_width  = width;
	vga_height = height;
	vga_stride = stride;

	platform_set_video_mode(width, height, stride);
}

static void demo_render_frame()
{
	int w, h;
	uint8_t *fb = platform_lock_framebuffer();
	const int stride = platform_get_framebuffer_stride();

	if (vga_chain4)
	{
		// mode 13h, convert linear VRAM to 24-bit RGB
		const uint8_t *src = vga_plane[0];
		for (h = 0; h < vga_height; h++)
		{
			uint8_t *dest = fb + (h * stride * 3);
			for (w = 0; w < vga_width; w++)
			{
				// read color index from VGA
				const int idx = (*src++) * 3;

				// write RGB colors (scaling up 6 bit to 8 bit)
				*dest++ = vga_pal[idx    ] << 2;
				*dest++ = vga_pal[idx + 1] << 2;
				*dest++ = vga_pal[idx + 2] << 2;
			}
		}
	}
	else
	{
		// mode x, convert four planed VRAM to 24-bit RGB
		for (h = 0; h < vga_height; h++)
		{
			const int row_offset = (h * (vga_stride >> 2)) + vga_start;
			const uint8_t *src0 = vga_plane[0] + row_offset;
			const uint8_t *src1 = vga_plane[1] + row_offset;
			const uint8_t *src2 = vga_plane[2] + row_offset;
			const uint8_t *src3 = vga_plane[3] + row_offset;
			uint8_t *dest = fb + (h * stride * 3);

			// advance pointers if horizontal panning is active
			switch (vga_horiz_pan & 3)
			{
				case 3: src2++;
				case 2: src1++;
				case 1: src0++;
			}

			for (w = vga_horiz_pan; w < vga_width + vga_horiz_pan; w++)
			{
				// obtain color index based on X position
				int idx;
				switch (w & 3)
				{
					case 0: idx = *src0++; break;
					case 1: idx = *src1++; break;
					case 2: idx = *src2++; break;
					case 3: idx = *src3++; break;
				}

				idx *= 3;

				// write RGB colors (scaling up 6 bit to 8 bit)
				*dest++ = vga_pal[idx    ] << 2;
				*dest++ = vga_pal[idx + 1] << 2;
				*dest++ = vga_pal[idx + 2] << 2;
			}
		}
	}

	// show the new frame buffer
	platform_unlock_framebuffer(fb);
	platform_draw_frame();
}

static void *demo_load_assetsz(const char *fname, int *size)
{
	void *res;

	PFILE *fp = platform_fopen(fname, "rb");
	if (fp == NULL)
	{
		LOGW("load_asset: failed to open [%s]!", fname);
		return NULL;
	}

	// get size of asset
	platform_fseek(fp, 0, SEEK_END);
	*size = platform_ftell(fp);
	platform_fseek(fp, 0, SEEK_SET);

	// allocate memory and load
	LOGI("load_asset: loading [%s] (%d bytes)...", fname, *size);
	res = malloc(*size + 1); // +1 since demo_load_asmincsz() puts a \0 char at the end during tokenizing
	platform_fread(res, 1, *size, fp);
	platform_fclose(fp);
	return res;
}

static void *demo_load_asset(const char *fname)
{
	int dummy;
	return demo_load_assetsz(fname, &dummy);
}

static void *demo_load_asmincsz(const char *fname, int *size)
{
	const int block_size = 16 * 1024;
	int asset_size;
	char *inc, *src, *end, *next;
	uint8_t *res  = NULL;
	uint8_t *dest = NULL;
	int dist, total = 0;

	inc = demo_load_assetsz(fname, &asset_size);
	if (inc == NULL)
		return NULL;

	src = inc;
	end = inc + asset_size;

	// parse the assembly file
	for (;;)
	{
		// skip over spaces, tabs, crlfs, or the letters 'd' and 'b'
		while ((src < end) && ((*src == ' ') || (*src == '\t') || (*src == '\r') || (*src == '\n') || (*src == 'd') || (*src == 'b')))
			src++;

		if (src >= end)
			break;

		// advance until we reach a non-integer character
		next = src;
		while ((next < end) && ((*next == '-') || isdigit(*next)))
			next++;

		*next = 0;

		// check if we need to expand our destination buffer
		dist = dest - res;
		if (dist >= total)
		{
			total += block_size;
			res    = realloc(res, total);
			dest   = res + dist;
		}

		// convert this value
		*dest++ = atoi(src);
		src = next + 1;
	}

	free(inc);
	*size = dest - res;
	LOGI("load_asminc: parsed to [%d] binary bytes", *size);
	return res;
}

static void *demo_load_asminc(const char *fname)
{
	int dummy;
	return demo_load_asmincsz(fname, &dummy);
}

static void *demo_append_asminc(void *data, const char *fname, int *size)
{
	int size2;
	void *data2 = demo_load_asmincsz(fname, &size2);
	if (data2 == NULL)
		return NULL;

	data = realloc(data, *size + size2);
	if (data == NULL)
		return NULL;

	memcpy((uint8_t *)data + *size, data2, size2);
	free(data2);
	*size += size2;
	return data;
}

void demo_execute()
{
	int size;
	void *tmp;

	// initialize internal state
	demo_init();

	// load assets
	// part 1: alku
	tmp = demo_load_asmincsz("fona.inc", &size);
	memcpy(font, tmp, size);
	free(tmp);

	tmp = demo_load_asmincsz("hoi.in0", &size);
	tmp = demo_append_asminc(tmp, "hoi.in1", &size);
	memcpy(hzpic, tmp, size);
	free(tmp);

	// part 3: pam
	tmp = demo_load_asmincsz("out.in0", &size);
	tmp = demo_append_asminc(tmp, "out.in1", &size);
	tmp = demo_append_asminc(tmp, "out.in2", &size);
	tmp = demo_append_asminc(tmp, "out.in3", &size);
	tmp = demo_append_asminc(tmp, "out.in4", &size);
	memcpy(memblock, tmp, size);
	free(tmp);

	tmp = demo_load_asmincsz("pal.inc", &size);
	memcpy(pal, tmp, size);
	free(tmp);

	// part 4: beg
	tmp = demo_load_asset("srtitle.up");
	memcpy(pic, tmp, sizeof(pic));
	free(tmp);

	// execute each part
	alku_main();

	dis_partstart();	// <-- required to increment dis_partid until u2a is implemented

	pam_main();

	// HACK: until we can properly determine the resolution from the VGA registers
    tw_opengraph();
	demo_set_video_mode(320, 400, 320);
	cop_start  = 0;		// <-- this is a hack too. beg_main() probably initializes these on the VGA though.
	cop_scrl   = 0;
	cop_dofade = 0;

	beg_main();

	// EVEN MORE HACK:
	demo_set_video_mode(320, 200, 320);
	outp(0x3C4, 2);
	outp(0x3C5, 1);
	outp(0x3C4, 4);
	outp(0x3C5, 8);
	memset(vga_buffer, 0, 65536);

	dots_main();
}

char *MK_FP(int seg, int off)
{
	// if segment points to VGA memory, then return a pointer to our emulated VGA buffer instead
	if ((seg >= 0xA000) && (seg < 0xB000))
		return vga_buffer + ((seg - 0xA000) << 4) + off;

	// otherwise, return a valid pointer to memory
	return (char *)(seg + (off - seg));
}

static void vga_set_plane(uint8_t mask)
{
	// multiple plane writes currently not supported
//	if (bitcount(mask) > 1)
//		LOGI("vga_set_plane: VGA plane change to multiple planes [mask=0x%02X]", mask);

	// determine new plane, and check if different
	const uint8_t new_plane = ilog2(mask);
	if (vga_cur_plane != new_plane)
	{
		// swap the current VGA buffer out with the new plane's buffer
		memcpy(vga_plane[vga_cur_plane], vga_buffer, 65536);
		memcpy(vga_buffer, vga_plane[new_plane], 65536);

		// store new plane
		vga_cur_plane = new_plane;
	}
}

void outportb(unsigned short int port, unsigned char val)
{
	switch (port)
	{
		case 0x3C0:
			// vga attribute address/data register
			if (vga_attr_reg == 0)
			{
				// store register index for next iteration
				vga_attr_reg = val;
			}
			else
			{
				// only registers 00h-1Fh seem to exist? but U2 references register 33h!
				switch (vga_attr_reg & 0x1F)
				{
					case 0x13:
						// horizontal pixel panning (not sure why this is scaled by 2 yet)
						vga_horiz_pan = val >> 1;
						break;

					default:
						LOGI("outportb(0x%03X, %d): unhandled VGA attribute register [%d]", port, val, vga_attr_reg);
						break;
				}

				// clear register index
				vga_attr_reg = 0;
			}
			break;

		case 0x3C4:
			// set vga address register
			vga_adr_reg = val;
			break;

		case 0x3C5:
			// vga data register write, using previously set address
			switch (vga_adr_reg)
			{
				case 0x02:
					// map mask register (lower 4 bits contain plane write enable flags)
					vga_set_plane(val);
					break;

				case 0x04:
					// memory mode register (bit 3 contains chain-4 bit)
					vga_chain4 = val & 0x08;
					break;

				default:
					LOGI("outportb(0x%03X, %d): unhandled VGA address register [%d]", port, val, vga_adr_reg);
					break;
			}
			break;

		case 0x3C7:
		case 0x3C8:
			// get(0x3C7) or set(0x3C8) palette color index, starting with red component
			vga_pal_index = val;
			vga_pal_comp  = 0;
			break;

		case 0x3C9:
			// set palette entry
			vga_pal[(vga_pal_index * 3) + vga_pal_comp] = val;

			// increment component, check for rollover
			if ((++vga_pal_comp) >= 3)
			{
				vga_pal_index++;
				vga_pal_comp = 0;
			}
			break;

		case 0x3CE:
			// set vga address register
			vga_adr_reg = val;
			break;
		
		case 0x3CF:
			// vga data register write, using previously set address
			switch (vga_adr_reg)
			{
				case 0x04:
					// read map select
					vga_set_plane(1 << val); // FIXME: shouldn't actually change the active write plane!
					break;

				default:
					LOGI("outportb(0x%03X, %d): unhandled VGA address register [%d]", port, val, vga_adr_reg);
					break;
			}
			break;

		case 0x3D4:
			// set vga address register
			vga_adr_reg = val;
			break;

		case 0x3D5:
			// vga data register write, using previously set address
			switch (vga_adr_reg)
			{
				case 0x0C:
					// start address high
					vga_start &= 0x00FF;
					vga_start |= ((int)val) << 8;
					break;

				case 0x0D:
					// start address low
					vga_start &= 0xFF00;
					vga_start |= val;
					break;

				default:
					LOGI("outportb(0x%03X, %d): unhandled VGA address register [%d]", port, val, vga_adr_reg);
					break;
			}
			break;

		default:
			LOGI("outportb(0x%03X, %d): unhandled port", port, val);
			break;
	}
}

void outport(unsigned short int port, unsigned short int val)
{
	outportb(port,     val & 0xFF);
	outportb(port + 1, val >> 8);
}

unsigned char inportb(unsigned short int port)
{
	unsigned char val = 0;
	switch (port)
	{
		case 0x3C9:
			// get palette entry
			val = vga_pal[(vga_pal_index * 3) + vga_pal_comp];

			// increment component, check for rollover
			if ((++vga_pal_comp) >= 3)
			{
				vga_pal_index++;
				vga_pal_comp = 0;
			}
			break;

		default:
			LOGI("inportb(0x%03X): unhandled port", port);
			break;
	}
	return val;
}


// from alku/asmyt.asm
void outline(char *src, char *dest)
{
	int mrol = 0x08, cnt, ccc;

	for (cnt = 4; cnt > 0; cnt--)
	{
		const char *si = src + cnt - 1;
		char *di = dest;

		outport(0x3C4, (mrol << 8) | 0x02);

		di[-352      ] = 0;
		di[-352 + 176] = 0;

		for (ccc = 0; ccc < 75; ccc++)
		{
			const char al = si[ccc * 640];
			di[ccc * 352] = al;
			di[ccc * 352 + 176] = al;
		}

		si += 75*40 * 16;		// scale by 16 for segment offset

		for (ccc = 0; ccc < 75; ccc++)
		{
			const char al = si[ccc * 640];
			di[ccc * 352 + 75 * 352] = al;
			di[ccc * 352 + 75 * 352 + 176] = al;
		}

		mrol >>= 1;
	}
}

// from alku/asmyt.asm
void ascrolltext(int scrl, int *text)
{
}

// from beg/asm.asm
void lineblit(char *buf, char *row)
{
	int zpl, zzz;

	for (zpl = 0; zpl < 4; zpl++)
	{
		outport(0x3C4, 0x02 + (0x100 << zpl));

		for (zzz = 0; zzz < 80; zzz += 2)
		{
			const uint8_t al = *(row + (zzz+0) * 4 + zpl);
			const uint8_t ah = *(row + (zzz+1) * 4 + zpl);
			*((uint16_t *)(buf + zzz)) = ((int)ah << 8) | al;
		}
	}
}

// from beg/asm.asm
void setpalarea(char *pal, int start, int cnt)
{
	outportb(0x3C8, start);
	cnt = (cnt << 1) + cnt;
	while (cnt--)
		outportb(0x3C9, *pal++);
}

int dis_indemo()
{
	// always 1 for us
	return 1;
}

int dis_musplus()
{
	// NIY
	return 0;
}

void dis_partstart()
{
	dis_partid++;
	LOGI("---------- starting next part: id=%d ----------", dis_partid);
}

int dis_waitb()
{
	// wait for vblank
	frame_count = 0;
	while (frame_count < 1 && !dis_exit());
	return 1;
}

int dis_sync()
{
//	LOGI("dis_sync = %d", dis_sync_val);

	// COMPLETE HACK (for now): increment sync every 7.5 seconds
	const int now = platform_get_usec();
	if ((now - dis_sync_time) >= (7500 * 1000))
	{
		dis_sync_val++;
		dis_sync_time = now;
		LOGI("dis_sync incremented to = %d", dis_sync_val);
	}

	return dis_sync_val;
}

int dis_exit()
{
//	LOGI("dis_exit: frame_count = %d", frame_count);

	// render a frame every 16.667 msec (60Hz vblank)
	const int now = platform_get_usec();
	if ((now - last_frame_time) >= 16667)
	{
		// copy current VGA buffer to the proper plane
		memcpy(vga_plane[vga_cur_plane], vga_buffer, 65536);

		// handle copper scrolling (func: copper1, alku/copper.asm)
		outport(0x3D4, ((cop_start & 0x00FF) << 8) | 0x0D);
		outport(0x3D4,  (cop_start & 0xFF00)       | 0x0C);
		outportb(0x3C0, 0x33);
		outportb(0x3C0, cop_scrl);

		// handle copper fading (func: copper3, alku/copper.asm)
		if (cop_dofade)
		{
			const uint16_t *src = (const uint16_t *)cop_fadepal;
			uint8_t *dest = fadepal;
			int ccc, cnt = 768 / 16;

			cop_dofade--;
			cop_pal = fadepal;
			do_pal = 1;

			while (cnt--)
			{
				for (ccc = 0; ccc < 16; ccc++)
				{
					const int ax  = src[ccc*2];
					const int sum = dest[ccc + 768] + (ax & 0xFF);
					dest[ccc + 768]  = sum & 0xFF;
					dest[ccc      ] += (ax >> 8) + (sum >> 8);
				}

				dest += 16;
				src  += 32;
			}
		}

		// handle copper palette updates (func: copper2, alku/copper.asm)
		if (do_pal)
		{
			tw_setpalette(cop_pal);
			do_pal = 0;
		}

		frame_count++;
		last_frame_time = now;

		// render this frame
		demo_render_frame();
	}

	return platform_handle_events();
}

void init_copper()
{
}

void close_copper()
{
}

// note: there are multiple tweak.asm sources, each with their own different implementation of tw_opengraph
void tw_opengraph()
{
	// turn off chain-4
	outport(0x3C4, 0x0604);

	switch (dis_partid)
	{
		case 1:
			// alku: 320x372 visible, with 704 pixel stride (176*4)
			demo_set_video_mode(320, 372, 704);
			break;

		case 3:
			// pam: 320x200, unchained
			demo_set_video_mode(320, 200, 320);
			break;
	}
}

void tw_putpixel(int x, int y, int color)
{
	int offset = x >> 2;

	// select write plane based on X coordinate
	const int plane = 1 << (x & 3);
	outport(0x3C4, (plane << 8) | 0x02);

	// calculate offset
	y <<= 4; offset += y;
	y <<= 1; offset += y;
	y <<= 2; offset += y;

	// write the pixel
	*(MK_FP(0xA000, offset)) = color;
}

int tw_getpixel(int x, int y)
{
	int offset = x >> 2;

	// select read plane based on X coordinate
	outport(0x3CE, ((x & 3) << 8) | 0x04);

	// calculate offset
	y <<= 4; offset += y;
	y <<= 1; offset += y;
	y <<= 2; offset += y;

	// read the pixel
	return *(MK_FP(0xA000, offset));
}

void tw_setpalette(char *pal)
{
	uint8_t *ptr = (uint8_t *)pal;
	int cnt = 768;

	outportb(0x3C8, 0);
	while (cnt--)
		outportb(0x3C9, *ptr++);
}

void tw_setstart(int s)
{
	cop_start = s;
}

void tw_waitvr()
{
	// TODO: is this timing sufficient?
	dis_waitb();
}

