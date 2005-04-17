/* Data East Hardware using Custom Chip 102
   placeholder driver

  all games on this driver have encrypted code,
  they may also have additional protection but
  at this stage that is unknown.  the current
  hold-up is the encryption.

  They use Data East Custom Chip 'DE102' which is
  probably the main processor. It might be an
  encrypted 68000

  Pocket Gal Deluxe gives many clues to the
  workings of the protection but many only enough
  to figure out the first stage of the encryption
  (and maybe only for that one game)

  This driver may be split up when (if) the actual
  code is decrypted as the games are not on identical
  hardware


  The Pocket Gal Deluxe bootleg appears to be a decrypted
  version of the original game.  The copyright has been
  changed to Data West.

*/

#define DE102CPU M68000

#include "driver.h"
#include "decocrpt.h"

data16_t* paletteram16_2;
data16_t* pcktgaldb_fgram;
data16_t* pcktgaldb_sprites;

/* dummy vidhrdw */
VIDEO_START(deco102)
{
	return 0;
}

VIDEO_UPDATE(deco102)
{

}


VIDEO_UPDATE(pktgaldb)
{
	int x,y;
	int offset=0;
	int tileno;
	int colour;

	fillbitmap(bitmap, get_black_pen(), cliprect);

	/* the original has a tilemap? */

#if 0
	for (y=0;y<32;y++)
	{
		for (x=0;x<64;x++)
		{
			tileno = (pcktgaldb_fgram[offset]&0x0fff);
			colour = (pcktgaldb_fgram[offset]&0xf000)>>12;
			drawgfx(bitmap,Machine->gfx[0],tileno,colour,0,0,x*8,y*8,cliprect,TRANSPARENCY_PEN,0);
			offset++;
		}

	}
#endif

	/* the bootleg has lots of sprites instead?  - i bet this is easier with real bootleg gfx roms */
	for (offset = 0;offset<0x1600/2;offset+=8)
	{
		tileno =  pcktgaldb_sprites[offset+3] | (pcktgaldb_sprites[offset+2]<<16);
		colour =  pcktgaldb_sprites[offset+1]>>1;
		x = pcktgaldb_sprites[offset+0];
		y = pcktgaldb_sprites[offset+4];

		x-=0xc2;
		y&=0x1ff;
		y-=8;

		tileno -=0x1000;
		drawgfx(bitmap,Machine->gfx[1],tileno,colour,0,0,x,y,cliprect,TRANSPARENCY_PEN,0);
	}

	for (offset = 0x1600/2;offset<0x2000/2;offset+=8)
	{
		tileno =  pcktgaldb_sprites[offset+3] | (pcktgaldb_sprites[offset+2]<<16);
		colour =  pcktgaldb_sprites[offset+1]>>1;
		x = pcktgaldb_sprites[offset+0]&0x1ff;
		y = pcktgaldb_sprites[offset+4]&0x0ff;

		x-=0xc2;
		y&=0x1ff;
		y-=8;

		drawgfx(bitmap,Machine->gfx[2],tileno,colour,0,0,x,y,cliprect,TRANSPARENCY_PEN,0);
	}

	for (offset = 0x2000/2;offset<0x4000/2;offset+=8)
	{
		tileno =  pcktgaldb_sprites[offset+3] | (pcktgaldb_sprites[offset+2]<<16);
		colour =  pcktgaldb_sprites[offset+1]>>1;
		x = pcktgaldb_sprites[offset+0]&0x1ff;
		y = pcktgaldb_sprites[offset+4]&0x0ff;

		x-=0xc2;
		y&=0x1ff;
		y-=8;

		drawgfx(bitmap,Machine->gfx[0],tileno,colour,0,0,x,y,cliprect,TRANSPARENCY_PEN,0);
	}

}

INPUT_PORTS_START( deco102 )
	PORT_START	/* 8bit */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* 16bit */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
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

