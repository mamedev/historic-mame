/***************************************************************************

Xevious memory map (preliminary)

CPU #1:

0000-3fff ROM
c400-c7ff Video RAM
write:
6830      watchdog reset

CPU #2:

0000-0fff ROM


CPU #3:

0000-1fff ROM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"


extern unsigned char *xevious_sharedram;
extern int xevious_sharedram_r(int offset);
extern void xevious_sharedram_w(int offset,int data);
extern int xevious_dsw_r(int offset);
extern void xevious_interrupt_enable_1_w(int offset,int data);
extern void xevious_interrupt_enable_2_w(int offset,int data);
extern void xevious_interrupt_enable_3_w(int offset,int data);
extern void xevious_halt_w(int offset,int data);
extern int xevious_customio_r(int offset);
extern void xevious_customio_w(int offset,int data);
extern int xevious_interrupt_1(void);
extern int xevious_interrupt_2(void);
extern int xevious_interrupt_3(void);

extern void pengo_sound_w(int offset,int data);
extern int rallyx_sh_start(void);
extern void pengo_sh_update(void);
extern unsigned char *pengo_soundregs;

extern void xevious_vh_screenrefresh(struct osd_bitmap *bitmap);


static struct MemoryReadAddress readmem_cpu1[] =
{
	{ 0x7800, 0xcfff, xevious_sharedram_r, &xevious_sharedram },
	{ 0x6800, 0x6807, xevious_dsw_r },
	{ 0x7100, 0x7100, xevious_customio_r },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu2[] =
{
	{ 0x7800, 0xcfff, xevious_sharedram_r },
	{ 0x6800, 0x6807, xevious_dsw_r },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu3[] =
{
	{ 0x7800, 0xcfff, xevious_sharedram_r },
	{ 0x6800, 0x6807, xevious_dsw_r },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu1[] =
{
	{ 0x7800, 0xcfff, xevious_sharedram_w },
	{ 0x6830, 0x6830, MWA_NOP },
	{ 0x7100, 0x7100, xevious_customio_w },
	{ 0x6800, 0x681f, pengo_sound_w, &pengo_soundregs },
        { 0x6820, 0x6820, xevious_interrupt_enable_1_w },
	{ 0x6821, 0x6821, xevious_interrupt_enable_2_w },
	{ 0x6822, 0x6822, xevious_interrupt_enable_3_w },
	{ 0x6823, 0x6823, xevious_halt_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8b80, 0x8bff, MWA_RAM, &spriteram },	/* these three are here just to initialize */
	{ 0x9380, 0x93ff, MWA_RAM, &spriteram_2 },	/* the pointers. The actual writes are */
	{ 0x9b80, 0x9bff, MWA_RAM, &spriteram_3 },	/* handled by xevious_sharedram_w() */
	{ 0xc400, 0xc7ff, MWA_RAM, &videoram },	/* dirtybuffer[] handling is not needed because */
	{ 0x8400, 0x87ff, MWA_RAM, &colorram },	/* characters are redrawn every frame */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu2[] =
{
	{ 0x7800, 0xcfff, xevious_sharedram_w },
	{ 0x6822, 0x6822, xevious_interrupt_enable_3_w },
	{ 0x0000, 0x0fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu3[] =
{
	{ 0x7800, 0xcfff, xevious_sharedram_w },
	{ 0x6822, 0x6822, xevious_interrupt_enable_3_w },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }	/* end of table */
};





static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, OSD_KEY_3, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_CONTROL, OSD_KEY_ALT, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xf7,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0xff,
		{ 0, 0, 0, OSD_KEY_F2, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { -1 }
};



static struct DSW dsw[] =
{
	{ 3, 0xc0, "LIVES", { "5", "2", "1", "3" }, 1 },
	{ 3, 0x30, "BONUS", { "30000 100000", "30000 80000", "20000 100000", "20000 80000" }, 1 },
	{ 4, 0x60, "DIFFICULTY", { "HARDEST", "HARD", "EASY", "NORMAL" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	3,	/* 2 bits per pixel */
	{ 0, 4, 128*64*8 },
	{ 39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
		7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout spritelayout2 =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	3,	/* 2 bits per pixel */
	{ 4, 128*64*8, 128*64*8+4 },
	{ 39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
		7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout spritelayout3 =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4,64*64*8 },
	{ 39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
		7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout, 0, 64 },
	{ 1, 0x1000, &charlayout1, 0, 64 },
	{ 1, 0x3000, &spritelayout1, 0, 64 },
	{ 1, 0x5000, &spritelayout2, 0, 64 },
	{ 1, 0x9000, &spritelayout3, 0, 64 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */

        0xff,0x00,0x00, /* RED */
        0x00,0xff,0x00, /* GREEN */
        0x00,0x00,0xff, /* BLUE */
        0xff,0xff,0x00, /* YELLOW */
        0xff,0x00,0xff, /* MAGENTA */
        0x00,0xff,0xff, /* CYAN */
        0xff,0xff,0xff, /* WHITE */
        0xE0,0xE0,0xE0, /* LTGRAY */
        0xC0,0xC0,0xC0, /* DKGRAY */

	0xe0,0xb0,0x70,	/* BROWN */
	0xd0,0xa0,0x60,	/* BROWN0 */
	0xc0,0x90,0x50,	/* BROWN1 */
	0xa3,0x78,0x3a,	/* BROWN2 */
	0x80,0x60,0x20,	/* BROWN3 */
	0x54,0x40,0x14,	/* BROWN4 */

        0x54,0xa8,0xff, /* LTBLUE */
        0x00,0xa0,0x00, /* DKGREEN */
        0x00,0xe0,0x00, /* GRASSGREEN */

	0xff,0xb6,0xdb,	/* PINK */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,0xb6,0x49,	/* DKORANGE */
	0x49,0xb6,0x92,	/* DKGREEN */
	0xff,0xb6,0x92,	/* LTORANGE */
	0xdb,0xdb,0xdb	/* GREY */
};

enum {BLACK,RED,GREEN,BLUE,YELLOW,MAGENTA,CYAN,WHITE,LTGRAY,DKGRAY,
       BROWN,BROWN0,BROWN1,BROWN2,BROWN3,BROWN4,
			 LTBLUE,DKGREEN,GRASSGREEN,
            };

static unsigned char colortable[] =
{
        0,1,2,14,4,13,6,7,
        0,2,14,4,13,6,7,8,
        0,14,4,13,6,7,8,7,
        0,4,13,6,7,8,17,9,
        0,13,6,7,8,7,9,2,
        0,6,7,8,7,9,2,5,
        0,7,8,7,9,2,5,13,
        0,8,7,9,2,5,13,8,
        0,7,9,2,5,13,8,13,
        0,9,2,5,13,8,13,7,
        0,2,5,13,8,13,7,2,
        0,5,13,8,13,7,2,14,
        0,13,8,13,7,2,14,4,
        0,8,13,7,2,14,4,5,
        0,13,7,2,14,4,5,6,
        0,13,7,2,14,4,5,6,
        0,1,2,14,4,13,6,7,
        0,2,14,4,13,6,7,8,
        0,14,4,13,6,7,8,7,
        0,4,13,6,7,8,17,9,
        0,13,6,7,8,7,9,2,
        0,6,7,8,7,9,2,5,
        0,7,8,7,9,2,5,13,
        0,8,7,9,2,5,13,8,
        0,7,9,2,5,13,8,13,
        0,9,2,5,13,8,13,7,
        0,2,5,13,8,13,7,2,
        0,5,13,8,13,7,2,14,
        0,13,8,13,7,2,14,4,
        0,8,13,7,2,14,4,5,
        0,13,7,2,14,4,5,6,
        0,13,7,2,14,4,5,6,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3125000,	/* 3.125 Mhz (?) */
			0,
			readmem_cpu1,writemem_cpu1,0,0,
			xevious_interrupt_1,1
		},
		{
			CPU_Z80,
			3125000,	/* 3.125 Mhz */
			2,	/* memory region #2 */
			readmem_cpu2,writemem_cpu2,0,0,
			xevious_interrupt_2,1
		},
		{
			CPU_Z80,
			3125000,	/* 3.125 Mhz */
			3,	/* memory region #3 */
			readmem_cpu3,writemem_cpu3,0,0,
			xevious_interrupt_3,2
		}
	},
	60,
	0,

	/* video hardware */
	28*8, 36*8, { 0*8, 28*8-1, 0*8, 36*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	xevious_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( xevious_rom )
	ROM_REGION(0x10000)	/* 64k for the first CPU */
	ROM_LOAD( "xe-1m-a.bin", 0x0000, 0x2000 )
	ROM_LOAD( "xe-1l-a.bin", 0x2000, 0x2000 )

	ROM_REGION(0xa000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "xe-3b.bin",   0x0000, 0x1000 )
	ROM_LOAD( "xe-3c.bin",   0x1000, 0x1000 )
	ROM_LOAD( "xe-3d.bin",   0x2000, 0x1000 )
	ROM_LOAD( "xe-4p.bin",   0x3000, 0x2000 )	/* sprite set #1, planes 0/1 */
	ROM_LOAD( "xe-4r.bin",   0x5000, 0x2000 )	/* sprite set #1, plane 2, set #2, plane 0 */
	ROM_LOAD( "xe-4m.bin",   0x7000, 0x2000 )	/* sprite set #2, planes 1/2 */
	ROM_LOAD( "xe-4n.bin",   0x9000, 0x1000 )	/* sprite set #3, planes 0/1 */

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "xe-2c-a.bin", 0x0000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "xe-4c-a.bin", 0x0000, 0x2000 )

	ROM_REGION(0x4000)	/* gfx map */
	ROM_LOAD( "xe-2a.bin",   0x0000, 0x1000 )
	ROM_LOAD( "xe-2b.bin",   0x1000, 0x2000 )
	ROM_LOAD( "xe-2c.bin",   0x3000, 0x1000 )
ROM_END



struct GameDriver xevious_driver =
{
	"Xevious",
	"xevious",
	"NICOLA SALMORIA\nMIRKO BUFFONI",
	&machine_driver,

	xevious_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, colortable,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	19, 0,
	8*13, 8*16, 20,

	0, 0
};
