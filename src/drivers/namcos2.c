/***************************************************************************

Namco System II driver by K.Wilkins Jun 1998

Phelios Notes:
Uses Custom protection IC 179
YM2151 and 8 Channel Custom 140


Cosmo Gang Notes:
CO1VOI1.BIN and CO1VOI2.BIN are both 11025Hz 8bit Signed raw samples (Looks like mono)

CPU1 - Game engine (68K)
CPU2 - Support engine (68K)
CPU3 - Sound/IO engine (6809)
CPU4 - microcontroller for dips/input ports

Notes from PCB

CPU1 PCB layout would indicate that it has the following local memory.

128Kx16 ROM		$000000-$03ffff
32Kx16 SRAM 	$100000-$10ffff
8Kx8 EEPROM 	$180000-$183fff
2Kx8 DPRAM		$460000-$460fff	- Dual ported to sound CPU
8Kx9 SRAM		$480000-$483fff

Looks like there is some kind of exchange I/F to the CPU2, as there are 10
buffer devices between the CPU, my guess 3-Address 2-data one set for read
and the other set for write.

There are two closeby ASIC devices marked 139 & 148.

Summary of graphics features:

2 x Static tile planes
4 x Scolling tile planes
2 x Rotate/Zoom tile planes
Sprite layer

***************************************************************************/

#define NAMCOS2_CREDITS "Keith Wilkins\nPhil Stroffolino"

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/namcos2.h"
#include "cpu/M6809/m6809.h"

/*************************************************************/

static struct YM2151interface ym2151_interface = {
	1,			/* 1 chip */
	3579580,	/* 3.58 MHZ ? */
	{ 100 },
	{ namcos2_sound_interrupt }
};

static int sound_protection_r( int offset ){
	static int data[4] = { 0x4b,0x4f,0x42,0x41 };
	return data[offset];
}

static struct MemoryReadAddress readmem_sound[] ={
	{ 0x0000, 0x0003, sound_protection_r },
	{ 0x0000, 0x3fff, BANKED_SOUND_ROM_R }, /* banked */
	{ 0x4000, 0x4001, YM2151_status_port_0_r },
	{ 0x5000, 0x51ff, MRA_RAM }, /* ..0x54ff shared? */
	{ 0x7000, 0x77ff, namcos2_dpram_byte_r },
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress writemem_sound[] = {
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4000, YM2151_register_port_0_w },
	{ 0x4001, 0x4001, YM2151_data_port_0_w },
	{ 0x5000, 0x51ff, MWA_RAM }, /* shared? */
	{ 0x7000, 0x77ff, namcos2_dpram_byte_w },
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xa000, MWA_NOP }, /* unknown port */
	{ 0xc000, 0xc001, namcos2_sound_bankselect_w },
	{ 0xd001, 0xd001, MWA_NOP },	/* watchdog? */
	{ 0xc000, 0xffff, MWA_ROM },
    { -1 }  /* end of table */
};


int namcos2_data_rom_r(int offset)
{
	int data;
	data= READ_WORD(&Machine->memory_region[MEM_DATA][offset]);
	return data;
}

/*************************************************************/

static struct MemoryReadAddress readmem[] = {
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x10ffff, CPU1_WORKINGRAM_R },
	{ 0x180000, 0x183fff, EEPROM_R },
	{ 0x1c0000, 0x1fffff, namcos2_1c0000_r },
	{ 0x200000, 0x23ffff, namcos2_data_rom_r },
	{ 0x400000, 0x40ffff, SHAREDVIDEORAM_R },
	{ 0x420000, 0x42003f, namcos2_420000_r },
	{ 0x440000, 0x44ffff, SHAREDRAM1_R },
	{ 0x460000, 0x460fff, namcos2_dpram_word_r },
	{ 0x480000, 0x483fff, CPU1_RAM2_R },
	{ 0x4a0000, 0x4a000f, namcos2_4a0000_r },
	{ 0xc00000, 0xc03fff, namcos2_sprite_word_r },
	{ 0xc40000, 0xc40001, namcos2_c40000_r },
	{ 0xc80000, 0xcbffff, ROZ_RAM_R },
	{ 0xcc0000, 0xcc000f, namcos2_roz_ctrl_r },
	{ 0xd00000, 0xd0000f, namcos2_d00000_r },
	{ -1 }
};

