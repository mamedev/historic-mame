#include "driver.h"
#include "sndhrdw/m72.h"



unsigned char *m72_videoram1,*m72_videoram2,*majtitle_rowscrollram;
static unsigned char *m72_spriteram;
static int rastersplit;
static int splitline;
static struct tilemap *fg_tilemap,*bg_tilemap;
static int xadjust;
static int scrollx1[256],scrolly1[256],scrollx2[256],scrolly2[256];
static int video_off;
extern unsigned char *spriteram,*spriteram_2;
extern size_t spriteram_size;

static int irq1,irq2;

void m72_init_machine(void)
{
	irq1 = 0x20;
	irq2 = 0x22;
	m72_init_sound();
}

void xmultipl_init_machine(void)
{
	irq1 = 0x08;
	irq2 = 0x0a;
	m72_init_sound();
}

void poundfor_init_machine(void)
{
	irq1 = 0x18;
	irq2 = 0x1a;
	m72_init_sound();
}

int m72_interrupt(void)
{
	int line = 255 - cpu_getiloops();

	if (line == 255)	/* vblank */
	{
		rastersplit = 0;
		interrupt_vector_w(0,irq1);
		return interrupt();
	}
	else
	{
		if (line != splitline - 128)
			return ignore_interrupt();

		rastersplit = line + 1;

		/* this is used to do a raster effect and show the score display at
		   the bottom of the screen or other things. The line where the
		   interrupt happens is programmable (and the interrupt can be triggered
		   multiple times, by changing the interrupt line register in the
		   interrupt handler).
		 */
		interrupt_vector_w(0,irq2);
		return interrupt();
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

INLINE void m72_get_tile_info(int tile_index,unsigned char *videoram,int gfxnum)
{
	int code,attr,color,pri;

	tile_index *= 4;

	code  = videoram[tile_index];
	attr  = videoram[tile_index+1];
	color = videoram[tile_index+2];
	SET_TILE_INFO(gfxnum,code + ((attr & 0x3f) << 8),color & 0x0f)

	if (color & 0x80) pri = 2;
	else if (color & 0x40) pri = 1;
	else pri = 0;
/* color & 0x10 is used in bchopper and hharry, more priority? */

	tile_info.flags = TILE_FLIPYX((attr & 0xc0) >> 6) | TILE_SPLIT(pri);
}

INLINE void rtype2_get_tile_info(int tile_index,unsigned char *videoram,int gfxnum)
{
	int code,attr,color,pri;

	tile_index *= 4;

	code  = videoram[tile_index] + (videoram[tile_index+1] << 8);
	color = videoram[tile_index+2];
	attr  = videoram[tile_index+3];
	SET_TILE_INFO(gfxnum,code,color & 0x0f)

	if (attr & 0x01) pri = 2;
	else if (color & 0x80) pri = 1;
	else pri = 0;

	tile_info.flags = TILE_FLIPYX((color & 0x60) >> 5) | TILE_SPLIT(pri);

/* (videoram[tile_index+2] & 0x10) is used by majtitle on the green, but it's not clear for what */
/* (videoram[tile_index+3] & 0xfe) are used as well */
}


static void m72_get_bg_tile_info(int tile_index)
{
	m72_get_tile_info(tile_index,m72_videoram2,2);
}

static void m72_get_fg_tile_info(int tile_index)
{
	m72_get_tile_info(tile_index,m72_videoram1,1);
}

static void hharry_get_bg_tile_info(int tile_index)
{
	m72_get_tile_info(tile_index,m72_videoram2,1);
}

static void hharry_get_fg_tile_info(int tile_index)
{
	m72_get_tile_info(tile_index,m72_videoram1,1);
}

static void rtype2_get_bg_tile_info(int tile_index)
{
	rtype2_get_tile_info(tile_index,m72_videoram2,1);
}

static void rtype2_get_fg_tile_info(int tile_index)
{
	rtype2_get_tile_info(tile_index,m72_videoram1,1);
}


static UINT32 majtitle_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return row*256 + col;
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int m72_vh_start(void)
{
	bg_tilemap = tilemap_create(m72_get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE | TILEMAP_SPLIT,8,8,64,64);
	fg_tilemap = tilemap_create(m72_get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT | TILEMAP_SPLIT,8,8,64,64);

	m72_spriteram = malloc(spriteram_size);

	if (!fg_tilemap || !bg_tilemap || !m72_spriteram)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_transmask(fg_tilemap,0,0xffff);
	tilemap_set_transmask(fg_tilemap,1,0x00ff);
	tilemap_set_transmask(fg_tilemap,2,0x0001);

	tilemap_set_transmask(bg_tilemap,0,0xffff);
	tilemap_set_transmask(bg_tilemap,1,0x00ff);
	tilemap_set_transmask(bg_tilemap,2,0x0001);

	memset(m72_spriteram,0,spriteram_size);

	xadjust = 0;

	return 0;
}

int rtype2_vh_start(void)
{
	bg_tilemap = tilemap_create(rtype2_get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE | TILEMAP_SPLIT,8,8,64,64);
	fg_tilemap = tilemap_create(rtype2_get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT | TILEMAP_SPLIT,8,8,64,64);

	m72_spriteram = malloc(spriteram_size);

	if (!fg_tilemap || !bg_tilemap || !m72_spriteram)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_transmask(fg_tilemap,0,0xffff);
	tilemap_set_transmask(fg_tilemap,1,0x00ff);
	tilemap_set_transmask(fg_tilemap,2,0x0001);

	tilemap_set_transmask(bg_tilemap,0,0xffff);
	tilemap_set_transmask(bg_tilemap,1,0x00ff);
	tilemap_set_transmask(bg_tilemap,2,0x0001);

	memset(m72_spriteram,0,spriteram_size);

	xadjust = -4;

	return 0;
}

/* Major Title has a larger background RAM, and rowscroll */
int majtitle_vh_start(void)
{
// The tilemap can be 256x64, but seems to be used at 128x64 (scroll wraparound).
// The layout ramains 256x64, the right half is just not displayed.
//	bg_tilemap = tilemap_create(rtype2_get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE | TILEMAP_SPLIT,8,8,256,64);
	bg_tilemap = tilemap_create(rtype2_get_bg_tile_info,majtitle_scan_rows,TILEMAP_OPAQUE | TILEMAP_SPLIT,8,8,128,64);
	fg_tilemap = tilemap_create(rtype2_get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT | TILEMAP_SPLIT,8,8,64,64);

	m72_spriteram = malloc(spriteram_size);

	if (!fg_tilemap || !bg_tilemap || !m72_spriteram)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_transmask(fg_tilemap,0,0xffff);
	tilemap_set_transmask(fg_tilemap,1,0x00ff);
	tilemap_set_transmask(fg_tilemap,2,0x0001);

	tilemap_set_transmask(bg_tilemap,0,0xffff);
	tilemap_set_transmask(bg_tilemap,1,0x00ff);
	tilemap_set_transmask(bg_tilemap,2,0x0001);

	memset(m72_spriteram,0,spriteram_size);

	xadjust = -4;

	return 0;
}

int hharry_vh_start(void)
{
	bg_tilemap = tilemap_create(hharry_get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE | TILEMAP_SPLIT,8,8,64,64);
	fg_tilemap = tilemap_create(hharry_get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT | TILEMAP_SPLIT,8,8,64,64);

	m72_spriteram = malloc(spriteram_size);

	if (!fg_tilemap || !bg_tilemap || !m72_spriteram)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_transmask(fg_tilemap,0,0xffff);
	tilemap_set_transmask(fg_tilemap,1,0x00ff);
	tilemap_set_transmask(fg_tilemap,2,0x0001);

	tilemap_set_transmask(bg_tilemap,0,0xffff);
	tilemap_set_transmask(bg_tilemap,1,0x00ff);
	tilemap_set_transmask(bg_tilemap,2,0x0001);

	memset(m72_spriteram,0,spriteram_size);

	xadjust = -4;

	return 0;
}

void m72_vh_stop(void)
{
	free(m72_spriteram);
	m72_spriteram = 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ_HANDLER( m72_palette1_r )
{
	return paletteram[offset];
}

READ_HANDLER( m72_palette2_r )
{
	return paletteram_2[offset];
}

INLINE void changecolor(int color,int r,int g,int b)
{
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_change_color(color,r,g,b);
}

WRITE_HANDLER( m72_palette1_w )
{
	paletteram[offset] = data;
	if (offset & 1) return;
	offset &= 0x3ff;
	changecolor(offset / 2,
			paletteram[offset + 0x000],
			paletteram[offset + 0x400],
			paletteram[offset + 0x800]);
}

WRITE_HANDLER( m72_palette2_w )
{
	paletteram_2[offset] = data;
	if (offset & 1) return;
	offset &= 0x3ff;
	changecolor(offset / 2 + 512,
			paletteram_2[offset + 0x000],
			paletteram_2[offset + 0x400],
			paletteram_2[offset + 0x800]);
}

READ_HANDLER( m72_videoram1_r )
{
	return m72_videoram1[offset];
}

READ_HANDLER( m72_videoram2_r )
{
	return m72_videoram2[offset];
}

WRITE_HANDLER( m72_videoram1_w )
{
	if (m72_videoram1[offset] != data)
	{
		m72_videoram1[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset/4);
	}
}

WRITE_HANDLER( m72_videoram2_w )
{
	if (m72_videoram2[offset] != data)
	{
		m72_videoram2[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset/4);
	}
}

WRITE_HANDLER( m72_irq_line_w )
{
	offset *= 8;
	splitline = (splitline & (0xff00 >> offset)) | (data << offset);
}

WRITE_HANDLER( m72_scrollx1_w )
{
	int i;

	offset *= 8;
	scrollx1[rastersplit] = (scrollx1[rastersplit] & (0xff00 >> offset)) | (data << offset);

	for (i = rastersplit+1;i < 256;i++)
		scrollx1[i] = scrollx1[rastersplit];
}

WRITE_HANDLER( m72_scrollx2_w )
{
	int i;

	offset *= 8;
	scrollx2[rastersplit] = (scrollx2[rastersplit] & (0xff00 >> offset)) | (data << offset);

	for (i = rastersplit+1;i < 256;i++)
		scrollx2[i] = scrollx2[rastersplit];
}

WRITE_HANDLER( m72_scrolly1_w )
{
	int i;

	offset *= 8;
	scrolly1[rastersplit] = (scrolly1[rastersplit] & (0xff00 >> offset)) | (data << offset);

	for (i = rastersplit+1;i < 256;i++)
		scrolly1[i] = scrolly1[rastersplit];
}

WRITE_HANDLER( m72_scrolly2_w )
{
	int i;

	offset *= 8;
	scrolly2[rastersplit] = (scrolly2[rastersplit] & (0xff00 >> offset)) | (data << offset);

	for (i = rastersplit+1;i < 256;i++)
		scrolly2[i] = scrolly2[rastersplit];
}

WRITE_HANDLER( m72_spritectrl_w )
{
//logerror("%04x: write %02x to sprite ctrl+%d\n",cpu_get_pc(),data,offset);
	/* TODO: this is ok for R-Type, but might be wrong for others */
	if (offset == 1)
	{
		memcpy(m72_spriteram,spriteram,spriteram_size);
		if (data & 0x40) memset(spriteram,0,spriteram_size);
		/* bit 7 is used by bchopper, nspirit, imgfight, loht, gallop - meaning unknown */
		/* rtype2 uses bits 4,5,6 and 7 - of course it could be a different chip */
	}
}

WRITE_HANDLER( hharry_spritectrl_w )
{
//logerror("%04x: write %02x to sprite ctrl+%d\n",cpu_get_pc(),data,offset);
	if (offset == 0)
	{
		memcpy(m72_spriteram,spriteram,spriteram_size);
		memset(spriteram,0,spriteram_size);
	}
}

WRITE_HANDLER( hharryu_spritectrl_w )
{
//logerror("%04x: write %02x to sprite ctrl+%d\n",cpu_get_pc(),data,offset);
	if (offset == 1)
	{
		memcpy(m72_spriteram,spriteram,spriteram_size);
		if (data & 0x80) memset(spriteram,0,spriteram_size);
		/* hharryu uses bits 2,3,4,5,6 and 7 - of course it could be a different chip */
		/* majtitle uses bits 2,3,5,6 and 7 - of course it could be a different chip */
	}
}


WRITE_HANDLER( m72_port02_w )
{
	if (offset != 0)
	{
		if (data) logerror("write %02x to port 03\n",data);
		return;
	}
	if (data & 0xe0) logerror("write %02x to port 02\n",data);

	/* bits 0/1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 2 is flip screen (handled both by software and hardware) */
	flip_screen_set(((data & 0x04) >> 2) ^ (~readinputport(5) & 1));

	/* bit 3 is display disable */
	video_off = data & 0x08;

	/* bit 4 resets sound CPU (active low) */
	if (data & 0x10)
		cpu_set_reset_line(1,CLEAR_LINE);
	else
		cpu_set_reset_line(1,ASSERT_LINE);

	/* other bits unknown */
}

WRITE_HANDLER( rtype2_port02_w )
{
	if (offset != 0)
	{
		if (data) logerror("write %02x to port 03\n",data);
		return;
	}
	if (data & 0xf0) logerror("write %02x to port 02\n",data);

	/* bits 0/1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 2 is flip screen (handled both by software and hardware) */
	flip_screen_set(((data & 0x04) >> 2) ^ (~readinputport(5) & 1));

	/* bit 3 is display disable */
	video_off = data & 0x08;

	/* other bits unknown */
}


static int majtitle_rowscroll;

/* the following is mostly a kludge. This register seems to be used for something else */
WRITE_HANDLER( majtitle_gfx_ctrl_w )
{
	if (offset == 1)
	{
		if (data) majtitle_rowscroll = 1;
		else majtitle_rowscroll = 0;
	}
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		int code,color,sx,sy,flipx,flipy,w,h,x,y;


		code = m72_spriteram[offs+2] | (m72_spriteram[offs+3] << 8);
		color = m72_spriteram[offs+4] & 0x0f;
		sx = -256+(m72_spriteram[offs+6] | ((m72_spriteram[offs+7] & 0x03) << 8));
		sy = 512-(m72_spriteram[offs+0] | ((m72_spriteram[offs+1] & 0x01) << 8));
		flipx = m72_spriteram[offs+5] & 0x08;
		flipy = m72_spriteram[offs+5] & 0x04;

		w = 1 << ((m72_spriteram[offs+5] & 0xc0) >> 6);
		h = 1 << ((m72_spriteram[offs+5] & 0x30) >> 4);
		sy -= 16 * h;

		if (flip_screen)
		{
			sx = 512 - 16*w - sx;
			sy = 512 - 16*h - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		for (x = 0;x < w;x++)
		{
			for (y = 0;y < h;y++)
			{
				int c = code;

				if (flipx) c += 8*(w-1-x);
				else c += 8*x;
				if (flipy) c += h-1-y;
				else c += y;

				drawgfx(bitmap,Machine->gfx[0],
						c,
						color,
						flipx,flipy,
						sx + 16*x,sy + 16*y,
						&Machine->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
}

static void majtitle_draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		int code,color,sx,sy,flipx,flipy,w,h,x,y;


		code = spriteram_2[offs+2] | (spriteram_2[offs+3] << 8);
		color = spriteram_2[offs+4] & 0x0f;
		sx = -256+(spriteram_2[offs+6] | ((spriteram_2[offs+7] & 0x03) << 8));
		sy = 512-(spriteram_2[offs+0] | ((spriteram_2[offs+1] & 0x01) << 8));
		flipx = spriteram_2[offs+5] & 0x08;
		flipy = spriteram_2[offs+5] & 0x04;

		w = 1;// << ((spriteram_2[offs+5] & 0xc0) >> 6);
		h = 1 << ((spriteram_2[offs+5] & 0x30) >> 4);
		sy -= 16 * h;

		if (flip_screen)
		{
			sx = 512 - 16*w - sx;
			sy = 512 - 16*h - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		for (x = 0;x < w;x++)
		{
			for (y = 0;y < h;y++)
			{
				int c = code;

				if (flipx) c += 8*(w-1-x);
				else c += 8*x;
				if (flipy) c += h-1-y;
				else c += y;

				drawgfx(bitmap,Machine->gfx[2],
						c,
						color,
						flipx,flipy,
						sx + 16*x,sy + 16*y,
						&Machine->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
}

static void mark_sprite_colors(unsigned char *ram)
{
	int offs,color,i;
	int colmask[32];
	int pal_base;


	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;

	for (color = 0;color < 32;color++) colmask[color] = 0;

	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		color = ram[offs+4] & 0x0f;
		colmask[color] |= 0xffff;
	}

	for (color = 0;color < 32;color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] |= PALETTE_COLOR_VISIBLE;
		}
	}
}

static void draw_layer(struct osd_bitmap *bitmap,
		struct tilemap *tilemap,int *scrollx,int *scrolly,int priority)
{
	int start,i;
	/* use clip regions to split the screen */
	struct rectangle clip;

	clip.min_x = Machine->visible_area.min_x;
	clip.max_x = Machine->visible_area.max_x;
	start = Machine->visible_area.min_y - 128;
	do
	{
		i = start;
		while (scrollx[i+1] == scrollx[start] && scrolly[i+1] == scrolly[start]
				&& i < Machine->visible_area.max_y - 128)
			i++;

		clip.min_y = start + 128;
		clip.max_y = i + 128;
		tilemap_set_clip(tilemap,&clip);
		tilemap_set_scrollx(tilemap,0,scrollx[start] + xadjust);
		tilemap_set_scrolly(tilemap,0,scrolly[start]);
		tilemap_draw(bitmap,tilemap,priority,0);

		start = i+1;
	} while (start < Machine->visible_area.max_y - 128);
}

static void draw_bg(struct osd_bitmap *bitmap,int priority)
{
	draw_layer(bitmap,bg_tilemap,scrollx2,scrolly2,priority);
}

static void draw_fg(struct osd_bitmap *bitmap,int priority)
{
	draw_layer(bitmap,fg_tilemap,scrollx1,scrolly1,priority);
}


void m72_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	if (video_off)
	{
		fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);
		return;
	}

	tilemap_set_clip(fg_tilemap,0);
	tilemap_set_clip(bg_tilemap,0);

	tilemap_update(bg_tilemap);
	tilemap_update(fg_tilemap);

	palette_init_used_colors();
	mark_sprite_colors(m72_spriteram);
	palette_recalc();

	draw_bg(bitmap,TILEMAP_BACK);
	draw_fg(bitmap,TILEMAP_BACK);
	draw_sprites(bitmap);
	draw_bg(bitmap,TILEMAP_FRONT);
	draw_fg(bitmap,TILEMAP_FRONT);
}

void majtitle_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i;


	if (video_off)
	{
		fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);
		return;
	}

	tilemap_set_clip(fg_tilemap,0);

	if (majtitle_rowscroll)
	{
		tilemap_set_scroll_rows(bg_tilemap,512);
		for (i = 0;i < 512;i++)
			tilemap_set_scrollx(bg_tilemap,(i+scrolly2[0])&0x1ff,
					256 + majtitle_rowscrollram[2*i] + (majtitle_rowscrollram[2*i+1] << 8) + xadjust);
	}
	else
	{
		tilemap_set_scroll_rows(bg_tilemap,1);
		tilemap_set_scrollx(bg_tilemap,0,256 + scrollx2[0] + xadjust);
	}
	tilemap_set_scrolly(bg_tilemap,0,scrolly2[0]);
	tilemap_update(bg_tilemap);
	tilemap_update(fg_tilemap);

	palette_init_used_colors();
	mark_sprite_colors(m72_spriteram);
	mark_sprite_colors(spriteram_2);
	palette_recalc();

	tilemap_draw(bitmap,bg_tilemap,TILEMAP_BACK,0);
	draw_fg(bitmap,TILEMAP_BACK);
	majtitle_draw_sprites(bitmap);
	draw_sprites(bitmap);
	tilemap_draw(bitmap,bg_tilemap,TILEMAP_FRONT,0);
	draw_fg(bitmap,TILEMAP_FRONT);
}


void m72_eof_callback(void)
{
	int i;

	for (i = 0;i < 255;i++)
	{
		scrollx1[i] = scrollx1[255];
		scrolly1[i] = scrolly1[255];
		scrollx2[i] = scrollx2[255];
		scrolly2[i] = scrolly2[255];
	}
}
