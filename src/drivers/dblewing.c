/* Double Wing */

/*

the least protected of the DE102 games?

-- you can't select your plane

Video Problems (currently hooked up to tumblep video, probably wrongly
-- gfx are wrong (double width sprites missing?)
-- tilemape are wrong

*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "decocrpt.h"
#include "vidhrdw/generic.h"
#include "deco16ic.h"

extern void deco102_decrypt(int region, int address_xor, int data_select_xor, int opcode_select_xor);


static void dblewing_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		sprite = spriteram16[offs+1];
		if (!sprite) continue;

		y = spriteram16[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spriteram16[offs+2];
		colour = (x >>9) & 0x1f;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
        x = 304 - x;

		if (x>320) continue;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flip_screen)
		{
			y=240-y;
			x=304-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[2],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					cliprect,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

static int dblewing_bank_callback(const int bank)
{
	return ((bank>>4) & 0x7) * 0x1000;
}

VIDEO_START(dblewing)
{
	if (deco16_1_video_init())
		return 1;

	deco16_set_tilemap_bank_callback(0,dblewing_bank_callback);
	deco16_set_tilemap_bank_callback(1,dblewing_bank_callback);

	return 0;
}

VIDEO_UPDATE(dblewing)
{
	flip_screen_set( deco16_pf12_control[0]&0x80 );
	deco16_pf12_update(deco16_pf1_rowscroll,deco16_pf2_rowscroll);

	fillbitmap(bitmap,Machine->pens[0x0],cliprect); /* not Confirmed */
	fillbitmap(priority_bitmap,0,NULL);

	deco16_tilemap_2_draw(bitmap,cliprect,0,0);
	dblewing_drawsprites(bitmap,cliprect);
	deco16_tilemap_1_draw(bitmap,cliprect,0,0);
}


static ADDRESS_MAP_START( dblewing_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM

	AM_RANGE(0x100000, 0x100fff) AM_READ(MRA16_RAM) AM_WRITE(deco16_pf1_data_w) AM_BASE(&deco16_pf1_data)
	AM_RANGE(0x102000, 0x102fff) AM_READ(MRA16_RAM) AM_WRITE(deco16_pf2_data_w) AM_BASE(&deco16_pf2_data)
	AM_RANGE(0x104000, 0x104fff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf1_rowscroll)
	AM_RANGE(0x106000, 0x106fff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf2_rowscroll)

	/* protection related? */
//  AM_RANGE(0x280104, 0x280105) AM_WRITE(MWA16_NOP) // ??
	AM_RANGE(0x2800ac, 0x2800ad) AM_READ(input_port_2_word_r) // dips
	AM_RANGE(0x280298, 0x280299) AM_READ(input_port_1_word_r) // vbl
	AM_RANGE(0x280476, 0x280477) AM_READ(input_port_1_word_r)
	AM_RANGE(0x2802B4, 0x2802B5) AM_READ(input_port_0_word_r) // inverted?
//  AM_RANGE(0x280330, 0x280331) AM_READ(MRA16_NOP) // sound?
	AM_RANGE(0x280380, 0x280381) AM_WRITE(MWA16_NOP) // sound

	AM_RANGE(0x284000, 0x284001) AM_RAM
	AM_RANGE(0x288000, 0x288001) AM_RAM
	AM_RANGE(0x28C000, 0x28C00f) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf12_control)
	AM_RANGE(0x300000, 0x3007ff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x320000, 0x3207ff) AM_READWRITE(MRA16_RAM,paletteram16_xxxxBBBBGGGGRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xff0000, 0xff3fff) AM_MIRROR(0xc000) AM_RAM
ADDRESS_MAP_END


static struct GfxLayout tile_8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static struct GfxLayout tile_16x16_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 256,257,258,259,260,261,262,263,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 24,8,16,0 },
	{ 512,513,514,515,516,517,518,519, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	32*32
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_8x8_layout,     0x000, 32 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,   0x000, 32 },	/* Tiles (16x16) */
	{ REGION_GFX2, 0, &spritelayout,        0x200, 32 },	/* Sprites (16x16) */
	{ -1 } /* end of array */
};

