#include "driver.h"
#include "vidhrdw/generic.h"


int  snk_vh_start(void);
void snk_vh_stop(void);
void snk_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void snk_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int  snk_hrdwmem_r(int offset);
int  snk_sharedram_r(int offset);
void snk_hrdwmem_w(int offset, int data);
void snk_sharedram_w(int offset, int data);
int  snk_read_f800(int offset);
void snk_write_f800(int offset, int data);
int  snk_soundcommand_r(int offset);

void snk_init_machine(void);
void ikarius_init_data(void);
void ikarijp_init_data(void);
void ikarijpb_init_data(void);

extern unsigned char *snk_hrdwmem;
extern unsigned char *snk_sharedram;



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, snk_hrdwmem_r },
	{ 0xd000, 0xffff, snk_sharedram_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, snk_hrdwmem_w, &snk_hrdwmem },
	{ 0xd000, 0xffff, snk_sharedram_w, &snk_sharedram },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_sub[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, snk_hrdwmem_r },
	{ 0xd000, 0xffff, snk_sharedram_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_sub[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, snk_hrdwmem_w },
	{ 0xd000, 0xffff, snk_sharedram_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_snd[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xe000, 0xe000, snk_soundcommand_r },
	{ 0xe800, 0xe800, YM3526_status_port_0_r },
	{ 0xf000, 0xf000, MRA_NOP },		/* YM3526 #2 status port */
	{ 0xf800, 0xf800, snk_read_f800 },	/* IRQ CTRL FOR Y8950  NOT EMULATED */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_snd[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xf000, 0xf000, MWA_NOP },		/* YM3526 #2 control port */
	{ 0xf400, 0xf400, MWA_NOP },		/* YM3526 #2 write port   */
	{ 0xe800, 0xe800, YM3526_control_port_0_w },
	{ 0xec00, 0xec00, YM3526_write_port_0_w },
	{ 0xf800, 0xf800, snk_write_f800 },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( ikarius_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) 	/* sound related ??? */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* DAIAL CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* DAIAL CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Players buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch 1 */
	/* WHEN THIS BIT IS SET IN IKARI-GUYS CAN KILL EACH OTHER */
	/* IN VICTORY ROAD : GUYS CAN WALK ALSO EVERYWHERE */
	PORT_DIPNAME( 0x01, 0x01, "Special bit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Every" )
	PORT_DIPSETTING(    0x00, "Only" )
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin /1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START	/* Dip switch 2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Demo Sounds On" )
	PORT_DIPSETTING(    0x04, "Freeze Game" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50000 100000" )
	PORT_DIPSETTING(    0x20, "60000 120000" )
	PORT_DIPSETTING(    0x10, "100000 200000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "No")
	PORT_DIPSETTING(    0x00, "Yes")

INPUT_PORTS_END


INPUT_PORTS_START( ikarijp_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,IPT_UNKNOWN ) 	/* sound related ? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  	/* tilt related ? (it resets 1 cpu) */

	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* DAIAL CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* DAIAL CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Players buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch 1 */
	/* WHEN THIS BIT IS SET IN IKARI-GUYS CAN KILL EACH OTHER */
	/* IN VICTORY ROAD : GUYS CAN WALK ALSO EVERYWHERE */
	PORT_DIPNAME( 0x01, 0x01, "Special bit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Every" )
	PORT_DIPSETTING(    0x00, "Only" )
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin /1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START	/* Dip switch 2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Demo Sounds On" )
	PORT_DIPSETTING(    0x04, "Freeze Game" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50000 100000" )
	PORT_DIPSETTING(    0x20, "60000 120000" )
	PORT_DIPSETTING(    0x10, "100000 200000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")

INPUT_PORTS_END


INPUT_PORTS_START( victroad_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) 	/* sound related ??? */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* DAIAL CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* DAIAL CONTROL 4 bits */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Players buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch 1 */
	/* WHEN THIS BIT IS SET IN IKARI-GUYS CAN KILL EACH OTHER */
	/* IN VICTORY ROAD : GUYS CAN WALK ALSO EVERYWHERE */
	PORT_DIPNAME( 0x01, 0x01, "Special bit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Every" )
	PORT_DIPSETTING(    0x00, "Only" )
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin /1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START	/* Dip switch 2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Demo Sounds On" )
	PORT_DIPSETTING(    0x04, "Freeze Game" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50000 100000" )
	PORT_DIPSETTING(    0x20, "60000 120000" )
	PORT_DIPSETTING(    0x10, "100000 200000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPSETTING(    0x40, "No" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "On")
	PORT_DIPSETTING(    0x00, "Off")

INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
	  32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8 	/* every char takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	3,	/* 3 bits per pixel */
	{ 0x10000*8, 0x8000*8, 0 },
	{    7,    6,     5,     4,     3,     2,     1,     0,
	    15,   14,    13,    12,    11,    10,     9,     8 },
	{ 0*16, 1*16,  2*16,  3*16,  4*16,  5*16,  6*16,  7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8 	/* every char takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout2 =
{
	32,32,	/* 32*32 sprites */
	512,	/* 512 sprites */
	3,	/* 3 bits per pixel */
	{ 0x20000*8, 0x10000*8, 0 },
	{     7,      6,    5,     4,     3,     2,     1,     0,
	     15,     14,   13,    12,    11,    10,     9,     8,
	 2*8+7 , 2*8+6, 2*8+5, 2*8+4, 2*8+3, 2*8+2, 2*8+1, 2*8+0,
	 2*8+15,2*8+14,2*8+13,2*8+12,2*8+11,2*8+10, 2*8+9, 2*8+8 },
	{ 0*32,   1*32,  2*32,  3*32,  4*32,  5*32,  6*32,  7*32,
	  8*32,   9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
	  16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32,
	  24*32, 25*32, 26*32, 27*32, 28*32, 29*32, 30*32, 31*32 },
	128*8 	/* every char takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,          0*16,  1 },
	{ 1, 0x04000, &tilelayout,     	    1*16, 16 },
	{ 1, 0x24000, &spritelayout1,      17*16, 16 },
	{ 1, 0x3C000, &spritelayout2, 17*16+16*8, 16 },
	{ -1 } /* end of array */
};

static struct YM3526interface ym3526_interface =
{
	1,		/* 1 chip (no more supported) but the games supports 2 */
	4000000,	/* 4 MHz ? (hand tuned) */
	{ 255 }		/* (not supported) */
};

static struct MachineDriver machine_driver =
{
	{
		{
			CPU_Z80,
			4000000,		/* ??? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,		/* ??? */
			2,
			readmem_sub,writemem_sub,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,		/* ??? */
			3,
			readmem_snd,writemem_snd,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION, 	/* frames per second, vblank duration */
	30,
	snk_init_machine,
	36*8, 28*8,
	{ 0*8, 36*8-1, 1*8, 28*8-1 },
	gfxdecodeinfo,
	512,17*16+32*8,
	snk_vh_convert_color_prom,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	snk_vh_start,
	snk_vh_stop,
	snk_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};


ROM_START( ikarius_rom )

	ROM_REGION(0x10000)			 	  /* 64k for cpuA code */
	ROM_LOAD( "1.rom",  0x00000, 0x10000, 0x52a8b2dd )

	ROM_REGION_DISPOSE(0x6c000)

	ROM_LOAD( "7.rom",  0x00000, 0x4000, 0xa7eb4917 ) /* characters */

	ROM_LOAD( "17.rom", 0x04000, 0x8000, 0xe0dba976 ) /* tiles */
	ROM_LOAD( "18.rom", 0x0c000, 0x8000, 0x24947d5f )
	ROM_LOAD( "19.rom", 0x14000, 0x8000, 0x9ee59e91 )
	ROM_LOAD( "20.rom", 0x1c000, 0x8000, 0x5da7ec1a )

	ROM_LOAD( "8.rom",  0x24000, 0x8000, 0x9827c14a ) /* sprites 16x16 */
	ROM_LOAD( "9.rom",  0x2c000, 0x8000, 0x545c790c )
	ROM_LOAD( "10.rom", 0x34000, 0x8000, 0xec9ba07e )

	ROM_LOAD( "11.rom", 0x3c000, 0x8000, 0x5c75ea8f ) /* sprites 32x32 */
	ROM_LOAD( "14.rom", 0x44000, 0x8000, 0x3293fde4 )
	ROM_LOAD( "12.rom", 0x4c000, 0x8000, 0x95138498 )
	ROM_LOAD( "15.rom", 0x54000, 0x8000, 0x65a61c99 )
	ROM_LOAD( "13.rom", 0x5c000, 0x8000, 0x315383d7 )
	ROM_LOAD( "16.rom", 0x64000, 0x8000, 0xe9b03e07 )

	ROM_REGION(0x10000)				  /* 64k for cpuB code */
	ROM_LOAD( "2.rom",  0x0000, 0x10000, 0x45364d55 )

	ROM_REGION(0x10000)				  /* 64k for sound code */
	ROM_LOAD( "3.rom",  0x0000, 0x10000, 0x56a26699 )

	ROM_REGION(0xc00) 				  /* color proms */
	ROM_LOAD( "7122er.prm", 0x000, 0x400, 0xb9bf2c2c )
	ROM_LOAD( "7122eg.prm", 0x400, 0x400, 0x0703a770 )
	ROM_LOAD( "7122eb.prm", 0x800, 0x400, 0x0a11cdde )

ROM_END


ROM_START( ikarijp_rom )

	ROM_REGION(0x10000)					/* 64k for cpuA code */
	ROM_LOAD( "up03_l4.rom",  0x00000, 0x4000, 0xcde006be )
	ROM_LOAD( "up03_k4.rom",  0x04000, 0x8000, 0x26948850 )

	ROM_REGION_DISPOSE( 0x6c000 )
	ROM_LOAD( "7.rom",  0x00000, 0x4000, 0xa7eb4917 ) /* characters */

	ROM_LOAD( "17.rom", 0x04000, 0x8000, 0xe0dba976 ) /* tiles */
	ROM_LOAD( "18.rom", 0x0c000, 0x8000, 0x24947d5f )
	ROM_LOAD( "ik19",   0x14000, 0x8000, 0x566242ec )
	ROM_LOAD( "20.rom", 0x1c000, 0x8000, 0x5da7ec1a )

	ROM_LOAD( "ik8",    0x24000, 0x8000, 0x75d796d0 ) /* sprites 16x16 */
	ROM_LOAD( "ik9",    0x2c000, 0x8000, 0x2c34903b )
	ROM_LOAD( "ik10",   0x34000, 0x8000, 0xda9ccc94 )

	ROM_LOAD( "11.rom", 0x3c000, 0x8000, 0x5c75ea8f ) /* sprites 32x32 */
	ROM_LOAD( "14.rom", 0x44000, 0x8000, 0x3293fde4 )
	ROM_LOAD( "12.rom", 0x4c000, 0x8000, 0x95138498 )
	ROM_LOAD( "15.rom", 0x54000, 0x8000, 0x65a61c99 )
	ROM_LOAD( "13.rom", 0x5c000, 0x8000, 0x315383d7 )
	ROM_LOAD( "16.rom", 0x64000, 0x8000, 0xe9b03e07 )

	ROM_REGION(0x10000)					/* 64k for cpuB code */
	ROM_LOAD( "ik3",    0x00000, 0x4000, 0x9bb385f8 )
	ROM_LOAD( "ik4",    0x04000, 0x8000, 0x3a144bca )

	ROM_REGION(0x10000)					/* 64k for sound code */
	ROM_LOAD( "ik5",    0x00000, 0x4000, 0x863448fa )
	ROM_LOAD( "ik6",    0x04000, 0x8000, 0x9b16aa57 )

	ROM_REGION(0xc00) 				  	/* color proms */
	ROM_LOAD( "7122er.prm", 0x000, 0x400, 0xb9bf2c2c )
	ROM_LOAD( "7122eg.prm", 0x400, 0x400, 0x0703a770 )
	ROM_LOAD( "7122eb.prm", 0x800, 0x400, 0x0a11cdde )

ROM_END


ROM_START( ikarijpb_rom )

	ROM_REGION(0x10000)				/* 64k for cpuA code */
	ROM_LOAD( "ik1",          0x00000, 0x4000, 0x2ef87dce )
	ROM_LOAD( "up03_k4.rom",  0x04000, 0x8000, 0x26948850 )

	ROM_REGION_DISPOSE(0x6c000)

	ROM_LOAD( "ik7",    0x00000, 0x4000, 0x9e88f536 )	/* characters */

	ROM_LOAD( "17.rom", 0x04000, 0x8000, 0xe0dba976 ) /* tiles */
	ROM_LOAD( "18.rom", 0x0c000, 0x8000, 0x24947d5f )
	ROM_LOAD( "ik19",   0x14000, 0x8000, 0x566242ec )
	ROM_LOAD( "20.rom", 0x1c000, 0x8000, 0x5da7ec1a )

	ROM_LOAD( "ik8",    0x24000, 0x8000, 0x75d796d0 ) /* sprites 16x16 */
	ROM_LOAD( "ik9",    0x2c000, 0x8000, 0x2c34903b )
	ROM_LOAD( "ik10",   0x34000, 0x8000, 0xda9ccc94 )

	ROM_LOAD( "11.rom", 0x3c000, 0x8000, 0x5c75ea8f ) /* sprites 32x32 */
	ROM_LOAD( "14.rom", 0x44000, 0x8000, 0x3293fde4 )
	ROM_LOAD( "12.rom", 0x4c000, 0x8000, 0x95138498 )
	ROM_LOAD( "15.rom", 0x54000, 0x8000, 0x65a61c99 )
	ROM_LOAD( "13.rom", 0x5c000, 0x8000, 0x315383d7 )
	ROM_LOAD( "16.rom", 0x64000, 0x8000, 0xe9b03e07 )

	ROM_REGION(0x10000)				/* 64k for cpuB code */
	ROM_LOAD( "ik3",    0x0000, 0x4000, 0x9bb385f8 )
	ROM_LOAD( "ik4",    0x4000, 0x8000, 0x3a144bca )

	ROM_REGION(0x10000)				/* 64k for sound code */
	ROM_LOAD( "ik5",    0x0000, 0x4000, 0x863448fa )
	ROM_LOAD( "ik6",    0x4000, 0x8000, 0x9b16aa57 )

	ROM_REGION(0xc00) 				/* color proms */
	ROM_LOAD( "7122er.prm", 0x000, 0x400, 0xb9bf2c2c )
	ROM_LOAD( "7122eg.prm", 0x400, 0x400, 0x0703a770 )
	ROM_LOAD( "7122eb.prm", 0x800, 0x400, 0x0a11cdde )
ROM_END



ROM_START( victroad_rom )

	ROM_REGION(0x10000)				/* 64k for cpuA code */
	ROM_LOAD( "p1",  0x00000, 0x10000, 0xe334acef )

	ROM_REGION_DISPOSE(0x6c000)			/* characters */

	ROM_LOAD( "p7",  0x00000, 0x4000, 0x2b6ed95b )

	ROM_LOAD( "p17", 0x04000, 0x8000, 0x19d4518c )  /* tiles */
	ROM_LOAD( "p18", 0x0c000, 0x8000, 0xd818be43 )
	ROM_LOAD( "p19", 0x14000, 0x8000, 0xd64e0f89 )
	ROM_LOAD( "p20", 0x1c000, 0x8000, 0xedba0f31 )

	ROM_LOAD( "p8",  0x24000, 0x8000, 0xdf7f252a ) /* sprites 16x16 */
	ROM_LOAD( "p9",  0x2c000, 0x8000, 0x9897bc05 )
	ROM_LOAD( "p10", 0x34000, 0x8000, 0xecd3c0ea )

	ROM_LOAD( "p11", 0x3c000, 0x8000, 0x668b25a4 ) /* sprites 32x32 */
	ROM_LOAD( "p14", 0x44000, 0x8000, 0xa7031d4a )
	ROM_LOAD( "p12", 0x4c000, 0x8000, 0xf44e95fa )
	ROM_LOAD( "p15", 0x54000, 0x8000, 0x120d2450 )
	ROM_LOAD( "p13", 0x5c000, 0x8000, 0x980ca3d8 )
	ROM_LOAD( "p16", 0x64000, 0x8000, 0x9f820e8a )

	ROM_REGION(0x10000)				/* 64k for cpuB code */
	ROM_LOAD( "p2",  0x0000, 0x10000, 0x907fac83 )

	ROM_REGION(0x10000)				/* 64k for sound code */
	ROM_LOAD( "p3",  0x0000, 0x10000, 0xbac745f6 )

	ROM_REGION(0xc00) 				/* color proms */
	ROM_LOAD( "mb7122e.1k", 0x000, 0x400, 0x491ab831 )
	ROM_LOAD( "mb7122e.2l", 0x400, 0x400, 0x8feca424 )
	ROM_LOAD( "mb7122e.1l", 0x800, 0x400, 0x220076ca )

	ROM_REGION(0x20000)				/* ADPCM Samples */
	ROM_LOAD( "p4",  0x00000, 0x10000, 0xe10fb8cc )
	ROM_LOAD( "p5",  0x10000, 0x10000, 0x93e5f110 )

ROM_END


ROM_START( dogosoke_rom )

	ROM_REGION(0x10000)					/* 64k for cpuA code */
	ROM_LOAD( "up03_p4.rom", 0x00000,0x10000, 0x37867ad2 )

	ROM_REGION_DISPOSE( 0x6c000 )				/* characters */
	ROM_LOAD( "up02_b3.rom", 0x00000, 0x4000, 0x51a4ec83 )

	ROM_LOAD( "p17",         0x04000, 0x8000, 0x19d4518c ) /* tiles */
	ROM_LOAD( "p18",         0x0c000, 0x8000, 0xd818be43 )
	ROM_LOAD( "p19",         0x14000, 0x8000, 0xd64e0f89 )
	ROM_LOAD( "p20",         0x1c000, 0x8000, 0xedba0f31 )

	ROM_LOAD( "up02_d3.rom", 0x24000, 0x8000, 0xd43044f8 ) /* sprites 16x16 */
	ROM_LOAD( "up02_e3.rom", 0x2c000, 0x8000, 0x365ed2d8 )
	ROM_LOAD( "up02_g3.rom", 0x34000, 0x8000, 0x92579bf3 )

	ROM_LOAD( "p11",         0x3c000, 0x8000, 0x668b25a4 ) /* sprites 32x32 */
	ROM_LOAD( "p14",         0x44000, 0x8000, 0xa7031d4a )
	ROM_LOAD( "p12",         0x4c000, 0x8000, 0xf44e95fa )
	ROM_LOAD( "p15",         0x54000, 0x8000, 0x120d2450 )
	ROM_LOAD( "p13",         0x5c000, 0x8000, 0x980ca3d8 )
	ROM_LOAD( "p16",         0x64000, 0x8000, 0x9f820e8a )

	ROM_REGION(0x10000)					/* 64k for cpuB code */
	ROM_LOAD( "p2",          0x00000, 0x10000, 0x907fac83 )

	ROM_REGION(0x10000)					/* 64k for sound code */
	ROM_LOAD( "up03_k7.rom", 0x00000, 0x10000, 0x173fa571 )

	ROM_REGION(0xC00)					/* color PROM */
	ROM_LOAD( "up03_k1.rom", 0x000, 0x400, 0x10a2ce2b )
	ROM_LOAD( "up03_l2.rom", 0x400, 0x400, 0x99dc9792 )
	ROM_LOAD( "up03_l1.rom", 0x800, 0x400, 0xe7213160 )

	ROM_REGION(0x20000)					/* ADPCM Samples */
	ROM_LOAD( "up03_f5.rom", 0x00000, 0x10000, 0x5b43fe9f )
	ROM_LOAD( "up03_g5.rom", 0x10000, 0x10000, 0xaae30cd6 )

ROM_END


static int ikari_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (memcmp(&RAM[0xfc5f],"\x00\x30\x00",3) == 0)
	{
		void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
		if (f)
		{
			osd_fread(f,&RAM[0xff0b], 0x50);
			osd_fclose(f);

			/* Copy the high score to the work ram as well */

			RAM[0xfc5f] = RAM[0xff0b];
			RAM[0xfc60] = RAM[0xff0c];
			RAM[0xfc61] = RAM[0xff0d];

		}
		return 1;
	}
	return 0;  /* we can't load the hi scores yet */
}

static int victroad_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (memcmp(&RAM[0xfc5f],"\x00\x30\x00",3) == 0)
	{
		void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
		if (f)
		{
			osd_fread(f,&RAM[0xff2c], 0x50);
			osd_fclose(f);

			/* Copy the high score to the work ram as well */

			RAM[0xfc5f] = RAM[0xff2c];
			RAM[0xfc60] = RAM[0xff2d];
			RAM[0xfc61] = RAM[0xff2e];

		}
		return 1;
	}
	return 0;  /* we can't load the hi scores yet */
}

static void ikari_hisave(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */

	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);

	/* The game clears hi score table after reset, */
	/* so we must be sure that hi score table was already loaded */

	if (memcmp(&RAM[0xfc2a],"\x48\x49",2) == 0)
	{
		if (f)
		{
			osd_fwrite(f,&RAM[0xff0b],0x50);
			osd_fclose(f);
		}
	}
}

static void victroad_hisave(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */

	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);

	/* The game clears hi score table after reset, */
	/* so we must be sure that hi score table was already loaded */

	if (memcmp(&RAM[0xfc2a],"\x48\x49",2) == 0)
	{
		if (f)
		{
			osd_fwrite(f,&RAM[0xff2c],0x50);
			osd_fclose(f);
		}
	}
}

