/***************************************************************************

Space Invaders memory map (preliminary)

0000-1fff ROM
2000-23ff RAM
2400-3fff Video RAM
4000-57ff ROM (some clones use more, others less)


I/O ports:
read:
01        IN0
02        IN1
03        bit shift register read

write:
02        shift amount
03        ?
04        shift data
05        ?
06        ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern int invaders_shift_data_r(int offset);
extern void invaders_shift_amount_w(int offset,int data);
extern void invaders_shift_data_w(int offset,int data);
extern int invaders_interrupt(void);

extern unsigned char *invaders_videoram;
extern void invaders_videoram_w(int offset,int data);
extern void invaders_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x57ff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, invaders_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort readport[] =
{
	{ 0x01, 0x01, input_port_0_r },
	{ 0x02, 0x02, input_port_1_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x04, 0x04, invaders_shift_data_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x81,
		{ OSD_KEY_3, OSD_KEY_2, OSD_KEY_1, 0,
				OSD_KEY_CONTROL, OSD_KEY_LEFT, OSD_KEY_RIGHT, 0 },
		{ 0, 0, 0, 0,
				OSD_JOY_FIRE, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0,
				OSD_KEY_CONTROL, OSD_KEY_LEFT, OSD_KEY_RIGHT, 0 },
		{ 0, 0, 0, 0,
				OSD_JOY_FIRE, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0 }
	},
	{ -1 }
};



static struct DSW dsw[] =
{
	{ 1, 0x03, "LIVES", { "3", "4", "5", "6" } },
	{ 1, 0x08, "BONUS", { "1500", "1000" }, 1 },
	{ -1 }
};



/* Space Invaders doesn't have character mapped graphics, this definition is here */
/* only for the dip switch menu */
static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	42,	/* 42 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 10 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x1e00, &charlayout, 0, 4 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xff,0x00,0x00,	/* RED */
	0x00,0xff,0x00,	/* GREEN */
	0xff,0xff,0x00,	/* YELLOW */
	0xff,0xff,0xff	/* WHITE */
};

enum { BLACK,RED,GREEN,YELLOW,WHITE };

static unsigned char colortable[] =
{
	BLACK,WHITE,
	BLACK,GREEN,
	BLACK,RED,
	BLACK,YELLOW
};



const struct MachineDriver invaders_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,	/* 2 Mhz? */
			0,
			readmem,writemem,readport,writeport,
			invaders_interrupt,2	/* two interrupts per frame */
		}
	},
	60,
	input_ports,dsw,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,0,palette,colortable,
	26,0,
	0,3,
	8*13,8*16,2,
	0,
	generic_vh_start,
	generic_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};
