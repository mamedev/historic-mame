/***************************************************************************

Playchoice 10 - (c) 1986 Nintendo of America

	Written by Ernesto Corvi.

	Portions of this code are heavily based on
	Brad Oliver's MESS implementation of the NES.

	Thanks to people that contributed to this driver, namely:
	- Brad Oliver, the NES guy.
	- Aaron Giles, the smart guy.
	- Al Kossow, da man!.

****************************************************************************

BIOS:
	Memory Map
	----------
	0000 - 3fff = Program ROM (8T)
	8000 - 87ff = RAM (8V)
	8800 - 8fff = RAM (8W)
	9000 - 97ff = SRAM (8R - Videoram)
	Cxxx = /INST ROM SEL
	Exxx = /IDSEL

	Input Ports
	-----------
	Read:
	- Port 0
	bit0 = CHSelect(?)
	bit1 = Enter button
	bit2 = Reset button
	bit3 = INTDETECT
	bit4 = N/C
	bit5 = Coin 2
	bit6 = Service button
	bit7 = Coin 1
	- Port 1 = Dipswitch 1
	- Port 2 = Dipswitch 2
	- Port 3 = /DETECTCLR

	Write: (always bit 0)
	- Port 0 = SDCS (ShareD CS)
	- Port 1 = /CNTRLMASK
	- Port 2 = /DISPMASK
	- Port 3 = /SOUNDMASK
	- Port 4 = /GAMERES
	- Port 5 = /GAMESTOP
	- Port 6 = N/C
	- Port 7 = N/C
	- Port 8 = NMI Enable
	- Port 9 = DOG DI
	- Port A = /PPURES
	- Port B = CSEL0 \
	- Port C = CSEL1  \ (Cartridge select: 0 to 9)
	- Port D = CSEL2  /
	- Port E = CSEL3 /
	- Port F = 8UP KEY

****************************************************************************

Working games:
--------------
	- Contra					(CT) - B board
	- Duck Hunt					(DH) - Standard board
	- Excite Bike				(EB) - Standard board
	- Ninja Gaiden				(NG) - F board
	- Pro Wrestling				(PW) - B board
	- Rush N' Attack			(RA) - B board
	- Super Mario Bros			(SM) - Standard board
	- Super Mario Bros 3		(UM) - G board
	- The Goonies				(GN) - C board


Non working games due to mapper/nes emulation issues:
-----------------------------------------------------
	- Mike Tyson's Punchout		(PT) - E board
	- Track & Field				(TR) - A board

Non working games due to missing roms:
--------------------------------------
	- Double Dragon				(WD) - F board

Non working games due to missing RP5H01 data:
---------------------------------------------
	- 1942						(NF) - Standard board
	- Balloon Fight				(BF) - Standard board
	- Baseball					(BA) - Standard board
	- Baseball Stars			(B9) - F board
	- Captain Sky Hawk			(YW) - i board
	- Castlevania				(CV) - B board
	- Double Dribble			(DW) - B board
	- Dr. Mario					(VU) - F board
	- Fester's Quest			(EQ) - F board
	- Gauntlet					(GL) - G board
	- Golf						(GF) - Standard board
	- Gradius					(GR) - A board
	- Hogan's Alley				(HA) - Standard board
	- Kung Fu					(SX) - Standard board
	- Mario Open Golf			(UG) - K board
	- Metroid					(MT) - D board
	- Ninja Gaiden 3			(3N) - G board
	- Pinbot					(io) - H board
	- Power Blade				(7T) - G board
	- Rad Racer					(RC) - D board
	- Rad Racer II				(QR) - G board
	- RC Pro Am					(PM) - F board
	- Rescue Rangers			(RU) - F board
	- Rockin' Kats				(7A) - G board
	- Rygar						(RY) - B board
	- Solar Jetman				(LJ) - i board
	- Super C					(UE) - G board
	- Tennis					(TE) - Standard board
	- TMNT						(U2) - F board
	- TMNT II					(2N) - G board
	- Volley Ball				(VB) - Standard board
	- Wild Gunman				(WG) - Standard board
	- World Cup					(XZ) - G board
	- Yo Noid					(YC) - F board

****************************************************************************

Notes & Todo:
-------------

- Fix Mike Tyson's Punchout gfx banking.
- Fix Track & Field. It requires you to press start after starting
  a game without displaying anything on screen. Bad rom?.
- Fix Ninja Gaiden?. After the attract mode it displays a black
screen. Game works fine tho. Normal behaviour?. Bad rom?.
- Dipswitches
- Better control layout?. This thing has odd buttons.
- Find dumps of the rest of the RP5H01's and add the remaining games.
- Any PPU optimizations that retain accuracy are certainly welcome.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/ppu2c03b.h"
#include "cpu/z80/z80.h"
#include "machine/rp5h01.h"

/* clock frequency */
#define N2A03_DEFAULTCLOCK (21477272.724 / 12)

