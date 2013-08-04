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

struct demo
{
	struct android_app *app;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int rendering;
	int width, height;
	unsigned char *framebuffer;
	float texu, texv;
	GLuint texid;
};

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
static int demo_init_display(struct demo *demo)
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
	ANativeWindow_setBuffersGeometry(demo->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, demo->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
	{
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);
	LOGI("display: width=%d, height=%d", w, h);
	demo->width  = w;
	demo->height = h;
	
	// calculate appropriate texture size and UV coordinates
	int texwidth  = nextPow2(demo->width);
	int texheight = nextPow2(demo->height);
	demo->texu = demo->width  / (float)texwidth;
	demo->texv = demo->height / (float)texheight;
	LOGI("texture: width=%d, height=%d", texwidth, texheight);

	demo->display = display;
	demo->context = context;
	demo->surface = surface;

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
	glGenTextures(1, &demo->texid);
	glBindTexture(GL_TEXTURE_2D, demo->texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwidth, texheight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	
	// allocate memory for the output RGB texture
	const int size = demo->width * demo->height * 3;
	demo->framebuffer = (unsigned char *)malloc(size);
	memset(demo->framebuffer, 0, size);
	
	// initialize rendering state
	demo->rendering = 1;
	return 0;
}

// render the current frame in the display
static void demo_draw_frame(struct demo *demo)
{
	const GLfloat verts[] = {
		demo->width,	0.0f,
		0.0f,			0.0f,
		demo->width,	demo->height,
		0.0f,			demo->height
	};
	
	const GLfloat texcoords[] = {
		demo->texu,		0.0f,
		0.0f,			0.0f,
		demo->texu,		demo->texv,
		0.0f,			demo->texv
	};
	
	if (demo->display == NULL)
		return;

	// clear the display to black
	glClearColor(0.0f, 0.0f, 0.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// update the GL texture
	glBindTexture(GL_TEXTURE_2D, demo->texid);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, demo->width, demo->height, GL_RGB, GL_UNSIGNED_BYTE, demo->framebuffer);

	// setup rendering
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// render the polygon
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	// and show the rendered surface to the display
	eglSwapBuffers(demo->display, demo->surface);
}

// tear down the EGL context currently associated with the display
static void demo_term_display(struct demo *demo)
{
	if (demo->display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(demo->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (demo->context != EGL_NO_CONTEXT)
			eglDestroyContext(demo->display, demo->context);

		if (demo->surface != EGL_NO_SURFACE)
			eglDestroySurface(demo->display, demo->surface);

		eglTerminate(demo->display);
	}

	demo->rendering = 0;
	demo->display = EGL_NO_DISPLAY;
	demo->context = EGL_NO_CONTEXT;
	demo->surface = EGL_NO_SURFACE;
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
				demo_init_display(demo);
				demo_draw_frame(demo);
			}
			break;

		case APP_CMD_TERM_WINDOW:
			// the window is being hidden or closed, clean it up
			demo_term_display(demo);
			break;

		case APP_CMD_GAINED_FOCUS:
			// our app gained focus, so restart rendering
			demo->rendering = 1;
			break;

		case APP_CMD_LOST_FOCUS:
			// our app lost focus, stop rendering
			demo->rendering = 0;
			demo_draw_frame(demo);
			break;
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

	// loop waiting for stuff to do
	while (1)
	{
		// read all pending events
		int ident;
		int events;
		struct android_poll_source *source;

		// if not rendering, we will block forever waiting for events.
		// if we are rendering, we loop until all events are read, then continue to draw the next frame of animation.
		while ((ident = ALooper_pollAll(demo.rendering ? 0 : -1, NULL, &events, (void **)&source)) >= 0)
		{
			// process this event
			if (source != NULL)
				source->process(state, source);

			// check if we are exiting
			if (state->destroyRequested != 0)
			{
				demo_term_display(&demo);
				return;
			}
		}

		// done with events, so render next frame
		if (demo.rendering)
			demo_draw_frame(&demo);
	}
}
