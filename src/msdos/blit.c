#define __INLINE__ static __inline__	/* keep allegro.h happy */
#include <allegro.h>
#undef __INLINE__
#include "driver.h"
#include <sys/farptr.h>
#include <go32.h>
#include "dirty.h"

/* from video.c (required for 15.75KHz Arcade Monitor Modes) */
extern int half_yres;
extern int unchained;

extern char *dirty_old;
extern char *dirty_new;

extern struct osd_bitmap *scrbitmap;
extern int gfx_xoffset;
extern int gfx_yoffset;
extern int gfx_display_lines;
extern int gfx_display_columns;
extern int gfx_width;
extern int gfx_height;
extern int skiplines;
extern int skipcolumns;
extern int use_triplebuf;
extern int triplebuf_pos,triplebuf_page_width;

unsigned int doublepixel[256];
unsigned int quadpixel[256]; /* for quadring pixels */


/*memsize of 'video page' for unchained blits (req. for 15.75KHz Arcade Monitor Modes) */
#define XPAGE_SIZE      0x8000
/* current 'page' for unchained modes */
static int xpage=-1;

/* this function lifted from Allegro */
static int vesa_scroll_async(int x, int y)
{
   int ret, seg;
   long a;
	extern void (*_pm_vesa_scroller)(void);	/* in Allegro */
	extern int _mmio_segment;	/* in Allegro */
	#define BYTES_PER_PIXEL(bpp)     (((int)(bpp) + 7) / 8)	/* in Allegro */
	extern __dpmi_regs _dpmi_reg;	/* in Allegro... I think */

//   vesa_xscroll = x;
//   vesa_yscroll = y;

#if 0
   if (_pm_vesa_scroller) {            /* use protected mode interface? */
      seg = _mmio_segment ? _mmio_segment : _my_ds();

      a = ((x * BYTES_PER_PIXEL(screen->vtable->color_depth)) +
	   (y * ((unsigned long)screen->line[1] - (unsigned long)screen->line[0]))) / 4;

      asm (
	 "  pushw %%es ; "
	 "  movw %w1, %%es ; "         /* set the IO segment */
	 "  call *%0 ; "               /* call the VESA function */
	 "  popw %%es "

      :                                /* no outputs */

      : "S" (_pm_vesa_scroller),       /* function pointer in esi */
	"a" (seg),                     /* IO segment in eax */
	"b" (0x00),                    /* mode in ebx */
	"c" (a & 0xFFFF),              /* low word of address in ecx */
	"d" (a >> 16)                  /* high word of address in edx */

//      : "memory", "%edi", "%cc"        /* clobbers edi and flags */
		: "memory", "%ebp", "%edi", "%cc" /* clobbers ebp, edi and flags (at least) */
      );

      ret = 0;
   }
   else
#endif
   {                              /* use a real mode interrupt call */
      _dpmi_reg.x.ax = 0x4F07;
      _dpmi_reg.x.bx = 0;
      _dpmi_reg.x.cx = x;
      _dpmi_reg.x.dx = y;

      __dpmi_int(0x10, &_dpmi_reg);
      ret = _dpmi_reg.h.ah;

//      _vsync_in();
   }

   return (ret ? -1 : 0);
}


void blitscreen_dirty1_vga(void)
{
	int width4, x, y;
	unsigned long *lb, address;

	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	address = 0xa0000 + gfx_xoffset + gfx_yoffset * gfx_width;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);

	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; )
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

/* for flipping between the 2 unchained VGA pages */
INLINE void unchained_flip(void)
{
	int	flip_value;
/* memory address of non-visible page */
	flip_value = ((XPAGE_SIZE * xpage)|0x0c);
/* flip the page var */
	xpage = !xpage;

/* need to change the offset address during the active display interval */
/* as the value is read during the vertical retrace */
	__asm__ __volatile__ (
	"movw	$0x3da,%%dx \n"
/* check for active display interval */
	"0:\n"
	"inb	%%dx,%%al \n"
	"testb	$1,%%al \n"
	"jnz 	0b \n"
/* change the offset address */
	"movw	$0x3d4,%%dx \n"
	"mov	%%cx,%%ax \n"
	"outw	%%ax,%%dx \n"
/* outputs  (none)*/
	:
/* inputs -
 ecx = flip_value */
	:"c" (flip_value)
/* registers modified */
	:"ax", "cx", "dx", "cc", "memory"
	);
}


