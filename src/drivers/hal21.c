/*
	Hal21 (possibly bad tile gfx ROMs)
	ASO (seems fine)
	Alpha Mission ('p3.6d' is a bad dump)

	todo:
	- hal21 gfx
	- hal21 colors
	- sound cpu status needs hooked up in both games
	- virtualize palette (background palette is bank selected) for further speedup

	Revisions:

	xx-xx-2002 Acho A. Tang

	[HAL21]
	- added sound
	- rewrote background and sprite drawing
	- improved color
	- modified hardware profile and DIP settings
	* When you kill a boss and happen to beat the high score or receive a 1up,
	  the bonus tune will halt the music until the next music change.(a game bug?)
	* The real HAL21 board has a low-pass filter to supress rings and scratches.
*/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


extern void tnk3_draw_text( struct mame_bitmap *bitmap, int bank, unsigned char *source );
extern void tnk3_draw_status( struct mame_bitmap *bitmap, int bank, unsigned char *source );
static int hal21_id; //AT
static int scrollx_base; /* this is the only difference in video hardware found so far */

static VIDEO_START( common ){
	dirtybuffer = auto_malloc( 64*64 );
	if( !dirtybuffer )
		return 1;
	tmpbitmap = auto_bitmap_alloc( 512, 512 );
	if( !tmpbitmap )
		return 1;
	memset( dirtybuffer, 1, 64*64  );
	return 0;
}

VIDEO_START( aso ){
	scrollx_base = -16;
	return video_start_common();
}

VIDEO_START( hal21 ){
	hal21_id = 1; //AT
	scrollx_base = -16; //AT
	return video_start_common();
}

PALETTE_INIT( aso )
{
	int i;
	int num_colors = 1024;
/* palette format is RRRG GGBB B??? the three unknown bits are used but */
/* I'm not sure how, I'm currently using them as least significant bit but */
/* that's most likely wrong. */
	for( i=0; i<num_colors; i++ ){
		int bit0=0,bit1,bit2,bit3,r,g,b;

		bit0 = (color_prom[i + 2*num_colors] >> 2) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[i + 2*num_colors] >> 1) & 0x01;
		bit1 = (color_prom[i + num_colors] >> 2) & 0x01;
		bit2 = (color_prom[i + num_colors] >> 3) & 0x01;
		bit3 = (color_prom[i] >> 0) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[i + 2*num_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + 2*num_colors] >> 3) & 0x01;
		bit2 = (color_prom[i + num_colors] >> 0) & 0x01;
		bit3 = (color_prom[i + num_colors] >> 1) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}
//AT: sprite highlights are set by the video hardware
	if (!strcmp(Machine->gamedrv->name,"hal21") || !strcmp(Machine->gamedrv->name,"hal21j"))
	{
		UINT8 r, g, b;

		palette_get_color(384, &r, &g, &b);

		for (i=6; i<0x80; i+=8)
			palette_set_color(i, r, g, b);
	}
//ZT
}

//AT: needs verification
static void hal21_draw_background( struct mame_bitmap *bitmap, int scrollx, int scrolly, int attrs,
								   const struct GfxElement *gfx )
{
	static int color[2] = {8, 8};
	struct rectangle *cliprect;
	int c, bankbase, tile_number, x, y, dx, dy, sx, sy, offs, offsx, offsy;

	cliprect = &Machine->visible_area;
	bankbase = attrs<<3 & 0x100;

	c = attrs & 0x0f;
	if (c > 11) { fillbitmap(bitmap,Machine->pens[(c<<4)+8], cliprect); return; }
	if (c<8 || color[0]<14 || bankbase)
	{
		c ^= 0x08;
		color[0] = c;
		color[1] = (c & 0x08) ? c : 8;
	}

	offsx = ((scrollx>>3) + 2) & 0x3f;
	dx = -(scrollx & 7) + 16;
	offsy = ((scrolly>>3) + 0) & 0x3f;
	dy = -(scrolly & 7) + 0;

	for (x=0; x<33; x++)
		for (y=0; y<28; y++)
		{
			offs = (((offsx+x)&0x3f)<<6) + ((offsy+y)&0x3f);
			tile_number = bankbase + videoram[offs];
			c = color[tile_number<0x40];
			sx = (x<<3) + dx;
			sy = (y<<3) + dy;
			drawgfx(bitmap, gfx,
				tile_number, c,
				0, 0,
				sx, sy,
				cliprect, TRANSPARENCY_NONE, 0);
		}
}

