/*********************************************************************

  common.c

  Generic functions, mostly ROM and graphics related.

*********************************************************************/

#include "driver.h"
#include "dirent.h"

/* LBO */
#ifdef LSB_FIRST
#define intelLong(x) (x)
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x &
0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#endif


void showdisclaimer(void)   /* MAURY_BEGIN: dichiarazione */
{
	printf("MAME is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		 "several arcade machines. But hardware is useless without software, so an image\n"
		 "of the ROMs which run on that hardware is required. Such ROMs, like any other\n"
		 "commercial software, are copyrighted material and it is therefore illegal to\n"
		 "use them if you don't own the original arcade machine. Needless to say, ROMs\n"
		 "are not distributed together with MAME. Distribution of MAME together with ROM\n"
		 "images is a violation of copyright law and should be promptly reported to the\n"
		 "authors so that appropriate legal action can be taken.\n\n");
}                           /* MAURY_END: dichiarazione */

/***************************************************************************

  Read ROMs into memory.

  Arguments:
  const struct RomModule *romp - pointer to an array of Rommodule structures,
                                 as defined in common.h.

  const char *basename - Name of the directory where the files are
                         stored. The function also supports the
						 control sequence %s in file names: for example,
						 if the RomModule gives the name "%s.bar", and
						 the basename is "foo", the file "foo/foo.bar"
						 will be loaded.

***************************************************************************/
int readroms(const struct RomModule *rommodule,const char *basename)
{
	int region;
	const struct RomModule *romp;
	int checksumwarning;


	checksumwarning = 0;
	romp = rommodule;

	for (region = 0;region < MAX_MEMORY_REGIONS;region++)
		Machine->memory_region[region] = 0;

	region = 0;

	while (romp->name || romp->offset || romp->length)
	{
		unsigned int region_size;


		if (romp->name || romp->length)
		{
			printf("Error in RomModule definition: expecting ROM_REGION\n");
			goto getout;
		}

		region_size = romp->offset;
		if ((Machine->memory_region[region] = malloc(region_size)) == 0)
		{
			printf("Unable to allocate %d bytes of RAM\n",region_size);
			goto getout;
		}

		/* some games (i.e. Pleiades) want the memory clear on startup */
		memset(Machine->memory_region[region],0,region_size);

		romp++;

		while (romp->length)
		{
			FILE *f;
                        DIR *dirp;
                        struct dirent *dp;
			char buf[100];
			char name[100];
			int sum,xor,expchecksum;


			if (romp->name == 0)
			{
				printf("Error in RomModule definition: ROM_CONTINUE not preceded by ROM_LOAD\n");
				goto getout;
			}
			else if (romp->name == (char *)-1)
			{
				printf("Error in RomModule definition: ROM_RELOAD not preceded by ROM_LOAD\n");
				goto getout;
			}

			sprintf(buf,romp->name,basename);

                        /* jamc: code to perform a upper/lowercase rom search */
                        /* try to open directory */
                        if ( (dirp=opendir(basename)) == (DIR *)0) {
                           fprintf(stderr,"Unable to open ROM directory %s\n",basename);
                           goto printromlist;
                        }
                        /* search entry and upcasecompare to desired rom name */
                        for (dp=readdir(dirp); dp ; dp=readdir(dirp))
                            if (! strcasecmp(dp->d_name,buf)) break;
                        if ( dp ) sprintf( name,"%s/%s",basename,dp->d_name);
                        else      sprintf( name,"%s/%s",basename,buf);
                        closedir(dirp);

			/* sprintf(name,"%s/%s",basename,buf); */

			if ((f = fopen(name,"rb")) == 0)
			{
				fprintf(stderr, "Unable to open ROM %s\n",name);
				goto printromlist;
			}

			sum = 0;
			xor = 0;
			expchecksum = romp->checksum;

			do
			{
				unsigned char *c;
				unsigned int i;


				/* ROM_RELOAD */
				if (romp->name == (char *)-1)
				{
					fseek(f,0,SEEK_SET);
					/* reinitialize checksum counters as well */
					sum = 0;
					xor = 0;
				}

				if (romp->offset + romp->length > region_size)
				{
					printf("Error in RomModule definition: %s out of memory region space\n",name);
					fclose(f);
					goto getout;
				}

				if (fread(Machine->memory_region[region] + romp->offset,1,romp->length,f) != romp->length)
				{
					printf("Unable to read ROM %s\n",name);
					fclose(f);
					goto printromlist;
				}

				/* calculate the checksum */
				c = Machine->memory_region[region] + romp->offset;
				for (i = 0;i < romp->length;i+=2)
				{
					int j;


					j = 256 * c[i] + c[i+1];
					sum += j;
					xor ^= j;
				}

				romp++;
			} while (romp->length && (romp->name == 0 || romp->name == (char *)-1));

			sum = ((sum & 0xffff) << 16) | (xor & 0xffff);
			if (expchecksum != -1 && expchecksum != sum)
			{
				if (checksumwarning == 0)
					printf("The checksum of some ROMs does not match that of the ones MAME was tested with.\n"
							"WARNING: the game might not run correctly.\n"
							"Name         Expected  Found\n");
				checksumwarning++;
				printf("%-12s %08x %08x\n",buf,expchecksum,sum);
			}

			fclose(f);
		}

		region++;
	}

	if (checksumwarning > 0)
	{
		printf("Press return to continue\n");
		getchar();
	}

	return 0;


printromlist:
	printf("\n");                         /* MAURY_BEGIN: dichiarazione */
	showdisclaimer();
	printf("Press return to continue\n"); /* MAURY_END: dichiarazione */
	getchar();

	printromlist(rommodule,basename);


getout:
	for (region = 0;region < MAX_MEMORY_REGIONS;region++)
	{
		free(Machine->memory_region[region]);
		Machine->memory_region[region] = 0;
	}
	return 1;
}



