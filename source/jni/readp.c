//
// ported from u2/{beg|end|endpic|jplogo}/readp.c
// note: u2/{start|techno}/readp.c may have some differences
//
#include <string.h>

struct st_readp
{
	short int	magic;
	short int	wid;
	short int	hig;
	short int	cols;
	short int	add;
};

void	readp(char *dest,int row,char *src)
{
	int	bytes,a,b;
	struct st_readp *hdr;
	char *end;

	hdr=(struct st_readp *)src;
	if(row==-1)
	{
		memcpy(dest,src+16,hdr->cols*3);
		return;
	}
	if(row>=hdr->hig) return;
	src+=hdr->add*16;
	while(row)
	{
		src+=*(short int *)src;
		src+=2;
		row--;
	}
	bytes=*(short int *)src;
	src+=2;

	end = src + bytes;
	while (src < end)
	{
		int8_t al = *src++;
		if (al < 0)
		{
			// al is signed
			uint8_t ah = al & 0x7F;
			al = *src++;
			while (ah--)
				*dest++ = al;
		}
		else
		{
			// l2
			*dest++ = al;
		}
	}
}
