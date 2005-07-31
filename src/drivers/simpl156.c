/* Data East Hardware
 'Simple' 156 based board  (PCB DE-0409-1)

  each game has a different memory map, probably what the GALs are for
  games seem to be protected.

  notes:

  Joe Mac Returns:

 to skip boot messge
  go 368
  do pc = 36c

  (or use hack in source..)
  eeprom not working!
 is game speed right?


  Magical Drop / Magical Drop Plus / Chain Reaction

 to skip boot messge
  go 3e0
  do pc = 3e8

  Magical drop may have random crashes, depends heavily upon CPU speed...


  Osman

  go 6A8
  do pc = 6ac

  --

  I suspect the protection is related to 0x201000 and the eeprom reading.

  General:

  Tilebanking is wrong
  Sound Banking is wrong (and loading of music rom?)
  Protection may not be protection
  Eeprom hookup is wrong causing various problems
  (Eeprom read with OKI Status? check.. something seems to have changed when i hooked up sound)
  Priority is wrong (Magical Drop)

  Osman has bad sprite colours.. maybe 16-bit ram on 32-bit CPU? check..


*/

#define DE156CPU ARM
extern void decrypt156(void);

#include "driver.h"
#include "decocrpt.h"
#include "deco32.h"
#include "cpu/arm/arm.h"
#include "machine/eeprom.h"
#include "vidhrdw/generic.h"
#include "machine/random.h"
#include "sound/okim6295.h"

data32_t *simpl156_pf1_data,*simpl156_pf2_data;
data32_t *simpl156_pf12_control;
data32_t *simpl156_pf1_rowscroll,*simpl156_pf2_rowscroll;

data32_t* deco_unknown;

extern WRITE32_HANDLER( simpl156_pf1_data_w );
extern WRITE32_HANDLER( simpl156_pf2_data_w );
extern VIDEO_START( simpl156 );
extern VIDEO_UPDATE( simpl156 );


INPUT_PORTS_START( simpl156 )
	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0xfff0, IP_ACTIVE_HIGH, IPT_VBLANK ) // all bits? check..


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


READ32_HANDLER( simpl156_inputs_read )
{
	return readinputport(0)^0xffff0000;
}


