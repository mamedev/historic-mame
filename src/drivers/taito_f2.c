/***************************************************************************

Taito F2 System

driver by David Graves, Bryan McPhail, Brad Oliver, Andrew Prime, Brian Troha
with some initial help from Richard Bush

The Taito F2 system is a fairly flexible hardware platform. It supports 5
separate layers of graphics - one 64x64 tiled scrolling background plane
of 8x8 tiles, a similar foreground plane, another plane used exclusively
for zooming and rotation(!), a sprite plane capable of handling
all the video chores by itself (used in e.g. Super Space Invaders) and a text
plane which may or may not scroll. The text plane has 8x8 characters which are
generated in RAM.

Sound is handled by a Z80 with a YM2610 connected to it.

The memory map for each of the games is similar but not identical.

Memory map for Liquid Kids

CPU 1 : 68000, uses irqs 5 & 6.

0x000000 - 0x0fffff : ROM (not all used)
0x100000 - 0x10ffff : 64k of RAM
0x200000 - 0x201fff : palette RAM, 4096 total colors
0x300000 - 0x30000f : input ports and dipswitches (writes may be IRQ acknowledge)
0x320000 - 0x320003 : communication with sound CPU
0x800000 - 0x803fff : 64x64 background layer ("SCREEN RAM")
0x804000 - 0x805fff : 64x64 text layer
0x806000 - 0x807fff : 256 (512?) character generator RAM
0x808000 - 0x80bfff : 64x64 foreground layer ("SCREEN RAM")
0x80c000 - 0x80ffff : unused?
0x820000 - 0x820005 : x scroll for 3 layers (3rd is unknown)
0x820006 - 0x82000b : y scroll for 3 layers (3rd is unknown)
0x82000c - 0x82000f : unknown (leds?)
0x900000 - 0x90ffff : 64k of sprite RAM
0xb00002 - 0xb00002 : watchdog?

F2 Game List

. Final Blow (1)
. Don Doko Don (2)
. Mega Blast (3)
. http://www.taito.co.jp/his/A_HIS/HTM/QUI_TORI.HTM (4)
. Liquid Kids (7)
. Super Space Invaders / Majestic 12 (8)
. Gun Frontier (9)
. Growl / Runark (10)
. Hat Trick Pro (11)
. Mahjong Quest (12)
. http://www.taito.co.jp/his/A_HIS/HTM/YOUYU.HTM (13)
. http://www.taito.co.jp/his/A_HIS/HTM/KOSHIEN.HTM (14)
. Ninja Kids (15)
. http://www.taito.co.jp/his/A_HIS/HTM/Q_QUEST.HTM (no number)
. Metal Black (no number)
. http://www.taito.co.jp/his/A_HIS/HTM/QUI_TIK.HTM (no number)
? Dinorex (no number)
? Pulirula (no number)

This list is translated version of
http://www.aianet.or.jp/~eisetu/rom/rom_tait.html
This page also contains info for other Taito boards.

F2 Motherboard ( Big ) K1100432A, J1100183A
               (Small) K1100608A, J1100242A

*Apr.1989 Final Blow (B82, M4300123A, K1100433A)
*Jul.1989 Don Doko Don (B95, M4300131A, K1100454A, J1100195A)
*Oct.1989 Mega Blast (C11)
Feb.1990 Quiz Torimonochou (C41, K1100554A)
*Apr.1990 Cameltry (C38, M4300167A, K1100556A)
Jul.1990 Quiz H.Q. (C53, K1100594A)
*Aug.1990 Thunder Fox (C28, M4300181A, K1100580A) (exists in F1 version too)
*Sep.1990 Liquid Kids/Mizubaku Daibouken (C49, K1100593A)
*Nov.1990 MJ-12/Super Space Invaders (C64, M4300195A, K1100616A, J1100248A)
Jan.1991 Gun Frontier (C71, M4300199A, K1100625A, K1100629A(overseas))
*Feb.1991 Growl/Runark (C74, M4300210A, K1100639A)
Mar.1991 Hat Trick Hero/Euro Football Championship (C80, K11J0646A)
Mar.1991 Yuu-yu no Quiz de Go!Go! (C83, K11J0652A)
Apr.1991 Ah Eikou no Koshien (C81, M43J0214A, K11J654A)
*Apr.1991 Ninja Kids (C85, M43J0217A, K11J0659A)
May.1991 Mahjong Quest (C77, K1100637A, K1100637B)
Jul.1991 Quiz Quest (C92, K11J0678A)
Sep.1991 Metal Black (D12)
Oct.1991 Drift Out (Visco) (M43X0241A, K11X0695A)
*Nov.1991 PuLiRuLa (C98, M43J0225A, K11J0672A)
Feb.1992 Quiz Chikyu Boueigun (D19, K11J0705A)
Jul.1992 Dead Connection (D28, K11J0715A)
*Nov.1992 Dinorex (D39, K11J0254A)
Mar.1993 Quiz Jinsei Gekijou (D48, M43J0262A, K11J0742A)
Aug.1993 Quiz Crayon Shinchan (D55, K11J0758A)
Dec.1993 Crayon Shinchan Orato Asobo (D63, M43J0276A, K11J0779A)

Mar.1992 Yes.No. Shinri Tokimeki Chart (Fortune teller machine) (D20, K11J0706B)

* means emulated by Raine. I don't know driftout in Raine is
F2 version or not.
Thunder Fox, Drift Out, "Quiz Crayon Shinchan", and "Crayon Shinchan
Orato Asobo" has "Not F2" version PCB.
Foreign version of Cameltry uses different hardware (B89's PLD,
K1100573A, K1100574A).


TODO Lists
==========

Growl/Runark
------------

growl_05.rom is not being used, does the YM2610 need separate ROM regions
like NeoGeo?

Layer priorities. Taito logo should draw above the waterfall sprites.


Ninja Kids
----------

Vertical sprite squashing not supported (see Raine attract).
Misplacement of sprites in attract and cut scenes between levels.

x-flips of char tiles not happening: see RHS of big black bat
which rises up screen at start of attract, or RHS of pentagram circles
on floor midway through game.

Screen transition effects (way it should appear and be cleared) missing.
Often text appears too dark (eg. service mode), palette issue?

Missing coin sound, DIPS


Don Doko Don
------------

Hook in a rotation / zoom layer (roz) at addresses indicated.

Roz Structure
=============

+0x000  row 0  top, first row visible (when layer is platforms)
+0x080  row 1
+0x100  row 2
etc.
+0xd80  row 27, last row visible? (when layer is platforms)
+0xe00  row 28  :last two rows that seem to
+0xe80  row 29  :contain data?

+0xf00 thru +0x1fff: unused?? can't see any colour stuff in there

Row structure  [goes from left to right]
=============

+0x00   data word 0 (each word is one 8x8 tile)
+0x02   data word 1
+0x4e   data word 39  [visible screen is 40 columns]
+0x50 to +0x7e  off-screen columns?

Data word (probable)
=========

%xyyyyyyy yyyyyyyy

x= control bit?
y= tile number, 0-7fff ?

Note tiles 4000-7fff are incorrectly decoded without a byte swap.
Pixels 0 and 1 should swap with 2 and 3, 4 and 5 with 6 and 7 etc.
In effect odd and even bytes of the scr rom b95_03.bin need swapping.

Where is colour byte for each roz tile?

Roz registers (speculative)
=============

+0x00  high word of x offset    :both used to x-scroll the layer
+0x02  low word of x offset     :between levels in game
+0x04  zoom related?
+0x06  zero when no rotation happening
+0x08  high word of y offset    :both used to y-scroll
+0x0a  low word of y offset     :black cloud in intro (see Raine)
+0x0c  zero when no rotation happening
+0x0e  zoom related?

Note that +0x06 ranges from f000-1000 centred about 0.
          +0x0c ranges from f800-0800 centred about 0.

Default zoom (ie zoom level while playing)
          +0x04 is 0x0800
          +0x0e is 0x1000

The 2 rotate/zooms early in attract see these registers tending
towards these values (1st time with overshoot) and +0x06 and +0x0c
working towards zero. 2nd time is just a rotate - see Raine attract.

It seems that while the "zoom" pair remain in certain relation to the
"rotate" pair (90 deg out of phase if looking at them as sine waves)
then no zooming in or out happens. If this relation shifts then the
layer zooms in or out.

Hence the initial zoom in to proper scale when you start level 1 is
achieved by keeping "rotate" pair zero, and linear increments of the
"zoom" pair from just over zero to the default values.

The rotating level 10 boss monster sees X, Y offsets in sin/cos waves
i.e. tracing a circle, and the other registers oscillating about zero
with max values as listed above.

Hope this helps someone, if not delete it.

b95-03.bin (in Raine romsets) was a bad dump with odd bytes all
zero. It has been replaced by b95_03.bin. But this needs odd and even
bytes swapped before it will decode correctly!

DIPS


Cameltry
--------

Missing a scrolling rotation and zoom layer (roz). Probably same
as rotation layer for DonDokoDon and Driftout.

Text palette and its horizontal position seem wrong e.g. text displayed
straight after reset.

Paddle input: should be 0x00 when left/right not pressed; climbing quickly
to 0x0f when left pressed, and falling quickly to 0xf1 when right pressed.
That's how Raine does it anyway.

DIPS


Driftout
--------

Palette structure may be xRRRRxGGGGxBBBBx (unused bits certain) and
not typical F2 RRRRGGGGBBBBxxxx. Reflect in driftout_writemem.

Rotation and zoom layer: probably same as Cameltry.

Unmapped inputs $b00018, $b0001a, analogous to paddles in Cameltry?
But service mode suggests a joystick, and inputs for only one player.
Perhaps clone Driveout uses paddle inputs.

Sound: none
Z80 is writing unmapped E600, needs new write handler.
DIPS


Gun Frontier
------------

Graphics garbage occurs on screen during game, sprites leave ghosts?

Text layer position and scrolling not working (watch early attract in Raine).
Missing magnification of planet as per attract in Raine.

DIPS


Metal Black
-----------

(LBO note: I bet this is very similar to Football Championship)

BG and FG layers not displaying correct tiles. Layer start addresses
probably incorrect, and structure within layers different to usual F2.
Seems BG layers 0-3 present, controlled by a 0x30 long register block
seems as follows: [probably identical to Dead Connection]

+0x00   BG0 x scroll     (topmost layer)
        BG1 x scroll
        BG2 x scroll
        BG3 x scroll     (bottom layer)

+0x08   BG0 y scroll
        BG1 y scroll
        BG2 y scroll
        BG3 y scroll

+0x10   BG0 zoom         (0x007f = normal size, 0xf??? = max zoom)
        BG1 zoom
        BG2 zoom
        BG3 zoom

+0x18   4 unknown words, apparently not layer specific

+0x20   BG0 unknown      (these four seem to remain constant, unused?)
        BG1 unknown
        BG2 unknown
        BG3 unknown

+0x28   BG0 unknown      (these four seem to remain constant, unused?)
        BG1 unknown
        BG2 unknown
        BG3 unknown

Guess each layer 0x1fff long and similar in structure to Cameltry or
DonDokoDon roz layer?

Letters A,E,O vanish

Palette area twice usual size, not correctly handled.

Z80 prg twice the usual 64K, so twice the banking required. New
bank handler?

DIPS


Dead Connection
---------------

BG and FG layers not displaying correct tiles. Layer start addresses
probably incorrect, and structure within layers different to usual F2.
Seems four BG layers present, controlled by a 0x30 long register block.

Letters A,E,O vanish.

DIPS


Dino Rex
--------

Doing log, 68000 seems to go off the rails after a few seconds and start
reading non-existent areas of memory.

Should the two 'm' ROMS interleaved in the CPU1 region be there?

Memory map has some unknown areas, screen areas unverified

GFX rom order is guesswork, may be wrong
6Mb of OBJ is higher than other F2 boards, sprite banking?

DIPS


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"

extern unsigned char *taitof2_scrollx;
extern unsigned char *taitof2_scrolly;
extern unsigned char *taitof2_rotate;
extern unsigned char *f2_backgroundram;
extern size_t f2_backgroundram_size;
extern unsigned char *f2_foregroundram;
extern size_t f2_foregroundram_size;
extern unsigned char *f2_textram;
extern size_t f2_textram_size;
extern unsigned char *taitof2_characterram;
extern size_t f2_characterram_size;
extern size_t f2_paletteram_size;
extern unsigned char *f2_pivotram;
extern size_t f2_pivotram_size;

int taitof2_0_vh_start (void);
int taitof2_vh_start (void);
int taitof2_2_vh_start (void);
void taitof2_vh_stop (void);
READ_HANDLER( taitof2_characterram_r );
WRITE_HANDLER( taitof2_characterram_w );
READ_HANDLER( taitof2_text_r );
WRITE_HANDLER( taitof2_text_w );
READ_HANDLER( taitof2_pivot_r );
WRITE_HANDLER( taitof2_pivot_w );
READ_HANDLER( taitof2_background_r );
WRITE_HANDLER( taitof2_background_w );
READ_HANDLER( taitof2_foreground_r );
WRITE_HANDLER( taitof2_foreground_w );
WRITE_HANDLER( taitof2_spritebank_w );
void taitof2_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);

READ_HANDLER( taitof2_spriteram_r );
WRITE_HANDLER( taitof2_spriteram_w );

WRITE_HANDLER( rastan_sound_port_w );
WRITE_HANDLER( rastan_sound_comm_w );
READ_HANDLER( rastan_sound_comm_r );

READ_HANDLER( rastan_a001_r );
WRITE_HANDLER( rastan_a000_w );
WRITE_HANDLER( rastan_a001_w );

void cchip1_init_machine(void);
READ_HANDLER( cchip1_r );
WRITE_HANDLER( cchip1_w );

static unsigned char *taitof2_ram; /* used for high score save */