void printromlist(const struct RomModule *romp,const char *basename)
{
	printf("This is the list of the ROMs required.\n"
			"All the ROMs must reside in a subdirectory called \"%s\".\n"
			"Name             Size\n",basename);
	while (romp->name || romp->offset || romp->length)
	{
		romp++;	/* skip memory region definition */

		while (romp->length)
		{
			char name[100];
			int length;


			sprintf(name,romp->name,basename);

			length = 0;

			do
			{
				/* ROM_RELOAD */
				if (romp->name == (char *)-1)
					length = 0;	/* restart */

				length += romp->length;

				romp++;
			} while (romp->length && (romp->name == 0 || romp->name == (char *)-1));

			printf("%-12s %5d bytes\n",name,length);
		}
	}
}



/***************************************************************************

  Read samples into memory.
  This function is different from readroms() because it doesn't fail if
  it doesn't find a file: it will load as many samples as it can find.

***************************************************************************/
struct GameSamples *readsamples(const char **samplenames,const char *basename)
{
	int i;
	struct GameSamples *samples;

	if (samplenames == 0 || samplenames[0] == 0) return 0;

	i = 0;
	while (samplenames[i] != 0) i++;

	if ((samples = malloc(sizeof(struct GameSamples) + (i-1)*sizeof(struct GameSample))) == 0)
		return 0;

	samples->total = i;
	for (i = 0;i < samples->total;i++)
		samples->sample[i] = 0;

	for (i = 0;i < samples->total;i++)
	{
		FILE *f;
		char buf[100];
		char name[100];


		if (samplenames[i][0])
		{
			sprintf(buf,samplenames[i],basename);
			sprintf(name,"%s/%s",basename,buf);

			if ((f = fopen(name,"rb")) != 0)
			{
				if (fseek(f,0,SEEK_END) == 0)
				{
                                        int dummy;
                                        unsigned char smpvol=0, smpres=0;
					unsigned smplen=0, smpfrq=0;

					fseek(f,0,SEEK_SET);
                                        fread(buf,1,4,f);
                                        if (memcmp(buf, "MAME", 4) == 0) {
                                           fread(&smplen,1,4,f);         /* all datas are LITTLE ENDIAN */
                                           fread(&smpfrq,1,4,f);
					   smplen = intelLong (smplen);  /* so convert them in the right endian-ness */
					   smpfrq = intelLong (smpfrq);
                                           fread(&smpres,1,1,f);
                                           fread(&smpvol,1,1,f);
                                           fread(&dummy,1,2,f);
					   if ((smplen != 0) && (samples->sample[i] = malloc(sizeof(struct GameSample) + (smplen)*sizeof(char))) != 0)
					   {
						   samples->sample[i]->length = smplen;
						   samples->sample[i]->volume = smpvol;
						   samples->sample[i]->smpfreq = smpfrq;
						   samples->sample[i]->resolution = smpres;
						   fread(samples->sample[i]->data,1,smplen,f);
					   }
                                        }
				}

				fclose(f);
			}
		}
	}

