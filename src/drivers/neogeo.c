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
	ROM_LOAD_ODD ("N022001a.038", 0x000000, 0x040000, 0xaca68aee)
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x120000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N022001a.378", 0x000000, 0x20000, 0x64f64ddc )
	ROM_LOAD( "N022001a.538", 0x020000, 0x80000, 0x91acf534 ) /* Plane 0,1 */
	ROM_LOAD( "N022001a.638", 0x0A0000, 0x80000, 0xd2f574c5 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( ridhero_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x4d335f81 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x220000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x000000, 0x20000, 0x0f7080de )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x89eb3269 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0A0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0xc70d6fb7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x120000, 0x40000, 0xdfddc5cd ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x160000, 0x40000, 0x28ce9334 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( ttbb_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x829ac1f8 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x220000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x000000, 0x20000, 0x23b37c91 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0xcb9e6734 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0a0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x33543bd8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x120000, 0x40000, 0x263ecb62 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1a0000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x160000, 0x40000, 0xd58909dd ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( lresort_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x954d9aeb )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x000000, 0x20000, 0x871a3e12 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x84229a28 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x349873c4 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0x77952209 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x1A0000, 0x40000, 0x43b89106 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x1E0000, 0x40000, 0x60018cf3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x220000, 0x40000, 0x6865777d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( ararmy_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x9ef7ffe9 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x000000, 0x20000, 0x6b1d1c51 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0xdf5cb028 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0xb645046f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0x3f253d7f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x1A0000, 0x40000, 0xd2542bca ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x1E0000, 0x40000, 0xdeceac9e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x220000, 0x40000, 0xd932018a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( fbfrenzy_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x8bfe2f3c )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x000000, 0x20000, 0xc6d79eed )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x936791f9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x9df5a633 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0xadf1b389 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x1A0000, 0x40000, 0x49f0e056 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x1E0000, 0x40000, 0xf054eaba ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x220000, 0x40000, 0x92fad81a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( alpham2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N082001a.038", 0x000000, 0x040000, 0x9a61c8c9 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N082001a.03C", 0x080000, 0x040000, 0xfac80000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N082001a.378", 0x000000, 0x20000, 0x1e767974 )

	ROM_LOAD( "N082001a.538", 0x020000, 0x40000, 0x92ad2b4d ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "N082001a.53c", 0x060000, 0x40000, 0x00be08f6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N082001b.538", 0x0A0000, 0x40000, 0xd238188e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "N082001a.638", 0x1A0000, 0x40000, 0x7b84a706 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N082001a.63c", 0x1E0000, 0x40000, 0x5099927f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N082001b.638", 0x220000, 0x40000, 0xfcd769fb ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( trally_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0xe80fdfc1 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N046001a.03c", 0x080000, 0x040000, 0x132fa6c9 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x000000, 0x20000, 0xe9ccba0a )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0xef22d0be ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0xb17fe101 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0x5c6ba5bf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1a0000, 0x40000 )

  	ROM_LOAD( "N046001a.638", 0x220000, 0x40000, 0x9604c64a ) /* Plane 2,3 */
  	ROM_CONTINUE(             0x320000, 0x40000 )
  	ROM_LOAD( "N046001a.63c", 0x260000, 0x40000, 0x7c49f737 ) /* Plane 2,3 */
  	ROM_CONTINUE(             0x260000, 0x40000 )
  	ROM_LOAD( "N046001b.638", 0x2a0000, 0x40000, 0xcb0f9e4d ) /* Plane 2,3 */
  	ROM_CONTINUE(             0x3a0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( eightman_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x3b227218 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x000000, 0x20000, 0x09625510 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x6ecb0b13 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0xaa767fda ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0xeb3faa85 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x1A0000, 0x40000, 0xa90186b9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x1E0000, 0x40000, 0x2a225c0e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x220000, 0x40000, 0x547ea2be ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( ncombat_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x7a00f452 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x000000, 0x20000, 0xd70c3894 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x6fbef8c8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x79611a8d ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0xed83acaf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x1A0000, 0x40000, 0x1537c62d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x1E0000, 0x40000, 0x42e8e64e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x220000, 0x40000, 0x05886710 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( socbrawl_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0xa1f12045 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x000000, 0x20000, 0xa82f2e69 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x4c75e0c9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x31faf80c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0x9c514437 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x1A0000, 0x40000, 0xc54eaaca ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x1E0000, 0x40000, 0x2a4bfed9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x220000, 0x40000, 0xeeeb7897 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( bstars_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N042001a.038", 0x000000, 0x040000, 0xc10f28df )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N138001a.378", 0x000000, 0x20000, 0xb87843ba )

	ROM_LOAD( "N138001a.538", 0x020000, 0x40000, 0x2ddb7e3b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0E0000, 0x40000 )
	ROM_LOAD( "N138001a.53c", 0x060000, 0x40000, 0xa2bfd43f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N138001b.538", 0x0A0000, 0x40000, 0xc1b46e16 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )

	ROM_LOAD( "N138001a.638", 0x1A0000, 0x40000, 0xc3028d66 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N138001a.63c", 0x1E0000, 0x40000, 0x16ab04db ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N138001b.638", 0x220000, 0x40000, 0x5ab02a68 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( gpilots_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N058001a.038", 0x000000, 0x040000, 0x5ca22fe6 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N058001a.03c", 0x080000, 0x040000, 0x6c640000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N058001a.378", 0x000000, 0x20000, 0x6118ea2a )

	ROM_LOAD( "N058001a.538", 0x020000, 0x40000, 0x022dd51f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N058001a.53c", 0x060000, 0x40000, 0x2fce4578 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N058001b.538", 0x0A0000, 0x40000, 0xcf579505 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N058001b.53c", 0x0E0000, 0x40000, 0x39227a48 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "N058001a.638", 0x220000, 0x40000, 0xe6fe9e6e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N058001a.63c", 0x260000, 0x40000, 0xead70b71 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N058001b.638", 0x2A0000, 0x40000, 0xc9d8ad90 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N058001b.63c", 0x2E0000, 0x40000, 0x2971acff ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( crsword_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x6e5ca756 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x000000, 0x20000, 0xc6de9456 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0xab929c8e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x6140cdc2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0x4d93fc9b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N046001b.53c", 0x0E0000, 0x40000, 0x6856e324 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x220000, 0x40000, 0x24104162 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x260000, 0x40000, 0xa1976df7 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x2A0000, 0x40000, 0x12f85b88 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N046001b.63c", 0x2E0000, 0x40000, 0x432aaf20 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( cyberlip_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N062001a.038", 0x000000, 0x040000, 0x04b4b1ce )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N062001a.03c", 0x080000, 0x040000, 0x0c000000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N062001a.378", 0x000000, 0x20000, 0x6ee68484 )

	ROM_LOAD( "N062001a.538", 0x020000, 0x40000, 0x13922c1a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N062001a.53c", 0x060000, 0x40000, 0x6df289ca ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N062001b.538", 0x0A0000, 0x40000, 0x5dedccdd ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N062001b.53c", 0x0E0000, 0x40000, 0xc4000000 ) /* Plane 0,1 - unnecessary? */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "N062001a.638", 0x220000, 0x40000, 0x4298d47c ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N062001a.63c", 0x260000, 0x40000, 0x0da06d02 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N062001b.638", 0x2A0000, 0x40000, 0x1aaea774 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N062001b.63c", 0x2E0000, 0x40000, 0x40000000 ) /* Plane 2,3 - unnecessary? */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( kingofm_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N058001a.038", 0x000000, 0x040000, 0xf753af95 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N058001a.03c", 0x080000, 0x040000, 0xfe700000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N058001a.378", 0x000000, 0x20000, 0x6f50f8f6 )

	ROM_LOAD( "N058001a.538", 0x020000, 0x40000, 0x8c31a5c5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N058001a.53c", 0x060000, 0x40000, 0x17ea64c6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N058001b.538", 0x0A0000, 0x40000, 0x0f5b896b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N058001b.53c", 0x0E0000, 0x40000, 0x108013b8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "N058001a.638", 0x220000, 0x40000, 0x7a8349d3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N058001a.63c", 0x260000, 0x40000, 0xcedf3485 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N058001b.638", 0x2A0000, 0x40000, 0x752f97f9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N058001b.63c", 0x2E0000, 0x40000, 0xb7adb0b9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( burningf_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N054001a.038", 0x000000, 0x040000, 0xb0e1f177 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N054001a.378", 0x000000, 0x20000, 0x58bb9619 )

	ROM_LOAD( "N054001a.538", 0x020000, 0x40000, 0xc6004c86 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N054001a.53c", 0x060000, 0x40000, 0x3989eb5f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N054001b.538", 0x0A0000, 0x40000, 0x42134faf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N054001b.53c", 0x0E0000, 0x40000, 0x39f78509 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "N054001a.638", 0x220000, 0x40000, 0x553288e0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N054001a.63c", 0x260000, 0x40000, 0x3566a06e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N054001b.638", 0x2A0000, 0x40000, 0x8ac2d988 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N054001b.63c", 0x2E0000, 0x40000, 0x473c8f34 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( mutnat_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N054001a.038", 0x000000, 0x040000, 0x925138ad )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N054001a.378", 0x000000, 0x20000, 0xecf8b234 )

	ROM_LOAD( "N054001a.538", 0x020000, 0x40000, 0x3fe926f3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N054001a.53c", 0x060000, 0x40000, 0x437ccb1a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N054001b.538", 0x0A0000, 0x40000, 0x35e14ec5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N054001b.53c", 0x0E0000, 0x40000, 0x48563fa0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "N054001a.638", 0x220000, 0x40000, 0xb831cfcb ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N054001a.63c", 0x260000, 0x40000, 0x0ac511df ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N054001b.638", 0x2A0000, 0x40000, 0x036edf00 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N054001b.63c", 0x2E0000, 0x40000, 0xeae9166d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( lbowling_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N050001a.038", 0x000000, 0x040000, 0x1586a050 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N050001a.378", 0x000000, 0x20000, 0x74395cd9 )

	ROM_LOAD( "N050001a.538", 0x020000, 0x40000, 0xf66b1bf3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N050001a.53c", 0x060000, 0x40000, 0xf0000000 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N050001b.538", 0x0A0000, 0x40000, 0xf2000000 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N050001b.53c", 0x0E0000, 0x40000, 0xa6000000 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "N050001a.638", 0x220000, 0x40000, 0xbf85c625 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N050001a.63c", 0x260000, 0x40000, 0x26000000 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N050001b.638", 0x2A0000, 0x40000, 0x94000000 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N050001b.63c", 0x2E0000, 0x40000, 0xc4000000 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( fatfury1_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N058001a.038", 0x000000, 0x040000, 0x5a09be4b )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N058001a.03c", 0x080000, 0x040000, 0x34d80000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N058001a.378", 0x000000, 0x20000, 0x3e30d76e )

	ROM_LOAD( "N058001a.538", 0x020000, 0x40000, 0x2382e626 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N058001a.53c", 0x060000, 0x40000, 0x043fc03b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N058001b.538", 0x0A0000, 0x40000, 0x528be7c9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N058001b.53c", 0x0E0000, 0x40000, 0x4b77c26d ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "N058001a.638", 0x220000, 0x40000, 0x33251363 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N058001a.63c", 0x260000, 0x40000, 0xad5f0307 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N058001b.638", 0x2A0000, 0x40000, 0xe94d2355 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N058001b.63c", 0x2E0000, 0x40000, 0x09119b45 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( ncommand_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N054001a.038", 0x000000, 0x040000, 0xb30bdd01 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N054001a.03c", 0x080000, 0x040000, 0x31216705 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N054001a.378", 0x000000, 0x20000, 0x282dd3c7 )

	ROM_LOAD( "N054001a.538", 0x020000, 0x40000, 0x95786326 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N054001a.53c", 0x060000, 0x40000, 0x1e4d26b3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N054001b.538", 0x0A0000, 0x40000, 0xa27b99d1 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N054001b.53c", 0x0E0000, 0x40000, 0x3a951ff7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "N054001a.638", 0x220000, 0x40000, 0x8297fd5d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N054001a.63c", 0x260000, 0x40000, 0x57aa8e62 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N054001b.638", 0x2A0000, 0x40000, 0x2b761d24 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N054001b.63c", 0x2E0000, 0x40000, 0x06ea4fe0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( sengoku_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N058001a.038", 0x000000, 0x040000, 0x12d6b4fa )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N058001a.03c", 0x080000, 0x040000, 0x70180000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N058001a.378", 0x000000, 0x20000, 0x9079c2bb )

	ROM_LOAD( "N058001a.538", 0x020000, 0x40000, 0x4171b5f1 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x120000, 0x40000 )
	ROM_LOAD( "N058001a.53c", 0x060000, 0x40000, 0x383e11d0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x160000, 0x40000 )
	ROM_LOAD( "N058001b.538", 0x0A0000, 0x40000, 0xb4340b6c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1A0000, 0x40000 )
	ROM_LOAD( "N058001b.53c", 0x0E0000, 0x40000, 0xb4747c20 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1E0000, 0x40000 )

	ROM_LOAD( "N058001a.638", 0x220000, 0x40000, 0x34194ca7 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N058001a.63c", 0x260000, 0x40000, 0xa4770e4f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N058001b.638", 0x2A0000, 0x40000, 0xf7b9983f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N058001b.63c", 0x2E0000, 0x40000, 0x53cb942f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( countb_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N106001a.038", 0x000000, 0x040000, 0x30b2d082 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N106001a.03c", 0x080000, 0x040000, 0xfcba1382 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N106001a.378", 0x000000, 0x20000, 0xae7f1d57 )

	ROM_LOAD( "N106001a.538", 0x020000, 0x40000, 0x200ff619 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "N106001a.53c", 0x060000, 0x40000, 0xb2343298 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N106001b.538", 0x0A0000, 0x40000, 0xba08b4b6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N106001b.53c", 0x0E0000, 0x40000, 0x71015131 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_LOAD( "N106001c.538", 0x120000, 0x40000, 0x36446a7a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N106001c.53c", 0x160000, 0x40000, 0xaaac75fc ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N106001d.538", 0x1A0000, 0x40000, 0x703b9d89 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N106001d.53c", 0x1E0000, 0x40000, 0xab7a4c04 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "N106001a.638", 0x420000, 0x40000, 0xba6d9ee5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "N106001a.63c", 0x460000, 0x40000, 0xc52e5608 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "N106001b.638", 0x4A0000, 0x40000, 0x5b742260 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "N106001b.63c", 0x4E0000, 0x40000, 0x0243d7d3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	ROM_LOAD( "N106001c.638", 0x520000, 0x40000, 0x5578634a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "N106001c.63c", 0x560000, 0x40000, 0xdadf5ebf ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "N106001d.638", 0x5A0000, 0x40000, 0xab6ec08e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "N106001d.63c", 0x5E0000, 0x40000, 0x85d33953 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( androdun_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N138001a.038", 0x000000, 0x040000, 0x2182d3fc )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N138001a.03c", 0x080000, 0x040000, 0xddabe807 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N138001a.378", 0x000000, 0x20000, 0x52549d32 )

	ROM_LOAD( "N138001a.538", 0x020000, 0x40000, 0xed5f5fdb ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "N138001a.53c", 0x060000, 0x40000, 0xed710af3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N138001b.538", 0x0A0000, 0x40000, 0x52000000 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N138001b.53c", 0x0E0000, 0x40000, 0x16000000 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can be tossed */
	ROM_LOAD( "N138001c.538", 0x120000, 0x40000, 0xed5f5fdb ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N138001c.53c", 0x160000, 0x40000, 0xed710af3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N138001d.538", 0x1A0000, 0x40000, 0x52000000 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N138001d.53c", 0x1E0000, 0x40000, 0x16000000 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "N138001a.638", 0x420000, 0x40000, 0x1ab212f6 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "N138001a.63c", 0x460000, 0x40000, 0x6965fed7 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "N138001b.638", 0x4A0000, 0x40000, 0x8a000000 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "N138001b.63c", 0x4E0000, 0x40000, 0xcc000000 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can be tossed */
	ROM_LOAD( "N138001c.638", 0x520000, 0x40000, 0x1ab212f6 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "N138001c.63c", 0x560000, 0x40000, 0x6965fed7 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "N138001d.638", 0x5A0000, 0x40000, 0x8a000000 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "N138001d.63c", 0x5E0000, 0x40000, 0xcc000000 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( artfight_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N102001a.038", 0x000000, 0x040000, 0x12f67fb8 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N102001a.378", 0x000000, 0x20000, 0xbd276af3 )

	ROM_LOAD( "N102001a.538", 0x020000, 0x40000, 0x310ea698 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "N102001a.53c", 0x060000, 0x40000, 0xba31af63 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N102001b.538", 0x0A0000, 0x40000, 0xb605ed55 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N102001b.53c", 0x0E0000, 0x40000, 0x84f34535 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_LOAD( "N102001c.538", 0x120000, 0x40000, 0x36b70017 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N102001c.53c", 0x160000, 0x40000, 0xda6b2055 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N102001d.538", 0x1A0000, 0x40000, 0x5c626bf8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N102001d.53c", 0x1E0000, 0x40000, 0x9749ce05 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "N102001a.638", 0x420000, 0x40000, 0x00993dbd ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "N102001a.63c", 0x460000, 0x40000, 0x202041b8 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "N102001b.638", 0x4A0000, 0x40000, 0xf0234291 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "N102001b.63c", 0x4E0000, 0x40000, 0xec02023c ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	ROM_LOAD( "N102001c.638", 0x520000, 0x40000, 0xbe4c3758 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "N102001c.63c", 0x560000, 0x40000, 0xbdcf16bf ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "N102001d.638", 0x5A0000, 0x40000, 0x8242d040 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "N102001d.63c", 0x5E0000, 0x40000, 0xbe7d0093 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( superspy_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N138001a.038", 0x000000, 0x040000, 0x7e59a7cb )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N138001a.03c", 0x080000, 0x040000, 0x834c0000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N138001a.378", 0x000000, 0x20000, 0x6991ff05 )

	ROM_LOAD( "N138001a.538", 0x020000, 0x40000, 0xeb4edd10 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "N138001a.53c", 0x060000, 0x40000, 0x43517a29 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N138001b.538", 0x0A0000, 0x40000, 0x063774db ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N138001b.53c", 0x0E0000, 0x40000, 0xf0f83c30 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can probably be tossed */
	ROM_LOAD( "N138001c.538", 0x120000, 0x40000, 0xeb4edd10 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N138001c.53c", 0x160000, 0x40000, 0x43517a29 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N138001d.538", 0x1A0000, 0x40000, 0x063774db ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N138001d.53c", 0x1E0000, 0x40000, 0xf0f83c30 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "N138001a.638", 0x420000, 0x40000, 0xbb9e761e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "N138001a.63c", 0x460000, 0x40000, 0x096681b2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "N138001b.638", 0x4A0000, 0x40000, 0x53a4ad1a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "N138001b.63c", 0x4E0000, 0x40000, 0x89fe2426 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	/* These 4 are repeats of the last 4 - can probably be tossed */
	ROM_LOAD( "N138001c.638", 0x520000, 0x40000, 0xbb9e761e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "N138001c.63c", 0x560000, 0x40000, 0x096681b2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "N138001d.638", 0x5A0000, 0x40000, 0x53a4ad1a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "N138001d.63c", 0x5E0000, 0x40000, 0x89fe2426 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( wheroes_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N138001a.038", 0x000000, 0x040000, 0xefb793b1 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N138001a.03c", 0x080000, 0x040000, 0xc6b752fd )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N138001a.378", 0x000000, 0x20000, 0x80fa9394 )

	ROM_LOAD( "N138001a.538", 0x020000, 0x40000, 0x5e3257a2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "N138001a.53c", 0x060000, 0x40000, 0xe578b220 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N138001b.538", 0x0A0000, 0x40000, 0xd49a2e2c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N138001b.53c", 0x0E0000, 0x40000, 0x5a23d57f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_LOAD( "N138001c.538", 0x120000, 0x40000, 0x85142004 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N138001c.53c", 0x160000, 0x40000, 0x8e2d10f1 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N138001d.538", 0x1A0000, 0x40000, 0xd49a2e2c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N138001d.53c", 0x1E0000, 0x40000, 0x5a23d57f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "N138001a.638", 0x420000, 0x40000, 0xd85954f7 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "N138001a.63c", 0x460000, 0x40000, 0x02d7f219 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "N138001b.638", 0x4A0000, 0x40000, 0xdfce4884 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "N138001b.63c", 0x4E0000, 0x40000, 0x606b1e3f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	ROM_LOAD( "N138001c.638", 0x520000, 0x40000, 0x99bb8e65 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "N138001c.63c", 0x560000, 0x40000, 0xac373d15 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "N138001d.638", 0x5A0000, 0x40000, 0xdfce4884 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "N138001d.63c", 0x5E0000, 0x40000, 0x606b1e3f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( sengoku2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ("N138001a.038", 0x000000, 0x040000, 0xfab509f3 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N138001a.03c", 0x080000, 0x040000, 0x4aab2621 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_REGION(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N138001a.378", 0x000000, 0x20000, 0x75055fa7 )

	ROM_LOAD( "N138001a.538", 0x020000, 0x40000, 0xec4c95d6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x40000 )
	ROM_LOAD( "N138001a.53c", 0x060000, 0x40000, 0xbebc0096 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x260000, 0x40000 )
	ROM_LOAD( "N138001b.538", 0x0A0000, 0x40000, 0x30ace9d6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2A0000, 0x40000 )
	ROM_LOAD( "N138001b.53c", 0x0E0000, 0x40000, 0x30ace9d6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2E0000, 0x40000 )

	ROM_LOAD( "N138001c.538", 0x120000, 0x40000, 0x9dd108c1 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x40000 )
	ROM_LOAD( "N138001c.53c", 0x160000, 0x40000, 0xbc893de3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x360000, 0x40000 )
	ROM_LOAD( "N138001d.538", 0x1A0000, 0x40000, 0x30ace9d6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3A0000, 0x40000 )
	ROM_LOAD( "N138001d.53c", 0x1E0000, 0x40000, 0x30ace9d6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3E0000, 0x40000 )

	ROM_LOAD( "N138001a.638", 0x420000, 0x40000, 0xc1b91517 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x40000 )
	ROM_LOAD( "N138001a.63c", 0x460000, 0x40000, 0x83508e88 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x660000, 0x40000 )
	ROM_LOAD( "N138001b.638", 0x4A0000, 0x40000, 0x7b1109c5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6A0000, 0x40000 )
	ROM_LOAD( "N138001b.63c", 0x4E0000, 0x40000, 0x7b1109c5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x6E0000, 0x40000 )

	ROM_LOAD( "N138001c.638", 0x520000, 0x40000, 0x8f3b4603 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x40000 )
	ROM_LOAD( "N138001c.63c", 0x560000, 0x40000, 0xed121a54 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x760000, 0x40000 )
	ROM_LOAD( "N138001d.638", 0x5A0000, 0x40000, 0x7b1109c5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7A0000, 0x40000 )
	ROM_LOAD( "N138001d.63c", 0x5E0000, 0x40000, 0x7b1109c5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x7E0000, 0x40000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

/* Rom list roms */
ROM_START( puzzledp_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "pdpon_p1.rom", 0x000000, 0x080000, 0x9e0d0727 )

	ROM_REGION(0x220000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pdpon_s1.rom", 0x000000, 0x020000, 0x6e1c6af2 )
	ROM_LOAD( "pdpon_c1.rom", 0x020000, 0x100000, 0x272b3889 ) /* Plane 0,1 */
	ROM_LOAD( "pdpon_c2.rom", 0x120000, 0x100000, 0xd6a5d681 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( bjourney_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "bj-p1.rom",   0x000000, 0x100000, 0xdf62eef6 )

	ROM_REGION(0x320000)
	ROM_LOAD( "bj-s1.rom", 0x000000, 0x020000, 0x274fcd61 )
	ROM_LOAD( "bj-c1.rom", 0x020000, 0x100000, 0xe5cc673c )
	ROM_LOAD( "bj-c3.rom", 0x120000, 0x080000, 0x231ab6bc )
	ROM_LOAD( "bj-c2.rom", 0x1A0000, 0x100000, 0x0a5503cb )
	ROM_LOAD( "bj-c4.rom", 0x2A0000, 0x080000, 0x70da7a0a )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( nam_1975_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "nam_p1.rom",  0x000000, 0x080000, 0x05c693c6 )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nam_s1.rom", 0x000000, 0x20000, 0x320481b0 )
	ROM_LOAD( "nam_c1.rom", 0x020000, 0x80000, 0x2406dbf6 ) /* Plane 0,1 */
	ROM_LOAD( "nam_c3.rom", 0x0A0000, 0x80000, 0x28e59777 ) /* Plane 0,1 */
	ROM_LOAD( "nam_c5.rom", 0x120000, 0x80000, 0x63a40fe8 ) /* Plane 0,1 */
	ROM_LOAD( "nam_c2.rom", 0x1A0000, 0x80000, 0xb1f2bfc6 ) /* Plane 2,3 */
	ROM_LOAD( "nam_c4.rom", 0x220000, 0x80000, 0x0949001d ) /* Plane 2,3 */
	ROM_LOAD( "nam_c6.rom", 0x2A0000, 0x80000, 0xfd75150b ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( maglord_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "magl_p1.rom", 0x000000, 0x080000, 0xa11991e9 )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "magl_s1.rom", 0x000000, 0x20000, 0xc6d559c9 )
	ROM_LOAD( "magl_c1.rom", 0x020000, 0x80000, 0x96bd6edf ) /* Plane 0,1 */
	ROM_LOAD( "magl_c3.rom", 0x0A0000, 0x80000, 0x522baaf3 ) /* Plane 0,1 */
	ROM_LOAD( "magl_c5.rom", 0x120000, 0x80000, 0x88b6e4ee ) /* Plane 0,1 */
	ROM_LOAD( "magl_c2.rom", 0x1A0000, 0x80000, 0x49d5ebb1 ) /* Plane 2,3 */
	ROM_LOAD( "magl_c4.rom", 0x220000, 0x80000, 0x46e1e381 ) /* Plane 2,3 */
	ROM_LOAD( "magl_c6.rom", 0x2A0000, 0x80000, 0x0f8f71b3 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( cybrlipa_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "cybl_p1.rom", 0x000000, 0x080000, 0x52423946 )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cybl_s1.rom", 0x000000, 0x20000, 0x6ee68484 )
	ROM_LOAD( "cybl_c1.rom", 0x020000, 0x80000, 0x296da99f ) /* Plane 0,1 */
	ROM_LOAD( "cybl_c3.rom", 0x0A0000, 0x80000, 0x2924fbb8 ) /* Plane 0,1 */
	ROM_LOAD( "cybl_c5.rom", 0x120000, 0x80000, 0xeaf51c0d ) /* Plane 0,1 */
	ROM_LOAD( "cybl_c2.rom", 0x1A0000, 0x80000, 0xc440268e ) /* Plane 2,3 */
	ROM_LOAD( "cybl_c4.rom", 0x220000, 0x80000, 0x1cc5b8d7 ) /* Plane 2,3 */
	ROM_LOAD( "cybl_c6.rom", 0x2A0000, 0x80000, 0x216d4a99 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( tpgolf_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "topg_p1.rom", 0x000000, 0x080000, 0xe6276feb )
	ROM_LOAD_WIDE_SWAP( "topg_p2.rom", 0x080000, 0x080000, 0x927c7fa4 )

	ROM_REGION(0x320000)
	ROM_LOAD( "topg_s1.rom", 0x000000, 0x20000, 0xfdf58a4b )
	ROM_LOAD( "topg_c1.rom", 0x020000, 0x80000, 0x0ae73a99 )
	ROM_LOAD( "topg_c3.rom", 0x0a0000, 0x80000, 0xf4793289 )
	ROM_LOAD( "topg_c5.rom", 0x120000, 0x80000, 0xfcf80ac2 )
	ROM_LOAD( "topg_c2.rom", 0x1a0000, 0x80000, 0xf1d3a1c9 )
	ROM_LOAD( "topg_c4.rom", 0x220000, 0x80000, 0x2151cda9 )
	ROM_LOAD( "topg_c6.rom", 0x2a0000, 0x80000, 0x450efb44 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( wjammers_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "windj_p1.rom", 0x000000, 0x100000, 0xb6f2d38a )

	ROM_REGION(0x420000)
	ROM_LOAD( "windj_s1.rom", 0x000000, 0x020000, 0xe1c28b30 )
	ROM_LOAD( "windj_c1.rom", 0x020000, 0x100000, 0xbf043d4e )
	ROM_LOAD( "windj_c3.rom", 0x120000, 0x100000, 0x7d144dec )
	ROM_LOAD( "windj_c2.rom", 0x220000, 0x100000, 0x585c1b54 )
	ROM_LOAD( "windj_c4.rom", 0x320000, 0x100000, 0x30d052d4 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( janshin_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "jans-p1.rom", 0x000000, 0x100000, 0x4adbd11f )

	ROM_REGION(0x420000)
	ROM_LOAD( "jans-s1.rom", 0x000000, 0x020000, 0x71af3c35 )
	ROM_LOAD( "jans-c1.rom", 0x020000, 0x200000, 0x96d96aef )
	ROM_LOAD( "jans-c2.rom", 0x220000, 0x200000, 0x1c13a67d )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( pbobble_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "puzzb_p1.rom", 0x000000, 0x080000, 0x6164b68a )

	ROM_REGION(0x520000)
	ROM_LOAD( "puzzb_s1.rom", 0x000000, 0x020000, 0x5fceacda )
	ROM_LOAD( "puzzb_c1.rom", 0x020000, 0x100000, 0xf5f24364 ) /* Plane 0,1 */
	ROM_LOAD( "puzzb_c3.rom", 0x120000, 0x100000, 0x6156f272 ) /* Plane 0,1 */
	ROM_LOAD( "puzzb_c5.rom", 0x220000, 0x080000, 0xf92e0064 ) /* Plane 0,1 */
	ROM_LOAD( "puzzb_c2.rom", 0x2A0000, 0x100000, 0xc53462d4 ) /* Plane 2,3 */
	ROM_LOAD( "puzzb_c4.rom", 0x3A0000, 0x100000, 0xd29c5824 ) /* Plane 2,3 */
	ROM_LOAD( "puzzb_c6.rom", 0x4A0000, 0x080000, 0x96fa725c ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( pspikes2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "spike_p1.rom", 0x000000, 0x100000, 0xc6e92385 )

	ROM_REGION(0x620000)
	ROM_LOAD( "spike_s1.rom", 0x000000, 0x020000, 0x0 )
	ROM_LOAD( "spike_c1.rom", 0x020000, 0x100000, 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "spike_c3.rom", 0x120000, 0x100000, 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "spike_c5.rom", 0x220000, 0x100000, 0x0 ) /* Plane 0,1 */
	ROM_LOAD( "spike_c2.rom", 0x320000, 0x100000, 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "spike_c4.rom", 0x420000, 0x100000, 0x0 ) /* Plane 2,3 */
	ROM_LOAD( "spike_c6.rom", 0x520000, 0x100000, 0x0 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( sidkicks_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "sidek_p1.rom", 0x000000, 0x080000, 0xc6e92385 )

	ROM_REGION(0x620000)	  /* 6 meg temporary */
	ROM_LOAD( "sidek_s1.rom", 0x000000, 0x020000, 0xb0eaf1f4 )
	ROM_LOAD( "sidek_c1.rom", 0x020000, 0x100000, 0xd8859477 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x100000 )
	ROM_LOAD( "sidek_c2.rom", 0x320000, 0x100000, 0x1ce63e80 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x520000, 0x100000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( viewpoin_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "viewp_p1.rom", 0x000000, 0x100000, 0xe7cbd8e3 )

	ROM_REGION(0x620000)
	ROM_LOAD( "viewp_s1.rom", 0x000000, 0x020000, 0x07f40dac )
	ROM_LOAD( "viewp_c1.rom", 0x020000, 0x100000, 0xea215125 )
	ROM_CONTINUE(             0x220000, 0x100000 )
	ROM_LOAD( "viewp_c2.rom", 0x320000, 0x100000, 0x58c80c72 )
	ROM_CONTINUE(             0x520000, 0x100000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( kotm2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "kotm2_p1.rom", 0x000000, 0x080000, 0x9a6d6e25 )
	ROM_LOAD_WIDE_SWAP( "kotm2_p2.rom", 0x080000, 0x080000, 0x8c041444 )

	ROM_REGION(0x820000)
	ROM_LOAD( "kotm2_s1.rom", 0x000000, 0x020000, 0x6e9f0d79 )
	ROM_LOAD( "kotm2_c1.rom", 0x020000, 0x100000, 0x81eafb70 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x220000, 0x100000 )
	ROM_LOAD( "kotm2_c3.rom", 0x120000, 0x100000, 0x2e0f570f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x320000, 0x100000 )

	ROM_LOAD( "kotm2_c2.rom", 0x420000, 0x100000, 0xbedc7a74 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x620000, 0x100000 )
	ROM_LOAD( "kotm2_c4.rom", 0x520000, 0x100000, 0x8eed9d31 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x720000, 0x100000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( tophuntr_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "thunt_p1.rom", 0x000000, 0x100000, 0x23d5a4d5 )
	ROM_LOAD_WIDE_SWAP( "thunt_p2.rom", 0x100000, 0x100000, 0x47c92f61 )

	ROM_REGION(0x820000)
	ROM_LOAD( "thunt_s1.rom", 0x000000, 0x020000, 0xf530a374 )
	ROM_LOAD( "thunt_c1.rom", 0x020000, 0x100000, 0xc900201a ) /* Plane 0,1 */
	ROM_LOAD( "thunt_c3.rom", 0x120000, 0x100000, 0xeac31af5 ) /* Plane 0,1 */
	ROM_LOAD( "thunt_c5.rom", 0x220000, 0x100000, 0x5f1ecae2 ) /* Plane 0,1 */
	ROM_LOAD( "thunt_c7.rom", 0x320000, 0x100000, 0x751dd773 ) /* Plane 0,1 */

	ROM_LOAD( "thunt_c2.rom", 0x420000, 0x100000, 0x81f70bf1 ) /* Plane 2,3 */
	ROM_LOAD( "thunt_c4.rom", 0x520000, 0x100000, 0x3a5132e3 ) /* Plane 2,3 */
	ROM_LOAD( "thunt_c6.rom", 0x620000, 0x100000, 0x3107d67f ) /* Plane 2,3 */
	ROM_LOAD( "thunt_c8.rom", 0x720000, 0x100000, 0x05ab104f ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( samsho_rom )
	ROM_REGION(0x180000)
	ROM_LOAD_WIDE_SWAP( "samsh_p1.rom", 0x000000, 0x080000, 0x12f72ba1 )
	ROM_LOAD_WIDE_SWAP( "samsh_p2.rom", 0x080000, 0x080000, 0x0d9ff367 )
	ROM_LOAD_WIDE_SWAP( "samsh_p3.rom", 0x100000, 0x080000, 0x711106ff )

	ROM_REGION(0x920000)
	ROM_LOAD( "samsh_s1.rom", 0x000000, 0x020000, 0xbdd0e2e8 )
	ROM_LOAD( "samsh_c1.rom", 0x020000, 0x200000, 0x65842ce8 ) /* Plane 0,1 */
	ROM_LOAD( "samsh_c3.rom", 0x220000, 0x200000, 0x40551c71 ) /* Plane 0,1 */
	ROM_LOAD( "samsh_c5.rom", 0x420000, 0x080000, 0x55ec08c2 ) /* Plane 0,1 */
	ROM_LOAD( "samsh_c2.rom", 0x4a0000, 0x200000, 0xa7b6085a ) /* Plane 2,3 */
	ROM_LOAD( "samsh_c4.rom", 0x6a0000, 0x200000, 0x898f635f ) /* Plane 2,3 */
	ROM_LOAD( "samsh_c6.rom", 0x8a0000, 0x080000, 0xbc249804 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( karnov_r_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "karev_p1.rom", 0x000000, 0x100000, 0x884db4a9 )

	ROM_REGION(0xc20000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "karev_s1.rom", 0x000000, 0x020000, 0x894a6b44 )

	ROM_LOAD( "karev_c1.rom", 0x020000, 0x200000, 0x3970e842 ) /* Plane 0,1 */
	ROM_LOAD( "karev_c3.rom", 0x220000, 0x200000, 0x3dd5504f ) /* Plane 0,1 */
	ROM_LOAD( "karev_c5.rom", 0x420000, 0x200000, 0xbd814fcb ) /* Plane 0,1 */

	ROM_LOAD( "karev_c2.rom", 0x620000, 0x200000, 0x49d5ebb1 ) /* Plane 2,3 */
	ROM_LOAD( "karev_c4.rom", 0x820000, 0x200000, 0x46e1e381 ) /* Plane 2,3 */
	ROM_LOAD( "karev_c6.rom", 0xa20000, 0x200000, 0x0f8f71b3 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( goalx3_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "goal!_p1.rom", 0x100000, 0x100000, 0xba42e8e4 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	ROM_REGION(0x1020000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "goal!_s1.rom", 0x000000, 0x020000, 0x4e9973c1 )

	ROM_LOAD( "goal!_c1.rom", 0x220000, 0x200000, 0x1dd3a7f1 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x020000, 0x200000 )
	ROM_LOAD( "goal!_c3.rom", 0x420000, 0x400000, 0x36be0000 ) /* Plane 0,1 */

	ROM_LOAD( "goal!_c2.rom", 0xa20000, 0x200000, 0xefb77413 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x820000, 0x200000 )
	ROM_LOAD( "goal!_c4.rom", 0xc20000, 0x400000, 0x2acc0000 ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( samsho2_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "sams2_p1.rom", 0x100000, 0x100000, 0x14a75045 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	ROM_REGION(0x1020000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sams2_s1.rom", 0x000000, 0x020000, 0xe65d89ed )

	ROM_LOAD( "sams2_c1.rom", 0x020000, 0x200000, 0x03c0b0a4 ) /* Plane 0,1 */
	ROM_LOAD( "sams2_c3.rom", 0x220000, 0x200000, 0x5e9cb8e2 ) /* Plane 0,1 */
	ROM_LOAD( "sams2_c5.rom", 0x420000, 0x200000, 0xa2e24856 ) /* Plane 0,1 */
	ROM_LOAD( "sams2_c7.rom", 0x620000, 0x200000, 0x13d59905 ) /* Plane 0,1 */

	ROM_LOAD( "sams2_c2.rom", 0x820000, 0x200000, 0x22776613 ) /* Plane 2,3 */
	ROM_LOAD( "sams2_c4.rom", 0xa20000, 0x200000, 0xce073a07 ) /* Plane 2,3 */
	ROM_LOAD( "sams2_c6.rom", 0xc20000, 0x200000, 0xf25e5b9e ) /* Plane 2,3 */
	ROM_LOAD( "sams2_c8.rom", 0xe20000, 0x200000, 0xa3bd190d ) /* Plane 2,3 */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( mslug_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "mslug_p1.rom",  0x100000, 0x100000, 0xf09edec2 )
	ROM_CONTINUE(                        0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	ROM_REGION(0x1020000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mslug_s1.rom", 0x000000, 0x020000, 0x075a18c4 )
	ROM_LOAD( "mslug_c1.rom", 0x220000, 0x200000, 0xeaf875c0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x020000, 0x200000 )
	ROM_LOAD( "mslug_c3.rom", 0x620000, 0x200000, 0x30104ebe ) /* Plane 0,1 */
	ROM_CONTINUE(             0x420000, 0x200000 )

	ROM_LOAD( "mslug_c2.rom", 0xa20000, 0x200000, 0xd1d1d749 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x820000, 0x200000 )
	ROM_LOAD( "mslug_c4.rom", 0xe20000, 0x200000, 0xe3530fcd ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc20000, 0x200000 )

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

ROM_START( wcup_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "wcupp.1",  0x000000, 0x200000, 0xa11991e9 )

	ROM_REGION(0xc20000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "wcups.1", 0x000000, 0x10000, 0xc6d559c9 ) /* check */

      /*  ROM_LOAD( "wcupch.2.rom", 0x020000, 0x200000, 0x96bd6edf ) */ /* Plane 0,1 */
      /*  ROM_LOAD( "mslug_c3.rom", 0x420000, 0x200000, 0x522baaf3 ) */ /* Plane 0,1 */

    /*    ROM_LOAD( "mslug_c2.rom", 0x420000, 0x200000, 0x49d5ebb1 ) */ /* Plane 2,3 */
    /*    ROM_LOAD( "mslug_c4.rom", 0xc20000, 0x200000, 0x46e1e381 ) */ /* Plane 2,3 */

/* m1 & v1 unused at moment */

	ROM_REGION(0x20000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0xf6c12685 )
ROM_END

/* dummy entry for the dummy bios driver */
ROM_START( bios_rom )
	ROM_REGION(0x020000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom",   0x000000, 0x020000, 0xf6c12685 )
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

NEODRIVER(joyjoy,"Joy Joy Kid/Puzzled","1990","SNK",,&neogeo_0x2000_machine_driver)
NEODRIVER(ridhero,"Riding Hero","1990","SNK",,&neogeo_0x4000_machine_driver)
NEODRIVERX(ttbb,"2020bb","2020 Baseball","1991","SNK",,&neogeo_0x4000_machine_driver)
NEODRIVER(lresort,"Last Resort","1992","SNK",,&neogeo_0x6000_machine_driver)
NEODRIVER(fbfrenzy,"Football Frenzy","19??","SNK",,&neogeo_0x6000_machine_driver)
NEODRIVER(alpham2,"Alpha Mission 2","1991","SNK",,&neogeo_0x6000_machine_driver)
NEODRIVER(eightman,"Eight Man","1991","SNK",,&neogeo_0x6000_machine_driver)
NEODRIVER(ararmy,"Robo Army","19??","SNK",,&neogeo_0x6000_machine_driver)
NEODRIVER(ncombat,"Ninja Combat","1990","Alpha Denshi Co",,&neogeo_0x6000_machine_driver)
NEODRIVER(socbrawl,"Soccer Brawl","19??","??",,&neogeo_0x6000_machine_driver)
NEODRIVER(bstars,"Baseball Stars","19??","????",,&neogeo_0x6000_machine_driver)
NEODRIVER(fatfury1,"Fatal Fury - King of Fighters","1991","SNK",,&neogeo_0x8000_machine_driver)
NEODRIVER(mutnat,"Mutation Nation","1992","SNK",,&neogeo_0x8000_machine_driver)
NEODRIVER(burningf,"Burning Fight","1991","SNK",,&neogeo_0x8000_machine_driver)
NEODRIVER(crsword,"Crossed Swords","1991","Alpha Denshi Co",,&neogeo_0x8000_machine_driver)
NEODRIVER(cyberlip,"Cyber Lip","1990","SNK",,&neogeo_0x8000_machine_driver)
NEODRIVER(kingofm,"King of the Monsters","19??","SNK",,&neogeo_0x8000_machine_driver)
NEODRIVER(gpilots,"Ghost Pilots","1991","SNK",,&neogeo_0x8000_machine_driver)
NEODRIVER(trally,"Thrash Rally","1991","Alpha Denshi Co",,&neogeo_0x8000_machine_driver)
NEODRIVER(lbowling,"League Bowling","1990","SNK",,&neogeo_0x8000_machine_driver)
NEODRIVER(ncommand,"Ninja Command","1992","Alpha Denshi Co",,&neogeo_0x8000_machine_driver)
NEODRIVER(sengoku,"Sengoku","19??","????",,&neogeo_0x8000_machine_driver)
NEODRIVERX(countb,"3countb","3 Count Bout","1993","SNK",,&neogeo_0x10000_machine_driver)
NEODRIVER(androdun,"Andro Dunos","1992","Visco",,&neogeo_0x10000_machine_driver)
NEODRIVER(artfight,"Art of Fighting","1992","SNK",,&neogeo_0x10000_machine_driver)
NEODRIVER(superspy,"Super Spy","1990","SNK",,&neogeo_0x10000_machine_driver)
NEODRIVER(wheroes,"World Heroes","1992","Alpha Denshi Co",,&neogeo_0x10000_machine_driver)
NEODRIVER(sengoku2,"Sengoku 2","19??","Alpha Denshi Co",,&neogeo_0x10000_machine_driver)

NEODRIVER(puzzledp,"Puzzle de Pon","1995","Taito (Visco license)",,&neogeo_0x4000_rl_machine_driver)
NEODRIVER(bjourney,"Blues Journey","1990","Alpha Denshi Co","TJ Grant",&neogeo_0x6000_rl_machine_driver)
NEODRIVER(nam_1975,"NAM 1975","1990","SNK",,&neogeo_0x6000_rl_machine_driver)
NEODRIVER(maglord,"Magician Lord","1990","Alpha Denshi Co",,&neogeo_0x6000_rl_machine_driver)
NEODRIVER(cybrlipa,"Cyber Lip (alt)","1990","SNK",,&neogeo_0x6000_rl_machine_driver)
NEODRIVER(tpgolf,"Top Player's Golf","1990","SNK",,&neogeo_0x6000_rl_machine_driver)
NEODRIVER(wjammers,"Wind Jammers","1994","Data East","TJ Grant",&neogeo_0x8000_rl_machine_driver)
NEODRIVER(janshin,"Quest of Jongmaster","1994","Aicom",,&neogeo_0x8000_rl_machine_driver)
NEODRIVER(pbobble,"Puzzle Bobble","1994","Taito",,&neogeo_0xa000_rl_machine_driver)
NEODRIVER(sidkicks,"Super Sidekicks","1992","SNK",,&neogeo_0xc000_rl_machine_driver)
NEODRIVER(pspikes2,"Power Spikes 2","1994","Video System Co",,&neogeo_0xc000_rl_machine_driver)
NEODRIVER(viewpoin,"Viewpoint","19??","??",,&neogeo_0xc000_rl_machine_driver)
NEODRIVER(kotm2,"King of the Monsters 2","1992","SNK",,&neogeo_0x10000_rl_machine_driver)
NEODRIVER(tophuntr,"Top Hunter","19??","???",,&neogeo_0x10000_rl_machine_driver)
NEODRIVER(samsho,"Samurai Shodown","1992","SNK",,&neogeo_0x12000_rl_machine_driver)
NEODRIVER(karnov_r,"Karnov's Revenge","1994","Data East",,&neogeo_0x18000_rl_machine_driver)
NEODRIVER(mslug,"Metal Slug","19??","??",,&neogeo_0x20000_rl_machine_driver)
NEODRIVER(goalx3,"Goal! Goal! Goal!","19??","??",,&neogeo_0x20000_rl_machine_driver)
NEODRIVER(samsho2,"Samurai Shodown 2","19??","??",,&neogeo_0x20000_rl_machine_driver)

/* preliminary drivers */
NEODRIVER(wcup,"Thrash Rally","1991","Alpha Denshi Co",,&neogeo_0x8000_machine_driver)
