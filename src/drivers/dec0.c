/***************************************************************************

	Data East 16 bit games - Bryan McPhail, mish@tendril.force9.net

  This file contains drivers for:

  	* Bad Dudes vs Dragonninja
    * Dragonninja
    * Robocop (Pirate rom set)
    * Hippodrome
    * Heavy Barrel (Romlist Set - 4 roms missing)
    * Heavy Barrel (Powerjaw Set - complete)
    * Sly Spy
    * Midnight Resistance

  Notes:
	Missing scroll field in Hippodrome
	Hippodrome can crash if you fight 'Serpent' enemy
	Midnight Resistance:
	  The final sequence is wrong (missing front layer which should hide the
	  sun until the grass field scrolls in place).
	  At the end you are asked to enter your name using black letters on black
	  background...

  Dark Seal/Gate Of Doom may fit in this driver but it's processor is unknown.

  Thanks to Gouky & Richard Bush for information along the way, especially
  Gouky's patch for Bad Dudes & YM3812 information!

Cheats:

Hippodrome: Level number held in 0xFF8011

Bad Dudes: level number held in FF821D
					Put a bpx at 0x1b94 to halt at level check and choose level or end-seq
Sly Spy: bpx at 5d8 for level check, can go to any level.
(see comments in patch section below).

Mid res: bpx at c02 for level check, can go straight to credits.
         Level number held in 100006 (word)
         bpx 10a6 - so you can choose level at start of game.
         Put PC to 0xa1e in game to trigger continue screen

Recent fixes:
	Tidied up dip switch settings
  Identified rowscroll bit (rather than byte :) and removed Sly Spy pf1 row
    scroll patch.  See comments in vidhrdw.

  Improved Heavy Barrel 8751 emulation
  Fixed Heavy Barrel title screen & attract mode
  Special weapons now appear in Heavy Barrel, however their placement can be
    wrong (or _is_ wrong even, they are controlled by the 8751 and I can't
    figure out what the correct values are).
  Removed all Heavy Barrel patches but one (as result of better 8751 emu)
  Fixed Heavy Barrel music speed (far too fast before)
  Implemented pf3 rowscroll in Heavy Barrel - sea & waterfall in level 1 and
    conveyor belts in level 5.  Column scroll registers are also altered in
    these areas but I can't work out what they should do...

  Fixed all playfields on Sly Spy (boat level still strange at start)
  Fixed corrupt 'Game over' screen in Sly Spy
  Fixed pf2 being corrupt when returning from Continue screen in Sly Spy
  Fixed end sequence of Sly Spy

  Identified column scroll areas - used for end credits in Bad Dudes, Robocop(?)
  Fixed end sequence of Bad Dudes
  Fixed end sequence of Dragon-ninja (different to above, quite nice really :)
  Fixed Robocop bonus stage (problem caused by palette remapping...  but now
  	high-score table screen is pretty bad..)
  Tweaked the YM2203 volume & frequency a bit (too quiet, too high before)

To do:
  Robocop end still doesnt work - all colours & playfields set to 0?!!?
  Sprite/background priority in boat stage of Sly Spy & forest level of Bad
    Dudes, current drivers MAY be correct (Sly Spy is certainly wrong), we
	need real boards to check against.

  Colour cycling in Mid Res level 7 (and 6?) kills performance..
  No sound in Sly Spy or MidRes (what processor is it?!)

  Better palette selection could avoid 256 colour overflow

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6502/M6502.h"

/* Video emulation definitions */
extern unsigned char *dec0_sprite,*dec0_mem;

int  dec0_vh_start(void);
void dec0_vh_stop(void);
void dec0_vh_screenrefresh(struct osd_bitmap *bitmap);
void midres_vh_screenrefresh(struct osd_bitmap *bitmap);
void slyspy_vh_screenrefresh(struct osd_bitmap *bitmap);
void hippodrm_vh_screenrefresh(struct osd_bitmap *bitmap);
void heavyb_vh_screenrefresh(struct osd_bitmap *bitmap);
void robocop_vh_screenrefresh(struct osd_bitmap *bitmap);

void dec0_priority_w(int offset,int data);
void hippo_sprite_fix(int offset, int data);

extern unsigned char *dec0_pf1_rowscroll,*dec0_pf2_rowscroll,*dec0_pf3_rowscroll;
extern unsigned char *dec0_pf1_colscroll,*dec0_pf2_colscroll,*dec0_pf3_colscroll;

void dec0_pf1_control_0_w(int offset,int data);
void dec0_pf1_control_1_w(int offset,int data);
void dec0_pf1_rowscroll_w(int offset,int data);
void dec0_pf1_colscroll_w(int offset,int data);
void dec0_pf1_data_w(int offset,int data);
int dec0_pf1_data_r(int offset);
void dec0_pf2_control_0_w(int offset,int data);
void dec0_pf2_control_1_w(int offset,int data);
void dec0_pf2_rowscroll_w(int offset,int data);
void dec0_pf2_colscroll_w(int offset,int data);
void dec0_pf2_data_w(int offset,int data);
int dec0_pf2_data_r(int offset);
void dec0_pf3_control_0_w(int offset,int data);
void dec0_pf3_control_1_w(int offset,int data);
void dec0_pf3_rowscroll_w(int offset,int data);
void dec0_pf3_colscroll_w(int offset,int data);
int dec0_pf3_colscroll_r(int offset); /* Used only by Heavy Barrel */
void dec0_pf3_data_w(int offset,int data);
int dec0_pf3_data_r(int offset);

/* Palette mappers & memory writes */
void dec0_palette_24bit_rg(int offset,int data);
void dec0_palette_24bit_b(int offset,int data);
void dec0_palette_12bit_w(int offset, int data);

/* System prototypes - from machine/dec0.c */
extern int dec0_controls_read(int offset);
extern int dec0_rotary_read(int offset);
extern int midres_controls_read(int offset);
extern int slyspy_controls_read(int offset);
extern int robocop_interrupt(void);
extern int dude_interrupt(void);
extern int hippodrm_protection(int offset);
extern void hb_8751_write(int data);

extern int dec0_unknown1,dec0_unknown2; /* Temporary */

/******************************************************************************/

void dec0_30c010_w(int offset,int data)
{
	switch (offset)
	{
		case 0:
			dec0_priority_w(0,data);
			break;

	  case 2: /* Unknown write in many games */
      dec0_unknown1=data;
    	break;

		case 4:
			soundlatch_w(0,data & 0xff);
			cpu_cause_interrupt(1,INT_NMI);
			break;

    case 6: /* Heavy Barrel Intel 8751 microcontroller - unknown for other games */
    	hb_8751_write(data);
      dec0_unknown2=data; /* Temp */
      break;

		case 8:
			watchdog_reset_w(0,0);
			break;

    case 0xa: /* Many games write here only at startup for unknown reasons */
      hb_8751_write(0x9000); /* We can use this to init HB microcontroller */
 			if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - write %02x to unmapped memory address %06x\n",cpu_getpc(),data,0x30c010+offset);
			break;

		default:
			if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - write %02x to unmapped memory address %06x\n",cpu_getpc(),data,0x30c010+offset);
			break;
	}
}

/******************************************************************************/

