/***************************************************************************
Namco System 21

Uses the same CPU/sound board as Namco System2,

Sprite hardware is identical to Namco NB1.

There are no tilemaps.

The 3d hardware works as follows:
The main CPUs populate a chunk of shared RAM with references to specific
3d objects and their position/orientation in 3d space (13 bytes per entry).

The main CPU also specified attributes for a master camera which provides
additional global transformations for all objects.

Point data in ROMS (24 bit words) encodes the 3d primitives.

Many thanks to Aaron Giles for making sense of the format:

The first part of the Point ROMs is an address table.  The first entry serves
a special (unknown) purpose.

Given an object index, this table provides an address into the second part of
the ROM.

The second part of the ROM is a series of display lists.
This is a sequence of pointers to actual polygon data. There may be
more than one, and the list is terminated by $ffffff.

The remainder of the ROM is a series of polygon data. The first word of each
entry is the length of the entry (in words, not counting the length word).

The rest of the data in each entry is organized as follows:

length (1 word)
unknown value (1 word) - this increments with each entry
vertex count (1 word) - the number of vertices encoded
unknown value (1 word) - almost always 0
vertex list (n x 3 words) - presumably x,y,z coordinates
quad count (1 word) - the number of quads to draw
quad primitives (n x 5 words) - the four verticies plus a color code



Known Issues:
- Starblade has some corrupt point data
- the camera transformation isn't quite right
- transformations for individual 3d objects may be wrong
- some DSP RAM status read/writes not emulated
- Cyber Sled isn't updating DSP RAM (due to missing status reads)
- Solvalou locks up on startup (not yet investigated)
- Winning Run '91 2d Graphics are unpacked incorrectly
- Winning Run '91 has different DSP ROMs
- lamps/vibration outputs not mapped
- MCU program may be wrong (it's copied from Namco SystemII)

Memory Map:
	Work RAM
		0x100000..0x10ffff

	DSP RAM
		200000..20ffff

	Object RAM
		700000..700fff	spriteram
		701000..701fff	spriteram
		702000..7021ff	spritelist
		702200..7023ff	spritelist
		702400..70243f	?
		704000..707fff	format
		708000..70ffff	tiles
		710000..710fff	spriteram2
		714000..7141ff	spritelist2
		720000..72000f	?
		740000..74ffff	palette (r,g)
		750000..75ffff	palette (b)
		760000..760001	0040 (?)

	Data1 (ROM):
		800000..8fffff

	Communications RAM:
		900000..90ffff

	Data2 (ROM)
		d00000..dfffff

-------------------
Air Combat by NAMCO
-------------------
malcor


Location        Device     File ID      Checksum
-------------------------------------------------
CPU68  1J       27C4001    MPR-L.AC1      9859   [ main program ]  [ rev AC1 ]
CPU68  3J       27C4001    MPR-U.AC1      97F1   [ main program ]  [ rev AC1 ]
CPU68  1J       27C4001    MPR-L.AC2      C778   [ main program ]  [ rev AC2 ]
CPU68  3J       27C4001    MPR-U.AC2      6DD9   [ main program ]  [ rev AC2 ]
CPU68  1C      MB834000    EDATA1-L.AC1   7F77   [    data      ]
CPU68  3C      MB834000    EDATA1-U.AC1   FA2F   [    data      ]
CPU68  3A      MB834000    EDATA-U.AC1    20F2   [    data      ]
CPU68  1A      MB834000    EDATA-L.AC1    9E8A   [    data      ]
CPU68  8J        27C010    SND0.AC1       71A8   [  sound prog  ]
CPU68  12B     MB834000    VOI0.AC1       08CF   [   voice 0    ]
CPU68  12C     MB834000    VOI1.AC1       925D   [   voice 1    ]
CPU68  12D     MB834000    VOI2.AC1       C498   [   voice 2    ]
CPU68  12E     MB834000    VOI3.AC1       DE9F   [   voice 3    ]
CPU68  4C        27C010    SPR-L.AC1      473B   [ slave prog L ]  [ rev AC1 ]
CPU68  6C        27C010    SPR-U.AC1      CA33   [ slave prog U ]  [ rev AC1 ]
CPU68  4C        27C010    SPR-L.AC2      08CE   [ slave prog L ]  [ rev AC2 ]
CPU68  6C        27C010    SPR-U.AC2      A3F1   [ slave prog U ]  [ rev AC2 ]
OBJ(B) 5S       HN62344    OBJ0.AC1       CB72   [ object data  ]
OBJ(B) 5X       HN62344    OBJ1.AC1       85E2   [ object data  ]
OBJ(B) 3S       HN62344    OBJ2.AC1       89DC   [ object data  ]
OBJ(B) 3X       HN62344    OBJ3.AC1       58FF   [ object data  ]
OBJ(B) 4S       HN62344    OBJ4.AC1       46D6   [ object data  ]
OBJ(B) 4X       HN62344    OBJ5.AC1       7B91   [ object data  ]
OBJ(B) 2S       HN62344    OBJ6.AC1       5736   [ object data  ]
OBJ(B) 2X       HN62344    OBJ7.AC1       6D45   [ object data  ]
OBJ(B) 17N     PLHS18P8    3P0BJ3         4342
OBJ(B) 17N     PLHS18P8    3POBJ4         1143
DSP    2N       HN62344    AC1-POIL.L     8AAF   [   DSP data   ]
DSP    2K       HN62344    AC1-POIL.L     CF90   [   DSP data   ]
DSP    2E       HN62344    AC1-POIH       4D02   [   DSP data   ]
DSP    17D     GAL16V8A    3PDSP5         6C00


NOTE:  CPU68  - CPU board        2252961002  (2252971002)
       OBJ(B) - Object board     8623961803  (8623963803)
       DSP    - DSP board        8623961703  (8623963703)
       PGN(C) - PGN board        2252961300  (8623963600)

       Namco System 21 Hardware

       ROMs that have the same locations are different revisions
       of the same ROMs (AC1 or AC2).


Jumper settings:


Location    Position set    alt. setting
----------------------------------------

CPU68 PCB:

  JP2          /D-ST           /VBL
  JP3

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/namcos2.h"
#include "cpu/m6809/m6809.h"
#include "namcos21.h"

/**************************************************************************/

int namcos21_gametype;

static data16_t *namcos21_data16;
static data16_t *shareram16;
data16_t *namcos21_dspram16;
static data8_t *namcos2_dualportram;


/**************************************************************************/

static READ16_HANDLER( namcos2_68k_dualportram_word_r )
{
	return namcos2_dualportram[offset];
}

static WRITE16_HANDLER( namcos2_68k_dualportram_word_w )
{
	if( ACCESSING_LSB )
	{
		namcos2_dualportram[offset] = data & 0xff;
	}
}

static READ_HANDLER( namcos2_dualportram_byte_r )
{
	return namcos2_dualportram[offset];
}

static WRITE_HANDLER( namcos2_dualportram_byte_w )
{
	namcos2_dualportram[offset] = data;
}

/**************************************************************************/

static READ16_HANDLER( shareram16_r )
{
	return shareram16[offset];
}

static WRITE16_HANDLER( shareram16_w )
{
	COMBINE_DATA( &shareram16[offset] );
}

/**************************************************************************/

static READ16_HANDLER( dspram16_r )
{
	return namcos21_dspram16[offset];
}

static WRITE16_HANDLER( dspram16_w )
{
	COMBINE_DATA( &namcos21_dspram16[offset] );
}

static READ16_HANDLER( dsp_status_r )
{
	return 1;
}

/**************************************************************************/

static READ16_HANDLER( data_r )
{
	return namcos21_data16[offset];
}

static READ16_HANDLER( data2_r )
{
	return namcos21_data16[0x100000/2+offset];
}

/*************************************************************/
/* MASTER 68000 CPU Memory declarations 					 */
/*************************************************************/

static MEMORY_READ16_START( readmem_master_default )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },		/* MAIN WORK RAM */
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_r },
	{ 0x200000, 0x20ffff, MRA16_RAM },		/* DSP RAM */
	{ 0x440000, 0x440001, dsp_status_r },
	{ 0x480000, 0x4807ff, MRA16_RAM },		/* unknown (aircombt) */
	{ 0x700000, 0x760001, MRA16_RAM },		/* OBJECT RAM */
	{ 0x800000, 0x8fffff, data_r },
	{ 0x900000, 0x90ffff, shareram16_r },	/* MAIN COMM RAM */
	{ 0xa00000, 0xa00fff, namcos2_68k_dualportram_word_r },
	{ 0xb00000, 0xb03fff, MRA16_RAM },		/* unknown (aircombt) */
	{ 0xd00000, 0xdfffff, data2_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem_master_default )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },		/* MAIN WORK RAM */
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_w },
	{ 0x200000, 0x20ffff, MWA16_RAM, &namcos21_dspram16 },
	{ 0x400000, 0x40ffff, MWA16_NOP },
	{ 0x440000, 0x47ffff, MWA16_NOP },
	{ 0x480000, 0x4807ff, MWA16_RAM },		/* unknown (aircombt) */
	{ 0x700000, 0x760001, MWA16_RAM, &spriteram16 },
	{ 0x900000, 0x90ffff, shareram16_w, &shareram16 }, /* MAIN COMM RAM */
	{ 0xa00000, 0xa00fff, namcos2_68k_dualportram_word_w },
	{ 0xb00000, 0xb03fff, MWA16_RAM },		/* unknown (aircombt) */
//	{ 0xb80000, 0xb8000f, MWA16_NOP },
MEMORY_END

/*************************************************************/
/* SLAVE 68000 CPU Memory declarations						 */
/*************************************************************/

static MEMORY_READ16_START( readmem_slave_default )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x13ffff, MRA16_RAM },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_r },
	{ 0x200000, 0x20ffff, dspram16_r },
	{ 0x700000, 0x760001, spriteram16_r },
	{ 0x800000, 0x8fffff, data_r },
	{ 0x900000, 0x90ffff, shareram16_r },
	{ 0xa00000, 0xa00fff, namcos2_68k_dualportram_word_r },
	{ 0xd00000, 0xdfffff, data2_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem_slave_default )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x13ffff, MWA16_RAM },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_w },
	{ 0x200000, 0x20ffff, dspram16_w },
	{ 0x700000, 0x760001, spriteram16_w },
	{ 0x900000, 0x90ffff, shareram16_w },
	{ 0xa00000, 0xa00fff, namcos2_68k_dualportram_word_w },
MEMORY_END

/* Sound CPU */

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x3fff, BANKED_SOUND_ROM_R }, /* banked */
	{ 0x4000, 0x4001, YM2151_status_port_0_r },
	{ 0x5000, 0x6fff, C140_r },
	{ 0x7000, 0x77ff, namcos2_dualportram_byte_r },
	{ 0x7800, 0x7fff, namcos2_dualportram_byte_r },	/* mirror */
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xd000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4000, YM2151_register_port_0_w },
	{ 0x4001, 0x4001, YM2151_data_port_0_w },
	{ 0x5000, 0x6fff, C140_w },
	{ 0x7000, 0x77ff, namcos2_dualportram_byte_w, &namcos2_dualportram },
	{ 0x7800, 0x7fff, namcos2_dualportram_byte_w }, /* mirror */
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xbfff, MWA_NOP }, /* amplifier enable on 1st write */
	{ 0xc000, 0xc001, namcos2_sound_bankselect_w },
	{ 0xd001, 0xd001, MWA_NOP }, /* watchdog */
	{ 0xc000, 0xffff, MWA_NOP }, /* avoid debug log noise; games write frequently to 0xe000 */
MEMORY_END

/* IO CPU */

static MEMORY_READ_START( readmem_mcu )
	{ 0x0000, 0x0000, MRA_NOP },
	{ 0x0001, 0x0001, input_port_0_r },			/* p1,p2 start */
	{ 0x0002, 0x0002, input_port_1_r },			/* coins */
	{ 0x0003, 0x0003, namcos2_mcu_port_d_r },
	{ 0x0007, 0x0007, input_port_10_r },		/* fire buttons */
	{ 0x0010, 0x0010, namcos2_mcu_analog_ctrl_r },
	{ 0x0011, 0x0011, namcos2_mcu_analog_port_r },
	{ 0x0008, 0x003f, MRA_RAM },
	{ 0x0040, 0x01bf, MRA_RAM },
	{ 0x01c0, 0x1fff, MRA_ROM },
	{ 0x2000, 0x2000, input_port_11_r }, /* dipswitches */
	{ 0x3000, 0x3000, input_port_12_r }, /* DIAL0 */
	{ 0x3001, 0x3001, input_port_13_r }, /* DIAL1 */
	{ 0x3002, 0x3002, input_port_14_r }, /* DIAL2 */
	{ 0x3003, 0x3003, input_port_15_r }, /* DIAL3 */
	{ 0x5000, 0x57ff, namcos2_dualportram_byte_r },
	{ 0x6000, 0x6fff, MRA_NOP }, /* watchdog */
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_mcu )
	{ 0x0003, 0x0003, namcos2_mcu_port_d_w },
	{ 0x0010, 0x0010, namcos2_mcu_analog_ctrl_w },
	{ 0x0011, 0x0011, namcos2_mcu_analog_port_w },
	{ 0x0000, 0x003f, MWA_RAM },
	{ 0x0040, 0x01bf, MWA_RAM },
	{ 0x01c0, 0x1fff, MWA_ROM },
	{ 0x5000, 0x57ff, namcos2_dualportram_byte_w, &namcos2_dualportram },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static struct GfxLayout tile_layout =
{
	16,16,
	RGN_FRAC(1,4),	/* number of tiles */
	8,		/* bits per pixel */
	{		/* plane offsets */
		0,1,2,3,4,5,6,7
	},
	{ /* x offsets */
		0*8,RGN_FRAC(1,4)+0*8,RGN_FRAC(2,4)+0*8,RGN_FRAC(3,4)+0*8,
		1*8,RGN_FRAC(1,4)+1*8,RGN_FRAC(2,4)+1*8,RGN_FRAC(3,4)+1*8,
		2*8,RGN_FRAC(1,4)+2*8,RGN_FRAC(2,4)+2*8,RGN_FRAC(3,4)+2*8,
		3*8,RGN_FRAC(1,4)+3*8,RGN_FRAC(2,4)+3*8,RGN_FRAC(3,4)+3*8
	},
	{ /* y offsets */
		0*32,1*32,2*32,3*32,
		4*32,5*32,6*32,7*32,
		8*32,9*32,10*32,11*32,
		12*32,13*32,14*32,15*32
	},
	8*64 /* sprite offset */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &tile_layout,  0*256, 0x80 },
	{ -1 }
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,	/* 3.58 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ NULL }	/* YM2151 IRQ line is NOT connected on the PCB */
};

static struct C140interface C140_interface =
{
	8000000/374,
	REGION_SOUND1,
	50
};

static const struct MachineDriver machine_driver_poly =
{
	{
		{
			CPU_M68000, /* Master */
			12288000,
			readmem_master_default,writemem_master_default,0,0,
			namcos2_68k_master_vblank,1
		},
		{
			CPU_M68000, /* Slave */
			12288000,
			readmem_slave_default,writemem_slave_default,0,0,
			namcos2_68k_slave_vblank,1
		},
		{
			CPU_M6809, /* Sound */
			3072000,
			readmem_sound,writemem_sound,0,0,
			interrupt,2,
			namcos2_sound_interrupt,120
		},
		{
			CPU_HD63705, /* IO */
			2048000,
			readmem_mcu,writemem_mcu,0,0,
			namcos2_mcu_interrupt,1,
			0,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100, /* 100 CPU slices per frame */
	namcos2_init_machine,

	/* video hardware */
	62*8, 60*8, { 0*8, 62*8-1, 0*8, 60*8-1 },
	gfxdecodeinfo,
	NAMCOS21_NUM_COLORS,NAMCOS21_NUM_COLORS,
	0,
	VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN,
	0,
	namcos21_vh_start,
	namcos21_vh_stop,
	namcos21_vh_update_default,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	/* Sound struct here */
	{
		{
			SOUND_C140,
			&C140_interface
		},
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	},
	namcos2_nvram_handler
};

ROM_START( aircombu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "mpr-u.ac2",  0x000000, 0x80000, 0xa7133f85 )
	ROM_LOAD16_BYTE( "mpr-l.ac2",  0x000001, 0x80000, 0x520a52e6 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "spr-u.ac2",  0x000000, 0x20000, 0x42aca956 )
	ROM_LOAD16_BYTE( "spr-l.ac2",  0x000001, 0x20000, 0x3e15fa19 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "snd0.ac1",	0x00c000, 0x004000, 0x5c1fb84b )
	ROM_CONTINUE(      		0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "obj0.ac2",  0x000000, 0x80000, 0x8327ff22 )
	ROM_LOAD( "obj4.ac2",  0x080000, 0x80000, 0xe433e344 )
	ROM_LOAD( "obj1.ac2",  0x100000, 0x80000, 0x43af566d )
	ROM_LOAD( "obj5.ac2",  0x180000, 0x80000, 0xecb19199 )
	ROM_LOAD( "obj2.ac2",  0x200000, 0x80000, 0xdafbf489 )
	ROM_LOAD( "obj6.ac2",  0x280000, 0x80000, 0x24cc3f36 )
	ROM_LOAD( "obj3.ac2",  0x300000, 0x80000, 0xbd555a1d )
	ROM_LOAD( "obj7.ac2",  0x380000, 0x80000, 0xd561fbe3 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* collision */
	ROM_LOAD16_BYTE( "edata-u.ac1",		0x000000, 0x80000, 0x82320c71 )
	ROM_LOAD16_BYTE( "edata-l.ac1", 	0x000001, 0x80000, 0xfd7947d3 )
	ROM_LOAD16_BYTE( "edata1-u.ac2",	0x100000, 0x80000, 0x40c07095 )
	ROM_LOAD16_BYTE( "edata1-l.ac1",	0x100001, 0x80000, 0xa87087dd )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "poih.ac1",   0x000001, 0x80000, 0x573bbc3b )	/* most significant */
	ROM_LOAD32_BYTE( "poil-u.ac1", 0x000002, 0x80000, 0xd99084b9 )
	ROM_LOAD32_BYTE( "poil-l.ac1", 0x000003, 0x80000, 0xabb32307 )	/* least significant */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("voi0.ac1",0x000000,0x80000,0xf427b119 )
	ROM_LOAD("voi1.ac1",0x080000,0x80000,0xc9490667 )
	ROM_LOAD("voi2.ac1",0x100000,0x80000,0x1fcb51ba )
	ROM_LOAD("voi3.ac1",0x180000,0x80000,0xcd202e06 )
ROM_END

ROM_START( aircombj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "mpr-u.ac1",  0x000000, 0x80000, 0xa4dec813 )
	ROM_LOAD16_BYTE( "mpr-l.ac1",  0x000001, 0x80000, 0x8577b6a2 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "spr-u.ac1",  0x000000, 0x20000, 0x5810e219 )
	ROM_LOAD16_BYTE( "spr-l.ac1",  0x000001, 0x20000, 0x175a7d6c )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "snd0.ac1",	0x00c000, 0x004000, 0x5c1fb84b )
	ROM_CONTINUE(			0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "obj0.ac1",  0x000000, 0x80000, 0xd2310c6a )
	ROM_LOAD( "obj4.ac1",  0x080000, 0x80000, 0x0c93b478 )
	ROM_LOAD( "obj1.ac1",  0x100000, 0x80000, 0xf5783a77 )
	ROM_LOAD( "obj5.ac1",  0x180000, 0x80000, 0x476aed15 )
	ROM_LOAD( "obj2.ac1",  0x200000, 0x80000, 0x01343d5c )
	ROM_LOAD( "obj6.ac1",  0x280000, 0x80000, 0xc67607b1 )
	ROM_LOAD( "obj3.ac1",  0x300000, 0x80000, 0x7717f52e )
	ROM_LOAD( "obj7.ac1",  0x380000, 0x80000, 0xcfa9fe5f )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "edata-u.ac1",   0x000000, 0x80000, 0x82320c71 )
	ROM_LOAD16_BYTE( "edata-l.ac1",   0x000001, 0x80000, 0xfd7947d3 )
	ROM_LOAD16_BYTE( "edata1-u.ac1",  0x100000, 0x80000, 0xa9547509 )
	ROM_LOAD16_BYTE( "edata1-l.ac1",  0x100001, 0x80000, 0xa87087dd )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "poih.ac1",   0x000001, 0x80000, 0x573bbc3b )	/* most significant */
	ROM_LOAD32_BYTE( "poil-u.ac1", 0x000002, 0x80000, 0xd99084b9 )
	ROM_LOAD32_BYTE( "poil-l.ac1", 0x000003, 0x80000, 0xabb32307 )	/* least significant */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("voi0.ac1",0x000000,0x80000,0xf427b119 )
	ROM_LOAD("voi1.ac1",0x080000,0x80000,0xc9490667 )
	ROM_LOAD("voi2.ac1",0x100000,0x80000,0x1fcb51ba )
	ROM_LOAD("voi3.ac1",0x180000,0x80000,0xcd202e06 )
ROM_END

ROM_START( cybsled )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "mpru.3j",  0x000000, 0x80000, 0xcc5a2e83 )
	ROM_LOAD16_BYTE( "mprl.1j",  0x000001, 0x80000, 0xf7ee8b48 )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "spru.6c",  0x000000, 0x80000, 0x28dd707b )
	ROM_LOAD16_BYTE( "sprl.4c",  0x000001, 0x80000, 0x437029de )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "snd0.8j",	0x00c000, 0x004000, 0x3dddf83b )
	ROM_CONTINUE(			0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "obj0.5s",  0x000000, 0x80000, 0x5ae542d5 )
	ROM_LOAD( "obj4.4s",  0x080000, 0x80000, 0x57904076 )
	ROM_LOAD( "obj1.5x",  0x100000, 0x80000, 0x4aae3eff )
	ROM_LOAD( "obj5.4x",  0x180000, 0x80000, 0x0e11ca47 )
	ROM_LOAD( "obj2.3s",  0x200000, 0x80000, 0xd64ec4c3 )
	ROM_LOAD( "obj6.2s",  0x280000, 0x80000, 0x7748b485 )
	ROM_LOAD( "obj3.3x",  0x300000, 0x80000, 0x3d1f7168 )
	ROM_LOAD( "obj7.2x",  0x380000, 0x80000, 0xb6eb6ad2 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "datau.3a",   0x000000, 0x80000, 0x570da15d )
	ROM_LOAD16_BYTE( "datal.1a",   0x000001, 0x80000, 0x9cf96f9e )
	ROM_LOAD16_BYTE( "edata0u.3b", 0x100000, 0x80000, 0x77452533 )
	ROM_LOAD16_BYTE( "edata0l.1b", 0x100001, 0x80000, 0xe812e290 )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "poih1.2f",  0x000001, 0x80000, 0xeaf8bac3 )	/* most significant */
	ROM_LOAD32_BYTE( "poilu1.2k", 0x000002, 0x80000, 0xc544a8dc )
	ROM_LOAD32_BYTE( "poill1.2n", 0x000003, 0x80000, 0x30acb99b )	/* least significant */
	ROM_LOAD32_BYTE( "poih2.2j",  0x200001, 0x80000, 0x4079f342 )
	ROM_LOAD32_BYTE( "poilu2.2l", 0x200002, 0x80000, 0x61d816d4 )
	ROM_LOAD32_BYTE( "poill2.2p", 0x200003, 0x80000, 0xfaf09158 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("voi0.12b",0x000000,0x80000,0x99d7ce46 )
	ROM_LOAD("voi1.12c",0x080000,0x80000,0x2b335f06 )
	ROM_LOAD("voi2.12d",0x100000,0x80000,0x10cd15f0 )
	ROM_LOAD("voi3.12e",0x180000,0x80000,0xc902b4a4 )
ROM_END

ROM_START( starblad )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "st1_mpu.bin",  0x000000, 0x80000, 0x483a311c )
	ROM_LOAD16_BYTE( "st1_mpl.bin",  0x000001, 0x80000, 0x0a4dd661 )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "st1_spu.bin",  0x000000, 0x40000, 0x9f9a55db )
	ROM_LOAD16_BYTE( "st1_spl.bin",  0x000001, 0x40000, 0xacbe39c7 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "st1snd0.bin",		 0x00c000, 0x004000, 0xc0e934a3 )
	ROM_CONTINUE(					 0x010000, 0x01c000 )
	ROM_RELOAD( 					 0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "st1obj0.bin",  0x000000, 0x80000, 0x5d42c71e )
	ROM_LOAD( "st1obj1.bin",  0x080000, 0x80000, 0xc98011ad )
	ROM_LOAD( "st1obj2.bin",  0x100000, 0x80000, 0x6cf5b608 )
	ROM_LOAD( "st1obj3.bin",  0x180000, 0x80000, 0xcdc195bb )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "st1_du.bin",  0x000000, 0x20000, 0x2433e911 )
	ROM_LOAD16_BYTE( "st1_dl.bin",  0x000001, 0x20000, 0x4a2cc252 )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE ) /* 24bit signed point data */
	ROM_LOAD32_BYTE( "st1pt0h.bin", 0x000001, 0x80000, BADCRC(0x84eb355f) ) /* corrupt! */
	ROM_LOAD32_BYTE( "st1pt0u.bin", 0x000002, 0x80000, 0x1956cd0a )
	ROM_LOAD32_BYTE( "st1pt0l.bin", 0x000003, 0x80000, 0xff577049 )
	//
	ROM_LOAD32_BYTE( "st1pt1h.bin", 0x200001, 0x80000, BADCRC(0x96b1bd7d) ) /* corrupt! */
	ROM_LOAD32_BYTE( "st1pt1u.bin", 0x200002, 0x80000, 0xecf21047 )
	ROM_LOAD32_BYTE( "st1pt1l.bin", 0x200003, 0x80000, 0x01cb0407 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("st1voi0.bin",0x000000,0x80000,0x5b3d43a9 )
	ROM_LOAD("st1voi1.bin",0x080000,0x80000,0x413e6181 )
	ROM_LOAD("st1voi2.bin",0x100000,0x80000,0x067d0720 )
	ROM_LOAD("st1voi3.bin",0x180000,0x80000,0x8b5aa45f )
ROM_END

ROM_START( solvalou )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "sv1mpu.bin",  0x000000, 0x20000, 0xb6f92762 )
	ROM_LOAD16_BYTE( "sv1mpl.bin",  0x000001, 0x20000, 0x28c54c42 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "sv1spu.bin",  0x000000, 0x20000, 0xebd4bf82 )
	ROM_LOAD16_BYTE( "sv1spl.bin",  0x000001, 0x20000, 0x7acab679 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "sv1snd0.bin",    0x00c000, 0x004000, 0x5e007864 )
	ROM_CONTINUE(				0x010000, 0x01c000 )
	ROM_RELOAD(					0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sv1obj0.bin",  0x000000, 0x80000, 0x773798bb )
	ROM_LOAD( "sv1obj4.bin",  0x080000, 0x80000, 0x33a008a7 )
	ROM_LOAD( "sv1obj1.bin",  0x100000, 0x80000, 0xa36d9e79 )
	ROM_LOAD( "sv1obj5.bin",  0x180000, 0x80000, 0x31551245 )
	ROM_LOAD( "sv1obj2.bin",  0x200000, 0x80000, 0xc8672b8a )
	ROM_LOAD( "sv1obj6.bin",  0x280000, 0x80000, 0xfe319530 )
	ROM_LOAD( "sv1obj3.bin",  0x300000, 0x80000, 0x293ef1c5 )
	ROM_LOAD( "sv1obj7.bin",  0x380000, 0x80000, 0x95ed6dcb )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "sv1du.bin",  0x000000, 0x80000, 0x2e561996 )
	ROM_LOAD16_BYTE( "sv1dl.bin",  0x000001, 0x80000, 0x495fb8dd )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "sv1pt0h.bin", 0x000001, 0x80000, 0x3be21115 ) /* most significant */
	ROM_LOAD32_BYTE( "sv1pt0u.bin", 0x000002, 0x80000, 0x4aacfc42 )
	ROM_LOAD32_BYTE( "sv1pt0l.bin", 0x000003, 0x80000, 0x6a4dddff ) /* least significant */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("sv1voi0.bin",0x000000,0x80000,0x7f61bbcf )
	ROM_LOAD("sv1voi1.bin",0x080000,0x80000,0xc732e66c )
	ROM_LOAD("sv1voi2.bin",0x100000,0x80000,0x51076298 )
	ROM_LOAD("sv1voi3.bin",0x180000,0x80000,0x33085ff3 )
ROM_END

ROM_START( winrun91 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "mpu.3k",  0x000000, 0x20000, 0x80a0e5be )
	ROM_LOAD16_BYTE( "mpl.1k",  0x000001, 0x20000, 0x942172d8 )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "spu.6b",  0x000000, 0x20000, 0x0221d4b2 )
	ROM_LOAD16_BYTE( "spl.4b",  0x000001, 0x20000, 0x288799e2 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "snd0.7c",	0x00c000, 0x004000, 0x6a321e1e )
	ROM_CONTINUE(			0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "d0u.3a",  0x000000, 0x20000, 0xdcb27da5 )
	ROM_LOAD16_BYTE( "d0l.1a",  0x000001, 0x20000, 0xf692a8f3 )
	ROM_LOAD16_BYTE( "d1u.3b",  0x000000, 0x20000, 0xac2afd1b )
	ROM_LOAD16_BYTE( "d1l.1b",  0x000001, 0x20000, 0xebb51af1 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "gd0l.3p",  0x0, 0x40000, 0x9a29500e )
	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "gd1u.1s",  0x0, 0x40000, 0x17e5a61c )
	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "gd0u.1p",  0x0, 0x40000, 0x33f5a19b )
	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "gd1l.3s",  0x0, 0x40000, 0x64df59a2 )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )
	ROM_LOAD32_BYTE( "pt0u.8j", 0x000002, 0x20000, 0xabf512a6 )
	ROM_LOAD32_BYTE( "pt0l.8d", 0x000003, 0x20000, 0xac8d468c )
	ROM_LOAD32_BYTE( "pt1u.8l", 0x200002, 0x20000, 0x7e5dab74 )
	ROM_LOAD32_BYTE( "pt1l.8e", 0x200003, 0x20000, 0x38a54ec5 )

	ROM_REGION16_BE( 0x80000, REGION_USER3, 0 ) /* ? */
	ROM_LOAD( "gp0l.3j",  0x00000, 0x20000, 0x5c18f596 )
	ROM_LOAD( "gp0u.1j",  0x00001, 0x20000, 0xf5469a29 )
	ROM_LOAD( "gp1l.3l",  0x40000, 0x20000, 0x96c2463c )
	ROM_LOAD( "gp1u.1l",  0x40001, 0x20000, 0x146ab6b8 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("avo1.11c",0x000000,0x80000,0x9fb33af3 )
	ROM_LOAD("avo3.11e",0x080000,0x80000,0x76e22f92 )
ROM_END

static void namcos21_init( int game_type )
{
	data32_t *pMem;
	int i;

	namcos21_gametype = game_type;
	namcos21_data16 = (data16_t *)memory_region( REGION_USER1 );

	/* sign-extend the point data */
	pMem = (data32_t *)memory_region( REGION_USER2 );
	for( i=0; i<0x400000/4; i++ )
	{
		if( pMem[i] &  0x00800000 )
		{
			pMem[i] |= 0xff000000;
		}

#if 1
		if( namcos21_gametype == NAMCOS21_STARBLADE )
		{
			/* hack! this partially fixed corrupt starblade ROMs */
			if( (pMem[i]&0xff00)==0 )
			{
				pMem[i]&= 0xff;
			}
			else if( (pMem[i]&0xff00)==0xff00 )
			{
				pMem[i] |= 0xffff0000;
			}
		}
#endif
	}
}

static void init_winrun( void )
{
	namcos21_init( NAMCOS21_WINRUN91 );
}

static void init_aircombt( void )
{
#if 0 /* Patch test mode: replace first four tests with hidden tests */
	data16_t *pMem = (data16_t *)memory_region( REGION_CPU1 );
	pMem[0x2a32/2] = 0x90;
	pMem[0x2a34/2] = 0x94;
	pMem[0x2a36/2] = 0x88;
	pMem[0x2a38/2] = 0x8c;
#endif

	namcos21_init( NAMCOS21_AIRCOMBAT );
}

void init_starblad( void )
{
	namcos21_init( NAMCOS21_STARBLADE );
}


void init_cybsled( void )
{
	namcos21_init( NAMCOS21_CYBERSLED );
}

/*************************************************************/
/*															 */
/*	NAMCO SYSTEM 21 INPUT PORTS								 */
/*															 */
/*************************************************************/

INPUT_PORTS_START( default )
	PORT_START		/* 63B05Z0 - PORT B */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* 63B05Z0 - PORT C & SCI */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x40, 0x40, "Test Switch")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xff )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y|IPF_CENTER, 100, 4, 0x00, 0xff )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - PORT H */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - $2000 DIP SW */
	PORT_DIPNAME( 0x01, 0x01, "DSW1 (Test Mode)")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW3")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW4")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW5")
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW6")
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW7")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DSW8")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* 63B05Z0 - $3000 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3001 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3002 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3003 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( aircombt )
	PORT_START		/* IN#0: 63B05Z0 - PORT B */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN#1: 63B05Z0 - PORT C & SCI */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x40, 0x40, "Test Switch")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START		/* IN#2: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN#3: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xff )

	PORT_START		/* IN#4: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y|IPF_CENTER, 100, 4, 0x00, 0xff )

	PORT_START		/* IN#5: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X|IPF_CENTER|IPF_PLAYER2, 100, 4, 0x00, 0xff )

	PORT_START		/* IN#6: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#7: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#8: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#9: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN#10: 63B05Z0 - PORT H */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) ///???
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) // prev color
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) // ???next color
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN#11: 63B05Z0 - $2000 DIP SW */
	PORT_DIPNAME( 0x01, 0x01, "DSW1 (Test Mode)")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW3")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW4")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW5")
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW6")
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW7")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DSW8")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN#12: 63B05Z0 - $3000 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#13: 63B05Z0 - $3001 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#14: 63B05Z0 - $3002 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#15: 63B05Z0 - $3003 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/*    YEAR, NAME,     PARENT,   MACHINE,  INPUT,    	INIT,     MONITOR,    COMPANY, FULLNAME,	    	FLAGS */
GAMEX( 1992, aircombj, 0,		poly,	  aircombt, 	aircombt, ROT0,		  "Namco", "Air Combat (JP)",	GAME_NOT_WORKING )
GAMEX( 1992, aircombu, aircombj,poly,	  aircombt, 	aircombt, ROT0, 	  "Namco", "Air Combat (US)",	GAME_NOT_WORKING )
GAMEX( 1993, cybsled,  0,       poly,     default,      cybsled,  ROT0,       "Namco", "Cyber Sled",		GAME_NOT_WORKING )
/* 199?, Driver's Eyes */
/* 1993, Rhinoceros Berth Lead-Lead */
/* 1992, ShimDrive */
GAMEX( 1991, solvalou, 0, 		poly,	  default,		starblad, ROT0, 	  "Namco", "Solvalou",			GAME_NOT_WORKING )
GAMEX( 1991, starblad, 0, 		poly,	  default,  	starblad, ROT0, 	  "Namco", "Starblade",			GAME_NOT_WORKING )
/* 1988, Winning Run */
/* 1989, Winning Run Suzuka Grand Prix */
GAMEX( 1991, winrun91, 0, 		poly,	  default,		winrun,	  ROT0, 	  "Namco", "Winning Run 91", 	GAME_NOT_WORKING )