static WRITE_HANDLER( bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int banknum = (data - 1) & 7;

	cpu_setbank (2, &RAM [0x10000 + (banknum * 0x4000)]);
}

static READ_HANDLER( taitof2_input_r )
{
    switch (offset)
    {
         case 0x00:
              return readinputport(3); /* DSW A */

         case 0x02:
              return readinputport(4); /* DSW B */

         case 0x04:
              return readinputport(0); /* IN0 */

         case 0x06:
              return readinputport(1); /* IN1 */

         case 0x0e:
              return readinputport(2); /* IN2 */
    }

logerror("CPU #0 input_r offset %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),offset);

	return 0xff;
}

static READ_HANDLER( growl_dsw_r )
{
    switch (offset)
    {
         case 0x00:
              return readinputport(3); /* DSW A */

         case 0x02:
              return readinputport(4); /* DSW B */
    }

logerror("CPU #0 dsw_r offset %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),offset);

	return 0xff;
}

static READ_HANDLER( growl_input_r )
{
    switch (offset)
    {
         case 0x00:
              return readinputport(0); /* IN0 */

         case 0x02:
              return readinputport(1); /* IN1 */

         case 0x04:
              return readinputport(2); /* IN2 */

    }

logerror("CPU #0 input_r offset %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),offset);

	return 0xff;
}

static READ_HANDLER( pulirula_input_r )
{
//	Debugger ();

    switch (offset)
    {
         case 0x04:
              return readinputport(0); /* IN0 */

         case 0x06:
              return readinputport(1); /* IN1 */

         case 0x0e:
              return readinputport(2); /* IN2 */
    }

logerror("CPU #0 input_r offset %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),offset);

	return 0xff;
}

READ_HANDLER( megab_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport (0);
		case 0x02:
			return readinputport (1);
		case 0x04:
			return readinputport (2);
		case 0x06:
			return readinputport (3);
		default:
			logerror("megab_input_r offset: %04x\n", offset);
			return 0xff;
	}
}

static READ_HANDLER( ninjak_input_r )
{
    switch (offset)
    {
         case 0x00:
              return (readinputport(3) << 8); /* DSW A */

         case 0x02:
              return (readinputport(4) << 8); /* DSW B */

         case 0x04:
              return (readinputport(0) << 8); /* IN 0 */

         case 0x06:
              return (readinputport(1) << 8); /* IN 1 */

         case 0x08:
              return (readinputport(5) << 8); /* IN 3 */

         case 0x0a:
              return (readinputport(6) << 8); /* IN 4 */

         case 0x0c:
              return (readinputport(2) << 8); /* IN 2 */

    }

logerror("CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),0x300000+offset);

	return 0xff;
}

static READ_HANDLER( cameltry_paddle_r )
{
    switch (offset)
    {
         case 0x00:
              return readinputport(5); /* Paddle A */

         case 0x04:
              return readinputport(6); /* Paddle B */
    }

logerror("CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),0x300018+offset);

        return 0xff;
}

static READ_HANDLER( driftout_input_r )
{
    switch (offset)
    {
         case 0x00:
              return readinputport(2); /* DSW A */

         case 0x02:
              return readinputport(3); /* DSW B */

         case 0x04:
              return readinputport(0); /* IN0 */

         case 0x0e:
              return readinputport(1); /* IN2 */
    }

logerror("CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),0xb00000+offset);

	return 0xff;
}

static READ_HANDLER( gunfront_input_r )
{
    switch (offset)
    {
         case 0x00:
              return readinputport(4); /* DSW B */

         case 0x02:
              return readinputport(3); /* DSW A */

         case 0x04:
              return readinputport(1); /* IN 1 */

         case 0x06:
              return readinputport(0); /* IN 0 */

         case 0x0c:
              return readinputport(2); /* IN 2 */

    }

logerror("CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),0x300000+offset);

	return 0xff;
}

static READ_HANDLER( metalb_input_r )
{
    switch (offset)
    {
         case 0x00:
              return readinputport(4); /* DSW B */

         case 0x02:
              return readinputport(3); /* DSW A */

         case 0x04:
              return readinputport(1); /* IN 1 */

         case 0x06:
              return readinputport(0); /* IN 0 */

         case 0x0c:
              return readinputport(2); /* IN 2 */

    }

logerror("CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),0x800000+offset);

	return 0xff;
}

static READ_HANDLER( deadconx_input_r )
{
    switch (offset)
    {
         case 0x00:
              return readinputport(3); /* DSW A */

         case 0x02:
              return readinputport(4); /* DSW B */

         case 0x04:
              return readinputport(2); /* IN2 */

         case 0x0a:
              return readinputport(0); /* IN0 */

         case 0x0c:
              return readinputport(1); /* IN1 */

         case 0x0e:
              return readinputport(5); /* IN3 */

         case 0x10:
              return readinputport(6); /* IN4 */
    }

logerror("CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),0x700000+offset);

	return 0xff;
}

static READ_HANDLER( dinorex_input_r )
{
    switch (offset)
    {
         case 0x00:
              return readinputport(3); /* DSW A */

         case 0x02:
              return readinputport(4); /* DSW B */

         case 0x04:
              return readinputport(0); /* IN 0 */

         case 0x06:
              return readinputport(1); /* IN 1 */

         case 0x0e:
              return readinputport(2); /* IN 2 */

    }

logerror("CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),0x300000+offset);

	return 0xff;
}



void taitof2_interrupt5(int x)
{
	cpu_cause_interrupt(0,MC68000_IRQ_5);
}

static int taitof2_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(200000-5000,0),0, taitof2_interrupt5);
	return MC68000_IRQ_6;
}

WRITE_HANDLER( taitof2_sound_w )
{
	if (offset == 0)
		rastan_sound_port_w (0, data & 0xff);
	else if (offset == 2)
		rastan_sound_comm_w (0, data & 0xff);
#ifdef MAME_DEBUG
	if (data & 0xff00)
	{
		char buf[80];

		sprintf(buf,"taitof2_sound_w to high byte: %04x",data);
		usrintf_showmessage(buf);
	}
#endif
}

READ_HANDLER( taitof2_sound_r )
{
	if (offset == 2)
		return ((rastan_sound_comm_r (0) & 0xff));
	else return 0;
}

WRITE_HANDLER( taitof2_msb_sound_w )
{
	if (offset == 0)
		rastan_sound_port_w (0,(data >> 8) & 0xff);
	else if (offset == 2)
		rastan_sound_comm_w (0,(data >> 8) & 0xff);
#ifdef MAME_DEBUG
	if (data & 0xff)
	{
		char buf[80];

		sprintf(buf,"taitof2_msb_sound_w to low byte: %04x",data);
		usrintf_showmessage(buf);
	}
#endif
}

READ_HANDLER( taitof2_msb_sound_r )
{
	if (offset == 2)
		return ((rastan_sound_comm_r (0) & 0xff) << 8);
	else return 0;
}

static READ_HANDLER( sound_hack_r )
{
	return YM2610_status_port_0_A_r (0) | 1;
}


/************************************
**                                  *
**PALETTE FINALBLOW  -  START HERE  *
**                                  *
************************************/


static unsigned int pal_ind = 0;
static unsigned int pal_tab[ 0x1000 ];

static WRITE_HANDLER( finalb_palette_w )
{
	if (offset==0)
	{
		/*data = palette register number (memory offset)*/

		pal_ind = (data>>1) & 0x7ff;
/*note:
*In test mode game writes to odd register number (that's why it is (data>>1) )
*/

		if (data>0xfff) logerror ("write to palette index > 0xfff\n");
	}
	else if (offset==2)
	{
		/*data = palette BGR value*/
		int r,g,b;

		pal_tab[ pal_ind ] = data & 0xffff;


		/* FWIW all r,g,b values seem to be using only top 4 bits */
		r = (data>>0)  & 0x1f;
		g = (data>>5)  & 0x1f;
		b = (data>>10) & 0x1f;

		r = (r<<3) | (r>>2);
		g = (g<<3) | (g>>2);
		b = (b<<3) | (b>>2);

		palette_change_color(pal_ind,r,g,b);
		/*logerror ("write %04x to palette index %04x [r=%x g=%x b=%x]\n",data, pal_ind,r,g,b);*/
	}
}

static READ_HANDLER( finalb_palette_r )
{
	if (offset == 2)
	{
		/*logerror ("reading val %04x from palette index %04x\n",pal_tab[pal_ind], pal_ind);*/
		return pal_tab[ pal_ind ];
	}
	return -1;
}


static struct MemoryReadAddress finalb_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },


	{ 0x200000, 0x200003, finalb_palette_r },



	{ 0x300000, 0x30000f, taitof2_input_r },
	{ 0x320000, 0x320003, taitof2_sound_r },
	{ 0x800000, 0x803fff, taitof2_background_r },
	{ 0x804000, 0x805fff, taitof2_text_r },
	{ 0x806000, 0x806fff, taitof2_characterram_r },
	{ 0x807000, 0x807fff, MRA_BANK3 },
	{ 0x808000, 0x80bfff, taitof2_foreground_r },
	{ 0x80c000, 0x80ffff, MRA_BANK4 },
	{ 0x900000, 0x90ffff, taitof2_spriteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress finalb_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1, &taitof2_ram },


	{ 0x200000, 0x200003, finalb_palette_w },




	{ 0x300000, 0x300001, MWA_NOP }, /* irq ack? liquidk */
	{ 0x300008, 0x300009, MWA_NOP }, /* lots of zero writes here */
	{ 0x320000, 0x320003, taitof2_sound_w },
	{ 0x800000, 0x803fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x804000, 0x805fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x806000, 0x806fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x807000, 0x807fff, MWA_BANK3 }, /* unused? */
	{ 0x808000, 0x80bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x80c000, 0x80ffff, MWA_BANK4 }, /* unused? */
	{ 0x810000, 0x81ffff, MWA_NOP },	/*error in game init code ?*/
	{ 0x820000, 0x820005, MWA_BANK5, &taitof2_scrollx },
	{ 0x820006, 0x82000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x900000, 0x90ffff, taitof2_spriteram_w, &spriteram, &spriteram_size  },
	{ 0xb00002, 0xb00003, MWA_NOP },	/* watchdog ?? liquidk */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress ssi_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x10000f, taitof2_input_r },
	{ 0x200000, 0x20ffff, MRA_BANK1 },
	{ 0x300000, 0x301fff, paletteram_word_r },
	{ 0x400000, 0x400003, taitof2_msb_sound_r },
	{ 0x600000, 0x60ffff, MRA_BANK3 },
	{ 0x800000, 0x80ffff, taitof2_spriteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ssi_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10000f, watchdog_reset_w },	/* ? */
	{ 0x200000, 0x20ffff, MWA_BANK1 },
	{ 0x300000, 0x301fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram },
	{ 0x400000, 0x400003, taitof2_msb_sound_w },
//	{ 0x500000, 0x500001, MWA_NOP },	/* ?? */
	{ 0x600000, 0x60ffff, MWA_BANK3 }, /* unused f2 video layers */
//	{ 0x620000, 0x62000f, MWA_NOP }, /* unused f2 video control registers */
	{ 0x800000, 0x80ffff, taitof2_spriteram_w, &spriteram, &spriteram_size }, /* sprite ram */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress liquidk_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },
	{ 0x200000, 0x201fff, paletteram_word_r },
	{ 0x300000, 0x30000f, taitof2_input_r },
	{ 0x320000, 0x320003, taitof2_sound_r },
	{ 0x800000, 0x803fff, taitof2_background_r },
	{ 0x804000, 0x805fff, taitof2_text_r },
	{ 0x806000, 0x806fff, taitof2_characterram_r },
	{ 0x807000, 0x807fff, MRA_BANK3 },
	{ 0x808000, 0x80bfff, taitof2_foreground_r },
	{ 0x80c000, 0x80ffff, MRA_BANK4 },
	{ 0x900000, 0x90ffff, taitof2_spriteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress liquidk_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1, &taitof2_ram },
	{ 0x200000, 0x201fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x300000, 0x300001, MWA_NOP }, /* irq ack? liquidk */
	{ 0x320000, 0x320003, taitof2_sound_w },
	{ 0x800000, 0x803fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x804000, 0x805fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x806000, 0x806fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x807000, 0x807fff, MWA_BANK3 }, /* unused? */
	{ 0x808000, 0x80bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x80c000, 0x80ffff, MWA_BANK4 }, /* unused? */
	{ 0x820000, 0x820005, MWA_BANK5, &taitof2_scrollx },
	{ 0x820006, 0x82000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x900000, 0x90ffff, taitof2_spriteram_w, &spriteram, &spriteram_size  },
	{ 0xb00002, 0xb00003, MWA_NOP },	/* watchdog ?? liquidk */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress dondokod_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },
	{ 0x200000, 0x201fff, paletteram_word_r },
	{ 0x300000, 0x30000f, taitof2_input_r },
	{ 0x320000, 0x320003, taitof2_msb_sound_r },
	{ 0x800000, 0x803fff, taitof2_background_r },
	{ 0x804000, 0x805fff, taitof2_text_r },
	{ 0x806000, 0x806fff, taitof2_characterram_r },
	{ 0x807000, 0x807fff, MRA_BANK3 },
	{ 0x808000, 0x80bfff, taitof2_foreground_r },
	{ 0x80c000, 0x80ffff, MRA_BANK4 },
	{ 0x900000, 0x90ffff, taitof2_spriteram_r },
	{ 0xa00000, 0xa01fff, taitof2_pivot_r }, /* ?? complete guess */
	{ 0xa02000, 0xa0200f, MRA_BANK7 }, /* complete guess */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress dondokod_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1, &taitof2_ram },
	{ 0x200000, 0x201fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x300000, 0x300001, MWA_NOP }, /* irq ack? as per liquidk */
	{ 0x320000, 0x320003, taitof2_msb_sound_w },
	{ 0x800000, 0x803fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x804000, 0x805fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x806000, 0x806fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x807000, 0x807fff, MWA_BANK3 }, /* unused? */
	{ 0x808000, 0x80bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x80c000, 0x80ffff, MWA_BANK4 }, /* unused? */
	{ 0x820000, 0x820005, MWA_BANK5, &taitof2_scrollx },
	{ 0x820006, 0x82000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x900000, 0x90ffff, taitof2_spriteram_w, &spriteram, &spriteram_size  },
	{ 0xa00000, 0xa01fff, taitof2_pivot_w, &f2_pivotram, &f2_pivotram_size }, /* ?? complete guess */
	{ 0xa02000, 0xa0200f, MWA_BANK7, &taitof2_rotate }, /* ?? complete guess */
	{ 0xb00002, 0xb00003, MWA_NOP },	/* watchdog ?? as per liquidk */
	{ 0xb00000, 0xb00001, MWA_NOP },	/* watchdog ?? growl */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress cameltry_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },
	{ 0x200000, 0x201fff, paletteram_word_r },
	{ 0x300000, 0x30000f, taitof2_input_r },
	{ 0x300018, 0x30001f, cameltry_paddle_r },
	{ 0x320000, 0x320003, taitof2_msb_sound_r },
	{ 0x800000, 0x803fff, taitof2_background_r },
	{ 0x804000, 0x805fff, taitof2_text_r },
	{ 0x806000, 0x806fff, taitof2_characterram_r },
	{ 0x807000, 0x807fff, MRA_BANK3 },
	{ 0x808000, 0x80bfff, taitof2_foreground_r },
	{ 0x80c000, 0x80ffff, MRA_BANK4 },
	{ 0x900000, 0x90ffff, taitof2_spriteram_r },
	{ 0xa00000, 0xa01fff, taitof2_pivot_r }, /* ?? complete guess */
	{ 0xa02000, 0xa0200f, MRA_BANK7 }, /* complete guess */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress cameltry_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1, &taitof2_ram },
	{ 0x200000, 0x201fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x300000, 0x300001, MWA_NOP }, /* irq ack? as per liquidk */
	{ 0x320000, 0x320003, taitof2_msb_sound_w },
	{ 0x800000, 0x803fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x804000, 0x805fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x806000, 0x806fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x807000, 0x807fff, MWA_BANK3 }, /* unused? */
	{ 0x808000, 0x80bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x80c000, 0x80ffff, MWA_BANK4 }, /* unused? */
	{ 0x820000, 0x820005, MWA_BANK5, &taitof2_scrollx },
	{ 0x820006, 0x82000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x900000, 0x90ffff, taitof2_spriteram_w, &spriteram, &spriteram_size  },
	{ 0xa00000, 0xa01fff, taitof2_pivot_w, &f2_pivotram, &f2_pivotram_size }, /* ?? complete guess */
	{ 0xa02000, 0xa0200f, MWA_BANK7, &taitof2_rotate }, /* ?? complete guess */
	{ 0xb00002, 0xb00003, MWA_NOP },	/* watchdog ?? as per liquidk */
	{ 0xb00000, 0xb00001, MWA_NOP },	/* watchdog ?? growl */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress growl_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },
	{ 0x200000, 0x201fff, paletteram_word_r },
	{ 0x300000, 0x30000f, growl_dsw_r },
	{ 0x320000, 0x32000f, growl_input_r },
	{ 0x400000, 0x400003, taitof2_msb_sound_r },
	{ 0x508000, 0x50800f, input_port_5_r }, /* IN3 */
	{ 0x50c000, 0x50c00f, input_port_6_r }, /* IN4 */
	{ 0x800000, 0x803fff, taitof2_background_r },
	{ 0x804000, 0x805fff, taitof2_text_r },
	{ 0x806000, 0x806fff, taitof2_characterram_r },
	{ 0x807000, 0x807fff, MRA_BANK3 },
	{ 0x808000, 0x80bfff, taitof2_foreground_r },
	{ 0x80c000, 0x80ffff, MRA_BANK4 },
	{ 0x900000, 0x90ffff, taitof2_spriteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress growl_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1, &taitof2_ram },
	{ 0x200000, 0x201fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x340000, 0x340001, MWA_NOP }, /* irq ack? growl */
	{ 0x400000, 0x400003, taitof2_msb_sound_w },
	{ 0x500000, 0x50000f, taitof2_spritebank_w },
	{ 0x800000, 0x803fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x804000, 0x805fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x806000, 0x806fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x807000, 0x807fff, MWA_BANK3 }, /* unused? */
	{ 0x808000, 0x80bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x80c000, 0x80ffff, MWA_BANK4 }, /* unused? */
	{ 0x820000, 0x820005, MWA_BANK5, &taitof2_scrollx },
	{ 0x820006, 0x82000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x900000, 0x90ffff, taitof2_spriteram_w, &spriteram, &spriteram_size  },
	{ 0xb00000, 0xb00001, MWA_NOP },	/* watchdog ?? growl */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress pulirula_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x200000, 0x200003, taitof2_msb_sound_r },
	{ 0x300000, 0x30ffff, MRA_BANK1 },
