/***************************************************************************

Amidar memory map (preliminary)

0000-3fff ROM (Turtles: 0000-4fff)
8000-87ff RAM
9000-93ff Video RAM
9800-983f Screen attributes
9840-985f sprites


read:
a800      watchdog reset
b000      IN0
b010      IN1
b020      IN2
b820      DSW2 (not Turtles)

*
 * IN0 (all bits are inverted)
 * bit 7 : COIN 1
 * bit 6 : COIN 2
 * bit 5 : LEFT player 1
 * bit 4 : RIGHT player 1
 * bit 3 : FIRE player 1
 * bit 2 : CREDIT
 * bit 1 : ?
 * bit 0 : UP player 2 (TABLE only)
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : START 1
 * bit 6 : START 2
 * bit 5 : LEFT player 2 (TABLE only)
 * bit 4 : RIGHT player 2 (TABLE only)
 * bit 3 : FIRE player 2 (TABLE only)
 * bit 2 : ?
 * bit 1 : ?
 * bit 0 : ?
*
 * IN2 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : DOWN player 1
 * bit 5 : ?
 * bit 4 : UP player 1
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 : bonus  0 = 30000 1 = 50000 (amidar)
 * bit 1 : ? (amidar)
 * bit 2 :\ coins per play
 * bit 1 :/ (turtles)
 * bit 0 : DOWN player 2 (TABLE only)
 *
*
 * DSW2 (all bits are inverted)
 * bit 7 :\
 * bit 6 :|  right coin slot
 * bit 5 :|
 * bit 4 :/  all 0 = free play
 * bit 3 :\
 * bit 2 :|  left coin slot
 * bit 1 :|
 * bit 0 :/
 *

write:
a008      interrupt enable

***************************************************************************/

#include "driver.h"
#include "machine.h"
#include "common.h"


extern unsigned char *amidar_videoram;
extern unsigned char *amidar_attributesram;
extern unsigned char *amidar_spriteram;
extern void amidar_videoram_w(int offset,int data);
extern void amidar_attributes_w(int offset,int data);
extern int amidar_vh_start(void);
extern void amidar_vh_stop(void);
extern void amidar_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress amidar_readmem[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9800, 0x985f, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xb000, 0xb000, input_port_0_r },	/* IN0 */
	{ 0xb010, 0xb010, input_port_1_r },	/* IN1 */
	{ 0xb020, 0xb020, input_port_2_r },	/* IN2 */
	{ 0xb820, 0xb820, input_port_3_r },	/* DSW2 */
	{ 0xa800, 0xa800, MRA_NOP },
	{ -1 }	/* end of table */
};
static struct MemoryReadAddress turtles_readmem[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9800, 0x985f, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xb000, 0xb000, input_port_0_r },	/* IN0 */
	{ 0xb010, 0xb010, input_port_1_r },	/* IN1 */
	{ 0xb020, 0xb020, input_port_2_r },	/* IN2 */
	{ 0xa800, 0xa800, MRA_NOP },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress amidar_writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x93ff, amidar_videoram_w, &amidar_videoram },
	{ 0x9800, 0x983f, amidar_attributes_w, &amidar_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &amidar_spriteram },
	{ 0x9860, 0x987f, MWA_NOP },
	{ 0xa008, 0xa008, interrupt_enable_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};
static struct MemoryWriteAddress turtles_writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x93ff, amidar_videoram_w, &amidar_videoram },
	{ 0x9800, 0x983f, amidar_attributes_w, &amidar_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &amidar_spriteram },
	{ 0x9860, 0x987f, MWA_NOP },
	{ 0xa008, 0xa008, interrupt_enable_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ 0, 0, OSD_KEY_3, OSD_KEY_CONTROL, OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_4, OSD_KEY_5 },
		{ 0, 0, 0, OSD_JOY_FIRE, OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_2, OSD_KEY_1 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xf1,
		{ 0, 0, 0, 0, OSD_KEY_UP, 0, OSD_KEY_DOWN, 0 },
		{ 0, 0, 0, 0, OSD_JOY_UP, 0, OSD_JOY_DOWN, 0 }
	},
	{	/* DSW2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW amidar_dsw[] =
{
	{ 2, 0x04, "BONUS", { "30000", "50000" } },
	{ 2, 0x02, "SW2", { "OFF", "ON" } },
	{ 2, 0x20, "SW6", { "OFF", "ON" } },
	{ 2, 0x80, "SW8", { "OFF", "ON" } },
	{ -1 }
};



static struct DSW turtles_dsw[] =
{
	{ 2, 0x20, "SW6", { "OFF", "ON" } },
	{ 2, 0x80, "SW8", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the color table */
static struct GfxLayout starslayout =
{
	0,0,
	0,
	1,	/* 1 star = 1 color */
	{ 0 },
	{ 0 },
	{ 0 },
	0
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0x10000, &charlayout,     0, 8 },
	{ 0x10000, &spritelayout,   0, 8 },
	{ 0,       &starslayout,   32, 64 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
    0, 0, 0,                    // BLACK
    63*4, 0, 0,                   // RED
    238, 162, 173,              // PURPLE
    0, 63*4, 63*4,                  // CYAN,
    63*4, 63*4, 0,                  // YELLOW,
    9, 9, 63*4,                   // BLUE
    0, 63*4, 0,                   // GREEN
    54*4, 54*4, 54*4                  // WHITE
};

enum
{
    black, red, purple, cyan, yellow, blue, green, white
};

static unsigned char colortable[] =
{
    black, red, blue, white,    // white text
    black, green, yellow, purple,   // yellow gorilla
    black, green, red, yellow,  // yellow line on title screen
    black, red, purple, green,
    black, yellow, green, red,
    black, red, yellow, green,
    black, white, red, purple,  // purple pig
    black, cyan, red, white    // white pig
};



const struct MachineDriver amidar_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	amidar_readmem,amidar_writemem,0,0,
	input_ports,amidar_dsw,
	0,
	nmi_interrupt,

	/* video hardware */
	256,256,
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,0,palette,colortable,
	0,17,
	0x07,0x01,
	8*13,8*16,0x06,
	0,
	amidar_vh_start,
	amidar_vh_stop,
	amidar_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};



const struct MachineDriver turtles_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	turtles_readmem,turtles_writemem,0,0,
	input_ports,turtles_dsw,
	0,
	nmi_interrupt,

	/* video hardware */
	256,256,
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,0,palette,colortable,
	0,17,
	0x07,0x01,
	8*13,8*16,0x06,
	0,
	amidar_vh_start,
	amidar_vh_stop,
	amidar_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};
