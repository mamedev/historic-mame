/***************************************************************************

Namco System II driver by K.Wilkins Jun 1998

The Namco System II board is a 4 (I think I've found a 5th!!) CPU system. The
complete system consists of two boards: CPU + GRAPHICS. It contains a large
number of custom ASICs to perform graphics operations, there is no documentation
available for these parts. There is an Atari Manual for ASSAULT on spies that
contains scans of the schematics.

CPU1 - Master CPU           (68K)
CPU2 - Slave CPU            (68K)
CPU3 - Sound/IO engine      (6809)
CPU4 - IO Microcontroller   (63705) Dips/input ports

The 4 CPU's are all connected via a central 2KByte dual port SRAM. The two 68000s
are on one side and the 6809/63705 are on the other side.

Each 68000 has its own private bus area AND a common shared area between the two
devices, which is where the video ram/dual port/Sprite Generation etc logic sits.

Summary of graphics features:

2 x Static tile planes
4 x Scolling tile planes
1 x Rotate/Zoom tile plane
Sprite layer

The system would seem to be capable of a total of 4096 colours based on the layout
of the palette memory.

The Dual 68000 Share memory map area is shown below, this is taken from the memory
decoding pal from the Cosmo Gang board and so should be accuate.

MAIN PCB
--------
SCR     = $400000-$41ffff           (Text screen memory)
SCRDT   = $420000-$43ffff
PALET   = $440000-$45ffff           (Pallette control)
DPCS    = $460000-$47ffff           (Dual port memory - All CPUs have access)
SCOM    = $480000-$49ffff           (Serial comms processor)
SCOMDT  = $4a0000-$4bffff
ROM1    = $300000-$3fffff           (Data roms)
ROM0    = $200000-$2fffff

GRAPHICS PCB
------------
OBJRAM  = $c00000-$???????          (4096 Sprites: 16 Banks of 128, 64 bits per sprite)
OBJBANK = $c80000-$???????          (Lower 4 bits control sprite bank)
RFRAM   = $c80000-$???????          (Rotation/Zoom RAM)
RFREG   = $cc0000-$???????
KEY     = $d00000-$???????          (Security device)

All interrupt handling is done on the 68000s by two identical custom devices (C148),
this device takes the level based signals and encodes them into the 3 bit encoded
form for the 68000 CPU. The master CPU C148 also controls the reset for the slave
CPU and MCU which are common. The C148 only has the lower 3 data bits connected.

C148 Features
-------------
3 Bit output port
3 Bit input port
3 Chip selects
68000 Interrupt encoding/handling
Data strobe control
Bus arbitration
Reset output
Watchdog


C148pin     Master CPU      Slave CPU
-------------------------------------
YBNK        VBLANK          VBLANK
IRQ4        SCIRQ           SCIRQ       (Serial comms IC Interrupt)
IRQ3        POSIRQ          POSIRQ      (Comes from C116, pixel generator, Position interrup ?? line based ??)
IRQ2        EXIRQ           EXIRQ       (Goes to video board but does not appear to be connected)
IRQ1        SCPUIRQ         MCPUIRQ     (Master/Slave interrupts)

OP0         SSRES                       (Sound CPU reset - 6809 only)
OP1
OP2

IP0         EEPROM BUSY
IP1
IP2



Protection
----------
The Chip at $d00000 seems to be heavily involved in protection, some games lock or reset if it doesnt
return the correct values, it MAY be a random number generator and is testing the values based on
the inputted seed value. rthun2 is sprinkled with reads to $d00006 which look like they are being
used as random numbers. rthun 2 also checks the response value after a number is written. Device
takes clock and vblank. Only output is reset.

This chip is based on the graphics board.

$d00000
$d00002
$d00004     Write 13 x $0000, read back $00bd from $d00002 (burnf)
$d00006     Write $b929, read $014a (cosmog)
$d00008     Write $13ec, read $013f (rthun2)
$d0000a     Write $f00f, read $f00f (phelios)
$d0000c     Write $8fc8, read $00b2 (rthun2)
$d0000e     Write $31ad, read $00bd (burnf)



----------

Phelios Notes:
Uses Custom protection IC 179
YM2151 and 8 Channel Custom 140

----------

Cosmo Gang Notes:
CO1VOI1.BIN and CO1VOI2.BIN are both 11025Hz 8bit Signed raw samples (Looks like mono)


***************************************************************************/

#define NAMCOS2_CREDITS "Keith Wilkins\nPhil Stroffolino"

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/namcos2.h"
#include "cpu/M6809/m6809.h"

/*************************************************************/

static int sound_protection_r( int offset )
{
	static int data[4] = { 0x4b,0x4f,0x42,0x41 };
	return data[offset];
}

/*************************************************************/
/* SHARED 68000 CPU Memory declarations                      */
/*************************************************************/

//  ROM0   = $200000-$2fffff
//  ROM1   = $300000-$3fffff
//  SCR    = $400000-$41ffff
//  SCRDT  = $420000-$43ffff
//  PALET  = $440000-$45ffff
//  DPCS   = $460000-$47ffff
//  SCOM   = $480000-$49ffff
//  SCOMDT = $4a0000-$4bffff

// 0xc00000 ONWARDS are unverified memory locations on the video board

#define NAMCOS2_68K_SHARED_MEMORY_READ \
    { 0x200000, 0x3fffff, namcos2_68k_data_rom_r },\
    { 0x400000, 0x41ffff, NAMCOS2_68K_VIDEORAM_R },\
    { 0x420000, 0x43ffff, namcos2_68k_vram_ctrl_r }, \
    { 0x440000, 0x45ffff, namcos2_68k_video_palette_r }, \
    { 0x460000, 0x47ffff, namcos2_68k_dpram_word_r }, \
    { 0x480000, 0x49ffff, namcos2_68k_serial_comms_ram_r }, \
    { 0x4a0000, 0x4bffff, namcos2_68k_serial_comms_ctrl_r },\
/*	{ 0xc00000, 0xc03fff, namcos2_68k_sprite_ram_r }, CANNOT READ BACK - DEBUG ONLY */  \
/*	{ 0xc40000, 0xc40001, namcos2_68k_sprite_bank_r },CANNOT READ BACK - DEBUG ONLY */  \
	{ 0xc80000, 0xcbffff, NAMCOS2_68K_ROZ_RAM_R },	\
	{ 0xcc0000, 0xcc000f, namcos2_68k_roz_ctrl_r },\
	{ 0xd00000, 0xd0000f, namcos2_68k_key_r },

#define NAMCOS2_68K_SHARED_MEMORY_WRITE \
    { 0x400000, 0x41ffff, NAMCOS2_68K_VIDEORAM_W },\
    { 0x420000, 0x43ffff, namcos2_68k_vram_ctrl_w }, \
    { 0x440000, 0x45ffff, namcos2_68k_video_palette_w }, \
    { 0x460000, 0x47ffff, namcos2_68k_dpram_word_w }, \
    { 0x480000, 0x49ffff, namcos2_68k_serial_comms_ram_w }, \
    { 0x4a0000, 0x4bffff, namcos2_68k_serial_comms_ctrl_w },\
	{ 0xc00000, 0xc03fff, namcos2_68k_sprite_ram_w }, \
	{ 0xc40000, 0xc40001, namcos2_68k_sprite_bank_w },\
	{ 0xc80000, 0xcbffff, NAMCOS2_68K_ROZ_RAM_W }, \
	{ 0xcc0000, 0xcc000f, namcos2_68k_roz_ctrl_w },\
	{ 0xd00000, 0xd0000f, namcos2_68k_key_w },



/*************************************************************/
/* MASTER 68000 CPU Memory declarations                      */
/*************************************************************/

static struct MemoryReadAddress readmem_master[] = {
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_R },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_r },
    NAMCOS2_68K_SHARED_MEMORY_READ
	{ -1 }
};

static struct MemoryWriteAddress writemem_master[] = {
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_W },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_w },
    NAMCOS2_68K_SHARED_MEMORY_WRITE
	{ -1 }
};

/*************************************************************/
/* SLAVE 68000 CPU Memory declarations                       */
/*************************************************************/

static struct MemoryReadAddress readmem_slave[] = {
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_r },
    NAMCOS2_68K_SHARED_MEMORY_READ
	{ -1 }
};

static struct MemoryWriteAddress writemem_slave[] ={
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_w },
    NAMCOS2_68K_SHARED_MEMORY_WRITE
	{ -1 }
};


/*************************************************************/
/* 6809 SOUND CPU Memory declarations                        */
/*************************************************************/

