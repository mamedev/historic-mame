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

	TODO :		-Properly emulate the hard and soft dipswitches.
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
extern unsigned char *rambank;
extern unsigned char *rambank2;

/* from vidhrdw/neogeo.c */
void neogeo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  neogeo_vh_start(void);
void neogeo_vh_stop(void);
void neogeo_paletteram_w(int offset,int data);
int  neogeo_paletteram_r(int offset);
int  neogeo_interrupt(void);

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



void setpalbank0(int offset, int data);
void setpalbank1(int offset, int data);

/* from machine/neogeo.c */
int neogeo_rom_loaded;
void neogeo_init_machine(void);

/* debug defined in vidhrdw/neogeo.c */
extern int dotiles;
extern int screen_offs;
/* end debug */

int irq_acknowledge;
int vblankcount=0;

int neogeo_interrupt(void)
{
	/* debugging */

#if 0
	/* hard reset, connected to F5 */
	if (readinputport(6) & 0x10)
	{
		memset (rambank, 0, 0x10000);
		machine_reset();
	}
#endif

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

int timer_r (int offset) {

	/* This is just for testing purposes */
	/* not real correct code */
	int coinflip = read_4990_testbit();

/*	 if (errorlog) fprintf(errorlog,"CPU %04x - Read timer\n",cpu_getpc()); */

	return (  readinputport(4) ^ (coinflip << 6) ^ (coinflip << 7) );
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

int read_softdip (int offset)
{
	int result=READ_WORD(&rambank[0xfd82]);
  /*  if (errorlog) fprintf(errorlog,"CPU %04x - Read soft dip\n",cpu_getpc()); */

    /* Memory check */
    if (cpu_getpc()==0xc13154) return result;

	/* software dipswitches are mapped in RAM space */
	/* OR the settings in when not in memory test mode */
	if (readinputport(5) & 0x01)
		result |= 0x02;

	if (readinputport(5) & 0x02)
		result |= 0x8000;

	return(result);
}

/* The number of cartridges in the system */
/* Currently a big hack, this will keep the system test 'sane' */
int read_cart_info(int offset)
{
	/* if (errorlog) fprintf(errorlog,"CPU %04x - Read cart number\n",cpu_getpc()); */

	if (cpu_getpc()==0xc13154 || cpu_getpc()==0xc13178 || cpu_getpc()==0xc1319c) return READ_WORD(&rambank[0xfce6]);

	return 1<<8;
}

extern int rand_ret(int offset);

static struct MemoryReadAddress neogeo_readmem[] =
{
	{ 0x000000, 0x0FFFFF, MRA_ROM },	/* ROM bank 1 */
	{ 0x10FD82, 0x10FD83, read_softdip },
	{ 0x10Fce6, 0x10Fce7, read_cart_info },
	{ 0x100000, 0x10ffff, MRA_BANK1 },  /* RAM bank 1 */
	{ 0x200000, 0x2fffff, MRA_BANK4 }, 	/* ROM bank 2 */

	{ 0x300000, 0x300001, controller1_r },
	{ 0x300080, 0x300081, controller4_r }, /* Test switch in here */

	{ 0x320000, 0x320001, timer_r },
	{ 0x340000, 0x340001, controller2_r },
	{ 0x380000, 0x380001, controller3_r },
	{ 0x3C0000, 0x3C0001, vidram_offset_r },
	{ 0x3C0002, 0x3C0003, vidram_data_r },
	{ 0x3C0004, 0x3C0005, vidram_modulo_r },

/*	{ 0x3C0006, 0x3C0007, vblankc_r },	*/

	{ 0x3c000a, 0x3c000b, vidram_data_r },	/* Puzzle de Pon */

	{ 0x400000, 0x401fff, neogeo_paletteram_r },

   	{ 0x600000, 0x610bff, mish_vid_r },

/*	{ 0x800000, 0x800FFF, MRA_BANK5 },  */    /* memory card */
	{ 0xC00000, 0xC1FFFF, MRA_BANK3 },        /* system rom */
	{ 0xD00000, 0xD0FFFF, MRA_BANK2 },   /* RAM bank 2 */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress neogeo_writemem[] =
{
	{ 0x000000, 0x0FFFFF, MWA_ROM },    /* ghost pilots writes to ROM */
	{ 0x100000, 0x10FFFF, MWA_BANK1 },

	{ 0x300000, 0x300001, MWA_NOP },        /* Watchdog */


	{ 0x320000, 0x320001, MWA_NOP }, /* Sound */
/*	{ 0x380030, 0x380031, neo_unknown2 }, */

	{ 0x380000, 0x380001, MWA_NOP },

 	{ 0x380030, 0x380031, neo_unknown3 },  /* No idea */
	{ 0x380040, 0x380041, MWA_NOP },  /* Either 1 or both of output leds */
	{ 0x380050, 0x380051, neo_unknown4 },

/*	{ 0x3a000e, 0x3a000f,  }, */

/*	{ 0x3a0000, 0x3a0001, neo_unknown1 }, */
/*	{ 0x3a0010, 0x3a0011, neo_unknown2 }, */

	{ 0x3a000c, 0x3a000d, neo_unknown1 },
	{ 0x3a001c, 0x3a001d, neo_unknown2 },

	{ 0x3a000e, 0x3a000f, setpalbank1 },
	{ 0x3a001e, 0x3a001f, setpalbank0 },    /* Palette banking */

	{ 0x3C0000, 0x3C0001, vidram_offset_w },
	{ 0x3C0002, 0x3C0003, vidram_data_w },
	{ 0x3C0004, 0x3C0005, vidram_modulo_w },

	{ 0x3C0006, 0x3C0007, MWA_NOP },
/*	{ 0x3C0006, 0x3C0007, vblankc_w }, */  /* Used a lot by Metal Slug */

/*	{ 0x3C0008, 0x3C0009, vidram_offset_w },
	{ 0x3C000A, 0x3C000B, vidram_data_w },	*/

	{ 0x3C000C, 0x3C000D, write_irq },

	{ 0x400000, 0x401FFF, neogeo_paletteram_w },

	{ 0x600000, 0x610bFF, mish_vid_w }, /* debug */

 /*	{ 0x800000, 0x800FFF, MWA_BANK5 }, */        /* mem card */
 /*	{ 0xC00000, 0xC1FFFF, MWA_BANK3 }, */	     /* system ROM */
	{ 0xD00000, 0xD0FFFF, MWA_BANK2 }, /* RAM bank 2 */
	{ -1 }  /* end of table */
};

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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x38, "Off" )
	PORT_DIPNAME( 0x40, 0x00, "Freeplay", IP_KEY_NONE ) /* CHANGE LATER */
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Stop mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )

	PORT_START      /* IN4 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "COIN 1",OSD_KEY_5, IP_JOY_NONE, 0)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "COIN 2",OSD_KEY_6, IP_JOY_NONE, 0)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "SERVICE",OSD_KEY_7, IP_JOY_NONE, 0)

	/* Fake  IN 5 */
	PORT_START
	PORT_DIPNAME( 0x01, 0x01,"Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00,"Japanese" )
	PORT_DIPSETTING(    0x01,"English" )
	PORT_DIPNAME( 0x02, 0x02,"Machine mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00,"Home" )
	PORT_DIPSETTING(    0x02,"Arcade" )

	/* debug */
	PORT_START      /* IN6 */
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, 0, "strong reset",OSD_KEY_F5, IP_JOY_NONE, 0)
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, 0, "tiles ram view", OSD_KEY_8, IP_JOY_NONE, 0)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, 0, "tile ram - 0x80", OSD_KEY_9, IP_JOY_NONE, 0)
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, 0, "tile ram + 0x80", OSD_KEY_0, IP_JOY_NONE, 0)
	/* end debug */

    PORT_START      /* test switch??? */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )  /* all dummy values except bit 8 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 ) /* TEST SWITCH */

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