/* maybe wrong */
WRITE32_HANDLER( simpl156_palette_write )
{
	int r,g,b;
	UINT16 dat;
	int color;

	COMBINE_DATA(&paletteram32[offset]);
	color = offset;

	/* eh, we ignore this color???! */

//  color = offset<<1;

//  dat = (paletteram32[offset]>>16)&0xffff;

//  r = (dat >>  5) & 0x1f;
//  g = (dat >>  0) & 0x1f;
//  b = (dat >> 10) & 0x1f;

//  r = (r << 3) | (r >> 2);
//  g = (g << 3) | (g >> 2);
//  b = (b << 3) | (b >> 2);

//  palette_set_color(color+1,r,g,b);

	dat = paletteram32[offset]&0xffff;

	g = (dat >>  5) & 0x1f;
	r = (dat >>  0) & 0x1f;
	b = (dat >> 10) & 0x1f;

	g = (g << 3) | (g >> 2);
	r = (r << 3) | (r >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(color,r,g,b);
}




static READ32_HANDLER(  simpl156_system_r )
{
	UINT32 returndata;

	returndata = readinputport(1);
	returndata |= EEPROM_read_bit()<<16;

	return returndata;
}

static WRITE32_HANDLER( simpl156_eeprom_w )
{
	EEPROM_set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
	EEPROM_write_bit(data & 0x10);
	EEPROM_set_cs_line((data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
}



READ32_HANDLER( simpl156_unknown_read )
{
	return 0xffffffff;
}

/* the different memory maps MAY just be mirroring, but the boards do have GALs near the CPU */

static data32_t *simpl156_systemram;

/* I'm not convinced it is protection */
static READ32_HANDLER(prot_r)
 {
 	if(offset==8)
 		return simpl156_systemram[offset]&(~0x40);
 	else
		return simpl156_systemram[offset];
}

static WRITE32_HANDLER(prot_w)
{
	COMBINE_DATA(&simpl156_systemram[offset]);
}

static READ32_HANDLER( oki_r )
{
	return OKIM6295_status_0_r(0);
}

static WRITE32_HANDLER( oki_w )
{
	OKIM6295_data_0_w(0, data & 0xff);
}

static READ32_HANDLER( oki2_r )
{
	return OKIM6295_status_1_r(0);
}

static WRITE32_HANDLER( oki2_w )
{
	OKIM6295_data_1_w(0, data & 0xff);
}

static WRITE32_HANDLER( oki2_bank_w )
{
	OKIM6295_set_bank_base(1, ((data & 3)-1) * 0x40000);
}

/* Chain Reaction */
static ADDRESS_MAP_START( chainrec_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x200000, 0x200003) AM_READ(simpl156_inputs_read)
	AM_RANGE(0x201000, 0x201fff) AM_READWRITE(prot_r,prot_w)  AM_BASE(&simpl156_systemram) // ram prot or extra?
	AM_RANGE(0x3c0000, 0x3c0003) AM_READWRITE(oki2_r,oki2_w)
	AM_RANGE(0x400000, 0x407fff) AM_RAM // main ram?
	AM_RANGE(0x410000, 0x411fff) AM_RAM AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)
	AM_RANGE(0x420000, 0x420fff) AM_READWRITE(MRA32_RAM,simpl156_palette_write) AM_BASE(&paletteram32)
	AM_RANGE(0x430000, 0x430003) AM_READ(simpl156_system_r) AM_WRITE(simpl156_eeprom_w)
	AM_RANGE(0x440000, 0x44001f) AM_RAM AM_BASE(&simpl156_pf12_control)
	AM_RANGE(0x450000, 0x451fff) AM_WRITE(simpl156_pf1_data_w) AM_BASE(&simpl156_pf1_data)
	AM_RANGE(0x452000, 0x453fff) AM_WRITE(simpl156_pf1_data_w)
	AM_RANGE(0x454000, 0x455fff) AM_WRITE(simpl156_pf2_data_w) AM_BASE(&simpl156_pf2_data)
	AM_RANGE(0x460000, 0x461fff) AM_WRITE(MWA32_RAM) AM_BASE(&simpl156_pf1_rowscroll)
	AM_RANGE(0x464000, 0x465fff) AM_WRITE(MWA32_RAM) AM_BASE(&simpl156_pf2_rowscroll)
	AM_RANGE(0x470000, 0x470003) AM_WRITE(oki2_bank_w)
	AM_RANGE(0x480000, 0x480003) AM_READWRITE(oki_r,oki_w)
ADDRESS_MAP_END


/* Magical Drop */
static ADDRESS_MAP_START( magdrop_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x200000, 0x200003) AM_READ(simpl156_inputs_read)
	AM_RANGE(0x201000, 0x201fff) AM_READWRITE(prot_r,prot_w)  AM_BASE(&simpl156_systemram) // ram prot or extra?
	AM_RANGE(0x340000, 0x340003) AM_READWRITE(oki2_r,oki2_w)
	AM_RANGE(0x380000, 0x387fff) AM_RAM // main ram?
	AM_RANGE(0x390000, 0x391fff) AM_RAM AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)
	AM_RANGE(0x3a0000, 0x3a0fff) AM_READWRITE(MRA32_RAM,simpl156_palette_write) AM_BASE(&paletteram32)
	AM_RANGE(0x3b0000, 0x3b0003) AM_READ(simpl156_system_r) AM_WRITE(simpl156_eeprom_w)
	AM_RANGE(0x3c0000, 0x3c001f) AM_RAM AM_BASE(&simpl156_pf12_control)
	AM_RANGE(0x3d0000, 0x3d1fff) AM_WRITE(simpl156_pf1_data_w) AM_BASE(&simpl156_pf1_data)
	AM_RANGE(0x3d2000, 0x3d3fff) AM_WRITE(simpl156_pf1_data_w)
	AM_RANGE(0x3d4000, 0x3d5fff) AM_WRITE(simpl156_pf2_data_w) AM_BASE(&simpl156_pf2_data)
	AM_RANGE(0x3e0000, 0x3e1fff) AM_WRITE(MWA32_RAM) AM_BASE(&simpl156_pf1_rowscroll)
	AM_RANGE(0x3e4000, 0x3e5fff) AM_WRITE(MWA32_RAM) AM_BASE(&simpl156_pf2_rowscroll)
	AM_RANGE(0x3f0000, 0x3f0003) AM_WRITE(oki2_bank_w)
	AM_RANGE(0x400000, 0x400003) AM_READWRITE(oki_r,oki_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( magdropp_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x200000, 0x200003) AM_READ(simpl156_inputs_read)
	AM_RANGE(0x201000, 0x201fff) AM_READWRITE(prot_r,prot_w)  AM_BASE(&simpl156_systemram) // ram prot or extra?
	AM_RANGE(0x4c0000, 0x4c0003) AM_READWRITE(oki2_r,oki2_w)
	AM_RANGE(0x680000, 0x687fff) AM_RAM // main ram?
	AM_RANGE(0x690000, 0x691fff) AM_RAM AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)
	AM_RANGE(0x6a0000, 0x6a0fff) AM_READWRITE(MRA32_RAM,simpl156_palette_write) AM_BASE(&paletteram32)
	AM_RANGE(0x6b0000, 0x6b0003) AM_READ(simpl156_system_r) AM_WRITE(simpl156_eeprom_w)
	AM_RANGE(0x6c0000, 0x6c001f) AM_RAM AM_BASE(&simpl156_pf12_control)
	AM_RANGE(0x6d0000, 0x6d1fff) AM_WRITE(simpl156_pf1_data_w) AM_BASE(&simpl156_pf1_data)
	AM_RANGE(0x6d2000, 0x6d3fff) AM_WRITE(simpl156_pf1_data_w)
	AM_RANGE(0x6d4000, 0x6d5fff) AM_WRITE(simpl156_pf2_data_w) AM_BASE(&simpl156_pf2_data)
	AM_RANGE(0x6e0000, 0x6e1fff) AM_WRITE(MWA32_RAM) AM_BASE(&simpl156_pf1_rowscroll)
	AM_RANGE(0x6e4000, 0x6e5fff) AM_WRITE(MWA32_RAM) AM_BASE(&simpl156_pf2_rowscroll)
	AM_RANGE(0x6f0000, 0x6f0003) AM_WRITE(oki2_bank_w)
	AM_RANGE(0x780000, 0x780003) AM_READWRITE(oki_r,oki_w)
ADDRESS_MAP_END

/* prtytime, charlien, osman */
static ADDRESS_MAP_START( mitchell156_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x100003) AM_READWRITE(oki_r,oki_w)
	AM_RANGE(0x140000, 0x140003) AM_READWRITE(oki2_r,oki2_w)
	AM_RANGE(0x180000, 0x187fff) AM_RAM // main ram?
	AM_RANGE(0x190000, 0x191fff) AM_RAM AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)
	AM_RANGE(0x1a0000, 0x1a0fff) AM_READWRITE(MRA32_RAM,simpl156_palette_write) AM_BASE(&paletteram32)
	AM_RANGE(0x1b0000, 0x1b0003) AM_READ(simpl156_system_r) AM_WRITE(simpl156_eeprom_w)
	AM_RANGE(0x1c0000, 0x1c001f) AM_RAM AM_BASE(&simpl156_pf12_control)
	AM_RANGE(0x1d0000, 0x1d1fff) AM_WRITE(simpl156_pf1_data_w) AM_BASE(&simpl156_pf1_data)
	AM_RANGE(0x1d2000, 0x1d3fff) AM_WRITE(simpl156_pf1_data_w)
	AM_RANGE(0x1d4000, 0x1d5fff) AM_WRITE(simpl156_pf2_data_w) AM_BASE(&simpl156_pf2_data)
	AM_RANGE(0x1e0000, 0x1e1fff) AM_WRITE(MWA32_RAM) AM_BASE(&simpl156_pf1_rowscroll)
	AM_RANGE(0x1e4000, 0x1e5fff) AM_WRITE(MWA32_RAM) AM_BASE(&simpl156_pf2_rowscroll)
	AM_RANGE(0x1f0000, 0x1f0003) AM_WRITE(oki2_bank_w)
	AM_RANGE(0x200000, 0x200003) AM_READ(simpl156_inputs_read)
	AM_RANGE(0x201000, 0x201fff) AM_READWRITE(prot_r,prot_w)  AM_BASE(&simpl156_systemram) // ram prot or extra?
