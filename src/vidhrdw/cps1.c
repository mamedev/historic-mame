/***************************************************************************

These are some of the CPS-B chip numbers:

NAME                                        CPS-B #                     C-board PAL's  B-board PAL's
Forgotten Worlds / Lost Worlds              CPS-B-01  ?                 None           ? & LW10
Ghouls 'n Ghosts                            CPS-B-01  DL-0411-10001     None           DM620, LW10
Strider                                     CPS-B-01  DL-0411-10001     None           ST24N1 & LW10
Final Fight (World / Japan)                 CPS-B-04  DL-0411-10005     None           ?
Final Fight (US)                            CPS-B-04* DL-0411-10001     None           S224B & LW10
                                            *the original number was scratched out and "04" stamped over it
UN Squadron / Area 88                       CPS-B-11  DL-0411-10004     None           AR24B & LW10
Mercs (US)                                  CPS-B-12  DL-0411-10007     C628           0224B & 10B1
Magic Sword (US)                            CPS-B-13  ?                 None           ?
Carrier Air Wing                            CPS-B-16  DL-0411-10011     ?              ?
Street Fighter II                           CPS-B-17  DL-0411-10012     C632           STF29 & 10B1
Street Fighter II (US Revision I)           CPS-B-14  DL-0411-10029     C632           ?
Captain Commando*(US)                       CPS-B-21  DL-0921-10014     10C1 & C632    CC63B, CCPRG & 10B1
King of Dragons*                            CPS-B-21  DL-0921-10014     10C1 & C632    KD29B & 10B1
Knights of the Round*                       CPS-B-21  DL-0921-10014     10C1 & C632    KR63B, BPRG1 & 10B1
Street Fighter II' Champion Edition         CPS-B-21  DL-0921-10014     10C1 & C632    ?
Street Fighter II Turbo Hyper Fighting      CPS-B-21  DL-0921-10014     10C1 & C632    S9263B, BPRG1 & 10B1
Warriors of Fate*                           CPS-B-21  DL-0921-10014     10C1           TK263B, BPRG1 & 10B1
Saturday Night Slam Masters*                CPS-B-21  DL-0921-10014     10C1           MB63B, BPRG1 & 10B1

*denotes Suicide Battery



OUTPUT PORTS
0x00-0x01     OBJ RAM base (/256)
0x02-0x03     Scroll1 (8x8) RAM base (/256)
0x04-0x05     Scroll2 (16x16) RAM base (/256)
0x06-0x07     Scroll3 (32x32) RAM base (/256)
0x08-0x09     rowscroll RAM base (/256)
0x0a-0x0b     Palette base (/256)
0x0c-0x0d     Scroll 1 X
0x0e-0x0f     Scroll 1 Y
0x10-0x11     Scroll 2 X
0x12-0x13     Scroll 2 Y
0x14-0x15     Scroll 3 X
0x16-0x17     Scroll 3 Y
0x18-0x19     Starfield 1 X
0x1a-0x1b     Starfield 1 Y
0x1c-0x1d     Starfield 2 X
0x1e-0x1f     Starfield 2 Y
0x20-0x21     start offset for the rowscroll matrix
0x22-0x23     unknown but widely used - usually 0x0e. bit 0 enables rowscroll
              on layer 2. bit 15 is flip screen.


Some registers move from game to game.. following example strider
0x66-0x67	Layer control register
			bits 14-15 seem to be unused
			bits 6-13 (4 groups of 2 bits) select layer draw order
			bits 1-5 enable the three tilemap layers and the two starfield
				layers (the bit order changes from game to game).
				Only Forgotten Worlds and Strider use the starfield.
			bit 0 could be rowscroll related. It is set by bionic commando,
			varth, mtwins, mssword, cawing while rowscroll is active. However
			kodj and sf2 do NOT set this bit while they are using rowscroll.
0x68-0x69	Priority mask \   Tiles in the layer just below sprites can have
0x6a-0x6b	Priority mask |   four priority levels, each one associated with one
0x6c-0x6d	Priority mask |   of these masks. The masks indicate pens in the tile
0x6e-0x6f	Priority mask /   that have priority over sprites.
0x70-0x71	Control register (usually 0x003f). The function of this register
            is unknown; experiments on the real board show that values
			different from 0x3f in the low 6 bits cause wrong colors. The
			other bits seem to be unused.
			The only place where this register seems to be set to a value
			different from 0x3f is during startup tests. Examples:
			ghouls  0x02
			strider 0x02
			unsquad 0x0f
			kod     0x0f
			mtwins  0x0f

Fixed registers
0x80-0x81     Sound command
0x88-0x89     Sound fade

Known Bug List
==============
All games
* There might be problems if high priority tiles (over sprites) and rowscroll
are used at the same time, because the priority buffer is not rowscrolled.
The only place I know were this might cause problems is the cave in mtwins,
which waves to simulate heat. I haven't noticed anything wrong, though.

CPS2:
* CPS2 can do raster effects, certainly used by ssf2 (Cammy and DeeJay levels)
  and maybe others (xmcota?). IRQ4 is some sort of scanline interrupt used for
  that purpose.

SF2
* Missing chain in the foreground in Ken's level, and sign in Cun Li's level.
  Those graphics are in the backmost layer.

Magic Sword.
* during attract mode, characters are shown with a black background. There is
a background, but the layers are disabled. I think this IS the correct
behaviour.

King of Dragons (World).
* Distortion effect missing on character description screen during attract
mode. The game rapidly toggles on and off the layer enable bit. Again, I
think this IS the correct behaviour. The Japanese version does the
distortion as expected.

3wonders
* one bad tile at the end of level 1
* writes to output ports 42, 44, 46.

qad
* layer enable mask incomplete

wof
* In round 8, when the player goes over a bridge, there is a problem with
some sprites. When an enemy falls to the floor near the edge of the bridge,
parts of it become visible under the bridge.


Unknown issues
==============

There are often some redundant high bits in the scroll layer's attributes.
I think that these are spare bits that the game uses for to store additional
information, not used by the hardware.
The games seem to use them to mark platforms, kill zones and no-go areas.

***************************************************************************/

#ifndef SELF_INCLUDE

#include "driver.h"
#include "vidhrdw/generic.h"
#include "drivers/cps1.h"

#define VERBOSE 0

#define CPS1_DUMP_VIDEO 0

#if MAME_DEBUG
extern int cps_tileviewer_start(void);
extern void cps_tileviewer_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void cps_tileviewer_stop(void);
static int cps_view_tiles=0;
#endif

/********************************************************************

			Configuration table:

********************************************************************/

/* Game specific data */
struct CPS1config
{
	char *name;             /* game driver name */

	/* Some games interrogate a couple of registers on bootup. */
	/* These are CPS1 board B self test checks. They wander from game to */
	/* game. */
	int cpsb_addr;        /* CPS board B test register address */
	int cpsb_value;       /* CPS board B test register expected value */

	/* some games use as a protection check the ability to do 16-bit multiplies */
	/* with a 32-bit result, by writing the factors to two ports and reading the */
	/* result from two other ports. */
	/* It looks like this feature was introduced with 3wonders (CPSB ID = 08xx) */
	int mult_factor1;
	int mult_factor2;
	int mult_result_lo;
	int mult_result_hi;

	int layer_control;
	int priority0;
	int priority1;
	int priority2;
	int priority3;
	int control_reg;  /* Control register? seems to be always 0x3f */

	/* ideally, the layer enable masks should consist of only one bit, */
	/* but in many cases it is unknown which bit is which. */
	int scrl1_enable_mask;
	int scrl2_enable_mask;
	int scrl3_enable_mask;
	int stars_enable_mask;

	int bank_scroll1;
	int bank_scroll2;
	int bank_scroll3;

	/* Some characters aren't visible */
	const int space_scroll1;
	const int start_scroll2;
	const int end_scroll2;
	const int start_scroll3;
	const int end_scroll3;

	int kludge;  /* Ghouls n Ghosts sprite kludge */
};

struct CPS1config *cps1_game_config;

/*                 CPSB ID    multiply protection  ctrl    priority masks  unknwn     layer enable    */
#define CPS_B_01 0x00,0x0000, 0,0,0,0, /* n/a */   0x66,0x68,0x6a,0x6c,0x6e,0x70, 0x02,0x04,0x08,0x30
#define UNKNW_02 0x00,0x0000, 0,0,0,0, /* n/a */   0x6c,0x6a,0x68,0x66,0x64,0x62, 0x02,0x04,0x08,0x00
#define UNKNW_03 0x00,0x0000, 0,0,0,0, /* n/a */   0x70,0x6e,0x6c,0x6a,0x68,0x66, 0x20,0x10,0x08,0x00
#define CPS_B_04 0x60,0x0004, 0,0,0,0, /* n/a */   0x6e,0x66,0x70,0x68,0x72,0x6a, 0x02,0x0c,0x0c,0x00
#define CPS_B_05 0x60,0x0005, 0,0,0,0, /* n/a */   0x68,0x6a,0x6c,0x6e,0x70,0x72, 0x02,0x08,0x20,0x14
#define CPS_B_11 0x00,0x0000, 0,0,0,0, /* n/a */   0x66,0x68,0x6a,0x6c,0x6e,0x70, 0x20,0x10,0x08,0x00
#define CPS_B_12 0x60,0x0402, 0,0,0,0, /* n/a */   0x6c,0x00,0x00,0x00,0x00,0x62, 0x02,0x04,0x08,0x00
#define CPS_B_13 0x6e,0x0403, 0,0,0,0, /* n/a */   0x62,0x64,0x66,0x68,0x6a,0x6c, 0x20,0x04,0x02,0x00
#define CPS_B_14 0x5e,0x0404, 0,0,0,0, /* n/a */   0x52,0x54,0x56,0x58,0x5a,0x5c, 0x08,0x30,0x30,0x00
#define CPS_B_15 0x4e,0x0405, 0,0,0,0, /* n/a */   0x42,0x44,0x46,0x48,0x4a,0x4c, 0x04,0x22,0x22,0x00
#define CPS_B_16 0x40,0x0406, 0,0,0,0, /* n/a */   0x4c,0x4a,0x48,0x46,0x44,0x42, 0x10,0x0a,0x0a,0x00
#define CPS_B_17 0x48,0x0407, 0,0,0,0, /* n/a */   0x54,0x52,0x50,0x4e,0x4c,0x4a, 0x08,0x10,0x02,0x00
#define CPS_B_18 0xd0,0x0408, 0,0,0,0, /* n/a */   0xdc,0xda,0xd8,0xd6,0xd4,0xd2, 0x10,0x0a,0x0a,0x00
#define NOBATTRY 0x00,0x0000, 0x40,0x42,0x44,0x46, 0x66,0x68,0x6a,0x6c,0x6e,0x70, 0x02,0x04,0x08,0x00
#define BATTRY_1 0x72,0x0800, 0x4e,0x4c,0x4a,0x48, 0x68,0x66,0x64,0x62,0x60,0x70, 0x20,0x04,0x08,0x12
#define BATTRY_2 0x00,0x0000, 0x5e,0x5c,0x5a,0x58, 0x60,0x6e,0x6c,0x6a,0x68,0x70, 0x30,0x08,0x30,0x00
#define BATTRY_3 0x00,0x0000, 0x46,0x44,0x42,0x40, 0x60,0x6e,0x6c,0x6a,0x68,0x70, 0x20,0x12,0x12,0x00
#define BATTRY_4 0x00,0x0000, 0x46,0x44,0x42,0x40, 0x68,0x66,0x64,0x62,0x60,0x70, 0x20,0x10,0x02,0x00
#define BATTRY_5 0x00,0x0000, 0x00,0x00,0x00,0x00, 0x6e,0x66,0x70,0x68,0x72,0x6a, 0x02,0x0c,0x0c,0x00
#define BATTRY_6 0x00,0x0000, 0x4e,0x4c,0x4a,0x48, 0x60,0x6e,0x6c,0x6a,0x68,0x70, 0x20,0x06,0x06,0x00
#define BATTRY_7 0x00,0x0000, 0x00,0x00,0x00,0x00, 0x60,0x6e,0x6c,0x6a,0x68,0x70, 0x20,0x14,0x14,0x00
#define BATTRY_8 0x00,0x0000, 0x00,0x00,0x00,0x00, 0x6c,0x00,0x00,0x00,0x00,0x52, 0x14,0x02,0x14,0x00
#define QSOUND_1 0x00,0x0000, 0x00,0x00,0x00,0x00, 0x62,0x64,0x66,0x68,0x6a,0x6c, 0x10,0x08,0x04,0x00
#define QSOUND_2 0x00,0x0000, 0x00,0x00,0x00,0x00, 0x4a,0x4c,0x4e,0x40,0x42,0x44, 0x16,0x16,0x16,0x00
#define QSOUND_3 0x4e,0x0c00, 0x00,0x00,0x00,0x00, 0x52,0x54,0x56,0x48,0x4a,0x4c, 0x04,0x02,0x20,0x00
#define QSOUND_4 0x6e,0x0c01, 0x00,0x00,0x00,0x00, 0x56,0x40,0x42,0x68,0x6a,0x6c, 0x04,0x08,0x10,0x00
#define QSOUND_5 0x5e,0x0c02, 0x00,0x00,0x00,0x00, 0x6a,0x6c,0x6e,0x70,0x72,0x5c, 0x04,0x08,0x10,0x00


