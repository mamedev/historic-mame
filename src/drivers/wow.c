/****************************************************************************
 * This driver is currently being worked on by Mike@Dissfulfils.co.uk
 *
 * Although each machine shares a proportion of common hardware, there
 * seem to be some marked differences in function of switches & keys
 *
 * To make it easier to differentiate between each machine's settings
 * I have split it into a separate section for each driver.
 *
 * Default settings are declared first, and these can be used by any
 * driver that does not need to customise them differently
 *
 * Memory Maps are at the end
 ****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"

extern unsigned char *wow_videoram;

int  wow_intercept_r(int offset);
void wow_videoram_w(int offset,int data);
void wow_magic_expand_color_w(int offset,int data);
void wow_magic_control_w(int offset,int data);
void wow_magicram_w(int offset,int data);
void wow_pattern_board_w(int offset,int data);
void wow_vh_screenrefresh(struct osd_bitmap *bitmap);
int  wow_video_retrace_r(int offset);

void wow_interrupt_enable_w(int offset, int data);
void wow_interrupt_w(int offset, int data);
int  wow_interrupt(void);

int  seawolf2_controller1_r(int offset);
int  seawolf2_controller2_r(int offset);
void seawolf2_vh_screenrefresh(struct osd_bitmap *bitmap);

int  gorf_interrupt(void);
void gorf_videoram_w(int offset,int data); 	/* ASG */
void gorf_magicram_w(int offset,int data);	/* ASG */

/* These calls don't do anything but they stop unwanted lines appearing
 * in the logfile! (and I may get them to do something later!)
 */

void colour_register_w(int offset, int data);
void paging_register_w(int offset, int data);

/*
 * For Testing ports
 */

#if 0

static struct InputPort testing_input_ports[] =
{
	{	/* IN0 */
		0x0,
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, OSD_KEY_4,
				OSD_KEY_5, OSD_KEY_6, OSD_KEY_7, OSD_KEY_8 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0x0,
		{ OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_KEY_R, OSD_KEY_T, OSD_KEY_Y, OSD_KEY_U, OSD_KEY_I },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x0,
		{ OSD_KEY_A, OSD_KEY_S, OSD_KEY_D, OSD_KEY_F, OSD_KEY_G, OSD_KEY_H, OSD_KEY_J, OSD_KEY_K },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x0,
		{ OSD_KEY_Z, OSD_KEY_X, OSD_KEY_C, OSD_KEY_V, OSD_KEY_B, OSD_KEY_N, OSD_KEY_M, OSD_KEY_L },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct DSW test_dsw[] =
{
	{ 3, 0x01, "BIT 0", { "ON", "OFF" }, 1 },
	{ 3, 0x02, "BIT 1", { "ON", "OFF" }, 1 },
	{ 3, 0x04, "BIT 2", { "ON", "OFF" }, 1 },
	{ 3, 0x08, "BIT 3", { "ON", "OFF" }, 1 },
	{ 3, 0x10, "BIT 4", { "ON", "OFF" }, 1 },
	{ 3, 0x20, "BIT 5", { "ON", "OFF" }, 1 },
	{ 3, 0x40, "BIT 6", { "ON", "OFF" }, 1 },
	{ 3, 0x80, "BIT 7", { "ON", "OFF" }, 1 },
	{ -1 }
};

static struct KEYSet test_keys[] =
{
        { -1 }
};

#endif


/*
 * Default Settings
 */

static struct MemoryReadAddress readmem[] =
{
    { 0xD000, 0xDfff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xcfff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
    { 0xD000, 0xDfff, MWA_RAM },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },	/* ASG */
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x8000, 0xcfff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x08, 0x08, wow_intercept_r },
    { 0x0E, 0x0E, wow_video_retrace_r },
	{ 0x10, 0x10, input_port_0_r },
	{ 0x11, 0x11, input_port_1_r },
	{ 0x12, 0x12, input_port_2_r },
	{ 0x13, 0x13, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x0d, 0x0d, interrupt_vector_w },
	{ 0x19, 0x19, wow_magic_expand_color_w },
	{ 0x0c, 0x0c, wow_magic_control_w },
	{ 0x78, 0x7e, wow_pattern_board_w },
    { 0x0F, 0x0F, wow_interrupt_w },
    { 0x0E, 0x0E, wow_interrupt_enable_w },
    { 0x00, 0x08, colour_register_w },
    { 0x5B, 0x5B, paging_register_w },
	{ -1 }	/* end of table */
};

static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_3, OSD_KEY_4, OSD_KEY_F1, OSD_KEY_F2,
				OSD_KEY_5, OSD_KEY_1, OSD_KEY_2, OSD_KEY_6 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_E, OSD_KEY_D, OSD_KEY_S, OSD_KEY_F, OSD_KEY_X, OSD_KEY_G, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_ALT, OSD_KEY_CONTROL, 0, OSD_KEY_O  },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, OSD_JOY_FIRE, 0, 0 }
	},
	{	/* DSW */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct KEYSet keys[] =
{
        { 2, 0, "MOVE UP" },
        { 2, 2, "MOVE LEFT"  },
        { 2, 3, "MOVE RIGHT" },
        { 2, 1, "MOVE DOWN" },
        { 2, 5, "FIRE1" },
        { 2, 4, "FIRE2" },
        { -1 }
};

static struct DSW dsw[] =
{
	{ 3, 0x10, "LIVES", { "3 7", "2 5" }, 1 },
	{ 3, 0x20, "BONUS", { "4TH LEVEL", "3RD LEVEL" }, 1 },
    { 3, 0x40, "PAYMENT", { "FREE", "COIN" }, 1 },
	{ 3, 0x80, "DEMO SOUNDS", { "OFF", "ON" }, 1 },
	{ -1 }
};

static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xff,0xff,0x00,	/* YELLOW */
	0x00,0x00,0xff,	/* BLUE */
	0xff,0x00,0x00,	/* RED */
	0xff,0xff,0xff	/* WHITE */
};

