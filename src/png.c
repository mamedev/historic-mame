/*********************************************************************

  png.c

  PNG reading functions.

  07/15/1998 Created by Mathis Rosenhauer
  10/02/1998 Code clean up and abstraction by Mike Balfour
             and Mathis Rosenhauer
  10/15/1998 Image filtering. MLR
  11/09/1998 Bit depths 1-8 MLR
  11/10/1998 Some additional PNG chunks recognized MLR

  TODO : Fully comply with the "Recommendations for Decoders"
         of the W3C

*********************************************************************/

#include "driver.h"
#include "inflate.h"
#include "png.h"

extern unsigned int crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);

#define PNG_Signature		"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"
#define PNG_MaxChunkLength	0x7FFFFFFFL

#define PNG_CN_IHDR 0x49484452L 	/* Chunk names */
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

#define PNG_PF_None 	0	/* Prediction filters */
#define PNG_PF_Sub		1
#define PNG_PF_Up		2
#define PNG_PF_Average	3
#define PNG_PF_Paeth	4

int use_artwork;

/* read_uint is here so we don't have to deal with byte-ordering issues */
static UINT32 read_uint (void *fp)
{
	UINT8 v[4];
	UINT32 i;

	if (osd_fread(fp, v, 4) != 4)
		if (errorlog)
			fprintf(errorlog,"Unexpected EOF in PNG\n");
	i = (v[0]<<24) | (v[1]<<16) | (v[2]<<8) | (v[3]);
	return i;
}

/* convert_uint is here so we don't have to deal with byte-ordering issues */
static UINT32 convert_uint (UINT8 *v)
{
	UINT32 i;

	i = (v[0]<<24) | (v[1]<<16) | (v[2]<<8) | (v[3]);
	return i;
}

int png_unfilter(struct png_info *p)
{
	UINT32 i, j, bpp, filter;
	INT32 prediction, pA, pB, pC, dA, dB, dC;
	UINT8 *src, *dst;

	if((p->image = (UINT8 *)malloc (p->height*p->rowbytes))==NULL)
	{
		if (errorlog)
			fprintf(errorlog,"Out of memory\n");
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
			memcpy (dst, src, p->rowbytes);
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
					if (errorlog)
						fprintf(errorlog,"Unknown filter type %i\n",filter);
					prediction = 0;
				}
				*dst = 0xff & (*src + prediction);
				dst++; src++;
			}
	}

	return 1;
}

int png_verify_signature (void *fp)
{
	INT8 signature[8];

	if (osd_fread (fp, signature, 8) != 8)
	{
		if (errorlog)
			fprintf(errorlog,"Unable to read PNG signature (EOF)\n");
		return 0;
	}

	if (memcmp(signature, PNG_Signature, 8))
	{
		if (errorlog)
			fprintf(errorlog,"PNG signature mismatch found: %s expected: %s\n",
				signature,PNG_Signature);
		return 0;
	}
	return 1;
}

int png_inflate_image (struct png_info *p)
{
	/* translates color_type to bytes per pixel */
	const int samples[] = {1, 0, 3, 1, 2, 0, 4};

	p->bpp = (samples[p->color_type]*p->bit_depth)/8;
	p->rowbytes=(p->width*p->bit_depth*samples[p->color_type])/8
		 + ((p->width*p->bit_depth*samples[p->color_type])%8?1:0);

	if((p->fimage = (UINT8 *)malloc (p->height*(p->rowbytes+2)))==NULL)
	{
		if (errorlog)
			fprintf(errorlog,"Out of memory\n");
		free (p->zimage);
		return 0;
	}

	/* Note: We have to strip the first 2 bytes (Compression method/flags code and
	   additional flags/check bits) and the last 4 bytes (check value) to
	   make inflate_memory happy. */
	if ( inflate_memory(p->zimage+2, p->zlength-6, p->fimage, p->height*(p->rowbytes+1) ) )
	{
		if (errorlog)
			fprintf(errorlog,"Error inflating compressed PNG data\n");
		free (p->zimage);
		free (p->fimage);
		return 0;
	}
	free (p->zimage);
	return 1;
}