/* unchained dirty modes */
void blitscreen_dirty1_unchained_vga(void)
{
	int x, y, i, outval, dirty_page;
	int plane, planeval, iloop, page;
	unsigned long *lb, address;
	unsigned char *lbsave;
	unsigned long asave;
	static int width4, word_blit, dirty_height;
	static int source_width, source_line_width, dest_width, dest_line_width;

	/* calculate our statics on the first call */
	if (xpage == -1)
	{
		width4 = ((scrbitmap->line[1] - scrbitmap->line[0]) >> 2);
		source_width = width4 << half_yres;
		dest_width = gfx_width >> 2;
		dest_line_width = (gfx_width << 2) >> half_yres;
		source_line_width = width4 << 4;
		xpage = 1;
		dirty_height = 16 >> half_yres;
		/* check if we have to do word rather than double word updates */
		word_blit = ((gfx_display_columns >> 2) & 3);
	}

	/* setup or selector */
	_farsetsel(screen->seg);

	dirty_page = xpage;
	/* need to update both 'pages', but only update each page while it isn't visible */
	for (page = 0; page < 2; page ++)
	{
		planeval=0x0100;
		/* go through each bit plane */
		for (plane = 0; plane < 4 ;plane ++)
		{
			address = 0xa0000 + (XPAGE_SIZE * dirty_page)+(gfx_xoffset >> 2) + (((gfx_yoffset >> half_yres) * gfx_width) >> 2);
			lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns + plane);
			/*set the bit plane */
			outportw(0x3c4, planeval|0x02);
			for (y = 0; y < gfx_display_lines ; y += 16)
			{
				for (x = 0; x < gfx_display_columns; )
				{
					int w = 16;
					if (ISDIRTY(x,y))
					{
						unsigned long *lb0 = lb + (x >> 2);
						unsigned long address0 = address + (x >> 2);
						int h;
						while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    	w += 16;
						if (x + w > gfx_display_columns)
                    		w = gfx_display_columns - x;


						if(word_blit)
						{
							iloop = w >> 3;
							for (h = 0; h < dirty_height && (y + (h << half_yres)) < gfx_display_lines; h++)
							{
								asave = address0;
								lbsave = (unsigned char *)lb0;
								for(i = 0; i < iloop; i++)
								{
									outval = *lbsave | (lbsave[4] << 8);
									_farnspokew(asave, outval);
									lbsave += 8;
									asave += 2;
								}
								lb0 += source_width;
								address0 += dest_width;
							}
						}
						else
						{
							iloop = w >> 4;
							for (h = 0; h < dirty_height && (y + (h << half_yres)) < gfx_display_lines; h++)
							{
								asave = address0;
								lbsave = (unsigned char *)lb0;
								for(i = 0; i < iloop; i++)
								{
									outval = *lbsave | (lbsave[4] << 8) | (lbsave[8] << 16) | (lbsave[12] << 24);
									_farnspokel(asave, outval);
									lbsave += 16;
									asave += 4;
								}
								lb0 += source_width;
								address0 += dest_width;
							}
						}
					}
					x += w;
        		}
				lb += source_line_width;
				address += dest_line_width;
			}
			/* move onto the next bit plane */
			planeval <<= 1;
		}
		/* 'bank switch' our unchained output on the first loop */
		if(!page)
			unchained_flip();
		/* move onto next 'page' */
		dirty_page = !dirty_page;
	}
}

