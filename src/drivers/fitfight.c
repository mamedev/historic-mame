/* Fit of Fighting / The History of Martial Arts / 'BB' */

/* NIX or Novatecnia (both spanish) may have produced these
   its probably NIX due to somes similarities with Pirates

	Supported Games                  Rip-off of

	Fit of Fighting                  Art of Fighting (neogeo.c)
	The History of Martial Arts      Fighter's History (deco32.c)
	'BB' Untitled Prototype          -none, original-

   'BB' Prototype isn't a game as such, 'BB' was the label on
   the prototype board. which appears to have been used simply
   for testings / practice.  There is no 'game' to it, no
   title screen, simply 3 characters, 1 portrait, 1 background,
   the characters don't appear to have any moves other than
   basic walking around.  There is also no sound.

   The lack of Sound, GFX and Gameplay in this 'game' are NOT
   emulation bugs.

   There are some unused GFX in the roms, it might be interesting
   to try and put them together and see what they form.

*/

/*

68k interrupts (fitfight)
lev 1 : 0x64 : 0000 150C -
lev 2 : 0x68 : 0000 3676 -
lev 3 : 0x6c : 0000 1752 -
lev 4 : 0x70 : 0000 1768 -
lev 5 : 0x74 : 0000 177e -
lev 6 : 0x78 : 0000 1794 -
lev 7 : 0x7c : 0000 17aa -


todo:

fix scrolling - is it related to the 0x700000 accesses? see
stephh's notes on the 0x700000 reads / writes
sprite priorities (see glass break in fitfight)
sound
fix sprite colour problems.
should these be considered clones or not since the game has
been rewritten, they just use the gfx ...

Stephh's notes :

1) 'fitfight'

  - Gameplay :
      * The player who beats the other wins a point and the round number is increased.
      * When there is a draw, nobody scores a point, but the round number is increased.
      * The level ends when a players reaches the needed number of points or when the
        round number is > the maximum round number.

  - Winner :
      * Player 1 wins if his number of points is >= player 2 number of points.
      * Player 2 wins if his number of points is >  player 1 number of points.

  - The "winner rule" introduces an ingame bug : when you play with LEFT player
    against the CPU, you can win a level by only scoring "draws".


2) 'histryma'

  - Gameplay :
      * The player who beats the other wins a point.
      * When there is a draw, both players score a point.
      * The level ends when a players reaches the needed number of points or when the
        total of points is >= the maximum number of points.

  - Winner :
      * Player 1 wins if his number of points is >  player 2 number of points.
      * Player 2 wins if his number of points is >= player 1 number of points.

  - The "winner rule" introduces an ingame bug : when you play with RIGHT player
    against the CPU, you can win a level by only scoring "draws".

  - The "test mode" isn't correct for the Dip Switches !

*/

#include "driver.h"

data16_t *fitfight_spriteram;
data16_t *fof_100000, *fof_600000, *fof_700000, *fof_800000, *fof_900000, *fof_a00000;

data16_t *fof_bak_tileram;
data16_t *fof_mid_tileram;
data16_t *fof_txt_tileram;
char bbprot_kludge;

static UINT16 fitfight_700000_data = 0;

WRITE16_HANDLER( fof_bak_tileram_w );
WRITE16_HANDLER( fof_mid_tileram_w );
WRITE16_HANDLER( fof_txt_tileram_w );
VIDEO_START(fitfight);
VIDEO_UPDATE(fitfight);

static READ16_HANDLER(fitfight_700000_r)
{
	/*
	01656A: 48E7 3000                movem.l D2-D3, -(A7)
	01656E: 4EB9 0001 698A           jsr     $1698a.l
	016574: 0240 000F                andi.w  #$f, D0
	016578: 3600                     move.w  D0, D3
	01657A: 33C3 0070 0000           move.w  D3, $700000.l
	016580: 33FC 0200 0070 0000      move.w  #$200, $700000.l
	016588: 33FC 0200 0070 0000      move.w  #$200, $700000.l
	016590: 3439 0070 0000           move.w  $700000.l, D2
	016596: 0242 00FF                andi.w  #$ff, D2
	01659A: 33F9 00E0 4C56 0070 0000 move.w  $e04c56.l, $700000.l
	0165A4: 3003                     move.w  D3, D0
	0165A6: 48C0                     ext.l   D0
	0165A8: 3202                     move.w  D2, D1
	0165AA: 48C1                     ext.l   D1
	0165AC: E481                     asr.l   #2, D1
	0165AE: B081                     cmp.l   D1, D0
	0165B0: 6702                     beq     165b4
	0165B2: 60FE                     bra     165b2			<- infinite loop
	0165B4: 4CDF 000C                movem.l (A7)+, D2-D3
	0165B8: 4E75                     rts
	*/

	/* D3 (write to 0x700000) - D2 (read from 0x700000)
	   D3 = D2 >> 2, so D2 = D3 << 2 */

	UINT16 data = fitfight_700000_data;

	return (data << 2);
}

