#include "driver.h"
#include "vidhrdw/system8.h"


int  system8_bank = 0x00;


void system8_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (((data & 0x0c)>>2) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);

	system8_bank=data;
}

void wbml_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (((data & 0x0c)>>2) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);
	/* TODO: the memory system doesn't yet support bank switching on an encrypted */
	/* ROM, so we have to copy the data manually */
	memcpy(&ROM[0x8000],&RAM[bankaddress+0x20000],0x4000);

	system8_bank=data;
}

int system8_bankswitch_r(int offset)
{
	return(system8_bank);
}

void system8_soundport_w(int offset, int data)
{
	soundlatch_w(0,data);
	cpu_cause_interrupt(1,Z80_NMI_INT);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xffff, MRA_RAM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &system8_spriteram },
	{ 0xd800, 0xdfff, system8_paletteram_w, &paletteram },
	{ 0xe000, 0xe7ff, system8_backgroundram_w, &system8_backgroundram, &system8_backgroundram_size },
	{ 0xe800, 0xeeff, MWA_RAM, &system8_videoram, &system8_videoram_size },
	{ 0xefbd, 0xefbd, MWA_RAM, &system8_scroll_y },
	{ 0xeffc, 0xeffd, MWA_RAM, &system8_scroll_x },
	{ 0xf020, 0xf03f, MWA_RAM, &system8_background_collisionram },
	{ 0xf800, 0xfbff, MWA_RAM, &system8_sprites_collisionram },
	{ 0xc000, 0xffff, MWA_RAM },
	{ -1 } /* end of table */
};

static struct MemoryReadAddress wbml_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress wbml_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd1ff, MWA_RAM, &system8_spriteram },
	{ 0xd800, 0xddff, system8_paletteram_w, &paletteram },
	{ 0xe000, 0xefff, wbml_paged_videoram_w },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress choplift_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &system8_spriteram },
	{ 0xd800, 0xdfff, system8_paletteram_w, &paletteram },
	{ 0xe7c0, 0xe7ff, choplifter_scroll_x_w, &system8_scrollx_ram },
	{ 0xe000, 0xe7ff, system8_videoram_w, &system8_videoram, &system8_videoram_size },
	{ 0xe800, 0xeeff, system8_backgroundram_w, &system8_backgroundram, &system8_backgroundram_size },
	{ 0xf020, 0xf03f, MWA_RAM, &system8_background_collisionram },
	{ 0xf800, 0xfbff, MWA_RAM, &system8_sprites_collisionram },
	{ 0xc000, 0xfbff, MWA_RAM },
	{ -1 } /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x0000, 0x0000, input_port_0_r },	/* joy1 */
	{ 0x0004, 0x0004, input_port_1_r },	/* joy2 */
	{ 0x0008, 0x0008, input_port_2_r },	/* coin,start */
	{ 0x000c, 0x000c, input_port_3_r },	/* DIP2 */
	{ 0x000d, 0x000d, input_port_4_r },	/* DIP1 (Up'n Down only) */
	{ 0x0010, 0x0010, input_port_4_r },	/* DIP1 */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x0014, 0x0014, system8_soundport_w }, 	/* sound commands */
/*
	{ 0x0015, 0x0015, UNKNOWN },
	{ 0x0017, 0x0017, UNKNOWN },
*/
	{ -1 }	/* end of table */
};

static struct IOReadPort pitfall2_readport[] =
{
	{ 0x0000, 0x0000, input_port_0_r },	/* joy1 */
	{ 0x0004, 0x0004, input_port_1_r },	/* joy2 */
	{ 0x0008, 0x0008, input_port_2_r },	/* coin,start */
	{ 0x000c, 0x000c, input_port_3_r },	/* DIP2 */
	{ 0x0010, 0x0010, input_port_4_r },	/* DIP1 */
/*	{ 0x0019, 0x0019, pitfall2_unknownport_r },  */
	{ -1 }	/* end of table */
};

static struct IOWritePort pitfall2_writeport[] =
{
	{ 0x0018, 0x0018, system8_soundport_w }, 	/* sound commands */
/*	{ 0x0019, 0x0019, pitfall2_unknownport_w },	*/
	{ -1 }	/* end of table */
};