int png_read_chunks(void *fp, struct png_info *p)
{
	UINT32 i;
	UINT32 chunk_length, chunk_type=0, chunk_crc, crc;
	UINT32 tot_idat=0, num_idat=0, l_idat[256];
	UINT8 *chunk_data, *zimage, *idat[256], *temp;
	UINT8 str_chunk_type[5];

	while (chunk_type != PNG_CN_IEND)
	{
		chunk_length=read_uint(fp);
		if (osd_fread(fp, str_chunk_type, 4) != 4)
			if (errorlog)
				fprintf(errorlog,"Unexpected EOF in PNG file\n");

		str_chunk_type[4]=0; /* terminate string */

		crc=crc32(0,str_chunk_type, 4);
		chunk_type = convert_uint(str_chunk_type);

		if (chunk_length)
		{
			if ((chunk_data = (UINT8 *)malloc(chunk_length+1))==NULL)
			{
				if (errorlog)
					fprintf(errorlog,"Out of memory\n");
				return 0;
			}
			if (osd_fread (fp, chunk_data, chunk_length) != chunk_length)
			{
				if (errorlog)
					fprintf(errorlog,"Unexpected EOF in PNG file\n");
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
			if (errorlog)
			{
				fprintf(errorlog,"CRC check failed while reading PNG chunk %s\n",
					str_chunk_type);
				fprintf(errorlog,"Found: %08X  Expected: %08X\n",crc,chunk_crc);
			}
			return 0;
		}

		if (errorlog)
			fprintf(errorlog,"Reading PNG chunk %s\n", str_chunk_type);

		switch (chunk_type)
		{
		case PNG_CN_IHDR:
			p->width = convert_uint(chunk_data);
			p->height = convert_uint(chunk_data+4);
			p->bit_depth = *(chunk_data+8);
			p->color_type = *(chunk_data+9);
			p->compression_method = *(chunk_data+10);
			p->filter_method = *(chunk_data+11);
			p->interlace_method = *(chunk_data+12);
			free (chunk_data);

			if (errorlog)
			{
				fprintf(errorlog,"PNG IHDR information:\n");
				fprintf(errorlog,"Width: %i, Height: %i\n", p->width, p->height);
				fprintf(errorlog,"Bit depth %i, color type: %i\n", p->bit_depth, p->color_type);
				fprintf(errorlog,"Compression method: %i, filter: %i, interlace: %i\n",
					p->compression_method, p->filter_method, p->interlace_method);
			}
			break;

		case PNG_CN_PLTE:
			p->num_palette=chunk_length/3;
			p->palette=chunk_data;
			if (errorlog)
				fprintf(errorlog,"%i palette entries\n", p->num_palette);
			break;

		case PNG_CN_tRNS:
			p->num_trans=chunk_length;
			p->trans=chunk_data;
			if (errorlog)
				fprintf(errorlog,"%i transparent palette entries\n", p->num_trans);
			break;

		case PNG_CN_IDAT:
			idat[num_idat]=chunk_data;
			l_idat[num_idat]=chunk_length;
			num_idat++;
			tot_idat += chunk_length;
			break;

		case PNG_CN_tEXt:
			if (errorlog)
			{
				UINT8 *text=chunk_data;

				while(*text++);
				chunk_data[chunk_length]=0;
				fprintf(errorlog, "Keyword: %s\n", chunk_data);
				fprintf(errorlog, "Text: %s\n", text);
			}
			free(chunk_data);
			break;

		case PNG_CN_tIME:
			if (errorlog)
			{
				UINT8 *t=chunk_data;
				fprintf(errorlog, "Image last-modification time: %i/%i/%i (%i:%i:%i) GMT\n",
					((short)(*t+1) << 8)+ (short)(*(t+1)), *(t+2), *(t+3), *(t+4), *(t+5), *(t+6));
				/* (*t+1) is _very_ strange probably a bug */
			}

			free(chunk_data);
			break;

		case PNG_CN_gAMA:
			p->source_gamma  = convert_uint(chunk_data)/100000.0;
			if (errorlog)
				fprintf(errorlog, "Source gamma: %f\n",p->source_gamma);

			free(chunk_data);
			break;

		case PNG_CN_pHYs:
			p->xres = convert_uint(chunk_data);
			p->yres = convert_uint(chunk_data+4);
			p->resolution_unit = *(chunk_data+8);
			if (errorlog)
			{
				fprintf(errorlog, "Pixel per unit, X axis: %i\n",p->xres);
				fprintf(errorlog, "Pixel per unit, Y axis: %i\n",p->yres);
				if (p->resolution_unit)
					fprintf(errorlog, "Unit is meter\n");
				else
					fprintf(errorlog, "Unit is unknown\n");
			}
			free(chunk_data);
			break;

		case PNG_CN_IEND:
			break;

		default:
			if (errorlog)
				fprintf (errorlog, "Ignoring unsupported chunk name %s\n",str_chunk_type);
			if (chunk_data)
				free(chunk_data);
			break;
		}
	}
	if ((zimage = (UINT8 *)malloc(tot_idat))==NULL)
	{
		if (errorlog)
			fprintf(errorlog,"Out of memory\n");
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
	p->trans = NULL;
	p->palette = NULL;

	if (!(png = osd_fopen(Machine->gamedrv->name, file_name, OSD_FILETYPE_ARTWORK, 0)))
	{
		if (errorlog)
			fprintf(errorlog,"Unable to open PNG %s\n", file_name);
		return 0;
	}

	if (png_verify_signature(png)==0)
	{
		osd_fclose(png);
		return 0;
	}

	if (png_read_chunks(png, p)==0)
	{
		osd_fclose(png);
		return 0;
	}
	osd_fclose(png);

	if (png_inflate_image(p)==0)
		return 0;

	if(png_unfilter (p)==0)
		return 0;
	free (p->fimage);

	return 1;
}

void png_expand_buffer (UINT8 *inbuf, UINT8 *outbuf, int width, int height, int bit_depth)
{
	int i,j, k;

	if (bit_depth==8)
		memcpy (outbuf, inbuf, width*height);
	else
	{
		for (i=0; i<height; i++)
		{
			for(j=0; j<width/(8/bit_depth); j++)
			{
				for (k=8/bit_depth-1; k>=0; k--)
					*outbuf++ = (*inbuf>>k*bit_depth) & (0xff >> (8-bit_depth));
				inbuf++;
			}
			if (width % (8/bit_depth))
			{
				for (k=width%(8/bit_depth)-1; k>=0; k--)
					*outbuf++ = (*inbuf>>k*bit_depth) & (0xff >> (8-bit_depth));
				inbuf++;
			}
		}
	}
}

static int png_read_bitmap(const char *file_name, struct osd_bitmap **bitmap, struct png_info *p)
{
	UINT8 *pixmap8, *tmp;
	UINT32 orientation, i;
	UINT32 j, x, y, h, w;

	if (!read_png(file_name, p))
		return 0;

	if (p->bit_depth > 8)
	{
		if (errorlog)
			fprintf(errorlog,"Unsupported bit depth %i (8 bit max.)\n", p->bit_depth);
		return 0;
	}

	if (p->color_type != 3)
	{
		if (errorlog)
			fprintf(errorlog,"Unsupported color type %i (has to be 3)\n", p->color_type);
		return 0;
	}
	if (p->interlace_method != 0)
	{
		if (errorlog)
			fprintf(errorlog,"Interlace unsupported\n");
		return 0;
	}

	if ((pixmap8 = (UINT8 *)malloc(p->width*p->height))==NULL)
	{
		if (errorlog)
			fprintf(errorlog,"Out of memory\n");
		free (p->fimage);
		return 0;
	}

	/* Convert to 8 bit */
	png_expand_buffer (p->image, pixmap8, p->width, p->height, p->bit_depth);
	free (p->image);

	if (!(*bitmap=osd_new_bitmap(p->width,p->height,8)))
	{
		if (errorlog)
			fprintf(errorlog,"Unable to allocate memory for artwork\n");
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

static void delete_unused_colors (struct png_info *p, struct osd_bitmap *bitmap)
{
	int i, j, tab[256], pen=0, trns=0;
	UINT8 ptemp[3*256], ttemp[256];

	memset (tab, 0, 256*sizeof(int));
	memcpy (ptemp, p->palette, 3*p->num_palette);
	memcpy (ttemp, p->trans, p->num_trans);

	/* check which colors are actually used */
	for ( j = 0; j < bitmap->height; j++)
		for (i = 0; i < bitmap->width; i++)
				tab[bitmap->line[j][i]]++;

	/* shrink palette and transparency */
	for (i = 0; i < p->num_palette; i++)
		if (tab[i])
		{
			p->palette[3*pen+0]=ptemp[3*i+0];
			p->palette[3*pen+1]=ptemp[3*i+1];
			p->palette[3*pen+2]=ptemp[3*i+2];
			if (i < p->num_trans)
			{
				p->trans[pen] = ttemp[i];
				trns++;
			}
			tab[i] = pen++;
		}

	/* remap colors */
	for ( j = 0; j < bitmap->height; j++)
		for (i = 0; i < bitmap->width; i++)
				bitmap->line[j][i]=tab[bitmap->line[j][i]];

	if (errorlog && (p->num_palette!=pen))
		fprintf (errorlog, "%i unused pen(s) deleted\n", p->num_palette-pen);

	p->num_palette = pen;
	p->num_trans = trns;
}

int png_read_artwork(const char *file_name, struct osd_bitmap **bitmap, UINT8 **palette, int *num_palette, UINT8 **trans, int *num_trans)
{
	struct png_info p;

	if (!png_read_bitmap(file_name, bitmap, &p))
		return 0;

	delete_unused_colors (&p, *bitmap);

	*num_palette = p.num_palette;
	*num_trans = p.num_trans;
	*palette = p.palette;
	*trans = p.trans;

	return 1;
}