static struct MemoryReadAddress readmem_sound[] ={
//	{ 0x0000, 0x0003, sound_protection_r },
	{ 0x0000, 0x3fff, BANKED_SOUND_ROM_R }, /* banked */
	{ 0x4000, 0x4001, YM2151_status_port_0_r },
    { 0x5000, 0x50ff, namcos1_wavedata_r,&namco_wavedata },  /* PSG ( Shared) */
    { 0x5100, 0x513f, namcos1_sound_r,&namco_soundregs }, /* PSG ( Shared ) */
	{ 0x5140, 0x54ff, MRA_RAM }, /* ..0x54ff shared? */
	{ 0x7000, 0x77ff, namcos2_dpram_byte_r },
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress writemem_sound[] = {
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4000, YM2151_register_port_0_w },
	{ 0x4001, 0x4001, YM2151_data_port_0_w },
    { 0x5000, 0x50ff, namcos1_wavedata_w,&namco_wavedata },  /* PSG ( Shared) */
    { 0x5100, 0x513f, namcos1_sound_w,&namco_soundregs }, /* PSG ( Shared ) */
	{ 0x5140, 0x54ff, MWA_RAM }, /* shared? */
	{ 0x7000, 0x77ff, namcos2_dpram_byte_w },
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xa000, MWA_NOP }, /* unknown port */
	{ 0xc000, 0xc001, namcos2_sound_bankselect_w },
	{ 0xd001, 0xd001, MWA_NOP },	/* watchdog? */
	{ 0xc000, 0xffff, MWA_ROM },
    { -1 }  /* end of table */
};


/*************************************************************/
/* 68705 IO CPU Memory declarations                          */
/*************************************************************/

static struct MemoryReadAddress readmem_mcu[] ={
	/* input ports and dips are mapped here */

	{ 0x0000, 0x0000, MRA_NOP },			// Keep logging quiet
    { 0x0001, 0x0001, input_port_0_r },
    { 0x0002, 0x0002, input_port_1_r },
    { 0x0003, 0x0003, input_port_2_r },
    { 0x0007, 0x0007, input_port_3_r },
	{ 0x0008, 0x003f, MRA_RAM },			// Fill in register to stop logging
	{ 0x0040, 0x01bf, MRA_RAM },
	{ 0x01c0, 0x1fff, MRA_ROM },
    { 0x2000, 0x2000, input_port_4_r },
    { 0x3000, 0x3000, input_port_5_r },
    { 0x3001, 0x3001, input_port_6_r },
    { 0x3002, 0x3002, input_port_7_r },
    { 0x3003, 0x3003, input_port_8_r },
	{ 0x5000, 0x57ff, namcos2_dpram_byte_r },
	{ 0x6000, 0x6000, MRA_NOP },				/* watchdog */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress writemem_mcu[] ={
	{ 0x0000, 0x003f, MWA_RAM },			// Fill in register to stop logging
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
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "Service", KEYCODE_F2, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPF_TOGGLE, "Test", KEYCODE_F1, IP_JOY_NONE)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )

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
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 20, 10, 0, 0, 0)

	PORT_START  	/* 63B05Z0 - $3001 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - $3002 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START  	/* 63B05Z0 - $3003 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END


static struct GfxLayout obj_layout = {
	32,32,
	0x800,  /* number of sprites */
	8,      /* bits per pixel */
	{       /* plane offsets */
		0x400000*0,0x400000*1,0x400000*2,0x400000*3,
		(0x400000*0)+4,(0x400000*1)+4,(0x400000*2)+4,(0x400000*3)+4
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
	0x10000,
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	{ 0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64 },
	8*64
};

static struct GfxLayout roz_layout = {
	8,8,
	0x10000,
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	{ 0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64 },
	8*64
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ MEM_GFX_OBJ, 0x000000, &obj_layout, 0, 64 },
	{ MEM_GFX_OBJ, 0x200000, &obj_layout, 0, 64 },
	{ MEM_GFX_CHR, 0x000000, &chr_layout, 0, 64 },
	{ MEM_GFX_ROZ, 0x000000, &roz_layout, 0, 64 },
	{ -1 }
};


static struct YM2151interface ym2151_interface = {
	1,			/* 1 chip */
	3579580,	/* 3.58 MHZ ? */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ NULL }    /* YM2151 IRQ line is NOT connected on the PCB */
};


static struct namco_interface namco_interface =
{
	23920/2,	/* sample rate (approximate value) */
	8,		/* number of voices */
	50,		/* playback volume */
//	MEM_VOICE,	/* memory region */
    -1,
	1		/* stereo */
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
            readmem_master,writemem_master,0,0,
            namcos2_68k_master_vblank,1,
            0,0
		},
		{
			CPU_M68000,
			12288000,
			MEM_CPU2,
			readmem_slave,writemem_slave,0,0,
            namcos2_68k_slave_vblank,1,
            0,0
		},
		{
			CPU_M6809, // Sound handling
			3072000,
			MEM_CPU_SOUND,
			readmem_sound,writemem_sound,0,0,
//			namcos2_sound_interrupt,1,   /* The FIRQ is used by the BIG PSG device */
			interrupt,1,
//			0,0
			namcos2_sound_interrupt,60
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
//	64*8, 64*8, { 0*8, 64*8-1, 0*8, 64*8-1 },
	36*8, 32*8, { 0*8, 36*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,256,
	0,					/* Convert colour prom     */

	VIDEO_TYPE_RASTER,	/* Video attributes        */
	0,					/* Video initialisation    */
	namcos2_vh_start,	/* Video start             */
	namcos2_vh_stop,	/* Video stop              */
	namcos2_vh_update,	/* Video update            */

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	/* Sound struct here */
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_NAMCO,
			&namco_interface
		}
	}
};

#if 0
static struct MachineDriver machine_driver_alt =
{
	{
		{
			CPU_M68000,
			12288000,
			MEM_CPU1,
			readmem_master,writemem_master,0,0,
			m68_level4_irq,1,
			0,0
		},
		{
			CPU_M68000,
			12288000,
			MEM_CPU2,
			readmem_slave,writemem_slave,0,0,
			m68_level4_irq,1,
			0,0
		},
		{
			CPU_M6809, // Sound handling
			3072000,
			MEM_CPU_SOUND,
			readmem_sound,writemem_sound,0,0,
			namcos2_sound_interrupt,1,
//			interrupt,1,
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
//	64*8, 64*8, { 0*8, 64*8-1, 0*8, 64*8-1 },
	36*8, 32*8, { 0*8, 36*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,256,
	0,					/* Convert colour prom     */

	VIDEO_TYPE_RASTER,	/* Video attributes        */
	0,					/* Video initialisation    */
	namcos2_vh_start,	/* Video start             */
	namcos2_vh_stop,	/* Video stop              */
	namcos2_vh_update,	/* Video update            */

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	/* Sound struct here */
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_NAMCO,
			&namco_interface
		}
	}
};
#endif


/*************************************************************/
/* Namco System II - ROM Declarations                        */
/*************************************************************/

#define NAMCOS2_BIOS_REGION\
	ROM_REGION(0x010000)		/* 64K RAM for MCU */ \
	ROM_LOAD("sys2mcpu.bin", 0x0000, 0x2000, 0xa342a97e )\
	ROM_LOAD("sys2c65c.bin", 0x8000, 0x8000, 0xa5b2a4ff )

/*************************************************************/
/* IF YOU ARE ADDING A NEW DRIVER PLEASE MAKE SURE YOU DO    */
/* NOT CHANGE THE SIZES OF THE MEMORY REGIONS, AS THESE ARE  */
/* ALWAYS SET TO THE CORRECT SYSTEM SIZE AS OPPOSED TO THE   */
/* SIZE OF THE INDIVIDUAL ROMS.                              */
/*************************************************************/


/*************************************************************/
/*                     ASSAULT (NAMCO)                       */
/*************************************************************/
ROM_START( assault_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "at2mp0b.bin" , 0x000000, 0x010000, 0x801f71c5 )
	ROM_LOAD_ODD(  "at2mp1b.bin" , 0x000000, 0x010000, 0x72312d4f )

	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "at1sp0.bin"  , 0x000000, 0x010000, 0x0de2a0da )
	ROM_LOAD_ODD(  "at1sp1.bin"  , 0x000000, 0x010000, 0x02d15fbe )

	ROM_REGION(0x030000)    /* 64K RAM + 128K banked ROM for the audio CPU, load the rom above the ram for paging */
	ROM_LOAD( "at1snd0.bin" , 0x00c000, 0x004000, 0x1d1ffe12 )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