/* =-=-=-=-=-=-=-= Tiles layout and decode info per game =-=-=-=-=-=-= */
/* =-=-=-=-=-=-=-=           MGD2 format                 =-=-=-=-=-=-= */
/* 8192 sprites - Joy Joy Kid */
static struct GfxLayout neogeo_tilelayout_0x2000 =
{
	16,16,  /* 16*16 sprites */
	0x2000,   /* sprites */
	4,      /* 4 bits per pixel */
	{ 0x2000*32*8*3, 0x2000*32*8*2, 0x2000*32*8, 0 },     /* plane offset */
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x2000[] =
{
	{ 1, 0x00000, &charlayout, 0, 256 },
	{ 1, 0x20000, &neogeo_tilelayout_0x2000, 0, 256 },
	{ -1 } /* end of array */
};

/* 16384 sprites - Riding Hero, 2020 bb */
static struct GfxLayout neogeo_tilelayout_0x4000 =
{
	16,16,  /* 16*16 sprites */
	0x4000,  /* sprites */
	4,      /* 4 bits per pixel */
	{ 0x4000*32*8*3, 0x4000*32*8*2, 0x4000*32*8, 0 },     /* plane offset */
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x4000[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x4000, 0, 256 },
	{ -1 } /* end of array */
};

/* 24576 sprites - Last Resort, eightman, alpha mission 2, football frenzy */
static struct GfxLayout neogeo_tilelayout_0x6000 =
{
	16,16,  /* 16*16 sprites */
	0x6000,  /* 3*8192 sprites */
	4,      /* 4 bits per pixel */
	{ 0x6000*32*8*3, 0x6000*32*8*2, 0x6000*32*8, 0 },     /* plane offset */
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x6000[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x6000, 0, 256 },
	{ -1 } /* end of array */
};

/* 32768 sprites - Ghost Pilots, fatal fury 1, mutation nation, burning Fight, crossed sword,
   cyberlip, King of Monsters 1 */
static struct GfxLayout neogeo_tilelayout_0x8000 =
{
	16,16,  /* 16*16 sprites */
	0x8000,  /* sprites */
	4,      /* 4 bits per pixel */
	{ 0x8000*32*8*3, 0x8000*32*8*2, 0x8000*32*8*1, 0 },     /* plane offset */
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x8000[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x8000, 0, 256 },
	{ -1 } /* end of array */
};

/* 65536 sprites - 3 Count Bout, Super Spy, art of fighting, andro dunos.
   Note that some of the roms of this one are empty. */
static struct GfxLayout neogeo_tilelayout_0x10000 =
{
	16,16,  /* 16*16 sprites */
	0x10000,  /* sprites */
	4,      /* 4 bits per pixel */
	{ 0x10000*32*8*3, 0x10000*32*8*2, 0x10000*32*8, 0 },     /* plane offset */
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x10000[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x10000, 0, 256 },
	{ -1 } /* end of array */
};

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* ROM list dumps, different format 		      */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

/* 16384 - Puzzle de Pon */
static struct GfxLayout neogeo_tilelayout_0x4000_rl =
{
	16,16,   /* 16*16 sprites */
	0x4000,   /* 16384 sprites */
	4,       /* 4 bits per pixel */
	{ 0x4000*64*8 + 8, 0x4000*64*8, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },

	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x4000_rl[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x4000_rl, 0, 256 },
	{ -1 } /* end of array */
};

/* 24576 - NAM 1975, magician lord */
static struct GfxLayout neogeo_tilelayout_0x6000_rl =
{
	16,16,  /* 16*16 sprites */
	0x6000,   /* 3*8192 sprites */
	4,      /* 4 bits per pixel */
	{ 0x6000*64*8 + 8, 0x6000*64*8, 8, 0 },     /* plane offset */
	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x6000_rl[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x6000_rl, 0, 256 },
	{ -1 } /* end of array */
};

/* 32768 - Wind Jammers */
static struct GfxLayout neogeo_tilelayout_0x8000_rl =
{
	16,16,  /* 16*16 sprites */
	0x8000,   /* 32768 sprites */
	4,      /* 4 bits per pixel */
	{ 0x8000*64*8 + 8, 0x8000*64*8, 8, 0 },     /* plane offset */
	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x8000_rl[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x8000_rl, 0, 256 },
	{ -1 } /* end of array */
};

/* 40960 - Puzzle Bobble */
static struct GfxLayout neogeo_tilelayout_0xa000_rl =
{
	16,16,   /* 16*16 sprites */
	0xa000,   /* 49152-8192 sprites of which the first 32768 are not used */
	4,       /* 4 bits per pixel */
	{ 0xa000*64*8 + 8, 0xa000*64*8, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },

	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0xa000_rl[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0xa000_rl, 0, 256 },
	{ -1 } /* end of array */
};

/* 49152 - Super Sidekicks */
static struct GfxLayout neogeo_tilelayout_0xc000_rl =
{
	16,16,   /* 16*16 sprites */
	0xc000,   /* 49152 sprites of which 16384 are not used */
	4,       /* 4 bits per pixel */
	{ 0xc000*64*8 + 8, 0xc000*64*8, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },

	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0xc000_rl[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0xc000_rl, 0, 256 },
	{ -1 } /* end of array */
};

/* 65536 - King of the Monsters 2 */
static struct GfxLayout neogeo_tilelayout_0x10000_rl =
{
	16,16,   /* 16*16 sprites */
	0x10000,   /* 65536 sprites */
	4,       /* 4 bits per pixel */
	{ 0x10000*64*8 + 8, 0x10000*64*8, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },

	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x10000_rl[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x10000_rl, 0, 256 },
	{ -1 } /* end of array */
};

static struct GfxLayout neogeo_tilelayout_0x12000_rl =
{
	16,16,   /* 16*16 sprites */
	0x12000,   /* Sh*t, thats a lotta sprites */
	4,       /* 4 bits per pixel */
	{ 0x12000*64*8 + 8, 0x12000*64*8 + 0, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x12000_rl[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x12000_rl, 0, 256 },
	{ -1 } /* end of array */
};

static struct GfxLayout neogeo_tilelayout_0x14000_rl =
{
	16,16,   /* 16*16 sprites */
	0x14000,   /* Sh*t, thats a lotta sprites */
	4,       /* 4 bits per pixel */
	{ 0x14000*64*8 + 8, 0x14000*64*8 + 0, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x14000_rl[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x14000_rl, 0, 256 },
	{ -1 } /* end of array */
};

static struct GfxLayout neogeo_tilelayout_0x18000_rl =
{
	16,16,   /* 16*16 sprites */
	0x18000,   /* Sh*t, thats a lotta sprites */
	4,       /* 4 bits per pixel */
	{ 0x18000*64*8 + 8, 0x18000*64*8 + 0, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x18000_rl[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x18000_rl, 0, 256 },
	{ -1 } /* end of array */
};

static struct GfxLayout neogeo_tilelayout_0x20000_rl =
{
	16,16,   /* 16*16 sprites */
	0x20000,   /* Sh*t, thats a lotta sprites */
	4,       /* 4 bits per pixel */
	{ 0x20000*64*8 + 8, 0x20000*64*8 + 0, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_gfxdecodeinfo_0x20000_rl[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &neogeo_tilelayout_0x20000_rl, 0, 256 },
	{ -1 } /* end of array */
};

static struct GfxLayout mslug_tilelayout =
{
	16,16,   /* 16*16 sprites */
	20000,   /* Sh*t, thats a lotta sprites */
	4,       /* 4 bits per pixel */
	{ 0x400000*8 + 8, 0x400000*8 + 0, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },

	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo mslug_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &mslug_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/******************************************************************************/

#define NEOMACHINE(NEONAME,NEOGFX)                   \
static struct MachineDriver NEONAME =                \
{                                                    \
	{                                                \
		{                                            \
			CPU_M68000,                              \
			12000000,                                \
			0,                                       \
			neogeo_readmem,neogeo_writemem,0,0,      \
			neogeo_interrupt,1                       \
		},                                           \
	},                                               \
	60, DEFAULT_60HZ_VBLANK_DURATION,                \
	1,                                               \
	neogeo_init_machine,                             \
                                                     \
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },        \
	NEOGFX,                                          \
	4096,4096,                                       \
	0,                                               \
                                                     \
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,      \
	0,                                               \
	neogeo_vh_start,                                 \
	neogeo_vh_stop,                                  \
	neogeo_vh_screenrefresh,                         \
                                                     \
	0,0,0,0,                                         \
};

NEOMACHINE(neogeo_0x2000_machine_driver,     neogeo_gfxdecodeinfo_0x2000)
NEOMACHINE(neogeo_0x4000_machine_driver,     neogeo_gfxdecodeinfo_0x4000)
NEOMACHINE(neogeo_0x6000_machine_driver,     neogeo_gfxdecodeinfo_0x6000)
NEOMACHINE(neogeo_0x8000_machine_driver,     neogeo_gfxdecodeinfo_0x8000)
NEOMACHINE(neogeo_0x10000_machine_driver,    neogeo_gfxdecodeinfo_0x10000)
NEOMACHINE(neogeo_0x4000_rl_machine_driver,  neogeo_gfxdecodeinfo_0x4000_rl)
NEOMACHINE(neogeo_0x6000_rl_machine_driver,  neogeo_gfxdecodeinfo_0x6000_rl)
NEOMACHINE(neogeo_0x8000_rl_machine_driver,  neogeo_gfxdecodeinfo_0x8000_rl)
NEOMACHINE(neogeo_0xa000_rl_machine_driver,  neogeo_gfxdecodeinfo_0xa000_rl)
NEOMACHINE(neogeo_0xc000_rl_machine_driver,  neogeo_gfxdecodeinfo_0xc000_rl)
NEOMACHINE(neogeo_0x10000_rl_machine_driver, neogeo_gfxdecodeinfo_0x10000_rl)
NEOMACHINE(neogeo_0x12000_rl_machine_driver, neogeo_gfxdecodeinfo_0x12000_rl)
NEOMACHINE(neogeo_0x14000_rl_machine_driver, neogeo_gfxdecodeinfo_0x14000_rl)
NEOMACHINE(neogeo_0x18000_rl_machine_driver, neogeo_gfxdecodeinfo_0x18000_rl)
NEOMACHINE(neogeo_0x20000_rl_machine_driver, neogeo_gfxdecodeinfo_0x20000_rl)
NEOMACHINE(mslug_machine_driver,mslug_gfxdecodeinfo)

/* DEBUG PURPOSES ONLY, delete this later */
static struct GfxDecodeInfo dummy_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ -1 } /* end of array */
};

static struct MemoryReadAddress dummy_readmem[] =
{
	{ 0x000000, 0x0FFFFF, MRA_ROM },	/* Rom bank 1 */
	{ 0x10FD82, 0x10FD83, read_softdip },

	{ 0x10Fce6, 0x10Fce7, read_cart_info },

	{ 0x100000, 0x10ffff, MRA_RAM },
	{ 0x200000, 0x2fffff, MRA_BANK4 }, /* Rom bank 2 */

	{ 0x300000, 0x300001, controller1_r },
	{ 0x300080, 0x300081, controller4_r }, /* Test switch in here */

	{ 0x320000, 0x320001, timer_r },
	{ 0x340000, 0x340001, controller2_r },
	{ 0x380000, 0x380001, controller3_r },
	{ 0x3C0000, 0x3C0001, vidram_offset_r },
	{ 0x3C0002, 0x3C0003, vidram_data_r },
	{ 0x3C0004, 0x3C0005, vidram_modulo_r },

/*        { 0x3C0006, 0x3C0007, vblankc_r },	*/

