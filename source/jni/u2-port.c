#include <jni.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android_native_app_glue.h>

#include "u2-port.h"

// Second Reality externed variables
char *hzpic;
char  font[31][1500];
int   frame_count;
char *cop_pal;
int   do_pal;
int   cop_start;
int   cop_scrl;
int   cop_dofade;
char *cop_fadepal;
char *fadepal;

// internal variables (for timing, etc)
static int last_frame_time;
static int dis_sync_val, dis_sync_time;

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
	unsigned char *framebuffer;
	unsigned char *vga_pal, *vga_mem[4];
	float texu, texv;
	GLuint texid;
};

static struct demo *the_demo = NULL;

// round value to next power of two (or return same if already a power of two)
static int nextPow2(int val)
{
	int next = 1;
	
	while (next < val)
		next <<= 1;
	
	return next;
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
	the_demo->framebuffer = (unsigned char *)malloc(size);
	memset(the_demo->framebuffer, 0, size);

	// allocate memory for the emulated VGA palette
	the_demo->vga_pal = (unsigned char *)malloc(768);
	memset(the_demo->vga_pal, 0, 768);

	// and the emulated VGA memory area (four planes at 64KB each)
	for (h = 0; h < 4; h++)
	{
		the_demo->vga_mem[h] = (unsigned char *)malloc(65536);
		memset(the_demo->vga_mem[h], 0, 65536);
	}

	return 0;
}

static void demo_set_video_mode(int width, int height)
{
	the_demo->texu = the_demo->tex_width  / (float)width;
	the_demo->texv = the_demo->tex_height / (float)height;
	LOGI("render: width=%d, height=%d", width, height);
}

// render the current frame in the display
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
	
	// update the GL texture
	glBindTexture(GL_TEXTURE_2D, the_demo->texid);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, the_demo->tex_width, the_demo->tex_height, GL_RGB, GL_UNSIGNED_BYTE, the_demo->framebuffer);

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

// the main entry point of a native application that uses android_native_app_glue.  it runs in its own thread, with its own event loop for receiving input events.
void android_main(struct android_app *state)
{
	struct demo demo;

	// make sure glue isn't stripped
	app_dummy();

	memset(&demo, 0, sizeof(demo));
	state->userData = &demo;
	state->onAppCmd = demo_handle_cmd;
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

	// execute each part
	alku_main();

	// end ourselves
	exit(0);
}

char *MK_FP(int seg, int off)
{
	// if segment points to VGA memory, then return a pointer to our emulated VGA buffer instead
	if (seg == 0xA000)
		return the_demo->vga_mem[0] + off;		// FIXME: only plane 0 for now, need to work with outport() and swap buffers

	// otherwise, return a valid pointer to memory
	return (char *)((seg << 16) + off);
}

void outport(unsigned short int port, unsigned char val)
{
}

int outline(char *f, char *t)
{
	return 0;
}

int ascrolltext(int scrl, int *dtau)
{
	return 0;
}

void dis_partstart()
{
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
		frame_count++;
		last_frame_time = now;

		// just to let part 1 continue...
		cop_dofade = 0;
	}

	demo_handle_events();
	return 0; // the_demo->exitflag;
}

int init_copper()
{
	return 0;
}

int close_copper()
{
	return 0;
}

void tw_opengraph()
{
	// set tweaked 640x400 mode
	demo_set_video_mode(640, 400);
}

void tw_putpixel(int x, int y, int color)
{
}

int tw_getpixel(int x, int y)
{
	return 0;
}

void tw_setpalette(void *pal)
{
}

