#include "driver.h"


static data8_t *vram[2],*unkram;
static int bankctrl,rambank,gfxrom_select;
static struct tilemap *tilemap[2];



void hexion_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

INLINE void get_tile_info(int tile_index,data8_t *ram)
{
	tile_index *= 4;
	SET_TILE_INFO(0,ram[tile_index] + ((ram[tile_index+1] & 0x3f) << 8),ram[tile_index+2] & 0x0f)
}

static void get_tile_info0(int tile_index)
{
	get_tile_info(tile_index,vram[0]);
}

static void get_tile_info1(int tile_index)
{
	get_tile_info(tile_index,vram[1]);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int hexion_vh_start(void)
{
	tilemap[0] = tilemap_create(get_tile_info0,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[1] = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,64,32);

	if (!tilemap[0] || !tilemap[1])
		return 1;

	tilemap_set_transparent_pen(tilemap[0],0);
	tilemap_set_scrollx(tilemap[1],0,-4);
	tilemap_set_scrolly(tilemap[1],0,4);

	vram[0] = memory_region(REGION_CPU1) + 0x30000;
	vram[1] = vram[0] + 0x2000;
	unkram = vram[1] + 0x2000;

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( hexion_bankswitch_w )
{
	unsigned char *rom = memory_region(REGION_CPU1) + 0x10000;

	/* bits 0-3 select ROM bank */
	cpu_setbank(1,rom + 0x2000 * (data & 0x0f));

	/* does bit 6 trigger the 052591? */
	if (data & 0x40)
	{
		int bank = unkram[0]&1;
		memset(vram[bank],unkram[1],0x2000);
		tilemap_mark_all_tiles_dirty(tilemap[bank]);
	}

	/* other bits unknown */
if (data & 0x30)
	usrintf_showmessage("bankswitch %02x",data&0xf0);

//logerror("%04x: bankswitch_w %02x\n",cpu_get_pc(),data);
}

READ_HANDLER( hexion_bankedram_r )
{
	if (gfxrom_select && offset < 0x1000)
	{
		return memory_region(REGION_GFX1)[((gfxrom_select & 0x7f) << 12) + offset];
	}
	else if (bankctrl == 0)
	{
		return vram[rambank][offset];
	}
	else if (bankctrl == 2 && offset < 0x800)
	{
		return unkram[offset];
	}
	else
	{
//logerror("%04x: bankedram_r offset %04x, bankctrl = %02x\n",cpu_get_pc(),offset,bankctrl);
		return 0;
	}
}

WRITE_HANDLER( hexion_bankedram_w )
{
	if (bankctrl == 3 && offset == 0 && (data & 0xfe) == 0)
	{
//logerror("%04x: bankedram_w offset %04x, data %02x, bankctrl = %02x\n",cpu_get_pc(),offset,data,bankctrl);
		rambank = data & 1;
	}
	else if (bankctrl == 0)
	{
//logerror("%04x: bankedram_w offset %04x, data %02x, bankctrl = %02x\n",cpu_get_pc(),offset,data,bankctrl);
		if (vram[rambank][offset] != data)
		{
			vram[rambank][offset] = data;
			tilemap_mark_tile_dirty(tilemap[rambank],offset/4);
		}
	}
	else if (bankctrl == 2 && offset < 0x800)
	{
//logerror("%04x: unkram_w offset %04x, data %02x, bankctrl = %02x\n",cpu_get_pc(),offset,data,bankctrl);
		unkram[offset] = data;
	}
	else
logerror("%04x: bankedram_w offset %04x, data %02x, bankctrl = %02x\n",cpu_get_pc(),offset,data,bankctrl);
}

WRITE_HANDLER( hexion_bankctrl_w )
{
//logerror("%04x: bankctrl_w %02x\n",cpu_get_pc(),data);
	bankctrl = data;
}

WRITE_HANDLER( hexion_gfxrom_select_w )
{
//logerror("%04x: gfxrom_select_w %02x\n",cpu_get_pc(),data);
	gfxrom_select = data;
}



/***************************************************************************

  Display refresh

***************************************************************************/

void hexion_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

	tilemap_draw(bitmap,tilemap[1],0,0);
	tilemap_draw(bitmap,tilemap[0],0,0);
}