/*asm routine for unchained blit - writes 4 bytes at a time */
INLINE void unchain_dword_blit(unsigned long *src,short seg,unsigned long address,int width,int height,int memwidth,int scrwidth)
{
	__asm__ __volatile__ (
/* save es and set it to our video selector */
	"pushw	%%es \n"
	"movw	%%dx,%%es \n"
	"movw 	$0x102,%%ax \n"
/* --bit plane loop-- */
	"cli \n"
	"0:\n"
/* save everything */
	"pushl	%%ebx \n"
	"pushl	%%edi \n"
	"pushl	%%eax \n"
	"pushl	%%ecx \n"

/* set the bit plane */
	"movw	$0x3c4,%%dx \n"
	"outw	%%ax,%%dx \n"

/* --height loop-- */
	"1:\n"
/* save counter, source address and dest destination */
	"pushl	%%ecx \n"
	"pushl	%%ebx \n"
	"pushl	%%edi \n"

/* --width loop-- */
	"xorl	%%ecx,%%ecx \n"
	"movl	%0,%%ecx \n"
	"2:\n"
/* get the 4 bytes */
/* bswap should be faster than shift */
	"movb	%%ds:12(%%ebx),%%al \n"
	"movb	%%ds:8(%%ebx),%%ah \n"
	"bswap	%%eax \n"
	"movb	%%ds:(%%ebx),%%al \n"
	"movb	%%ds:4(%%ebx),%%ah \n"

/* write the thing to video */
	"movl	%%eax,%%es:(%%edi) \n"
/* move onto next source/destination address */
	"addl	$4,%%edi \n"
	"addl	$16,%%ebx \n"

	"loop	2b \n"
/* --end of width loop-- */

/* get counter, source address and dest address back */
	"popl	%%edi \n"
	"popl	%%ebx \n"
	"popl	%%ecx \n"
/* move to the next line */
	"addl	%1,%%ebx \n"
	"addl	%2,%%edi \n"
	"loop	1b \n"
/* --end of height loop-- */

/* get everything back */
	"popl	%%ecx \n"
	"popl	%%eax \n"
	"popl	%%edi \n"
	"popl	%%ebx \n"
/* move onto next bit plane */
	"incl	%%ebx \n"
	"shlb	$1,%%ah \n"
/* check if we've done all 4 or not */
	"testb	$0x10,%%ah \n"
	"jz		0b \n"
/* --end of bit plane loop-- */
	"sti \n"
/* restore es */
	"popw	%%es \n"
/* outputs  (none)*/
	:
/* inputs -
 %0=width, %1=memwidth, %2=scrwidth
 ebx = src, ecx = height
 edx = seg, edi = address */
	:"g" (width),
	"g" (memwidth),
	"g" (scrwidth),
	"b" (src),
	"c" (height),
	"d" (seg),
	"D" (address)
/* registers modified */
	:"ax", "bx", "cx", "dx", "si", "di", "cc", "memory"
	);
}

/*asm routine for unchained blit - writes 2 bytes at a time */
INLINE void unchain_word_blit(unsigned long *src,short seg,unsigned long address,int width,int height,int memwidth,int scrwidth)
{
	__asm__ __volatile__ (
/* save es and set it to our video selector */
	"pushw	%%es \n"
	"movw	%%dx,%%es \n"
	"movw	$0x102,%%ax \n"
/* --bit plane loop-- */
	"cli \n"
	"0:\n"
/* save everything */
	"pushl	%%ebx \n"
	"pushl	%%edi \n"
	"pushw	%%ax \n"
	"pushl	%%ecx \n"

/* set the bit plane */
	"movw	$0x3c4,%%dx \n"
	"outw	%%ax,%%dx \n"

/* --height loop-- */
	"1:\n"
/* save counter, source address and dest destination */
	"pushl	%%ecx \n"
	"pushl	%%ebx \n"
	"pushl	%%edi \n"

/* --width loop-- */
	"movl	%0,%%ecx \n"
	"2:\n"
/* get the 2 bytes */
	"movb	%%ds:(%%ebx),%%al \n"
	"movb	%%ds:4(%%ebx),%%ah \n"
/* write the thing to video */
	"movw	%%ax,%%es:(%%edi) \n"
/* move onto next source/destination address */
	"addl	$2,%%edi \n"
	"addl	$8,%%ebx \n"

	"loop	2b \n"
/* --end of width loop-- */

/* get counter, source address and dest address back */
	"popl	%%edi \n"
	"popl	%%ebx \n"
	"popl	%%ecx \n"
/* move to the next line */
	"addl	%1,%%ebx \n"
	"addl	%2,%%edi \n"
	"loop	1b \n"
/* --end of height loop-- */

/* get everything back */
	"popl	%%ecx \n"
	"popw	%%ax \n"
	"popl	%%edi \n"
	"popl	%%ebx \n"
/* move onto next bit plane */
	"incl	%%ebx \n"
	"shlb	$1,%%ah \n"
/* check if we've done all 4 or not */
	"testb	$0x10,%%ah \n"
	"jz 	0b \n"
/* --end of bit plane loop-- */
	"sti \n"
/* restore es */
	"popw	%%es \n"
/* outputs  (none)*/
	:
/* inputs -
	%0=width, %1=memwidth, %2=scrwidth
 	ebx = src, ecx = height
 	edx = seg, edi = address */
	:"g" (width),
	"g" (memwidth),
	"g" (scrwidth),
	"b" (src),
	"c" (height),
	"d" (seg),
	"D" (address)
/* registers modified */
	:"ax", "bx", "cx", "dx", "si", "di", "cc", "memory"
	);
}