static struct IOReadPort wbml_readport[] =
{
	{ 0x0000, 0x0000, input_port_0_r },	/* joy1 */
	{ 0x0004, 0x0004, input_port_1_r },	/* joy2 */
	{ 0x0008, 0x0008, input_port_2_r },	/* coin,start */
	{ 0x000c, 0x000c, input_port_3_r },	/* DIP2 */
	{ 0x000d, 0x000d, input_port_4_r },	/* DIP1 */
	{ 0x0015, 0x0015, system8_bankswitch_r },
	{ 0x0016, 0x0016, wbml_bg_bankselect_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort wbml_writeport[] =
{
	{ 0x0014, 0x0014, system8_soundport_w },	/* sound commands */
	{ 0x0015, 0x0015, wbml_bankswitch_w },
	{ 0x0016, 0x0016, wbml_bg_bankselect_w },
	{ -1 }	/* end of table */
};

static struct IOReadPort choplift_readport[] =
{
	{ 0x0000, 0x0000, input_port_0_r },	/* joy1 */
	{ 0x0004, 0x0004, input_port_1_r },	/* joy2 */
	{ 0x0008, 0x0008, input_port_2_r },	/* coin,start */
	{ 0x000c, 0x000c, input_port_3_r },	/* DIP2 */
	{ 0x000d, 0x000d, input_port_4_r },	/* DIP1 */
	{ 0x0015, 0x0015, system8_bankswitch_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort choplift_writeport[] =
{
	{ 0x0014, 0x0014, system8_soundport_w },	/* sound commands */
	{ 0x0015, 0x0015, system8_bankswitch_w },
//	{ 0x0017, 0x0017, choplifter_unknownport_w },	// ?
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa003, SN76496_0_w },	/* Choplifter writes to the four addresses */
	{ 0xc000, 0xc003, SN76496_1_w },	/* in sequence */
	{ -1 } /* end of table */
};



INPUT_PORTS_START( wbdeluxe_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Has to be 0 otherwise the game resets */
												/* if you die after level 1. */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* down - unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* up - unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* down - unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* up - unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "A Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4/1" )
	PORT_DIPSETTING(    0x08, "3/1" )
	PORT_DIPSETTING(    0x09, "2/1" )
	PORT_DIPSETTING(    0x05, "2/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x04, "2/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x0f, "1/1" )
	PORT_DIPSETTING(    0x03, "1/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x02, "1/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x01, "1/1 + Bonus each 2" )
	PORT_DIPSETTING(    0x06, "2/3" )
	PORT_DIPSETTING(    0x0e, "1/2" )
	PORT_DIPSETTING(    0x0d, "1/3" )
	PORT_DIPSETTING(    0x0c, "1/4" )
	PORT_DIPSETTING(    0x0b, "1/5" )
	PORT_DIPSETTING(    0x0a, "1/6" )
/*	PORT_DIPSETTING(    0x00, "1/1" ) */
	PORT_DIPNAME( 0xf0, 0xf0, "B Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4/1" )
	PORT_DIPSETTING(    0x80, "3/1" )
	PORT_DIPSETTING(    0x90, "2/1" )
	PORT_DIPSETTING(    0x50, "2/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x40, "2/1 + Bonus each 4" )
	PORT_DIPSETTING(    0xf0, "1/1" )
	PORT_DIPSETTING(    0x30, "1/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x20, "1/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x10, "1/1 + Bonus each 2" )
	PORT_DIPSETTING(    0x60, "2/3" )
	PORT_DIPSETTING(    0xe0, "1/2" )
	PORT_DIPSETTING(    0xd0, "1/3" )
	PORT_DIPSETTING(    0xc0, "1/4" )
	PORT_DIPSETTING(    0xb0, "1/5" )
	PORT_DIPSETTING(    0xa0, "1/6" )
/*	PORT_DIPSETTING(    0x00, "1/1" ) */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x10, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "30000 100000 170000" )
	PORT_DIPSETTING(    0x00, "30000 120000 210000" )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Energy Consumption", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x80, "Fast" )
INPUT_PORTS_END

INPUT_PORTS_START( wbml_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x08, "5" )
//	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Test Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "A Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4/1" )
	PORT_DIPSETTING(    0x08, "3/1" )
	PORT_DIPSETTING(    0x09, "2/1" )
	PORT_DIPSETTING(    0x05, "2/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x04, "2/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x0f, "1/1" )
	PORT_DIPSETTING(    0x03, "1/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x02, "1/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x01, "1/1 + Bonus each 2" )
	PORT_DIPSETTING(    0x06, "2/3" )
	PORT_DIPSETTING(    0x0e, "1/2" )
	PORT_DIPSETTING(    0x0d, "1/3" )
	PORT_DIPSETTING(    0x0c, "1/4" )
	PORT_DIPSETTING(    0x0b, "1/5" )
	PORT_DIPSETTING(    0x0a, "1/6" )
/*	PORT_DIPSETTING(    0x00, "1/1" ) */
	PORT_DIPNAME( 0xf0, 0xf0, "B Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4/1" )
	PORT_DIPSETTING(    0x80, "3/1" )
	PORT_DIPSETTING(    0x90, "2/1" )
	PORT_DIPSETTING(    0x50, "2/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x40, "2/1 + Bonus each 4" )
	PORT_DIPSETTING(    0xf0, "1/1" )
	PORT_DIPSETTING(    0x30, "1/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x20, "1/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x10, "1/1 + Bonus each 2" )
	PORT_DIPSETTING(    0x60, "2/3" )
	PORT_DIPSETTING(    0xe0, "1/2" )
	PORT_DIPSETTING(    0xd0, "1/3" )
	PORT_DIPSETTING(    0xc0, "1/4" )
	PORT_DIPSETTING(    0xb0, "1/5" )
	PORT_DIPSETTING(    0xa0, "1/6" )
/*	PORT_DIPSETTING(    0x00, "1/1" ) */
INPUT_PORTS_END

INPUT_PORTS_START( upndown_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "A Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4/1" )
	PORT_DIPSETTING(    0x08, "3/1" )
	PORT_DIPSETTING(    0x09, "2/1" )
	PORT_DIPSETTING(    0x05, "2/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x04, "2/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x0f, "1/1" )
	PORT_DIPSETTING(    0x03, "1/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x02, "1/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x01, "1/1 + Bonus each 2" )
	PORT_DIPSETTING(    0x06, "2/3" )
	PORT_DIPSETTING(    0x0e, "1/2" )
	PORT_DIPSETTING(    0x0d, "1/3" )
	PORT_DIPSETTING(    0x0c, "1/4" )
	PORT_DIPSETTING(    0x0b, "1/5" )
	PORT_DIPSETTING(    0x0a, "1/6" )
/*	PORT_DIPSETTING(    0x00, "1/1" ) */
	PORT_DIPNAME( 0xf0, 0xf0, "B Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4/1" )
	PORT_DIPSETTING(    0x80, "3/1" )
	PORT_DIPSETTING(    0x90, "2/1" )
	PORT_DIPSETTING(    0x50, "2/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x40, "2/1 + Bonus each 4" )
	PORT_DIPSETTING(    0xf0, "1/1" )
	PORT_DIPSETTING(    0x30, "1/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x20, "1/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x10, "1/1 + Bonus each 2" )
	PORT_DIPSETTING(    0x60, "2/3" )
	PORT_DIPSETTING(    0xe0, "1/2" )
	PORT_DIPSETTING(    0xd0, "1/3" )
	PORT_DIPSETTING(    0xc0, "1/4" )
	PORT_DIPSETTING(    0xb0, "1/5" )
	PORT_DIPSETTING(    0xa0, "1/6" )
/*	PORT_DIPSETTING(    0x00, "1/1" ) */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x06, 0x06, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x38, 0x38, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "10000" )
	PORT_DIPSETTING(    0x30, "20000" )
	PORT_DIPSETTING(    0x28, "30000" )
	PORT_DIPSETTING(    0x20, "40000" )
	PORT_DIPSETTING(    0x18, "50000" )
	PORT_DIPSETTING(    0x10, "60000" )
	PORT_DIPSETTING(    0x08, "70000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( pitfall2_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "A Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4/1" )
	PORT_DIPSETTING(    0x08, "3/1" )
	PORT_DIPSETTING(    0x09, "2/1" )
	PORT_DIPSETTING(    0x05, "2/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x04, "2/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x0f, "1/1" )
	PORT_DIPSETTING(    0x03, "1/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x02, "1/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x01, "1/1 + Bonus each 2" )
	PORT_DIPSETTING(    0x06, "2/3" )
	PORT_DIPSETTING(    0x0e, "1/2" )
	PORT_DIPSETTING(    0x0d, "1/3" )
	PORT_DIPSETTING(    0x0c, "1/4" )
	PORT_DIPSETTING(    0x0b, "1/5" )
	PORT_DIPSETTING(    0x0a, "1/6" )
/*	PORT_DIPSETTING(    0x00, "1/1" ) */
	PORT_DIPNAME( 0xf0, 0xf0, "B Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4/1" )
	PORT_DIPSETTING(    0x80, "3/1" )
	PORT_DIPSETTING(    0x90, "2/1" )
	PORT_DIPSETTING(    0x50, "2/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x40, "2/1 + Bonus each 4" )
	PORT_DIPSETTING(    0xf0, "1/1" )
	PORT_DIPSETTING(    0x30, "1/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x20, "1/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x10, "1/1 + Bonus each 2" )
	PORT_DIPSETTING(    0x60, "2/3" )
	PORT_DIPSETTING(    0xe0, "1/2" )
	PORT_DIPSETTING(    0xd0, "1/3" )
	PORT_DIPSETTING(    0xc0, "1/4" )
	PORT_DIPSETTING(    0xb0, "1/5" )
	PORT_DIPSETTING(    0xa0, "1/6" )
/*	PORT_DIPSETTING(    0x00, "1/1" ) */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x06, 0x06, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "3")
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x18, 0x18, "Starting Stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "1")
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "No")
	PORT_DIPSETTING(    0x00, "Yes")
	PORT_DIPNAME( 0x40, 0x40, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "3 minutes" )
	PORT_DIPSETTING(    0x00, "2 minutes" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END


INPUT_PORTS_START( choplift_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3")
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "20000,70000,...")
	PORT_DIPSETTING(    0x20, "50000,100000,...")
	PORT_DIPNAME( 0x40, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "A Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4/1" )
	PORT_DIPSETTING(    0x08, "3/1" )
	PORT_DIPSETTING(    0x09, "2/1" )
	PORT_DIPSETTING(    0x05, "2/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x04, "2/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x0f, "1/1" )
	PORT_DIPSETTING(    0x03, "1/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x02, "1/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x01, "1/1 + Bonus each 2" )
	PORT_DIPSETTING(    0x06, "2/3" )
	PORT_DIPSETTING(    0x0e, "1/2" )
	PORT_DIPSETTING(    0x0d, "1/3" )
	PORT_DIPSETTING(    0x0c, "1/4" )
	PORT_DIPSETTING(    0x0b, "1/5" )
	PORT_DIPSETTING(    0x0a, "1/6" )
/*	PORT_DIPSETTING(    0x00, "1/1" ) */
	PORT_DIPNAME( 0xf0, 0xf0, "B Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4/1" )
	PORT_DIPSETTING(    0x80, "3/1" )
	PORT_DIPSETTING(    0x90, "2/1" )
	PORT_DIPSETTING(    0x50, "2/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x40, "2/1 + Bonus each 4" )
	PORT_DIPSETTING(    0xf0, "1/1" )
	PORT_DIPSETTING(    0x30, "1/1 + Bonus each 5" )
	PORT_DIPSETTING(    0x20, "1/1 + Bonus each 4" )
	PORT_DIPSETTING(    0x10, "1/1 + Bonus each 2" )
	PORT_DIPSETTING(    0x60, "2/3" )
	PORT_DIPSETTING(    0xe0, "1/2" )
	PORT_DIPSETTING(    0xd0, "1/3" )
	PORT_DIPSETTING(    0xc0, "1/4" )
	PORT_DIPSETTING(    0xb0, "1/5" )
	PORT_DIPSETTING(    0xa0, "1/6" )
/*	PORT_DIPSETTING(    0x00, "1/1" ) */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8 by 8 */
	2048,	/* 2048 characters */
	3,		/* 3 bits per pixel */
	{ 0, 2048*8*8, 2*2048*8*8 },			/* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7},			/* x bit */
	{ 0, 8, 16, 24, 32, 40, 48, 56},	/* y bit */
	8*8
};

static struct GfxLayout choplift_charlayout =
{
	8,8,	/* 8 by 8 */
	4096,	/* 4096 characters */
	3,	/* 3 bits per pixel */
	{ 0, 4096*8*8, 2*4096*8*8 },		/* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7},		/* x bit */
	{ 0, 8, 16, 24, 32, 40, 48, 56},	/* y bit */
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* sprites use colors 0-511, but are not defined here */
	{ 1, 0x0000, &charlayout, 512, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo choplift_gfxdecodeinfo[] =
{
	/* sprites use colors 0-511, but are not defined here */
	{ 1, 0x0000, &choplift_charlayout, 512, 128 },
	{ -1 } /* end of array */
};



static void wbdeluxe_init_machine(void)
{
	system8_define_checkspriteram(NULL);
	system8_define_banksupport(SYSTEM8_SUPPORTS_SPRITEBANKS);
	system8_define_spritememsize(2, 0x10000);
	system8_define_sprite_pixelmode(SYSTEM8_SPRITE_PIXEL_MODE1);
}

static void pitfall2_init_machine(void)
{
	system8_define_checkspriteram(pitfall2_clear_spriteram);
	system8_define_banksupport(SYSTEM8_NO_SPRITEBANKS);
	system8_define_spritememsize(2, 0x8000);
	system8_define_sprite_pixelmode(SYSTEM8_SPRITE_PIXEL_MODE1);
}

static void choplift_init_machine(void)
{
	system8_bankswitch_w(0,123);
	system8_define_checkspriteram(NULL);
	system8_define_banksupport(SYSTEM8_SUPPORTS_SPRITEBANKS);
	system8_define_spritememsize(2, 0x20000);
	system8_define_sprite_pixelmode(SYSTEM8_SPRITE_PIXEL_MODE2);
}

static void wbml_init_machine(void)
{
	system8_define_checkspriteram(NULL);
	system8_define_banksupport(SYSTEM8_SUPPORTS_SPRITEBANKS);
	system8_define_spritememsize(2, 0x20000);
	system8_define_sprite_pixelmode(SYSTEM8_SPRITE_PIXEL_MODE2);
}


static unsigned char wbml_color_prom[] =
{
	0x00,0x00,0x00,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0A,0x0B,0x0C,0x00,0x0E,0x0F,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0A,0x0B,0x0A,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,
	0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x08,0x09,0x0A,
	0x0B,0x0C,0x0D,0x0E,0x0F,0x00,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,
	0x0E,0x0F,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x0F,0x0F,0x0F,0x0F,
	0x0F,0x0F,0x0F,0x0F,0x0F,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x09,0x0A,
	0x0B,0x0C,0x0D,0x0E,0x0F,0x07,0x08,0x08,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x00,0x00,
	0x0B,0x0C,0x0D,0x0E,0x0F,0x00,0x00,0x00,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,
	0x0E,0x0F,0x00,0x00,0x00,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x00,0x00,
	0x00,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x05,0x08,0x07,0x08,0x09,0x0A,0x0B,
	0x0C,0x0D,0x0E,0x0F,0x0F,0x0F,0x0F,0x0F,0x0A,0x09,0x0B,0x0D,0x0E,0x0D,0x0C,0x00,

	0x00,0x00,0x00,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x00,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
	0x09,0x0A,0x0B,0x0C,0x0A,0x0B,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,
	0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x06,0x07,0x08,0x09,0x0A,
	0x0B,0x0C,0x0D,0x0E,0x0F,0x0B,0x08,0x05,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x07,0x08,0x09,
	0x0A,0x0B,0x0C,0x0D,0x0E,0x00,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
	0x0C,0x0D,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x03,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0A,0x0B,0x0C,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x05,0x06,
	0x07,0x08,0x09,0x0A,0x0B,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x00,0x00,0x04,0x05,0x06,0x07,0x08,
	0x09,0x0A,0x0B,0x0C,0x0A,0x0B,0x0C,0x0D,0x0F,0x0C,0x0D,0x0F,0x0E,0x0D,0x0C,0x00,

	0x00,0x03,0x05,0x07,0x0D,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x01,0x02,0x03,0x04,
	0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x00,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
	0x0C,0x0D,0x0E,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
	0x0F,0x00,0x00,0x00,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,
	0x06,0x07,0x08,0x09,0x0A,0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,
	0x04,0x05,0x06,0x07,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x02,0x03,
	0x04,0x05,0x06,0x07,0x08,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x00,0x00,
	0x02,0x03,0x04,0x05,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x00,0x00,
	0x00,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x07,0x0A,0x07,0x08,0x09,0x0A,0x0B,
	0x0C,0x0D,0x0E,0x0F,0x0A,0x0B,0x0C,0x0D,0x0A,0x09,0x0B,0x0D,0x0F,0x0F,0x0F,0x00,
};

static unsigned char choplift_color_prom[] =
{
	/* 4B - palette red component */
	0x00,0x02,0x04,0x07,0x09,0x0B,0x0C,0x0D,0x0F,0x00,0x00,0x04,0x07,0x0D,0x00,0x07,
	0x09,0x0A,0x0B,0x00,0x08,0x09,0x0F,0x00,0x08,0x09,0x0B,0x0D,0x0F,0x0A,0x0C,0x0D,
	0x0F,0x00,0x08,0x0B,0x0D,0x0F,0x09,0x0C,0x0F,0x00,0x09,0x0D,0x0E,0x0F,0x00,0x0F,
	0x00,0x0A,0x05,0x00,0x00,0x09,0x08,0x09,0x04,0x0B,0x09,0x0B,0x09,0x0D,0x0D,0x00,
	0x07,0x09,0x0B,0x0D,0x0F,0x0A,0x00,0x04,0x07,0x09,0x0B,0x0C,0x0D,0x05,0x00,0x07,
	0x0B,0x0C,0x0D,0x0F,0x0F,0x07,0x0D,0x0F,0x06,0x06,0x08,0x0A,0x0C,0x0A,0x0E,0x0B,
	0x0F,0x0C,0x0D,0x0F,0x0D,0x0E,0x0F,0x00,0x0F,0x00,0x07,0x08,0x07,0x0A,0x0F,0x09,
	0x0B,0x0E,0x05,0x06,0x00,0x08,0x0D,0x08,0x0F,0x06,0x08,0x09,0x0A,0x0A,0x0D,0x0C,
	0x00,0x0C,0x00,0x06,0x0C,0x07,0x0A,0x09,0x09,0x0A,0x0F,0x0B,0x0D,0x0D,0x00,0x00,
	0x00,0x00,0x07,0x00,0x08,0x00,0x07,0x09,0x0D,0x0F,0x0A,0x0C,0x0E,0x00,0x07,0x09,
	0x0A,0x0B,0x0D,0x0F,0x0C,0x0F,0x00,0x09,0x0B,0x0D,0x0E,0x0F,0x0E,0x0F,0x0B,0x00,
	0x00,0x09,0x0A,0x0B,0x0B,0x0C,0x0D,0x0E,0x0F,0x0F,0x00,0x00,0x00,0x09,0x00,0x00,
	0x0B,0x0C,0x0C,0x0E,0x0C,0x0F,0x00,0x00,0x00,0x00,0x0D,0x0D,0x0F,0x0B,0x0F,0x0E,
	0x0C,0x0E,0x0F,0x00,0x0F,0x0F,0x00,0x09,0x00,0x09,0x0F,0x00,0x09,0x0C,0x0F,0x00,
	0x0F,0x09,0x0D,0x0F,0x00,0x0C,0x0F,0x00,0x09,0x0B,0x0D,0x0F,0x0F,0x0F,0x0F,0x0F,
	0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,

	/* 4B - palette greeen component */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x04,0x04,0x04,0x06,0x07,0x07,
	0x07,0x07,0x07,0x08,0x08,0x08,0x08,0x09,0x09,0x09,0x09,0x09,0x09,0x0A,0x0A,0x0A,
	0x0A,0x0B,0x0B,0x0B,0x0B,0x0B,0x0C,0x0C,0x0C,0x0D,0x0D,0x0D,0x0E,0x0E,0x0F,0x0F,
	0x00,0x07,0x08,0x00,0x06,0x06,0x07,0x07,0x08,0x08,0x02,0x02,0x04,0x04,0x06,0x07,
	0x07,0x07,0x07,0x07,0x07,0x08,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x0A,0x0B,0x0B,
	0x0B,0x0B,0x0B,0x0B,0x0C,0x0D,0x0D,0x0F,0x06,0x07,0x08,0x08,0x08,0x09,0x09,0x0A,
	0x0A,0x0B,0x0B,0x0B,0x0C,0x0D,0x0E,0x00,0x04,0x06,0x07,0x07,0x08,0x08,0x08,0x09,
	0x0A,0x0B,0x05,0x06,0x07,0x07,0x07,0x08,0x08,0x09,0x09,0x09,0x09,0x0A,0x0A,0x0B,
	0x00,0x04,0x06,0x06,0x06,0x08,0x08,0x09,0x0A,0x0A,0x0A,0x0B,0x0B,0x0C,0x00,0x05,
	0x06,0x07,0x07,0x08,0x08,0x09,0x09,0x09,0x09,0x09,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,
	0x0B,0x0B,0x0B,0x0B,0x0C,0x0C,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0E,0x0E,0x0F,0x00,
	0x07,0x09,0x0A,0x0B,0x0C,0x0C,0x0D,0x0E,0x0E,0x0F,0x00,0x07,0x08,0x08,0x09,0x0A,
	0x0B,0x0C,0x0D,0x0E,0x0F,0x0F,0x00,0x08,0x09,0x0A,0x0D,0x0E,0x0F,0x0B,0x0C,0x0E,
	0x0F,0x0F,0x0F,0x00,0x00,0x08,0x09,0x09,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x0B,0x0C,
	0x0C,0x0D,0x0D,0x0D,0x0E,0x0E,0x0E,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
	0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,

	/* 4B - palette blue component */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x02,0x03,0x03,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x05,0x05,0x05,0x05,0x05,0x05,
	0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,
	0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,
	0x07,0x07,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
	0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x0A,0x0A,
	0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,
	0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0B,
	0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,
	0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0E,0x0E,0x0E,
	0x0E,0x0E,0x0E,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
	0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
	0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F
};



static struct SN76496interface sn76496_interface =
{
	2,		/* 2 chips */
	3000000,	/* ??  */
	{ 255, 255 }
};


static struct MachineDriver wbdeluxe_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3650000,		/* 3.65 MHz ? */
			0,			/* memory region */
			readmem,writemem,readport,writeport,
			interrupt,		/* interrupt routine */
			1			/* interrupts per frame */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,			/* 3 Mhz ? */
			3,			/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},

	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	wbdeluxe_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	gfxdecodeinfo,				/* GfxDecodeInfo */
	2048,						/* total colors */
	2048,						/* color table length */
	system8_vh_convert_color_prom,	/* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,					/* vh_init routine */
	system8_vh_start,			/* vh_start routine */
	system8_vh_stop,			/* vh_stop routine */
	system8_vh_screenrefresh,		/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

static struct MachineDriver pitfall2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3650000,			/* 3.65 MHz ? */
			0,				/* memory region */
			readmem,writemem,pitfall2_readport,pitfall2_writeport,
			interrupt,			/* interrupt routine */
			1				/* interrupts per frame */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,			/* 3 Mhz ? */
			3,				/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},

	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	pitfall2_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	gfxdecodeinfo,				/* GfxDecodeInfo */
	2048,						/* total colors */
	2048,						/* color table length */
	system8_vh_convert_color_prom,	/* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,					/* vh_init routine */
	system8_vh_start,			/* vh_start routine */
	system8_vh_stop,			/* vh_stop routine */
	system8_vh_screenrefresh,		/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

static struct MachineDriver choplift_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3650000,			/* 3.65 MHz ? */
			0,				/* memory region */
			wbml_readmem,choplift_writemem,choplift_readport,choplift_writeport,
			interrupt,			/* interrupt routine */
			1				/* interrupts per frame */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,			/* 3 Mhz ? */
			3,				/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,					/* single CPU, no need for interleaving */
	choplift_init_machine,

	/* video hardware */
	256, 256,						/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	choplift_gfxdecodeinfo,			/* GfxDecodeInfo */
	2048,						/* total colors */
	2048,						/* color table length */
	system8_vh_convert_color_prom,	/* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,								/* vh_init routine */
	system8_vh_start,				/* vh_start routine */
	system8_vh_stop,				/* vh_stop routine */
	choplifter_vh_screenrefresh,		/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

static struct MachineDriver wbml_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3650000,			/* 3.65 MHz ? */
			0,					/* memory region */
			wbml_readmem,wbml_writemem,wbml_readport,wbml_writeport,
			interrupt,			/* interrupt routine */
			1					/* interrupts per frame */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,			/* 3 Mhz ? */
			3,					/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	wbml_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	choplift_gfxdecodeinfo,				/* GfxDecodeInfo */
	1536, 1536,
	system8_vh_convert_color_prom,	/* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,					/* vh_init routine */
	system8_vh_start,			/* vh_start routine */
	system8_vh_stop,			/* vh_stop routine */
	wbml_vh_screenrefresh,		/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( wbdeluxe_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "WBD1.BIN", 0x0000, 0x2000, 0xfc881b62 )
	ROM_LOAD( "WBD2.BIN", 0x2000, 0x2000, 0x620b2985 )
	ROM_LOAD( "WBD3.BIN", 0x4000, 0x2000, 0x16949c5e )
	ROM_LOAD( "WBD4.BIN", 0x6000, 0x2000, 0x95cc4dd6 )
	ROM_LOAD( "WBD5.BIN", 0x8000, 0x2000, 0xbb73e2a9 )
	ROM_LOAD( "WBD6.BIN", 0xA000, 0x2000, 0xc57f0e3b )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "WB.008", 0x00000, 0x4000, 0x18e4fd80 )
	ROM_LOAD( "WB.007", 0x04000, 0x4000, 0x140ec290 )
	ROM_LOAD( "WB.006", 0x08000, 0x4000, 0xbd90ec0c )

	ROM_REGION(0x10000)
	ROM_LOAD( "WB.004", 0x00000, 0x4000, 0xa849864b )	/* sprites */
	ROM_CONTINUE(0x8000,0x4000)
	ROM_LOAD( "WB.005", 0x04000, 0x4000, 0x7c7d23f3 )
	ROM_CONTINUE(0xC000,0x4000)

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "WB.009", 0x00000, 0x2000, 0xb2d5545b )
ROM_END


ROM_START( wbml_rom )
	ROM_REGION(0x40000)		/* 256k for code */
	ROM_LOAD( "WBML.01", 0x20000, 0x8000, 0xa5329b82 )	/* Unencrypted opcodes */
	ROM_CONTINUE(        0x00000, 0x8000 )              /* Now load the operands in RAM */
	ROM_LOAD( "WBML.02", 0x30000, 0x8000, 0xcdafb89f )	/* Unencrypted opcodes */
	ROM_CONTINUE(        0x10000, 0x8000 )
	ROM_LOAD( "WBML.03", 0x38000, 0x8000, 0x31cd6733 )	/* Unencrypted opcodes */
	ROM_CONTINUE(        0x18000, 0x8000 )

	ROM_REGION(0x18000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "WBML.08", 0x00000, 0x8000, 0xef89ac27 )
	ROM_LOAD( "WBML.09", 0x08000, 0x8000, 0x60a6011e )
	ROM_LOAD( "WBML.10", 0x10000, 0x8000, 0x563078ba )

	ROM_REGION(0x20000)
	ROM_LOAD( "WBML.05", 0x00000, 0x8000, 0x7d7dd491 )	/* sprites */
	ROM_LOAD( "WBML.04", 0x08000, 0x8000, 0x5ba6090a )
	ROM_LOAD( "WBML.07", 0x10000, 0x8000, 0xf3a6277a )
	ROM_LOAD( "WBML.06", 0x18000, 0x8000, 0x387db381 )

	ROM_REGION(0x10000)		/* 64k for sound cpu */
	ROM_LOAD( "WBML.11", 0x0000, 0x8000, 0x8057f681 )
ROM_END

ROM_START( upndown_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "UPND5679.BIN", 0x0000, 0x2000, 0xf4bca48a )
	ROM_LOAD( "UPND5680.BIN", 0x2000, 0x2000, 0xa24d61c1 )
	ROM_LOAD( "UPND5681.BIN", 0x4000, 0x2000, 0x3aa82e56 )
	ROM_LOAD( "UPND5682.BIN", 0x6000, 0x2000, 0x4ba5c101 )
	ROM_LOAD( "UPND5683.BIN", 0x8000, 0x2000, 0x14e87170 )
	ROM_LOAD( "UPND5684.BIN", 0xA000, 0x2000, 0xdfbc8d48 )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "UPND5527.BIN", 0x00000, 0x2000, 0x0caa0934 )
	ROM_LOAD( "UPND5526.BIN", 0x02000, 0x2000, 0x377bebb9 )
	ROM_LOAD( "UPND5525.BIN", 0x04000, 0x2000, 0x0bf84b26 )
	ROM_LOAD( "UPND5524.BIN", 0x06000, 0x2000, 0x8aacb4ae )
	ROM_LOAD( "UPND5523.BIN", 0x08000, 0x2000, 0x3971bd0d )
	ROM_LOAD( "UPND5522.BIN", 0x0A000, 0x2000, 0xbca7b3a9 )

	ROM_REGION(0x10000)
	ROM_LOAD( "UPND5514.BIN", 0x00000, 0x4000, 0x953bcab7 )  /* sprites */
	ROM_LOAD( "UPND5515.BIN", 0x04000, 0x4000, 0xb93737a3 )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "UPND5528.BIN", 0x00000, 0x2000, 0x058ca4a2 )
ROM_END

ROM_START( pitfall2_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "EPR6623" , 0x0000, 0x4000, 0xf9cbca17 )
	ROM_LOAD( "EPR6624A", 0x4000, 0x4000, 0xec525d6e )
	ROM_LOAD( "EPR6625" , 0x8000, 0x4000, 0x3eb82172 )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "EPR6474A", 0x00000, 0x2000, 0x2516b784 )
	ROM_LOAD( "EPR6473" , 0x02000, 0x2000, 0x9b50df0c )
	ROM_LOAD( "EPR6472A", 0x04000, 0x2000, 0x352e5484 )
	ROM_LOAD( "EPR6471A", 0x06000, 0x2000, 0x7649c70f )
	ROM_LOAD( "EPR6470A", 0x08000, 0x2000, 0xa1bfc31d )
	ROM_LOAD( "EPR6469A", 0x0a000, 0x2000, 0x680bebef )

	ROM_REGION(0x8000)
	ROM_LOAD( "EPR6454A" , 0x00000, 0x4000, 0x460b8fe3 )	/* sprites */
	ROM_LOAD( "EPR6455"  , 0x04000, 0x4000, 0x83827770 )

	ROM_REGION(0x10000)		/* 64k for sound cpu */
	ROM_LOAD( "EPR6462"  , 0x00000, 0x2000, 0x0ecd1add )