static struct MemoryWriteAddress writemem[] = {
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, CPU1_WORKINGRAM_W },
	{ 0x180000, 0x183fff, EEPROM_W },
	{ 0x1c0000, 0x1fffff, namcos2_1c0000_w },
	{ 0x400000, 0x40ffff, SHAREDVIDEORAM_W },
	{ 0x420000, 0x42003f, namcos2_420000_w },
	{ 0x440000, 0x44ffff, SHAREDRAM1_W },			/* Unknown */
	{ 0x460000, 0x460fff, namcos2_dpram_word_w },
	{ 0x480000, 0x483fff, CPU1_RAM2_W },			/* Unknown */
	{ 0x4a0000, 0x4a000f, namcos2_4a0000_w },		/* Unknown */
	{ 0xc00000, 0xc03fff, namcos2_sprite_word_w },
	{ 0xc40000, 0xc40001, namcos2_c40000_w },		/* Unknown */
	{ 0xc80000, 0xcbffff, ROZ_RAM_W },
	{ 0xcc0000, 0xcc000f, namcos2_roz_ctrl_w },		/* Unknown */
	{ 0xd00000, 0xd0000f, namcos2_d00000_w },		/* Unknown */
	{ -1 }
};

/*************************************************************/

static struct MemoryReadAddress readmem_sub[] = {
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x13ffff, CPU2_WORKINGRAM_R },
	{ 0x1e6000, 0x1e6001, MRA_NOP }, 		/* watchdog */
	{ 0x200000, 0x23ffff, namcos2_data_rom_r },
	{ 0x400000, 0x40ffff, SHAREDVIDEORAM_R },
	{ 0x440000, 0x44ffff, SHAREDRAM1_R },
	{ 0x460000, 0x460fff, namcos2_dpram_word_r },
	{ 0xc00000, 0xc03fff, namcos2_sprite_word_r },
	{ -1 }
};

static struct MemoryWriteAddress writemem_sub[] ={
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x13ffff, CPU2_WORKINGRAM_W },
	{ 0x1c0000, 0x1c0001, MWA_NOP },
	{ 0x1c2000, 0x1c2001, MWA_NOP },
	{ 0x1c4000, 0x1c4001, MWA_NOP },
	{ 0x1ce000, 0x1ce001, MWA_NOP },
	{ 0x1d2000, 0x1d2001, MWA_NOP },
	{ 0x1e4000, 0x1e4001, MWA_NOP },
	{ 0x1e6000, 0x1e6001, MWA_NOP },		/* watchdog */
	{ 0x400000, 0x40ffff, SHAREDVIDEORAM_W },
	{ 0x440000, 0x44ffff, SHAREDRAM1_W },
	{ 0x460000, 0x460fff, namcos2_dpram_word_w },
	{ 0xc00000, 0xc03fff, namcos2_sprite_word_w },
	{ -1 }
};

int wibble(int offset)
{
	return 0xff;
}

/*************************************************************/

static struct MemoryReadAddress readmem_mcu[] ={
	/* input ports and dips are mapped here */

