/***************************************************************************

Namco System II driver by K.Wilkins  (Jun1998, Oct1999)
Email: kwns2@dysfunction.demon.co.uk


TODO:
	General:
	- I don't think the implementation of shadows is 100% correct
	- ROZ layer priority/banking is hacked
	- road colors are wrong
	- sprite-tilemap priority orthagonality needed

	Game-Specific:
	- valkyrie gives ADSMISS error on startup
	- dsaber has garbage ROZ layer spinning in the background of lava level (see attract mode)
	- suzuka8h has bogus palette (missing data ROM)
	- Final Lap (1,2,3): tilemap scroll, I/O, road/sprite handling glitches

The Namco System II board is a 5 ( only 4 are emulated ) CPU system. The
complete system consists of two boards: CPU + GRAPHICS. It contains a large
number of custom ASICs to perform graphics operations, there is no
documentation available for these parts.

The system is extremely powerful and flexible. A standard CPU board is coupled
with a number of different graphics boards to produce a system.



CPU Board details
=================

CPU BOARD - Master/Slave CPU, Sound CPU, I/O CPU, Serial I/F CPU
			Text/Scrolling layer generation and video pixel generator.
			Sound Generation.

CPU1 - Master CPU				(68K)
CPU2 - Slave CPU				(68K)
CPU3 - Sound/IO engine			(6809)
CPU4 - IO Microcontroller		(63705) Dips/input ports
CPU5 - Serial I/F Controller	(??? - Not emulated)

The 4 CPU's are all connected via a central 2KByte dual port SRAM. The two
68000s are on one side and the 6809/63705 are on the other side.

Each 68000 has its own private bus area AND a common shared area between the
two devices, which is where the video ram/dual port/Sprite Generation etc
logic sits.

So far only 1 CPU board variant has been identified, unlike the GFX board...

All sound circuitry is contained on the CPU board, it consists of:
	YM2151
	C140 (24 Channel Stereo PCM Sample player)

The CPU board also contains the frame timing and video image generation
circuitry along with text/scroll planes and the palette generator. The system
has 8192 pens of which 4096+2048 are displayable at any given time. These
pens refernce a 24 bit colour lookup table (R8:G8:B8).

The text/tile plane generator has the following capabilities:

2 x Static tile planes (36x28 tiles)
4 x Scolling tile planes (64x64 tiles)

Each plane has its own colour index (8 total) that is used alongside the
pen number to be looked up in the pen index and generate a 24 bit pixel. Each
plane also has its own priority level.

The video image generator receives a pixel stream from the graphics board
which contains:

		PEN NUMBER
		COLOUR BANK
		PIXEL PRIORITY

This stream is then combined with the stream from the text plane pixel
generator with the highest priority pixel being displayed on screen.


Graphics Board details
======================

There are several variants of graphics board with unique capabilities
separate memory map definition. The PCB outputs a pixel stream to the
main PCB board via one of the system connectors.


ROZ(A):    1 256x256 ROZ plane composed of 8x8 tiles
ROZ(B):    2 ROZ planes, composed of 16x16 tiles (same as Namco NB2)
Sprite(A): 128 Sprites displayable, but 16 banks of 128 sprites
Sprite(B): (same as Namco NB2)
Roadway:   tiles and road attributes in RAM

                          ROZ  Sprites  Roadway
Standard Namco System 2   (A)  (A)      n/a
Final Lap (1/2/3)         n/a  (A)      yes
Metal Hawk                (B)  (A)      no
Steel Gunner 2            n/a  (B)      no
Suzuka (1/2)              n/a  (B)      yes
Lucky&Wild                (B)  (B)      yes


Memory Map
==========

The Dual 68000 Shared memory map area is shown below, this is taken from the memory
decoding pal from the Cosmo Gang board.


#############################################################
#															#
#		MASTER 68000 PRIVATE MEMORY AREA (MAIN PCB) 		#
#															#
#############################################################
# Function						   Address		  R/W  DATA #
#############################################################
Program ROM 					   000000-03FFFF  R    D00-D15

Program RAM 					   100000-10FFFF  R/W  D00-D15

EEPROM							   180000-183FFF  R/W  D00-D07

Interrupt Controller C148		   1C0000-1FFFFF  R/W  D00-D02
	????????					   1C0XXX
	????????					   1C2XXX
	????????					   1C4XXX
	Master/Slave IRQ level		   1C6XXX			   D00-D02
	EXIRQ level 				   1C8XXX			   D00-D02
	POSIRQ level				   1CAXXX			   D00-D02
	SCIRQ level 				   1CCXXX			   D00-D02
	VBLANK IRQ level			   1CEXXX			   D00-D02
	????????					   1D0XXX
	Acknowlegde Master/Slave IRQ   1D6XXX
	Acknowledge EXIRQ			   1D8XXX
	Acknowledge POSIRQ			   1DAXXX
	Acknowledge SCIRQ			   1DCXXX
	Acknowledge VBLANK IRQ		   1DEXXX
	EEPROM Ready status 		   1E0XXX		  R    D01
	Sound CPU Reset control 	   1E2XXX			W  D01
	Slave 68000 & IO CPU Reset	   1E4XXX			W  D01
	Watchdog reset kicker		   1E6XXX			W



#############################################################
#															#
#		 SLAVE 68000 PRIVATE MEMORY AREA (MAIN PCB) 		#
#															#
#############################################################
# Function						   Address		  R/W  DATA #
#############################################################
Program ROM 					   000000-03FFFF  R    D00-D15

Program RAM 					   100000-10FFFF  R/W  D00-D15

Interrupt Controller C148		   1C0000-1FFFFF  R/W  D00-D02
	????????					   1C0XXX
	????????					   1C2XXX
	????????					   1C4XXX
	Master/Slave IRQ level		   1C6XXX			   D00-D02
	EXIRQ level 				   1C8XXX			   D00-D02
	POSIRQ level				   1CAXXX			   D00-D02
	SCIRQ level 				   1CCXXX			   D00-D02
	VBLANK IRQ level			   1CEXXX			   D00-D02
	????????					   1D0XXX
	Acknowlegde Master/Slave IRQ   1D6XXX
	Acknowledge EXIRQ			   1D8XXX
	Acknowledge POSIRQ			   1DAXXX
	Acknowledge SCIRQ			   1DCXXX
	Acknowledge VBLANK IRQ		   1DEXXX
	Watchdog reset kicker		   1E6XXX			W




#############################################################
#															#
#			SHARED 68000 MEMORY AREA (MAIN PCB) 			#
#															#
#############################################################
# Function						   Address		  R/W  DATA #
#############################################################
Data ROMS 0-1					   200000-2FFFFF  R    D00-D15

Data ROMS 2-3					   300000-3FFFFF  R    D00-D15

Screen memory for text planes	   400000-41FFFF  R/W  D00-D15

Screen control registers		   420000-43FFFF  R/W  D00-D15

	Scroll plane 0 - X offset	   42XX02			W  D00-D11
	Scroll plane 0 - X flip 	   42XX02			W  D15

	??????						   42XX04			W  D14-D15

	Scroll plane 0 - Y offset	   42XX06			W  D00-D11
	Scroll plane 0 - Y flip 	   42XX06			W  D15

	??????						   42XX08			W  D14-D15

	Scroll plane 1 - X offset	   42XX0A			W  D00-D11
	Scroll plane 1 - X flip 	   42XX0A			W  D15

	??????						   42XX0C			W  D14-D15

	Scroll plane 1 - Y offset	   42XX0E			W  D00-D11
	Scroll plane 1 - Y flip 	   42XX0E			W  D15

	??????						   42XX10			W  D14-D15

	Scroll plane 2 - X offset	   42XX12			W  D00-D11
	Scroll plane 2 - X flip 	   42XX12			W  D15

	??????						   42XX14			W  D14-D15

	Scroll plane 2 - Y offset	   42XX16			W  D00-D11
	Scroll plane 2 - Y flip 	   42XX16			W  D15

	??????						   42XX18			W  D14-D15

	Scroll plane 3 - X offset	   42XX1A			W  D00-D11
	Scroll plane 3 - X flip 	   42XX1A			W  D15

	??????						   42XX1C			W  D14-D15

	Scroll plane 3 - Y offset	   42XX1E			W  D00-D11
	Scroll plane 3 - Y flip 	   42XX1E			W  D15

	Scroll plane 0 priority 	   42XX20			W  D00-D02
	Scroll plane 1 priority 	   42XX22			W  D00-D02
	Scroll plane 2 priority 	   42XX24			W  D00-D02
	Scroll plane 3 priority 	   42XX26			W  D00-D02
	Text plane 0 priority		   42XX28			W  D00-D02
	Text plane 1 priority		   42XX2A			W  D00-D02

	Scroll plane 0 colour		   42XX30			W  D00-D03
	Scroll plane 1 colour		   42XX32			W  D00-D03
	Scroll plane 2 colour		   42XX34			W  D00-D03
	Scroll plane 3 colour		   42XX36			W  D00-D03
	Text plane 0 colour 		   42XX38			W  D00-D03
	Text plane 1 colour 		   42XX3A			W  D00-D03

Screen palette control/data 	   440000-45FFFF  R/W  D00-D15
	RED   ROZ/Sprite pens 8x256    440000-440FFF
	GREEN						   441000-441FFF
	BLUE						   442000-442FFF
	Control registers			   443000-44300F  R/W  D00-D15
	RED   ROZ/Sprite pens 8x256    444000-444FFF
	GREEN						   445000-445FFF
	BLUE						   446000-446FFF
																   447000-447FFF
	RED   Text plane pens 8x256    448000-448FFF
	GREEN						   449000-449FFF
	BLUE						   44A000-44AFFF
																   44B000-44BFFF
	RED   Unused pens 8x256 	   44C000-44CFFF
	GREEN						   44D000-44DFFF
	BLUE						   44E000-44EFFF

Dual port memory				   460000-47FFFF  R/W  D00-D07

Serial comms processor			   480000-49FFFF

Serial comms processor - Data	   4A0000-4BFFFF



#############################################################
#															#
#			SHARED 68000 MEMORY AREA (GFX PCB)				#
#			  (STANDARD NAMCO SYSTEM 2 BOARD)				#
#															#
#############################################################
# Function						   Address		  R/W  DATA #
#############################################################
Sprite RAM - 16 banks x 128 spr.   C00000-C03FFF  R/W  D00-D15

Sprite bank select				   C40000			W  D00-D03
Rotate colour bank select							W  D08-D11
Rotate priority level								W  D12-D14

Rotate/Zoom RAM (ROZ)			   C80000-CBFFFF  R/W  D00-D15

Rotate/Zoom - Down dy	  (8:8)    CC0000		  R/W  D00-D15
Rotate/Zoom - Right dy	  (8.8)    CC0002		  R/W  D00-D15
Rotate/Zoom - Down dx	  (8.8)    CC0004		  R/W  D00-D15
Rotate/Zoom - Right dx	  (8.8)    CC0006		  R/W  D00-D15
Rotate/Zoom - Start Ypos  (12.4)   CC0008		  R/W  D00-D15
Rotate/Zoom - Start Xpos  (12.4)   CC000A		  R/W  D00-D15
Rotate/Zoom control 			   CC000E		  R/W  D00-D15

Key generator/Security device	   D00000-D0000F  R/W  D00-D15



#############################################################
#															#
#			SHARED 68000 MEMORY AREA (GFX PCB)				#
#			(METAL HAWK PCB - DUAL ROZ PLANES)				#
#															#
#############################################################
# Function						   Address		  R/W  DATA #
#############################################################
Sprite RAM - 16 banks x 128 spr.   C00000-C03FFF  R/W  D00-D15

Rotate/Zoom RAM (ROZ1)			   C40000-C47FFF  R/W  D00-D15

Rotate/Zoom RAM (ROZ2)			   C48000-C4FFFF  R/W  D00-D15

Rotate/Zoom1 - Down dy	   (8:8)   D00000		  R/W  D00-D15
Rotate/Zoom1 - Right dy    (8.8)   D00002		  R/W  D00-D15
Rotate/Zoom1 - Down dx	   (8.8)   D00004		  R/W  D00-D15
Rotate/Zoom1 - Right dx    (8.8)   D00006		  R/W  D00-D15
Rotate/Zoom1 - Start Ypos  (12.4)  D00008		  R/W  D00-D15
Rotate/Zoom1 - Start Xpos  (12.4)  D0000A		  R/W  D00-D15
Rotate/Zoom1 - control			   D0000E		  R/W  D00-D15

Rotate/Zoom2 - Down dy	   (8:8)   D00010		  R/W  D00-D15
Rotate/Zoom2 - Right dy    (8.8)   D00012		  R/W  D00-D15
Rotate/Zoom2 - Down dx	   (8.8)   D00014		  R/W  D00-D15
Rotate/Zoom2 - Right dx    (8.8)   D00016		  R/W  D00-D15
Rotate/Zoom2 - Start Ypos  (12.4)  D00018		  R/W  D00-D15
Rotate/Zoom2 - Start Xpos  (12.4)  D0001A		  R/W  D00-D15
Rotate/Zoom2 - control			   D0001E		  R/W  D00-D15

Sprite bank select ?			   E00000			W  D00-D15


#############################################################
#															#
#			SHARED 68000 MEMORY AREA (GFX PCB)				#
#			(FINAL LAP PCB) 								#
#															#
#############################################################
# Function						   Address		  R/W  DATA #
#############################################################
Sprite RAM - ?? banks x ??? spr.   800000-80FFFF  R/W  D00-D15
Sprite bank select ?			   840000			W  D00-D15
Road RAM for tile layout		   880000-88FFFF  R/W  D00-D15
Road RAM for tiles gfx data 	   890000-897FFF  R/W  D00-D15
Road Generator controls 		   89F000-89FFFF  R/W  D00-D15
Key generator/Security device	   A00000-A0000F  R/W  D00-D15



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


C148pin 	Master CPU		Slave CPU
-------------------------------------
YBNK		VBLANK			VBLANK
IRQ4		SCIRQ			SCIRQ		(Serial comms IC Interrupt)
IRQ3		POSIRQ			POSIRQ		(Comes from C116, pixel generator, Position interrup ?? line based ??)
IRQ2		EXIRQ			EXIRQ		(Goes to video board but does not appear to be connected)
IRQ1		SCPUIRQ 		MCPUIRQ 	(Master/Slave interrupts)

OP0 		SSRES						(Sound CPU reset - 6809 only)
OP1
OP2

IP0 		EEPROM BUSY
IP1
IP2



Protection
----------
The Chip at $d00000 seems to be heavily involved in protection, some games lock
or reset if it doesnt return the correct values.
rthun2 is sprinkled with reads to $d00006 which look like they are being used as
random numbers. rthun2 also checks the response value after a number is written.
Device takes clock and vblank. Only output is reset.

This chip is based on the graphics board.


Palette
-------

0x800 (2048) colours

Ram test does:

$440000-$442fff 	Object ???
$444000-$446fff 	Char   ???
$448000-$44afff 	Roz    ???
$44c000-$44efff

$448000-$4487ff 	Red??
$448800-$448fff 	Green??
$449000-$4497ff 	Blue??


Steel Gunner 2
--------------
Again this board has a different graphics layout, also the protection checks
are done at $a00000 as opposed to $d00000 on a standard board. Similar
$a00000 checks have been seen on the Final Lap boards.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcos2.h"
#include "cpu/m6809/m6809.h"
#include "namcoic.h"
#include "artwork.h"


/*************************************************************/
/* 68000/6809/63705 Shared memory area - DUAL PORT Memory	 */
/*************************************************************/

static data8_t *namcos2_dpram;	/* 2Kx8 */

static void
GollyGhostUpdateLED_c4( int data )
{
	static char zip100[32];
	static char zip10[32];
	int i = 0;
	for(;;)
	{
		artwork_show(zip100,i);
		artwork_show(zip10,i);
		if( i ) return;
		sprintf( zip100, "zip100_%d",data>>4);
		sprintf( zip10,  "zip10_%d", data&0xf);
		i=1;
	}
}

static void
GollyGhostUpdateLED_c6( int data )
{
	static char zip1[32];
	static char time10[32];
	int i = 0;
	for(;;)
	{
		artwork_show(zip1,i);
		artwork_show(time10,i);
		if( i ) return;
		sprintf( zip1,   "zip1_%d",  data>>4);
		sprintf( time10, "time10_%d",data&0xf);
		i=1;
	}
}

static void
GollyGhostUpdateLED_c8( int data )
{
	static char time1[32];
	static char zap100[32];
	int i = 0;
	for(;;)
	{
		artwork_show(time1,i);
		artwork_show(zap100,i);
		if( i ) return;
		sprintf( time1,  "time1_%d", data>>4);
		sprintf( zap100, "zap100_%d",data&0xf);
		i=1;
	}
}

static void
GollyGhostUpdateLED_ca( int data )
{
	static char zap10[32];
	static char zap1[32];
	int i = 0;
	for(;;)
	{
		artwork_show(zap10,i);
		artwork_show(zap1,i);
		if( i ) return;
		sprintf( zap10,  "zap10_%d", data>>4);
		sprintf( zap1,   "zap1_%d",  data&0xf);
		i=1;
	}
}

static void
GollyGhostUpdateDiorama_c0( int data )
{
	if( data&0x80 )
	{
		artwork_show("fulldark",0 );
		artwork_show("dollhouse",1); /* diorama is lit up */

		/* dollhouse controller; solenoids control physical components */
		artwork_show("toybox",      data&0x01 );
		artwork_show("bathroom",    data&0x02 );
		artwork_show("bureau",      data&0x04 );
		artwork_show("refrigerator",data&0x08 );
		artwork_show("porch",       data&0x10 );
		/* data&0x20 : player#1 (ZIP) force feedback
		 * data&0x40 : player#2 (ZAP) force feedback
		 */
	}
	else
	{
		artwork_show("fulldark",1 );
		artwork_show("dollhouse",0);
		artwork_show("toybox",0);
		artwork_show("bathroom",0);
		artwork_show("bureau",0);
		artwork_show("refrigerator",0);
		artwork_show("porch",0);
	}
}

static READ16_HANDLER( namcos2_68k_dpram_word_r )
{
	return namcos2_dpram[offset];
}

static WRITE16_HANDLER( namcos2_68k_dpram_word_w )
{
	if( ACCESSING_LSB )
	{
		namcos2_dpram[offset] = data&0xff;

		if( namcos2_gametype==NAMCOS2_GOLLY_GHOST )
		{
			switch( offset )
			{
			case 0xc0/2: GollyGhostUpdateDiorama_c0(data); break;
			case 0xc2/2: break; /* unknown; 0x00 or 0x01 - probably lights up guns */
			case 0xc4/2: GollyGhostUpdateLED_c4(data); break;
			case 0xc6/2: GollyGhostUpdateLED_c6(data); break;
			case 0xc8/2: GollyGhostUpdateLED_c8(data); break;
			case 0xca/2: GollyGhostUpdateLED_ca(data); break;
			default:
				break;
			}
		}
	}
}

static READ_HANDLER( namcos2_dpram_byte_r )
{
	return namcos2_dpram[offset];
}

static WRITE_HANDLER( namcos2_dpram_byte_w )
{
	namcos2_dpram[offset] = data;
}

/*************************************************************/
/* SHARED 68000 CPU Memory declarations 					 */
/*************************************************************/

//	ROM0   = $200000-$2fffff
//	ROM1   = $300000-$3fffff
//	SCR    = $400000-$41ffff
//	SCRDT  = $420000-$43ffff
//	PALET  = $440000-$45ffff
//	DPCS   = $460000-$47ffff
//	SCOM   = $480000-$49ffff
//	SCOMDT = $4a0000-$4bffff

// 0xc00000 ONWARDS are unverified memory locations on the video board

#define NAMCOS2_68K_DEFAULT_CPU_BOARD_READ \
	{ 0x200000, 0x3fffff, namcos2_68k_data_rom_r },\
	{ 0x400000, 0x41ffff, namcos2_68k_vram_r },\
	{ 0x420000, 0x42003f, namcos2_68k_vram_ctrl_r }, \
	{ 0x440000, 0x44ffff, namcos2_68k_video_palette_r }, \
	{ 0x460000, 0x460fff, namcos2_68k_dpram_word_r }, \
	{ 0x468000, 0x468fff, namcos2_68k_dpram_word_r }, /* mirror */ \
	{ 0x480000, 0x483fff, namcos2_68k_serial_comms_ram_r }, \
	{ 0x4a0000, 0x4a000f, namcos2_68k_serial_comms_ctrl_r },

#define NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE \
	{ 0x400000, 0x41ffff, namcos2_68k_vram_w, &videoram16, &namcos2_68k_vram_size },\
	{ 0x420000, 0x42003f, namcos2_68k_vram_ctrl_w }, \
	{ 0x440000, 0x44ffff, namcos2_68k_video_palette_w, &namcos2_68k_palette_ram, &namcos2_68k_palette_size }, \
	{ 0x460000, 0x460fff, namcos2_68k_dpram_word_w }, \
	{ 0x468000, 0x468fff, namcos2_68k_dpram_word_w }, /* mirror */ \
	{ 0x480000, 0x483fff, namcos2_68k_serial_comms_ram_w, &namcos2_68k_serial_comms_ram }, \
	{ 0x4a0000, 0x4a000f, namcos2_68k_serial_comms_ctrl_w },

