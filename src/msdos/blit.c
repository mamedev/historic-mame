#define __INLINE__ static __inline__	/* keep allegro.h happy */
#include <allegro.h>
#undef __INLINE__
#include "driver.h"
#include <sys/farptr.h>
#include <go32.h>
#include "dirty.h"

extern char *dirty_old;
extern char *dirty_new;

extern struct osd_bitmap *scrbitmap;
extern int gfx_xoffset;
extern int gfx_yoffset;
extern int gfx_display_lines;
extern int gfx_display_columns;
extern int gfx_width;
extern int skiplines;
extern int skipcolumns;

unsigned int doublepixel[256];
unsigned int quadpixel[256]; /* for quadring pixels */


void blitscreen_dirty1_vga(void)
{
	int width4, x, y, columns4;
	unsigned long *lb, address;

	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	columns4 = gfx_display_columns/4;
	address = 0xa0000 + gfx_xoffset + gfx_yoffset * gfx_width;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
			{
				unsigned long *lb0 = lb + x / 4;
				unsigned long address0 = address + x;
				int h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
                    w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
				{
					_dosmemputl(lb0, w/4, address0);
					lb0 += width4;
					address0 += gfx_width;
				}
			}
			x += w;
        }
		lb += 16 * width4;
		address += 16 * gfx_width;
	}
}

void blitscreen_dirty0_vga(void)
{
	int width4,y,columns4;
	unsigned long *lb, address;

	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	columns4 = gfx_display_columns/4;
	address = 0xa0000 + gfx_xoffset + gfx_yoffset * gfx_width;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
	for (y = 0; y < gfx_display_lines; y++)
	{
		_dosmemputl(lb,columns4,address);
		lb+=width4;
		address+=gfx_width;
	}
}




INLINE void copyline_1x_8bpp(unsigned char *src,short seg,unsigned long address,int width4)
{
	short src_seg;

	src_seg = _my_ds();

	_movedatal(src_seg,(unsigned long)src,seg,address,width4);
}

INLINE void copyline_1x_16bpp(unsigned char *src,short seg,unsigned long address,int width2)
{
	short src_seg;

	src_seg = _my_ds();

	_movedatal(src_seg,(unsigned long)src,seg,address,width2);
}

#if 1 /* use the C approach instead */
INLINE void copyline_2x_8bpp(unsigned char *src,short seg,unsigned long address,int width4)
{
	__asm__ __volatile__ (
	"pushw %%es              \n"
	"movw %%dx, %%es         \n"
	"cld                     \n"
	".align 4                \n"
	"0:                      \n"
	"lodsl                   \n"
	"movl %%eax, %%ebx       \n"
	"bswap %%eax             \n"
	"xchgw %%ax,%%bx         \n"
	"roll $8, %%eax          \n"
	"stosl                   \n"
	"movl %%ebx, %%eax       \n"
	"rorl $8, %%eax          \n"
	"stosl                   \n"
	"loop 0b                 \n"
	"popw %%ax               \n"
	"movw %%ax, %%es         \n"
	::
	"d" (seg),
	"c" (width4),
	"S" (src),
	"D" (address):
	"ax", "bx", "cx", "dx", "si", "di", "cc", "memory");
}
#else
INLINE void copyline_2x_8bpp(unsigned char *src,short seg,unsigned long address,int width4)
{
	int i;

	/* set up selector */
	_farsetsel(seg);

	for (i = width; i > 0; i--)
	{
		_farnspokel (address, doublepixel[*src] | (doublepixel[*(src+1)] << 16));
		_farnspokel (address+4, doublepixel[*(src+2)] | (doublepixel[*(src+3)] << 16));
		address+=8;
		src+=4;
	}
}
#endif