static READ16_HANDLER(histryma_700000_r)
{
	/*
	017F82: 48E7 3800                movem.l D2-D4, -(A7)
	017F86: 33F9 00E0 7900 0070 0000 move.w  $e07900.l, $700000.l
	017F90: 4EB9 0000 74C8           jsr     $74c8.l
	017F96: 4EB9 0001 8356           jsr     $18356.l
	017F9C: 0240 00FE                andi.w  #$fe, D0
	017FA0: 3600                     move.w  D0, D3
	017FA2: 33C3 0070 0000           move.w  D3, $700000.l
	017FA8: 33FC 0200 0070 0000      move.w  #$200, $700000.l
	017FB0: 3439 0070 0000           move.w  $700000.l, D2
	017FB6: 0242 00FF                andi.w  #$ff, D2
	017FBA: 3802                     move.w  D2, D4
	017FBC: 0244 00AA                andi.w  #$aa, D4
	017FC0: 33F9 00E0 7900 0070 0000 move.w  $e07900.l, $700000.l
	017FCA: 0242 0055                andi.w  #$55, D2
	017FCE: E54A                     lsl.w   #2, D2
	017FD0: 3002                     move.w  D2, D0
	017FD2: 8044                     or.w    D4, D0
	017FD4: 48C0                     ext.l   D0
	017FD6: 3400                     move.w  D0, D2
	017FD8: B642                     cmp.w   D2, D3
	017FDA: 6702                     beq     17fde
	017FDC: 60FE                     bra     17fdc			<- infinite loop
	017FDE: 4EB9 0000 74C2           jsr     $74c2.l
	017FE4: 4CDF 001C                movem.l (A7)+, D2-D4
	017FE8: 4E75                     rts
	*/

	/* D3 (write to 0x700000) - D2 (read from 0x700000)
	   D3 = ((D2 & 0x55) << 2) | (D2 & 0xaa), so D2 = ???
	   (please help me in finding the formula to avoid the table) */

	static const UINT16 table[128] =
	{
		0x00, 0x02, 0x01, 0x03, 0x08, 0x0a, 0x09, 0x0b,		// 0x00 .. 0x0e
		0x04, 0x06, 0x05, 0x07, 0x0c, 0x0e, 0x0d, 0x0f,		// 0x10 .. 0x1e
		0x20, 0x22, 0x21, 0x23, 0x28, 0x2a, 0x29, 0x2b,		// 0x20 .. 0x2e
		0x24, 0x26, 0x25, 0x27, 0x2c, 0x2e, 0x2d, 0x2f,		// 0x30 .. 0x3e
		0x10, 0x12, 0x11, 0x13, 0x18, 0x1a, 0x19, 0x1b,		// 0x40 .. 0x4e
		0x14, 0x16, 0x15, 0x17, 0x1c, 0x1e, 0x1d, 0x1f,		// 0x50 .. 0x5e
		0x30, 0x32, 0x31, 0x33, 0x38, 0x3a, 0x39, 0x3b,		// 0x60 .. 0x6e
		0x34, 0x36, 0x35, 0x37, 0x3c, 0x3e, 0x3d, 0x3f,		// 0x70 .. 0x7e
		0x80, 0x82, 0x81, 0x83, 0x88, 0x8a, 0x89, 0x8b,		// 0x80 .. 0x8e
		0x84, 0x86, 0x85, 0x87, 0x8c, 0x8e, 0x8d, 0x8f,		// 0x90 .. 0x9e
		0xa0, 0xa2, 0xa1, 0xa3, 0xa8, 0xaa, 0xa9, 0xab,		// 0xa0 .. 0xae
		0xa4, 0xa6, 0xa5, 0xa7, 0xac, 0xae, 0xad, 0xaf,		// 0xb0 .. 0xbe
		0x90, 0x92, 0x91, 0x93, 0x98, 0x9a, 0x99, 0x9b,		// 0xc0 .. 0xce
		0x94, 0x96, 0x95, 0x97, 0x9c, 0x9e, 0x9d, 0x9f,		// 0xd0 .. 0xde
		0xb0, 0xb2, 0xb1, 0xb3, 0xb8, 0xba, 0xb9, 0xbb,		// 0xe0 .. 0xee
		0xb4, 0xb6, 0xb5, 0xb7, 0xbc, 0xbe, 0xbd, 0xbf,		// 0xf0 .. 0xfe
	};

	UINT16 data = fitfight_700000_data;

	return (table[data/2]);
}