static struct MemoryReadAddress dec0_readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
	{ 0x244000, 0x245fff, dec0_pf1_data_r },
	{ 0x300000, 0x30001f, dec0_rotary_read },
	{ 0x30c000, 0x30c00b, dec0_controls_read },
	{ 0xff8000, 0xffbfff, MRA_BANK1 }, /* Main ram */
	{ 0xffc000, 0xffcfff, MRA_BANK2 }, /* Sprites */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress dec0_writemem[] =
{
	{ 0x000000, 0x05ffff, MWA_ROM },

	{ 0x240000, 0x240007, dec0_pf1_control_0_w },	/* text layer */
	{ 0x240010, 0x240017, dec0_pf1_control_1_w },
 	{ 0x242000, 0x24207f, dec0_pf1_colscroll_w, &dec0_pf1_colscroll },
	{ 0x242400, 0x2427ff, dec0_pf1_rowscroll_w, &dec0_pf1_rowscroll },
	{ 0x244000, 0x245fff, dec0_pf1_data_w },

	{ 0x246000, 0x246007, dec0_pf2_control_0_w },	/* first tile layer */
	{ 0x246010, 0x246017, dec0_pf2_control_1_w },
	{ 0x248000, 0x24807f, dec0_pf2_colscroll_w, &dec0_pf2_colscroll },
	{ 0x248400, 0x2487ff, dec0_pf2_rowscroll_w, &dec0_pf2_rowscroll },
	{ 0x24a000, 0x24a7ff, dec0_pf2_data_w },

	{ 0x24c000, 0x24c007, dec0_pf3_control_0_w },	/* second tile layer */
	{ 0x24c010, 0x24c017, dec0_pf3_control_1_w },
	{ 0x24c800, 0x24c87f, dec0_pf3_colscroll_w, &dec0_pf3_colscroll },
	{ 0x24cc00, 0x24cfff, dec0_pf3_rowscroll_w, &dec0_pf3_rowscroll },
	{ 0x24d000, 0x24d7ff, dec0_pf3_data_w },

	{ 0x30c010, 0x30c01f, dec0_30c010_w },	/* Priority, sound, watchdog */
	{ 0x310000, 0x3107ff, dec0_palette_24bit_rg },	/* Red & Green bits */
	{ 0x314000, 0x3147ff, dec0_palette_24bit_b  },	/* Blue bits */
	{ 0xff8000, 0xffbfff, MWA_BANK1 },
	{ 0xffc000, 0xffcfff, MWA_BANK2, &dec0_sprite },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress robocop_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x242a00, 0x242bff, MRA_BANK3 },	/* ?? used during attract mode */
	{ 0x243200, 0x2433ff, MRA_BANK4 },	/* ?? used during attract mode */
	{ 0x243800, 0x243813, MRA_BANK5 },	/* ?? needed for the pictures at the beginning of the game to work */
	{ 0x244000, 0x245fff, dec0_pf1_data_r },
	{ 0x30c000, 0x30c00b, dec0_controls_read },
	{ 0xff8000, 0xffbfff, MRA_BANK1 }, /* Main ram */
	{ 0xffc000, 0xffcfff, MRA_BANK2 }, /* Sprites */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress robocop_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x240000, 0x240007, dec0_pf1_control_0_w },	/* text layer */
	{ 0x240010, 0x240017, dec0_pf1_control_1_w },
	{ 0x242000, 0x24207f, dec0_pf1_colscroll_w, &dec0_pf1_colscroll },
	{ 0x242400, 0x2427ff, dec0_pf1_rowscroll_w, &dec0_pf1_rowscroll },
	{ 0x242a00, 0x242bff, MWA_BANK3 },	/* ?? used during attract mode */
	{ 0x243200, 0x2433ff, MWA_BANK4 },	/* ?? used during attract mode */
	{ 0x243800, 0x243813, MWA_BANK5 },	/* ?? needed for the pictures at the beginning of the game to work */
	{ 0x244000, 0x245fff, dec0_pf1_data_w },

	{ 0x246000, 0x246007, dec0_pf2_control_0_w },	/* first tile layer */
	{ 0x246010, 0x246017, dec0_pf2_control_1_w },
	{ 0x248400, 0x2487ff, dec0_pf2_rowscroll_w, &dec0_pf2_rowscroll },
	{ 0x24a000, 0x24a7ff, dec0_pf2_data_w },

	{ 0x24c000, 0x24c007, dec0_pf3_control_0_w },	/* second tile layer */
	{ 0x24c010, 0x24c017, dec0_pf3_control_1_w },
	{ 0x24d000, 0x24d7ff, dec0_pf3_data_w },
	{ 0x30c010, 0x30c01f, dec0_30c010_w },	/* playfield priority at 30c010, */

	{ 0x310000, 0x3107ff, dec0_palette_24bit_rg },	/* Red & Green bits */
	{ 0x314000, 0x3147ff, dec0_palette_24bit_b },
	{ 0xff8000, 0xffbfff, MWA_BANK1, &dec0_mem },
	{ 0xffc000, 0xffcfff, MWA_BANK2, &dec0_sprite },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress heavyb_readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
	{ 0x244000, 0x245fff, dec0_pf1_data_r },
	{ 0x24c800, 0x24c87f, dec0_pf3_colscroll_r },
	{ 0x300000, 0x30001f, dec0_rotary_read },
	{ 0x30c000, 0x30c00b, dec0_controls_read },
	{ 0xff8000, 0xffbfff, MRA_BANK1 }, /* Main ram */
	{ 0xffc000, 0xffcfff, MRA_BANK2 }, /* Sprites */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress heavyb_writemem[] =
{
	{ 0x000000, 0x05ffff, MWA_ROM },
	{ 0x240000, 0x240007, dec0_pf1_control_0_w },	/* text layer */
	{ 0x240010, 0x240017, dec0_pf1_control_1_w },
	{ 0x242000, 0x24207f, dec0_pf1_colscroll_w, &dec0_pf1_colscroll },
	{ 0x242400, 0x2427ff, dec0_pf1_rowscroll_w, &dec0_pf1_rowscroll },
	{ 0x244000, 0x245fff, dec0_pf1_data_w },

	{ 0x246000, 0x246007, dec0_pf2_control_0_w },	/* first tile layer */
	{ 0x246010, 0x246017, dec0_pf2_control_1_w },
	{ 0x248000, 0x24807f, dec0_pf2_colscroll_w, &dec0_pf2_colscroll },
	{ 0x248400, 0x2487ff, dec0_pf2_rowscroll_w, &dec0_pf2_rowscroll },
	{ 0x24a000, 0x24a7ff, dec0_pf2_data_w },

	{ 0x24c000, 0x24c007, dec0_pf3_control_0_w },	/* second tile layer */
	{ 0x24c010, 0x24c017, dec0_pf3_control_1_w },
	{ 0x24c800, 0x24c87f, dec0_pf3_colscroll_w, &dec0_pf3_colscroll },
	{ 0x24cc00, 0x24cfff, dec0_pf3_rowscroll_w, &dec0_pf3_rowscroll },
	{ 0x24d000, 0x24d7ff, dec0_pf3_data_w },

	{ 0x30c010, 0x30c01f, dec0_30c010_w },	/* playfield priority at 30c010, */
	{ 0x310000, 0x3107ff, dec0_palette_24bit_rg },	/* Red & Green bits */
	{ 0x314000, 0x3147ff, dec0_palette_24bit_b  },	/* Blue bits */
	{ 0xff8000, 0xffbfff, MWA_BANK1 },
	{ 0xffc000, 0xffcfff, MWA_BANK2, &dec0_sprite },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress hippodrm_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x180000, 0x18001f, hippodrm_protection },
	{ 0x30c000, 0x30c00b, dec0_controls_read },
	{ 0xff8000, 0xffbfff, MRA_BANK1 },
	{ 0xffc000, 0xffcfff, MRA_BANK2 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress hippodrm_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x180000, 0x18001f, MWA_NOP },	/* ??? protection ??? */

  { 0x230000, 0x23007f, dec0_pf1_colscroll_w, &dec0_pf1_colscroll }, /* Temporary */

	{ 0x240000, 0x240007, dec0_pf1_control_0_w },	/* text layer */
	{ 0x240010, 0x240017, dec0_pf1_control_1_w },
	{ 0x242400, 0x2427ff, dec0_pf1_rowscroll_w, &dec0_pf1_rowscroll },
	{ 0x244000, 0x245fff, dec0_pf1_data_w },

	{ 0x246000, 0x246007, dec0_pf2_control_0_w },	/* first tile layer */
	{ 0x246010, 0x246017, dec0_pf2_control_1_w },
	{ 0x248400, 0x2487ff, dec0_pf2_rowscroll_w, &dec0_pf2_rowscroll },
	{ 0x24a000, 0x24a7ff, dec0_pf2_data_w },

	{ 0x30c010, 0x30c01f, dec0_30c010_w },	/* playfield priority at 30c010, */
	{ 0x310000, 0x3107ff, dec0_palette_24bit_rg },
	{ 0x314000, 0x3147ff, dec0_palette_24bit_b },
	{ 0xff8000, 0xffbfff, MWA_BANK1 },
	{ 0xffc000, 0xffc7ff, MWA_BANK2, &dec0_sprite },
	{ 0xffc800, 0xffcfff, hippo_sprite_fix },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress midres_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK1 },
	{ 0x120000, 0x1207ff, MRA_BANK2 },
	{ 0x180000, 0x18000f, midres_controls_read },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress midres_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK1 },
	{ 0x120000, 0x1207ff, MWA_BANK2, &dec0_sprite },
	{ 0x140000, 0x1407ff, dec0_palette_12bit_w },
	{ 0x160000, 0x160003, dec0_priority_w },
	{ 0x180008, 0x18000f, MWA_NOP }, /* ?? watchdog ?? */
	{ 0x1a0000, 0x1a0003, MWA_NOP },	/* sound */

	{ 0x200000, 0x200007, dec0_pf2_control_0_w },
	{ 0x200010, 0x200017, dec0_pf2_control_1_w },
	{ 0x220000, 0x2207ff, dec0_pf2_data_w },
	{ 0x240000, 0x24007f, dec0_pf2_colscroll_w, &dec0_pf2_colscroll },
	{ 0x240400, 0x2407ff, dec0_pf2_rowscroll_w, &dec0_pf2_rowscroll },

	{ 0x280000, 0x280007, dec0_pf3_control_0_w },
	{ 0x280010, 0x280017, dec0_pf3_control_1_w },
	{ 0x2a0000, 0x2a07ff, dec0_pf3_data_w },
	{ 0x2c0000, 0x2c007f, dec0_pf3_colscroll_w, &dec0_pf3_colscroll },
	{ 0x2c0400, 0x2c07ff, dec0_pf3_rowscroll_w, &dec0_pf3_rowscroll },

	{ 0x300000, 0x300007, dec0_pf1_control_0_w },
	{ 0x300010, 0x300017, dec0_pf1_control_1_w },
	{ 0x320000, 0x321fff, dec0_pf1_data_w },
	{ 0x340000, 0x34007f, dec0_pf1_colscroll_w, &dec0_pf1_colscroll },
	{ 0x340400, 0x3407ff, dec0_pf1_rowscroll_w, &dec0_pf1_rowscroll },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress slyspy_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x244000, 0x244003, MRA_NOP }, /* ?? watchdog ?? */
	{ 0x304000, 0x307fff, MRA_BANK1 }, /* Sly spy main ram */
	{ 0x308000, 0x3087ff, MRA_BANK2 }, /* Sprites */
	{ 0x314008, 0x31400f, slyspy_controls_read },
	{ 0x31c00c, 0x31c00f, MRA_NOP },	/* sound CPU read? */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress slyspy_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
  /* All 0x23xxxx are moved from 0x24xxxx via patches to avoid conflicts */
	{ 0x230000, 0x230007, dec0_pf2_control_0_w },
	{ 0x230010, 0x230017, dec0_pf2_control_1_w },
  { 0x232000, 0x23207f, dec0_pf2_colscroll_w, &dec0_pf2_colscroll },
	{ 0x232400, 0x2327ff, dec0_pf2_rowscroll_w, &dec0_pf2_rowscroll },
	{ 0x238000, 0x238007, dec0_pf1_control_0_w },
	{ 0x238010, 0x238017, dec0_pf1_control_1_w },
  { 0x23c000, 0x23c07f, dec0_pf1_colscroll_w, &dec0_pf1_colscroll },
	{ 0x23c400, 0x23c7ff, dec0_pf1_rowscroll_w, &dec0_pf1_rowscroll },
  { 0x238000, 0x239fff, dec0_pf1_data_w }, /* Used only by end sequence */

  /* PRIORITY WORD - still not found, perhaps mixed in with other playfields */

	{ 0x240000, 0x2407ff, dec0_pf2_data_w },
	{ 0x242000, 0x243fff, dec0_pf1_data_w },
	{ 0x244000, 0x244003, MWA_NOP }, /* ?? watchdog ?? */
	{ 0x246000, 0x2467ff, dec0_pf2_data_w },
	{ 0x248000, 0x2487ff, dec0_pf2_data_w },
	{ 0x24a000, 0x24a003, MWA_NOP }, /* ?? watchdog ?? */
	{ 0x24c000, 0x24c7ff, dec0_pf2_data_w },
	{ 0x24e000, 0x24ffff, dec0_pf1_data_w },

	{ 0x300000, 0x300007, dec0_pf3_control_0_w },
	{ 0x300010, 0x300017, dec0_pf3_control_1_w },
	{ 0x300800, 0x30087f, dec0_pf3_colscroll_w, &dec0_pf3_colscroll },
	{ 0x301000, 0x3017ff, dec0_pf3_data_w },
	{ 0x304000, 0x307fff, MWA_BANK1 }, /* Sly spy main ram */
	{ 0x308000, 0x3087ff, MWA_BANK2, &dec0_sprite },
	{ 0x310000, 0x3107ff, dec0_palette_12bit_w },
	{ 0x314000, 0x314003, MWA_NOP }, /* Sound */
	{ -1 }  /* end of table */
};