/* from vidhrdw */
extern int playch10_vh_start( void );
extern void playch10_vh_stop( void );
extern void playch10_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void playch10_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh );

/* from machine */
extern void pc10_init_machine( void );
extern void init_playch10( void );	/* standard games */
extern void init_pc_gun( void );	/* gun games */
extern void init_pcaboard( void );	/* a-board games */
extern void init_pcbboard( void );	/* b-board games */
extern void init_pccboard( void );	/* c-board games */
extern void init_pceboard( void );	/* e-board games */
extern void init_pcfboard( void );	/* f-board games */
extern void init_pcgboard( void );	/* g-board games */
extern READ_HANDLER( pc10_port_0_r );
extern READ_HANDLER( pc10_instrom_r );
extern READ_HANDLER( pc10_prot_r );
extern READ_HANDLER( pc10_detectclr_r );
extern READ_HANDLER( pc10_in0_r );
extern READ_HANDLER( pc10_in1_r );
extern WRITE_HANDLER( pc10_SDCS_w );
extern WRITE_HANDLER( pc10_CNTRLMASK_w );
extern WRITE_HANDLER( pc10_DISPMASK_w );
extern WRITE_HANDLER( pc10_SOUNDMASK_w );
extern WRITE_HANDLER( pc10_NMIENABLE_w );
extern WRITE_HANDLER( pc10_DOGDI_w );
extern WRITE_HANDLER( pc10_GAMERES_w );
extern WRITE_HANDLER( pc10_GAMESTOP_w );
extern WRITE_HANDLER( pc10_PPURES_w );
extern WRITE_HANDLER( pc10_prot_w );
extern WRITE_HANDLER( pc10_CARTSEL_w );
extern WRITE_HANDLER( pc10_in0_w );
extern int pc10_sdcs;
extern int pc10_nmi_enable;
extern int pc10_dog_di;

/******************************************************************************/

/* local stuff */
static UINT8 *work_ram, *ram_8w;
static int up_8w;

static WRITE_HANDLER( up8w_w )
{
	up_8w = data & 1;
}

static READ_HANDLER( ram_8w_r )
{
	if ( offset >= 0x400 && up_8w )
		return ram_8w[offset];

	return ram_8w[offset & 0x3ff];
}

static WRITE_HANDLER( ram_8w_w )
{
	if ( offset >= 0x400 && up_8w )
		ram_8w[offset] = data;
	else
		ram_8w[offset & 0x3ff] = data;
}


static WRITE_HANDLER( video_w ) {
	/* only write to videoram when allowed */
	if ( pc10_sdcs )
		videoram_w( offset, data );
}

static READ_HANDLER( mirror_ram_r )
{
	return work_ram[ offset & 0x7ff ];
}

static WRITE_HANDLER( mirror_ram_w )
{
	work_ram[ offset & 0x7ff ] = data;
}

static WRITE_HANDLER( sprite_dma_w )
{
	int source = ( data & 7 ) * 0x100;

	ppu2c03b_spriteram_dma( 0, &work_ram[source] );
}

static void nvram_handler(void *file, int read_or_write)
{
	UINT8 *mem = memory_region( REGION_CPU2 ) + 0x6000;

	if ( read_or_write )
		osd_fwrite( file, mem, 0x1000 );
	else if (file)
		osd_fread( file, mem, 0x1000 );
	else
		memset(mem, 0, 0x1000);
}

/******************************************************************************/