//	{ 0x508000, 0x50800f, input_port_5_r }, /* IN3 */
//	{ 0x50c000, 0x50c00f, input_port_6_r }, /* IN4 */
	{ 0x400000, 0x401fff, taitof2_pivot_r }, /* pivot RAM? */
	{ 0x700000, 0x701fff, paletteram_word_r },
	{ 0x800000, 0x803fff, taitof2_background_r },
	{ 0x804000, 0x805fff, taitof2_text_r },
	{ 0x806000, 0x806fff, taitof2_characterram_r },
	{ 0x807000, 0x807fff, MRA_BANK3 },
	{ 0x808000, 0x80bfff, taitof2_foreground_r },
	{ 0x80c000, 0x80ffff, MRA_BANK4 },
	{ 0x900000, 0x90ffff, taitof2_spriteram_r },
	{ 0xb00000, 0xb0000f, taitof2_input_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress pulirula_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x200000, 0x200003, taitof2_msb_sound_w },
	{ 0x300000, 0x30ffff, MWA_BANK1, &taitof2_ram },
//	{ 0x500000, 0x50000f, taitof2_spritebank_w },
	{ 0x400000, 0x401fff, taitof2_pivot_w, &f2_pivotram, &f2_pivotram_size },
	{ 0x402000, 0x40200f, MWA_BANK7, &taitof2_rotate }, /* pivot controls */
	{ 0x700000, 0x701fff, paletteram_xRRRRRGGGGGBBBBB_word_w, &paletteram, &f2_paletteram_size },
	{ 0x800000, 0x803fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x804000, 0x805fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x806000, 0x806fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x807000, 0x807fff, MWA_BANK3 }, /* unused? */
	{ 0x808000, 0x80bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x80c000, 0x80ffff, MWA_BANK4 }, /* unused? */
	{ 0x820000, 0x820005, MWA_BANK5, &taitof2_scrollx },
	{ 0x820006, 0x82000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x900000, 0x90ffff, taitof2_spriteram_w, &spriteram, &spriteram_size  },
	{ 0xa00000, 0xa00001, MWA_NOP },	/* watchdog ?? */