    { 0x0001, 0x0001, input_port_0_r },
    { 0x0002, 0x0002, input_port_1_r },
    { 0x0003, 0x0003, input_port_2_r },
    { 0x0007, 0x0007, input_port_3_r },
	{ 0x0040, 0x01bf, MRA_RAM },
	{ 0x01c0, 0x1fff, MRA_ROM },
    { 0x2000, 0x2000, input_port_4_r },
    { 0x3000, 0x3000, input_port_5_r },
    { 0x3001, 0x3001, input_port_6_r },
    { 0x3002, 0x3002, input_port_7_r },
    { 0x3003, 0x3003, input_port_8_r },
	{ 0x5000, 0x57ff, namcos2_dpram_byte_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress writemem_mcu[] ={
	{ 0x0040, 0x01bf, MWA_RAM },
	{ 0x01c0, 0x1fff, MWA_ROM },
	{ 0x5000, 0x57ff, namcos2_dpram_byte_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }
};

/*************************************************************/

INPUT_PORTS_START( namcos2_input_ports )

	PORT_START      /* 63B05Z0 - PORT B */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START  	/* 63B05Z0 - PORT C & SCI */
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPF_TOGGLE, "Test SW", OSD_KEY_F1, IP_JOY_NONE)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - PORT D - ANALOG PORT 8 CHANNEL */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 63B05Z0 - PORT H */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )

	PORT_START  	/* 63B05Z0 - $2000 DIP SW */
	PORT_DIPNAME( 0x80, 0x80, "Test Mode")
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Test Mode" )
	PORT_DIPNAME( 0x40, 0x40, "$2000-6")
	PORT_DIPSETTING(    0x40, "H" )
	PORT_DIPSETTING(    0x00, "L" )
	PORT_DIPNAME( 0x20, 0x20, "$2000-5")
	PORT_DIPSETTING(    0x20, "H" )
	PORT_DIPSETTING(    0x00, "L" )
	PORT_DIPNAME( 0x10, 0x10, "$2000-4")
	PORT_DIPSETTING(    0x10, "H" )
	PORT_DIPSETTING(    0x00, "L" )
	PORT_DIPNAME( 0x08, 0x08, "$2000-3")
	PORT_DIPSETTING(    0x08, "H" )
	PORT_DIPSETTING(    0x00, "L" )
	PORT_DIPNAME( 0x04, 0x04, "$2000-2")
	PORT_DIPSETTING(    0x04, "H" )
	PORT_DIPSETTING(    0x00, "L" )
	PORT_DIPNAME( 0x02, 0x02, "$2000-1")
	PORT_DIPSETTING(    0x02, "H" )
	PORT_DIPSETTING(    0x00, "L" )
	PORT_DIPNAME( 0x01, 0x01, "$2000-0")
	PORT_DIPSETTING(    0x01, "H" )
	PORT_DIPSETTING(    0x00, "L" )

	PORT_START      /* 63B05Z0 - $3000 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 20, 0, 0, 0)

	PORT_START  	/* 63B05Z0 - $3001 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - $3002 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START  	/* 63B05Z0 - $3003 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END


static struct GfxLayout obj_layout = {
	32,32,
	0x800, /* number of sprites */
	8, /* bits per pixel */
	{ /* plane offsets */
		0x400000*0,0x400000*1,0x400000*2,0x400000*3,
		0x400000*0+4,0x400000*1+4,0x400000*2+4,0x400000*3+4
	},
	{ /* x offsets */
		0*8,0*8+1,0*8+2,0*8+3,
		1*8,1*8+1,1*8+2,1*8+3,
		2*8,2*8+1,2*8+2,2*8+3,
		3*8,3*8+1,3*8+2,3*8+3,

		4*8,4*8+1,4*8+2,4*8+3,
		5*8,5*8+1,5*8+2,5*8+3,
		6*8,6*8+1,6*8+2,6*8+3,
		7*8,7*8+1,7*8+2,7*8+3,
	},
	{ /* y offsets */
		0*128,0*128+64,1*128,1*128+64,
		2*128,2*128+64,3*128,3*128+64,
		4*128,4*128+64,5*128,5*128+64,
		6*128,6*128+64,7*128,7*128+64,

		8*128,8*128+64,9*128,9*128+64,
		10*128,10*128+64,11*128,11*128+64,
		12*128,12*128+64,13*128,13*128+64,
		14*128,14*128+64,15*128,15*128+64
	},
	0x800 /* sprite offset */
};

static struct GfxLayout chr_layout = {
	8,8,
	0x8000,
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	{ 0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64 },
	8*64
};

static struct GfxLayout roz_layout = {
	8,8,
	0x8000,
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	{ 0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64 },
	8*64
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ MEM_GFX_OBJ, 0x00000, &obj_layout, 0, 64 },
	{ MEM_GFX_CHR, 0x00000, &chr_layout, 0, 64 },
	{ MEM_GFX_ROZ, 0x00000, &roz_layout, 0, 64 },
	{ -1 }
};

/******************************************

Master clock = 49.152Mhz

68000 Measured at  84ns = 12.4Mhz   BUT 49.152MHz/4 = 12.288MHz = 81ns
6809  Measured at 343ns = 2.915 MHz BUT 49.152MHz/16 = 3.072MHz = 325ns
63B05 Measured at 120ns = 8.333 MHz BUT 49.152MHz/6 = 8.192MHz = 122ns

I've corrected all frequencies to be multiples of integer divisions of
the 49.152Mhz master clock. Additionally the 6305 looks to hav an
internal divider.

Soooo;

680000  = 12288000
6809    =  3072000
63B05Z0 =  2048000

The interrupts to CPU4 has been measured at 60Hz (16.5mS period) on a
logic analyser. This interrupt is wired to port PA1 which is configured
via software as INT1

*******************************************/
static struct MachineDriver machine_driver =
{
	{
		{
			CPU_M68000,
			12288000,
			MEM_CPU1,
			readmem,writemem,0,0,
			m68_level1_irq,1,
			0,0
		},
		{
			CPU_M68000,
			12288000,
			MEM_CPU2,
			readmem_sub,writemem_sub,0,0,
			m68_level1_irq,1,
			0,0
		},
		{
			CPU_M6809, // Sound handling
			3072000,
			MEM_CPU_SOUND,
			readmem_sound,writemem_sound,0,0,
			interrupt,1,
			0,0
		},
		{
			CPU_HD63705, // I/O handling
			2048000,
			MEM_CPU_MCU,
			readmem_mcu,writemem_mcu,0,0,
			interrupt,1,
			0,0
		}
	},
	60,
	DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100, /* 100 CPU slices per frame */
	namcos2_init_machine,