ROM_END

ROM_START( chplft_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "7124.90", 0x00000, 0x8000, 0xe67e1670 )
	ROM_LOAD( "7125.91", 0x10000, 0x8000, 0x56d3a903 )
	ROM_LOAD( "7126.92", 0x18000, 0x8000, 0x2f4c41fa )

	ROM_REGION(0x18000) 	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7127.4", 0x00000, 0x8000, 0x684454da )
	ROM_LOAD( "7128.5", 0x08000, 0x8000, 0x60640aac )
	ROM_LOAD( "7129.6", 0x10000, 0x8000, 0x0e274493 )

	ROM_REGION(0x20000)
	ROM_LOAD( "7121.87", 0x00000, 0x8000, 0xa4f7add7 )   /* sprites */
	ROM_LOAD( "7120.86", 0x08000, 0x8000, 0xb59f8d71 )
	ROM_LOAD( "7123.89", 0x10000, 0x8000, 0x23911e63 )
	ROM_LOAD( "7122.88", 0x18000, 0x8000, 0x087b75e1 )

	ROM_REGION(0x10000)   /* 64k for sound cpu */
	ROM_LOAD( "7130.126", 0x00000, 0x8000, 0xf3ed7509 )
ROM_END

ROM_START( chplftb_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "7152.90", 0x00000, 0x8000, 0x59a80b20 )
	ROM_LOAD( "7153.91", 0x10000, 0x8000, 0xb6d3a903 )
	ROM_LOAD( "7154.92", 0x18000, 0x8000, 0x0c4c50fa )

	ROM_REGION(0x18000) 	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7127.4", 0x00000, 0x8000, 0x684454da )
	ROM_LOAD( "7128.5", 0x08000, 0x8000, 0x60640aac )
	ROM_LOAD( "7129.6", 0x10000, 0x8000, 0x0e274493 )

	ROM_REGION(0x20000)
	ROM_LOAD( "7121.87", 0x00000, 0x8000, 0xa4f7add7 )   /* sprites */
	ROM_LOAD( "7120.86", 0x08000, 0x8000, 0xb59f8d71 )
	ROM_LOAD( "7123.89", 0x10000, 0x8000, 0x23911e63 )
	ROM_LOAD( "7122.88", 0x18000, 0x8000, 0x087b75e1 )

	ROM_REGION(0x10000)   /* 64k for sound cpu */
	ROM_LOAD( "7130.126", 0x00000, 0x8000, 0xf3ed7509 )