struct GameDriver ikari_driver =
{
	__FILE__,
	0,
	"ikari",
	"Ikari Warriors (US)",
	"1986",
	"SNK",
	"Jarek Parchanski (MAME driver)\n",
	0,
	&machine_driver,
	0,
	ikarius_rom,
	ikarius_init_data,0,
	0,
	0, /* sound prom */

	ikarius_input_ports,

	PROM_MEMORY_REGION(4), 0, 0,
	ORIENTATION_ROTATE_270,
	ikari_hiload,ikari_hisave
};

struct GameDriver ikarijp_driver =
{
	__FILE__,
	&ikari_driver,
	"ikarijp",
	"Ikari Warriors (Japan)",
	"1986",
	"SNK",
	"Jarek Parchanski (MAME driver)\n",
	0,
	&machine_driver,
	0,
	ikarijp_rom,
	ikarijp_init_data,0,
	0,
	0, /* sound prom */

	ikarijp_input_ports,

	PROM_MEMORY_REGION(4), 0, 0,
	ORIENTATION_ROTATE_270,
	ikari_hiload,ikari_hisave
};

struct GameDriver ikarijpb_driver =
{
	__FILE__,
	&ikari_driver,
	"ikarijpb",
	"Ikari Warriors (Japan bootleg)",
	"1986",
	"SNK",
	"Jarek Parchanski (MAME driver)\n",
	0,
	&machine_driver,
	0,
	ikarijpb_rom,
	ikarijpb_init_data,0,
	0,
	0, /* sound prom */

	ikarijp_input_ports,

	PROM_MEMORY_REGION(4), 0, 0,
	ORIENTATION_ROTATE_270,
	ikari_hiload,ikari_hisave
};


struct GameDriver victroad_driver =
{
	__FILE__,
	0,
	"victroad",
	"Victory Road",
	"1986",
	"SNK",
	"Jarek Parchanski (MAME driver)\n",
	0,
	&machine_driver,
	0,
	victroad_rom,
	ikarius_init_data,0,
	0,
	0, /* sound prom */

	victroad_input_ports,

	PROM_MEMORY_REGION(4), 0, 0,
	ORIENTATION_ROTATE_270,
	victroad_hiload,victroad_hisave
};

struct GameDriver dogosoke_driver =
{
	__FILE__,
	&victroad_driver,
	"dogosoke",
	"Dogo Soken",
	"1986",
	"SNK",
	"Jarek Parchanski (MAME driver)\n",
	0,
	&machine_driver,
	0,
	dogosoke_rom,
	ikarius_init_data,0,
	0,
	0, /* sound prom */

	victroad_input_ports,

	PROM_MEMORY_REGION(4), 0, 0,
	ORIENTATION_ROTATE_270,
	victroad_hiload,victroad_hisave
};

