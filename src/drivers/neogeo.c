/***************************************************************************
	M.A.M.E. Neo Geo driver presented to you by the Shin Emu Keikaku team.

	Shin Emu Keikaku team is now :

	AVDB, Mish and Fuzz.

	Revision History :

	AVDB 27/06/98	New name, Shin Emu Keikaku team.
	AVDB 27/06/98	Fixed some shadow registers, Puzzle de Pon
			now works 100 %.
	AVDB 27/06/98	New tile banking code, Sidekicks now works.
	AVDB 28/06/98	Palette stuff implemented but not activated.
	AVDB 28/06/98   A number of shadow registers found.
	AVDB 29/06/98   Finally fixed Y scrolling (YES! :-) ).
	AVDB 29/06/98   Implemented a very crude zoom, probably have
			to write my own zoom function :-(.
	AVDB 30/06/98   Added fake dipswitch settings for
			Language and system.
	AVDB 04/07/98   Converted to Mb6 with old allegro.
			The new allegro library does not work
			with my graphics card (a Matrox Millenium 2 MB).
	AVDB 09/07/98   Neo Geo can have 2 meg of ROM, the second bank
			is mapped to 0x200000 - 0x2fffff.
	AVDB 13/07/98   Small change to the y tile parameters.
	AVDB 14/07/98   Implemented banking to bring down the
			memory requirements. Unfortunately
			debugging memory area 0xc00000 does
			not work anymore (weird (?) ).
	AVDB 20/07/98   Palette banking activated. Used by
			Alpha Mission 2.
	AVDB 20/07/98	Now featuring Mac friendliness (tm).
			rom loading should work on the Mac and
			the colors should be OK (tip from Brad).

	Mish 28/07/98	Removed prom init function, added compressed palette code
	Mish 29/07/98	Added Karnovs Revenge
   			Core change: Make sure 'total' in GfxLayout is an INT not a short
			Unsigned int in GfxElement
	Mish 30/07/98	Found sound byte
			Found test switch (from hardware test)
			Coin inputs still dont work...

			Note: puzzle de pon, puzzle bobble, karnov revenge have tiles
			lowered for testing.. fix later. Changed all REAL_60HZ to just
			60HZ - no vbl is used here, slight speedup.
			Fix the drivers at bottom.

	Mish 31/07/98	Started new layout of drivers, below.
			Added first attempt at zoom graphics
			Made all games clone of NeoGeo parent, so the bios will load from
			it's own zip file (doesn't work yet).
			Added Thrash Rally - has lots of full screen zooms.

	Mish 1/08/98	Currently, the 4bpp NeoGeo graphics are expanded to 8bpp Mame
			graphics, giving HUGE memory requirements.  We have to use a
			custom zoom function anyway, why not just change it to draw
			from the 4bpp data...  Would save a lot of memory.

			Compressed all the machine drivers.
			Added colour fade at start :)
			Fixed the crash when going into soft dips. (bpx at c19c0a)
			RAM at 10f300 to 10ffff seems to be static?

	AVDB 3/08/98	All drivers now load the bios rom from the dummy parent
			no need for zipping the rom in every set.
			Removed all CPP style comments.

	Mish:	All graphics nearly fixed, including missing tiles on zooms (P Bobble,
etc).  All zooms nearly 100%.
			Added USA/Japan/Europe mode.
			Altered start/select button placement, hardly anything uses select so I
put start in it's usual Mame place (1 & 2).
			The 68000 can read back the byte it sends to the z80, at 0x320000
            Fixed soft dips!  And implemented backup ram to save them.
            No crash at end of demo when trying to switch cartridges.
            Coin inputs work.

    TODO :  -Properly emulate the hard and soft dipswitches.
			-Bring memory requirements down.
			-The system is trying to switch games. Leaving it running in
			attract mode will cause the game to hang, because the system
			thinks it has got 255 cartridges inserted (see test mode).
			-implement proper scaling.
			-Properly emulate the uP4990.
			-Fix coin inputs.
			-NeoRage has American, Japan & Europe setting,
			we only have Jap/English for now, NeoRage also has 'censor'
			dip setting.
==============================================================================

Memory Map Neo Geo :

	0x000000 - 0x0fffff     ROM

	0x100000 - 0x10ffff     RAM

	0x10FD80 - 0x10FD83	Software dipswitches
				These are some sort of combination of
				the hardware dipswitch and the inputs.
				(weird ?, I don't understand them).

        Mish - seems to be 'internal' dip switch, not a dip on the board.

	0x400000 - 0x401fff     ColorRam 2 Banks

	0x800000 - 0x800fff     backup ram
	0xc00000 - 0xc1ffff     Bios
	0xd00000 - 0xd0ffff     RAM bank 2

	IO byte read

	0x300081		Unknown IO ?

	0x300000		controller 1
				bit 0 : joy up
				bit 1 : joy down
				bit 2 : joy left
				bit 3 : joy right
				bit 4 : button A
				bit 5 : button B
				bit 6 : button C
				bit 7 : button D

	0x3000001		Dipswitches
				bit 0 : Selftest
				bit 1 : Unknown (Unused ?) \ something to do with
				bit 2 : Unknown (Unused ?) / auto repeating keys ?
				bit 3 : \
				bit 4 :  | communication setting ?
				bit 5 : /
				bit 6 : free play
				bit 7 : stop mode ?

	0x320001		bit 0 : COIN 1
				bit 1 : COIN 2
				bit 2 : SERVICE
				bit 3 : UNKNOWN
				bit 4 : UNKNOWN
				bit 5 : UNKNOWN
				bit 6 : 4990 test pulse bit.
				bit 7 : 4990 data bit.

	0x340000		controller 2
				bit 0 : joy up
				bit 1 : joy down
				bit 2 : joy left
				bit 3 : joy right
				bit 4 : button A
				bit 5 : button B
				bit 6 : button C
				bit 7 : button D

	0x380000		bit 0 : Start  1
				bit 1 : Select 1
				bit 2 : Start  2
				bit 3 : Select 2
				bit 4 - 7 unknown

	IO byte write

	0x320000		Byte written to sound CPU

	0x380001		Unknown
	0x380031		Unknown
	0x380041		Test switch (see hardware test) + ??

	0x380051		4990 control write register
				bit 0: C0
				bit 1: C1
				bit 2: C2
				bit 3-7: unused.

				0x02 = shift.
				0x00 = register hold.
				0x04 = ????.
				0x03 = time read (reset register).

	0x3c000c		IRQ acknowledge

	0x380011		Backup bank select

	0x3a0001		Enable display.
	0x3a0011		Disable display

	0x3a0003		Swap in Bios (0x80 bytes vector table of BIOS)
	0x3a0013		Swap in Rom  (0x80 bytes vector table of ROM bank)

	0x3a000f		Select Color Bank 2
	0x3a001f		Select Color Bank 1

	0x300001		Watchdog (?)

	0x3a000d		lock backup ram
	0x3a001d		unlock backup ram

	0x3a000b		set game vector table (?)  mirror ?
	0x3a001b		set bios vector table (?)  mirror ?

	0x3a000c		Unknown	(ghost pilots)
	0x31001c		Unknown (ghost pilots)

	IO word read

	0x3c0002		return vidram word (pointed to by 0x3c0000)
	0x3c0006		?????.
	0x3c0008		shadow adress for 0x3c0000 (not confirmed).
	0x3c000a		shadow adress for 0x3c0002 (confirmed, see
							   Puzzle de Pon).
	IO word write

	0x3c0006		Unknown, set vblank counter (?)

	0x3c0008		shadow address for 0x3c0000	(not confirmed)
	0x3c000a		shadow address for 0x3c0002	(not confirmed)

	0x3c0000		vidram pointer
	0x3c0002		vidram data write
	0x3c0004		modulo add write

	The Neo Geo contains an NEC 4990 Serial I/O calendar & clock.
	accesed through 0x320001, 0x380050, 0x280050 (shadow adress).
	A schematic for this device can be found on the NEC webpages.

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/pd4990a.h"

extern unsigned char *vidram;
extern unsigned char *neogeo_ram;
extern unsigned char *neogeo_sram;

/* from vidhrdw/neogeo.c */
void neogeo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  neogeo_mvs_vh_start(void);
int  neogeo_mgd2_vh_start(void);
void neogeo_vh_stop(void);
void neogeo_paletteram_w(int offset,int data);
int  neogeo_paletteram_r(int offset);
void setpalbank0(int offset, int data);
void setpalbank1(int offset, int data);

void neo_board_fix(int offset, int data);
void neo_game_fix(int offset, int data);


void vidram_modulo_w(int offset, int data);
void vidram_data_w(int offset, int data);
void vidram_offset_w(int offset, int data);

int vidram_data_r(int offset);
int vidram_modulo_r(int offset);
int vidram_offset_r(int offset);

/* debug, used to 'see' the locations mapped in ROM space */
/* with the debugger */
int mish_vid_r(int offset);
void mish_vid_w(int offset, int data);
/* end debug */

void neo_unknown1(int offset, int data);
void neo_unknown2(int offset, int data);
void neo_unknown3(int offset, int data);
void neo_unknown4(int offset, int data);




/* from machine/neogeo.c */
int neogeo_rom_loaded;
void neogeo_init_machine(void);

/* debug defined in vidhrdw/neogeo.c */
extern int dotiles;
extern int screen_offs;
/* end debug */

/******************************************************************************/

static int irq_acknowledge;
static int vblankcount=0;
static int neo_z80_byte;

/******************************************************************************/

static int neogeo_interrupt(void)
{

	/* debug setting, tile view mode connected to '8' */
	if (readinputport(6) & 0x01) {
		while (osd_key_pressed(OSD_KEY_8)) ;
		dotiles ^= 1;
	}

	/* tile view - 0x80, connected to '9' */
	if (readinputport(6) & 0x02) {
		if (screen_offs > 0)
			screen_offs -= 0x80;
	}

	/* tile view + 0x80, connected to '0' */
	if (readinputport(6) & 0x04) {
		if (screen_offs < 0x10000)
			screen_offs += 0x80;
	}
	/* end debugging */

	/* Add a timer tick to the pd4990a */
	addretrace();

	/* internal counter, probably used for 0x3c0006 */
	vblankcount--;

	/* return a standard vblank interrupt */
	return(1);      /* vertical blank */
}

void write_irq(int offset, int data) {
	irq_acknowledge = data;
}

/* Calendar, coins + Z80 communication */
static int timer_r (int offset)
{

	/* This is just for testing purposes */
	/* not real correct code */
	int coinflip = read_4990_testbit();

   //	 if (errorlog) fprintf(errorlog,"CPU %04x - Read timer\n",cpu_getpc());

	return (  readinputport(4) ^ (coinflip << 6) ^ (coinflip << 7) )
    		+ (neo_z80_byte<<8);
}

static void neo_z80_w(int offset, int data)
{
	/* Change this to soundlatch_w later */
	neo_z80_byte=(data)>>8;
}

int vblankc_r(int offset) {
	return (vblankcount);
}

void vblankc_w(int offset,int data) {
	vblankcount = data;
}

int controller1_r (int offset) {

/* if (errorlog) fprintf(errorlog,"CPU %04x - Read controller 1\n",cpu_getpc()); */


	return ( (readinputport(0) << 8) + readinputport(3) );
}

int controller2_r (int offset) {

/* if (errorlog) fprintf(errorlog,"CPU %04x - Read controller 2\n",cpu_getpc()); */

	return (readinputport(1) << 8);
}

int controller3_r (int offset) {

/* if (errorlog) fprintf(errorlog,"CPU %04x - Read controller 3\n",cpu_getpc()); */

	return (readinputport(2) << 8);
}

int controller4_r (int offset) {
	return readinputport(7); /* << 8); */
}

static int read_softdip (int offset)
{
	int result;
	int pc = cpu_getpc();

	/* Clear out the fake dipswitch values */
	result = READ_WORD (&neogeo_ram[0xfd82]) & ~0x8003;

    /* Memory check */
/*	if (cpu_getpc()==0xc13154) return result; */

    if (errorlog) fprintf (errorlog,"CPU %04x - Read soft dip, val: %04x\n", pc, result);

	/* Protection checks */

	/* Samurai Shodown 3 */
//	if (pc == 0x740) return 0x00;

	/* Samurai Shodown 4 */
//	if (pc == 0xafa4) return 0x00;

	/* Art of Fighting 3 */
//	if (pc == 0x17bf8) return 0x00;

	/* Fatal Fury 3 */
//	if (pc == 0x9b50) return 0x00;

	/* Pulstar */
//	if (pc == 0x0a4c) return 0x00;

	/* King of Fighters '95 */
//	if ((pc == 0x1604) || (pc == 0x656c)) return 0x00;

	/* software dipswitches are mapped in RAM space */
	/* OR the settings in when not in memory test mode */

	/* Set up machine country */
    result |= (readinputport(5) & 0x3);

    /* Console/arcade mode */
	if (readinputport(5) & 0x04)
		result |= 0x8000;

	return (result);
}


static int test_r(int offset)
{

return 0;
}

static int neogeo_rom_bank;

static void rom_bank_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[0];
	if (errorlog) fprintf (errorlog,"CPU %04x: Changed rom bank to %d\n", cpu_getpc(), data);

	neogeo_rom_bank = data;
	data = data & 0x7;
	cpu_setbank (4, &RAM[(data+1)*0x100000]);
}

static int rom_bank_r (int offset)
{
	if (errorlog) fprintf (errorlog,"CPU %04x: rom bank r: %d\n", cpu_getpc(), neogeo_rom_bank);

	return (neogeo_rom_bank);
}

static void rom_bank_alt_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[0];

	data >>= 8;
	if (errorlog) fprintf(errorlog,"CPU %04x: Changed rom bank (alt) to %d\n",cpu_getpc(),data & 0x07);

	data=data&0x7;
	cpu_setbank(4,&RAM[(data+1)*0x100000]);
}

static int bios_cycle_skip_r(int offset)
{
	cpu_spinuntil_int();
	return 0;
}

