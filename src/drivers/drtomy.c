/*

Dr. Tomy

A rip-off of Dr. Mario by Playmark, using some code from Gaelco's Big Karnak on
similar hardware.

Quote of the driver ----
<Sophitia> Dr Tomy is not in great condition
<Sophitia> it was sent in a pizza box...

this is a copy of things from gaelco.c
tilemaps might not be so similar ...

do sprites /really/ work like that? seems very strange..

*/

#include "driver.h"
#include "sound/okim6295.h"

data16_t *drtomy_spriteram;


/***************************************************************************

	Sprites - from gaelco.c

***************************************************************************/
int sprite_count[5];
int *sprite_table[5];

static void gaelco_sort_sprites(void)
{
	int i;

	sprite_count[0] = 0;
	sprite_count[1] = 0;
	sprite_count[2] = 0;
	sprite_count[3] = 0;
	sprite_count[4] = 0;

	for (i = 3; i < (0x1000 - 6)/2; i += 4){
		int color = (drtomy_spriteram[i+2] & 0x7e00) >> 9;
		int priority = (drtomy_spriteram[i] & 0x3000) >> 12;

		/* palettes 0x38-0x3f are used for high priority sprites in Big Karnak */
		if (color >= 0x38){
			sprite_table[4][sprite_count[4]] = i;
			sprite_count[4]++;
		}

		/* save sprite number in the proper array for later */
		sprite_table[priority][sprite_count[priority]] = i;
		sprite_count[priority]++;
	}
}

/*
	Sprite Format
	-------------

	Word | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | -------- xxxxxxxx | y position
	  0  | -----xxx -------- | not used
	  0  | ----x--- -------- | sprite size
	  0  | --xx---- -------- | sprite priority
	  0  | -x------ -------- | flipx
	  0  | x------- -------- | flipy
	  1  | xxxxxxxx xxxxxxxx | not used
	  2  | -------x xxxxxxxx | x position
	  2  | -xxxxxx- -------- | sprite color
	  3	 | -------- ------xx | sprite code (8x8 cuadrant)
	  3  | xxxxxxxx xxxxxx-- | sprite code
*/

static void gaelco_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri)
{
	int j, x, y, ex, ey;
	const struct GfxElement *gfx = Machine->gfx[0];

	static int x_offset[2] = {0x0,0x2};
	static int y_offset[2] = {0x0,0x1};

	for (j = 0; j < sprite_count[pri]; j++){
		int i = sprite_table[pri][j];
		int sx = drtomy_spriteram[i+2] & 0x01ff;
		int sy = (240 - (drtomy_spriteram[i] & 0x00ff)) & 0x00ff;
		int number = drtomy_spriteram[i+3];
		int color = (drtomy_spriteram[i+2] & 0x7e00) >> 9;
		int attr = (drtomy_spriteram[i] & 0xfe00) >> 9;

		int xflip = attr & 0x20;
		int yflip = attr & 0x40;
		int spr_size;

		if (attr & 0x04){
			spr_size = 1;
		}
		else{
			spr_size = 2;
			number &= (~3);
		}

		for (y = 0; y < spr_size; y++){
			for (x = 0; x < spr_size; x++){

				ex = xflip ? (spr_size-1-x) : x;
				ey = yflip ? (spr_size-1-y) : y;

				drawgfx(bitmap,gfx,number + x_offset[ex] + y_offset[ey],
						color,xflip,yflip,
						sx-0x0f+x*8,sy+y*8,
						cliprect,TRANSPARENCY_PEN,0);
			}
		}
	}
}


VIDEO_START(drtomy)
{
	int i;

	for (i = 0; i < 5; i++){
		sprite_table[i] = auto_malloc(512*sizeof(int));
		if (!sprite_table[i]){
			return 1;
		}
	}

	return 0;
}

