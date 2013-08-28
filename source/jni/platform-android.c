#include <jni.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android_native_app_glue.h>
#include <android/asset_manager.h>

#include "platform.h"

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
	int vid_height;
	uint8_t *framebuffer;
	float texu, texv;
	GLuint texid;
};

static AAssetManager *android_asset_manager;
static struct demo *the_demo = NULL;

int platform_get_usec()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (t.tv_sec * 1000000) + t.tv_usec;
}

// initialize an EGL context for the current display
static int android_init_display()
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
	
	// use a fixed size of 512x512 for the frame buffer texture
	the_demo->tex_width  = 512;
	the_demo->tex_height = 512;
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
	return 0;
}

void platform_set_video_mode(int width, int height, int stride)
{
	the_demo->vid_height = height;
	the_demo->texu = width  / (float)the_demo->tex_width;
	the_demo->texv = height / (float)the_demo->tex_height;
	LOGI("render: width=%d, height=%d", width, height);
}

uint8_t *platform_lock_framebuffer()
{
	return the_demo->framebuffer;
}

int platform_get_framebuffer_stride()
{
	return the_demo->tex_width;
}

void platform_unlock_framebuffer(uint8_t *ptr)
{
	// nothing to do
}

// show the current framebuffer on the display
void platform_draw_frame()
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
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, the_demo->tex_width, the_demo->vid_height, GL_RGB, GL_UNSIGNED_BYTE, the_demo->framebuffer);

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
static void android_term_display()
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
static void android_handle_cmd(struct android_app *app, int32_t cmd)
{
    struct demo *demo = (struct demo *)app->userData;

	switch (cmd)
	{
		case APP_CMD_INIT_WINDOW:
			// the window is being shown, so initialize it
			if (demo->app->window != NULL)
			{
				android_init_display();
				platform_draw_frame();
				demo->initialized = 1;
			}
			break;

		case APP_CMD_TERM_WINDOW:
			// the window is being hidden or closed, clean it up
			android_term_display();
			break;

		case APP_CMD_LOST_FOCUS:
			// our app lost focus, quit the demo
			the_demo->exitflag = 1;
			break;
	}
}

int platform_handle_events()
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
			android_term_display();
			break;
		}
	}

	return the_demo->exitflag;
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

PFILE *platform_fopen(const char *fname, const char *mode)
{
	if (mode[0] == 'w')
		return NULL;

	AAsset *asset = AAssetManager_open(android_asset_manager, fname, 0);
	if (!asset)
		return NULL;

	return funopen(asset, android_fread, android_fwrite, android_fseek, android_fclose);
}

size_t platform_fread(void *buf, size_t size, size_t count, PFILE *fp)
{
	return fread(buf, size, count, fp);
}

int platform_fseek(PFILE *fp, size_t offset, int origin)
{
	return fseek(fp, offset, origin);
}

size_t platform_ftell(PFILE *fp)
{
	return ftell(fp);
}

int platform_fclose(PFILE *fp)
{
	return fclose(fp);
}

// the main entry point of a native application that uses android_native_app_glue.  it runs in its own thread, with its own event loop for receiving input events.
void android_main(struct android_app *state)
{
	struct demo demo;

	// make sure glue isn't stripped
	app_dummy();

	memset(&demo, 0, sizeof(demo));
	state->userData = &demo;
	state->onAppCmd = android_handle_cmd;
	android_asset_manager = state->activity->assetManager;
	demo.app = state;
	the_demo = &demo;

	// handle events until the window is initialized
	while (!the_demo->initialized)
		platform_handle_events();

	// run the demo
	demo_execute();

	// end ourselves
	exit(0);
}