	/* video hardware */
	64*8, 64*8, { 0*8, 64*8-1, 0*8, 64*8-1 },
	gfxdecodeinfo,
	256,256,
	0,					/* Convert colour prom     */

	VIDEO_TYPE_RASTER,	/* Video attributes        */
	0,					/* Video initialisation    */
	namcos2_vh_start,	/* Video start             */
	namcos2_vh_stop,	/* Video stop              */
	namcos2_vh_update,	/* Video update            */

	/* sound hardware */
	0,0,0,0,
	/* Sound struct here */
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};


ROM_START( cosmogng_rom )
	ROM_REGION(0x40000)		/* Main CPU */
	ROM_LOAD_EVEN( "CO1MPR0.BIN" , 0x000000, 0x020000, 0xd1b4c8db )
	ROM_LOAD_ODD(  "CO1MPR1.BIN" , 0x000000, 0x020000, 0x2f391906 )

	ROM_REGION(0x040000)	/* Sprite/tile engine CPU ? */
	ROM_LOAD_EVEN( "CO1SPR0.BIN" , 0x000000, 0x020000, 0xbba2c28f )
	ROM_LOAD_ODD(  "CO1SPR1.BIN" , 0x000000, 0x020000, 0xc029b459 )

	ROM_REGION(0x30000)     /* 64K RAM + 128K banked ROM for the audio CPU, load the rom above the ram for paging */
	ROM_LOAD( "CO1SND0.BIN" , 0x010000, 0x020000, 0x6bfa619f )

	ROM_REGION(0x10000)		/* 64K RAM for MCU */
	ROM_LOAD("SYS2MCPU.BIN" , 0x0000, 0x2000, 0xa342a97e )
	ROM_LOAD("SYS2C65C.BIN" , 0x8000, 0x8000, 0xa5b2a4ff )

	ROM_REGION(0x200000)	/* Sprites */
	ROM_LOAD( "CO1OBJ0.BIN" , 0x000000, 0x080000, 0x5df8ce0c )
	ROM_LOAD( "CO1OBJ1.BIN" , 0x080000, 0x080000, 0x3d152497 )
	ROM_LOAD( "CO1OBJ2.BIN" , 0x100000, 0x080000, 0x4e50b6ee )
	ROM_LOAD( "CO1OBJ3.BIN" , 0x180000, 0x080000, 0x7beed669 )

	ROM_REGION(0x200000)	/* Tiles */
	ROM_LOAD( "CO1CHR0.BIN" , 0x000000, 0x080000, 0xee375b3e )
	ROM_LOAD( "CO1CHR1.BIN" , 0x080000, 0x080000, 0x0149de65 )
	ROM_LOAD( "CO1CHR2.BIN" , 0x100000, 0x080000, 0x93d565a0 )
	ROM_LOAD( "CO1CHR3.BIN" , 0x180000, 0x080000, 0x4d971364 )

	ROM_REGION(0x200000)		/* Tiles */
	ROM_LOAD( "CO1ROZ0.BIN" , 0x000000, 0x080000, 0x2bea6951 )

	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "CO1DAT0.BIN" , 0x000000, 0x020000, 0xb53da2ae )
	ROM_LOAD_ODD(  "CO1DAT1.BIN" , 0x000000, 0x020000, 0xd21ad10b )

ROM_END