/* BIOS */
static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },	/* 8V */
	{ 0x8800, 0x8fff, ram_8w_r },	/* 8W */
	{ 0x9000, 0x97ff, videoram_r },
	{ 0xc000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xffff, pc10_prot_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM }, /* 8V */
	{ 0x8800, 0x8fff, ram_8w_w, &ram_8w }, /* 8W */
	{ 0x9000, 0x97ff, video_w, &videoram, &videoram_size },
	{ 0xc000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xffff, pc10_prot_w },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, pc10_port_0_r },	/* coins, service */
	{ 0x01, 0x01, input_port_1_r },	/* dipswitch 1 */
	{ 0x02, 0x02, input_port_2_r }, /* dipswitch 2 */
	{ 0x03, 0x03, pc10_detectclr_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, pc10_SDCS_w },
	{ 0x01, 0x01, pc10_CNTRLMASK_w },
	{ 0x02, 0x02, pc10_DISPMASK_w },
	{ 0x03, 0x03, pc10_SOUNDMASK_w },
	{ 0x04, 0x04, pc10_GAMERES_w },
	{ 0x05, 0x05, pc10_GAMESTOP_w },
	{ 0x06, 0x07, IOWP_NOP },
	{ 0x08, 0x08, pc10_NMIENABLE_w },
	{ 0x09, 0x09, pc10_DOGDI_w },
	{ 0x0a, 0x0a, pc10_PPURES_w },
	{ 0x0b, 0x0e, pc10_CARTSEL_w },
	{ 0x0f, 0x0f, up8w_w },
	{ -1 }  /* end of table */
};

/* Cart */
static struct MemoryReadAddress cart_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x0800, 0x1fff, mirror_ram_r },
	{ 0x2000, 0x3fff, ppu2c03b_0_r },
	{ 0x4000, 0x4015, NESPSG_0_r },
	{ 0x4016, 0x4016, pc10_in0_r },
	{ 0x4017, 0x4017, pc10_in1_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress cart_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM, &work_ram },
	{ 0x0800, 0x1fff, mirror_ram_w },
	{ 0x2000, 0x3fff, ppu2c03b_0_w },
	{ 0x4011, 0x4011, DAC_0_data_w },
	{ 0x4014, 0x4014, sprite_dma_w },
	{ 0x4000, 0x4015, NESPSG_0_w },
	{ 0x4016, 0x4016, pc10_in0_w },
	{ 0x4017, 0x4017, MWA_NOP }, /* in 1 writes ignored */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

/******************************************************************************/

INPUT_PORTS_START( playch10 )
    PORT_START	/* These are the BIOS buttons */
    PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE2, "Channel Select", KEYCODE_9, IP_JOY_NONE )	/* CHSelect 		*/
    PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_SERVICE3, "Enter", KEYCODE_0, IP_JOY_NONE )				/* Enter button 	*/
    PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_SERVICE4, "Reset", KEYCODE_MINUS, IP_JOY_NONE ) 		/* Reset button 	*/
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )												/* INT Detect		*/
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )												/* N/C				*/
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )													/* Coin 2			*/
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )												/* Service button	*/
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )													/* Coin 1			*/

    PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )	/* select button - masked */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )	/* start button - masked */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )	/* wired to 1p select button */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* wired to 1p start button */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START	/* IN2 - FAKE - Gun X pos */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X, 70, 30, 0, 255 )

	PORT_START	/* IN3 - FAKE - Gun Y pos */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y, 50, 30, 0, 255 )
INPUT_PORTS_END


static struct GfxLayout bios_charlayout =
{
    8,8,    /* 8*8 characters */
    1024,   /* 1024 characters */
    3,      /* 3 bits per pixel */
    { 0, 0x2000*8, 0x4000*8 },     /* the bitplanes are separated */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bios_charlayout,   0,  32 },
	{ -1 } /* end of array */
};

static int playch10_interrupt( void ) {

	/* LS161A, Sheet 1 - bottom left of Z80 */
	if ( !pc10_dog_di && !pc10_nmi_enable ) {
		cpu_set_reset_line( 0, PULSE_LINE );
		return ignore_interrupt();
	}

	if ( pc10_nmi_enable )
		return nmi_interrupt();

	return ignore_interrupt();
}

static struct NESinterface nes_interface =
{
	1,
	{ REGION_CPU2 },
	{ 50 },
};

static struct DACinterface nes_dac_interface =
{
	1,
	{ 50 },
};