	{ 0x3C000A, 0x3c000B, vidram_data_r },	/* Puzzle de Pon */

	{ 0x400000, 0x401FFF, neogeo_paletteram_r },

   	{ 0x600000, 0x610bFF, mish_vid_r },

/*  	{ 0x800000, 0x800FFF, MRA_BANK2 },  */      /* memory card */
	{ 0xC00000, 0xC1FFFF, MRA_ROM },            /* system rom */
	{ 0xD00000, 0xD0FFFF, MRA_BANK3 },
	{ -1 }  /* end of table */
};

static struct MachineDriver dummy_bios_machine_driver =
{
	{
		{
			CPU_M68000,
			12000000,
			0,
			dummy_readmem,neogeo_writemem,0,0,
			neogeo_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	neogeo_init_machine,

	40*8, 28*8, { 1*8, 39*8-1, 0*8, 28*8-1 },
	dummy_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	neogeo_vh_start,
	neogeo_vh_stop,
	neogeo_vh_screenrefresh,

	0,0,0,0,
};

/******************************************************************************/

/* MGD2 roms */
ROM_START( joyjoy_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n022001a.038", 0x000000, 0x040000, 0xaca68aee, 0xea512c9f )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x120000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n022001a.378", 0x000000, 0x20000, 0x64f64ddc , 0x6956d778 )
	ROM_LOAD( "n022001a.538", 0x020000, 0x80000, 0x91acf534 , 0xcb27be65 ) /* Plane 0,1 */
	ROM_LOAD( "n022001a.638", 0x0A0000, 0x80000, 0xd2f574c5 , 0x7e134979 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( ridhero_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x4d335f81 , 0xdabfac95 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x220000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0x0f7080de , 0xeb5189f0 )

	ROM_LOAD( "n046001a.538", 0x020000, 0x40000, 0x89eb3269 , 0x24096241 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0A0000, 0x40000 )
	ROM_LOAD( "n046001a.53c", 0x060000, 0x40000, 0xc70d6fb7 , 0x7026a3a2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )

	ROM_LOAD( "n046001a.638", 0x120000, 0x40000, 0xdfddc5cd , 0xdf6a5b00 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n046001a.63c", 0x160000, 0x40000, 0x28ce9334 , 0x15220d51 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( ttbb_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x829ac1f8 , 0xefb016a2 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x220000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0x23b37c91 , 0x7015b8fc )

	ROM_LOAD( "n046001a.538", 0x020000, 0x40000, 0xcb9e6734 , 0x746bf48a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0a0000, 0x40000 )
	ROM_LOAD( "n046001a.53c", 0x060000, 0x40000, 0x33543bd8 , 0x57bdcec0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )

	ROM_LOAD( "n046001a.638", 0x120000, 0x40000, 0x263ecb62 , 0x5c123d9c ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1a0000, 0x40000 )
	ROM_LOAD( "n046001a.63c", 0x160000, 0x40000, 0xd58909dd , 0x2f4bb615 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( lresort_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x954d9aeb , 0x5f0a5a4b )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0x871a3e12 , 0x5cef5cc6 )

	ROM_LOAD( "n046001a.538", 0x020000, 0x40000, 0x84229a28 , 0x9f7995a9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "n046001a.53c", 0x060000, 0x40000, 0x349873c4 , 0xe122b155 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n046001b.538", 0x0A0000, 0x40000, 0x77952209 , 0xe7138cb9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "n046001a.638", 0x1A0000, 0x40000, 0x43b89106 , 0x68c70bac ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n046001a.63c", 0x1E0000, 0x40000, 0x60018cf3 , 0xf18a9b02 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n046001b.638", 0x220000, 0x40000, 0x6865777d , 0x08178e27 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( ararmy_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x9ef7ffe9 , 0x99c7b4fc )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0x6b1d1c51 , 0xac0daa1b )

	ROM_LOAD( "n046001a.538", 0x020000, 0x40000, 0xdf5cb028 , 0xe3afaf17 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "n046001a.53c", 0x060000, 0x40000, 0xb645046f , 0x17098f54 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n046001b.538", 0x0A0000, 0x40000, 0x3f253d7f , 0x13cbb7c5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "n046001a.638", 0x1A0000, 0x40000, 0xd2542bca , 0x3a098b3b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n046001a.63c", 0x1E0000, 0x40000, 0xdeceac9e , 0x8e3b2b88 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n046001b.638", 0x220000, 0x40000, 0xd932018a , 0xb6f5fc62 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( fbfrenzy_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x8bfe2f3c , 0xc9fc879c )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0xc6d79eed , 0x8472ed44 )

	ROM_LOAD( "n046001a.538", 0x020000, 0x40000, 0x936791f9 , 0xcd377680 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "n046001a.53c", 0x060000, 0x40000, 0x9df5a633 , 0x2f6d09c2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n046001b.538", 0x0A0000, 0x40000, 0xadf1b389 , 0x9abe41c8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "n046001a.638", 0x1A0000, 0x40000, 0x49f0e056 , 0x8b76358f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n046001a.63c", 0x1E0000, 0x40000, 0xf054eaba , 0x77e45dd2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n046001b.638", 0x220000, 0x40000, 0x92fad81a , 0x336540a8 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( alpham2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n082001a.038", 0x000000, 0x040000, 0x9a61c8c9 , 0x4400b34c )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n082001a.03c", 0x080000, 0x040000, 0xfac80000 , 0xb0366875 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n082001a.378", 0x000000, 0x20000, 0x1e767974 , 0x85ec9acf )

	ROM_LOAD( "n082001a.538", 0x020000, 0x40000, 0x92ad2b4d , 0xc516b09e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "n082001a.53c", 0x060000, 0x40000, 0x00be08f6 , 0xd9a0ff6c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n082001b.538", 0x0A0000, 0x40000, 0xd238188e , 0x3a7fe4fd ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "n082001a.638", 0x1A0000, 0x40000, 0x7b84a706 , 0x6b674581 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n082001a.63c", 0x1E0000, 0x40000, 0x5099927f , 0x4ff21008 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n082001b.638", 0x220000, 0x40000, 0xfcd769fb , 0xd0e8eef3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( trally_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0xe80fdfc1 , 0x400bed38 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n046001a.03c", 0x080000, 0x040000, 0x132fa6c9 , 0x77196e9a )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0xe9ccba0a , 0xfff62ae3 )

	ROM_LOAD( "n046001a.538", 0x020000, 0x40000, 0xef22d0be , 0x4d002ecb ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n046001a.53c", 0x060000, 0x40000, 0xb17fe101 , 0xb0be56db ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n046001b.538", 0x0A0000, 0x40000, 0x5c6ba5bf , 0x2f213750 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1a0000, 0x40000 )

  	ROM_LOAD( "n046001a.638", 0x220000, 0x40000, 0x9604c64a , 0x6b2f79de ) /* Plane 2,3 */
  	ROM_CONTINUE(             0x320000, 0x40000 )
  	ROM_LOAD( "n046001a.63c", 0x260000, 0x40000, 0x7c49f737 , 0x091f38b4 ) /* Plane 2,3 */
  	ROM_CONTINUE(             0x260000, 0x40000 )
  	ROM_LOAD( "n046001b.638", 0x2a0000, 0x40000, 0xcb0f9e4d , 0x268be38b ) /* Plane 2,3 */
  	ROM_CONTINUE(             0x3a0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( eightman_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x3b227218 , 0xe23e2631 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0x09625510 , 0xa402202b )

	ROM_LOAD( "n046001a.538", 0x020000, 0x40000, 0x6ecb0b13 , 0xc916c9bf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "n046001a.53c", 0x060000, 0x40000, 0xaa767fda , 0x4b057b13 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n046001b.538", 0x0A0000, 0x40000, 0xeb3faa85 , 0x12d53af0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "n046001a.638", 0x1A0000, 0x40000, 0xa90186b9 , 0x7114bce3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n046001a.63c", 0x1E0000, 0x40000, 0x2a225c0e , 0x51da9a34 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n046001b.638", 0x220000, 0x40000, 0x547ea2be , 0x43cf58f9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( ncombat_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x7a00f452 , 0x89660a31 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0xd70c3894 , 0xd49afee8 )

	ROM_LOAD( "n046001a.538", 0x020000, 0x40000, 0x6fbef8c8 , 0x0147a4b5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "n046001a.53c", 0x060000, 0x40000, 0x79611a8d , 0x4df6123a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n046001b.538", 0x0A0000, 0x40000, 0xed83acaf , 0x19441c78 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "n046001a.638", 0x1A0000, 0x40000, 0x1537c62d , 0xe3d367f8 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n046001a.63c", 0x1E0000, 0x40000, 0x42e8e64e , 0x1c1f6101 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n046001b.638", 0x220000, 0x40000, 0x05886710 , 0xf417f9ac ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( socbrawl_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0xa1f12045 , 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0xa82f2e69 , 0x0 )

	ROM_LOAD( "n046001a.538", 0x020000, 0x40000, 0x4c75e0c9 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "n046001a.53c", 0x060000, 0x40000, 0x31faf80c , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n046001b.538", 0x0A0000, 0x40000, 0x9c514437 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "n046001a.638", 0x1A0000, 0x40000, 0xc54eaaca , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n046001a.63c", 0x1E0000, 0x40000, 0x2a4bfed9 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n046001b.638", 0x220000, 0x40000, 0xeeeb7897 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( bstars_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n042001a.038", 0x000000, 0x040000, 0xc10f28df , 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n138001a.378", 0x000000, 0x20000, 0xb87843ba , 0x0 )

	ROM_LOAD( "n138001a.538", 0x020000, 0x40000, 0x2ddb7e3b , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "n138001a.53c", 0x060000, 0x40000, 0xa2bfd43f , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n138001b.538", 0x0A0000, 0x40000, 0xc1b46e16 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "n138001a.638", 0x1A0000, 0x40000, 0xc3028d66 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n138001a.63c", 0x1E0000, 0x40000, 0x16ab04db , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n138001b.638", 0x220000, 0x40000, 0x5ab02a68 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( gpilots_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0x5ca22fe6 , 0xfc5837c0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0x6c640000 , 0x47a641da )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n058001a.378", 0x000000, 0x20000, 0x6118ea2a , 0xa6d83d53 )

	ROM_LOAD( "n058001a.538", 0x020000, 0x40000, 0x022dd51f , 0x92b8ee5f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n058001a.53c", 0x060000, 0x40000, 0x2fce4578 , 0x8c8e42e9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n058001b.538", 0x0A0000, 0x40000, 0xcf579505 , 0x4f12268b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n058001b.53c", 0x0E0000, 0x40000, 0x39227a48 , 0x7c3c9c7e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "n058001a.638", 0x220000, 0x40000, 0xe6fe9e6e , 0x05733639 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n058001a.63c", 0x260000, 0x40000, 0xead70b71 , 0x347fef2b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n058001b.638", 0x2A0000, 0x40000, 0xc9d8ad90 , 0x2c586176 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n058001b.63c", 0x2E0000, 0x40000, 0x2971acff , 0x9b2eee8b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( crsword_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x6e5ca756 , 0x42c78fe1 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n046001a.378", 0x000000, 0x20000, 0xc6de9456 , 0x74651f27 )

	ROM_LOAD( "n046001a.538", 0x020000, 0x40000, 0xab929c8e , 0x4b373de7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n046001a.53c", 0x060000, 0x40000, 0x6140cdc2 , 0xcddf6d69 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n046001b.538", 0x0A0000, 0x40000, 0x4d93fc9b , 0x61d25cb3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n046001b.53c", 0x0E0000, 0x40000, 0x6856e324 , 0x00bc3d84 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "n046001a.638", 0x220000, 0x40000, 0x24104162 , 0xe05f5f33 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n046001a.63c", 0x260000, 0x40000, 0xa1976df7 , 0x91ab11a4 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n046001b.638", 0x2A0000, 0x40000, 0x12f85b88 , 0x01357559 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n046001b.63c", 0x2E0000, 0x40000, 0x432aaf20 , 0x28c6d19a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( cyberlip_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n062001a.038", 0x000000, 0x040000, 0x04b4b1ce , 0x4012e465 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n062001a.03c", 0x080000, 0x040000, 0x0c000000 , 0xee899b19 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n062001a.378", 0x000000, 0x20000, 0x6ee68484 , 0x79a35264 )

	ROM_LOAD( "n062001a.538", 0x020000, 0x40000, 0x13922c1a , 0x0239fc08 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n062001a.53c", 0x060000, 0x40000, 0x6df289ca , 0x23f03b97 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n062001b.538", 0x0A0000, 0x40000, 0x5dedccdd , 0x065061ed ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n062001b.53c", 0x0E0000, 0x40000, 0xc4000000 , 0xe485e72d ) /* Plane 0,1 - unnecessary? */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "n062001a.638", 0x220000, 0x40000, 0x4298d47c , 0xe102a1bd ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n062001a.63c", 0x260000, 0x40000, 0x0da06d02 , 0x520571d2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n062001b.638", 0x2A0000, 0x40000, 0x1aaea774 , 0x2e31e2d2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n062001b.63c", 0x2E0000, 0x40000, 0x40000000 , 0x9e2af4db ) /* Plane 2,3 - unnecessary? */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( kingofm_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0xf753af95 , 0xd239c184 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0xfe700000 , 0x7291a388 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n058001a.378", 0x000000, 0x20000, 0x6f50f8f6 , 0x1a2eeeb3 )

	ROM_LOAD( "n058001a.538", 0x020000, 0x40000, 0x8c31a5c5 , 0x493db90e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n058001a.53c", 0x060000, 0x40000, 0x17ea64c6 , 0x0d211945 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n058001b.538", 0x0A0000, 0x40000, 0x0f5b896b , 0xcabb7b58 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n058001b.53c", 0x0E0000, 0x40000, 0x108013b8 , 0xc7c20718 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "n058001a.638", 0x220000, 0x40000, 0x7a8349d3 , 0x8bc1c3a0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n058001a.63c", 0x260000, 0x40000, 0xcedf3485 , 0xcc793bbf ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n058001b.638", 0x2A0000, 0x40000, 0x752f97f9 , 0xfde45b59 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n058001b.63c", 0x2E0000, 0x40000, 0xb7adb0b9 , 0xb89b4201 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( burningf_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n054001a.038", 0x000000, 0x040000, 0xb0e1f177 , 0x188c5e11 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n054001a.378", 0x000000, 0x20000, 0x58bb9619 , 0x6799ea0d )

	ROM_LOAD( "n054001a.538", 0x020000, 0x40000, 0xc6004c86 , 0x4ddc137b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n054001a.53c", 0x060000, 0x40000, 0x3989eb5f , 0x896d8545 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n054001b.538", 0x0A0000, 0x40000, 0x42134faf , 0x2b2cb196 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n054001b.53c", 0x0E0000, 0x40000, 0x39f78509 , 0x0f49caa9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "n054001a.638", 0x220000, 0x40000, 0x553288e0 , 0x7d7d87dc ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n054001a.63c", 0x260000, 0x40000, 0x3566a06e , 0x39fe5307 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n054001b.638", 0x2A0000, 0x40000, 0x8ac2d988 , 0x03aa8a36 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n054001b.63c", 0x2E0000, 0x40000, 0x473c8f34 , 0xf759626e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( mutnat_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n054001a.038", 0x000000, 0x040000, 0x925138ad , 0x30cbd46b )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n054001a.378", 0x000000, 0x20000, 0xecf8b234 , 0x99419733 )

	ROM_LOAD( "n054001a.538", 0x020000, 0x40000, 0x3fe926f3 , 0x83d59ccf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n054001a.53c", 0x060000, 0x40000, 0x437ccb1a , 0xb2f1409d ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n054001b.538", 0x0A0000, 0x40000, 0x35e14ec5 , 0xeaa2801a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n054001b.53c", 0x0E0000, 0x40000, 0x48563fa0 , 0xc718b731 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "n054001a.638", 0x220000, 0x40000, 0xb831cfcb , 0x9e115a04 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n054001a.63c", 0x260000, 0x40000, 0x0ac511df , 0x1bb648c1 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n054001b.638", 0x2A0000, 0x40000, 0x036edf00 , 0x32bf4a2d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n054001b.63c", 0x2E0000, 0x40000, 0xeae9166d , 0x7d120067 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( lbowling_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n050001a.038", 0x000000, 0x040000, 0x1586a050 , 0x380e358d )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n050001a.378", 0x000000, 0x20000, 0x74395cd9 , 0x5fcdc0ed )

	ROM_LOAD( "n050001a.538", 0x020000, 0x40000, 0xf66b1bf3 , 0x17df7955 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n050001a.53c", 0x060000, 0x40000, 0xf0000000 , 0x67bf2d89 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n050001b.538", 0x0A0000, 0x40000, 0xf2000000 , 0x00d36f90 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n050001b.53c", 0x0E0000, 0x40000, 0xa6000000 , 0x4e971be9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "n050001a.638", 0x220000, 0x40000, 0xbf85c625 , 0x84fd2c90 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n050001a.63c", 0x260000, 0x40000, 0x26000000 , 0xcb4fbeb0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n050001b.638", 0x2A0000, 0x40000, 0x94000000 , 0xc2ddf431 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n050001b.63c", 0x2E0000, 0x40000, 0xc4000000 , 0xe67f8c81 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( fatfury1_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0x5a09be4b , 0x47e51379 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0x34d80000 , 0x19d36805 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n058001a.378", 0x000000, 0x20000, 0x3e30d76e , 0x3c3bdf8c )

	ROM_LOAD( "n058001a.538", 0x020000, 0x40000, 0x2382e626 , 0x9aaa6d73 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n058001a.53c", 0x060000, 0x40000, 0x043fc03b , 0xa986f4a9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n058001b.538", 0x0A0000, 0x40000, 0x528be7c9 , 0x7aefe57d ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n058001b.53c", 0x0E0000, 0x40000, 0x4b77c26d , 0xe3057c96 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "n058001a.638", 0x220000, 0x40000, 0x33251363 , 0x9cae3703 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n058001a.63c", 0x260000, 0x40000, 0xad5f0307 , 0x308b619f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n058001b.638", 0x2A0000, 0x40000, 0xe94d2355 , 0xb39a0cde ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n058001b.63c", 0x2E0000, 0x40000, 0x09119b45 , 0x737bc030 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( ncommand_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n054001a.038", 0x000000, 0x040000, 0xb30bdd01 , 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n054001a.03c", 0x080000, 0x040000, 0x31216705 , 0x0 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n054001a.378", 0x000000, 0x20000, 0x282dd3c7 , 0x0 )

	ROM_LOAD( "n054001a.538", 0x020000, 0x40000, 0x95786326 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n054001a.53c", 0x060000, 0x40000, 0x1e4d26b3 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n054001b.538", 0x0A0000, 0x40000, 0xa27b99d1 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n054001b.53c", 0x0E0000, 0x40000, 0x3a951ff7 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "n054001a.638", 0x220000, 0x40000, 0x8297fd5d , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n054001a.63c", 0x260000, 0x40000, 0x57aa8e62 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n054001b.638", 0x2A0000, 0x40000, 0x2b761d24 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n054001b.63c", 0x2E0000, 0x40000, 0x06ea4fe0 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( sengoku_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0x12d6b4fa , 0x4483bae1 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0x70180000 , 0xd0d55b2a )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n058001a.378", 0x000000, 0x20000, 0x9079c2bb , 0xb246204d )

	ROM_LOAD( "n058001a.538", 0x020000, 0x40000, 0x4171b5f1 , 0xe834b925 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "n058001a.53c", 0x060000, 0x40000, 0x383e11d0 , 0x66be6d46 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "n058001b.538", 0x0A0000, 0x40000, 0xb4340b6c , 0x443c552c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "n058001b.53c", 0x0E0000, 0x40000, 0xb4747c20 , 0xecb41adc ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "n058001a.638", 0x220000, 0x40000, 0x34194ca7 , 0x96de5eb9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n058001a.63c", 0x260000, 0x40000, 0xa4770e4f , 0x25f5fd7b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n058001b.638", 0x2A0000, 0x40000, 0xf7b9983f , 0xafbd5b0b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n058001b.63c", 0x2E0000, 0x40000, 0x53cb942f , 0x78b25278 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( countb_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n106001a.038", 0x000000, 0x040000, 0x30b2d082 , 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n106001a.03c", 0x080000, 0x040000, 0xfcba1382 , 0x0 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n106001a.378", 0x000000, 0x20000, 0xae7f1d57 , 0x0 )

	ROM_LOAD( "n106001a.538", 0x020000, 0x40000, 0x200ff619 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "n106001a.53c", 0x060000, 0x40000, 0xb2343298 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n106001b.538", 0x0A0000, 0x40000, 0xba08b4b6 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n106001b.53c", 0x0E0000, 0x40000, 0x71015131 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_LOAD( "n106001c.538", 0x120000, 0x40000, 0x36446a7a , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n106001c.53c", 0x160000, 0x40000, 0xaaac75fc , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n106001d.538", 0x1A0000, 0x40000, 0x703b9d89 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n106001d.53c", 0x1E0000, 0x40000, 0xab7a4c04 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "n106001a.638", 0x420000, 0x40000, 0xba6d9ee5 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "n106001a.63c", 0x460000, 0x40000, 0xc52e5608 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "n106001b.638", 0x4A0000, 0x40000, 0x5b742260 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "n106001b.63c", 0x4E0000, 0x40000, 0x0243d7d3 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	ROM_LOAD( "n106001c.638", 0x520000, 0x40000, 0x5578634a , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "n106001c.63c", 0x560000, 0x40000, 0xdadf5ebf , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "n106001d.638", 0x5A0000, 0x40000, 0xab6ec08e , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "n106001d.63c", 0x5E0000, 0x40000, 0x85d33953 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( androdun_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0x2182d3fc , 0x4639b419 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0xddabe807 , 0x11beb098 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n138001a.378", 0x000000, 0x20000, 0x52549d32 , 0x6349de5d )

	ROM_LOAD( "n138001a.538", 0x020000, 0x40000, 0xed5f5fdb , 0xca08e432 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "n138001a.53c", 0x060000, 0x40000, 0xed710af3 , 0xfcbcb305 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n138001b.538", 0x0A0000, 0x40000, 0x52000000 , 0x806ab937 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n138001b.53c", 0x0E0000, 0x40000, 0x16000000 , 0xe7e1a2be ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can be tossed */
	ROM_LOAD( "n138001c.538", 0x120000, 0x40000, 0xed5f5fdb , 0xca08e432 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n138001c.53c", 0x160000, 0x40000, 0xed710af3 , 0xfcbcb305 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n138001d.538", 0x1A0000, 0x40000, 0x52000000 , 0x806ab937 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n138001d.53c", 0x1E0000, 0x40000, 0x16000000 , 0xe7e1a2be ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "n138001a.638", 0x420000, 0x40000, 0x1ab212f6 , 0x7a0deb9e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "n138001a.63c", 0x460000, 0x40000, 0x6965fed7 , 0xb1c640f5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "n138001b.638", 0x4A0000, 0x40000, 0x8a000000 , 0x33bee10f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "n138001b.63c", 0x4E0000, 0x40000, 0xcc000000 , 0x70f0d263 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can be tossed */
	ROM_LOAD( "n138001c.638", 0x520000, 0x40000, 0x1ab212f6 , 0x7a0deb9e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "n138001c.63c", 0x560000, 0x40000, 0x6965fed7 , 0xb1c640f5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "n138001d.638", 0x5A0000, 0x40000, 0x8a000000 , 0x33bee10f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "n138001d.63c", 0x5E0000, 0x40000, 0xcc000000 , 0x70f0d263 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( artfight_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n102001a.038", 0x000000, 0x040000, 0x12f67fb8 , 0x95102254 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n102001a.378", 0x000000, 0x20000, 0xbd276af3 , 0x89903f39 )

	ROM_LOAD( "n102001a.538", 0x020000, 0x40000, 0x310ea698 , 0xa2e4a168 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "n102001a.53c", 0x060000, 0x40000, 0xba31af63 , 0xda389ef7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n102001b.538", 0x0A0000, 0x40000, 0xb605ed55 , 0x2a0c385b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n102001b.53c", 0x0E0000, 0x40000, 0x84f34535 , 0x4a4317bf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_LOAD( "n102001c.538", 0x120000, 0x40000, 0x36b70017 , 0x471d9e57 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n102001c.53c", 0x160000, 0x40000, 0xda6b2055 , 0x23fe5675 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n102001d.538", 0x1A0000, 0x40000, 0x5c626bf8 , 0x204e7b29 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n102001d.53c", 0x1E0000, 0x40000, 0x9749ce05 , 0x7f6d5144 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "n102001a.638", 0x420000, 0x40000, 0x00993dbd , 0xca12c80f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "n102001a.63c", 0x460000, 0x40000, 0x202041b8 , 0xd59746b0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "n102001b.638", 0x4A0000, 0x40000, 0xf0234291 , 0x8b73b3da ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "n102001b.63c", 0x4E0000, 0x40000, 0xec02023c , 0x9fc3f8ea ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	ROM_LOAD( "n102001c.638", 0x520000, 0x40000, 0xbe4c3758 , 0xcbf8a72e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "n102001c.63c", 0x560000, 0x40000, 0xbdcf16bf , 0x5ec93c96 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "n102001d.638", 0x5A0000, 0x40000, 0x8242d040 , 0x47763b6d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "n102001d.63c", 0x5E0000, 0x40000, 0xbe7d0093 , 0x4408f4eb ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( superspy_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0x7e59a7cb , 0x2e949e32 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0x834c0000 , 0x54443d72 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n138001a.378", 0x000000, 0x20000, 0x6991ff05 , 0xec5fdb96 )

	ROM_LOAD( "n138001a.538", 0x020000, 0x40000, 0xeb4edd10 , 0x239f22c4 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "n138001a.53c", 0x060000, 0x40000, 0x43517a29 , 0xce80c326 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n138001b.538", 0x0A0000, 0x40000, 0x063774db , 0x1edcf268 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n138001b.53c", 0x0E0000, 0x40000, 0xf0f83c30 , 0xa41602a0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can probably be tossed */
	ROM_LOAD( "n138001c.538", 0x120000, 0x40000, 0xeb4edd10 , 0x239f22c4 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n138001c.53c", 0x160000, 0x40000, 0x43517a29 , 0xce80c326 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n138001d.538", 0x1A0000, 0x40000, 0x063774db , 0x1edcf268 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n138001d.53c", 0x1E0000, 0x40000, 0xf0f83c30 , 0xa41602a0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "n138001a.638", 0x420000, 0x40000, 0xbb9e761e , 0x5f2e5184 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "n138001a.63c", 0x460000, 0x40000, 0x096681b2 , 0x79b3e0b1 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "n138001b.638", 0x4A0000, 0x40000, 0x53a4ad1a , 0xb2afe822 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "n138001b.63c", 0x4E0000, 0x40000, 0x89fe2426 , 0xd425f967 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can probably be tossed */
	ROM_LOAD( "n138001c.638", 0x520000, 0x40000, 0xbb9e761e , 0x5f2e5184 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "n138001c.63c", 0x560000, 0x40000, 0x096681b2 , 0x79b3e0b1 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "n138001d.638", 0x5A0000, 0x40000, 0x53a4ad1a , 0xb2afe822 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "n138001d.63c", 0x5E0000, 0x40000, 0x89fe2426 , 0xd425f967 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( wheroes_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0xefb793b1 , 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0xc6b752fd , 0x0 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n138001a.378", 0x000000, 0x20000, 0x80fa9394 , 0x0 )

	ROM_LOAD( "n138001a.538", 0x020000, 0x40000, 0x5e3257a2 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "n138001a.53c", 0x060000, 0x40000, 0xe578b220 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n138001b.538", 0x0A0000, 0x40000, 0xd49a2e2c , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n138001b.53c", 0x0E0000, 0x40000, 0x5a23d57f , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_LOAD( "n138001c.538", 0x120000, 0x40000, 0x85142004 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n138001c.53c", 0x160000, 0x40000, 0x8e2d10f1 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n138001d.538", 0x1A0000, 0x40000, 0xd49a2e2c , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n138001d.53c", 0x1E0000, 0x40000, 0x5a23d57f , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "n138001a.638", 0x420000, 0x40000, 0xd85954f7 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "n138001a.63c", 0x460000, 0x40000, 0x02d7f219 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "n138001b.638", 0x4A0000, 0x40000, 0xdfce4884 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "n138001b.63c", 0x4E0000, 0x40000, 0x606b1e3f , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	ROM_LOAD( "n138001c.638", 0x520000, 0x40000, 0x99bb8e65 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "n138001c.63c", 0x560000, 0x40000, 0xac373d15 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "n138001d.638", 0x5A0000, 0x40000, 0xdfce4884 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "n138001d.63c", 0x5E0000, 0x40000, 0x606b1e3f , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( sengoku2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0xfab509f3 , 0xd1bf3fa5 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0x4aab2621 , 0xee9d0bb4 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION_DISPOSE(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n138001a.378", 0x000000, 0x20000, 0x75055fa7 , 0xcd9802a3 )

	ROM_LOAD( "n138001a.538", 0x020000, 0x40000, 0xec4c95d6 , 0xda18aaed ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "n138001a.53c", 0x060000, 0x40000, 0xbebc0096 , 0x19796c4f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "n138001b.538", 0x0A0000, 0x40000, 0x30ace9d6 , 0x891b6386 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "n138001b.53c", 0x0E0000, 0x40000, 0x30ace9d6 , 0x891b6386 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_LOAD( "n138001c.538", 0x120000, 0x40000, 0x9dd108c1 , 0xc5eaabe8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "n138001c.53c", 0x160000, 0x40000, 0xbc893de3 , 0x22633905 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "n138001d.538", 0x1A0000, 0x40000, 0x30ace9d6 , 0x891b6386 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "n138001d.53c", 0x1E0000, 0x40000, 0x30ace9d6 , 0x891b6386 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "n138001a.638", 0x420000, 0x40000, 0xc1b91517 , 0x5b27c829 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "n138001a.63c", 0x460000, 0x40000, 0x83508e88 , 0xe8b46e26 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "n138001b.638", 0x4A0000, 0x40000, 0x7b1109c5 , 0x93d25955 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "n138001b.63c", 0x4E0000, 0x40000, 0x7b1109c5 , 0x93d25955 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	ROM_LOAD( "n138001c.638", 0x520000, 0x40000, 0x8f3b4603 , 0x432bd7d0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "n138001c.63c", 0x560000, 0x40000, 0xed121a54 , 0xba3f54b2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "n138001d.638", 0x5A0000, 0x40000, 0x7b1109c5 , 0x93d25955 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "n138001d.63c", 0x5E0000, 0x40000, 0x7b1109c5 , 0x93d25955 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

/* Rom list roms */
ROM_START( puzzledp_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "pdpon_p1.rom", 0x000000, 0x080000, 0x9e0d0727 , 0x2b61415b )

	ROM_REGION_DISPOSE(0x220000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pdpon_s1.rom", 0x000000, 0x020000, 0x6e1c6af2 , 0xcd19264f )
	ROM_LOAD( "pdpon_c1.rom", 0x020000, 0x100000, 0x272b3889 , 0xcc0095ef ) /* Plane 0,1 */
	ROM_LOAD( "pdpon_c2.rom", 0x120000, 0x100000, 0xd6a5d681 , 0x42371307 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( neomrdo_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "neomd-p1.rom", 0x000000, 0x100000, 0x9b19dd5d , 0x334ea51e )

	ROM_REGION_DISPOSE(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "neomd-s1.rom", 0x000000, 0x020000, 0x68617553 , 0x6aebafce )
	ROM_LOAD( "neomd-c1.rom", 0x020000, 0x200000, 0xdcb3239b , 0xc7541b9d )
	ROM_LOAD( "neomd-c2.rom", 0x220000, 0x200000, 0x063505df , 0xf57166d2 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( bjourney_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "bj-p1.rom", 0x000000, 0x100000, 0xdf62eef6 , 0x6a2f6d4a )

	ROM_REGION_DISPOSE(0x320000)
	ROM_LOAD( "bj-s1.rom", 0x000000, 0x020000, 0x274fcd61 , 0x843c3624 )
	ROM_LOAD( "bj-c1.rom", 0x020000, 0x100000, 0xe5cc673c , 0x4d47a48c )
	ROM_LOAD( "bj-c3.rom", 0x120000, 0x080000, 0x231ab6bc , 0x66e69753 )
	ROM_LOAD( "bj-c2.rom", 0x1A0000, 0x100000, 0x0a5503cb , 0xe8c1491a )
	ROM_LOAD( "bj-c4.rom", 0x2A0000, 0x080000, 0x70da7a0a , 0x71bfd48a )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( nam_1975_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "nam_p1.rom", 0x000000, 0x080000, 0x05c693c6 , 0xcc9fc951 )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nam_s1.rom", 0x000000, 0x20000, 0x320481b0 , 0x7988ba51 )
	ROM_LOAD( "nam_c1.rom", 0x020000, 0x80000, 0x2406dbf6 , 0x32ea98e1 ) /* Plane 0,1 */
	ROM_LOAD( "nam_c3.rom", 0x0A0000, 0x80000, 0x28e59777 , 0x0151054c ) /* Plane 0,1 */
	ROM_LOAD( "nam_c5.rom", 0x120000, 0x80000, 0x63a40fe8 , 0x90b74cc2 ) /* Plane 0,1 */
	ROM_LOAD( "nam_c2.rom", 0x1A0000, 0x80000, 0xb1f2bfc6 , 0xcbc4064c ) /* Plane 2,3 */
	ROM_LOAD( "nam_c4.rom", 0x220000, 0x80000, 0x0949001d , 0x0a32570d ) /* Plane 2,3 */
	ROM_LOAD( "nam_c6.rom", 0x2A0000, 0x80000, 0xfd75150b , 0xe62bed58 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( maglord_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "magl_p1.rom", 0x000000, 0x080000, 0xa11991e9 , 0xbd0a492d )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "magl_s1.rom", 0x000000, 0x20000, 0xc6d559c9 , 0x1c5369a2 )
	ROM_LOAD( "magl_c1.rom", 0x020000, 0x80000, 0x96bd6edf , 0x806aee34 ) /* Plane 0,1 */
	ROM_LOAD( "magl_c3.rom", 0x0A0000, 0x80000, 0x522baaf3 , 0xc4c2b926 ) /* Plane 0,1 */
	ROM_LOAD( "magl_c5.rom", 0x120000, 0x80000, 0x88b6e4ee , 0x69086dec ) /* Plane 0,1 */
	ROM_LOAD( "magl_c2.rom", 0x1A0000, 0x80000, 0x49d5ebb1 , 0x34aa9a86 ) /* Plane 2,3 */
	ROM_LOAD( "magl_c4.rom", 0x220000, 0x80000, 0x46e1e381 , 0x9c46dcf4 ) /* Plane 2,3 */
	ROM_LOAD( "magl_c6.rom", 0x2A0000, 0x80000, 0x0f8f71b3 , 0xab7ac142 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( cybrlipa_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "cybl_p1.rom", 0x000000, 0x080000, 0x52423946 , 0x0 )

	ROM_REGION_DISPOSE(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cybl_s1.rom", 0x000000, 0x20000, 0x6ee68484 , 0x0 )
	ROM_LOAD( "cybl_c1.rom", 0x020000, 0x80000, 0x296da99f , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "cybl_c3.rom", 0x0A0000, 0x80000, 0x2924fbb8 , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "cybl_c5.rom", 0x120000, 0x80000, 0xeaf51c0d , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "cybl_c2.rom", 0x1A0000, 0x80000, 0xc440268e , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "cybl_c4.rom", 0x220000, 0x80000, 0x1cc5b8d7 , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "cybl_c6.rom", 0x2A0000, 0x80000, 0x216d4a99 , 0x0 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( tpgolf_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "topg_p1.rom", 0x000000, 0x080000, 0xe6276feb , 0x0 )
	ROM_LOAD_WIDE_SWAP( "topg_p2.rom", 0x080000, 0x080000, 0x927c7fa4 , 0x0 )

	ROM_REGION_DISPOSE(0x320000)
	ROM_LOAD( "topg_s1.rom", 0x000000, 0x20000, 0xfdf58a4b , 0x0 )
	ROM_LOAD( "topg_c1.rom", 0x020000, 0x80000, 0x0ae73a99 , 0x0 )
	ROM_LOAD( "topg_c3.rom", 0x0a0000, 0x80000, 0xf4793289 , 0x0 )
	ROM_LOAD( "topg_c5.rom", 0x120000, 0x80000, 0xfcf80ac2 , 0x0 )
	ROM_LOAD( "topg_c2.rom", 0x1a0000, 0x80000, 0xf1d3a1c9 , 0x0 )
	ROM_LOAD( "topg_c4.rom", 0x220000, 0x80000, 0x2151cda9 , 0x0 )
	ROM_LOAD( "topg_c6.rom", 0x2a0000, 0x80000, 0x450efb44 , 0x0 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( wjammers_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "windj_p1.rom", 0x000000, 0x100000, 0xb6f2d38a , 0x0 )

	ROM_REGION_DISPOSE(0x420000)
	ROM_LOAD( "windj_s1.rom", 0x000000, 0x020000, 0xe1c28b30 , 0x0 )
	ROM_LOAD( "windj_c1.rom", 0x020000, 0x100000, 0xbf043d4e , 0x0 )
	ROM_LOAD( "windj_c3.rom", 0x120000, 0x100000, 0x7d144dec , 0x0 )
	ROM_LOAD( "windj_c2.rom", 0x220000, 0x100000, 0x585c1b54 , 0x0 )
	ROM_LOAD( "windj_c4.rom", 0x320000, 0x100000, 0x30d052d4 , 0x0 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( janshin_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "jans-p1.rom", 0x000000, 0x100000, 0x4adbd11f , 0x7514cb7a )

	ROM_REGION_DISPOSE(0x420000)
	ROM_LOAD( "jans-s1.rom", 0x000000, 0x020000, 0x71af3c35 , 0x8285b25a )
	ROM_LOAD( "jans-c1.rom", 0x020000, 0x200000, 0x96d96aef , 0x3fa890e9 )
	ROM_LOAD( "jans-c2.rom", 0x220000, 0x200000, 0x1c13a67d , 0x59c48ad8 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( pbobble_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "puzzb_p1.rom", 0x000000, 0x080000, 0x6164b68a , 0x6102ca14 )

	ROM_REGION_DISPOSE(0x520000)
	ROM_LOAD( "puzzb_s1.rom", 0x000000, 0x020000, 0x5fceacda , 0x9caae538 )
	ROM_LOAD( "puzzb_c1.rom", 0x020000, 0x100000, 0xf5f24364 , 0x7f250f76 ) /* Plane 0,1 */
	ROM_LOAD( "puzzb_c3.rom", 0x120000, 0x100000, 0x6156f272 , 0x4b641ba1 ) /* Plane 0,1 */
	ROM_LOAD( "puzzb_c5.rom", 0x220000, 0x080000, 0xf92e0064 , 0xe89ad494 ) /* Plane 0,1 */
	ROM_LOAD( "puzzb_c2.rom", 0x2A0000, 0x100000, 0xc53462d4 , 0x20912873 ) /* Plane 2,3 */
	ROM_LOAD( "puzzb_c4.rom", 0x3A0000, 0x100000, 0xd29c5824 , 0x35072596 ) /* Plane 2,3 */
	ROM_LOAD( "puzzb_c6.rom", 0x4A0000, 0x080000, 0x96fa725c , 0x4b42d7eb ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( pspikes2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "spike_p1.rom", 0x000000, 0x100000, 0xc6e92385 , 0x0 )

	ROM_REGION_DISPOSE(0x620000)
	ROM_LOAD( "spike_s1.rom", 0x000000, 0x020000, 0x0 , 0x0 )
	ROM_LOAD( "spike_c1.rom", 0x020000, 0x100000, 0x0 , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "spike_c3.rom", 0x120000, 0x100000, 0x0 , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "spike_c5.rom", 0x220000, 0x100000, 0x0 , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "spike_c2.rom", 0x320000, 0x100000, 0x0 , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "spike_c4.rom", 0x420000, 0x100000, 0x0 , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "spike_c6.rom", 0x520000, 0x100000, 0x0 , 0x0 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( sidkicks_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "sidek_p1.rom", 0x000000, 0x080000, 0xc6e92385 , 0x9cd97256 )

	ROM_REGION_DISPOSE(0x620000)	  /* 6 meg temporary */
	ROM_LOAD( "sidek_s1.rom", 0x000000, 0x020000, 0xb0eaf1f4 , 0x97689804 )
	ROM_LOAD( "sidek_c1.rom", 0x020000, 0x100000, 0xd8859477 , 0x53e1c002 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x100000 )
	ROM_LOAD( "sidek_c2.rom", 0x320000, 0x100000, 0x1ce63e80 , 0x776a2d1f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x520000, 0x100000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( viewpoin_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "viewp_p1.rom", 0x000000, 0x100000, 0xe7cbd8e3 , 0x17aa899d )

	ROM_REGION_DISPOSE(0x620000)
	ROM_LOAD( "viewp_s1.rom", 0x000000, 0x020000, 0x07f40dac , 0x9fea5758 )
	ROM_LOAD( "viewp_c1.rom", 0x020000, 0x100000, 0xea215125 , 0xd624c132 )
	ROM_CONTINUE(             0x220000, 0x100000 )
	ROM_LOAD( "viewp_c2.rom", 0x320000, 0x100000, 0x58c80c72 , 0x40d69f1e )
	ROM_CONTINUE(             0x520000, 0x100000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( kotm2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "kotm2_p1.rom", 0x000000, 0x080000, 0x9a6d6e25 , 0xb372d54c )
	ROM_LOAD_WIDE_SWAP( "kotm2_p2.rom", 0x080000, 0x080000, 0x8c041444 , 0x28661afe )

	ROM_REGION_DISPOSE(0x820000)
	ROM_LOAD( "kotm2_s1.rom", 0x000000, 0x020000, 0x6e9f0d79 , 0x63ee053a )
	ROM_LOAD( "kotm2_c1.rom", 0x020000, 0x100000, 0x81eafb70 , 0x6d1c4aa9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x100000 )
	ROM_LOAD( "kotm2_c3.rom", 0x120000, 0x100000, 0x2e0f570f , 0x40156dca ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x100000 )

	ROM_LOAD( "kotm2_c2.rom", 0x420000, 0x100000, 0xbedc7a74 , 0xf7b75337 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x100000 )
	ROM_LOAD( "kotm2_c4.rom", 0x520000, 0x100000, 0x8eed9d31 , 0xb0d44111 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x100000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( tophuntr_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "thunt_p1.rom", 0x000000, 0x100000, 0x23d5a4d5 , 0x0 )
	ROM_LOAD_WIDE_SWAP( "thunt_p2.rom", 0x100000, 0x100000, 0x47c92f61 , 0x0 )

	ROM_REGION_DISPOSE(0x820000)
	ROM_LOAD( "thunt_s1.rom", 0x000000, 0x020000, 0xf530a374 , 0x0 )
	ROM_LOAD( "thunt_c1.rom", 0x020000, 0x100000, 0xc900201a , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "thunt_c3.rom", 0x120000, 0x100000, 0xeac31af5 , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "thunt_c5.rom", 0x220000, 0x100000, 0x5f1ecae2 , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "thunt_c7.rom", 0x320000, 0x100000, 0x751dd773 , 0x0 ) /* Plane 0,1 */

	ROM_LOAD( "thunt_c2.rom", 0x420000, 0x100000, 0x81f70bf1 , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "thunt_c4.rom", 0x520000, 0x100000, 0x3a5132e3 , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "thunt_c6.rom", 0x620000, 0x100000, 0x3107d67f , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "thunt_c8.rom", 0x720000, 0x100000, 0x05ab104f , 0x0 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( samsho_rom )
	ROM_REGION(0x180000)
	ROM_LOAD_WIDE_SWAP( "samsh_p1.rom", 0x000000, 0x080000, 0x12f72ba1 , 0x80aa6c97 )
	ROM_LOAD_WIDE_SWAP( "samsh_p2.rom", 0x080000, 0x080000, 0x0d9ff367 , 0x71768728 )
	ROM_LOAD_WIDE_SWAP( "samsh_p3.rom", 0x100000, 0x080000, 0x711106ff , 0x38ee9ba9 )

	ROM_REGION_DISPOSE(0x920000)
	ROM_LOAD( "samsh_s1.rom", 0x000000, 0x020000, 0xbdd0e2e8 , 0x9142a4d3 )
	ROM_LOAD( "samsh_c1.rom", 0x020000, 0x200000, 0x65842ce8 , 0x2e5873a4 ) /* Plane 0,1 */
	ROM_LOAD( "samsh_c3.rom", 0x220000, 0x200000, 0x40551c71 , 0xf3dabd1e ) /* Plane 0,1 */
	ROM_LOAD( "samsh_c5.rom", 0x420000, 0x080000, 0x55ec08c2 , 0xa2bb8284 ) /* Plane 0,1 */
	ROM_LOAD( "samsh_c2.rom", 0x4a0000, 0x200000, 0xa7b6085a , 0x04febb10 ) /* Plane 2,3 */
	ROM_LOAD( "samsh_c4.rom", 0x6a0000, 0x200000, 0x898f635f , 0x935c62f0 ) /* Plane 2,3 */
	ROM_LOAD( "samsh_c6.rom", 0x8a0000, 0x080000, 0xbc249804 , 0x4fa71252 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( karnov_r_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "karev_p1.rom", 0x000000, 0x100000, 0x884db4a9 , 0x0 )

	ROM_REGION_DISPOSE(0xc20000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "karev_s1.rom", 0x000000, 0x020000, 0x894a6b44 , 0x0 )

	ROM_LOAD( "karev_c1.rom", 0x020000, 0x200000, 0x3970e842 , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "karev_c3.rom", 0x220000, 0x200000, 0x3dd5504f , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "karev_c5.rom", 0x420000, 0x200000, 0xbd814fcb , 0x0 ) /* Plane 0,1 */

	ROM_LOAD( "karev_c2.rom", 0x620000, 0x200000, 0x49d5ebb1 , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "karev_c4.rom", 0x820000, 0x200000, 0x46e1e381 , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "karev_c6.rom", 0xa20000, 0x200000, 0x0f8f71b3 , 0x0 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( goalx3_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "goal!_p1.rom", 0x100000, 0x100000, 0xba42e8e4 , 0x2a019a79 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	ROM_REGION_DISPOSE(0x1020000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "goal!_s1.rom", 0x000000, 0x020000, 0x4e9973c1 , 0xc0eaad86 )

	ROM_LOAD( "goal!_c1.rom", 0x220000, 0x200000, 0x1dd3a7f1 , 0xd061f1f5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x020000, 0x200000 )
	ROM_LOAD( "goal!_c3.rom", 0x420000, 0x400000, 0x36be0000 , 0xcea2656b ) /* Plane 0,1 */

	ROM_LOAD( "goal!_c2.rom", 0xa20000, 0x200000, 0xefb77413 , 0x3f63c1a2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x820000, 0x200000 )
	ROM_LOAD( "goal!_c4.rom", 0xc20000, 0x400000, 0x2acc0000 , 0x04782666 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( samsho2_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "sams2_p1.rom", 0x100000, 0x100000, 0x14a75045 , 0x0 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	ROM_REGION_DISPOSE(0x1020000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sams2_s1.rom", 0x000000, 0x020000, 0xe65d89ed , 0x0 )

	ROM_LOAD( "sams2_c1.rom", 0x020000, 0x200000, 0x03c0b0a4 , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "sams2_c3.rom", 0x220000, 0x200000, 0x5e9cb8e2 , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "sams2_c5.rom", 0x420000, 0x200000, 0xa2e24856 , 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "sams2_c7.rom", 0x620000, 0x200000, 0x13d59905 , 0x0 ) /* Plane 0,1 */

	ROM_LOAD( "sams2_c2.rom", 0x820000, 0x200000, 0x22776613 , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "sams2_c4.rom", 0xa20000, 0x200000, 0xce073a07 , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "sams2_c6.rom", 0xc20000, 0x200000, 0xf25e5b9e , 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "sams2_c8.rom", 0xe20000, 0x200000, 0xa3bd190d , 0x0 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( mslug_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "mslug_p1.rom", 0x100000, 0x100000, 0xf09edec2 , 0x0 )
	ROM_CONTINUE(                        0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	ROM_REGION_DISPOSE(0x1020000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mslug_s1.rom", 0x000000, 0x020000, 0x075a18c4 , 0x0 )
	ROM_LOAD( "mslug_c1.rom", 0x220000, 0x200000, 0xeaf875c0 , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x020000, 0x200000 )
	ROM_LOAD( "mslug_c3.rom", 0x620000, 0x200000, 0x30104ebe , 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x420000, 0x200000 )

	ROM_LOAD( "mslug_c2.rom", 0xa20000, 0x200000, 0xd1d1d749 , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x820000, 0x200000 )
	ROM_LOAD( "mslug_c4.rom", 0xe20000, 0x200000, 0xe3530fcd , 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc20000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x9036d879 )
ROM_END

ROM_START( wcup_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "wcupp.1", 0x000000, 0x200000, 0xa11991e9 , 0x0 )

	ROM_REGION_DISPOSE(0xc20000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "wcups.1", 0x000000, 0x10000, 0xc6d559c9 , 0x0 ) /* check */

      /*  ROM_LOAD( "wcupch.2.rom", 0x020000, 0x200000, 0x96bd6edf ) */ /* Plane 0,1 */
      /*  ROM_LOAD( "mslug_c3.rom", 0x420000, 0x200000, 0x522baaf3 ) */ /* Plane 0,1 */

    /*    ROM_LOAD( "mslug_c2.rom", 0x420000, 0x200000, 0x49d5ebb1 ) */ /* Plane 2,3 */
    /*    ROM_LOAD( "mslug_c4.rom", 0xc20000, 0x200000, 0x46e1e381 ) */ /* Plane 2,3 */

/* m1 & v1 unused at moment */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 , 0x0 )
ROM_END

/* dummy entry for the dummy bios driver */
ROM_START( bios_rom )
	ROM_REGION(0x020000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x000000, 0x020000, 0xf6c12685 , 0x0 )
ROM_END

/* Not really a ROM decode routine. This just clears stuff for init_machine to check */
static void neogeo_decode(void)
{
	neogeo_rom_loaded = 0;
	rambank = rambank2 = NULL;
}

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
	&dummy_bios_machine_driver, /* Dummy */
	bios_rom,
	0, 0,
	0,
	0,      /* sound_prom */
	neogeo_ports,
	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0,0
};

NEODRIVER(joyjoy,"Joy Joy Kid/Puzzled","1990","SNK","",&neogeo_0x2000_machine_driver)
NEODRIVER(ridhero,"Riding Hero","1990","SNK","",&neogeo_0x4000_machine_driver)
NEODRIVERX(ttbb,"2020bb","2020 Baseball","1991","SNK","",&neogeo_0x4000_machine_driver)
NEODRIVER(lresort,"Last Resort","1992","SNK","",&neogeo_0x6000_machine_driver)
NEODRIVER(fbfrenzy,"Football Frenzy","19??","SNK","",&neogeo_0x6000_machine_driver)
NEODRIVER(alpham2,"Alpha Mission 2","1991","SNK","",&neogeo_0x6000_machine_driver)
NEODRIVER(eightman,"Eight Man","1991","SNK","",&neogeo_0x6000_machine_driver)
NEODRIVER(ararmy,"Robo Army","19??","SNK","",&neogeo_0x6000_machine_driver)
NEODRIVER(ncombat,"Ninja Combat","1990","Alpha Denshi Co","",&neogeo_0x6000_machine_driver)
NEODRIVER(socbrawl,"Soccer Brawl","19??","??","",&neogeo_0x6000_machine_driver)
NEODRIVER(bstars,"Baseball Stars","19??","????","",&neogeo_0x6000_machine_driver)
NEODRIVER(fatfury1,"Fatal Fury - King of Fighters","1991","SNK","",&neogeo_0x8000_machine_driver)
NEODRIVER(mutnat,"Mutation Nation","1992","SNK","",&neogeo_0x8000_machine_driver)
NEODRIVER(burningf,"Burning Fight","1991","SNK","",&neogeo_0x8000_machine_driver)
NEODRIVER(crsword,"Crossed Swords","1991","Alpha Denshi Co","",&neogeo_0x8000_machine_driver)
NEODRIVER(cyberlip,"Cyber Lip","1990","SNK","",&neogeo_0x8000_machine_driver)
NEODRIVER(kingofm,"King of the Monsters","19??","SNK","",&neogeo_0x8000_machine_driver)
NEODRIVER(gpilots,"Ghost Pilots","1991","SNK","",&neogeo_0x8000_machine_driver)
NEODRIVER(trally,"Thrash Rally","1991","Alpha Denshi Co","",&neogeo_0x8000_machine_driver)
NEODRIVER(lbowling,"League Bowling","1990","SNK","",&neogeo_0x8000_machine_driver)
NEODRIVER(ncommand,"Ninja Command","1992","Alpha Denshi Co","",&neogeo_0x8000_machine_driver)
NEODRIVER(sengoku,"Sengoku","19??","????","",&neogeo_0x8000_machine_driver)
NEODRIVERX(countb,"3countb","3 Count Bout","1993","SNK","",&neogeo_0x10000_machine_driver)
NEODRIVER(androdun,"Andro Dunos","1992","Visco","",&neogeo_0x10000_machine_driver)
NEODRIVER(artfight,"Art of Fighting","1992","SNK","",&neogeo_0x10000_machine_driver)
NEODRIVER(superspy,"Super Spy","1990","SNK","",&neogeo_0x10000_machine_driver)
NEODRIVER(wheroes,"World Heroes","1992","Alpha Denshi Co","",&neogeo_0x10000_machine_driver)
NEODRIVER(sengoku2,"Sengoku 2","19??","Alpha Denshi Co","",&neogeo_0x10000_machine_driver)

NEODRIVER(puzzledp,"Puzzle de Pon","1995","Taito (Visco license)","",&neogeo_0x4000_rl_machine_driver)
NEODRIVER(bjourney,"Blues Journey","1990","Alpha Denshi Co","TJ Grant",&neogeo_0x6000_rl_machine_driver)
NEODRIVER(nam_1975,"NAM 1975","1990","SNK","",&neogeo_0x6000_rl_machine_driver)
NEODRIVER(maglord,"Magician Lord","1990","Alpha Denshi Co","",&neogeo_0x6000_rl_machine_driver)
NEODRIVER(cybrlipa,"Cyber Lip (alt)","1990","SNK","",&neogeo_0x6000_rl_machine_driver)
NEODRIVER(tpgolf,"Top Player's Golf","1990","SNK","",&neogeo_0x6000_rl_machine_driver)
NEODRIVER(wjammers,"Wind Jammers","1994","Data East","TJ Grant",&neogeo_0x8000_rl_machine_driver)
NEODRIVER(janshin,"Quest of Jongmaster","1994","Aicom","",&neogeo_0x8000_rl_machine_driver)
NEODRIVER(pbobble,"Puzzle Bobble","1994","Taito","",&neogeo_0xa000_rl_machine_driver)
NEODRIVER(sidkicks,"Super Sidekicks","1992","SNK","",&neogeo_0xc000_rl_machine_driver)
NEODRIVER(pspikes2,"Power Spikes 2","1994","Video System Co","",&neogeo_0xc000_rl_machine_driver)
NEODRIVER(viewpoin,"Viewpoint","19??","??","",&neogeo_0xc000_rl_machine_driver)
NEODRIVER(kotm2,"King of the Monsters 2","1992","SNK","",&neogeo_0x10000_rl_machine_driver)
NEODRIVER(tophuntr,"Top Hunter","19??","???","",&neogeo_0x10000_rl_machine_driver)
NEODRIVER(samsho,"Samurai Shodown","1992","SNK","",&neogeo_0x12000_rl_machine_driver)
NEODRIVER(karnov_r,"Karnov's Revenge","1994","Data East","",&neogeo_0x18000_rl_machine_driver)
NEODRIVER(mslug,"Metal Slug","19??","??","",&neogeo_0x20000_rl_machine_driver)
NEODRIVER(goalx3,"Goal! Goal! Goal!","19??","??","",&neogeo_0x20000_rl_machine_driver)
NEODRIVER(samsho2,"Samurai Shodown 2","19??","??","",&neogeo_0x20000_rl_machine_driver)
NEODRIVER(neomrdo,"Neo Mr Do!","1996","Visco","",&neogeo_0x8000_rl_machine_driver)

/* preliminary drivers */
NEODRIVER(wcup,"Thrash Rally","1991","Alpha Denshi Co","",&neogeo_0x8000_machine_driver)