static void hal21_draw_sprites( struct mame_bitmap *bitmap, int xscroll, int yscroll,
								const struct GfxElement *gfx )
{
	UINT8 *sprptr, *endptr;
	struct rectangle *cliprect;
	int attrs, tile, x, y, color, fy ,data;

	cliprect = &Machine->visible_area;
	sprptr = spriteram;
	endptr = spriteram + 0x100;

	for (; sprptr<endptr; sprptr+=4)
	{
		data = *(int*)sprptr;
		if (data && data != -1)
		{
			attrs = sprptr[3];
			tile = sprptr[1] + (attrs<<2 & 0x100);
			color = attrs & 0x0f;
			fy = attrs & 0x20;
			y = (sprptr[0] + (attrs<<4 & 0x100) - yscroll) & 0x1ff;
			x = (0x100 - (sprptr[2] + (attrs<<1 & 0x100) - xscroll)) & 0x1ff;
			drawgfx(bitmap, gfx,
					tile, color,
					0, fy,
					x, y,
					cliprect, TRANSPARENCY_PEN, 7);
		}
	}
}
//ZT

static void aso_draw_background(
		struct mame_bitmap *bitmap,
		int scrollx, int scrolly,
		int bank, int color,
		const struct GfxElement *gfx )
{
	const struct rectangle *clip = &Machine->visible_area;
	int offs;

	static int old_bank, old_color;

	if( color!=old_color || bank!=old_bank ){
		memset( dirtybuffer, 1, 64*64  );
		old_bank = bank;
		old_color = color;
	}

	for( offs=0; offs<64*64; offs++ ){
		if( dirtybuffer[offs] ){
			int tile_number = videoram[offs]+bank*256;
			int sy = (offs%64)*8;
			int sx = (offs/64)*8;

			drawgfx( tmpbitmap,gfx,
				tile_number,
				color,
				0,0, /* no flip */
				sx,sy,
				0,TRANSPARENCY_NONE,0);

			dirtybuffer[offs] = 0;
		}
	}

	copyscrollbitmap(bitmap,tmpbitmap,
		1,&scrollx,1,&scrolly,
		clip,
		TRANSPARENCY_NONE,0);
}

void aso_draw_sprites(
		struct mame_bitmap *bitmap,
		int xscroll, int yscroll,
		const struct GfxElement *gfx
){
	const unsigned char *source = spriteram;
	const unsigned char *finish = source+60*4;

	struct rectangle clip = Machine->visible_area;

	while( source<finish ){
		int attributes = source[3]; /* YBBX.CCCC */
		int tile_number = source[1];
		int sy = source[0] + ((attributes&0x10)?256:0) - yscroll;
		int sx = source[2] + ((attributes&0x80)?256:0) - xscroll;
		int color = attributes&0xf;

		if( !(attributes&0x20) ) tile_number += 512;
		if( attributes&0x40 ) tile_number += 256;

		drawgfx(bitmap,gfx,
			tile_number,
			color,
			0,0,
			(256-sx)&0x1ff,sy&0x1ff,
			&clip,TRANSPARENCY_PEN,7);

		source+=4;
	}
}

int hal21_vreg[6];

WRITE_HANDLER( hal21_vreg0_w ){ hal21_vreg[0] = data; }
WRITE_HANDLER( hal21_vreg1_w ){ hal21_vreg[1] = data; }
WRITE_HANDLER( hal21_vreg2_w ){ hal21_vreg[2] = data; }
WRITE_HANDLER( hal21_vreg3_w ){ hal21_vreg[3] = data; }
WRITE_HANDLER( hal21_vreg4_w ){ hal21_vreg[4] = data; }
WRITE_HANDLER( hal21_vreg5_w ){ hal21_vreg[5] = data; }