INPUT_PORTS_START( pktgaldb )
	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* 16bit */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	// Maybe Flip Screen
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_DIPSETTING(      0x0300, "4" )
	PORT_DIPSETTING(      0x0200, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Time"  )
	PORT_DIPSETTING(      0x0000, "60" )
	PORT_DIPSETTING(      0x0400, "80" )
	PORT_DIPSETTING(      0x0c00, "100" )
	PORT_DIPSETTING(      0x0800, "120" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

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
INPUT_PORTS_END


static ADDRESS_MAP_START( deco102_map, ADDRESS_SPACE_PROGRAM, 16 )
ADDRESS_MAP_END


/* Pocket Gal Deluxe (bootleg!) */

READ16_HANDLER( pckgaldx_unknown_r )
{
	return 0xffff;
}

WRITE16_HANDLER( paletteram16_xRGB_w )
{
	int pen;
	int r,g,b;
	data32_t paldat;

	COMBINE_DATA(&paletteram16[offset]);

	pen = offset/2;

	paldat = (paletteram16[pen*2] << 16) | (paletteram16[pen*2+1] << 0);

	r = ((paldat & 0x000000ff) >>0);
	g = ((paldat & 0x0000ff00) >>8);
	b = ((paldat & 0x00ff0000) >>16);

	palette_set_color(pen,r,g,b);
}

READ16_HANDLER( pckgaldx_protection_r )
{
	logerror("pckgaldx_protection_r address %06x\n",activecpu_get_pc());
	return -1;
}

static ADDRESS_MAP_START( pktgaldb_readmem, ADDRESS_SPACE_PROGRAM, 16 )

	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x100fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x102000, 0x102fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x130000, 0x130fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x120000, 0x123fff) AM_READ(MRA16_RAM)

	AM_RANGE(0x140006, 0x140007) AM_READ(pckgaldx_unknown_r) // sound?
	AM_RANGE(0x150006, 0x150007) AM_READ(pckgaldx_unknown_r) // sound?

//	AM_RANGE(0x160000, 0x167fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x170000, 0x17ffff) AM_READ(MRA16_RAM)

/*
cpu #0 (PC=0000A0DE): unmapped program memory word read from 00167DB2 & FFFF
cpu #0 (PC=00009DFA): unmapped program memory word read from 00167C4C & FFFF
cpu #0 (PC=00009E58): unmapped program memory word read from 00167842 & 00FF
cpu #0 (PC=00009E80): unmapped program memory word read from 00167842 & FF00
cpu #0 (PC=00009AFE): unmapped program memory word read from 00167842 & 00FF
cpu #0 (PC=00009B12): unmapped program memory word read from 00167842 & FF00
cpu #0 (PC=0000923C): unmapped program memory word read from 00167DB2 & 00FF
*/

	/* do the 300000 addresses somehow interact with the protection addresses on this bootleg? */

	AM_RANGE(0x16500A, 0x16500b) AM_READ(pckgaldx_unknown_r)

	/* should we really be using these to read the i/o in the BOOTLEG?
	  these look like i/o through protection ... */
	AM_RANGE(0x167842, 0x167843) AM_READ(input_port_2_word_r) // player controls
	AM_RANGE(0x167c4c, 0x167c4d) AM_READ(input_port_1_word_r)
	AM_RANGE(0x167db2, 0x167db3) AM_READ(input_port_0_word_r)

	AM_RANGE(0x167d10, 0x167d11) AM_READ(pckgaldx_protection_r) // check code at 6ea
	AM_RANGE(0x167d1a, 0x167d1b) AM_READ(pckgaldx_protection_r) // check code at 7C4

	AM_RANGE(0x300000, 0x30000f) AM_READ(MRA16_RAM) // ??

	AM_RANGE(0x330000, 0x330bff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( pktgaldb_writemem, ADDRESS_SPACE_PROGRAM, 16 )

	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x100fff) AM_WRITE(MWA16_RAM) AM_BASE(&pcktgaldb_fgram) // fgram on original?
	AM_RANGE(0x102000, 0x102fff) AM_WRITE(MWA16_RAM) // bgram on original?
	AM_RANGE(0x120000, 0x123fff) AM_WRITE(MWA16_RAM)AM_BASE(&pcktgaldb_sprites)
	AM_RANGE(0x130000, 0x130fff) AM_WRITE(MWA16_RAM) // palette on original?
	AM_RANGE(0x140000, 0x140003) AM_WRITE(MWA16_RAM) // sound?
	AM_RANGE(0x140008, 0x140009) AM_WRITE(MWA16_RAM) // sound?
	AM_RANGE(0x150000, 0x150003) AM_WRITE(MWA16_RAM) // sound?

	/* or maybe protection writes go to sound ... */
	AM_RANGE(0x160000, 0x167fff) AM_WRITE(MWA16_RAM)
//cpu #0 (PC=00009246): unmapped program memory word write to 00167942 = 0000 & FFFF
//cpu #0 (PC=0000924E): unmapped program memory word write to  = 0000 & FFFF
	AM_RANGE(0x16500a, 0x16500b) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x166800, 0x166801) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x167942, 0x167943) AM_WRITE(MWA16_RAM)

	AM_RANGE(0x170000, 0x17ffff) AM_WRITE(MWA16_RAM)

	AM_RANGE(0x300000, 0x30000f) AM_WRITE(MWA16_RAM)

	AM_RANGE(0x330000, 0x330bff) AM_WRITE(MWA16_RAM) AM_WRITE(paletteram16_xRGB_w) AM_BASE(&paletteram16) // extra colours?
