#include <stdarg.h>
#include <stddef.h>
#include <windows.h>
#include <tchar.h>
#include "platform.h"

struct Me
{
	HWND hwnd;
	LARGE_INTEGER initial_perf_counter;
	LARGE_INTEGER perf_frequency;
	int framebuffer_width_in_pixels;
	int framebuffer_height_in_pixels;
	uint8_t framebuffer[0x100000]; /* RGB */
	uint8_t bitmap[0x100000]; /* BGR, each scan line is aligned on a DWORD boundary */
};

static struct Me me;

static void logv(char pri, const char *tag, const char *fmt, va_list args)
{
	char a[256], b[256];
	vsprintf_s(a, sizeof(a), fmt, args);
	sprintf_s(b, sizeof(b), "[%c][%s]%s\n", pri, tag, a);
	OutputDebugStringA(b);
}

void platform_log(char level, const char *tag, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	logv(level, tag, fmt, args);
	va_end(args);
}

int platform_get_usec(void)
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (int)((now.QuadPart - me.initial_perf_counter.QuadPart) * 1000000 / me.perf_frequency.QuadPart);
}

void platform_set_video_mode(int width, int height, int stride)
{
	me.framebuffer_width_in_pixels = width;
	me.framebuffer_height_in_pixels = height;
}

uint8_t *platform_lock_framebuffer(void)
{
	return me.framebuffer;
}

int platform_get_framebuffer_stride(void)
{
	return me.framebuffer_width_in_pixels;
}

void platform_unlock_framebuffer(uint8_t *ptr)
{
	const int width_in_bytes = 3 * me.framebuffer_width_in_pixels;
	const int padding_bytes_per_row = ((width_in_bytes + 3) & ~3) - width_in_bytes;
	uint8_t *dest = me.bitmap;
	int rows = me.framebuffer_height_in_pixels;
	while (rows-- > 0) {
		const uint8_t *const end = ptr + width_in_bytes;
		for (; ptr < end; ptr += 3) {
			*dest++ = ptr[2];
			*dest++ = ptr[1];
			*dest++ = ptr[0];
		}
		dest += padding_bytes_per_row;
	}
}

void platform_draw_frame(void)
{
	InvalidateRect(me.hwnd, NULL, FALSE);
}

int platform_handle_events(void)
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		DispatchMessage(&msg);
	return 0;
}

PFILE *platform_fopen(const char *fname, const char *mode)
{
	PFILE *fp;
	return fopen_s(&fp, fname, mode) ? NULL : fp;
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
	return (size_t)ftell(fp);
}

int platform_fclose(PFILE *fp)
{
	return fclose(fp);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_PAINT) {
		const BITMAPINFOHEADER bmih = {
			sizeof(BITMAPINFOHEADER),
			me.framebuffer_width_in_pixels, -me.framebuffer_height_in_pixels, 1, 24, BI_RGB
		};
		RECT rc;
		PAINTSTRUCT ps;
		GetClientRect(hwnd, &rc);
		StretchDIBits(
			BeginPaint(hwnd, &ps),
			0, 0, rc.right, rc.bottom, /* TODO: letterboxing */
			0, 0, me.framebuffer_width_in_pixels, me.framebuffer_height_in_pixels, me.bitmap,
			(const BITMAPINFO *)&bmih, DIB_RGB_COLORS, SRCCOPY);
		EndPaint(hwnd, &ps);
		return 0;
	}
	if (msg == WM_DESTROY)
		ExitProcess(0);
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

static LPCTSTR register_class(HINSTANCE hinst, LPCTSTR class_name)
{
	WNDCLASS wc = {0};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = window_proc;
	wc.hInstance = hinst;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = class_name;
	return (LPCTSTR)(INT_PTR)RegisterClass(&wc);
}

static void show_window(HWND hwnd, int width, int height)
{
	int x, y, cx, cy;
	RECT rc = {0, 0, width, height};
	AdjustWindowRect(&rc, GetWindowLong(hwnd, GWL_STYLE), FALSE);
	cx = rc.right - rc.left;
	cy = rc.bottom - rc.top;
	x = (GetSystemMetrics(SM_CXSCREEN) - cx) / 2;
	y = (GetSystemMetrics(SM_CYSCREEN) - cy) / 2;
	SetWindowPos(me.hwnd, HWND_NOTOPMOST, x, y, cx, cy, SWP_SHOWWINDOW);
	UpdateWindow(me.hwnd);
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE dont_care, LPSTR doont_care, int dooont_care)
{
	static LPCTSTR name = _T("Second Reality");
	me.hwnd = CreateWindow(register_class(hinst, name), name, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, hinst, NULL);
	if (me.hwnd) {
		show_window(me.hwnd, 640, 480);
		QueryPerformanceCounter(&me.initial_perf_counter);
		QueryPerformanceFrequency(&me.perf_frequency);
		demo_execute();
	}
	return 0;
}