VIDEO_UPDATE( aso ){
	unsigned char *ram = memory_region(REGION_CPU1);
	int attributes = hal21_vreg[1];

	{
//AT
#if 0 // old code
		unsigned char bg_attrs = hal21_vreg[0];
		int scrolly = -8+hal21_vreg[4]+((attributes&0x10)?256:0);
		int scrollx = scrollx_base + hal21_vreg[5]+((attributes&0x02)?0:256);

		aso_draw_background( bitmap, -scrollx, -scrolly,
			bg_attrs>>4, /* tile bank */
			bg_attrs&0xf, /* color bank */
			Machine->gfx[1]
		);
#endif
		int bg_attrs, scrollx, scrolly, shiftx;

		bg_attrs = (int)hal21_vreg[0];
		scrollx = scrollx_base + hal21_vreg[5];
		scrolly = (attributes<<4 & 0x100) + hal21_vreg[4] - 8;
		shiftx = attributes << 7;

		if (hal21_id)
		{
			scrollx += shiftx & 0x100;
			hal21_draw_background(bitmap, scrollx, scrolly, bg_attrs, Machine->gfx[1]);
		}
		else
		{
			scrollx += ~shiftx & 0x100;
			aso_draw_background(bitmap, -scrollx, -scrolly, bg_attrs>>4, bg_attrs&0xf, Machine->gfx[1]);
		}
//ZT
	}

	{
		int scrollx = 0x1e + hal21_vreg[3] + ((attributes&0x01)?256:0);
		int scrolly = -8+0x11+hal21_vreg[2] + ((attributes&0x08)?256:0);
		if (hal21_id) hal21_draw_sprites(bitmap, scrollx, scrolly, Machine->gfx[2]); else //AT
		aso_draw_sprites( bitmap, scrollx, scrolly, Machine->gfx[2] );
	}

	{
		int bank = (attributes&0x40)?1:0;
		tnk3_draw_text( bitmap, bank, &ram[0xf800] );
		tnk3_draw_status( bitmap, bank, &ram[0xfc00] );
	}
/*
	{
		int i;
		for( i=0; i<6; i++ ){
			int data = hal21_vreg[i];
			drawgfx( bitmap, Machine->uifont,
				"0123456789abcdef"[data>>4],0,0,0,
				0,i*16,
				&Machine->visible_area,
				TRANSPARENCY_NONE,0 );
			drawgfx( bitmap, Machine->uifont,
				"0123456789abcdef"[data&0xf],0,0,0,
				8,i*16,
				&Machine->visible_area,
				TRANSPARENCY_NONE,0 );
		}
	}
*/
}


INPUT_PORTS_START( hal21 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* sound CPU status */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START /* P1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* P2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
//AT
	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xc0, "20000 60000" )
	PORT_DIPSETTING(    0x80, "40000 90000" )
	PORT_DIPSETTING(    0x40, "50000 120000" )
	PORT_DIPSETTING(    0x00, "None" )

	PORT_START  /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Bonus Type" )
	PORT_DIPSETTING(    0x01, "Every Bonus Set" )
	PORT_DIPSETTING(    0x00, "Second Bonus Set" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, "Easy" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x18, 0x18, "Special" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x10, DEF_STR( Demo_Sounds) )
	PORT_DIPSETTING(    0x08, "Infinite Lives" )
	PORT_DIPSETTING(    0x00, "Freeze" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) ) // 0x20 -> fe65
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
//ZT
INPUT_PORTS_END

/**************************************************************************/

INPUT_PORTS_START( aso )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_VBLANK )  /* ? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Allow Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xc0, "50k 100k" )
	PORT_DIPSETTING(    0x80, "60k 120k" )
	PORT_DIPSETTING(    0x40, "100k 200k" )
	PORT_DIPSETTING(    0x00, "None" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bonus Occurrence" )
	PORT_DIPSETTING(    0x01, "1st & every 2nd" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX( 0x10,    0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Cheat of some kind", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Start Area" )
	PORT_DIPSETTING(    0xc0, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x00, "4" )
INPUT_PORTS_END


/**************************************************************************/

static struct GfxLayout char256 = {
	8,8,
	0x100,
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	256
};

static struct GfxLayout char1024 = {
	8,8,
	0x400,
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	256
};

static struct GfxLayout sprite1024 = {
	16,16,
	0x400,
	3,
	{ 2*1024*256,1*1024*256,0*1024*256 },
	{
		7,6,5,4,3,2,1,0,
		15,14,13,12,11,10,9,8
	},
	{
		0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
	},
	256
};