ROM_END

ROM_START( chplftbl_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "7124bl.90", 0x00000, 0x8000, 0xaa09ffb1 )
	ROM_LOAD( "7125.91",   0x10000, 0x8000, 0x56d3a903 )
	ROM_LOAD( "7126.92",   0x18000, 0x8000, 0x2f4c41fa )

	ROM_REGION(0x18000) 	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7127.4", 0x00000, 0x8000, 0x684454da )
	ROM_LOAD( "7128.5", 0x08000, 0x8000, 0x60640aac )
	ROM_LOAD( "7129.6", 0x10000, 0x8000, 0x0e274493 )

	ROM_REGION(0x20000)
	ROM_LOAD( "7121.87", 0x00000, 0x8000, 0xa4f7add7 )   /* sprites */
	ROM_LOAD( "7120.86", 0x08000, 0x8000, 0xb59f8d71 )
	ROM_LOAD( "7123.89", 0x10000, 0x8000, 0x23911e63 )
	ROM_LOAD( "7122.88", 0x18000, 0x8000, 0x087b75e1 )

	ROM_REGION(0x10000)   /* 64k for sound cpu */
	ROM_LOAD( "7130.126", 0x00000, 0x8000, 0xf3ed7509 )
ROM_END



static void wbml_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	for (A = 0x0000;A < 0x8000;A++)
	{
		ROM[A] = RAM[A+0x20000];
	}
}


