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

	0x300081                Unknown IO ?

	0x300000                controller 1
				bit 0 : joy up
				bit 1 : joy down
				bit 2 : joy left
				bit 3 : joy right
				bit 4 : button A
				bit 5 : button B
				bit 6 : button C
				bit 7 : button D

	0x3000001               Dipswitches
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

	0x380000                bit 0 : Start  1
				bit 1 : Select 1
				bit 2 : Start  2
				bit 3 : Select 2
				bit 4 - 7 unknown

	IO byte write

	0x320000		Byte written to sound CPU

	0x380001                Unknown
	0x380031                Unknown
	0x380041                Test switch (see hardware test) + ??

	0x380051                4990 control write register
				bit 0: C0
				bit 1: C1
				bit 2: C2
				bit 3-7: unused.

				0x02 = shift.
				0x00 = register hold.
				0x04 = ????.
				0x03 = time read (reset register).

	0x3c000c                IRQ acknowledge

	0x380011                Backup bank select

	0x3a0001                Enable display.
	0x3a0011                Disable display

	0x3a0003                Swap in Bios (0x80 bytes vector table of BIOS)
	0x3a0013                Swap in Rom  (0x80 bytes vector table of ROM bank)

	0x3a000f                Select Color Bank 2
	0x3a001f                Select Color Bank 1

	0x300001                Watchdog (?)

	0x3a000d                lock backup ram
	0x3a001d                unlock backup ram

	0x3a000b                set game vector table (?)  mirror ?
	0x3a001b                set bios vector table (?)  mirror ?

	0x3a000c		Unknown	(ghost pilots)
	0x31001c		Unknown (ghost pilots)

	IO word read

	0x3c0002                return vidram word (pointed to by 0x3c0000)
	0x3c0006                ?????.
	0x3c0008		shadow adress for 0x3c0000 (not confirmed).
	0x3c000a		shadow adress for 0x3c0002 (confirmed, see
							   Puzzle de Pon).
	IO word write

	0x3c0006                Unknown, set vblank counter (?)

	0x3c0008                shadow address for 0x3c0000	(not confirmed)
	0x3c000a                shadow address for 0x3c0002	(not confirmed)

	0x3c0000                vidram pointer
	0x3c0002                vidram data write
	0x3c0004                modulo add write

	The Neo Geo contains an NEC 4990 Serial I/O calendar & clock.
	accesed through 0x320001, 0x380050, 0x280050 (shadow adress).
	A schematic for this device can be found on the NEC webpages.

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/pd4990a.h"

extern unsigned char *vidram;

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
void neogeo_init_machine(void);

/* debug defined in vidhrdw/neogeo.c */
extern int dotiles;
extern int screen_offs;
/* end debug */

int irq_acknowledge;
int vblankcount=0;