	return samples;
}



void freesamples(struct GameSamples *samples)
{
	int i;


	if (samples == 0) return;

	for (i = 0;i < samples->total;i++)
		free(samples->sample[i]);

	free(samples);
}



/***************************************************************************

  Function to convert the information stored in the graphic roms into a
  more usable format.

  Back in the early '80s, arcade machines didn't have the memory or the
  speed to handle bitmaps like we do today. They used "character maps",
  instead: they had one or more sets of characters (usually 8x8 pixels),
  which would be placed on the screen in order to form a picture. This was
  very fast: updating a character mapped display is, rougly speaking, 64
  times faster than updating an equivalent bitmap display, since you only
  modify groups of 8x8 pixels and not the single pixels. However, it was
  also much less versatile than a bitmap screen, since with only 256
  characters you had to do all of your graphics. To overcome this
  limitation, some special hardware graphics were used: "sprites". A sprite
  is essentially a bitmap, usually larger than a character, which can be
  placed anywhere on the screen (not limited to character boundaries) by
  just telling the hardware the coordinates. Moreover, sprites can be
  flipped along the major axis just by setting the appropriate bit (some
  machines can flip characters as well). This saves precious memory, since
  you need only one copy of the image instead of four.

  What about colors? Well, the early machines had a limited palette (let's
  say 16-32 colors) and each character or sprite could not use all of them
  at the same time. Characters and sprites data would use a limited amount
  of bits per pixel (typically 2, which allowed them to address only four
  different colors). You then needed some way to tell to the hardware which,
  among the available colors, were the four colors. This was done using a
  "color attribute", which typically was an index for a lookup table.

  OK, after this brief and incomplete introduction, let's come to the
  purpose of this section: how to interpret the data which is stored in
  the graphic roms. Unfortunately, there is no easy answer: it depends on
  the hardware. The easiest way to find how data is encoded, is to start by
  making a bit by bit dump of the rom. You will usually be able to
  immediately recognize some pattern: if you are lucky, you will see
  letters and numbers right away, otherwise you will see something which
  looks like letters and numbers, but with halves switched, dilated, or
  something like that. You'll then have to find a way to put it all
  together to obtain our standard one byte per pixel representation. Two
  things to remember:
  - keep in mind that every pixel has typically two (or more) bits
    associated with it, and they are not necessarily near to each other.
  - characters might be rotated 90 degrees. That's because many games used a
    tube rotated 90 degrees. Think how your monitor would look like if you
	put it on its side ;-)

  After you have successfully decoded the characters, you have to decode
  the sprites. This is usually more difficult, because sprites are larger,
  maybe have more colors, and are more difficult to recognize when they are
  messed up, since they are pure graphics without letters and numbers.
  However, with some work you'll hopefully be able to work them out as
  well. As a rule of thumb, the sprites should be encoded in a way not too
  dissimilar from the characters.

***************************************************************************/
int readbit(const unsigned char *src,int bitnum)
{
	int bit;


	bit = src[bitnum / 8] << (bitnum % 8);

	if (bit & 0x80) return 1;
	else return 0;
}