/* unchained 'non-dirty' modes */
void blitscreen_dirty0_unchained_vga(void)
{
	int y;
	unsigned long *lb, address;

	static int width4,columns4,column_chained,memwidth,scrwidth,disp_height,word_blit;
	int	outval;

   /* only calculate our statics the first time around */
	if(xpage==-1)
	{
      	/* vars for normal chained blit */
		width4 = (scrbitmap->line[1] - scrbitmap->line[0]) >> 2;
		columns4 = gfx_display_columns >> 2;
		disp_height = gfx_display_lines >> half_yres;
		/*vars for unchained blit */
		xpage = 1;
		memwidth = (scrbitmap->line[1] - scrbitmap->line[0]) << half_yres;
		scrwidth = gfx_width >> 2;
		column_chained = columns4 >> 2;
		/* check if we have to do word rather than double word updates */
		if ((word_blit = (columns4 & 3)))
			column_chained = column_chained << 1;
	}

	/* get the start of the screen bitmap */
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
	/* and the start address in video memory */
	address = 0xa0000 + (XPAGE_SIZE * xpage)+(gfx_xoffset >> 2) + (((gfx_yoffset >> half_yres) * gfx_width) >> 2);
	if (word_blit)
		unchain_word_blit (lb, screen->seg, address, column_chained, disp_height, memwidth, scrwidth);
	else
		unchain_dword_blit(lb, screen->seg, address, column_chained, disp_height, memwidth, scrwidth);
	/* 'bank switch' our unchained output */
	unchained_flip();
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
	xoffs = (BPP/8)*(gfx_xoffset + triplebuf_pos); \
	width = gfx_display_columns/(4/(BPP/8)); \
	src = scrbitmap->line[skiplines] + (BPP/8)*skipcolumns;	\

#define TRIPLEBUF_FLIP \
	if (use_triplebuf) \
	{ \
		vesa_scroll_async(triplebuf_pos,0); \
		triplebuf_pos = (triplebuf_pos + triplebuf_page_width) % (3*triplebuf_page_width); \
	}

#define DIRTY0_NXNS(MX,MY,BPP) \
	for (y = 0;y < gfx_display_lines;y++) \
	{ \
		address = bmp_write_line(screen,vesa_line) + xoffs; \
		copyline_##MX##x_##BPP##bpp(src,dest_seg,address,width); \
		vesa_line += MY; \
		src += line_offs; \
	} \
	TRIPLEBUF_FLIP

#define DIRTY0_NX2(MX,BPP) \
	for (y = 0;y < gfx_display_lines;y++) \
	{ \
		address = bmp_write_line(screen,vesa_line) + xoffs; \
		copyline_##MX##x_##BPP##bpp(src,dest_seg,address,width); \
		address = bmp_write_line(screen,vesa_line+1) + xoffs; \
		copyline_##MX##x_##BPP##bpp(src,dest_seg,address,width); \
		vesa_line += 2; \
		src += line_offs; \
	} \
	TRIPLEBUF_FLIP

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
	} \
	TRIPLEBUF_FLIP