ROM_START( assault_rom )
	ROM_REGION(0x40000)		/* Main CPU */
	ROM_LOAD_EVEN( "AT2MP0B.BIN" , 0x000000, 0x010000, 0x801f71c5 )
	ROM_LOAD_ODD(  "AT2MP1B.BIN" , 0x000000, 0x010000, 0x72312d4f )

	ROM_REGION(0x040000)	/* Sprite/tile engine CPU ? */
	ROM_LOAD_EVEN( "AT1SP0.BIN"  , 0x000000, 0x010000, 0x0de2a0da )
	ROM_LOAD_ODD(  "AT1SP1.BIN"  , 0x000000, 0x010000, 0x02d15fbe )

	ROM_REGION(0x30000)     /* 64K RAM + 128K banked ROM for the audio CPU, load the rom above the ram for paging */
	ROM_LOAD( "AT1SND0.BIN" , 0x010000, 0x020000, 0x1d1ffe12 )

	ROM_REGION(0x10000)		/* 64K RAM for MCU */
	ROM_LOAD("SYS2MCPU.BIN" , 0x0000, 0x2000, 0xa342a97e )
	ROM_LOAD("SYS2",          0x8000, 0x8000, 0xe9f2922a )

	ROM_REGION(0x200000)	/* Sprites */
	ROM_LOAD( "ATOBJ0.BIN" , 0x000000, 0x020000, 0x22240076 )
	ROM_LOAD( "ATOBJ1.BIN" , 0x020000, 0x020000, 0x2284a8e8 )
	ROM_LOAD( "ATOBJ2.BIN" , 0x040000, 0x020000, 0x51425476 )
	ROM_LOAD( "ATOBJ3.BIN" , 0x060000, 0x020000, 0x791f42ce )
	ROM_LOAD( "ATOBJ4.BIN" , 0x080000, 0x020000, 0x4782e1b0 )
	ROM_LOAD( "ATOBJ5.BIN" , 0x0a0000, 0x020000, 0xf5d158cf )
	ROM_LOAD( "ATOBJ6.BIN" , 0x0c0000, 0x020000, 0x12f6a569 )
	ROM_LOAD( "ATOBJ7.BIN" , 0x0e0000, 0x020000, 0x06a929f2 )

	ROM_REGION(0x200000)	/* Tiles */
	ROM_LOAD( "ATCHR0.BIN" , 0x000000, 0x020000, 0x6f8e968a )
	ROM_LOAD( "ATCHR1.BIN" , 0x020000, 0x020000, 0x88cf7cbe )

	ROM_REGION(0x200000)		/* Tiles */
	ROM_LOAD( "ATROZ0.BIN" , 0x000000, 0x020000, 0x8c247a97 )
	ROM_LOAD( "ATROZ1.BIN" , 0x020000, 0x020000, 0xe44c475b )
	ROM_LOAD( "ATROZ2.BIN" , 0x040000, 0x020000, 0x770f377f )
	ROM_LOAD( "ATROZ3.BIN" , 0x060000, 0x020000, 0x01d93d0b )
	ROM_LOAD( "ATROZ4.BIN" , 0x080000, 0x020000, 0xf96feab5 )
	ROM_LOAD( "ATROZ5.BIN" , 0x0a0000, 0x020000, 0xda2f0d9e )
	ROM_LOAD( "ATROZ6.BIN" , 0x0c0000, 0x020000, 0x9089e477 )
	ROM_LOAD( "ATROZ7.BIN" , 0x0e0000, 0x020000, 0x62b2783a )

	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "AT1DAT0.BIN" , 0x000000, 0x020000, 0x844890f4 )
	ROM_LOAD_ODD(  "AT1DAT1.BIN" , 0x000000, 0x020000, 0x21715313 )

ROM_END

ROM_START( phelios_rom )
	ROM_REGION( 0x80000 ) /* main CPU */
	ROM_LOAD_EVEN( "ps1mpr0.bin",	0x00000, 0x20000, 0xbfbe96c6 )
	ROM_LOAD_ODD ( "ps1mpr1.bin",	0x00000, 0x20000, 0xf5c0f883 )

	ROM_REGION( 0x20000 ) /* sub CPU */
	ROM_LOAD_EVEN( "ps1spr0.bin",	0x00000, 0x10000, 0xe9c6987e )
	ROM_LOAD_ODD ( "ps1spr1.bin",	0x00000, 0x10000, 0x02b074fb )

	ROM_REGION( 0x20000 ) /* sound CPU */
	ROM_LOAD( "ps1snd1.bin", 0x00000, 0x20000, 0xda694838 )

	ROM_REGION( 0x10000 ) /* MCU */
	ROM_LOAD("SYS2MCPU.BIN" , 0x0000, 0x2000, 0xa342a97e )
	ROM_LOAD("SYS2C65C.BIN" , 0x8000, 0x8000, 0x00000000 )

	ROM_REGION(0x200000) /* Sprites */
	ROM_LOAD( "psobj0.bin",		0x000000, 0x40000, 0xf323db2b )
	ROM_LOAD( "psobj4.bin",		0x040000, 0x40000, 0x81ea86c6 )
	ROM_LOAD( "psobj1.bin",		0x080000, 0x40000, 0xfaf7c2f5 )
	ROM_LOAD( "psobj5.bin",		0x0c0000, 0x40000, 0xaaebd51a )
	ROM_LOAD( "psobj2.bin",		0x100000, 0x40000, 0x828178ba )
	ROM_LOAD( "psobj6.bin",		0x140000, 0x40000, 0x032ea497 )
	ROM_LOAD( "psobj3.bin",		0x180000, 0x40000, 0xe84771c8 )
	ROM_LOAD( "psobj7.bin",		0x1c0000, 0x40000, 0xf6183b36 )

	ROM_REGION( 0x200000 ) /* Tiles */
	ROM_LOAD( "pschr0.bin",		0x00000, 0x20000, 0x668b6670 )
	ROM_LOAD( "pschr1.bin",		0x20000, 0x20000, 0x80c30742 )
	ROM_LOAD( "pschr2.bin",		0x40000, 0x20000, 0xf4e11bf7 )
	ROM_LOAD( "pschr3.bin",		0x60000, 0x20000, 0x97a93dc5 )
	ROM_LOAD( "pschr4.bin",		0x80000, 0x20000, 0x81d965bf )
	ROM_LOAD( "pschr5.bin",		0xa0000, 0x20000, 0x8ca72d35 )
	ROM_LOAD( "pschr6.bin",		0xc0000, 0x20000, 0xda3543a9 )

	ROM_REGION( 0x200000 ) /* Tiles */
	ROM_LOAD( "psroz0.bin",		0x00000, 0x20000, 0x68043d7e )
	ROM_LOAD( "psroz1.bin",		0x20000, 0x20000, 0x029802b4 )
	ROM_LOAD( "psroz2.bin",		0x40000, 0x20000, 0xbf0b76dc )
	ROM_LOAD( "psroz3.bin",		0x60000, 0x20000, 0x9c821455 )
	ROM_LOAD( "psroz4.bin",		0x80000, 0x20000, 0x63a39b7a )
	ROM_LOAD( "psroz5.bin",		0xa0000, 0x20000, 0xfc5a99d0 )
	ROM_LOAD( "psroz6.bin",		0xc0000, 0x20000, 0xa2a17587 )

	ROM_REGION( 0x40000 ) /* sub CPU */
	ROM_LOAD_EVEN( "ps1dat0.bin", 	0x00000, 0x20000, 0xee4194b0 )
	ROM_LOAD_ODD ( "ps1dat1.bin", 	0x00000, 0x20000, 0x5b22d714 )