static struct GfxDecodeInfo aso_gfxdecodeinfo[] =
{
	/* colors 512-1023 are currently unused, I think they are a second bank */
	{ REGION_GFX1, 0, &char256,    128*3,  8 }, /* colors 384..511 */
	{ REGION_GFX2, 0, &char1024,   128*1, 16 }, /* colors 128..383 */
	{ REGION_GFX3, 0, &sprite1024, 128*0, 16 }, /* colors   0..127 */
	{ -1 }
};

/**************************************************************************/

#define SNK_NMI_ENABLE  1
#define SNK_NMI_PENDING 2

static int snk_soundcommand = 0;
static unsigned char *shared_ram, *shared_auxram;

static READ_HANDLER( shared_auxram_r ){ return shared_auxram[offset]; }
static WRITE_HANDLER( shared_auxram_w ){ shared_auxram[offset] = data; }

static READ_HANDLER( shared_ram_r ){ return shared_ram[offset]; }
static WRITE_HANDLER( shared_ram_w ){ shared_ram[offset] = data; }

static int CPUA_latch = 0;
static int CPUB_latch = 0;

static WRITE_HANDLER( CPUA_int_enable_w ){
	if( CPUA_latch & SNK_NMI_PENDING ){
		cpu_set_irq_line( 0, IRQ_LINE_NMI, PULSE_LINE );
		CPUA_latch = 0;
	}
	else {
		CPUA_latch |= SNK_NMI_ENABLE;
	}
}

static READ_HANDLER( CPUA_int_trigger_r ){
	if( CPUA_latch&SNK_NMI_ENABLE ){
		cpu_set_irq_line( 0, IRQ_LINE_NMI, PULSE_LINE );
		CPUA_latch = 0;
	}
	else {
		CPUA_latch |= SNK_NMI_PENDING;
	}
	return 0xff;
}

static WRITE_HANDLER( CPUB_int_enable_w ){
	if( CPUB_latch & SNK_NMI_PENDING ){
		cpu_set_irq_line( 1, IRQ_LINE_NMI, PULSE_LINE );
		CPUB_latch = 0;
	}
	else {
		CPUB_latch |= SNK_NMI_ENABLE;
	}
}

static READ_HANDLER( CPUB_int_trigger_r ){
	if( CPUB_latch&SNK_NMI_ENABLE ){
		cpu_set_irq_line( 1, IRQ_LINE_NMI, PULSE_LINE );
		CPUB_latch = 0;
	}
	else {
		CPUB_latch |= SNK_NMI_PENDING;
	}
	return 0xff;
}

static WRITE_HANDLER( snk_soundcommand_w ){
	snk_soundcommand = data;
	cpu_set_irq_line( 2, 0, HOLD_LINE );
}

static READ_HANDLER( snk_soundcommand_r )
{
	int val = snk_soundcommand;
	snk_soundcommand = 0;
	return val;
}
//AT
static WRITE_HANDLER( hal21_soundcommand_w ){
	soundlatch_w(0, data);
	cpu_set_nmi_line(2, PULSE_LINE);
}
//ZT
/**************************************************************************/

static struct YM3526interface ym3526_interface ={
	1,          /* number of chips */
	4000000,    /* 4 MHz? (hand tuned) */
	//{ 50 }      /* (not supported) */
	{ 100 } //AT: 50% is too soft
};

static MEMORY_READ_START( aso_readmem_sound )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd000, snk_soundcommand_r },
	{ 0xf000, 0xf000, YM3526_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( aso_writemem_sound )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM3526_control_port_0_w }, /* YM3526 #1 control port? */
	{ 0xf001, 0xf001, YM3526_write_port_0_w },   /* YM3526 #1 write port?  */
MEMORY_END

/**************************************************************************/

static struct AY8910interface ay8910_interface = {
	2, /* number of chips */
	1500000, //AT: yields better tone
	{ 30,50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static MEMORY_READ_START( hal21_readmem_sound )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r }, //AT
	{ 0xc000, 0xc000, MRA_NOP }, // ack
MEMORY_END

static MEMORY_WRITE_START( hal21_writemem_sound )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xe000, 0xe000, AY8910_control_port_0_w },
	{ 0xe001, 0xe001, AY8910_write_port_0_w },
	{ 0xe002, 0xe002, MWA_NOP }, //AT: unknown
	{ 0xe008, 0xe008, AY8910_control_port_1_w },
	{ 0xe009, 0xe009, AY8910_write_port_1_w },