int neogeo_interrupt(void) {
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* debugging */

	/* hard reset, connected to F5 */
	if (readinputport(6) & 0x10) {
		memset( &RAM[0x100000],0,0x10000);
		machine_reset();
	}

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

int read_softdip (int offset) {
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	int result=READ_WORD(&RAM[0x10FD82]);
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
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* if (errorlog) fprintf(errorlog,"CPU %04x - Read cart number\n",cpu_getpc()); */

	if (cpu_getpc()==0xc13154 || cpu_getpc()==0xc13178 || cpu_getpc()==0xc1319c) return READ_WORD(&RAM[0x10Fce6]);

	return 1<<8;
}

extern int rand_ret(int offset);

static struct MemoryReadAddress neogeo_readmem[] =
{
	{ 0x000000, 0x0FFFFF, MRA_ROM },	/* Rom bank 1 */
	{ 0x10FD82, 0x10FD83, read_softdip },
	{ 0x10Fce6, 0x10Fce7, read_cart_info },
	{ 0x100000, 0x10ffff, MRA_RAM },
	{ 0x200000, 0x2fffff, MRA_BANK4 }, 	/* Rom bank 2 */

	{ 0x300000, 0x300001, controller1_r },
	{ 0x300080, 0x300081, controller4_r }, /* Test switch in here */

	{ 0x320000, 0x320001, timer_r },
	{ 0x340000, 0x340001, controller2_r },
	{ 0x380000, 0x380001, controller3_r },
	{ 0x3C0000, 0x3C0001, vidram_offset_r },
	{ 0x3C0002, 0x3C0003, vidram_data_r },
	{ 0x3C0004, 0x3C0005, vidram_modulo_r },

/*        { 0x3C0006, 0x3C0007, vblankc_r },	*/

	{ 0x3c000a, 0x3c000b, vidram_data_r },	/* Puzzle de Pon */

	{ 0x400000, 0x401fff, neogeo_paletteram_r },

   	{ 0x600000, 0x610bff, mish_vid_r },

  /*	{ 0x800000, 0x800FFF, MRA_BANK2 },  */    /* memory card */
	{ 0xC00000, 0xC1FFFF, MRA_BANK1 },        /* system rom */
	{ 0xD00000, 0xD0FFFF, MRA_BANK3 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress neogeo_writemem[] =
{
	{ 0x000000, 0x0FFFFF, MWA_ROM },    /* ghost pilots writes to ROM */
	{ 0x100000, 0x10FFFF, MWA_RAM },

	{ 0x300000, 0x300001, MWA_NOP },        /* Watchdog */


	{ 0x320000, 0x320001, MWA_NOP }, /* Sound */
  /*	{ 0x380030, 0x380031, neo_unknown2 }, */

	{ 0x380000, 0x380001, MWA_NOP },

 	{ 0x380030, 0x380031, neo_unknown3 },  /* No idea */
	{ 0x380040, 0x380041, MWA_NOP },  /* Either 1 or both of output leds */
	{ 0x380050, 0x380051, neo_unknown4 },

/*	{ 0x3a000e, 0x3a000f,  }, */

/*    { 0x3a0000, 0x3a0001, neo_unknown1 }, */
/*    { 0x3a0010, 0x3a0011, neo_unknown2 }, */

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

 /*	{ 0x800000, 0x800FFF, MWA_BANK2 }, */        /* mem card */
 /*	{ 0xC00000, 0xC1FFFF, MWA_BANK1 }, */	     /* system ROM */
	{ 0xD00000, 0xD0FFFF, MWA_BANK3 },
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
static struct GfxLayout charlayout = {	/* All games */
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
/* Joy Joy Kid */
static struct GfxLayout joyjoy_tilelayout = {
	16,16,  /* 16*16 sprites */
	8192,   /* sprites */
	4,      /* 4 bits per pixel */
	{ 8192*32*8*3, 8192*32*8*2, 8192*32*8, 0 },     /* plane offset */
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo joyjoy_gfxdecodeinfo[] = {
	{ 1, 0x00000, &charlayout, 0, 256 },
	{ 1, 0x20000, &joyjoy_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/* Ghost Pilots also used for :
   fatal fury 1,
   mutation nation,
   burning Fight,
   crossed sword,
   cyberlip,
   King of Monsters 1 */
static struct GfxLayout ghost_pilots_tilelayout = { /* Ghost Pilots */
	16,16,  /* 16*16 sprites */
	32768,  /* sprites */
	4,      /* 4 bits per pixel */
	{ 32768*32*8*3, 32768*32*8*2, 32768*32*8*1, 0 },     /* plane offset */
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo ghost_pilots_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &ghost_pilots_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/* Last Resort also used for:
   eightman,
   alpha mission 2,
   football frenzy */
static struct GfxLayout lresort_tilelayout = {
	16,16,  /* 16*16 sprites */
	3*8192,  /* sprites */
	4,      /* 4 bits per pixel */
	{ 3*8192*32*8*3, 3*8192*32*8*2, 3*8192*32*8, 0 },     /* plane offset */
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo lresort_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &lresort_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/*
 * 3 Count Bout also used for Super Spy also used for art of fighting
 * and andro dunos. Note that some of the roms of this one are empty.
 */
static struct GfxLayout countb_tilelayout = {
	16,16,  /* 16*16 sprites */
	65535,  /* sprites */			/* missing 1 tile */
	4,      /* 4 bits per pixel */
	{ 65536*32*8*3, 65536*32*8*2, 65536*32*8, 0 },     /* plane offset */
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo countb_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &countb_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/* Riding Hero, also used for 2020 bb */
static struct GfxLayout ridhero_tilelayout = {
	16,16,  /* 16*16 sprites */
	16384,  /* sprites */
	4,      /* 4 bits per pixel */
	{ 16384*32*8*3, 16384*32*8*2, 16384*32*8, 0 },     /* plane offset */
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo ridhero_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &ridhero_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* ROM list dumps, different format 		  */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

/* NAM 1975 also used for magician lord */
static struct GfxLayout nam_1975_tilelayout = {
	16,16,  /* 16*16 sprites */
	24576,   /* 3*8192 sprites */
	4,      /* 4 bits per pixel */
	{ 24576*64*8 + 8, 24576*64*8, 8, 0 },     /* plane offset */
	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo nam_1975_gfxdecodeinfo[] = {
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &nam_1975_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/* Puzzle de Pon */
static struct GfxLayout pdpon_tilelayout = {
	16,16,   /* 16*16 sprites */
	16384,   /* 16384 sprites */
	4,       /* 4 bits per pixel */
	{ 16384*64*8 + 8, 16384*64*8, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },

	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo pdpon_gfxdecodeinfo[] = {
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &pdpon_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/* Super Sidekicks */
static struct GfxLayout skicks_tilelayout = {
	16,16,   /* 16*16 sprites */
	49152,   /* 49152 sprites of which 16384 are not used */
	4,       /* 4 bits per pixel */
	{ 49152*64*8 + 8, 49152*64*8, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },

	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo skicks_gfxdecodeinfo[] = {
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &skicks_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/* Puzzle Bobble */
static struct GfxLayout pbobble_tilelayout = {
	16,16,   /* 16*16 sprites */
	40960,   /* 49152-8192 sprites of which the first 32768 are not used */
	4,       /* 4 bits per pixel */
	{ 40960*64*8 + 8, 40960*64*8, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },

	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo pbobble_gfxdecodeinfo[] = {
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &pbobble_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

static struct GfxLayout karnov_r_tilelayout = {
	16,16,   /* 16*16 sprites */
	0xc000,   /* Sh*t, thats a lotta sprites */
	4,       /* 4 bits per pixel */
	{ 0x600000*8 + 8, 0x600000*8 + 0, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout karnov_2_tilelayout = {
	16,16,   /* 16*16 sprites */
	0xc000,   /* Sh*t, thats a lotta sprites */
	4,       /* 4 bits per pixel */
	{ 0x600000*8 + 8, 0x600000*8 + 0, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo karnov_r_gfxdecodeinfo[] = {
	{ 1, 0x000000, &charlayout, 0, 256 },
	{ 1, 0x020000, &karnov_r_tilelayout, 0, 256 },
/*      { 1, 0x320000, &karnov_2_tilelayout, 0, 256 }, */
	{ -1 } /* end of array */
};

static struct GfxLayout mslug_tilelayout = {
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

static struct GfxDecodeInfo mslug_gfxdecodeinfo[] = {
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

NEOMACHINE(joyjoy_machine_driver,joyjoy_gfxdecodeinfo)
NEOMACHINE(ghost_pilots_machine_driver,ghost_pilots_gfxdecodeinfo)
NEOMACHINE(lresort_machine_driver,lresort_gfxdecodeinfo)
NEOMACHINE(countb_machine_driver,countb_gfxdecodeinfo)
NEOMACHINE(ridhero_machine_driver,ridhero_gfxdecodeinfo)
NEOMACHINE(nam_1975_machine_driver,nam_1975_gfxdecodeinfo)
NEOMACHINE(pdpon_machine_driver,pdpon_gfxdecodeinfo)
NEOMACHINE(pbobble_machine_driver,pbobble_gfxdecodeinfo)
NEOMACHINE(skicks_machine_driver,skicks_gfxdecodeinfo)
NEOMACHINE(karnov_r_machine_driver,karnov_r_gfxdecodeinfo)
NEOMACHINE(mslug_machine_driver,mslug_gfxdecodeinfo)

/* DEBUG PURPOSES ONLY, delete this later */
static struct GfxDecodeInfo dummy_gfxdecodeinfo[] = {
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

	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
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
	ROM_REGION(0x120000)

	/* load it on the odd position */
	ROM_LOAD_ODD ("N022001a.038", 0x000000, 0x040000, 0xaca68aee)
	/* load it on the even position */
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	/* Bios */
	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x120000)    /* temporary space for graphics (disposed after conversion) */
	/* charset rom */
	ROM_LOAD( "N022001a.378", 0x00000, 0x20000, 0x64f64ddc )
	/* Tiles roms */
	ROM_LOAD( "N022001a.538", 0x020000, 0x80000, 0x91acf534 ) /* Plane 0,1 */
	ROM_LOAD( "N022001a.638", 0x0A0000, 0x80000, 0xd2f574c5 ) /* Plane 2,3 */
ROM_END

ROM_START( ghost_pilots_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N058001a.038", 0x000000, 0x040000, 0x5ca22fe6 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N058001a.03c", 0x080000, 0x040000, 0x6c640000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N058001a.378", 0x00000, 0x20000, 0x6118ea2a )

	ROM_LOAD( "N058001a.538", 0x020000, 0x40000, 0x022dd51f ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N058001a.53c", 0x060000, 0x40000, 0x2fce4578 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )
	ROM_LOAD( "N058001b.538", 0x0A0000, 0x40000, 0xcf579505 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1A0000, 0x40000 )
	ROM_LOAD( "N058001b.53c", 0x0E0000, 0x40000, 0x39227a48 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1E0000, 0x40000 )

	ROM_LOAD( "N058001a.638", 0x220000, 0x40000, 0xe6fe9e6e ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N058001a.63c", 0x260000, 0x40000, 0xead70b71 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N058001b.638", 0x2A0000, 0x40000, 0xc9d8ad90 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N058001b.63c", 0x2E0000, 0x40000, 0x2971acff ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )
ROM_END

ROM_START( crsword_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x6e5ca756 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x00000, 0x20000, 0xc6de9456 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0xab929c8e ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x6140cdc2 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0x4d93fc9b ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1A0000, 0x40000 )
	ROM_LOAD( "N046001b.53c", 0x0E0000, 0x40000, 0x6856e324 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1E0000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x220000, 0x40000, 0x24104162 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x260000, 0x40000, 0xa1976df7 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x2A0000, 0x40000, 0x12f85b88 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N046001b.63c", 0x2E0000, 0x40000, 0x432aaf20 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )
ROM_END

ROM_START( cyberlip_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N062001a.038", 0x000000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N062001a.03c", 0x080000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N062001a.378", 0x00000, 0x20000, 0x0 )

	ROM_LOAD( "N062001a.538", 0x020000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N062001a.53c", 0x060000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )
	ROM_LOAD( "N062001b.538", 0x0A0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1A0000, 0x40000 )
	ROM_LOAD( "N062001b.53c", 0x0E0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1E0000, 0x40000 )

	ROM_LOAD( "N062001a.638", 0x220000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N062001a.63c", 0x260000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N062001b.638", 0x2A0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N062001b.63c", 0x2E0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )
ROM_END

ROM_START( kingofm_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N058001a.038", 0x000000, 0x040000, 0xf753af95 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N058001a.03c", 0x080000, 0x040000, 0xfe700000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N058001a.378", 0x00000, 0x20000, 0x6f50f8f6 )

	ROM_LOAD( "N058001a.538", 0x020000, 0x40000, 0x8c31a5c5 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N058001a.53c", 0x060000, 0x40000, 0x17ea64c6 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )
	ROM_LOAD( "N058001b.538", 0x0A0000, 0x40000, 0x0f5b896b ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1A0000, 0x40000 )
	ROM_LOAD( "N058001b.53c", 0x0E0000, 0x40000, 0x108013b8 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1E0000, 0x40000 )

	ROM_LOAD( "N058001a.638", 0x220000, 0x40000, 0x7a8349d3 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N058001a.63c", 0x260000, 0x40000, 0xcedf3485 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N058001b.638", 0x2A0000, 0x40000, 0x752f97f9 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N058001b.63c", 0x2E0000, 0x40000, 0xb7adb0b9 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )
ROM_END

ROM_START( bfight_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N054001a.038", 0x000000, 0x040000, 0xb0e1f177 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N054001a.378", 0x00000, 0x20000, 0x58bb9619 )

	ROM_LOAD( "N054001a.538", 0x020000, 0x40000, 0xc6004c86 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N054001a.53c", 0x060000, 0x40000, 0x3989eb5f ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )
	ROM_LOAD( "N054001b.538", 0x0A0000, 0x40000, 0x42134faf ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1A0000, 0x40000 )
	ROM_LOAD( "N054001b.53c", 0x0E0000, 0x40000, 0x39f78509 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1E0000, 0x40000 )

	ROM_LOAD( "N054001a.638", 0x220000, 0x40000, 0x553288e0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N054001a.63c", 0x260000, 0x40000, 0x3566a06e ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N054001b.638", 0x2A0000, 0x40000, 0x8ac2d988 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N054001b.63c", 0x2E0000, 0x40000, 0x473c8f34 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )
ROM_END

ROM_START( mutnat_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N054001a.038", 0x000000, 0x040000, 0x925138ad )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N054001a.378", 0x00000, 0x20000, 0xecf8b234 )

	ROM_LOAD( "N054001a.538", 0x020000, 0x40000, 0x3fe926f3 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N054001a.53c", 0x060000, 0x40000, 0x437ccb1a ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )
	ROM_LOAD( "N054001b.538", 0x0A0000, 0x40000, 0x35e14ec5 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1A0000, 0x40000 )
	ROM_LOAD( "N054001b.53c", 0x0E0000, 0x40000, 0x48563fa0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1E0000, 0x40000 )

	ROM_LOAD( "N054001a.638", 0x220000, 0x40000, 0xb831cfcb ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N054001a.63c", 0x260000, 0x40000, 0x0ac511df ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N054001b.638", 0x2A0000, 0x40000, 0x036edf00 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N054001b.63c", 0x2E0000, 0x40000, 0xeae9166d ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )
ROM_END

ROM_START( lresort_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x954d9aeb )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x00000, 0x20000, 0x871a3e12 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x84229a28 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x0E0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x349873c4 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0x77952209 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x1A0000, 0x40000, 0x43b89106 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x260000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x1E0000, 0x40000, 0x60018cf3 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x2A0000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x220000, 0x40000, 0x6865777d ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x2E0000, 0x40000 )
ROM_END

ROM_START( fbfrenzy_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x8bfe2f3c )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x00000, 0x20000, 0xc6d79eed )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x936791f9 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x0E0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x9df5a633 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0xadf1b389 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x1A0000, 0x40000, 0x49f0e056 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x260000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x1E0000, 0x40000, 0xf054eaba ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x2A0000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x220000, 0x40000, 0x92fad81a ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x2E0000, 0x40000 )
ROM_END

ROM_START( alpham2_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N082001a.038", 0x000000, 0x040000, 0x9a61c8c9 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N082001a.03C", 0x080000, 0x040000, 0xfac80000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N082001a.378", 0x00000, 0x20000, 0x1e767974 )

	ROM_LOAD( "N082001a.538", 0x020000, 0x40000, 0x92ad2b4d ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x0E0000, 0x40000 )
	ROM_LOAD( "N082001a.53c", 0x060000, 0x40000, 0x00be08f6 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N082001b.538", 0x0A0000, 0x40000, 0xd238188e ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )

	ROM_LOAD( "N082001a.638", 0x1A0000, 0x40000, 0x7b84a706 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x260000, 0x40000 )
	ROM_LOAD( "N082001a.63c", 0x1E0000, 0x40000, 0x5099927f ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x2A0000, 0x40000 )
	ROM_LOAD( "N082001b.638", 0x220000, 0x40000, 0xfcd769fb ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x2E0000, 0x40000 )
ROM_END

ROM_START( eightman_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x3b227218 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x00000, 0x20000, 0x09625510 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x6ecb0b13 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x0E0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0xaa767fda ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0xeb3faa85 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x1A0000, 0x40000, 0xa90186b9 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x260000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x1E0000, 0x40000, 0x2a225c0e ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x2A0000, 0x40000 )
	ROM_LOAD( "N046001b.638", 0x220000, 0x40000, 0x547ea2be ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x2E0000, 0x40000 )
ROM_END

ROM_START( ffury1_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N058001a.038", 0x000000, 0x040000, 0x5a09be4b )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N058001a.03c", 0x080000, 0x040000, 0x34d80000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N058001a.378", 0x00000, 0x20000, 0x3e30d76e )

	ROM_LOAD( "N058001a.538", 0x020000, 0x40000, 0x2382e626 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N058001a.53c", 0x060000, 0x40000, 0x043fc03b ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )
	ROM_LOAD( "N058001b.538", 0x0A0000, 0x40000, 0x528be7c9 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1A0000, 0x40000 )
	ROM_LOAD( "N058001b.53c", 0x0E0000, 0x40000, 0x4b77c26d ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1E0000, 0x40000 )

	ROM_LOAD( "N058001a.638", 0x220000, 0x40000, 0x33251363 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N058001a.63c", 0x260000, 0x40000, 0xad5f0307 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N058001b.638", 0x2A0000, 0x40000, 0xe94d2355 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N058001b.63c", 0x2E0000, 0x40000, 0x09119b45 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )
ROM_END

ROM_START( countb_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N106001a.038", 0x000000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N106001a.03c", 0x080000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N106001a.378", 0x00000, 0x20000, 0x0 )

	ROM_LOAD( "N106001a.538", 0x020000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x220000, 0x40000 )
	ROM_LOAD( "N106001a.53c", 0x060000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x260000, 0x40000 )
	ROM_LOAD( "N106001b.538", 0x0A0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x2A0000, 0x40000 )
	ROM_LOAD( "N106001b.53c", 0x0E0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x2E0000, 0x40000 )

	ROM_LOAD( "N106001c.538", 0x120000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N106001c.53c", 0x160000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N106001d.538", 0x1A0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N106001d.53c", 0x1E0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )

	ROM_LOAD( "N106001a.638", 0x420000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x620000, 0x40000 )
	ROM_LOAD( "N106001a.63c", 0x460000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x660000, 0x40000 )
	ROM_LOAD( "N106001b.638", 0x4A0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x6A0000, 0x40000 )
	ROM_LOAD( "N106001b.63c", 0x4E0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x6E0000, 0x40000 )

	ROM_LOAD( "N106001c.638", 0x520000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x720000, 0x40000 )
	ROM_LOAD( "N106001c.63c", 0x560000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x760000, 0x40000 )
	ROM_LOAD( "N106001d.638", 0x5A0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x7A0000, 0x40000 )
	ROM_LOAD( "N106001d.63c", 0x5E0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x7E0000, 0x40000 )
ROM_END

ROM_START( ridhero_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x220000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x00000, 0x20000, 0x0 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x0A0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x0E0000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x120000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x1A0000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x160000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x1E0000, 0x40000 )
ROM_END

ROM_START( ttbb_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x220000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x00000, 0x20000, 0x0 )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x0A0000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x0E0000, 0x40000 )

	ROM_LOAD( "N046001a.638", 0x120000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x1A0000, 0x40000 )
	ROM_LOAD( "N046001a.63c", 0x160000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x1E0000, 0x40000 )
ROM_END

ROM_START( andro_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N138001a.038", 0x000000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N138001a.03c", 0x080000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N138001a.378", 0x00000, 0x20000, 0x0 )

	ROM_LOAD( "N138001a.538", 0x020000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x220000, 0x40000 )
	ROM_LOAD( "N138001a.53c", 0x060000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x260000, 0x40000 )
	ROM_LOAD( "N138001b.538", 0x0A0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x2A0000, 0x40000 )
	ROM_LOAD( "N138001b.53c", 0x0E0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x2E0000, 0x40000 )

	ROM_LOAD( "N138001c.538", 0x120000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N138001c.53c", 0x160000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N138001d.538", 0x1A0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N138001d.53c", 0x1E0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )

	ROM_LOAD( "N138001a.638", 0x420000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x620000, 0x40000 )
	ROM_LOAD( "N138001a.63c", 0x460000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x660000, 0x40000 )
	ROM_LOAD( "N138001b.638", 0x4A0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x6A0000, 0x40000 )
	ROM_LOAD( "N138001b.63c", 0x4E0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x6E0000, 0x40000 )

	ROM_LOAD( "N138001c.638", 0x520000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x720000, 0x40000 )
	ROM_LOAD( "N138001c.63c", 0x560000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x760000, 0x40000 )
	ROM_LOAD( "N138001d.638", 0x5A0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x7A0000, 0x40000 )
	ROM_LOAD( "N138001d.63c", 0x5E0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x7E0000, 0x40000 )
ROM_END

ROM_START( artfight_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N102001a.038", 0x000000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N102001a.378", 0x00000, 0x20000, 0x0 )

	ROM_LOAD( "N102001a.538", 0x020000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x220000, 0x40000 )
	ROM_LOAD( "N102001a.53c", 0x060000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x260000, 0x40000 )
	ROM_LOAD( "N102001b.538", 0x0A0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x2A0000, 0x40000 )
	ROM_LOAD( "N102001b.53c", 0x0E0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x2E0000, 0x40000 )

	ROM_LOAD( "N102001c.538", 0x120000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N102001c.53c", 0x160000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N102001d.538", 0x1A0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N102001d.53c", 0x1E0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )

	ROM_LOAD( "N102001a.638", 0x420000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x620000, 0x40000 )
	ROM_LOAD( "N102001a.63c", 0x460000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x660000, 0x40000 )
	ROM_LOAD( "N102001b.638", 0x4A0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x6A0000, 0x40000 )
	ROM_LOAD( "N102001b.63c", 0x4E0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x6E0000, 0x40000 )

	ROM_LOAD( "N102001c.638", 0x520000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x720000, 0x40000 )
	ROM_LOAD( "N102001c.63c", 0x560000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x760000, 0x40000 )
	ROM_LOAD( "N102001d.638", 0x5A0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x7A0000, 0x40000 )
	ROM_LOAD( "N102001d.63c", 0x5E0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x7E0000, 0x40000 )
ROM_END

ROM_START( superspy_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N138001a.038", 0x000000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N138001a.03c", 0x080000, 0x040000, 0x0 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	ROM_LOAD_WIDE_SWAP(  "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x820000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N138001a.378", 0x00000, 0x20000, 0x0 )

	ROM_LOAD( "N138001a.538", 0x020000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x220000, 0x40000 )
	ROM_LOAD( "N138001a.53c", 0x060000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x260000, 0x40000 )
	ROM_LOAD( "N138001b.538", 0x0A0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x2A0000, 0x40000 )
	ROM_LOAD( "N138001b.53c", 0x0E0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x2E0000, 0x40000 )

	ROM_LOAD( "N138001c.538", 0x120000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x320000, 0x40000 )
	ROM_LOAD( "N138001c.53c", 0x160000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x360000, 0x40000 )
	ROM_LOAD( "N138001d.538", 0x1A0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x3A0000, 0x40000 )
	ROM_LOAD( "N138001d.53c", 0x1E0000, 0x40000, 0x0 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x3E0000, 0x40000 )

	ROM_LOAD( "N138001a.638", 0x420000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x620000, 0x40000 )
	ROM_LOAD( "N138001a.63c", 0x460000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x660000, 0x40000 )
	ROM_LOAD( "N138001b.638", 0x4A0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x6A0000, 0x40000 )
	ROM_LOAD( "N138001b.63c", 0x4E0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x6E0000, 0x40000 )

	ROM_LOAD( "N138001c.638", 0x520000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x720000, 0x40000 )
	ROM_LOAD( "N138001c.63c", 0x560000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x760000, 0x40000 )
	ROM_LOAD( "N138001d.638", 0x5A0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x7A0000, 0x40000 )
	ROM_LOAD( "N138001d.63c", 0x5E0000, 0x40000, 0x0 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x7E0000, 0x40000 )
ROM_END

/* Rom list roms */
ROM_START( nam_1975_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_WIDE_SWAP( "nam_p1.rom",  0x000000, 0x080000, 0x05c693c6 )
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nam_s1.rom", 0x00000, 0x20000, 0x320481b0 )

	ROM_LOAD( "nam_c1.rom", 0x020000, 0x80000, 0x2406dbf6 ) /* Plane 0,1 */
	ROM_LOAD( "nam_c3.rom", 0x0A0000, 0x80000, 0x28e59777 ) /* Plane 0,1 */
	ROM_LOAD( "nam_c5.rom", 0x120000, 0x80000, 0x63a40fe8 ) /* Plane 0,1 */

	ROM_LOAD( "nam_c2.rom", 0x1A0000, 0x80000, 0xb1f2bfc6 ) /* Plane 2,3 */
	ROM_LOAD( "nam_c4.rom", 0x220000, 0x80000, 0x0949001d ) /* Plane 2,3 */
	ROM_LOAD( "nam_c6.rom", 0x2A0000, 0x80000, 0xfd75150b ) /* Plane 2,3 */
ROM_END

ROM_START( pdpon_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_WIDE_SWAP( "pdpon_p1.rom",  0x000000, 0x080000, 0x9e0d0727 )
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom",   0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x220000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pdpon_s1.rom", 0x000000, 0x020000, 0x6e1c6af2 )
	ROM_LOAD( "pdpon_c1.rom", 0x020000, 0x100000, 0x272b3889 ) /* Plane 0,1 */
	ROM_LOAD( "pdpon_c2.rom", 0x120000, 0x100000, 0xd6a5d681 ) /* Plane 2,3 */
ROM_END

ROM_START( pbobble_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_WIDE_SWAP( "puzzb_p1.rom", 0x000000, 0x080000, 0x6164b68a )
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom",  0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x520000)
	ROM_LOAD( "puzzb_s1.rom", 0x000000, 0x020000, 0x5fceacda )
	ROM_LOAD( "puzzb_c1.rom", 0x020000, 0x100000, 0xf5f24364 ) /* Plane 0,1 */
	ROM_LOAD( "puzzb_c3.rom", 0x120000, 0x100000, 0x6156f272 ) /* Plane 0,1 */
	ROM_LOAD( "puzzb_c5.rom", 0x220000, 0x080000, 0xf92e0064 ) /* Plane 0,1 */
	ROM_LOAD( "puzzb_c2.rom", 0x2A0000, 0x100000, 0xc53462d4 ) /* Plane 2,3 */
	ROM_LOAD( "puzzb_c4.rom", 0x3A0000, 0x100000, 0xd29c5824 ) /* Plane 2,3 */
	ROM_LOAD( "puzzb_c6.rom", 0x4A0000, 0x080000, 0x96fa725c ) /* Plane 2,3 */
ROM_END

ROM_START( skicks_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_WIDE_SWAP( "sidek_p1.rom", 0x000000, 0x080000, 0xc6e92385 )
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom",  0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x620000)	  /* 6 meg temporary */
	ROM_LOAD( "sidek_s1.rom", 0x000000, 0x020000, 0xb0eaf1f4 )
	ROM_LOAD( "sidek_c1.rom", 0x020000, 0x100000, 0xd8859477 ) /* Plane 0,1 */
	ROM_CONTINUE( 		  0x220000, 0x100000      )
	ROM_LOAD( "sidek_c2.rom", 0x320000, 0x100000, 0x1ce63e80 ) /* Plane 2,3 */
	ROM_CONTINUE(		  0x520000, 0x100000	  )
ROM_END

ROM_START( maglord_rom )
        ROM_REGION(0x120000)
        ROM_LOAD_WIDE_SWAP( "magl_p1.rom",  0x000000, 0x080000, 0xa11991e9 )
        ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

        ROM_REGION(0x320000)    /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "magl_s1.rom", 0x000000, 0x20000, 0xc6d559c9 )

        ROM_LOAD( "magl_c1.rom", 0x020000, 0x80000, 0x96bd6edf ) /* Plane 0,1 */
        ROM_LOAD( "magl_c3.rom", 0x0A0000, 0x80000, 0x522baaf3 ) /* Plane 0,1 */
        ROM_LOAD( "magl_c5.rom", 0x120000, 0x80000, 0x88b6e4ee ) /* Plane 0,1 */

        ROM_LOAD( "magl_c2.rom", 0x1A0000, 0x80000, 0x49d5ebb1 ) /* Plane 2,3 */
        ROM_LOAD( "magl_c4.rom", 0x220000, 0x80000, 0x46e1e381 ) /* Plane 2,3 */
        ROM_LOAD( "magl_c6.rom", 0x2A0000, 0x80000, 0x0f8f71b3 ) /* Plane 2,3 */
ROM_END

ROM_START( karnov_r_rom )
        ROM_REGION(0x120000)
        ROM_LOAD_WIDE_SWAP( "karev_p1.rom",  0x000000, 0x100000, 0xa11991e9 )
        ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

        ROM_REGION(0xc20000)    /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "karev_s1.rom", 0x000000, 0x20000, 0xc6d559c9 )

        ROM_LOAD( "karev_c1.rom", 0x020000, 0x200000, 0x96bd6edf ) /* Plane 0,1 */
        ROM_LOAD( "karev_c3.rom", 0x220000, 0x200000, 0x522baaf3 ) /* Plane 0,1 */
        ROM_LOAD( "karev_c5.rom", 0x420000, 0x200000, 0x88b6e4ee ) /* Plane 0,1 */

        ROM_LOAD( "karev_c2.rom", 0x620000, 0x200000, 0x49d5ebb1 ) /* Plane 2,3 */
        ROM_LOAD( "karev_c4.rom", 0x820000, 0x200000, 0x46e1e381 ) /* Plane 2,3 */
        ROM_LOAD( "karev_c6.rom", 0xa20000, 0x200000, 0x0f8f71b3 ) /* Plane 2,3 */

/* m1 & v1 unused at moment */
ROM_END

ROM_START( mslug_rom )
        ROM_REGION(0x220000)
        ROM_LOAD_WIDE_SWAP( "mslug_p1.rom",  0x120000, 0x100000, 0xa11991e9 )
		ROM_CONTINUE(0,0x100000) /* Hmm, probably breaks mac version */
        ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

        ROM_REGION(0xc20000)    /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "mslug_s1.rom", 0x000000, 0x20000, 0xc6d559c9 )

        ROM_LOAD( "mslug_c1.rom", 0x020000, 0x200000, 0x96bd6edf ) /* Plane 0,1 */
     /*   ROM_LOAD( "mslug_c3.rom", 0x420000, 0x200000, 0x522baaf3 ) */ /* Plane 0,1 */

        ROM_LOAD( "mslug_c2.rom", 0x420000, 0x200000, 0x49d5ebb1 ) /* Plane 2,3 */
    /*    ROM_LOAD( "mslug_c4.rom", 0xc20000, 0x200000, 0x46e1e381 ) */ /* Plane 2,3 */
/* m1 & v1 unused at moment */
ROM_END

ROM_START( wcup_rom )
        ROM_REGION(0x220000)
        ROM_LOAD_WIDE_SWAP( "wcupp.1",  0x000000, 0x100000, 0xa11991e9 )
	ROM_CONTINUE	  (0x120000,0x100000) /* Hmm, probably breaks mac version */
        ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

        ROM_REGION(0xc20000)    /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "wcups.1", 0x000000, 0x10000, 0xc6d559c9 ) /* check */

      /*  ROM_LOAD( "wcupch.2.rom", 0x020000, 0x200000, 0x96bd6edf ) */ /* Plane 0,1 */
      /*  ROM_LOAD( "mslug_c3.rom", 0x420000, 0x200000, 0x522baaf3 ) */ /* Plane 0,1 */

    /*    ROM_LOAD( "mslug_c2.rom", 0x420000, 0x200000, 0x49d5ebb1 ) */ /* Plane 2,3 */
    /*    ROM_LOAD( "mslug_c4.rom", 0xc20000, 0x200000, 0x46e1e381 ) */ /* Plane 2,3 */

/* m1 & v1 unused at moment */
ROM_END

ROM_START( trally_rom )
	ROM_REGION(0x120000)
	ROM_LOAD_ODD ("N046001a.038", 0x000000, 0x040000, 0x8bfe2f3c )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ("N046001a.03c", 0x080000, 0x040000, 0x6c640000 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_WIDE_SWAP("neo-geo.rom", 0x100000, 0x020000, 0xf6c12685 )

	ROM_REGION(0x420000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N046001a.378", 0x00000, 0x20000, 0xc6d79eed )

	ROM_LOAD( "N046001a.538", 0x020000, 0x40000, 0x936791f9 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x120000, 0x40000 )
	ROM_LOAD( "N046001a.53c", 0x060000, 0x40000, 0x9df5a633 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x160000, 0x40000 )
	ROM_LOAD( "N046001b.538", 0x0A0000, 0x40000, 0xadf1b389 ) /* Plane 0,1 */
	ROM_CONTINUE(		  0x1a0000, 0x40000 )

  	ROM_LOAD( "N046001a.638", 0x220000, 0x40000, 0x49f0e056 ) /* Plane 2,3 */
  	ROM_CONTINUE(		  0x320000, 0x40000 )
  	ROM_LOAD( "N046001a.63c", 0x260000, 0x40000, 0xf054eaba ) /* Plane 2,3 */
  	ROM_CONTINUE(		  0x260000, 0x40000 )
  	ROM_LOAD( "N046001b.638", 0x2a0000, 0x40000, 0x92fad81a ) /* Plane 2,3 */
  	ROM_CONTINUE(		  0x3a0000, 0x40000 )
ROM_END

ROM_START( bios_rom )
	ROM_REGION(0x020000)
/*	ROM_LOAD_WIDE_SWAP( "karev_p1.rom",  0x000000, 0x100000, 0xa11991e9 ) */
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom",   0x100000, 0x020000, 0xf6c12685 )

/*	ROM_REGION(0x20000)
	ROM_LOAD( "karev_s1.rom", 0x000000, 0x20000, 0xc6d559c9 ) */
ROM_END

/* Inspired by the CPS1 driver, compressed version of GameDrivers */
#define NEODRIVER(NEO_DRIVER,NEO_FILENAME,NEO_REALNAME,NEO_YEAR,NEO_MANU,NEO_CREDIT,NEO_MACHINE,NEO_ROM) \
struct GameDriver NEO_DRIVER  = \
{                               \
	__FILE__,                   \
	&neogeo_bios,               \
	NEO_FILENAME,               \
	NEO_REALNAME,               \
	NEO_YEAR,                   \
	NEO_MANU,                   \
	NEO_CREDIT,                 \
	0,                          \
	NEO_MACHINE,                \
	NEO_ROM,                    \
	0, 0,                       \
	0,                          \
	0, 	    	            \
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

NEODRIVER(trally_driver,"trally","Thrash Rally","1991","Alpha Denshi Co","The Shin Emu Keikaku team",&ghost_pilots_machine_driver,trally_rom)
NEODRIVER(lresort_driver,"lresort","Last Resort","19??","SNK","The Shin Emu Keikau team",&lresort_machine_driver,lresort_rom)
NEODRIVER(fbfrenzy_driver,"fbfrenzy","Football Frenzy","19??","SNK","The Shin Emu Keikau team",&lresort_machine_driver,fbfrenzy_rom)
NEODRIVER(alpham2_driver,"alpham2","Alpha Mission 2","19??","SNK","The Shin Emu Keikau team",&lresort_machine_driver,alpham2_rom)
NEODRIVER(eightman_driver,"eightman","Eight Man","19??","SNK","The Shin Emu Keikau team",&lresort_machine_driver,eightman_rom)
NEODRIVER(countb_driver,"3countb","3 Count Bout","19??","SNK","The Shin Emu Keikau team",&countb_machine_driver,countb_rom)
NEODRIVER(ridhero_driver,"ridhero","Riding Hero","19??","SNK","The Shin Emu Keikau team",&ridhero_machine_driver,ridhero_rom)
NEODRIVER(ttbb_driver,"2020bb","2020 Baseball","19??","SNK","The Shin Emu Keikau team",&ridhero_machine_driver,ttbb_rom)
NEODRIVER(andro_driver,"androdun","Andro Dunos","19??","SNK","The Shin Emu Keikau team",&countb_machine_driver,andro_rom)
NEODRIVER(artfight_driver,"artfight","Art of Fighting","19??","SNK","The Shin Emu Keikau team",&countb_machine_driver,artfight_rom)
NEODRIVER(superspy_driver,"superspy","Super Spy","19??","SNK","The Shin Emu Keikau team",&countb_machine_driver,superspy_rom)
NEODRIVER(nam_1975_driver,"nam_1975","NAM 1975","1990","SNK","The Shin Emu Keikau team",&nam_1975_machine_driver,nam_1975_rom)
NEODRIVER(pbobble_driver,"pbobble","Puzzle Bobble","19??","SNK","The Shin Emu Keikau team",&pbobble_machine_driver,pbobble_rom)
NEODRIVER(skicks_driver,"sidkicks","Super Sidekicks","19??","SNK","The Shin Emu Keikau team",&skicks_machine_driver,skicks_rom)
NEODRIVER(maglord_driver,"maglord","Magician Lord","19??","SNK","The Shin Emu Keikau team",&nam_1975_machine_driver,maglord_rom)
NEODRIVER(pdpon_driver,"puzzledp","Puzzle de Pon / Puzzled","1990","SNK","The Shin Emu Keikaku team",&pdpon_machine_driver,pdpon_rom)
NEODRIVER(joyjoy_driver,"joyjoy","Joy Joy Kid","19??","SNK","The Shin Emu Keikaku team",&joyjoy_machine_driver,joyjoy_rom)
NEODRIVER(ffury1_driver,"fatfury1","Fatal Fury 1","19??","SNK","The Shin Emu Keikaku team",&ghost_pilots_machine_driver,ffury1_rom)
NEODRIVER(mutnat_driver,"mutnat","Mutation Nation","19??","SNK","The Shin Emu Keikaku team",&ghost_pilots_machine_driver,mutnat_rom)
NEODRIVER(bfight_driver,"burningf","Burning Fight","19??","SNK","The Shin Emu Keikaku team",&ghost_pilots_machine_driver,bfight_rom)
NEODRIVER(crsword_driver,"crsword","Crossed Swords","19??","SNK","The Shin Emu Keikaku team",&ghost_pilots_machine_driver,crsword_rom)
NEODRIVER(cyberlip_driver,"cyberlip","Cyber Lip","19??","SNK","The Shin Emu Keikaku team",&ghost_pilots_machine_driver,cyberlip_rom)
NEODRIVER(kingofm_driver,"kingofm","King of Monsters","19??","SNK","The Shin Emu Keikaku team",&ghost_pilots_machine_driver,kingofm_rom)
NEODRIVER(ghost_pilots_driver,"gpilots","Ghost Pilots","19??","SNK","The Shin Emu Keikaku team",&ghost_pilots_machine_driver,ghost_pilots_rom)

/* preliminary drivers */
NEODRIVER(karnov_r_driver,"karnov_r","Karnov's Revenge","19xx","?","The Shin Emu Keikaku team",&karnov_r_machine_driver,karnov_r_rom)
NEODRIVER(mslug_driver,"mslug","Metal Slug","19xx","?","The Shin Emu Keikaku team",&mslug_machine_driver,mslug_rom)
NEODRIVER(wcup_driver,"wcup","Thrash Rally","1991","Alpha Denshi Co","The Shin Emu Keikaku team",&ghost_pilots_machine_driver,wcup_rom)