//	ROM_REGION( 512*1024 ) /* voice */
//	ROM_LOAD( "psvoi-1.bin",	0x00000, 512*1024, 0 )
ROM_END

ROM_START( rthun2_rom )
	ROM_REGION(0x40000)		/* Main CPU */
	ROM_LOAD_EVEN( "MPR0.BIN" , 0x000000, 0x020000, 0xe09a3549 )
	ROM_LOAD_ODD(  "MPR1.BIN" , 0x000000, 0x020000, 0x09573bff )

	ROM_REGION( 0x20000 )	/* Sprite/tile engine CPU ? */
	ROM_LOAD_EVEN( "SPR0.BIN" , 0x000000, 0x010000, 0x570d1a62 )
	ROM_LOAD_ODD(  "SPR1.BIN" , 0x000000, 0x010000, 0x060eb393 )

	ROM_REGION(0x50000)	/* sound CPU */
	ROM_LOAD( "SND0.BIN" , 0x10000, 0x20000, 0x55b7562a )
	ROM_LOAD( "SND1.BIN" , 0x30000, 0x20000, 0x00445a4f )

	ROM_REGION( 0x10000 ) /* MCU */
	ROM_LOAD("SYS2MCPU.BIN" , 0x0000, 0x2000, 0xa342a97e )
	ROM_LOAD("SYS.BIN"      , 0x8000, 0x8000, 0xa5b2a4ff )

	ROM_REGION(0x200000)	/* Sprites */
	ROM_LOAD( "OBJ0.BIN" , 0x000000, 0x80000, 0xe5cb82c1 )
	ROM_LOAD( "OBJ1.BIN" , 0x080000, 0x80000, 0x19ebe9fd )
	ROM_LOAD( "OBJ2.BIN" , 0x100000, 0x80000, 0x455c4a2f )
	ROM_LOAD( "OBJ3.BIN" , 0x180000, 0x80000, 0xfdcae8a9 )

	ROM_REGION(0x200000)	/* Tiles */
	ROM_LOAD( "CHR0.BIN" , 0x000000, 0x80000, 0x6f0e9a68 )
	ROM_LOAD( "CHR1.BIN" , 0x080000, 0x80000, 0x15e44adc )

	ROM_REGION(0x200000)		/* Tiles */
	ROM_LOAD( "ROZ0.BIN" , 0x00000, 0x80000, 0x482d0554 )

	ROM_REGION( 0x80000 )	/* Sprite/tile engine CPU ? */
	ROM_LOAD_EVEN( "DATA0.BIN" , 0x000000, 0x020000, 0x0baf44ee )
	ROM_LOAD_ODD(  "DATA1.BIN" , 0x000000, 0x020000, 0x58a8daac )
	ROM_LOAD_EVEN( "DATA2.BIN" , 0x040000, 0x020000, 0x8e850a2a )

	//shape.bin
	//voi1.bin, voi2.bin