VIDEO_UPDATE(drtomy)
{
	gaelco_sort_sprites();


	fillbitmap( bitmap, Machine->pens[0], cliprect );

	gaelco_draw_sprites(bitmap,cliprect,3);
	gaelco_draw_sprites(bitmap,cliprect,2);
	gaelco_draw_sprites(bitmap,cliprect,1);
	gaelco_draw_sprites(bitmap,cliprect,0);

}

static ADDRESS_MAP_START( drtomy_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM								/* ROM */
	AM_RANGE(0x100000, 0x101fff) AM_RAM								/* RAM */
	AM_RANGE(0x200000, 0x2007ff) AM_READWRITE(MRA16_RAM, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)/* Palette */
	AM_RANGE(0x440000, 0x440fff) AM_RAM AM_BASE(&drtomy_spriteram)			/* Sprite RAM */

	AM_RANGE(0x700000, 0x700001) AM_READ(input_port_0_word_r)/* DIPSW #2 */
	AM_RANGE(0x700002, 0x700003) AM_READ(input_port_1_word_r)/* DIPSW #1 */
	AM_RANGE(0x700004, 0x700005) AM_READ(input_port_2_word_r)/* INPUT #1 */
	AM_RANGE(0x700006, 0x700007) AM_READ(input_port_3_word_r)/* INPUT #2 */

	AM_RANGE(0xff0000, 0xffffff) AM_RAM							/* Work RAM */
ADDRESS_MAP_END

static struct GfxLayout tilelayout8=
{
	8,8,									/* 8x8 tiles */
	RGN_FRAC(1,4),									/* number of tiles */
	4,										/* bitplanes */
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static struct GfxLayout tilelayout16 =
{
	16,16,									/* 16x16 tiles */
	RGN_FRAC(1,4),									/* number of tiles */
	4,										/* bitplanes */
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7, 16*8+0,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout8,0,	16 },
	{ REGION_GFX1, 0, &tilelayout16,0,	16 },
	{ -1 }
};


static INPUT_PORTS_START( drtomy )
	PORT_START	/* DSW #2 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Sound Type" )
	PORT_DIPSETTING(    0x00, DEF_STR( Stereo ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Mono ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )

	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1C/1C or Free Play (if Coin A too)" )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1C/1C or Free Play (if Coin B too)" )

	PORT_START	/* 1P INPUTS & COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* 2P INPUTS & STARTSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END


static MACHINE_DRIVER_START( drtomy )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)			/* ? MHz */
	MDRV_CPU_PROGRAM_MAP(drtomy_map,0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 320-1, 16, 256-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(drtomy)
	MDRV_VIDEO_UPDATE(drtomy)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


ROM_START( drtomy )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE(	"15.u21",	0x000001, 0x020000, CRC(0b8d763b) SHA1(082005985a2de7b941ea227bbf6e761a197132e6) )
	ROM_LOAD16_BYTE(	"16.u22",	0x000000, 0x020000, CRC(206f4d65) SHA1(f4a28bc6041981d50a03477e63e90d5ff8ffb765) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "17.u83",	0x300000, 0x040000, CRC(42044b1c) SHA1(6fd01911932e0fb800ffefec595a9e7c524faa8f) )
	ROM_LOAD( "18.u82", 0x200000, 0x040000, CRC(8ee5c921) SHA1(6ba43eeb3b633c3db22f7b18b8fe91f250da2242) )
	ROM_LOAD( "19.u81",	0x100000, 0x040000, CRC(49ecbfe2) SHA1(16889663bdd3b7d0a350d5b18e221480413f6b4f) )
	ROM_LOAD( "20.u80",	0x000000, 0x040000, CRC(4d4d86ff) SHA1(60df0bf8ba62fea42ff756cd7c5485b57f597098) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "14.u23",	0x000000, 0x080000, CRC(479614ec) SHA1(b6300b344422f0a64146b853411f5285eaaada28) )
ROM_END


GAME( 1993, drtomy, 0,        drtomy, drtomy, 0, ROT0, "Playmark", "Dr Tomy" )
