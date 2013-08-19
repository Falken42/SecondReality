#include <jni.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android_native_app_glue.h>
#include <android/asset_manager.h>

#include "u2-port.h"

// Second Reality externed variables
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

// internal variables (for timing, vga emulation, etc)
static int last_frame_time;
static int dis_sync_val, dis_sync_time;
static unsigned int vga_width, vga_height, vga_stride, vga_start;
static uint8_t vga_pal[768], *vga_plane[4], *vga_buffer;
static uint8_t vga_pal_index, vga_pal_comp, vga_adr_reg, vga_attr_reg, vga_cur_plane, vga_chain4, vga_horiz_pan;

// GL/Android data struct
struct demo
{
	struct android_app *app;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	volatile int initialized, exitflag;
	int disp_width, disp_height;
	int tex_width, tex_height;
	uint8_t *framebuffer;
	float texu, texv;
	GLuint texid;
};

static AAssetManager *android_asset_manager;
static struct demo *the_demo = NULL;

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

// get current time in microseconds
static int getUsec()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (t.tv_sec * 1000000) + t.tv_usec;
}

static int getUsecDiff(int time)
{
	return getUsec() - time;
}

// initialize an EGL context for the current display
static int demo_init_display()
{
	// initialize OpenGL ES and EGL
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
		EGL_BLUE_SIZE,		8,
		EGL_GREEN_SIZE,		8,
		EGL_RED_SIZE,		8,
		EGL_NONE
	};

	EGLint w, h, dummy, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(display, 0, 0);

	// pick the first EGLConfig that matches our criteria
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
	ANativeWindow_setBuffersGeometry(the_demo->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, the_demo->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
	{
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);
	LOGI("display: width=%d, height=%d", w, h);
	the_demo->disp_width  = w;
	the_demo->disp_height = h;
	
	// use a fixed size of 1024x1024 for the frame buffer texture
	the_demo->tex_width  = 1024;
	the_demo->tex_height = 1024;
	LOGI("texture: width=%d, height=%d", the_demo->tex_width, the_demo->tex_height);

	the_demo->display = display;
	the_demo->context = context;
	the_demo->surface = surface;

	// initialize GL state
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	
	// set orthographic projection and view matrices to use fullscreen pixel coordinates
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0, w, h, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// generate and initialize a texture to display the rendered frame
	glGenTextures(1, &the_demo->texid);
	glBindTexture(GL_TEXTURE_2D, the_demo->texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, the_demo->tex_width, the_demo->tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	
	// allocate memory for the output RGB texture
	const int size = the_demo->tex_width * the_demo->tex_height * 3;
	the_demo->framebuffer = (uint8_t *)malloc(size);
	memset(the_demo->framebuffer, 0, size);

	// allocate the emulated direct access VGA memory area
	vga_buffer = (uint8_t *)malloc(65536);
	memset(vga_buffer, 0, 65536);

	// allocate space for all four VGA memory planes
	for (h = 0; h < 4; h++)
	{
		vga_plane[h] = (uint8_t *)malloc(65536);
		memset(vga_plane[h], 0, 65536);
	}

	// init some VGA register defaults
	vga_start	  = 0;
	vga_cur_plane = 0;
	vga_chain4	  = 0x08;
	vga_horiz_pan = 0;
	vga_attr_reg  = 0;
	return 0;
}

static void demo_set_video_mode(int width, int height, int stride)
{
	vga_width  = width;
	vga_height = height;
	vga_stride = stride;

	the_demo->texu = vga_width  / (float)the_demo->tex_width;
	the_demo->texv = vga_height / (float)the_demo->tex_height;
	LOGI("render: width=%d, height=%d", vga_width, vga_height);
}

// show the current framebuffer on the display
static void demo_draw_frame()
{
	const GLfloat verts[] = {
		the_demo->disp_width,	0.0f,
		0.0f,					0.0f,
		the_demo->disp_width,	the_demo->disp_height,
		0.0f,					the_demo->disp_height
	};
	
	const GLfloat texcoords[] = {
		the_demo->texu,			0.0f,
		0.0f,					0.0f,
		the_demo->texu,			the_demo->texv,
		0.0f,					the_demo->texv
	};
	
	if (the_demo->display == NULL)
		return;

	// clear the display to black
	glClearColor(0.0f, 0.0f, 0.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// update the GL texture (optimize the texture upload by only sending up to the visible height)
	glBindTexture(GL_TEXTURE_2D, the_demo->texid);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, the_demo->tex_width, vga_height, GL_RGB, GL_UNSIGNED_BYTE, the_demo->framebuffer);

	// setup rendering
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// render the polygon
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	// and show the rendered surface to the display
	eglSwapBuffers(the_demo->display, the_demo->surface);
}

static void demo_render_frame()
{
	int w, h;

	if (vga_chain4)
	{
		// mode 13h, convert linear VRAM to 24-bit RGB
		const uint8_t *src = vga_plane[0];
		for (h = 0; h < vga_height; h++)
		{
			uint8_t *dest = the_demo->framebuffer + (h * the_demo->tex_width * 3);
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
			uint8_t *dest = the_demo->framebuffer + (h * the_demo->tex_width * 3);

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
	demo_draw_frame();
}

// tear down the EGL context currently associated with the display
static void demo_term_display()
{
	if (the_demo->display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(the_demo->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (the_demo->context != EGL_NO_CONTEXT)
			eglDestroyContext(the_demo->display, the_demo->context);

		if (the_demo->surface != EGL_NO_SURFACE)
			eglDestroySurface(the_demo->display, the_demo->surface);

		eglTerminate(the_demo->display);
	}

	the_demo->display  = EGL_NO_DISPLAY;
	the_demo->context  = EGL_NO_CONTEXT;
	the_demo->surface  = EGL_NO_SURFACE;
	the_demo->exitflag = 1;
}

// process the next main command
static void demo_handle_cmd(struct android_app *app, int32_t cmd)
{
    struct demo *demo = (struct demo *)app->userData;

	switch (cmd)
	{
		case APP_CMD_INIT_WINDOW:
			// the window is being shown, so initialize it
			if (demo->app->window != NULL)
			{
				demo_init_display();
				demo_draw_frame();
				demo->initialized = 1;
			}
			break;

		case APP_CMD_TERM_WINDOW:
			// the window is being hidden or closed, clean it up
			demo_term_display();
			break;

		case APP_CMD_LOST_FOCUS:
			// our app lost focus, quit the demo
			demo->exitflag = 1;
			break;
	}
}

static void demo_handle_events()
{
	int ident, events;
	struct android_poll_source *source;

	// read and process all pending events
	while ((ident = ALooper_pollAll(0, NULL, &events, (void **)&source)) >= 0)
	{
		struct android_app *state = the_demo->app;

		// process this event
		if (source != NULL)
			source->process(state, source);

		// check if we are exiting
		if (state->destroyRequested != 0)
		{
			demo_term_display();
			return;
		}
	}
}

// android asset fopen technique from: http://www.50ply.com/blog/2013/01/19/loading-compressed-android-assets-with-file-pointer/
static int android_fread(void *cookie, char *buf, int size)
{
	return AAsset_read((AAsset *)cookie, buf, size);
}

static int android_fwrite(void *cookie, const char *buf, int size)
{
	return EACCES;
}

static fpos_t android_fseek(void *cookie, fpos_t offset, int whence)
{
	return AAsset_seek((AAsset *)cookie, offset, whence);
}

static int android_fclose(void *cookie)
{
	AAsset_close((AAsset *)cookie);
	return 0;
}

static FILE *android_fopen(const char *fname, const char *mode)
{
	if (mode[0] == 'w')
		return NULL;

	AAsset *asset = AAssetManager_open(android_asset_manager, fname, 0);
	if (!asset)
		return NULL;

	return funopen(asset, android_fread, android_fwrite, android_fseek, android_fclose);
}

static void *demo_load_assetsz(const char *fname, int *size)
{
	FILE *fp = android_fopen(fname, "rb");
	if (fp == NULL)
	{
		LOGW("load_asset: failed to open [%s]!", fname);
		return NULL;
	}

	// get size of asset
	fseek(fp, 0, SEEK_END);
	*size = ftell(fp);
	rewind(fp);

	// allocate memory and load
	LOGI("load_asset: loading [%s] (%d bytes)...", fname, *size);
	void *res = malloc(*size);
	fread(res, 1, *size, fp);
	fclose(fp);
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
	char *inc = demo_load_assetsz(fname, &asset_size);
	if (inc == NULL)
		return NULL;

	char *src = inc;
	char *end = inc + asset_size;

	uint8_t *res  = NULL;
	uint8_t *dest = NULL;
	int total     = 0;

	// parse the assembly file
	for (;;)
	{
		// skip over spaces, tabs, crlfs, or the letters 'd' and 'b'
		while ((src < end) && ((*src == ' ') || (*src == '\t') || (*src == '\r') || (*src == '\n') || (*src == 'd') || (*src == 'b')))
			src++;

		if (src >= end)
			break;

		// advance until we reach a non-integer character
		char *next = src;
		while ((next < end) && ((*next == '-') || isdigit(*next)))
			next++;

		*next = 0;

		// check if we need to expand our destination buffer
		const int dist = dest - res;
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

// the main entry point of a native application that uses android_native_app_glue.  it runs in its own thread, with its own event loop for receiving input events.
void android_main(struct android_app *state)
{
	struct demo demo;

	// make sure glue isn't stripped
	app_dummy();

	memset(&demo, 0, sizeof(demo));
	state->userData = &demo;
	state->onAppCmd = demo_handle_cmd;
	android_asset_manager = state->activity->assetManager;
	demo.app = state;
	the_demo = &demo;

	// handle events until the window is initialized
	while (!the_demo->initialized)
		demo_handle_events();

	// initialize internal state
	frame_count     = 0;
	last_frame_time = getUsec();
	dis_sync_val    = 0;
	dis_sync_time   = last_frame_time;

#if 0
	// test standard mode 13h rendering
	int t;
	demo_set_video_mode(320, 200, 320);

	// setup palette: 64 entries each of reds, greens, blues, and greyscale
	outportb(0x3C8, 0);
	for (t = 0; t < 64; t++) { outportb(0x3C9, t); outportb(0x3C9, 0); outportb(0x3C9, 0); }
	for (t = 0; t < 64; t++) { outportb(0x3C9, 0); outportb(0x3C9, t); outportb(0x3C9, 0); }
	for (t = 0; t < 64; t++) { outportb(0x3C9, 0); outportb(0x3C9, 0); outportb(0x3C9, t); }
	for (t = 0; t < 64; t++) { outportb(0x3C9, t); outportb(0x3C9, t); outportb(0x3C9, t); }

	// draw pixels to the VRAM area
	char *vmem = MK_FP(0xA000, 0);
	for (t = 0; t < 320 * 200; t++)
		*vmem++ = (uint8_t)(t >> 4);

	// show it for 10 seconds
	while (frame_count < 600 && !dis_exit());
#elif 0
	// test modex rendering
	int t;
	tw_opengraph();

	// setup palette: just black and white
	outportb(0x3C8, 0); outportb(0x3C9, 0);    outportb(0x3C9, 0);    outportb(0x3C9, 0);
	outportb(0x3C8, 1); outportb(0x3C9, 0x3F); outportb(0x3C9, 0x3F); outportb(0x3C9, 0x3F);

	// draw a line from 0,0 to 200,200
	for (t = 0; t < 200; t++)
		tw_putpixel(t, t, 1);

	// show it for 10 seconds
	while (frame_count < 600 && !dis_exit());
#else
	int size;
	void *tmp;

	// load assets
	tmp = demo_load_asminc("fona.inc");
	memcpy(font, tmp, sizeof(font));
	free(tmp);

	tmp = demo_load_asmincsz("hoi.in0", &size);
	tmp = demo_append_asminc(tmp, "hoi.in1", &size);
	memcpy(hzpic, tmp, size);
	free(tmp);

	// execute each part
	alku_main();
#endif

	// end ourselves
	exit(0);
}

char *MK_FP(int seg, int off)
{
	// if segment points to VGA memory, then return a pointer to our emulated VGA buffer instead
	if (seg == 0xA000)
		return vga_buffer + off;

	// otherwise, return a valid pointer to memory
	return (char *)((seg << 16) + off);
}

static void vga_set_plane(uint8_t mask)
{
	// multiple plane writes currently not supported
	if (bitcount(mask) > 1)
		LOGI("vga_set_plane: VGA plane change to multiple planes [mask=0x%02X]", mask);

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

		case 0x3C8:
			// set palette color index, starting with red component
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

// from alku/asmyt.asm
void outline(char *src, char *dest)
{
	int mrol = 0x08, cnt, ccc;

	for (cnt = 4; cnt > 0; cnt--)
	{
		outport(0x3C4, (mrol << 8) | 0x02);

		const char *si = src + cnt - 1;
		char *di = dest;
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

void ascrolltext(int scrl, int *text)
{
}

void dis_partstart()
{
	LOGI("---------- starting next part ----------");
}

int dis_sync()
{
//	LOGI("dis_sync = %d", dis_sync_val);

	// COMPLETE HACK (for now): increment sync every 7.5 seconds
	const int now = getUsec();
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

	// render a frame every 16.666 msec (60Hz vblank)
	const int now = getUsec();
	if ((now - last_frame_time) >= 16666)
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
			cop_dofade--;
			cop_pal = fadepal;
			do_pal = 1;

			const uint16_t *src = (const uint16_t *)cop_fadepal;
			uint8_t *dest = fadepal;
			int ccc, cnt = 768 / 16;

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

	demo_handle_events();
	return the_demo->exitflag;
}

void init_copper()
{
}

void close_copper()
{
}

void tw_opengraph()
{
	// turn off chain-4
	outport(0x3C4, 0x0604);

	// 320x372 visible, with 704 pixel stride (176*4)
	demo_set_video_mode(320, 372, 704);
}

void tw_putpixel(int x, int y, int color)
{
	// select write plane based on X coordinate
	const int plane = 1 << (x & 3);
	outport(0x3C4, (plane << 8) | 0x02);

	// calculate offset
	int offset = x >> 2;
	y <<= 4; offset += y;
	y <<= 1; offset += y;
	y <<= 2; offset += y;

	// write the pixel
	*(MK_FP(0xA000, offset)) = color;
//	LOGI("tw_putpixel(%d, %d) = 0x%02X (%d)", x, y >> 7, color, color);
}

int tw_getpixel(int x, int y)
{
	// select read plane based on X coordinate
	outport(0x3CE, ((x & 3) << 8) | 0x04);

	// calculate offset
	int offset = x >> 2;
	y <<= 4; offset += y;
	y <<= 1; offset += y;
	y <<= 2; offset += y;

	// read the pixel
	const int col = *(MK_FP(0xA000, offset));
//	LOGI("tw_getpixel(%d, %d) = 0x%02X (%d)", x, y >> 7, col, col);

	// FIXME: text fade in breaks on last frame when returing proper color
	return col;
}

void tw_setpalette(void *pal)
{
	uint8_t *ptr = (uint8_t *)pal;
	int cnt = 768;

	outportb(0x3C8, 0);
	while (cnt--)
		outportb(0x3C9, *ptr++);
}