INLINE void copyline_2x_16bpp(unsigned char *src,short seg,unsigned long address,int width2)
{
	__asm__ __volatile__ (
	"pushw %%es              \n"
	"movw %%dx, %%es         \n"
	"cld                     \n"
	".align 4                \n"
	"0:                      \n"
	"lodsl                   \n"
	"movl %%eax, %%ebx       \n"
	"roll $16, %%eax         \n"
	"xchgw %%ax,%%bx         \n"
	"stosl                   \n"
	"movl %%ebx, %%eax       \n"
	"stosl                   \n"
	"loop 0b                 \n"
	"popw %%ax               \n"
	"movw %%ax, %%es         \n"
	::
	"d" (seg),
	"c" (width2),
	"S" (src),
	"D" (address):
	"ax", "bx", "cx", "dx", "si", "di", "cc", "memory");
}

INLINE void copyline_3x_8bpp(unsigned char *src,short seg,unsigned long address,int width4)
{
	int i;

	/* set up selector */
	_farsetsel(seg);

	for (i = width4; i > 0; i--)
	{
		_farnspokel (address, (quadpixel[*src] & 0x00ffffff) | (quadpixel[*(src+1)] & 0xff000000));
		_farnspokel (address+4, (quadpixel[*(src+1)] & 0x0000ffff) | (quadpixel[*(src+2)] & 0xffff0000));
		_farnspokel (address+8, (quadpixel[*(src+2)] & 0x000000ff) | (quadpixel[*(src+3)] & 0xffffff00));
		address+=3*4;
		src+=4;
	}
}

INLINE void copyline_4x_8bpp(unsigned char *src,short seg,unsigned long address,int width4)
{
	int i;

	/* set up selector */
	_farsetsel(seg);

	for (i = width4; i > 0; i--)
	{
		_farnspokel (address, quadpixel[*src]);
		_farnspokel (address+4, quadpixel[*(src+1)]);
		_farnspokel (address+8, quadpixel[*(src+2)]);
		_farnspokel (address+12, quadpixel[*(src+3)]);
		address+=16;
		src+=4;
	}
}



#define DIRTY1(BPP) \
	short dest_seg; \
	int x,y,vesa_line,line_offs,xoffs; \
	unsigned char *lb; \
	unsigned long address; \
	dest_seg = screen->seg; \
	vesa_line = gfx_yoffset; \
	line_offs = scrbitmap->line[1] - scrbitmap->line[0]; \
	xoffs = (BPP/8)*gfx_xoffset; \
	lb = scrbitmap->line[skiplines] + (BPP/8)*skipcolumns; \

#define DIRTY1_NXNS(MX,MY,BPP) \
	for (y = 0;y < gfx_display_lines;y += 16) \
	{ \
		for (x = 0;x < gfx_display_columns; /* */) \
		{ \
			int w = 16; \
			if (ISDIRTY(x,y)) \
            { \
				unsigned char *src = lb + (BPP/8)*x; \
                int vesa_line0 = vesa_line, h; \
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y)) \
                    w += 16; \
				if (x + w > gfx_display_columns) \
					w = gfx_display_columns - x; \
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++) \
                { \
					address = bmp_write_line(screen,vesa_line0) + xoffs + MX*(BPP/8)*x; \
					copyline_##MX##x_##BPP##bpp(src,dest_seg,address,w/(4/(BPP/8))); \
					vesa_line0 += MY; \
					src += line_offs; \
				} \
			} \
			x += w; \
		} \
		vesa_line += MY*16; \
		lb += 16 * line_offs; \
	}

#define DIRTY1_NX2(MX,BPP) \
	for (y = 0;y < gfx_display_lines;y += 16) \
	{ \
		for (x = 0;x < gfx_display_columns; /* */) \
		{ \
			int w = 16; \
			if (ISDIRTY(x,y)) \
            { \
				unsigned char *src = lb + (BPP/8)*x; \
                int vesa_line0 = vesa_line, h; \
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y)) \
                    w += 16; \
				if (x + w > gfx_display_columns) \
					w = gfx_display_columns - x; \
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++) \
                { \
					address = bmp_write_line(screen,vesa_line0) + xoffs + MX*(BPP/8)*x; \
					copyline_##MX##x_##BPP##bpp(src,dest_seg,address,w/(4/(BPP/8))); \
					address = bmp_write_line(screen,vesa_line0+1) + xoffs + MX*(BPP/8)*x; \
					copyline_##MX##x_##BPP##bpp(src,dest_seg,address,w/(4/(BPP/8))); \
					vesa_line0 += 2; \
					src += line_offs; \
				} \
			} \
			x += w; \
		} \
		vesa_line += 16 * 2; \
		lb += 16 * line_offs; \
	}

