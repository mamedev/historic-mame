/*********************************************************************

  png.c

  PNG reading functions.

  07/15/1998 Created by Mathis Rosenhauer
  10/02/1998 Code clean up and abstraction by Mike Balfour
             and Mathis Rosenhauer

  TODO : Add support for other color types and bit depths

*********************************************************************/

#include "driver.h"
#include "inflate.h"

/* These are the only two functions that should be needed */
int png_read_artwork(const char *file_name, struct osd_bitmap **bitmap, unsigned char **palette, int *num_palette, unsigned char **trans, int *num_trans);

/* obsolete */
int png_read_backdrop(char *file_name, struct osd_bitmap **ovl_bitmap, unsigned char *palette, int *num_palette);


extern unsigned int crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);

#define PNG_Signature       "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"
#define PNG_MaxChunkLength  0x7FFFFFFFL

#define PNG_CN_IHDR 0x49484452L     /* Chunk names */
#define PNG_CN_PLTE 0x504C5445L
#define PNG_CN_IDAT 0x49444154L
#define PNG_CN_IEND 0x49454E44L
#define PNG_CN_gAMA 0x67414D41L
#define PNG_CN_sBIT 0x73424954L
#define PNG_CN_cHRM 0x6348524DL
#define PNG_CN_tRNS 0x74524E53L
#define PNG_CN_bKGD 0x624B4744L
#define PNG_CN_hIST 0x68495354L
#define PNG_CN_tEXt 0x74455874L
#define PNG_CN_zTXt 0x7A545874L
#define PNG_CN_pHYs 0x70485973L
#define PNG_CN_oFFs 0x6F464673L
#define PNG_CN_tIME 0x74494D45L
#define PNG_CN_sCAL 0x7343414CL

#define PNG_PF_None     0   /* Prediction filters */
#define PNG_PF_Sub      1
#define PNG_PF_Up       2
#define PNG_PF_Average  3
#define PNG_PF_Paeth    4

struct png_info {
	unsigned int width, height;
	unsigned int xoffset, yoffset;
	unsigned int xres, yres;
	double xscale, yscale;
	double source_gamma;
	unsigned int chromaticities[8];
	int resolution_unit, offset_unit, scale_unit;
	unsigned char bit_depth;
	int significant_bits[4];
	int background_color[4];
	unsigned char color_type;
	unsigned char compression_method;
	unsigned char filter_method;
	unsigned char interlace_method;
	unsigned int num_palette;
	unsigned char *palette;
	unsigned int num_trans;
	unsigned char *trans;
	unsigned char *image;

	/* The rest is private and should not be used
	 * by the public functions
	 */
	unsigned char bpp;
	unsigned int rowbytes;
	unsigned char *zimage;
	unsigned int zlength;
	unsigned char *fimage;
};

static void error (char *msg)
{
	if (errorlog)
		fprintf(errorlog,"PNG - Error/Warning reading file: %s\n", msg);
}

/* read_uint is here so we don't have to deal with byte-ordering issues */
static unsigned int read_uint (void *fp)
{
	unsigned char v[4];
	unsigned int i;

	if (osd_fread(fp, v, 4) != 4)
        error("unexpected EOF");
	i = (v[0]<<24) | (v[1]<<16) | (v[2]<<8) | (v[3]);
	return i;
}

/* convert_uint is here so we don't have to deal with byte-ordering issues */
static unsigned int convert_uint (unsigned char *v)
{
	unsigned int i;

	i = (v[0]<<24) | (v[1]<<16) | (v[2]<<8) | (v[3]);
	return i;
}

static int unfilter(struct png_info *p)
{
	unsigned int i, j, bpp, filter;
	int prediction, pA, pB, pC, dA, dB, dC;
	unsigned char *src, *dst;

	if((p->image = (unsigned char *)malloc (p->height*p->rowbytes))==NULL)
	{
		error("out of memory");
		free (p->fimage);
		return 0;
	}

	src = p->fimage;
	dst = p->image;
	bpp = p->bpp;

	for (i=0; i<p->height; i++)
	{
		filter = *src++;
		if (!filter)
		{
			memcpy (dst, src, p->width);
			src += p->rowbytes;
			dst += p->rowbytes;
		}
		else
			for (j=0; j<p->width; j++)
			{
				pA = (j<bpp) ? 0: *(dst - bpp);
				pB = (i<bpp) ? 0: *(dst - p->rowbytes);
				pC = ((j<bpp)||(i<bpp)) ? 0: *(dst - p->rowbytes - bpp);

				switch (filter)
				{
				case PNG_PF_Sub:
					prediction = pA;
					break;
				case PNG_PF_Up:
					prediction = pB;
					break;
				case PNG_PF_Average:
					prediction = ((pA + pB) / 2);
					break;
				case PNG_PF_Paeth:
					prediction = pA + pB - pC;
					dA = abs(prediction - pA);
					dB = abs(prediction - pB);
					dC = abs(prediction - pC);
					if (dA <= dB && dA <= dC) prediction = pA;
					else if (dB <= dC) prediction = pB;
					else prediction = pC;
					break;
				default:
					error ("unknown filter type");
					prediction = 0;
				}
				*dst = 0xff & (*src + prediction);
				dst++; src++;
			}
	}

	return 1;
}

