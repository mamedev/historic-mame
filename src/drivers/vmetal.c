/*

Varia Metal

Notes:

It has Sega and Taito logos in the roms ?!

whats going on with the dipswitches
how should the rom reading work

are the gfx roms ok

sprites are non tile based

can't figure out the how the tiles work (again are the gfx roms ok?)

strange text layer .. skiiping tiles .. bleah ...

also has a strange sound chip

*/

#include "driver.h"

data16_t *vmetal_texttileram;
data16_t *vmetal_mid1tileram;
data16_t *vmetal_mid2tileram;

static struct tilemap *vmetal_texttilemap;
static struct tilemap *vmetal_mid1tilemap;
static struct tilemap *vmetal_mid2tilemap;

static data16_t *varia_spriteram16;

READ16_HANDLER ( varia_random )
{
	return rand();
}

static ADDRESS_MAP_START( varia_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x11ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x120000, 0x13ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x15ffff) AM_READ(MRA16_RAM)

	AM_RANGE(0x160000, 0x16ffff) AM_READ(varia_random) // cgrom read window ..

	AM_RANGE(0x170000, 0x171fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x172000, 0x173fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x174000, 0x17ffff) AM_READ(MRA16_RAM)

	AM_RANGE(0x200000, 0x200001) AM_READ(input_port_0_word_r )
	AM_RANGE(0x200002, 0x200003) AM_READ(input_port_1_word_r )

	AM_RANGE(0x400000, 0x400001) AM_READ(input_port_2_word_r )

	/* i have no idea whats meant to be going on here .. it seems to read one bit of the dips from some of them, protection ??? */

	AM_RANGE(0x30fffe, 0x30ffff) AM_READ(varia_random )

	AM_RANGE(0x317ffe, 0x317fff) AM_READ(varia_random )
	AM_RANGE(0x31bffe, 0x31bfff) AM_READ(varia_random )
	AM_RANGE(0x31dffe, 0x31dfff) AM_READ(varia_random )
	AM_RANGE(0x31effe, 0x31efff) AM_READ(varia_random )

	AM_RANGE(0x31f7fe, 0x31f7ff) AM_READ(varia_random )
	AM_RANGE(0x31fbfe, 0x31fbff) AM_READ(varia_random )
	AM_RANGE(0x31fdfe, 0x31fdff) AM_READ(varia_random )
	AM_RANGE(0x31fefe, 0x31feff) AM_READ(varia_random )

	AM_RANGE(0x31ff7e, 0x31ff7f) AM_READ(varia_random )
	AM_RANGE(0x31ffbe, 0x31ffbf) AM_READ(varia_random )
	AM_RANGE(0x31ffde, 0x31ffdf) AM_READ(varia_random )
	AM_RANGE(0x31ffee, 0x31ffef) AM_READ(varia_random )
	AM_RANGE(0x31fffe, 0x31ffff) AM_READ(varia_random )

	AM_RANGE(0x31fff6, 0x31fff7) AM_READ(varia_random )
	AM_RANGE(0x31fffa, 0x31fffb) AM_READ(varia_random )
	AM_RANGE(0x31fffc, 0x31fffd) AM_READ(varia_random )

	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static void varia_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	const struct GfxElement *gfx = Machine->gfx[1];
	data16_t *source = varia_spriteram16;
	data16_t *finish = source + 0x1000/2;

	while( source<finish )
	{
		int x,y;

		x = source[0]&0x1ff;
		y = source[1]& 0x1ff;


		drawgfx(bitmap,gfx,rand(),1,0,0,x,y,cliprect,TRANSPARENCY_PEN,0);


		source += 0x04;
	}
}

WRITE16_HANDLER( vmetal_texttileram_w )
{
	COMBINE_DATA(&vmetal_texttileram[offset]);
	tilemap_mark_tile_dirty(vmetal_texttilemap,offset);
}

WRITE16_HANDLER( vmetal_mid1tileram_w )
{
	COMBINE_DATA(&vmetal_mid1tileram[offset]);
	tilemap_mark_tile_dirty(vmetal_mid1tilemap,offset);
}
WRITE16_HANDLER( vmetal_mid2tileram_w )
{
	COMBINE_DATA(&vmetal_mid2tileram[offset]);
	tilemap_mark_tile_dirty(vmetal_mid2tilemap,offset);
}

static ADDRESS_MAP_START( varia_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x11ffff) AM_WRITE(vmetal_texttileram_w) AM_BASE(&vmetal_texttileram)
	AM_RANGE(0x120000, 0x13ffff) AM_WRITE(vmetal_mid1tileram_w) AM_BASE(&vmetal_mid1tileram)
	AM_RANGE(0x140000, 0x15ffff) AM_WRITE(vmetal_mid2tileram_w) AM_BASE(&vmetal_mid2tileram)

	AM_RANGE(0x170000, 0x171fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x172000, 0x173fff) AM_WRITE(paletteram16_RRRRRGGGGGBBBBBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x174000, 0x174fff) AM_WRITE(MWA16_RAM) AM_BASE(&varia_spriteram16)
	AM_RANGE(0x175000, 0x17ffff) AM_WRITE(MWA16_RAM)

	AM_RANGE(0x200000, 0x200001) AM_WRITE(MWA16_NOP)

	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM)