static int tr(int offset)
{
 if (errorlog) fprintf(errorlog,"CPU %04x - Read %06x\n",cpu_getpc(),offset+0x100000);

	return READ_WORD(&neogeo_ram[0]);
}
static int t_read(int offset)
{
 	unsigned char *RAM = Machine->memory_region[4];
 if (errorlog && cpu_getpc()<0xc00000) fprintf(errorlog,"CPU %04x - Read %06x\n",cpu_getpc(),offset+0xc00000);

	return READ_WORD(&RAM[offset]);
}
static void t_write(int offset,int data)
{

 if (errorlog) fprintf(errorlog,"CPU %04x - Write %06x\n",cpu_getpc(),offset+0xc00000);

	WRITE_WORD(&neogeo_sram[offset],data);
}

/******************************************************************************/

static struct MemoryReadAddress neogeo_readmem[] =
{
	{ 0x000000, 0x0FFFFF, MRA_ROM },	/* Rom bank 1 */


//{ 0x100000, 0x100001, tr },

//{ 0x10005a, 0x10005b, bios_cycle_skip_r },

	{ 0x10fd82, 0x10fd83, read_softdip },
//    { 0x10fdae, 0x10fdaf, read_startup },

//   	{ 0x103466, 0x103467, cycle_skip_r }, /* Karnov R */
 	{ 0x10fe8c, 0x10fe8d, bios_cycle_skip_r },
	{ 0x100000, 0x10ffff, MRA_BANK1 }, /* Ram bank */
 	{ 0x2ffff0, 0x2ffff1, rom_bank_r }, /* used as a protection check */
 	{ 0x2ffffe, 0x2fffff, rom_bank_r }, /* used as a protection check */
 	{ 0x200000, 0x2fffef, MRA_BANK4 }, /* Rom bank 2 */

	{ 0x300000, 0x300001, controller1_r },
	{ 0x300080, 0x300081, controller4_r }, /* Test switch in here */
	{ 0x320000, 0x320001, timer_r }, /* Coins, Calendar, Z80 communication */
	{ 0x340000, 0x340001, controller2_r },
	{ 0x380000, 0x380001, controller3_r },
	{ 0x3C0000, 0x3C0001, vidram_offset_r },
	{ 0x3C0002, 0x3C0003, vidram_data_r },
	{ 0x3C0004, 0x3C0005, vidram_modulo_r },

//    { 0x3C0006, 0x3C0007, test_r },
    { 0x3C0006, 0x3C0007, MRA_NOP },
      //  { 0x3C0006, 0x3C0007, vblankc_r },	/* Test */

	{ 0x3c000a, 0x3c000b, vidram_data_r },	/* Puzzle de Pon */
	{ 0x400000, 0x401fff, neogeo_paletteram_r },

   	{ 0x600000, 0x610bff, mish_vid_r },

	{ 0x800000, 0x800FFF, MRA_BANK5 }, /* memory card */
	{ 0xC00000, 0xC1FFFF, MRA_BANK3 }, /* system bios rom */
	{ 0xD00000, 0xD0FFFF, MRA_BANK2 }, /* 64k battery backed SRAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress neogeo_writemem[] =
{
	{ 0x000000, 0x0FFFFF, MWA_ROM },    /* ghost pilots writes to ROM */
	{ 0x100000, 0x10FFFF, MWA_BANK1 },
//	{ 0x280050, 0x280051, neo_pd4990a_control_w },
	{ 0x280050, 0x280051, MWA_NOP },

	{ 0x2ffff0, 0x2ffff1, rom_bank_w },
	{ 0x2ffffe, 0x2fffff, rom_bank_alt_w },
	{ 0x300000, 0x300001, MWA_NOP },        /* Watchdog */


	{ 0x320000, 0x320001, neo_z80_w }, /* Sound CPU */
  	{ 0x380030, 0x380031, neo_unknown2 },

   	{ 0x380000, 0x380001, MWA_NOP },

   //	{ 0x380030, 0x380031, neo_unknown3 },  /* No idea */
	{ 0x380040, 0x380041, MWA_NOP },  /* Either 1 or both of output leds */

//	{ 0x380050, 0x380051, neo_pd4990a_control_w },
	{ 0x380050, 0x380051, MWA_NOP },


/*	{ 0x3a000e, 0x3a000f,  }, */

/*    { 0x3a0000, 0x3a0001, neo_unknown1 }, */
/*    { 0x3a0010, 0x3a0011, neo_unknown2 }, */
	{ 0x3a000a, 0x3a000b, neo_board_fix }, /* Select board FIX char rom */
	{ 0x3a001a, 0x3a001b, neo_game_fix },  /* Select game FIX char rom */
	{ 0x3a000c, 0x3a000d, neo_unknown1 },
   	{ 0x3a001c, 0x3a001d, neo_unknown2 },
	{ 0x3a000e, 0x3a000f, setpalbank1 },
	{ 0x3a001e, 0x3a001f, setpalbank0 },    /* Palette banking */

	{ 0x3C0000, 0x3C0001, vidram_offset_w },
	{ 0x3C0002, 0x3C0003, vidram_data_w },
	{ 0x3C0004, 0x3C0005, vidram_modulo_w },

	{ 0x3C0006, 0x3C0007, MWA_NOP },
  //	{ 0x3C0006, 0x3C0007, vblankc_w },   /* Used a lot by Metal Slug */

 /*	{ 0x3C0008, 0x3C0009, vidram_offset_w },
	{ 0x3C000A, 0x3C000B, vidram_data_w },
*/


	{ 0x3C000C, 0x3C000D, write_irq },

	{ 0x400000, 0x401FFF, neogeo_paletteram_w },

	{ 0x600000, 0x610bFF, mish_vid_w }, /* debug */

 	{ 0x800000, 0x800FFF, MWA_BANK5 },         /* mem card */
	{ 0xD00000, 0xD0FFFF, MWA_BANK2 },
	{ -1 }  /* end of table */
};

/******************************************************************************/

#if 0
static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
 	{ 0xf800, 0xfffc, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
 	{ 0xf800, 0xfffc, MWA_RAM },
	{ -1 }	/* end of table */
};
#endif

/******************************************************************************/

INPUT_PORTS_START( neogeo_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "SELECT 1",OSD_KEY_6, IP_JOY_NONE, 0) /* Player 1 Select */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "SELECT 2",OSD_KEY_7, IP_JOY_NONE, 0) /* Player 2 Select */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Bits 10 & 20 have significance.. don't know what yet */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "Off" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Freeplay", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Stop mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) /* Service */

	/* Fake  IN 5 */
	PORT_START
	PORT_DIPNAME( 0x03, 0x02,"Territory", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00,"Japan" )
	PORT_DIPSETTING(    0x01,"USA" )
	PORT_DIPSETTING(    0x02,"Europe" )
	PORT_DIPNAME( 0x04, 0x04,"Machine mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04,"Arcade" )
	PORT_DIPSETTING(    0x00,"Home" )

	/* debug */
	PORT_START      /* IN6 */
/*	PORT_BITX( 0x10, IP_ACTIVE_HIGH, 0, "strong reset",OSD_KEY_F5, IP_JOY_NONE, 0) */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, 0, "tiles ram view", OSD_KEY_8, IP_JOY_NONE, 0)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, 0, "tile ram - 0x80", OSD_KEY_9, IP_JOY_NONE, 0)
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, 0, "tile ram + 0x80", OSD_KEY_0, IP_JOY_NONE, 0)
	/* end debug */

	PORT_START      /* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* This bit is used.. */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", OSD_KEY_F2, IP_JOY_NONE, 0)

INPUT_PORTS_END

/******************************************************************************/