#define PC10_MACHINE_DRIVER( name, nvram )								\
static struct MachineDriver machine_driver_##name =						\
{																		\
	/* basic machine hardware */										\
	{																	\
		{																\
			CPU_Z80,													\
			8000000 / 2,        /* 8 MHz / 2 */							\
			readmem,writemem,readport,writeport,						\
			playch10_interrupt, 1										\
		},																\
		{																\
			CPU_N2A03,													\
			N2A03_DEFAULTCLOCK,											\
			cart_readmem, cart_writemem, 0, 0,							\
			ignore_interrupt, 0											\
		}																\
	},																	\
	60, ( ( ( 1.0 / 60.0 ) * 1000000.0 ) / 262 ) * ( 262 - 239 ) * 2,  /* fps, vblank duration */				\
	1,	/* cpus dont talk to each other */								\
	pc10_init_machine,													\
																		\
	/* video hardware */												\
	32*8, 30*8*2, { 0*8, 32*8-1, 0*8, 30*8*2-1 },						\
	gfxdecodeinfo,														\
	256+4*16, 256+4*8,													\
	playch10_vh_convert_color_prom,										\
																		\
	VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR,								\
	0,																	\
	playch10_vh_start,													\
	playch10_vh_stop,													\
	playch10_vh_screenrefresh,											\
																		\
	/* sound hardware */												\
	0,0,0,0,															\
	{																	\
		{																\
			SOUND_NES,													\
			&nes_interface												\
		},																\
		{																\
			SOUND_DAC,													\
			&nes_dac_interface											\
		}																\
	},																	\
	nvram																\
};

PC10_MACHINE_DRIVER( playch10, )
PC10_MACHINE_DRIVER( playchnv, nvram_handler )

/***************************************************************************

  Game driver(s)

***************************************************************************/

#define BIOS_CPU											\
	ROM_REGION( 0x10000, REGION_CPU1 )						\
    ROM_LOAD( "pch1-c.8t",    0x00000, 0x4000, 0xd52fa07a )

#define BIOS_GFX											\
	ROM_REGION( 0x6000, REGION_GFX1 | REGIONFLAG_DISPOSE )	\
	ROM_LOAD( "pch1-c.8p",    0x00000, 0x2000, 0x30c15e23 )	\
    ROM_LOAD( "pch1-c.8m",    0x02000, 0x2000, 0xc1232eee )	\
    ROM_LOAD( "pch1-c.8k",    0x04000, 0x2000, 0x9acffb30 )	\
    ROM_REGION( 0x0300, REGION_PROMS )						\
    ROM_LOAD( "82s129.6f",    0x0000, 0x0100, 0xe5414ca3 )	\
    ROM_LOAD( "82s129.6e",    0x0100, 0x0100, 0xa2625c6e )	\
    ROM_LOAD( "82s129.6d",    0x0200, 0x0100, 0x1213ebd4 )

/******************************************************************************/

/* Standard Games */
ROM_START( pc_smb )		/* Super Mario Bros. */
	BIOS_CPU
	ROM_LOAD( "u3sm",    0x0c000, 0x2000, 0x4b5f717d ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x10000, REGION_CPU2 )  /* 64k for code */
    ROM_LOAD( "u1sm",    0x08000, 0x8000, 0x5cf548d3 )

    ROM_REGION( 0x02000, REGION_GFX2 )	/* cart gfx */
    ROM_LOAD( "u2sm",    0x00000, 0x2000, 0x867b51ad )

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0xbd82d775 )
ROM_END

ROM_START( pc_ebike )	/* Excite Bike */
	BIOS_CPU
	ROM_LOAD( "u3eb",    0x0c000, 0x2000, 0x8ff0e787 ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x10000, REGION_CPU2 )  /* 64k for code */
    ROM_LOAD( "u1eb",    0x0c000, 0x4000, 0x3a94fa0b )

    ROM_REGION( 0x02000, REGION_GFX2 )	/* cart gfx */
    ROM_LOAD( "u2eb",    0x00000, 0x2000, 0xe5f72401 )

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0xa0263750 )
ROM_END

/* Gun Games */
ROM_START( pc_duckh )	/* Duck Hunt */
	BIOS_CPU
	ROM_LOAD( "u3",      0x0c000, 0x2000, 0x2f9ec5c6 ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x10000, REGION_CPU2 )  /* 64k for code */
    ROM_LOAD( "u1",      0x0c000, 0x4000, 0x90ca616d )

    ROM_REGION( 0x04000, REGION_GFX2 )	/* cart gfx */
    ROM_LOAD( "u2",      0x00000, 0x2000, 0x4e049e03 )

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0x8cd6aad6 )
ROM_END