ADDRESS_MAP_END

/* Joe and Mac Returns */
static ADDRESS_MAP_START( joemacr_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x107fff) AM_RAM // main ram?
	AM_RANGE(0x110000, 0x111fff) AM_RAM AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)
	AM_RANGE(0x120000, 0x120fff) AM_READWRITE(MRA32_RAM,simpl156_palette_write) AM_BASE(&paletteram32)
	AM_RANGE(0x130000, 0x130003) AM_READ(simpl156_system_r) AM_WRITE(simpl156_eeprom_w)
	AM_RANGE(0x140000, 0x14001f) AM_RAM AM_BASE(&simpl156_pf12_control)
	AM_RANGE(0x150000, 0x151fff) AM_WRITE(simpl156_pf1_data_w) AM_BASE(&simpl156_pf1_data)
	AM_RANGE(0x152000, 0x153fff) AM_WRITE(simpl156_pf1_data_w)
	AM_RANGE(0x154000, 0x155fff) AM_WRITE(simpl156_pf2_data_w) AM_BASE(&simpl156_pf2_data)
	AM_RANGE(0x160000, 0x161fff) AM_WRITE(MWA32_RAM) AM_BASE(&simpl156_pf1_rowscroll)
	AM_RANGE(0x164000, 0x165fff) AM_WRITE(MWA32_RAM) AM_BASE(&simpl156_pf2_rowscroll)
	AM_RANGE(0x170000, 0x170003) AM_WRITE(oki2_bank_w)
	AM_RANGE(0x180000, 0x180003) AM_READWRITE(oki_r,oki_w)
	AM_RANGE(0x1c0000, 0x1c0003) AM_READWRITE(oki2_r,oki2_w)
	AM_RANGE(0x200000, 0x200003) AM_READ(simpl156_inputs_read)
	AM_RANGE(0x201000, 0x201fff) AM_READWRITE(prot_r,prot_w)  AM_BASE(&simpl156_systemram) // ram prot or extra?
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
	{ REGION_GFX1, 0, &tile_8x8_layout,     0,     16 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,   0,     16 },	/* Tiles (16x16) */
	{ REGION_GFX1, 0, &tile_8x8_layout,     0x100, 16 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,   0x100, 16 },	/* Tiles (16x16) */

	{ REGION_GFX2, 0, &spritelayout, 0x200, 32},	/* Sprites (16x16) */

	{ -1 } /* end of array */
};