void blitscreen_dirty1_vesa_1x_1x_8bpp(void)   { DIRTY1(8)  DIRTY1_NXNS(1,1,8)  }
void blitscreen_dirty1_vesa_1x_2x_8bpp(void)   { DIRTY1(8)  DIRTY1_NX2 (1,8)  }
void blitscreen_dirty1_vesa_1x_2xs_8bpp(void)  { DIRTY1(8)  DIRTY1_NXNS(1,2,8)  }
void blitscreen_dirty1_vesa_2x_1x_8bpp(void)   { DIRTY0(8)  DIRTY0_NXNS(2,1,8)  }
void blitscreen_dirty1_vesa_2x_2x_8bpp(void)   { DIRTY1(8)  DIRTY1_NX2 (2,8)  }
void blitscreen_dirty1_vesa_2x_2xs_8bpp(void)  { DIRTY1(8)  DIRTY1_NXNS(2,2,8)  }
void blitscreen_dirty1_vesa_2x_3x_8bpp(void)   { DIRTY1(8)  DIRTY1_NX3 (2,8)  }
void blitscreen_dirty1_vesa_2x_3xs_8bpp(void)  { DIRTY1(8)  DIRTY1_NXNS(2,3,8)  }
void blitscreen_dirty1_vesa_3x_2x_8bpp(void)   { DIRTY1(8)  DIRTY1_NX2 (3,8)  }
void blitscreen_dirty1_vesa_3x_2xs_8bpp(void)  { DIRTY1(8)  DIRTY1_NXNS(3,2,8)  }
void blitscreen_dirty1_vesa_3x_3x_8bpp(void)   { DIRTY1(8)  DIRTY1_NX3 (3,8)  }
void blitscreen_dirty1_vesa_3x_3xs_8bpp(void)  { DIRTY1(8)  DIRTY1_NXNS(3,3,8)  }
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
void blitscreen_dirty0_vesa_2x_1x_8bpp(void)   { DIRTY0(8)  DIRTY0_NXNS(2,1,8)  }
void blitscreen_dirty0_vesa_2x_2x_8bpp(void)   { DIRTY0(8)  DIRTY0_NX2 (2,8)  }
void blitscreen_dirty0_vesa_2x_2xs_8bpp(void)  { DIRTY0(8)  DIRTY0_NXNS(2,2,8)  }
void blitscreen_dirty0_vesa_2x_3x_8bpp(void)   { DIRTY0(8)  DIRTY0_NX3 (2,8)  }
void blitscreen_dirty0_vesa_2x_3xs_8bpp(void)  { DIRTY0(8)  DIRTY0_NXNS(2,3,8)  }
void blitscreen_dirty0_vesa_3x_2x_8bpp(void)   { DIRTY0(8)  DIRTY0_NX2 (3,8)  }
void blitscreen_dirty0_vesa_3x_2xs_8bpp(void)  { DIRTY0(8)  DIRTY0_NXNS(3,2,8)	}
void blitscreen_dirty0_vesa_3x_3x_8bpp(void)   { DIRTY0(8)  DIRTY0_NX3 (3,8)  }
void blitscreen_dirty0_vesa_3x_3xs_8bpp(void)  { DIRTY0(8)  DIRTY0_NXNS(3,3,8)  }
void blitscreen_dirty0_vesa_4x_3x_8bpp(void)   { DIRTY0(8)  DIRTY0_NX3 (4,8)  }
void blitscreen_dirty0_vesa_4x_3xs_8bpp(void)  { DIRTY0(8)  DIRTY0_NXNS(4,3,8)  }

void blitscreen_dirty0_vesa_1x_1x_16bpp(void)  { DIRTY0(16) DIRTY0_NXNS(1,1,16) }
void blitscreen_dirty0_vesa_1x_2x_16bpp(void)  { DIRTY0(16) DIRTY0_NX2 (1,16) }
void blitscreen_dirty0_vesa_1x_2xs_16bpp(void) { DIRTY0(16) DIRTY0_NXNS(1,2,16) }
void blitscreen_dirty0_vesa_2x_2x_16bpp(void)  { DIRTY0(16) DIRTY0_NX2 (2,16) }
void blitscreen_dirty0_vesa_2x_2xs_16bpp(void) { DIRTY0(16) DIRTY0_NXNS(2,2,16) }