//  NAMCOS2_BIOS_REGION
	ROM_REGION(0x010000)		/* 64K RAM for MCU */
	ROM_LOAD("sys2mcpu.bin" , 0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD("sys2"         , 0x008000, 0x008000, 0xe9f2922a )       /* This one has its own version ??? */

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "atobj0.bin"  , 0x000000, 0x020000, 0x22240076 )
	ROM_LOAD( "atobj1.bin"  , 0x080000, 0x020000, 0x2284a8e8 )
	ROM_LOAD( "atobj2.bin"  , 0x100000, 0x020000, 0x51425476 )
	ROM_LOAD( "atobj3.bin"  , 0x180000, 0x020000, 0x791f42ce )
	ROM_LOAD( "atobj4.bin"  , 0x200000, 0x020000, 0x4782e1b0 )
	ROM_LOAD( "atobj5.bin"  , 0x280000, 0x020000, 0xf5d158cf )
	ROM_LOAD( "atobj6.bin"  , 0x300000, 0x020000, 0x12f6a569 )
	ROM_LOAD( "atobj7.bin"  , 0x380000, 0x020000, 0x06a929f2 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "atchr0.bin"  , 0x000000, 0x020000, 0x6f8e968a )
	ROM_LOAD( "atchr1.bin"  , 0x040000, 0x020000, 0x88cf7cbe )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "atroz0.bin"  , 0x000000, 0x020000, 0x8c247a97 )
	ROM_LOAD( "atroz1.bin"  , 0x080000, 0x020000, 0xe44c475b )
	ROM_LOAD( "atroz2.bin"  , 0x100000, 0x020000, 0x770f377f )
	ROM_LOAD( "atroz3.bin"  , 0x180000, 0x020000, 0x01d93d0b )
	ROM_LOAD( "atroz4.bin"  , 0x200000, 0x020000, 0xf96feab5 )
	ROM_LOAD( "atroz5.bin"  , 0x280000, 0x020000, 0xda2f0d9e )
	ROM_LOAD( "atroz6.bin"  , 0x300000, 0x020000, 0x9089e477 )
	ROM_LOAD( "atroz7.bin"  , 0x380000, 0x020000, 0x62b2783a )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "at1dat0.bin" , 0x000000, 0x020000, 0x844890f4 )
	ROM_LOAD_ODD(  "at1dat1.bin" , 0x000000, 0x020000, 0x21715313 )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "atvoi1.bin"  , 0x000000, 0x080000, 0xd36a649e )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "atshape.bin" , 0x000000, 0x020000, 0xdfcad82b )


ROM_END


/*************************************************************/
/*                     ASSAULT (ATARI)                       */
/*************************************************************/
ROM_START( assaulta_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "at1_mp0.bin" , 0x000000, 0x010000, 0x2d3e5c8c )
	ROM_LOAD_ODD(  "at1_mp1.bin" , 0x000000, 0x010000, 0x851cec3a )

	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "at1_sp0.bin" , 0x000000, 0x010000, 0x0de2a0da )
	ROM_LOAD_ODD(  "at1_sp1.bin" , 0x000000, 0x010000, 0x02d15fbe )

	ROM_REGION(0x030000)    /* 64K RAM + 128K banked ROM for the audio CPU, load the rom above the ram for paging */
	ROM_LOAD( "at1_sd0.bin" , 0x00c000, 0x004000, 0x1d1ffe12 )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

//  NAMCOS2_BIOS_REGION
	ROM_REGION(0x010000)		/* 64K RAM for MCU */
	ROM_LOAD("sys2mcpu.bin" , 0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD("sys2"         , 0x008000, 0x008000, 0xe9f2922a )       /* This one has its own version ??? */

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "at1_ob0.bin" , 0x000000, 0x020000, 0x22240076 )
	ROM_LOAD( "at1_ob1.bin" , 0x080000, 0x020000, 0x2284a8e8 )
	ROM_LOAD( "at1_ob2.bin" , 0x100000, 0x020000, 0x51425476 )
	ROM_LOAD( "at1_ob3.bin" , 0x180000, 0x020000, 0x791f42ce )
	ROM_LOAD( "at1_ob4.bin" , 0x200000, 0x020000, 0x4782e1b0 )
	ROM_LOAD( "at1_ob5.bin" , 0x280000, 0x020000, 0xf5d158cf )
	ROM_LOAD( "at1_ob6.bin" , 0x300000, 0x020000, 0x12f6a569 )
	ROM_LOAD( "at1_ob7.bin" , 0x380000, 0x020000, 0x06a929f2 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "at1_ch0.bin" , 0x000000, 0x020000, 0x6f8e968a )
	ROM_LOAD( "at1_ch1.bin" , 0x080000, 0x020000, 0x88cf7cbe )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "at1_rz0.bin" , 0x000000, 0x020000, 0x8c247a97 )
	ROM_LOAD( "at1_rz1.bin" , 0x080000, 0x020000, 0xe44c475b )
	ROM_LOAD( "at1_rz2.bin" , 0x100000, 0x020000, 0x770f377f )
	ROM_LOAD( "at1_rz3.bin" , 0x180000, 0x020000, 0x01d93d0b )
	ROM_LOAD( "at1_rz4.bin" , 0x200000, 0x020000, 0xf96feab5 )
	ROM_LOAD( "at1_rz5.bin" , 0x280000, 0x020000, 0xda2f0d9e )
	ROM_LOAD( "at1_rz6.bin" , 0x300000, 0x020000, 0x9089e477 )
	ROM_LOAD( "at1_rz7.bin" , 0x380000, 0x020000, 0x62b2783a )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "at1_dt0.bin" , 0x000000, 0x020000, 0x844890f4 )
	ROM_LOAD_ODD(  "at1_dt1.bin" , 0x000000, 0x020000, 0x21715313 )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "at1_vo1.bin" , 0x000000, 0x080000, 0xd36a649e )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "at1_sha.bin" , 0x000000, 0x020000, 0xdfcad82b )


ROM_END


/*************************************************************/
/*                     ASSAULT PLUS (NAMCO)                  */
/*************************************************************/
ROM_START( assaultp_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "mpr0.bin"    , 0x000000, 0x010000, 0x97519f9f )
	ROM_LOAD_ODD(  "mpr1.bin"    , 0x000000, 0x010000, 0xc7f437c7 )

	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "spr0.bin"    , 0x000000, 0x020000, 0x6b9e5f31 )
	ROM_LOAD_ODD(  "spr1.bin"    , 0x000000, 0x020000, 0x9c349871 )

	ROM_REGION(0x030000)    /* 64K RAM + 128K banked ROM for the audio CPU, load the rom above the ram for paging */
	ROM_LOAD( "snd0.bin"    , 0x00c000, 0x004000, 0x1d1ffe12 )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

//  NAMCOS2_BIOS_REGION
	ROM_REGION(0x010000)		/* 64K RAM for MCU */
	ROM_LOAD("sys2mcpu.bin" , 0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD("sys2"         , 0x008000, 0x008000, 0xe9f2922a )       /* This one has its own version ??? */

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "obj0.bin"    , 0x000000, 0x020000, 0xc79192dd )
	ROM_LOAD( "obj1.bin"    , 0x080000, 0x020000, 0x9ca23ff1 )
	ROM_LOAD( "obj2.bin"    , 0x100000, 0x020000, 0x51425476 )
	ROM_LOAD( "obj3.bin"    , 0x180000, 0x020000, 0xefe8ef94 )
	ROM_LOAD( "obj4.bin"    , 0x200000, 0x020000, 0x4782e1b0 )
	ROM_LOAD( "obj5.bin"    , 0x280000, 0x020000, 0xf5d158cf )
	ROM_LOAD( "obj6.bin"    , 0x300000, 0x020000, 0x4c89aa54 )
	ROM_LOAD( "obj7.bin"    , 0x380000, 0x020000, 0x06a929f2 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "chr0.bin"    , 0x000000, 0x020000, 0x6f8e968a )
	ROM_LOAD( "chr1.bin"    , 0x080000, 0x020000, 0x88cf7cbe )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "roz0.bin"    , 0x000000, 0x020000, 0x8c247a97 )
	ROM_LOAD( "roz1.bin"    , 0x080000, 0x020000, 0xe44c475b )
	ROM_LOAD( "roz2.bin"    , 0x100000, 0x020000, 0x770f377f )
	ROM_LOAD( "roz3.bin"    , 0x180000, 0x020000, 0x01d93d0b )
	ROM_LOAD( "roz4.bin"    , 0x200000, 0x020000, 0xf96feab5 )
	ROM_LOAD( "roz5.bin"    , 0x280000, 0x020000, 0xda2f0d9e )
	ROM_LOAD( "roz6.bin"    , 0x300000, 0x020000, 0xf21a6d83 )
	ROM_LOAD( "roz7.bin"    , 0x380000, 0x020000, 0x92ec2ef4 )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "dat0.bin"    , 0x000000, 0x020000, 0x844890f4 )
	ROM_LOAD_ODD(  "dat1.bin"    , 0x000000, 0x020000, 0x21715313 )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "voice1.bin"  , 0x000000, 0x020000, 0x23146b11 )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "shape.bin"   , 0x000000, 0x010000, 0xcdaa2270 )


ROM_END