#define DIRTY1_NX3(MX,BPP) \
	for (y = 0;y < gfx_display_lines;y += 16) \
	{ \
		for (x = 0;x < gfx_display_columns; /* */) \
		{ \
			int w = 16; \
			if (ISDIRTY(x,y)) \
            { \
				unsigned char *src = lb + (BPP/8)*x; \
                int vesa_line0 = vesa_line, h; \
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y)) \
                    w += 16; \
				if (x + w > gfx_display_columns) \
					w = gfx_display_columns - x; \
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++) \
                { \
					address = bmp_write_line(screen,vesa_line0) + xoffs + MX*(BPP/8)*x; \
					copyline_##MX##x_##BPP##bpp(src,dest_seg,address,w/(4/(BPP/8))); \
					address = bmp_write_line(screen,vesa_line0+1) + xoffs + MX*(BPP/8)*x; \
					copyline_##MX##x_##BPP##bpp(src,dest_seg,address,w/(4/(BPP/8))); \
					address = bmp_write_line(screen,vesa_line0+2) + xoffs + MX*(BPP/8)*x; \
					copyline_##MX##x_##BPP##bpp(src,dest_seg,address,w/(4/(BPP/8))); \
					vesa_line0 += 3; \
					src += line_offs; \
				} \
			} \
			x += w; \
		} \
		vesa_line += 16 * 3; \
		lb += 16 * line_offs; \
	}



#define DIRTY0(BPP) \
	short dest_seg; \
	int y,vesa_line,line_offs,xoffs,width; \
	unsigned char *src; \
	unsigned long address; \
	dest_seg = screen->seg; \
	vesa_line = gfx_yoffset; \
	line_offs = (scrbitmap->line[1] - scrbitmap->line[0]); \
	xoffs = (BPP/8)*gfx_xoffset; \
	width = gfx_display_columns/(4/(BPP/8)); \
	src = scrbitmap->line[skiplines] + (BPP/8)*skipcolumns; \

#define DIRTY0_NXNS(MX,MY,BPP) \
	for (y = 0;y < gfx_display_lines;y++) \
	{ \
		address = bmp_write_line(screen,vesa_line) + xoffs; \
		copyline_##MX##x_##BPP##bpp(src,dest_seg,address,width); \
		vesa_line += MY; \
		src += line_offs; \
	}

#define DIRTY0_NX2(MX,BPP) \
	for (y = 0;y < gfx_display_lines;y++) \
	{ \
		address = bmp_write_line(screen,vesa_line) + xoffs; \
		copyline_##MX##x_##BPP##bpp(src,dest_seg,address,width); \
		address = bmp_write_line(screen,vesa_line+1) + xoffs; \
		copyline_##MX##x_##BPP##bpp(src,dest_seg,address,width); \
		vesa_line += 2; \
		src += line_offs; \
	}

#define DIRTY0_NX3(MX,BPP) \
	for (y = 0;y < gfx_display_lines;y++) \
	{ \
		address = bmp_write_line(screen,vesa_line) + xoffs; \
		copyline_##MX##x_##BPP##bpp(src,dest_seg,address,width); \
		address = bmp_write_line(screen,vesa_line+1) + xoffs; \
		copyline_##MX##x_##BPP##bpp(src,dest_seg,address,width); \
		address = bmp_write_line(screen,vesa_line+2) + xoffs; \
		copyline_##MX##x_##BPP##bpp(src,dest_seg,address,width); \
		vesa_line += 3; \
		src += line_offs; \
	}