void decodechar(struct GfxElement *gfx,int num,const unsigned char *src,const struct GfxLayout *gl)
{
	int plane;


	for (plane = 0;plane < gl->planes;plane++)
	{
		int offs,y;


		offs = num * gl->charincrement + gl->planeoffset[plane];
		for (y = 0;y < gl->height;y++)
		{
			int x;


			for (x = 0;x < gl->width;x++)
			{
				unsigned char *dp;


				dp = gfx->gfxdata->line[num * gl->height + y];
				if (plane == 0) dp[x] = 0;
				else dp[x] <<= 1;

				dp[x] += readbit(src,offs + gl->yoffset[y] + gl->xoffset[x]);
			}
		}
	}
}



struct GfxElement *decodegfx(const unsigned char *src,const struct GfxLayout *gl)
{
	int c;
	struct osd_bitmap *bm;
	struct GfxElement *gfx;


	if ((bm = osd_create_bitmap(gl->width,gl->total * gl->height)) == 0)
		return 0;
	if ((gfx = malloc(sizeof(struct GfxElement))) == 0)
	{
		osd_free_bitmap(bm);
		return 0;
	}
	gfx->width = gl->width;
	gfx->height = gl->height;
	gfx->total_elements = gl->total;
	gfx->color_granularity = 1 << gl->planes;
	gfx->gfxdata = bm;

	for (c = 0;c < gl->total;c++)
		decodechar(gfx,c,src,gl);

	return gfx;
}



void freegfx(struct GfxElement *gfx)
{
	if (gfx)
	{
		osd_free_bitmap(gfx->gfxdata);
		free(gfx);
	}
}