/*************************************************************/
/*                     BURNING FORCE                         */
/*************************************************************/
ROM_START( burnforc_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "bumpr0c.bin" , 0x000000, 0x020000, 0xcc5864c6 )
	ROM_LOAD_ODD(  "bumpr1c.bin" , 0x000000, 0x020000, 0x3e6b4b1b )

	ROM_REGION(0x020000)
	ROM_LOAD_EVEN( "bu1spr0.bin" , 0x000000, 0x010000, 0x17022a21 )
	ROM_LOAD_ODD(  "bu1spr1.bin" , 0x000000, 0x010000, 0x5255f8a5 )

	ROM_REGION(0x030000)    /* sound CPU */
	ROM_LOAD( "busnd0.bin"  , 0x00c000, 0x004000, 0xfabb1150 )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "buobj0.bin"  , 0x000000, 0x080000, 0x24c919a1 )
	ROM_LOAD( "buobj1.bin"  , 0x080000, 0x080000, 0x5bcb519b )
	ROM_LOAD( "buobj2.bin"  , 0x100000, 0x080000, 0x509dd5d0 )
	ROM_LOAD( "buobj3.bin"  , 0x180000, 0x080000, 0x270a161e )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "buchr0.bin"  , 0x000000, 0x020000, 0xc2109f73 )
	ROM_LOAD( "buchr1.bin"  , 0x080000, 0x020000, 0x67d6aa67 )
	ROM_LOAD( "buchr2.bin"  , 0x100000, 0x020000, 0x52846eff )
	ROM_LOAD( "buchr3.bin"  , 0x180000, 0x020000, 0xd1326d7f )
	ROM_LOAD( "buchr4.bin"  , 0x200000, 0x020000, 0x81a66286 )
	ROM_LOAD( "buchr5.bin"  , 0x280000, 0x020000, 0x629aa67f )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "buroz0.bin"  , 0x000000, 0x020000, 0x65fefc83 )
	ROM_LOAD( "buroz1.bin"  , 0x080000, 0x020000, 0x979580c2 )
	ROM_LOAD( "buroz2.bin"  , 0x100000, 0x020000, 0x548b6ad8 )
	ROM_LOAD( "buroz3.bin"  , 0x180000, 0x020000, 0xa633cea0 )
	ROM_LOAD( "buroz4.bin"  , 0x200000, 0x020000, 0x1b1f56a6 )
	ROM_LOAD( "buroz5.bin"  , 0x280000, 0x020000, 0x4b864b0e )
	ROM_LOAD( "buroz6.bin"  , 0x300000, 0x020000, 0x38bd25ba )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "bu1dat0.bin" , 0x000000, 0x020000, 0xe0a9d92f )
	ROM_LOAD_ODD(  "bu1dat1.bin" , 0x000000, 0x020000, 0x5fe54b73 )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "buvoi1.bin"  , 0x000000, 0x080000, 0x99d8a239 )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "bushape.bin" , 0x000000, 0x20000, 0x80a6b722 )

ROM_END


/*************************************************************/
/*                     COSMO GANG THE VIDEO                  */
/*************************************************************/
ROM_START( cosmogng_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "co1mpr0.bin" , 0x000000, 0x020000, 0xd1b4c8db )
	ROM_LOAD_ODD(  "co1mpr1.bin" , 0x000000, 0x020000, 0x2f391906 )

	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "co1spr0.bin" , 0x000000, 0x020000, 0xbba2c28f )
	ROM_LOAD_ODD(  "co1spr1.bin" , 0x000000, 0x020000, 0xc029b459 )

	ROM_REGION(0x030000)    /* 64K RAM + 128K banked ROM for the audio CPU, load the rom above the ram for paging */
	ROM_LOAD( "co1snd0.bin" , 0x00c000, 0x004000, 0x6bfa619f )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "co1obj0.bin" , 0x000000, 0x080000, 0x5df8ce0c )
	ROM_LOAD( "co1obj1.bin" , 0x080000, 0x080000, 0x3d152497 )
	ROM_LOAD( "co1obj2.bin" , 0x100000, 0x080000, 0x4e50b6ee )
	ROM_LOAD( "co1obj3.bin" , 0x180000, 0x080000, 0x7beed669 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "co1chr0.bin" , 0x000000, 0x080000, 0xee375b3e )
	ROM_LOAD( "co1chr1.bin" , 0x080000, 0x080000, 0x0149de65 )
	ROM_LOAD( "co1chr2.bin" , 0x100000, 0x080000, 0x93d565a0 )
	ROM_LOAD( "co1chr3.bin" , 0x180000, 0x080000, 0x4d971364 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "co1roz0.bin" , 0x000000, 0x080000, 0x2bea6951 )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN("co1dat0.bin"  , 0x000000, 0x020000, 0xb53da2ae )
	ROM_LOAD_ODD( "co1dat1.bin"  , 0x000000, 0x020000, 0xd21ad10b )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "co1voi1.bin" , 0x000000, 0x080000, 0xb5ba8f15 )
	ROM_LOAD_EVEN( "co1voi2.bin" , 0x100000, 0x080000, 0xb566b105 )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "co1sha0.bin" , 0x000000, 0x080000, 0x063a70cc )

ROM_END


/*************************************************************/
/*                     DRAGON SABER                          */
/*************************************************************/
ROM_START( dsaber_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "mpr0.bin"    , 0x000000, 0x020000, 0x45309ddc )
	ROM_LOAD_ODD(  "mpr1.bin"    , 0x000000, 0x020000, 0xcbfc4cba )

	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "spr0.bin"    , 0x000000, 0x010000, 0x013faf80 )
	ROM_LOAD_ODD(  "spr1.bin"    , 0x000000, 0x010000, 0xc36242bb )

	ROM_REGION(0x050000)    /* 64K RAM + 128K banked ROM for the audio CPU, load the rom above the ram for paging */
	ROM_LOAD( "snd0.bin"    , 0x00c000, 0x004000, 0xaf5b1ff8 )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )
	ROM_LOAD( "snd1.bin"    , 0x020000, 0x020000, 0xc4ca6f3f )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "obj0.bin"    , 0x000000, 0x080000, 0xf08c6648 )
	ROM_LOAD( "obj1.bin"    , 0x080000, 0x080000, 0x34e0810d )
	ROM_LOAD( "obj2.bin"    , 0x100000, 0x080000, 0xbccdabf3 )
	ROM_LOAD( "obj3.bin"    , 0x180000, 0x080000, 0x2a60a4b8 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "co1chr0.bin" , 0x000000, 0x080000, 0xc6058df6 )
	ROM_LOAD( "co1chr1.bin" , 0x080000, 0x080000, 0x67aaab36 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "roz0.bin"    , 0x000000, 0x080000, 0x32aab758 )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN("data0.bin"    , 0x000000, 0x020000, 0x3e53331f )
	ROM_LOAD_ODD( "data1.bin"    , 0x000000, 0x020000, 0xd5427f11 )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "voi1.bin"    , 0x000000, 0x080000, 0xdadf6a57 )
	ROM_LOAD_EVEN( "voi2.bin"    , 0x100000, 0x080000, 0x81078e01 )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "shape.bin"   , 0x000000, 0x080000, 0x698e7a3e )

ROM_END


/*************************************************************/
/*                     FINEST HOUR                           */
/*************************************************************/
ROM_START( finehour_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "fh1_mp0.bin" , 0x000000, 0x020000, 0x355d9119 )
	ROM_LOAD_ODD(  "fh1_mp1.bin" , 0x000000, 0x020000, 0x647eb621 )

	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "fh1_sp0.bin" , 0x000000, 0x020000, 0xaa6289e9 )
	ROM_LOAD_ODD(  "fh1_sp1.bin" , 0x000000, 0x020000, 0x8532d5c7 )

	ROM_REGION(0x030000)    /* 64K RAM + 128K banked ROM for the audio CPU, load the rom above the ram for paging */
	ROM_LOAD( "fh1_sd0.bin" , 0x00c000, 0x004000, 0x059a9cfd )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "fh1_ob0.bin" , 0x000000, 0x080000, 0xb1fd86f1 )
	ROM_LOAD( "fh1_ob1.bin" , 0x080000, 0x080000, 0x519c44ce )
	ROM_LOAD( "fh1_ob2.bin" , 0x100000, 0x080000, 0x9c5de4fa )
	ROM_LOAD( "fh1_ob3.bin" , 0x180000, 0x080000, 0x54d4edce )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "fh1_ch0.bin" , 0x000000, 0x040000, 0x516900d1 )
	ROM_LOAD( "fh1_ch1.bin" , 0x080000, 0x040000, 0x964d06bd )
	ROM_LOAD( "fh1_ch2.bin" , 0x100000, 0x040000, 0xfbb9449e )
	ROM_LOAD( "fh1_ch3.bin" , 0x180000, 0x040000, 0xc18eda8a )
	ROM_LOAD( "fh1_ch4.bin" , 0x200000, 0x040000, 0x80dd188a )
	ROM_LOAD( "fh1_ch5.bin" , 0x280000, 0x040000, 0x40969876 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "fh1_rz0.bin" , 0x000000, 0x020000, 0x6c96c5c1 )
	ROM_LOAD( "fh1_rz1.bin" , 0x080000, 0x020000, 0x44699eb9 )
	ROM_LOAD( "fh1_rz2.bin" , 0x100000, 0x020000, 0x5ec14abf )
	ROM_LOAD( "fh1_rz3.bin" , 0x180000, 0x020000, 0x9f5a91b2 )
	ROM_LOAD( "fh1_rz4.bin" , 0x200000, 0x020000, 0x0b4379e6 )
	ROM_LOAD( "fh1_rz5.bin" , 0x280000, 0x020000, 0xe034e560 )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN("fh1_dt0.bin"  , 0x000000, 0x020000, 0x2441c26f )
	ROM_LOAD_ODD( "fh1_dt1.bin"  , 0x000000, 0x020000, 0x48154deb )
	ROM_LOAD_EVEN("fh1_dt2.bin"  , 0x100000, 0x020000, 0x12453ba4 )
	ROM_LOAD_ODD( "fh1_dt3.bin"  , 0x100000, 0x020000, 0x50bab9da )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "fh1_vo1.bin" , 0x000000, 0x080000, 0x07560fc7 )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "fh1_sha.bin" , 0x000000, 0x040000, 0x15875eb0 )

