/*
Quiz Panicuru Fantasy

- preliminary by David Haywood

- only two layers is hooked up .. other tilemaps is  just  after the first one
- no scroll , no colors, no tilebanks
- { 0x100000, 0x10000f, fake_r },//???? - dips ? status ?




NMK, 1993

PCB No: QZ93094
CPU   : TMP68000P-12
SOUND : Oki M6295
OSC   : 16.000MHz, 10.000MHz
RAM   : 62256 (x2), 6116 (x2), 6264 (x4)
DIPSW : 8 position (x2)
CUSTOM: NMK112 (QFP64, near ROMs 31,32,4, possible sound chip/CPU?)
        NMK111 (QFP64, 1x input-related near JAMMA, 2x gfx related near ROMs 11,12,21,22)
        NMK903 (QFP44, x2, near ROMs 11,12,21,22)
        NMK005 (QFP64, near DIPs, possible MCU?)

ROMs  :
        93094-51.127	27c4002     near 68000
        93094-52.126    27c4001     near 68000
        93094-53.125    27c1001     near 68000
        93090-4.56      8M Mask     oki samples
        93090-31.58     8M Mask     oki samples
        93090-32.57     8M Mask     oki samples
        93090-21.10     8M Mask     gfx
        93090-22.9      8M Mask     gfx
        93090-11.2      8M Mask     gfx
        93090-12.1      8M Mask     gfx
        QZ6.88          82s129      prom
        QZ7.99          82s129      prom
        QZ8.121         82s135      prom

*/

#include "driver.h"

data16_t *quizpani_bg_videoram;
data16_t *quizpani_fg_videoram;



data16_t *bgvideoram;
static struct tilemap *bg_tilemap,*fg_tilemap;

int bgbank=3,fgbank=2;

static void bg_tile_info(int tile_index)
{
	int code = bgvideoram[tile_index];
	SET_TILE_INFO(
			1,
			(code & 0xfff)+0x1000*bgbank,
			code >> 12,
			0)
}

static void fg_tile_info(int tile_index)
{
	int code = bgvideoram[tile_index+16*256];//test
	SET_TILE_INFO(
			1,
			(code & 0xfff)+0x1000*fgbank,
			code >> 12,
			0)
}



WRITE16_HANDLER( bgvideoram_w )
{
	bgvideoram[offset]=data;
	tilemap_mark_all_tiles_dirty(bg_tilemap);
	tilemap_mark_all_tiles_dirty(fg_tilemap);
}

READ16_HANDLER( bgvideoram_r )
{
	return bgvideoram[offset];
}




VIDEO_START( quizpani )
{
	bg_tilemap = tilemap_create(bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,16,16,256,16);//?
	fg_tilemap = tilemap_create(fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,256,16);//?
	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

static int map=0;

VIDEO_UPDATE( quizpani )
{
	if(!map)
		tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	else
		tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);

	if (keyboard_pressed_memory(KEYCODE_W))
		map^=1;


 	if (keyboard_pressed_memory(KEYCODE_Q))
 	{
 		bgbank++;
 		fgbank++;
 		tilemap_mark_all_tiles_dirty(bg_tilemap);
 		tilemap_mark_all_tiles_dirty(fg_tilemap);
 		printf("%x\n",fgbank);
 	}

}


static READ16_HANDLER(fake_r)
{
	return 0xffff;
}

static MEMORY_READ16_START( quizpani_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },

	{ 0x100000, 0x10000f, fake_r },//????

	{ 0x108000, 0x108fff, MRA16_RAM },
	{ 0x10c000, 0x10ffff, MRA16_RAM },
	{ 0x110000, 0x11ffff, bgvideoram_r },


	{ 0x180000, 0x18ffff, MRA16_RAM },
	{ 0xf00000, 0xffffff, MRA16_ROM },
MEMORY_END

static MEMORY_WRITE16_START( quizpani_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },

	{ 0x104000, 0x104001, MWA16_NOP },

	{ 0x108000, 0x108fff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x10c000, 0x10ffff, MWA16_RAM },
	{ 0x110000, 0x11ffff, bgvideoram_w, &bgvideoram },//at least two layers

	{ 0x180000, 0x18ffff, MWA16_RAM },


	{ 0xf00000, 0xffffff, MWA16_ROM },