/******************************************************************************/

static struct MemoryReadAddress dec0_s_readmem[] =
{
	{ 0x0000, 0x05ff, MRA_RAM },
	{ 0x3000, 0x3000, soundlatch_r },
	{ 0x3800, 0x3800, OKIM6295_status_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress dec0_s_writemem[] =
{
	{ 0x0000, 0x05ff, MWA_RAM },
	{ 0x0800, 0x0800, YM2203_control_port_0_w },
	{ 0x0801, 0x0801, YM2203_write_port_0_w },
	{ 0x1000, 0x1000, YM3812_control_port_0_w },
	{ 0x1001, 0x1001, YM3812_write_port_0_w },
	{ 0x3800, 0x3800, OKIM6295_data_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

/******************************************************************************/

INPUT_PORTS_START( robocop_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 4 - unused */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 4 - unused */

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )	/* could be ACTIVE_LOW, not sure */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Player Energy", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Low" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "High" )
	PORT_DIPSETTING(    0x00, "Very High" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_DIPNAME( 0x20, 0x20, "Bonus Stage Energy", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x20, "High" )
	PORT_DIPNAME( 0x40, 0x40, "Brink Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Less" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( hippodrm_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 4 - unused */

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 4 - unused */

	PORT_START	/* Credits, start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )	/* could be ACTIVE_LOW, not sure */

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, "Opponent Energy", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Low" )
	PORT_DIPSETTING(    0x0c, "Medium" )
	PORT_DIPSETTING(    0x04, "High" )
	PORT_DIPSETTING(    0x00, "Very High" )
	PORT_DIPNAME( 0x30, 0x30, "Player Energy", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Low" )
	PORT_DIPSETTING(    0x20, "Medium" )
	PORT_DIPSETTING(    0x30, "High" )
	PORT_DIPSETTING(    0x00, "Very High" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( baddudes_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 4 - unused */

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 4 - unused */

	PORT_START	/* Credits, start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* PL1 Button 5 - unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* PL2 Button 5 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )	/* could be ACTIVE_LOW, not sure */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* player 1 12-way rotary control - converted in controls_r() */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused */

	PORT_START	/* player 2 12-way rotary control - converted in controls_r() */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused */
INPUT_PORTS_END

INPUT_PORTS_START( heavyb_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 4 - unused */

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Button 4 - unused */

	PORT_START	/* Credits, start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* PL1 Button 5 - unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* PL2 Button 5 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )	/* could be ACTIVE_LOW, not sure */

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30000 80000 160000" )
	PORT_DIPSETTING(    0x10, "50000 120000 190000" )
	PORT_DIPSETTING(    0x20, "100000 200000 300000" )
	PORT_DIPSETTING(    0x00, "150000 300000 450000" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* player 1 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 25, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 8 )

	PORT_START	/* player 2 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 25, 0, 0, 0, OSD_KEY_N, OSD_KEY_M, 0, 0, 8 )
INPUT_PORTS_END


INPUT_PORTS_START( midres_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )	/* ACTIVE_HIGH causes slowdowns */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* player 1 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 25, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 8 )

	PORT_START	/* player 2 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 25, 0, 0, 0, OSD_KEY_N, OSD_KEY_M, 0, 0, 8 )
INPUT_PORTS_END

INPUT_PORTS_START( slyspy_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits, start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )	/* could be ACTIVE_LOW, not sure */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Energy", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Low" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "High" )
	PORT_DIPSETTING(    0x00, "Very High" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	4096,
	4,		/* 4 bits per pixel  */
	{ 0x00000*8, 0x10000*8, 0x8000*8, 0x18000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,
	4096,
	4,
	{ 0x20000*8, 0x60000*8, 0x00000*8, 0x40000*8 },
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,   0, 16 },	/* Characters 8x8 */
	{ 1, 0x020000, &tilelayout, 512, 16 },	/* Tiles 16x16 */
	{ 1, 0x0a0000, &tilelayout, 768, 16 },	/* Tiles 16x16 */
	{ 1, 0x120000, &tilelayout, 256, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo midres_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 256, 16 },	/* Characters 8x8 */
	{ 1, 0x020000, &tilelayout, 512, 16 },	/* Tiles 16x16 */
	{ 1, 0x0a0000, &tilelayout, 768, 16 },	/* Tiles 16x16 */
	{ 1, 0x120000, &tilelayout,   0, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};


/******************************************************************************/

static struct YM2203interface ym2203_interface =
{
	1,
	1250000,	/* 1.25 MHz */
	{ YM2203_VOL(140,0x20ff) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip (no more supported) */
	3000000,	/* 3 MHz ? (not supported) */
	{ 255 }		/* (not supported) */
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz frequency */
	3,              /* memory region 3 */
	{ 255 }
};

/******************************************************************************/

static struct MachineDriver robocop_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			0,
			robocop_readmem,robocop_writemem,0,0,
			robocop_interrupt,1
		} ,
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,	/* Unconfirmed */
			2,
			dec0_s_readmem,dec0_s_writemem,0,0,
			interrupt,32
		}
	},
	57, 1536, /* frames per second, vblank duration taken from Burger Time */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	robocop_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver hippodrm_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			0,
			hippodrm_readmem,hippodrm_writemem,0,0,
			robocop_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,
			2,
			dec0_s_readmem,dec0_s_writemem,0,0,
			interrupt,32
		}
	},
	57, 1536, /* frames per second, vblank duration taken from Burger Time */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	hippodrm_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver baddudes_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			0,
			dec0_readmem,dec0_writemem,0,0,
			dude_interrupt,2
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,	/* Unknown speed */
			2,
			dec0_s_readmem,dec0_s_writemem,0,0,
			interrupt,32
		}
	},
	57, 1536, /* frames per second, vblank duration taken from Burger Time */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	dec0_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver slyspy_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			0,
			slyspy_readmem,slyspy_writemem,0,0,
			robocop_interrupt,1
		} /*,     =
		{
			CPU_Z80, // | CPU_AUDIO_CPU,
			1250000,
			2,
			dec0_s_readmem,dec0_s_writemem,0,0,
			interrupt,1
		}  */
	},
	57, 1536, /* frames per second, vblank duration taken from Burger Time */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	slyspy_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

static struct MachineDriver midres_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			0,
			midres_readmem,midres_writemem,0,0,
			dude_interrupt,2
		}  /*,
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,
			2,
			dec0_s_readmem,dec0_s_writemem,0,0,
			interrupt,1
		}   */
	},
	57, 1536, /* frames per second, vblank duration taken from Burger Time */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	midres_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	midres_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