ADDRESS_MAP_END


static MACHINE_INIT (deco102)
{
	/* code isn't decrypted so we don't want the cpu going anywhere */
	cpunum_set_input_line(0, INPUT_LINE_HALT, ASSERT_LINE);
	cpunum_set_input_line(0, INPUT_LINE_RESET, ASSERT_LINE);
}


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
	{ REGION_GFX1, 0, &tile_8x8_layout,     0, 64 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,     0, 64 },	/* Tiles (16x16) */
	{ REGION_GFX2, 0, &spritelayout, 0, 64},	/* Sprites (16x16) */

	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_2[] =
{
	{ REGION_GFX1, 0, &tile_8x8_layout,     0, 16 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,     0, 16 },	/* Tiles (16x16) */
	{ REGION_GFX2, 0, &tile_8x8_layout,     0, 16 },	/* Tiles (8x8) */
	{ REGION_GFX2, 0, &tile_16x16_layout,     0, 16 },	/* Tiles (16x16) */
	{ REGION_GFX3, 0, &spritelayout, 0, 1},	/* Sprites (16x16) */
	{ REGION_GFX4, 0, &spritelayout, 0, 1},	/* Sprites (16x16) */

	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( deco102_1 )
	/* basic machine hardware */
	MDRV_CPU_ADD(DE102CPU, 14000000)	/* DE102 */
	MDRV_CPU_PROGRAM_MAP(deco102_map,0)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_MACHINE_INIT (deco102)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(deco102)
	MDRV_VIDEO_UPDATE(deco102)

	/* sound hardware */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( deco102_2 )
	/* basic machine hardware */
	MDRV_CPU_ADD(DE102CPU, 14000000)	/* DE102 */
	MDRV_CPU_PROGRAM_MAP(deco102_map,0)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_MACHINE_INIT (deco102)
	MDRV_GFXDECODE(gfxdecodeinfo_2)

	MDRV_VIDEO_START(deco102)
	MDRV_VIDEO_UPDATE(deco102)

	/* sound hardware */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pktgaldb )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(pktgaldb_readmem,pktgaldb_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(deco102)
	MDRV_VIDEO_UPDATE(pktgaldb)

	/* sound hardware */
MACHINE_DRIVER_END

/*

Pocket Gal Deluxe
Nihon System Inc., 1993

This game runs on Data East hardware.

PCB Layout
----------

DEC-22V0  DE-0378-2
|-------------------------------------|
|  M6295(2)  KG01.14F     DE102  62256|
|  M6295(1)  MAZ-03.13F          62256|
|                                     |
|   32.220MHz              KG00-2.12A |
|                                     |
|J           DE56              DE71   |
|A                                    |
|M     DE153            28MHz         |
|M                    6264     DE52   |
|A                    6264            |
|                                     |
|                                     |
|                                     |
|DSW1                   MAZ-01.3B     |
|     DE104  MAZ-02.2H                |
|DSW2                   MAZ-00.1B     |
|-------------------------------------|

Notes:
      - CPU is unknown. It's chip DE102. The clock input is 14.000MHz on pin 6
        It's thought to be a custom-made encrypted 68000.
        The package is a Quad Flat Pack, has 128 pins and is symmetrically square (1 1/4" square from tip to tip).
        So the question arises...  "Which CPU's manufactured around 1993 had 128 pins?"
      - M6295(1) clock: 2.01375MHz (32.220 / 16)
        M6295(2) clock: 1.006875MHz (32.220 / 32)
      - VSync: 58Hz

*/

ROM_START( pktgaldx )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD( "kg00-2.12a",    0x00000, 0x80000, CRC(62dc4137) SHA1(23887dc3f6e7c4cdcb1bf4f4c87fe3cbe8cdbe69) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "maz-02.2h",    0x00000, 0x100000, CRC(c9d35a59) SHA1(07b44c7d7d76b668b4d6ca5672bd1c2910228e68) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "maz-00.1b",    0x000000, 0x080000, CRC(fa3071f4) SHA1(72e7d920e9ca94f8cb166007a9e9e5426a201af8) )
	ROM_LOAD16_BYTE( "maz-01.3b",    0x000001, 0x080000, CRC(4934fe21) SHA1(b852249f59906d69d32160ebaf9b4781193227e4) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "kg01.14f",    0x00000, 0x20000, CRC(8a106263) SHA1(229ab17403c2b8f4e89a90a8cda2f3c3a4b55d9e) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* Oki samples (banked?) */
	ROM_LOAD( "maz-03.13f",    0x00000, 0x100000, CRC(a313c964) SHA1(4a3664c4e2c44a017a0ab6a6d4361799cbda57b5) )
ROM_END

ROM_START( pktgaldb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE ("4.bin", 0x00000, 0x80000, CRC(67ce30aa) SHA1(c5228ed19eebbfb6d5f7cbfcb99734d9fcd5aba3) )
	ROM_LOAD16_BYTE( "5.bin", 0x00001, 0x80000, CRC(64cb4c33) SHA1(02f988f558113dd9a77079dee59e23583394fa98) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "kg01.14f",    0x00000, 0x20000, CRC(8a106263) SHA1(229ab17403c2b8f4e89a90a8cda2f3c3a4b55d9e) ) // 1.bin on the bootleg

/* ORIGINAL ROMS! .. these are not from the bootleg, we should replace them with bootleg roms below */
	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "maz-02.2h",    0x00000, 0x100000, CRC(c9d35a59) SHA1(07b44c7d7d76b668b4d6ca5672bd1c2910228e68) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "maz-00.1b",    0x000000, 0x080000, CRC(fa3071f4) SHA1(72e7d920e9ca94f8cb166007a9e9e5426a201af8) )
	ROM_LOAD16_BYTE( "maz-01.3b",    0x000001, 0x080000, CRC(4934fe21) SHA1(b852249f59906d69d32160ebaf9b4781193227e4) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* Oki samples (banked?) */
	ROM_LOAD( "maz-03.13f",    0x00000, 0x100000, CRC(a313c964) SHA1(4a3664c4e2c44a017a0ab6a6d4361799cbda57b5) )

/* BOOTLEG ROMS! .. sort use these later */
	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "2.bin",    0x00000, 0x80000, CRC(f841d995) SHA1(0ef2f8fd9be62b979862c3688e7aad34c7b0404d) )
	ROM_LOAD( "3.bin",    0x00000, 0x80000, CRC(4638747b) SHA1(56d79cd8d4d7b41b71f1e942b5a5bf1bafc5c6e7) )
	ROM_LOAD( "6.bin",    0x00000, 0x80000, CRC(0e3335a1) SHA1(2d6899336302d222e8404dde159e64911a8f94e6) )
	ROM_LOAD( "7.bin",    0x00000, 0x80000, CRC(0ebf12b5) SHA1(17b6c2ce21de3671d75d89a41317efddf5b49339) )
	ROM_LOAD( "8.bin",    0x00000, 0x80000, CRC(40f5a032) SHA1(c2ad585ddbc3ef40c6214cb30b4d78a2cd0a9446) )
	ROM_LOAD( "9.bin",    0x00000, 0x80000, CRC(078f371c) SHA1(5b510a0f7f50c55cce1ffcc8f2e9c3432b23e352) )
	ROM_LOAD( "10.bin",    0x00000, 0x80000, CRC(9dd743a9) SHA1(dbc3e2bd044dbf21b04c174bd860969ee53b4050) )
	ROM_LOAD( "11.bin",    0x00000, 0x80000, CRC(a8c8f1fd) SHA1(9fd5fa500967a1bd692abdbeef89ce195c8aecd4) )
ROM_END

/* Diet Go Go */

ROM_START( dietgo )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "jx.00",    0x000000, 0x040000, CRC(1a9de04f) SHA1(7ce1e7cf4cdce2b02da4df2a6ae9a9e665e24422) )
	ROM_LOAD16_BYTE( "jx.01",    0x000001, 0x040000, CRC(79c097c8) SHA1(be49055ee324535e1118d243bd49e74ec1d2a2d7) )

	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_DISPOSE ) // sound cpu?
	ROM_LOAD( "jx.02",    0x00000, 0x10000, CRC(4e3492a5) SHA1(5f302bdbacbf95ea9f3694c48545a1d6bba4b019) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "may00",    0x00000, 0x100000, CRC(234d1f8d) SHA1(42d23aad20df20cbd2359cc12bdd47636b2027d3) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "may01",    0x000000, 0x100000, CRC(2da57d04) SHA1(3898e9fef365ecaa4d86aa11756b527a4fffb494) )
	ROM_LOAD16_BYTE( "may02",    0x000001, 0x100000, CRC(3a66a713) SHA1(beeb99156332cf4870738f7769b719a02d7b40af) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "may03",    0x00000, 0x80000, CRC(b6e42bae) SHA1(c282cdf7db30fb63340cc609bf00f5ab63a75583) )
