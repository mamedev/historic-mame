/***************************************************************************

Super Cobra memory map (preliminary)

0000-5fff ROM (Lost Tomb: 0000-6fff)
8000-87ff RAM
8800-8bff Video RAM
9000-90ff Object RAM
  9000-903f  screen attributes
  9040-905f  sprites
  9060-907f  bullets
  9080-90ff  unused?

read:
b000      Watchdog Reset
9800      IN0
9801      IN1
9802      IN2

*
 * IN0 (all bits are inverted)
 * bit 7 : COIN 1
 * bit 6 : COIN 2
 * bit 5 : LEFT player 1
 * bit 4 : RIGHT player 1
 * bit 3 : SHOOT 1 player 1
 * bit 2 : CREDIT
 * bit 1 : SHOOT 2 player 1
 * bit 0 : UP player 2 (TABLE only)
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : START 1
 * bit 6 : START 2
 * bit 5 : LEFT player 2 (TABLE only)
 * bit 4 : RIGHT player 2 (TABLE only)
 * bit 3 : SHOOT 1 player 2 (TABLE only)
 * bit 2 : SHOOT 2 player 2 (TABLE only)
 * bit 1 : nr of lives  0 = 3  1 = 5
 * bit 0 : allow continue 0 = NO  1 = YES
*
 * IN2 (all bits are inverted)
 * bit 7 : protection check?
 * bit 6 : DOWN player 1
 * bit 5 : protection check?
 * bit 4 : UP player 1
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 :\ coins per play
 * bit 1 :/ (00 = 99 credits!)
 * bit 0 : DOWN player 2 (TABLE only)
 *

write:
a801      interrupt enable
a802      coin counter
a803      ? (POUT1)
a804      stars on
a805      ? (POUT2)
a806      screen vertical flip
a807      screen horizontal flip
a000      To AY-3-8910 port A (commands for the audio CPU)
a001      bit 3 = trigger interrupt on audio CPU
a002      protection check control?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



int scramble_IN2_r(int offset);
int scramble_protection_r(int offset);

extern unsigned char *scramble_attributesram;
extern unsigned char *scramble_bulletsram;
extern int scramble_bulletsram_size;
void scramble_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void scramble_attributes_w(int offset,int data);
void scramble_stars_w(int offset,int data);
int scramble_vh_start(void);
void scramble_vh_screenrefresh(struct osd_bitmap *bitmap);

int scramble_sh_interrupt(void);
int scramble_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x8bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0xb000, 0xb000, MRA_NOP },
	{ 0x9800, 0x9800, input_port_0_r },	/* IN0 */
	{ 0x9801, 0x9801, input_port_1_r },	/* IN1 */
	{ 0x9802, 0x9802, scramble_IN2_r },	/* IN2 */
	{ 0x9000, 0x907f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0xa002, 0xa002, scramble_protection_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8bff, videoram_w, &videoram, &videoram_size },
	{ 0x9000, 0x903f, scramble_attributes_w, &scramble_attributesram },
	{ 0x9040, 0x905f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9060, 0x907f, MWA_RAM, &scramble_bulletsram, &scramble_bulletsram_size },
	{ 0xa801, 0xa801, interrupt_enable_w },
	{ 0xa804, 0xa804, scramble_stars_w },
	{ 0xa000, 0xa000, sound_command_w },
	{ 0x0000, 0x6fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x0000, 0x17ff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x0000, 0x17ff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort sound_readport[] =
{
	{ 0x80, 0x80, AY8910_read_port_0_r },
	{ 0x20, 0x20, AY8910_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x40, 0x40, AY8910_control_port_0_w },
	{ 0x80, 0x80, AY8910_write_port_0_w },
	{ 0x10, 0x10, AY8910_control_port_1_w },
	{ 0x20, 0x20, AY8910_write_port_1_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ 0, OSD_KEY_ALT, OSD_KEY_3, OSD_KEY_CONTROL, OSD_KEY_RIGHT, OSD_KEY_LEFT, 0, 0 },
		{ 0, OSD_JOY_FIRE2, 0, OSD_JOY_FIRE1, OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{	/* IN1 */
		0xfd,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_2, OSD_KEY_1 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xf3,
		{ 0, 0, 0, 0, OSD_KEY_UP, 0, OSD_KEY_DOWN, 0 },
		{ 0, 0, 0, 0, OSD_JOY_UP, 0, OSD_JOY_DOWN, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct KEYSet keys[] =
{
        { 2, 4, "MOVE UP" },
        { 0, 5, "MOVE LEFT"  },
        { 0, 4, "MOVE RIGHT" },
        { 2, 6, "MOVE DOWN" },
        { 0, 3, "FIRE1" },
        { 0, 1, "FIRE2" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 1, 0x02, "LIVES", { "3", "5" } },
	{ 1, 0x01, "ALLOW CONTINUE", { "NO", "YES" } },
	{ -1 }
};


/*
        JRT: Order is 0 -> 7
                IN0                                     IN1:                            IN2:
                0: Start2                       0: (DIP) Free Play      0: DIP?
                1: Start1                       1: (DIP) Demo Mode      1: DIP?
                2: Up                           2: Fire U                       2: DIP?
                3: Down                         3: Fire D                       3: DIP?
                4: Left                         4: Fire R                       4: DIP?
                5: Right                        5: Fire L                       5: DIP?
                6: Coin (2?)            6: Whip                         6: DIP?
                7: Coin (1?)            7: DIP?                         7: DIP?

*/
static struct InputPort LTinput_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_2, OSD_KEY_1, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_3, OSD_KEY_4 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xfd,
		{ 0, 0, OSD_KEY_E, OSD_KEY_D, OSD_KEY_F, OSD_KEY_S, OSD_KEY_CONTROL, OSD_KEY_1 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct DSW LTdsw[] =
{
	{ 1, 0x02, "FREE PLAY", { "NO", "YES" } },
	{ 1, 0x01, "PLAYER CAN DIE", { "NO", "YES" } },
/*   JRT: In Development
        { 1, 0x01, "DEMO MODE", { "ON", "OFF" } },

        { 1, 0x02, "DIP1 1", { "ON", "OFF" } },

        { 2, 0x80, "DIP1 7", { "ON", "OFF" } },
        { 2, 0x01, "DIP2 0", { "ON", "OFF" } },
        { 2, 0x02, "DIP2 1", { "ON", "OFF" } },
        { 2, 0x04, "DIP2 2", { "ON", "OFF" } },
        { 2, 0x08, "DIP2 3", { "ON", "OFF" } },
        { 2, 0x10, "DIP2 4", { "ON", "OFF" } },
        { 2, 0x20, "DIP2 5", { "ON", "OFF" } },
        { 2, 0x40, "DIP2 6", { "ON", "OFF" } },
        { 2, 0x80, "DIP2 7", { "ON", "OFF" } },
*/
	{ -1 }
};


static struct KEYSet LTkeys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 3, "MOVE DOWN" },
        { 0, 5, "MOVE RIGHT"  },
        { 0, 4, "MOVE LEFT" },
        { 1, 2, "FIRE UP" },
        { 1, 3, "FIRE DOWN" },
        { 1, 4, "FIRE RIGHT"  },
        { 1, 5, "FIRE LEFT" },
        { 1, 6, "WHIP" },
        { -1 }
};


/*
        RESCUE  (Thanx To Chris Hardy)
        JRT: Order is 0 -> 7
                IN0                                     IN1:                            IN2:
                0: Bomb                         0: (DIP) Free Play      0: Start1
                1: ???                          1: (DIP) Demo Mode      1:
                2: Up                           2: Fire U                       2:
                3: Down                         3: Fire D                       3:
                4: Right                        4: Fire R                       4:
                5: Left                         5: Fire L                       5:
                6: Coin (2?)            6:                                      6: Start2
                7: Coin (1?)            7:                                      7:

*/
static struct InputPort Rescueinput_ports[] =
{
        {       /* IN0 */
                0xff,
                { OSD_KEY_CONTROL, OSD_KEY_ALT, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_3, OSD_KEY_4 },
                { 0, 0, 0, 0, 0, 0, 0, 0 }
        },
        {       /* IN1 */
                0xff,
                { 0, 0, OSD_KEY_E, OSD_KEY_D, OSD_KEY_F, OSD_KEY_S, OSD_KEY_CONTROL, OSD_KEY_1 },
                { 0, 0, 0, 0, 0, 0, 0, 0 }
        },
        {       /* IN2 */
                0xff,
                { OSD_KEY_1, 0, 0, 0, 0, 0, OSD_KEY_2, 0 },
                { 0, 0, 0, 0, 0, 0, 0, 0 }
        },
        { -1 }  /* end of table */
};


static struct DSW Rescuedsw[] =
{
        { 1, 0x02, "FREE PLAY", { "NO", "YES" } },
        { 1, 0x01, "PLAYER CAN DIE", { "NO", "YES" } },
        { -1 }
};


static struct KEYSet Rescuekeys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 3, "MOVE DOWN" },
        { 0, 5, "MOVE RIGHT"  },
        { 0, 4, "MOVE LEFT" },
        { 1, 2, "FIRE UP" },
        { 1, 3, "FIRE DOWN" },
        { 1, 5, "FIRE RIGHT"  },
        { 1, 4, "FIRE LEFT" },
        { 2, 0, "BOMB" },
        { 2, 6, "FIRE2?" },
        { -1 }
};


/*
        ANTEATER
        JRT: Order is 0 -> 7
                IN0                                     IN1:                            IN2:
                0: Retract                      0: (DIP) Free Play      0: Start1
                1: Fire(?)1                     1: (DIP) Demo Mode      1:
                2: Up                           2:                                      2:
                3: Down                         3:                                      3:
                4: Right                        4:                                      4:
                5: Left                         5:                                      5:
                6: Coin (2?)            6:                                      6: Start2
                7: Coin (1?)            7:                                      7:

*/
static struct InputPort AntEaterinput_ports[] =
{
        {       /* IN0 */
                0xff,
                { OSD_KEY_CONTROL, OSD_KEY_ALT, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_3, OSD_KEY_4 },
                { OSD_JOY_FIRE1, OSD_JOY_FIRE2, OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
        },
        {       /* IN1 */
                0xfd,
                { 0, 0, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 0, 0, 0, 0 }
        },
        {       /* IN2 */
                0xff,
                { OSD_KEY_1, 0, 0, 0, 0, 0, OSD_KEY_2, 0 },
                { 0, 0, 0, 0, 0, 0, 0, 0 }
        },
        { -1 }  /* end of table */
};


static struct DSW AntEaterdsw[] =
{
        { 1, 0x02, "FREE PLAY", { "NO", "YES" } },
        { 1, 0x01, "LIVES", { "5", "3" } },
        { -1 }
};


static struct KEYSet AntEaterkeys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 3, "MOVE DOWN" },
        { 0, 5, "MOVE RIGHT"  },
        { 0, 4, "MOVE LEFT" },
        { 0, 0, "RETRACT" },
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



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* palette */
	0x00,0x17,0xC7,0xF6,0x00,0x17,0xC0,0x3F,0x00,0x07,0xC0,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC7,0x31,0x17,0x00,0x31,0xC7,0x3F,0x00,0xF6,0x07,0xF0,0x00,0x3F,0x07,0xC4
};

static unsigned char rescue_color_prom[] =
{
	/* palette */
	0x00,0xA4,0x18,0x5B,0x00,0xB6,0x07,0x36,0x00,0xDB,0xA3,0x9B,0x00,0xDC,0x27,0xAD,
	0x00,0xC0,0x2B,0x5F,0x00,0xAD,0x2F,0x85,0x00,0xE4,0x3F,0x28,0x00,0x9A,0xC0,0x27
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1789750,	/* 1.78975 Mhz?????? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			scramble_sh_interrupt,10
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	32+64, 32,	/* 32 for the characters, 64 for the stars */
	scramble_vh_convert_color_prom,

	0,
	scramble_vh_start,
	generic_vh_stop,
	scramble_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	scramble_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( scobra_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "scobra2c.bin", 0x0000, 0x1000, 0x0fe64f76 )
	ROM_LOAD( "scobra2e.bin", 0x1000, 0x1000, 0x205664e2 )
	ROM_LOAD( "scobra2f.bin", 0x2000, 0x1000, 0x59e8525e )
	ROM_LOAD( "scobra2h.bin", 0x3000, 0x1000, 0x303ac596 )
	ROM_LOAD( "scobra2j.bin", 0x4000, 0x1000, 0x156d771d )
	ROM_LOAD( "scobra2l.bin", 0x5000, 0x1000, 0xbc79c629 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "scobra5f.bin", 0x0000, 0x0800, 0x4b2a202a )
	ROM_LOAD( "scobra5h.bin", 0x0800, 0x0800, 0xaa5bbcd1 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "scobra5c.bin", 0x0000, 0x0800, 0x5b7ffd15 )
	ROM_LOAD( "scobra5d.bin", 0x0800, 0x0800, 0xc1522792 )
	ROM_LOAD( "scobra5e.bin", 0x1000, 0x0800, 0xc4b1e3e7 )
ROM_END

ROM_START( scobrak_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c", 0x0000, 0x1000, 0x71fcefba )
	ROM_LOAD( "2e", 0x1000, 0x1000, 0xd8edcd97 )
	ROM_LOAD( "2f", 0x2000, 0x1000, 0xd884517c )
	ROM_LOAD( "2h", 0x3000, 0x1000, 0x81707f54 )
	ROM_LOAD( "2j", 0x4000, 0x1000, 0xe9ac3850 )
	ROM_LOAD( "2l", 0x5000, 0x1000, 0x3b37371b )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f", 0x0000, 0x0800, 0x4b2a202a )
	ROM_LOAD( "5h", 0x0800, 0x0800, 0xaa5bbcd1 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c", 0x0000, 0x0800, 0x5b7ffd15 )
	ROM_LOAD( "5d", 0x0800, 0x0800, 0xc1522792 )
	ROM_LOAD( "5e", 0x1000, 0x0800, 0xc4b1e3e7 )
ROM_END

ROM_START( scobrab_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "vid_2c.bin",   0x0000, 0x0800, 0x9eab7bb3 )
	ROM_LOAD( "vid_2e.bin",   0x0800, 0x0800, 0x3b6585fd )
	ROM_LOAD( "vid_2f.bin",   0x1000, 0x0800, 0x3336090a )
	ROM_LOAD( "vid_2h.bin",   0x1800, 0x0800, 0xed206de8 )
	ROM_LOAD( "vid_2j_l.bin", 0x2000, 0x0800, 0xbefdffe5 )
	ROM_LOAD( "vid_2l_l.bin", 0x2800, 0x0800, 0x9aebadbb )
	ROM_LOAD( "vid_2m_l.bin", 0x3000, 0x0800, 0x2a29599d )
	ROM_LOAD( "vid_2p_l.bin", 0x3800, 0x0800, 0x06119c0b )
	ROM_LOAD( "vid_2j_u.bin", 0x4000, 0x0800, 0xf35e2d38 )
	ROM_LOAD( "vid_2l_u.bin", 0x4800, 0x0800, 0x220f5a25 )
	ROM_LOAD( "vid_2m_u.bin", 0x5000, 0x0800, 0xef190401 )
	ROM_LOAD( "vid_2p_u.bin", 0x5800, 0x0800, 0xcd60c228 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vid_5f.bin", 0x0000, 0x0800, 0x4b2a202a )
	ROM_LOAD( "vid_5h.bin", 0x0800, 0x0800, 0xaa5bbcd1 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "snd_5c.bin", 0x0000, 0x0800, 0xb5c45422 )
	ROM_LOAD( "snd_5d.bin", 0x0800, 0x0800, 0xaa50e11a )
	ROM_LOAD( "snd_5e.bin", 0x1000, 0x0800, 0xb7b4dd96 )
ROM_END

ROM_START( losttomb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c",      0x0000, 0x1000, 0x55248120 )
	ROM_LOAD( "2e",      0x1000, 0x1000, 0x8d5d9663 )
	ROM_LOAD( "2f",      0x2000, 0x1000, 0xb53e6390 )
	ROM_LOAD( "2h-easy", 0x3000, 0x1000, 0x18d580a9 )
	ROM_LOAD( "2j",      0x4000, 0x1000, 0x5d21f893 )
	ROM_LOAD( "2l",      0x5000, 0x1000, 0x030788df )
	ROM_LOAD( "2m",      0x6000, 0x1000, 0x24205f0e )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f",      0x1000, 0x0800, 0x5be70b13 )	/* we load the roms at 0x1000-0x1fff, they */
	ROM_LOAD( "5h",      0x1800, 0x0800, 0xd6e32f73 )	/* will be decrypted at 0x0000-0x0fff */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c",      0x0000, 0x0800, 0xa28e0d60 )
	ROM_LOAD( "5d",      0x0800, 0x0800, 0x70ea19ea )
ROM_END

ROM_START( anteater_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ra1-2c", 0x0000, 0x1000, 0x3a99be31 )
	ROM_LOAD( "ra1-2e", 0x1000, 0x1000, 0xb46154bb )
	ROM_LOAD( "ra1-2f", 0x2000, 0x1000, 0xd467888d )
	ROM_LOAD( "ra1-2h", 0x3000, 0x1000, 0xfbcffc91 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ra6-5f", 0x1000, 0x0800, 0x5a8a7eb6 )	/* we load the roms at 0x1000-0x1fff, they */
	ROM_LOAD( "ra6-5h", 0x1800, 0x0800, 0x491cbf24 )	/* will be decrypted at 0x0000-0x0fff */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ra4-5c", 0x0000, 0x0800, 0xa8a615f0 )
	ROM_LOAD( "ra4-5d", 0x0800, 0x0800, 0x2ff74b03 )
ROM_END

ROM_START( rescue_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "rb15acpu.bin", 0x0000, 0x1000, 0x2d7f947b )
	ROM_LOAD( "rb15bcpu.bin", 0x1000, 0x1000, 0x09fbbe11 )
	ROM_LOAD( "rb15ccpu.bin", 0x2000, 0x1000, 0x7d76d748 )
	ROM_LOAD( "rb15dcpu.bin", 0x3000, 0x1000, 0x592c8dd0 )
	ROM_LOAD( "rb15ecpu.bin", 0x4000, 0x1000, 0x9c059431 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "rb15fcpu.bin", 0x1000, 0x0800, 0xb55f561f )	/* we load the roms at 0x1000-0x1fff, they */
	ROM_LOAD( "rb15hcpu.bin", 0x1800, 0x0800, 0x7a13c447 )	/* will be decrypted at 0x0000-0x0fff */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "rb15csnd.bin", 0x0000, 0x0800, 0xfd8c78cc )
	ROM_LOAD( "rb15dsnd.bin", 0x0800, 0x0800, 0xfdbb116f )
ROM_END

ROM_START( hunchy_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "1b.bin", 0x0000, 0x1000, 0xedc7328b )
	ROM_LOAD( "2a.bin", 0x1000, 0x1000, 0x07a951d3 )
	ROM_LOAD( "3a.bin", 0x2000, 0x1000, 0x84d6fc0e )
	ROM_LOAD( "4c.bin", 0x3000, 0x1000, 0xe2a39fbd )
	ROM_LOAD( "5a.bin", 0x4000, 0x1000, 0x2f46d302 )
	ROM_LOAD( "6c.bin", 0x5000, 0x1000, 0x8d6b2637 )

	ROM_REGION(0x2000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "8a.bin",  0x0000, 0x0800, 0x7afe3b86 )
	ROM_LOAD( "9b.bin",  0x0800, 0x0800, 0x015ee94c )
	ROM_LOAD( "10b.bin", 0x1000, 0x0800, 0x72d82f86 )
	ROM_LOAD( "11a.bin", 0x1800, 0x0800, 0xf5280414 )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "5b_snd.bin", 0x0000, 0x0800, 0xd71b1c53 )
ROM_END





static int bit(int i,int n)
{
	return ((i >> n) & 1);
}

static void losttomb_decode(void)
{
	/*
	*   Code To Decode Lost Tomb by Mirko Buffoni
	*   Optimizations done by Fabio Buffoni
	*/
	int i,j;
	unsigned char *RAM;


	/* The gfx ROMs are scrambled. Decode them. They have been loaded at 0x1000, */
	/* we write them at 0x0000. */
	RAM = Machine->memory_region[1];

	for (i = 0;i < 0x1000;i++)
	{
		j = i & 0xa7f;
		j |= ( bit(i,7) ^ (bit(i,1) & ( bit(i,7) ^ bit(i,10) ))) << 8;
		j |= ( (bit(i,1) & bit(i,7)) | ((1 ^ bit(i,1)) & (bit(i,8)))) << 10;
		j |= ( (bit(i,1) & bit(i,8)) | ((1 ^ bit(i,1)) & (bit(i,10)))) << 7;
		RAM[i] = RAM[j + 0x1000];
	}
}

static void anteater_decode(void)
{
	/*
	*   Code To Decode Lost Tomb by Mirko Buffoni
	*   Optimizations done by Fabio Buffoni
	*/
	int i,j;
	unsigned char *RAM;


/*
//      Patch To Bypass The Self Test
//
//      Thanx To Chris Hardy For The Patch Code
//  To Remove The Self Test For Rescue.
//      (Also Works For Ant Eater, and Lost Tomb)
*/
Machine -> memory_region[ 0 ][ 0x008A ] = 0;
Machine -> memory_region[ 0 ][ 0x008B ] = 0;
Machine -> memory_region[ 0 ][ 0x008C ] = 0;

Machine -> memory_region[ 0 ][ 0x0091 ] = 0;
Machine -> memory_region[ 0 ][ 0x0092 ] = 0;
Machine -> memory_region[ 0 ][ 0x0093 ] = 0;

Machine -> memory_region[ 0 ][ 0x0097 ] = 0;
Machine -> memory_region[ 0 ][ 0x0098 ] = 0;
Machine -> memory_region[ 0 ][ 0x0099 ] = 0;

	/* The gfx ROMs are scrambled. Decode them. They have been loaded at 0x1000, */
	/* we write them at 0x0000. */
	RAM = Machine->memory_region[1];

	for (i = 0;i < 0x1000;i++)
	{
		j = i & 0x9bf;
		j |= ( bit(i,0) ^ bit(i,6) ^ 1) << 10;
		j |= ( bit(i,2) ^ bit(i,10) ) << 9;
		j |= ( bit(i,4) ^ bit(i,9) ^ ( bit(i,2) & bit (i,10) )) << 6;
		RAM[i] = RAM[j + 0x1000];
	}
}

static void rescue_decode(void)
{
	/*
	*   Code To Decode Lost Tomb by Mirko Buffoni
	*   Optimizations done by Fabio Buffoni
	*/
	int i,j;
	unsigned char *RAM;


/*
//      Patch To Bypass The Self Test
//
//      Thanx To Chris Hardy For The Patch Code
//  To Remove The Self Test For Rescue.
//      (Also Works For Ant Eater, and Lost Tomb)
*/
Machine -> memory_region[ 0 ][ 0x008A ] = 0;
Machine -> memory_region[ 0 ][ 0x008B ] = 0;
Machine -> memory_region[ 0 ][ 0x008C ] = 0;

Machine -> memory_region[ 0 ][ 0x0091 ] = 0;
Machine -> memory_region[ 0 ][ 0x0092 ] = 0;
Machine -> memory_region[ 0 ][ 0x0093 ] = 0;

Machine -> memory_region[ 0 ][ 0x0097 ] = 0;
Machine -> memory_region[ 0 ][ 0x0098 ] = 0;
Machine -> memory_region[ 0 ][ 0x0099 ] = 0;

	/* The gfx ROMs are scrambled. Decode them. They have been loaded at 0x1000, */
	/* we write them at 0x0000. */
	RAM = Machine->memory_region[1];

	for (i = 0;i < 0x1000;i++)
	{
		j = i & 0xa7f;
		j |= ( bit(i,3) ^ bit(i,10) ) << 7;
		j |= ( bit(i,1) ^ bit(i,7) ) << 8;
		j |= ( bit(i,0) ^ bit(i,8) ) << 10;
		RAM[i] = RAM[j + 0x1000];
	}
}



struct GameDriver scobra_driver =
{
	"Super Cobra (Stern)",
	"scobra",
	"NICOLA SALMORIA",
	&machine_driver,

	scobra_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	0, 0
};

struct GameDriver scobrak_driver =
{
	"Super Cobra (Konami)",
	"scobrak",
	"NICOLA SALMORIA",
	&machine_driver,

	scobrak_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	0, 0
};

struct GameDriver scobrab_driver =
{
	"Super Cobra (bootleg)",
	"scobrab",
	"NICOLA SALMORIA",
	&machine_driver,

	scobrab_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	0, 0
};

struct GameDriver losttomb_driver =
{
	"Lost Tomb",
	"losttomb",
	"NICOLA SALMORIA\nJAMES R. TWINE\nMIRKO BUFFONI\nFABIO BUFFONI",
	&machine_driver,

	losttomb_rom,
	losttomb_decode, 0,
	0,

	LTinput_ports, trak_ports, LTdsw, LTkeys,

	color_prom, 0, 0,
	8*13, 8*16,

	0, 0
};


static int anteater_hiload(const char *name)
{
  unsigned char *RAM = Machine->memory_region[0];
  FILE *f;

  /* Wait for machine initialization to be done. */
  if (memcmp(&RAM[0x146a], "\x01\x04\x80", 3) != 0) return 0;

  if ((f = fopen(name,"rb")) != 0)
    {
      /* Load and set hiscore table. */
      fread(&RAM[0x146a],1,6*10,f);
      fclose(f);
    }

  return 1;
}

static void anteater_hisave(const char *name)
{
  unsigned char *RAM = Machine->memory_region[0];
  FILE *f;

  if ((f = fopen(name,"wb")) != 0)
    {
      /* Write hiscore table. */
      fwrite(&RAM[0x80ef],1,6*10,f);
      fclose(f);
    }
}


struct GameDriver anteater_driver =
{
	"Ant Eater",
	"anteater",
	"JAMES R. TWINE\nCHRIS HARDY\nMIRKO BUFFONI\nFABIO BUFFONI",
	&machine_driver,

	anteater_rom,
	anteater_decode, 0,
	0,

	AntEaterinput_ports, trak_ports, AntEaterdsw, AntEaterkeys,

	color_prom, 0, 0,
	8*13, 8*16,

	anteater_hiload, anteater_hisave
};


struct GameDriver rescue_driver =
{
	"Rescue",
	"rescue",
	"JAMES R. TWINE\nCHRIS HARDY\nMIRKO BUFFONI\nFABIO BUFFONI\nALAN J MCCORMICK",
	&machine_driver,

	rescue_rom,
	rescue_decode, 0,
	0,

	Rescueinput_ports, trak_ports, Rescuedsw, Rescuekeys,

	rescue_color_prom, 0, 0,
	8*13, 8*16,

	0, 0
};


struct GameDriver hunchy_driver =
{
        "Hunchback",
        "hunchy",
        "JAMES R. TWINE\nCHRIS HARDY",
        &machine_driver,

        hunchy_rom,
        0, 0,
        0,

        LTinput_ports, trak_ports, LTdsw, LTkeys,

        color_prom, 0, 0,
        8*13, 8*16,

        0, 0
};