static INTERRUPT_GEN( simpl156_vbl_interrupt )
{
	cpunum_set_input_line(0, ARM_IRQ_LINE, HOLD_LINE);
}


static MACHINE_DRIVER_START( chainrec )
	/* basic machine hardware */

	MDRV_CPU_ADD_TAG("DE156", ARM, 28000000 /* /4 */)	/*DE156*/ /* 7.000 MHz */ /* measured at 7.. seems to need 28? */
	MDRV_CPU_PROGRAM_MAP(chainrec_map,0)
	MDRV_CPU_VBLANK_INT(simpl156_vbl_interrupt,1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(800)
	MDRV_NVRAM_HANDLER(93C46) // 93C45

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_VIDEO_START(simpl156)
	MDRV_VIDEO_UPDATE(simpl156)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 32220000/32/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)

	MDRV_SOUND_ADD(OKIM6295, 32220000/16/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( magdrop )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(chainrec)
	MDRV_CPU_MODIFY("DE156")
	MDRV_CPU_PROGRAM_MAP(magdrop_map,0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( magdropp )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(chainrec)
	MDRV_CPU_MODIFY("DE156")
	MDRV_CPU_PROGRAM_MAP(magdropp_map,0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( joemacr )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(chainrec)
	MDRV_CPU_MODIFY("DE156")
	MDRV_CPU_PROGRAM_MAP(joemacr_map,0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mitchell156 )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(chainrec)
	MDRV_CPU_MODIFY("DE156")
	MDRV_CPU_PROGRAM_MAP(mitchell156_map,0)
MACHINE_DRIVER_END


DRIVER_INIT(simpl156)
{
	deco56_decrypt(REGION_GFX1);
	decrypt156();
}



/*
Osman
Mitchell Corp., 1996

This game runs on Data East hardware. The game is very
similar to Strider.

PCB Layout
----------

DEC-22VO
MT5601-0
------------------------------------------------------------
|                 MCF04.14H     MCF-03.14D     MCF-02.14A  |
|                                                          |
|     OKI M6295   SA01-0.13H            52     MCF-01.13A  |
|                                                          |
--|   OKI M6295                                            |
  |                                                        |
--|                               CY7C185-25   MCF-00.9A   |
|                                 CY7C185-25               |
|                                                          |
|                                                          |
|J                     GAL16V8           28.000MHz         |
|A                  CY7C185-25                             |
|M                  CY7C185-25     141                     |
|M                                                         |
|A          223                                            |
|                                             GAL16V8      |
|                                GAL16V8                   |
|                             CY7C185-25   223             |
--|                93C45.2H   CY7C185-25                   |
  |                                                  156   |
--| TESTSW                  SA00-0.1E                      |
|                                                          |
------------------------------------------------------------

*/

ROM_START( osman )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "sa00-0.1e",    0x000000, 0x080000, CRC(ec6b3257) SHA1(10a42a680ce122ab030eaa2ccd99d302cb77854e) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcf-00.9a",    0x000000, 0x080000, CRC(247712dc) SHA1(bcb765afd7e756b68131c97c30d210de115d6b50) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mcf-02.14a",    0x000001, 0x200000, CRC(21251b33) SHA1(d252fe5c6eef8cbc9327e4176b4868b1cb17a738) )
	ROM_LOAD16_BYTE( "mcf-04.14h",    0x000000, 0x200000, CRC(4fa55577) SHA1(e229ba9cce46b92ce255aa33b974e19b214c4017) )
	ROM_LOAD16_BYTE( "mcf-01.13a",    0x400001, 0x200000, CRC(83881e25) SHA1(ae82cf0f704e6efea94c6c1d276d4e3e5b3ebe43) )
	ROM_LOAD16_BYTE( "mcf-03.14d",    0x400000, 0x200000, CRC(faf1d51d) SHA1(675dbbfe15b8010d54b2b3af26d42cdd753c2ce2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "sa01-0.13h",    0x00000, 0x40000,  CRC(cea8368e) SHA1(1fcc641381fdc29bd50d3a4b23e67647f79e505a))

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcf-05.12f",    0x00000, 0x200000, CRC(f007d376) SHA1(4ba20e5dabeacc3278b7f30c4462864cbe8f6984) )
ROM_END

/*

Chain Reaction
Data East 1995

DE-0409-1

156           E1
       223    2063     93C45
              2063
                              223
               141
       28MHz         5864
                     5864
            6264
            6264
  MCC-00

  U5  U6     52      MCC-03   AD-65
  U3  U4             MCC-04   AD-65


*/

ROM_START( chainrec )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "e1",    0x000000, 0x080000, CRC(8a8340ef) SHA1(4aaee56127b73453b862ff2a33dc241eeabf5658) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcc-00",    0x000000, 0x100000, CRC(646b03ec) SHA1(9a2fc11b1575032b5a784d88c3a90913068d1e69) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "u3",    0x000000, 0x080000, CRC(92659721) SHA1(b446ce98ec9c2c16375ef00639cfb463b365b8f7) )
	ROM_LOAD32_BYTE( "u4",    0x000002, 0x080000, CRC(e304eb32) SHA1(61a647ec89695a6b25ff924bdc6d29cbd7aca82b) )
	ROM_LOAD32_BYTE( "u5",    0x000001, 0x080000, CRC(1b6f01ea) SHA1(753fc670707432e317d035b09b0bad0762fea731) )
	ROM_LOAD32_BYTE( "u6",    0x000003, 0x080000, CRC(531a56f2) SHA1(89602bb873a3b110bffc216f921ba228e53380f9) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mcc-04",    0x00000, 0x40000,  CRC(86ee6ade) SHA1(56ad3f432c7f430f19fcba7c89940c63da165906) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcc-03",    0x00000, 0x100000, CRC(da2ebba0) SHA1(96d31dea4c7226ee1d386b286919fa334388c7a1) )
ROM_END

ROM_START( magdrop )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "re00-2.e1",    0x000000, 0x080000,  CRC(7138f10f) SHA1(ca93c3c2dc9a7dd6901c8429a6bf6883076a9b8f) )
//  ROM_LOAD( "re-00-2.1e",    0x000000, 0x080000,  CRC(d66d347f) SHA1(08de0415816730260d8e497b5cb06d2ab62fa70c) )
	/* one byte change.. which is good, or is it alt region? */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcc-00",    0x000000, 0x100000, CRC(646b03ec) SHA1(9a2fc11b1575032b5a784d88c3a90913068d1e69) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mcc-01.a13",    0x000001, 0x100000, CRC(13d88745) SHA1(0ce4ec1481f31be860ee80322de6e32f9a566229) )
	ROM_LOAD16_BYTE( "mcc-02.a14",    0x000000, 0x100000, CRC(d0f97126) SHA1(3848a6f00d0e57aaf383298c4d111eb63a88b073) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mcc-04",    0x00000, 0x40000,  CRC(86ee6ade) SHA1(56ad3f432c7f430f19fcba7c89940c63da165906) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcc-03",    0x00000, 0x100000, CRC(da2ebba0) SHA1(96d31dea4c7226ee1d386b286919fa334388c7a1) )

	ROM_REGION( 0x80, REGION_USER1, 0 ) /* eeprom */
	ROM_LOAD( "93c45.h2",    0x00, 0x80, CRC(16ce8d2d) SHA1(1a6883c75d34febbd92a16cfe204ff12550c85fd) )
ROM_END

ROM_START( magdropp )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "rz00-1.e1",    0x000000, 0x080000,  CRC(28caf639) SHA1(a17e792c82e65009e21680094acf093c0c4f1021) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcc-00",    0x000000, 0x100000, CRC(646b03ec) SHA1(9a2fc11b1575032b5a784d88c3a90913068d1e69) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mcc-01.a13",    0x000001, 0x100000, CRC(13d88745) SHA1(0ce4ec1481f31be860ee80322de6e32f9a566229) )
	ROM_LOAD16_BYTE( "mcc-02.a14",    0x000000, 0x100000, CRC(d0f97126) SHA1(3848a6f00d0e57aaf383298c4d111eb63a88b073) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mcc-04",    0x00000, 0x40000,  CRC(86ee6ade) SHA1(56ad3f432c7f430f19fcba7c89940c63da165906) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcc-03",    0x00000, 0x100000, CRC(da2ebba0) SHA1(96d31dea4c7226ee1d386b286919fa334388c7a1) )

	ROM_REGION( 0x80, REGION_USER1, 0 ) /* eeprom */
	ROM_LOAD( "eeprom.2h",    0x00, 0x80, CRC(d13d9edd) SHA1(e98ee2b696f0a7a8752dc30ef8b41bfe6598cbe4) )