enum { BLACK,YELLOW,BLUE,RED,WHITE };

static unsigned char colortable[] =
{
	BLACK,YELLOW,BLUE,RED,
	BLACK,WHITE,BLACK,RED	/* not used by the game, here only for the dip switch menu */
};


/****************************************************************************
 * Wizard of Wor
 ****************************************************************************/

ROM_START( wow_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "wow.x1", 0x0000, 0x1000, 0x222baf9b )
	ROM_LOAD( "wow.x2", 0x1000, 0x1000, 0xccb3fafb )
	ROM_LOAD( "wow.x3", 0x2000, 0x1000, 0xe4a6665e )
	ROM_LOAD( "wow.x4", 0x3000, 0x1000, 0xf3659d69 )
	ROM_LOAD( "wow.x5", 0x8000, 0x1000, 0xda26e8d8 )
	ROM_LOAD( "wow.x6", 0x9000, 0x1000, 0x550a3720 )
	ROM_LOAD( "wow.x7", 0xa000, 0x1000, 0x0ef6502c )
/*	ROM_LOAD( "wow.x8", 0xc000, 0x1000, ? )	here would go the foreign language ROM */
ROM_END

static struct InputPort wow_input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_3, OSD_KEY_4, 0, OSD_KEY_F2,
				0, OSD_KEY_1, OSD_KEY_2, 0},
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xef,
		{ OSD_KEY_E, OSD_KEY_D, OSD_KEY_S, OSD_KEY_F, OSD_KEY_X, OSD_KEY_G, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xef,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_ALT, OSD_KEY_CONTROL, 0, 0},
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, OSD_JOY_FIRE, 0, 0 }
	},
	{	/* DSW */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct DSW wow_dsw[] =
{
	{ 3, 0x10, "LIVES", { "3 7", "2 5" }, 1 },
	{ 3, 0x20, "BONUS", { "4TH LEVEL", "3RD LEVEL" }, 1 },
    { 3, 0x40, "PAYMENT", { "FREE", "COIN" }, 1 },
	{ 3, 0x80, "DEMO SOUNDS", { "OFF", "ON" }, 1 },
    { 0, 0x80, "FLIP SCREEN", { "YES", "NO" }, 1 },
/* 	{ 3, 0x08, "LANGUAGE", { "FOREIGN", "ENGLISH" }, 1 }, */
	{ -1 }
};

static struct MachineDriver wow_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	320, 204, { 0, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

struct GameDriver wow_driver =
{
        "Wizard of Wor",
	"wow",
        "NICOLA SALMORIA\nSTEVE SCAVONE\nMIKE COATES",
	&wow_machine_driver,

	wow_rom,
	0, 0,
	0,

	wow_input_ports, trak_ports, wow_dsw, keys,

	0, palette, colortable,
	(320-8*6)/2, (204-10)/2,

	0, 0
};

/****************************************************************************
 * Robby Roto
 ****************************************************************************/

ROM_START( robby_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "rotox1.bin",  0x0000, 0x1000, 0x90debdc2 )
	ROM_LOAD( "rotox2.bin",  0x1000, 0x1000, 0xfa125fb2 )
	ROM_LOAD( "rotox3.bin",  0x2000, 0x1000, 0x88e6e5bc )
	ROM_LOAD( "rotox4.bin",  0x3000, 0x1000, 0x111241ea )
	ROM_LOAD( "rotox5.bin",  0x8000, 0x1000, 0x3b7e01f0 )
	ROM_LOAD( "rotox6.bin",  0x9000, 0x1000, 0x478421ee )
	ROM_LOAD( "rotox7.bin",  0xa000, 0x1000, 0xf9ac77a0 )
	ROM_LOAD( "rotox8.bin",  0xb000, 0x1000, 0x799f8a3d )
  	ROM_LOAD( "rotox9.bin",  0xc000, 0x1000, 0xaa9f62f5 )
	ROM_LOAD( "rotox10.bin", 0xd000, 0x1000, 0x55a239fa )
ROM_END

static struct MemoryReadAddress robby_readmem[] =
{
	{ 0xe000, 0xffff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xdfff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress robby_writemem[] =
{
	{ 0xe000, 0xffff, MWA_RAM },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x8000, 0xdfff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MachineDriver robby_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			robby_readmem,robby_writemem,readport,writeport,
			wow_interrupt,230
		}
	},
	60,
	0,

	/* video hardware */
	320, 204, { 0, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

struct GameDriver robby_driver =
{
        "Robby Roto",
	"robby",
        "NICOLA SALMORIA\nSTEVE SCAVONE\nMIKE COATES",
	&robby_machine_driver,

	robby_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	0, palette, colortable,
	(320-8*6)/2, (204-10)/2,

	0, 0
};

/****************************************************************************
 * Gorf
 ****************************************************************************/

ROM_START( gorf_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "gorf-a.bin", 0x0000, 0x1000, 0x4dbb1c41 )
	ROM_LOAD( "gorf-b.bin", 0x1000, 0x1000, 0xed7b0427 )
	ROM_LOAD( "gorf-c.bin", 0x2000, 0x1000, 0xe101f49b )
	ROM_LOAD( "gorf-d.bin", 0x3000, 0x1000, 0xd12d3e47 )
	ROM_LOAD( "gorf-e.bin", 0x8000, 0x1000, 0x07ab966d )
	ROM_LOAD( "gorf-f.bin", 0x9000, 0x1000, 0xf7d14e9b )
	ROM_LOAD( "gorf-g.bin", 0xa000, 0x1000, 0x0ddd505f )
	ROM_LOAD( "gorf-h.bin", 0xb000, 0x1000, 0x5e488f10 )
ROM_END

/* ASG begin */
static struct MemoryWriteAddress gorf_writemem[] =
{
    { 0xD000, 0xDfff, MWA_RAM },
	{ 0x4000, 0x7fff, gorf_videoram_w, &wow_videoram, &videoram_size },
	{ 0x0000, 0x3fff, gorf_magicram_w },
	{ 0x8000, 0xcfff, MWA_ROM },
	{ -1 }	/* end of table */
};
/* ASG end */

static struct InputPort Gorf_input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_3, OSD_KEY_4, OSD_KEY_F1, OSD_KEY_F2,
				OSD_KEY_1, OSD_KEY_2, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* DSW */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct KEYSet Gorf_keys[] =
{
        { 2, 0, "MOVE UP" },
        { 2, 2, "MOVE LEFT"  },
        { 2, 3, "MOVE RIGHT" },
        { 2, 1, "MOVE DOWN" },
        { 2, 4, "FIRE" },
        { -1 }
};

static struct MachineDriver gorf_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,gorf_writemem,readport,writeport,		/* ASG */
			gorf_interrupt,256
		}
	},
	60,
	0,

	/* video hardware */
	204, 320, { 0, 204-1, 0, 320-1 },	/* ASG */
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

struct GameDriver gorf_driver =
{
    "Gorf",
	"gorf",
    "NICOLA SALMORIA\nSTEVE SCAVONE\nMIKE COATES",
	&gorf_machine_driver,

	gorf_rom,
	0, 0,
	0,

	Gorf_input_ports, trak_ports, dsw, Gorf_keys,

	0, palette, colortable,
	(204-10)/2, (320-8*6)/2, 	/* ASG */

	0, 0
};

/****************************************************************************
 * Space Zap
 ****************************************************************************/

ROM_START( spacezap_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "0662.01", 0x0000, 0x1000, 0xaf642ac8 )
	ROM_LOAD( "0663.xx", 0x1000, 0x1000, 0x0cb6228e )
	ROM_LOAD( "0664.xx", 0x2000, 0x1000, 0x2986a6d6 )
	ROM_LOAD( "0665.xx", 0x3000, 0x1000, 0xb3037b39 )
ROM_END

static struct InputPort spacezap_input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_3, OSD_KEY_4, OSD_KEY_F1, OSD_KEY_F2,
				0, OSD_KEY_1, OSD_KEY_2, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* DSW */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct KEYSet spacezap_keys[] =
{
        { 2, 0, "MOVE UP" },
        { 2, 2, "MOVE LEFT"  },
        { 2, 3, "MOVE RIGHT" },
        { 2, 1, "MOVE DOWN" },
        { 2, 4, "FIRE" },
        { -1 }
};

static struct DSW spacezap_dsw[] =
{
	{ 3, 0x20, "BONUS", { "4TH LEVEL", "3RD LEVEL" }, 1 },
	{ 3, 0x80, "DEMO SOUNDS", { "OFF", "ON" }, 1 },
	{ -1 }
};

static struct MachineDriver spacezap_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,readport,writeport,
			wow_interrupt,230
		}
	},
	60,
	0,

	/* video hardware */
	320, 204, { 0, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

struct GameDriver spacezap_driver =
{
        "Space Zap",
	"spacezap",
        "NICOLA SALMORIA\nSTEVE SCAVONE\nMIKE COATES",
	&spacezap_machine_driver,

	spacezap_rom,
	0, 0,
	0,

	spacezap_input_ports, trak_ports, spacezap_dsw, spacezap_keys,

	0, palette, colortable,
	(320-8*6)/2, (204-10)/2,

	0, 0
};

/****************************************************************************
 * Seawolf II
 ****************************************************************************/

ROM_START( seawolf2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sw2x1.bin", 0x0000, 0x0800, 0x8a11d667 )
	ROM_LOAD( "sw2x2.bin", 0x0800, 0x0800, 0x1d7d50a1 )
	ROM_LOAD( "sw2x3.bin", 0x1000, 0x0800, 0x60364550 )
	ROM_LOAD( "sw2x4.bin", 0x1800, 0x0800, 0xd28a15b2 )
ROM_END

static struct MemoryReadAddress seawolf2_readmem[] =
{
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress seawolf2_writemem[] =
{
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ -1 }	/* end of table */
};

static struct InputPort seawolf2_input_ports[] =
{
	{	/* IN0 */
		0x0,
		{ 0, 0, 0, 0, 0, 0, 0, OSD_KEY_CONTROL },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0x0,
		{ OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_KEY_R, OSD_KEY_T, OSD_KEY_Y, OSD_KEY_U, OSD_KEY_I },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x0,
		{ OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0xFF,
		{ OSD_KEY_Z, OSD_KEY_X, OSD_KEY_C, OSD_KEY_V, OSD_KEY_B, OSD_KEY_N, OSD_KEY_M, OSD_KEY_L },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct DSW seawolf2_dsw[] =
{
	{ 0, 0x40, "LANGUAGE 1", { "LANGUAGE 2", "FRENCH" }, 1 },
	{ 2, 0x08, "LANGUAGE 2", { "ENGLISH", "GERMAN" }, 1 },
        { 3, 0x06, "PLAY TIME", { "70", "60", "50", "40" } },
	{ 3, 0x80, "TEST MODE", { "ON", "OFF" }, 1 },
	{ -1 }
};

static struct IOReadPort seawolf2_readport[] =
{
	{ 0x08, 0x08, wow_intercept_r },
        { 0x0E, 0x0E, wow_video_retrace_r },
	{ 0x10, 0x10, seawolf2_controller2_r },
	{ 0x11, 0x11, seawolf2_controller1_r },
	{ 0x12, 0x12, input_port_2_r },
	{ 0x13, 0x13, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct KEYSet seawolf2_keys[] =
{
        { 0, 0, "PLAYER 1 LEFT"  },
        { 0, 1, "PLAYER 1 RIGHT" },
        { 0, 7, "PLAYER 1 FIRE" },
        { 1, 0, "PLAYER 2 LEFT"  },
        { 1, 1, "PLAYER 2 RIGHT" },
        { 1, 7, "PLAYER 2 FIRE" },
        { -1 }
};


static struct MachineDriver seawolf_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			seawolf2_readmem,seawolf2_writemem,seawolf2_readport,writeport,
			wow_interrupt,230
		}
	},
	60,
	0,

	/* video hardware */
	320, 204, { 1, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	seawolf2_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

struct GameDriver seawolf_driver =
{
        "Sea Wolf II",
	"seawolf2",
        "NICOLA SALMORIA\nSTEVE SCAVONE\nMIKE COATES",
	&seawolf_machine_driver,

	seawolf2_rom,
	0, 0,
	0,

	seawolf2_input_ports, trak_ports, seawolf2_dsw, seawolf2_keys,

	0, palette, colortable,
	(320-8*6)/2, (204-10)/2,

	0, 0
};

/***************************************************************************

Wizard of Wor memory map (preliminary)

0000-3fff ROM A/B/C/D but also "magic" RAM (which processes data and copies it to the video RAM)
4000-7fff SCREEN RAM (bitmap)
8000-afff ROM E/F/G
c000-cfff ROM X (Foreign language - not required)
d000-d3ff STATIC RAM

I/O ports:
IN:
08        intercept register (collision detector)
          bit 0: intercept in pixel 3 in an OR or XOR write since last reset
          bit 1: intercept in pixel 2 in an OR or XOR write since last reset
          bit 2: intercept in pixel 1 in an OR or XOR write since last reset
          bit 3: intercept in pixel 0 in an OR or XOR write since last reset
          bit 4: intercept in pixel 3 in last OR or XOR write
          bit 5: intercept in pixel 2 in last OR or XOR write
          bit 6: intercept in pixel 1 in last OR or XOR write
          bit 7: intercept in pixel 0 in last OR or XOR write
10        IN0
11        IN1
12        IN2
13        DSW
14		  Video Retrace
15        ?
17        ?

*
 * IN0 (all bits are inverted)
 * bit 7 : flip screen
 * bit 6 : START 2
 * bit 5 : START 1
 * bit 4 : SLAM
 * bit 3 : TEST
 * bit 2 : COIN 3
 * bit 1 : COIN 2
 * bit 0 : COIN 1
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : FIRE player 1
 * bit 4 : MOVE player 1
 * bit 3 : RIGHT player 1
 * bit 2 : LEFT player 1
 * bit 1 : DOWN player 1
 * bit 0 : UP player 1
 *
*
 * IN2 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : FIRE player 2
 * bit 4 : MOVE player 2
 * bit 3 : RIGHT player 2
 * bit 2 : LEFT player 2
 * bit 1 : DOWN player 2
 * bit 0 : UP player 2
 *
*
 * DSW (all bits are inverted)
 * bit 7 : Attract mode sound  0 = Off  1 = On
 * bit 6 : 1 = Coin play  0 = Free Play
 * bit 5 : Bonus  0 = After fourth level  1 = After third level
 * bit 4 : Number of lives  0 = 3/7  1 = 2/5
 * bit 3 : language  1 = English  0 = Foreign
 * bit 2 : \ right coin slot  00 = 1 coin 5 credits  01 = 1 coin 3 credits
 * bit 1 : /                  10 = 2 coins 1 credit  11 = 1 coin 1 credit
 * bit 0 : left coin slot 0 = 2 coins 1 credit  1 = 1 coin 1 credit
 *


OUT:
00-07     palette (00-03 = left part of screen; 04-07 right part)
09        position where to switch from the "left" to the "right" palette.
08        select video mode (0 = low res 160x102, 1 = high res 320x204)
0a        screen height
0b        color block transfer
0c        magic RAM control
          bit 7: ?
          bit 6: flip
          bit 5: draw in XOR mode
          bit 4: draw in OR mode
          bit 3: "expand" mode (convert 1bpp data to 2bpp)
          bit 2: ?
          bit 1:\ shift amount to be applied before copying
          bit 0:/
0d        set interrupt vector
10-18     sound
19        magic RAM expand mode color
78-7e     pattern board (see vidhrdw.c for details)

***************************************************************************

Seawolf Specific Bits

IN
10		Controller Player 2
		bit 0-5	= rotary controller
        bit 7   = fire button

11		Controller Player 1
		bit 0-5 = rotary controller
        bit 6   = Language Select 1  (On = French, Off = language 2)
        bit 7   = fire button

12      bit 0   = Coin
		bit 1   = Start 1 Player
        bit 2   = Start 2 Player
        bit 3   = Language 2         (On = German, Off = English)

13      bit 0   = ?
		bit 1   = \ Length of time	 (40, 50, 60 or 70 Seconds)
        bit 2   = / for Game
        bit 3   = ?
        bit 4   = ?
        bit 5   = ?
        bit 6   = ?
        bit 7   = Test

***************************************************************************/