static READ16_HANDLER(bbprot_700000_r)
{
	/*
	016FCC: 4E56 FFEC                link    A6, #-$14
	016FD0: 48E7 2030                movem.l D2/A2-A3, -(A7)
	016FD4: 45EE FFEC                lea     (-$14,A6), A2
	016FD8: 47EE FFF6                lea     (-$a,A6), A3
	016FDC: 33F9 00E0 9F68 0070 0000 move.w  $e09f68.l, $700000.l
	016FE6: 007C 0700                ori     #$700, SR
	016FEA: 4EB9 0001 74DC           jsr     $174dc.l
	016FF0: 0240 00FE                andi.w  #$fe, D0
	016FF4: 3400                     move.w  D0, D2
	016FF6: 0280 0000 0100           andi.l  #$100, D0
	016FFC: E080                     asr.l   #8, D0
	016FFE: 1D40 FFF6                move.b  D0, (-$a,A6)
	017002: 3002                     move.w  D2, D0
	017004: 0280 0000 0080           andi.l  #$80, D0
	01700A: EE80                     asr.l   #7, D0
	01700C: 1740 0001                move.b  D0, ($1,A3)
	017010: 3002                     move.w  D2, D0
	017012: 0280 0000 0040           andi.l  #$40, D0
	017018: EC80                     asr.l   #6, D0
	01701A: 1740 0002                move.b  D0, ($2,A3)
	01701E: 3002                     move.w  D2, D0
	017020: 0280 0000 0020           andi.l  #$20, D0
	017026: EA80                     asr.l   #5, D0
	017028: 1740 0003                move.b  D0, ($3,A3)
	01702C: 3002                     move.w  D2, D0
	01702E: 0280 0000 0010           andi.l  #$10, D0
	017034: E880                     asr.l   #4, D0
	017036: 1740 0004                move.b  D0, ($4,A3)
	01703A: 3002                     move.w  D2, D0
	01703C: 0280 0000 0008           andi.l  #$8, D0
	017042: E680                     asr.l   #3, D0
	017044: 1740 0005                move.b  D0, ($5,A3)
	017048: 3002                     move.w  D2, D0
	01704A: 0280 0000 0004           andi.l  #$4, D0
	017050: E480                     asr.l   #2, D0
	017052: 1740 0006                move.b  D0, ($6,A3)
	017056: 3002                     move.w  D2, D0
	017058: 0280 0000 0002           andi.l  #$2, D0
	01705E: E280                     asr.l   #1, D0
	017060: 1740 0007                move.b  D0, ($7,A3)
	017064: 1002                     move.b  D2, D0
	017066: 0200 0001                andi.b  #$1, D0
	01706A: 1740 0008                move.b  D0, ($8,A3)
	01706E: 33C2 0070 0000           move.w  D2, $700000.l
	017074: 33FC 0200 0070 0000      move.w  #$200, $700000.l
	01707C: 3439 0070 0000           move.w  $700000.l, D2
	017082: 3002                     move.w  D2, D0
	017084: 0280 0000 0100           andi.l  #$100, D0
	01708A: E080                     asr.l   #8, D0
	01708C: 1D40 FFEC                move.b  D0, (-$14,A6)
	017090: 3002                     move.w  D2, D0
	017092: 0280 0000 0080           andi.l  #$80, D0
	017098: EE80                     asr.l   #7, D0
	01709A: 1540 0001                move.b  D0, ($1,A2)
	01709E: 3002                     move.w  D2, D0
	0170A0: 0280 0000 0040           andi.l  #$40, D0
	0170A6: EC80                     asr.l   #6, D0
	0170A8: 1540 0002                move.b  D0, ($2,A2)
	0170AC: 3002                     move.w  D2, D0
	0170AE: 0280 0000 0020           andi.l  #$20, D0
	0170B4: EA80                     asr.l   #5, D0
	0170B6: 1540 0003                move.b  D0, ($3,A2)
	0170BA: 3002                     move.w  D2, D0
	0170BC: 0280 0000 0010           andi.l  #$10, D0
	0170C2: E880                     asr.l   #4, D0
	0170C4: 1540 0004                move.b  D0, ($4,A2)
	0170C8: 3002                     move.w  D2, D0
	0170CA: 0280 0000 0008           andi.l  #$8, D0
	0170D0: E680                     asr.l   #3, D0
	0170D2: 1540 0005                move.b  D0, ($5,A2)
	0170D6: 3002                     move.w  D2, D0
	0170D8: 0280 0000 0004           andi.l  #$4, D0
	0170DE: E480                     asr.l   #2, D0
	0170E0: 1540 0006                move.b  D0, ($6,A2)
	0170E4: 3002                     move.w  D2, D0
	0170E6: 0280 0000 0002           andi.l  #$2, D0
	0170EC: E280                     asr.l   #1, D0
	0170EE: 1540 0007                move.b  D0, ($7,A2)
	0170F2: 1002                     move.b  D2, D0
	0170F4: 0200 0001                andi.b  #$1, D0
	0170F8: 1540 0008                move.b  D0, ($8,A2)
	0170FC: 33F9 00E0 9F68 0070 0000 move.w  $e09f68.l, $700000.l
	017106: 102B 0008                move.b  ($8,A3), D0
	01710A: B02A 0008                cmp.b   ($8,A2), D0
	01710E: 6650                     bne     17160
	017110: 102B 0007                move.b  ($7,A3), D0
	017114: B02A 0007                cmp.b   ($7,A2), D0
	017118: 6646                     bne     17160
	01711A: 102B 0004                move.b  ($4,A3), D0
	01711E: B02A 0006                cmp.b   ($6,A2), D0
	017122: 663C                     bne     17160
	017124: 102B 0005                move.b  ($5,A3), D0
	017128: B02A 0005                cmp.b   ($5,A2), D0
	01712C: 6632                     bne     17160
	01712E: 102B 0002                move.b  ($2,A3), D0
	017132: B02A 0004                cmp.b   ($4,A2), D0
	017136: 6628                     bne     17160
	017138: 102B 0001                move.b  ($1,A3), D0
	01713C: B02A 0003                cmp.b   ($3,A2), D0
	017140: 661E                     bne     17160
	017142: 102E FFF6                move.b  (-$a,A6), D0
	017146: B02A 0002                cmp.b   ($2,A2), D0
	01714A: 6614                     bne     17160
	01714C: 102B 0003                move.b  ($3,A3), D0
	017150: B02A 0001                cmp.b   ($1,A2), D0
	017154: 660A                     bne     17160
	017156: 102B 0006                move.b  ($6,A3), D0
	01715A: B02E FFEC                cmp.b   (-$14,A6), D0
	01715E: 6702                     beq     17162
	017160: 60FE                     bra     17160
	017162: 027C F9FF                andi    #$f9ff, SR
	017166: 4CEE 0C04 FFE0           movem.l (-$20,A6), D2/A2-A3
	01716C: 4E5E                     unlk    A6
	01716E: 4E75                     rts
	*/

	/* A3 = offset of value written to 0x700000.w
         A2 = offset of value read from 0x700000.w
	   (-$a,A6) = $0,A3 and (-$14,A6) = $0,A2

	     write bits   8 7 6 5 4 3 2 1 0    =    read bits    6 5 4 7 2 3 8 1 0
	     A3 offset    0 1 2 3 4 5 6 7 8         A2 offset    2 3 4 1 6 5 0 7 8
	                                      <=>
	     read bits    8 7 6 5 4 3 2 1 0    =    write bits   2 5 8 7 6 3 4 1 0
	     A2 offset    0 1 2 3 4 5 6 7 8         A3 offset    6 3 0 1 2 5 4 7 8

	   Comparaison between bits of what is read and bits of what is written :
	     - bits 0, 1 & 3 are the same
	     - bits 4, 6, 7 & 8 are >> 2
	     - bit  2 is << 6
	     - bit  5 is << 2
	*/

	UINT16 data = 0;

	data  =  (fitfight_700000_data & 0x000b);
	data |= ((fitfight_700000_data & 0x01d0) >> 2);
	data |= ((fitfight_700000_data & 0x0004) << 6);
	data |= ((fitfight_700000_data & 0x0020) << 2);

	return (data);
}