MEMORY_END

INPUT_PORTS_START( quizpani ) // not done
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	// "Rotate" - also IPT_START1
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	// "Help"
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )	// "Rotate" - also IPT_START2
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	// "Help"
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Helps" )			// "Power Count" in test mode
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPSETTING(      0x0400, "1" )
	PORT_DIPNAME( 0x0800, 0x0800, "Bonus Bar Level" )
	PORT_DIPSETTING(      0x0800, "Normal" )
	PORT_DIPSETTING(      0x0000, "High" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Picture View" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			16*32+0*4, 16*32+1*4, 16*32+2*4, 16*32+3*4, 16*32+4*4, 16*32+5*4, 16*32+6*4, 16*32+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};






static struct GfxDecodeInfo gfxdecodeinfo[] =
{

	{ REGION_GFX1, 0, &tilelayout,   0x0, 16  }, /* bg tiles */
	{ REGION_GFX2, 0, &tilelayout,  0x0, 16  }, /* fg tiles */

	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 47 }
};


static MACHINE_DRIVER_START( quizpani )
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(quizpani_readmem,quizpani_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
//	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)
/*
4d4e
4d78
4d7a
4d82
4d7c
4d7e
*/
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(quizpani)
	MDRV_VIDEO_UPDATE(quizpani)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

ROM_START( quizpani )
	ROM_REGION( 0x1000000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "93094-51.127", 0x000000, 0x080000,  CRC(2b7a29d4) SHA1(f87b875e69410745ee46d5d94b6c28e5417afb0d) )

	// wrong?
	ROM_LOAD16_BYTE( "93094-52.126", 0xf00000, 0x080000, CRC(0617524e) SHA1(91ab5cb8a605c37c92632cf007ddb67172cc9863) )
	ROM_LOAD16_BYTE( "93094-53.125", 0xf00001, 0x020000, CRC(7e0ab49c) SHA1(dd10f723ef74f3153e04b1a271b8761585799aa6) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "93090-4.56",  0x000000, 0x100000,  CRC(ee370ed6) SHA1(9b1edfada5805014aa23d28d0c70227728b0e04f) )
	ROM_LOAD( "93090-32.57", 0x000000, 0x100000,  CRC(5d38f62e) SHA1(22fe95de6e1de1be0cec73b8163ab4283f2b8186) )
	ROM_LOAD( "93090-31.58", 0x000000, 0x100000,  CRC(1cce0e13) SHA1(43816762e7907a8ff4b5a7b8da9f799b5baa64d5) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "93090-12.1", 0x000000, 0x100000,  CRC(3f2ebfa5) SHA1(1c935d566f3980483356264a9216f9bf298bb815) )
	ROM_LOAD( "93090-11.2", 0x100000, 0x100000,  CRC(4b3ab155) SHA1(fc1210853ca262c42b927689cb8f04aca15de7d6))


	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* BG Tiles */
	ROM_LOAD( "93090-22.9", 0x000000, 0x100000, CRC(93382cd3) SHA1(6527e92f696c21aae65d008bb237231eaba7a105) )
	ROM_LOAD( "93090-21.10", 0x100000, 0x100000, CRC(63754242) SHA1(3698b89d8515b45b9bc0fff87ca94ab5c2b3d53a) )

	ROM_REGION( 0x200, REGION_PROMS, 0 ) /* BG Tiles */
	ROM_LOAD( "qz6.88", 0x000, 0x100,  CRC(19dbbad2) SHA1(ebf7950d1869ca3bc1e72228505fbc17d095746a) )
	ROM_LOAD( "qz7.99", 0x000, 0x100,  CRC(1f802af1) SHA1(617bb7e5105ac202b5a8cf83c8c66178b91099e0) )
	ROM_LOAD( "qz8.121", 0x000, 0x100, CRC(b4c19741) SHA1(a6d3686bad6ef2336463b89bc2d249003d9b4bcc) )
ROM_END

GAMEX( 1994, quizpani, 0, quizpani, quizpani, 0, ROT0, "NMK", "Quiz Panicuru Fantasy", GAME_NOT_WORKING )