ROM_END

/* Charlie Ninja

Charlie Ninja by Mitchell

rom E1  27c4002  labeled "ND 00-1"
rom H13  27c2001  labeled "ND 01-0"

maskrom b14  27c800  labeled "MBR-01"
maskrom b9  27c160  labeled "MBR-03"
maskrom h12  27c800  labeled "MBR-02"
maskrom h14  27c160  labeled "MBR-00"

28.8 mHz
oki m6295

*/

ROM_START( charlien )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "c-ninja.e1",    0x000000, 0x080000,  CRC(f18f4b23) SHA1(cb0c159b4dde3a3c5f295f270485996811e5e4d2) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c-ninja.b9",    0x000000, 0x080000, CRC(ecf2c7f0) SHA1(3c735a4eef2bc49f16ac9365a5689101f43c13e9) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c-ninja.b14",    0x000000, 0x100000, CRC(46c90215) SHA1(152acdeea34ec1db3f761066a0c1ff6e43e47f9d) )
	/* i think we're missing a rom .. c-ninja.b9 was in here twice .. */
	ROM_LOAD16_BYTE( "c-ninja.gfx",    0x000001, 0x100000, NO_DUMP )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "c-ninja.h13",    0x00000, 0x40000,  CRC(635a100a) SHA1(f6ec70890892e7557097ccd519de37247bb8c98d) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "c-ninja.h12",    0x00000, 0x100000, CRC(4f67d333) SHA1(608f921bfa6b7020c0ce72e5229b3f1489208b23) )
