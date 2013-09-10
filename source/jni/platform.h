#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define LOGI(...)	platform_log('I', "SecondReality", __VA_ARGS__)
#define LOGW(...)	platform_log('W', "SecondReality", __VA_ARGS__)

typedef FILE		PFILE;

extern void demo_execute();

void	platform_log(char level, const char *tag, const char *fmt, ...);
int		platform_get_usec();

void	platform_set_video_mode(int width, int height, int stride);
uint8_t *platform_lock_framebuffer();
int		platform_get_framebuffer_stride();
void	platform_unlock_framebuffer(uint8_t *ptr);
void	platform_draw_frame();

int 	platform_handle_events();

PFILE *	platform_fopen(const char *fname, const char *mode);
size_t	platform_fread(void *buf, size_t size, size_t count, PFILE *fp);
int		platform_fseek(PFILE *fp, size_t offset, int origin);
size_t	platform_ftell(PFILE *fp);
int		platform_fclose(PFILE *fp);

#endif // PLATFORM_H