ROM_END


/*************************************************************/
/*                     FOUR TRAX                             */
/*************************************************************/
ROM_START( fourtrax_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "fx4mpr0"     , 0x000000, 0x020000, 0xf147cd6b )
	ROM_LOAD_ODD(  "fx4mpr1"     , 0x000000, 0x020000, 0xd1138c85 )

	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "fx2sp0"      , 0x000000, 0x020000, 0x48548e78 )
	ROM_LOAD_ODD(  "fx2sp1"      , 0x000000, 0x020000, 0xd2861383 )

	ROM_REGION(0x030000)    /* 64K RAM + 128K banked ROM for the audio CPU, load the rom above the ram for paging */
	ROM_LOAD( "fx1sd0"      , 0x00c000, 0x004000, 0xacccc934 )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)    /* Sprites */
    /* No OBJ files in zip, not sure if they are missing ? */

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "fxchr0"      , 0x000000, 0x020000, 0x6658c1c3 )
	ROM_LOAD( "fxchr1"      , 0x080000, 0x020000, 0x3a888943 )
	ROM_LOAD( "fxchr2a"     , 0x100000, 0x020000, 0xa5d1ab10 )
	ROM_LOAD( "fxchr3"      , 0x180000, 0x020000, 0x47fa7e61 )
	ROM_LOAD( "fxchr4"      , 0x200000, 0x020000, 0xc720c5f5 )
	ROM_LOAD( "fxchr5"      , 0x280000, 0x020000, 0x9eacdbc8 )
	ROM_LOAD( "fxchr6"      , 0x300000, 0x020000, 0xc3dba42e )
	ROM_LOAD( "fxchr7"      , 0x380000, 0x020000, 0xc009f3ae )

	ROM_REGION(0x400000)    /* Tiles */
    /* No ROZ files in zip, not sure if they are missing ? */

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN("fxdat0"      , 0x000000, 0x040000, 0x63abf69b )
	ROM_LOAD_ODD( "fxdat1"      , 0x000000, 0x040000, 0x725bed14 )
	ROM_LOAD_EVEN("fxdat2"      , 0x100000, 0x040000, 0x71e4a5a0 )
	ROM_LOAD_ODD( "fxdat3"      , 0x100000, 0x040000, 0x605725f7 )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN("fxvoi1"      , 0x000000, 0x080000, 0x6173364f )


    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "fxsha"       , 0x000000, 0x020000, 0xf7aa4af7 )

ROM_END


/*************************************************************/
/*                         MARVEL LAND                       */
/*************************************************************/
ROM_START( marvland_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "mv1-mpr0.bin", 0x000000, 0x010000, 0x8369120f )
	ROM_LOAD_ODD(  "mv1-mpr1.bin", 0x000000, 0x010000, 0x6d5442cc )

	ROM_REGION(0x020000)
	ROM_LOAD_EVEN( "mv1-spr0.bin", 0x000000, 0x010000, 0xc3909925 )
	ROM_LOAD_ODD(  "mv1-spr1.bin", 0x000000, 0x010000, 0x1c5599f5 )

	ROM_REGION(0x30000)	    /* sound CPU */
	ROM_LOAD( "mv1-snd0.bin", 0x0c000, 0x04000, 0x51b8ccd7 )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "mv1-obj0.bin", 0x000000, 0x040000, 0x73a29361 )
	ROM_LOAD( "mv1-obj1.bin", 0x080000, 0x040000, 0xabbe4a99 )
	ROM_LOAD( "mv1-obj2.bin", 0x100000, 0x040000, 0x753659e0 )
	ROM_LOAD( "mv1-obj3.bin", 0x180000, 0x040000, 0xd1ce7339 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "mv1-chr0.bin", 0x000000, 0x040000, 0x1c7e8b4f )
	ROM_LOAD( "mv1-chr1.bin", 0x080000, 0x040000, 0x01e4cafd )
	ROM_LOAD( "mv1-chr2.bin", 0x100000, 0x040000, 0x198fcc6f )
	ROM_LOAD( "mv1-chr3.bin", 0x180000, 0x040000, 0xed6f22a5 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "mv1-roz0.bin", 0x000000, 0x020000, 0x7381a5a9 )
	ROM_LOAD( "mv1-roz1.bin", 0x080000, 0x020000, 0xe899482e )
	ROM_LOAD( "mv1-roz2.bin", 0x100000, 0x020000, 0xde141290 )
	ROM_LOAD( "mv1-roz3.bin", 0x180000, 0x020000, 0xe310324d )
	ROM_LOAD( "mv1-roz4.bin", 0x200000, 0x020000, 0x48ddc5a9 )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "mv1-dat0.bin", 0x000000, 0x020000, 0xe15f412e )
	ROM_LOAD_ODD(  "mv1-dat1.bin", 0x000000, 0x020000, 0x73e1545a )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "mv1-voi1.bin", 0x000000, 0x080000, 0xde5cac09 )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "mv1-sha.bin" , 0x000000, 0x040000, 0xa47db5d3 )

ROM_END


/*************************************************************/
/*                         METAL HAWK                        */
/*************************************************************/
ROM_START( metlhawk_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "mh2mp0c.11d" , 0x000000, 0x020000, 0xcd7dae6e )
	ROM_LOAD_ODD(  "mh2mp1c.13d" , 0x000000, 0x020000, 0xe52199fd )

	ROM_REGION(0x020000)
	ROM_LOAD_EVEN( "mh1sp0f.11k" , 0x000000, 0x010000, 0x2c141fea )
	ROM_LOAD_ODD(  "mh1sp1f.13k" , 0x000000, 0x010000, 0x8ccf98e0 )

	ROM_REGION(0x30000)	    /* sound CPU */
	ROM_LOAD( "mh1s0.7j"   , 0x0c000, 0x04000, 0x79e054cf )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "mhobj-0.5d"  , 0x000000, 0x040000, 0x52ae6620 )
	ROM_LOAD( "mhobj-1.5b"  , 0x080000, 0x040000, 0x2c2a1291 )
	ROM_LOAD( "mhobj-2.6d"  , 0x100000, 0x040000, 0x6221b927 )
	ROM_LOAD( "mhobj-3.6b"  , 0x180000, 0x040000, 0xfd09f2f1 )
	ROM_LOAD( "mhobj-4.5c"  , 0x200000, 0x040000, 0xe3590e1a )
	ROM_LOAD( "mhobj-5.5a"  , 0x280000, 0x040000, 0xb85c0d07 )
	ROM_LOAD( "mhobj-6.6c"  , 0x300000, 0x040000, 0x90c4523d )
	ROM_LOAD( "mhobj-7.6a"  , 0x380000, 0x040000, 0xf00edb39 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "mhchr-0.11n" , 0x000000, 0x020000, 0xe2da1b14 )
	ROM_LOAD( "mhchr-1.11p" , 0x080000, 0x020000, 0x023c78f9 )
	ROM_LOAD( "mhchr-2.11r" , 0x100000, 0x020000, 0xece47e91 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "mhr0z-0.2d"  , 0x000000, 0x040000, 0x30ade98f )
	ROM_LOAD( "mhr0z-1.2c"  , 0x080000, 0x040000, 0xa7fff42a )
	ROM_LOAD( "mhr0z-2.2b"  , 0x100000, 0x040000, 0x6abec820 )
	ROM_LOAD( "mhr0z-3.2a"  , 0x180000, 0x040000, 0xd53cec6d )
	ROM_LOAD( "mhr0z-4.3d"  , 0x200000, 0x040000, 0x922213e2 )
	ROM_LOAD( "mhr0z-5.3c"  , 0x280000, 0x040000, 0x78418a54 )
	ROM_LOAD( "mhr0z-6.3b"  , 0x300000, 0x040000, 0x6c74977e )
	ROM_LOAD( "mhr0z-7.3a"  , 0x380000, 0x040000, 0x68a19cbd )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "mh1d0.13s"   , 0x000000, 0x020000, 0x8b178ac7 )
	ROM_LOAD_ODD(  "mh1d1.13p"   , 0x000000, 0x020000, 0x10684fd6 )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "mhvoi-1.bin" , 0x000000, 0x080000, 0x2723d137 )
	ROM_LOAD_EVEN( "mhvoi-2.bin" , 0x100000, 0x080000, 0xdbc92d91 )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "mh1sha.7n"   , 0x000000, 0x020000, 0x6ac22294 )