//	{ 0xb00000, 0xb00001, MWA_NOP },	/* watchdog ?? */
	{ 0xb00000, 0xb0000f, taitof2_spritebank_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress megab_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x100000, 0x100003, taitof2_msb_sound_r },
	{ 0x120000, 0x12000f, megab_input_r },
	{ 0x180000, 0x180fff, cchip1_r },
	{ 0x200000, 0x20ffff, MRA_BANK1 },
	{ 0x300000, 0x301fff, paletteram_word_r },
	{ 0x600000, 0x603fff, taitof2_background_r },
	{ 0x604000, 0x605fff, taitof2_text_r },
	{ 0x606000, 0x606fff, taitof2_characterram_r },
	{ 0x607000, 0x607fff, MRA_BANK3 },
	{ 0x608000, 0x60bfff, taitof2_foreground_r },
	{ 0x60c000, 0x60ffff, MRA_BANK4 },
	{ 0x610000, 0x61ffff, MRA_BANK7 }, /* unused? */
	{ 0x800000, 0x80ffff, taitof2_spriteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress megab_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x200000, 0x20ffff, MWA_BANK1 },
	{ 0x300000, 0x301fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x100000, 0x100003, taitof2_msb_sound_w },
	{ 0x120000, 0x120001, MWA_NOP }, /* irq ack? */
	{ 0x180000, 0x180fff, cchip1_w },
	{ 0x400000, 0x400001, MWA_NOP },	/* watchdog ?? */
	{ 0x600000, 0x603fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x604000, 0x605fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x606000, 0x606fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x607000, 0x607fff, MWA_BANK3 }, /* unused? */
	{ 0x608000, 0x60bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x60c000, 0x60ffff, MWA_BANK4 }, /* unused? */
	{ 0x610000, 0x61ffff, MWA_BANK7 }, /* unused? */
	{ 0x620000, 0x620005, MWA_BANK5, &taitof2_scrollx },
	{ 0x620006, 0x62000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x800000, 0x80ffff, taitof2_spriteram_w, &spriteram, &spriteram_size  },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress qzchikyu_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x500000, 0x50ffff, MRA_BANK1 },

//	{ 0x100000, 0x100003, taitof2_msb_sound_r },
//	{ 0x120000, 0x12000f, megab_input_r },
//	{ 0x180000, 0x180fff, cchip1_r },

	//{ 0x300000, 0x301fff, paletteram_word_r },
	{ 0x700000, 0x703fff, taitof2_background_r },
	{ 0x704000, 0x705fff, taitof2_text_r },
	{ 0x706000, 0x706fff, taitof2_characterram_r },
	{ 0x707000, 0x707fff, MRA_BANK3 },
	{ 0x708000, 0x70bfff, taitof2_foreground_r },
	{ 0x70c000, 0x70ffff, MRA_BANK4 },
//	{ 0x710000, 0x61ffff, MRA_BANK7 }, /* unused? */
	{ 0x600000, 0x60ffff, taitof2_spriteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress qzchikyu_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x200000, 0x200001, MWA_NOP },	/* watchdog */
	{ 0x500000, 0x50ffff, MWA_BANK1 },


//	{ 0x300000, 0x301fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
//	{ 0x100000, 0x100003, taitof2_msb_sound_w },
//	{ 0x120000, 0x120001, MWA_NOP }, /* irq ack? */
//	{ 0x180000, 0x180fff, cchip1_w },

	{ 0x700000, 0x703fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x704000, 0x705fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x706000, 0x706fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x707000, 0x707fff, MWA_BANK3 }, /* unused? */
	{ 0x708000, 0x70bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x70c000, 0x70ffff, MWA_BANK4 }, /* unused? */
//{ 0x710000, 0x71ffff, MWA_BANK7 }, /* unused? */

//	{ 0x620000, 0x620005, MWA_BANK5, &taitof2_scrollx },
//	{ 0x620006, 0x62000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x600000, 0x60ffff, taitof2_spriteram_w, &spriteram, &spriteram_size  },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress ninjak_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },
	{ 0x200000, 0x201fff, paletteram_word_r },
	{ 0x300000, 0x30000f, ninjak_input_r },
	{ 0x400000, 0x400003, taitof2_msb_sound_r },
	{ 0x800000, 0x803fff, taitof2_background_r },
	{ 0x804000, 0x805fff, taitof2_text_r },
	{ 0x806000, 0x806fff, taitof2_characterram_r },
	{ 0x807000, 0x807fff, MRA_BANK3 },
	{ 0x808000, 0x80bfff, taitof2_foreground_r },
	{ 0x80c000, 0x80ffff, MRA_BANK4 },
	{ 0x900000, 0x90ffff, taitof2_spriteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ninjak_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1, &taitof2_ram },
	{ 0x200000, 0x201fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x380000, 0x380001, MWA_NOP }, /* irq ack? ninjak */
	{ 0x400000, 0x400003, taitof2_msb_sound_w },
	{ 0x600000, 0x60000f, taitof2_spritebank_w },
	{ 0x800000, 0x803fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x804000, 0x805fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x806000, 0x806fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x807000, 0x807fff, MWA_BANK3 }, /* unused? */
	{ 0x808000, 0x80bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x80c000, 0x80ffff, MWA_BANK4 }, /* unused? */
	{ 0x820000, 0x820005, MWA_BANK5, &taitof2_scrollx },
	{ 0x820006, 0x82000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x900000, 0x90ffff, taitof2_spriteram_w, &spriteram, &spriteram_size },
	{ 0xb00002, 0xb00003, MWA_NOP },        /* watchdog ?? ninjak */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress driftout_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x200000, 0x200003, taitof2_msb_sound_r },
	{ 0x300000, 0x30ffff, MRA_BANK1 },
	{ 0x400000, 0x401fff, taitof2_pivot_r }, /* pivot RAM? */
//	{ 0x402000, 0x40200f, MRA_BANK7 },    /* for roz debugging */
	{ 0x700000, 0x701fff, paletteram_word_r },
	{ 0x800000, 0x803fff, taitof2_background_r },
	{ 0x804000, 0x805fff, taitof2_text_r },
	{ 0x806000, 0x806fff, taitof2_characterram_r },
	{ 0x807000, 0x80ffff, MRA_BANK3 },
	{ 0x900000, 0x90ffff, taitof2_spriteram_r },
	{ 0xa80000, 0xa83fff, taitof2_foreground_r },   /* Dummy address, unused layer? */
	{ 0xb00000, 0xb0000f, driftout_input_r },