static int wbdeluxe_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xC100],"\x20\x11\x20\x20",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int i;


			osd_fread(f,&RAM[0xC100],320);
			osd_fclose(f);

			/* copy the high score to the screen as well */
			for (i = 0; i < 6; i++) /* 6 digits are stored, one per byte */
			{
				if (RAM[0xC102 + i] == 0x20)  /* spaces */
					RAM[0xE858 + i * 2] = 0x01;
				else                          /* digits */
					RAM[0xE858 + i * 2] = RAM[0xC102 + i] - 0x30 + 0x10;
			}
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void wbdeluxe_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xC100],320);
		osd_fclose(f);
	}
}


static int upndown_hiload(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[0];

        /* check if the hi score table has already been initialized */

        if (memcmp(&RAM[0xc93f],"\x01\x00\x00",3) == 0)
        {
                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        osd_fread(f,&RAM[0xc93f],(6*10)+3);
                        osd_fclose(f);
                }
                return 1;
        }
        else return 0;  /* we can't load the hi scores yet */
}

static void upndown_hisave(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[0];

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0xc93f],(6*10)+3);
                osd_fclose(f);
        }
}


static int wbml_hiload(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[0];

        /* check if the hi score table has already been initialized */

        if (memcmp(&RAM[0xc17b],"\x00\x00\x03",3) == 0)
        {
                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        osd_fread(f,&RAM[0xc179],8);
                        osd_fclose(f);
                }
                return 1;
        }
        else return 0;  /* we can't load the hi scores yet */
}