MEMORY_END

/**************************** ASO/Alpha Mission *************************/

static MEMORY_READ_START( aso_readmem_cpuA )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc000, input_port_0_r }, /* coin, start */
	{ 0xc100, 0xc100, input_port_1_r }, /* P1 */
	{ 0xc200, 0xc200, input_port_2_r }, /* P2 */
	{ 0xc500, 0xc500, input_port_3_r }, /* DSW1 */
	{ 0xc600, 0xc600, input_port_4_r }, /* DSW2 */
	{ 0xc700, 0xc700, CPUB_int_trigger_r },
	{ 0xd000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( aso_writemem_cpuA )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc400, 0xc400, snk_soundcommand_w },
	{ 0xc700, 0xc700, CPUA_int_enable_w },
	{ 0xc800, 0xc800, hal21_vreg1_w },
	{ 0xc900, 0xc900, hal21_vreg2_w },
	{ 0xca00, 0xca00, hal21_vreg3_w },
	{ 0xcb00, 0xcb00, hal21_vreg4_w },
	{ 0xcc00, 0xcc00, hal21_vreg5_w },
	{ 0xcf00, 0xcf00, hal21_vreg0_w },
	{ 0xd800, 0xdfff, MWA_RAM, &shared_auxram },
	{ 0xe000, 0xe7ff, MWA_RAM, &spriteram },
	{ 0xe800, 0xf7ff, videoram_w, &videoram },
	{ 0xf800, 0xffff, MWA_RAM, &shared_ram },
MEMORY_END

static MEMORY_READ_START( aso_readmem_cpuB )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc000, CPUA_int_trigger_r },
	{ 0xc800, 0xe7ff, shared_auxram_r },
	{ 0xe800, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, shared_ram_r },
MEMORY_END
static MEMORY_WRITE_START( aso_writemem_cpuB )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc000, CPUB_int_enable_w },
	{ 0xc800, 0xd7ff, shared_auxram_w },
	{ 0xd800, 0xe7ff, videoram_w },
	{ 0xe800, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xffff, shared_ram_w },
MEMORY_END

/**************************** HAL21 *************************/

static MEMORY_READ_START( hal21_readmem_CPUA )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc000, input_port_0_r }, /* coin, start */
	{ 0xc100, 0xc100, input_port_1_r }, /* P1 */
	{ 0xc200, 0xc200, input_port_2_r }, /* P2 */
	{ 0xc400, 0xc400, input_port_3_r }, /* DSW1 */
	{ 0xc500, 0xc500, input_port_4_r }, /* DSW2 */
	{ 0xc700, 0xc700, CPUB_int_trigger_r },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( hal21_writemem_CPUA )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc300, 0xc300, hal21_soundcommand_w }, //AT
	{ 0xc600, 0xc600, hal21_vreg0_w },
	{ 0xc700, 0xc700, CPUA_int_enable_w },
	{ 0xd300, 0xd300, hal21_vreg1_w },
	{ 0xd400, 0xd400, hal21_vreg2_w },
	{ 0xd500, 0xd500, hal21_vreg3_w },
	{ 0xd600, 0xd600, hal21_vreg4_w },
	{ 0xd700, 0xd700, hal21_vreg5_w },
	{ 0xe000, 0xefff, MWA_RAM, &spriteram },
	{ 0xf000, 0xffff, MWA_RAM, &shared_ram },
MEMORY_END

READ_HANDLER( hal21_spriteram_r ){
	return spriteram[offset];
}
WRITE_HANDLER( hal21_spriteram_w ){
	spriteram[offset] = data;
}

static MEMORY_READ_START( hal21_readmem_CPUB )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xc000, 0xcfff, hal21_spriteram_r },
	{ 0xd000, 0xdfff, MRA_RAM }, /* background */
	{ 0xe000, 0xefff, shared_ram_r },
MEMORY_END

static MEMORY_WRITE_START( hal21_writemem_CPUB )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa000, CPUB_int_enable_w },
	{ 0xc000, 0xcfff, hal21_spriteram_w },
	{ 0xd000, 0xdfff, videoram_w, &videoram },
	{ 0xe000, 0xefff, shared_ram_w },