/*************************************************************/

#define NAMCOS2_68K_DEFAULT_GFX_BOARD_READ \
	{ 0xc80000, 0xc9ffff, namcos2_68k_roz_ram_r },	\
	{ 0xcc0000, 0xcc000f, namcos2_68k_roz_ctrl_r }, \
	{ 0xd00000, 0xd0000f, namcos2_68k_key_r },

#define NAMCOS2_68K_DEFAULT_GFX_BOARD_WRITE \
	{ 0xc00000, 0xc03fff, namcos2_sprite_ram_w, &namcos2_sprite_ram }, \
	{ 0xc40000, 0xc40001, namcos2_gfx_ctrl_w }, /* sprite bank, roz color, roz priority */ \
	{ 0xc80000, 0xc9ffff, namcos2_68k_roz_ram_w, &namcos2_68k_roz_ram }, \
	{ 0xcc0000, 0xcc000f, namcos2_68k_roz_ctrl_w }, \
	{ 0xd00000, 0xd0000f, namcos2_68k_key_w },

static MEMORY_READ16_START( readmem_slave_default )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_r },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_READ
	NAMCOS2_68K_DEFAULT_GFX_BOARD_READ
MEMORY_END

static MEMORY_WRITE16_START( writemem_slave_default )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_w },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE
	NAMCOS2_68K_DEFAULT_GFX_BOARD_WRITE
MEMORY_END

static MEMORY_READ16_START( readmem_master_default )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_R },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_r },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_READ
	NAMCOS2_68K_DEFAULT_GFX_BOARD_READ
MEMORY_END

static MEMORY_WRITE16_START( writemem_master_default )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_W },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_w },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE
	NAMCOS2_68K_DEFAULT_GFX_BOARD_WRITE
MEMORY_END

/*************************************************************/

static unsigned mFinalLapProtCount;

static READ16_HANDLER( flap_prot_r )
{
	/* this works for finalap2 */
	const data16_t table0[8] = { 0x0000,0x0040,0x0440,0x2440,0x2480,0xa080,0x8081,0x8041 };
	const data16_t table1[8] = { 0x0040,0x0060,0x0060,0x0860,0x0864,0x08e4,0x08e5,0x08a5 };
	data16_t data;

	switch( offset )
	{
	case 0:
		data = 0x0101;
		break;

	case 1:
		data = 0x3e55;
		break;

	case 2:
		data = table1[mFinalLapProtCount&7];
		data = (data&0xff00)>>8;
		break;

	case 3:
		data = table1[mFinalLapProtCount&7];
		mFinalLapProtCount++;
		data = data&0x00ff;
		break;

	case 0x3fffc/2:
		data = table0[mFinalLapProtCount&7];
		data = data&0xff00;
		break;

	case 0x3fffe/2:
		data = table0[mFinalLapProtCount&7];
		mFinalLapProtCount++;
		data = (data&0x00ff)<<8;
		break;

	default:
		data = 0;
	}
	return data;
}

#define NAMCOS2_68K_FINALLAP_GFX_BOARD_READ \
	{ 0x880000, 0x89ffff, namco_road16_r },

#define NAMCOS2_68K_FINALLAP_GFX_BOARD_WRITE \
	{ 0x800000, 0x80ffff, namcos2_sprite_ram_w, &namcos2_sprite_ram }, \
	{ 0x840000, 0x840001, namcos2_gfx_ctrl_w }, \
	{ 0x880000, 0x89ffff, namco_road16_w }, \
	{ 0x8c0000, 0x8c0001, MWA16_NOP },

static MEMORY_READ16_START( readmem_master_finallap )
	{ 0x300000, 0x33ffff, flap_prot_r },
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_R },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_r },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_READ
	NAMCOS2_68K_FINALLAP_GFX_BOARD_READ
MEMORY_END

static MEMORY_WRITE16_START( writemem_master_finallap )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_W },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_w },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE
	NAMCOS2_68K_FINALLAP_GFX_BOARD_WRITE
MEMORY_END

static MEMORY_READ16_START( readmem_slave_finallap )
	{ 0x300000, 0x33ffff, flap_prot_r },
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_r },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_READ
	NAMCOS2_68K_FINALLAP_GFX_BOARD_READ
MEMORY_END

static MEMORY_WRITE16_START( writemem_slave_finallap )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_w },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE
	NAMCOS2_68K_FINALLAP_GFX_BOARD_WRITE
MEMORY_END
/*************************************************************/

#define NAMCOS2_68K_SGUNNER_GFX_BOARD_READ \
	{ 0x800000, 0x8141ff, namco_obj16_r }, \
	{ 0xa00000, 0xa0000f, namcos2_68k_key_r },

#define NAMCOS2_68K_SGUNNER_GFX_BOARD_WRITE \
	{ 0x800000, 0x8141ff, namco_obj16_w }, \
	{ 0xa00000, 0xa0000f, namcos2_68k_key_w },

static MEMORY_READ16_START( readmem_master_sgunner )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_R },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_r },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_READ
	NAMCOS2_68K_SGUNNER_GFX_BOARD_READ
MEMORY_END

static MEMORY_WRITE16_START( writemem_master_sgunner )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_W },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_w },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE
	NAMCOS2_68K_SGUNNER_GFX_BOARD_WRITE
MEMORY_END

static MEMORY_READ16_START( readmem_slave_sgunner )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_r },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_READ
	NAMCOS2_68K_SGUNNER_GFX_BOARD_READ
MEMORY_END

static MEMORY_WRITE16_START( writemem_slave_sgunner )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_w },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE
	NAMCOS2_68K_SGUNNER_GFX_BOARD_WRITE
MEMORY_END

/*************************************************************/

static READ16_HANDLER( metlhawk_center_r )
{
	unsigned pc = activecpu_get_pc();
	if( pc>=0x4A0 && pc <0x4C4 )
	{
		return 0x80; /* HACK: "center" value for analog controls */
	}
	return namcos2_68k_master_ram[0xffc0/2+offset];
}

#define NAMCOS2_68K_METLHAWK_GFX_BOARD_READ \
	{ 0xc00000, 0xc03fff, namcos2_sprite_ram_r }, \
	{ 0xc40000, 0xc4ffff, namco_rozvideoram16_r }, \
	{ 0xd00000, 0xd0001f, namco_rozcontrol16_r },

#define NAMCOS2_68K_METLHAWK_GFX_BOARD_WRITE \
	{ 0xc00000, 0xc03fff, namcos2_sprite_ram_w, &namcos2_sprite_ram }, \
	{ 0xc40000, 0xc4ffff, namco_rozvideoram16_w }, \
	{ 0xd00000, 0xd0001f, namco_rozcontrol16_w }, \
	{ 0xe00000, 0xe00001, namcos2_gfx_ctrl_w },

static MEMORY_READ16_START( readmem_master_metlhawk )
	{ 0x10ffc0, 0x10ffc9, metlhawk_center_r },
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_R },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_r },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_READ
	NAMCOS2_68K_METLHAWK_GFX_BOARD_READ
MEMORY_END

static MEMORY_WRITE16_START( writemem_master_metlhawk )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_W },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_w },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE
	NAMCOS2_68K_METLHAWK_GFX_BOARD_WRITE
MEMORY_END

static MEMORY_READ16_START( readmem_slave_metlhawk )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_r },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_READ
	NAMCOS2_68K_METLHAWK_GFX_BOARD_READ
MEMORY_END

static MEMORY_WRITE16_START( writemem_slave_metlhawk )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_w },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE
	NAMCOS2_68K_METLHAWK_GFX_BOARD_WRITE
MEMORY_END

/*************************************************************/

#define NAMCOS2_68K_LUCKYWLD_GFX_BOARD_READ \
	{ 0x800000, 0x8141ff, namco_obj16_r }, \
	{ 0x818000, 0x818001, MRA16_RAM }, \
	{ 0x840000, 0x840001, MRA16_RAM }, \
	{ 0xa00000, 0xa1ffff, namco_road16_r }, \
	{ 0xc00000, 0xc0ffff, namco_rozvideoram16_r }, \
	{ 0xd00000, 0xd0001f, namco_rozcontrol16_r }, \
	{ 0xf00000, 0xf00007, namcos2_68k_key_r },

#define NAMCOS2_68K_LUCKYWLD_GFX_BOARD_WRITE \
	{ 0x800000, 0x8141ff, namco_obj16_w }, \
	{ 0x818000, 0x818001, MWA16_NOP }, /* enable? */ \
	{ 0x81a000, 0x81a001, MWA16_NOP }, /* enable? */ \
	{ 0x900000, 0x900007, namco_spritepos16_w }, \
	{ 0xa00000, 0xa1ffff, namco_road16_w }, \
	{ 0xc00000, 0xc0ffff, namco_rozvideoram16_w }, \
	{ 0xd00000, 0xd0001f, namco_rozcontrol16_w }, \
	{ 0xf00000, 0xf00007, namcos2_68k_key_w },

static MEMORY_READ16_START( readmem_master_luckywld )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_R },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_r },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_READ
	NAMCOS2_68K_LUCKYWLD_GFX_BOARD_READ
MEMORY_END

static MEMORY_WRITE16_START( writemem_master_luckywld )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, NAMCOS2_68K_MASTER_RAM_W },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_w },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE
	NAMCOS2_68K_LUCKYWLD_GFX_BOARD_WRITE
MEMORY_END

static MEMORY_READ16_START( readmem_slave_luckywld )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_r },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_READ
	NAMCOS2_68K_LUCKYWLD_GFX_BOARD_READ
MEMORY_END

static MEMORY_WRITE16_START( writemem_slave_luckywld )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x13ffff, NAMCOS2_68K_SLAVE_RAM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_w },
	NAMCOS2_68K_DEFAULT_CPU_BOARD_WRITE
	NAMCOS2_68K_LUCKYWLD_GFX_BOARD_WRITE
MEMORY_END

/*************************************************************/
/* 6809 SOUND CPU Memory declarations						 */
/*************************************************************/

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x3fff, BANKED_SOUND_ROM_R }, /* banked */
	{ 0x4000, 0x4001, YM2151_status_port_0_r },
	{ 0x5000, 0x6fff, C140_r },
	{ 0x7000, 0x77ff, namcos2_dpram_byte_r },
	{ 0x7800, 0x7fff, namcos2_dpram_byte_r },	/* mirror */
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xd000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4000, YM2151_register_port_0_w },
	{ 0x4001, 0x4001, YM2151_data_port_0_w },
	{ 0x5000, 0x6fff, C140_w },
	{ 0x7000, 0x77ff, namcos2_dpram_byte_w, &namcos2_dpram },
	{ 0x7800, 0x7fff, namcos2_dpram_byte_w },	/* mirror */
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xbfff, MWA_NOP },					/* Amplifier enable on 1st write */
	{ 0xc000, 0xc001, namcos2_sound_bankselect_w },
	{ 0xd001, 0xd001, MWA_NOP },					/* Watchdog */
	{ 0xe000, 0xe000, MWA_NOP },
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END


/*************************************************************/
/* 68705 IO CPU Memory declarations 						 */
/*************************************************************/

static MEMORY_READ_START( readmem_mcu )
	/* input ports and dips are mapped here */

	{ 0x0000, 0x0000, MRA_NOP },			// Keep logging quiet
	{ 0x0001, 0x0001, namcos2_input_port_0_r },
	{ 0x0002, 0x0002, input_port_1_r },
	{ 0x0003, 0x0003, namcos2_mcu_port_d_r },
	{ 0x0007, 0x0007, namcos2_input_port_10_r },
	{ 0x0010, 0x0010, namcos2_mcu_analog_ctrl_r },
	{ 0x0011, 0x0011, namcos2_mcu_analog_port_r },
	{ 0x0008, 0x003f, MRA_RAM },			// Fill in register to stop logging
	{ 0x0040, 0x01bf, MRA_RAM },
	{ 0x01c0, 0x1fff, MRA_ROM },
	{ 0x2000, 0x2000, input_port_11_r },
	{ 0x3000, 0x3000, namcos2_input_port_12_r },
	{ 0x3001, 0x3001, input_port_13_r },
	{ 0x3002, 0x3002, input_port_14_r },
	{ 0x3003, 0x3003, input_port_15_r },
	{ 0x5000, 0x57ff, namcos2_dpram_byte_r },
	{ 0x6000, 0x6fff, MRA_NOP },				/* watchdog */
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_mcu )
	{ 0x0003, 0x0003, namcos2_mcu_port_d_w },
	{ 0x0010, 0x0010, namcos2_mcu_analog_ctrl_w },
	{ 0x0011, 0x0011, namcos2_mcu_analog_port_w },
	{ 0x0000, 0x003f, MWA_RAM },			// Fill in register to stop logging
	{ 0x0040, 0x01bf, MWA_RAM },
	{ 0x01c0, 0x1fff, MWA_ROM },
	{ 0x5000, 0x57ff, namcos2_dpram_byte_w, &namcos2_dpram },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************************************/
/*															 */
/*	NAMCO SYSTEM 2 PORT MACROS								 */
/*															 */
/*	Below are the port defintion macros that should be used  */
/*	as the basis for defining a port set for a Namco System2  */
/*	game.													 */
/*															 */
/*************************************************************/

#define NAMCOS2_MCU_PORT_B_DEFAULT \
	PORT_START		/* 63B05Z0 - PORT B */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

#define NAMCOS2_MCU_PORT_C_DEFAULT \
	PORT_START		/* 63B05Z0 - PORT C & SCI */ \
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_SERVICE, "Service Button", KEYCODE_NONE, IP_JOY_NONE ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

#define NAMCOS2_MCU_ANALOG_PORT_DEFAULT \
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 5 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 6 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 7 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

#define NAMCOS2_MCU_PORT_H_DEFAULT \
	PORT_START		/* 63B05Z0 - PORT H */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )

#define NAMCOS2_MCU_DIPSW_DEFAULT \
	PORT_START		/* 63B05Z0 - $2000 DIP SW */ \
	PORT_DIPNAME( 0x01, 0x01, "Video Display") \
	PORT_DIPSETTING(	0x01, "Normal" ) \
	PORT_DIPSETTING(	0x00, "Frozen" ) \
	PORT_DIPNAME( 0x02, 0x02, "$2000-1") \
	PORT_DIPSETTING(	0x02, "H" ) \
	PORT_DIPSETTING(	0x00, "L" ) \
	PORT_DIPNAME( 0x04, 0x04, "$2000-2") \
	PORT_DIPSETTING(	0x04, "H" ) \
	PORT_DIPSETTING(	0x00, "L" ) \
	PORT_DIPNAME( 0x08, 0x08, "$2000-3") \
	PORT_DIPSETTING(	0x08, "H" ) \
	PORT_DIPSETTING(	0x00, "L" ) \
	PORT_DIPNAME( 0x10, 0x10, "$2000-4") \
	PORT_DIPSETTING(	0x10, "H" ) \
	PORT_DIPSETTING(	0x00, "L" ) \
	PORT_DIPNAME( 0x20, 0x20, "$2000-5") \
	PORT_DIPSETTING(	0x20, "H" ) \
	PORT_DIPSETTING(	0x00, "L" ) \
	PORT_DIPNAME( 0x40, 0x40, "$2000-6") \
	PORT_DIPSETTING(	0x40, "H" ) \
	PORT_DIPSETTING(	0x00, "L" ) \
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

#define NAMCOS2_MCU_DIAL_DEFAULT \
	PORT_START		/* 63B05Z0 - $3000 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) \
/*	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 20, 10, 0, 0 ) */ \
	PORT_START		/* 63B05Z0 - $3001 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_START		/* 63B05Z0 - $3002 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_START		/* 63B05Z0 - $3003 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

/*************************************************************/
/*															 */
/*	NAMCO SYSTEM 2 PORT DEFINITIONS 						 */
/*															 */
/*	There is a standard port definition defined that will	 */
/*	work for most games, if you wish to produce a special	 */
/*	definition for a particular game then see the assault	 */
/*	and dirtfox definitions for examples of how to construct */
/*	a special port definition								 */
/*															 */
/*	The default definitions includes only the following list */
/*	of connections :										 */
/*	  2 Joysticks, 6 Buttons, 1 Service, 1 Advance			 */
/*	  2 Start												 */
/*															 */
/*************************************************************/

INPUT_PORTS_START( default )
	NAMCOS2_MCU_PORT_B_DEFAULT
	NAMCOS2_MCU_PORT_C_DEFAULT
	NAMCOS2_MCU_ANALOG_PORT_DEFAULT
	NAMCOS2_MCU_PORT_H_DEFAULT
	NAMCOS2_MCU_DIPSW_DEFAULT
	NAMCOS2_MCU_DIAL_DEFAULT
INPUT_PORTS_END

INPUT_PORTS_START( gollygho )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	NAMCOS2_MCU_PORT_C_DEFAULT

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPF_REVERSE|IPT_LIGHTGUN_X, 50, 8, 0, 0xff )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPF_REVERSE|IPT_LIGHTGUN_Y, 50, 8, 0, 0xff )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPF_REVERSE|IPT_LIGHTGUN_X|IPF_PLAYER2, 50, 8, 0, 0xff )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPF_REVERSE|IPT_LIGHTGUN_Y|IPF_PLAYER2, 50, 8, 0, 0xff )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - PORT H */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	NAMCOS2_MCU_DIPSW_DEFAULT
	NAMCOS2_MCU_DIAL_DEFAULT
INPUT_PORTS_END

INPUT_PORTS_START( finallap )
	NAMCOS2_MCU_PORT_B_DEFAULT /* 0 */

	NAMCOS2_MCU_PORT_C_DEFAULT /* 1 */

	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* Steering Wheel 7 */		/* sensitivity, delta, min, max */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL|IPF_PLAYER1|IPF_CENTER, 50, 1, 0x00, 0xff )
	PORT_START		/* Brake Pedal 8 */
	PORT_ANALOG( 0xff, 0xff, IPT_PEDAL|IPF_PLAYER2|IPF_CENTER, 100, 30, 0x00, 0xff )
	PORT_START		/* Accelerator Pedal 9 */
	PORT_ANALOG( 0xff, 0xff, IPT_PEDAL|IPF_PLAYER1, 100, 15, 0x00, 0xff )

	NAMCOS2_MCU_PORT_H_DEFAULT
	NAMCOS2_MCU_DIPSW_DEFAULT
	NAMCOS2_MCU_DIAL_DEFAULT

#if 0
	PORT_START /* 16 */
	PORT_DIPNAME( 0x03, 0x03, "Car Type")
	PORT_DIPSETTING(	0x00, "Williams (white, blue, yellow)" )
	PORT_DIPSETTING(	0x01, "McLaren (red & white)" )
	PORT_DIPSETTING(	0x02, "Lotus (yellow)" )
	PORT_DIPSETTING(	0x03, "March (blue)" )
#endif

INPUT_PORTS_END

INPUT_PORTS_START( assault )
	PORT_START		/* 63B05Z0 - PORT B */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	NAMCOS2_MCU_PORT_C_DEFAULT
	NAMCOS2_MCU_ANALOG_PORT_DEFAULT

	PORT_START		/* 63B05Z0 - PORT H */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT )

	NAMCOS2_MCU_DIPSW_DEFAULT

	PORT_START	 /* 63B05Z0 - $3000 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT )
	PORT_START	 /* 63B05Z0 - $3001 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START	 /* 63B05Z0 - $3002 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
//	PORT_START	 /* 63B05Z0 - $3003 */
//	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* fake port15 for single joystick control */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_CHEAT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_CHEAT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_CHEAT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_CHEAT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_CHEAT )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( driving )
	NAMCOS2_MCU_PORT_B_DEFAULT
	NAMCOS2_MCU_PORT_C_DEFAULT

	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* Steering Wheel */
	PORT_ANALOG( 0xff, 0x7f, IPT_DIAL|IPF_PLAYER1, 70, 50, 0x00, 0xff )
	PORT_START		/* Brake pedal */
	PORT_ANALOG( 0xff, 0xff, IPT_PEDAL|IPF_PLAYER2, 100, 30, 0x00, 0x7f )
	PORT_START		/* Accelerator pedal */
	PORT_ANALOG( 0xff, 0xff, IPT_PEDAL|IPF_PLAYER1, 100, 15, 0x00, 0x7f )

	NAMCOS2_MCU_PORT_H_DEFAULT
	NAMCOS2_MCU_DIPSW_DEFAULT
	NAMCOS2_MCU_DIAL_DEFAULT
INPUT_PORTS_END

INPUT_PORTS_START( luckywld )
	PORT_START		/* 63B05Z0 - PORT B */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	NAMCOS2_MCU_PORT_C_DEFAULT

	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_Y | IPF_PLAYER2, 50, 8, 0, 0xff )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_Y, 50, 8, 0, 0xff )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_X | IPF_PLAYER2, 50, 8, 0, 0xff )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_X, 50, 8, 0, 0xff )
	PORT_START		/* Steering Wheel */
	PORT_ANALOG( 0xff, 0x7f, IPT_DIAL|IPF_CENTER|IPF_PLAYER1, 70, 50, 0x00, 0xff )
	PORT_START		/* Brake pedal */
	PORT_ANALOG( 0xff, 0xff, IPT_PEDAL|IPF_PLAYER2, 100, 30, 0x00, 0x7f )
	PORT_START		/* Accelerator pedal */
	PORT_ANALOG( 0xff, 0xff, IPT_PEDAL|IPF_PLAYER1, 100, 15, 0x00, 0x7f )

	PORT_START		/* 63B05Z0 - PORT H */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	NAMCOS2_MCU_DIPSW_DEFAULT
	NAMCOS2_MCU_DIAL_DEFAULT
INPUT_PORTS_END

INPUT_PORTS_START( sgunner  )
	PORT_START		/* 63B05Z0 - PORT B */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	NAMCOS2_MCU_PORT_C_DEFAULT

	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_X, 50, 8, 0, 0xff )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_X|IPF_PLAYER2, 50, 8, 0, 0xff )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_Y, 50, 8, 0, 0xff )
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_LIGHTGUN_Y|IPF_PLAYER2, 50, 8, 0, 0xff )

	PORT_START		/* 63B05Z0 - PORT H */
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	NAMCOS2_MCU_DIPSW_DEFAULT
	NAMCOS2_MCU_DIAL_DEFAULT
INPUT_PORTS_END

INPUT_PORTS_START( dirtfox )
	PORT_START		/* 63B05Z0 - PORT B */ \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )	/* Gear shift up */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )	/* Gear shift down */

	NAMCOS2_MCU_PORT_C_DEFAULT

	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* Steering Wheel */
	PORT_ANALOG( 0xff, 0x7f, IPT_DIAL|IPF_CENTER|IPF_PLAYER1, 70, 50, 0x00, 0xff )
	PORT_START		/* Brake pedal */
	PORT_ANALOG( 0xff, 0xff, IPT_PEDAL|IPF_PLAYER2, 100, 30, 0x00, 0x7f )
	PORT_START		/* Accelerator pedal */
	PORT_ANALOG( 0xff, 0xff, IPT_PEDAL|IPF_PLAYER1, 100, 15, 0x00, 0x7f )

	PORT_START		/* 63B05Z0 - PORT H */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	NAMCOS2_MCU_DIPSW_DEFAULT
	NAMCOS2_MCU_DIAL_DEFAULT
INPUT_PORTS_END

INPUT_PORTS_START( metlhawk )
	PORT_START		/* 63B05Z0 - PORT B */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	NAMCOS2_MCU_PORT_C_DEFAULT

	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* Joystick Y */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y|IPF_CENTER, 100, 16, 0x20, 0xe0 )
	PORT_START		/* Joystick X */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X|IPF_CENTER, 100, 16, 0x20, 0xe0 )
	PORT_START		/* Lever */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y|IPF_CENTER|IPF_REVERSE|IPF_PLAYER2, 100, 16, 0x20, 0xe0 )

	PORT_START		/* 63B05Z0 - PORT H */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	NAMCOS2_MCU_DIPSW_DEFAULT
	NAMCOS2_MCU_DIAL_DEFAULT
INPUT_PORTS_END


/*************************************************************/
/* Namco System II - Graphics Declarations					 */
/*************************************************************/

static struct GfxLayout obj_layout = {
	32,32,
	0x800,	/* number of sprites */
	8,		/* bits per pixel */
	{		/* plane offsets */
		(0x400000*3),(0x400000*3)+4,(0x400000*2),(0x400000*2)+4,
		(0x400000*1),(0x400000*1)+4,(0x400000*0),(0x400000*0)+4
	},
	{ /* x offsets */
		0*8,0*8+1,0*8+2,0*8+3,1*8,1*8+1,1*8+2,1*8+3,
		2*8,2*8+1,2*8+2,2*8+3,3*8,3*8+1,3*8+2,3*8+3,
		4*8,4*8+1,4*8+2,4*8+3,5*8,5*8+1,5*8+2,5*8+3,
		6*8,6*8+1,6*8+2,6*8+3,7*8,7*8+1,7*8+2,7*8+3,
	},
	{ /* y offsets */
		0x0*128,0x0*128+64,0x1*128,0x1*128+64,0x2*128,0x2*128+64,0x3*128,0x3*128+64,
		0x4*128,0x4*128+64,0x5*128,0x5*128+64,0x6*128,0x6*128+64,0x7*128,0x7*128+64,
		0x8*128,0x8*128+64,0x9*128,0x9*128+64,0xa*128,0xa*128+64,0xb*128,0xb*128+64,
		0xc*128,0xc*128+64,0xd*128,0xd*128+64,0xe*128,0xe*128+64,0xf*128,0xf*128+64
	},
	0x800 /* sprite offset */
};

static struct GfxLayout chr_layout = {
	8,8,
	RGN_FRAC(1,1),
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

static struct GfxLayout luckywld_sprite_layout = /* same as Namco System21 */
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
		0x0*32,0x1*32,0x2*32,0x3*32,
		0x4*32,0x5*32,0x6*32,0x7*32,
		0x8*32,0x9*32,0xa*32,0xb*32,
		0xc*32,0xd*32,0xe*32,0xf*32
	},
	8*64 /* sprite offset */
};

static struct GfxLayout luckywld_roz_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	{ 0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128 },
	16*128
};

static struct GfxLayout metlhawk_sprite_layout = {
	32,32,
	0x1000,	/* number of sprites */
	8, /* bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248 },
	{ 0, 256, 512, 768, 1024, 1280, 1536, 1792, 2048, 2304, 2560, 2816, 3072, 3328, 3584, 3840, 4096, 4352, 4608, 4864, 5120, 5376, 5632, 5888, 6144, 6400, 6656, 6912, 7168, 7424, 7680, 7936 },
	32*32*8
};

/*
static struct GfxLayout mask_layout16 = {
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 },
	{ 16*0,16*1,16*2,16*3,16*4,16*5,16*6,16*7,16*8,16*9,16*10,16*11,16*12,16*13,16*14,16*15 },
	16*16
};
static struct GfxLayout mask_layout8 = {
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0,1,2,3,4,5,6,7 },
	{ 8*0,8*1,8*2,8*3,8*4,8*5,8*6,8*7 },
	8*8
};
*/

static struct GfxDecodeInfo metlhawk_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &metlhawk_sprite_layout,	 0*256, 16 },
	{ REGION_GFX3, 0x000000, &luckywld_roz_layout,		 0*256, 16 },
	{ REGION_GFX2, 0x000000, &chr_layout,				16*256, 12 },
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &obj_layout,  0*256, 16 },
	{ REGION_GFX1, 0x200000, &obj_layout,  0*256, 16 },
	{ REGION_GFX2, 0x000000, &chr_layout, 16*256, 12 },
	{ REGION_GFX3, 0x000000, &roz_layout, 28*256,  2 },
	{ -1 }
};

static struct GfxDecodeInfo finallap_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &obj_layout,  0*256, 16 },
	{ REGION_GFX1, 0x200000, &obj_layout,  0*256, 16 },
	{ REGION_GFX2, 0x000000, &chr_layout, 16*256, 12 },
	{ -1 }
};

static struct GfxDecodeInfo sgunner_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &luckywld_sprite_layout,	 0*256, 16 },
	{ REGION_GFX3, 0x000000, &luckywld_roz_layout,		 0*256, 16 },
	{ REGION_GFX2, 0x000000, &chr_layout,				16*256, 12 },
	{ -1 }
};

static struct GfxDecodeInfo luckywld_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &luckywld_sprite_layout,	 0*256, 16 },
	{ REGION_GFX3, 0x000000, &luckywld_roz_layout,		 0*256, 16 },
	{ REGION_GFX2, 0x000000, &chr_layout,				16*256, 12 },
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
	C140_TYPE_SYSTEM2,
	8000000/374,
	REGION_SOUND1,
	50
};



/******************************************

Master clock = 49.152MHz

68000 Measured at  84ns = 12.4MHz	BUT 49.152MHz/4 = 12.288MHz = 81ns
6809  Measured at 343ns = 2.915 MHz BUT 49.152MHz/16 = 3.072MHz = 325ns
63B05 Measured at 120ns = 8.333 MHz BUT 49.152MHz/6 = 8.192MHz = 122ns

I've corrected all frequencies to be multiples of integer divisions of
the 49.152MHz master clock. Additionally the 6305 looks to hav an
internal divider.

Soooo;

680000	= 12288000
6809	=  3072000
63B05Z0 =  2048000

The interrupts to CPU4 has been measured at 60Hz (16.5mS period) on a
logic analyser. This interrupt is wired to port PA1 which is configured
via software as INT1

*******************************************/

/*************************************************************/
/*															 */
/*	NAMCO SYSTEM 2 MACHINE DEFINTIONS						 */
/*															 */
/*************************************************************/

static MACHINE_DRIVER_START( default )
	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_master_default,writemem_master_default)
	MDRV_CPU_VBLANK_INT(namcos2_68k_master_vblank,1)

	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_slave_default,writemem_slave_default)
	MDRV_CPU_VBLANK_INT(namcos2_68k_slave_vblank,1)

	MDRV_CPU_ADD(M6809,3072000) // Sound handling
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,120)

	MDRV_CPU_ADD(HD63705,2048000) // I/O handling
	MDRV_CPU_MEMORY(readmem_mcu,writemem_mcu)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND( (49152000.0 / 8) / (384 * 264) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* CPU slices per frame */

	MDRV_MACHINE_INIT(namcos2)
	MDRV_NVRAM_HANDLER(namcos2)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(VIRTUAL_PALETTE_BANKS*256)	/* virtual palette (physical palette has 8192 colors) */

	MDRV_VIDEO_START(namcos2)
	MDRV_VIDEO_UPDATE(namcos2_default)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(C140, C140_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gollygho )
	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_master_default,writemem_master_default)
	MDRV_CPU_VBLANK_INT(namcos2_68k_master_vblank,1)

	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_slave_default,writemem_slave_default)
	MDRV_CPU_VBLANK_INT(namcos2_68k_slave_vblank,1)

	MDRV_CPU_ADD(M6809,3072000) // Sound handling
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,120)

	MDRV_CPU_ADD(HD63705,2048000) // I/O handling
	MDRV_CPU_MEMORY(readmem_mcu,writemem_mcu)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND( (49152000.0 / 8) / (384 * 264) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* CPU slices per frame */

	MDRV_MACHINE_INIT(namcos2)
	MDRV_NVRAM_HANDLER(namcos2)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(VIRTUAL_PALETTE_BANKS*256)	/* virtual palette (physical palette has 8192 colors) */

	MDRV_VIDEO_START(namcos2)
	MDRV_VIDEO_UPDATE(namcos2_default)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(C140, C140_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( finallap )
	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_master_finallap,writemem_master_finallap)
	MDRV_CPU_VBLANK_INT(namcos2_68k_master_vblank,1)

	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_slave_finallap,writemem_slave_finallap)
	MDRV_CPU_VBLANK_INT(namcos2_68k_slave_vblank,1)

	MDRV_CPU_ADD(M6809,3072000) // Sound handling
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,120)

	MDRV_CPU_ADD(HD63705,2048000) // I/O handling
	MDRV_CPU_MEMORY(readmem_mcu,writemem_mcu)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND( (49152000.0 / 8) / (384 * 264) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* CPU slices per frame */

	MDRV_MACHINE_INIT(namcos2)
	MDRV_NVRAM_HANDLER(namcos2)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)

	MDRV_GFXDECODE(finallap_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(VIRTUAL_PALETTE_BANKS*256)	/* virtual palette (physical palette has 8192 colors) */

	MDRV_VIDEO_START(finallap)
	MDRV_VIDEO_UPDATE(finallap)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(C140, C140_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sgunner )
	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_master_sgunner,writemem_master_sgunner)
	MDRV_CPU_VBLANK_INT(namcos2_68k_master_vblank,1)

	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_slave_sgunner,writemem_slave_sgunner)
	MDRV_CPU_VBLANK_INT(namcos2_68k_slave_vblank,1)

	MDRV_CPU_ADD(M6809,3072000) // Sound handling
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,120)

	MDRV_CPU_ADD(HD63705,2048000) // I/O handling
	MDRV_CPU_MEMORY(readmem_mcu,writemem_mcu)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND( (49152000.0 / 8) / (384 * 264) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* CPU slices per frame */

	MDRV_MACHINE_INIT(namcos2)
	MDRV_NVRAM_HANDLER(namcos2)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sgunner_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(VIRTUAL_PALETTE_BANKS*256)	/* virtual palette (physical palette has 8192 colors) */

	MDRV_VIDEO_START(sgunner)
	MDRV_VIDEO_UPDATE(sgunner)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(C140, C140_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( luckywld )
	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_master_luckywld,writemem_master_luckywld)
	MDRV_CPU_VBLANK_INT(namcos2_68k_master_vblank,1)

	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_slave_luckywld,writemem_slave_luckywld)
	MDRV_CPU_VBLANK_INT(namcos2_68k_slave_vblank,1)

	MDRV_CPU_ADD(M6809,3072000) /* Sound handling */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,120)

	MDRV_CPU_ADD(HD63705,2048000) /* I/O handling */
	MDRV_CPU_MEMORY(readmem_mcu,writemem_mcu)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND( (49152000.0 / 8) / (384 * 264) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* CPU slices per frame */

	MDRV_MACHINE_INIT(namcos2)
	MDRV_NVRAM_HANDLER(namcos2)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(luckywld_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(VIRTUAL_PALETTE_BANKS*256)	/* virtual palette (physical palette has 8192 colors) */

	MDRV_VIDEO_START(luckywld)
	MDRV_VIDEO_UPDATE(luckywld)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(C140, C140_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( metlhawk )
	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_master_metlhawk,writemem_master_metlhawk)
	MDRV_CPU_VBLANK_INT(namcos2_68k_master_vblank,1)

	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem_slave_metlhawk,writemem_slave_metlhawk)
	MDRV_CPU_VBLANK_INT(namcos2_68k_slave_vblank,1)

	MDRV_CPU_ADD(M6809,3072000) /* Sound handling */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,120)

	MDRV_CPU_ADD(HD63705,2048000) /* I/O handling */
	MDRV_CPU_MEMORY(readmem_mcu,writemem_mcu)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND( (49152000.0 / 8) / (384 * 264) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* CPU slices per frame */

	MDRV_MACHINE_INIT(namcos2)
	MDRV_NVRAM_HANDLER(namcos2)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)

	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)

    MDRV_GFXDECODE(metlhawk_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(VIRTUAL_PALETTE_BANKS*256)	/* virtual palette (physical palette has 8192 colors) */

	MDRV_VIDEO_START(metlhawk)
	MDRV_VIDEO_UPDATE(metlhawk)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(C140, C140_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END


/*************************************************************/
/* Namco System II - ROM Declarations						 */
/*************************************************************/

#define NAMCOS2_GFXROM_LOAD_128K(romname,start,chksum)\
	ROM_LOAD( romname		, (start + 0x000000), 0x020000, chksum )\
	ROM_RELOAD( 			  (start + 0x020000), 0x020000 )\
	ROM_RELOAD( 			  (start + 0x040000), 0x020000 )\
	ROM_RELOAD( 			  (start + 0x060000), 0x020000 )

#define NAMCOS2_GFXROM_LOAD_256K(romname,start,chksum)\
	ROM_LOAD( romname		, (start + 0x000000), 0x040000, chksum )\
	ROM_RELOAD( 			  (start + 0x040000), 0x040000 )

#define NAMCOS2_DATA_LOAD_E_128K(romname,start,chksum)\
	ROM_LOAD16_BYTE(romname	, (start + 0x000000), 0x020000, chksum )\
	ROM_RELOAD(				  (start + 0x040000), 0x020000 )\
	ROM_RELOAD(				  (start + 0x080000), 0x020000 )\
	ROM_RELOAD(				  (start + 0x0c0000), 0x020000 )

#define NAMCOS2_DATA_LOAD_O_128K(romname,start,chksum)\
	ROM_LOAD16_BYTE( romname, (start + 0x000001), 0x020000, chksum )\
	ROM_RELOAD( 			  (start + 0x040001), 0x020000 )\
	ROM_RELOAD( 			  (start + 0x080001), 0x020000 )\
	ROM_RELOAD( 			  (start + 0x0c0001), 0x020000 )

#define NAMCOS2_DATA_LOAD_E_256K(romname,start,chksum)\
	ROM_LOAD16_BYTE(romname	, (start + 0x000000), 0x040000, chksum )\
	ROM_RELOAD(				  (start + 0x080000), 0x040000 )

#define NAMCOS2_DATA_LOAD_O_256K(romname,start,chksum)\
	ROM_LOAD16_BYTE( romname, (start + 0x000001), 0x040000, chksum )\
	ROM_RELOAD(	 			  (start + 0x080001), 0x040000 )

#define NAMCOS2_DATA_LOAD_E_512K(romname,start,chksum)\
	ROM_LOAD16_BYTE(romname	, (start + 0x000000), 0x080000, chksum )

#define NAMCOS2_DATA_LOAD_O_512K(romname,start,chksum)\
	ROM_LOAD16_BYTE( romname, (start + 0x000001), 0x080000, chksum )


/* ASSAULT (NAMCO) */
ROM_START( assault )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "at2mp0b.bin",  0x000000, 0x010000, 0x801f71c5 )
	ROM_LOAD16_BYTE( "at2mp1b.bin",  0x000001, 0x010000, 0x72312d4f )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "at1sp0.bin",  0x000000, 0x010000, 0x0de2a0da )
	ROM_LOAD16_BYTE( "at1sp1.bin",  0x000001, 0x010000, 0x02d15fbe )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "at1snd0.bin",  0x00c000, 0x004000, 0x1d1ffe12 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65b.bin",  0x008000, 0x008000, 0xe9f2922a )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	NAMCOS2_GFXROM_LOAD_128K( "atobj0.bin",  0x000000, 0x22240076 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj1.bin",  0x080000, 0x2284a8e8 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj2.bin",  0x100000, 0x51425476 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj3.bin",  0x180000, 0x791f42ce )
	NAMCOS2_GFXROM_LOAD_128K( "atobj4.bin",  0x200000, 0x4782e1b0 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj5.bin",  0x280000, 0xf5d158cf )
	NAMCOS2_GFXROM_LOAD_128K( "atobj6.bin",  0x300000, 0x12f6a569 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj7.bin",  0x380000, 0x06a929f2 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "atchr0.bin",  0x000000, 0x6f8e968a )
	NAMCOS2_GFXROM_LOAD_128K( "atchr1.bin",  0x080000, 0x88cf7cbe )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "atroz0.bin",  0x000000, 0x8c247a97 )
	NAMCOS2_GFXROM_LOAD_128K( "atroz1.bin",  0x080000, 0xe44c475b )
	NAMCOS2_GFXROM_LOAD_128K( "atroz2.bin",  0x100000, 0x770f377f )
	NAMCOS2_GFXROM_LOAD_128K( "atroz3.bin",  0x180000, 0x01d93d0b )
	NAMCOS2_GFXROM_LOAD_128K( "atroz4.bin",  0x200000, 0xf96feab5 )
	NAMCOS2_GFXROM_LOAD_128K( "atroz5.bin",  0x280000, 0xda2f0d9e )
	NAMCOS2_GFXROM_LOAD_128K( "atroz6.bin",  0x300000, 0x9089e477 )
	NAMCOS2_GFXROM_LOAD_128K( "atroz7.bin",  0x380000, 0x62b2783a )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "atshape.bin",  0x000000, 0xdfcad82b )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "at1dat0.bin",  0x000000, 0x844890f4 )
	NAMCOS2_DATA_LOAD_O_128K( "at1dat1.bin",  0x000000, 0x21715313 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "atvoi1.bin",  0x000000, 0x080000, 0xd36a649e )
ROM_END

/* ASSAULT (JAPAN) */
ROM_START( assaultj )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "at1_mp0.bin",  0x000000, 0x010000, 0x2d3e5c8c )
	ROM_LOAD16_BYTE( "at1_mp1.bin",  0x000001, 0x010000, 0x851cec3a )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "at1sp0.bin",  0x000000, 0x010000, 0x0de2a0da )
	ROM_LOAD16_BYTE( "at1sp1.bin",  0x000001, 0x010000, 0x02d15fbe )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "at1snd0.bin",  0x00c000, 0x004000, 0x1d1ffe12 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65b.bin",  0x008000, 0x008000, 0xe9f2922a )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	NAMCOS2_GFXROM_LOAD_128K( "atobj0.bin",  0x000000, 0x22240076 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj1.bin",  0x080000, 0x2284a8e8 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj2.bin",  0x100000, 0x51425476 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj3.bin",  0x180000, 0x791f42ce )
	NAMCOS2_GFXROM_LOAD_128K( "atobj4.bin",  0x200000, 0x4782e1b0 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj5.bin",  0x280000, 0xf5d158cf )
	NAMCOS2_GFXROM_LOAD_128K( "atobj6.bin",  0x300000, 0x12f6a569 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj7.bin",  0x380000, 0x06a929f2 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "atchr0.bin",  0x000000, 0x6f8e968a )
	NAMCOS2_GFXROM_LOAD_128K( "atchr1.bin",  0x080000, 0x88cf7cbe )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "atroz0.bin",  0x000000, 0x8c247a97 )
	NAMCOS2_GFXROM_LOAD_128K( "atroz1.bin",  0x080000, 0xe44c475b )
	NAMCOS2_GFXROM_LOAD_128K( "atroz2.bin",  0x100000, 0x770f377f )
	NAMCOS2_GFXROM_LOAD_128K( "atroz3.bin",  0x180000, 0x01d93d0b )
	NAMCOS2_GFXROM_LOAD_128K( "atroz4.bin",  0x200000, 0xf96feab5 )
	NAMCOS2_GFXROM_LOAD_128K( "atroz5.bin",  0x280000, 0xda2f0d9e )
	NAMCOS2_GFXROM_LOAD_128K( "atroz6.bin",  0x300000, 0x9089e477 )
	NAMCOS2_GFXROM_LOAD_128K( "atroz7.bin",  0x380000, 0x62b2783a )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "atshape.bin",  0x000000, 0xdfcad82b )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "at1dat0.bin",  0x000000, 0x844890f4 )
	NAMCOS2_DATA_LOAD_O_128K( "at1dat1.bin",  0x000000, 0x21715313 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "atvoi1.bin",  0x000000, 0x080000, 0xd36a649e )
ROM_END

/* ASSAULT PLUS (NAMCO) */
ROM_START( assaultp )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "mpr0.bin",	0x000000, 0x010000, 0x97519f9f )
	ROM_LOAD16_BYTE( "mpr1.bin",	0x000001, 0x010000, 0xc7f437c7 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "at1sp0.bin",  0x000000, 0x010000, 0x0de2a0da )
	ROM_LOAD16_BYTE( "at1sp1.bin",  0x000001, 0x010000, 0x02d15fbe )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "at1snd0.bin",  0x00c000, 0x004000, 0x1d1ffe12 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65b.bin",  0x008000, 0x008000, 0xe9f2922a )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	NAMCOS2_GFXROM_LOAD_128K( "atobj0.bin",  0x000000, 0x22240076 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj1.bin",  0x080000, 0x2284a8e8 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj2.bin",  0x100000, 0x51425476 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj3.bin",  0x180000, 0x791f42ce )
	NAMCOS2_GFXROM_LOAD_128K( "atobj4.bin",  0x200000, 0x4782e1b0 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj5.bin",  0x280000, 0xf5d158cf )
	NAMCOS2_GFXROM_LOAD_128K( "atobj6.bin",  0x300000, 0x12f6a569 )
	NAMCOS2_GFXROM_LOAD_128K( "atobj7.bin",  0x380000, 0x06a929f2 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "atchr0.bin",  0x000000, 0x6f8e968a )
	NAMCOS2_GFXROM_LOAD_128K( "atchr1.bin",  0x080000, 0x88cf7cbe )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "atroz0.bin",  0x000000, 0x8c247a97 )
	NAMCOS2_GFXROM_LOAD_128K( "atroz1.bin",  0x080000, 0xe44c475b )
	NAMCOS2_GFXROM_LOAD_128K( "atroz2.bin",  0x100000, 0x770f377f )
	NAMCOS2_GFXROM_LOAD_128K( "atroz3.bin",  0x180000, 0x01d93d0b )
	NAMCOS2_GFXROM_LOAD_128K( "atroz4.bin",  0x200000, 0xf96feab5 )
	NAMCOS2_GFXROM_LOAD_128K( "atroz5.bin",  0x280000, 0xda2f0d9e )
	NAMCOS2_GFXROM_LOAD_128K( "atroz6.bin",  0x300000, 0x9089e477 )
	NAMCOS2_GFXROM_LOAD_128K( "atroz7.bin",  0x380000, 0x62b2783a )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "atshape.bin",  0x000000, 0xdfcad82b )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "at1dat0.bin",  0x000000, 0x844890f4 )
	NAMCOS2_DATA_LOAD_O_128K( "at1dat1.bin",  0x000000, 0x21715313 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "atvoi1.bin",  0x000000, 0x080000, 0xd36a649e )
ROM_END

/* BURNING FORCE */
ROM_START( burnforc )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "bumpr0c.bin",  0x000000, 0x020000, 0xcc5864c6 )
	ROM_LOAD16_BYTE( "bumpr1c.bin",  0x000001, 0x020000, 0x3e6b4b1b )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "bu1spr0.bin",  0x000000, 0x010000, 0x17022a21 )
	ROM_LOAD16_BYTE( "bu1spr1.bin",  0x000001, 0x010000, 0x5255f8a5 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "busnd0.bin",  0x00c000, 0x004000, 0xfabb1150 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "buobj0.bin",  0x000000, 0x80000, 0x24c919a1 )
	ROM_LOAD( "buobj1.bin",  0x080000, 0x80000, 0x5bcb519b )
	ROM_LOAD( "buobj2.bin",  0x100000, 0x80000, 0x509dd5d0 )
	ROM_LOAD( "buobj3.bin",  0x180000, 0x80000, 0x270a161e )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "buchr0.bin",  0x000000, 0xc2109f73 )
	NAMCOS2_GFXROM_LOAD_128K( "buchr1.bin",  0x080000, 0x67d6aa67 )
	NAMCOS2_GFXROM_LOAD_128K( "buchr2.bin",  0x100000, 0x52846eff )
	NAMCOS2_GFXROM_LOAD_128K( "buchr3.bin",  0x180000, 0xd1326d7f )
	NAMCOS2_GFXROM_LOAD_128K( "buchr4.bin",  0x200000, 0x81a66286 )
	NAMCOS2_GFXROM_LOAD_128K( "buchr5.bin",  0x280000, 0x629aa67f )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "buroz0.bin",  0x000000, 0x65fefc83 )
	NAMCOS2_GFXROM_LOAD_128K( "buroz1.bin",  0x080000, 0x979580c2 )
	NAMCOS2_GFXROM_LOAD_128K( "buroz2.bin",  0x100000, 0x548b6ad8 )
	NAMCOS2_GFXROM_LOAD_128K( "buroz3.bin",  0x180000, 0xa633cea0 )
	NAMCOS2_GFXROM_LOAD_128K( "buroz4.bin",  0x200000, 0x1b1f56a6 )
	NAMCOS2_GFXROM_LOAD_128K( "buroz5.bin",  0x280000, 0x4b864b0e )
	NAMCOS2_GFXROM_LOAD_128K( "buroz6.bin",  0x300000, 0x38bd25ba )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "bushape.bin",  0x000000,0x80a6b722 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "bu1dat0.bin",  0x000000, 0xe0a9d92f )
	NAMCOS2_DATA_LOAD_O_128K( "bu1dat1.bin",  0x000000, 0x5fe54b73 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "buvoi1.bin",  0x000000, 0x080000, 0x99d8a239 )
ROM_END

/* COSMO GANG THE VIDEO (USA) */
ROM_START( cosmogng )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "co2_mp0",  0x000000, 0x020000, 0x2632c209 )
	ROM_LOAD16_BYTE( "co2_mp1",  0x000001, 0x020000, 0x65840104 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "co1spr0.bin",  0x000000, 0x020000, 0xbba2c28f )
	ROM_LOAD16_BYTE( "co1spr1.bin",  0x000001, 0x020000, 0xc029b459 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "co2_s0",  0x00c000, 0x004000, 0x4ca59338 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "co1obj0.bin",  0x000000, 0x80000, 0x5df8ce0c )
	ROM_LOAD( "co1obj1.bin",  0x080000, 0x80000, 0x3d152497 )
	ROM_LOAD( "co1obj2.bin",  0x100000, 0x80000, 0x4e50b6ee )
	ROM_LOAD( "co1obj3.bin",  0x180000, 0x80000, 0x7beed669 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "co1chr0.bin",  0x000000, 0x80000, 0xee375b3e )
	ROM_LOAD( "co1chr1.bin",  0x080000, 0x80000, 0x0149de65 )
	ROM_LOAD( "co1chr2.bin",  0x100000, 0x80000, 0x93d565a0 )
	ROM_LOAD( "co1chr3.bin",  0x180000, 0x80000, 0x4d971364 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	ROM_LOAD( "co1roz0.bin",  0x000000, 0x80000, 0x2bea6951 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "co1sha0.bin",  0x000000, 0x80000, 0x063a70cc )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "co1dat0.bin",  0x000000, 0xb53da2ae )
	NAMCOS2_DATA_LOAD_O_128K( "co1dat1.bin",  0x000000, 0xd21ad10b )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "co2_v1",  0x000000, 0x080000, 0x5a301349 )
	ROM_LOAD( "co2_v2",  0x080000, 0x080000, 0xa27cb45a )
ROM_END

/* COSMO GANG THE VIDEO (JAPAN) */
ROM_START( cosmognj )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "co1mpr0.bin",  0x000000, 0x020000, 0xd1b4c8db )
	ROM_LOAD16_BYTE( "co1mpr1.bin",  0x000001, 0x020000, 0x2f391906 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "co1spr0.bin",  0x000000, 0x020000, 0xbba2c28f )
	ROM_LOAD16_BYTE( "co1spr1.bin",  0x000001, 0x020000, 0xc029b459 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "co1snd0.bin",  0x00c000, 0x004000, 0x6bfa619f )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "co1obj0.bin",  0x000000, 0x80000, 0x5df8ce0c )
	ROM_LOAD( "co1obj1.bin",  0x080000, 0x80000, 0x3d152497 )
	ROM_LOAD( "co1obj2.bin",  0x100000, 0x80000, 0x4e50b6ee )
	ROM_LOAD( "co1obj3.bin",  0x180000, 0x80000, 0x7beed669 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "co1chr0.bin",  0x000000, 0x80000, 0xee375b3e )
	ROM_LOAD( "co1chr1.bin",  0x080000, 0x80000, 0x0149de65 )
	ROM_LOAD( "co1chr2.bin",  0x100000, 0x80000, 0x93d565a0 )
	ROM_LOAD( "co1chr3.bin",  0x180000, 0x80000, 0x4d971364 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	ROM_LOAD( "co1roz0.bin",  0x000000, 0x80000, 0x2bea6951 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "co1sha0.bin",  0x000000, 0x80000, 0x063a70cc )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "co1dat0.bin",  0x000000, 0xb53da2ae )
	NAMCOS2_DATA_LOAD_O_128K( "co1dat1.bin",  0x000000, 0xd21ad10b )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "co1voi1.bin",  0x000000, 0x080000, 0xb5ba8f15 )
	ROM_LOAD( "co1voi2.bin",  0x080000, 0x080000, 0xb566b105 )
ROM_END

/* DIRT FOX (JAPAN) */
ROM_START( dirtfoxj )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "df1_mpr0.bin",	0x000000, 0x020000, 0x8386c820 )
	ROM_LOAD16_BYTE( "df1_mpr1.bin",	0x000001, 0x020000, 0x51085728 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "df1_spr0.bin",	0x000000, 0x020000, 0xd4906585 )
	ROM_LOAD16_BYTE( "df1_spr1.bin",	0x000001, 0x020000, 0x7d76cf57 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "df1_snd0.bin",  0x00c000, 0x004000, 0x66b4f3ab )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "df1_obj0.bin",  0x000000, 0x80000, 0xb6bd1a68 )
	ROM_LOAD( "df1_obj1.bin",  0x080000, 0x80000, 0x05421dc1 )
	ROM_LOAD( "df1_obj2.bin",  0x100000, 0x80000, 0x9390633e )
	ROM_LOAD( "df1_obj3.bin",  0x180000, 0x80000, 0xc8447b33 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "df1_chr0.bin",  0x000000, 0x4b10e4ed )
	NAMCOS2_GFXROM_LOAD_128K( "df1_chr1.bin",  0x080000, 0x8f63f3d6 )
	NAMCOS2_GFXROM_LOAD_128K( "df1_chr2.bin",  0x100000, 0x5a1b852a )
	NAMCOS2_GFXROM_LOAD_128K( "df1_chr3.bin",  0x180000, 0x28570676 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_256K( "df1_roz0.bin",  0x000000, 0xa6129f94 )
	NAMCOS2_GFXROM_LOAD_256K( "df1_roz1.bin",  0x080000, 0xc8e7ce73 )
	NAMCOS2_GFXROM_LOAD_256K( "df1_roz2.bin",  0x100000, 0xc598e923 )
	NAMCOS2_GFXROM_LOAD_256K( "df1_roz3.bin",  0x180000, 0x5a38b062 )
	NAMCOS2_GFXROM_LOAD_256K( "df1_roz4.bin",  0x200000, 0xe196d2e8 )
	NAMCOS2_GFXROM_LOAD_256K( "df1_roz5.bin",  0x280000, 0x1f8a1a3c )
	NAMCOS2_GFXROM_LOAD_256K( "df1_roz6.bin",  0x300000, 0x7f3a1ed9 )
	NAMCOS2_GFXROM_LOAD_256K( "df1_roz7.bin",  0x380000, 0xdd546ae8 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "df1_sha.bin",  0x000000, 0x9a7c9a9b )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_256K( "df1_dat0.bin",  0x000000, 0xf5851c85 )
	NAMCOS2_DATA_LOAD_O_256K( "df1_dat1.bin",  0x000000, 0x1a31e46b )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "df1_voi1.bin",  0x000000, 0x080000, 0x15053904 )
ROM_END

/* DRAGON SABER */
ROM_START( dsaber )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "mpr0.bin",	0x000000, 0x020000, 0x45309ddc )
	ROM_LOAD16_BYTE( "mpr1.bin",	0x000001, 0x020000, 0xcbfc4cba )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "spr0.bin",	0x000000, 0x010000, 0x013faf80 )
	ROM_LOAD16_BYTE( "spr1.bin",	0x000001, 0x010000, 0xc36242bb )

	ROM_REGION( 0x050000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "snd0.bin",  0x00c000, 0x004000, 0xaf5b1ff8 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )
	ROM_LOAD( "snd1.bin",  0x030000, 0x020000, 0xc4ca6f3f )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "obj0.bin",  0x000000, 0x80000, 0xf08c6648 )
	ROM_LOAD( "obj1.bin",  0x080000, 0x80000, 0x34e0810d )
	ROM_LOAD( "obj2.bin",  0x100000, 0x80000, 0xbccdabf3 )
	ROM_LOAD( "obj3.bin",  0x180000, 0x80000, 0x2a60a4b8 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "co1chr0.bin",  0x000000, 0x80000, 0xc6058df6 )
	ROM_LOAD( "co1chr1.bin",  0x080000, 0x80000, 0x67aaab36 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	ROM_LOAD( "roz0.bin",  0x000000, 0x80000, 0x32aab758 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "shape.bin",	0x000000, 0x80000, 0x698e7a3e )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "data0.bin",	0x000000, 0x3e53331f )
	NAMCOS2_DATA_LOAD_O_128K( "data1.bin",	0x000000, 0xd5427f11 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "voi1.bin",  0x000000, 0x080000, 0xdadf6a57 )
	ROM_LOAD( "voi2.bin",  0x080000, 0x080000, 0x81078e01 )
ROM_END

/* DRAGON SABER (JAPAN) */
ROM_START( dsaberj )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "do1mpr0b.bin",	0x000000, 0x020000, 0x2898e791 )
	ROM_LOAD16_BYTE( "do1mpr1b.bin",	0x000001, 0x020000, 0x5fa9778e )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "spr0.bin",	0x000000, 0x010000, 0x013faf80 )
	ROM_LOAD16_BYTE( "spr1.bin",	0x000001, 0x010000, 0xc36242bb )

	ROM_REGION( 0x050000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "snd0.bin",  0x00c000, 0x004000, 0xaf5b1ff8 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )
	ROM_LOAD( "snd1.bin",  0x030000, 0x020000, 0xc4ca6f3f )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "obj0.bin",  0x000000, 0x80000, 0xf08c6648 )
	ROM_LOAD( "obj1.bin",  0x080000, 0x80000, 0x34e0810d )
	ROM_LOAD( "obj2.bin",  0x100000, 0x80000, 0xbccdabf3 )
	ROM_LOAD( "obj3.bin",  0x180000, 0x80000, 0x2a60a4b8 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "co1chr0.bin",  0x000000, 0x80000, 0xc6058df6 )
	ROM_LOAD( "co1chr1.bin",  0x080000, 0x80000, 0x67aaab36 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	ROM_LOAD( "roz0.bin",  0x000000, 0x80000, 0x32aab758 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "shape.bin",	0x000000, 0x80000, 0x698e7a3e )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "data0.bin",	0x000000, 0x3e53331f )
	NAMCOS2_DATA_LOAD_O_128K( "data1.bin",	0x000000, 0xd5427f11 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "voi1.bin",  0x000000, 0x080000, 0xdadf6a57 )
	ROM_LOAD( "voi2.bin",  0x080000, 0x080000, 0x81078e01 )
ROM_END

/* FINAL LAP (REV E) */
ROM_START( finallap )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "fl2mp0e",  0x000000, 0x010000, 0xed805674 )
	ROM_LOAD16_BYTE( "fl2mp1e",  0x000001, 0x010000, 0x4c1d523b )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "fl1-sp0",  0x000000, 0x010000, 0x2c5ff15d )
	ROM_LOAD16_BYTE( "fl1-sp1",  0x000001, 0x010000, 0xea9d1a2e )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "fl1-s0b",  0x00c000, 0x004000, 0xf5d76989 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "obj-0b",  0x000000, 0x80000, 0xc6986523 )
	ROM_LOAD( "obj-1b",  0x080000, 0x80000, 0x6af7d284 )
	ROM_LOAD( "obj-2b",  0x100000, 0x80000, 0xde45ca8d )
	ROM_LOAD( "obj-3b",  0x180000, 0x80000, 0xdba830a2 )

	ROM_LOAD( "obj-0b",  0x200000, 0x80000, 0xc6986523 )
	ROM_LOAD( "obj-1b",  0x280000, 0x80000, 0x6af7d284 )
	ROM_LOAD( "obj-2b",  0x300000, 0x80000, 0xde45ca8d )
	ROM_LOAD( "obj-3b",  0x380000, 0x80000, 0xdba830a2 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c0",  0x000000, 0xcd9d2966 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c1",  0x080000, 0xb0efec87 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c2",  0x100000, 0x263b8e31 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c3",  0x180000, 0xc2c56743 )
	NAMCOS2_GFXROM_LOAD_128K( "fl2-c4",  0x200000, 0x83c77a50 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c5",  0x280000, 0xab89da77 )
	NAMCOS2_GFXROM_LOAD_128K( "fl2-c6",  0x300000, 0x239bd9a0 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "fl2-sha",  0x000000, 0x5fda0b6d )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	/* No DAT files present in ZIP archive */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "fl1-v1",  0x000000, 0x020000, 0x86b21996 )
	ROM_RELOAD(  0x020000, 0x020000 )
	ROM_RELOAD(  0x040000, 0x020000 )
	ROM_RELOAD(  0x060000, 0x020000 )
	ROM_LOAD( "fl1-v2",  0x080000, 0x020000, 0x6a164647 )
	ROM_RELOAD(  0x0a0000, 0x020000 )
	ROM_RELOAD(  0x0c0000, 0x020000 )
	ROM_RELOAD(  0x0e0000, 0x020000 )
ROM_END

/* FINAL LAP (revision D) */
ROM_START( finalapd )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "fl2-mp0d",	0x000000, 0x010000, 0x3576d3aa )
	ROM_LOAD16_BYTE( "fl2-mp1d",	0x000001, 0x010000, 0x22d3906d )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "fl1-sp0",  0x000000, 0x010000, 0x2c5ff15d )
	ROM_LOAD16_BYTE( "fl1-sp1",  0x000001, 0x010000, 0xea9d1a2e )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "fl1-s0b",  0x00c000, 0x004000, 0xf5d76989 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "obj-0b",  0x000000, 0x80000, 0xc6986523 )
	ROM_LOAD( "obj-1b",  0x080000, 0x80000, 0x6af7d284 )
	ROM_LOAD( "obj-2b",  0x100000, 0x80000, 0xde45ca8d )
	ROM_LOAD( "obj-3b",  0x180000, 0x80000, 0xdba830a2 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c0",  0x000000, 0xcd9d2966 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c1",  0x080000, 0xb0efec87 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c2",  0x100000, 0x263b8e31 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c3",  0x180000, 0xc2c56743 )
	NAMCOS2_GFXROM_LOAD_128K( "fl2-c4",  0x200000, 0x83c77a50 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c5",  0x280000, 0xab89da77 )
	NAMCOS2_GFXROM_LOAD_128K( "fl2-c6",  0x300000, 0x239bd9a0 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) 				  /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "fl2-sha",  0x000000, 0x5fda0b6d )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	/* No DAT files present in ZIP archive */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "fl1-v1",  0x000000, 0x020000, 0x86b21996 )
	ROM_RELOAD(  0x020000, 0x020000 )
	ROM_RELOAD(  0x040000, 0x020000 )
	ROM_RELOAD(  0x060000, 0x020000 )
	ROM_LOAD( "fl1-v2",  0x080000, 0x020000, 0x6a164647 )
	ROM_RELOAD(  0x0a0000, 0x020000 )
	ROM_RELOAD(  0x0c0000, 0x020000 )
	ROM_RELOAD(  0x0e0000, 0x020000 )
ROM_END

/* FINAL LAP (revision C) */
ROM_START( finalapc )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "fl2-mp0c",	0x000000, 0x010000, 0xf667f2c9 )
	ROM_LOAD16_BYTE( "fl2-mp1c",	0x000001, 0x010000, 0xb8615d33 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "fl1-sp0",  0x000000, 0x010000, 0x2c5ff15d )
	ROM_LOAD16_BYTE( "fl1-sp1",  0x000001, 0x010000, 0xea9d1a2e )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "fl1-s0",  0x00c000, 0x004000, 0x1f8ff494 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) 				  /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "obj-0b",  0x000000, 0x80000, 0xc6986523 )
	ROM_LOAD( "obj-1b",  0x080000, 0x80000, 0x6af7d284 )
	ROM_LOAD( "obj-2b",  0x100000, 0x80000, 0xde45ca8d )
	ROM_LOAD( "obj-3b",  0x180000, 0x80000, 0xdba830a2 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c0",  0x000000, 0xcd9d2966 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c1",  0x080000, 0xb0efec87 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c2",  0x100000, 0x263b8e31 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c3",  0x180000, 0xc2c56743 )
	NAMCOS2_GFXROM_LOAD_128K( "fl2-c4",  0x200000, 0x83c77a50 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c5",  0x280000, 0xab89da77 )
	NAMCOS2_GFXROM_LOAD_128K( "fl2-c6",  0x300000, 0x239bd9a0 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "fl2-sha",  0x000000, 0x5fda0b6d )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	/* No DAT files present in ZIP archive */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "fl1-v1",  0x000000, 0x020000, 0x86b21996 )
	ROM_RELOAD(  0x020000, 0x020000 )
	ROM_RELOAD(  0x040000, 0x020000 )
	ROM_RELOAD(  0x060000, 0x020000 )
	ROM_LOAD( "fl1-v2",  0x080000, 0x020000, 0x6a164647 )
	ROM_RELOAD(  0x0a0000, 0x020000 )
	ROM_RELOAD(  0x0c0000, 0x020000 )
	ROM_RELOAD(  0x0e0000, 0x020000 )
ROM_END

/* FINAL LAP (Rev C - Japan) */
ROM_START( finlapjc )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "fl1_mp0c.bin",	0x000000, 0x010000, 0x63cd7304 )
	ROM_LOAD16_BYTE( "fl1_mp1c.bin",	0x000001, 0x010000, 0xcc9c5fb6 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "fl1-sp0",  0x000000, 0x010000, 0x2c5ff15d )
	ROM_LOAD16_BYTE( "fl1-sp1",  0x000001, 0x010000, 0xea9d1a2e )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "fl1_s0b",  0x00c000, 0x004000, 0xf5d76989 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "obj-0b",  0x000000, 0x80000, 0xc6986523 )
	ROM_LOAD( "obj-1b",  0x080000, 0x80000, 0x6af7d284 )
	ROM_LOAD( "obj-2b",  0x100000, 0x80000, 0xde45ca8d )
	ROM_LOAD( "obj-3b",  0x180000, 0x80000, 0xdba830a2 )

	ROM_LOAD( "obj-0b",  0x200000, 0x80000, 0xc6986523 )
	ROM_LOAD( "obj-1b",  0x280000, 0x80000, 0x6af7d284 )
	ROM_LOAD( "obj-2b",  0x300000, 0x80000, 0xde45ca8d )
	ROM_LOAD( "obj-3b",  0x380000, 0x80000, 0xdba830a2 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c0",  0x000000, 0xcd9d2966 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c1",  0x080000, 0xb0efec87 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c2",  0x100000, 0x263b8e31 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c3",  0x180000, 0xc2c56743 )
	NAMCOS2_GFXROM_LOAD_128K( "fl2-c4",  0x200000, 0x83c77a50 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c5",  0x280000, 0xab89da77 )
	NAMCOS2_GFXROM_LOAD_128K( "fl2-c6",  0x300000, 0x239bd9a0 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "fl2-sha",  0x000000, 0x5fda0b6d )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	/* No DAT files present in ZIP archive */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "fl1-v1",  0x000000, 0x020000, 0x86b21996 )
	ROM_RELOAD(  0x020000, 0x020000 )
	ROM_RELOAD(  0x040000, 0x020000 )
	ROM_RELOAD(  0x060000, 0x020000 )
	ROM_LOAD( "fl1-v2",  0x080000, 0x020000, 0x6a164647 )
	ROM_RELOAD(  0x0a0000, 0x020000 )
	ROM_RELOAD(  0x0c0000, 0x020000 )
	ROM_RELOAD(  0x0e0000, 0x020000 )
ROM_END

/* FINAL LAP  (REV B - JAPAN) */
ROM_START( finlapjb )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "fl1_mp0b.bin",	0x000000, 0x010000, 0x870a482a )
	ROM_LOAD16_BYTE( "fl1_mp1b.bin",	0x000001, 0x010000, 0xaf52c991 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "fl1-sp0",  0x000000, 0x010000, 0x2c5ff15d )
	ROM_LOAD16_BYTE( "fl1-sp1",  0x000001, 0x010000, 0xea9d1a2e )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "fl1_s0.bin",  0x00c000, 0x004000, 0x1f8ff494 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "obj-0b",  0x000000, 0x80000, 0xc6986523 )
	ROM_LOAD( "obj-1b",  0x080000, 0x80000, 0x6af7d284 )
	ROM_LOAD( "obj-2b",  0x100000, 0x80000, 0xde45ca8d )
	ROM_LOAD( "obj-3b",  0x180000, 0x80000, 0xdba830a2 )
	ROM_LOAD( "obj-0b",  0x200000, 0x80000, 0xc6986523 )
	ROM_LOAD( "obj-1b",  0x280000, 0x80000, 0x6af7d284 )
	ROM_LOAD( "obj-2b",  0x300000, 0x80000, 0xde45ca8d )
	ROM_LOAD( "obj-3b",  0x380000, 0x80000, 0xdba830a2 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c0",		0x000000, 0xcd9d2966 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c1",		0x080000, 0xb0efec87 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c2",		0x100000, 0x263b8e31 )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c3",		0x180000, 0xc2c56743 )
	NAMCOS2_GFXROM_LOAD_128K( "fl2_c4.bin",	0x200000, 0xcdc1de2e )
	NAMCOS2_GFXROM_LOAD_128K( "fl1-c5",		0x280000, 0xab89da77 )
	NAMCOS2_GFXROM_LOAD_128K( "fl2_c6.bin",	0x300000, 0x8e78a3c3 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "fl1_sha.bin",  0x000000, 0xb7e1c7a3 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	/* No DAT files present in ZIP archive */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "fl1-v1",  0x000000, 0x020000, 0x86b21996 )
	ROM_RELOAD(  0x020000, 0x020000 )
	ROM_RELOAD(  0x040000, 0x020000 )
	ROM_RELOAD(  0x060000, 0x020000 )
	ROM_LOAD( "fl1-v2",  0x080000, 0x020000, 0x6a164647 )
	ROM_RELOAD(  0x0a0000, 0x020000 )
	ROM_RELOAD(  0x0c0000, 0x020000 )
	ROM_RELOAD(  0x0e0000, 0x020000 )
ROM_END

/* FINAL LAP 2 */
ROM_START( finalap2 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "fls2mp0b",	0x000000, 0x020000, 0x97b48aae )
	ROM_LOAD16_BYTE( "fls2mp1b",	0x000001, 0x020000, 0xc9f3e0e7 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "fls2sp0b",	0x000000, 0x020000, 0x8bf15d9c )
	ROM_LOAD16_BYTE( "fls2sp1b",	0x000001, 0x020000, 0xc1a31086 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "flss0",	0x00c000, 0x004000, 0xc07cc10a )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "fl3obj0",  0x000000, 0x80000, 0xeab19ec6 )
	ROM_LOAD( "fl3obj2",  0x080000, 0x80000, 0x2a3b7ded )
	ROM_LOAD( "fl3obj4",  0x100000, 0x80000, 0x84aa500c )
	ROM_LOAD( "fl3obj6",  0x180000, 0x80000, 0x33118e63 )
	ROM_LOAD( "fl3obj1",  0x200000, 0x80000, 0x4ef37a51 )
	ROM_LOAD( "fl3obj3",  0x280000, 0x80000, 0xb86dc7cd )
	ROM_LOAD( "fl3obj5",  0x300000, 0x80000, 0x6a53e603 )
	ROM_LOAD( "fl3obj7",  0x380000, 0x80000, 0xb52a85e2 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* Tiles */
	ROM_LOAD( "fls2chr0",  0x000000, 0x40000, 0x7bbda499 )
	ROM_LOAD( "fls2chr1",  0x040000, 0x40000, 0xac8940e5 )
	ROM_LOAD( "fls2chr2",  0x080000, 0x40000, 0x1756173d )
	ROM_LOAD( "fls2chr3",  0x0c0000, 0x40000, 0x69032785 )
	ROM_LOAD( "fls2chr4",  0x100000, 0x40000, 0x8216cf42 )
	ROM_LOAD( "fls2chr5",  0x140000, 0x40000, 0xdc3e8e1c )
	ROM_LOAD( "fls2chr6",  0x180000, 0x40000, 0x1ef4bdde )
	ROM_LOAD( "fls2chr7",  0x1c0000, 0x40000, 0x53dafcde )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_256K( "fls2sha",  0x000000, 0xf7b40a85 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_256K( "fls2dat0",  0x000000, 0xf1af432c )
	NAMCOS2_DATA_LOAD_O_256K( "fls2dat1",  0x000000, 0x8719533e )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "flsvoi1",  0x000000, 0x080000, 0x590be52f )
	ROM_LOAD( "flsvoi2",  0x080000, 0x080000, 0x204b3c27 )
ROM_END

/* FINAL LAP 2 (Japan) */
ROM_START( finalp2j )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "fls1_mp0.bin",	0x000000, 0x020000, 0x05ea8090 )
	ROM_LOAD16_BYTE( "fls1_mp1.bin",	0x000001, 0x020000, 0xfb189f50 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "fls2sp0b",	0x000000, 0x020000, 0x8bf15d9c )
	ROM_LOAD16_BYTE( "fls2sp1b",	0x000001, 0x020000, 0xc1a31086 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "flss0",	0x00c000, 0x004000, 0xc07cc10a )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "fl3obj0",  0x000000, 0x80000, 0xeab19ec6 )
	ROM_LOAD( "fl3obj2",  0x080000, 0x80000, 0x2a3b7ded )
	ROM_LOAD( "fl3obj4",  0x100000, 0x80000, 0x84aa500c )
	ROM_LOAD( "fl3obj6",  0x180000, 0x80000, 0x33118e63 )
	ROM_LOAD( "fl3obj1",  0x200000, 0x80000, 0x4ef37a51 )
	ROM_LOAD( "fl3obj3",  0x280000, 0x80000, 0xb86dc7cd )
	ROM_LOAD( "fl3obj5",  0x300000, 0x80000, 0x6a53e603 )
	ROM_LOAD( "fl3obj7",  0x380000, 0x80000, 0xb52a85e2 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* Tiles */
	ROM_LOAD( "fls2chr0",  0x000000, 0x40000, 0x7bbda499 )
	ROM_LOAD( "fls2chr1",  0x040000, 0x40000, 0xac8940e5 )
	ROM_LOAD( "fls2chr2",  0x080000, 0x40000, 0x1756173d )
	ROM_LOAD( "fls2chr3",  0x0c0000, 0x40000, 0x69032785 )
	ROM_LOAD( "fls2chr4",  0x100000, 0x40000, 0x8216cf42 )
	ROM_LOAD( "fls2chr5",  0x140000, 0x40000, 0xdc3e8e1c )
	ROM_LOAD( "fls2chr6",  0x180000, 0x40000, 0x1ef4bdde )
	ROM_LOAD( "fls2chr7",  0x1c0000, 0x40000, 0x53dafcde )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "fls2sha",  0x000000, 0x95a63037 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_256K( "fls2dat0",  0x000000, 0xf1af432c )
	NAMCOS2_DATA_LOAD_O_256K( "fls2dat1",  0x000000, 0x8719533e )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "flsvoi1",  0x000000, 0x080000, 0x590be52f )
	ROM_LOAD( "flsvoi2",  0x080000, 0x080000, 0x204b3c27 )
ROM_END

/* FINAL LAP 3 */
ROM_START( finalap3 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "fltmp0",  0x000000, 0x020000, 0x2f2a997a )
	ROM_LOAD16_BYTE( "fltmp1",  0x000001, 0x020000, 0xb505ca0b )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "flt1sp0",  0x000000, 0x020000, 0xe804ced1 )
	ROM_LOAD16_BYTE( "flt1sp1",  0x000001, 0x020000, 0x3a2b24ee )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "flt1snd0",  0x00c000, 0x004000, 0x60b72aed )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "fltobj0",  0x000000, 0x80000, 0xeab19ec6 )
	ROM_LOAD( "fltobj2",  0x080000, 0x80000, 0x2a3b7ded )
	ROM_LOAD( "fltobj4",  0x100000, 0x80000, 0x84aa500c )
	ROM_LOAD( "fltobj6",  0x180000, 0x80000, 0x33118e63 )
	ROM_LOAD( "fltobj1",  0x200000, 0x80000, 0x4ef37a51 )
	ROM_LOAD( "fltobj3",  0x280000, 0x80000, 0xb86dc7cd )
	ROM_LOAD( "fltobj5",  0x300000, 0x80000, 0x6a53e603 )
	ROM_LOAD( "fltobj7",  0x380000, 0x80000, 0xb52a85e2 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "fltchr0",  0x00000, 0x20000, 0x93d58fbb ) /* incomplete! */
	ROM_LOAD( "fltchr1",  0x20000, 0x20000, 0xabbc411b ) /* incomplete! */
	ROM_LOAD( "fltchr2",  0x40000, 0x20000, 0x7de05a4a ) /* incomplete! */
	ROM_LOAD( "fltchr3",  0x60000, 0x20000, 0xac4e9b8a ) /* incomplete! */
	ROM_LOAD( "fltchr4",  0x80000, 0x20000, 0x55c3434d ) /* incomplete! */
	ROM_LOAD( "fltchr5",  0xa0000, 0x20000, 0xfbaa5c89 ) /* incomplete! */
	ROM_LOAD( "fltchr6",  0xc0000, 0x20000, 0xe90279ce ) /* incomplete! */
	ROM_LOAD( "fltchr7",  0xe0000, 0x20000, 0xb9c1ea47 ) /* incomplete! */

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "fltsha",  0x000000, 0x089dc194 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "flt1d0",  0x000000, 0x80004966 )
	NAMCOS2_DATA_LOAD_O_128K( "flt1d1",  0x000000, 0xa2e93e8c )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "fltvoi1",  0x000000, 0x080000, 0x4fc7c0ba )
	ROM_LOAD( "fltvoi2",  0x080000, 0x080000, 0x409c62df )
ROM_END

/* FINEST HOUR */
ROM_START( finehour )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "fh1_mp0.bin",  0x000000, 0x020000, 0x355d9119 )
	ROM_LOAD16_BYTE( "fh1_mp1.bin",  0x000001, 0x020000, 0x647eb621 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "fh1_sp0.bin",  0x000000, 0x020000, 0xaa6289e9 )
	ROM_LOAD16_BYTE( "fh1_sp1.bin",  0x000001, 0x020000, 0x8532d5c7 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "fh1_sd0.bin",  0x00c000, 0x004000, 0x059a9cfd )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "fh1_ob0.bin",  0x000000, 0x80000, 0xb1fd86f1 )
	ROM_LOAD( "fh1_ob1.bin",  0x080000, 0x80000, 0x519c44ce )
	ROM_LOAD( "fh1_ob2.bin",  0x100000, 0x80000, 0x9c5de4fa )
	ROM_LOAD( "fh1_ob3.bin",  0x180000, 0x80000, 0x54d4edce )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_256K( "fh1_ch0.bin",  0x000000, 0x516900d1 )
	NAMCOS2_GFXROM_LOAD_256K( "fh1_ch1.bin",  0x080000, 0x964d06bd )
	NAMCOS2_GFXROM_LOAD_256K( "fh1_ch2.bin",  0x100000, 0xfbb9449e )
	NAMCOS2_GFXROM_LOAD_256K( "fh1_ch3.bin",  0x180000, 0xc18eda8a )
	NAMCOS2_GFXROM_LOAD_256K( "fh1_ch4.bin",  0x200000, 0x80dd188a )
	NAMCOS2_GFXROM_LOAD_256K( "fh1_ch5.bin",  0x280000, 0x40969876 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "fh1_rz0.bin",  0x000000, 0x6c96c5c1 )
	NAMCOS2_GFXROM_LOAD_128K( "fh1_rz1.bin",  0x080000, 0x44699eb9 )
	NAMCOS2_GFXROM_LOAD_128K( "fh1_rz2.bin",  0x100000, 0x5ec14abf )
	NAMCOS2_GFXROM_LOAD_128K( "fh1_rz3.bin",  0x180000, 0x9f5a91b2 )
	NAMCOS2_GFXROM_LOAD_128K( "fh1_rz4.bin",  0x200000, 0x0b4379e6 )
	NAMCOS2_GFXROM_LOAD_128K( "fh1_rz5.bin",  0x280000, 0xe034e560 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_256K( "fh1_sha.bin",  0x000000, 0x15875eb0 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "fh1_dt0.bin",  0x000000, 0x2441c26f )
	NAMCOS2_DATA_LOAD_O_128K( "fh1_dt1.bin",  0x000000, 0x48154deb )
	NAMCOS2_DATA_LOAD_E_128K( "fh1_dt2.bin",  0x100000, 0x12453ba4 )
	NAMCOS2_DATA_LOAD_O_128K( "fh1_dt3.bin",  0x100000, 0x50bab9da )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "fh1_vo1.bin",  0x000000, 0x080000, 0x07560fc7 )
ROM_END

/* FOUR TRAX */
ROM_START( fourtrax )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "fx2mp0",  0x000000, 0x020000, 0xf147cd6b )
	ROM_LOAD16_BYTE( "fx2mp1",  0x000001, 0x020000, 0x8af4a309 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "fx2sp0",  0x000000, 0x020000, 0x48548e78 )
	ROM_LOAD16_BYTE( "fx2sp1",  0x000001, 0x020000, 0xd2861383 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "fx1sd0",  0x00c000, 0x004000, 0xacccc934 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "fxobj0",  0x000000, 0x040000, 0x1aa60ffa )
	ROM_LOAD( "fxobj1",  0x040000, 0x040000, 0x7509bc09 )
	ROM_LOAD( "fxobj4",  0x080000, 0x040000, 0x30add52a )
	ROM_LOAD( "fxobj5",  0x0c0000, 0x040000, 0xe3cd2776 )
	ROM_LOAD( "fxobj8",  0x100000, 0x040000, 0xb165acab )
	ROM_LOAD( "fxobj9",  0x140000, 0x040000, 0x90f0735b )
	ROM_LOAD( "fxobj12", 0x180000, 0x040000, 0xf5e23b78 )
	ROM_LOAD( "fxobj13", 0x1c0000, 0x040000, 0x04a25007 )
	ROM_LOAD( "fxobj2",  0x200000, 0x040000, 0x243affc7 )
	ROM_LOAD( "fxobj3",  0x240000, 0x040000, 0xb7e5d17d )
	ROM_LOAD( "fxobj6",  0x280000, 0x040000, 0xa2d5ce4a )
	ROM_LOAD( "fxobj7",  0x2c0000, 0x040000, 0x4d91c929 )
	ROM_LOAD( "fxobj10", 0x300000, 0x040000, 0x7a01e86f )
	ROM_LOAD( "fxobj11", 0x340000, 0x040000, 0x514b3fe5 )
	ROM_LOAD( "fxobj14", 0x380000, 0x040000, 0xc1658c77 )
	ROM_LOAD( "fxobj15", 0x3c0000, 0x040000, 0x2bc909b3 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "fxchr0",  0x000000, 0x6658c1c3 )
	NAMCOS2_GFXROM_LOAD_128K( "fxchr1",  0x080000, 0x3a888943 )
	NAMCOS2_GFXROM_LOAD_128K( "fxch2",	 0x100000, 0xfdf1e86b )
	NAMCOS2_GFXROM_LOAD_128K( "fxchr3",  0x180000, 0x47fa7e61 )
	NAMCOS2_GFXROM_LOAD_128K( "fxchr4",  0x200000, 0xc720c5f5 )
	NAMCOS2_GFXROM_LOAD_128K( "fxchr5",  0x280000, 0x9eacdbc8 )
	NAMCOS2_GFXROM_LOAD_128K( "fxchr6",  0x300000, 0xc3dba42e )
	NAMCOS2_GFXROM_LOAD_128K( "fxchr7",  0x380000, 0xc009f3ae )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	/* No ROZ files in zip, not sure if they are missing ? */

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "fxsha",	0x000000, 0xf7aa4af7 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_256K( "fxdat0",  0x000000, 0x63abf69b )
	NAMCOS2_DATA_LOAD_O_256K( "fxdat1",  0x000000, 0x725bed14 )
	NAMCOS2_DATA_LOAD_E_256K( "fxdat2",  0x100000, 0x71e4a5a0 )
	NAMCOS2_DATA_LOAD_O_256K( "fxdat3",  0x100000, 0x605725f7 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "fxvoi1",  0x000000, 0x080000, 0x6173364f )
ROM_END


/* MARVEL LAND (USA) */
ROM_START( marvland )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "mv2_mpr0",	0x000000, 0x020000, 0xd8b14fee )
	ROM_LOAD16_BYTE( "mv2_mpr1",	0x000001, 0x020000, 0x29ff2738 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "mv2_spr0",	0x000000, 0x010000, 0xaa418f29 )
	ROM_LOAD16_BYTE( "mv2_spr1",	0x000001, 0x010000, 0xdbd94def )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "mv2_snd0",  0x0c000, 0x04000, 0xa5b99162 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	NAMCOS2_GFXROM_LOAD_256K( "mv1-obj0.bin",  0x000000, 0x73a29361 )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-obj1.bin",  0x080000, 0xabbe4a99 )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-obj2.bin",  0x100000, 0x753659e0 )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-obj3.bin",  0x180000, 0xd1ce7339 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_256K( "mv1-chr0.bin",  0x000000, 0x1c7e8b4f )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-chr1.bin",  0x080000, 0x01e4cafd )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-chr2.bin",  0x100000, 0x198fcc6f )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-chr3.bin",  0x180000, 0xed6f22a5 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "mv1-roz0.bin",  0x000000, 0x7381a5a9 )
	NAMCOS2_GFXROM_LOAD_128K( "mv1-roz1.bin",  0x080000, 0xe899482e )
	NAMCOS2_GFXROM_LOAD_128K( "mv1-roz2.bin",  0x100000, 0xde141290 )
	NAMCOS2_GFXROM_LOAD_128K( "mv1-roz3.bin",  0x180000, 0xe310324d )
	NAMCOS2_GFXROM_LOAD_128K( "mv1-roz4.bin",  0x200000, 0x48ddc5a9 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_256K( "mv1-sha.bin",  0x000000, 0xa47db5d3 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "mv2_dat0",  0x000000, 0x62e6318b )
	NAMCOS2_DATA_LOAD_O_128K( "mv2_dat1",  0x000000, 0x8a6902ca )
	NAMCOS2_DATA_LOAD_E_128K( "mv2_dat2",  0x100000, 0xf5c6408c )
	NAMCOS2_DATA_LOAD_O_128K( "mv2_dat3",  0x100000, 0x6df76955 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "mv1-voi1.bin",  0x000000, 0x080000, 0xde5cac09 )
ROM_END

/* MARVEL LAND (JAPAN) */
ROM_START( marvlanj )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "mv1-mpr0.bin",	0x000000, 0x010000, 0x8369120f )
	ROM_LOAD16_BYTE( "mv1-mpr1.bin",	0x000001, 0x010000, 0x6d5442cc )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "mv1-spr0.bin",	0x000000, 0x010000, 0xc3909925 )
	ROM_LOAD16_BYTE( "mv1-spr1.bin",	0x000001, 0x010000, 0x1c5599f5 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "mv1-snd0.bin",  0x0c000, 0x04000, 0x51b8ccd7 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	NAMCOS2_GFXROM_LOAD_256K( "mv1-obj0.bin",  0x000000, 0x73a29361 )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-obj1.bin",  0x080000, 0xabbe4a99 )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-obj2.bin",  0x100000, 0x753659e0 )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-obj3.bin",  0x180000, 0xd1ce7339 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_256K( "mv1-chr0.bin",  0x000000, 0x1c7e8b4f )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-chr1.bin",  0x080000, 0x01e4cafd )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-chr2.bin",  0x100000, 0x198fcc6f )
	NAMCOS2_GFXROM_LOAD_256K( "mv1-chr3.bin",  0x180000, 0xed6f22a5 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "mv1-roz0.bin",  0x000000, 0x7381a5a9 )
	NAMCOS2_GFXROM_LOAD_128K( "mv1-roz1.bin",  0x080000, 0xe899482e )
	NAMCOS2_GFXROM_LOAD_128K( "mv1-roz2.bin",  0x100000, 0xde141290 )
	NAMCOS2_GFXROM_LOAD_128K( "mv1-roz3.bin",  0x180000, 0xe310324d )
	NAMCOS2_GFXROM_LOAD_128K( "mv1-roz4.bin",  0x200000, 0x48ddc5a9 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_256K( "mv1-sha.bin",  0x000000, 0xa47db5d3 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "mv1-dat0.bin",  0x000000, 0xe15f412e )
	NAMCOS2_DATA_LOAD_O_128K( "mv1-dat1.bin",  0x000000, 0x73e1545a )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "mv1-voi1.bin",  0x000000, 0x080000, 0xde5cac09 )
ROM_END

/* METAL HAWK */
ROM_START( metlhawk )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "mh2mp0c.11d",  0x000000, 0x020000, 0xcd7dae6e )
	ROM_LOAD16_BYTE( "mh2mp1c.13d",  0x000001, 0x020000, 0xe52199fd )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "mh1sp0f.11k",  0x000000, 0x010000, 0x2c141fea )
	ROM_LOAD16_BYTE( "mh1sp1f.13k",  0x000001, 0x010000, 0x8ccf98e0 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "mh1s0.7j",  0x0c000, 0x04000, 0x79e054cf )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD32_BYTE( "mhobj-4.5c", 0x000000, 0x40000, 0xe3590e1a )
	ROM_LOAD32_BYTE( "mhobj-5.5a", 0x000001, 0x40000, 0xb85c0d07 )
	ROM_LOAD32_BYTE( "mhobj-6.6c", 0x000002, 0x40000, 0x90c4523d )
	ROM_LOAD32_BYTE( "mhobj-7.6a", 0x000003, 0x40000, 0xf00edb39 )
	ROM_LOAD32_BYTE( "mhobj-0.5d", 0x100000, 0x40000, 0x52ae6620 )
	ROM_LOAD32_BYTE( "mhobj-1.5b", 0x100001, 0x40000, 0x2c2a1291 )
	ROM_LOAD32_BYTE( "mhobj-2.6d", 0x100002, 0x40000, 0x6221b927 )
	ROM_LOAD32_BYTE( "mhobj-3.6b", 0x100003, 0x40000, 0xfd09f2f1 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "mhchr-0.11n",  0x000000, 0xe2da1b14 )
	NAMCOS2_GFXROM_LOAD_128K( "mhchr-1.11p",  0x080000, 0x023c78f9 )
	NAMCOS2_GFXROM_LOAD_128K( "mhchr-2.11r",  0x100000, 0xece47e91 )
	NAMCOS2_GFXROM_LOAD_128K( "mh1c3.11s",    0x180000, 0x9303aa7f )

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* ROZ Tiles */
	ROM_LOAD( "mhr0z-0.2d",  0x000000, 0x40000, 0x30ade98f )
	ROM_LOAD( "mhr0z-1.2c",  0x040000, 0x40000, 0xa7fff42a )
	ROM_LOAD( "mhr0z-2.2b",  0x080000, 0x40000, 0x6abec820 )
	ROM_LOAD( "mhr0z-3.2a",  0x0c0000, 0x40000, 0xd53cec6d )
	ROM_LOAD( "mhr0z-4.3d",  0x100000, 0x40000, 0x922213e2 )
	ROM_LOAD( "mhr0z-5.3c",  0x140000, 0x40000, 0x78418a54 )
	ROM_LOAD( "mhr0z-6.3b",  0x180000, 0x40000, 0x6c74977e )
	ROM_LOAD( "mhr0z-7.3a",  0x1c0000, 0x40000, 0x68a19cbd )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape (tiles) */
	NAMCOS2_GFXROM_LOAD_128K( "mh1sha.7n",	0x000000, 0x6ac22294 )

	ROM_REGION( 0x40000, REGION_GFX5, 0 ) /* Mask shape (ROZ) */
	ROM_LOAD( "mh-rzsh.bin",	0x000000, 0x40000, 0x5090b1cf )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "mh1d0.13s",	0x000000, 0x8b178ac7 )
	NAMCOS2_DATA_LOAD_O_128K( "mh1d1.13p",	0x000000, 0x10684fd6 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "mhvoi-1.bin",  0x000000, 0x080000, 0x2723d137 )
	ROM_LOAD( "mhvoi-2.bin",  0x080000, 0x080000, 0xdbc92d91 )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* sprite zoom lookup table */
	ROM_LOAD( "mh5762.7p",    0x00000,  0x002000, 0x90db1bf6 )
ROM_END

/* MIRAI NINJA */
ROM_START( mirninja )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "mn_mpr0e.bin",	0x000000, 0x010000, 0xfa75f977 )
	ROM_LOAD16_BYTE( "mn_mpr1e.bin",	0x000001, 0x010000, 0x58ddd464 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "mn1_spr0.bin",	0x000000, 0x010000, 0x3f1a17be )
	ROM_LOAD16_BYTE( "mn1_spr1.bin",	0x000001, 0x010000, 0x2bc66f60 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "mn_snd0.bin",  0x0c000, 0x04000, 0x6aa1ae84 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65b.bin",  0x008000, 0x008000, 0xe9f2922a )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	NAMCOS2_GFXROM_LOAD_128K( "mn_obj0.bin",  0x000000, 0x6bd1e290 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_obj1.bin",  0x080000, 0x5e8503be )
	NAMCOS2_GFXROM_LOAD_128K( "mn_obj2.bin",  0x100000, 0xa96d9b45 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_obj3.bin",  0x180000, 0x0086ef8b )
	NAMCOS2_GFXROM_LOAD_128K( "mn_obj4.bin",  0x200000, 0xb3f48755 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_obj5.bin",  0x280000, 0xc21e995b )
	NAMCOS2_GFXROM_LOAD_128K( "mn_obj6.bin",  0x300000, 0xa052c582 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_obj7.bin",  0x380000, 0x1854c6f5 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "mn_chr0.bin",  0x000000, 0x4f66df26 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_chr1.bin",  0x080000, 0xf5de5ea7 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_chr2.bin",  0x100000, 0x9ff61924 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_chr3.bin",  0x180000, 0xba208bf5 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_chr4.bin",  0x200000, 0x0ef00aff )
	NAMCOS2_GFXROM_LOAD_128K( "mn_chr5.bin",  0x280000, 0x4cd9d377 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_chr6.bin",  0x300000, 0x114aca76 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_chr7.bin",  0x380000, 0x2d5705d3 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "mn_roz0.bin",  0x000000, 0x677a4f25 )
	NAMCOS2_GFXROM_LOAD_128K( "mn_roz1.bin",  0x080000, 0xf00ae572 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "mn_sha.bin",  0x000000, 0xc28af90f )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "mn1_dat0.bin",  0x000000, 0x104bcca8 )
	NAMCOS2_DATA_LOAD_O_128K( "mn1_dat1.bin",  0x000000, 0xd2a918fb )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "mn_voi1.bin",  0x000000, 0x080000, 0x2ca3573c )
	ROM_LOAD( "mn_voi2.bin",  0x080000, 0x080000, 0x466c3b47 )
ROM_END

/* ORDYNE */
ROM_START( ordyne )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "or1_mp0.bin",  0x000000, 0x020000, 0xf5929ed3 )
	ROM_LOAD16_BYTE( "or1_mp1.bin",  0x000001, 0x020000, 0xc1c8c1e2 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "or1_sp0.bin",  0x000000, 0x010000, 0x01ef6638 )
	ROM_LOAD16_BYTE( "or1_sp1.bin",  0x000001, 0x010000, 0xb632adc3 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "or1_sd0.bin",  0x00c000, 0x004000, 0xc41e5d22 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	NAMCOS2_GFXROM_LOAD_128K( "or1_ob0.bin",  0x000000, 0x67b2b9e4 )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ob1.bin",  0x080000, 0x8a54fa5e )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ob2.bin",  0x100000, 0xa2c1cca0 )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ob3.bin",  0x180000, 0xe0ad292c )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ob4.bin",  0x200000, 0x7aefba59 )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ob5.bin",  0x280000, 0xe4025be9 )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ob6.bin",  0x300000, 0xe284c30c )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ob7.bin",  0x380000, 0x262b7112 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "or1_ch0.bin",  0x000000, 0xe7c47934 )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ch1.bin",  0x080000, 0x874b332d )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ch3.bin",  0x180000, 0x5471a834 )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ch5.bin",  0x280000, 0xa7d3a296 )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ch6.bin",  0x300000, 0x3adc09c8 )
	NAMCOS2_GFXROM_LOAD_128K( "or1_ch7.bin",  0x380000, 0xf050a152 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "or1_rz0.bin",  0x000000, 0xc88a9e6b )
	NAMCOS2_GFXROM_LOAD_128K( "or1_rz1.bin",  0x080000, 0xc20cc749 )
	NAMCOS2_GFXROM_LOAD_128K( "or1_rz2.bin",  0x100000, 0x148c9866 )
	NAMCOS2_GFXROM_LOAD_128K( "or1_rz3.bin",  0x180000, 0x4e727b7e )
	NAMCOS2_GFXROM_LOAD_128K( "or1_rz4.bin",  0x200000, 0x17b04396 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "or1_sha.bin",  0x000000, 0x7aec9dee )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "or1_dt0.bin",  0x000000, 0xde214f7a )
	NAMCOS2_DATA_LOAD_O_128K( "or1_dt1.bin",  0x000000, 0x25e3e6c8 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "or1_vo1.bin",  0x000000, 0x080000, 0x369e0bca )
	ROM_LOAD( "or1_vo2.bin",  0x080000, 0x080000, 0x9f4cd7b5 )
ROM_END

/* PHELIOS */
ROM_START( phelios )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "ps1mpr0.bin",  0x000000, 0x020000, 0xbfbe96c6 )
	ROM_LOAD16_BYTE( "ps1mpr1.bin",  0x000001, 0x020000, 0xf5c0f883 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "ps1spr0.bin",  0x000000, 0x010000, 0xe9c6987e )
	ROM_LOAD16_BYTE( "ps1spr1.bin",  0x000001, 0x010000, 0x02b074fb )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "ps1snd1.bin",  0x00c000, 0x004000, 0xda694838 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	NAMCOS2_GFXROM_LOAD_256K( "psobj0.bin",  0x000000, 0xf323db2b )
	NAMCOS2_GFXROM_LOAD_256K( "psobj1.bin",  0x080000, 0xfaf7c2f5 )
	NAMCOS2_GFXROM_LOAD_256K( "psobj2.bin",  0x100000, 0x828178ba )
	NAMCOS2_GFXROM_LOAD_256K( "psobj3.bin",  0x180000, 0xe84771c8 )
	NAMCOS2_GFXROM_LOAD_256K( "psobj4.bin",  0x200000, 0x81ea86c6 )
	NAMCOS2_GFXROM_LOAD_256K( "psobj5.bin",  0x280000, 0xaaebd51a )
	NAMCOS2_GFXROM_LOAD_256K( "psobj6.bin",  0x300000, 0x032ea497 )
	NAMCOS2_GFXROM_LOAD_256K( "psobj7.bin",  0x380000, 0xf6183b36 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "pschr0.bin",  0x000000, 0x668b6670 )
	NAMCOS2_GFXROM_LOAD_128K( "pschr1.bin",  0x080000, 0x80c30742 )
	NAMCOS2_GFXROM_LOAD_128K( "pschr2.bin",  0x100000, 0xf4e11bf7 )
	NAMCOS2_GFXROM_LOAD_128K( "pschr3.bin",  0x180000, 0x97a93dc5 )
	NAMCOS2_GFXROM_LOAD_128K( "pschr4.bin",  0x200000, 0x81d965bf )
	NAMCOS2_GFXROM_LOAD_128K( "pschr5.bin",  0x280000, 0x8ca72d35 )
	NAMCOS2_GFXROM_LOAD_128K( "pschr6.bin",  0x300000, 0xda3543a9 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "psroz0.bin",  0x000000, 0x68043d7e )
	NAMCOS2_GFXROM_LOAD_128K( "psroz1.bin",  0x080000, 0x029802b4 )
	NAMCOS2_GFXROM_LOAD_128K( "psroz2.bin",  0x100000, 0xbf0b76dc )
	NAMCOS2_GFXROM_LOAD_128K( "psroz3.bin",  0x180000, 0x9c821455 )
	NAMCOS2_GFXROM_LOAD_128K( "psroz4.bin",  0x200000, 0x63a39b7a )
	NAMCOS2_GFXROM_LOAD_128K( "psroz5.bin",  0x280000, 0xfc5a99d0 )
	NAMCOS2_GFXROM_LOAD_128K( "psroz6.bin",  0x300000, 0xa2a17587 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "ps1-sha.bin",  0x000000, 0x58e26fcf )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "ps1dat0.bin",  0x000000, 0xee4194b0 )
	NAMCOS2_DATA_LOAD_O_128K( "ps1dat1.bin",  0x000000, 0x5b22d714 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "psvoi-1.bin",  0x000000, 0x080000, 0xf67376ed )
ROM_END

/* ROLLING THUNDER 2 */
ROM_START( rthun2 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "mpr0.bin",	0x000000, 0x020000, 0xe09a3549 )
	ROM_LOAD16_BYTE( "mpr1.bin",	0x000001, 0x020000, 0x09573bff )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "spr0.bin",	0x000000, 0x010000, 0x54c22ac5 )
	ROM_LOAD16_BYTE( "spr1.bin",	0x000001, 0x010000, 0x060eb393 )

	ROM_REGION( 0x050000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "snd0.bin",  0x00c000, 0x004000, 0x55b7562a )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )
	ROM_LOAD( "snd1.bin",  0x030000, 0x020000, 0x00445a4f )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "obj0.bin",  0x000000, 0x80000, 0xe5cb82c1 )
	ROM_LOAD( "obj1.bin",  0x080000, 0x80000, 0x19ebe9fd )
	ROM_LOAD( "obj2.bin",  0x100000, 0x80000, 0x455c4a2f )
	ROM_LOAD( "obj3.bin",  0x180000, 0x80000, 0xfdcae8a9 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "chr0.bin",  0x000000, 0x80000, 0x6f0e9a68 )
	ROM_LOAD( "chr1.bin",  0x080000, 0x80000, 0x15e44adc )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	ROM_LOAD( "roz0.bin",  0x000000, 0x80000, 0x482d0554 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "shape.bin",	0x000000, 0x80000, 0xcf58fbbe )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "data0.bin",	0x000000, 0x0baf44ee )
	NAMCOS2_DATA_LOAD_O_128K( "data1.bin",	0x000000, 0x58a8daac )
	NAMCOS2_DATA_LOAD_E_128K( "data2.bin",	0x100000, 0x8e850a2a )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "voi1.bin",  0x000000, 0x080000, 0xe42027cd )
	ROM_LOAD( "voi2.bin",  0x080000, 0x080000, 0x0c4c2b66 )
ROM_END

/* ROLLING THUNDER 2 (Japan) */
ROM_START( rthun2j )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "mpr0j.bin",  0x000000, 0x020000, 0x2563b9ee )
	ROM_LOAD16_BYTE( "mpr1j.bin",  0x000001, 0x020000, 0x14c4c564 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "spr0j.bin",  0x000000, 0x010000, 0xf8ef5150 )
	ROM_LOAD16_BYTE( "spr1j.bin",  0x000001, 0x010000, 0x52ed3a48 )

	ROM_REGION( 0x050000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "snd0.bin",  0x00c000, 0x004000, 0x55b7562a )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )
	ROM_LOAD( "snd1.bin",  0x030000, 0x020000, 0x00445a4f )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "obj0.bin",  0x000000, 0x80000, 0xe5cb82c1 )
	ROM_LOAD( "obj1.bin",  0x080000, 0x80000, 0x19ebe9fd )
	ROM_LOAD( "obj2.bin",  0x100000, 0x80000, 0x455c4a2f )
	ROM_LOAD( "obj3.bin",  0x180000, 0x80000, 0xfdcae8a9 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "chr0.bin",  0x000000, 0x80000, 0x6f0e9a68 )
	ROM_LOAD( "chr1.bin",  0x080000, 0x80000, 0x15e44adc )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	ROM_LOAD( "roz0.bin",  0x000000, 0x80000, 0x482d0554 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "shape.bin",	0x000000, 0x80000, 0xcf58fbbe )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "data0.bin",	0x000000, 0x0baf44ee )
	NAMCOS2_DATA_LOAD_O_128K( "data1.bin",	0x000000, 0x58a8daac )
	NAMCOS2_DATA_LOAD_E_128K( "data2.bin",	0x100000, 0x8e850a2a )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "voi1.bin",  0x000000, 0x080000, 0xe42027cd )
	ROM_LOAD( "voi2.bin",  0x080000, 0x080000, 0x0c4c2b66 )
ROM_END

/* STEEL GUNNER 2 */
ROM_START( sgunner2)
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "sns_mpr0.bin",	0x000000, 0x020000, 0xf1a44039 )
	ROM_LOAD16_BYTE( "sns_mpr1.bin",	0x000001, 0x020000, 0x9184c4db )

	/* for alternate set (not yet hooked up/tested): */
//	ROM_LOAD16_BYTE( "sns_mpr0.bin",	0x000000, 0x020000, 0xe7216ad7 )
//	ROM_LOAD16_BYTE( "sns_mpr1.bin",	0x000001, 0x020000, 0x6caef2ee )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "sns_spr0.bin",	0x000000, 0x010000, 0xe5e40ed0 )
	ROM_LOAD16_BYTE( "sns_spr1.bin",	0x000001, 0x010000, 0x3a85a5e9 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "sns_snd0.bin",  0x00c000, 0x004000, 0xf079cd32 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "sns_obj0.bin",  0x000000, 0x80000, 0xc762445c )
	ROM_LOAD( "sns_obj1.bin",  0x100000, 0x80000, 0xe9e379d8 )
	ROM_LOAD( "sns_obj2.bin",  0x200000, 0x80000, 0x0d076f6c )
	ROM_LOAD( "sns_obj3.bin",  0x300000, 0x80000, 0x0fb01e8b )
	ROM_LOAD( "sns_obj4.bin",  0x080000, 0x80000, 0x0b1be894 )
	ROM_LOAD( "sns_obj5.bin",  0x180000, 0x80000, 0x416b14e1 )
	ROM_LOAD( "sns_obj6.bin",  0x280000, 0x80000, 0xc2e94ed2 )
	ROM_LOAD( "sns_obj7.bin",  0x380000, 0x80000, 0xfc1f26af )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "sns_chr0.bin",  0x000000, 0x80000, 0xcdc42b61 )
	ROM_LOAD( "sns_chr1.bin",  0x080000, 0x80000, 0x42d4cbb7 )
	ROM_LOAD( "sns_chr2.bin",  0x100000, 0x80000, 0x7dbaa14e )
	ROM_LOAD( "sns_chr3.bin",  0x180000, 0x80000, 0xb562ff72 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	/* NO ROZ ROMS PRESENT IN ZIP */

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "sns_sha0.bin",  0x000000, 0x80000, 0x0374fd67 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "sns_dat0.bin",  0x000000, 0x48295d93 )
	NAMCOS2_DATA_LOAD_O_128K( "sns_dat1.bin",  0x000000, 0xb44cc656 )
	NAMCOS2_DATA_LOAD_E_128K( "sns_dat2.bin",  0x100000, 0xca2ae645 )
	NAMCOS2_DATA_LOAD_O_128K( "sns_dat3.bin",  0x100000, 0x203bb018 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "sns_voi1.bin",  0x000000, 0x080000, 0x219c97f7 )
	ROM_LOAD( "sns_voi2.bin",  0x080000, 0x080000, 0x562ec86b )
ROM_END

/* SUPER WORLD STADIUM 92 */
ROM_START( sws92 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "sss1mpr0.bin",	0x000000, 0x020000, 0xdbea0210 )
	ROM_LOAD16_BYTE( "sss1mpr1.bin",	0x000001, 0x020000, 0xb5e6469a )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "sst1spr0.bin",	0x000000, 0x020000, 0x9777ee2f )
	ROM_LOAD16_BYTE( "sst1spr1.bin",	0x000001, 0x020000, 0x27a35c69 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "sst1snd0.bin",  0x00c000, 0x004000, 0x8fc45114 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c68.bin",  0x008000, 0x008000, 0xca64550a )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "sss_obj0.bin",  0x000000, 0x80000, 0x375e8f1f )
	ROM_LOAD( "sss_obj1.bin",  0x080000, 0x80000, 0x675c1014 )
	ROM_LOAD( "sss_obj2.bin",  0x100000, 0x80000, 0xbdc55f1c )
	ROM_LOAD( "sss_obj3.bin",  0x180000, 0x80000, 0xe32ac432 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "sss_chr0.bin",  0x000000, 0x80000, 0x1d2876f2 )
	ROM_LOAD( "sss_chr6.bin",  0x300000, 0x80000, 0x354f0ed2 )
	ROM_LOAD( "sss_chr7.bin",  0x380000, 0x80000, 0x4032f4c1 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	ROM_LOAD( "ss_roz0.bin",  0x000000, 0x80000, 0x40ce9a58 )
	ROM_LOAD( "ss_roz1.bin",  0x080000, 0x80000, 0xc98902ff )
	ROM_LOAD( "sss_roz2.bin", 0x100000, 0x80000, 0xc9855c10 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "sss_sha0.bin",  0x000000, 0x80000, 0xb71a731a )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_256K( "sss1dat0.bin",  0x000000, 0xdb3e6aec )
	NAMCOS2_DATA_LOAD_O_256K( "sss1dat1.bin",  0x000000, 0x463b5ba8 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "ss_voi1.bin",  0x000000, 0x080000, 0x503e51b7 )
ROM_END

/* SUPER WORLD STADIUM 92 */
ROM_START( sws92g )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "ssg1mpr0.bin",	0x000000, 0x020000, 0x5596c535 )
	ROM_LOAD16_BYTE( "ssg1mpr1.bin",	0x000001, 0x020000, 0x3289ef0c )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "sst1spr0.bin",	0x000000, 0x020000, 0x9777ee2f )
	ROM_LOAD16_BYTE( "sst1spr1.bin",	0x000001, 0x020000, 0x27a35c69 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "sst1snd0.bin",  0x00c000, 0x004000, 0x8fc45114 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c68.bin",  0x008000, 0x008000, 0xca64550a )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "sss_obj0.bin",  0x000000, 0x80000, 0x375e8f1f )
	ROM_LOAD( "sss_obj1.bin",  0x080000, 0x80000, 0x675c1014 )
	ROM_LOAD( "sss_obj2.bin",  0x100000, 0x80000, 0xbdc55f1c )
	ROM_LOAD( "sss_obj3.bin",  0x180000, 0x80000, 0xe32ac432 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "sss_chr0.bin",  0x000000, 0x80000, 0x1d2876f2 )
	ROM_LOAD( "sss_chr6.bin",  0x300000, 0x80000, 0x354f0ed2 )
	ROM_LOAD( "sss_chr7.bin",  0x380000, 0x80000, 0x4032f4c1 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	ROM_LOAD( "ss_roz0.bin",  0x000000, 0x80000, 0x40ce9a58 )
	ROM_LOAD( "ss_roz1.bin",  0x080000, 0x80000, 0xc98902ff )
	ROM_LOAD( "sss_roz2.bin", 0x100000, 0x80000, 0xc9855c10 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "sss_sha0.bin",  0x000000, 0x80000, 0xb71a731a )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_256K( "sss1dat0.bin",  0x000000, 0xdb3e6aec )
	NAMCOS2_DATA_LOAD_O_256K( "sss1dat1.bin",  0x000000, 0x463b5ba8 )
	NAMCOS2_DATA_LOAD_E_256K( "ssg1dat2.bin",  0x080000, 0x754128aa )
	NAMCOS2_DATA_LOAD_O_256K( "ssg1dat3.bin",  0x080000, 0xcb3fed01 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "ss_voi1.bin",  0x000000, 0x080000, 0x503e51b7 )
ROM_END

/* SUPER WORLD STADIUM 93 */
ROM_START( sws93 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "sst1mpr0.bin",	0x000000, 0x020000, 0xbd2679bc )
	ROM_LOAD16_BYTE( "sst1mpr1.bin",	0x000001, 0x020000, 0x9132e220 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "sst1spr0.bin",	0x000000, 0x020000, 0x9777ee2f )
	ROM_LOAD16_BYTE( "sst1spr1.bin",	0x000001, 0x020000, 0x27a35c69 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "sst1snd0.bin",  0x00c000, 0x004000, 0x8fc45114 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "sst_obj0.bin",  0x000000, 0x80000, 0x4089dfd7 )
	ROM_LOAD( "sst_obj1.bin",  0x080000, 0x80000, 0xcfbc25c7 )
	ROM_LOAD( "sst_obj2.bin",  0x100000, 0x80000, 0x61ed3558 )
	ROM_LOAD( "sst_obj3.bin",  0x180000, 0x80000, 0x0e3bc05d )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "sst_chr0.bin",  0x000000, 0x80000, 0x3397850d )
	ROM_LOAD( "sss_chr6.bin",  0x300000, 0x80000, 0x354f0ed2 )
	ROM_LOAD( "sst_chr7.bin",  0x380000, 0x80000, 0xe0abb763 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	ROM_LOAD( "ss_roz0.bin",  0x000000, 0x80000, 0x40ce9a58 )
	ROM_LOAD( "ss_roz1.bin",  0x080000, 0x80000, 0xc98902ff )
	ROM_LOAD( "sss_roz2.bin", 0x100000, 0x80000, 0xc9855c10 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "sst_sha0.bin", 0x000000, 0x80000, 0x4f64d4bd )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_512K( "sst1dat0.bin",  0x000000, 0xb99c9656 )
	NAMCOS2_DATA_LOAD_O_512K( "sst1dat1.bin",  0x000000, 0x60cf6281 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "ss_voi1.bin",  0x000000, 0x080000, 0x503e51b7 )
ROM_END

/* SUZUKA 8 HOURS */
ROM_START( suzuka8h )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "eh1-mp0b.bin",	0x000000, 0x020000, 0x2850f469 )
	ROM_LOAD16_BYTE( "eh1-mpr1.bin",	0x000001, 0x020000, 0xbe83eb2c )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "eh1-sp0.bin",  0x000000, 0x020000, 0x4a8c4709 )
	ROM_LOAD16_BYTE( "eh1-sp1.bin",  0x000001, 0x020000, 0x2256b14e )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "eh1-snd0.bin",  0x00c000, 0x004000, 0x36748d3c )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, BADCRC(0xa5b2a4ff))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "eh1-obj0.bin",  0x000000, 0x80000, 0x864b6816 )
	ROM_LOAD( "eh1-obj1.bin",  0x100000, 0x80000, 0xd4921c35 )
	ROM_LOAD( "eh1-obj2.bin",  0x200000, 0x80000, 0x966d3f19 )
	ROM_LOAD( "eh1-obj3.bin",  0x300000, 0x80000, 0x7d253cbe )
	ROM_LOAD( "eh1-obj4.bin",  0x080000, 0x80000, 0xcde13867 )
	ROM_LOAD( "eh1-obj5.bin",  0x180000, 0x80000, 0x9f210546 )
	ROM_LOAD( "eh1-obj6.bin",  0x280000, 0x80000, 0x6019fc8c )
	ROM_LOAD( "eh1-obj7.bin",  0x380000, 0x80000, 0x0bd966b8 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "eh1-chr0.bin",  0x000000, 0x80000, 0xbc90ebef )
	ROM_LOAD( "eh1-chr1.bin",  0x080000, 0x80000, 0x61395018 )
	ROM_LOAD( "eh1-chr2.bin",  0x100000, 0x80000, 0x8150f644 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	/* No ROZ files present in ZIP archive */

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "eh1-shrp.bin",  0x000000, 0x80000, 0x39585cf9 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "eh1-d0.bin",  0x000000, 0xb3c4243b )
	NAMCOS2_DATA_LOAD_O_128K( "eh1-d1.bin",  0x000000, 0xc946e79c )
	NAMCOS2_DATA_LOAD_E_128K( "eh1-d2.bin",  0x100000, 0x00000000 ) /* not dumped! */
	NAMCOS2_DATA_LOAD_O_128K( "eh1-d3.bin",  0x100000, 0x8425a9c7 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "eh1-voi1.bin",  0x000000, 0x080000, 0x71e534d3 )
	ROM_LOAD( "eh1-voi2.bin",  0x080000, 0x080000, 0x3e20df8e )
ROM_END

/* SUZUKA 8 HOURS 2 */
ROM_START( suzuk8h2 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "ehs2mp0b.bin",	0x000000, 0x020000, 0xade97f90 )
	ROM_LOAD16_BYTE( "ehs2mp1b.bin",	0x000001, 0x020000, 0x19744a66 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "ehs1sp0.bin",  0x000000, 0x020000, 0x9ca967bc )
	ROM_LOAD16_BYTE( "ehs1sp1.bin",  0x000001, 0x020000, 0xf25bfaaa )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "ehs1snd0.bin",  0x00c000, 0x004000, 0xfc95993b )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "ehs1obj0.bin",  0x000000, 0x80000, 0xa0acf307 )
	ROM_LOAD( "ehs1obj1.bin",  0x100000, 0x80000, 0xca780b44 )
	ROM_LOAD( "ehs1obj2.bin",  0x200000, 0x80000, 0x83b45afe )
	ROM_LOAD( "ehs1obj3.bin",  0x300000, 0x80000, 0x360c03a8 )
	ROM_LOAD( "ehs1obj4.bin",  0x080000, 0x80000, 0x4e503ca5 )
	ROM_LOAD( "ehs1obj5.bin",  0x180000, 0x80000, 0x5405f2d9 )
	ROM_LOAD( "ehs1obj6.bin",  0x280000, 0x80000, 0xf5fc8b23 )
	ROM_LOAD( "ehs1obj7.bin",  0x380000, 0x80000, 0xda6bf51b )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "ehs1chr0.bin", 0x000000, 0x80000, 0x844efe0d )
	ROM_LOAD( "ehs1chr1.bin", 0x080000, 0x80000, 0xe8480a6d )
	ROM_LOAD( "ehs1chr2.bin", 0x100000, 0x80000, 0xace2d871 )
	ROM_LOAD( "ehs1chr3.bin", 0x180000, 0x80000, 0xc1680818 )
	ROM_LOAD( "ehs1chr4.bin", 0x200000, 0x80000, 0x82e8c1d5 )
	ROM_LOAD( "ehs1chr5.bin", 0x280000, 0x80000, 0x9448537c )
	ROM_LOAD( "ehs1chr6.bin", 0x300000, 0x80000, 0x2d1c01ad )
	ROM_LOAD( "ehs1chr7.bin", 0x380000, 0x80000, 0x18dd8676 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	/* No ROZ files present in ZIP archive */

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	ROM_LOAD( "ehs1shap.bin",  0x000000, 0x80000, 0x0f0e2dbf )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_512K( "ehs1dat0.bin",  0x000000, 0x12a202fb )
	NAMCOS2_DATA_LOAD_O_512K( "ehs1dat1.bin",  0x000000, 0x91790905 )
	NAMCOS2_DATA_LOAD_E_512K( "ehs1dat2.bin",  0x100000, 0x087da1f3 )
	NAMCOS2_DATA_LOAD_O_512K( "ehs1dat3.bin",  0x100000, 0x85aecb3f )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "ehs1voi1.bin",  0x000000, 0x080000, 0xbf94eb42 )
	ROM_LOAD( "ehs1voi2.bin",  0x080000, 0x080000, 0x0e427604 )
ROM_END

/* LEGEND OF THE VALKYRIE */
ROM_START( valkyrie )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) 	 /* Master CPU */
	ROM_LOAD16_BYTE( "wd1mpr0.bin",  0x000000, 0x020000, 0x94111a2e )
	ROM_LOAD16_BYTE( "wd1mpr1.bin",  0x000001, 0x020000, 0x57b5051c )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) 	 /* Slave CPU */
	ROM_LOAD16_BYTE( "wd1spr0.bin",  0x000000, 0x010000, 0xb2398321 )
	ROM_LOAD16_BYTE( "wd1spr1.bin",  0x000001, 0x010000, 0x38dba897 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) 	 /* Sound CPU (Banked) */
	ROM_LOAD( "wd1snd0.bin",  0x00c000, 0x004000, 0xd0fbf58b )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) 	 /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )		   /* Sprites */
	NAMCOS2_GFXROM_LOAD_256K( "wdobj0.bin",  0x000000, 0xe8089451 )
	NAMCOS2_GFXROM_LOAD_256K( "wdobj1.bin",  0x080000, 0x7ca65666 )
	NAMCOS2_GFXROM_LOAD_256K( "wdobj2.bin",  0x100000, 0x7c159407 )
	NAMCOS2_GFXROM_LOAD_256K( "wdobj3.bin",  0x180000, 0x649f8760 )
	NAMCOS2_GFXROM_LOAD_256K( "wdobj4.bin",  0x200000, 0x7ca39ae7 )
	NAMCOS2_GFXROM_LOAD_256K( "wdobj5.bin",  0x280000, 0x9ead2444 )
	NAMCOS2_GFXROM_LOAD_256K( "wdobj6.bin",  0x300000, 0x9fa2ea21 )
	NAMCOS2_GFXROM_LOAD_256K( "wdobj7.bin",  0x380000, 0x66e07a36 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )		   /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "wdchr0.bin",  0x000000, 0xdebb0116 )
	NAMCOS2_GFXROM_LOAD_128K( "wdchr1.bin",  0x080000, 0x8a1431e8 )
	NAMCOS2_GFXROM_LOAD_128K( "wdchr2.bin",  0x100000, 0x62f75f69 )
	NAMCOS2_GFXROM_LOAD_128K( "wdchr3.bin",  0x180000, 0xcc43bbe7 )
	NAMCOS2_GFXROM_LOAD_128K( "wdchr4.bin",  0x200000, 0x2f73d05e )
	NAMCOS2_GFXROM_LOAD_128K( "wdchr5.bin",  0x280000, 0xb632b2ec )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) 	 /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "wdroz0.bin",  0x000000, 0xf776bf66 )
	NAMCOS2_GFXROM_LOAD_128K( "wdroz1.bin",  0x080000, 0xc1a345c3 )
	NAMCOS2_GFXROM_LOAD_128K( "wdroz2.bin",  0x100000, 0x28ffb44a )
	NAMCOS2_GFXROM_LOAD_128K( "wdroz3.bin",  0x180000, 0x7e77b46d )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) 	 /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "wdshape.bin",  0x000000, 0x3b5e0249 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )	 /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "wd1dat0.bin",  0x000000, 0xea209f48 )
	NAMCOS2_DATA_LOAD_O_128K( "wd1dat1.bin",  0x000000, 0x04b48ada )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	 /* Sound voices */
	ROM_LOAD( "wd1voi1.bin",  0x000000, 0x040000, 0xf1ace193 )
	ROM_RELOAD(  0x040000, 0x040000 )
	ROM_LOAD( "wd1voi2.bin",  0x080000, 0x020000, 0xe95c5cf3 )
	ROM_RELOAD(  0x0a0000, 0x020000 )
	ROM_RELOAD(  0x0c0000, 0x020000 )
	ROM_RELOAD(  0x0e0000, 0x020000 )
ROM_END

