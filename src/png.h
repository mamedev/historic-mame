
#ifndef MAME_PNG_H
#define MAME_PNG_H

/* PNG support */
struct png_info {
	UINT32 width, height;
	UINT32 xoffset, yoffset;
	UINT32 xres, yres;
	double xscale, yscale;
	double source_gamma;
	UINT32 chromaticities[8];
	UINT32 resolution_unit, offset_unit, scale_unit;
	UINT8 bit_depth;
	UINT32 significant_bits[4];
	UINT32 background_color[4];
	UINT8 color_type;
	UINT8 compression_method;
	UINT8 filter_method;
	UINT8 interlace_method;
	UINT32 num_palette;
	UINT8 *palette;
	UINT32 num_trans;
	UINT8 *trans;
	UINT8 *image;

	/* The rest is private and should not be used
	 * by the public functions
	 */
	UINT8 bpp;
	UINT32 rowbytes;
	UINT8 *zimage;
	UINT32 zlength;
	UINT8 *fimage;
};

extern int  png_read_artwork(const char *file_name, struct osd_bitmap **bitmap, UINT8 **palette, int *num_palette, UINT8 **trans, int *num_trans);

extern int  png_verify_signature (void *fp);
extern int  png_inflate_image (struct png_info *p);
extern int  png_read_chunks(void *fp, struct png_info *p);
extern void png_expand_buffer (UINT8 *inbuf, UINT8 *outbuf, int width, int height, int bit_depth);
extern int  png_unfilter(struct png_info *p);

#endif