ROM_END


ROM_START( metlhawk_rom )
	ROM_REGION(0x40000)		/* Main CPU */
	ROM_LOAD_EVEN( "MH2MP0C.11D" , 0x000000, 0x020000, 0xcd7dae6e )
	ROM_LOAD_ODD(  "MH2MP1C.13D" , 0x000000, 0x020000, 0xe52199fd )

	ROM_REGION( 0x20000 )	/* Sprite/tile engine CPU ? */
	ROM_LOAD_EVEN( "MH1SP0F.11K" , 0x000000, 0x010000, 0x2c141fea )
	ROM_LOAD_ODD(  "MH1SP1F.13K" , 0x000000, 0x010000, 0x8ccf98e0 )

	ROM_REGION(0x50000)	/* sound CPU */
	ROM_LOAD( "MH1S0.7J" , 0x00000, 0x20000, 0x79e054cf )

	ROM_REGION( 0x10000 ) /* MCU */
	ROM_LOAD("SYS2MCPU.BIN" , 0x0000, 0x2000, 0xa342a97e )
	ROM_LOAD("SYS2C65C.3F"  , 0x8000, 0x8000, 0xa5b2a4ff )

	ROM_REGION(0x200000)	/* Sprites */
	ROM_LOAD( "MHOBJ-0.5D" , 0x000000, 0x40000, 0x52ae6620 )
	ROM_LOAD( "MHOBJ-1.5B" , 0x040000, 0x40000, 0x2c2a1291 )
	ROM_LOAD( "MHOBJ-2.6D" , 0x080000, 0x40000, 0x6221b927 )
	ROM_LOAD( "MHOBJ-3.6B" , 0x0c0000, 0x40000, 0xfd09f2f1 )
	ROM_LOAD( "MHOBJ-4.5C" , 0x100000, 0x40000, 0xe3590e1a )
	ROM_LOAD( "MHOBJ-5.5A" , 0x140000, 0x40000, 0xb85c0d07 )
	ROM_LOAD( "MHOBJ-6.6C" , 0x180000, 0x40000, 0x90c4523d )
	ROM_LOAD( "MHOBJ-7.6A" , 0x1c0000, 0x40000, 0xf00edb39 )

	ROM_REGION(0x200000)	/* Tiles */
	ROM_LOAD( "MHCHR-0.11N" , 0x000000, 0x20000, 0xe2da1b14 )
	ROM_LOAD( "MHCHR-1.11P" , 0x020000, 0x20000, 0x023c78f9 )
	ROM_LOAD( "MHCHR-2.11R" , 0x040000, 0x20000, 0xece47e91 )

	ROM_REGION(0x200000)		/* Tiles */
	ROM_LOAD( "MHR0Z-0.2D" , 0x000000, 0x40000, 0x30ade98f )
	ROM_LOAD( "MHR0Z-1.2C" , 0x040000, 0x40000, 0xa7fff42a )
	ROM_LOAD( "MHR0Z-2.2B" , 0x080000, 0x40000, 0x6abec820 )
	ROM_LOAD( "MHR0Z-3.2A" , 0x0C0000, 0x40000, 0xd53cec6d )
	ROM_LOAD( "MHR0Z-4.3D" , 0x100000, 0x40000, 0x922213e2 )
	ROM_LOAD( "MHR0Z-5.3C" , 0x140000, 0x40000, 0x78418a54 )
	ROM_LOAD( "MHR0Z-6.3B" , 0x180000, 0x40000, 0x6c74977e )
	ROM_LOAD( "MHR0Z-7.3A" , 0x1c0000, 0x40000, 0x68a19cbd )

	ROM_REGION( 0x80000 )	/* Sprite/tile engine CPU ? */
	ROM_LOAD_EVEN( "MH1D0.13S" , 0x000000, 0x020000, 0x8b178ac7 )
	ROM_LOAD_ODD(  "MH1D1.13P" , 0x000000, 0x020000, 0x10684fd6 )

	//shape.bin
	//voi1.bin, voi2.bin
ROM_END