ROM_END


/*************************************************************/
/*                         MIRAI NINJA                       */
/*************************************************************/
ROM_START( mirninja_rom )
	ROM_REGION(0x040000)
	ROM_LOAD_EVEN( "mn_mpr0e.bin", 0x000000, 0x010000, 0xfa75f977 )
	ROM_LOAD_ODD(  "mn_mpr1e.bin", 0x000000, 0x010000, 0x58ddd464 )

	ROM_REGION(0x020000)
	ROM_LOAD_EVEN( "mn1_spr0.bin", 0x000000, 0x010000, 0x3f1a17be )
	ROM_LOAD_ODD(  "mn1_spr1.bin", 0x000000, 0x010000, 0x2bc66f60 )

	ROM_REGION(0x30000)	    /* sound CPU */
	ROM_LOAD( "mn_snd0.bin" , 0x0c000, 0x04000, 0x00000000 )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

//  NAMCOS2_BIOS_REGION
	ROM_REGION(0x010000)		/* 64K RAM for MCU */
	ROM_LOAD("sys2mcpu.bin" , 0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD("mn65b.bin"    , 0x008000, 0x008000, 0xe9f2922a )       /* This one has its own version ??? */

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "mn_obj0.bin" , 0x000000, 0x020000, 0x6bd1e290 )
	ROM_LOAD( "mn_obj1.bin" , 0x080000, 0x020000, 0x5e8503be )
	ROM_LOAD( "mn_obj2.bin" , 0x100000, 0x020000, 0xa96d9b45 )
	ROM_LOAD( "mn_obj3.bin" , 0x180000, 0x020000, 0x0086ef8b )
	ROM_LOAD( "mn_obj4.bin" , 0x200000, 0x020000, 0xb3f48755 )
	ROM_LOAD( "mn_obj5.bin" , 0x280000, 0x020000, 0xc21e995b )
	ROM_LOAD( "mn_obj6.bin" , 0x300000, 0x020000, 0xa052c582 )
	ROM_LOAD( "mn_obj7.bin" , 0x380000, 0x020000, 0x1854c6f5 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "mn_chr0.bin" , 0x000000, 0x020000, 0x4f66df26 )
	ROM_LOAD( "mn_chr1.bin" , 0x080000, 0x020000, 0xf5de5ea7 )
	ROM_LOAD( "mn_chr2.bin" , 0x100000, 0x020000, 0x9ff61924 )
	ROM_LOAD( "mn_chr3.bin" , 0x180000, 0x020000, 0xba208bf5 )
	ROM_LOAD( "mn_chr4.bin" , 0x200000, 0x020000, 0x0ef00aff )
	ROM_LOAD( "mn_chr5.bin" , 0x280000, 0x020000, 0x4cd9d377 )
	ROM_LOAD( "mn_chr6.bin" , 0x300000, 0x020000, 0x114aca76 )
	ROM_LOAD( "mn_chr7.bin" , 0x380000, 0x020000, 0x2d5705d3 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "mn_roz0.bin" , 0x000000, 0x020000, 0x677a4f25 )
	ROM_LOAD( "mn_roz1.bin" , 0x080000, 0x020000, 0xf00ae572 )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "mn1_dat0.bin", 0x000000, 0x020000, 0x104bcca8 )
	ROM_LOAD_ODD(  "mn1_dat1.bin", 0x000000, 0x020000, 0xd2a918fb )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "mn_voi1.bin" , 0x000000, 0x080000, 0xebfad87b )
	ROM_LOAD_EVEN( "mn_voi2.bin" , 0x100000, 0x080000, 0x0db27c0b )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "mn_sha.bin"  , 0x000000, 0x020000, 0xc28af90f )

ROM_END


/*************************************************************/
/*                         ORDYNE                            */
/*************************************************************/
ROM_START( ordyne_rom )
	ROM_REGION(0x080000)
	ROM_LOAD_EVEN( "or1_mp0.bin" , 0x000000, 0x020000, 0xf5929ed3 )
	ROM_LOAD_ODD ( "or1_mp1.bin" , 0x000000, 0x020000, 0xc1c8c1e2 )

	ROM_REGION(0x020000)
	ROM_LOAD_EVEN( "or1_sp0.bin" , 0x000000, 0x010000, 0x01ef6638 )
	ROM_LOAD_ODD ( "or1_sp1.bin" , 0x000000, 0x010000, 0xb632adc3 )

	ROM_REGION(0x030000)
	ROM_LOAD( "or1_sd0.bin" , 0x00c000, 0x004000, 0xc41e5d22 )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "or1_ob0.bin" , 0x000000, 0x020000, 0x67b2b9e4 )
	ROM_LOAD( "or1_ob1.bin" , 0x080000, 0x020000, 0x8a54fa5e )
	ROM_LOAD( "or1_ob2.bin" , 0x100000, 0x020000, 0xa2c1cca0 )
	ROM_LOAD( "or1_ob3.bin" , 0x180000, 0x020000, 0xe0ad292c )
	ROM_LOAD( "or1_ob4.bin" , 0x200000, 0x020000, 0x7aefba59 )
	ROM_LOAD( "or1_ob5.bin" , 0x280000, 0x020000, 0xe4025be9 )
	ROM_LOAD( "or1_ob6.bin" , 0x300000, 0x020000, 0xe284c30c )
	ROM_LOAD( "or1_ob7.bin" , 0x380000, 0x020000, 0x262b7112 )

	ROM_REGION( 0x400000 )  /* Tiles */
	ROM_LOAD( "or1_ch0.bin" , 0x000000, 0x020000, 0xe7c47934 )
	ROM_LOAD( "or1_ch1.bin" , 0x080000, 0x020000, 0x874b332d )
	ROM_LOAD( "or1_ch3.bin" , 0x180000, 0x020000, 0x5471a834 )
	ROM_LOAD( "or1_ch5.bin" , 0x280000, 0x020000, 0xa7d3a296 )
	ROM_LOAD( "or1_ch6.bin" , 0x300000, 0x020000, 0x3adc09c8 )
	ROM_LOAD( "or1_ch7.bin" , 0x380000, 0x020000, 0xf050a152 )

	ROM_REGION( 0x400000 )  /* Tiles */
	ROM_LOAD( "or1_rz0.bin" , 0x000000, 0x020000, 0xc88a9e6b )
	ROM_LOAD( "or1_rz1.bin" , 0x080000, 0x020000, 0xc20cc749 )
	ROM_LOAD( "or1_rz2.bin" , 0x100000, 0x020000, 0x148c9866 )
	ROM_LOAD( "or1_rz3.bin" , 0x180000, 0x020000, 0x4e727b7e )
	ROM_LOAD( "or1_rz4.bin" , 0x200000, 0x020000, 0x17b04396 )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "or1_dt0.bin" , 0x000000, 0x020000, 0xde214f7a )
	ROM_LOAD_ODD(  "or1_dt1.bin" , 0x000000, 0x020000, 0x25e3e6c8 )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "or1_vo1.bin" , 0x000000, 0x080000, 0x369e0bca )
	ROM_LOAD_EVEN( "or1_vo2.bin" , 0x100000, 0x080000, 0x9f4cd7b5 )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "or1_sha.bin" , 0x000000, 0x020000, 0x7aec9dee )
ROM_END