/* A-Board Games */
ROM_START( pc_tkfld )	/* Track & Field */
	BIOS_CPU
	ROM_LOAD( "u4tr",    0x0c000, 0x2000, 0x70184fd7 ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x10000, REGION_CPU2 )  /* 64k for code */
    ROM_LOAD( "u2tr",    0x08000, 0x8000, 0xd7961e01 )

    ROM_REGION( 0x08000, REGION_GFX2 )	/* cart gfx */
    ROM_LOAD( "u3tr",    0x00000, 0x8000, 0x03bfbc4b )

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0x1e2e7f1e )
ROM_END

/* B-Board Games */
ROM_START( pc_rnatk )	/* Rush N' Attack */
	BIOS_CPU
	ROM_LOAD( "ra-u4",   0x0c000, 0x2000, 0xebab7f8c ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x30000, REGION_CPU2 )  /* 64k for code */
    ROM_LOAD( "ra-u1",   0x10000, 0x10000, 0x5660b3a6 ) /* banked */
    ROM_LOAD( "ra-u2",   0x20000, 0x10000, 0x2a1bca39 ) /* banked */

	/* No cart gfx - uses vram */

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0x1f6596b2 )
ROM_END

ROM_START( pc_cntra )	/* Contra */
	BIOS_CPU
	ROM_LOAD( "u4ct",    0x0c000, 0x2000, 0x431486cf ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x30000, REGION_CPU2 )  /* 64k for code */
    ROM_LOAD( "u1ct",    0x10000, 0x10000, 0x9fcc91d4 ) /* banked */
    ROM_LOAD( "u2ct",    0x20000, 0x10000, 0x612ad51d ) /* banked */

	/* No cart gfx - uses vram */

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0x8ab3977a )
ROM_END

ROM_START( pc_pwrst )	/* Pro Wrestling */
	BIOS_CPU
	ROM_LOAD( "pw-u4",   0x0c000, 0x2000, 0x0f03d71b ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x30000, REGION_CPU2 )  /* 64k for code */
    ROM_LOAD( "pw-u1",   0x10000, 0x08000, 0x6242c2ce ) /* banked */
    ROM_RELOAD(			 0x18000, 0x08000 )
    ROM_LOAD( "pw-u2",   0x20000, 0x10000, 0xef6aa17c ) /* banked */

	/* No cart gfx - uses vram */

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0x4c6b7983 )
ROM_END

/* C-Board Games */
ROM_START( pc_goons )	/* The Goonies */
	BIOS_CPU
	ROM_LOAD( "gn-u3",   0x0c000, 0x2000, 0x33adedd2 ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x10000, REGION_CPU2 )  /* 64k for code */
    ROM_LOAD( "gn-u1",   0x08000, 0x8000, 0xefeb0c34 )

    ROM_REGION( 0x04000, REGION_GFX2 )	/* cart gfx */
    ROM_LOAD( "gn-u2",   0x00000, 0x4000, 0x0f9c7f49 )

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0xcdd62d08 )
ROM_END

/* E-Board Games */
ROM_START( pc_miket )	/* Mike Tyson's Punchout */
	BIOS_CPU
	ROM_LOAD( "u5pt",    0x0c000, 0x2000, 0xb434e567 ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x30000, REGION_CPU2 )  /* 64k for code */
    ROM_LOAD( "u1pt",    0x10000, 0x20000, 0xdfd9a2ee ) /* banked */

    ROM_REGION( 0x20000, REGION_GFX2 )	/* cart gfx */
    ROM_LOAD( "u3pt",    0x00000, 0x20000, 0x570b48ea )

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0x60f7ea1d )
ROM_END

/* F-Board Games */
ROM_START( pc_ngaid )	/* Ninja Gaiden */
	BIOS_CPU
	ROM_LOAD( "u2ng",    0x0c000, 0x2000, 0x7505de96 ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x50000, REGION_CPU2 )  /* 64k for code */
	ROM_LOAD( "u4ng",    0x10000, 0x20000, 0x5f1e7b19 )	/* banked */
	ROM_RELOAD(			 0x30000, 0x20000 )

    ROM_REGION( 0x020000, REGION_GFX2 )	/* cart gfx */
    ROM_LOAD( "u1ng",   0x00000, 0x20000, 0xeccd2dcb )	/* banked */

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0xec5641d6 )
ROM_END