/*	{ 0xb00018, 0xb0001b, driftout_paddle_r }    ?        */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress driftout_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x200000, 0x200003, taitof2_msb_sound_w },
	{ 0x300000, 0x30ffff, MWA_BANK1, &taitof2_ram },
	{ 0x400000, 0x401fff, taitof2_pivot_w, &f2_pivotram, &f2_pivotram_size },
	{ 0x402000, 0x40200f, MWA_BANK7, &taitof2_rotate }, /* pivot controls */
	{ 0x500000, 0x500001, MWA_NOP }, /* irq ack? */
	{ 0x700000, 0x701fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x800000, 0x803fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x804000, 0x805fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x806000, 0x806fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x807000, 0x80ffff, MWA_BANK3 }, /* unused? */
	{ 0x820000, 0x820005, MWA_BANK5, &taitof2_scrollx },
	{ 0x820006, 0x82000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x900000, 0x90ffff, taitof2_spriteram_w, &spriteram, &spriteram_size },
	{ 0xa00000, 0xa00001, MWA_NOP },        /* watchdog ?? */
	{ 0xa80000, 0xa83fff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* dummy address */
	{ 0xb00000, 0xb00001, MWA_NOP },        /* watchdog ?? */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress gunfront_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },
	{ 0x200000, 0x201fff, paletteram_word_r },
	{ 0x300000, 0x30000f, gunfront_input_r },
	{ 0x320000, 0x320003, taitof2_msb_sound_r },
	{ 0x800000, 0x803fff, taitof2_background_r },
	{ 0x804000, 0x805fff, taitof2_text_r },
	{ 0x806000, 0x806fff, taitof2_characterram_r },
	{ 0x807000, 0x807fff, MRA_BANK3 },
	{ 0x808000, 0x80bfff, taitof2_foreground_r },
	{ 0x80c000, 0x80ffff, MRA_BANK4 },
	{ 0x900000, 0x90ffff, taitof2_spriteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress gunfront_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1, &taitof2_ram },
	{ 0x200000, 0x201fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x300002, 0x300003, MWA_NOP }, /* irq ack? gunfront */
	{ 0x320000, 0x320003, taitof2_msb_sound_w },
	{ 0x800000, 0x803fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x804000, 0x805fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x806000, 0x806fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x807000, 0x807fff, MWA_BANK3 }, /* unused? */
	{ 0x808000, 0x80bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x80c000, 0x80ffff, MWA_BANK4 }, /* unused? */
	{ 0x820000, 0x820005, MWA_BANK5, &taitof2_scrollx },
	{ 0x820006, 0x82000b, MWA_BANK6, &taitof2_scrolly },
	{ 0x900000, 0x90ffff, taitof2_spriteram_w, &spriteram, &spriteram_size },
	{ 0xb00000, 0xb00001, MWA_NOP },        /* watchdog ?? gunfront */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress metalb_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },
	{ 0x300000, 0x30ffff, taitof2_spriteram_r },
	{ 0x500000, 0x503fff, taitof2_background_r },
	{ 0x504000, 0x507fff, taitof2_foreground_r },
	{ 0x508000, 0x50bfff, MRA_BANK4 },
	{ 0x50c000, 0x50dfff, taitof2_text_r },
	{ 0x50e000, 0x50ffff, taitof2_characterram_r },
	{ 0x530000, 0x53002f, MRA_BANK5 },   /* debugging roz layers */
	{ 0x700000, 0x703fff, paletteram_word_r },
	{ 0x800000, 0x80000f, metalb_input_r },
	{ 0x900000, 0x900003, taitof2_msb_sound_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress metalb_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1, &taitof2_ram },
	{ 0x300000, 0x30ffff, taitof2_spriteram_w, &spriteram, &spriteram_size },
	{ 0x420000, 0x420005, MWA_BANK2, &taitof2_scrollx },   /* not verified */
	{ 0x420006, 0x42000b, MWA_BANK3, &taitof2_scrolly },
	{ 0x500000, 0x503fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x504000, 0x507fff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x508000, 0x50bfff, MWA_BANK4 },   /* unused? */
	{ 0x50c000, 0x50dfff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x50e000, 0x50ffff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x530000, 0x53002f, MWA_BANK5 },   /* BG layers control regs? */
	{ 0x600000, 0x600001, MWA_NOP },     /* watchdog ?? metalb */
	{ 0x700000, 0x703fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x800002, 0x800003, MWA_NOP }, /* irq ack? metalb */
	{ 0x900000, 0x900003, taitof2_msb_sound_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress deadconx_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },
	{ 0x200000, 0x20ffff, taitof2_spriteram_r },
	{ 0x400000, 0x403fff, taitof2_background_r },
	{ 0x404000, 0x407fff, MRA_BANK3 },
	{ 0x408000, 0x40bfff, taitof2_foreground_r },
	{ 0x40c000, 0x40dfff, taitof2_text_r },
	{ 0x40e000, 0x40ffff, taitof2_characterram_r },
	{ 0x430000, 0x43002f, MRA_BANK6 },   /* debug roz layers */
	{ 0x600000, 0x601fff, paletteram_word_r },
	{ 0x700000, 0x70001f, deadconx_input_r },
	{ 0xa00000, 0xa00003, taitof2_msb_sound_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress deadconx_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1, &taitof2_ram },
	{ 0x200000, 0x20ffff, taitof2_spriteram_w, &spriteram, &spriteram_size },
	{ 0x300000, 0x30000f, taitof2_spritebank_w },
	{ 0x400000, 0x403fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x404000, 0x407fff, MWA_BANK3 }, /* unused? */
	{ 0x408000, 0x40bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x40c000, 0x40dfff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x40e000, 0x40ffff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x420000, 0x420005, MWA_BANK4, &taitof2_scrollx },   /* not verified */
	{ 0x420006, 0x42000b, MWA_BANK5, &taitof2_scrolly },
	{ 0x430000, 0x43002f, MWA_BANK6 },  /* BG layers control regs */
	{ 0x500000, 0x500003, MWA_NOP },    /* irq ack? deadconx */
	{ 0x600000, 0x601fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x800000, 0x800001, MWA_NOP },        /* watchdog ? deadconx */
	{ 0xa00000, 0xa00003, taitof2_msb_sound_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress dinorex_readmem[] =
{
	{ 0x000000, 0x2fffff, MRA_ROM },
	{ 0x300000, 0x30000f, dinorex_input_r },
	{ 0x500000, 0x501fff, paletteram_word_r },
	{ 0x600000, 0x60ffff, MRA_BANK1 },
	{ 0x800000, 0x803fff, taitof2_background_r },
	{ 0x804000, 0x805fff, taitof2_text_r },
	{ 0x806000, 0x806fff, taitof2_characterram_r },
	{ 0x807000, 0x807fff, MRA_BANK3 },
	{ 0x808000, 0x80bfff, taitof2_foreground_r },
	{ 0x80c000, 0x80ffff, MRA_BANK4 },
	{ 0x900000, 0x90ffff, taitof2_spriteram_r },
	{ 0xa00000, 0xa00003, taitof2_msb_sound_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress dinorex_writemem[] =
{
	{ 0x000000, 0x2fffff, MWA_ROM },
	{ 0x300000, 0x300001, MWA_NOP }, /* irq ack? dinorex */
/*	{ 0x400000, 0x400fff, dinorex_unknown_w },     !unknown!  */
	{ 0x500000, 0x501fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &f2_paletteram_size },
	{ 0x600000, 0x60ffff, MWA_BANK1, &taitof2_ram },
	{ 0x700000, 0x700003, MWA_NOP },
	{ 0x800000, 0x803fff, taitof2_background_w, &f2_backgroundram, &f2_backgroundram_size }, /* background layer */
	{ 0x804000, 0x805fff, taitof2_text_w, &f2_textram, &f2_textram_size }, /* text layer */
	{ 0x806000, 0x806fff, taitof2_characterram_w, &taitof2_characterram, &f2_characterram_size },
	{ 0x807000, 0x807fff, MWA_BANK3 }, /* unused? */
	{ 0x808000, 0x80bfff, taitof2_foreground_w, &f2_foregroundram, &f2_foregroundram_size }, /* foreground layer */
	{ 0x80c000, 0x80ffff, MWA_BANK4 }, /* unused? */
/*	{ 0x820000, 0x82000f, MWA_NOP },       unused?     */
	{ 0x900000, 0x90ffff, taitof2_spriteram_w, &spriteram, &spriteram_size },
	{ 0x920000, 0x920005, MWA_BANK5, &taitof2_scrollx },
	{ 0x920006, 0x92000b, MWA_BANK6, &taitof2_scrolly },
	{ 0xa00000, 0xa00003, taitof2_msb_sound_w },
	{ 0xb00000, 0xb00001, MWA_NOP },        /* watchdog ?? dinorex */
	{ -1 }  /* end of table */
};


/***************************************************************************/

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, sound_hack_r },
//	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, rastan_a001_r },
	{ 0xea00, 0xea00, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, rastan_a000_w },
	{ 0xe201, 0xe201, rastan_a001_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, bankswitch_w },	/* ?? */
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( ssi )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Shields" )
	PORT_DIPSETTING(    0x00, "None")
	PORT_DIPSETTING(    0x0c, "1")
	PORT_DIPSETTING(    0x04, "2")
	PORT_DIPSETTING(    0x08, "3")
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2")
	PORT_DIPSETTING(    0x10, "3")
	PORT_DIPNAME( 0xa0, 0xa0, "2 Players Mode" )
	PORT_DIPSETTING(    0xa0, "Simultaneous")
	PORT_DIPSETTING(    0x80, "Alternate, Single")
	PORT_DIPSETTING(    0x00, "Alternate, Dual")
	PORT_DIPSETTING(    0x20, "Not Allowed")
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( majest12 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Shields" )
	PORT_DIPSETTING(    0x00, "None")
	PORT_DIPSETTING(    0x0c, "1")
	PORT_DIPSETTING(    0x04, "2")
	PORT_DIPSETTING(    0x08, "3")
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2")
	PORT_DIPSETTING(    0x10, "3")
	PORT_DIPNAME( 0xa0, 0xa0, "2 Players Mode" )
	PORT_DIPSETTING(    0xa0, "Simultaneous")
	PORT_DIPSETTING(    0x80, "Alternate, Single Controls")
	PORT_DIPSETTING(    0x00, "Alternate, Dual Controls")
	PORT_DIPSETTING(    0x20, "Not Allowed")
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
INPUT_PORTS_END


INPUT_PORTS_START( liquidk )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "30k 100k" )
	PORT_DIPSETTING(    0x08, "30k 150k" )
	PORT_DIPSETTING(    0x04, "50k 250k" )
	PORT_DIPSETTING(    0x00, "50k 350k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( finalb )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen?" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Shields" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x20, "2 Players Mode" )
	PORT_DIPSETTING(    0x00, "Alternate" )
	PORT_DIPSETTING(    0x20, "Simultaneous" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Simultaneous Game" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( growl )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service A", KEYCODE_9, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( On) )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( On) )
	PORT_DIPNAME( 0x30, 0x30, "Game Type" )
	PORT_DIPSETTING(    0x30, "1 or 2 Players only" )
	PORT_DIPSETTING(    0x20, "Up to 4 Players dipendent" )
	PORT_DIPSETTING(    0x10, "Up to 4 Players indipendent" )
//    PORT_DIPSETTING(    0x00, "Up to 4 Players indipendent" )
    PORT_DIPNAME( 0x40, 0x40, "Last Stage Continue" )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service B", KEYCODE_0, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( pulirula )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service A", KEYCODE_9, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( On) )
	PORT_DIPNAME( 0x30, 0x30, "Game Type" )
	PORT_DIPSETTING(    0x30, "1 or 2 Players only" )
	PORT_DIPSETTING(    0x20, "Up to 4 Players dipendent" )
	PORT_DIPSETTING(    0x10, "Up to 4 Players indipendent" )
	PORT_DIPSETTING(    0x00, "Up to 4 Players indipendent" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service B", KEYCODE_0, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( megab )
	PORT_START /* DSW A */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, "1 Joystick" )
    PORT_DIPSETTING(    0x01, "2 Joysticks" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* DSW c */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Norm" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "50k, 150k" )
	PORT_DIPSETTING(    0x0a, "Bonus 2??" )
	PORT_DIPSETTING(    0x08, "Bonus 3??" )
	PORT_DIPSETTING(    0x00, "Bonus 4??" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* DSW D */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
    PORT_DIPNAME( 0x04, 0x04, "2 Player Mode" )
    PORT_DIPSETTING(    0x00, "Alternate" )
    PORT_DIPSETTING(    0x04, "Simultaneous" )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( dondokod )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen?" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sound?" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "10k 100k" )
	PORT_DIPSETTING(    0x08, "10k 150k" )
	PORT_DIPSETTING(    0x04, "10k 250k" )
	PORT_DIPSETTING(    0x00, "10k 350k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( cameltry )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen?" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sound?" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Start remain time" )
	PORT_DIPSETTING(    0x0c, "50" )
	PORT_DIPSETTING(    0x08, "60" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "35" )
	PORT_DIPNAME( 0x30, 0x30, "Continue play time" )
	PORT_DIPSETTING(    0x30, "+30" )
	PORT_DIPSETTING(    0x20, "+40" )
	PORT_DIPSETTING(    0x10, "+25" )
	PORT_DIPSETTING(    0x00, "+20" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Pair Play" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START  /* Paddle A */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER1, 50, 10, 0, 0 )

	PORT_START  /* Paddle B */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 50, 10, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( ninjak )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service A", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service B", KEYCODE_0, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( On) )
	PORT_DIPNAME( 0x30, 0x30, "Game Type" )
	PORT_DIPSETTING(    0x30, "1 or 2 Players only" )
	PORT_DIPSETTING(    0x20, "Up to 4 Players dependent" )
	PORT_DIPSETTING(    0x10, "Up to 4 Players independent" )
	PORT_DIPSETTING(    0x00, "Up to 4 Players independent" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( driftout )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1)

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen?" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sound?" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START /* DSW B copied from cameltry, probably wrong */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Start remain time" )
	PORT_DIPSETTING(    0x0c, "50" )
	PORT_DIPSETTING(    0x08, "60" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "35" )
	PORT_DIPNAME( 0x30, 0x30, "Continue play time" )
	PORT_DIPSETTING(    0x30, "+30" )
	PORT_DIPSETTING(    0x20, "+40" )
	PORT_DIPSETTING(    0x10, "+25" )
	PORT_DIPSETTING(    0x00, "+20" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Pair Play" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( gunfront )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service A", KEYCODE_9, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( On) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Pair Play" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( metalb )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service A", KEYCODE_9, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( On) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Pair Play" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( deadconx )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, "Service A", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE, "Service B", KEYCODE_0, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_SERVICE, "Service C", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B, probably all wrong */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "30k 100k" )
	PORT_DIPSETTING(    0x08, "30k 150k" )
	PORT_DIPSETTING(    0x04, "50k 250k" )
	PORT_DIPSETTING(    0x00, "50k 350k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( dinorex )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service A", KEYCODE_9, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4, 9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 4 bits per pixel */
	{ 0, 8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout pivotlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout finalb_tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(2,3),
	4,	/* 6 bits per pixel */
	{ 0, 1, 2, 3/*, RGN_FRAC(2,3), RGN_FRAC(2,3)+1*/ },
//	6,	/* 6 bits per pixel */
//	{ 0, 1, 2, 3, RGN_FRAC(2,3), RGN_FRAC(2,3)+1 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4, 9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo ssi_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x000000, &tilelayout, 0, 256 },         /* sprites & playfield */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tilelayout,  0, 256 },  /* sprites & playfield */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },  /* sprites & playfield */
    { 0, 0x000000, &charlayout2, 0, 256 },	/* the game dynamically modifies this */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo pivot_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tilelayout,  0, 256 },  /* sprites & playfield */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },  /* sprites & playfield */
    { 0, 0x000000, &charlayout2, 0, 256 },	/* the game dynamically modifies this */
	{ REGION_GFX3, 0x0, &pivotlayout,  0, 256 },  /* sprites & playfield */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo finalb_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &finalb_tilelayout,  0, 256 },  /* sprites & playfield */
//	{ REGION_GFX2, 0x0, &tilelayout,  0, 256 },  /* sprites & playfield */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },  /* sprites & playfield */
    { 0, 0x000000, &charlayout2, 0, 256 },	/* the game dynamically modifies this */
	{ -1 } /* end of array */
};

/******
We could have used standard F2 decode routine for charlayout, but the structure looks funny, with adjacent tiles not as similar as in other F2 games. Until 4 roz layers hooked up won't know why.
******/

static struct GfxLayout deadconx_charlayout =
{
	8,8,    /* 8*8 characters */
	32768,  /* 32768 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout deadconx_charlayout2 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 3*4, 2*4, 1*4, 0*4, 7*4, 6*4, 5*4, 4*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo deadconx_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tilelayout,  0, 256 },  /* sprites & playfield */
	{ REGION_GFX1, 0x0, &deadconx_charlayout,  0, 256 },  /* sprites & playfield */
	{ 0, 0x000000, &deadconx_charlayout2, 0, 256 },      /* the game dynamically modifies this */
	{ -1 } /* end of array */
};



/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz ?????? */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};

