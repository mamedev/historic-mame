/***************************************************************************

Q*bert Qubes: same as Q*bert with two banks of sprites
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern int qbert_vh_start(void);
extern void gottlieb_vh_init_optimized_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void gottlieb_sh_w(int offset, int data);
extern void gottlieb_sh_update(void);
extern void gottlieb_output(int offset, int data);
extern int qbert_IN1_r(int offset);
extern unsigned char *gottlieb_videoram;
extern unsigned char *gottlieb_paletteram;
extern unsigned char *gottlieb_spriteram;
extern void gottlieb_videoram_w(int offset,int data);
extern void gottlieb_paletteram_w(int offset,int data);
extern void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);
extern const char *gottlieb_sample_names[];


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5800, input_port_0_r },     /* DSW */
	{ 0x5801, 0x5801, qbert_IN1_r },     /* buttons */
	{ 0x5802, 0x5802, input_port_2_r },     /* trackball: not used */
	{ 0x5803, 0x5803, input_port_3_r },     /* trackball: not used */
	{ 0x5804, 0x5804, input_port_4_r },     /* joystick */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x2fff, MWA_RAM },
	{ 0x3000, 0x37ff, MWA_RAM, &gottlieb_spriteram },
	{ 0x3800, 0x3fff, gottlieb_videoram_w, &gottlieb_videoram },
	{ 0x4000, 0x4fff, MWA_RAM }, /* bg object ram... ? not used ? */
	{ 0x5000, 0x57ff, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x5800, 0x5800, MWA_RAM },    /* watchdog timer clear */
	{ 0x5801, 0x5801, MWA_RAM },    /* trackball: not used */
	{ 0x5802, 0x5802, gottlieb_sh_w }, /* sound/speech command */
	{ 0x5803, 0x5803, gottlieb_output },       /* OUT1 */
	{ 0x5804, 0x5804, MWA_RAM },    /* OUT2 */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct InputPort input_ports[] =
{
	{       /* DSW */
		0x0,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* buttons */
		0x40,   /* test mode off */
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, 0 /* coin 2 */,
				0, 0, 0, OSD_KEY_F2 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball: not used */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball: not used */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* 2 joysticks (cocktail mode) mapped to one */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
			OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
			OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN },
	},
	{ -1 }  /* end of table */
};


static struct KEYSet keys[] =
{
	{ 4, 0, "MOVE DOWN RIGHT" },
	{ 4, 1, "MOVE UP LEFT"  },
	{ 4, 2, "MOVE UP RIGHT" },
	{ 4, 3, "MOVE DOWN LEFT" },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 0, 0x08, "ATTRACT MODE SOUND", { "ON", "OFF" } },
/* 
   Too lazy to enter such a big table of coins/credit until non-consecutive
   dip-switches are handled... 8-)
   Dip-switches 2-3-4-5 are at locations 0x01, 0x04, 0x10, 0x20
   Two remarkable values:
	0x01 0x04 0x10 0x20
	-------------------
	 0    0    0    0     1-1 1-1
	 0    1    1    1     Free play
*/
	{ 0, 0x02, "1ST EXTRA LIFE AT", { "10000", "15000" } },
	{ 0, 0x40, "ADDITIONAL LIFE EVERY", { "20K", "25K" } },
	{ 0, 0x80, "DIFFICULTY LEVEL", { "NORMAL", "HARD" } },
/* the following switch must be connected to the IP16 line */
/*      { 1, 0x40, "TEST MODE", {"ON", "OFF"} },*/
	{ -1 }
};


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	{ 28, 24, 20, 16, 12, 8, 4, 0},
	32*8    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	512,    /* 512 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 0x4000*8, 0x8000*8, 0xC000*8 },
	{ 0 * 16, 1 * 16, 2 * 16, 3 * 16, 4 * 16, 5 * 16, 6 * 16, 7 * 16,
		8 * 16, 9 * 16, 10 * 16, 11 * 16, 12 * 16, 13 * 16, 14 * 16, 15 * 16 },
	{ 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout fakelayout =
{
	1,1,
	0,
	1,
	{ 0 },
	{ 0 },
	{ 0 },
	0
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   16, 2 }, /* white & yellow palettes for Mame's messages */
	{ 1, 0x0000, &charlayout,   0, 1 },
	{ 1, 0x2000, &spritelayout, 0, 1 },
	{ 0, 0, &fakelayout, 3*16, 16 },
	{ -1 } /* end of array */
};

static const struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_I86,
			5000000,        /* 5 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		}
	},
	60,     /* frames / second */
	0,      /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,256+3*16,        /* 256 for colormap, 1*16 for the game, 2*16 for the dsw menu. Silly, isn't it ? */
	gottlieb_vh_init_optimized_color_palette,

	0,      /* init vh */
	qbert_vh_start,
	generic_vh_stop,
	gottlieb_vh_screenrefresh,

	/* sound hardware */
	0,      /* samples */
	0,
	0,
	0,
	gottlieb_sh_update
};