MEMORY_END

/**************************************************************************/

static MACHINE_DRIVER_START( aso )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000) /* ? */
	MDRV_CPU_MEMORY(aso_readmem_cpuA,aso_writemem_cpuA)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000) /* ? */
	MDRV_CPU_MEMORY(aso_readmem_cpuB,aso_writemem_cpuB)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)   /* 4 MHz (?) */
	MDRV_CPU_MEMORY(aso_readmem_sound,aso_writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)    /* CPU slices per frame */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 1*8, 28*8-1)
	MDRV_GFXDECODE(aso_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_PALETTE_INIT(aso)
	MDRV_VIDEO_START(aso)
	MDRV_VIDEO_UPDATE(aso)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3526, ym3526_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hal21 )
//AT
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_MEMORY(hal21_readmem_CPUA,hal21_writemem_CPUA)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_MEMORY(hal21_readmem_CPUB,hal21_writemem_CPUB)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(hal21_readmem_sound,hal21_writemem_sound)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,220) //AT: music tempo, hand tuned
//ZT
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)    /* CPU slices per frame */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 1*8, 28*8-1)
	MDRV_GFXDECODE(aso_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_PALETTE_INIT(aso)
	MDRV_VIDEO_START(hal21)
	MDRV_VIDEO_UPDATE(aso)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

/**************************************************************************/

ROM_START( hal21 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )   /* 64k for CPUA code */
	ROM_LOAD( "hal21p1.bin",    0x0000, 0x2000, CRC(9d193830) SHA1(8e4e9c8bc774d7c7c0b68a5fa5cabdc6b5cfa41b) )
	ROM_LOAD( "hal21p2.bin",    0x2000, 0x2000, CRC(c1f00350) SHA1(8709455a980931565ccca60162a04c6c3133099b) )
	ROM_LOAD( "hal21p3.bin",    0x4000, 0x2000, CRC(881d22a6) SHA1(4b2a65dc18620f7f77532f791212fccfe1f0b245) )
	ROM_LOAD( "hal21p4.bin",    0x6000, 0x2000, CRC(ce692534) SHA1(e1d8e6948578ec9d0b6dc2aff17ad23b8ce46d6a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )   /* 64k for CPUB code */
	ROM_LOAD( "hal21p5.bin",    0x0000, 0x2000, CRC(3ce0684a) SHA1(5e76770a3252d5565a8f11a79ac3a9a6c31a43e2) )
	ROM_LOAD( "hal21p6.bin",    0x2000, 0x2000, CRC(878ef798) SHA1(0aae152947c9c6733b77dd1ac14f2f6d6bfabeaa) )
	ROM_LOAD( "hal21p7.bin",    0x4000, 0x2000, CRC(72ebbe95) SHA1(b1f7dc535e7670647500391d21dfa971d5e342a2) )
	ROM_LOAD( "hal21p8.bin",    0x6000, 0x2000, CRC(17e22ad3) SHA1(0e10a3c0f2e2ec284f4e0f1055397a8ccd1ff0f7) )
	ROM_LOAD( "hal21p9.bin",    0x8000, 0x2000, CRC(b146f891) SHA1(0b2db3e14b0401a7914002c6f7c26933a1cba162) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )   /* 64k for sound code */
	ROM_LOAD( "hal21p10.bin",   0x0000, 0x4000, CRC(916f7ba0) SHA1(7b8bcd59d768c4cd226de96895d3b9755bb3ba79) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hal21p12.bin", 0x0000, 0x2000, CRC(9839a7cd) SHA1(d3f9d964263a64aa3648faf5eb2e4fa532ae7852) ) /* char */

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE ) /* background tiles */
	ROM_LOAD( "hal21p11.bin", 0x0000, 0x4000, CRC(24abc57e) SHA1(1d7557a62adc059fb3fe20a09be18c2f40441581) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE ) /* 16x16 sprites */
	ROM_LOAD( "hal21p13.bin", 0x00000, 0x4000, CRC(052b4f4f) SHA1(032eb5771d33defce86e222f3e7aa22bc37db6db) )
	ROM_RELOAD(               0x04000, 0x4000 )
	ROM_LOAD( "hal21p14.bin", 0x08000, 0x4000, CRC(da0cb670) SHA1(1083bdd3488dfaa5094a2ef52cfc4206f35c9612) )
	ROM_RELOAD(               0x0c000, 0x4000 )
	ROM_LOAD( "hal21p15.bin", 0x10000, 0x4000, CRC(5c5ea945) SHA1(f9ce206cab4fad1f6478d731d4b096ec33e7b99f) )
	ROM_RELOAD(               0x14000, 0x4000 )

	ROM_REGION( 0x0c00, REGION_PROMS, 0 ) //AT: corrected PROM order
	ROM_LOAD( "hal21_3.prm",  0x000, 0x400, CRC(605afff8) SHA1(94e80ebd574b1580dac4a2aebd57e3e767890c0d) )
	ROM_LOAD( "hal21_2.prm",  0x400, 0x400, CRC(c5d84225) SHA1(cc2cd32f81ed7c1bcdd68e91d00f8081cb706ce7) )
	ROM_LOAD( "hal21_1.prm",  0x800, 0x400, CRC(195768fc) SHA1(c88bc9552d57d52fb4b030d118f48fedccf563f4) )
ROM_END

ROM_START( hal21j )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )   /* 64k for CPUA code */
	ROM_LOAD( "hal21p1.bin",    0x0000, 0x2000, CRC(9d193830) SHA1(8e4e9c8bc774d7c7c0b68a5fa5cabdc6b5cfa41b) )
	ROM_LOAD( "hal21p2.bin",    0x2000, 0x2000, CRC(c1f00350) SHA1(8709455a980931565ccca60162a04c6c3133099b) )
	ROM_LOAD( "hal21p3.bin",    0x4000, 0x2000, CRC(881d22a6) SHA1(4b2a65dc18620f7f77532f791212fccfe1f0b245) )
	ROM_LOAD( "hal21p4.bin",    0x6000, 0x2000, CRC(ce692534) SHA1(e1d8e6948578ec9d0b6dc2aff17ad23b8ce46d6a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )   /* 64k for CPUB code */
	ROM_LOAD( "hal21p5.bin",    0x0000, 0x2000, CRC(3ce0684a) SHA1(5e76770a3252d5565a8f11a79ac3a9a6c31a43e2) )
	ROM_LOAD( "hal21p6.bin",    0x2000, 0x2000, CRC(878ef798) SHA1(0aae152947c9c6733b77dd1ac14f2f6d6bfabeaa) )
	ROM_LOAD( "hal21p7.bin",    0x4000, 0x2000, CRC(72ebbe95) SHA1(b1f7dc535e7670647500391d21dfa971d5e342a2) )
	ROM_LOAD( "hal21p8.bin",    0x6000, 0x2000, CRC(17e22ad3) SHA1(0e10a3c0f2e2ec284f4e0f1055397a8ccd1ff0f7) )
	ROM_LOAD( "hal21p9.bin",    0x8000, 0x2000, CRC(b146f891) SHA1(0b2db3e14b0401a7914002c6f7c26933a1cba162) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )   /* 64k for sound code */
	ROM_LOAD( "hal21-10.bin",   0x0000, 0x4000, CRC(a182b3f0) SHA1(b76eff97a58a96467e9f3a74125a0a770e7678f8) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hal21p12.bin", 0x0000, 0x2000, CRC(9839a7cd) SHA1(d3f9d964263a64aa3648faf5eb2e4fa532ae7852) ) /* char */

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE ) /* background tiles */
	ROM_LOAD( "hal21p11.bin", 0x0000, 0x4000, CRC(24abc57e) SHA1(1d7557a62adc059fb3fe20a09be18c2f40441581) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE ) /* 16x16 sprites */
	ROM_LOAD( "hal21p13.bin", 0x00000, 0x4000, CRC(052b4f4f) SHA1(032eb5771d33defce86e222f3e7aa22bc37db6db) )
	ROM_RELOAD(               0x04000, 0x4000 )
	ROM_LOAD( "hal21p14.bin", 0x08000, 0x4000, CRC(da0cb670) SHA1(1083bdd3488dfaa5094a2ef52cfc4206f35c9612) )
	ROM_RELOAD(               0x0c000, 0x4000 )
	ROM_LOAD( "hal21p15.bin", 0x10000, 0x4000, CRC(5c5ea945) SHA1(f9ce206cab4fad1f6478d731d4b096ec33e7b99f) )
	ROM_RELOAD(               0x14000, 0x4000 )

	ROM_REGION( 0x0c00, REGION_PROMS, 0 ) //AT: corrected PROM order
	ROM_LOAD( "hal21_3.prm",  0x000, 0x400, CRC(605afff8) SHA1(94e80ebd574b1580dac4a2aebd57e3e767890c0d) )
	ROM_LOAD( "hal21_2.prm",  0x400, 0x400, CRC(c5d84225) SHA1(cc2cd32f81ed7c1bcdd68e91d00f8081cb706ce7) )
	ROM_LOAD( "hal21_1.prm",  0x800, 0x400, CRC(195768fc) SHA1(c88bc9552d57d52fb4b030d118f48fedccf563f4) )
ROM_END

ROM_START( aso )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )   /* 64k for cpuA code */
	ROM_LOAD( "aso.1",    0x0000, 0x8000, CRC(3fc9d5e4) SHA1(1318904d3d896affd5affd8e475ac9ee6929b955) )
	ROM_LOAD( "aso.3",    0x8000, 0x4000, CRC(39a666d2) SHA1(b5426520eb600d44bc5566d742d7b88194076494) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )   /* 64k for cpuB code */
	ROM_LOAD( "aso.4",    0x0000, 0x8000, CRC(2429792b) SHA1(674e81880f359f7e8d34d0ad9074267360afadbf) )
	ROM_LOAD( "aso.6",    0x8000, 0x4000, CRC(c0bfdf1f) SHA1(65b15ce9c2e78df79cb603c58639421d29701633) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )   /* 64k for sound code */
	ROM_LOAD( "aso.7",    0x0000, 0x8000, CRC(49258162) SHA1(c265b79d012be1e065389f910f7b4ce61f5b27ce) )  /* YM3526 */
	ROM_LOAD( "aso.9",    0x8000, 0x4000, CRC(aef5a4f4) SHA1(e908e79e27ff892fe75d1ba5cb0bc9dc6b7b4268) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "aso.14",   0x0000, 0x2000, CRC(8baa2253) SHA1(e6e4a5aa005e89744c4e2a19a080cf322edc6b52) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE ) /* background tiles */
	ROM_LOAD( "aso.10",   0x0000, 0x8000, CRC(00dff996) SHA1(4f6ce4c0f2da0d2a711bcbf9aa998b4e31d0d9bf) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE ) /* 16x16 sprites */
	ROM_LOAD( "aso.11",   0x00000, 0x8000, CRC(7feac86c) SHA1(13b81f006ec587583416c1e7432da4c3f0375924) )
	ROM_LOAD( "aso.12",   0x08000, 0x8000, CRC(6895990b) SHA1(e84554cae9a768021c3dc7183bc3d28e2dd768ee) )
	ROM_LOAD( "aso.13",   0x10000, 0x8000, CRC(87a81ce1) SHA1(28c1069e6c08ecd579f99620c1cb6df01ad1aa74) )

	ROM_REGION( 0x0c00, REGION_PROMS, 0 )
	ROM_LOAD( "up02_f12.rom",  0x000, 0x00400, CRC(5b0a0059) SHA1(f61e17c8959f1cd6cc12b38f2fb7c6190ebd0e0c) )
	ROM_LOAD( "up02_f13.rom",  0x400, 0x00400, CRC(37e28dd8) SHA1(681726e490872a574dd0295823a44d64ef3a7b45) )
	ROM_LOAD( "up02_f14.rom",  0x800, 0x00400, CRC(c3fd1dd3) SHA1(c48030cc458f0bebea0ffccf3d3c43260da6a7fb) )
ROM_END



GAMEX( 1985, aso,    0,     aso,   aso,   0, ROT270, "SNK", "ASO - Armored Scrum Object", GAME_IMPERFECT_SOUND )
GAMEX( 1985, hal21,  0,     hal21, hal21, 0, ROT270, "SNK", "HAL21", GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND )
GAMEX( 1985, hal21j, hal21, hal21, hal21, 0, ROT270, "SNK", "HAL21 (Japan)", GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND )
