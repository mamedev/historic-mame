/*
Namco System NB-1

Notes:
- tilemap system is virtually identical to Namco System2
- gunbulet's gun input ports require patch

ToDo:
- some lingering gfx priority glitches
- gunbulet force feedback
- scanline interrupts (see IRQ2 in GunBullet)
- sound CPU (unknown)
- shadow pens (used frequently in the baseball games)
- flip handling is incorrect
- MCU simulation (coin/inputs) is incomplete

Main CPU : Motorola 68020 32-bit processor @ 25MHz
Secondary CPUs : C329 + 137 (both custom)
Custom Graphics Chips : GFX:123,145,156,C116 - Motion Objects:C355,187,C347
Sound CPU : C351 (custom)
PCM Sound chip : C352 (custom)
I/O Chip : 160 (custom)
Board composition : Single board

Known games using this hardware:
- Great Sluggers '93
- Great Sluggers '94
- Gun Bullet / Point Blank
- Nebulas Ray
- Super World Stadium '95
- Super World Stadium '96
- Super World Stadium '97
- OutFoxies
- Mach Breakers

*****************************************************

Gun Bullet (JPN Ver.)
(c)1994 Namco
KeyCus.:C386

Note: CPU - Main PCB  (NB-1)
      MEM - MEMEXT OBJ8 PCB at J103 on the main PCB
      SPR - MEMEXT SPR PCB at location 5B on the main PCB

          - Namco main PCB:  NB-1  8634961101, (8634963101)
          - MEMEXT OBJ8 PCB:       8635902201, (8635902301)

        * - Surface mounted ROMs
        # - 32 pin DIP Custom IC (16bytes x 16-bit)

Brief hardware overview
-----------------------

Main processor    - MC68EC020FG25 25MHz   (100 pin PQFP)
                  - C329 custom           (100 pin PQFP)
                  - 137  custom PLD       (28 pin NDIP)
                  - C366 Key Custom

Sound processor   - C351 custom           (160 pin PQFP)
 (PCM)            - C352 custom           (100 pin PQFP)
 (control inputs) - 160  custom           (80 pin PQFP)

GFX               - 123  custom           (80 pin PQFP)
                  - 145  custom           (80 pin PQFP)
                  - 156  custom           (64 pin PQFP)
                  - C116 custom           (80 pin PQFP)

Motion Objects    - C355 custom           (160 pin PQFP)
                  - 187  custom           (160 pin PQFP)
                  - C347 custom           (80 pin PQFP)

PCB Jumper Settings
-------------------

Location      Setting       Alt. Setting
----------------------------------------
  JP1           1M              4M
  JP2           4M              1M
  JP5          <1M              1M
  JP6           8M             >8M
  JP7           4M              1M
  JP8           4M              1M
  JP9           cON             cOFF
  JP10          24M (MHz)       12M (MHz)
  JP11          24M (MHz)       28M (MHz)
  JP12          355             F32

*****************************************************

Super World Stadium '96 (JPN Ver.)
(c)1986-1993 1995 1996 Namco

Namco NB-1 system

KeyCus.:C426

*****************************************************

Super World Stadium '97 (JPN Ver.)
(c)1986-1993 1995 1996 1997 Namco

Namco NB-1 system

KeyCus.:C434

*****************************************************

Great Sluggers Featuring 1994 Team Rosters
(c)1993 1994 Namco / 1994 MLBPA

Namco NB-1 system

KeyCus.:C359

*****************************************************

--------------------------
Nebulasray by NAMCO (1994)
--------------------------
Location      Device        File ID      Checksum
-------------------------------------------------
CPU 13B         27C402      NR1-MPRU       B0ED      [  MAIN PROG  ]
CPU 15B         27C240      NR1-MPRL       90C4      [  MAIN PROG  ]
SPR            27C1024      NR1-SPR0       99A6      [  SOUND PRG  ]
CPU 5M       MB834000B      NR1-SHA0       DD59      [    SHAPE    ]
CPU 8J       MB838000B      NR1-CHR0       22A4      [  CHARACTER  ]
CPU 9J       MB838000B      NR1-CHR1       19D0      [  CHARACTER  ]
CPU 10J      MB838000B      NR1-CHR2       B524      [  CHARACTER  ]
CPU 11J      MB838000B      NR1-CHR3       0AF4      [  CHARACTER  ]
CPU 5J       KM2316000      NR1-VOI0       8C41      [    VOICE    ]
MEM IC1     MT26C1600 *     NR1-OBJ0L      FD7C      [ MOTION OBJL ]
MEM IC3     MT26C1600 *     NR1-OBJ1L      7069      [ MOTION OBJL ]
MEM IC5     MT26C1600 *     NR1-OBJ2L      07DC      [ MOTION OBJL ]
MEM IC7     MT26C1600 *     NR1-OBJ3L      A61E      [ MOTION OBJL ]
MEM IC2     MT26C1600 *     NR1-OBJ0U      44D3      [ MOTION OBJU ]
MEM IC4     MT26C1600 *     NR1-OBJ1U      F822      [ MOTION OBJU ]
MEM IC6     MT26C1600 *     NR1-OBJ2U      DD24      [ MOTION OBJU ]
MEM IC8     MT26C1600 *     NR1-OBJ3U      F750      [ MOTION OBJU ]
CPU 11D        Custom #      C366.BIN      1C93      [  KEYCUSTUM  ]

Note: CPU - Main PCB  (NB-1)
      MEM - MEMEXT OBJ8 PCB at J103 on the main PCB
      SPR - MEMEXT SPR PCB at location 5B on the main PCB

          - Namco main PCB:  NB-1  8634961101, (8634963101)
          - MEMEXT OBJ8 PCB:       8635902201, (8635902301)

        * - Surface mounted ROMs
        # - 32 pin DIP Custom IC (16bytes x 16-bit)

Brief hardware overview
-----------------------

Main processor    - MC68EC020FG25 25MHz   (100 pin PQFP)
                  - C329 custom           (100 pin PQFP)
                  - 137  custom PLD       (28 pin NDIP)
                  - C366 Key Custom

Sound processor   - C351 custom           (160 pin PQFP)
 (PCM)            - C352 custom           (100 pin PQFP)
 (control inputs) - 160  custom           (80 pin PQFP)

GFX               - 123  custom           (80 pin PQFP)
                  - 145  custom           (80 pin PQFP)
                  - 156  custom           (64 pin PQFP)
                  - C116 custom           (80 pin PQFP)

Motion Objects    - C355 custom           (160 pin PQFP)
                  - 187  custom           (160 pin PQFP)
                  - C347 custom           (80 pin PQFP)

PCB Jumper Settings
-------------------

Location      Setting       Alt. Setting
----------------------------------------
  JP1           1M              4M
  JP2           4M              1M
  JP5          <1M              1M
  JP6           8M             >8M
  JP7           4M              1M
  JP8           4M              1M
  JP9           cON             cOFF
  JP10          24M (MHz)       12M (MHz)
  JP11          24M (MHz)       28M (MHz)
  JP12          355             F32


Namco System NB2

Games running on this hardware:
- Outfoxies
- Mach Breakers

Changes from Namcon System NB1 include:
- different memory map
- more complex sprite and tile banking
- 2 additional ROZ layers

To do:
- sprite-tilemap priority is hacked; there's a lot of layers (4 scrolling, 2 fixed, 2 ROZ) and
  no guarentee that the constraints necessary for pdrawgfx will be met.
  At the moment, we draw sprites in multiple passes in-between drawing the tilemap layers.

- roz priority isn't understood; it's hacked with values that work for most stages, but are
	clearly wrong for attract mode and cut scenes.  ROZ control attribute#1 appears to be
	involved, but I don't see a direct relationship between it and the expected 3 bit priority code.

- in attract mode/intro, some of the sprites overlap oddly (i.e. in the television sets).
	I suspect there is an "ignore transparency" bit that makes a sprite opaque.  If this were
	true, opaque sprites would obscure any sprites behind them.

- sound; identify the sound CPU (we have code for it)


-----------------------------
The Outfoxies by NAMCO (1994)
-----------------------------
Location            Device     File ID     Checksum
----------------------------------------------------
CPU 11C PRGL       27C4002     OU2-MPRL      166F
CPU 11D PRGU       27C4002     OU2-MPRU      F4C1
CPU 5B  SPR0        27C240     OU1-SPR0      7361
CPU 20A DAT0       27C4002     OU1-DAT0      FCD1
CPU 20B DAT1       27C4002     OU1-DAT1      0973
CPU 18S SHAPE-R    MB83800     OU1-SHAR      C922
CPU 12S SHAPE-S    MB83400     OU1-SHAS      2820
CPU 6N  VOICE0    MB831600     OU1-VOI0      4132
ROM 4C  OBJ0L    16Meg SMD     OU1-OBJ0L     171B
ROM 8C  OBJ0U    16Meg SMD     OU1-OBJ0U     F961
ROM 4B  OBJ1L    16Meg SMD     OU1-OBJ1L     1579
ROM 8B  OBJ1U    16Meg SMD     OU1-OBJ1U     E8DF
ROM 4A  OBJ2L    16Meg SMD     OU1-OBJ2L     AE7B
ROM 8A  OBJ2U    16Meg SMD     OU1-OBJ2U     6588
ROM 6C  OBJ3L    16Meg SMD     OU1-OBJ3L     9ED3
ROM 9C  OBJ3U    16Meg SMD     OU1-OBJ3U     ED3B
ROM 6B  OBJ4L    16Meg SMD     OU1-OBJ4L     59D4
ROM 9B  OBJ4U    16Meg SMD     OU1-OBJ4U     56CA
ROM 3D  ROT0     16Meg SMD     OU1-ROT0      A615
ROM 3C  ROT1     16Meg SMD     OU1-ROT1      6C0A
ROM 3B  ROT2     16Meg SMD     OU1-ROT2      313E
ROM 1D  SCR0     16Meg SMD     OU1-SCR0      751A

CPU 8B  DEC75     PAL16L8A        NB1-2
CPU 16N MIXER     PAL16V8H        NB2-1
CPU 11E SIZE      PAL16L8A        NB2-2
CPU 22C KEYCUS   KeyCustom         C390

CPU  -  Namco NB-2 Main PCB        8639960102 (8639970102)
ROM  -  Namco NB-2 Mask ROM PCB    8639969800 (8639979800)

     -  Audio out is Stereo

Jumper Settings:

     Setting     Alternate
JP1    4M           1M
JP2    GND          A20
JP3    GND          A20
JP6    4M           1M
JP8    GND          A20
JP9    CON          COFF
JP10   GND          A20

Hardware info:

Main CPU:           MC68EC020FG25
                    Custom C383    (100 pin PQFP)
                    Custom C385    (144 pin PQFP)

Slave CPU:         ?Custom C382    (160 pin PQFP)
                    Custom 160     ( 80 pin PQFP)
                    Custom C352    (100 pin PQFP)

GFX:                Custom 145     ( 80 pin PQFP)
                    Custom 156     ( 64 pin PQFP)
                    Custom 123     ( 64 pin PQFP)
                 3x Custom 384     ( 48 pin PQFP)
                    Custom C355    (160 pin PQFP)
                    Custom 187     (120 pin PQFP)
                    Custom 169     (120 pin PQFP)
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namconb1.h"
#include "namcoic.h"

#define NB1_NVMEM_SIZE (0x800)
static data32_t *nvmem32;

data32_t *namconb1_workram32;
data32_t *namconb1_spritebank32;
data32_t *namconb1_scrollram32;

enum namconb1_type namconb1_type;

static NVRAM_HANDLER( namconb1 ){
	if( read_or_write )
	{
		osd_fwrite( file, nvmem32, NB1_NVMEM_SIZE );
	}
	else {
		if (file)
		{
			osd_fread( file, nvmem32, NB1_NVMEM_SIZE );
		}
		else
		{
			memset( nvmem32, 0x00, NB1_NVMEM_SIZE );
		}
	}
}

static DRIVER_INIT( nebulray )
{
	namconb1_type = key_nebulray;
}

static DRIVER_INIT( gslgr94u )
{
	namconb1_type = key_gslgr94u;
}

static DRIVER_INIT( sws96 )
{
	namconb1_type = key_sws96;
}

static DRIVER_INIT( sws97 )
{
	namconb1_type = key_sws97;
}

static DRIVER_INIT( gunbulet )
{
//	data32_t *pMem = (data32_t *)memory_region(REGION_CPU1);
	namconb1_type = key_gunbulet;

	//p1 gun patch; without it you cannot shoot in the left 24 pixels
	//pMem[0xA798/4] = 0x4E714E71;
	//pMem[0xA7B4/4] = 0x4E714E71;

	//p2 gun patch; without it you cannot shoot in the left 24 pixels
	//pMem[0xA87C/4] = 0x4E714E71;
	//pMem[0xA898/4] = 0x4E714E71;
}

static DRIVER_INIT( outfxies )
{
	data8_t *pMem8 = (data8_t *)memory_region( REGION_USER1 );
	data32_t *pMem32 = (data32_t *)pMem8;
	int i;

	namconb1_type = key_outfoxies;

	/* shuffle data ROMs */
	for( i=0; i<0x100000/4; i++ )
	{
		pMem32[i] = (pMem8[0]<<16)|(pMem8[1]<<24)|(pMem8[2]<<0)|(pMem8[3]<<8);
		pMem8+=4;
	}
	cpu_setbank( 1, pMem32 );
}

static READ32_HANDLER( custom_key_r )
{
	static data16_t count;
	data16_t old_count;

	old_count = count;
	do
	{
		count = rand();
	} while( count==old_count );

	switch( namconb1_type )
	{
	case key_gunbulet:
		return 0; /* no protection */

	case key_sws96:
		switch( offset )
		{
		case 0: return 0x01aa<<16;
		case 4: return count<<16;
		}
		break;

	case key_sws97:
		switch( offset )
		{
		case 2: return 0x1b2<<16;
		case 5: return count<<16;
		}
		break;

	case key_gslgr94u:
		switch( offset )
		{
		case 0: return 0x0167;
		case 1: return count<<16;
		}
		break;

	case key_nebulray:
		switch( offset )
		{
		case 1: return 0x016e;
		case 3: return count;
		}
		break;

	case key_outfoxies:
		switch( offset )
		{
		case 0: /* 0x00 */ return 0x0186;
		case 1: /* 0x04 */ return count<<16;
		}
		break;
	}

	logerror( "custom_key_r(%d); pc=%08x\n", offset, activecpu_get_pc() );
	return 0;
}

/***************************************************************/

static struct GfxLayout obj_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8, /* bits per pixel */
	{
		/* plane offsets */
		0,1,2,3,4,5,6,7,
	},
	{
		0*16+8,1*16+8,0*16,1*16,
		2*16+8,3*16+8,2*16,3*16,
		4*16+8,5*16+8,4*16,5*16,
		6*16+8,7*16+8,6*16,7*16
	},
	{
		0x0*128,0x1*128,0x2*128,0x3*128,0x4*128,0x5*128,0x6*128,0x7*128,
		0x8*128,0x9*128,0xa*128,0xb*128,0xc*128,0xd*128,0xe*128,0xf*128
	},
	16*128
};

static struct GfxLayout tile_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	{ 0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64 },
	8*64
};

static struct GfxLayout roz_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	{
		0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128
	},
	16*128
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ NAMCONB1_TILEGFXREGION,	0, &tile_layout,	0x1000, 0x10 },
	{ NAMCONB1_SPRITEGFXREGION,	0, &obj_layout,		0x0000, 0x10 },
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo2[] =
{
	{ NAMCONB1_TILEGFXREGION,	0, &tile_layout,	0x1000, 0x08 },
	{ NAMCONB1_SPRITEGFXREGION,	0, &obj_layout,		0x0000, 0x10 },
	{ NAMCONB1_ROTGFXREGION,	0, &roz_layout,		0x1800, 0x08 },
	{ -1 }
}; /* gfxdecodeinfo */

/***************************************************************/

static READ32_HANDLER( gunbulet_gun_r )
{
	int result = 0;

	switch( offset )
	{
	case 0: case 1: result = readinputport(7); break; /* Y (p2) */
	case 2: case 3: result = readinputport(6); break; /* X (p2) */
	case 4: case 5: result = readinputport(5); break; /* Y (p1) */
	case 6: case 7: result = readinputport(4); break; /* X (p1) */
	}
	return result<<24;
}

static MEMORY_READ32_START( namconb1_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x10001f, gunbulet_gun_r },
	{ 0x1c0000, 0x1cffff, MRA32_RAM }, /* workram */
	{ 0x1e4000, 0x1e4003, MRA32_NOP }, /* unknown */
	{ 0x200000, 0x23ffff, MRA32_RAM }, /* workram (shared with MCU) */
	{ 0x400000, 0x40001f, MRA32_RAM }, /* ? */
	{ 0x580000, 0x5807ff, MRA32_RAM }, /* nvmem */
	{ 0x600000, 0x6141ff, namco_obj32_r },
	{ 0x620000, 0x620007, MRA32_RAM }, /* spritepos */
	{ 0x640000, 0x64ffff, MRA32_RAM }, /* videoram (4 scrolling + 2 fixed) */
	{ 0x660000, 0x66003f, MRA32_RAM }, /* scrollram */
	{ 0x680000, 0x68000f, MRA32_RAM }, /* spritebank */
	{ 0x6e0000, 0x6e001f, custom_key_r },
	{ 0x700000, 0x707fff, MRA32_RAM }, /* palette */
MEMORY_END

static MEMORY_WRITE32_START( namconb1_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x1c0000, 0x1cffff, MWA32_RAM }, /* workram */
	{ 0x200000, 0x23ffff, MWA32_RAM, &namconb1_workram32 },
	{ 0x400000, 0x40001f, MWA32_RAM }, /* cpu control registers */
	/*  400016: watchdog */
	{ 0x580000, 0x5807ff, MWA32_RAM, &nvmem32 },
	{ 0x600000, 0x6141ff, namco_obj32_w },
	{ 0x618000, 0x618003, MWA32_NOP }, /* spriteram latch */
	{ 0x620000, 0x620007, MWA32_RAM, &namconb1_spritepos32 },
	{ 0x640000, 0x64ffff, namconb1_videoram_w, &videoram32 },
	{ 0x660000, 0x66003f, MWA32_RAM, &namconb1_scrollram32 },
	/*	660000..66001f: tilemap scroll/flip
	 *	660020..66002f: tilemap priority
	 *	660030..66003f: tilemap color
	 */
	{ 0x680000, 0x68000f, MWA32_RAM, &namconb1_spritebank32 },
	{ 0x6e0000, 0x6e001f, MWA32_NOP }, /* custom key write */
	{ 0x700000, 0x707fff, MWA32_RAM, &paletteram32 },
MEMORY_END

static MEMORY_READ32_START( namconb2_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x1c0000, 0x1cffff, MRA32_RAM },	/* workram */
	{ 0x1e4000, 0x1e4003, MRA32_NOP },	/* ? */
	{ 0x200000, 0x2fffff, MRA32_RAM },	/* workram (shared with MCU) */
	{ 0x400000, 0x4fffff, MRA32_BANK1 },/* data ROMs */
	{ 0x600000, 0x6023ff, MRA32_RAM }, /* ? */
	{ 0x602400, 0x60247f, MRA32_RAM }, /* ? */
	{ 0x602480, 0x603fff, MRA32_RAM }, /* ? */
	{ 0x600000, 0x6141ff, namco_obj32_r },
	{ 0x620000, 0x620007, MRA32_RAM }, /* spritepos */
	{ 0x640000, 0x64000f, MRA32_RAM }, /* unknown xy offset */
	{ 0x680000, 0x68ffff, MRA32_RAM }, /* videoram (4 scrolling + 2 fixed) */
	{ 0x6c0000, 0x6c003f, MRA32_RAM }, /* scrollram, color, pri */
	{ 0x700000, 0x71ffff, namco_rozvideoram32_r },
	{ 0x740000, 0x74001f, namco_rozcontrol32_r },
	{ 0x800000, 0x807fff, MRA32_RAM }, /* palette */
	{ 0x900008, 0x90000f, MRA32_RAM }, /* sprite bank */
	{ 0x940000, 0x94000f, MRA32_RAM }, /* ? */
	{ 0x980000, 0x98000f, namco_rozbank32_r },
	{ 0xa00000, 0xa007ff, MRA32_RAM }, /* nvmem */
	{ 0xc00000, 0xc0001f, custom_key_r },
	{ 0xf00000, 0xf0001f, MRA32_RAM }, /* ? */
MEMORY_END /* namconb2_readmem */

static MEMORY_WRITE32_START( namconb2_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x1c0000, 0x1cffff, MWA32_RAM }, /* workram */
	{ 0x200000, 0x2fffff, MWA32_RAM, &namconb1_workram32 },
	{ 0x600000, 0x6141ff, namco_obj32_w },
	{ 0x618000, 0x618003, MWA32_NOP }, /* written when spriteram has been updated */
	{ 0x620000, 0x620007, MWA32_RAM, &namconb1_spritepos32 },
	{ 0x640000, 0x64000f, MWA32_RAM }, /* ? */
	{ 0x680000, 0x68ffff, namconb1_videoram_w, &videoram32 },
	{ 0x6c0000, 0x6c003f, MWA32_RAM, &namconb1_scrollram32 },
		/* 0x00..0x1f	tilemap (scrollx,scrolly) pairs
		 * 0x20..0x2f	tilemap priority
		 * 0x30..0x3f	tilemap color
		 */
	{ 0x700000, 0x71ffff, namco_rozvideoram32_w },
	{ 0x740000, 0x74001f, namco_rozcontrol32_w },
	{ 0x800000, 0x807fff, MWA32_RAM, &paletteram32 },
	{ 0x900008, 0x90000f, MWA32_RAM, &namconb1_spritebank32 },
	{ 0x940000, 0x94000f, MWA32_RAM }, /* ? */
	{ 0x980000, 0x98000f, namco_rozbank32_w },
	{ 0xa00000, 0xa007ff, MWA32_RAM }, /* nvmem */
	{ 0xc00000, 0xc00003, MWA32_NOP }, /* custom key (protection) */
	{ 0xf00000, 0xf0001f, MWA32_RAM }, /* misc cpu control registers */
MEMORY_END /* namconb2_writemem */

static INTERRUPT_GEN( namconb1_interrupt )
{
	if( namconb1_type == key_gunbulet ) cpu_set_irq_line(0, 5, HOLD_LINE);
	else cpu_set_irq_line(0, 2, HOLD_LINE);
}

static INTERRUPT_GEN( namconb2_interrupt )
{
	if (cpu_getiloops() == 0)
	{
		cpu_set_irq_line(0, 1, HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(0, 5, HOLD_LINE);
	}
}

static MACHINE_DRIVER_START( namconb1 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020,25000000/2) /* 25 MHz? */
	MDRV_CPU_MEMORY(namconb1_readmem,namconb1_writemem)
	MDRV_CPU_VBLANK_INT(namconb1_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(namconb1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(NAMCONB1_COLS*8, NAMCONB1_ROWS*8) /* 288x224 pixels */
	MDRV_VISIBLE_AREA(0*8, NAMCONB1_COLS*8-1, 0*8, NAMCONB1_ROWS*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x2000)

	MDRV_VIDEO_START(namconb1)
	MDRV_VIDEO_UPDATE(namconb1)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	/* similar to C140?  managed by MCU */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( namconb2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020,25000000) /* 25 MHz? */
	MDRV_CPU_MEMORY(namconb2_readmem,namconb2_writemem)
	MDRV_CPU_VBLANK_INT(namconb2_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(NAMCONB1_COLS*8, NAMCONB1_ROWS*8) /* 288x224 pixels */
	MDRV_VISIBLE_AREA(0*8, NAMCONB1_COLS*8-1, 0*8, NAMCONB1_ROWS*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo2)
	MDRV_PALETTE_LENGTH(0x2000)

	MDRV_VIDEO_START(namconb2)
	MDRV_VIDEO_UPDATE(namconb2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

/***************************************************************/

ROM_START( ptblank )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "gn2mprlb.15b", 0x00002, 0x80000, 0xfe2d9425 )
	ROM_LOAD32_WORD( "gn2mprub.13b", 0x00000, 0x80000, 0x3bf4985a )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound program */
	ROM_LOAD( "gn1-spr0.bin", 0, 0x20000, 0x6836ba38 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "gn1-voi0.bin", 0, 0x200000, 0x05477eb7 )

	ROM_REGION( 0x800000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gn1obj0l.bin", 0x000001, 0x200000, 0x06722dc8 )
	ROM_LOAD16_BYTE( "gn1obj0u.bin", 0x000000, 0x200000, 0xfcefc909 )
	ROM_LOAD16_BYTE( "gn1obj1u.bin", 0x400000, 0x200000, 0x3109a071 )
	ROM_LOAD16_BYTE( "gn1obj1l.bin", 0x400001, 0x200000, 0x48468df7 )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "gn1-chr0.bin", 0x000000, 0x100000, 0xa5c61246 )
	ROM_LOAD( "gn1-chr1.bin", 0x100000, 0x100000, 0xc8c59772 )
	ROM_LOAD( "gn1-chr2.bin", 0x200000, 0x100000, 0xdc96d999 )
	ROM_LOAD( "gn1-chr3.bin", 0x300000, 0x100000, 0x4352c308 )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "gn1-sha0.bin", 0, 0x80000, 0x86d4ff85 )
ROM_END

ROM_START( gunbulet )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "gn1-mprl.bin", 0x00002, 0x80000, 0xf99e309e )
	ROM_LOAD32_WORD( "gn1-mpru.bin", 0x00000, 0x80000, 0x72a4db07 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound program */
	ROM_LOAD( "gn1-spr0.bin", 0, 0x20000, 0x6836ba38 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "gn1-voi0.bin", 0, 0x200000, 0x05477eb7 )

	ROM_REGION( 0x800000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gn1obj0l.bin", 0x000001, 0x200000, 0x06722dc8 )
	ROM_LOAD16_BYTE( "gn1obj0u.bin", 0x000000, 0x200000, 0xfcefc909 )
	ROM_LOAD16_BYTE( "gn1obj1u.bin", 0x400000, 0x200000, 0x3109a071 )
	ROM_LOAD16_BYTE( "gn1obj1l.bin", 0x400001, 0x200000, 0x48468df7 )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "gn1-chr0.bin", 0x000000, 0x100000, 0xa5c61246 )
	ROM_LOAD( "gn1-chr1.bin", 0x100000, 0x100000, 0xc8c59772 )
	ROM_LOAD( "gn1-chr2.bin", 0x200000, 0x100000, 0xdc96d999 )
	ROM_LOAD( "gn1-chr3.bin", 0x300000, 0x100000, 0x4352c308 )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "gn1-sha0.bin", 0, 0x80000, 0x86d4ff85 )
ROM_END

ROM_START( nebulray )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "nr2-mpru.13b", 0x00000, 0x80000, 0x049b97cb )
	ROM_LOAD32_WORD( "nr2-mprl.15b", 0x00002, 0x80000, 0x0431b6d4 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound program */
	ROM_LOAD( "nr1-spr0", 0, 0x20000, 0x1cc2b44b )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "nr1-voi0", 0, 0x200000, 0x332d5e26 )

	ROM_REGION( 0x1000000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "nr1obj0u", 0x000000, 0x200000, 0xfb82a881 )
	ROM_LOAD16_BYTE( "nr1obj0l", 0x000001, 0x200000, 0x0e99ef46 )
	ROM_LOAD16_BYTE( "nr1obj1u", 0x400000, 0x200000, 0x49d9dbd7 )
	ROM_LOAD16_BYTE( "nr1obj1l", 0x400001, 0x200000, 0xf7a898f0 )
	ROM_LOAD16_BYTE( "nr1obj2u", 0x800000, 0x200000, 0x8c8205b1 )
	ROM_LOAD16_BYTE( "nr1obj2l", 0x800001, 0x200000, 0xb39871d1 )
	ROM_LOAD16_BYTE( "nr1obj3u", 0xc00000, 0x200000, 0xd5918c9e )
	ROM_LOAD16_BYTE( "nr1obj3l", 0xc00001, 0x200000, 0xc90d13ae )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "nr1-chr0", 0x000000, 0x100000,0x8d5b54ea )
	ROM_LOAD( "nr1-chr1", 0x100000, 0x100000,0xcd21630c )
	ROM_LOAD( "nr1-chr2", 0x200000, 0x100000,0x70a11023 )
	ROM_LOAD( "nr1-chr3", 0x300000, 0x100000,0x8f4b1d51 )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "nr1-sha0", 0, 0x80000,0xca667e13 )

	ROM_REGION( 0x20, REGION_PROMS, 0 ) /* custom key data? */
	ROM_LOAD( "c366.bin", 0, 0x20, 0x8c96f31d )
ROM_END

ROM_START( nebulryj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "nr1-mpru", 0x00000, 0x80000, 0x42ef71f9 )
	ROM_LOAD32_WORD( "nr1-mprl", 0x00002, 0x80000, 0xfae5f62c )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound program */
	ROM_LOAD( "nr1-spr0", 0, 0x20000, 0x1cc2b44b )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "nr1-voi0", 0, 0x200000, 0x332d5e26 )

	ROM_REGION( 0x1000000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "nr1obj0u", 0x000000, 0x200000, 0xfb82a881 )
	ROM_LOAD16_BYTE( "nr1obj0l", 0x000001, 0x200000, 0x0e99ef46 )
	ROM_LOAD16_BYTE( "nr1obj1u", 0x400000, 0x200000, 0x49d9dbd7 )
	ROM_LOAD16_BYTE( "nr1obj1l", 0x400001, 0x200000, 0xf7a898f0 )
	ROM_LOAD16_BYTE( "nr1obj2u", 0x800000, 0x200000, 0x8c8205b1 )
	ROM_LOAD16_BYTE( "nr1obj2l", 0x800001, 0x200000, 0xb39871d1 )
	ROM_LOAD16_BYTE( "nr1obj3u", 0xc00000, 0x200000, 0xd5918c9e )
	ROM_LOAD16_BYTE( "nr1obj3l", 0xc00001, 0x200000, 0xc90d13ae )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "nr1-chr0", 0x000000, 0x100000,0x8d5b54ea )
	ROM_LOAD( "nr1-chr1", 0x100000, 0x100000,0xcd21630c )
	ROM_LOAD( "nr1-chr2", 0x200000, 0x100000,0x70a11023 )
	ROM_LOAD( "nr1-chr3", 0x300000, 0x100000,0x8f4b1d51 )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "nr1-sha0", 0, 0x80000,0xca667e13 )

	ROM_REGION( 0x20, REGION_PROMS, 0 ) /* custom key data? */
	ROM_LOAD( "c366.bin", 0, 0x20, 0x8c96f31d )
ROM_END

ROM_START( gslgr94u )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "gse2mprl.bin", 0x00002, 0x80000, 0xa514349c )
	ROM_LOAD32_WORD( "gse2mpru.bin", 0x00000, 0x80000, 0xb6afd238 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound program */
	ROM_LOAD( "gse2spr0.bin", 0, 0x20000, 0x17e87cfc )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "gse-voi0.bin", 0, 0x200000, 0xd3480574 )

	ROM_REGION( 0x400000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gseobj0l.bin", 0x000001, 0x200000, 0x531520ca )
	ROM_LOAD16_BYTE( "gseobj0u.bin", 0x000000, 0x200000, 0xfcc1283c )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "gse-chr0.bin", 0x000000, 0x100000, 0x9314085d )
	ROM_LOAD( "gse-chr1.bin", 0x100000, 0x100000, 0xc128a887 )
	ROM_LOAD( "gse-chr2.bin", 0x200000, 0x100000, 0x48f0a311 )
	ROM_LOAD( "gse-chr3.bin", 0x300000, 0x100000, 0xadbd1f88 )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "gse-sha0.bin", 0, 0x80000, 0x6b2beabb )
ROM_END

ROM_START( sws96 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "ss61mprl.bin", 0x00002, 0x80000, 0x6f55e73 )
	ROM_LOAD32_WORD( "ss61mpru.bin", 0x00000, 0x80000, 0xabdbb83 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* sound program */
	ROM_LOAD( "ss61spr0.bin", 0, 0x80000, 0x71cb12f5 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "ss61voi0.bin", 0, 0x200000, 0x2740ec72 )

	ROM_REGION( 0x400000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ss61ob0l.bin", 0x000001, 0x200000, 0x579b19d4 )
	ROM_LOAD16_BYTE( "ss61ob0u.bin", 0x000000, 0x200000, 0xa69bbd9e )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ss61chr0.bin", 0x000000, 0x100000, 0x9d2ae07b )
	ROM_LOAD( "ss61chr1.bin", 0x100000, 0x100000, 0x4dc75da6 )
	ROM_LOAD( "ss61chr2.bin", 0x200000, 0x100000, 0x1240704b )
	ROM_LOAD( "ss61chr3.bin", 0x300000, 0x100000, 0x066581d4 )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "ss61sha0.bin", 0, 0x80000, 0xfceaa19c )
ROM_END

ROM_START( sws97 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "ss71mprl.bin", 0x00002, 0x80000, 0xbd60b50e )
	ROM_LOAD32_WORD( "ss71mpru.bin", 0x00000, 0x80000, 0x3444f5a8 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* sound program */
	ROM_LOAD( "ss71spr0.bin", 0, 0x80000, 0x71cb12f5 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "ss71voi0.bin", 0, 0x200000, 0x2740ec72 )

	ROM_REGION( 0x400000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ss71ob0l.bin", 0x000001, 0x200000, 0x9559ad44 )
	ROM_LOAD16_BYTE( "ss71ob0u.bin", 0x000000, 0x200000, 0x4df4a722 )

	ROM_REGION( 0x400000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ss71chr0.bin", 0x000000, 0x100000, 0xbd606356 )
	ROM_LOAD( "ss71chr1.bin", 0x100000, 0x100000, 0x4dc75da6 )
	ROM_LOAD( "ss71chr2.bin", 0x200000, 0x100000, 0x1240704b )
	ROM_LOAD( "ss71chr3.bin", 0x300000, 0x100000, 0x066581d4 )

	ROM_REGION( 0x80000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "ss71sha0.bin", 0, 0x80000, 0xbe8c2758 )
ROM_END

ROM_START( outfxies )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD( "ou2mprl.11c", 0x00002, 0x80000, 0xf414a32e )
	ROM_LOAD32_WORD( "ou2mpru.11d", 0x00000, 0x80000, 0xab5083fb )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* sound program */
	ROM_LOAD( "ou1spr0.5b", 0, 0x80000, 0x60cee566 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "ou1voi0.6n", 0, 0x200000, 0x2d8fb271 )

	ROM_REGION( 0x200000, NAMCONB1_TILEMASKREGION, 0 )
	ROM_LOAD( "ou1shas.12s", 0, 0x200000,0x9bcb0397	)

	ROM_REGION( 0x200000, NAMCONB1_ROTMASKREGION, 0 )
	ROM_LOAD( "ou1shar.18s", 0, 0x200000,	0xfbb48194 )

	ROM_REGION( 0x2000000, NAMCONB1_SPRITEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ou1obj0l", 0x0000001, 0x200000, 0x1b4f7184 )
	ROM_LOAD16_BYTE( "ou1obj0u", 0x0000000, 0x200000, 0xd0a69794 )
	ROM_LOAD16_BYTE( "ou1obj1l", 0x0400001, 0x200000, 0x48a93e84 )
	ROM_LOAD16_BYTE( "ou1obj1u", 0x0400000, 0x200000, 0x999de386 )
	ROM_LOAD16_BYTE( "ou1obj2l", 0x0800001, 0x200000, 0x30386cd0 )
	ROM_LOAD16_BYTE( "ou1obj2u", 0x0800000, 0x200000, 0xccada5f8 )
	ROM_LOAD16_BYTE( "ou1obj3l", 0x0c00001, 0x200000, 0x5f41b44e )
	ROM_LOAD16_BYTE( "ou1obj3u", 0x0c00000, 0x200000, 0xbc852c8e )
	ROM_LOAD16_BYTE( "ou1obj4l", 0x1000001, 0x200000, 0x99a5f9d7 )
	ROM_LOAD16_BYTE( "ou1obj4u", 0x1000000, 0x200000, 0x70ecaabb )

	ROM_REGION( 0x600000, NAMCONB1_ROTGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ou1-rot0", 0x000000, 0x200000, 0xa50c67c8 )
	ROM_LOAD( "ou1-rot1", 0x200000, 0x200000, 0x14866780 )
	ROM_LOAD( "ou1-rot2", 0x400000, 0x200000, 0x55ccf3af )

	ROM_REGION( 0x200000, NAMCONB1_TILEGFXREGION, ROMREGION_DISPOSE )
	ROM_LOAD( "ou1-scr0", 0x000000, 0x200000, 0xb3b3f2e9 )

	ROM_REGION( 0x100000, REGION_USER1, 0 )
	ROM_LOAD( "ou1dat0.20a", 0x00000, 0x80000, 0x1a49aead )
	ROM_LOAD( "ou1dat1.20b", 0x80000, 0x80000, 0x63bb119d )
ROM_END

/***************************************************************/

INPUT_PORTS_START( gunbulet )
	PORT_START /* inp0 */
	PORT_DIPNAME( 0x01, 0x00, "DSW2 (Unused)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DSW1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) /* C75 status */

	PORT_START /* inp1 */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START /* inp2 */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START /* inp3: fake: coin input */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START /* inp4: fake analog X (p1 gun) */
	PORT_ANALOG( 0xff, 30, IPT_AD_STICK_X, 50, 4, 0, 230 )

	PORT_START /* inp5: fake analog Y (p1 gun) */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y, 50, 4, 0, 255 )

	PORT_START /* inp6: fake analog X (p2 gun) */
	PORT_ANALOG( 0xff, 200, IPT_AD_STICK_X | IPF_PLAYER2, 50, 4, 0, 230 )

	PORT_START /* inp7: fake analog Y (p2 gun) */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2, 50, 4, 0, 255 )
INPUT_PORTS_END

INPUT_PORTS_START( namconb1 )
	PORT_START /* inp0 */
	PORT_DIPNAME( 0x01, 0x00, "DSW2 (Unused)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DSW1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) /* C75 status */

	PORT_START /* inp1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START /* inp2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START /* inp3: fake: coin input */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
INPUT_PORTS_END



GAMEX( 1994, nebulray, 0,        namconb1, namconb1, nebulray, ROT90, "Namco", "Nebulas Ray (World)", GAME_NO_SOUND )
GAMEX( 1994, nebulryj, nebulray, namconb1, namconb1, nebulray, ROT90, "Namco", "Nebulas Ray (Japan)", GAME_NO_SOUND )
GAMEX( 1994, ptblank,  0,        namconb1, gunbulet, gunbulet, ROT0,  "Namco", "Point Blank", GAME_NO_SOUND )
GAMEX( 1994, gunbulet, ptblank,  namconb1, gunbulet, gunbulet, ROT0,  "Namco", "Gun Bullet (Japan)", GAME_NO_SOUND )
GAMEX( 1994, gslgr94u, 0,        namconb1, namconb1, gslgr94u, ROT0,  "Namco", "Great Sluggers '94", GAME_NO_SOUND )
GAMEX( 1996, sws96,    0,        namconb1, namconb1, sws96,    ROT0,  "Namco", "Super World Stadium '96 (Japan)", GAME_NO_SOUND )
GAMEX( 1997, sws97,    0,        namconb1, namconb1, sws97,    ROT0,  "Namco", "Super World Stadium '97 (Japan)", GAME_NO_SOUND )

//     YEAR, NAME,     PARENT,   MACHINE,  INPUT,    INIT,     MNTR,  COMPANY, FULLNAME,   FLAGS)
GAMEX( 1994, outfxies, 0,		 namconb2, namconb1, outfxies, ROT0, "Namco", "Outfoxies", GAME_NO_SOUND )