static struct MachineDriver heavyb_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			0,
			heavyb_readmem,heavyb_writemem,0,0,
			dude_interrupt,2
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1000000,
			2,
			dec0_s_readmem,dec0_s_writemem,0,0,
			interrupt,16
		}
	},
	57, 1536, /* frames per second, vblank duration taken from Burger Time */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	heavyb_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/******************************************************************************/

ROM_START( baddudes_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code, middle 0x20000 unused */
	ROM_LOAD_EVEN( "baddudes.4", 0x00000, 0x10000, 0xa188e4a2 )
	ROM_LOAD_ODD ( "baddudes.1", 0x00000, 0x10000, 0x38832ed3 )
	ROM_LOAD_EVEN( "baddudes.6", 0x40000, 0x10000, 0x765062f6 )
	ROM_LOAD_ODD ( "baddudes.3", 0x40000, 0x10000, 0x65b58a61 )

	ROM_REGION(0x1a0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "baddudes.25", 0x000000, 0x04000, 0xd051f18f )	/* chars */
	/* 04000-07fff empty */
	ROM_CONTINUE(            0x008000, 0x04000 )
	/* 0c000-0ffff empty */
	ROM_LOAD( "baddudes.26", 0x010000, 0x04000, 0xf0479c51 )
	/* 14000-17fff empty */
	ROM_CONTINUE(            0x018000, 0x04000 )
	/* 1c000-1ffff empty */
	ROM_LOAD( "baddudes.18", 0x020000, 0x10000, 0x595c307a )	/* tiles */
	/* 30000-3ffff empty */
	ROM_LOAD( "baddudes.20", 0x040000, 0x10000, 0x6cfb821b )
	/* 50000-5ffff empty */
	ROM_LOAD( "baddudes.22", 0x060000, 0x10000, 0xeef0ab94 )
	/* 70000-7ffff empty */
	ROM_LOAD( "baddudes.24", 0x080000, 0x10000, 0x0ff71667 )
	/* 90000-9ffff empty */
	ROM_LOAD( "baddudes.30", 0x0c0000, 0x08000, 0x4ceaddbc )	/* tiles */
	/* c8000-dffff empty */
	ROM_CONTINUE(            0x0a0000, 0x08000 )	/* the two halves are swapped */
	/* a8000-bffff empty */
	ROM_LOAD( "baddudes.28", 0x100000, 0x08000, 0xab40b480 )
	/* 108000-11ffff empty */
	ROM_CONTINUE(            0x0e0000, 0x08000 )
	/* e8000-fffff empty */
	ROM_LOAD( "baddudes.15", 0x120000, 0x10000, 0xddd97537 )	/* sprites */
	ROM_LOAD( "baddudes.16", 0x130000, 0x08000, 0x5b301cea )
	/* 138000-13ffff empty */
	ROM_LOAD( "baddudes.11", 0x140000, 0x10000, 0xecd3ef13 )
	ROM_LOAD( "baddudes.12", 0x150000, 0x08000, 0x0f5cc24a )
	/* 158000-15ffff empty */
	ROM_LOAD( "baddudes.13", 0x160000, 0x10000, 0xf51c75f8 )
	ROM_LOAD( "baddudes.14", 0x170000, 0x08000, 0x794562c5 )
	/* 178000-17ffff empty */
	ROM_LOAD( "baddudes.9",  0x180000, 0x10000, 0x038c40a0 )
	ROM_LOAD( "baddudes.10", 0x190000, 0x08000, 0x6ba86872 )
	/* 198000-19ffff empty */

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "baddudes.7", 0x8000, 0x8000, 0xb737c5b3 )

	ROM_REGION(0x10000)	/* ADPCM samples */
	/* Taken from a Dragon Ninja board, the Bad Dudes one might be different! */
	ROM_LOAD( "baddudes.8", 0x0000, 0x10000, 0xa90d88b7 )