ROM_END

/* Joe and Mac Returns

Joe and Mac Returns
Data East 1994

DE-0491-1

156         MW00
      223              223
             141

  MBN00

  MBN01   52   MBN03  M6295
  MBN02        MBN04  M6295

*/

ROM_START( joemacr )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "mw00",    0x000000, 0x080000,  CRC(e1b78f40) SHA1(e611c317ada5a049a5e05d69c051e22a43fa2845) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) // rebuilt with roms from other set
	ROM_LOAD( "mbn00",    0x000000, 0x100000, CRC(11b2dac7) SHA1(71a50f606caddeb0ef266e2d3df9e429a4873f21) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mbn01",    0x000001, 0x080000, CRC(a3a37353) SHA1(c4509c8268afb647c20e71b42ae8ebd2bdf075e6) )
	ROM_LOAD16_BYTE( "mbn02",    0x000000, 0x080000,  CRC(aa2230c5) SHA1(43b7ac5c69cde1840a5255a8897e1c5d5f89fd7b) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */  // this could be bad, could probably be fixed with other set
	ROM_LOAD( "mbn04",    0x00000, 0x80000,  CRC(a4226947) SHA1(c6f7aae41bddd474b83916003d1e16ba0f1fe52c) ) // both halves identical (bad?)

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mbn03",    0x00000, 0x200000, CRC(70b71a2a) SHA1(45851b0692de73016fc9b913316001af4690534c) )
ROM_END

/*
This is a bootleg board, dumped by Marco_A
there was a 42-pin rom which isn't dumped, i imagine its sound..

there is a rom which still looks to be encrypted, i guess its the cpu code
*/

ROM_START( joemacra )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "05.rom",    0x000000, 0x080000,  CRC(74e9a158) SHA1(eee447303ac0884e152b89f59a9694afade87336) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )  // rebuilt with roms from other set
	ROM_LOAD( "01.rom",    0x000000, 0x080000,  CRC(4da4a2c1) SHA1(1ed4bd4337d8b185b56e326e662a8715e4d09e17) )
	ROM_LOAD( "02.rom",    0x080000, 0x080000,  CRC(642c08db) SHA1(9a541fd56ae34c24f803e08869702be6fafd81d1) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mbn01",    0x000001, 0x080000, CRC(a3a37353) SHA1(c4509c8268afb647c20e71b42ae8ebd2bdf075e6) ) // 03.bin
	ROM_LOAD16_BYTE( "mbn02",    0x000000, 0x080000,  CRC(aa2230c5) SHA1(43b7ac5c69cde1840a5255a8897e1c5d5f89fd7b) ) // 04.bin

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "07.rom",    0x00000, 0x40000,  CRC(dcbd4771) SHA1(2a1ab6b0fc372333c7eb17aab077fe1ca5ba1dea) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) not in this set */
	ROM_LOAD( "mbn03",    0x00000, 0x200000, CRC(70b71a2a) SHA1(45851b0692de73016fc9b913316001af4690534c) )
ROM_END


/*

Ganbare! Gonta!! 2 (Lady Killer Part 2 - Party Time)
(c)1995 Mitchell
DEC-22V0 (board is manufactured by DECO)
MT5601-0

CPU  : surface-scratched 100pin PQFP DECO custom?
Sound: M6295x2
OSC  : 28.0000MHz

ROMs:
rd_00-0.1e - Main program (27c4096)

mcb-00.9a  - Graphics? (23c16000)

mcb-01.13a - Graphics (23c16000)
mcb-02.14a |
mcb-03.14d |
mcb-05.14h /

rd_01-0.13h - Samples (27c020)

mcb-04.12f - Samples (23c16000)

GALs (16V8B, not dumped):
vz-00.5c
vz-01.4e
vz-02.8f

Custom chips:
Surface-scratched 128pin PQFP (location 12d) DECO 52?
Surface-scratched 100pin PQFP (location 5j)
Surface-scratched 160pin PQFP (location 6e) DECO 56?
Surface-scratched 100pin PQFP (location 2a)
Surface-scratched 100pin PQFP (location 3d)

Other:
EEPROM 93C45

*/

ROM_START( prtytime )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "pz_00-0.1e",    0x000000, 0x080000, CRC(ec715c87) SHA1(c9f28399d59b37977f31a5c67cb97af6c58947ae) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcb-00.9a",    0x000000, 0x080000, CRC(c48a4f2b) SHA1(2dee5f8507b2a7e6f7e44b14f9abca36d0ebf78b) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mcb-02.14a",    0x000001, 0x200000, CRC(423cfb38) SHA1(b8c772a8ab471c365a11a88c85e1c8c7d2ad6e80) )
	ROM_LOAD16_BYTE( "mcb-05.14h",    0x000000, 0x200000, CRC(81540cfb) SHA1(6f7bc62c3c4d4a29eb1e0cfb261ace461bbca57c) )
	ROM_LOAD16_BYTE( "mcb-01.13a",    0x400001, 0x200000, CRC(06f40a57) SHA1(896f1d373e911dcff7223bf21756ad35b28b4c5d) )
	ROM_LOAD16_BYTE( "mcb-03.14d",    0x400000, 0x200000, CRC(0aef73af) SHA1(76cf13f53da5202da80820f98660edee1eef7f1a) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "pz_01-0.13h",    0x00000, 0x40000,  CRC(8925bce2) SHA1(0ff2d5db7a24a2af30bd753eba274572c32cc2e7) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcb-04.12f",    0x00000, 0x200000, CRC(e23d3590) SHA1(dc8418edc525f56e84f26e9334d5576000b14e5f) )
ROM_END

ROM_START( gangonta )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "rd_00-0.1e",    0x000000, 0x080000, CRC(f80f43bb) SHA1(f9d26829eb90d41a6c410d4d673fe9595f814868) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcb-00.9a",    0x000000, 0x080000, CRC(c48a4f2b) SHA1(2dee5f8507b2a7e6f7e44b14f9abca36d0ebf78b) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mcb-02.14a",    0x000001, 0x200000, CRC(423cfb38) SHA1(b8c772a8ab471c365a11a88c85e1c8c7d2ad6e80) )
	ROM_LOAD16_BYTE( "mcb-05.14h",    0x000000, 0x200000, CRC(81540cfb) SHA1(6f7bc62c3c4d4a29eb1e0cfb261ace461bbca57c) )
	ROM_LOAD16_BYTE( "mcb-01.13a",    0x400001, 0x200000, CRC(06f40a57) SHA1(896f1d373e911dcff7223bf21756ad35b28b4c5d) )
	ROM_LOAD16_BYTE( "mcb-03.14d",    0x400000, 0x200000, CRC(0aef73af) SHA1(76cf13f53da5202da80820f98660edee1eef7f1a) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "rd_01-0.13h",    0x00000, 0x40000,  CRC(70fd18c6) SHA1(368cd8e10c5f5a13eb3813974a7e6b46a4fa6b6c) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcb-04.12f",    0x00000, 0x200000, CRC(e23d3590) SHA1(dc8418edc525f56e84f26e9334d5576000b14e5f) )
ROM_END


/* Everything seems more stable if we run the CPU speed x4 and use Idle skips.. maybe it has an internal multipler? */
static READ32_HANDLER( joemacr_speedup_r )
{
	if (activecpu_get_pc()==0x284)  cpu_spinuntil_time(TIME_IN_USEC(400));
	return simpl156_systemram[0x18/4];
}

static DRIVER_INIT (joemacr)
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0201018, 0x020101b, 0, 0, joemacr_speedup_r );
	init_simpl156();
}