static WRITE16_HANDLER(fitfight_700000_w)
{
	COMBINE_DATA(&fof_700000[offset]);		// really needed for scrolling ?
	if (data < 0x0200)				// to avoid considering writes of 0x0200
		fitfight_700000_data = data;
}

static MEMORY_READ16_START( fitfight_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },

	{ 0x200000, 0x200001, input_port_0_word_r },
	{ 0x300000, 0x300001, input_port_1_word_r },	// for 'histryma' only
	{ 0x400000, 0x400001, input_port_2_word_r },
	{ 0x500000, 0x500001, input_port_3_word_r },
//	{ 0x700000, 0x700001, xxxx }, /* see init */

	{ 0xb00000, 0xb0ffff, MRA16_RAM },
	{ 0xc00000, 0xc003ff, MRA16_RAM },
	{ 0xc00400, 0xc00fff, MRA16_RAM },
	{ 0xd00000, 0xd007ff, MRA16_RAM },
	{ 0xe00000, 0xe0ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( fitfight_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },

	{ 0x100000, 0x100001, MWA16_RAM, &fof_100000 },

	{ 0xb00000, 0xb03fff,  MWA16_RAM }, /* unused layer? */
	{ 0xb04000, 0xb07fff,  fof_bak_tileram_w, &fof_bak_tileram },
	{ 0xb08000, 0xb0bfff,  fof_mid_tileram_w, &fof_mid_tileram },
	{ 0xb0c000, 0xb0ffff,  fof_txt_tileram_w, &fof_txt_tileram },

	{ 0x600000, 0x600001, MWA16_RAM, &fof_600000 },
	{ 0x700000, 0x700001, fitfight_700000_w, &fof_700000 },
	{ 0x800000, 0x800001, MWA16_RAM, &fof_800000 },
	{ 0x900000, 0x900001, MWA16_RAM, &fof_900000 },
	{ 0xa00000, 0xa00001, MWA16_RAM, &fof_a00000 },

	{ 0xc00000, 0xc00fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0xd00000, 0xd007ff, MWA16_RAM, &fitfight_spriteram },
	{ 0xe00000, 0xe0ffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( bbprot_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },

	{ 0x300000, 0x300001, input_port_0_word_r },
	{ 0x380000, 0x380001, input_port_1_word_r },
	{ 0x400000, 0x400001, input_port_2_word_r },
	{ 0x480000, 0x480001, input_port_3_word_r },
	{ 0x700000, 0x700001, bbprot_700000_r },

	{ 0xb00000, 0xb0ffff, MRA16_RAM },
	{ 0xc00000, 0xc003ff, MRA16_RAM },
	{ 0xc00400, 0xc00fff, MRA16_RAM },
	{ 0xd00000, 0xd007ff, MRA16_RAM },
	{ 0xe00000, 0xe0ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( bbprot_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },

	{ 0x100000, 0x100001, MWA16_RAM, &fof_100000 },

	{ 0xb00000, 0xb03fff,  MWA16_RAM }, /* unused layer? */
	{ 0xb04000, 0xb07fff,  fof_bak_tileram_w, &fof_bak_tileram },
	{ 0xb08000, 0xb0bfff,  fof_mid_tileram_w, &fof_mid_tileram },
	{ 0xb0c000, 0xb0ffff,  fof_txt_tileram_w, &fof_txt_tileram },

	{ 0x600000, 0x600001, MWA16_RAM, &fof_600000 },
	{ 0x700000, 0x700001, fitfight_700000_w, &fof_700000 },
	{ 0x800000, 0x800001, MWA16_RAM, &fof_800000 },
	{ 0x900000, 0x900001, MWA16_RAM, &fof_900000 },
	{ 0xa00000, 0xa00001, MWA16_RAM, &fof_a00000 },

	{ 0xc00000, 0xc03fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0xd00000, 0xd007ff, MWA16_RAM, &fitfight_spriteram },
	{ 0xe00000, 0xe0ffff, MWA16_RAM },
MEMORY_END

/* I've put the inputs the same way they can be read in the "test mode" */

INPUT_PORTS_START( fitfight )
	PORT_START	// 0x200000.w
	/* players inputs -> 0xe022cc.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	// 0x300000.w (unused)
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// 0x400000.w
	/* LSB : system inputs -> 0xe022cf.b */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )			// "Test"
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )			// "Fault" (= "Tilt" ?)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* MSB : SW2 -> 0xe04c26.b (cpl) */
	PORT_DIPNAME( 0xf800, 0xf800, "Time" )
	PORT_DIPSETTING(      0xf000, "02" )
	PORT_DIPSETTING(      0xe800, "05" )
	PORT_DIPSETTING(      0xe000, "08" )
	PORT_DIPSETTING(      0xd800, "11" )
	PORT_DIPSETTING(      0xd000, "14" )
	PORT_DIPSETTING(      0xc800, "17" )
	PORT_DIPSETTING(      0xc000, "20" )
	PORT_DIPSETTING(      0xb800, "23" )
	PORT_DIPSETTING(      0xb000, "26" )
	PORT_DIPSETTING(      0xa800, "29" )
	PORT_DIPSETTING(      0xa000, "32" )
	PORT_DIPSETTING(      0x9800, "35" )
	PORT_DIPSETTING(      0x9000, "38" )
	PORT_DIPSETTING(      0x8800, "41" )
	PORT_DIPSETTING(      0x8000, "44" )
	PORT_DIPSETTING(      0x7800, "47" )
	PORT_DIPSETTING(      0x7000, "50" )
	PORT_DIPSETTING(      0x6800, "53" )
	PORT_DIPSETTING(      0x6000, "56" )
	PORT_DIPSETTING(      0x5800, "59" )
	PORT_DIPSETTING(      0x5000, "62" )
	PORT_DIPSETTING(      0x4800, "65" )
	PORT_DIPSETTING(      0x4000, "68" )
	PORT_DIPSETTING(      0x3800, "71" )
	PORT_DIPSETTING(      0x3000, "74" )
	PORT_DIPSETTING(      0x2800, "77" )
	PORT_DIPSETTING(      0x2000, "80" )
	PORT_DIPSETTING(      0x1800, "83" )
	PORT_DIPSETTING(      0x1000, "86" )
	PORT_DIPSETTING(      0x0800, "89" )
	PORT_DIPSETTING(      0x0000, "92" )
	PORT_DIPSETTING(      0xf800, "99" )
	PORT_DIPNAME( 0x0700, 0x0700, "First Credit" )
	PORT_DIPSETTING(      0x0000, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )

	PORT_START	// 0x500000.w
	/* MSB : SW3 -> 0xe04c24.b (cpl) */
	PORT_DIPNAME( 0xe000, 0xe000, "Next Credit" )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1c00, 0x1000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x1c00, "Easiest" )
	PORT_DIPSETTING(      0x1800, "Easier" )
	PORT_DIPSETTING(      0x1400, "Easy" )
	PORT_DIPSETTING(      0x1000, "Normal" )
	PORT_DIPSETTING(      0x0c00, "Medium" )
	PORT_DIPSETTING(      0x0800, "Hard" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0200, 0x0000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( On ) )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	/* LSB : SW1 -> 0xe04c25.b (cpl) */
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )	// To be confirmed
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0070, 0x0060, "Needed Points/Maximum Rounds" )	// see notes
//	PORT_DIPSETTING(      0x0070, "Endless" )
	PORT_DIPSETTING(      0x0060, "1/2" )
	PORT_DIPSETTING(      0x0050, "2/3" )
	PORT_DIPSETTING(      0x0040, "2/4" )
	PORT_DIPSETTING(      0x0030, "3/5" )
	PORT_DIPSETTING(      0x0020, "3/6" )
	PORT_DIPSETTING(      0x0010, "4/7" )
	PORT_DIPSETTING(      0x0000, "4/8" )
	PORT_DIPNAME( 0x0008, 0x0000, "Select All Players" )		// in a 1 player game
	PORT_DIPSETTING(      0x0008, DEF_STR( No ) )			// only Ryo and Robert available
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )		// must be Off during P.O.S.T. !
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( histryma )
	PORT_START	// 0x200000.w
	/* players inputs -> 0xe02cf2.w and 0xe02cf8.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	// 0x300000.w
	/* LSB : players extra inputs -> 0xe02cf5.b and 0xe02cfb.b */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* MSB : unused */
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// 0x400000.w
	/* LSB : system inputs -> 0xe02cf7.b and 0xe02cfd.b */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )			// "Test"
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )			// "Fault" (= "Tilt" ?)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )			// "Test" (duplicated)
	/* MSB : SW2 -> 0xe05874.b (cpl) */
	PORT_DIPNAME( 0xf800, 0x0000, "Time" )