ROM_END

ROM_START( drgninja_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code, middle 0x20000 unused */
	ROM_LOAD_EVEN( "drgninja.04", 0x00000, 0x10000, 0x0cb7f35b )
	ROM_LOAD_ODD ( "drgninja.01", 0x00000, 0x10000, 0x53e293c8 )
	ROM_LOAD_EVEN( "drgninja.06", 0x40000, 0x10000, 0x0c45aabf )
	ROM_LOAD_ODD ( "drgninja.03", 0x40000, 0x10000, 0x9c8b7d75 )

	ROM_REGION(0x1a0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "drgninja.25", 0x000000, 0x04000, 0x0c62789a )	/* chars */
	/* 04000-07fff empty */
	ROM_CONTINUE(            0x008000, 0x04000 )
	/* 0c000-0ffff empty */
	ROM_LOAD( "drgninja.26", 0x010000, 0x04000, 0x52e003ec )
	/* 14000-17fff empty */
	ROM_CONTINUE(            0x018000, 0x04000 )
	/* 1c000-1ffff empty */
	ROM_LOAD( "drgninja.18", 0x020000, 0x10000, 0x595c307a )	/* tiles */
	/* 30000-3ffff empty */
	ROM_LOAD( "drgninja.20", 0x040000, 0x10000, 0x6cfb821b )
	/* 50000-5ffff empty */
	ROM_LOAD( "drgninja.22", 0x060000, 0x10000, 0xeef0ab94 )
	/* 70000-7ffff empty */
	ROM_LOAD( "drgninja.24", 0x080000, 0x10000, 0x0ff71667 )
	/* 90000-9ffff empty */
	ROM_LOAD( "drgninja.30", 0x0c0000, 0x08000, 0x198036ce )	/* tiles */
	/* c8000-dffff empty */
	ROM_CONTINUE(            0x0a0000, 0x08000 )	/* the two halves are swapped */
	/* a8000-bffff empty */
	ROM_LOAD( "drgninja.28", 0x100000, 0x08000, 0x33e12307 )
	/* 108000-11ffff empty */
	ROM_CONTINUE(            0x0e0000, 0x08000 )
	/* e8000-fffff empty */
	ROM_LOAD( "drgninja.15", 0x120000, 0x10000, 0x6f32c34c )	/* sprites */
	ROM_LOAD( "drgninja.16", 0x130000, 0x08000, 0x5b301cea )
	/* 138000-13ffff empty */
	ROM_LOAD( "drgninja.11", 0x140000, 0x10000, 0xf2adc1b9 )
	ROM_LOAD( "drgninja.12", 0x150000, 0x08000, 0x0f5cc24a )
	/* 158000-15ffff empty */
	ROM_LOAD( "drgninja.13", 0x160000, 0x10000, 0xc75036bc )
	ROM_LOAD( "drgninja.14", 0x170000, 0x08000, 0x794562c5 )
	/* 178000-17ffff empty */
	ROM_LOAD( "drgninja.09", 0x180000, 0x10000, 0x038c40a0 )
	ROM_LOAD( "drgninja.10", 0x190000, 0x08000, 0x6ba86872 )
	/* 198000-19ffff empty */

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "drgninja.07", 0x8000, 0x8000, 0x1b146c98 )

	ROM_REGION(0x10000)	/* ADPCM samples */
	ROM_LOAD( "drgninja.08", 0x0000, 0x10000, 0xa90d88b7 )
ROM_END

ROM_START( robocopp_rom )
	ROM_REGION(0x40000) /* 68000 code */
	ROM_LOAD_EVEN( "robop_05.rom", 0x00000, 0x10000, 0xd56d97e9 )
	ROM_LOAD_ODD ( "robop_01.rom", 0x00000, 0x10000, 0x6e59da83 )
	ROM_LOAD_EVEN( "robop_04.rom", 0x20000, 0x10000, 0x55a44a9c )
	ROM_LOAD_ODD ( "robop_00.rom", 0x20000, 0x10000, 0xaf9255ce )

	ROM_REGION(0x1a0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "robop_23.rom", 0x000000, 0x10000, 0xc350f776 )	/* chars */
	ROM_LOAD( "robop_22.rom", 0x010000, 0x10000, 0x434ccb9e )
	ROM_LOAD( "robop_20.rom", 0x020000, 0x10000, 0xfbe48598 )	/* tiles */
	ROM_LOAD( "robop_21.rom", 0x040000, 0x10000, 0xb754fb0e )
	ROM_LOAD( "robop_18.rom", 0x060000, 0x10000, 0x51d51535 )
	ROM_LOAD( "robop_19.rom", 0x080000, 0x10000, 0xa7e7e665 )
	ROM_LOAD( "robop_14.rom", 0x0a0000, 0x08000, 0x624fb18d )	/* tiles */
	ROM_LOAD( "robop_15.rom", 0x0c0000, 0x08000, 0x94ca2b74 )
	ROM_LOAD( "robop_16.rom", 0x0e0000, 0x08000, 0x2f773ecb )
	ROM_LOAD( "robop_17.rom", 0x100000, 0x08000, 0x690681a0 )
	ROM_LOAD( "robop_07.rom", 0x120000, 0x10000, 0x2d646366 )	/* sprites */
	ROM_LOAD( "robop_06.rom", 0x130000, 0x08000, 0xfb1985fd )
	/* 98000-9ffff empty */
	ROM_LOAD( "robop_11.rom", 0x140000, 0x10000, 0xda1d3fb1 )
	ROM_LOAD( "robop_10.rom", 0x150000, 0x08000, 0xe2808b6a )
	/* b8000-bffff empty */
	ROM_LOAD( "robop_09.rom", 0x160000, 0x10000, 0x9e9d36e9 )
	ROM_LOAD( "robop_08.rom", 0x170000, 0x08000, 0xb9a109d5 )
	/* d8000-dffff empty */
	ROM_LOAD( "robop_13.rom", 0x180000, 0x10000, 0xed07f363 )
	ROM_LOAD( "robop_12.rom", 0x190000, 0x08000, 0xe777b73d )
	/* f8000-fffff empty */

	ROM_REGION(0x10000)	/* 6502 Sound */
	ROM_LOAD( "robop_03.rom", 0x08000, 0x08000, 0x1adde131 )

	ROM_REGION(0x10000)	/* ADPCM samples */
	ROM_LOAD( "robop_02.rom", 0x00000, 0x10000, 0x513686c6 )
ROM_END