static void wbml_hisave(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[0];

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0xc179],8);
                osd_fclose(f);
        }
}


static int pitfall2_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */

	if (memcmp(&RAM[0xD302],"\x02\x00\x59\x41",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xD300],56);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void pitfall2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xD300],56);
		osd_fclose(f);
	}
}


static int choplift_hiload(void)
{
	void *f;
	unsigned char *choplifter_ram = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */

	if (memcmp(&choplifter_ram[0xEF00],"\x00\x00\x05\x00",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&choplifter_ram[0xEF00],49);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void choplift_hisave(void)
{
	void *f;
	unsigned char *choplifter_ram = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&choplifter_ram[0xEF00],49);
		osd_fclose(f);
	}
}



struct GameDriver wbdeluxe_driver =
{
	__FILE__,
	0,
	"wbdeluxe",
	"Wonder Boy Deluxe",
	"1986",
	"Sega (Escape license)",
	"Jarek Parchanski (MAME driver)\nRoberto Ventura  (hardware info)\nJarek Burczynski (sound)\nMirko Buffoni (additional code)",
	0,
	&wbdeluxe_machine_driver,

	wbdeluxe_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wbdeluxe_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	wbdeluxe_hiload, wbdeluxe_hisave
};

struct GameDriver upndown_driver =
{
	__FILE__,
	0,
	"upndown",
	"Up'n Down",
	"1983",
	"Sega",
	"Jarek Parchanski (MAME driver)\nRoberto Ventura  (hardware info)\nJarek Burczynski (sound)\nMirko Buffoni (additional code)",
	0,
	&wbdeluxe_machine_driver,

