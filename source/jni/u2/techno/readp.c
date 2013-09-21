struct st_readp
{
	int16_t	magic;
	int16_t	wid;
	int16_t	hig;
	int16_t	cols;
	int16_t	add;
};

static void	readp(char *dest,int row,char *src)
{
	int	bytes,a,b;
	struct st_readp *hdr;
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
		src+=*(int16_t *)src;
		src+=2;
		row--;
		if(*(uint16_t *)&src[0]>=0x4000)
		{
			*(uint16_t *)&src[0]-=0x4000;
			*(uint16_t *)&src[2]+=0x400;
		}
	}
	bytes=*(int16_t *)src;
	src+=2;
	{
		const char *const cx=src+bytes;
		while(src<cx)
			if(*(const int8_t *)src>=0)
				*dest++=*++src;
			else
			{
				char ah=*src++&0x7f;
				while(ah--)
					*dest++=*src;
				src++;
			}
	}
}
