/*************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

**************************************************************************/
#include "driver.h"
#include "cpu/tms34010/tms34010.h"

#define PARTIAL_UPDATING		0
#define PARTIAL_UPDATE_CHUNK	16

static void partial_update_callback(int scanline);

void wms_stateload(void);
void wms_statesave(void);

unsigned short *wms_videoram;
int wms_videoram_size = 0x80000;

/* Variables in machine/smashtv.c */
extern unsigned char *wms_cmos_ram;
extern unsigned int wms_autoerase_enable;
extern unsigned int wms_autoerase_start;
extern unsigned int wms_dma_pal_word;

void wms_vram_w(int offset, int data)
{
	unsigned int tempw,tempwb;
	unsigned short tempwordhi;
	unsigned short tempwordlo;
	unsigned short datalo;
	unsigned short datahi;
	unsigned int mask;

	/* first vram data */
	tempwordhi = wms_videoram[offset+1];
	tempwordlo = wms_videoram[offset];
	tempw = (wms_videoram[offset]&0x00ff) + ((wms_videoram[offset+1]&0x00ff)<<8);
	tempwb = COMBINE_WORD(tempw,data);
	datalo = tempwb&0x00ff;
	datahi = (tempwb&0xff00)>>8;
	wms_videoram[offset] = (tempwordlo&0xff00)|datalo;
	wms_videoram[offset+1] = (tempwordhi&0xff00)|datahi;

	/* now palette data */
	tempwordhi = wms_videoram[offset+1];
	tempwordlo = wms_videoram[offset];
	mask = (~(((unsigned int) data)>>16))|0xffff0000;
	data = ((data&0xffff0000) | wms_dma_pal_word) & mask;
	tempw = (((unsigned short) (wms_videoram[offset]&0xff00))>>8) + (wms_videoram[offset+1]&0xff00);
	tempwb = COMBINE_WORD(tempw,data);
	datalo = tempwb&0x00ff;
	datahi = (tempwb&0xff00)>>8;
	wms_videoram[offset] = (tempwordlo&0x00ff)|(datalo<<8);
	wms_videoram[offset+1] = (tempwordhi&0x00ff)|(datahi<<8);
}
void wms_objpalram_w(int offset, int data)
{
	unsigned int tempw,tempwb;
	unsigned short tempwordhi;
	unsigned short tempwordlo;
	unsigned short datalo;
	unsigned short datahi;

	tempwordhi = wms_videoram[offset+1];
	tempwordlo = wms_videoram[offset];
	tempw = ((wms_videoram[offset]&0xff00)>>8) + (wms_videoram[offset+1]&0xff00);
	tempwb = COMBINE_WORD(tempw,data);
	datalo = tempwb&0x00ff;
	datahi = (tempwb&0xff00)>>8;
	wms_videoram[offset] = (tempwordlo&0x00ff)|(datalo<<8);
	wms_videoram[offset+1] = (tempwordhi&0x00ff)|(datahi<<8);
}
int wms_vram_r(int offset)
{
	return (wms_videoram[offset]&0x00ff) | (wms_videoram[offset+1]<<8);
}
int wms_objpalram_r(int offset)
{
	return (wms_videoram[offset]>>8) | (wms_videoram[offset+1]&0xff00);
}