/***************************************************************************

  Draw graphic elements in the specified bitmap.

  transparency == TRANSPARENCY_NONE - no transparency.
  transparency == TRANSPARENCY_PEN - bits whose _original_ value is == transparent_color
                                     are transparent. This is the most common kind of
									 transparency.
  transparency == TRANSPARENCY_COLOR - bits whose _remapped_ value is == Machine->pens[transparent_color]
                                     are transparent. This is used by e.g. Pac Man.
  transparency == TRANSPARENCY_THROUGH - if the _destination_ pixel is == Machine->pens[transparent_color],
                                     the source pixel is drawn over it. This is used by
									 e.g. Jr. Pac Man to draw the sprites when the background
									 has priority over them.

***************************************************************************/
void drawgfx(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color)
{
	int ox,oy,ex,ey,x,y,start,dy;
	const unsigned char *sd;
	unsigned char *bm;
	int col;


	if (!gfx) return;

	/* check bounds */
	ox = sx;
	oy = sy;
	ex = sx + gfx->width-1;
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	ey = sy + gfx->height-1;
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	/* start = (code % gfx->total_elements) * gfx->height; */
	if (flipy)	/* Y flop */
	{
		start = (code % gfx->total_elements) * gfx->height + gfx->height-1 - (sy-oy);
		dy = -1;
	}
	else		/* normal */
	{
		start = (code % gfx->total_elements) * gfx->height + (sy-oy);
		dy = 1;
	}


	/* if necessary, remap the transparent color */
	if (transparency == TRANSPARENCY_COLOR || transparency == TRANSPARENCY_THROUGH)
		transparent_color = Machine->pens[transparent_color];


	if (gfx->colortable)	/* remap colors */
	{
		const unsigned char *paldata;

		paldata = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)];

		switch (transparency)
		{
			case TRANSPARENCY_NONE:
				if (flipx)	/* X flip */
				{
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd = gfx->gfxdata->line[start] + gfx->width-1 - (sx-ox);
						for( x = sx ; x <= ex-7 ; x+=8 )
						{
							*(bm++) = paldata[*(sd--)];
							*(bm++) = paldata[*(sd--)];
							*(bm++) = paldata[*(sd--)];
							*(bm++) = paldata[*(sd--)];
							*(bm++) = paldata[*(sd--)];
							*(bm++) = paldata[*(sd--)];
							*(bm++) = paldata[*(sd--)];
							*(bm++) = paldata[*(sd--)];
							/* bm+=7; */
						}
						for( ; x <= ex ; x++)
							*(bm++) = paldata[*(sd--)];
						start+=dy;
					}
				}
				else		/* normal */
				{
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd = gfx->gfxdata->line[start] + (sx-ox);
						for( x = sx ; x <= ex-7 ; x+=8 )
						{
							*(bm++) = paldata[*(sd++)];
							*(bm++) = paldata[*(sd++)];
							*(bm++) = paldata[*(sd++)];
							*(bm++) = paldata[*(sd++)];
							*(bm++) = paldata[*(sd++)];
							*(bm++) = paldata[*(sd++)];
							*(bm++) = paldata[*(sd++)];
							*(bm++) = paldata[*(sd++)];
						}
						for( ; x <= ex ; x++)
							*(bm++) = paldata[*(sd++)];
						start+=dy;
					}
				}
				break;

			case TRANSPARENCY_PEN:
				if (flipx)	/* X flip */
				{
					int *sd4;
					int trans4,col4;

					trans4 = transparent_color * 0x01010101;
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd4 = (int *)(gfx->gfxdata->line[start] + gfx->width -1 - (sx-ox) -3);
						for (x = sx;x <= ex-3;x+=4)
						{
							if ((col4=*sd4) != trans4){
								col4 = intelLong (col4); /* LBO */
								col = (col4>>24)&0xff;
								if (col != transparent_color) bm[0] = paldata[col];
								col = (col4>>16)&0xff;
								if (col != transparent_color) bm[1] = paldata[col];
								col = (col4>>8)&0xff;
								if (col != transparent_color) bm[2] = paldata[col];
								col = col4&0xff;
								if (col != transparent_color) bm[3] = paldata[col];
							}
							bm+=4;
							sd4--;
						}
						sd = (unsigned char *)sd4+3;
						for (;x <= ex;x++)
						{
							col = *(sd--);
							if (col != transparent_color) *bm = paldata[col];
							bm++;
						}
						start+=dy;
					}
				}
				else		/* normal */
				{
					int *sd4;
					int trans4,col4;

					trans4 = transparent_color * 0x01010101;
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd4 = (int *)(gfx->gfxdata->line[start] + (sx-ox));
						for (x = sx;x <= ex-3;x+=4)
						{
							if ((col4=*sd4) != trans4){
								col4 = intelLong (col4); /* LBO */
								col = col4&0xff;
								if (col != transparent_color) bm[0] = paldata[col];
								col = (col4>>8)&0xff;
								if (col != transparent_color) bm[1] = paldata[col];
								col = (col4>>16)&0xff;
								if (col != transparent_color) bm[2] = paldata[col];
								col = (col4>>24)&0xff;
								if (col != transparent_color) bm[3] = paldata[col];
							}
							bm+=4;
							sd4++;
						}
						sd = (unsigned char *)sd4;
						for (;x <= ex;x++)
						{
							col = *(sd++);
							if (col != transparent_color) *bm = paldata[col];
							bm++;
						}
						start+=dy;
					}
				}
				break;

			case TRANSPARENCY_COLOR:
				if (flipx)	/* X flip */
				{
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd = gfx->gfxdata->line[start] + gfx->width-1 - (sx-ox);
						for (x = sx;x <= ex;x++)
						{
							col = paldata[*(sd--)];
							if (col != transparent_color) *bm = col;
							bm++;
						}
						start+=dy;
					}
				}
				else		/* normal */
				{
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd = gfx->gfxdata->line[start] + (sx-ox);
						for (x = sx;x <= ex;x++)
						{
							col = paldata[*(sd++)];
							if (col != transparent_color) *bm = col;
							bm++;
						}
						start+=dy;
					}
				}
				break;

			case TRANSPARENCY_THROUGH:
				if (flipx)	/* X flip */
				{
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd = gfx->gfxdata->line[start] + gfx->width-1 - (sx-ox);
						for (x = sx;x <= ex;x++)
						{
							if (*bm == transparent_color)
								*bm = paldata[*sd];
							bm++;
							sd--;
						}
						start+=dy;
					}
				}
				else		/* normal */
				{
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd = gfx->gfxdata->line[start] + (sx-ox);
						for (x = sx;x <= ex;x++)
						{
							if (*bm == transparent_color)
								*bm = paldata[*sd];
							bm++;
							sd++;
						}
						start+=dy;
					}
				}
				break;
		}
	}
	else
	{
		switch (transparency)
		{
			case TRANSPARENCY_NONE:		/* do a verbatim copy (faster) */
				if (flipx)	/* X flip */
				{
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd = gfx->gfxdata->line[start] + gfx->width-1 - (sx-ox);
						for( x = sx ; x <= ex-7 ; x+=8 )
						{
							*(bm++) = *(sd--);
							*(bm++) = *(sd--);
							*(bm++) = *(sd--);
							*(bm++) = *(sd--);
							*(bm++) = *(sd--);
							*(bm++) = *(sd--);
							*(bm++) = *(sd--);
							*(bm++) = *(sd--);
							/* bm+=7; */
						}
						for( ; x <= ex ; x++)
							*(bm++) = *(sd--);
						start+=dy;
					}
				}
				else		/* normal */
				{
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd = gfx->gfxdata->line[start] + (sx-ox);
						memcpy(bm,sd,ex-sx+1);
						start+=dy;
					}
				}
				break;

			case TRANSPARENCY_PEN:
			case TRANSPARENCY_COLOR:
				if (flipx)	/* X flip */
				{
					int *sd4;
					int trans4,col4;

					trans4 = transparent_color * 0x01010101;

					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd4 = (int *)(gfx->gfxdata->line[start] + gfx->width-1 - (sx-ox) - 3);
						for (x = sx;x <= ex-3;x+=4)
						{
							if( (col4=*sd4) != trans4 )
							{
								col4 = intelLong (col4); /* LBO */
								col = col4>>24;
								if (col != transparent_color) bm[0] = col;
								col = (col4>>16)&0xff;
								if (col != transparent_color) bm[1] = col;
								col = (col4>>8)&0xff;
								if (col != transparent_color) bm[2] = col;
								col = col4&0xff;
								if (col != transparent_color) bm[3] = col;
							}
							bm+=4;
							sd4--;
						}
						sd = (unsigned char *)sd4+3;
						for (;x <= ex;x++)
						{
							col = *(sd--);
							if (col != transparent_color) *bm = col;
							bm++;
						}
						start+=dy;
					}
				}
				else	/* normal */
				{
					int *sd4;
					int trans4,col4;
					int xod4;

					trans4 = transparent_color * 0x01010101;

					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd4 = (int *)(gfx->gfxdata->line[start] + (sx-ox));
						for (x = sx;x <= ex-3;x+=4)
						{
							if( (col4=*sd4) != trans4 )
							{
								xod4 = col4^trans4;
								if( (xod4&0x000000ff) && (xod4&0x0000ff00) &&
								    (xod4&0x00ff0000) && (xod4&0xff000000) )
											*(int *)bm = col4;
								else
								{
									col4 = intelLong (col4); /* LBO */
									xod4 = intelLong (xod4); /* LBO */
									if(xod4&0x000000ff) bm[0] = col4&0xff;
									if(xod4&0x0000ff00) bm[1] = (col4>>8)&0xff;
									if(xod4&0x00ff0000) bm[2] = (col4>>16)&0xff;
									if(xod4&0xff000000) bm[3] = (col4>>24);
								}
							}
							bm+=4;
							sd4++;
						}
						sd = (unsigned char *)sd4;
						for (;x <= ex;x++)
						{
							col = *(sd++);
							if (col != transparent_color) *bm = col;
							bm++;
						}
						start+=dy;
					}
				}
				break;

			case TRANSPARENCY_THROUGH:
				if (flipx)	/* X flip */
				{
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd = gfx->gfxdata->line[start] + gfx->width-1 - (sx-ox);
						for (x = sx;x <= ex;x++)
						{
							if (*bm == transparent_color)
								*bm = *sd;
							bm++;
							sd--;
						}
						start+=dy;
					}
				}
				else		/* normal */
				{
					for (y = sy;y <= ey;y++)
					{
						bm = dest->line[y] + sx;
						sd = gfx->gfxdata->line[start] + (sx-ox);
						for (x = sx;x <= ex;x++)
						{
							if (*bm == transparent_color)
								*bm = *sd;
							bm++;
							sd++;
						}
						start+=dy;
					}
				}
				break;
		}
	}
}