#if 0
static struct YM2610interface pulirula_ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz ?????? */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};
#endif


static struct MachineDriver machine_driver_ssi =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ? */
			ssi_readmem, ssi_writemem,0,0,
			m68_level5_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, 1800,	/* frames per second, vblank duration hand tuned to avoid flicker */
	10,
	0,

	/* video hardware */
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },

	ssi_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_AFTER_VBLANK,
	0,
	taitof2_0_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_liquidk =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ? */
			liquidk_readmem, liquidk_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, 2000,	/* frames per second, vblank duration hand tuned to avoid flicker */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_AFTER_VBLANK,
	0,
	taitof2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_finalb =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			finalb_readmem, finalb_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	finalb_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitof2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_growl =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			growl_readmem, growl_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitof2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_pulirula =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			pulirula_readmem, pulirula_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	pivot_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitof2_2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_megab =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			megab_readmem, megab_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	cchip1_init_machine,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitof2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_dondokod =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ? */
			dondokod_readmem, dondokod_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, 2000,	/* frames per second, vblank duration copied from liquidk */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	pivot_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_AFTER_VBLANK,
	0,
	taitof2_2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_cameltry =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ? */
			cameltry_readmem, cameltry_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, 2000,	/* frames per second, vblank duration copied from liquidk */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	pivot_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_AFTER_VBLANK,
	0,
	taitof2_2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_qzchikyu =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ? */
			qzchikyu_readmem, qzchikyu_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, 500,	/* frames per second, vblank duration copied from liquidk */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	pivot_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_AFTER_VBLANK,
	0,
	taitof2_2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_ninjak =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			ninjak_readmem, ninjak_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitof2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_driftout =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ? */
			driftout_readmem, driftout_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, 2000,	/* frames per second, vblank duration copied from liquidk */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_AFTER_VBLANK,
	0,
	taitof2_2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_gunfront =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			gunfront_readmem, gunfront_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitof2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_metalb =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			metalb_readmem, metalb_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	deadconx_gfxdecodeinfo,     /* is scr correctly decoded? */
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitof2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_deadconx =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			deadconx_readmem, deadconx_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	deadconx_gfxdecodeinfo,    /* is scr correctly decoded? */
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitof2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_dinorex =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			dinorex_readmem, dinorex_writemem, 0, 0,
			taitof2_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitof2_vh_start,
	taitof2_vh_stop,
	taitof2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( finalb )
	ROM_REGION( 0x40000, REGION_CPU1 )     /* 256k for 68000 code */
	ROM_LOAD_EVEN( "fb_09.rom",  0x00000, 0x20000, 0x632f1ecd )
	ROM_LOAD_ODD ( "fb_17.rom",  0x00000, 0x20000, 0xe91b2ec9 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD_GFX_EVEN( "fb_m06.rom", 0x000000, 0x020000, 0xfc450a25 )
	ROM_LOAD_GFX_ODD ( "fb_m07.rom", 0x000000, 0x020000, 0xec3df577 )

	ROM_REGION( 0x180000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD_GFX_EVEN( "fb_m03.rom", 0x000000, 0x080000, 0xdaa11561 ) /* sprites */
	ROM_LOAD_GFX_ODD ( "fb_m04.rom", 0x000000, 0x080000, 0x6346f98e ) /* sprites */
	ROM_LOAD         ( "fb_m05.rom", 0x100000, 0x080000, 0xaa90b93a ) /* sprites */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "fb_10.rom",   0x00000, 0x04000, 0xa38aaaed )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "fb_m02.rom", 0x00000, 0x80000, 0x5dd06bdd )
	ROM_LOAD( "fb_m01.rom", 0x80000, 0x80000, 0xf0eb6846 )
ROM_END

ROM_START( megab )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 256k for 68000 code */
	ROM_LOAD_EVEN( "c11-07",  0x00000, 0x20000, 0x11d228b6 )
	ROM_LOAD_ODD ( "c11-08",  0x00000, 0x20000, 0xa79d4dca )
	ROM_LOAD_EVEN( "c11-06",  0x40000, 0x20000, 0x7c249894 ) /* ?? */
	ROM_LOAD_ODD ( "c11-11",  0x40000, 0x20000, 0x263ecbf9 ) /* ?? */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c11-05", 0x000000, 0x080000, 0x733e6d8e )

	ROM_REGION( 0x100000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD_GFX_EVEN( "c11-03", 0x000000, 0x080000, 0x46718c7a )
	ROM_LOAD_GFX_ODD ( "c11-04", 0x000000, 0x080000, 0x663f33cc )

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "c11-12", 0x00000, 0x04000, 0xb11094f1 )
	ROM_CONTINUE(       0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "c11-01", 0x000000, 0x080000, 0xfd1ea532 )
	ROM_LOAD( "c11-02", 0x080000, 0x080000, 0x451cc187 )
ROM_END

ROM_START( ssi )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "ssi_15-1.rom", 0x00000, 0x40000, 0xce9308a6 )
	ROM_LOAD_ODD ( "ssi_16-1.rom", 0x00000, 0x40000, 0x470a483a )

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "ssi_m01.rom",  0x00000, 0x100000, 0xa1b4f486 )

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "ssi_09.rom",   0x00000, 0x04000, 0x88d7f65c )
	ROM_CONTINUE(             0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* 2610 samples */
	ROM_LOAD( "ssi_m02.rom",  0x00000, 0x20000, 0x3cb0b907 )
ROM_END

ROM_START( majest12 )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "c64-07.bin", 0x00000, 0x20000, 0xf29ed5c9 )
	ROM_LOAD_EVEN( "c64-06.bin", 0x40000, 0x20000, 0x18dc71ac )
	ROM_LOAD_ODD ( "c64-08.bin", 0x00000, 0x20000, 0xddfd33d5 )
	ROM_LOAD_ODD ( "c64-05.bin", 0x40000, 0x20000, 0xb61866c0 )

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "ssi_m01.rom",  0x00000, 0x100000, 0xa1b4f486 )

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "ssi_09.rom",   0x00000, 0x04000, 0x88d7f65c )
	ROM_CONTINUE(             0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* 2610 samples */
	ROM_LOAD( "ssi_m02.rom",  0x00000, 0x20000, 0x3cb0b907 )
ROM_END

ROM_START( liquidk )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "lq09.bin",  0x00000, 0x20000, 0x6ae09eb9 )
	ROM_LOAD_ODD ( "lq11.bin",  0x00000, 0x20000, 0x42d2be6e )
	ROM_LOAD_EVEN( "lq10.bin",  0x40000, 0x20000, 0x50bef2e0 )
	ROM_LOAD_ODD ( "lq12.bin",  0x40000, 0x20000, 0xcb16bad5 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lk_scr.bin",  0x000000, 0x080000, 0xc3364f9b )

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lk_obj0.bin", 0x000000, 0x080000, 0x67cc3163 )
	ROM_LOAD( "lk_obj1.bin", 0x080000, 0x080000, 0xd2400710 )

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "lq08.bin",    0x00000, 0x04000, 0x413c310c )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "lk_snd.bin",  0x00000, 0x80000, 0x474d45a4 )
ROM_END

ROM_START( liquidku )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "lq09.bin",  0x00000, 0x20000, 0x6ae09eb9 )
	ROM_LOAD_ODD ( "lq11.bin",  0x00000, 0x20000, 0x42d2be6e )
	ROM_LOAD_EVEN( "lq10.bin",  0x40000, 0x20000, 0x50bef2e0 )
	ROM_LOAD_ODD ( "lq14.bin",  0x40000, 0x20000, 0xbc118a43 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lk_scr.bin",  0x000000, 0x080000, 0xc3364f9b )

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lk_obj0.bin", 0x000000, 0x080000, 0x67cc3163 )
	ROM_LOAD( "lk_obj1.bin", 0x080000, 0x080000, 0xd2400710 )

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "lq08.bin",    0x00000, 0x04000, 0x413c310c )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "lk_snd.bin",  0x00000, 0x80000, 0x474d45a4 )
ROM_END

ROM_START( mizubaku )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "lq09.bin",  0x00000, 0x20000, 0x6ae09eb9 )
	ROM_LOAD_ODD ( "lq11.bin",  0x00000, 0x20000, 0x42d2be6e )
	ROM_LOAD_EVEN( "lq10.bin",  0x40000, 0x20000, 0x50bef2e0 )
	ROM_LOAD_ODD ( "c49-13",    0x40000, 0x20000, 0x2518dbf9 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lk_scr.bin",  0x000000, 0x080000, 0xc3364f9b )

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lk_obj0.bin", 0x000000, 0x080000, 0x67cc3163 )
	ROM_LOAD( "lk_obj1.bin", 0x080000, 0x080000, 0xd2400710 )

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "lq08.bin",    0x00000, 0x04000, 0x413c310c )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "lk_snd.bin",  0x00000, 0x80000, 0x474d45a4 )
ROM_END