/*************************************************************/
/*                         PHELIOS                           */
/*************************************************************/
ROM_START( phelios_rom )
	ROM_REGION(0x080000)
	ROM_LOAD_EVEN( "ps1mpr0.bin" , 0x000000, 0x020000, 0xbfbe96c6 )
	ROM_LOAD_ODD ( "ps1mpr1.bin" , 0x000000, 0x020000, 0xf5c0f883 )

	ROM_REGION(0x020000)
	ROM_LOAD_EVEN( "ps1spr0.bin" , 0x000000, 0x010000, 0xe9c6987e )
	ROM_LOAD_ODD ( "ps1spr1.bin" , 0x000000, 0x010000, 0x02b074fb )

	ROM_REGION(0x030000)
	ROM_LOAD( "ps1snd1.bin" , 0x00c000, 0x004000, 0xda694838 )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)    /* Sprites */
	ROM_LOAD( "psobj0.bin"  , 0x000000, 0x040000, 0xf323db2b )
	ROM_LOAD( "psobj1.bin"  , 0x080000, 0x040000, 0xfaf7c2f5 )
	ROM_LOAD( "psobj2.bin"  , 0x100000, 0x040000, 0x828178ba )
	ROM_LOAD( "psobj3.bin"  , 0x180000, 0x040000, 0xe84771c8 )
	ROM_LOAD( "psobj4.bin"  , 0x200000, 0x040000, 0x81ea86c6 )
	ROM_LOAD( "psobj5.bin"  , 0x280000, 0x040000, 0xaaebd51a )
	ROM_LOAD( "psobj6.bin"  , 0x300000, 0x040000, 0x032ea497 )
	ROM_LOAD( "psobj7.bin"  , 0x380000, 0x040000, 0xf6183b36 )

	ROM_REGION( 0x400000 )  /* Tiles */
	ROM_LOAD( "pschr0.bin"  , 0x000000, 0x020000, 0x668b6670 )
	ROM_LOAD( "pschr1.bin"  , 0x080000, 0x020000, 0x80c30742 )
	ROM_LOAD( "pschr2.bin"  , 0x100000, 0x020000, 0xf4e11bf7 )
	ROM_LOAD( "pschr3.bin"  , 0x180000, 0x020000, 0x97a93dc5 )
	ROM_LOAD( "pschr4.bin"  , 0x200000, 0x020000, 0x81d965bf )
	ROM_LOAD( "pschr5.bin"  , 0x280000, 0x020000, 0x8ca72d35 )
	ROM_LOAD( "pschr6.bin"  , 0x300000, 0x020000, 0xda3543a9 )

	ROM_REGION( 0x400000 )  /* Tiles */
	ROM_LOAD( "psroz0.bin"  , 0x000000, 0x020000, 0x68043d7e )
	ROM_LOAD( "psroz1.bin"  , 0x080000, 0x020000, 0x029802b4 )
	ROM_LOAD( "psroz2.bin"  , 0x100000, 0x020000, 0xbf0b76dc )
	ROM_LOAD( "psroz3.bin"  , 0x180000, 0x020000, 0x9c821455 )
	ROM_LOAD( "psroz4.bin"  , 0x200000, 0x020000, 0x63a39b7a )
	ROM_LOAD( "psroz5.bin"  , 0x280000, 0x020000, 0xfc5a99d0 )
	ROM_LOAD( "psroz6.bin"  , 0x300000, 0x020000, 0xa2a17587 )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "ps1dat0.bin" , 0x000000, 0x020000, 0xee4194b0 )
	ROM_LOAD_ODD(  "ps1dat1.bin" , 0x000000, 0x020000, 0x5b22d714 )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "psvoi-1.bin" , 0x000000, 0x080000, 0xf67376ed )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "psshape.bin" , 0x000000, 0x020000, 0x00000000 )
ROM_END


/*************************************************************/
/*                     ROLLING THUNDER 2                     */
/*************************************************************/
ROM_START( rthun2_rom )
	ROM_REGION(0x040000)		/* Main CPU */
	ROM_LOAD_EVEN( "mpr0.bin"    , 0x000000, 0x020000, 0xe09a3549 )
	ROM_LOAD_ODD(  "mpr1.bin"    , 0x000000, 0x020000, 0x09573bff )

	ROM_REGION(0x020000)	/* Sprite/tile engine CPU ? */
	ROM_LOAD_EVEN( "spr0.bin"    , 0x000000, 0x010000, 0x570d1a62 )
	ROM_LOAD_ODD(  "spr1.bin"    , 0x000000, 0x010000, 0x060eb393 )

	ROM_REGION(0x050000)	/* sound CPU */
	ROM_LOAD( "snd0.bin"    , 0x00c000, 0x004000, 0x55b7562a )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )
	ROM_LOAD( "snd1.bin"    , 0x030000, 0x020000, 0x00445a4f )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)	/* Sprites */
	ROM_LOAD( "obj0.bin"    , 0x000000, 0x080000, 0xe5cb82c1 )
	ROM_LOAD( "obj1.bin"    , 0x080000, 0x080000, 0x19ebe9fd )
	ROM_LOAD( "obj2.bin"    , 0x100000, 0x080000, 0x455c4a2f )
	ROM_LOAD( "obj3.bin"    , 0x180000, 0x080000, 0xfdcae8a9 )

	ROM_REGION(0x400000)	/* Tiles */
	ROM_LOAD( "chr0.bin"    , 0x000000, 0x080000, 0x6f0e9a68 )
	ROM_LOAD( "chr1.bin"    , 0x080000, 0x080000, 0x15e44adc )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "roz0.bin"    , 0x000000, 0x080000, 0x482d0554 )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "data0.bin"   , 0x000000, 0x020000, 0x0baf44ee )
	ROM_LOAD_ODD(  "data1.bin"   , 0x000000, 0x020000, 0x58a8daac )
	ROM_LOAD_EVEN( "data2.bin"   , 0x100000, 0x020000, 0x8e850a2a )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "voi1.bin"    , 0x000000, 0x080000, 0xe42027cd )
	ROM_LOAD_EVEN( "voi2.bin"    , 0x100000, 0x080000, 0x0c4c2b66 )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "shape.bin"   , 0x000000, 0x080000, 0xcf58fbbe )

ROM_END


/*************************************************************/
/*                     LEGEND OF THE WALKYRIE                */
/*************************************************************/
ROM_START( walkyrie_rom )
	ROM_REGION(0x040000)		/* Main CPU */
	ROM_LOAD_EVEN( "wd1mpr0.bin" , 0x000000, 0x020000, 0x94111a2e )
	ROM_LOAD_ODD(  "wd1mpr1.bin" , 0x000000, 0x020000, 0x57b5051c )

	ROM_REGION(0x020000)	/* Sprite/tile engine CPU ? */
	ROM_LOAD_EVEN( "wd1spr0.bin" , 0x000000, 0x010000, 0xb2398321 )
	ROM_LOAD_ODD(  "wd1spr1.bin" , 0x000000, 0x010000, 0x38dba897 )

	ROM_REGION(0x030000)	/* sound CPU */
	ROM_LOAD( "wd1snd0.bin" , 0x00c000, 0x004000, 0xd0fbf58b )
	ROM_CONTINUE(             0x010000, 0x01c000 )				/* Stops warning message */
	ROM_RELOAD(               0x010000, 0x020000 )

    NAMCOS2_BIOS_REGION

	ROM_REGION(0x400000)	/* Sprites */
	ROM_LOAD( "wdobj0.bin"  , 0x000000, 0x040000, 0xe8089451 )
	ROM_LOAD( "wdobj1.bin"  , 0x080000, 0x040000, 0x7ca65666 )
	ROM_LOAD( "wdobj2.bin"  , 0x100000, 0x040000, 0x7c159407 )
	ROM_LOAD( "wdobj3.bin"  , 0x180000, 0x040000, 0x649f8760 )
	ROM_LOAD( "wdobj4.bin"  , 0x200000, 0x040000, 0x7ca39ae7 )
	ROM_LOAD( "wdobj5.bin"  , 0x280000, 0x040000, 0x9ead2444 )
	ROM_LOAD( "wdobj6.bin"  , 0x300000, 0x040000, 0x9fa2ea21 )
	ROM_LOAD( "wdobj7.bin"  , 0x380000, 0x040000, 0x66e07a36 )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "wdchr0.bin"  , 0x000000, 0x020000, 0xdebb0116 )
	ROM_LOAD( "wdchr1.bin"  , 0x080000, 0x020000, 0x8a1431e8 )
	ROM_LOAD( "wdchr2.bin"  , 0x100000, 0x020000, 0x62f75f69 )
	ROM_LOAD( "wdchr3.bin"  , 0x180000, 0x020000, 0xcc43bbe7 )
	ROM_LOAD( "wdchr4.bin"  , 0x200000, 0x020000, 0x2f73d05e )
	ROM_LOAD( "wdchr5.bin"  , 0x280000, 0x020000, 0xb632b2ec )

	ROM_REGION(0x400000)    /* Tiles */
	ROM_LOAD( "wdroz0.bin"  , 0x000000, 0x020000, 0xf776bf66 )
	ROM_LOAD( "wdroz1.bin"  , 0x080000, 0x020000, 0xc1a345c3 )
	ROM_LOAD( "wdroz2.bin"  , 0x100000, 0x020000, 0x28ffb44a )
	ROM_LOAD( "wdroz3.bin"  , 0x180000, 0x020000, 0x7e77b46d )

	ROM_REGION(0x200000)    /* Shared data roms */
	ROM_LOAD_EVEN( "wd1dat0.bin" , 0x000000, 0x020000, 0xea209f48 )
	ROM_LOAD_ODD(  "wd1dat1.bin" , 0x000000, 0x020000, 0x04b48ada )

	ROM_REGION(0x200000)    /* Sound voices 1M x 12 bit */
	ROM_LOAD_EVEN( "wd1voi1.bin" , 0x000000, 0x040000, 0xf1ace193 )
	ROM_LOAD_EVEN( "wd1voi2.bin" , 0x000000, 0x020000, 0xe95c5cf3 )

    ROM_REGION(0x080000)    /* Shape memory */
	ROM_LOAD( "wdshape.bin" , 0x000000, 0x20000, 0x3b5e0249 )