ROM_END

/*

Diet Go Go (Euro version 1.1)

Alternative program ROMs only

Hold both START buttons on bootup to display version notice.

*/

ROM_START( dietgoe )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "jy00-1.4h",    0x000000, 0x040000, CRC(8bce137d) SHA1(55f5b1c89330803c6147f9656f2cabe8d1de8478) )
	ROM_LOAD16_BYTE( "jy01-1.5h",    0x000001, 0x040000, CRC(eca50450) SHA1(1a24117e3b1b66d7dbc5484c94cc2c627d34e6a3) )

	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_DISPOSE ) // sound cpu?
	ROM_LOAD( "jy.02.bin",    0x00000, 0x10000, CRC(9caa2cc9) SHA1(26721f33675a222a691af276c8ca692e5dc3f9af) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "may00",    0x00000, 0x100000, CRC(234d1f8d) SHA1(42d23aad20df20cbd2359cc12bdd47636b2027d3) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "may01",    0x000000, 0x100000, CRC(2da57d04) SHA1(3898e9fef365ecaa4d86aa11756b527a4fffb494) )
	ROM_LOAD16_BYTE( "may02",    0x000001, 0x100000, CRC(3a66a713) SHA1(beeb99156332cf4870738f7769b719a02d7b40af) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "may03",    0x00000, 0x80000, CRC(b6e42bae) SHA1(c282cdf7db30fb63340cc609bf00f5ab63a75583) )