ROM_START( qbertqub_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "qq-rom0.bin",  0xe000, 0x2000 )
	ROM_LOAD( "qq-rom1.bin",  0xc000, 0x2000 )
	ROM_LOAD( "qq-rom2.bin",  0xa000, 0x2000 )
	ROM_LOAD( "qq-rom3.bin",  0x8000, 0x2000 )

	ROM_REGION(0x12000)      /* temporary space for graphics */
	ROM_LOAD( "qq-bg0.bin",  0x0000, 0x1000 )
	ROM_LOAD( "qq-bg1.bin",  0x1000, 0x1000 )
	ROM_LOAD( "qq-fg3.bin",  0x2000, 0x4000 )       /* sprites */
	ROM_LOAD( "qq-fg2.bin",  0x6000, 0x4000 )       /* sprites */
	ROM_LOAD( "qq-fg1.bin",  0xA000, 0x4000 )       /* sprites */
	ROM_LOAD( "qq-fg0.bin",  0xE000, 0x4000 )       /* sprites */
ROM_END

static unsigned short qbertqub_colors[256]={
	0x000, 0xff0, 0xfff,
	0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007,
	0x008, 0x009, 0x00a, 0x00b, 0x00f, 0x010, 0x011, 0x013,
	0x017, 0x018, 0x01a, 0x020, 0x022, 0x024, 0x028, 0x029,
	0x02b, 0x030, 0x033, 0x035, 0x039, 0x03a, 0x03c, 0x040,
	0x044, 0x045, 0x046, 0x047, 0x048, 0x049, 0x04a, 0x04b,
	0x04c, 0x04d, 0x050, 0x055, 0x057, 0x05b, 0x05c, 0x060,
	0x066, 0x068, 0x06c, 0x06d, 0x070, 0x077, 0x078, 0x079,
	0x07d, 0x07f, 0x080, 0x088, 0x08e, 0x090, 0x099, 0x09a,
	0x09b, 0x09c, 0x09d, 0x09e, 0x09f, 0x0f0, 0x0f7, 0x0ff,
	0x100, 0x101, 0x104, 0x10c, 0x110, 0x111, 0x112, 0x113,
	0x170, 0x200, 0x202, 0x20d, 0x212, 0x215, 0x220, 0x222,
	0x280, 0x300, 0x301, 0x303, 0x313, 0x320, 0x323, 0x324,
	0x325, 0x326, 0x330, 0x332, 0x333, 0x390, 0x400, 0x402,
	0x404, 0x410, 0x414, 0x420, 0x434, 0x440, 0x442, 0x443,
	0x444, 0x4a0, 0x4b0, 0x500, 0x503, 0x504, 0x505, 0x510,
	0x514, 0x520, 0x535, 0x543, 0x549, 0x550, 0x552, 0x553,
	0x555, 0x560, 0x570, 0x580, 0x590, 0x5a0, 0x5b0, 0x600,
	0x604, 0x606, 0x614, 0x620, 0x621, 0x630, 0x631, 0x636,
	0x643, 0x650, 0x653, 0x662, 0x666, 0x700, 0x701, 0x704,
	0x705, 0x706, 0x707, 0x715, 0x730, 0x732, 0x737, 0x740,
	0x742, 0x743, 0x750, 0x753, 0x767, 0x772, 0x777, 0x800,
	0x804, 0x806, 0x808, 0x812, 0x826, 0x837, 0x840, 0x843,
	0x850, 0x853, 0x867, 0x882, 0x888, 0x900, 0x901, 0x904,
	0x923, 0x937, 0x950, 0x960, 0x967, 0x992, 0x999, 0xa00,
	0xa02, 0xa04, 0xa10, 0xa34, 0xa50, 0xa60, 0xa67, 0xa70,
	0xa92, 0xaaa, 0xb00, 0xb03, 0xb04, 0xb20, 0xb45, 0xb50,
	0xb67, 0xb70, 0xb81, 0xb92, 0xbbb, 0xc00, 0xc04, 0xc20,
	0xc30, 0xc50, 0xc56, 0xc67, 0xc80, 0xc92, 0xccc, 0xd00,
	0xd40, 0xd50, 0xd67, 0xd90, 0xddd, 0xe00, 0xe50, 0xea0,
	0xeee, 0xf00, 0xf07, 0xf0f, 0xf70, 0xf86, 0xfb0
};


struct GameDriver qbertqub_driver =
{
        "Q*Bert Qubes",
	"qbertqub",
        "FABRICE FRANCES\nRODIMUS PRIME",
	&machine_driver,

	qbertqub_rom,
	0, 0,   /* rom decode and opcode decode functions */
	gottlieb_sample_names,

	input_ports, dsw, keys,

	(char *)qbertqub_colors,
	0,0,    /* palette, colortable */

	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,    /* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,       /* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	1,2,      /* white & yellow for dsw menu */
	8*11,8*20,1, /* paused message displayed at X,Y and color */

	0,0     /* hi-score load and save */
};