/* KYUUKAI DOUCHUUKI */
ROM_START( kyukaidk )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) 	 /* Master CPU */
	ROM_LOAD16_BYTE( "ky1_mp0b.bin", 0x000000, 0x010000, 0xd1c992c8 )
	ROM_LOAD16_BYTE( "ky1_mp1b.bin", 0x000001, 0x010000, 0x723553af )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) 	 /* Slave CPU */
	ROM_LOAD16_BYTE( "ky1_sp0.bin",  0x000000, 0x010000, 0x4b4d2385 )
	ROM_LOAD16_BYTE( "ky1_sp1.bin",  0x000001, 0x010000, 0xbd3368cd )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) 	 /* Sound CPU (Banked) */
	ROM_LOAD( "ky1_s0.bin",   0x00c000, 0x004000, 0x27aea3e9 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) 	 /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )		   /* Sprites */
	ROM_LOAD( "ky1_o0.bin",  0x000000, 0x80000, 0xebec5132 )
	ROM_LOAD( "ky1_o1.bin",  0x080000, 0x80000, 0xfde7e5ae )
	ROM_LOAD( "ky1_o2.bin",  0x100000, 0x80000, 0x2a181698 )
	ROM_LOAD( "ky1_o3.bin",  0x180000, 0x80000, 0x71fcd3a6 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )		   /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "ky1_c0.bin",  0x000000, 0x7bd69a2d )
	NAMCOS2_GFXROM_LOAD_128K( "ky1_c1.bin",  0x080000, 0x66a623fe )
	NAMCOS2_GFXROM_LOAD_128K( "ky1_c2.bin",  0x100000, 0xe84b3dfd )
	NAMCOS2_GFXROM_LOAD_128K( "ky1_c3.bin",  0x180000, 0x69e67c86 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) 	 /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_256K( "ky1_r0.bin",  0x000000, 0x9213e8c4 )
	NAMCOS2_GFXROM_LOAD_256K( "ky1_r1.bin",  0x080000, 0x97d1a641 )
	NAMCOS2_GFXROM_LOAD_256K( "ky1_r2.bin",  0x100000, 0x39b58792 )
	NAMCOS2_GFXROM_LOAD_256K( "ky1_r3.bin",  0x180000, 0x90c60d92 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) 	 /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "ky1_sha.bin",  0x000000, 0x380a20d7 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )	 /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "ky1_d0.bin",   0x000000, 0xc9cf399d )
	NAMCOS2_DATA_LOAD_O_128K( "ky1_d1.bin",   0x000000, 0x6d4f21b9 )
	NAMCOS2_DATA_LOAD_E_128K( "ky1_d2.bin",   0x100000, 0xeb6d19c8 )
	NAMCOS2_DATA_LOAD_O_128K( "ky1_d3.bin",   0x100000, 0x95674701 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	 /* Sound voices */
	ROM_LOAD( "ky1_v1.bin", 0x000000, 0x080000, 0x5ff81aec )
ROM_END

/* KYUUKAI DOUCHUUKI (OLD) */
ROM_START( kyukaido )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) 	 /* Master CPU */
	ROM_LOAD16_BYTE( "ky1_mp0.bin",  0x000000, 0x010000, 0x01978a19 )
	ROM_LOAD16_BYTE( "ky1_mp1.bin",  0x000001, 0x010000, 0xb40717a7 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) 	 /* Slave CPU */
	ROM_LOAD16_BYTE( "ky1_sp0.bin",  0x000000, 0x010000, 0x4b4d2385 )
	ROM_LOAD16_BYTE( "ky1_sp1.bin",  0x000001, 0x010000, 0xbd3368cd )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) 	 /* Sound CPU (Banked) */
	ROM_LOAD( "ky1_s0.bin",   0x00c000, 0x004000, 0x27aea3e9 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) 	 /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, 0xa5b2a4ff )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )		   /* Sprites */
	ROM_LOAD( "ky1_o0.bin",  0x000000, 0x80000, 0xebec5132 )
	ROM_LOAD( "ky1_o1.bin",  0x080000, 0x80000, 0xfde7e5ae )
	ROM_LOAD( "ky1_o2.bin",  0x100000, 0x80000, 0x2a181698 )
	ROM_LOAD( "ky1_o3.bin",  0x180000, 0x80000, 0x71fcd3a6 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )		   /* Tiles */
	NAMCOS2_GFXROM_LOAD_128K( "ky1_c0.bin",  0x000000, 0x7bd69a2d )
	NAMCOS2_GFXROM_LOAD_128K( "ky1_c1.bin",  0x080000, 0x66a623fe )
	NAMCOS2_GFXROM_LOAD_128K( "ky1_c2.bin",  0x100000, 0xe84b3dfd )
	NAMCOS2_GFXROM_LOAD_128K( "ky1_c3.bin",  0x180000, 0x69e67c86 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) 	 /* ROZ Tiles */
	NAMCOS2_GFXROM_LOAD_256K( "ky1_r0.bin",  0x000000, 0x9213e8c4 )
	NAMCOS2_GFXROM_LOAD_256K( "ky1_r1.bin",  0x080000, 0x97d1a641 )
	NAMCOS2_GFXROM_LOAD_256K( "ky1_r2.bin",  0x100000, 0x39b58792 )
	NAMCOS2_GFXROM_LOAD_256K( "ky1_r3.bin",  0x180000, 0x90c60d92 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) 	 /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "ky1_sha.bin",  0x000000, 0x380a20d7 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )	 /* Shared data roms */
	NAMCOS2_DATA_LOAD_E_128K( "ky1_d0.bin",   0x000000, 0xc9cf399d )
	NAMCOS2_DATA_LOAD_O_128K( "ky1_d1.bin",   0x000000, 0x6d4f21b9 )
	NAMCOS2_DATA_LOAD_E_128K( "ky1_d2.bin",   0x100000, 0xeb6d19c8 )
	NAMCOS2_DATA_LOAD_O_128K( "ky1_d3.bin",   0x100000, 0x95674701 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	 /* Sound voices */
	ROM_LOAD( "ky1_v1.bin", 0x000000, 0x080000, 0x5ff81aec )
ROM_END

/* GOLLY GHOST */
ROM_START( gollygho )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "gl2mpr0.11d",	0x000000, 0x010000, 0xe5d48bb9 )
	ROM_LOAD16_BYTE( "gl2mpr1.13d",	0x000001, 0x010000, 0x584ef971 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "gl1spr0.11k",	0x000000, 0x010000, 0xa108136f )
	ROM_LOAD16_BYTE( "gl1spr1.13k",	0x000001, 0x010000, 0xda8443b7 )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "gl1snd0.7j",  0x00c000, 0x004000, 0x008bce72 )
	ROM_CONTINUE(  0x010000, 0x01c000 )
	ROM_RELOAD(   0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin", 0x0000, 0x2000, 0xa342a97e )
	ROM_LOAD( "gl1edr0c.ic7", 0x8000, 0x8000, 0xdb60886f )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "gl1obj0.5b",  0x000000, 0x40000, 0x6809d267 )
	ROM_LOAD( "gl1obj1.4b",  0x080000, 0x40000, 0xae4304d4 )
	ROM_LOAD( "gl1obj2.5d",  0x100000, 0x40000, 0x9f2e9eb0 )
	ROM_LOAD( "gl1obj3.4d",  0x180000, 0x40000, 0x3a85f3c2 )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles */
	ROM_LOAD( "gl1chr0.11n",  0x00000, 0x20000, 0x1a7c8abd )
	ROM_LOAD( "gl1chr1.11p",  0x20000, 0x20000, 0x36aa0fbc )
	ROM_LOAD( "gl1chr2.11r",  0x40000, 0x10000, 0x6c1964ba )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* ROZ Tiles */
	/* All ROZ ROM sockets unpopulated on PCB */

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Mask shape */
	NAMCOS2_GFXROM_LOAD_128K( "gl1sha0.7n",	0x000000, 0x8886f6f5 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	/* All DAT ROM sockets unpopulated on PCB */

	ROM_REGION16_BE( 0x2000, REGION_USER2, 0 ) /* sprite zoom */
	ROM_LOAD( "04544191.6n",  0x000000, 0x002000, 0x90db1bf6 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "gl1voi1.3m",  0x000000, 0x080000, 0x0eca0efb )
ROM_END

/* LUCKY & WILD */
ROM_START( luckywld )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Master CPU */
	ROM_LOAD16_BYTE( "lw2mp0.11d",	0x000000, 0x020000, 0x368306bb )
	ROM_LOAD16_BYTE( "lw2mp1.13d",	0x000001, 0x020000, 0x9be3a4b8 )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* Slave CPU */
	ROM_LOAD16_BYTE( "lw1sp0.11k",	0x000000, 0x020000, 0x1eed12cb )
	ROM_LOAD16_BYTE( "lw1sp1.13k",	0x000001, 0x020000, 0x535033bc )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound CPU (Banked) */
	ROM_LOAD( "lw1snd0.7j",  0x00c000, 0x004000, 0xcc83c6b6 )
	ROM_CONTINUE( 0x010000, 0x01c000 )
	ROM_RELOAD(  0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, 0xa342a97e )
        /* MCU code only, C68PRG socket is unpopulated on real Lucky & Wild PCB */

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "lw1obj0.3p",  0x000000, 0x80000, 0x21485830 )
	ROM_LOAD( "lw1obj1.3w",  0x100000, 0x80000, 0xd6437a82 )
	ROM_LOAD( "lw1obj2.3t",  0x200000, 0x80000, 0xceb6f516 )
	ROM_LOAD( "lw1obj3.3y",  0x300000, 0x80000, 0x5d32c7e9 )

	ROM_LOAD( "lw1obj4.3s",  0x080000, 0x80000, 0x0050458a )
	ROM_LOAD( "lw1obj5.3x",  0x180000, 0x80000, 0xcbc08f46 )
	ROM_LOAD( "lw1obj6.3u",  0x280000, 0x80000, 0x29740c88 )
	ROM_LOAD( "lw1obj7.3z",  0x380000, 0x80000, 0x8cbd62b4 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* 8x8 Tiles */
	ROM_LOAD( "lw1chr0.11n", 0x000000, 0x80000, 0xa0da15fd )
	ROM_LOAD( "lw1chr1.11p", 0x080000, 0x80000, 0x89102445 )
	ROM_LOAD( "lw1chr2.11r", 0x100000, 0x80000, 0xc749b778 )
	ROM_LOAD( "lw1chr3.11s", 0x180000, 0x80000, 0xd76f9578 )
	ROM_LOAD( "lw1chr4.9n",  0x200000, 0x80000, 0x2f8ab45e )
	ROM_LOAD( "lw1chr5.9p",  0x280000, 0x80000, 0xc9acbe61 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* 16x16 Tiles */
	ROM_LOAD( "lw1roz1.23c", 0x080000, 0x80000, 0x74e98793 )
	ROM_LOAD( "lw1roz2.23e", 0x000000, 0x80000, 0x1ef46d82 )
	ROM_LOAD( "lw1roz0.23b", 0x1c0000, 0x80000, 0xa14079c9 )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* 8x8 shape */
	ROM_LOAD( "lw1sha0.7n",  0x000000, 0x80000, 0xe3a65196 )

	ROM_REGION( 0x80000, REGION_GFX5, 0 ) /* 16x16 shape */
 	ROM_LOAD( "lw1rzs0.20z", 0x000000, 0x80000, 0xa1071537 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* Shared data roms */
	ROM_LOAD16_BYTE( "lw1dat0.13s",  0x000000, 0x80000, 0x5d387d47 )
	ROM_LOAD16_BYTE( "lw1dat1.13p",  0x000001, 0x80000, 0x7ba94476 )
	ROM_LOAD16_BYTE( "lw1dat2.13r",  0x100000, 0x80000, 0xeeba7c62 )
	ROM_LOAD16_BYTE( "lw1dat3.13n",  0x100001, 0x80000, 0xec3b36ea )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Sound voices */
	ROM_LOAD( "lw1voi1.3m",  0x000000, 0x080000, 0xb3e57993 )
	ROM_LOAD( "lw1voi2.3l",  0x080000, 0x080000, 0xcd8b86a2 )
ROM_END


DRIVER_INIT( assault ){
	namcos2_gametype=NAMCOS2_ASSAULT;
}

DRIVER_INIT( assaultj ){
	namcos2_gametype=NAMCOS2_ASSAULT_JP;
}

DRIVER_INIT( assaultp ){
	namcos2_gametype=NAMCOS2_ASSAULT_PLUS;
}

DRIVER_INIT( burnforc ){
    namcos2_gametype=NAMCOS2_BURNING_FORCE;
}

DRIVER_INIT( cosmogng ){
	namcos2_gametype=NAMCOS2_COSMO_GANG;
}

DRIVER_INIT( dsaber ){
	namcos2_gametype=NAMCOS2_DRAGON_SABER;
}

DRIVER_INIT( dsaberj ){
	namcos2_gametype=NAMCOS2_DRAGON_SABER;
}

DRIVER_INIT( dirtfoxj ){
	namcos2_gametype=NAMCOS2_DIRT_FOX_JP;
}

DRIVER_INIT( finallap ){
	data16_t *rom = (void *)memory_region(REGION_CPU2);
	/* HACK: patch bypasses a nasty race condition that casues the road-drawing routine to
	 * be skipped.
	 *
	 * Similar problems exist in finallap2, finallap3
	 */
	rom[0x1F6E/2] = 0x4e71;
	rom[0x1F70/2] = 0x4e71;
	rom[0x1F72/2] = 0x4e71;
	rom[0x1F74/2] = 0x4e71;
	namcos2_gametype=NAMCOS2_FINAL_LAP;
	mFinalLapProtCount = 0;
}

DRIVER_INIT( finalap2 ){
	namcos2_gametype=NAMCOS2_FINAL_LAP_2;
	mFinalLapProtCount = 0;
}

DRIVER_INIT( finalp2j ){
	namcos2_gametype=NAMCOS2_FINAL_LAP_2;
	mFinalLapProtCount = 0;
}

DRIVER_INIT( finalap3 ){
	namcos2_gametype=NAMCOS2_FINAL_LAP_3;
	mFinalLapProtCount = 0;
}

DRIVER_INIT( finehour ){
	namcos2_gametype=NAMCOS2_FINEST_HOUR;
}

DRIVER_INIT( fourtrax ){
	namcos2_gametype=NAMCOS2_FOUR_TRAX;
}

DRIVER_INIT( kyukaidk ){
	namcos2_gametype=NAMCOS2_KYUUKAI_DOUCHUUKI;
}

DRIVER_INIT( marvlanj ){
	namcos2_gametype=NAMCOS2_MARVEL_LAND;
}

DRIVER_INIT( marvland ){
	namcos2_gametype=NAMCOS2_MARVEL_LAND;
}

DRIVER_INIT( metlhawk ){
	/* unscramble sprites */
	int i, j, k, l;
	unsigned char *data = memory_region(REGION_GFX1);
	for(i=0; i<0x200000; i+=32*32)
	{
		for(j=0; j<32*32; j+=32*4)
			for(k=0; k<32; k+=4)
			{
				unsigned char v;
				int a;

				a = i+j+k+32;
				v = data[a];
				data[a]   = data[a+3];
				data[a+3] = data[a+2];
				data[a+2] = data[a+1];
				data[a+1] = v;

				a += 32;
				v = data[a];
				data[a]   = data[a+2];
				data[a+2] = v;
				v = data[a+1];
				data[a+1] = data[a+3];
				data[a+3] = v;

				a += 32;
				data[a]   = data[a+1];
				data[a+1] = data[a+2];
				data[a+2] = data[a+3];
				data[a+3] = v;

				a = i+j+k;
				for(l=0; l<4; l++) {
					v = data[a+l+32];
					data[a+l+32] = data[a+l+32*3];
					data[a+l+32*3] = v;
				}
			}
	}
	// 90 degrees prepare a turned character.
	for(i=0; i<0x200000; i+=32*32)
	{
		for(j=0; j<32; j++)
		{
			for(k=0; k<32; k++)
			{
				data[0x200000+i+j*32+k] = data[i+j+k*32];
			}
		}
	}
	namcos2_gametype=NAMCOS2_METAL_HAWK;
}


DRIVER_INIT( mirninja ){
	namcos2_gametype=NAMCOS2_MIRAI_NINJA;
}

DRIVER_INIT( ordyne ){
	namcos2_gametype=NAMCOS2_ORDYNE;
}

DRIVER_INIT( phelios ){
	namcos2_gametype=NAMCOS2_PHELIOS;
}

DRIVER_INIT( rthun2 ){
	namcos2_gametype=NAMCOS2_ROLLING_THUNDER_2;
}

DRIVER_INIT( rthun2j ){
	namcos2_gametype=NAMCOS2_ROLLING_THUNDER_2;
}

DRIVER_INIT( sgunner2 ){
	namcos2_gametype=NAMCOS2_STEEL_GUNNER_2;
}

DRIVER_INIT( sws92 ){
	namcos2_gametype=NAMCOS2_SUPER_WSTADIUM_92;
}

DRIVER_INIT( sws92g ){
	namcos2_gametype=NAMCOS2_SUPER_WSTADIUM_92T;
}

DRIVER_INIT( sws93 ){
	namcos2_gametype=NAMCOS2_SUPER_WSTADIUM_93;
}

DRIVER_INIT( suzuka8h ){
        namcos2_gametype=NAMCOS2_SUZUKA_8_HOURS;
}

DRIVER_INIT( suzuk8h2 ){
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);
	rom[0x0045f6/2] = 0x4e71; // HACK: patch problem with scroll layers for now
	rom[0x0045f8/2] = 0x4e71;
	rom[0x0045fa/2] = 0x4e71;
	rom[0x0045fc/2] = 0x4e71;
	namcos2_gametype=NAMCOS2_SUZUKA_8_HOURS_2;
}

DRIVER_INIT( valkyrie ){
	namcos2_gametype=NAMCOS2_VALKYRIE;
}

DRIVER_INIT( gollygho ){
	namcos2_gametype=NAMCOS2_GOLLY_GHOST;
}

DRIVER_INIT( luckywld ){
	UINT8 *pData = (UINT8 *)memory_region( REGION_GFX5 );
	int i;
	for( i=0; i<32*0x4000; i++ )
	{ /* unscramble gfx mask */
		int code = pData[i];
		int out = 0;
		if( code&0x01 ) out |= 0x80;
		if( code&0x02 ) out |= 0x40;
		if( code&0x04 ) out |= 0x20;
		if( code&0x08 ) out |= 0x10;
		if( code&0x10 ) out |= 0x08;
		if( code&0x20 ) out |= 0x04;
		if( code&0x40 ) out |= 0x02;
		if( code&0x80 ) out |= 0x01;
		pData[i] = out;
	}
	namcos2_gametype=NAMCOS2_LUCKY_AND_WILD;
}

/* Missing ROM sets/games */

/* Bubble Trouble */
/* Super World Stadium */
/* Steel Gunner */

/* Based on the dumped BIOS versions it looks like Namco changed the BIOS rom */
/* from sys2c65b to sys2c65c sometime between 1988 and 1990 as mirai ninja	  */
/* and metal hawk have the B version and dragon saber has the C version 	  */

/*    YEAR, NAME,     PARENT,   MACHINE,  INPUT,    INIT,     MONITOR,    COMPANY, FULLNAME,	    	FLAGS */
GAMEX(1987, finallap, 0,        finallap, finallap, finallap, ROT0,   "Namco", "Final Lap (Rev E)", GAME_NOT_WORKING)
GAMEX(1987, finalapd, finallap, finallap, finallap, finallap, ROT0,   "Namco", "Final Lap (Rev D)", GAME_NOT_WORKING)
GAMEX(1987, finalapc, finallap, finallap, finallap, finallap, ROT0,   "Namco", "Final Lap (Rev C)", GAME_NOT_WORKING)
GAMEX(1987, finlapjc, finallap, finallap, finallap, finallap, ROT0,   "Namco", "Final Lap (Japan - Rev C)", GAME_NOT_WORKING)
GAMEX(1987, finlapjb, finallap, finallap, finallap, finallap, ROT0,   "Namco", "Final Lap (Japan - Rev B)", GAME_NOT_WORKING)
GAME( 1988, assault,  0,        default,  assault,  assault , ROT90,  "Namco", "Assault" )
GAME( 1988, assaultj, assault,  default,  assault,  assaultj, ROT90,  "Namco", "Assault (Japan)" )
GAME( 1988, assaultp, assault,  default,  assault,  assaultp, ROT90,  "Namco", "Assault Plus (Japan)" )
GAMEX(1988, metlhawk, 0,        metlhawk, metlhawk, metlhawk, ROT90,  "Namco", "Metal Hawk (Japan)", GAME_IMPERFECT_GRAPHICS)
GAME( 1988, ordyne,   0,        default,  default,  ordyne,   ROT180, "Namco", "Ordyne (Japan)" )
GAME( 1988, mirninja, 0,        default,  default,  mirninja, ROT0,   "Namco", "Mirai Ninja (Japan)" )
GAME( 1988, phelios,  0,        default,  default,  phelios , ROT90,  "Namco", "Phelios (Japan)" )
GAME( 1989, dirtfoxj, 0,        default,  dirtfox,  dirtfoxj, ROT90,  "Namco", "Dirt Fox (Japan)" )
GAMEX(1989, fourtrax, 0,        finallap, driving,  fourtrax, ROT0,   "Namco", "Four Trax", GAME_NOT_WORKING)
GAME( 1989, valkyrie, 0,        default,  default,  valkyrie, ROT90,  "Namco", "Valkyrie No Densetsu (Japan)" )
GAME( 1989, finehour, 0,        default,  default,  finehour, ROT0,   "Namco", "Finest Hour (Japan)" )
GAME( 1989, burnforc, 0,        default,  default,  burnforc, ROT0,   "Namco", "Burning Force (Japan)" )
GAME( 1989, marvland, 0,        default,  default,  marvland, ROT0,   "Namco", "Marvel Land (US)" )
GAME( 1989, marvlanj, marvland, default,  default,  marvlanj, ROT0,   "Namco", "Marvel Land (Japan)" )
GAME( 1990, kyukaidk, 0,        default,  default,  kyukaidk, ROT0,   "Namco", "Kyuukai Douchuuki (Japan new version)" )
GAME( 1990, kyukaido, kyukaidk, default,  default,  kyukaidk, ROT0,   "Namco", "Kyuukai Douchuuki (Japan old version)" )
GAME( 1990, dsaber,   0,        default,  default,  dsaber,   ROT90,  "Namco", "Dragon Saber" )
GAME( 1990, dsaberj,  dsaber,   default,  default,  dsaberj,  ROT90,  "Namco", "Dragon Saber (Japan)" )
GAMEX(1990, finalap2, 0,        finallap, finallap, finalap2, ROT0,   "Namco", "Final Lap 2", GAME_NOT_WORKING )
GAMEX(1990, finalp2j, finalap2, finallap, finallap, finalp2j, ROT0,   "Namco", "Final Lap 2 (Japan)", GAME_NOT_WORKING )
GAME( 1990, gollygho, 0,        gollygho, gollygho, gollygho, ROT180, "Namco", "Golly! Ghost!" )
GAME( 1990, rthun2,   0,        default,  default,  rthun2,   ROT0,   "Namco", "Rolling Thunder 2" )
GAME( 1990, rthun2j,  rthun2,   default,  default,  rthun2j,  ROT0,   "Namco", "Rolling Thunder 2 (Japan)" )
GAME( 1991, sgunner2, 0,        sgunner,  sgunner,  sgunner2, ROT0,   "Namco", "Steel Gunner 2 (Japan)" )
GAME( 1991, cosmogng, 0,        default,  default,  cosmogng, ROT90,  "Namco", "Cosmo Gang the Video (US)" )
GAME( 1991, cosmognj, cosmogng, default,  default,  cosmogng, ROT90,  "Namco", "Cosmo Gang the Video (Japan)" )
GAMEX(1992, finalap3, 0,        finallap, finallap, finalap3, ROT0,   "Namco", "Final Lap 3 (Japan)", GAME_NOT_WORKING )
GAMEX(1992, luckywld, 0,        luckywld, luckywld, luckywld, ROT0,   "Namco", "Lucky & Wild",GAME_IMPERFECT_GRAPHICS )
GAMEX(1992, suzuka8h, 0,        luckywld, finallap, suzuka8h, ROT0,   "Namco", "Suzuka 8 Hours (Japan)", GAME_NOT_WORKING )
GAME( 1992, sws92,    0,        default,  default,  sws92,    ROT0,   "Namco", "Super World Stadium '92 (Japan)" )
GAME( 1992, sws92g,   sws92,    default,  default,  sws92g,   ROT0,   "Namco", "Super World Stadium '92 Gekitouban (Japan)" )
GAMEX(1993, suzuk8h2, 0,        luckywld, driving,  suzuk8h2, ROT0,   "Namco", "Suzuka 8 Hours 2 (Japan)", GAME_IMPERFECT_GRAPHICS )
GAME( 1993, sws93,    0,        default,  default,  sws93,    ROT0,   "Namco", "Super World Stadium '93 (Japan)" )