ROM_START( growl )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 1024k for 68000 code */
	ROM_LOAD_EVEN( "growl_10.rom",  0x00000, 0x40000, 0xca81a20b )
	ROM_LOAD_ODD ( "growl_08.rom",  0x00000, 0x40000, 0xaa35dd9e )
	ROM_LOAD_EVEN( "growl_11.rom",  0x80000, 0x40000, 0xee3bd6d5 )
	ROM_LOAD_ODD ( "growl_14.rom",  0x80000, 0x40000, 0xb6c24ec7 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "growl_01.rom", 0x000000, 0x100000, 0x3434ce80 ) /* characters */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "growl_03.rom", 0x000000, 0x100000, 0x1a0d8951 ) /* sprites */
	ROM_LOAD( "growl_02.rom", 0x100000, 0x100000, 0x15a21506 ) /* sprites */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "growl_12.rom", 0x00000, 0x04000, 0xbb6ed668 )
	ROM_CONTINUE(             0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "growl_04.rom", 0x000000, 0x100000, 0x2d97edf2 )

	ROM_REGION( 0x080000, REGION_SOUND2 )	/* ADPCM samples */
	ROM_LOAD( "growl_05.rom", 0x000000, 0x080000, 0xe29c0828 )
ROM_END

ROM_START( growlu )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 1024k for 68000 code */
	ROM_LOAD_EVEN( "growl_10.rom",  0x00000, 0x40000, 0xca81a20b )
	ROM_LOAD_ODD ( "growl_08.rom",  0x00000, 0x40000, 0xaa35dd9e )
	ROM_LOAD_EVEN( "growl_11.rom",  0x80000, 0x40000, 0xee3bd6d5 )
	ROM_LOAD_ODD ( "c74-13.rom",    0x80000, 0x40000, 0xc1c57e51 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "growl_01.rom", 0x000000, 0x100000, 0x3434ce80 ) /* characters */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "growl_03.rom", 0x000000, 0x100000, 0x1a0d8951 ) /* sprites */
	ROM_LOAD( "growl_02.rom", 0x100000, 0x100000, 0x15a21506 ) /* sprites */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "growl_12.rom", 0x00000, 0x04000, 0xbb6ed668 )
	ROM_CONTINUE(             0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "growl_04.rom", 0x000000, 0x100000, 0x2d97edf2 )

	ROM_REGION( 0x080000, REGION_SOUND2 )	/* ADPCM samples */
	ROM_LOAD( "growl_05.rom", 0x000000, 0x080000, 0xe29c0828 )
ROM_END

ROM_START( runark )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 1024k for 68000 code */
	ROM_LOAD_EVEN( "growl_10.rom",  0x00000, 0x40000, 0xca81a20b )
	ROM_LOAD_ODD ( "growl_08.rom",  0x00000, 0x40000, 0xaa35dd9e )
	ROM_LOAD_EVEN( "growl_11.rom",  0x80000, 0x40000, 0xee3bd6d5 )
	ROM_LOAD_ODD ( "c74_09.14",     0x80000, 0x40000, 0x58cc2feb )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "growl_01.rom", 0x000000, 0x100000, 0x3434ce80 ) /* characters */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "growl_03.rom", 0x000000, 0x100000, 0x1a0d8951 ) /* sprites */
	ROM_LOAD( "growl_02.rom", 0x100000, 0x100000, 0x15a21506 ) /* sprites */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "growl_12.rom", 0x00000, 0x04000, 0xbb6ed668 )
	ROM_CONTINUE(             0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "growl_04.rom", 0x000000, 0x100000, 0x2d97edf2 )

	ROM_REGION( 0x080000, REGION_SOUND2 )	/* ADPCM samples */
	ROM_LOAD( "growl_05.rom", 0x000000, 0x080000, 0xe29c0828 )
ROM_END

ROM_START( pulirula )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 1024k for 68000 code */
	ROM_LOAD_EVEN( "c98-12.rom", 0x00000, 0x40000, 0x816d6cde )
	ROM_LOAD_ODD ( "c98-16.rom", 0x00000, 0x40000, 0x59df5c77 )
	ROM_LOAD_EVEN( "c98-06.rom", 0x80000, 0x20000, 0x64a71b45 ) /* ?? */
	ROM_LOAD_ODD ( "c98-07.rom", 0x80000, 0x20000, 0x90195bc0 ) /* ?? */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "c98-04.rom", 0x000000, 0x100000, 0x0e1fe3b2 ) /* sprites */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "c98-03.rom", 0x000000, 0x100000, 0x589a678f ) /* sprites */
	ROM_LOAD( "c98-02.rom", 0x100000, 0x100000, 0x4a2ad2b3 ) /* sprites */

	ROM_REGION( 0x080000, REGION_GFX3 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c98-05.rom", 0x000000, 0x080000, 0x9ddd9c39 ) /* sprites ?? */

	ROM_REGION( 0x2c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "c98-14.rom", 0x00000, 0x04000, 0xa858e17c )
	ROM_CONTINUE(           0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "c98-01.rom", 0x000000, 0x100000, 0x197f66f5 )
ROM_END

ROM_START( dondokod )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "b95-12.bin",   0x00000, 0x20000, 0xd0fce87a )
	ROM_LOAD_ODD ( "b95-11-1.bin", 0x00000, 0x20000, 0xdad40cd3 )
	ROM_LOAD_EVEN( "b95-10.bin",   0x40000, 0x20000, 0xa46e1f0b )
	ROM_LOAD_ODD ( "b95-09.bin",   0x40000, 0x20000, 0xd8c86d39 )

	ROM_REGION( 0x080000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b95-02.bin", 0x000000, 0x080000, 0x67b4e979 )	 /* background/foreground */

	ROM_REGION( 0x080000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b95-01.bin", 0x000000, 0x080000, 0x51c176ce )	 /* sprites */

	ROM_REGION( 0x080000, REGION_GFX3 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b95_03.bin", 0x000000, 0x080000, 0x543aa0d1 )     /* pivot graphics */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "b95-08.bin",  0x00000, 0x04000, 0xb5aa49e1 )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples? */
	ROM_LOAD( "b95-04.bin",  0x00000, 0x80000, 0xac4c1716 )
ROM_END

ROM_START( cameltry )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 256k for 68000 code */
	ROM_LOAD_EVEN( "c38-09.bin", 0x00000, 0x20000, 0x2ae01120 )
	ROM_LOAD_ODD ( "c38-10.bin", 0x00000, 0x20000, 0x48d8ff56 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
//	ROM_LOAD( "c38-02.bin", 0x000000, 0x020000, 0x1a11714b )	 /* background/foreground */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c38-01.bin", 0x000000, 0x080000, 0xc170ff36 )	 /* sprites */

	ROM_REGION( 0x080000, REGION_GFX3 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
//	ROM_LOAD( "c38-03.bin", 0x000000, 0x020000, 0x59fa59a7 )     /* pivot graphics */
	ROM_LOAD( "c38-02.bin", 0x000000, 0x020000, 0x1a11714b )	 /* ?? */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "c38-08.bin", 0x00000, 0x04000, 0x7ff78873 )
	ROM_CONTINUE(           0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "c38-03.bin", 0x000000, 0x020000, 0x59fa59a7 )     /* ?? */
ROM_END

ROM_START( cameltru )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 256k for 68000 code */
	ROM_LOAD_EVEN( "c38-11", 0x00000, 0x20000, 0xbe172da0 )
	ROM_LOAD_ODD ( "c38-14", 0x00000, 0x20000, 0xffa430de )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
//	ROM_LOAD( "c38-02.bin", 0x000000, 0x020000, 0x1a11714b )	 /* background/foreground */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c38-01.bin", 0x000000, 0x080000, 0xc170ff36 )	 /* sprites */

	ROM_REGION( 0x080000, REGION_GFX3 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
//	ROM_LOAD( "c38-03.bin", 0x000000, 0x020000, 0x59fa59a7 )     /* pivot graphics */
	ROM_LOAD( "c38-02.bin", 0x000000, 0x020000, 0x1a11714b )	 /* ?? */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "c38-08.bin", 0x00000, 0x04000, 0x7ff78873 )
	ROM_CONTINUE(           0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "c38-03.bin", 0x000000, 0x020000, 0x59fa59a7 )     /* ?? */
ROM_END

ROM_START( qzchikyu )
	ROM_REGION( 0x40000, REGION_CPU1 )     /* 256k for 68000 code */
	ROM_LOAD_EVEN( "d19-06.bin", 0x00000, 0x20000, 0x2ae01120 )
	ROM_LOAD_ODD ( "d19-05.bin", 0x00000, 0x20000, 0x48d8ff56 )

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "d19-07.bin", 0x00000, 0x04000, 0x7ff78873 )
	ROM_CONTINUE(           0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d19-02.bin", 0x000000, 0x100000, 0x1a11714b )

	ROM_REGION( 0x100000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "d19-01.bin", 0x000000, 0x100000, 0xc170ff36 )	 /* sprites */

	ROM_REGION( 0x080000, REGION_GFX3 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
//	ROM_LOAD( "c38-03.bin", 0x000000, 0x020000, 0x59fa59a7 )     /* pivot graphics */
	//ROM_LOAD( "c38-02.bin", 0x000000, 0x020000, 0x1a11714b )	 /* ?? */
	ROM_LOAD( "d19-03.bin", 0x000000, 0x080000, 0x1a11714b )


	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
//	ROM_LOAD( "c38-03.bin", 0x000000, 0x020000, 0x59fa59a7 )     /* ?? */
ROM_END

ROM_START( ninjak )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "nk_0h.rom",  0x00000, 0x20000, 0xba7e6e74 )
	ROM_LOAD_ODD ( "nk_0l.rom",  0x00000, 0x20000, 0x0ac2cba2 )
	ROM_LOAD_EVEN( "nk_1lh.rom",  0x40000, 0x20000, 0x3eccfd0a )
	ROM_LOAD_ODD ( "nk_1ll.rom",  0x40000, 0x20000, 0xd126ded1 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nk_scrn.rom", 0x000000, 0x080000, 0x4cc7b9df ) /* characters */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nk_obj-0.rom", 0x000000, 0x100000, 0xa711977c ) /* sprites */
	ROM_LOAD( "nk_obj-1.rom", 0x100000, 0x100000, 0xa6ad0f3d ) /* sprites */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "nk_snd.rom", 0x00000, 0x04000, 0xf2a52a51 )
	ROM_CONTINUE(             0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "nk_sch-a.rom", 0x000000, 0x080000, 0x5afb747e )
	ROM_LOAD( "nk_sch-b.rom", 0x080000, 0x080000, 0x3c1b0ed0 )
ROM_END

ROM_START( driftout )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 1024k for 68000 code */
	ROM_LOAD_EVEN( "do_46.rom",  0x00000, 0x80000, 0xf960363e )
	ROM_LOAD_ODD ( "do_45.rom",  0x00000, 0x80000, 0xe3fe66b9 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "do_piv.rom",  0x000000, 0x080000, 0xa3fda55f )

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "do_obj.rom", 0x000000, 0x080000, 0x5491f1c4 )

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "do_50.rom",   0x00000, 0x04000, 0xffe10124 )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples? */
	ROM_LOAD( "do_snd.rom",  0x00000, 0x80000, 0xf2deb82b )
ROM_END

ROM_START( gunfront )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 768k for 68000 code */
	ROM_LOAD_EVEN( "c71-09.rom",  0x00000, 0x20000, 0x10a544a2 )
	ROM_LOAD_ODD ( "c71-08.rom",  0x00000, 0x20000, 0xc17dc0a0 )
	ROM_LOAD_EVEN( "c71-10.rom",  0x40000, 0x20000, 0xf39c0a06 )
	ROM_LOAD_ODD ( "c71-14.rom",  0x40000, 0x20000, 0x312da036 )
	ROM_LOAD_EVEN( "c71-16.rom",  0x80000, 0x20000, 0x1bbcc2d4 )
	ROM_LOAD_ODD ( "c71-15.rom",  0x80000, 0x20000, 0xdf3e00bb )

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c71-02.rom", 0x000000, 0x100000, 0x2a600c92 ) /* characters */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c71-03.rom", 0x000000, 0x100000, 0x9133c605 ) /* sprites */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "c71-12.rom", 0x00000, 0x04000, 0x0038c7f8 )
	ROM_CONTINUE(           0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "c71-01.rom", 0x000000, 0x100000, 0x0e73105a )