static int verify_signature (void *fp)
{
	char signature[9];

	if (osd_fread (fp, signature, 8) != 8)
	{
		error("unable to read signature (EOF)");
		return 0;
	}

	signature[8]=0;

	if (strcmp(signature, PNG_Signature))
	{
		error("PNG signature mismatch");
		return 0;
	}
	return 1;
}

static int inflate_image (struct png_info *p)
{
	/* translates color_type to bytes per pixel */
	const int samples[] = {1, 0, 3, 1, 2, 0, 4};

	p->bpp = (samples[p->color_type]*p->bit_depth)/8;
	p->rowbytes=(p->width*p->bit_depth*samples[p->color_type])/8
		 + ((p->width*p->bit_depth*samples[p->color_type])%8?1:0);

	if((p->fimage = (unsigned char *)malloc (p->height*(p->rowbytes+2)))==NULL)
	{
		error("out of memory");
		free (p->zimage);
		return 0;
	}


	/* Note: We have to strip the first 2 bytes (Compression method/flags code and
	   additional flags/check bits) and the last 4 bytes (check value) to
	   make inflate_memory happy. */
	if ( inflate_memory(p->zimage+2, p->zlength-6, p->fimage, p->height*(p->rowbytes+1) ) )
	{
		error ("error inflating compressed data");
		free (p->zimage);
		free (p->fimage);
		return 0;
	}
	free (p->zimage);
	return 1;
}

static int read_chunks(void *fp, struct png_info *p)
{
	unsigned int IEND_read=0, i;
	unsigned int chunk_length, chunk_type, chunk_crc, crc;
	unsigned int tot_idat=0, num_idat=0, l_idat[256];
	unsigned char *chunk_data, *zimage, *idat[256], *temp;
	unsigned char str_chunk_type[4];

	while (!IEND_read)
	{
		chunk_length=read_uint(fp);
		if (osd_fread(fp, str_chunk_type, 4) != 4)
		    error("unexpected EOF");
		crc=crc32(0,str_chunk_type, 4);
		chunk_type = convert_uint(str_chunk_type);

		if (chunk_length)
		{
			if ((chunk_data = (unsigned char *)malloc(chunk_length))==NULL)
			{
				error("out of memory");
				return 0;
			}
			if (osd_fread (fp, chunk_data, chunk_length) != chunk_length)
			{
				error("unexpected EOF");
				free(chunk_data);
				return 0;
			}

			crc=crc32(crc,chunk_data, chunk_length);
		}
		else
			chunk_data = NULL;

		chunk_crc=read_uint(fp);
		if (crc != chunk_crc)
		{
			error ("CRC check failed");
			if (errorlog) fprintf(errorlog,"Found: %08X  Expected: %08X\n",crc,chunk_crc);
			return 0;
		}

		switch (chunk_type) {
		case PNG_CN_IHDR:
		{
			p->width = convert_uint(chunk_data);
			p->height = convert_uint(chunk_data+4);
			p->bit_depth = *(chunk_data+8);
			p->color_type = *(chunk_data+9);
			p->compression_method = *(chunk_data+10);
			p->filter_method = *(chunk_data+11);
			p->interlace_method = *(chunk_data+12);
			free (chunk_data);

			if ((p->bit_depth != 8) && (p->bit_depth != 4) && (p->bit_depth != 2))
			{
				error ("unsupported bit depth (has to be 4 or 8)");
				return 0;
			}

			if (p->color_type != 3)
			{
				error ("unsupported color type (has to be 3)");
				return 0;
			}
			if (p->interlace_method != 0)
			{
				error ("interlace unsupported");
				return 0;
			}
			break;
		}
		case PNG_CN_PLTE:
		{
			p->num_palette=chunk_length/3;
			p->palette=chunk_data;
			break;
		}
		case PNG_CN_tRNS:
		{
			p->num_trans=chunk_length;
			p->trans=chunk_data;
			break;
		}
		case PNG_CN_IDAT:
		{
			idat[num_idat]=chunk_data;
			l_idat[num_idat]=chunk_length;
			num_idat++;
			tot_idat += chunk_length;
			break;
		}
		case PNG_CN_IEND:
		{
			IEND_read=1;
			break;
		}
		default:
		{
			error ("Unknown chunk name");
			if (chunk_data)
				free(chunk_data);
			break;
		}
		}
	}
	if ((zimage = (unsigned char *)malloc(tot_idat))==NULL)
	{
		error("out of memory");
		return 0;
	}

	p->zlength = tot_idat;

	/* combine idat chunks to compressed image data */
	temp = zimage;
	for (i=0; i<num_idat; i++)
	{
		memcpy (temp, idat[i], l_idat[i]);
		free (idat[i]);
		temp += l_idat[i];
	}
	p->zimage=zimage;
	return 1;
}