void blitscreen_dirty1_vesa_1x_1x_8bpp(void)   { DIRTY1(8)  DIRTY1_NXNS(1,1,8)  }
void blitscreen_dirty1_vesa_1x_2x_8bpp(void)   { DIRTY1(8)  DIRTY1_NX2 (1,8)  }
void blitscreen_dirty1_vesa_1x_2xs_8bpp(void)  { DIRTY1(8)  DIRTY1_NXNS(1,2,8)  }
void blitscreen_dirty1_vesa_2x_2x_8bpp(void)   { DIRTY1(8)  DIRTY1_NX2 (2,8)  }
void blitscreen_dirty1_vesa_2x_2xs_8bpp(void)  { DIRTY1(8)  DIRTY1_NXNS(2,2,8)  }
void blitscreen_dirty1_vesa_3x_2x_8bpp(void)   { DIRTY1(8)  DIRTY1_NX2 (3,8)  }
void blitscreen_dirty1_vesa_3x_2xs_8bpp(void)  { DIRTY1(8)  DIRTY1_NXNS(3,2,8)  }
void blitscreen_dirty1_vesa_4x_3x_8bpp(void)   { DIRTY1(8)  DIRTY1_NX3 (4,8)  }
void blitscreen_dirty1_vesa_4x_3xs_8bpp(void)  { DIRTY1(8)  DIRTY1_NXNS(4,3,8)  }

void blitscreen_dirty1_vesa_1x_1x_16bpp(void)  { DIRTY1(16) DIRTY1_NXNS(1,1,16) }
void blitscreen_dirty1_vesa_1x_2x_16bpp(void)  { DIRTY1(16) DIRTY1_NX2 (1,16) }
void blitscreen_dirty1_vesa_1x_2xs_16bpp(void) { DIRTY1(16) DIRTY1_NXNS(1,2,16) }
void blitscreen_dirty1_vesa_2x_2x_16bpp(void)  { DIRTY1(16) DIRTY1_NX2 (2,16) }
void blitscreen_dirty1_vesa_2x_2xs_16bpp(void) { DIRTY1(16) DIRTY1_NXNS(2,2,16) }

void blitscreen_dirty0_vesa_1x_1x_8bpp(void)   { DIRTY0(8)  DIRTY0_NXNS(1,1,8)  }
void blitscreen_dirty0_vesa_1x_2x_8bpp(void)   { DIRTY0(8)  DIRTY0_NX2 (1,8)  }
void blitscreen_dirty0_vesa_1x_2xs_8bpp(void)  { DIRTY0(8)  DIRTY0_NXNS(1,2,8)  }
void blitscreen_dirty0_vesa_2x_2x_8bpp(void)   { DIRTY0(8)  DIRTY0_NX2 (2,8)  }
void blitscreen_dirty0_vesa_2x_2xs_8bpp(void)  { DIRTY0(8)  DIRTY0_NXNS(2,2,8)  }
void blitscreen_dirty0_vesa_3x_2x_8bpp(void)   { DIRTY0(8)  DIRTY0_NX2 (3,8)  }
void blitscreen_dirty0_vesa_3x_2xs_8bpp(void)  { DIRTY0(8)  DIRTY0_NXNS(3,2,8)  }
void blitscreen_dirty0_vesa_4x_3x_8bpp(void)   { DIRTY0(8)  DIRTY0_NX3 (4,8)  }
void blitscreen_dirty0_vesa_4x_3xs_8bpp(void)  { DIRTY0(8)  DIRTY0_NXNS(4,3,8)  }

void blitscreen_dirty0_vesa_1x_1x_16bpp(void)  { DIRTY0(16) DIRTY0_NXNS(1,1,16) }
void blitscreen_dirty0_vesa_1x_2x_16bpp(void)  { DIRTY0(16) DIRTY0_NX2 (1,16) }
void blitscreen_dirty0_vesa_1x_2xs_16bpp(void) { DIRTY0(16) DIRTY0_NXNS(1,2,16) }
void blitscreen_dirty0_vesa_2x_2x_16bpp(void)  { DIRTY0(16) DIRTY0_NX2 (2,16) }
void blitscreen_dirty0_vesa_2x_2xs_16bpp(void) { DIRTY0(16) DIRTY0_NXNS(2,2,16) }