ROM_END

ROM_START( metalb )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 768k for 68000 code */
	ROM_LOAD_EVEN( "d16-16.rom",  0x00000, 0x40000, 0x3150be61 )
	ROM_LOAD_ODD ( "d16-18.rom",  0x00000, 0x40000, 0x5216d092 )
	ROM_LOAD_EVEN( "d12-07.rom",  0x80000, 0x20000, 0xe07f5136 )
	ROM_LOAD_ODD ( "d12-06.rom",  0x80000, 0x20000, 0x131df731 )

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d12-04.rom", 0x000000, 0x080000, 0xab66d141 ) /* characters */
	ROM_LOAD( "d12-03.rom", 0x080000, 0x080000, 0x46b498c0 ) /* characters */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d12-01.rom", 0x000000, 0x100000, 0xb81523b9 ) /* sprites */

	ROM_REGION( 0x2c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "d12-13.rom", 0x00000, 0x04000, 0xbcca2649 )
	ROM_CONTINUE(             0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "d12-02.rom", 0x000000, 0x100000, 0x79263e74 )

	ROM_REGION( 0x080000, REGION_SOUND2 )   /* ADPCM samples, unused like Growl? */
	ROM_LOAD( "d12-05.rom", 0x000000, 0x080000, 0x7fd036c5 )
ROM_END

ROM_START( deadconx )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 1024k for 68000 code */
	ROM_LOAD_EVEN( "d28-06.rom",  0x00000, 0x40000, 0x5b4bff51 )
	ROM_LOAD_ODD ( "d28_12.rom",  0x00000, 0x40000, 0x9b74e631 )
	ROM_LOAD_EVEN( "d28-09.rom",  0x80000, 0x40000, 0x143a0cc1 )
	ROM_LOAD_ODD ( "d28-08.rom",  0x80000, 0x40000, 0x4c872bd9 )

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d28-05.rom", 0x000000, 0x080000, 0x862f9665 ) /* characters, unsure of rom order! */
	ROM_LOAD( "d28-04.rom", 0x080000, 0x080000, 0xdcabc26b ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d28-01.rom", 0x000000, 0x100000, 0x181d7b69 ) /* sprites */
	ROM_LOAD( "d28-02.rom", 0x100000, 0x100000, 0xd301771c ) /* sprites */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "d28-10.rom", 0x00000, 0x04000, 0x40805d74 )
	ROM_CONTINUE(             0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "d28-03.rom", 0x000000, 0x100000, 0xa1804b52 )
ROM_END

ROM_START( dinorex )
	ROM_REGION( 0x300000, REGION_CPU1 )     /* 3Mb for 68000 code */
	ROM_LOAD_EVEN( "drex_14.rom",  0x00000, 0x80000, 0xe6aafdac )
	ROM_LOAD_ODD ( "drex_16.rom",  0x00000, 0x80000, 0xcedc8537 )
	ROM_LOAD_EVEN( "drex_04m.rom", 0x100000, 0x100000, 0x3800506d )  /*  ??  */
	ROM_LOAD_ODD ( "drex_05m.rom", 0x100000, 0x100000, 0xe2ec3b5d )  /*  ??  */

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "drex_06m.rom", 0x000000, 0x100000, 0x52f62835 ) /* characters */

	ROM_REGION( 0xa00000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "drex_02m.rom", 0x000000, 0x200000, 0x6c304403 ) /* sprites */
	ROM_LOAD( "drex_03m.rom", 0x200000, 0x200000, 0xfc9cdab4 ) /* sprites */
	ROM_LOAD( "drex_01m.rom", 0x400000, 0x200000, 0xd10e9c7d ) /* sprites */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "drex_12.rom", 0x00000, 0x04000, 0x8292c7c1 )
	ROM_CONTINUE(             0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "drex_07m.rom", 0x000000, 0x100000, 0x28262816 )

	ROM_REGION( 0x080000, REGION_SOUND2 )   /* ADPCM samples, unused as growl? */
	ROM_LOAD( "drex_08m.rom", 0x000000, 0x080000, 0x377b8b7b )
ROM_END

#if 0
/* Sets without drivers */
ROM_START( thundfox )		/* Thunder Fox */
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "c28mainl.12", 0x00000, 0x20000, 0xf04db477 )
	ROM_LOAD_ODD ( "c28mainh.13", 0x00000, 0x20000, 0xacb07013 )
	ROM_LOAD_EVEN( "c28lo.07",    0x40000, 0x20000, 0x24419abb )
	ROM_LOAD_ODD ( "c28hi.08",    0x40000, 0x20000, 0x38e038f1 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c28scr1.01", 0x000000, 0x080000, 0x6230a09d )
	ROM_LOAD( "c28scr2.01", 0x080000, 0x080000, 0x44552b25 )

	ROM_REGION( 0x100000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c28objl.03", 0x000000, 0x080000, 0x51bdc7af )
	ROM_LOAD( "c28objh.04", 0x080000, 0x080000, 0xba7ed535 )

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "c28snd.14", 0x00000, 0x04000, 0x45ef3616 )
	ROM_CONTINUE(          0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "c28snda.06", 0x000000, 0x080000, 0xdb6983db )	/* Channel A */

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* ADPCM samples */
	ROM_LOAD( "c28sndb.05", 0x000000, 0x080000, 0xd3b238fa )	/* Channel B */
ROM_END

ROM_START( solfigtr )	/* Solitary Fighter */
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "c91-05",  0x00000, 0x40000, 0xc1260e7c ) /* Prog1 */
	ROM_LOAD_ODD ( "c91-09",  0x00000, 0x40000, 0xd82b5266 ) /* Prog2 */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c91-03", 0x000000, 0x100000, 0x8965da12 ) /* Screen   */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c91-01", 0x000000, 0x100000, 0x0f3f4e00 ) /* Object 0 */
	ROM_LOAD( "c91-02", 0x100000, 0x100000, 0xe14ab98e ) /* Object 1 */

	ROM_REGION( 0x1c000, REGION_CPU2 )      /* sound cpu */
	ROM_LOAD( "c91-07",    0x00000, 0x04000, 0xe471a05a ) /* AUD Prog */
	ROM_CONTINUE(          0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "c91-04",  0x00000, 0x80000, 0x390b1065 )	/* Channel A */
ROM_END
#endif



/* Working games */
GAMEX( 1990, ssi,      0,        ssi,      ssi,      0, ROT270, "Taito Corporation Japan", "Super Space Invaders '91 (World)", GAME_NO_COCKTAIL )
GAMEX( 1990, majest12, ssi,      ssi,      majest12, 0, ROT270, "Taito Corporation", "Majestic Twelve - The Space Invaders Part IV (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1990, liquidk,  0,        liquidk,  liquidk,  0, ROT180, "Taito Corporation Japan", "Liquid Kids (World)", GAME_NO_COCKTAIL )
GAMEX( 1990, liquidku, liquidk,  liquidk,  liquidk,  0, ROT180, "Taito America Corporation", "Liquid Kids (US)", GAME_NO_COCKTAIL )
GAMEX( 1990, mizubaku, liquidk,  liquidk,  liquidk,  0, ROT180, "Taito Corporation", "Mizubaku Daibouken (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1990, growl,    0,        growl,    growl,    0, ROT0,   "Taito Corporation Japan", "Growl (World)", GAME_NO_COCKTAIL )
GAMEX( 1990, growlu,   growl,    growl,    growl,    0, ROT0,   "Taito America Corporation", "Growl (US)", GAME_NO_COCKTAIL )
GAMEX( 1990, runark,   growl,    growl,    growl,    0, ROT0,   "Taito Corporation", "Runark (Japan)", GAME_NO_COCKTAIL )

/* Busted games */
GAMEX( 1988, finalb,   0,        finalb,   finalb,   0, ROT0,   "Taito", "Final Blow", GAME_NO_COCKTAIL )
GAMEX( 1989, megab,    0,        megab,    megab,    0, ROT0,   "Taito", "Mega Blast", GAME_UNEMULATED_PROTECTION | GAME_NO_COCKTAIL )
GAMEX( 1989, dondokod, 0,        dondokod, dondokod, 0, ROT180, "Taito Corporation", "Don Doko Don", GAME_NO_COCKTAIL )
GAMEX( 1990, gunfront, 0,        gunfront, gunfront, 0, ROT270, "Taito Corp. Japan", "Gun Frontier", GAME_NO_COCKTAIL )
GAMEX( 1990, cameltry, 0,        cameltry, cameltry, 0, ROT0,   "Taito Corporation", "Camel Try", GAME_NO_COCKTAIL )
GAMEX( 1990, cameltru, cameltry, cameltry, cameltry, 0, ROT0,   "Taito America Corporation", "Camel Try (US)", GAME_NO_COCKTAIL )
GAMEX( 1990, ninjak,   0,        ninjak,   ninjak,   0, ROT0,   "Taito Corporation Japan", "Ninja Kids", GAME_NO_COCKTAIL )
GAMEX( 1991, driftout, 0,        driftout, driftout, 0, ROT270, "Visco Corporation", "Drift Out", GAME_NO_COCKTAIL )
GAMEX( 1991, pulirula, 0,        pulirula, pulirula, 0, ROT0,   "Taito Corporation", "PuLiRuLa", GAME_NO_COCKTAIL )
GAMEX( 1991, metalb,   0,        metalb,   metalb,   0, ROT0,   "Taito Corp. Japan", "Metal Black (Gun Frontier 2)", GAME_NO_COCKTAIL )
GAMEX( 1992, deadconx, 0,        deadconx, deadconx, 0, ROT0,   "Taito Corporation Japan", "Dead Connection", GAME_NO_COCKTAIL )
GAMEX( 1992, qzchikyu, 0,        qzchikyu, growl,    0, ROT0,   "Taito Corporation", "Quiz Chikyu Bouei Gun (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1992, dinorex,  0,        dinorex,  dinorex,  0, ROT0,   "Taito Corporation Japan", "Dino Rex", GAME_NO_COCKTAIL )