ADDRESS_MAP_END

INPUT_PORTS_START( varia )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START1 )


	PORT_START		/* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x0010, IP_ACTIVE_LOW )				// "Test Mode"
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown )  )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )


	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, "2" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown )  )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ 0,1,2,3 },
	{ RGN_FRAC(1,4)+4, RGN_FRAC(1,4)+0, RGN_FRAC(1,4)+12, RGN_FRAC(1,4)+8, RGN_FRAC(3,4)+4, RGN_FRAC(3,4)+0, RGN_FRAC(3,4)+12, RGN_FRAC(3,4)+8, 4, 0, 12, 8, RGN_FRAC(2,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+12, RGN_FRAC(2,4)+8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static struct GfxLayout charxlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, RGN_FRAC(2,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+12, RGN_FRAC(2,4)+8 },
	{ RGN_FRAC(1,4)+0*16,0*16, RGN_FRAC(1,4)+1*16, 1*16, RGN_FRAC(1,4)+2*16,  2*16, RGN_FRAC(1,4)+3*16, 3*16 },
	4*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x0, 512  }, /* bg tiles */
	{ REGION_GFX1, 0, &charxlayout,   0x0, 512  }, /* bg tiles */
	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 47 }
};

static void get_vmetal_texttilemap_tile_info(int tile_index)
{
	int tileno;
	tileno = vmetal_texttileram[tile_index] & 0x7ff;

	// this tile layout in the roms is *strange*

	tileno = (tileno & 0x0f) | ((tileno & 0xff0) << 1);

	SET_TILE_INFO(1,tileno+0x1f000,0xf0,0)
}

int tbb;

static void get_vmetal_mid1tilemap_tile_info(int tile_index)
{
	int tileno;
	tileno = vmetal_mid1tileram[tile_index] & 0xff;


	SET_TILE_INFO(0,tileno+tbb*0x100,1,TILE_FLIPYX(0x0))
}
static void get_vmetal_mid2tilemap_tile_info(int tile_index)
{
	int tileno;
	tileno = vmetal_mid2tileram[tile_index] & 0x0ff;



	SET_TILE_INFO(0,tileno+tbb*0x100,1,TILE_FLIPYX(0x0))
}

VIDEO_START(varia)
{
	vmetal_texttilemap = tilemap_create(get_vmetal_texttilemap_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,      8, 8, 256,256);
	vmetal_mid1tilemap = tilemap_create(get_vmetal_mid1tilemap_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,      16,16, 256,256);
	vmetal_mid2tilemap = tilemap_create(get_vmetal_mid2tilemap_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,      16,16, 256,256);
	tilemap_set_transparent_pen(vmetal_texttilemap,15);

	return 0;
}

VIDEO_UPDATE(varia)
{
	tilemap_draw(bitmap,cliprect,vmetal_mid2tilemap,0,0);

	tilemap_draw(bitmap,cliprect,vmetal_texttilemap,0,0);

	if ( keyboard_pressed_memory(KEYCODE_W) )
	{
		tbb ++;
		tilemap_mark_all_tiles_dirty (vmetal_mid1tilemap);
		tilemap_mark_all_tiles_dirty (vmetal_mid2tilemap);

	}

	varia_drawsprites (bitmap,cliprect);

}

static MACHINE_DRIVER_START( varia )
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(varia_readmem,varia_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1) // also level 3
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(2048, 2048)
	MDRV_VISIBLE_AREA(0, 511, 0, 511)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(varia)
	MDRV_VIDEO_UPDATE(varia)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



ROM_START( vmetal )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "vm5.bin", 0x00001, 0x80000, CRC(43ef844e) SHA1(c673f34fcc9e406282c9008795b52d01a240099a) )
	ROM_LOAD16_BYTE( "vm6.bin", 0x00000, 0x80000, CRC(cb292ab1) SHA1(41fdfe67e6cb848542fd5aa0dfde3b1936bb3a28) )


	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD( "vm1.bin", 0x000000, 0x200000, CRC(b470c168) SHA1(c30462dc134da1e71a94b36ef96ecd65c325b07e) )
	ROM_LOAD( "vm2.bin", 0x200000, 0x200000, CRC(b36f8d60) SHA1(1676859d0fee4eb9897ce1601a2c9fd9a6dc4a43) )
	ROM_LOAD( "vm3.bin", 0x400000, 0x200000, CRC(00fca765) SHA1(ca9010bd7f59367e483868018db9a9abf871386e) )
	ROM_LOAD( "vm4.bin", 0x600000, 0x200000, CRC(5a25a49c) SHA1(c30781202ec882e1ec6adfb560b0a1075b3cce55) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "vm8.bin", 0x00000, 0x80000, CRC(c14c001c) SHA1(bad96b5cd40d1c34ef8b702262168ecab8192fb6) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* more samples? */
	ROM_LOAD( "vm7.bin", 0x00000, 0x200000, CRC(a88c52f1) SHA1(d74a5a11f84ba6b1042b33a2c156a1071b6fbfe1) )
ROM_END

GAMEX( 1995, vmetal, 0, varia, varia, 0, ROT270, "Excellent Systems", "Varia Metal", GAME_NOT_WORKING )