ROM_END




static void namcos2_decode(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM;

	// CPU4 Patches
	RAM = Machine->memory_region[NAMCOS2_CPU4];
	RAM[0x81f9]=0x9d;
	RAM[0x81fa]=0x9d;							// Stop waiting for A/D conversion complete
	RAM[0x823f]=0x9d;
	RAM[0x8240]=0x9d;							// Stop waiting for A/D conversion complete

}


/*************************************************************/
/* Set gametype so that the protection code knows how to     */
/* emulate the correct responses                             */
/*                                                           */
/* 0x4e71 == 68000 NOP                                       */
/* 0x4e75 == 68000 RTS                                       */
/*                                                           */
/*************************************************************/

void assault_init(void)  {  namcos2_gametype=NAMCOS2_ASSAULT; }
void assaulta_init(void) {  namcos2_gametype=NAMCOS2_ASSAULTA; }
void assaultp_init(void) {  namcos2_gametype=NAMCOS2_ASSAULT_PLUS; }

void burnforc_init(void)
{
	unsigned char *RAM=Machine->memory_region[NAMCOS2_CPU1];
    namcos2_gametype=NAMCOS2_BURNING_FORCE;
    WRITE_WORD( &RAM[0x001e18], 0x4e75 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x003a9c], 0x4e75 );   // Patch $d00000 checks
}

void cosmogng_init(void)
{
	unsigned char *RAM=Machine->memory_region[NAMCOS2_CPU1];
    namcos2_gametype=NAMCOS2_COSMO_GANG;
    WRITE_WORD( &RAM[0x0034d2], 0x4e75 );   // Patch $d00000 checks
}

void dsaber_init(void)
{
	unsigned char *RAM=Machine->memory_region[NAMCOS2_CPU1];
    namcos2_gametype=NAMCOS2_DRAGON_SABER;
    WRITE_WORD( &RAM[0x001172], 0x4e75 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x00119c], 0x4e75 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x002160], 0x4e75 );   // Patch $d00000 checks
}

void finehour_init(void)
{
	unsigned char *RAM=Machine->memory_region[NAMCOS2_CPU1];
    namcos2_gametype=NAMCOS2_FINEST_HOUR;
    WRITE_WORD( &RAM[0x001892], 0x4e75 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x003ac0], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x00467c], 0x4e71 );   // Patch $d00000 checks
}

void fourtrax_init(void) {  namcos2_gametype=NAMCOS2_FOUR_TRAX; }

void marvland_init(void)
{
	unsigned char *RAM=Machine->memory_region[NAMCOS2_CPU1];
    namcos2_gametype=NAMCOS2_MARVEL_LAND;
    WRITE_WORD( &RAM[0x000f24], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x001fb2], 0x4e75 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x0048b6], 0x4e75 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x0048d2], 0x4e75 );   // Patch $d00000 checks
}

void metlhawk_init(void) {  namcos2_gametype=NAMCOS2_METAL_HAWK; }

void mirninja_init(void)
{
	unsigned char *RAM=Machine->memory_region[NAMCOS2_CPU1];
    namcos2_gametype=NAMCOS2_MIRAI_NINJA;
    WRITE_WORD( &RAM[0x00052a], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x01de68], 0x4e75 );   // Patch $d00000 checks
}

void ordyne_init(void)
{
	unsigned char *RAM=Machine->memory_region[NAMCOS2_CPU1];
    namcos2_gametype=NAMCOS2_ORDYNE;
    WRITE_WORD( &RAM[0x0025a4], 0x4e75 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x0025c2], 0x4e75 );   // Patch $d00000 checks
}

void phelios_init(void)
{
	unsigned char *RAM=Machine->memory_region[NAMCOS2_CPU1];
    namcos2_gametype=NAMCOS2_PHELIOS;
    WRITE_WORD( &RAM[0x0011ea], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x0011ec], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x0011f6], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x0011f8], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x00120a], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x00120c], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x001216], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x001218], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x001222], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x001224], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x00122e], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x001230], 0x4e71 );   // Patch $d00000 checks
    WRITE_WORD( &RAM[0x02607a], 0x4e75 );   // Patch $d00000 checks
}

void rthun2_init(void)
{
	unsigned char *RAM=Machine->memory_region[NAMCOS2_CPU1];
    namcos2_gametype=NAMCOS2_ROLLING_THUNDER_2;
    WRITE_WORD( &RAM[0x0042b0], 0x4e71 );   // Patch $d00000 checks
}

void walkyrie_init(void) {  namcos2_gametype=NAMCOS2_WALKYRIE; }



#define NAMCO_SYSTEM2_DRIVER(DRVNAME,DRVCLONE,ENTRYNAME,ENTRYDETAIL,ENTRYDATE,ENTRYCOMPANY,ENTRYROTATE) \
    struct GameDriver DRVNAME##_driver =\
    {\
	    __FILE__,\
    	DRVCLONE,\
    	ENTRYNAME,\
    	ENTRYDETAIL,\
      	ENTRYDATE,\
	    ENTRYCOMPANY,\
    	NAMCOS2_CREDITS,\
    	GAME_NOT_WORKING,\
    	&machine_driver,\
    	DRVNAME##_init,\
        DRVNAME##_rom,\
        namcos2_decode, 0,\
        0,\
        0,\
        namcos2_input_ports,\
        0, 0, 0,\
        ENTRYROTATE,\
        namcos2_hiload,namcos2_hisave\
    };


NAMCO_SYSTEM2_DRIVER(assault , 0               , "assault" ,"Assault"               ,"1988","Namco",ORIENTATION_ROTATE_90)
NAMCO_SYSTEM2_DRIVER(assaulta, &assault_driver , "assaulta","Assault (Set2)"        ,"1988","Namco",ORIENTATION_ROTATE_90)
NAMCO_SYSTEM2_DRIVER(assaultp, &assault_driver , "assaultp","Assault Plus"          ,"1988","Namco",ORIENTATION_ROTATE_90)
NAMCO_SYSTEM2_DRIVER(metlhawk, 0               , "metlhawk","Metal Hawk"            ,"1988","Namco",ORIENTATION_ROTATE_90)
NAMCO_SYSTEM2_DRIVER(mirninja, 0               , "mirninja","Mirai Ninja"           ,"1988","Namco",ORIENTATION_DEFAULT)
NAMCO_SYSTEM2_DRIVER(ordyne,   0               , "ordyne"  ,"Ordyne"                ,"1988","Namco",ORIENTATION_DEFAULT)
NAMCO_SYSTEM2_DRIVER(phelios , 0               , "phelios" ,"Phelios"               ,"1988","Namco",ORIENTATION_ROTATE_90)
NAMCO_SYSTEM2_DRIVER(burnforc, 0               , "burnforc","Burning Force"         ,"1989","Namco",ORIENTATION_DEFAULT)
NAMCO_SYSTEM2_DRIVER(finehour, 0               , "finehour","Finest Hour"           ,"1989","Namco",ORIENTATION_DEFAULT)
NAMCO_SYSTEM2_DRIVER(walkyrie, 0               , "walkyrie","Legend of the Walkyrie","1989","Namco",ORIENTATION_ROTATE_90)
NAMCO_SYSTEM2_DRIVER(dsaber,   0               , "dsaber"  ,"Dragon Saber"          ,"1990","Namco",ORIENTATION_ROTATE_90)
NAMCO_SYSTEM2_DRIVER(marvland, 0               , "marvland","Marvel Land"           ,"1990","Namco",ORIENTATION_DEFAULT)
NAMCO_SYSTEM2_DRIVER(rthun2  , 0               , "rthun2"  ,"Rolling Thunder 2"     ,"1991","Namco",ORIENTATION_DEFAULT)
NAMCO_SYSTEM2_DRIVER(cosmogng, 0               , "cosmogng","Cosmo Gang the Video"  ,"1992","Namco",ORIENTATION_ROTATE_90)
NAMCO_SYSTEM2_DRIVER(fourtrax, 0               , "fourtrax","Four Trax"             ,"19??","Namco",ORIENTATION_DEFAULT)