ROM_START( hippodrm_rom )
	/* there's an unused ROM, ew08. Its place is unknown. */

	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "ew02", 0x00000, 0x10000, 0x2d322dde )
	ROM_LOAD_ODD ( "ew01", 0x00000, 0x10000, 0xfe0f3859 )
	ROM_LOAD_EVEN( "ew05", 0x20000, 0x10000, 0x8e892251 )
	ROM_LOAD_ODD ( "ew00", 0x20000, 0x10000, 0x486c7b5a )

	ROM_REGION(0x1a0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ew14", 0x000000, 0x10000, 0x4b7bc501 )	/* chars */
	ROM_LOAD( "ew13", 0x010000, 0x10000, 0x45a2f302 )
	ROM_LOAD( "ew19", 0x020000, 0x08000, 0x68e172bb )	/* tiles */
	/* 28000-3ffff empty */
	ROM_LOAD( "ew18", 0x040000, 0x08000, 0xadc9d98f )
	/* 48000-5ffff empty */
	ROM_LOAD( "ew20", 0x060000, 0x08000, 0x383f60fb )
	/* 68000-7ffff empty */
	ROM_LOAD( "ew21", 0x080000, 0x08000, 0xb8e29600 )
	/* 88000-9ffff empty */
	ROM_LOAD( "ew23", 0x0a0000, 0x08000, 0x8ed64ca2 )	/* tiles */
	/* a8000-bffff empty */
	ROM_LOAD( "ew22", 0x0c0000, 0x08000, 0xf1794dcb )
	/* c8000-dffff empty */
	ROM_LOAD( "ew24", 0x0e0000, 0x08000, 0xdaf2c428 )
	/* e8000-fffff empty */
	ROM_LOAD( "ew25", 0x100000, 0x08000, 0x529d4bed )
	/* 108000-11ffff empty */
	ROM_LOAD( "ew15", 0x120000, 0x10000, 0x5c239191 )	/* sprites */
	ROM_LOAD( "ew16", 0x130000, 0x10000, 0x7a161cbc )
	ROM_LOAD( "ew10", 0x140000, 0x10000, 0x6dee825a )
	ROM_LOAD( "ew11", 0x150000, 0x10000, 0x20b53a27 )
	ROM_LOAD( "ew06", 0x160000, 0x10000, 0x70f2733c )
	ROM_LOAD( "ew07", 0x170000, 0x10000, 0x867c30d4 )
	ROM_LOAD( "ew17", 0x180000, 0x10000, 0xe573f8d7 )
	ROM_LOAD( "ew12", 0x190000, 0x10000, 0x4d39404d )

	ROM_REGION(0x10000)	/* 6502 sound */
	ROM_LOAD( "ew04", 0x8000, 0x8000, 0xb8a3b86b )

	ROM_REGION(0x10000)	/* ADPCM sounds */
	ROM_LOAD( "ew03", 0x0000, 0x10000, 0x3d50a8dc )
ROM_END

ROM_START( ffantasy_rom )
	/* there's an unused ROM, ev08. Its place is unknown. */

	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "ev02", 0x00000, 0x10000, 0x24fe7c9c )
	ROM_LOAD_ODD ( "ev01", 0x00000, 0x10000, 0xbbb03ff4 )
	ROM_LOAD_EVEN( "ev05", 0x20000, 0x10000, 0x8e892251 )
	ROM_LOAD_ODD ( "ev00", 0x20000, 0x10000, 0x486c7b5a )

	ROM_REGION(0x1a0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ev14", 0x000000, 0x10000, 0xdf57f8d7 )	/* chars */
	ROM_LOAD( "ev13", 0x010000, 0x10000, 0x7b13d21b )
	ROM_LOAD( "ev19", 0x020000, 0x08000, 0x68e172bb )	/* tiles */
	/* 28000-3ffff empty */
	ROM_LOAD( "ev18", 0x040000, 0x08000, 0xadc9d98f )
	/* 48000-5ffff empty */
	ROM_LOAD( "ev20", 0x060000, 0x08000, 0x383f60fb )
	/* 68000-7ffff empty */
	ROM_LOAD( "ev21", 0x080000, 0x08000, 0xb8e29600 )
	/* 88000-9ffff empty */
	ROM_LOAD( "ev23", 0x0a0000, 0x08000, 0x8ed64ca2 )	/* tiles */
	/* a8000-bffff empty */
	ROM_LOAD( "ev22", 0x0c0000, 0x08000, 0xf1794dcb )
	/* c8000-dffff empty */
	ROM_LOAD( "ev24", 0x0e0000, 0x08000, 0xdaf2c428 )
	/* e8000-fffff empty */
	ROM_LOAD( "ev25", 0x100000, 0x08000, 0x529d4bed )
	/* 108000-11ffff empty */
	ROM_LOAD( "ev15", 0x120000, 0x10000, 0x7a187868 )	/* sprites */
	ROM_LOAD( "ev16", 0x130000, 0x10000, 0x7a161cbc )
	ROM_LOAD( "ev10", 0x140000, 0x10000, 0x8be36ba3 )
	ROM_LOAD( "ev11", 0x150000, 0x10000, 0x20b53a27 )
	ROM_LOAD( "ev06", 0x160000, 0x10000, 0x8ee79ac5 )
	ROM_LOAD( "ev07", 0x170000, 0x10000, 0x867c30d4 )
	ROM_LOAD( "ev17", 0x180000, 0x10000, 0x0368112e )
	ROM_LOAD( "ev12", 0x190000, 0x10000, 0x4d39404d )

	ROM_REGION(0x10000)	/* 6502 sound */
	ROM_LOAD( "ev04", 0x8000, 0x8000, 0xb8a3b86b )

	ROM_REGION(0x10000)	/* ADPCM sounds */
	ROM_LOAD( "ev03", 0x0000, 0x10000, 0x3d50a8dc )
ROM_END

ROM_START( heavyb_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "HB04.BIN", 0x00000, 0x10000, 0x3a3a153a )
	ROM_LOAD_ODD ( "HB01.BIN", 0x00000, 0x10000, 0x94922044 )
	ROM_LOAD_EVEN( "HB05.BIN", 0x20000, 0x10000, 0x78b2245c )
	ROM_LOAD_ODD ( "HB02.BIN", 0x20000, 0x10000, 0x2d180d42 )
	ROM_LOAD_EVEN( "HB06.BIN", 0x40000, 0x10000, 0xa32aebea )
	ROM_LOAD_ODD ( "HB03.BIN", 0x40000, 0x10000, 0x68e7b4f7 )

	ROM_REGION(0x1a0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "HB25.BIN", 0x000000, 0x10000, 0x6ce711ab )	/* chars */
	ROM_LOAD( "HB26.BIN", 0x010000, 0x10000, 0x438de025 )
	ROM_LOAD( "HB18.BIN", 0x020000, 0x10000, 0xb7fd55af )	/* tiles */
	ROM_LOAD( "HB17.BIN", 0x030000, 0x10000, 0x3c3a5e24 )
	ROM_LOAD( "HB20.BIN", 0x040000, 0x10000, 0x7f1adece )
	ROM_LOAD( "HB19.BIN", 0x050000, 0x10000, 0xdb6d548f )
	ROM_LOAD( "HB22.BIN", 0x060000, 0x10000, 0xa86f2849 )
	ROM_LOAD( "HB21.BIN", 0x070000, 0x10000, 0x5c30dfa8 )
	ROM_LOAD( "HB24.BIN", 0x080000, 0x10000, 0x36dd8003 )
	ROM_LOAD( "HB23.BIN", 0x090000, 0x10000, 0xc3744af2 )
	ROM_LOAD( "HB29.BIN", 0x0a0000, 0x10000, 0xecc05140 )	/* tiles */
	/* b0000-bfff empty */
	ROM_LOAD( "HB30.BIN", 0x0c0000, 0x10000, 0x64394369 )
	/* d0000-dfff empty */
	ROM_LOAD( "HB27.BIN", 0x0e0000, 0x10000, 0x37f5c863 )
	/* f0000-ffff empty */
	ROM_LOAD( "HB28.BIN", 0x100000, 0x10000, 0xd5ccead8 )
	/* 110000-11fff empty */
	ROM_LOAD( "HB15.BIN", 0x120000, 0x10000, 0x30b6bb34 )	/* sprites */
	ROM_LOAD( "HB16.BIN", 0x130000, 0x10000, 0x19bcf8ec )
	ROM_LOAD( "HB11.BIN", 0x140000, 0x10000, 0xa2817053 )
	ROM_LOAD( "HB12.BIN", 0x150000, 0x10000, 0x7de7b9f7 )
	ROM_LOAD( "HB13.BIN", 0x160000, 0x10000, 0xcea5802b )
	ROM_LOAD( "HB14.BIN", 0x170000, 0x10000, 0x4d942794 )
	ROM_LOAD( "HB09.BIN", 0x180000, 0x10000, 0x4c370c4d )
	ROM_LOAD( "HB10.BIN", 0x190000, 0x10000, 0x46722c8a )

	ROM_REGION(0x10000)	/* 6502 Sound */
	ROM_LOAD( "HB07.BIN", 0x8000, 0x8000, 0x0063a9fb )

	ROM_REGION(0x10000)	/* ADPCM samples */
	ROM_LOAD( "HB08.BIN", 0x0000, 0x10000, 0x3743341b )
ROM_END

ROM_START( heavyb2_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "HB_EC04.ROM", 0x00000, 0x10000, 0x17e8d870 )
	ROM_LOAD_ODD ( "HB_EC01.ROM", 0x00000, 0x10000, 0x041e5e7c )
	ROM_LOAD_EVEN( "HB_EC05.ROM", 0x20000, 0x10000, 0x78b2245c )
	ROM_LOAD_ODD ( "HB_EC02.ROM", 0x20000, 0x10000, 0x2d180d42 )
	ROM_LOAD_EVEN( "HB_EC06.ROM", 0x40000, 0x10000, 0xcc5be9df )
	ROM_LOAD_ODD ( "HB_EC03.ROM", 0x40000, 0x10000, 0x0582257c )

	ROM_REGION(0x1a0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "HB_EC25.ROM", 0x000000, 0x10000, 0x31a223b0 )	/* chars */
	ROM_LOAD( "HB_EC26.ROM", 0x010000, 0x10000, 0x4409e027 )
	ROM_LOAD( "HB_EC18.ROM", 0x020000, 0x10000, 0xb7fd55af )	/* tiles */
	ROM_LOAD( "HB_EC17.ROM", 0x030000, 0x10000, 0x3c3a5e24 )
	ROM_LOAD( "HB_EC20.ROM", 0x040000, 0x10000, 0x7f1adece )
	ROM_LOAD( "HB_EC19.ROM", 0x050000, 0x10000, 0xdb6d548f )
	ROM_LOAD( "HB_EC22.ROM", 0x060000, 0x10000, 0xa86f2849 )
	ROM_LOAD( "HB_EC21.ROM", 0x070000, 0x10000, 0x5c30dfa8 )
	ROM_LOAD( "HB_EC24.ROM", 0x080000, 0x10000, 0x36dd8003 )
	ROM_LOAD( "HB_EC23.ROM", 0x090000, 0x10000, 0xc3744af2 )
	ROM_LOAD( "HB29.BIN", 0x0a0000, 0x10000, 0xecc05140 )	/* tiles */
	/* b0000-bfff empty */
	ROM_LOAD( "HB30.BIN", 0x0c0000, 0x10000, 0x64394369 )
	/* d0000-dfff empty */
	ROM_LOAD( "HB27.BIN", 0x0e0000, 0x10000, 0x37f5c863 )
	/* f0000-ffff empty */
	ROM_LOAD( "HB28.BIN", 0x100000, 0x10000, 0xd5ccead8 )
	/* 110000-11fff empty */
	ROM_LOAD( "HB_EC15.ROM", 0x120000, 0x10000, 0x30b6bb34 )	/* sprites */
	ROM_LOAD( "HB_EC16.ROM", 0x130000, 0x10000, 0x19bcf8ec )
	ROM_LOAD( "HB_EC11.ROM", 0x140000, 0x10000, 0xa2817053 )
	ROM_LOAD( "HB_EC12.ROM", 0x150000, 0x10000, 0x7de7b9f7 )
	ROM_LOAD( "HB_EC13.ROM", 0x160000, 0x10000, 0xcea5802b )
	ROM_LOAD( "HB_EC14.ROM", 0x170000, 0x10000, 0x4d942794 )
	ROM_LOAD( "HB_EC09.ROM", 0x180000, 0x10000, 0x4c370c4d )
	ROM_LOAD( "HB_EC10.ROM", 0x190000, 0x10000, 0x46722c8a )

	ROM_REGION(0x10000)	/* 6502 Sound */
	ROM_LOAD( "HB_EC07.ROM", 0x8000, 0x8000, 0x7eefbddf )

	ROM_REGION(0x10000)	/* ADPCM samples */
	ROM_LOAD( "HB_EC08.ROM", 0x0000, 0x10000, 0xffb3d08b )
ROM_END

ROM_START( slyspy_rom )
	ROM_REGION(0x40000) /* 68000 code */
	ROM_LOAD_EVEN("fa14-2.bin", 0x00000, 0x10000, 0xdc199611 )
	ROM_LOAD_ODD ("fa12-2.bin", 0x00000, 0x10000, 0x9c925d90 )
	ROM_LOAD_EVEN("fa15-.bin",  0x20000, 0x10000, 0xf2ad9ce9 )
	ROM_LOAD_ODD ("fa13-.bin",  0x20000, 0x10000, 0xe89a147a )

	ROM_REGION(0x1a0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "fa05-.bin", 0x008000, 0x04000, 0x775763bf )	/* chars */
	/* 0c000-0ffff empty */
	ROM_CONTINUE(          0x000000, 0x04000 )	/* the two halves are swapped */
	/* 04000-07fff empty */
	ROM_LOAD( "fa04-.bin", 0x018000, 0x04000, 0x1b99c003 )
	/* 1c000-1ffff empty */
	ROM_CONTINUE(          0x010000, 0x04000 )
	/* 14000-17fff empty */
	ROM_LOAD( "fa07-.bin", 0x020000, 0x08000, 0xa54b6467 )	/* tiles */
	/* 28000-3ffff empty */
	ROM_CONTINUE(          0x040000, 0x08000 )
	/* 48000-5ffff empty */
	ROM_LOAD( "fa06-.bin", 0x060000, 0x08000, 0x3ad165b3 )
	/* 68000-7ffff empty */
	ROM_CONTINUE(          0x080000, 0x08000 )
	/* 88000-9ffff empty */
	ROM_LOAD( "fa09-.bin", 0x0a0000, 0x10000, 0x49dc0cd2 )	/* tiles */
	/* b0000-bffff empty */
	ROM_CONTINUE(          0x0c0000, 0x10000 )
	/* d0000-dffff empty */
	ROM_LOAD( "fa08-.bin", 0x0e0000, 0x10000, 0x86def798 )
	/* f0000-fffff empty */
	ROM_CONTINUE(          0x100000, 0x10000 )
	/* 110000-11ffff empty */
	ROM_LOAD( "fa01-.bin", 0x120000, 0x20000, 0x352b25d5 )	/* sprites */
	ROM_LOAD( "fa03-.bin", 0x140000, 0x20000, 0xf51c58bc )
	ROM_LOAD( "fa00-.bin", 0x160000, 0x20000, 0x26b47612 )
	ROM_LOAD( "fa02-.bin", 0x180000, 0x20000, 0x31ad4955 )

	ROM_REGION (0x10000)	/* Unknown sound CPU */
	ROM_LOAD( "fa10-.bin", 0x00000, 0x10000, 0xc9e40514 )

	ROM_REGION (0x10000)	/* ADPCM samples */
	ROM_LOAD( "fa11-.bin", 0x00000, 0x10000, 0x8e6cc36c )
ROM_END

ROM_START( midres_rom )
	ROM_REGION(0x80000) /* 68000 code */
	ROM_LOAD_EVEN ("fl14", 0x00000, 0x20000, 0x35b0ed92 )
	ROM_LOAD_ODD ( "fl12", 0x00000, 0x20000, 0x189ab686 )
	ROM_LOAD_EVEN ("fl15", 0x40000, 0x20000, 0xfd56c2aa )
	ROM_LOAD_ODD  ("fl13", 0x40000, 0x20000, 0xa23a451a )

	ROM_REGION(0x1a0000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "fl05", 0x008000, 0x08000, 0x68a502ff )	/* chars */
	ROM_CONTINUE(     0x000000, 0x08000 )	/* the two halves are swapped */
	ROM_LOAD( "fl04", 0x018000, 0x08000, 0x64be5ec2 )
	ROM_CONTINUE(     0x010000, 0x08000 )
	ROM_LOAD( "fl09", 0x020000, 0x20000, 0x2460122c )	/* tiles */
	ROM_LOAD( "fl08", 0x040000, 0x20000, 0x2f70347e )
	ROM_LOAD( "fl07", 0x060000, 0x20000, 0x3a8e3efe )
	ROM_LOAD( "fl06", 0x080000, 0x20000, 0x1c50e2f4 )
	ROM_LOAD( "fl11", 0x0a0000, 0x10000, 0xb939aea9 )	/* tiles */
	/* 0d0000-0dffff empty */
	ROM_CONTINUE(     0x0c0000, 0x10000 )
	/* 110000-11ffff empty */
	ROM_LOAD( "fl10", 0x0e0000, 0x10000, 0x280e7b06 )
	/* 0b0000-0bffff empty */
	ROM_CONTINUE(     0x100000, 0x10000 )
	/* 0f0000-0fffff empty */
	ROM_LOAD( "fl01", 0x120000, 0x20000, 0xccf643f2 )	/* sprites */
	ROM_LOAD( "fl03", 0x140000, 0x20000, 0x5c7da3a9 )
	ROM_LOAD( "fl00", 0x160000, 0x20000, 0x14f493e8 )
	ROM_LOAD( "fl02", 0x180000, 0x20000, 0xd223c495 )

	ROM_REGION(0x10000)	/* Unknown or corrupt sound CPU */
	ROM_LOAD( "fl16", 0x00000, 0x10000, 0xf43804e2 )

	ROM_REGION(0x20000)	/* ADPCM samples */
	ROM_LOAD( "fl17", 0x00000, 0x20000, 0x16d7aeff )
ROM_END

/******************************************************************************/

static void heavyb_patch(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	WRITE_WORD (&RAM[0x8B2],0x4E71);
}

static void heavyb2_patch(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	WRITE_WORD (&RAM[0x8A6],0x4E71);
}

static void drgninja_patch(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* Lamppost patch & test mode patch */
	WRITE_WORD (&RAM[0xDF9E],0x4E71);

	/* Uncomment this to make end sequence appear at end of first level *
	WRITE_WORD (&RAM[0x1b84],0x4E71);
	WRITE_WORD (&RAM[0x1b86],0x4E71);	 */
}

static void baddudes_patch(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* Gouky's amazing patch to fix lamposts and corrupt tiles!!! */
	WRITE_WORD (&RAM[0xe32e],0x4E71);

	/* Uncomment this to make end sequence appear at end of first level *
	WRITE_WORD (&RAM[0x1ba2],0x4E71);
	WRITE_WORD (&RAM[0x1ba4],0x4E71);  */
}

/* Sly Spy has many areas all mixed in with one another, they need to be
	seperated via patches unless we find some byte written somewhere that
  is used to enable each area on it's own */
static void slyspy_patch(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* Move pf2 registers so they don't conflict */
	WRITE_WORD (&RAM[0x132E],0x0023);
	WRITE_WORD (&RAM[0x133A],0x0023);
	WRITE_WORD (&RAM[0x1346],0x0023);
	WRITE_WORD (&RAM[0x1352],0x0023);
	WRITE_WORD (&RAM[0x135E],0x0023);
	WRITE_WORD (&RAM[0x136A],0x0023);
	WRITE_WORD (&RAM[0x1376],0x0023);
	WRITE_WORD (&RAM[0x1382],0x0023);

	WRITE_WORD (&RAM[0xD292],0x0023);

	/* Move pf1 registers so they don't conflict */
	WRITE_WORD (&RAM[0x12BC],0x0023);
	WRITE_WORD (&RAM[0x12C8],0x0023);
	WRITE_WORD (&RAM[0x12D4],0x0023);
	WRITE_WORD (&RAM[0x12E0],0x0023);
	WRITE_WORD (&RAM[0x12EC],0x0023);
	WRITE_WORD (&RAM[0x12F8],0x0023);
	WRITE_WORD (&RAM[0x1304],0x0023);
	WRITE_WORD (&RAM[0x1310],0x0023);

	/* Move row scroll */
	WRITE_WORD (&RAM[0xE3D8],0x0023);
	WRITE_WORD (&RAM[0xE3DE],0x0023);
	WRITE_WORD (&RAM[0xE3E4],0x0023);
	WRITE_WORD (&RAM[0xE3EA],0x0023);
	WRITE_WORD (&RAM[0xE3F0],0x0023);

	/* Playfield 2(?) rowscroll area & 0x80 byte colscroll area */
	WRITE_WORD (&RAM[0x1162],0x0023);
	WRITE_WORD (&RAM[0x1176],0x0023);

	/* Playfield 3(?) rowscroll area & 0x80 byte colscroll area */
	WRITE_WORD (&RAM[0x10EE],0x0023);
	WRITE_WORD (&RAM[0x1102],0x0023);

	/* Text areas used by end sequence, conflicts with pf2 */
	WRITE_WORD (&RAM[0x4542],0x0023);
	WRITE_WORD (&RAM[0x48D2],0x0023);
	WRITE_WORD (&RAM[0x4900],0x0023);
	WRITE_WORD (&RAM[0x4946],0x0023);
	WRITE_WORD (&RAM[0x49D0],0x0023);
	WRITE_WORD (&RAM[0x49FC],0x0023);
	WRITE_WORD (&RAM[0x4A40],0x0023);
	WRITE_WORD (&RAM[0x4A70],0x0023);

	/* Uncomment this to make end sequence appear at end of first level *
	WRITE_WORD (&RAM[0x5e0],0x4E71);
	WRITE_WORD (&RAM[0x5e2],0x4E71);	 */
}

/******************************************************************************/

struct GameDriver baddudes_driver =
{
	__FILE__,
	0,
	"baddudes",
	"Bad Dudes",
	"????",
	"?????",
	"Bryan McPhail (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&baddudes_machine_driver,

	baddudes_rom,
	baddudes_patch, 0,
	0,
	0,	/* sound_prom */

	baddudes_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver drgninja_driver =
{
	__FILE__,
	0,
	"drgninja",
	"Dragonninja",
	"????",
	"?????",
	"Bryan McPhail (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&baddudes_machine_driver,

	drgninja_rom,
	drgninja_patch, 0,
	0,
	0,	/* sound_prom */

	baddudes_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver robocopp_driver =
{
	__FILE__,
	0,
	"robocopp",
	"Robocop (bootleg)",
	"????",
	"?????",
	"Bryan McPhail (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&robocop_machine_driver,

	robocopp_rom,
	0, 0,
	0,
	0,

	robocop_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver heavyb_driver =
{
	__FILE__,
	0,
	"hbarrel",
	"Heavy Barrel",
	"????",
	"?????",
	"Bryan McPhail (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&heavyb_machine_driver,

	heavyb_rom,
	heavyb_patch, 0,
	0,
	0,	/* sound_prom */

	heavyb_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver heavyb2_driver =
{
	__FILE__,
	0,
	"hbarrel2",
	"Heavy Barrel (alternate)",
	"????",
	"?????",
	"Bryan McPhail (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&heavyb_machine_driver,

	heavyb2_rom,
	heavyb2_patch, 0,
	0,
	0,	/* sound_prom */

	heavyb_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver slyspy_driver =
{
	__FILE__,
	0,
	"slyspy",
	"Sly Spy",
	"????",
	"?????",
	"Bryan McPhail (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&slyspy_machine_driver,

	slyspy_rom,
	slyspy_patch, 0,
	0,
	0,	/* sound_prom */

	slyspy_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver hippodrm_driver =
{
	__FILE__,
	0,
	"hippodrm",
	"Hippodrome",
	"????",
	"?????",
	"Bryan McPhail (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&hippodrm_machine_driver,

	hippodrm_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	hippodrm_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver ffantasy_driver =
{
	__FILE__,
	0,
	"ffantasy",
	"Fighting Fantasy",
	"????",
	"?????",
	"Bryan McPhail (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&hippodrm_machine_driver,

	ffantasy_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	hippodrm_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver midres_driver =
{
	__FILE__,
	0,
	"midres",
	"Midnight Resistance",
	"????",
	"?????",
	"Bryan McPhail (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&midres_machine_driver,

	midres_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	midres_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