static int read_png(const char *file_name, struct png_info *p)
{
	void *png;

	p->num_palette = 0;
	p->num_trans = 0;

#ifdef MESS
	if (!(png = osd_fopen(Machine->gamedrv->name, file_name, OSD_FILETYPE_IMAGE, 0)))
#else
	if (!(png = osd_fopen(Machine->gamedrv->name, file_name, OSD_FILETYPE_ARTWORK, 0)))
#endif
	{
		error("unable to open overlay");
		return 0;
	}

	if (verify_signature(png)==0)
	{
		osd_fclose(png);
		return 0;
	}

	if (read_chunks(png, p)==0)
	{
		osd_fclose(png);
		return 0;
	}
	osd_fclose(png);

	if (inflate_image(p)==0)
		return 0;

	if(unfilter (p)==0)
		return 0;
	free (p->fimage);

	return 1;
}

static void expand_buffer (unsigned char *inbuf, unsigned char *outbuf, int width, int height, int bit_depth)
{
	int i,j, k;

	switch (bit_depth)
	{
	case 2: /* expand 2 bit to 8 bit */
		for (i=0; i<height; i++)
		{
			for(j=0; j<(width/4); j++)
			{
				for (k=3; k>=0; k--)
					*outbuf++ = (*inbuf>>(k*2))&0x03;
				inbuf++;
			}
			if (width % 4)
				for (k=((width%4)-1); k>=0; k--)
					*outbuf++ = (*inbuf>>(k*2))&0x03;
		}
		break;
	case 4: /* expand 4 bit to 8 bit */
		for (i=0; i<height; i++)
		{
			for(j=0; j<(width/2); j++)
			{
				*outbuf++=*inbuf>>4;
				*outbuf++=*inbuf++ & 0x0f;
			}
			if (width % 2)
				*outbuf++=*inbuf++>>4;
		}
		break;
	case 8: /* we have already 8 bit so just copy */
		memcpy (outbuf, inbuf, width*height);
		break;
	default:
		error ("Unsupported bit depth");
		break;
	}
}

static int png_read_bitmap(const char *file_name, struct osd_bitmap **bitmap, struct png_info *p)
{
	unsigned char *pixmap8, *tmp;
	unsigned int orientation, i;
	unsigned int j, x, y, h, w;

	if (!read_png(file_name, p))
		return 0;

	if ((pixmap8 = (unsigned char *)malloc(p->width*p->height))==NULL)
	{
		error("out of memory");
		free (p->fimage);
		return 0;
	}

	/* Convert to 8 bit */
	expand_buffer (p->image, pixmap8, p->width, p->height, p->bit_depth);
	free (p->image);

	if (!(*bitmap=osd_new_bitmap(p->width,p->height,8)))
	{
		error("unable to allocate memory for overlay");
		free (pixmap8);
		return 0;
	}

	orientation = Machine->orientation;
	if (orientation != ORIENTATION_DEFAULT)
	{
		tmp = pixmap8;
		for (i=0; i<p->height; i++)
			for (j=0; j<p->width; j++)
			{
				if (orientation & ORIENTATION_SWAP_XY)
				{
					x=i; y=j; w=p->height; h=p->width;
				}
				else
				{
					x=j; y=i; w=p->width; h=p->height;
				}
				if (orientation & ORIENTATION_FLIP_X)
					x=w-x-1;
				if (orientation & ORIENTATION_FLIP_Y)
					y=h-y-1;

				(*bitmap)->line[y][x] = *tmp++;
			}
	}
	else
	{
		for (i=0; i<p->height; i++)
			memcpy ((*bitmap)->line[i], pixmap8+i*p->width, p->width);
	}
	free (pixmap8);
	return 1;
}

int png_read_artwork(const char *file_name, struct osd_bitmap **bitmap, unsigned char **palette, int *num_palette, unsigned char **trans, int *num_trans)
{
	struct png_info p;

	if (!png_read_bitmap(file_name, bitmap, &p))
		return 0;

	*num_palette = p.num_palette;
	*num_trans = p.num_trans;
	*palette = p.palette;
	*trans = p.trans;

	return 1;
}

int png_read_backdrop(char *file_name, struct osd_bitmap **ovl_bitmap, unsigned char *palette, int *num_palette)
{
	unsigned char *trans, *ovl_palette;
	int num_trans;

	if (!png_read_artwork(file_name, ovl_bitmap, &ovl_palette, num_palette, &trans, &num_trans))
		return 0;
	memcpy (palette, ovl_palette, 3*(*num_palette));
	return 1;
}