//	PORT_DIPSETTING(      0xf800, "15" )				// duplicated setting
//	PORT_DIPSETTING(      0xf000, "15" )				// duplicated setting
//	PORT_DIPSETTING(      0xe800, "15" )				// duplicated setting
	PORT_DIPSETTING(      0xe000, "15" )
	PORT_DIPSETTING(      0xd800, "18" )
	PORT_DIPSETTING(      0xd000, "21" )
	PORT_DIPSETTING(      0xc800, "24" )
	PORT_DIPSETTING(      0xc000, "27" )
	PORT_DIPSETTING(      0xb800, "30" )
	PORT_DIPSETTING(      0xb000, "33" )
	PORT_DIPSETTING(      0xa800, "36" )
	PORT_DIPSETTING(      0xa000, "39" )
	PORT_DIPSETTING(      0x9800, "42" )
	PORT_DIPSETTING(      0x9000, "45" )
	PORT_DIPSETTING(      0x8800, "48" )
	PORT_DIPSETTING(      0x8000, "51" )
	PORT_DIPSETTING(      0x7800, "54" )
	PORT_DIPSETTING(      0x7000, "57" )
	PORT_DIPSETTING(      0x6800, "60" )
	PORT_DIPSETTING(      0x6000, "63" )
	PORT_DIPSETTING(      0x5800, "66" )
	PORT_DIPSETTING(      0x5000, "69" )
	PORT_DIPSETTING(      0x4800, "72" )
	PORT_DIPSETTING(      0x4000, "75" )
	PORT_DIPSETTING(      0x3800, "78" )
	PORT_DIPSETTING(      0x3000, "81" )
	PORT_DIPSETTING(      0x2800, "84" )
	PORT_DIPSETTING(      0x2000, "87" )
	PORT_DIPSETTING(      0x1800, "90" )
	PORT_DIPSETTING(      0x1000, "93" )
	PORT_DIPSETTING(      0x0800, "96" )
	PORT_DIPSETTING(      0x0000, "99" )
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )

	PORT_START	// 0x500000.w
	/* MSB : SW3 -> 0xe05872.b (cpl) */
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1c00, 0x1000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x1c00, "Easiest" )
	PORT_DIPSETTING(      0x1800, "Easier" )
	PORT_DIPSETTING(      0x1400, "Easy" )
	PORT_DIPSETTING(      0x1000, "Normal" )
	PORT_DIPSETTING(      0x0c00, "Medium" )
	PORT_DIPSETTING(      0x0800, "Hard" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0200, 0x0000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( On ) )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	/* LSB : SW1 -> 0xe05873.b (cpl) */
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )	// To be confirmed
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0070, 0x0060, "Needed Points/Maximum Points" )	// see notes
//	PORT_DIPSETTING(      0x0070, "Endless" )				// ends on a draw
	PORT_DIPSETTING(      0x0060, "1/2" )
	PORT_DIPSETTING(      0x0050, "2/3" )
	PORT_DIPSETTING(      0x0040, "2/4" )
	PORT_DIPSETTING(      0x0030, "3/5" )
	PORT_DIPSETTING(      0x0020, "3/6" )
	PORT_DIPSETTING(      0x0010, "4/7" )
	PORT_DIPSETTING(      0x0000, "4/8" )
	PORT_DIPNAME( 0x0008, 0x0000, "Buttons" )				// 3 or 6 buttons as default ?
	PORT_DIPSETTING(      0x0008, "3" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )		// must be Off during P.O.S.T. !
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/* Check inputs again when video emulation is better */
/* Surprisingly, the Dip Switches look very similar to the ones from 'histryma'
   (the only difference being that there is no "Needed Points/Maximum Points"
   Dip Switch, the value always being set to "2/3") */
INPUT_PORTS_START( bbprot )
	PORT_START	// 0x300000.w
	/* players inputs -> 0xe0545e.w and 0xe05464.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	// 0x380000.w
	/* LSB : players extra inputs -> 0xe05461.b and 0xe05467.b */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* MSB : unused */
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// 0x400000.w
	/* LSB : system inputs -> 0xe05463.b and 0xe05469.b */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )			// "Test"
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )			// "Fault" (= "Tilt" ?)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )			// "Test" (duplicated)
	/* MSB : SW2 -> 0xe07e84.b (cpl) */
	PORT_DIPNAME( 0xf800, 0x0000, "Time" )
//	PORT_DIPSETTING(      0xf800, "15" )				// duplicated setting
//	PORT_DIPSETTING(      0xf000, "15" )				// duplicated setting
//	PORT_DIPSETTING(      0xe800, "15" )				// duplicated setting
	PORT_DIPSETTING(      0xe000, "15" )
	PORT_DIPSETTING(      0xd800, "18" )
	PORT_DIPSETTING(      0xd000, "21" )
	PORT_DIPSETTING(      0xc800, "24" )
	PORT_DIPSETTING(      0xc000, "27" )
	PORT_DIPSETTING(      0xb800, "30" )
	PORT_DIPSETTING(      0xb000, "33" )
	PORT_DIPSETTING(      0xa800, "36" )
	PORT_DIPSETTING(      0xa000, "39" )
	PORT_DIPSETTING(      0x9800, "42" )
	PORT_DIPSETTING(      0x9000, "45" )
	PORT_DIPSETTING(      0x8800, "48" )
	PORT_DIPSETTING(      0x8000, "51" )
	PORT_DIPSETTING(      0x7800, "54" )
	PORT_DIPSETTING(      0x7000, "57" )
	PORT_DIPSETTING(      0x6800, "60" )
	PORT_DIPSETTING(      0x6000, "63" )
	PORT_DIPSETTING(      0x5800, "66" )
	PORT_DIPSETTING(      0x5000, "69" )
	PORT_DIPSETTING(      0x4800, "72" )
	PORT_DIPSETTING(      0x4000, "75" )
	PORT_DIPSETTING(      0x3800, "78" )
	PORT_DIPSETTING(      0x3000, "81" )
	PORT_DIPSETTING(      0x2800, "84" )
	PORT_DIPSETTING(      0x2000, "87" )
	PORT_DIPSETTING(      0x1800, "90" )
	PORT_DIPSETTING(      0x1000, "93" )
	PORT_DIPSETTING(      0x0800, "96" )
	PORT_DIPSETTING(      0x0000, "99" )
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )

	PORT_START	// 0x480000.w
	/* MSB : SW3 -> 0xe07e82.b (cpl) */
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1c00, 0x1000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x1c00, "Easiest" )
	PORT_DIPSETTING(      0x1800, "Easier" )
	PORT_DIPSETTING(      0x1400, "Easy" )
	PORT_DIPSETTING(      0x1000, "Normal" )
	PORT_DIPSETTING(      0x0c00, "Medium" )
	PORT_DIPSETTING(      0x0800, "Hard" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0200, 0x0000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( On ) )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	/* LSB : SW1 -> 0xe07e83.b (cpl) */
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )	// To be confirmed
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, "Buttons" )				// 3 or 6 buttons as default ?
	PORT_DIPSETTING(      0x0008, "3" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )		// must be Off during P.O.S.T. !
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout fof_tile_layout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ 0,RGN_FRAC(1,4),RGN_FRAC(2,4),RGN_FRAC(3,4) },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout fof_sprite_layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ 0,RGN_FRAC(1,4),RGN_FRAC(2,4),RGN_FRAC(3,4) },
	{ 0,1,2,3,4,5,6,7,8*8+0,8*8+1,8*8+2,8*8+3,8*8+4,8*8+5,8*8+6,8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	16*8+0*8,16*8+1*8,16*8+2*8,16*8+3*8,16*8+4*8,16*8+5*8,16*8+6*8,16*8+7*8

	},
	16*16
};