ROM_END

/*

               DIET GO GO    DATA EAST



NAME    LOCATION   TYPE
-----------------------
JW-02    14M       27C512
JW-01-2  5H        27C2001
JW-00-2  4H         "
PAL16L8B 7H
PAL16L8B 6H
PAL16R6A 11H

*/

ROM_START( dietgoa )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "jw-00-2.4h",    0x000000, 0x040000, CRC(e6ba6c49) SHA1(d5eaea81f1353c58c03faae67428f7ee98e766b1) )
	ROM_LOAD16_BYTE( "jw-01-2.5h",    0x000001, 0x040000, CRC(684a3d57) SHA1(bd7a57ba837a1dc8f92b5ebcb46e50db1f98524f) )

	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_DISPOSE ) // sound cpu?
	ROM_LOAD( "jx.02",    0x00000, 0x10000, CRC(4e3492a5) SHA1(5f302bdbacbf95ea9f3694c48545a1d6bba4b019) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "may00",    0x00000, 0x100000, CRC(234d1f8d) SHA1(42d23aad20df20cbd2359cc12bdd47636b2027d3) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "may01",    0x000000, 0x100000, CRC(2da57d04) SHA1(3898e9fef365ecaa4d86aa11756b527a4fffb494) )
	ROM_LOAD16_BYTE( "may02",    0x000001, 0x100000, CRC(3a66a713) SHA1(beeb99156332cf4870738f7769b719a02d7b40af) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "may03",    0x00000, 0x80000, CRC(b6e42bae) SHA1(c282cdf7db30fb63340cc609bf00f5ab63a75583) )