INPUT_PORTS_START( dblewing )
	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* 16bit */
	PORT_DIPNAME( 0x0001, 0x0001, "1" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, DEF_STR( Unknown ) )
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
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
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


static MACHINE_DRIVER_START( dblewing )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)	/* DE102 */
	MDRV_CPU_PROGRAM_MAP(dblewing_map,0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(dblewing)
	MDRV_VIDEO_UPDATE(dblewing)

	/* sound hardware */
MACHINE_DRIVER_END


/*

Double Wings (JPN Ver.)
(c)1993 Mitchell
DEC-22V0 (S-NK-3220)

Software is by Mitchell, but the PCB is pure Data East.
It's the standard DEC-22V0 PCB that runs many Data East games from around the mid 90's.

Data East ROM code = KP

CPU     :68000 ?
Sound   :TMPZ84C00AP-6,YM2151,OKI M6295, YM3014B
OSC     :28.0000MHz,32.2200MHz
RAM     :LH6168 x 1, CXK5814 x 6, CXK5864 x 4
DIP     :2 x 8 position
Other   :DATA EAST Chips (numbers scratched)
         --------------------------------------
         DATA EAST #?  9235EV 205941  VC5259-0001 JAPAN (confirmed #52) - 128 pin PQFP
         DATA EAST #?  DATA EAST 250 JAPAN - 128 Pin PQFP
         DATA EAST #?  24220F008 (confirmed #141) - 160 pin PQFP
         DATA EAST #?  L7A0717   9143  (confirmed #104 and definitely the CPU) - 100 pin PQFP

         PALs: PAL16L8 (x 2, VG-00, VG-01) between program ROMs and CPU
               PAL16L8 (x 1, VG-02) next to #52

         Small surface-mounted chip with number scratched off (28 pin SOP), but has number 9303K9200.
         A similar chip exists on Capt. America PCB and has the number 77 on it. Possibly the same chip?

KP_00-.3D    [547dc83e]
KP_01-.5D    [7a210c33]

KP_02-.10H   [def035fa]

KP_03-.16H   [5d7f930d]

MBE-00.14A   [e33f5c93]
MBE-01.16A   [ef452ad7]
MBE-02.8H    [5a6d3ac5]

*/

ROM_START( dblewing )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "kp_00-.3d",    0x000001, 0x040000, CRC(547dc83e) SHA1(f6f96bd4338d366f06df718093f035afabc073d1) )
	ROM_LOAD16_BYTE( "kp_01-.5d",    0x000000, 0x040000, CRC(7a210c33) SHA1(ced89140af6d6a1bc0ffb7728afca428ed007165) )

	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_DISPOSE ) // sound cpu?
	ROM_LOAD( "kp_02-.10h",    0x00000, 0x10000, CRC(def035fa) SHA1(fd50314e5c94c25df109ee52c0ce701b0ff2140c) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mbe-02.8h",    0x00000, 0x100000, CRC(5a6d3ac5) SHA1(738bb833e2c5d929ac75fe4e69ee0af88197d8a6) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mbe-00.14a",    0x000000, 0x100000, CRC(e33f5c93) SHA1(720904b54d02dace2310ac6bd07d5ed4bc4fd69c) )
	ROM_LOAD16_BYTE( "mbe-01.16a",    0x000001, 0x100000, CRC(ef452ad7) SHA1(7fe49123b5c2778e46104eaa3a2104ce09e05705) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "kp_03-.16h",    0x00000, 0x20000, CRC(5d7f930d) SHA1(ad23aa804ea3ccbd7630ade9b53fc3ea2718a6ec) )
ROM_END

static DRIVER_INIT( dblewing )
{
	deco56_decrypt(REGION_GFX1);
	deco102_decrypt(REGION_CPU1, 0x399d, 0x25, 0x3d);
}


GAMEX(1993, dblewing, 0,        dblewing, dblewing,  dblewing,  ROT90,"Mitchell", "Double Wings", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