int wms_vh_start(void)
{
	if ((wms_cmos_ram = malloc(0x8000)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc CMOS RAM\n");
		return 1;
	}
	if ((paletteram = malloc(0x4000)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc color RAM\n");
		free(wms_cmos_ram);
		return 1;
	}
	if ((wms_videoram = malloc(wms_videoram_size)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc video RAM\n");
		free(wms_cmos_ram);
		free(paletteram);
		return 1;
	}
	memset(wms_cmos_ram,0,0x8000);
#if PARTIAL_UPDATING
	timer_set(cpu_getscanlinetime(0), 0, partial_update_callback);
#endif
	return 0;
}
int wms_t_vh_start(void)
{
	if ((wms_cmos_ram = malloc(0x10000)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc CMOS RAM\n");
		return 1;
	}
	if ((paletteram = malloc(0x20000)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc color RAM\n");
		free(wms_cmos_ram);
		return 1;
	}
	if ((wms_videoram = malloc(0x100000)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc video RAM\n");
		free(wms_cmos_ram);
		free(paletteram);
		return 1;
	}
	memset(wms_cmos_ram,0,0x8000);
#if PARTIAL_UPDATING
	timer_set(cpu_getscanlinetime(0), 0, partial_update_callback);
#endif
	return 0;
}
void wms_vh_stop (void)
{
	free(wms_cmos_ram);
	free(wms_videoram);
	free(paletteram);
}

void wms_update_partial(struct osd_bitmap *bitmap, int min, int max)
{
	int v,h;
	unsigned short *pens = Machine->pens;
	unsigned short *rv;
	int skip,col;

	rv = &wms_videoram[(~TMS34010_get_DPYSTRT(0) & 0x1ff0)<<5];
	rv = (((rv + 512 * (min - Machine->drv->visible_area.min_y)) - wms_videoram) & 0x3ffff) + wms_videoram;
	col = Machine->drv->visible_area.max_x;
	skip = 511 - col;

	if (bitmap->depth==16)
	{
		for (v = min; v <= max; v++)
		{
			unsigned short *rows = &((unsigned short *)bitmap->line[v])[0];
			unsigned int diff = rows - rv;
			h = col;
			do
			{
				*(rv+diff) = pens[*rv];
				rv++;
			}
			while (h--);

			if (wms_autoerase_enable||(v>=wms_autoerase_start))
			{
				memcpy(rv-col-1,&wms_videoram[510*512],(col+1)*sizeof(unsigned short));
			}

			rv = (((rv + skip) - wms_videoram) & 0x3ffff) + wms_videoram;
		}
	}
	else
	{
		for (v = min; v <= max; v++)
		{
			unsigned char *rows = &(bitmap->line[v])[0];
			h = col;
			do
			{
				*(rows++) = pens[*(rv++)];
			}
			while (h--);

			if (wms_autoerase_enable||(v>=wms_autoerase_start))
			{
				memcpy(rv-col-1,&wms_videoram[510*512],(col+1)*sizeof(unsigned short));
			}

			rv = (((rv + skip) - wms_videoram) & 0x3ffff) + wms_videoram;
		}
	}
}

void partial_update_callback(int scanline)
{
	int min, max;

	if (scanline == 0)
	{
		min = (Machine->drv->screen_height / PARTIAL_UPDATE_CHUNK) * PARTIAL_UPDATE_CHUNK;
		max = Machine->drv->screen_height - 1;
	}
	else
	{
		min = scanline - PARTIAL_UPDATE_CHUNK;
		max = scanline - 1;
	}

	if (max >= Machine->drv->visible_area.min_y && min <= Machine->drv->visible_area.max_y)
	{
		if (min < Machine->drv->visible_area.min_y)
			min = Machine->drv->visible_area.min_y;
		if (max > Machine->drv->visible_area.max_y)
			max = Machine->drv->visible_area.max_y;
		wms_update_partial(Machine->scrbitmap, min, max);
	}

	scanline += PARTIAL_UPDATE_CHUNK;
	if (scanline >= Machine->drv->screen_height) scanline = 0;
	timer_set(cpu_getscanlinetime(scanline), scanline, partial_update_callback);
}


void wms_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	//if (keyboard_pressed(KEYCODE_Q)) wms_statesave();
	//if (keyboard_pressed(KEYCODE_W)) wms_stateload();

	if (keyboard_pressed(KEYCODE_E)&&errorlog) fprintf(errorlog, "log spot\n");
	//if (keyboard_pressed(KEYCODE_R)&&errorlog) fprintf(errorlog, "adpcm: okay\n");

#if !PARTIAL_UPDATING
	wms_update_partial(bitmap, Machine->drv->visible_area.min_y, Machine->drv->visible_area.max_y);
#endif
	wms_autoerase_start=1000;
}