ROM_END

/*

Double Wings (JPN Ver.)
(c)1993 Mitchell
DEC-22V0 (S-NK-3220)

Software is by Mitchell, but the PCB is pure Data East.
It's the standard DEC-22V0 PCB that runs many Data East games from around the mid 90's.

Data East ROM code = KP

CPU 	:68000 ?
Sound	:TMPZ84C00AP-6,YM2151,OKI M6295, YM3014B
OSC 	:28.0000MHz,32.2200MHz
RAM     :LH6168 x 1, CXK5814 x 6, CXK5864 x 4
DIP     :2 x 8 position
Other	:DATA EAST Chips (numbers scratched)
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
	ROM_LOAD16_BYTE( "kp_00-.3d",    0x000000, 0x040000, CRC(547dc83e) SHA1(f6f96bd4338d366f06df718093f035afabc073d1) )
	ROM_LOAD16_BYTE( "kp_01-.5d",    0x000001, 0x040000, CRC(7a210c33) SHA1(ced89140af6d6a1bc0ffb7728afca428ed007165) )

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

DRIVER_INIT(deco102_1)
{
	deco56_decrypt(REGION_GFX1);
	//deco102_decrypt(REGION_CPU1);
}

/*

Boogie Wings (aka The Great Ragtime Show)
Data East, 1992

PCB No: DE-0379-1

CPU: ? (DE102?)
Sound: HuC6280A, YM2151, YM3012, OKI M6295 (x2)
OSC: 32.220MHz, 28.000MHz
DIPs: 8 position (x2)
PALs: (not dumped) VF-01 (PAL16L8)
                   VF-02 (22CV10P)
                   VF-03 (PAL16L8)
                   VF-04 (PAL 16L8)
                   VF-05 (PAL 16L8)

PROM: MB7122 (compatible to 82S137, located near MBD-09)

RAM: 62256 (x2), 6264 (x5)

Data East Chips:   52 (x2)
                   141 (x2)
                   102
                   104
                   113
                   99
                   71 (x2)
                   200

ROMs:
kn00-2.2b	27c2001	  \
kn01-2.4b	27c2001    |  Main program
kn02-2.2e	27c2001    |
kn03-2.4e	27c2001   /
kn04.8e		27c512    \
kn05.9e		27c512    /   near 141's and MBD-00, MBD-01 and MBD-02
kn06.18p	27c512	      Sound Program
mbd-00.8b	16M
mbd-01.9b	16M
mbd-02.10e	4M
mbd-03.13b	16M
mbd-04.14b	16M
mbd-05.16b	16M
mbd-06.17b	16M
mbd-07.18b	16M
mbd-08.19b	16M
mbd-09.16p	4M	       Oki Samples
mbd-10.17p	4M	       Oki Samples

ROMCMP says the 16M ROMs have identical halves, but the PCB labels them as 16M,
so for now I dump them as 16M. If necessary, just chop them in half to make them 8M dumps.

*/