	upndown_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	upndown_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	upndown_hiload, upndown_hisave
};

struct GameDriver wbml_driver =
{
	__FILE__,
	0,
	"wbml",
	"Wonder Boy in Monster Land",
	"1987",
	"bootleg",
	"Mirko Buffoni\nNicola Salmoria",
	0,
	&wbml_machine_driver,

	wbml_rom,
	0, wbml_decode,   		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wbml_input_ports,

	wbml_color_prom, 0, 0,
	ORIENTATION_DEFAULT,
	wbml_hiload, wbml_hisave
};


struct GameDriver pitfall2_driver =
{
	__FILE__,
	0,
	"pitfall2",
	"Pitfall II",
	"1985",
	"Sega",
	"Jarek Parchanski (MAME driver)\nRoberto Ventura  (hardware info)\nJarek Burczynski (sound)\nMirko Buffoni (additional code)",
	0,
	&pitfall2_machine_driver,

	pitfall2_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	pitfall2_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	pitfall2_hiload, pitfall2_hisave
};

struct GameDriver chplft_driver =
{
	__FILE__,
	0,
	"chplft",
	"Choplifter",
	"1985",
	"Sega",
	"Jarek Parchanski (MAME driver)\nRoberto Ventura  (hardware info)",
	GAME_NOT_WORKING,
	&choplift_machine_driver,

	chplft_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	choplift_input_ports,

	choplift_color_prom,0,0,
	ORIENTATION_DEFAULT,
	choplift_hiload, choplift_hisave
};

struct GameDriver chplftb_driver =
{
	__FILE__,
	&chplft_driver,
	"chplftb",
	"Choplifter (alternate)",
	"1985",
	"Sega",
	"Jarek Parchanski (MAME driver)\nRoberto Ventura  (hardware info)",
	0,
	&choplift_machine_driver,

	chplftb_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	choplift_input_ports,

	choplift_color_prom,0,0,
	ORIENTATION_DEFAULT,
	choplift_hiload, choplift_hisave
};

struct GameDriver chplftbl_driver =
{
	__FILE__,
	&chplft_driver,
	"chplftbl",
	"Choplifter (bootleg)",
	"1985",
	"bootleg",
	"Jarek Parchanski (MAME driver)\nRoberto Ventura  (hardware info)",
	0,
	&choplift_machine_driver,

	chplftbl_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	choplift_input_ports,

	choplift_color_prom,0,0,
	ORIENTATION_DEFAULT,
	choplift_hiload, choplift_hisave
};