/* character layout (same for all games) */
static struct GfxLayout charlayout =	/* All games */
{
	8,8,            /* 8 x 8 chars */
	4096,           /* 4096 in total */
	4,              /* 4 bits per pixel */
	{ 0, 1, 2, 3 },    /* planes are packed in a nibble */
	{ 33*4, 32*4, 49*4, 48*4, 1*4, 0*4, 17*4, 16*4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8    /* 32 bytes per char */
};

/* Placeholder and also reminder of how this graphic format is put together */
static struct GfxLayout dummy_mgd2_tilelayout =
{
	16,16,  /* 16*16 sprites */
	20,  /* sprites */
	4,      /* 4 bits per pixel */
	{ /*0x10000*32*8*3*/3, /*0x10000*32*8*2*/2, /*0x10000*32*8*/1, 0 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

/* Placeholder and also reminder of how this graphic format is put together */
static struct GfxLayout dummy_mvs_tilelayout =
{
	16,16,   /* 16*16 sprites */
	20,
	4,
	{ /*0x400000*8 +*/ 8, /*0x400000*8 + */0, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },

	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_mvs_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 16 },
	{ 1, 0x020000, &charlayout, 0, 16 },
	{ 1, 0x000000, &dummy_mvs_tilelayout, 0, 256 },  /* Placeholder */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo neogeo_mgd2_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 16 },
	{ 1, 0x020000, &charlayout, 0, 16 },
	{ 1, 0x000000, &dummy_mvs_tilelayout, 0, 256 },  /* Placeholder */
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct MachineDriver neogeo_mgd2_machine_driver =
{
	{
		{
			CPU_M68000,
			12000000,
			0,
			neogeo_readmem,neogeo_writemem,0,0,
			neogeo_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	neogeo_init_machine,
 	40*8, 28*8, { 1*8, 39*8-1, 0*8, 28*8-1 },
 //	40*8, 56*8, { 1*8, 39*8-1, 0*8, 56*8-1 },

	neogeo_mgd2_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	neogeo_mgd2_vh_start,
	neogeo_vh_stop,
	neogeo_vh_screenrefresh,

	0,0,0,0,
};

static struct MachineDriver neogeo_mvs_machine_driver =
{
	{
		{
			CPU_M68000,
			12000000,
			0,
			neogeo_readmem,neogeo_writemem,0,0,
			neogeo_interrupt,1
		},
#if 0
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,
			5,
			sound_readmem,sound_writemem,0,0,
			interrupt,1
		}
#endif
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	neogeo_init_machine,
	40*8, 28*8, { 1*8, 39*8-1, 0*8, 28*8-1 },
//40*8, 56*8, { 1*8, 39*8-1, 0*8, 56*8-1 },
	neogeo_mvs_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	neogeo_mvs_vh_start,
	neogeo_vh_stop,
	neogeo_vh_screenrefresh,

	0,0,0,0,
};

/******************************************************************************/

/* MGD2 roms */
ROM_START( joyjoy_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n022001a.038", 0x000000, 0x040000, 0xea512c9f )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n022001a.378", 0x000000, 0x20000, 0x6956d778 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x80000)
    ROM_LOAD( "n022001a.538", 0x000000, 0x80000, 0xcb27be65 ) /* Plane 0,1 */

	ROM_REGION(0x80000)
    ROM_LOAD( "n022001a.638", 0x000000, 0x80000, 0x7e134979 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( ridhero_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0xdabfac95 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0xeb5189f0 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x100000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x24096241 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x080000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x7026a3a2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )

	ROM_REGION(0x100000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0xdf6a5b00 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x080000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x15220d51 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( ttbb_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0xefb016a2 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0x7015b8fc )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x100000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x746bf48a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x080000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x57bdcec0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )

	ROM_REGION(0x100000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x5c123d9c ) /* Plane 2,3 */
	ROM_CONTINUE(             0x080000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x2f4bb615 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( lresort_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x5f0a5a4b )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0x5cef5cc6 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x9f7995a9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x080000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0xe122b155 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0xe7138cb9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x68c70bac ) /* Plane 2,3 */
	ROM_CONTINUE(             0x080000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0xf18a9b02 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x08178e27 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( ararmy_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x99c7b4fc )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0xac0daa1b )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0xe3afaf17 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x17098f54 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x13cbb7c5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x3a098b3b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x8e3b2b88 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0xb6f5fc62 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( fbfrenzy_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0xc9fc879c )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0x8472ed44 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0xcd377680 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x2f6d09c2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x9abe41c8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x8b76358f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x77e45dd2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x336540a8 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( alpham2_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n082001a.038", 0x000000, 0x040000, 0x4400b34c )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n082001a.03c", 0x080000, 0x040000, 0xb0366875 )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n082001a.378", 0x000000, 0x20000, 0x85ec9acf )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "n082001a.538", 0x000000, 0x40000, 0xc516b09e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n082001a.53c", 0x040000, 0x40000, 0xd9a0ff6c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n082001b.538", 0x080000, 0x40000, 0x3a7fe4fd ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n082001a.638", 0x000000, 0x40000, 0x6b674581 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n082001a.63c", 0x040000, 0x40000, 0x4ff21008 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n082001b.638", 0x080000, 0x40000, 0xd0e8eef3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( trally_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x400bed38 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n046001a.03c", 0x080000, 0x040000, 0x77196e9a )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0xfff62ae3 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x4d002ecb ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0xb0be56db ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x2f213750 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x6b2f79de ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x091f38b4 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x268be38b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( eightman_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0xe23e2631 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0xa402202b )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0xc916c9bf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x4b057b13 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x12d53af0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x7114bce3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x51da9a34 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x43cf58f9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( ncombat_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x89660a31 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0xd49afee8 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x0147a4b5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x4df6123a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x19441c78 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0xe3d367f8 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x1c1f6101 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0xf417f9ac ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( socbrawl_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x9fa17911)
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0x4c117174)
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x945bf77c) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x9a210d0d) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x057dbb51) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x6708fc58) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x9a97c680) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x434c17c8) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( bstars_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n042001a.038", 0x000000, 0x040000, 0x68ce5b06)
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n138001a.378", 0x000000, 0x20000, 0x1a7fd0c6)
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "n138001a.538", 0x000000, 0x40000, 0xc55a7229) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n138001a.53c", 0x040000, 0x40000, 0xa0074bd9) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n138001b.538", 0x080000, 0x40000, 0xd57767e6) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n138001a.638", 0x000000, 0x40000, 0xcd3eeb2d) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n138001a.63c", 0x040000, 0x40000, 0xd651fecf) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n138001b.638", 0x080000, 0x40000, 0x5d0a8692) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( gpilots_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0xfc5837c0 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0x47a641da )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n058001a.378", 0x000000, 0x20000, 0xa6d83d53 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.538", 0x000000, 0x40000, 0x92b8ee5f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.53c", 0x040000, 0x40000, 0x8c8e42e9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.538", 0x080000, 0x40000, 0x4f12268b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.53c", 0x0c0000, 0x40000, 0x7c3c9c7e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.638", 0x000000, 0x40000, 0x05733639 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.63c", 0x040000, 0x40000, 0x347fef2b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.638", 0x080000, 0x40000, 0x2c586176 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.63c", 0x0c0000, 0x40000, 0x9b2eee8b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( crsword_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x42c78fe1 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0x74651f27 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x4b373de7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0xcddf6d69 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x61d25cb3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n046001b.53c", 0x0c0000, 0x40000, 0x00bc3d84 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0xe05f5f33 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x91ab11a4 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x01357559 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n046001b.63c", 0x0c0000, 0x40000, 0x28c6d19a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( cyberlip_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n062001a.038", 0x000000, 0x040000, 0x4012e465 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n062001a.03c", 0x080000, 0x040000, 0xee899b19 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n062001a.378", 0x000000, 0x20000, 0x79a35264 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n062001a.538", 0x000000, 0x40000, 0x0239fc08 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n062001a.53c", 0x040000, 0x40000, 0x23f03b97 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n062001b.538", 0x080000, 0x40000, 0x065061ed ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n062001b.53c", 0x0c0000, 0x40000, 0xe485e72d ) /* Plane 0,1 - unnecessary? */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n062001a.638", 0x000000, 0x40000, 0xe102a1bd ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n062001a.63c", 0x040000, 0x40000, 0x520571d2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n062001b.638", 0x080000, 0x40000, 0x2e31e2d2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n062001b.63c", 0x0c0000, 0x40000, 0x9e2af4db ) /* Plane 2,3 - unnecessary? */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( kingofm_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0xd239c184 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0x7291a388 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n058001a.378", 0x000000, 0x20000, 0x1a2eeeb3 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.538", 0x000000, 0x40000, 0x493db90e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.53c", 0x040000, 0x40000, 0x0d211945 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.538", 0x080000, 0x40000, 0xcabb7b58 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.53c", 0x0c0000, 0x40000, 0xc7c20718 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.638", 0x000000, 0x40000, 0x8bc1c3a0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.63c", 0x040000, 0x40000, 0xcc793bbf ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.638", 0x080000, 0x40000, 0xfde45b59 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.63c", 0x0c0000, 0x40000, 0xb89b4201 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( burningf_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n054001a.038", 0x000000, 0x040000, 0x188c5e11 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n054001a.378", 0x000000, 0x20000, 0x6799ea0d )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.538", 0x000000, 0x40000, 0x4ddc137b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.53c", 0x040000, 0x40000, 0x896d8545 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.538", 0x080000, 0x40000, 0x2b2cb196 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.53c", 0x0c0000, 0x40000, 0x0f49caa9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.638", 0x000000, 0x40000, 0x7d7d87dc ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.63c", 0x040000, 0x40000, 0x39fe5307 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.638", 0x080000, 0x40000, 0x03aa8a36 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.63c", 0x0c0000, 0x40000, 0xf759626e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( mutnat_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n054001a.038", 0x000000, 0x040000, 0x30cbd46b )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n054001a.378", 0x000000, 0x20000, 0x99419733 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.538", 0x000000, 0x40000, 0x83d59ccf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.53c", 0x040000, 0x40000, 0xb2f1409d ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.538", 0x080000, 0x40000, 0xeaa2801a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.53c", 0x0c0000, 0x40000, 0xc718b731 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.638", 0x000000, 0x40000, 0x9e115a04 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.63c", 0x040000, 0x40000, 0x1bb648c1 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.638", 0x080000, 0x40000, 0x32bf4a2d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.63c", 0x0c0000, 0x40000, 0x7d120067 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( lbowling_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n050001a.038", 0x000000, 0x040000, 0x380e358d )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n050001a.378", 0x000000, 0x20000, 0x5fcdc0ed )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n050001a.538", 0x000000, 0x40000, 0x17df7955 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n050001a.53c", 0x040000, 0x40000, 0x67bf2d89 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n050001b.538", 0x080000, 0x40000, 0x00d36f90 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n050001b.53c", 0x0c0000, 0x40000, 0x4e971be9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n050001a.638", 0x000000, 0x40000, 0x84fd2c90 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n050001a.63c", 0x040000, 0x40000, 0xcb4fbeb0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n050001b.638", 0x080000, 0x40000, 0xc2ddf431 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n050001b.63c", 0x0c0000, 0x40000, 0xe67f8c81 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( fatfury1_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0x47e51379 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0x19d36805 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n058001a.378", 0x000000, 0x20000, 0x3c3bdf8c )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.538", 0x000000, 0x40000, 0x9aaa6d73 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.53c", 0x040000, 0x40000, 0xa986f4a9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.538", 0x080000, 0x40000, 0x7aefe57d ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.53c", 0x0c0000, 0x40000, 0xe3057c96 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.638", 0x000000, 0x40000, 0x9cae3703 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.63c", 0x040000, 0x40000, 0x308b619f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.638", 0x080000, 0x40000, 0xb39a0cde ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.63c", 0x0c0000, 0x40000, 0x737bc030 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( ncommand_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n054001a.038", 0x000000, 0x040000, 0xfdaaca42)
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n054001a.03c", 0x080000, 0x040000, 0xb34e91fe)
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n054001a.378", 0x000000, 0x20000, 0xdb8f9c8e)
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.538", 0x000000, 0x40000, 0x73acaa79) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.53c", 0x040000, 0x40000, 0xad56623d) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.538", 0x080000, 0x40000, 0xc8d763cd) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.53c", 0x0c0000, 0x40000, 0x63829529) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.638", 0x000000, 0x40000, 0x7b24359f) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.63c", 0x040000, 0x40000, 0x0913a784) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.638", 0x080000, 0x40000, 0x574612ec) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.63c", 0x0c0000, 0x40000, 0x990d302a) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( sengoku_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0x4483bae1 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0xd0d55b2a )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n058001a.378", 0x000000, 0x20000, 0xb246204d )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.538", 0x000000, 0x40000, 0xe834b925 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.53c", 0x040000, 0x40000, 0x66be6d46 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.538", 0x080000, 0x40000, 0x443c552c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.53c", 0x0c0000, 0x40000, 0xecb41adc ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.638", 0x000000, 0x40000, 0x96de5eb9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.63c", 0x040000, 0x40000, 0x25f5fd7b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.638", 0x080000, 0x40000, 0xafbd5b0b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.63c", 0x0c0000, 0x40000, 0x78b25278 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( countb_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n106001a.038", 0x000000, 0x040000, 0x10dbe66a)
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n106001a.03c", 0x080000, 0x040000, 0x6d5bfb61)
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n106001a.378", 0x000000, 0x20000, 0xc362d484)
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "n106001a.538", 0x000000, 0x40000, 0xbe0d2fe0) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n106001a.53c", 0x040000, 0x40000, 0xfdb4df65) /* Plane 0,1 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n106001b.538", 0x080000, 0x40000, 0x714c2c01) /* Plane 0,1 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n106001b.53c", 0x0c0000, 0x40000, 0xc57ce8b0) /* Plane 0,1 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n106001c.538", 0x100000, 0x40000, 0x2e7e59df) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n106001c.53c", 0x140000, 0x40000, 0xa93185ce) /* Plane 0,1 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n106001d.538", 0x180000, 0x40000, 0x410938c5) /* Plane 0,1 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n106001d.53c", 0x1c0000, 0x40000, 0x50d66909) /* Plane 0,1 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x400000)
    ROM_LOAD( "n106001a.638", 0x000000, 0x40000, 0xf56dafa5) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n106001a.63c", 0x040000, 0x40000, 0xf2f68b2a) /* Plane 2,3 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n106001b.638", 0x080000, 0x40000, 0xb15a7f25) /* Plane 2,3 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n106001b.63c", 0x0c0000, 0x40000, 0x25f00cd3) /* Plane 2,3 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n106001c.638", 0x100000, 0x40000, 0x341438e4) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n106001c.63c", 0x140000, 0x40000, 0xfb8adce8) /* Plane 2,3 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n106001d.638", 0x180000, 0x40000, 0x74d995c5) /* Plane 2,3 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n106001d.63c", 0x1c0000, 0x40000, 0x521b6df1) /* Plane 2,3 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( androdun_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0x4639b419 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0x11beb098 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n138001a.378", 0x000000, 0x20000, 0x6349de5d )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "n138001a.538", 0x000000, 0x40000, 0xca08e432 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n138001a.53c", 0x040000, 0x40000, 0xfcbcb305 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n138001b.538", 0x080000, 0x40000, 0x806ab937 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n138001b.53c", 0x0c0000, 0x40000, 0xe7e1a2be ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can be tossed */
    ROM_LOAD( "n138001c.538", 0x100000, 0x40000, 0xca08e432 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n138001c.53c", 0x140000, 0x40000, 0xfcbcb305 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n138001d.538", 0x180000, 0x40000, 0x806ab937 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n138001d.53c", 0x1c0000, 0x40000, 0xe7e1a2be ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x400000)
    ROM_LOAD( "n138001a.638", 0x000000, 0x40000, 0x7a0deb9e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n138001a.63c", 0x040000, 0x40000, 0xb1c640f5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n138001b.638", 0x080000, 0x40000, 0x33bee10f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n138001b.63c", 0x0c0000, 0x40000, 0x70f0d263 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can be tossed */
    ROM_LOAD( "n138001c.638", 0x100000, 0x40000, 0x7a0deb9e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n138001c.63c", 0x140000, 0x40000, 0xb1c640f5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n138001d.638", 0x180000, 0x40000, 0x33bee10f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n138001d.63c", 0x1c0000, 0x40000, 0x70f0d263 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( artfight_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n102001a.038", 0x000000, 0x040000, 0x95102254 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n102001a.378", 0x000000, 0x20000, 0x89903f39 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "n102001a.538", 0x000000, 0x40000, 0xa2e4a168 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n102001a.53c", 0x040000, 0x40000, 0xda389ef7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n102001b.538", 0x080000, 0x40000, 0x2a0c385b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n102001b.53c", 0x0c0000, 0x40000, 0x4a4317bf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n102001c.538", 0x100000, 0x40000, 0x471d9e57 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n102001c.53c", 0x140000, 0x40000, 0x23fe5675 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n102001d.538", 0x180000, 0x40000, 0x204e7b29 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n102001d.53c", 0x1c0000, 0x40000, 0x7f6d5144 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x400000)
    ROM_LOAD( "n102001a.638", 0x000000, 0x40000, 0xca12c80f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n102001a.63c", 0x040000, 0x40000, 0xd59746b0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n102001b.638", 0x080000, 0x40000, 0x8b73b3da ) /* Plane 2,3 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n102001b.63c", 0x0c0000, 0x40000, 0x9fc3f8ea ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n102001c.638", 0x100000, 0x40000, 0xcbf8a72e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n102001c.63c", 0x140000, 0x40000, 0x5ec93c96 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n102001d.638", 0x180000, 0x40000, 0x47763b6d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n102001d.63c", 0x1c0000, 0x40000, 0x4408f4eb ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( superspy_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0x2e949e32 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0x54443d72 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n138001a.378", 0x000000, 0x20000, 0xec5fdb96 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "n138001a.538", 0x000000, 0x40000, 0x239f22c4 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n138001a.53c", 0x040000, 0x40000, 0xce80c326 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n138001b.538", 0x080000, 0x40000, 0x1edcf268 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n138001b.53c", 0x0c0000, 0x40000, 0xa41602a0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can probably be tossed */
    ROM_LOAD( "n138001c.538", 0x100000, 0x40000, 0x239f22c4 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n138001c.53c", 0x140000, 0x40000, 0xce80c326 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n138001d.538", 0x180000, 0x40000, 0x1edcf268 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n138001d.53c", 0x1c0000, 0x40000, 0xa41602a0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x400000)
    ROM_LOAD( "n138001a.638", 0x000000, 0x40000, 0x5f2e5184 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n138001a.63c", 0x040000, 0x40000, 0x79b3e0b1 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n138001b.638", 0x080000, 0x40000, 0xb2afe822 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n138001b.63c", 0x0c0000, 0x40000, 0xd425f967 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can probably be tossed */
    ROM_LOAD( "n138001c.638", 0x100000, 0x40000, 0x5f2e5184 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n138001c.63c", 0x140000, 0x40000, 0x79b3e0b1 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n138001d.638", 0x180000, 0x40000, 0xb2afe822 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n138001d.63c", 0x1c0000, 0x40000, 0xd425f967 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( wheroes_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0xab39923d)
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0x5adc98ef)
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n138001a.378", 0x000000, 0x20000, 0x8c2c2d6b)
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "n138001a.538", 0x000000, 0x40000, 0xad8fcc5d) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n138001a.53c", 0x040000, 0x40000, 0x0dca726e) /* Plane 0,1 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n138001b.538", 0x080000, 0x40000, 0xbb807a43) /* Plane 0,1 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n138001b.53c", 0x0c0000, 0x40000, 0xe913f93c) /* Plane 0,1 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n138001c.538", 0x100000, 0x40000, 0x3c359a14) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n138001c.53c", 0x140000, 0x40000, 0xb1327d84) /* Plane 0,1 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n138001d.538", 0x180000, 0x40000, 0xbb807a43) /* Plane 0,1 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n138001d.53c", 0x1c0000, 0x40000, 0xe913f93c) /* Plane 0,1 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x400000)
    ROM_LOAD( "n138001a.638", 0x000000, 0x40000, 0x3182b4db) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n138001a.63c", 0x040000, 0x40000, 0x1cb0a840) /* Plane 2,3 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n138001b.638", 0x080000, 0x40000, 0xc9f439f8) /* Plane 2,3 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n138001b.63c", 0x0c0000, 0x40000, 0x80441c48) /* Plane 2,3 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n138001c.638", 0x100000, 0x40000, 0x7c4b85b4) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n138001c.63c", 0x140000, 0x40000, 0x959f29db) /* Plane 2,3 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n138001d.638", 0x180000, 0x40000, 0xc9f439f8) /* Plane 2,3 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n138001d.63c", 0x1c0000, 0x40000, 0x80441c48) /* Plane 2,3 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( sengoku2_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0xd1bf3fa5 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0xee9d0bb4 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "n138001a.378", 0x000000, 0x20000, 0xcd9802a3 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "n138001a.538", 0x000000, 0x40000, 0xda18aaed ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n138001a.53c", 0x040000, 0x40000, 0x19796c4f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n138001b.538", 0x080000, 0x40000, 0x891b6386 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n138001b.53c", 0x0c0000, 0x40000, 0x891b6386 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n138001c.538", 0x100000, 0x40000, 0xc5eaabe8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n138001c.53c", 0x140000, 0x40000, 0x22633905 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n138001d.538", 0x180000, 0x40000, 0x891b6386 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n138001d.53c", 0x1c0000, 0x40000, 0x891b6386 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x400000)
    ROM_LOAD( "n138001a.638", 0x000000, 0x40000, 0x5b27c829 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n138001a.63c", 0x040000, 0x40000, 0xe8b46e26 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n138001b.638", 0x080000, 0x40000, 0x93d25955 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n138001b.63c", 0x0c0000, 0x40000, 0x93d25955 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n138001c.638", 0x100000, 0x40000, 0x432bd7d0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n138001c.63c", 0x140000, 0x40000, 0xba3f54b2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n138001d.638", 0x180000, 0x40000, 0x93d25955 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n138001d.63c", 0x1c0000, 0x40000, 0x93d25955 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

/************************** MVS FORMAT ROMS ***********************************/

ROM_START( puzzledp_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "pdpon_p1.rom", 0x000000, 0x080000, 0x2b61415b )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "pdpon_s1.rom", 0x000000, 0x020000, 0xcd19264f )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x100000)
    ROM_LOAD( "pdpon_c1.rom", 0x000000, 0x100000, 0xcc0095ef ) /* Plane 0,1 */

	ROM_REGION(0x100000)
    ROM_LOAD( "pdpon_c2.rom", 0x000000, 0x100000, 0x42371307 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( mahretsu_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "maj_p1.rom", 0x000000, 0x080000, 0xfc6f53db )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "maj_s1.rom",  0x000000, 0x10000, 0xb0d16529 )
    ROM_LOAD( "ng-sfix.rom", 0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x100000) /* Plane 0,1 */
    ROM_LOAD( "maj_c1.rom", 0x000000, 0x80000, 0xf1ae16bc )
    ROM_LOAD( "maj_c3.rom", 0x080000, 0x80000, 0x9c571a37 )

	ROM_REGION(0x100000) /* Plane 2,3 */
    ROM_LOAD( "maj_c2.rom", 0x000000, 0x80000, 0xbdc13520 )
    ROM_LOAD( "maj_c4.rom", 0x080000, 0x80000, 0x7e81cb29 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( bjourney_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "bj-p1.rom", 0x000000, 0x100000, 0x6a2f6d4a )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "bj-s1.rom",   0x00000, 0x20000, 0x843c3624 )
    ROM_LOAD( "ng-sfix.rom", 0x20000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "bj-c1.rom", 0x000000, 0x100000, 0x4d47a48c )
    ROM_LOAD( "bj-c3.rom", 0x100000, 0x080000, 0x66e69753 )

	ROM_REGION(0x180000)
    ROM_LOAD( "bj-c2.rom", 0x000000, 0x100000, 0xe8c1491a )
    ROM_LOAD( "bj-c4.rom", 0x100000, 0x080000, 0x71bfd48a )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( nam_1975_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "nam_p1.rom", 0x000000, 0x080000, 0xcc9fc951 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "nam_s1.rom",  0x00000, 0x20000, 0x7988ba51 )
    ROM_LOAD( "ng-sfix.rom", 0x20000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "nam_c1.rom", 0x000000, 0x80000, 0x32ea98e1 ) /* Plane 0,1 */
    ROM_LOAD( "nam_c3.rom", 0x080000, 0x80000, 0x0151054c ) /* Plane 0,1 */
    ROM_LOAD( "nam_c5.rom", 0x100000, 0x80000, 0x90b74cc2 ) /* Plane 0,1 */

	ROM_REGION(0x180000)
    ROM_LOAD( "nam_c2.rom", 0x000000, 0x80000, 0xcbc4064c ) /* Plane 2,3 */
    ROM_LOAD( "nam_c4.rom", 0x080000, 0x80000, 0x0a32570d ) /* Plane 2,3 */
    ROM_LOAD( "nam_c6.rom", 0x100000, 0x80000, 0xe62bed58 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( maglord_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "magl_p1.rom", 0x000000, 0x080000, 0xbd0a492d )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "magl_s1.rom", 0x00000, 0x20000, 0x1c5369a2 )
    ROM_LOAD( "ng-sfix.rom", 0x20000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "magl_c1.rom", 0x000000, 0x80000, 0x806aee34 ) /* Plane 0,1 */
    ROM_LOAD( "magl_c3.rom", 0x080000, 0x80000, 0xc4c2b926 ) /* Plane 0,1 */
    ROM_LOAD( "magl_c5.rom", 0x100000, 0x80000, 0x69086dec ) /* Plane 0,1 */

	ROM_REGION(0x180000)
    ROM_LOAD( "magl_c2.rom", 0x000000, 0x80000, 0x34aa9a86 ) /* Plane 2,3 */
    ROM_LOAD( "magl_c4.rom", 0x080000, 0x80000, 0x9c46dcf4 ) /* Plane 2,3 */
    ROM_LOAD( "magl_c6.rom", 0x100000, 0x80000, 0xab7ac142 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( cybrlipa_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "cybl_p1.rom", 0x000000, 0x080000, 0x69a6b42d )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "cybl_s1.rom", 0x00000, 0x20000, 0x79a35264 )
    ROM_LOAD( "ng-sfix.rom", 0x20000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "cybl_c1.rom", 0x000000, 0x80000, 0x8bba5113 ) /* Plane 0,1 */
    ROM_LOAD( "cybl_c3.rom", 0x080000, 0x80000, 0xe4f86efc ) /* Plane 0,1 */
    ROM_LOAD( "cybl_c5.rom", 0x100000, 0x80000, 0xe8076da0 ) /* Plane 0,1 */

	ROM_REGION(0x180000)
    ROM_LOAD( "cybl_c2.rom", 0x000000, 0x80000, 0xcbf66432 ) /* Plane 2,3 */
    ROM_LOAD( "cybl_c4.rom", 0x080000, 0x80000, 0xf7be4674 ) /* Plane 2,3 */
    ROM_LOAD( "cybl_c6.rom", 0x100000, 0x80000, 0xc495c567 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( tpgolf_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "topg_p1.rom", 0x000000, 0x080000, 0xf75549ba )
    ROM_LOAD_WIDE_SWAP( "topg_p2.rom", 0x080000, 0x080000, 0xb7809a8f )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "topg_s1.rom", 0x000000, 0x20000, 0x7b3eb9b1 )
    ROM_LOAD( "ng-sfix.rom", 0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "topg_c1.rom", 0x000000, 0x80000, 0x0315fbaf )
    ROM_LOAD( "topg_c3.rom", 0x080000, 0x80000, 0xb09f1612 )
    ROM_LOAD( "topg_c5.rom", 0x100000, 0x80000, 0x9a7146da )

	ROM_REGION(0x180000)
    ROM_LOAD( "topg_c2.rom", 0x000000, 0x80000, 0xb4c15d59 )
    ROM_LOAD( "topg_c4.rom", 0x080000, 0x80000, 0x150ea7a1 )
    ROM_LOAD( "topg_c6.rom", 0x100000, 0x80000, 0x1e63411a )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( legendos_rom )
	ROM_REGION(0x080000)
    ROM_LOAD_WIDE_SWAP( "joe_p1.rom", 0x000000, 0x080000, 0x9d563f19 )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "joe_s1.rom",  0x000000, 0x020000, 0xbcd502f0 )
    ROM_LOAD( "ng-sfix.rom", 0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "joe_c1.rom", 0x000000, 0x100000, 0x2f5ab875 )
    ROM_LOAD( "joe_c3.rom", 0x100000, 0x100000, 0x6bc52cb2 )

	ROM_REGION(0x200000)
    ROM_LOAD( "joe_c2.rom", 0x000000, 0x100000, 0x318b2711 )
    ROM_LOAD( "joe_c4.rom", 0x100000, 0x100000, 0x37ef298c )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( bstars2_rom )
	ROM_REGION(0x080000)
    ROM_LOAD_WIDE_SWAP( "star2_p1.rom", 0x000000, 0x080000, 0x523567fd )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "star2_s1.rom", 0x000000, 0x020000, 0x015c5c94 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "star2_c1.rom", 0x000000, 0x100000, 0xb39a12e1 )
    ROM_LOAD( "star2_c3.rom", 0x100000, 0x100000, 0xfb31339d )

	ROM_REGION(0x200000)
    ROM_LOAD( "star2_c2.rom", 0x000000, 0x100000, 0x766cfc2f )
    ROM_LOAD( "star2_c4.rom", 0x100000, 0x100000, 0x70457a0c )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( wjammers_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "windj_p1.rom", 0x000000, 0x100000, 0x6692c140 )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "windj_s1.rom", 0x000000, 0x020000, 0x66cb96eb )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "windj_c1.rom", 0x000000, 0x100000, 0xc7650204 )
    ROM_LOAD( "windj_c3.rom", 0x100000, 0x100000, 0x40986386 )

	ROM_REGION(0x200000)
    ROM_LOAD( "windj_c2.rom", 0x000000, 0x100000, 0xd9f3e71d )
    ROM_LOAD( "windj_c4.rom", 0x100000, 0x100000, 0x715e15ff )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( janshin_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "jans-p1.rom", 0x000000, 0x100000, 0x7514cb7a )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "jans-s1.rom", 0x000000, 0x020000, 0x8285b25a )
    ROM_LOAD( "ng-sfix.rom", 0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "jans-c1.rom", 0x000000, 0x200000, 0x3fa890e9 )

	ROM_REGION(0x200000)
    ROM_LOAD( "jans-c2.rom", 0x000000, 0x200000, 0x59c48ad8 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( neomrdo_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "neomd-p1.rom", 0x000000, 0x100000, 0x334ea51e )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "neomd-s1.rom", 0x000000, 0x020000, 0x6aebafce )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x200000)
    ROM_LOAD( "neomd-c1.rom", 0x000000, 0x200000, 0xc7541b9d )

	ROM_REGION(0x200000)
    ROM_LOAD( "neomd-c2.rom", 0x000000, 0x200000, 0xf57166d2 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( pbobble_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "puzzb_p1.rom", 0x000000, 0x080000, 0x6102ca14 )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "puzzb_s1.rom", 0x000000, 0x020000, 0x9caae538 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x280000)
    ROM_LOAD( "puzzb_c1.rom", 0x000000, 0x100000, 0x7f250f76 ) /* Plane 0,1 */
    ROM_LOAD( "puzzb_c3.rom", 0x100000, 0x100000, 0x4b641ba1 ) /* Plane 0,1 */
    ROM_LOAD( "puzzb_c5.rom", 0x200000, 0x080000, 0xe89ad494 ) /* Plane 0,1 */

	ROM_REGION(0x280000)
    ROM_LOAD( "puzzb_c2.rom", 0x000000, 0x100000, 0x20912873 ) /* Plane 2,3 */
    ROM_LOAD( "puzzb_c4.rom", 0x100000, 0x100000, 0x35072596 ) /* Plane 2,3 */
    ROM_LOAD( "puzzb_c6.rom", 0x200000, 0x080000, 0x4b42d7eb ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
#if 0
	ROM_REGION(0x20000)
    ROM_LOAD( "puzzb_m1.rom", 0x00000, 0x020000, 0x9036d879 )
#endif
ROM_END

ROM_START( pspikes2_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "spike_p1.rom", 0x000000, 0x100000, 0x105a408f )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "spike_s1.rom", 0x000000, 0x020000, 0x18082299 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x300000)
    ROM_LOAD( "spike_c1.rom", 0x000000, 0x100000, 0x7f250f76 ) /* Plane 0,1 */
    ROM_LOAD( "spike_c3.rom", 0x100000, 0x100000, 0x4b641ba1 ) /* Plane 0,1 */
    ROM_LOAD( "spike_c5.rom", 0x200000, 0x100000, 0x151dd624 ) /* Plane 0,1 */

	ROM_REGION(0x300000)
    ROM_LOAD( "spike_c2.rom", 0x000000, 0x100000, 0x20912873 ) /* Plane 2,3 */
    ROM_LOAD( "spike_c4.rom", 0x100000, 0x100000, 0x35072596 ) /* Plane 2,3 */
    ROM_LOAD( "spike_c6.rom", 0x200000, 0x100000, 0xa6722604 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( sidkicks_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "sidek_p1.rom", 0x000000, 0x080000, 0x9cd97256 )

	ROM_REGION_DISPOSE(0x40000)	  /* 6 meg temporary */
    ROM_LOAD( "sidek_s1.rom", 0x000000, 0x020000, 0x97689804 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x300000)
    ROM_LOAD( "sidek_c1.rom", 0x000000, 0x100000, 0x53e1c002 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x100000 )

	ROM_REGION(0x300000) /* CHECK THIS LATER */
    ROM_LOAD( "sidek_c2.rom", 0x000000, 0x100000, 0x776a2d1f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x100000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( panicbom_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "panic_p1.rom", 0x000000, 0x080000, 0xadc356ad )

	ROM_REGION_DISPOSE(0x40000)	  /* 6 meg temporary */
    ROM_LOAD( "panic_s1.rom", 0x000000, 0x020000, 0xb876de7e )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x300000)
    ROM_LOAD( "panic_c1.rom", 0x000000, 0x100000, 0x314534d6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x100000 )

	ROM_REGION(0x300000) /* CHECK THIS LATER */
    ROM_LOAD( "panic_c2.rom", 0x000000, 0x100000, 0x39a8c65a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x100000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( viewpoin_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "viewp_p1.rom", 0x000000, 0x100000, 0x17aa899d )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "viewp_s1.rom", 0x000000, 0x020000, 0x9fea5758 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x300000)
    ROM_LOAD( "viewp_c1.rom", 0x000000, 0x100000, 0xd624c132 )
	ROM_CONTINUE(             0x200000, 0x100000 )

	ROM_REGION(0x300000)
    ROM_LOAD( "viewp_c2.rom", 0x000000, 0x100000, 0x40d69f1e )
	ROM_CONTINUE(             0x200000, 0x100000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( zedblade_rom )
    ROM_REGION(0x080000)
    ROM_LOAD_WIDE_SWAP( "zedbl_p1.rom", 0x000000, 0x080000, 0xd7c1effd )

    ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "zedbl_s1.rom", 0x000000, 0x020000, 0xf4c25dd5 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

    ROM_REGION(0x400000)
    ROM_LOAD( "zedbl_c1.rom", 0x000000, 0x200000, 0x4d9cb038 ) /* Plane 0,1 */
    ROM_LOAD( "zedbl_c3.rom", 0x200000, 0x200000, 0xd06431e3 ) /* Plane 0,1 */

    ROM_REGION(0x400000)
    ROM_LOAD( "zedbl_c2.rom", 0x000000, 0x200000, 0x09233884 ) /* Plane 2,3 */
    ROM_LOAD( "zedbl_c4.rom", 0x200000, 0x200000, 0x4b1c089b ) /* Plane 2,3 */

    ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( kotm2_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "kotm2_p1.rom", 0x000000, 0x080000, 0xb372d54c )
    ROM_LOAD_WIDE_SWAP( "kotm2_p2.rom", 0x080000, 0x080000, 0x28661afe )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "kotm2_s1.rom", 0x000000, 0x020000, 0x63ee053a )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "kotm2_c1.rom", 0x000000, 0x100000, 0x6d1c4aa9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x100000 )
    ROM_LOAD( "kotm2_c3.rom", 0x100000, 0x100000, 0x40156dca ) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x100000 )

	ROM_REGION(0x400000)
    ROM_LOAD( "kotm2_c2.rom", 0x000000, 0x100000, 0xf7b75337 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x100000 )
    ROM_LOAD( "kotm2_c4.rom", 0x100000, 0x100000, 0xb0d44111 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x100000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( stakwin_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "stakw_p1.rom",  0x100000, 0x100000, 0xbd5814f6 )
	ROM_CONTINUE(                        0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "stakw_s1.rom", 0x000000, 0x020000, 0x073cb208 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "stakw_c1.rom", 0x000000, 0x200000, 0x6e733421 ) /* Plane 0,1 */
    ROM_LOAD( "stakw_c3.rom", 0x200000, 0x200000, 0x8fa5a9eb ) /* Plane 0,1 */

	ROM_REGION(0x400000)
    ROM_LOAD( "stakw_c2.rom", 0x000000, 0x200000, 0x4d865347 ) /* Plane 2,3 */
    ROM_LOAD( "stakw_c4.rom", 0x200000, 0x200000, 0x4604f0dc ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( turfmast_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "turfm_p1.rom",  0x100000, 0x100000, 0x28c83048 )
	ROM_CONTINUE(                        0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	ROM_REGION_DISPOSE(0x40000)
	ROM_LOAD( "turfm_s1.rom", 0x000000, 0x020000, 0x9a5402b2 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x400000)
	ROM_LOAD( "turfm_c1.rom", 0x200000, 0x200000, 0x8c6733f2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )

	ROM_REGION(0x400000)
	ROM_LOAD( "turfm_c2.rom", 0x200000, 0x200000, 0x596cc256 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( neodrift_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "drift_p1.rom",  0x100000, 0x100000, 0xe397d798 )
	ROM_CONTINUE(                        0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	ROM_REGION_DISPOSE(0x40000)
	ROM_LOAD( "drift_s1.rom", 0x000000, 0x020000, 0xb76b61bc )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x400000)
	ROM_LOAD( "drift_c1.rom", 0x200000, 0x200000, 0x62c5edc9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )

	ROM_REGION(0x400000)
	ROM_LOAD( "drift_c2.rom", 0x200000, 0x200000, 0x9dc9c72a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( fatfury2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "fury2_p1.rom", 0x000000, 0x080000, 0xbe40ea92 )
	ROM_LOAD_WIDE_SWAP( "fury2_p2.rom", 0x080000, 0x080000, 0x2a9beac5 )

	ROM_REGION_DISPOSE(0x40000)
	ROM_LOAD( "fury2_s1.rom", 0x000000, 0x020000, 0xd7dbbf39 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x400000)
	ROM_LOAD( "fury2_c1.rom", 0x000000, 0x100000, 0xf72a939e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x100000 )
	ROM_LOAD( "fury2_c3.rom", 0x100000, 0x100000, 0x01e00738 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x100000 )

	ROM_REGION(0x400000)
	ROM_LOAD( "fury2_c2.rom", 0x000000, 0x100000, 0x05119a0d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x100000 )
	ROM_LOAD( "fury2_c4.rom", 0x100000, 0x100000, 0x9fe27432 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x100000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( sonicwi2_rom )
    ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "afig2_p1.rom", 0x100000, 0x100000, 0x92871738 )
    ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

    ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "afig2_s1.rom", 0x000000, 0x020000, 0x47cc6177 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

    ROM_REGION(0x400000)
    ROM_LOAD( "afig2_c1.rom", 0x000000, 0x200000, 0x3278e73e ) /* Plane 0,1 */
    ROM_LOAD( "afig2_c3.rom", 0x200000, 0x200000, 0xc1b438f1 ) /* Plane 0,1 */

    ROM_REGION(0x400000)
    ROM_LOAD( "afig2_c2.rom", 0x000000, 0x200000, 0xfe6355d6 ) /* Plane 2,3 */
    ROM_LOAD( "afig2_c4.rom", 0x200000, 0x200000, 0x1f777206 ) /* Plane 2,3 */

    ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( ssideki2_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "kick2_p1.rom", 0x000000, 0x100000, 0x5969e0dc )

    ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "kick2_s1.rom", 0x000000, 0x020000, 0x226d1b68 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

    ROM_REGION(0x400000)
    ROM_LOAD( "kick2_c1.rom", 0x000000, 0x200000, 0xa626474f ) /* Plane 0,1 */
    ROM_LOAD( "kick2_c3.rom", 0x200000, 0x200000, 0x2a7b98b9 ) /* Plane 0,1 */

    ROM_REGION(0x400000)
    ROM_LOAD( "kick2_c2.rom", 0x000000, 0x200000, 0xc3be42ae ) /* Plane 2,3 */
    ROM_LOAD( "kick2_c4.rom", 0x200000, 0x200000, 0xc0be9a1f ) /* Plane 2,3 */

    ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( tophuntr_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "thunt_p1.rom", 0x000000, 0x100000, 0x69fa9e29 )
    ROM_LOAD_WIDE_SWAP( "thunt_p2.rom", 0x100000, 0x100000, 0xf182cb3e )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "thunt_s1.rom", 0x000000, 0x020000, 0x6a454dd1 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "thunt_c1.rom", 0x000000, 0x100000, 0xfa720a4a ) /* Plane 0,1 */
    ROM_LOAD( "thunt_c3.rom", 0x100000, 0x100000, 0x880e3c25 ) /* Plane 0,1 */
    ROM_LOAD( "thunt_c5.rom", 0x200000, 0x100000, 0x4b735e45 ) /* Plane 0,1 */
    ROM_LOAD( "thunt_c7.rom", 0x300000, 0x100000, 0x12829c4c ) /* Plane 0,1 */

	ROM_REGION(0x400000)
    ROM_LOAD( "thunt_c2.rom", 0x000000, 0x100000, 0xc900c205 ) /* Plane 2,3 */
    ROM_LOAD( "thunt_c4.rom", 0x100000, 0x100000, 0x7a2248aa ) /* Plane 2,3 */
    ROM_LOAD( "thunt_c6.rom", 0x200000, 0x100000, 0x273171df ) /* Plane 2,3 */
    ROM_LOAD( "thunt_c8.rom", 0x300000, 0x100000, 0xc944e03d ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( spinmast_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "spnm_p1.rom", 0x000000, 0x100000, 0x37aba1aa )
    ROM_LOAD_WIDE_SWAP( "spnm_p2.rom", 0x100000, 0x100000, 0xf025ab77 )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "spnm_s1.rom", 0x000000, 0x020000, 0x289e2bbe )
    ROM_LOAD( "ng-sfix.rom", 0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "spnm_c1.rom", 0x000000, 0x100000, 0xa9375aa2 ) /* Plane 0,1 */
    ROM_LOAD( "spnm_c3.rom", 0x100000, 0x100000, 0xdf51e465 ) /* Plane 0,1 */
    ROM_LOAD( "spnm_c5.rom", 0x200000, 0x100000, 0x7babd692 ) /* Plane 0,1 */
    ROM_LOAD( "spnm_c7.rom", 0x300000, 0x100000, 0xbb2fd7c0 ) /* Plane 0,1 */

	ROM_REGION(0x400000)
    ROM_LOAD( "spnm_c2.rom", 0x000000, 0x100000, 0x0e73b758 ) /* Plane 2,3 */
    ROM_LOAD( "spnm_c4.rom", 0x100000, 0x100000, 0x38517e90 ) /* Plane 2,3 */
    ROM_LOAD( "spnm_c6.rom", 0x200000, 0x100000, 0xcde5ade5 ) /* Plane 2,3 */
    ROM_LOAD( "spnm_c8.rom", 0x300000, 0x100000, 0x8d7be933 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( samsho_rom )
	ROM_REGION(0x180000)
    ROM_LOAD_WIDE_SWAP( "samsh_p1.rom", 0x000000, 0x080000, 0x80aa6c97 )
    ROM_LOAD_WIDE_SWAP( "samsh_p2.rom", 0x080000, 0x080000, 0x71768728 )
    ROM_LOAD_WIDE_SWAP( "samsh_p3.rom", 0x100000, 0x080000, 0x38ee9ba9 )

	ROM_REGION_DISPOSE(0x40000)
    ROM_LOAD( "samsh_s1.rom", 0x000000, 0x020000, 0x9142a4d3 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x480000)
    ROM_LOAD( "samsh_c1.rom", 0x000000, 0x200000, 0x2e5873a4 ) /* Plane 0,1 */
    ROM_LOAD( "samsh_c3.rom", 0x200000, 0x200000, 0xf3dabd1e ) /* Plane 0,1 */
    ROM_LOAD( "samsh_c5.rom", 0x400000, 0x080000, 0xa2bb8284 ) /* Plane 0,1 */

	ROM_REGION(0x480000)
    ROM_LOAD( "samsh_c2.rom", 0x000000, 0x200000, 0x04febb10 ) /* Plane 2,3 */
    ROM_LOAD( "samsh_c4.rom", 0x200000, 0x200000, 0x935c62f0 ) /* Plane 2,3 */
    ROM_LOAD( "samsh_c6.rom", 0x400000, 0x080000, 0x4fa71252 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( neobombe_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "bombm_p1.rom", 0x000000, 0x100000, 0xa1a71d0d )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "bombm_s1.rom", 0x000000, 0x020000, 0x4b3fa119 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x480000) /* Plane 0,1 */
    ROM_LOAD( "bombm_c1.rom", 0x200000, 0x200000, 0xb90ebed4 )
	ROM_CONTINUE(0,0x200000)
    ROM_LOAD( "bombm_c3.rom", 0x400000, 0x080000, 0xe37578c5 )

	ROM_REGION(0x480000) /* Plane 2,3 */
    ROM_LOAD( "bombm_c2.rom", 0x200000, 0x200000, 0x41e62b4f )
	ROM_CONTINUE(0,0x200000)
    ROM_LOAD( "bombm_c4.rom", 0x400000, 0x080000, 0x59826783 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( tws96_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "tecmo_p1.rom", 0x000000, 0x100000, 0x03e20ab6 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "tecmo_s1.rom", 0x000000, 0x020000, 0x6f5e2b3a )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

    ROM_REGION(0x500000) /* Plane 0,1 */
    ROM_LOAD( "tecmo_c1.rom", 0x200000, 0x200000, 0xd301a867 )
	ROM_CONTINUE(0,0x200000)
    ROM_LOAD( "tecmo_c3.rom", 0x400000, 0x100000, 0x750ddc0c )

    ROM_REGION(0x500000) /* Plane 2,3 */
    ROM_LOAD( "tecmo_c2.rom", 0x200000, 0x200000, 0x305fc74f )
	ROM_CONTINUE(0,0x200000)
    ROM_LOAD( "tecmo_c4.rom", 0x400000, 0x100000, 0x7a6e7d82 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( twinspri_rom )
	ROM_REGION(0x400000)
    ROM_LOAD_WIDE_SWAP( "sprit_p1.rom", 0x100000, 0x100000, 0x38ba4bff )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )
	ROM_CONTINUE(                       0x200000, 0x200000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "sprit_s1.rom", 0x000000, 0x020000, 0xeeed5758 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

    ROM_REGION(0x500000) /* Plane 0,1 */
    ROM_LOAD( "sprit_c1.rom", 0x200000, 0x200000, 0x73b2a70b )
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "sprit_c3.rom", 0x400000, 0x100000, 0xc59e4129 )

    ROM_REGION(0x500000) /* Plane 2,3 */
    ROM_LOAD( "sprit_c2.rom", 0x200000, 0x200000, 0x3a3e506c )
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "sprit_c4.rom", 0x400000, 0x100000, 0xb5532e53 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( karnov_r_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "karev_p1.rom", 0x000000, 0x100000, 0x8c86fd22 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "karev_s1.rom", 0x000000, 0x020000, 0xbae5d5e5 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x600000)
    ROM_LOAD( "karev_c1.rom", 0x000000, 0x200000, 0x09dfe061 ) /* Plane 0,1 */
    ROM_LOAD( "karev_c3.rom", 0x200000, 0x200000, 0xa673b4f7 ) /* Plane 0,1 */
    ROM_LOAD( "karev_c5.rom", 0x400000, 0x200000, 0x9a28785d ) /* Plane 0,1 */

	ROM_REGION(0x600000)
    ROM_LOAD( "karev_c2.rom", 0x000000, 0x200000, 0xe0f6682a ) /* Plane 2,3 */
    ROM_LOAD( "karev_c4.rom", 0x200000, 0x200000, 0xcb3dc5f4 ) /* Plane 2,3 */
    ROM_LOAD( "karev_c6.rom", 0x400000, 0x200000, 0xc15c01ed ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( fatfursp_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "ffspe_p1.rom", 0x000000, 0x100000, 0x2f585ba2 )
    ROM_LOAD_WIDE_SWAP( "ffspe_p2.rom", 0x100000, 0x080000, 0xd7c71a6b )
    ROM_LOAD_WIDE_SWAP( "ffspe_p3.rom", 0x180000, 0x080000, 0x9f0c1e1a )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "ffspe_s1.rom", 0x000000, 0x020000, 0x2df03197 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x600000)
    ROM_LOAD( "ffspe_c1.rom", 0x000000, 0x200000, 0x044ab13c ) /* Plane 0,1 */
    ROM_LOAD( "ffspe_c3.rom", 0x200000, 0x200000, 0x6f7938d5 ) /* Plane 0,1 */
    ROM_LOAD( "ffspe_c5.rom", 0x400000, 0x200000, 0x49c5e0bf ) /* Plane 0,1 */

	ROM_REGION(0x600000)
    ROM_LOAD( "ffspe_c2.rom", 0x000000, 0x200000, 0x11e6bf96 ) /* Plane 2,3 */
    ROM_LOAD( "ffspe_c4.rom", 0x200000, 0x200000, 0x4ad066ff ) /* Plane 2,3 */
    ROM_LOAD( "ffspe_c6.rom", 0x400000, 0x200000, 0x8ff1f43d ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( sonicwi3_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "sonw3_p1.rom", 0x100000, 0x100000, 0x0547121d )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sonw3_s1.rom", 0x000000, 0x020000, 0x8dd66743 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0xc20000)    /* temporary space for graphics (disposed after conversion) */

	ROM_REGION(0x600000)
	ROM_LOAD( "sonw3_c1.rom", 0x200000, 0x200000, 0x3ca97864 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sonw3_c3.rom", 0x400000, 0x200000, 0xc339fff5 ) /* Plane 0,1 */

	ROM_REGION(0x600000)
	ROM_LOAD( "sonw3_c2.rom", 0x200000, 0x200000, 0x1da4b3a9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sonw3_c4.rom", 0x400000, 0x200000, 0x84a40c6e ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( worlher2_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "hero2_p1.rom", 0x000000, 0x100000, 0x65a891d9 )
	ROM_CONTINUE(                       0x100000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hero2_s1.rom", 0x000000, 0x020000, 0xfcaeb3a4 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0xc20000)    /* temporary space for graphics (disposed after conversion) */

	ROM_REGION(0x600000)
	ROM_LOAD( "hero2_c1.rom", 0x000000, 0x200000, 0x21c6bb91 ) /* Plane 0,1 */
	ROM_LOAD( "hero2_c3.rom", 0x200000, 0x200000, 0xb725a219 ) /* Plane 0,1 */
	ROM_LOAD( "hero2_c5.rom", 0x400000, 0x200000, 0xb20354af ) /* Plane 0,1 */

	ROM_REGION(0x600000)
	ROM_LOAD( "hero2_c2.rom", 0x000000, 0x200000, 0xa3999925 ) /* Plane 2,3 */
	ROM_LOAD( "hero2_c4.rom", 0x200000, 0x200000, 0x8d96425e ) /* Plane 2,3 */
	ROM_LOAD( "hero2_c6.rom", 0x400000, 0x200000, 0xb13d1de3 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( doubledr_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "ddrag_p1.rom", 0x100000, 0x100000, 0x34ab832a )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "ddrag_s1.rom", 0x000000, 0x020000, 0xbef995c5 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

    ROM_REGION(0x700000)
    ROM_LOAD( "ddrag_c1.rom", 0x000000, 0x200000, 0xb478c725 )
    ROM_LOAD( "ddrag_c3.rom", 0x200000, 0x200000, 0x8b0d378e )
    ROM_LOAD( "ddrag_c5.rom", 0x400000, 0x200000, 0xec87bff6 )
    ROM_LOAD( "ddrag_c7.rom", 0x600000, 0x100000, 0x727c4d02 )

    ROM_REGION(0x700000)
    ROM_LOAD( "ddrag_c2.rom", 0x000000, 0x200000, 0x2857da32 )
    ROM_LOAD( "ddrag_c4.rom", 0x200000, 0x200000, 0xc7d2f596 )
    ROM_LOAD( "ddrag_c6.rom", 0x400000, 0x200000, 0x844a8a11 )
    ROM_LOAD( "ddrag_c8.rom", 0x600000, 0x100000, 0x69a5fa37 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( goalx3_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "goal!_p1.rom", 0x100000, 0x100000, 0x2a019a79 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "goal!_s1.rom", 0x000000, 0x020000, 0xc0eaad86 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "goal!_c1.rom", 0x200000, 0x200000, 0xd061f1f5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "goal!_c3.rom", 0x400000, 0x400000, 0xcea2656b ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "goal!_c2.rom", 0x200000, 0x200000, 0x3f63c1a2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "goal!_c4.rom", 0x400000, 0x400000, 0x04782666 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( magdrop3_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "drop3_p1.rom", 0x000000, 0x100000, 0x931e17fa )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "drop3_s1.rom", 0x000000, 0x020000, 0x7399e68a )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "drop3_c1.rom", 0x200000, 0x200000, 0x734db3d6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "drop3_c3.rom", 0x600000, 0x200000, 0xec65f472 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	ROM_REGION(0x800000)
    ROM_LOAD( "drop3_c2.rom", 0x200000, 0x200000, 0xd78f50e5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "drop3_c4.rom", 0x600000, 0x200000, 0xf55dddf3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( samsho2_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "sams2_p1.rom", 0x100000, 0x100000, 0x22368892 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "sams2_s1.rom", 0x000000, 0x020000, 0x64a5cd66 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "sams2_c1.rom", 0x000000, 0x200000, 0x86cd307c ) /* Plane 0,1 */
    ROM_LOAD( "sams2_c3.rom", 0x200000, 0x200000, 0x7a63ccc7 ) /* Plane 0,1 */
    ROM_LOAD( "sams2_c5.rom", 0x400000, 0x200000, 0x20d3a475 ) /* Plane 0,1 */
    ROM_LOAD( "sams2_c7.rom", 0x600000, 0x200000, 0x2df3cbcf ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "sams2_c2.rom", 0x000000, 0x200000, 0xcdfcc4ca ) /* Plane 2,3 */
    ROM_LOAD( "sams2_c4.rom", 0x200000, 0x200000, 0x751025ce ) /* Plane 2,3 */
    ROM_LOAD( "sams2_c6.rom", 0x400000, 0x200000, 0xae4c0a88 ) /* Plane 2,3 */
    ROM_LOAD( "sams2_c8.rom", 0x600000, 0x200000, 0x1ffc6dfa ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( aof2_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "aof2_p1.rom", 0x100000, 0x100000, 0xd98debc1 )
	ROM_CONTINUE(                      0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "aof2_s1.rom", 0x000000, 0x020000, 0x8b02638e )
    ROM_LOAD( "ng-sfix.rom", 0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "aof2_c1.rom", 0x000000, 0x200000, 0x17b9cbd2 ) /* Plane 0,1 */
    ROM_LOAD( "aof2_c3.rom", 0x200000, 0x200000, 0xd2c88768 ) /* Plane 0,1 */
    ROM_LOAD( "aof2_c5.rom", 0x400000, 0x200000, 0xc3074137 ) /* Plane 0,1 */
    ROM_LOAD( "aof2_c7.rom", 0x600000, 0x200000, 0x3f36df57 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "aof2_c2.rom", 0x000000, 0x200000, 0x5fd76b67 ) /* Plane 2,3 */
    ROM_LOAD( "aof2_c4.rom", 0x200000, 0x200000, 0xdb39b883 ) /* Plane 2,3 */
    ROM_LOAD( "aof2_c6.rom", 0x400000, 0x200000, 0x31de68d3 ) /* Plane 2,3 */
    ROM_LOAD( "aof2_c8.rom", 0x600000, 0x200000, 0xe546d7a8 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( aodk_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "aodk_p1.rom", 0x100000, 0x100000, 0x62369553 )
	ROM_CONTINUE(                      0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "aodk_s1.rom", 0x000000, 0x020000, 0x96148d2b )
    ROM_LOAD( "ng-sfix.rom", 0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "aodk_c1.rom", 0x000000, 0x200000, 0xa0b39344 ) /* Plane 0,1 */
    ROM_LOAD( "aodk_c3.rom", 0x200000, 0x200000, 0x7fff4d41 ) /* Plane 0,1 */
    ROM_LOAD( "aodk_c5.rom", 0x400000, 0x200000, 0xc74c5e51 ) /* Plane 0,1 */
    ROM_LOAD( "aodk_c7.rom", 0x600000, 0x200000, 0xac7daa01 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "aodk_c2.rom", 0x000000, 0x200000, 0x203f6074 ) /* Plane 2,3 */
    ROM_LOAD( "aodk_c4.rom", 0x200000, 0x200000, 0x48db3e0a ) /* Plane 2,3 */
    ROM_LOAD( "aodk_c6.rom", 0x400000, 0x200000, 0x73e8e7e0 ) /* Plane 2,3 */
    ROM_LOAD( "aodk_c8.rom", 0x600000, 0x200000, 0x14e7ad71 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( kof94_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "kof94_p1.rom", 0x100000, 0x100000, 0xf10a2042 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "kof94_s1.rom", 0x000000, 0x020000, 0x825976c1 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "kof94_c1.rom", 0x000000, 0x200000, 0xb96ef460 ) /* Plane 0,1 */
    ROM_LOAD( "kof94_c3.rom", 0x200000, 0x200000, 0x54f66254 ) /* Plane 0,1 */
    ROM_LOAD( "kof94_c5.rom", 0x400000, 0x200000, 0xee759363 ) /* Plane 0,1 */
    ROM_LOAD( "kof94_c7.rom", 0x600000, 0x200000, 0x62f66888 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "kof94_c2.rom", 0x000000, 0x200000, 0x15e096a7 ) /* Plane 2,3 */
    ROM_LOAD( "kof94_c4.rom", 0x200000, 0x200000, 0x0b01765f ) /* Plane 2,3 */
    ROM_LOAD( "kof94_c6.rom", 0x400000, 0x200000, 0x498da52c ) /* Plane 2,3 */
    ROM_LOAD( "kof94_c8.rom", 0x600000, 0x200000, 0xfe0a235d ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( galaxyfg_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "galfi_p1.rom", 0x100000, 0x100000, 0x45906309 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "galfi_s1.rom", 0x000000, 0x020000, 0x72f8923e )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "galfi_c1.rom", 0x000000, 0x200000, 0xc890c7c0 ) /* Plane 0,1 */
    ROM_LOAD( "galfi_c3.rom", 0x200000, 0x200000, 0x9d87e761 ) /* Plane 0,1 */
    ROM_LOAD( "galfi_c5.rom", 0x400000, 0x200000, 0xe6b77e6a ) /* Plane 0,1 */
    ROM_LOAD( "galfi_c7.rom", 0x600000, 0x200000, 0x571379fe ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "galfi_c2.rom", 0x000000, 0x200000, 0xb6d25419 ) /* Plane 2,3 */
    ROM_LOAD( "galfi_c4.rom", 0x200000, 0x200000, 0x765d7cb8 ) /* Plane 2,3 */
    ROM_LOAD( "galfi_c6.rom", 0x400000, 0x200000, 0xd779a181 ) /* Plane 2,3 */
    ROM_LOAD( "galfi_c8.rom", 0x600000, 0x200000, 0x3dc1c12b ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( savagere_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "savag_p1.rom", 0x100000, 0x100000, 0x01d4e9c0 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "savag_s1.rom", 0x000000, 0x020000, 0xe08978ca )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "savag_c1.rom", 0x000000, 0x200000, 0x763ba611 ) /* Plane 0,1 */
    ROM_LOAD( "savag_c3.rom", 0x200000, 0x200000, 0x3e4eba4b ) /* Plane 0,1 */
    ROM_LOAD( "savag_c5.rom", 0x400000, 0x200000, 0x59013f9e ) /* Plane 0,1 */
    ROM_LOAD( "savag_c7.rom", 0x600000, 0x200000, 0xc88f7035 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "savag_c2.rom", 0x000000, 0x200000, 0xe05e8ca6 ) /* Plane 2,3 */
    ROM_LOAD( "savag_c4.rom", 0x200000, 0x200000, 0x3c2a3808 ) /* Plane 2,3 */
    ROM_LOAD( "savag_c6.rom", 0x400000, 0x200000, 0x1c8d5def ) /* Plane 2,3 */
    ROM_LOAD( "savag_c8.rom", 0x600000, 0x200000, 0x484ce3ba ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( mslug_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "mslug_p1.rom", 0x100000, 0x100000, 0x08d8daa5 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "mslug_s1.rom", 0x000000, 0x020000, 0x2f55958d )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "mslug_c1.rom", 0x200000, 0x200000, 0xd00bd152 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "mslug_c3.rom", 0x600000, 0x200000, 0xd3d5f9e5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	ROM_REGION(0x800000)
    ROM_LOAD( "mslug_c2.rom", 0x200000, 0x200000, 0xddff1dea ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "mslug_c4.rom", 0x600000, 0x200000, 0x5ac1d497 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( kabukikl_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "klash_p1.rom", 0x100000, 0x100000, 0x28ec9b77 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "klash_s1.rom", 0x000000, 0x020000, 0xa3d68ee2 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "klash_c1.rom", 0x200000, 0x200000, 0x4d896a58 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "klash_c3.rom", 0x600000, 0x200000, 0x58c454e7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	ROM_REGION(0x800000)
    ROM_LOAD( "klash_c2.rom", 0x200000, 0x200000, 0x3cf78a18 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "klash_c4.rom", 0x600000, 0x200000, 0xe1a8aa6a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( strhoops_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "shoop_p1.rom", 0x000000, 0x100000, 0x5e78328e )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "shoop_s1.rom", 0x000000, 0x020000, 0xa8205610 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "shoop_c1.rom", 0x000000, 0x400000, 0x0d995f21 ) /* Plane 0,1 */
    ROM_LOAD( "shoop_c3.rom", 0x400000, 0x400000, 0x8e260e07 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "shoop_c2.rom", 0x000000, 0x400000, 0x8f1e5958 ) /* Plane 2,3 */
    ROM_LOAD( "shoop_c4.rom", 0x400000, 0x400000, 0x07b8bd06 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( overtop_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "ovr_p1.rom", 0x100000, 0x100000, 0x16c063a9 )
	ROM_CONTINUE(                     0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "ovr_s1.rom",  0x000000, 0x020000, 0x481d3ddc )
    ROM_LOAD( "ng-sfix.rom", 0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0xa00000)
    ROM_LOAD( "ovr_c1.rom", 0x000000, 0x400000, 0x50f43087 ) /* Plane 0,1 */
    ROM_LOAD( "ovr_c3.rom", 0x400000, 0x400000, 0x9252ea02 ) /* Plane 0,1 */
    ROM_LOAD( "ovr_c5.rom", 0x800000, 0x200000, 0xfc858bef ) /* Plane 0,1 */

	ROM_REGION(0xa00000)
    ROM_LOAD( "ovr_c2.rom", 0x000000, 0x400000, 0xa5b39807 ) /* Plane 2,3 */
    ROM_LOAD( "ovr_c4.rom", 0x400000, 0x400000, 0x5f41a699 ) /* Plane 2,3 */
    ROM_LOAD( "ovr_c6.rom", 0x800000, 0x200000, 0x0589c15e ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( fatfury3_rom )
	ROM_REGION(0x300000)
    ROM_LOAD_WIDE_SWAP( "fury3_p1.rom", 0x000000, 0x100000, 0xc7b743eb )
    ROM_LOAD_WIDE_SWAP( "fury3_p2.rom", 0x100000, 0x200000, 0xdbe963ed )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "fury3_s1.rom", 0x000000, 0x020000, 0x0b33a800 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0xa00000)
    ROM_LOAD( "fury3_c1.rom", 0x200000, 0x200000, 0xc73e86e4 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "fury3_c3.rom", 0x600000, 0x200000, 0xf6738c87 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
    ROM_LOAD( "fury3_c5.rom", 0x800000, 0x200000, 0xb3ec6fa6 ) /* Plane 0,1 */

	ROM_REGION(0xa00000)
    ROM_LOAD( "fury3_c2.rom", 0x200000, 0x200000, 0xbfaf3258 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "fury3_c4.rom", 0x600000, 0x200000, 0x9c31e334 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
    ROM_LOAD( "fury3_c6.rom", 0x800000, 0x200000, 0x69210441 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( wakuwak7_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "waku7_p1.rom", 0x000000, 0x100000, 0xb14da766 )
	ROM_LOAD_WIDE_SWAP( "waku7_p2.rom", 0x100000, 0x200000, 0xfe190665 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "waku7_s1.rom", 0x000000, 0x20000, 0x71c4b4b5 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x0c00000)
	ROM_LOAD( "waku7_c1.rom", 0x200000, 0x200000, 0xd91d386f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "waku7_c3.rom", 0x600000, 0x200000, 0x02fcff2f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "waku7_c5.rom", 0xa00000, 0x200000, 0x03d32f25 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )

	ROM_REGION(0x0c00000)
	ROM_LOAD( "waku7_c2.rom", 0x200000, 0x200000, 0x36b5cf41 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "waku7_c4.rom", 0x600000, 0x200000, 0xcd7f1241 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "waku7_c6.rom", 0xa00000, 0x200000, 0xd996a90a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( samsho3_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "sams3_p1.rom", 0x000000, 0x100000, 0x282a336e )
	ROM_LOAD_WIDE_SWAP( "sams3_p2.rom", 0x100000, 0x200000, 0x9bbe27e0 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sams3_s1.rom", 0x000000, 0x20000, 0x74ec7d9f )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x0d00000)
	ROM_LOAD( "sams3_c1.rom", 0x200000, 0x200000, 0xe079f767 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sams3_c3.rom", 0x600000, 0x200000, 0xc61218d7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "sams3_c5.rom", 0xa00000, 0x200000, 0x05feee47 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "sams3_c7.rom", 0xc00000, 0x100000, 0xae450e3d ) /* Plane 0,1 */

	ROM_REGION(0x0d00000)
	ROM_LOAD( "sams3_c2.rom", 0x200000, 0x200000, 0xfc045909 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sams3_c4.rom", 0x600000, 0x200000, 0x054ec754 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "sams3_c6.rom", 0xa00000, 0x200000, 0xef7d9e29 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "sams3_c8.rom", 0xc00000, 0x100000, 0xa9e82717 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( kof95_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "kof95_p1.rom", 0x100000, 0x100000, 0x5e54cf95 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "kof95_s1.rom", 0x000000, 0x20000, 0xde716f8a )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x0d00000)
	ROM_LOAD( "kof95_c1.rom", 0x200000, 0x200000, 0x33bf8657 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "kof95_c3.rom", 0x600000, 0x200000, 0x0cee1ddb ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "kof95_c5.rom", 0xa00000, 0x200000, 0x843a1009 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "kof95_c7.rom", 0xc00000, 0x100000, 0x9904025f ) /* Plane 0,1 */

	ROM_REGION(0x0d00000)
	ROM_LOAD( "kof95_c2.rom", 0x200000, 0x200000, 0xf21908a4 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "kof95_c4.rom", 0x600000, 0x200000, 0x729db15d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "kof95_c6.rom", 0xa00000, 0x200000, 0xd3efd773 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "kof95_c8.rom", 0xc00000, 0x100000, 0x78eb0f9b ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( aof3_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "aof3_p1.rom", 0x000000, 0x100000, 0x9edb420d )
	ROM_LOAD_WIDE_SWAP( "aof3_p2.rom", 0x100000, 0x200000, 0x4d5a2602 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "aof3_s1.rom", 0x000000, 0x20000, 0xcc7fd344 )
    ROM_LOAD( "ng-sfix.rom", 0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x0e00000)
	ROM_LOAD( "aof3_c1.rom", 0x200000, 0x200000, 0xf6c74731 ) /* Plane 0,1 */
	ROM_CONTINUE(            0x000000, 0x200000 )
	ROM_LOAD( "aof3_c3.rom", 0x600000, 0x200000, 0x7749f5e6 ) /* Plane 0,1 */
	ROM_CONTINUE(            0x400000, 0x200000 )
	ROM_LOAD( "aof3_c5.rom", 0xa00000, 0x200000, 0x1718bdcd ) /* Plane 0,1 */
	ROM_CONTINUE(            0x800000, 0x200000 )
	ROM_LOAD( "aof3_c7.rom", 0xc00000, 0x200000, 0x51bd8ab2 ) /* Plane 0,1 */

	ROM_REGION(0x0e00000)
	ROM_LOAD( "aof3_c2.rom", 0x200000, 0x200000, 0xf587f149 ) /* Plane 2,3 */
	ROM_CONTINUE(            0x000000, 0x200000 )
	ROM_LOAD( "aof3_c4.rom", 0x600000, 0x200000, 0xcbd58369 ) /* Plane 2,3 */
	ROM_CONTINUE(            0x400000, 0x200000 )
	ROM_LOAD( "aof3_c6.rom", 0xa00000, 0x200000, 0x4fca967f ) /* Plane 2,3 */
	ROM_CONTINUE(            0x800000, 0x200000 )
	ROM_LOAD( "aof3_c8.rom", 0xc00000, 0x200000, 0x9a34f99c ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( mslug2_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "ms2_p1.rom", 0x000000, 0x100000, 0x2a53c5da )
	ROM_LOAD_WIDE_SWAP( "ms2_p2.rom", 0x100000, 0x200000, 0x38883f44 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ms2_s1.rom",  0x000000, 0x20000, 0xac6beb43 )
    ROM_LOAD( "ng-sfix.rom", 0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x1000000)
	ROM_LOAD( "ms2_c1.rom", 0x0000000, 0x800000, 0x394b5e0d ) /* Plane 0,1 */
	ROM_LOAD( "ms2_c3.rom", 0x0800000, 0x800000, 0x9f6bfa6f ) /* Plane 0,1 */

	ROM_REGION(0x1000000)
	ROM_LOAD( "ms2_c2.rom", 0x0000000, 0x800000, 0xe5806221 ) /* Plane 2,3 */
	ROM_LOAD( "ms2_c4.rom", 0x0800000, 0x800000, 0x7d3e306f ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( pulstar_rom )
	ROM_REGION(0x400000)
	ROM_LOAD_WIDE_SWAP( "pstar_p1.rom", 0x000000, 0x100000, 0x5e5847a2 )
	ROM_LOAD_WIDE_SWAP( "pstar_p2.rom", 0x100000, 0x200000, 0x028b774c )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pstar_s1.rom", 0x000000, 0x20000, 0xc79fc2c8 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x1000000)
	ROM_LOAD( "pstar_c1.rom", 0x200000, 0x200000, 0x63020fc6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "pstar_c3.rom", 0x600000, 0x200000, 0x21ef41d7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "pstar_c5.rom", 0xa00000, 0x200000, 0x6fe9259c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "pstar_c7.rom", 0xe00000, 0x200000, 0x3745b86f ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "pstar_c2.rom", 0x200000, 0x200000, 0x260e9d4d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "pstar_c4.rom", 0x600000, 0x200000, 0x3b9e288f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "pstar_c6.rom", 0xa00000, 0x200000, 0xdc32f2b4 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "pstar_c8.rom", 0xe00000, 0x200000, 0xf692d1a9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( blazstar_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "bstar_p1.rom", 0x000000, 0x100000, 0x183682f8 )
	ROM_LOAD_WIDE_SWAP( "bstar_p2.rom", 0x100000, 0x200000, 0x9a9f4154 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bstar_s1.rom", 0x000000, 0x20000, 0xd56cb498 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x1000000)
	ROM_LOAD( "bstar_c1.rom", 0x200000, 0x200000, 0x754744e0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "bstar_c3.rom", 0x600000, 0x200000, 0x7b39b590 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "bstar_c5.rom", 0xa00000, 0x200000, 0x9ceb113b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "bstar_c7.rom", 0xe00000, 0x200000, 0x50d28eca ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "bstar_c2.rom", 0x200000, 0x200000, 0xaf98c037 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "bstar_c4.rom", 0x600000, 0x200000, 0x6e731b30 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "bstar_c6.rom", 0xa00000, 0x200000, 0x6a78e810 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "bstar_c8.rom", 0xe00000, 0x200000, 0xcdbbb7d7 ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( ninjamas_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "ninjm_p1.rom", 0x000000, 0x100000, 0x3e97ed69 )
	ROM_LOAD_WIDE_SWAP( "ninjm_p2.rom", 0x100000, 0x200000, 0x191fca88 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ninjm_s1.rom", 0x000000, 0x20000, 0x8ff782f0 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x1000000)
	ROM_LOAD( "ninjm_c1.rom", 0x200000, 0x200000, 0x58f91ae0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "ninjm_c3.rom", 0x600000, 0x200000, 0x36c29ce3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "ninjm_c5.rom", 0xa00000, 0x200000, 0x4683ffc0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "ninjm_c7.rom", 0xe00000, 0x200000, 0x3e1885c0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "ninjm_c2.rom", 0x200000, 0x200000, 0x4258147f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "ninjm_c4.rom", 0x600000, 0x200000, 0x17e97a6e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "ninjm_c6.rom", 0xa00000, 0x200000, 0xde004f4a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "ninjm_c8.rom", 0xe00000, 0x200000, 0x5a5df034 ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( kof96m_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "kof96_p1.rom", 0x000000, 0x100000, 0x52755d74 )
	ROM_LOAD_WIDE_SWAP( "kof96_p2.rom", 0x100000, 0x200000, 0x002ccb73 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "kof96_s1.rom", 0x000000, 0x20000, 0x1254cbdb )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x1000000)
	ROM_LOAD( "kof96_c1.rom", 0x200000, 0x200000, 0x7ecf4aa2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "kof96_c3.rom", 0x600000, 0x200000, 0x64989a65 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "kof96_c5.rom", 0xa00000, 0x200000, 0x2a3bbd26 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "kof96_c7.rom", 0xe00000, 0x200000, 0x3687331b ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "kof96_c2.rom", 0x200000, 0x200000, 0x05b54f37 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "kof96_c4.rom", 0x600000, 0x200000, 0xafbea515 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "kof96_c6.rom", 0xa00000, 0x200000, 0x44d30dc7 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "kof96_c8.rom", 0xe00000, 0x200000, 0xfa1461ad ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( samsho4_rom )
	ROM_REGION(0x500000)
	ROM_LOAD_WIDE_SWAP( "sams4_p1.rom", 0x000000, 0x100000, 0x1a5cb56d )
	ROM_LOAD_WIDE_SWAP( "sams4_p2.rom", 0x100000, 0x400000, 0x7587f09b )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sams4_s1.rom", 0x000000, 0x20000, 0x8d3d3bf9 )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x1000000)
	ROM_LOAD( "sams4_c1.rom", 0x200000, 0x200000, 0x289100fa ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sams4_c3.rom", 0x600000, 0x200000, 0x6659734f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "sams4_c5.rom", 0xa00000, 0x200000, 0xe22254ed ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "sams4_c7.rom", 0xe00000, 0x200000, 0xe3e3b0cd ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "sams4_c2.rom", 0x200000, 0x200000, 0xc2716ea0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sams4_c4.rom", 0x600000, 0x200000, 0x91ebea00 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "sams4_c6.rom", 0xa00000, 0x200000, 0x00947b2e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "sams4_c8.rom", 0xe00000, 0x200000, 0xf33967f1 ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( shocktro_rom )
	ROM_REGION(0x500000)
	ROM_LOAD_WIDE_SWAP( "shock_p1.rom", 0x000000, 0x100000, 0x5677456f )
	ROM_LOAD_WIDE_SWAP( "shock_p2.rom", 0x100000, 0x400000, 0x646f6c76 )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "shock_s1.rom", 0x000000, 0x20000, 0x1f95cedb )
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	ROM_REGION(0x1000000)
	ROM_LOAD( "shock_c1.rom", 0x200000, 0x200000, 0xaad087fc ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "shock_c3.rom", 0x600000, 0x200000, 0x6682a458 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "shock_c5.rom", 0xa00000, 0x200000, 0xe17762b1 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "shock_c7.rom", 0xe00000, 0x200000, 0xa47e62d2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "shock_c2.rom", 0x200000, 0x200000, 0x7e39df1f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "shock_c4.rom", 0x600000, 0x200000, 0xcbef1f17 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "shock_c6.rom", 0xa00000, 0x200000, 0x28beab71 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "shock_c8.rom", 0xe00000, 0x200000, 0xe8e890fb ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

/* World cup - rom sets below not dumped correctly yet - don't include them.. */
ROM_START( wcup_rom )
	ROM_REGION(0x220000)
    ROM_LOAD_WIDE_SWAP( "wcupp.1",  0x000000, 0x100000, 0x0 )
	ROM_CONTINUE(0x100000,0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "wcups.1", 0x000000, 0x10000, 0x0 ) /* Correct size */
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x800000)
    ROM_LOAD( "wcupc.1", 0x100000, 0x300000, 0x0 )
    ROM_CONTINUE(0,0x100000)
    ROM_LOAD( "wcupc.3", 0x400000, 0x400000, 0x0 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "wcupc.2", 0x100000, 0x300000, 0x0 ) /* Plane 2,3 */
    ROM_CONTINUE(0,0x100000)
    ROM_LOAD( "wcupc.4", 0x400000, 0x400000, 0x0 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

ROM_START( turf_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "turfp.1",  0x100000, 0x100000, 0x0 )
	ROM_CONTINUE(0x000000,0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	ROM_REGION_DISPOSE(0x40000)    /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "turfs.1", 0x000000, 0x10000, 0x0 ) /* Correct size */
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x020000, 0x354029fc )

	ROM_REGION(0x400000)
    ROM_LOAD( "turfc.1", 0x000000, 0x400000, 0x0 )

	ROM_REGION(0x400000)
    ROM_LOAD( "turfc.2", 0x000000, 0x400000, 0x0 )

	ROM_REGION(0x20000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )
ROM_END

/******************************************************************************/

/* dummy entry for the dummy bios driver */
ROM_START( bios_rom )
	ROM_REGION(0x020000)
    ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )

	ROM_REGION(0x020000)
    ROM_LOAD( "ng-sfix.rom",  0x000000, 0x020000, 0x354029fc )
ROM_END

/******************************************************************************/

/* Not really a ROM decode routine. This just clears stuff for init_machine to check */
static void neogeo_decode(void)
{
	neogeo_rom_loaded = 0;
	neogeo_ram = neogeo_sram = NULL;
}

/******************************************************************************/

#define NGCRED "The Shin Emu Keikaku team\n"

/* Inspired by the CPS1 driver, compressed version of GameDrivers */
#define NEODRIVER(NEO_NAME,NEO_REALNAME,NEO_YEAR,NEO_MANU,NEO_CREDIT,NEO_MACHINE) \
struct GameDriver NEO_NAME##_driver  = \
{                               \
	__FILE__,                   \
	&neogeo_bios,               \
	#NEO_NAME,                  \
	NEO_REALNAME,               \
	NEO_YEAR,                   \
	NEO_MANU,                   \
	NGCRED NEO_CREDIT,          \
	0,                          \
	NEO_MACHINE,                \
	0,                          \
	NEO_NAME##_rom,             \
	neogeo_decode, 0,           \
	0,                          \
	0, 	    	                \
	neogeo_ports,               \
	0, 0, 0,                    \
	ORIENTATION_DEFAULT,        \
	0,0                         \
};

/* Use this macro when the romset name starts with a number */
#define NEODRIVERX(NEO_NAME,NEO_ROMNAME,NEO_REALNAME,NEO_YEAR,NEO_MANU,NEO_CREDIT,NEO_MACHINE) \
struct GameDriver NEO_NAME##_driver  = \
{                               \
	__FILE__,                   \
	&neogeo_bios,               \
	NEO_ROMNAME,                \
	NEO_REALNAME,               \
	NEO_YEAR,                   \
	NEO_MANU,                   \
	NGCRED NEO_CREDIT,          \
	0,                          \
	NEO_MACHINE,                \
	0,                          \
	NEO_NAME##_rom,             \
	neogeo_decode, 0,           \
	0,                          \
	0, 	    	                \
	neogeo_ports,               \
	0, 0, 0,                    \
	ORIENTATION_DEFAULT,        \
	0,0                         \
};

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the neo-geo.rom file, so that we do not have to include */
/* it in every zip file */
struct GameDriver neogeo_bios =
{
	__FILE__,
	0,
	"neogeo",
	"NeoGeo BIOS - NOT A REAL DRIVER",
	"19??",
	"SNK",
	"Do NOT link this from driver.c",
	0,
	&neogeo_mvs_machine_driver, /* Dummy */
	0,
	bios_rom,
	0, 0,
	0,
	0,      /* sound_prom */
	neogeo_ports,
	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0,0
};

/******************************************************************************/

/* MGD2 format dumps */
NEODRIVER(joyjoy,  "Joy Joy Kid/Puzzled","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(ridhero, "Riding Hero","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVERX(ttbb,   "2020bb","2020 Baseball","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(lresort, "Last Resort","1992","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(fbfrenzy,"Football Frenzy","1992","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(alpham2, "Alpha Mission 2","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(eightman,"Eight Man","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(ararmy,  "Robo Army","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(ncombat, "Ninja Combat","1990","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)
NEODRIVER(socbrawl,"Soccer Brawl","1992","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(bstars,  "Baseball Stars","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(fatfury1,"Fatal Fury - King of Fighters","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(mutnat,  "Mutation Nation","1992","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(burningf,"Burning Fight","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(crsword, "Crossed Swords","1991","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)
NEODRIVER(cyberlip,"Cyber Lip","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(kingofm, "King of the Monsters","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(gpilots, "Ghost Pilots","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(trally,  "Thrash Rally","1991","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)
NEODRIVER(lbowling,"League Bowling","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(ncommand,"Ninja Commando","1992","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)
NEODRIVER(sengoku, "Sengoku","1991","Alpha Denshi Co????","",&neogeo_mgd2_machine_driver)
NEODRIVERX(countb, "3countb","3 Count Bout","1993","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(androdun,"Andro Dunos","1992","Visco","",&neogeo_mgd2_machine_driver)
NEODRIVER(artfight,"Art of Fighting","1992","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(superspy,"Super Spy","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(wheroes, "World Heroes","1992","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)
NEODRIVER(sengoku2,"Sengoku 2","1993","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)

/* MVS format dumps */
NEODRIVER(puzzledp,"Puzzle de Pon","1995","Taito (Visco license)","",&neogeo_mvs_machine_driver)
NEODRIVER(bjourney,"Blues Journey","1990","Alpha Denshi Co","TJ Grant",&neogeo_mvs_machine_driver)
NEODRIVER(nam_1975,"NAM 1975","1990","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(maglord, "Magician Lord","1990","Alpha Denshi Co","",&neogeo_mvs_machine_driver)
NEODRIVER(cybrlipa,"Cyber Lip (alt)","1990","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(tpgolf,  "Top Player's Golf","1990","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(legendos,"Legend of Success Joe","1991","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(bstars2, "Baseball Stars 2","1992","SNK","Raphael Lukov\nTJ Grant",&neogeo_mvs_machine_driver)
NEODRIVER(wjammers,"Wind Jammers","1994","Data East","TJ Grant",&neogeo_mvs_machine_driver)
NEODRIVER(janshin, "Quest of Jongmaster","1994","Aicom","",&neogeo_mvs_machine_driver)
NEODRIVER(pbobble, "Puzzle Bobble","1994","Taito","",&neogeo_mvs_machine_driver)
NEODRIVER(sidkicks,"Super Sidekicks","1992","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(panicbom,"Panic Bomber","1994","Hudson Soft","Raphael Lukov\nTJ Grant",&neogeo_mvs_machine_driver)
NEODRIVER(pspikes2,"Power Spikes 2","1994","Video System Co","",&neogeo_mvs_machine_driver)
NEODRIVER(viewpoin,"Viewpoint","1992","Sammy","",&neogeo_mvs_machine_driver)
NEODRIVER(zedblade,"Zed Blade","1994","NMK","",&neogeo_mvs_machine_driver)
NEODRIVER(kotm2,   "King of the Monsters 2","1992","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(stakwin, "Stakes Winner","1995","Saurus","",&neogeo_mvs_machine_driver)
NEODRIVER(fatfury2,"Fatal Fury 2","1992","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(sonicwi2,"Aero Fighters 2","1994","Video System Co","Santeri Saarimaa\nAtila Mert",&neogeo_mvs_machine_driver)
NEODRIVER(ssideki2,"Super Sidekicks 2","1994","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(tophuntr,"Top Hunter","1994","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(spinmast,"Spin Masters","19??","???","",&neogeo_mvs_machine_driver) //
NEODRIVER(turfmast,"Neo Turf Masters","1996","Nazca","",&neogeo_mvs_machine_driver)
NEODRIVER(neodrift,"Neo Drift Out","1996","Video System Co","",&neogeo_mvs_machine_driver)
NEODRIVER(samsho,  "Samurai Shodown","1992","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(tws96,   "Tecmo World Soccer 96","1996","Tecmo","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(twinspri,"Twinklestar Sprites","1996","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(karnov_r,"Karnov's Revenge","1994","Data East","",&neogeo_mvs_machine_driver)
NEODRIVER(fatfursp,"Fatal Fury Special","1993","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(sonicwi3,"Sonic Wings 3","1995","Video System Co","",&neogeo_mvs_machine_driver)
NEODRIVER(mslug,   "Metal Slug","1996","Nazca","",&neogeo_mvs_machine_driver)
NEODRIVER(kabukikl,"Kabuki Klash","1995","Hudson Soft","Raphael Lukov\nTJ Grant",&neogeo_mvs_machine_driver)
NEODRIVER(strhoops,"Street Hoop","1994","Data East","",&neogeo_mvs_machine_driver)
NEODRIVER(overtop, "Over Top","1996","ADK","",&neogeo_mvs_machine_driver)
NEODRIVER(fatfury3,"Fatal Fury 3","1993","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(worlher2,"World Heroes 2","19??","??","",&neogeo_mvs_machine_driver) //
NEODRIVER(doubledr,"Double Dragon (NeoGeo)","1995","Technos","Santeri Saarimaa\nAtila Mert",&neogeo_mvs_machine_driver)
NEODRIVER(goalx3,  "Goal! Goal! Goal!","19??","??","",&neogeo_mvs_machine_driver)
NEODRIVER(magdrop3,"Magical Drop 3","1997","Data East","",&neogeo_mvs_machine_driver)
NEODRIVER(samsho2, "Samurai Shodown 2","1994","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(aof2,    "Art of Fighting 2","1994","SNK","Raphael Lukov\nTJ Grant",&neogeo_mvs_machine_driver)
NEODRIVER(aodk,    "Agressors of Dark Kombat","1994","SNK/ADK","",&neogeo_mvs_machine_driver)
NEODRIVER(kof94,   "King of Fighters '94","1994","SNK","Raphael Lukov\nTJ Grant",&neogeo_mvs_machine_driver)
NEODRIVER(galaxyfg,"Galaxy Fight","1995","Sunsoft","Raphael Lukov\nTJ Grant",&neogeo_mvs_machine_driver)
NEODRIVER(savagere,"Savage Reign","1995","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(neomrdo, "Neo Mr Do!","1996","Video System Co","",&neogeo_mvs_machine_driver)
NEODRIVER(neobombe,"Neo Bomberman","1997","Hudson Soft","",&neogeo_mvs_machine_driver)
NEODRIVER(mahretsu,"Mahjong Kyo Retsuden","1990","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(wakuwak7,"Waku Waku 7","1996","Sunsoft","",&neogeo_mvs_machine_driver)
NEODRIVER(samsho3, "Samurai Shodown 3","1995","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(kof95,   "King of Fighters '95","1995","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(aof3,    "Art of Fighting 3","1996","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(mslug2,  "Metal Slug 2","1998","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(pulstar, "Pulstar","1996","Aicom","",&neogeo_mvs_machine_driver)
NEODRIVER(blazstar,"Pulstar 2 - Blazing Star","1998","Yamekobo","",&neogeo_mvs_machine_driver)
NEODRIVER(ninjamas,"Ninja Masters","1996","SNK/ADK","",&neogeo_mvs_machine_driver)
NEODRIVER(kof96m,  "King of Fighters '96","19??","??","",&neogeo_mvs_machine_driver) //
NEODRIVER(samsho4, "Samurai Shodown 4","1996","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(shocktro,"Shock Troopers","19??","??","",&neogeo_mvs_machine_driver) //


/* Tests - delete later
NEODRIVER(wcup    ,"Neo Bomberman","1997","Hudson Soft","",&neogeo_mvs_machine_driver)
NEODRIVER(turf    ,"Neo Bomberman","1997","Hudson Soft","",&neogeo_mvs_machine_driver)
*/