static struct CPS1config cps1_config_table[]=
{
	/* name       CPSB    banks spacechr      tile limits            kludge */
	{"forgottn",CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff, 7},
	{"lostwrld",CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff, 7},
	{"ghouls",  CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff, 1},
	{"ghoulsu", CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff, 1},
	{"ghoulsj", CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff, 1},
	{"strider", CPS_B_01, 1,0,1,     -1,0x0000,0xffff,0x0000,0xffff },
	{"striderj",CPS_B_01, 1,0,1,     -1,0x0000,0xffff,0x0000,0xffff },
	{"stridrja",CPS_B_01, 1,0,1,     -1,0x0000,0xffff,0x0000,0xffff },
	{"dwj",     UNKNW_02, 0,1,1,     -1,0x0000,0xffff,0x0000,0xffff },
	{"willow",  UNKNW_03, 0,1,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"willowj", UNKNW_03, 0,1,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"unsquad", CPS_B_11, 0,0,0,     -1,0x0000,0xffff,0x0001,0xffff },
	{"area88",  CPS_B_11, 0,0,0,     -1,0x0000,0xffff,0x0001,0xffff },
	{"ffight",  CPS_B_04, 0,0,0,     -1,0x0001,0xffff,0x0001,0xffff },
	{"ffightu", CPS_B_01, 0,0,0,     -1,0x0001,0xffff,0x0001,0xffff },
	{"ffightj", CPS_B_04, 0,0,0,     -1,0x0001,0xffff,0x0001,0xffff },
	{"1941",    CPS_B_05, 0,0,0,     -1,0x0000,0xffff,0x0400,0x07ff },
	{"1941j",   CPS_B_05, 0,0,0,     -1,0x0000,0xffff,0x0400,0x07ff },
	{"mercs",   CPS_B_12, 0,0,0,     -1,0x0600,0x5bff,0x0700,0x17ff, 4 },	/* (uses port 74) */
	{"mercsu",  CPS_B_12, 0,0,0,     -1,0x0600,0x5bff,0x0700,0x17ff, 4 },	/* (uses port 74) */
	{"mercsj",  CPS_B_12, 0,0,0,     -1,0x0600,0x5bff,0x0700,0x17ff, 4 },	/* (uses port 74) */
	{"msword",  CPS_B_13, 0,0,0,     -1,0x2800,0x37ff,0x0000,0xffff },	/* CPSB ID not checked, but it's the same as sf2j */
	{"mswordu", CPS_B_13, 0,0,0,     -1,0x2800,0x37ff,0x0000,0xffff },	/* CPSB ID not checked, but it's the same as sf2j */
	{"mswordj", CPS_B_13, 0,0,0,     -1,0x2800,0x37ff,0x0000,0xffff },	/* CPSB ID not checked, but it's the same as sf2j */
	{"mtwins",  CPS_B_14, 0,0,0,     -1,0x0000,0x3fff,0x0e00,0xffff },
	{"chikij",  CPS_B_14, 0,0,0,     -1,0x0000,0x3fff,0x0e00,0xffff },
	{"nemo",    CPS_B_15, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"nemoj",   CPS_B_15, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"cawing",  CPS_B_16, 0,0,0, 0x0002,0x0000,0xffff,0x0000,0xffff, 6},
	{"cawingj", CPS_B_16, 0,0,0, 0x0002,0x0000,0xffff,0x0000,0xffff, 6},
	{"sf2",     CPS_B_17, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2a",    CPS_B_17, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2b",    CPS_B_17, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2e",    CPS_B_18, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2i",    CPS_B_14, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2j",    CPS_B_13, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2jb",   CPS_B_17, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	/* from here onwards the CPS-B board has suicide battery and multiply protection */
	{"3wonders",BATTRY_1, 0,1,1,     -1,0x0000,0xffff,0x0000,0xffff, 2},
	{"wonder3", BATTRY_1, 0,1,1,     -1,0x0000,0xffff,0x0000,0xffff, 2},
	{"kod",     BATTRY_2, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"kodu",    BATTRY_2, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"kodj",    BATTRY_2, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"kodb",    BATTRY_2, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },	/* bootleg, doesn't use multiply protection */
	{"captcomm",BATTRY_3, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"captcomu",BATTRY_3, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"captcomj",BATTRY_3, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"knights", BATTRY_4, 0,0,0, 0xf020,0x0000,0xffff,0x0000,0xffff },
	{"knightsu",BATTRY_4, 0,0,0, 0xf020,0x0000,0xffff,0x0000,0xffff },
	{"knightsj",BATTRY_4, 0,0,0, 0xf020,0x0000,0xffff,0x0000,0xffff },
	{"sf2ce",   NOBATTRY, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2cea",  NOBATTRY, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2ceb",  NOBATTRY, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2cej",  NOBATTRY, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2rb",   NOBATTRY, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2red",  NOBATTRY, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2accp2",NOBATTRY, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"varth",   BATTRY_5, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },	/* CPSB test has been patched out (60=0008) */
	{"varthu",  BATTRY_5, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },	/* CPSB test has been patched out (60=0008) */
	{"varthj",  BATTRY_6, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },	/* CPSB test has been patched out (72=0001) */
	{"cworld2j",BATTRY_7, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },  /* The 0x76 priority values are incorrect values */
	{"wof",     CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },	/* bootleg? */
	{"wofa",    CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },	/* bootleg? */
	{"wofu",    QSOUND_1, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"wofj",    QSOUND_1, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"dino",    QSOUND_2, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },	/* layer enable never used */
	{"dinoj",   QSOUND_2, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },	/* layer enable never used */
	{"punisher",QSOUND_3, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"punishru",QSOUND_3, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"punishrj",QSOUND_3, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"slammast",QSOUND_4, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"slammasu",QSOUND_4, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"mbomberj",QSOUND_4, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"mbombrd", QSOUND_5, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"mbombrdj",QSOUND_5, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2t",    NOBATTRY, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"sf2tj",   NOBATTRY, 2,2,2,     -1,0x0000,0xffff,0x0000,0xffff },
	{"qad",     BATTRY_8, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },	/* TODO: layer enable */
	{"qadj",    NOBATTRY, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"qtono2",  NOBATTRY, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"megaman", CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"rockmanj",CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"pnickj",  CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff },
	{"pang3",   CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff, 5},	/* EEPROM port is among the CPS registers */
	{"pang3j",  CPS_B_01, 0,0,0,     -1,0x0000,0xffff,0x0000,0xffff, 5},	/* EEPROM port is among the CPS registers */

    /* CPS2 games */
    {"cps2",    CPS_B_01, 4,4,4,     -1,0x0000,0xffff,0x0000,0xffff },
    {"ssf2",    CPS_B_01, 4,4,0,     -1,0x0000,0xffff,0x0000,0xffff },
    {"xmcota",  CPS_B_01, 4,4,4,     -1,0x0000,0xffff,0x0000,0xffff, 8},
    {"xmcotaj", CPS_B_01, 4,4,4,     -1,0x0000,0xffff,0x0000,0xffff, 8},
    {"xmcotaj1",CPS_B_01, 4,4,4,     -1,0x0000,0xffff,0x0000,0xffff, 8},

	{0}		/* End of table */
};

static int cps_version;

void cps_setversion(int v)
{
    cps_version=v;
}

int cps_getversion(void)
{
    return cps_version;
}


static void cps_init_machine(void)
{
	const char *gamename = Machine->gamedrv->name;
	struct CPS1config *pCFG=&cps1_config_table[0];

	while(pCFG->name)
	{
		if (strcmp(pCFG->name, gamename) == 0)
		{
			break;
		}
		pCFG++;
	}
	cps1_game_config=pCFG;

    if (!cps1_game_config->name)
    {
        gamename="cps2";
        pCFG=&cps1_config_table[0];
        while(pCFG->name)
        {
            if (strcmp(pCFG->name, gamename) == 0)
            {
                break;
            }
            pCFG++;
        }
        cps1_game_config=pCFG;
   }

#if MAME_DEBUG
    cps_view_tiles=0;
#endif

	if (strcmp(gamename, "sf2rb" )==0)
	{
		/* Patch out protection check */
		UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
		rom[0xe5464/2] = 0x6012;
	}

	if (strcmp(gamename, "sf2accp2" )==0)
	{
		/* Patch out a odd branch which would be incorrectly interpreted
		   by the cpu core as a 32-bit branch. This branch would make the
		   game crash (address error, since it would branch to an odd address)
		   if location 180ca6 (outside ROM space) isn't 0. Protection check? */
		UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
		rom[0x11756/2] = 0x4e71;
	}
#if 0
	else if (strcmp(gamename, "ghouls" )==0)
	{
		/* Patch out self-test... it takes forever */
		UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
		rom[0x61964/2] = 0x4ef9;
		rom[0x61966/2] = 0x0000;
		rom[0x61968/2] = 0x0400;
	}
#endif
}


INLINE int cps1_port(int offset)
{
	return cps1_output[offset/2];
}

INLINE data16_t *cps1_base(int offset,int boundary)
{
	int base=cps1_port(offset)*256;
	/*
	The scroll RAM must start on a 0x4000 boundary.
	Some games do not do this.
	For example:
	   Captain commando     - continue screen will not display
	   Muscle bomber games  - will animate garbage during gameplay
	Mask out the irrelevant bits.
	*/
	base &= ~(boundary-1);
 	return &cps1_gfxram[(base&0x3ffff)/2];
}



READ16_HANDLER( cps1_output_r )
{
#if VERBOSE
if (offset >= 0x18/2) logerror("PC %06x: read output port %02x\n",cpu_get_pc(),offset*2);
#endif

	/* Some games interrogate a couple of registers on bootup. */
	/* These are CPS1 board B self test checks. They wander from game to */
	/* game. */
	if (offset && offset == cps1_game_config->cpsb_addr/2)
		return cps1_game_config->cpsb_value;

	/* some games use as a protection check the ability to do 16-bit multiplies */
	/* with a 32-bit result, by writing the factors to two ports and reading the */
	/* result from two other ports. */
	if (offset && offset == cps1_game_config->mult_result_lo/2)
		return (cps1_output[cps1_game_config->mult_factor1/2] *
				cps1_output[cps1_game_config->mult_factor2/2]) & 0xffff;
	if (offset && offset == cps1_game_config->mult_result_hi/2)
		return (cps1_output[cps1_game_config->mult_factor1/2] *
				cps1_output[cps1_game_config->mult_factor2/2]) >> 16;

	/* Pang 3 EEPROM interface */
	if (cps1_game_config->kludge == 5 && offset == 0x7a/2)
		return cps1_eeprom_port_r(0);

	return cps1_output[offset];
}

WRITE16_HANDLER( cps1_output_w )
{
	/* Pang 3 EEPROM interface */
	if (cps1_game_config->kludge == 5 && offset == 0x7a/2)
	{
		cps1_eeprom_port_w(0,data,mem_mask);
		return;
	}

	data = COMBINE_DATA(&cps1_output[offset]);

#ifdef MAME_DEBUG
if (cps1_game_config->control_reg && offset == cps1_game_config->control_reg/2 && data != 0x3f)
	usrintf_showmessage("control_reg = %04x",data);
#endif
#if VERBOSE
if (offset > 0x22/2 &&
        offset != cps1_game_config->layer_control/2 &&
		offset != cps1_game_config->priority0/2 &&
		offset != cps1_game_config->priority1/2 &&
		offset != cps1_game_config->priority2/2 &&
		offset != cps1_game_config->priority3/2 &&
		offset != cps1_game_config->control_reg/2)
	logerror("PC %06x: write %02x to output port %02x\n",cpu_get_pc(),data,offset*2);

#ifdef MAME_DEBUG
//if (offset == 0x22/2 && (data & ~0x8001) != 0x0e)
//	usrintf_showmessage("port 22 = %02x",data);
if (cps1_game_config->priority0 && offset == cps1_game_config->priority0/2 && data != 0x00)
	usrintf_showmessage("priority0 %04x",data);
#endif
#endif
}



/* Public variables */
data16_t *cps1_gfxram;
data16_t *cps1_output;

size_t cps1_gfxram_size;
size_t cps1_output_size;

/* Offset of each palette entry */
const int cps1_obj_palette     = 0*32;
const int cps1_scroll1_palette = 1*32;
const int cps1_scroll2_palette = 2*32;
const int cps1_scroll3_palette = 3*32;
const int cps1_stars1_palette  = 4*32;
const int cps1_stars2_palette  = 5*32;
#define cps1_palette_entries (32*8)  /* Number colour schemes in palette */

const int cps1_scroll1_size=0x4000;
const int cps1_scroll2_size=0x4000;
const int cps1_scroll3_size=0x4000;
const int cps1_obj_size    =0x0800;
const int cps1_other_size  =0x0800;
const int cps1_palette_size=cps1_palette_entries*32; /* Size of palette RAM */
static int cps1_flip_screen;    /* Flip screen on / off */

static data16_t *cps1_scroll1;
static data16_t *cps1_scroll2;
static data16_t *cps1_scroll3;
static data16_t *cps1_obj;
static data16_t *cps1_buffered_obj;
static data16_t *cps1_palette;
static data16_t *cps1_other;
static data16_t *cps1_old_palette;

/* Working variables */
static int cps1_last_sprite_offset;     /* Offset of the last sprite */
static int cps1_layer_enabled[4];       /* Layer enabled [Y/N] */
static int cps1_stars_enabled;          /* Layer enabled [Y/N] */

int scroll1x, scroll1y, scroll2x, scroll2y, scroll3x, scroll3y;
int stars1x, stars1y, stars2x, stars2y;
static data16_t *cps1_scroll2_old;
static struct osd_bitmap *cps1_scroll2_bitmap;


/* Output ports */
#define CPS1_OBJ_BASE			0x00    /* Base address of objects */
#define CPS1_SCROLL1_BASE       0x02    /* Base address of scroll 1 */
#define CPS1_SCROLL2_BASE       0x04    /* Base address of scroll 2 */
#define CPS1_SCROLL3_BASE       0x06    /* Base address of scroll 3 */
#define CPS1_OTHER_BASE			0x08    /* Base address of other video */
#define CPS1_PALETTE_BASE       0x0a    /* Base address of palette */
#define CPS1_SCROLL1_SCROLLX    0x0c    /* Scroll 1 X */
#define CPS1_SCROLL1_SCROLLY    0x0e    /* Scroll 1 Y */
#define CPS1_SCROLL2_SCROLLX    0x10    /* Scroll 2 X */
#define CPS1_SCROLL2_SCROLLY    0x12    /* Scroll 2 Y */
#define CPS1_SCROLL3_SCROLLX    0x14    /* Scroll 3 X */
#define CPS1_SCROLL3_SCROLLY    0x16    /* Scroll 3 Y */
#define CPS1_STARS1_SCROLLX     0x18    /* Stars 1 X */
#define CPS1_STARS1_SCROLLY     0x1a    /* Stars 1 Y */
#define CPS1_STARS2_SCROLLX     0x1c    /* Stars 2 X */
#define CPS1_STARS2_SCROLLY     0x1e    /* Stars 2 Y */

#define CPS1_ROWSCROLL_OFFS     0x20    /* base of row scroll offsets in other RAM */

#define CPS1_SCROLL2_WIDTH      0x40
#define CPS1_SCROLL2_HEIGHT     0x40


/*
CPS1 VIDEO RENDERER

*/
static UINT32 *cps1_gfx;		 /* Converted GFX memory */
static int *cps1_char_pen_usage;	/* pen usage array */
static int *cps1_tile16_pen_usage;      /* pen usage array */
static int *cps1_tile32_pen_usage;      /* pen usage array */
static int cps1_max_char;	       /* Maximum number of 8x8 chars */
static int cps1_max_tile16;	     /* Maximum number of 16x16 tiles */
static int cps1_max_tile32;	     /* Maximum number of 32x32 tiles */
static UINT8 *stars_rom;

/* first 0x4000 of gfx ROM are used, but 0x0000-0x1fff is == 0x2000-0x3fff */
const int stars_rom_size = 0x2000;

/* PSL: CPS2 support */
const int cps2_obj_size    =0x2000;
data16_t *cps2_objram;
data16_t *cps2_output;

size_t cps2_objram_size;
size_t cps2_output_size;
static data16_t *cps2_buffered_obj;
static int cps2_last_sprite_offset;     /* Offset of the last sprite */


#define CPS2_OBJ_BASE           0x00    /* Base address of objects */
#define CPS2_UK1_BASE           0x02    /* Unknown (nearly always 0x807d) */
#define CPS2_PRI_BASE           0x04    /* Layers priorities */
#define CPS2_UK2_BASE           0x06    /* Unknown (always 0x0000) */
#define CPS2_UK3_START          0x08    /* Unknown (always 0x0040) */
#define CPS2_UK4_SIZE           0x0a    /* Unknown (always 0x0010)  */

INLINE int cps2_port(int offset)
{
    return cps2_output[offset/2];
}




static void cps1_gfx_decode(void)
{
	int size=memory_region_length(REGION_GFX1);
	int i,j,gfxsize;
	UINT8 masks[] = {
			0xff, 0x00, 0x00, 0x00,
			0x00, 0xff, 0x00, 0x00,
			0x00, 0x00, 0xff, 0x00,
			0x00, 0x00, 0x00, 0xff,
		};
	UINT32 mask1,mask2,mask4,mask8;

	/* compensate for machine endianness */
	mask1 = *((UINT32 *)&masks[0]);
	mask2 = *((UINT32 *)&masks[4]);
	mask4 = *((UINT32 *)&masks[8]);
	mask8 = *((UINT32 *)&masks[12]);

	cps1_gfx = (UINT32 *)memory_region(REGION_GFX1);

	gfxsize=size/4;

	for (i = 0;i < gfxsize;i++)
	{
		UINT32 src = cps1_gfx[i];
		UINT32 dwval = 0;
		int penusage = 0;

		for (j = 0;j < 8;j++)
		{
			int n = 0;
			UINT32 mask = (0x01010101 << j) & src;

			if (mask & mask1) n |= 1;
			if (mask & mask2) n |= 2;
			if (mask & mask4) n |= 4;
			if (mask & mask8) n |= 8;

			dwval |= n << (j * 4);
			penusage |= 1 << n;
		}
		cps1_gfx[i] = dwval;
		cps1_char_pen_usage[i/16]    |= penusage;
		cps1_tile16_pen_usage[i/32]  |= penusage;
		cps1_tile32_pen_usage[i/128] |= penusage;
	}
}

static void unshuffle(UINT64 *buf,int len)
{
	int i;
	UINT64 t;

	if (len == 2) return;

	if (len % 4) exit(1);   /* must not happen */

	len /= 2;

	unshuffle(buf,len);
	unshuffle(buf + len,len);

	for (i = 0;i < len/2;i++)
	{
		t = buf[len/2 + i];
		buf[len/2 + i] = buf[len + i];
		buf[len + i] = t;
	}
}

static void cps2_gfx_decode(void)
{
	const int banksize=0x200000;
	int size=memory_region_length(REGION_GFX1);
	int i;

	for (i = 0;i < size;i += banksize)
		unshuffle((UINT64 *)(memory_region(REGION_GFX1) + i),banksize/8);

	cps1_gfx_decode();
}


int cps1_gfx_start(void)
{
	int size=memory_region_length(REGION_GFX1);
	int gfxsize;

	gfxsize=size/4;

	/* Set up maximum values */
	cps1_max_char  =(gfxsize/2)/8;
	cps1_max_tile16=(gfxsize/4)/8;
	cps1_max_tile32=(gfxsize/16)/8;

	cps1_char_pen_usage=malloc(cps1_max_char*sizeof(int));
	if (!cps1_char_pen_usage)
	{
		return -1;
	}
	memset(cps1_char_pen_usage, 0, cps1_max_char*sizeof(int));

	cps1_tile16_pen_usage=malloc(cps1_max_tile16*sizeof(int));
	if (!cps1_tile16_pen_usage)
		return -1;
	memset(cps1_tile16_pen_usage, 0, cps1_max_tile16*sizeof(int));

	cps1_tile32_pen_usage=malloc(cps1_max_tile32*sizeof(int));
	if (!cps1_tile32_pen_usage)
	{
		return -1;
	}
	memset(cps1_tile32_pen_usage, 0, cps1_max_tile32*sizeof(int));

	stars_rom = malloc(4*stars_rom_size);
	if (!stars_rom) return -1;
	memcpy(stars_rom,memory_region(REGION_GFX1),4*stars_rom_size);

    if (cps_version == 1)
        cps1_gfx_decode();
    else
        cps2_gfx_decode();
#if MAME_DEBUG
    cps_tileviewer_start();
#endif
	return 0;
}

void cps1_gfx_stop(void)
{
	cps1_gfx = NULL;
	free(cps1_char_pen_usage);
	cps1_char_pen_usage = NULL;
	free(cps1_tile16_pen_usage);
	cps1_tile16_pen_usage = NULL;
	free(cps1_tile32_pen_usage);
	cps1_tile32_pen_usage = NULL;
	free(stars_rom);
	stars_rom = NULL;
#if MAME_DEBUG
    cps_tileviewer_start();
#endif
}


void cps1_draw_gfx(
	struct osd_bitmap *dest,const struct GfxElement *gfx,
	int code,
	int color,
	int flipx,int flipy,
	int sx,int sy,
	int tpens,
	int *pusage,
	const int size,
	const int max,
	const int delta,
	const int srcdelta)
{
	#define DATATYPE unsigned char
	#define IF_NOT_TRANSPARENT(n,x,y) if (tpens & (0x01 << n))
	if (dest == priority_bitmap)
	{
	#define PALDATA(n) 1
	#define SELF_INCLUDE
	#include "cps1.c"
	#undef SELF_INCLUDE
	#undef PALDATA
	}
	else
	{
	#define PALDATA(n) paldata[n]
	#define SELF_INCLUDE
	#include "cps1.c"
	#undef SELF_INCLUDE
	#undef PALDATA
	}

	#undef DATATYPE
	#undef IF_NOT_TRANSPARENT
}

void cps1_draw_gfx16(
	struct osd_bitmap *dest,const struct GfxElement *gfx,
	int code,
	int color,
	int flipx,int flipy,
	int sx,int sy,
	int tpens,
	int *pusage,
	const int size,
	const int max,
	const int delta,
	const int srcdelta)
{
	#define DATATYPE unsigned short
	#define IF_NOT_TRANSPARENT(n,x,y) if (tpens & (0x01 << n))
	if (dest == priority_bitmap)
	{
	#define PALDATA(n) 1
	#define SELF_INCLUDE
	#include "cps1.c"
	#undef SELF_INCLUDE
	#undef PALDATA
	}
	else
	{
	#define PALDATA(n) paldata[n]
	#define SELF_INCLUDE
	#include "cps1.c"
	#undef SELF_INCLUDE
	#undef PALDATA
	}
	#undef DATATYPE
	#undef IF_NOT_TRANSPARENT
}

void cps1_draw_gfx_pri(
	struct osd_bitmap *dest,const struct GfxElement *gfx,
	int code,
	int color,
	int flipx,int flipy,
	int sx,int sy,
	int tpens,
	int *pusage,
	const int size,
	const int max,
	const int delta,
	const int srcdelta,
	struct osd_bitmap *pribm)
{
	#define DATATYPE unsigned char
	#define IF_NOT_TRANSPARENT(n,x,y) if ((tpens & (0x01 << n)) && pribm->line[y][x] == 0)
	#define PALDATA(n) paldata[n]
	#define SELF_INCLUDE
	#include "cps1.c"
	#undef SELF_INCLUDE
	#undef DATATYPE
	#undef IF_NOT_TRANSPARENT
	#undef PALDATA
}

void cps1_draw_gfx16_pri(
	struct osd_bitmap *dest,const struct GfxElement *gfx,
	int code,
	int color,
	int flipx,int flipy,
	int sx,int sy,
	int tpens,
	int *pusage,
	const int size,
	const int max,
	const int delta,
	const int srcdelta,
	struct osd_bitmap *pribm)
{
	#define DATATYPE unsigned short
	#define IF_NOT_TRANSPARENT(n,x,y) if ((tpens & (0x01 << n)) && pribm->line[y][x] == 0)
	#define PALDATA(n) paldata[n]
	#define SELF_INCLUDE
	#include "cps1.c"
	#undef SELF_INCLUDE
	#undef DATATYPE
	#undef IF_NOT_TRANSPARENT
	#undef PALDATA
}

/*

This is an optimized version that doesn't take into account transparency

Draws complete tiles without checking transparency. Used for scroll 2 low
priority rendering.

*/
void cps1_draw_gfx_opaque(
	struct osd_bitmap *dest,const struct GfxElement *gfx,
	int code,
	int color,
	int flipx,int flipy,
	int sx,int sy,
	int tpens,
	int *pusage,
	const int size,
	const int max,
	const int delta,
	const int srcdelta)
{
	#define DATATYPE unsigned char
	#define IF_NOT_TRANSPARENT(n,x,y)
	#define PALDATA(n) paldata[n]
	#define SELF_INCLUDE
	#include "cps1.c"
	#undef SELF_INCLUDE
	#undef DATATYPE
	#undef IF_NOT_TRANSPARENT
	#undef PALDATA
}

void cps1_draw_gfx_opaque16(
	struct osd_bitmap *dest,const struct GfxElement *gfx,
	int code,
	int color,
	int flipx,int flipy,
	int sx,int sy,
	int tpens,
	int *pusage,
	const int size,
	const int max,
	const int delta,
	const int srcdelta)
{
	#define DATATYPE unsigned short
	#define IF_NOT_TRANSPARENT(n,x,y)
	#define PALDATA(n) paldata[n]
	#define SELF_INCLUDE
	#include "cps1.c"
	#undef SELF_INCLUDE
	#undef DATATYPE
	#undef IF_NOT_TRANSPARENT
	#undef PALDATA
}



INLINE void cps1_draw_scroll1(
	struct osd_bitmap *dest,
	int code, int color,
	int flipx, int flipy,int sx, int sy, int tpens)
{
	if (dest->depth==16)
	{
		cps1_draw_gfx16(dest,
			Machine->gfx[1],
			code,color,flipx,flipy,sx,sy,
			tpens,cps1_char_pen_usage,8, cps1_max_char, 16, 1);
	}
	else
	{
		cps1_draw_gfx(dest,
			Machine->gfx[1],
			code,color,flipx,flipy,sx,sy,
			tpens,cps1_char_pen_usage,8, cps1_max_char, 16, 1);
	}
}


INLINE void cps1_draw_tile16(struct osd_bitmap *dest,
	const struct GfxElement *gfx,
	int code, int color,
	int flipx, int flipy,int sx, int sy, int tpens)
{
	if (dest->depth==16)
	{
		cps1_draw_gfx16(dest,
			gfx,
			code,color,flipx,flipy,sx,sy,
			tpens,cps1_tile16_pen_usage,16, cps1_max_tile16, 16*2,0);
	}
	else
	{
		cps1_draw_gfx(dest,
			gfx,
			code,color,flipx,flipy,sx,sy,
			tpens,cps1_tile16_pen_usage,16, cps1_max_tile16, 16*2,0);
	}
}

INLINE void cps1_draw_tile16_pri(struct osd_bitmap *dest,
	const struct GfxElement *gfx,
	int code, int color,
	int flipx, int flipy,int sx, int sy, int tpens)
{
	if (dest->depth==16)
	{
		cps1_draw_gfx16_pri(dest,
			gfx,
			code,color,flipx,flipy,sx,sy,
			tpens,cps1_tile16_pen_usage,16, cps1_max_tile16, 16*2,0, priority_bitmap);
	}
	else
	{
		cps1_draw_gfx_pri(dest,
			gfx,
			code,color,flipx,flipy,sx,sy,
			tpens,cps1_tile16_pen_usage,16, cps1_max_tile16, 16*2,0, priority_bitmap);
	}
}

INLINE void cps1_draw_tile32(struct osd_bitmap *dest,
	const struct GfxElement *gfx,
	int code, int color,
	int flipx, int flipy,int sx, int sy, int tpens)
{
	if (dest->depth==16)
	{
		cps1_draw_gfx16(dest,
			gfx,
			code,color,flipx,flipy,sx,sy,
			tpens,cps1_tile32_pen_usage,32, cps1_max_tile32, 16*2*4,0);
	}
	else
	{
		cps1_draw_gfx(dest,
			gfx,
			code,color,flipx,flipy,sx,sy,
			tpens,cps1_tile32_pen_usage,32, cps1_max_tile32, 16*2*4,0);
	}
}


INLINE void cps1_draw_blank16(struct osd_bitmap *dest, int sx, int sy )
{
	int i,j;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;
		temp=sx;
		sx=sy;
		sy=dest->height-temp-16;
	}

	if (cps1_flip_screen)
	{
		/* Handle flipped screen */
		sx=dest->width-sx-16;
		sy=dest->height-sy-16;
	}

	if (dest->depth==16)
	{
		for (i=15; i>=0; i--)
		{
			register unsigned short *bm=(unsigned short *)dest->line[sy+i]+sx;
			for (j=15; j>=0; j--)
			{
				*bm=palette_transparent_pen;
				bm++;
			}
		}
	}
	else
	{
		for (i=15; i>=0; i--)
		{
			register unsigned char *bm=dest->line[sy+i]+sx;
			for (j=15; j>=0; j--)
			{
				*bm=palette_transparent_pen;
				bm++;
			}
		}
	}
}



INLINE void cps1_draw_tile16_bmp(struct osd_bitmap *dest,
	const struct GfxElement *gfx,
	int code, int color,
	int flipx, int flipy,int sx, int sy)
{
	if (dest->depth==16)
	{
		cps1_draw_gfx_opaque16(dest,
			gfx,
			code,color,flipx,flipy,sx,sy,
			-1,cps1_tile16_pen_usage,16, cps1_max_tile16, 16*2,0);
	}
	else
	{
		cps1_draw_gfx_opaque(dest,
			gfx,
			code,color,flipx,flipy,sx,sy,
			-1,cps1_tile16_pen_usage,16, cps1_max_tile16, 16*2,0);
	}
}




static int cps1_transparency_scroll[4];



#if CPS1_DUMP_VIDEO
void cps1_dump_video(void)
{
	FILE *fp;
	fp=fopen("SCROLL1.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_scroll1, cps1_scroll1_size, 1, fp);
		fclose(fp);
	}
	fp=fopen("SCROLL2.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_scroll2, cps1_scroll2_size, 1, fp);
		fclose(fp);
	}
	fp=fopen("SCROLL3.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_scroll3, cps1_scroll3_size, 1, fp);
		fclose(fp);
	}

    fp=fopen("OBJ.DMP", "w+b");
    if (fp)
    {
        fwrite(cps1_obj, cps1_obj_size, 1, fp);
        fclose(fp);
    }
    if (cps_version == 2)
    {
        /* PSL: CPS2 support */
        fp=fopen("OBJCPS2.DMP", "w+b");
        if (fp)
        {
            fwrite(cps2_objram, cps2_objram_size, 1, fp);
            fclose(fp);
        }
        fp=fopen("CPS2OUTP.DMP", "w+b");
        if (fp)
        {
            fwrite(cps2_output, cps2_output_size, 1, fp);
            fclose(fp);
        }

    }


	fp=fopen("OTHER.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_other, cps1_other_size, 1, fp);
		fclose(fp);
	}

	fp=fopen("PALETTE.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_palette, cps1_palette_size, 1, fp);
		fclose(fp);
	}

	fp=fopen("OUTPUT.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_output, cps1_output_size, 1, fp);
		fclose(fp);
	}
	fp=fopen("VIDEO.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_gfxram, cps1_gfxram_size, 1, fp);
		fclose(fp);
	}

}
#endif


INLINE void cps1_get_video_base(void )
{
	int layercontrol;

	/* Re-calculate the VIDEO RAM base */
	cps1_scroll1=cps1_base(CPS1_SCROLL1_BASE,cps1_scroll1_size);
	cps1_scroll2=cps1_base(CPS1_SCROLL2_BASE,cps1_scroll2_size);
	cps1_scroll3=cps1_base(CPS1_SCROLL3_BASE,cps1_scroll3_size);
    cps1_obj=cps1_base(CPS1_OBJ_BASE, cps1_obj_size);

	cps1_palette=cps1_base(CPS1_PALETTE_BASE,cps1_palette_size);
	cps1_other=cps1_base(CPS1_OTHER_BASE,cps1_other_size);

	/* Get scroll values */
	scroll1x=cps1_port(CPS1_SCROLL1_SCROLLX);
	scroll1y=cps1_port(CPS1_SCROLL1_SCROLLY);
	scroll2x=cps1_port(CPS1_SCROLL2_SCROLLX);
	scroll2y=cps1_port(CPS1_SCROLL2_SCROLLY);
	scroll3x=cps1_port(CPS1_SCROLL3_SCROLLX);
	scroll3y=cps1_port(CPS1_SCROLL3_SCROLLY);
	stars1x =cps1_port(CPS1_STARS1_SCROLLX);
	stars1y =cps1_port(CPS1_STARS1_SCROLLY);
	stars2x =cps1_port(CPS1_STARS2_SCROLLX);
	stars2y =cps1_port(CPS1_STARS2_SCROLLY);

	/* Get transparency registers */
	if (cps1_game_config->priority1)
	{
		cps1_transparency_scroll[0]=cps1_port(cps1_game_config->priority0);
		cps1_transparency_scroll[1]=cps1_port(cps1_game_config->priority1);
		cps1_transparency_scroll[2]=cps1_port(cps1_game_config->priority2);
		cps1_transparency_scroll[3]=cps1_port(cps1_game_config->priority3);
	}

	/* Get layer enable bits */
	layercontrol=cps1_port(cps1_game_config->layer_control);
	cps1_layer_enabled[0]=1;
	cps1_layer_enabled[1]=layercontrol & cps1_game_config->scrl1_enable_mask;
	cps1_layer_enabled[2]=layercontrol & cps1_game_config->scrl2_enable_mask;
	cps1_layer_enabled[3]=layercontrol & cps1_game_config->scrl3_enable_mask;
	cps1_stars_enabled   =layercontrol & cps1_game_config->stars_enable_mask;


#ifdef MAME_DEBUG
{
	int enablemask;

if (keyboard_pressed(KEYCODE_Z))
{
	if (keyboard_pressed(KEYCODE_Q)) cps1_layer_enabled[3]=0;
	if (keyboard_pressed(KEYCODE_W)) cps1_layer_enabled[2]=0;
	if (keyboard_pressed(KEYCODE_E)) cps1_layer_enabled[1]=0;
	if (keyboard_pressed(KEYCODE_R)) cps1_layer_enabled[0]=0;
	if (keyboard_pressed(KEYCODE_T))
	{
		usrintf_showmessage("%d %d %d %d layer %02x",
			(layercontrol>>0x06)&03,
			(layercontrol>>0x08)&03,
			(layercontrol>>0x0a)&03,
			(layercontrol>>0x0c)&03,
			layercontrol&0xc03f
			);
	}

}

	enablemask = 0;
	if (cps1_game_config->scrl1_enable_mask == cps1_game_config->scrl2_enable_mask)
		enablemask = cps1_game_config->scrl1_enable_mask;
	if (cps1_game_config->scrl1_enable_mask == cps1_game_config->scrl3_enable_mask)
		enablemask = cps1_game_config->scrl1_enable_mask;
	if (cps1_game_config->scrl2_enable_mask == cps1_game_config->scrl3_enable_mask)
		enablemask = cps1_game_config->scrl2_enable_mask;
	if (enablemask)
	{
		if (((layercontrol & enablemask) && (layercontrol & enablemask) != enablemask))
			usrintf_showmessage("layer %02x",layercontrol&0xc03f);
	}
}
#endif

{
	int enablemask;
	enablemask = cps1_game_config->scrl1_enable_mask | cps1_game_config->scrl2_enable_mask
			| cps1_game_config->scrl3_enable_mask | cps1_game_config->stars_enable_mask;
	if (((layercontrol & ~enablemask) & 0xc03e) != 0)
		usrintf_showmessage("layer %02x contact MAMEDEV",layercontrol&0xc03f);
}

}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int cps_vh_start(void)
{
	int i;

    cps_init_machine();

	if (cps1_gfx_start())
	{
		return -1;
	}

	cps1_scroll2_bitmap=bitmap_alloc(CPS1_SCROLL2_WIDTH*16,CPS1_SCROLL2_HEIGHT*16);
	if (!cps1_scroll2_bitmap)
	{
		return -1;
	}
	cps1_scroll2_old=malloc(cps1_scroll2_size);
	if (!cps1_scroll2_old)
	{
		return -1;
	}
	memset(cps1_scroll2_old, 0xff, cps1_scroll2_size);


	cps1_old_palette=malloc(cps1_palette_size);
	if (!cps1_old_palette)
	{
		return -1;
	}
	memset(cps1_old_palette, 0x00, cps1_palette_size);
	for (i = 0;i < cps1_palette_entries*16;i++)
	{
		palette_change_color(i,0,0,0);
	}

    cps1_buffered_obj = malloc (cps1_obj_size);
    if (!cps1_buffered_obj)
    {
		return -1;
	}
    memset(cps1_buffered_obj, 0x00, cps1_obj_size);

    cps2_buffered_obj = malloc (2*cps2_objram_size);
    if (!cps2_buffered_obj)
    {
		return -1;
	}
    memset(cps2_buffered_obj, 0x00, 2*cps2_objram_size);


	memset(cps1_gfxram, 0, cps1_gfxram_size);   /* Clear GFX RAM */
	memset(cps1_output, 0, cps1_output_size);   /* Clear output ports */

    if (cps_version == 2)
    {
        memset(cps2_objram, 0, cps2_objram_size);
    }

	/* Put in some defaults */
	cps1_output[CPS1_OBJ_BASE/2]     = 0x9200;
	cps1_output[CPS1_SCROLL1_BASE/2] = 0x9000;
	cps1_output[CPS1_SCROLL2_BASE/2] = 0x9040;
	cps1_output[CPS1_SCROLL3_BASE/2] = 0x9080;
	cps1_output[CPS1_OTHER_BASE/2]   = 0x9100;
	cps1_output[CPS1_PALETTE_BASE/2] = 0x90c0;

	if (!cps1_game_config)
	{
		logerror("cps1_game_config hasn't been set up yet");
		return -1;
	}


	/* Set up old base */
	cps1_get_video_base();   /* Calculate base pointers */
	cps1_get_video_base();   /* Calculate old base pointers */


	for (i=0; i<4; i++)
	{
		cps1_transparency_scroll[i]=0x0000;
	}
	return 0;
}

int cps1_vh_start(void)
{
    cps_version=1;
    return cps_vh_start();
}

int cps2_vh_start(void)
{
    if (cps_version != 99)
    {
        cps_version=2;
    }
    return cps_vh_start();
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void cps1_vh_stop(void)
{
	if (cps1_old_palette)
		free(cps1_old_palette);
	if (cps1_scroll2_bitmap)
		bitmap_free(cps1_scroll2_bitmap);
	if (cps1_scroll2_old)
		free(cps1_scroll2_old);
	if (cps1_buffered_obj)
		free(cps1_buffered_obj);
    if (cps2_buffered_obj)
        free(cps2_buffered_obj);
	cps1_gfx_stop();
}

/***************************************************************************

  Build palette from palette RAM

  12 bit RGB with a 4 bit brightness value.

***************************************************************************/

void cps1_build_palette(void)
{
	int offset;

	for (offset = 0; offset < cps1_palette_entries*16; offset++)
	{
		int palette = cps1_palette[offset];

		if (palette != cps1_old_palette[offset])
		{
		   int red, green, blue, bright;

		   bright= (palette>>12);
		   if (bright) bright += 2;

		   red   = ((palette>>8)&0x0f) * bright;
		   green = ((palette>>4)&0x0f) * bright;
		   blue  = (palette&0x0f) * bright;

		   palette_change_color (offset, red, green, blue);
		   cps1_old_palette[offset] = palette;
		}
	}
}

/***************************************************************************

  Scroll 1 (8x8)

  Attribute word layout:
  0x0001	colour
  0x0002	colour
  0x0004	colour
  0x0008	colour
  0x0010	colour
  0x0020	X Flip
  0x0040	Y Flip
  0x0080
  0x0100
  0x0200
  0x0400
  0x0800
  0x1000
  0x2000
  0x4000
  0x8000


***************************************************************************/

INLINE void cps1_palette_scroll1(unsigned short *base)
{
	int x,y, offs, offsx;

	int scrlxrough=(scroll1x>>3)+8;
	int scrlyrough=(scroll1y>>3);
	int basecode=cps1_game_config->bank_scroll1*0x08000;

	for (x=0; x<0x36; x++)
	{
		 offsx=(scrlxrough+x)*0x80;
		 offsx&=0x1fff;

		 for (y=0; y<0x20; y++)
		 {
			int code, colour, offsy;
			int n=scrlyrough+y;
			offsy=( (n&0x1f)*4 | ((n&0x20)*0x100)) & 0x3fff;
			offs=offsy+offsx;
			offs &= 0x3fff;
			code=basecode+cps1_scroll1[offs/2];
			colour=cps1_scroll1[(offs+2)/2];
			if (code < cps1_max_char)
			{
				base[colour&0x1f] |=
					  cps1_char_pen_usage[code]&0x7fff;
			}
		}
	}
}

void cps1_render_scroll1(struct osd_bitmap *bitmap,int priority)
{
	int x,y, offs, offsx, sx, sy, ytop;

    int scrlxrough=(scroll1x>>3)+4;
	int scrlyrough=(scroll1y>>3);
	int base=cps1_game_config->bank_scroll1*0x08000;
    const int spacechar=cps1_game_config->space_scroll1;


	sx=-(scroll1x&0x07);
	ytop=-(scroll1y&0x07)+32;

	for (x=0; x<0x35; x++)
	{
		 sy=ytop;
		 offsx=(scrlxrough+x)*0x80;
		 offsx&=0x1fff;

		 for (y=0; y<0x20; y++)
		 {
			int code, offsy, colour;
			int n=scrlyrough+y;
			offsy=( (n&0x1f)*4 | ((n&0x20)*0x100)) & 0x3fff;
			offs=offsy+offsx;
			offs &= 0x3fff;

			code  =cps1_scroll1[offs/2];
			colour=cps1_scroll1[(offs+2)/2];

            if (code != 0x20 && code != spacechar)
			{
				int transp;

				/* 0x0020 appears to never be drawn */
				if (priority)
				{
					transp=cps1_transparency_scroll[(colour & 0x0180)>>7];
					cps1_draw_scroll1(priority_bitmap,
							code+base,
							colour&0x1f,
							colour&0x20,
							colour&0x40,
							sx,sy,transp);
				}
				else
				{
					transp = 0x7fff;
					cps1_draw_scroll1(bitmap,
							code+base,
							colour&0x1f,
							colour&0x20,
							colour&0x40,
							sx,sy,transp);
				}
			 }
			 sy+=8;
		 }
		 sx+=8;
	}
}



/***************************************************************************

								Sprites
								=======

  Sprites are represented by a number of 8 byte values

  xx xx yy yy nn nn aa aa

  where xxxx = x position
		yyyy = y position
		nnnn = tile number
		aaaa = attribute word
					0x0001	colour
					0x0002	colour
					0x0004	colour
					0x0008	colour
					0x0010	colour
					0x0020	X Flip
					0x0040	Y Flip
					0x0080	unknown
					0x0100	X block size (in sprites)
					0x0200	X block size
					0x0400	X block size
					0x0800	X block size
					0x1000	Y block size (in sprites)
					0x2000	Y block size
					0x4000	Y block size
					0x8000	Y block size

  The end of the table (may) be marked by an attribute value of 0xff00.

***************************************************************************/

void cps1_find_last_sprite(void)    /* Find the offset of last sprite */
{
    int offset=0;
	/* Locate the end of table marker */
    while (offset < cps1_obj_size/2)
	{
        int colour=cps1_buffered_obj[offset+3];
		if (colour == 0xff00)
		{
			/* Marker found. This is the last sprite. */
            cps1_last_sprite_offset=offset-4;
			return;
		}
        offset+=4;
	}
	/* Sprites must use full sprite RAM */
    cps1_last_sprite_offset=cps1_obj_size/2-4;
}

void cps2_find_last_sprite(void)    /* Find the offset of last sprite */
{
    int offset=0;
	/* Locate the end of table marker */
    while (offset < cps2_obj_size/2)
	{
        if (cps2_buffered_obj[offset+1]==0x8000 )
		{
			/* Marker found. This is the last sprite. */
            cps2_last_sprite_offset=offset-4;
			return;
		}
        offset+=4;
	}
	/* Sprites must use full sprite RAM */
    cps2_last_sprite_offset=cps2_obj_size/2-4;
}


/* Find used colours */

void cps_palette_sprites(unsigned short *base, data16_t *objram, int last)
{
   int i;
   for (i=last; i>=0; i-=4)
   {
        int x=objram[i];
        int y=objram[i+1];
		if (x && y)
		{
            int colour=objram[i+3];
			int col=colour&0x1f;
            int code=objram[i+2];

            if (cps_version == 2)
            {
                code+=((y & 0x6000) <<3);
            }
            else
            {
                if (cps1_game_config->kludge == 7)
                {
                    code += 0x4000;
                }
                if (cps1_game_config->kludge == 1 && code >= 0x01000)
                {
                    code += 0x4000;
                }
                if (cps1_game_config->kludge == 2 && code >= 0x02a00)
                {
                    code += 0x4000;
                }
            }
			if ( colour & 0xff00 )
			{
				int nys, nxs;
				int nx=(colour & 0x0f00) >> 8;
				int ny=(colour & 0xf000) >> 12;
				nx++;
				ny++;

                if (colour & 0x40)   /* Y Flip */
				{
					if (colour &0x20)
					{
					for (nys=0; nys<ny; nys++)
					{
						for (nxs=0; nxs<nx; nxs++)
						{
							int cod=code+(nx-1)-nxs+0x10*(ny-1-nys);
							base[col] |=
							cps1_tile16_pen_usage[cod % cps1_max_tile16];
						}
					}
				}
				else
				{
					for (nys=0; nys<ny; nys++)
					{
						for (nxs=0; nxs<nx; nxs++)
						{
							int cod=code+nxs+0x10*(ny-1-nys);
							base[col] |=
							cps1_tile16_pen_usage[cod % cps1_max_tile16];
						}
					}
				}
			}
			else
			{
				if (colour &0x20)
				{
					for (nys=0; nys<ny; nys++)
					{
						for (nxs=0; nxs<nx; nxs++)
						{
							int cod=code+(nx-1)-nxs+0x10*nys;
							base[col] |=
							cps1_tile16_pen_usage[cod % cps1_max_tile16];
						}
					}
				}
				else
				{
					for (nys=0; nys<ny; nys++)
					{
						for (nxs=0; nxs<nx; nxs++)
						{
							int cod=code+nxs+0x10*nys;
							base[col] |=
							cps1_tile16_pen_usage[cod % cps1_max_tile16];
						}
					}
				}
			}
			base[col]&=0x7fff;
			}
			else
			{
				base[col] |=
				cps1_tile16_pen_usage[code % cps1_max_tile16]&0x7fff;
			}
		}
    }
}

void cps1_render_sprites(struct osd_bitmap *bitmap)
{
	int i;
	data16_t *base=cps1_buffered_obj+cps1_last_sprite_offset;
	for (i=cps1_last_sprite_offset; i>=0; i-=4)
	{
		const int mask=0x7fff;
		int x=*(base+0);
		int y=*(base+1);
		int code  =*(base+2);
		int colour=*(base+3);
		int col=colour&0x1f;

		x-=0x20;
		y+=0x20;

		if (cps1_game_config->kludge == 7)
		{
			code += 0x4000;
		}
		if (cps1_game_config->kludge == 1 && code >= 0x01000)
		{
			code += 0x4000;
		}
		if (cps1_game_config->kludge == 2 && code >= 0x02a00)
		{
			code += 0x4000;
		}

		if (colour & 0xff00 )
		{
			/* handle blocked sprites */
			int nx=(colour & 0x0f00) >> 8;
			int ny=(colour & 0xf000) >> 12;
			int nxs,nys,sx,sy;
			nx++;
			ny++;

			if (colour & 0x40)
			{
				/* Y flip */
				if (colour &0x20)
				{
					for (nys=0; nys<ny; nys++)
					{
						for (nxs=0; nxs<nx; nxs++)
						{
							sx = (x+nxs*16) & 0x1ff;
							sy = (y+nys*16) & 0x1ff;

							cps1_draw_tile16_pri(bitmap,Machine->gfx[0],
								code+(nx-1)-nxs+0x10*(ny-1-nys),
								col&0x1f,
								1,1,
								sx,sy,mask);
						}
					}
				}
				else
				{
					for (nys=0; nys<ny; nys++)
					{
						for (nxs=0; nxs<nx; nxs++)
						{
							sx = (x+nxs*16) & 0x1ff;
							sy = (y+nys*16) & 0x1ff;

							cps1_draw_tile16_pri(bitmap,Machine->gfx[0],
								code+nxs+0x10*(ny-1-nys),
								col&0x1f,
								0,1,
								sx,sy,mask );
						}
					}
				}
			}
			else
			{
				if (colour &0x20)
				{
					for (nys=0; nys<ny; nys++)
					{
						for (nxs=0; nxs<nx; nxs++)
						{
							sx = (x+nxs*16) & 0x1ff;
							sy = (y+nys*16) & 0x1ff;

							cps1_draw_tile16_pri(bitmap,Machine->gfx[0],
								code+(nx-1)-nxs+0x10*nys,
								col&0x1f,
								1,0,
								sx,sy,mask
								);
						}
					}
				}
				else
				{
					for (nys=0; nys<ny; nys++)
					{
						for (nxs=0; nxs<nx; nxs++)
						{
							sx = (x+nxs*16) & 0x1ff;
							sy = (y+nys*16) & 0x1ff;

							cps1_draw_tile16_pri(bitmap,Machine->gfx[0],
								code+nxs+0x10*nys,
								col&0x1f,
								0,0,
								sx,sy, mask);
						}
					}
				}
			}
		}
		else
		{
			/* Simple case... 1 sprite */
			cps1_draw_tile16_pri(bitmap,Machine->gfx[0],
				   code,
				   col&0x1f,
				   colour&0x20,colour&0x40,
				   x & 0x1ff,y & 0x1ff,mask);
		}
		base -= 4;
	}
}

int cps2_render_sprites(struct osd_bitmap *bitmap, int drawpriority)
{
	int i;
	int ret=0;
	data16_t *base=cps2_buffered_obj;


#ifdef MAME_DEBUG
	if (keyboard_pressed(KEYCODE_Z) && keyboard_pressed(KEYCODE_1+drawpriority))
	{
		return 0;
	}
#endif

	for (i=0; i<=cps2_last_sprite_offset; i+=4)
	{
		const int mask=0x7fff;
		int x=*(base+0);
		int y=*(base+1);
		int priority=(x>>13)&0x07;

		if (priority == drawpriority)
		{
			int code  =*(base+2)+((y & 0x6000) <<3);
			int colour=*(base+3);
			int col=colour&0x1f;
			ret = 1;

			x-=0x20;
			y+=0x20;

			if (colour & 0xff00 )
			{
				/* handle blocked sprites */
				int nx=(colour & 0x0f00) >> 8;
				int ny=(colour & 0xf000) >> 12;
				int nxs,nys,sx,sy;
				nx++;
				ny++;

				if (colour & 0x40)
				{
					/* Y flip */
					if (colour &0x20)
					{
						for (nys=0; nys<ny; nys++)
						{
							for (nxs=0; nxs<nx; nxs++)
							{
								sx = (x+nxs*16) & 0x3ff;
								sy = (y+nys*16) & 0x3ff;

								cps1_draw_tile16(bitmap,Machine->gfx[0],
									code+(nx-1)-nxs+0x10*(ny-1-nys),
									col&0x1f,
									1,1,
									sx,sy,mask);
							}
						}
					}
					else
					{
						for (nys=0; nys<ny; nys++)
						{
							for (nxs=0; nxs<nx; nxs++)
							{
								sx = (x+nxs*16) & 0x3ff;
								sy = (y+nys*16) & 0x3ff;

								cps1_draw_tile16(bitmap,Machine->gfx[0],
									code+nxs+0x10*(ny-1-nys),
									col&0x1f,
									0,1,
									sx,sy,mask );
							}
						}
					}
				}
				else
				{
					if (colour &0x20)
					{
						for (nys=0; nys<ny; nys++)
						{
							for (nxs=0; nxs<nx; nxs++)
							{
								sx = (x+nxs*16) & 0x3ff;
								sy = (y+nys*16) & 0x3ff;

								cps1_draw_tile16(bitmap,Machine->gfx[0],
									code+(nx-1)-nxs+0x10*nys,
									col&0x1f,
									1,0,
									sx,sy,mask
									);
							}
						}
					}
					else
					{
						for (nys=0; nys<ny; nys++)
						{
							for (nxs=0; nxs<nx; nxs++)
							{
								sx = (x+nxs*16) & 0x3ff;
								sy = (y+nys*16) & 0x3ff;

								cps1_draw_tile16(bitmap,Machine->gfx[0],
									code+nxs+0x10*nys,
									col&0x1f,
									0,0,
									sx,sy, mask);
							}
						}
					}
				}
			}
			else
			{
				/* Simple case... 1 sprite */
				cps1_draw_tile16(bitmap,Machine->gfx[0],
					   code,
					   col&0x1f,
					   colour&0x20,colour&0x40,
					   x & 0x3ff,y & 0x3ff,mask);
			}
		}
		base += 4;
	}
	return ret;
}


/***************************************************************************

  Scroll 2 (16x16 layer)

  Attribute word layout:
  0x0001	colour
  0x0002	colour
  0x0004	colour
  0x0008	colour
  0x0010	colour
  0x0020	X Flip
  0x0040	Y Flip
  0x0080	??? Priority
  0x0100	??? Priority
  0x0200
  0x0400
  0x0800
  0x1000
  0x2000
  0x4000
  0x8000


***************************************************************************/

INLINE void cps1_palette_scroll2(unsigned short *base)
{
	int offs, code, colour;
	int basecode=cps1_game_config->bank_scroll2*0x04000;

	for (offs=cps1_scroll2_size-4; offs>=0; offs-=4)
	{
		code=basecode+cps1_scroll2[offs/2];
		colour=cps1_scroll2[(offs+2)/2]&0x1f;
		if (code < cps1_max_tile16)
		{
			base[colour] |= cps1_tile16_pen_usage[code];
		}
	}
}

void cps1_render_scroll2_bitmap(struct osd_bitmap *bitmap)
{
	int sx, sy;
	int ny=(scroll2y>>4);	  /* Rough Y */
	int base=cps1_game_config->bank_scroll2*0x04000;
	const int startcode=cps1_game_config->start_scroll2;
	const int endcode=cps1_game_config->end_scroll2;
	const int kludge=cps1_game_config->kludge;

	for (sx=CPS1_SCROLL2_WIDTH-1; sx>=0; sx--)
	{
		int n=ny;
		for (sy=0x09*2-1; sy>=0; sy--)
		{
			long newvalue;
			int offsy, offsx, offs, colour, code;

			n&=0x3f;
			offsy  = ((n&0x0f)*4 | ((n&0x30)*0x100))&0x3fff;
			offsx=(sx*0x040)&0xfff;
			offs=offsy+offsx;

			colour=cps1_scroll2[(offs+2)/2];

			newvalue=*(long*)(&cps1_scroll2[offs/2]);
			if ( newvalue != *(long*)(&cps1_scroll2_old[offs/2]) )
			{
				*(long*)(&cps1_scroll2_old[offs/2])=newvalue;
				code=cps1_scroll2[offs/2];
				if ( code >= startcode && code <= endcode
					/*
					MERCS has an gap in the scroll 2 layout
					(bad tiles at start of level 2)*/
					&&	!(kludge == 4 && (code >= 0x1e00 && code < 0x5400))
					)
				{
					code += base;
					cps1_draw_tile16_bmp(bitmap,
						Machine->gfx[2],
						code,
						colour&0x1f,
						colour&0x20,colour&0x40,
						16*sx, 16*n);
				}
				else
				{
					cps1_draw_blank16(bitmap, 16*sx, 16*n);
				}
				//cps1_print_debug_tile_info(bitmap, 16*sx, 16*n, colour,1);
			}
			n++;
		}
	}
}


void cps1_render_scroll2_high(struct osd_bitmap *bitmap)
{
#ifdef LAYER_DEBUG
	static int s=0;
#endif
	int sx, sy;
	int nxoffset=(scroll2x&0x0f)+32;    /* Smooth X */
	int nyoffset=(scroll2y&0x0f);    /* Smooth Y */
	int nx=(scroll2x>>4);	  /* Rough X */
	int ny=(scroll2y>>4)-4;	/* Rough Y */
	int base=cps1_game_config->bank_scroll2*0x04000;
	const int startcode=cps1_game_config->start_scroll2;
	const int endcode=cps1_game_config->end_scroll2;
	const int kludge=cps1_game_config->kludge;

	for (sx=0; sx<0x32/2+4; sx++)
	{
		for (sy=0; sy<0x09*2; sy++)
		{
			int offsy, offsx, offs, colour, code, transp;
			int n;
			n=ny+sy+2;
			offsy  = ((n&0x0f)*4 | ((n&0x30)*0x100))&0x3fff;
			offsx=((nx+sx)*0x040)&0xfff;
			offs=offsy+offsx;
			offs &= 0x3fff;

			code=cps1_scroll2[offs/2];

			if ( code >= startcode && code <= endcode
				/*
				MERCS has an gap in the scroll 2 layout
				(bad tiles at start of level 2)*/
				&&	!(kludge == 4 && (code >= 0x1e00 && code < 0x5400))
				)
			{
				colour=cps1_scroll2[(offs+2)/2];

				transp=cps1_transparency_scroll[(colour & 0x0180)>>7];

				cps1_draw_tile16(priority_bitmap,
							Machine->gfx[2],
							code+base,
							colour&0x1f,
							colour&0x20,colour&0x40,
							16*sx-nxoffset,
							16*sy-nyoffset,
							transp);
			}
		}
	}
}

void cps1_render_scroll2_low(struct osd_bitmap *bitmap)
{
	int scrly=-(scroll2y-0x20);
	int scrlx=-(scroll2x+0x40-0x20);

	if (cps1_flip_screen)
	{
		scrly=(CPS1_SCROLL2_HEIGHT*16)-scrly;
	}

	cps1_render_scroll2_bitmap(cps1_scroll2_bitmap);

	copyscrollbitmap(bitmap,cps1_scroll2_bitmap,1,&scrlx,1,&scrly,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
}


void cps1_render_scroll2_distort(struct osd_bitmap *bitmap)
{
	int scrly=-scroll2y;
	int i,scrollx[1024];
	int otheroffs;

/*
	Games known to use row scrolling:

	SF2
	Mega Twins (underwater, cave)
	Carrier Air Wing (hazy background at beginning of mission 8, put 07 at ff8501 to jump there)
	Magic Sword (fire on floor 3; screen distort after continue)
	Varth (title screen)
	Bionic Commando (end game sequence)
*/

	if (cps1_flip_screen)
		scrly=(CPS1_SCROLL2_HEIGHT*16)-scrly;

	cps1_render_scroll2_bitmap(cps1_scroll2_bitmap);

	otheroffs = cps1_port(CPS1_ROWSCROLL_OFFS);

	for (i = 0;i < 256;i++)
		scrollx[(i - scrly) & 0x3ff] = -(scroll2x+0x40-0x20) - cps1_other[(i + otheroffs) & 0x3ff];

	scrly+=0x20;

	copyscrollbitmap(bitmap,cps1_scroll2_bitmap,1024,scrollx,1,&scrly,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
}


/***************************************************************************

  Scroll 3 (32x32 layer)

  Attribute word layout:
  0x0001	colour
  0x0002	colour
  0x0004	colour
  0x0008	colour
  0x0010	colour
  0x0020	X Flip
  0x0040	Y Flip
  0x0080
  0x0100
  0x0200
  0x0400
  0x0800
  0x1000
  0x2000
  0x4000
  0x8000

***************************************************************************/

void cps1_palette_scroll3(unsigned short *base)
{
	int sx,sy;
	int nx=(scroll3x>>5)+1;
	int ny=(scroll3y>>5)-1;
	int basecode=cps1_game_config->bank_scroll3*0x01000;

	for (sx=0; sx<0x32/4+2; sx++)
	{
		for (sy=0; sy<0x20/4+2; sy++)
		{
			int offsy, offsx, offs, colour, code;
			int n;
			n=ny+sy;
			offsy  = ((n&0x07)*4 | ((n&0xf8)*0x0100))&0x3fff;
			offsx=((nx+sx)*0x020)&0x7ff;
			offs=offsy+offsx;
			offs &= 0x3fff;
			code=basecode+cps1_scroll3[offs/2];
			if (cps1_game_config->kludge == 2 && code >= 0x01500)
			{
				code -= 0x1000;
			}
			if (cps1_game_config->kludge == 8 && code >= 0x05800)
			{
				code -= 0x4000;
			}
			colour=cps1_scroll3[(offs+2)/2];
			if (code < cps1_max_tile32)
			{
				base[colour&0x1f] |= cps1_tile32_pen_usage[code];
			}
		}
	}
}


void cps1_render_scroll3(struct osd_bitmap *bitmap, int priority)
{
	int sx,sy;
	int nxoffset=scroll3x&0x1f;
	int nyoffset=scroll3y&0x1f;
	int nx=(scroll3x>>5)+1;
	int ny=(scroll3y>>5)-1;
	int basecode=cps1_game_config->bank_scroll3*0x01000;
	const int startcode=cps1_game_config->start_scroll3;
	const int endcode=cps1_game_config->end_scroll3;

	for (sx=1; sx<0x32/4+2; sx++)
	{
		for (sy=1; sy<0x20/4+2; sy++)
		{
			int offsy, offsx, offs, colour, code;
			int n;
			int transp;
			n=ny+sy;
			offsy  = ((n&0x07)*4 | ((n&0xf8)*0x0100))&0x3fff;
			offsx=((nx+sx)*0x020)&0x7ff;
			offs=offsy+offsx;
			offs &= 0x3fff;
			code=cps1_scroll3[offs/2];
			if (code >= startcode && code <= endcode)
			{
				code+=basecode;
				if (cps1_game_config->kludge == 2 && code >= 0x01500)
				{
					code -= 0x1000;
				}
				if (cps1_game_config->kludge == 8 && code >= 0x05800)
				{
					code -= 0x4000;
				}

				colour=cps1_scroll3[(offs+2)/2];
				if (priority)
				{
					transp=cps1_transparency_scroll[(colour & 0x0180)>>7];
					cps1_draw_tile32(priority_bitmap,Machine->gfx[3],
							code,
							colour&0x1f,
							colour&0x20,colour&0x40,
							32*sx-nxoffset,32*sy-nyoffset,
							transp);
				}
				else
				{
					transp = 0x7fff;
					cps1_draw_tile32(bitmap,Machine->gfx[3],
							code,
							colour&0x1f,
							colour&0x20,colour&0x40,
							32*sx-nxoffset,32*sy-nyoffset,
							transp);
				}
			}
		}
	}
}


/* the following is COMPLETELY WRONG. It's there just to draw something */
void cps1_render_stars(struct osd_bitmap *bitmap)
{
	if (cps1_stars_enabled)
	{
		int offs;


		for (offs = 0;offs < stars_rom_size/2;offs++)
		{
			int col = stars_rom[8*offs+4];
			if (col != 0x0f)
			{
				int sx = (offs / 256) * 32;
				int sy = (offs % 256);
				sx = (sx - stars2x - 64 + (col & 0x1f)) & 0x1ff;
				sy = (sy - stars2y) & 0xff;
				if (cps1_flip_screen)
				{
					sx = 383 - sx;
					sy = 255 - sy;
				}

				col = ((col & 0xe0) >> 1) + (cpu_getcurrentframe()/16 & 0x0f);

				if (sx+32 <= Machine->visible_area.max_x &&
						sy+32 <= Machine->visible_area.max_y)
					plot_pixel(bitmap,sx+32,sy+32,Machine->pens[0xa00+col]);
			}
		}

		for (offs = 0;offs < stars_rom_size/2;offs++)
		{
			int col = stars_rom[8*offs];
			if (col != 0x0f)
			{
				int sx = (offs / 256) * 32;
				int sy = (offs % 256);
				sx = (sx - stars1x - 64+ (col & 0x1f)) & 0x1ff;
				sy = (sy - stars1y) & 0xff;
				if (cps1_flip_screen)
				{
					sx = 383 - sx;
					sy = 255 - sy;
				}

				col = ((col & 0xe0) >> 1) + (cpu_getcurrentframe()/16 & 0x0f);

				if (sx+32 <= Machine->visible_area.max_x &&
						sy+32 <= Machine->visible_area.max_y)
					plot_pixel(bitmap,sx+32,sy+32,Machine->pens[0x800+col]);
			}
		}
	}
}


void cps1_render_layer(struct osd_bitmap *bitmap, int layer, int distort)
{
	if (cps1_layer_enabled[layer])
	{
		switch (layer)
		{
			case 0:
                cps1_render_sprites(bitmap);
				break;
			case 1:
				cps1_render_scroll1(bitmap, 0);
				break;
			case 2:
				if (distort)
					cps1_render_scroll2_distort(bitmap);
				else
					cps1_render_scroll2_low(bitmap);
				break;
			case 3:
				cps1_render_scroll3(bitmap, 0);
				break;
		}
	}
}

void cps1_render_high_layer(struct osd_bitmap *bitmap, int layer)
{
	if (cps1_layer_enabled[layer])
	{
		switch (layer)
		{
			case 0:
				/* there are no high priority sprites */
				break;
			case 1:
				cps1_render_scroll1(bitmap, 1);
				break;
			case 2:
				cps1_render_scroll2_high(bitmap);
				break;
			case 3:
				cps1_render_scroll3(bitmap, 1);
				break;
		}
	}
}


/***************************************************************************

	Refresh screen

***************************************************************************/

void cps1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned short palette_usage[cps1_palette_entries];
    int layercontrol,l0,l1,l2,l3;
	int i,offset;
	int distort_scroll2=0;
	int videocontrol=cps1_port(0x22);
	int old_flip;


#if MAME_DEBUG
    if (keyboard_pressed_memory(KEYCODE_T))
    {
        cps_view_tiles=~cps_view_tiles;
    }
    if (cps_view_tiles || cps_version == 99)
    {
        memset(palette_used_colors, PALETTE_COLOR_VISIBLE, 4096);
        cps_tileviewer_screenrefresh(bitmap, full_refresh);
        return;
    }
#endif

	old_flip=cps1_flip_screen;
	cps1_flip_screen=videocontrol&0x8000;
	if (old_flip != cps1_flip_screen)
	{
		 /* Mark all of scroll 2 as dirty */
		memset(cps1_scroll2_old, 0xff, cps1_scroll2_size);
	}

	layercontrol = cps1_output[cps1_game_config->layer_control/2];

	distort_scroll2 = videocontrol & 0x01;

	/* Get video memory base registers */
	cps1_get_video_base();

	/* Find the offset of the last sprite in the sprite table */
    cps1_find_last_sprite();
    if (cps_version == 2)
    {
        cps2_find_last_sprite();
    }
	/* Build palette */
	cps1_build_palette();

	/* Compute the used portion of the palette */
	memset (palette_usage, 0, sizeof (palette_usage));
    cps_palette_sprites (&palette_usage[cps1_obj_palette], cps1_buffered_obj, cps1_last_sprite_offset);
    if (cps_version == 2)
    {
        cps_palette_sprites (&palette_usage[cps1_obj_palette], cps2_buffered_obj, cps2_last_sprite_offset);
    }
    if (cps1_layer_enabled[1])
		cps1_palette_scroll1 (&palette_usage[cps1_scroll1_palette]);
	if (cps1_layer_enabled[2])
		cps1_palette_scroll2 (&palette_usage[cps1_scroll2_palette]);
	else
		memset(cps1_scroll2_old, 0xff, cps1_scroll2_size);
	if (cps1_layer_enabled[3])
		cps1_palette_scroll3 (&palette_usage[cps1_scroll3_palette]);

	for (i = offset = 0; i < cps1_palette_entries; i++)
	{
		int usage = palette_usage[i];
		if (usage)
		{
			int j;
			for (j = 0; j < 15; j++)
			{
				if (usage & (1 << j))
					palette_used_colors[offset++] = PALETTE_COLOR_USED;
				else
					palette_used_colors[offset++] = PALETTE_COLOR_UNUSED;
			}
			palette_used_colors[offset++] = PALETTE_COLOR_TRANSPARENT;
		}
		else
		{
			memset (&palette_used_colors[offset], PALETTE_COLOR_UNUSED, 16);
			offset += 16;
		}
	}

	if (cps1_stars_enabled)
	{
		for (i = 0;i < 128;i++)
		{
			palette_used_colors[0x800+i] = PALETTE_COLOR_VISIBLE;
			palette_used_colors[0xa00+i] = PALETTE_COLOR_VISIBLE;
		}
	}

	if (palette_recalc ())
	{
		 /* Mark all of scroll 2 as dirty */
		memset(cps1_scroll2_old, 0xff, cps1_scroll2_size);
	}

	/* Blank screen */
//	fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);
// TODO: the draw functions don't clip correctly at the sides of the screen, so
// for now let's clear the whole bitmap otherwise ctrl-f11 would show wrong counts
	fillbitmap(bitmap,palette_transparent_pen,0);

	cps1_render_stars(bitmap);

	/* Draw layers (0 = sprites, 1-3 = tilemaps) */
	l0 = (layercontrol >> 0x06) & 03;
	l1 = (layercontrol >> 0x08) & 03;
	l2 = (layercontrol >> 0x0a) & 03;
	l3 = (layercontrol >> 0x0c) & 03;
	fillbitmap(priority_bitmap,0,NULL);

	if (cps_version == 1)
	{
		cps1_render_layer(bitmap,l0,distort_scroll2);
		if (l1 == 0) cps1_render_high_layer(bitmap,l0); /* prepare mask for sprites */
		cps1_render_layer(bitmap,l1,distort_scroll2);
		if (l2 == 0) cps1_render_high_layer(bitmap,l1); /* prepare mask for sprites */
		cps1_render_layer(bitmap,l2,distort_scroll2);
		if (l3 == 0) cps1_render_high_layer(bitmap,l2); /* prepare mask for sprites */
		cps1_render_layer(bitmap,l3,distort_scroll2);
	}
	else
	{
		int pri_ctrl = cps2_port(CPS2_PRI_BASE);
		int l0pri,l1pri,l2pri,l3pri;

		l0pri = (pri_ctrl >> 4*l0) & 0x0f;
		l1pri = (pri_ctrl >> 4*l1) & 0x0f;
		l2pri = (pri_ctrl >> 4*l2) & 0x0f;
		l3pri = (pri_ctrl >> 4*l3) & 0x0f;

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
	usrintf_showmessage("order: %d (%d) %d (%d) %d (%d) %d (%d)",l0,l0pri,l1,l1pri,l2,l2pri,l3,l3pri);
#endif

		/* take out the CPS1 sprites layer */
		if (l0 == 0) { l0 = l1; l1 = 0; l0pri = l1pri; }
		if (l1 == 0) { l1 = l2; l2 = 0; l1pri = l2pri; }
		if (l2 == 0) { l2 = l3; l3 = 0; l2pri = l3pri; }

//		if (l1pri < l0pri) usrintf_showmessage("l1pri < l0pri!");
//		if (l2pri < l1pri) usrintf_showmessage("l2pri < l1pri!");

		for (i = 0;i <= l0pri;i++)
			cps2_render_sprites(bitmap,i);
		cps1_render_layer(bitmap,l0,distort_scroll2);
		for (i = l0pri+1;i <= l1pri;i++)
			cps2_render_sprites(bitmap,i);
		cps1_render_layer(bitmap,l1,distort_scroll2);
		for (i = l1pri+1;i <= l2pri;i++)
			cps2_render_sprites(bitmap,i);
		cps1_render_layer(bitmap,l2,distort_scroll2);
		for (i = l2pri+1;i <= 7;i++)
			cps2_render_sprites(bitmap,i);
	}

#if CPS1_DUMP_VIDEO
	if (keyboard_pressed(KEYCODE_F))
	{
		cps1_dump_video();
	}
#endif
}

void cps1_eof_callback(void)
{
	/* Get video memory base registers */
	cps1_get_video_base();

	/* CPS1 sprites have to be delayed one frame */
    memcpy(cps1_buffered_obj, cps1_obj, cps1_obj_size);
    if (cps_version == 2)
    {
		/* CPS2 sprites have to be delayed two frames */
        memcpy(cps2_buffered_obj, cps2_buffered_obj+cps2_obj_size, cps2_obj_size);
        memcpy(cps2_buffered_obj+cps2_obj_size, cps2_objram, cps2_obj_size);
    }
}


#include "cps2.c"



#else	/* SELF_INCLUDE */
/* this is #included several times generate 8-bit and 16-bit versions */

{
	int i, j;
	UINT32 dwval;
	UINT32 *src;
	const UINT32 *paldata;
	UINT32 n;
	DATATYPE *bm;

	if (code > max || (tpens & pusage[code])==0)
	{
		/* Do not draw blank object */
		return;
	}
	src = cps1_gfx+code*delta;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;
		temp=sx;
		sx=sy;
		sy=dest->height-temp-size;
		temp=flipx;
		flipx=flipy;
		flipy=!temp;
	}

	if (cps1_flip_screen)
	{
		/* Handle flipped screen */
		flipx=!flipx;
		flipy=!flipy;
		sx=dest->width-sx-size;
		sy=dest->height-sy-size;
	}

	if (sx<0 || sx > dest->width-size || sy<0 || sy>dest->height-size )
	{
		/* Don't draw clipped tiles (for sprites) */
		return;
	}

	paldata=&gfx->colortable[gfx->color_granularity * color];

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int bmdelta,dir;

		bmdelta = (dest->line[1] - dest->line[0]);
		dir = 1;
		if (flipy)
		{
			bmdelta = -bmdelta;
			dir = -1;
			sy += size-1;
		}
		if (flipx) sx+=size-1;
		for (i=0; i<size; i++)
		{
			int ny=sy;
			for (j=0; j<size/8; j++)
			{
				dwval=*src;
				n=(dwval>>28)&0x0f;
				bm = (DATATYPE *)dest->line[ny]+sx;
				IF_NOT_TRANSPARENT(n,sx,ny) bm[0]=PALDATA(n);
				n=(dwval>>24)&0x0f;
				bm = (DATATYPE *)(((unsigned char *)bm) + bmdelta);
				IF_NOT_TRANSPARENT(n,sx,ny+dir) bm[0]=PALDATA(n);
				n=(dwval>>20)&0x0f;
				bm = (DATATYPE *)(((unsigned char *)bm) + bmdelta);
				IF_NOT_TRANSPARENT(n,sx,ny+2*dir) bm[0]=PALDATA(n);
				n=(dwval>>16)&0x0f;
				bm = (DATATYPE *)(((unsigned char *)bm) + bmdelta);
				IF_NOT_TRANSPARENT(n,sx,ny+3*dir) bm[0]=PALDATA(n);
				n=(dwval>>12)&0x0f;
				bm = (DATATYPE *)(((unsigned char *)bm) + bmdelta);
				IF_NOT_TRANSPARENT(n,sx,ny+4*dir) bm[0]=PALDATA(n);
				n=(dwval>>8)&0x0f;
				bm = (DATATYPE *)(((unsigned char *)bm) + bmdelta);
				IF_NOT_TRANSPARENT(n,sx,ny+5*dir) bm[0]=PALDATA(n);
				n=(dwval>>4)&0x0f;
				bm = (DATATYPE *)(((unsigned char *)bm) + bmdelta);
				IF_NOT_TRANSPARENT(n,sx,ny+6*dir) bm[0]=PALDATA(n);
				n=dwval&0x0f;
				bm = (DATATYPE *)(((unsigned char *)bm) + bmdelta);
				IF_NOT_TRANSPARENT(n,sx,ny+7*dir) bm[0]=PALDATA(n);
				if (flipy) ny-=8;
				else ny+=8;
				src++;
			}
			if (flipx) sx--;
			else sx++;
			src+=srcdelta;
		}
	}
	else
	{
		if (flipy) sy+=size-1;
		if (flipx)
		{
			sx+=size;
			for (i=0; i<size; i++)
			{
				int x,y;
				x=sx;
				if (flipy) y=sy-i;
				else y=sy+i;
				bm=(DATATYPE *)dest->line[y]+sx;
				for (j=0; j<size/8; j++)
				{
					dwval=*src;
					n=(dwval>>28)&0x0f;
					IF_NOT_TRANSPARENT(n,x-1,y) bm[-1]=PALDATA(n);
					n=(dwval>>24)&0x0f;
					IF_NOT_TRANSPARENT(n,x-2,y) bm[-2]=PALDATA(n);
					n=(dwval>>20)&0x0f;
					IF_NOT_TRANSPARENT(n,x-3,y) bm[-3]=PALDATA(n);
					n=(dwval>>16)&0x0f;
					IF_NOT_TRANSPARENT(n,x-4,y) bm[-4]=PALDATA(n);
					n=(dwval>>12)&0x0f;
					IF_NOT_TRANSPARENT(n,x-5,y) bm[-5]=PALDATA(n);
					n=(dwval>>8)&0x0f;
					IF_NOT_TRANSPARENT(n,x-6,y) bm[-6]=PALDATA(n);
					n=(dwval>>4)&0x0f;
					IF_NOT_TRANSPARENT(n,x-7,y) bm[-7]=PALDATA(n);
					n=dwval&0x0f;
					IF_NOT_TRANSPARENT(n,x-8,y) bm[-8]=PALDATA(n);
					bm-=8;
					x-=8;
					src++;
				}
				src+=srcdelta;
			}
		}
		else
		{
			for (i=0; i<size; i++)
			{
				int x,y;
				x=sx;
				if (flipy) y=sy-i;
				else y=sy+i;
				bm=(DATATYPE *)dest->line[y]+sx;
				for (j=0; j<size/8; j++)
				{
					dwval=*src;
					n=(dwval>>28)&0x0f;
					IF_NOT_TRANSPARENT(n,x+0,y) bm[0]=PALDATA(n);
					n=(dwval>>24)&0x0f;
					IF_NOT_TRANSPARENT(n,x+1,y) bm[1]=PALDATA(n);
					n=(dwval>>20)&0x0f;
					IF_NOT_TRANSPARENT(n,x+2,y) bm[2]=PALDATA(n);
					n=(dwval>>16)&0x0f;
					IF_NOT_TRANSPARENT(n,x+3,y) bm[3]=PALDATA(n);
					n=(dwval>>12)&0x0f;
					IF_NOT_TRANSPARENT(n,x+4,y) bm[4]=PALDATA(n);
					n=(dwval>>8)&0x0f;
					IF_NOT_TRANSPARENT(n,x+5,y) bm[5]=PALDATA(n);
					n=(dwval>>4)&0x0f;
					IF_NOT_TRANSPARENT(n,x+6,y) bm[6]=PALDATA(n);
					n=dwval&0x0f;
					IF_NOT_TRANSPARENT(n,x+7,y) bm[7]=PALDATA(n);
					bm+=8;
					x+=8;
					src++;
				}
				src+=srcdelta;
			}
		}
	}
}
#endif	/* SELF_INCLUDE */