static struct GfxLayout bbprot_sprite_layout =
{
	16,16,
	RGN_FRAC(1,5),
	5,
	{ 0,RGN_FRAC(1,5),RGN_FRAC(2,5),RGN_FRAC(3,5),RGN_FRAC(4,5) },
	{ 0,1,2,3,4,5,6,7,8*8+0,8*8+1,8*8+2,8*8+3,8*8+4,8*8+5,8*8+6,8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	16*8+0*8,16*8+1*8,16*8+2*8,16*8+3*8,16*8+4*8,16*8+5*8,16*8+6*8,16*8+7*8

	},
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fof_tile_layout,   0x000, 256  }, /* tx tiles */
	{ REGION_GFX1, 0, &fof_tile_layout,   0x200, 256  }, /* mid tiles */
	{ REGION_GFX1, 0, &fof_tile_layout,   0x400, 256  }, /* bg tiles */
	{ REGION_GFX2, 0, &fof_sprite_layout, 0x600, 256  }, /* sprites */

	{ -1 } /* end of array */
};

static struct GfxDecodeInfo prot_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fof_tile_layout,     0x0000, 256  }, /* tx tiles */
	{ REGION_GFX1, 0, &fof_tile_layout,     0x0800, 256  }, /* mid tiles */
	{ REGION_GFX1, 0, &fof_tile_layout,     0x1000, 256  }, /* bg tiles */
	{ REGION_GFX2, 0, &bbprot_sprite_layout,0x1800, 256  }, /* sprites */

	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( fitfight )
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(fitfight_readmem,fitfight_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(2*8, 39*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(fitfight)
	MDRV_VIDEO_UPDATE(fitfight)

//	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bbprot )
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(bbprot_readmem,bbprot_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(prot_gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(2*8, 39*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x2000)

	MDRV_VIDEO_START(fitfight)
	MDRV_VIDEO_UPDATE(fitfight)

//	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/***

Here's the info about this dump:

Name:            Fit of Fighting
Manufacturer:    Unknow (There are chances that was produced by NIX, but it's not                          possible to verify)
Year:            Unknow
Date Dumped:     16-07-2002 (DD-MM-YYYY)

CPU:             68000, possibly at 12mhz
SOUND:           OKIM6295
GFX:             Unknown

About the game:

This game is a very horrible Art of Fighting rip-off, ripped graphics but
reprogrammed from 0, FM music, no zooms, no damage in the fighters faces, poor IA,
poor gameplay (you have to execute the special attacks very slowly to get them
running!), but incredibly fun to see such a thing :) Hope you enjoy it!

***/

ROM_START( fitfight )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "u138_ff1.bin", 0x000001, 0x080000, 0x165600fe )
	ROM_LOAD16_BYTE( "u125_ff1.bin", 0x000000, 0x080000, 0x2f9bdb66 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) /* Sound Program? */
	ROM_LOAD( "u23_ff1.bin",  0x000000, 0x010000, 0xe2d6d768 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* OKI Samples? */
	ROM_LOAD( "h7e_ff1.bin",  0x000000, 0x080000, 0x3e12dfd8 )
	ROM_LOAD( "h18e_ff1.bin", 0x080000, 0x080000, 0xa7f36dbe )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "p1_ff1.bin",   0x0c0000, 0x040000, 0x542593b3 )
	ROM_LOAD( "p2_ff1.bin",   0x080000, 0x040000, 0xfc517470 )
	ROM_LOAD( "p4_ff1.bin",   0x040000, 0x040000, 0xa8754268 )
	ROM_LOAD( "p8_ff1.bin",   0x000000, 0x040000, 0xbd55182a )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "s1_ff1.bin",   0x300000, 0x080000, 0x90a57445 )
	ROM_LOAD( "s1_ff1h.bin",  0x380000, 0x080000, 0x07e23f95 )

	ROM_LOAD( "s2_ff1.bin",   0x200000, 0x080000, 0xee4a972b )
	ROM_LOAD( "s2_ff1h.bin",  0x280000, 0x080000, 0x726add2e )

	ROM_LOAD( "s4_ff1.bin",   0x100000, 0x080000, 0xcfdcbdfb )
	ROM_LOAD( "s4_ff1h.bin",  0x180000, 0x080000, 0xeecce2d7 )

	ROM_LOAD( "s8_ff1.bin",   0x000000, 0x080000, 0x0edf5706 )
	ROM_LOAD( "s8_ff1h.bin",  0x080000, 0x080000, 0x1d00074f )
ROM_END

/***

Name:            The History of Martial Arts
Manufacturer:    Unknow, maybe NIX / Novatecnia
Year:            Unknow
Date Dumped:     18-07-2002 (DD-MM-YYYY)

CPU:             68000
SOUND:           OKIM6295
GFX:             Unknown

Country:         Maybe Spain

About the game:

This is a Karnov's revenge ripp off like Fit of Fighting with Art of Fighting.
Same GFX, but reprogrammed from 0, and with FM music... Another nice bootleg!
It was dumped from a faulty board, wich doesn't boot, but with intact eproms :)

***/

ROM_START( histryma )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "l_th.bin", 0x000001, 0x080000, 0x5af9356a )
	ROM_LOAD16_BYTE( "r_th.bin", 0x000000, 0x080000, 0x1a44b504 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) /* Sound Program? */
	ROM_LOAD( "y61f.bin",  0x000000, 0x010000, 0xb588525a )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* OKI Samples? */
	ROM_LOAD( "u7_th.bin",  0x000000, 0x080000, 0x88b41ef5 )
	ROM_LOAD( "u18_th.bin", 0x080000, 0x080000, 0xa734cd77 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "p1_th.bin",   0x0c0000, 0x040000, 0x501c5336 )
	ROM_LOAD( "p2_th.bin",   0x080000, 0x040000, 0xf50666c7 )
	ROM_LOAD( "p4_th.bin",   0x040000, 0x040000, 0xc70223cf )
	ROM_LOAD( "p8_th.bin",   0x000000, 0x040000, 0x8104b963 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "s1_th.bin",   0x300000, 0x080000, 0xe9c2d27a )
	ROM_LOAD( "s1h_th.bin",  0x380000, 0x080000, 0xd806c92e )

	ROM_LOAD( "s2_th.bin",   0x200000, 0x080000, 0xfa011056 )
	ROM_LOAD( "s2h_th.bin",  0x280000, 0x080000, 0xef5f2268 )

	ROM_LOAD( "s4_th.bin",   0x100000, 0x080000, 0xfa80fdec )
	ROM_LOAD( "s4h_th.bin",  0x180000, 0x080000, 0x0fd3b43e )

	ROM_LOAD( "s8_th.bin",   0x000000, 0x080000, 0x57fd170f )
	ROM_LOAD( "s8h_th.bin",  0x080000, 0x080000, 0xcd7bd0de )
ROM_END

/***

Here's the info about this dump:

Name:            "BB" (Protoype name in some of the EPROM stickers)
Manufacturer:    Unknow (There are chances that was produced by NIX, but it's not                          possible to verify, same as Fit of Fighting )
Year:            Unknow
Date Dumped:     17-07-2002 (DD-MM-YYYY)

CPU:             68000, possibly at 12mhz
SOUND:           OKIM6295 (It is present in the board, but it's not used,
                           this prototype game does not have sound)
GFX:             Unknown

About the game:

This is a prototype in VERY early stages of development. Maybe it was coded
to gain some experience to be able to make Fit of Fighting bootleg.  A lot of
missing/random graphics, no sound, no game itself, just jump, move, crouch... :)
Badly coded scroll, no screen when you power on the board (just start pushing first
button, coin and start to get into a game) or leave it some seconds to see a
dramatically lame fight)... Character selection just shows garbage, maybe stage
selection shows rubbish, and the game itself is all the time displaying a lot of
garbage.. But this is VERY nice! :) The most funny thing about this game is that
the three visible fighter characters can be based on some of the NIX workers, but
i really can't verify this :)

Some of the eproms seem to be two times inserted in different slots of the board,
but with a different date wrote on the stickers, those are the ??_DD_MM.bin files.

***/

ROM_START( bbprot )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "l_bb.bin", 0x000001, 0x080000, 0x2b7b9a9a )
	ROM_LOAD16_BYTE( "r_bb.bin", 0x000000, 0x080000, 0x28480f3e )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* BG GFX */
	ROM_LOAD( "p1_29_11.bin",   0x180000, 0x080000, 0xe7da36c1 )
	ROM_LOAD( "p2_29_11.bin",   0x100000, 0x080000, 0x0411e1aa )
	ROM_LOAD( "p4_29_11.bin",   0x080000, 0x080000, 0x885942bf )
	ROM_LOAD( "p8_29_11.bin",   0x000000, 0x080000, 0x44f94575 )

	ROM_REGION( 0x780000, REGION_GFX2, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "s1_21_12.bin",  0x600000, 0x080000, 0x0b396256 )
	ROM_LOAD( "s1_x.bin",      0x700000, 0x080000, 0xb95dfbb8 )
	ROM_LOAD( "s1_h.bin",      0x680000, 0x080000, 0xd20b6ac3 )

	ROM_LOAD( "s2_21_12.bin",  0x480000, 0x080000, 0x46e8b73c )
	ROM_LOAD( "s2_x.bin",      0x580000, 0x080000, 0xc90a52e8 )
	ROM_LOAD( "s2_h.bin",      0x500000, 0x080000, 0x7970be11 )

	ROM_LOAD( "s4_21_12.bin",  0x300000, 0x080000, 0xf46d47bd )
	ROM_LOAD( "s4_x.bin",      0x400000, 0x080000, 0x0fe4325d )
	ROM_LOAD( "s4_h.bin",      0x380000, 0x080000, 0x32a0bbb2 )

	ROM_LOAD( "s8_21_12.bin",  0x180000, 0x080000, 0xf810567c )
	ROM_LOAD( "s8_x.bin",      0x280000, 0x080000, 0x6ec466ea )
	ROM_LOAD( "s8_h.bin",      0x200000, 0x080000, 0xa425cc5b )

	ROM_LOAD( "s16_21_1.bin",  0x000000, 0x080000, 0x023c1e63 )
	ROM_LOAD( "s16_x_mc.bin",  0x100000, 0x080000, 0x1ad5447f )
	ROM_LOAD( "s16_h_mc.bin",  0x080000, 0x080000, 0x3b9091de )
ROM_END

/* INIT */

static DRIVER_INIT( fitfight )
{
//	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
//	mem16[0x0165B2/2]=0x4e71; // for now so it boots
	install_mem_read16_handler (0, 0x700000, 0x700001, fitfight_700000_r);
	bbprot_kludge = 0;
}

static DRIVER_INIT( histryma )
{
//	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
//	mem16[0x017FDC/2]=0x4e71; // for now so it boots
	install_mem_read16_handler (0, 0x700000, 0x700001, histryma_700000_r);
	bbprot_kludge = 0;
}

static DRIVER_INIT( bbprot )
{
	bbprot_kludge = 1;
}

/* GAME */

GAMEX(199?, fitfight, 0, fitfight, fitfight, fitfight, ROT0, "bootleg", "Fit of Fighting", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX(199?, histryma, 0, fitfight, histryma, histryma, ROT0, "bootleg", "The History of Martial Arts", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX(199?, bbprot,   0, bbprot,   bbprot,   bbprot,   ROT0, "unknown", "Untitled Fighter 'BB' (prototype)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
