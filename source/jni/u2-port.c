#include <jni.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "demo", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "demo", __VA_ARGS__))

char *hzpic;
char font[31][1500];
int frame_count;
char *cop_pal;
int do_pal;
int cop_start;
int cop_scrl;
int cop_dofade;
char *cop_fadepal;
char *fadepal;

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
	float texu, texv;
	GLuint texid;
};

struct demo *the_demo = NULL;

// round value to next power of two (or return same if already a power of two)
static int nextPow2(int val)
{
	int next = 1;
	
	while (next < val)
		next <<= 1;
	
	return next;
}

// get current time in milliseconds
static int getMsec()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (t.tv_sec * 1000) + (t.tv_usec / 1000);
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

	// execute each part
	alku_main();
}

char *MK_FP(int seg, int off)
{
	return 0;
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
//	LOGI("dis_sync");
	return 0;
}

int dis_exit()
{
//	LOGI("dis_exit");
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

