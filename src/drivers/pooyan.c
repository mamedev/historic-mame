/***************************************************************************

Pooyan memory map (preliminary)

Thanks must go to Mike Cuddy for providing information on this one.

Sound processor memory map.
0x3000-0x33ff RAM.
AY-8910 #1 : reg 0x5000
	     wr  0x4000
             rd  0x4000

AY-8910 #2 : reg 0x7000
	     wr  0x6000
             rd  0x6000

CPU command register : 0x8000

Main processor memory map.
0000-7fff ROM
8000-83ff color RAM
8400-87ff video RAM
9000-97ff sprite RAM (only areas 0x9010 and 0x9410 are used).

memory mapped ports:

read:
0xA000	Dipswitch 2 tdddball
        t = table / upright
        ddd = difficulty 0=easy, 7=hardest.
        b = bonus setting (easy/hard)
        a = attract mode
        ll = lives: 11=3, 10=4, 01=5, 00=255.

0xA0E0  llllrrrr
        l == left coin mech, r = right coinmech.

0xA080	IN0 Port
0xA0A0  IN1 Port

0xA0C0	?????

write:
0xA100	read as 0x8000 on audio CPU.
0xA180	NMI enable. (0xA180 == 1 = deliver NMI to CPU).

0xA181	interrupt trigger on audio CPU.

0xA183	????

0xA184	????

0xA187	Watchdog reset.

interrupts:
standard NMI at 0x66

***************************************************************************/

#include "driver.h"


extern unsigned char *pooyan_videoram;
extern unsigned char *pooyan_colorram;
extern unsigned char *pooyan_spriteram1;
extern unsigned char *pooyan_spriteram2;
extern void pooyan_videoram_w(int offset,int data);
extern void pooyan_colorram_w(int offset,int data);
extern int pooyan_vh_start(void);
extern void pooyan_vh_stop(void);
extern void pooyan_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },	/* color and video RAM */
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xa080, 0xa080, input_port_0_r },	/* IN0 */
	{ 0xa0a0, 0xa0a0, input_port_1_r },	/* IN1 */
	{ 0xa000, 0xa000, input_port_2_r },	/* DSW1 */
	{ 0xa0e0, 0xa0e0, input_port_3_r },	/* DSW2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x83ff, pooyan_colorram_w, &pooyan_colorram },
	{ 0x8400, 0x87ff, pooyan_videoram_w, &pooyan_videoram },
	{ 0x9010, 0x903f, MWA_RAM, &pooyan_spriteram1 },
	{ 0x9410, 0x943f, MWA_RAM, &pooyan_spriteram2 },
	{ 0xa180, 0xa180, interrupt_enable_w },
	{ 0xa187, 0xa187, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ 0, 0, OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_CONTROL, 0, 0, 0 },
		{ 0, 0, OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x73,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 2, 0x03, "LIVES", { "255", "5", "4", "3" },1 },
	{ 2, 0x08, "BONUS", { "30000", "50000" } },
	{ 2, 0x70, "DIFFICULTY", { "HARDEST", "HARD", "DIFFICULT", "MEDIUM", "NORMAL", "EASIER", "EASY" , "EASIEST" }, 1 },
	{ 2, 0x04, "ATTRACT MODE", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0, 4, 0x1000*8, 0x1000*8 + 4 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 8*8+0,8*8+1,8*8+2,8*8+3 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 4, 0x1000*8, 0x1000*8 + 4 },
	{ 39 * 8, 38 * 8, 37 * 8, 36 * 8, 35 * 8, 34 * 8, 33 * 8, 32 * 8,
	 	7 * 8, 6 * 8, 5 * 8, 4 * 8, 3 * 8, 2 * 8, 1 * 8, 0 * 8 },
	{ 0, 1, 2, 3,  8*8, 8*8+1, 8*8+2, 8*8+3,
          16*8+0, 16*8+1, 16*8+2, 16*8+3,  24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 32 },
	{ 1, 0x2000, &spritelayout, 0, 32 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xdb,0x00,0x00,	/* RED */
	0xdb,0x92,0x49,	/* BROWN */
	0xff,0xb6,0xdb,	/* PINK */
	0x00,0xdb,0x00,	/* UNUSED */
	0x00,0xdb,0xdb,	/* CYAN */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,0xb6,0x49,	/* DKORANGE */
	0x88,0x88,0x88,	/* UNUSED */
	0xdb,0xdb,0x00,	/* YELLOW */
	0xff,0x00,0xdb,	/* UNUSED */
	0x24,0x24,0xdb,	/* BLUE */
	0x00,0xdb,0x00,	/* GREEN */
	0x49,0xb6,0x92,	/* DKGREEN */
	0xff,0xb6,0x92,	/* LTORANGE */
	0xdb,0xdb,0xdb	/* GREY */
};

enum {BLACK,RED,BROWN,PINK,UNUSED1,CYAN,DKCYAN,DKORANGE,
		UNUSED2,YELLOW,UNUSED3,BLUE,GREEN,DKGREEN,LTORANGE,GREY};

static unsigned char colortable[] =
{
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,
	0,3,4,5,6,7,8,9,10,11,12,13,14,15,1,2,
	0,4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,
	0,5,6,7,8,9,10,11,12,13,14,15,1,2,3,4,
	0,6,7,8,9,10,11,12,13,14,15,1,2,3,4,5,
	0,7,8,9,10,11,12,13,14,15,1,2,3,4,5,6,
	0,8,9,10,11,12,13,14,15,1,2,3,4,5,6,7,
	0,9,10,11,12,13,14,15,1,2,3,4,5,6,7,8,
	0,10,11,12,13,14,15,1,2,3,4,5,6,7,8,9,
	0,11,12,13,14,15,1,2,3,4,5,6,7,8,9,10,
	0,12,13,14,15,1,2,3,4,5,6,7,8,9,10,11,
	0,13,14,15,1,2,3,4,5,6,7,8,9,10,11,12,
	0,14,15,1,2,3,4,5,6,7,8,9,10,11,12,13,
	0,15,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
	0,15,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,
	0,3,4,5,6,7,8,9,10,11,12,13,14,15,1,2,
	0,4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,
	0,5,6,7,8,9,10,11,12,13,14,15,1,2,3,4,
	0,6,7,8,9,10,11,12,13,14,15,1,2,3,4,5,
	0,7,8,9,10,11,12,13,14,15,1,2,3,4,5,6,
	0,8,9,10,11,12,13,14,15,1,2,3,4,5,6,7,
	0,9,10,11,12,13,14,15,1,2,3,4,5,6,7,8,
	0,10,11,12,13,14,15,1,2,3,4,5,6,7,8,9,
	0,11,12,13,14,15,1,2,3,4,5,6,7,8,9,10,
	0,12,13,14,15,1,2,3,4,5,6,7,8,9,10,11,
	0,13,14,15,1,2,3,4,5,6,7,8,9,10,11,12,
	0,14,15,1,2,3,4,5,6,7,8,9,10,11,12,13,
	0,15,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
	0,15,1,2,3,4,5,6,7,8,9,10,11,12,13,14
};



const struct MachineDriver pooyan_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
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
	0,17,
	0x01,0x18,
	8*11,8*20,0x16,
	0,
	pooyan_vh_start,
	pooyan_vh_stop,
	pooyan_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};