/***************************************************************************

  Use drawgfx() to copy a bitmap onto another at the given position.
  This function will very likely change in the future.

***************************************************************************/
void copybitmap(struct osd_bitmap *dest,struct osd_bitmap *src,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color)
{
	static struct GfxElement mygfx =
	{
		0,0,0,	/* filled in later */
		1,1,0,1
	};

	mygfx.width = src->width;
	mygfx.height = src->height;
	mygfx.gfxdata = src;
	drawgfx(dest,&mygfx,0,0,flipx,flipy,sx,sy,clip,transparency,transparent_color);
}



/***************************************************************************

  Copy a bitmap onto another with scroll and wraparound.
  This function supports multiple independently scrolling rows/columns.
  "rows" is the number of indepentently scrolling rows. "rowscroll" is an
  array of integers telling how much to scroll each row. Same thing for
  "cols" and "colscroll".
  If the bitmap cannot scroll in one direction, set rows or columns to 0.
  If the bitmap scrolls as a whole, set rows and/or cols to 1.
  Bidirectional scrolling is, of course, supported only if the bitmap
  scrolls as a whole in at least one direction.

***************************************************************************/
void copyscrollbitmap(struct osd_bitmap *dest,struct osd_bitmap *src,
		int rows,int *rowscroll,int cols,int *colscroll,
		const struct rectangle *clip,int transparency,int transparent_color)
{
	if (rows == 0)
	{
		/* scrolling columns */
		int col,colwidth;
		struct rectangle myclip;


		colwidth = src->width / cols;

		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		col = 0;
		while (col < cols)
		{
			int cons,scroll;


			/* count consecutive columns scrolled by the same amount */
			scroll = colscroll[col];
			cons = 1;
			while (col + cons < cols &&	colscroll[col + cons] == scroll)
				cons++;

			if (scroll < 0) scroll = src->height - (-scroll) % src->height;
			else scroll %= src->height;

			myclip.min_x = col * colwidth;
			if (myclip.min_x < clip->min_x) myclip.min_x = clip->min_x;
			myclip.max_x = (col + cons) * colwidth - 1;
			if (myclip.max_x > clip->max_x) myclip.max_x = clip->max_x;

			copybitmap(dest,src,0,0,0,scroll,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,0,scroll - src->height,&myclip,transparency,transparent_color);

			col += cons;
		}
	}
	else if (cols == 0)
	{
		/* scrolling rows */
		int row,rowheight;
		struct rectangle myclip;


		rowheight = src->height / rows;

		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;

		row = 0;
		while (row < rows)
		{
			int cons,scroll;


			/* count consecutive rows scrolled by the same amount */
			scroll = rowscroll[row];
			cons = 1;
			while (row + cons < rows &&	rowscroll[row + cons] == scroll)
				cons++;

			if (scroll < 0) scroll = src->width - (-scroll) % src->width;
			else scroll %= src->width;

			myclip.min_y = row * rowheight;
			if (myclip.min_y < clip->min_y) myclip.min_y = clip->min_y;
			myclip.max_y = (row + cons) * rowheight - 1;
			if (myclip.max_y > clip->max_y) myclip.max_y = clip->max_y;

			copybitmap(dest,src,0,0,scroll,0,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,scroll - src->width,0,&myclip,transparency,transparent_color);

			row += cons;
		}
	}
	else if (rows == 1 && cols == 1)
	{
		/* XY scrolling playfield */
		int scrollx,scrolly;


		if (rowscroll[0] < 0) scrollx = src->width - (-rowscroll[0]) % src->width;
		else scrollx = rowscroll[0] % src->width;

		if (colscroll[0] < 0) scrolly = src->height - (-colscroll[0]) % src->height;
		else scrolly = colscroll[0] % src->height;

		copybitmap(dest,src,0,0,scrollx,scrolly,clip,transparency,transparent_color);
		copybitmap(dest,src,0,0,scrollx,scrolly - src->height,clip,transparency,transparent_color);
		copybitmap(dest,src,0,0,scrollx - src->width,scrolly,clip,transparency,transparent_color);
		copybitmap(dest,src,0,0,scrollx - src->width,scrolly - src->height,clip,transparency,transparent_color);
	}
	else if (rows == 1)
	{
		/* scrolling columns + horizontal scroll */
		int col,colwidth;
		int scrollx;
		struct rectangle myclip;


		if (rowscroll[0] < 0) scrollx = src->width - (-rowscroll[0]) % src->width;
		else scrollx = rowscroll[0] % src->width;

		colwidth = src->width / cols;

		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		col = 0;
		while (col < cols)
		{
			int cons,scroll;


			/* count consecutive columns scrolled by the same amount */
			scroll = colscroll[col];
			cons = 1;
			while (col + cons < cols &&	colscroll[col + cons] == scroll)
				cons++;

			if (scroll < 0) scroll = src->height - (-scroll) % src->height;
			else scroll %= src->height;

			myclip.min_x = col * colwidth + scrollx;
			if (myclip.min_x < clip->min_x) myclip.min_x = clip->min_x;
			myclip.max_x = (col + cons) * colwidth - 1 + scrollx;
			if (myclip.max_x > clip->max_x) myclip.max_x = clip->max_x;

			copybitmap(dest,src,0,0,scrollx,scroll,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,scrollx,scroll - src->height,&myclip,transparency,transparent_color);

			myclip.min_x = col * colwidth + scrollx - src->width;
			if (myclip.min_x < clip->min_x) myclip.min_x = clip->min_x;
			myclip.max_x = (col + cons) * colwidth - 1 + scrollx - src->width;
			if (myclip.max_x > clip->max_x) myclip.max_x = clip->max_x;

			copybitmap(dest,src,0,0,scrollx - src->width,scroll,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,scrollx - src->width,scroll - src->height,&myclip,transparency,transparent_color);

			col += cons;
		}
	}
	else if (cols == 1)
	{
		/* scrolling rows + vertical scroll */
		int row,rowheight;
		int scrolly;
		struct rectangle myclip;


		if (colscroll[0] < 0) scrolly = src->height - (-colscroll[0]) % src->height;
		else scrolly = colscroll[0] % src->height;

		rowheight = src->height / rows;

		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;

		row = 0;
		while (row < rows)
		{
			int cons,scroll;


			/* count consecutive rows scrolled by the same amount */
			scroll = rowscroll[row];
			cons = 1;
			while (row + cons < rows &&	rowscroll[row + cons] == scroll)
				cons++;

			if (scroll < 0) scroll = src->width - (-scroll) % src->width;
			else scroll %= src->width;

			myclip.min_y = row * rowheight + scrolly;
			if (myclip.min_y < clip->min_y) myclip.min_y = clip->min_y;
			myclip.max_y = (row + cons) * rowheight - 1 + scrolly;
			if (myclip.max_y > clip->max_y) myclip.max_y = clip->max_y;

			copybitmap(dest,src,0,0,scroll,scrolly,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,scroll - src->width,scrolly,&myclip,transparency,transparent_color);

			myclip.min_y = row * rowheight + scrolly - src->height;
			if (myclip.min_y < clip->min_y) myclip.min_y = clip->min_y;
			myclip.max_y = (row + cons) * rowheight - 1 + scrolly - src->height;
			if (myclip.max_y > clip->max_y) myclip.max_y = clip->max_y;

			copybitmap(dest,src,0,0,scroll,scrolly - src->height,&myclip,transparency,transparent_color);
			copybitmap(dest,src,0,0,scroll - src->width,scrolly - src->height,&myclip,transparency,transparent_color);

			row += cons;
		}
	}
}



void clearbitmap(struct osd_bitmap *bitmap)
{
	int i;


	for (i = 0;i < bitmap->height;i++)
		memset(bitmap->line[i],Machine->pens[0],bitmap->width);
}