static READ32_HANDLER( chainrec_speedup_r )
{
	if (activecpu_get_pc()==0x2d4)  cpu_spinuntil_time(TIME_IN_USEC(400));
	return simpl156_systemram[0x18/4];
}

static DRIVER_INIT (chainrec)
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0201018, 0x020101b, 0, 0, chainrec_speedup_r );
	init_simpl156();
}

static READ32_HANDLER( prtytime_speedup_r )
{
	if (activecpu_get_pc()==0x4f0)  cpu_spinuntil_time(TIME_IN_USEC(400));
	return simpl156_systemram[0xae0/4];
}

static DRIVER_INIT (prtytime)
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0201ae0, 0x0201ae3, 0, 0, prtytime_speedup_r );
	init_simpl156();
}

static READ32_HANDLER( charlien_speedup_r )
{
	if (activecpu_get_pc()==0xc8c8)  cpu_spinuntil_time(TIME_IN_USEC(400));
	return simpl156_systemram[0x10/4];
}

static DRIVER_INIT (charlien)
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0201010, 0x0201013, 0, 0, charlien_speedup_r );
	init_simpl156();
}


GAMEX(1996, osman,   0,        mitchell156,    simpl156, simpl156,      ROT0, "Mitchell", "Osman", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING) // doesn't boot without hack
GAMEX(1995, chainrec,0,        chainrec,       simpl156, chainrec,      ROT0, "Data East","Chain Reaction", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING) // not sure its stable
GAMEX(1995, magdrop, chainrec, magdrop,        simpl156, chainrec,      ROT0, "Data East","Magical Drop", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING) // not sure its stable
GAMEX(1995, magdropp,chainrec, magdropp,       simpl156, chainrec,      ROT0, "Data East","Magical Drop Plus", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING) // not sure its stable
GAMEX(1995, charlien,0,        mitchell156,    simpl156, charlien,      ROT0, "Mitchell", "Charlie Ninja", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING) // bad dump
GAMEX(1994, joemacr, 0,        joemacr,        simpl156, joemacr,       ROT0, "Data East", "Joe & Mac Returns", GAME_IMPERFECT_GRAPHICS|GAME_IMPERFECT_SOUND)
GAMEX(1994, joemacra,joemacr,  joemacr,        simpl156, joemacr,       ROT0, "Data East", "Joe & Mac Returns (set 2)", GAME_IMPERFECT_GRAPHICS|GAME_IMPERFECT_SOUND)
GAMEX(1995, prtytime, 0,       mitchell156,    simpl156, prtytime,      ROT90,"Mitchell", "Party Time: Gonta the Diver II", GAME_IMPERFECT_GRAPHICS|GAME_IMPERFECT_SOUND)
GAMEX(1995, gangonta, prtytime,mitchell156,    simpl156, prtytime,      ROT90,"Mitchell", "Ganbare! Gonta!! 2", GAME_IMPERFECT_GRAPHICS|GAME_IMPERFECT_SOUND)