ROM_START( pc_ddrgn )	/* Double Dragon */
	BIOS_CPU
	ROM_LOAD( "wd-u2",   0x0c000, 0x2000, 0xdfca1578 ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x50000, REGION_CPU2 )  /* 64k for code */
	ROM_LOAD( "wd-u4",  0x10000, 0x20000, 0x05c97f64 )	/* banked */
	ROM_RELOAD(			0x30000, 0x20000 )

    ROM_REGION( 0x020000, REGION_GFX2 )	/* cart gfx */
	ROM_LOAD( "wd-u1",  0x00000, 0x20000, 0x00000000 )

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0xf9739d62 )
ROM_END

/* G-Board Games */
ROM_START( pc_smb3 )	/* Super Mario Bros 3 */
	BIOS_CPU
	ROM_LOAD( "u3um",    0x0c000, 0x2000, 0x45e92f7f ) /* extra bios code for this game */
    BIOS_GFX

    ROM_REGION( 0x50000, REGION_CPU2 )  /* 64k for code */
	ROM_LOAD( "u4um",    0x10000, 0x20000, 0x590b4d7c )	/* banked */
	ROM_LOAD( "u5um",    0x30000, 0x20000, 0xbce25425 )	/* banked */

    ROM_REGION( 0x020000, REGION_GFX2 )	/* cart gfx */
    ROM_LOAD( "u1um",    0x00000, 0x20000, 0xc2928c49 )

    ROM_REGION( 0x0100,  REGION_USER1 )	/* rp5h01 data */
    ROM_LOAD( "security.prm", 0x00000, 0x10, 0xe48f4945 )
ROM_END


/***************************************************************************

  BIOS driver(s)

***************************************************************************/

ROM_START( playch10 )
    BIOS_CPU
	BIOS_GFX
ROM_END

/******************************************************************************/


/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the other drivers, so that we do not have to include */
/* them in every zip file */
GAMEX( 1986, playch10, 0, playch10, playch10, 0, ROT0, "Nintendo of America", "Playchoice-10", NOT_A_DRIVER )

/******************************************************************************/

/*    YEAR  NAME      PARENT	   MACHINE	 INPUT     INIT  	 MONITOR  */
GAME( 1984, pc_ebike, playch10, playch10, playch10, playch10, ROT0, "Nintendo", "PlayChoice-10: Excite Bike" )
GAME( 1985, pc_smb,	  playch10, playch10, playch10, playch10, ROT0, "Nintendo", "PlayChoice-10: Super Mario Bros." )

GAME( 1984, pc_duckh, playch10, playch10, playch10, pc_gun,   ROT0, "Nintendo", "PlayChoice-10: Duck Hunt" )

GAMEX(1987, pc_tkfld, playch10, playch10, playch10, pcaboard, ROT0, "Konami (Nintendo of America License)", "PlayChoice-10: Track & Field", GAME_NOT_WORKING )

GAME( 1986, pc_pwrst, playch10, playch10, playch10, pcbboard, ROT0, "Nintendo", "PlayChoice-10: Pro Wrestling" )
GAME( 1987, pc_rnatk, playch10, playch10, playch10, pcbboard, ROT0, "Konami (Nintendo of America License)", "PlayChoice-10: Rush N' Attack" )
GAME( 1988, pc_cntra, playch10, playch10, playch10, pcbboard, ROT0, "Konami (Nintendo of America License)", "PlayChoice-10: Contra" )

GAME( 1986, pc_goons, playch10, playch10, playch10, pccboard, ROT0, "Konami", "PlayChoice-10: The Goonies" )

GAMEX(1987, pc_miket, playch10, playchnv, playch10, pceboard, ROT0, "Nintendo", "PlayChoice-10: Mike Tyson's Punchout", GAME_NOT_WORKING )

GAME( 1989, pc_ngaid, playch10, playch10, playch10, pcfboard, ROT0, "Tecmo (Nintendo of America License)", "PlayChoice-10: Ninja Gaiden" )
GAMEX(198?, pc_ddrgn, playch10, playch10, playch10, pcfboard, ROT0, "Tecmo (Nintendo of America License)", "PlayChoice-10: Double Dragon", GAME_NOT_WORKING )

GAME( 1988, pc_smb3,  playch10, playch10, playch10, pcgboard, ROT0, "Nintendo", "PlayChoice-10: Super Mario Bros. 3" )