static void cosmogng_decode(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM;

//	RAM = Machine->memory_region[NAMCOS2_CPU1];
// nop out some checks
//	WRITE_WORD( &RAM[0x00008c], 0x4e71 );
//	WRITE_WORD( &RAM[0x0027c2], 0x4e71 );

	// Force RTS and Stop DIP Switch test
//	WRITE_WORD( &RAM[0x00193a], 0x4e75 );

	// Stop hang CPU1 IRQ with on Sound CPU
	//	WRITE_WORD( &RAM[0x00015a], 0x4e71 );
	// Removed now that CPU4 is partially implemented

	// Stop sound CPU test failing
	//	WRITE_WORD( &RAM[0x00118a], 0x6028 );	// Branch over fail test
	// Removed now that CPU4 is partially implemented

	// CPU2 Patches
//	RAM = Machine->memory_region[NAMCOS2_CPU2];
//	WRITE_WORD( &RAM[0x001140], 0x4e71 );       // Patch out CPU2 tests for DIPSW that are causing halt
//	WRITE_WORD( &RAM[0x00114a], 0x4e71 );

	// CPU4 Patches
	RAM = Machine->memory_region[NAMCOS2_CPU4];
	RAM[0x81f9]=0x9d;
	RAM[0x81fa]=0x9d;							// Stop waiting for A/D conversion complete

}

static void assault_decode(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM;
	RAM = Machine->memory_region[NAMCOS2_CPU4];
	RAM[0x81f9]=0x9d;
	RAM[0x81fa]=0x9d;							// Stop waiting for A/D conversion complete
	RAM[0x823f]=0x9d;
	RAM[0x8240]=0x9d;							// Stop waiting for A/D conversion complete

}

static void phelios_decode(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM;
	RAM = Machine->memory_region[NAMCOS2_CPU4];
	RAM[0x81f9]=0x9d;
	RAM[0x81fa]=0x9d;							// Stop waiting for A/D conversion complete
	RAM[0x823f]=0x9d;
	RAM[0x8240]=0x9d;							// Stop waiting for A/D conversion complete

}

static void rthun2_decode(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM;
	RAM = Machine->memory_region[NAMCOS2_CPU4];
	RAM[0x81f9]=0x9d;
	RAM[0x81fa]=0x9d;							// Stop waiting for A/D conversion complete
	RAM[0x823f]=0x9d;
	RAM[0x8240]=0x9d;							// Stop waiting for A/D conversion complete

}

static void metlhawk_decode(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM;
	RAM = Machine->memory_region[NAMCOS2_CPU4];
	RAM[0x81f9]=0x9d;
	RAM[0x81fa]=0x9d;							// Stop waiting for A/D conversion complete
	RAM[0x823f]=0x9d;
	RAM[0x8240]=0x9d;							// Stop waiting for A/D conversion complete

}

struct GameDriver cosmogng_driver =
{
	__FILE__,
	0,
	"cosmogng",
	"Cosmo Gang the Video",
	"1991",
	"Namco",
	NAMCOS2_CREDITS,
//	GAME_NOT_WORKING,		/* Flags */
	0,
	&machine_driver,
	0,	/* One-time driver init */

	cosmogng_rom,
	cosmogng_decode, 0,
	0,
	0,

	namcos2_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,
	namcos2_hiload,namcos2_hisave
};


struct GameDriver assault_driver =
{
	__FILE__,
	0,
	"assault",
	"Assault",
	"1988",
	"Namco",
	NAMCOS2_CREDITS,
	GAME_NOT_WORKING,		/* Flags */
	&machine_driver,
	0,	/* One-time driver init */

	assault_rom,
	assault_decode, 0,
	0,
	0,

	namcos2_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,
	namcos2_hiload,namcos2_hisave
};


struct GameDriver phelios_driver =
{
	__FILE__,
	0,
	"phelios",
	"Phelios",
	"1988",
	"Namco",
	NAMCOS2_CREDITS,
	GAME_NOT_WORKING,		/* Flags */
	&machine_driver,
	0,	/* One-time driver init */
	phelios_rom,
	phelios_decode, 0,
	0,
	0,
	namcos2_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_90,
	namcos2_hiload,namcos2_hisave
};

struct GameDriver rthun2_driver =
{
	__FILE__,
	0,
	"rthun2",
	"Rolling Thunder II",
	"198?",
	"Namco",
	NAMCOS2_CREDITS,
	GAME_NOT_WORKING,		/* Flags */
	&machine_driver,
	0,	/* One-time driver init */
	rthun2_rom,
	rthun2_decode, 0,
	0,
	0,
	namcos2_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	namcos2_hiload,namcos2_hisave
};

struct GameDriver metlhawk_driver =
{
	__FILE__,
	0,
	"metlhawk",
	"Metal Hawk",
	"1988",
	"Namco",
	NAMCOS2_CREDITS,
	GAME_NOT_WORKING,		/* Flags */
	&machine_driver,
	0,	/* One-time driver init */
	metlhawk_rom,
	metlhawk_decode, 0,
	0,
	0,
	namcos2_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_90,
	namcos2_hiload,namcos2_hisave
};