ROM_START( boogwing )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "kn00-2.2b",    0x000000, 0x040000, CRC(e38892b9) SHA1(49b5637965a43e0378e1258c5f0a780926f1f283) )
	ROM_LOAD16_BYTE( "kn01-2.4b",    0x000001, 0x040000, CRC(3ad4b54c) SHA1(5141001768266995078407851b445378b21453de) )
	ROM_LOAD16_BYTE( "kn02-2.2e",    0x080000, 0x040000, CRC(8426efef) SHA1(2ea33cbd58b638053d75668a484648dbf67dabb8) )
	ROM_LOAD16_BYTE( "kn03-2.4e",    0x080001, 0x040000, CRC(10b61f4a) SHA1(41d7f670defbd7dae89afafac9839a9e237814d5) )

	ROM_REGION( 0x80000, REGION_USER1, 0 ) /* unknown 1 (more program code?) */
	ROM_LOAD16_BYTE( "kn04.8e",    0x000000, 0x010000, CRC(329323a8) SHA1(e2ec7b059301c0a2e052dfc683e044c808ad9b33) )
	ROM_LOAD16_BYTE( "kn05.9e",    0x000001, 0x010000, CRC(d10aef95) SHA1(a611a35ab312caee19c31da079c647679d31673d) )

	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_DISPOSE ) // sound cpu?
	ROM_LOAD( "kn06.18p",    0x00000, 0x10000, CRC(3e8bc4e1) SHA1(7e4c357afefa47b8f101727e06485eb9ebae635d) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Tiles 1 */
	ROM_LOAD( "mbd-00.8b",    0x000000, 0x200000, CRC(c4ad85c2) SHA1(52cbb450a5bd0adb229ab677e81f9a3a03ef3c3a) )
	ROM_LOAD( "mbd-01.9b",    0x200000, 0x200000, CRC(d319f169) SHA1(9b0f8820c1de32ef49e9bf2c2bcc449332a79164) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles 2 */
	ROM_LOAD( "mbd-03.13b",    0x000000, 0x200000, CRC(4d8457d0) SHA1(26e655bf9ef921186bdac158e49247982a1472f6) )
	ROM_LOAD( "mbd-04.14b",    0x200000, 0x200000, CRC(a4eb1027) SHA1(c63c263d0518c597f32ac86c2089761e9ce28b60) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites 1 */
	ROM_LOAD16_BYTE( "mbd-05.16b",    0x000000, 0x200000, CRC(1768c66a) SHA1(06bf3bb187c65db9dcce959a43a7231e2ac45c17) )
	ROM_LOAD16_BYTE( "mbd-06.17b",    0x000001, 0x200000, CRC(7750847a) SHA1(358266ed68a9816094e7aab0905d958284c8ce98) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites 2 */
	ROM_LOAD16_BYTE( "mbd-07.18b",    0x000000, 0x200000, CRC(241faac1) SHA1(588be0cf2647c1d185a99c987a5a20ab7ad8dea8) )
	ROM_LOAD16_BYTE( "mbd-08.19b",    0x000001, 0x200000, CRC(f13b1e56) SHA1(f8f5e8c4e6c159f076d4e6505bd901ade5c6a0ca) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-09.16p",    0x000000, 0x080000, CRC(f44f2f87) SHA1(d941520bdfc9e6d88c45462bc1f697c18f33498e) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-10.17p",    0x000000, 0x080000, CRC(f159f76a) SHA1(0b1ea69fecdd151e2b1fa96a21eade492499691d) )

	ROM_REGION( 0x080000, REGION_USER2, ROMREGION_DISPOSE ) /* unknown 2 (more gfx?) */
	ROM_LOAD( "mbd-02.10e",    0x000000, 0x080000, CRC(b25aa721) SHA1(efe800759080bd1dac2da93bd79062a48c5da2b2) )

	ROM_REGION( 0x001000, REGION_USER3, ROMREGION_DISPOSE ) /* unknown 3 (prom?) */
	ROM_LOAD( "kj-00.15n",    0x000000, 0x00400, CRC(add4d50b) SHA1(080e5a8192a146d5141aef5c8d9996ddf8cd3ab4) )
ROM_END

DRIVER_INIT(deco102_2)
{
	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);
	//deco102_decrypt(REGION_CPU1);
}

/* no sound cpu? */
GAMEX(1993, pktgaldx, 0,        deco102_1,      deco102, deco102_1,      ROT0, "Nihon System", "Pocket Gal Deluxe", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1993, pktgaldb, pktgaldx, pktgaldb,       pktgaldb,deco102_1,      ROT0, "bootleg", "Pocket Gal Deluxe (bootleg)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )

/* these have a sound cpu */
GAMEX(199?, dietgo,   0,        deco102_1,      deco102, deco102_1,      ROT0, "Data East", "Diet Go Go", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(199?, dietgoe,  dietgo,   deco102_1,      deco102, deco102_1,      ROT0, "Data East", "Diet Go Go (Euro v1.1)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(199?, dietgoa,  dietgo,   deco102_1,      deco102, deco102_1,      ROT0, "Data East", "Diet Go Go (alt)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1993, dblewing, 0,        deco102_1,      deco102, deco102_1,      ROT90,"Mitchell", "Double Wings", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)

/* this is clearly different hardware (more of everything) */
GAMEX(1992, boogwing, 0,        deco102_2,      deco102, deco102_2,      ROT0, "Data East", "Boogie Wings", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
