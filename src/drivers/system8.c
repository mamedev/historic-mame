/******************************************************************************

Up'n Down, Mister Viking, Flicky, SWAT, Water Match and Bull Fight are known
to run on IDENTICAL hardware (they were sold by Bally-Midway as ROM swaps).

******************************************************************************/

#include "driver.h"
#include "vidhrdw/system8.h"

/* in machine/segacrpt.c */
void regulus_decode(void);
void mrviking_decode(void);
void swat_decode(void);
void flicky_decode(void);
void bullfgtj_decode(void);
void pitfall2_decode(void);
void nprinces_decode(void);
void seganinj_decode(void);
void imsorry_decode(void);
void teddybb_decode(void);
void hvymetal_decode(void);
void myheroj_decode(void);
void fdwarrio_decode(void);
void wboy3_decode(void);
void wboy4_decode(void);
void gardia_decode(void);



static void system8_init_machine(void)
{
	/* skip the long IC CHECK in Teddyboy Blues and Choplifter */
	/* this is not a ROM patch, the game checks a RAM location */
	/* before doing the test */
	Machine->memory_region[0][0xeffe] = 0x4f;
	Machine->memory_region[0][0xefff] = 0x4b;

	system8_define_sprite_pixelmode(SYSTEM8_SPRITE_PIXEL_MODE1);

	SN76496_change_clock( 1, 4000000 );
}

static void chplft_init_machine(void)
{
	/* skip the long IC CHECK in Teddyboy Blues and Choplifter */
	/* this is not a ROM patch, the game checks a RAM location */
	/* before doing the test */
	Machine->memory_region[0][0xeffe] = 0x4f;
	Machine->memory_region[0][0xefff] = 0x4b;

	system8_define_sprite_pixelmode(SYSTEM8_SPRITE_PIXEL_MODE2);

	SN76496_change_clock( 1, 4000000 );
}


static int bankswitch;

int wbml_bankswitch_r(int offset)
{
	return bankswitch;
}

void hvymetal_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* patch out the obnoxiously long startup RAM tests */
	ROM[0x4a55] = 0xc3;
	RAM[0x4a56] = 0xb6;
	RAM[0x4a57] = 0x4a;

	bankaddress = 0x10000 + (((data & 0x04)>>2) * 0x4000) + (((data & 0x40)>>5) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);
	/* TODO: the memory system doesn't yet support bank switching on an encrypted */
	/* ROM, so we have to copy the data manually */
	memcpy(&ROM[0x8000],&RAM[bankaddress],0x4000);

	bankswitch = data;
}

void chplft_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (((data & 0x0c)>>2) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);

	bankswitch = data;
}

void wbml_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (((data & 0x0c)>>2) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);
	/* TODO: the memory system doesn't yet support bank switching on an encrypted */
	/* ROM, so we have to copy the data manually */
	if (ROM != RAM)
		memcpy(&ROM[0x8000],&RAM[bankaddress+0x20000],0x4000);

	bankswitch = data;
}

void system8_soundport_w(int offset, int data)
{
	soundlatch_w(0,data);
	cpu_cause_interrupt(1,Z80_NMI_INT);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xefff, MRA_RAM },
	{ 0xf020, 0xf03f, MRA_RAM },
	{ 0xf800, 0xfbff, MRA_RAM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAMROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xdfff, system8_paletteram_w, &paletteram },
	{ 0xe000, 0xe7ff, system8_backgroundram_w, &system8_backgroundram, &system8_backgroundram_size },
	{ 0xe800, 0xeeff, MWA_RAM, &system8_videoram, &system8_videoram_size },
	{ 0xefbd, 0xefbd, MWA_RAM, &system8_scroll_y },
	{ 0xeffc, 0xeffd, MWA_RAM, &system8_scroll_x },
	{ 0xf000, 0xf3ff, system8_background_collisionram_w, &system8_background_collisionram },
	{ 0xf800, 0xfbff, system8_sprites_collisionram_w, &system8_sprites_collisionram },
	{ -1 } /* end of table */
};

static struct MemoryReadAddress wbml_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, wbml_paged_videoram_r },
	{ 0xf020, 0xf03f, MRA_RAM },
	{ 0xf800, 0xfbff, MRA_RAM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress wbml_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAMROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xddff, system8_paletteram_w, &paletteram },
	{ 0xe000, 0xefff, wbml_paged_videoram_w },
	{ 0xf000, 0xf3ff, system8_background_collisionram_w, &system8_background_collisionram },
	{ 0xf800, 0xfbff, system8_sprites_collisionram_w, &system8_sprites_collisionram },
	{ -1 } /* end of table */
};

static struct MemoryReadAddress chplft_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xefff, MRA_RAM },
	{ 0xf020, 0xf03f, MRA_RAM },
	{ 0xf800, 0xfbff, MRA_RAM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress chplft_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAMROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xdfff, system8_paletteram_w, &paletteram },
	{ 0xe7c0, 0xe7ff, choplifter_scroll_x_w, &system8_scrollx_ram },
	{ 0xe000, 0xe7ff, system8_videoram_w, &system8_videoram, &system8_videoram_size },
	{ 0xe800, 0xeeff, system8_backgroundram_w, &system8_backgroundram, &system8_backgroundram_size },
	{ 0xf000, 0xf3ff, system8_background_collisionram_w, &system8_background_collisionram },
	{ 0xf800, 0xfbff, system8_sprites_collisionram_w, &system8_sprites_collisionram },
	{ -1 } /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },	/* joy1 */
	{ 0x04, 0x04, input_port_1_r },	/* joy2 */
	{ 0x08, 0x08, input_port_2_r },	/* coin,start */
	{ 0x0c, 0x0c, input_port_3_r },	/* DIP2 */
	{ 0x0d, 0x0d, input_port_4_r },	/* DIP1 some games read it from here... */
	{ 0x10, 0x10, input_port_4_r },	/* DIP1 ... and some others from here */
									/* but there are games which check BOTH! */
	{ 0x15, 0x15, system8_videomode_r },
	{ 0x19, 0x19, system8_videomode_r },	/* mirror address */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x14, 0x14, system8_soundport_w },	/* sound commands */
	{ 0x15, 0x15, system8_videomode_w },	/* video control and (in some games) bank switching */
	{ 0x18, 0x18, system8_soundport_w },	/* mirror address */
	{ 0x19, 0x19, system8_videomode_w },	/* mirror address */
	{ -1 }	/* end of table */
};

static struct IOReadPort wbml_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },	/* joy1 */
	{ 0x04, 0x04, input_port_1_r },	/* joy2 */
	{ 0x08, 0x08, input_port_2_r },	/* coin,start */
	{ 0x0c, 0x0c, input_port_3_r },	/* DIP2 */
	{ 0x0d, 0x0d, input_port_4_r },	/* DIP1 some games read it from here... */
	{ 0x10, 0x10, input_port_4_r },	/* DIP1 ... and some others from here */
									/* but there are games which check BOTH! */
	{ 0x15, 0x15, wbml_bankswitch_r },
	{ 0x16, 0x16, wbml_bg_bankselect_r },
	{ 0x19, 0x19, wbml_bankswitch_r },	/* mirror address */
	{ -1 }	/* end of table */
};

static struct IOWritePort wbml_writeport[] =
{
	{ 0x14, 0x14, system8_soundport_w },	/* sound commands */
	{ 0x15, 0x15, wbml_bankswitch_w },
	{ 0x16, 0x16, wbml_bg_bankselect_w },
	{ -1 }	/* end of table */
};

static struct IOWritePort hvymetal_writeport[] =
{
	{ 0x18, 0x18, system8_soundport_w },	/* sound commands */
	{ 0x19, 0x19, hvymetal_bankswitch_w },
	{ -1 }	/* end of table */
};

static struct IOWritePort chplft_writeport[] =
{
	{ 0x14, 0x14, system8_soundport_w },	/* sound commands */
	{ 0x15, 0x15, chplft_bankswitch_w },
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


#define IN0_PORT \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define DSW1_PORT \
	PORT_DIPNAME( 0x0f, 0x0f, "A Coin/Cred", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x07, "4/1" ) \
	PORT_DIPSETTING(    0x08, "3/1" ) \
	PORT_DIPSETTING(    0x09, "2/1" ) \
	PORT_DIPSETTING(    0x05, "2/1 + Bonus each 5" ) \
	PORT_DIPSETTING(    0x04, "2/1 + Bonus each 4" ) \
	PORT_DIPSETTING(    0x0f, "1/1" ) \
	PORT_DIPSETTING(    0x03, "1/1 + Bonus each 5" ) \
	PORT_DIPSETTING(    0x02, "1/1 + Bonus each 4" ) \
	PORT_DIPSETTING(    0x01, "1/1 + Bonus each 2" ) \
	PORT_DIPSETTING(    0x06, "2/3" ) \
	PORT_DIPSETTING(    0x0e, "1/2" ) \
	PORT_DIPSETTING(    0x0d, "1/3" ) \
	PORT_DIPSETTING(    0x0c, "1/4" ) \
	PORT_DIPSETTING(    0x0b, "1/5" ) \
	PORT_DIPSETTING(    0x0a, "1/6" ) \
/*	PORT_DIPSETTING(    0x00, "1/1" ) */ \
	PORT_DIPNAME( 0xf0, 0xf0, "B Coin/Cred", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x70, "4/1" ) \
	PORT_DIPSETTING(    0x80, "3/1" ) \
	PORT_DIPSETTING(    0x90, "2/1" ) \
	PORT_DIPSETTING(    0x50, "2/1 + Bonus each 5" ) \
	PORT_DIPSETTING(    0x40, "2/1 + Bonus each 4" ) \
	PORT_DIPSETTING(    0xf0, "1/1" ) \
	PORT_DIPSETTING(    0x30, "1/1 + Bonus each 5" ) \
	PORT_DIPSETTING(    0x20, "1/1 + Bonus each 4" ) \
	PORT_DIPSETTING(    0x10, "1/1 + Bonus each 2" ) \
	PORT_DIPSETTING(    0x60, "2/3" ) \
	PORT_DIPSETTING(    0xe0, "1/2" ) \
	PORT_DIPSETTING(    0xd0, "1/3" ) \
	PORT_DIPSETTING(    0xc0, "1/4" ) \
	PORT_DIPSETTING(    0xb0, "1/5" ) \
	PORT_DIPSETTING(    0xa0, "1/6" ) \
/*	PORT_DIPSETTING(    0x00, "1/1" ) */


INPUT_PORTS_START( starjack_input_ports )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x06, 0x06, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x38, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "20000 50000" )
	PORT_DIPSETTING(    0x20, "30000 70000" )
	PORT_DIPSETTING(    0x10, "40000 90000" )
	PORT_DIPSETTING(    0x00, "50000 110000" )
	PORT_DIPSETTING(    0x38, "20000" )
	PORT_DIPSETTING(    0x28, "30000" )
	PORT_DIPSETTING(    0x18, "40000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( starjacs_input_ports )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x06, 0x06, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x08, 0x08, "Ship", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Single" )
	PORT_DIPSETTING(    0x00, "Multi" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30000 70000" )
	PORT_DIPSETTING(    0x20, "40000 90000" )
	PORT_DIPSETTING(    0x10, "50000 110000" )
	PORT_DIPSETTING(    0x00, "60000 130000" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( regulus_input_ports )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

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

INPUT_PORTS_START( mrviking_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "10000 30000 60000" )
	PORT_DIPSETTING(    0x20, "20000 40000 70000" )
	PORT_DIPSETTING(    0x10, "30000 60000 90000" )
	PORT_DIPSETTING(    0x00, "40000 70000 100000" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
INPUT_PORTS_END

INPUT_PORTS_START( swat_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

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
	PORT_DIPSETTING(    0x38, "30000" )
	PORT_DIPSETTING(    0x30, "40000" )
	PORT_DIPSETTING(    0x28, "50000" )
	PORT_DIPSETTING(    0x20, "60000" )
	PORT_DIPSETTING(    0x18, "70000" )
	PORT_DIPSETTING(    0x10, "80000" )
	PORT_DIPSETTING(    0x08, "90000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( flicky_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30000 80000 160000" )
	PORT_DIPSETTING(    0x20, "30000 100000 200000" )
	PORT_DIPSETTING(    0x10, "40000 120000 240000" )
	PORT_DIPSETTING(    0x00, "40000 140000 280000" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty?", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "Easy?" )
	PORT_DIPSETTING(    0x80, "Medium?" )
	PORT_DIPSETTING(    0x40, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
INPUT_PORTS_END

INPUT_PORTS_START( bullfgtj_input_ports )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

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
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30000" )
	PORT_DIPSETTING(    0x20, "50000" )
	PORT_DIPSETTING(    0x10, "70000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

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
	PORT_DIPNAME( 0x10, 0x10, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "20000 50000" )
	PORT_DIPSETTING(    0x00, "30000 70000" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x40, 0x40, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Minutes" )
	PORT_DIPSETTING(    0x40, "3 Minutes" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( pitfallu_input_ports )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x06, 0x06, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x18, 0x18, "Starting Stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x40, 0x40, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Minutes" )
	PORT_DIPSETTING(    0x40, "3 Minutes" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( seganinj_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

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
	PORT_DIPNAME( 0x10, 0x10, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "20k 70k 120k 170k" )
	PORT_DIPSETTING(    0x00, "60k 100k 160k 200k" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( imsorry_input_ports )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0C, 0x0C, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30000" )
	PORT_DIPSETTING(    0x20, "40000" )
	PORT_DIPSETTING(    0x10, "50000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( teddybb_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x0C, 0x0C, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0C, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "100k 400k" )
	PORT_DIPSETTING(    0x20, "200k 600k" )
	PORT_DIPSETTING(    0x10, "400k 800k" )
	PORT_DIPSETTING(    0x00, "600k" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( hvymetal_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0C, 0x0C, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50000 100000" )
	PORT_DIPSETTING(    0x20, "60000 120000" )
	PORT_DIPSETTING(    0x10, "70000 150000" )
	PORT_DIPSETTING(    0x00, "100000" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
INPUT_PORTS_END

INPUT_PORTS_START( myhero_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0C, 0x0C, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30000" )
	PORT_DIPSETTING(    0x20, "50000" )
	PORT_DIPSETTING(    0x10, "70000" )
	PORT_DIPSETTING(    0x00, "90000" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( chplft_input_ports )
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
	IN0_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "20000 70000" )
	PORT_DIPSETTING(    0x20, "50000 100000" )
	PORT_DIPNAME( 0x40, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW0 */
	DSW1_PORT
INPUT_PORTS_END

INPUT_PORTS_START( fdwarrio_input_ports )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

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
	PORT_DIPSETTING(    0x38, "30000" )
	PORT_DIPSETTING(    0x30, "40000" )
	PORT_DIPSETTING(    0x28, "50000" )
	PORT_DIPSETTING(    0x20, "60000" )
	PORT_DIPSETTING(    0x18, "70000" )
	PORT_DIPSETTING(    0x10, "80000" )
	PORT_DIPSETTING(    0x08, "90000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( brain_input_ports )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

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
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( wboy_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

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
	PORT_DIPSETTING(    0x10, "30k 100k 170k 240k" )
	PORT_DIPSETTING(    0x00, "30k 120k 210k 300k" )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

/* same as wboy, additional Energy Consumption switch */
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
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

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
	PORT_DIPSETTING(    0x10, "30k 100k 170k 240k" )
	PORT_DIPSETTING(    0x00, "30k 120k 210k 300k" )
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

INPUT_PORTS_START( wboyu_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
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
	IN0_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x06, 0x06, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "4 Coins/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "2 Coins/2 Credits" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0xc0, 0xc0, "Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "Normal Game" )
	PORT_DIPSETTING(    0x80, "Free Play" )
	PORT_DIPSETTING(    0x40, "Test Mode" )
	PORT_DIPSETTING(    0x00, "Endless Game" )
INPUT_PORTS_END

INPUT_PORTS_START( tokisens_input_ports )
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
	IN0_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	DSW1_PORT
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
	IN0_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x08, "5" )
/*	PORT_DIPSETTING(    0x00, "4" ) */
	PORT_DIPNAME( 0x10, 0x00, "Unknown (Difficulty?)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Test Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW0 */
	DSW1_PORT
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8 by 8 */
	2048,	/* 2048 characters */
	3,		/* 3 bits per pixel */
	{ 0, 2048*8*8, 2*2048*8*8 },			/* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout chplft_charlayout =
{
	8,8,	/* 8 by 8 */
	4096,	/* 4096 characters */
	3,	/* 3 bits per pixel */
	{ 0, 4096*8*8, 2*4096*8*8 },		/* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* sprites use colors 0-511, but are not defined here */
	{ 1, 0x0000, &charlayout, 512, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo chplft_gfxdecodeinfo[] =
{
	/* sprites use colors 0-511, but are not defined here */
	{ 1, 0x0000, &chplft_charlayout, 512, 128 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,		/* 2 chips */
	2000000,	/* 8 MHz / 4 ?*/
	{ 255, 255 }
};



static struct MachineDriver system8_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* My Hero has 2 OSCs 8 & 20 MHz (Cabbe Info) */
			0,		/* memory region */
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,
			3,		/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	system8_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	gfxdecodeinfo,				/* GfxDecodeInfo */
	2048,					/* total colors */
	2048,					/* color table length */
	system8_vh_convert_color_prom,		/* convert color prom routine */

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

/* driver with reduced visible area for scrolling games */
static struct MachineDriver system8_small_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* My Hero has 2 OSCs 8 & 20 MHz (Cabbe Info) */
			0,		/* memory region */
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,
			3,		/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	system8_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8+8, 32*8-1-8, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	gfxdecodeinfo,				/* GfxDecodeInfo */
	2048,					/* total colors */
	2048,					/* color table length */
	system8_vh_convert_color_prom,		/* convert color prom routine */

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
			3650000,			/* 3.65 MHz ? changing it to 4 makes the title disappear */
			0,				/* memory region */
			readmem,writemem,readport,writeport,
			interrupt,1
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
	system8_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },		/* struct rectangle visible_area */
	gfxdecodeinfo,				/* GfxDecodeInfo */
	2048,					/* total colors */
	2048,					/* color table length */
	system8_vh_convert_color_prom,	        /* convert color prom routine */

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

static struct MachineDriver hvymetal_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,			/* 4 MHz ? */
			0,				/* memory region */
			chplft_readmem,writemem,wbml_readport,hvymetal_writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,			/* 4 Mhz ? */
			3,				/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	        /* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	chplft_init_machine,

	/* video hardware */
	256, 256,					/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	chplft_gfxdecodeinfo,			        /* GfxDecodeInfo */
	2048,						/* total colors */
	2048,						/* color table length */
	system8_vh_convert_color_prom,	/* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,								/* vh_init routine */
	system8_vh_start,				/* vh_start routine */
	system8_vh_stop,				/* vh_stop routine */
	system8_vh_screenrefresh,		        /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

static struct MachineDriver chplft_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,			/* 4 MHz ? */
			0,				/* memory region */
			chplft_readmem,chplft_writemem,wbml_readport,chplft_writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,			/* 4 Mhz ? */
			3,				/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	        /* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	chplft_init_machine,

	/* video hardware */
	256, 256,					/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	chplft_gfxdecodeinfo,			        /* GfxDecodeInfo */
	2048,						/* total colors */
	2048,						/* color table length */
	system8_vh_convert_color_prom,	/* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,								/* vh_init routine */
	system8_vh_start,				/* vh_start routine */
	system8_vh_stop,				/* vh_stop routine */
	choplifter_vh_screenrefresh,		        /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

static struct MachineDriver brain_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* My Hero has 2 OSCs 8 & 20 MHz (Cabbe Info) */
			0,		/* memory region */
			readmem,writemem,readport,hvymetal_writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,
			3,		/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	system8_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	gfxdecodeinfo,				/* GfxDecodeInfo */
	2048,					/* total colors */
	2048,					/* color table length */
	system8_vh_convert_color_prom,		/* convert color prom routine */

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

static struct MachineDriver wbml_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,			/* 4 MHz ? */
			0,				/* memory region */
			wbml_readmem,wbml_writemem,wbml_readport,wbml_writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,			/* 4 Mhz ? */
			3,				/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	chplft_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },		/* struct rectangle visible_area */
	chplft_gfxdecodeinfo,			/* GfxDecodeInfo */
	1536, 1536,
	system8_vh_convert_color_prom,	        /* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,					/* vh_init routine */
	system8_vh_start,			/* vh_start routine */
	system8_vh_stop,			/* vh_stop routine */
	wbml_vh_screenrefresh,		        /* vh_update routine */

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

/* Since the standard System 8 PROM has part # 5317, Star Jacker, whose first */
/* ROM is #5318, is probably the first or second System 8 game produced */
ROM_START( starjack_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "5320b", 0x0000, 0x2000, 0x17306628 , 0x7ab72ecd )
	ROM_LOAD( "5321a", 0x2000, 0x2000, 0xeb13729b , 0x38b99050 )
	ROM_LOAD( "5322a", 0x4000, 0x2000, 0x20a44092 , 0x103a595b )
	ROM_LOAD( "5323", 0x6000, 0x2000, 0xcbe12a9d , 0x46af0d58 )
	ROM_LOAD( "5324", 0x8000, 0x2000, 0xa5b12405 , 0x1e89efe2 )
	ROM_LOAD( "5325", 0xa000, 0x2000, 0x1110e8e0 , 0xd6e379a1 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5331", 0x0000, 0x2000, 0x4f2c7db0 , 0x251d898f )
	ROM_LOAD( "5330", 0x2000, 0x2000, 0x8a282108 , 0xeb048745 )
	ROM_LOAD( "5329", 0x4000, 0x2000, 0xa43fd3fd , 0x3e8bcaed )
	ROM_LOAD( "5328", 0x6000, 0x2000, 0xf6ff65a1 , 0x9ed7849f )
	ROM_LOAD( "5327", 0x8000, 0x2000, 0x524b0071 , 0x79e92cb1 )
	ROM_LOAD( "5326", 0xa000, 0x2000, 0xef63c1fd , 0xba7e2b47 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "5318", 0x0000, 0x4000, 0x6086ba1c , 0x6f2e1fd3 )
	ROM_LOAD( "5319", 0x4000, 0x4000, 0x2e80bddc , 0xebee4999 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "5332", 0x0000, 0x2000, 0x6281a187 , 0x7a72ab3d )
ROM_END

ROM_START( starjacs_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sja1ic29", 0x0000, 0x2000, 0x7e425e80 , 0x59a22a1f )
	ROM_LOAD( "sja1ic30", 0x2000, 0x2000, 0xbf736f7d , 0x7f4597dc )
	ROM_LOAD( "sja1ic31", 0x4000, 0x2000, 0x5a73537f , 0x6074c046 )
	ROM_LOAD( "sja1ic32", 0x6000, 0x2000, 0xf1747034 , 0x1c48a3fa )
	ROM_LOAD( "sja1ic33", 0x8000, 0x2000, 0xb55a3b02 , 0x7598bd51 )
	ROM_LOAD( "sja1ic34", 0xa000, 0x2000, 0x81dd84e7 , 0xf66fa604 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5331", 0x0000, 0x2000, 0x4f2c7db0 , 0x251d898f )
	ROM_LOAD( "sja1ic65", 0x2000, 0x2000, 0x8e282508 , 0x0ab1893c )
	ROM_LOAD( "5329", 0x4000, 0x2000, 0xa43fd3fd , 0x3e8bcaed )
	ROM_LOAD( "sja1ic64", 0x6000, 0x2000, 0xfaff61a1 , 0x7f628ae6 )
	ROM_LOAD( "5327", 0x8000, 0x2000, 0x524b0071 , 0x79e92cb1 )
	ROM_LOAD( "sja1ic63", 0xa000, 0x2000, 0xf363c5fd , 0x5bcb253e )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	/* SJA1IC86 and SJA1IC93 in the original set were bad, so I'm using the ones */
	/* from the Sega version. However I suspect the real ones should be slightly */
	/* different. */
	ROM_LOAD( "5318", 0x0000, 0x4000, 0x6086ba1c , 0x6f2e1fd3 )
	ROM_LOAD( "5319", 0x4000, 0x4000, 0x2e80bddc , 0xebee4999 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "5332", 0x0000, 0x2000, 0x6281a187 , 0x7a72ab3d )
ROM_END

ROM_START( regulus_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr5640a.29", 0x0000, 0x2000, 0x57361916 , 0xdafb1528 )	/* encrypted */
	ROM_LOAD( "epr5641a.30", 0x2000, 0x2000, 0x3554cd26 , 0x0fcc850e )	/* encrypted */
	ROM_LOAD( "epr5642a.31", 0x4000, 0x2000, 0xfbc0235a , 0x4feffa17 )	/* encrypted */
	ROM_LOAD( "epr5643a.32", 0x6000, 0x2000, 0x9e60a026 , 0xb8ac7eb4 )	/* encrypted */
	ROM_LOAD( "epr5644.33", 0x8000, 0x2000, 0x43475a5b , 0xffd05b7d )
	ROM_LOAD( "epr5645a.34", 0xa000, 0x2000, 0xe73c5390 , 0x6b4bf77c )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr5651.82", 0x0000, 0x2000, 0x8789e4b1 , 0xf07f3e82 )
	ROM_LOAD( "epr5650.65", 0x2000, 0x2000, 0xaca98e1d , 0x84c1baa2 )
	ROM_LOAD( "epr5649.81", 0x4000, 0x2000, 0xa64a0050 , 0x6774c895 )
	ROM_LOAD( "epr5648.64", 0x6000, 0x2000, 0xacedc277 , 0x0c69e92a )
	ROM_LOAD( "epr5647.80", 0x8000, 0x2000, 0x7571e9d1 , 0x9330f7b5 )
	ROM_LOAD( "epr5646.63", 0xa000, 0x2000, 0x25969ae0 , 0x4dfacbbc )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "epr5638.92", 0x0000, 0x4000, 0x8ab05178 , 0x617363dd )
	ROM_LOAD( "epr5639.93", 0x4000, 0x4000, 0x9c3c356a , 0xa4ec5131 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr5652.3", 0x0000, 0x2000, 0xc9bebbcc , 0x74edcb98 )
ROM_END

ROM_START( upndown_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "upnd5679.bin", 0x0000, 0x2000, 0xf4bca48a , 0xc4f2f9c2 )
	ROM_LOAD( "upnd5680.bin", 0x2000, 0x2000, 0xa24d61c1 , 0x837f021c )
	ROM_LOAD( "upnd5681.bin", 0x4000, 0x2000, 0x3aa82e56 , 0xe1c7ff7e )
	ROM_LOAD( "upnd5682.bin", 0x6000, 0x2000, 0x4ba5c101 , 0x4a5edc1e )
	ROM_LOAD( "upnd5683.bin", 0x8000, 0x2000, 0x14e87170 , 0x208dfbdf )
	ROM_LOAD( "upnd5684.bin", 0xA000, 0x2000, 0xdfbc8d48 , 0x32fa95da )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "upnd5527.bin", 0x0000, 0x2000, 0x0caa0934 , 0xb2d616f1 )
	ROM_LOAD( "upnd5526.bin", 0x2000, 0x2000, 0x377bebb9 , 0x8a8b33c2 )
	ROM_LOAD( "upnd5525.bin", 0x4000, 0x2000, 0x0bf84b26 , 0xe749c5ef )
	ROM_LOAD( "upnd5524.bin", 0x6000, 0x2000, 0x8aacb4ae , 0x8b886952 )
	ROM_LOAD( "upnd5523.bin", 0x8000, 0x2000, 0x3971bd0d , 0xdede35d9 )
	ROM_LOAD( "upnd5522.bin", 0xA000, 0x2000, 0xbca7b3a9 , 0x5e6d9dff )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "upnd5514.bin", 0x0000, 0x4000, 0x953bcab7 , 0xfcc0a88b )
	ROM_LOAD( "upnd5515.bin", 0x4000, 0x4000, 0xb93737a3 , 0x60908838 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "upnd5528.bin", 0x0000, 0x2000, 0x058ca4a2 , 0x00cd44ab )
ROM_END

ROM_START( mrviking_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "vepr5873", 0x0000, 0x2000, 0x09ac75ec , 0x14d21624 )	/* encrypted */
	ROM_LOAD( "vepr5874", 0x2000, 0x2000, 0x29dd566b , 0x6df7de87 )	/* encrypted */
	ROM_LOAD( "vepr5975", 0x4000, 0x2000, 0x3129e8e5 , 0xac226100 )	/* encrypted */
	ROM_LOAD( "vepr5876", 0x6000, 0x2000, 0x1cc75bf1 , 0xe77db1dc )	/* encrypted */
	ROM_LOAD( "vepr5755", 0x8000, 0x2000, 0x724cbf9c , 0xedd62ae1 )
	ROM_LOAD( "vepr5756", 0xa000, 0x2000, 0x1150cffc , 0x11974040 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vepr5762", 0x0000, 0x2000, 0xea1613ce , 0x4a91d08a )
	ROM_LOAD( "vepr5761", 0x2000, 0x2000, 0x8fea105a , 0xf7d61b65 )
	ROM_LOAD( "vepr5760", 0x4000, 0x2000, 0x1d49c9fb , 0x95045820 )
	ROM_LOAD( "vepr5759", 0x6000, 0x2000, 0x2faa3840 , 0x5f9bae4e )
	ROM_LOAD( "vepr5758", 0x8000, 0x2000, 0x9c224f40 , 0x808ee706 )
	ROM_LOAD( "vepr5757", 0xa000, 0x2000, 0x3dc24aa4 , 0x480f7074 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "viepr574", 0x0000, 0x4000, 0xcbfc40b2 , 0xe24682cd )
	ROM_LOAD( "vepr5750", 0x4000, 0x4000, 0xf392a85c , 0x6564d1ad )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "vepr5763", 0x0000, 0x2000, 0x8e6f1da3 , 0xd712280d )
ROM_END

ROM_START( swat_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr5807b.29", 0x0000, 0x2000, 0x631e2c6e , 0x93db9c9f )	/* encrypted */
	ROM_LOAD( "epr5808.30", 0x2000, 0x2000, 0xb7ee7ad8 , 0x67116665 )	/* encrypted */
	ROM_LOAD( "epr5809.31", 0x4000, 0x2000, 0xadf564f1 , 0xfd792fc9 )	/* encrypted */
	ROM_LOAD( "epr5810.32", 0x6000, 0x2000, 0xac3ab09a , 0xdc2b279d )	/* encrypted */
	ROM_LOAD( "epr5811.33", 0x8000, 0x2000, 0x986423be , 0x093e3ab1 )
	ROM_LOAD( "epr5812.34", 0xa000, 0x2000, 0x99f21438 , 0x5bfd692f )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr5818.82", 0x0000, 0x2000, 0x47fc6780 , 0xb22033d9 )
	ROM_LOAD( "epr5817.65", 0x2000, 0x2000, 0x119458a8 , 0xfd942797 )
	ROM_LOAD( "epr5816.81", 0x4000, 0x2000, 0xe8766bbc , 0x4384376d )
	ROM_LOAD( "epr5815.64", 0x6000, 0x2000, 0x4016df4a , 0x16ad046c )
	ROM_LOAD( "epr5814.80", 0x8000, 0x2000, 0x0df2a8f8 , 0xbe721c99 )
	ROM_LOAD( "epr5813.63", 0xa000, 0x2000, 0x2ebb4261 , 0x0d42c27e )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "epr5805.92", 0x0000, 0x4000, 0x77454241 , 0x5a732865 )
	ROM_LOAD( "epr5806.93", 0x4000, 0x4000, 0xec0f77d7 , 0x26ac258c )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr5819.3", 0x0000, 0x2000, 0x84d76e9d , 0xf6afd0fd )
ROM_END

ROM_START( flicky_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr5978", 0x0000, 0x4000, 0x54f40c56 , 0x296f1492 )	/* encrypted */
	ROM_LOAD( "epr5979", 0x4000, 0x4000, 0x2cbebf9a , 0x64b03ef9 )	/* encrypted */

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6001", 0x0000, 0x4000, 0x933e3ed6 , 0xf1a75200 )
	ROM_LOAD( "epr6000", 0x4000, 0x4000, 0x9b3e5b02 , 0x299aefb7 )
	ROM_LOAD( "epr5999", 0x8000, 0x4000, 0x91f438f4 , 0x1ca53157 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "epr5855", 0x0000, 0x4000, 0x55f40962 , 0xb5f894a1 )
	ROM_LOAD( "epr5856", 0x4000, 0x4000, 0xae813df9 , 0x266af78f )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr5869", 0x0000, 0x2000, 0x9e237d9b , 0x6d220d4e )
ROM_END

ROM_START( flicky2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "f_9", 0x0000, 0x2000, 0x3433eb99 , 0xa65ac88e )	/* encrypted */
	ROM_LOAD( "f_10", 0x2000, 0x2000, 0x152b3063 , 0x18b412f4 )	/* encrypted */
	ROM_LOAD( "f_11", 0x4000, 0x2000, 0xfb0cfdb2 , 0xa5558d7e )	/* encrypted */
	ROM_LOAD( "f_12", 0x6000, 0x2000, 0x6f1a9a42 , 0x1b35fef1 )	/* encrypted */

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6001", 0x0000, 0x4000, 0x933e3ed6 , 0xf1a75200 )
	ROM_LOAD( "epr6000", 0x4000, 0x4000, 0x9b3e5b02 , 0x299aefb7 )
	ROM_LOAD( "epr5999", 0x8000, 0x4000, 0x91f438f4 , 0x1ca53157 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "epr5855", 0x0000, 0x4000, 0x55f40962 , 0xb5f894a1 )
	ROM_LOAD( "epr5856", 0x4000, 0x4000, 0xae813df9 , 0x266af78f )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr5869", 0x0000, 0x2000, 0x9e237d9b , 0x6d220d4e )
ROM_END

ROM_START( bullfgtj_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "6071", 0x0000, 0x4000, 0x962feedd , 0x96b57df9 )	/* encrypted */
	ROM_LOAD( "6072", 0x4000, 0x4000, 0x98bc6886 , 0xf7baadd0 )	/* encrypted */
	ROM_LOAD( "6073", 0x8000, 0x4000, 0xceb85852 , 0x721af166 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6089", 0x0000, 0x2000, 0xb9c37229 , 0xa183e5ff )
	ROM_LOAD( "6088", 0x2000, 0x2000, 0x785953d3 , 0xb919b4a6 )
	ROM_LOAD( "6087", 0x4000, 0x2000, 0x63b24558 , 0x2677742c )
	ROM_LOAD( "6086", 0x6000, 0x2000, 0xe98f0681 , 0x76b5a084 )
	ROM_LOAD( "6085", 0x8000, 0x2000, 0x2e64d932 , 0x9c3ddc62 )
	ROM_LOAD( "6084", 0xA000, 0x2000, 0x936ea324 , 0x90e1fa5f )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "6069", 0x0000, 0x4000, 0x63318d9f , 0xfe691e41 )
	ROM_LOAD( "6070", 0x4000, 0x4000, 0xc415c0a7 , 0x34f080df )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "6077", 0x0000, 0x2000, 0xb1141920 , 0x02a37602 )
ROM_END

ROM_START( pitfall2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr6456a.116", 0x0000, 0x4000, 0x625ed296 , 0xbcc8406b )	/* encrypted */
	ROM_LOAD( "epr6457a.109", 0x4000, 0x4000, 0x5d48d072 , 0xa016fd2a )	/* encrypted */
	ROM_LOAD( "epr6458a.96", 0x8000, 0x4000, 0x52a46294 , 0x5c30b3e8 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6474a.62", 0x0000, 0x2000, 0x2516b784 , 0x9f1711b9 )
	ROM_LOAD( "epr6473a.61", 0x2000, 0x2000, 0x9b50df0c , 0x8e53b8dd )
	ROM_LOAD( "epr6472a.64", 0x4000, 0x2000, 0x352e5484 , 0xe0f34a11 )
	ROM_LOAD( "epr6471a.63", 0x6000, 0x2000, 0x7649c70f , 0xd5bc805c )
	ROM_LOAD( "epr6470a.66", 0x8000, 0x2000, 0xa1bfc31d , 0x1439729f )
	ROM_LOAD( "epr6469a.65", 0xa000, 0x2000, 0x680bebef , 0xe4ac6921 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "epr6454a.117", 0x0000, 0x4000, 0x460b8fe3 , 0xa5d96780 )
	ROM_LOAD( "epr6455.05", 0x4000, 0x4000, 0x83827770 , 0x32ee64a1 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr6462.120", 0x0000, 0x2000, 0x0ecd1add , 0x86bb9185 )
ROM_END

ROM_START( pitfallu_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr6623", 0x0000, 0x4000, 0xf9cbca17 , 0xbcb47ed6 )
	ROM_LOAD( "epr6624a", 0x4000, 0x4000, 0xec525d6e , 0x6e8b09c1 )
	ROM_LOAD( "epr6625", 0x8000, 0x4000, 0x3eb82172 , 0xdc5484ba )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6474a.62", 0x0000, 0x2000, 0x2516b784 , 0x9f1711b9 )
	ROM_LOAD( "epr6473a.61", 0x2000, 0x2000, 0x9b50df0c , 0x8e53b8dd )
	ROM_LOAD( "epr6472a.64", 0x4000, 0x2000, 0x352e5484 , 0xe0f34a11 )
	ROM_LOAD( "epr6471a.63", 0x6000, 0x2000, 0x7649c70f , 0xd5bc805c )
	ROM_LOAD( "epr6470a.66", 0x8000, 0x2000, 0xa1bfc31d , 0x1439729f )
	ROM_LOAD( "epr6469a.65", 0xa000, 0x2000, 0x680bebef , 0xe4ac6921 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "epr6454a.117", 0x0000, 0x4000, 0x460b8fe3 , 0xa5d96780 )
	ROM_LOAD( "epr6455.05", 0x4000, 0x4000, 0x83827770 , 0x32ee64a1 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr6462.120", 0x0000, 0x2000, 0x0ecd1add , 0x86bb9185 )
ROM_END

ROM_START( seganinj_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic116.bin", 0x0000, 0x4000, 0x5d7df5a9 , 0xa5d0c9d0 )	/* encrypted */
	ROM_LOAD( "ic109.bin", 0x4000, 0x4000, 0xd54cf260 , 0xb9e6775c )	/* encrypted */
	ROM_LOAD( "7151.96", 0x8000, 0x4000, 0x51e6fc7e , 0xf2eeb0d8 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6593.62", 0x0000, 0x2000, 0xc6859747 , 0x2af9eaeb )
	ROM_LOAD( "6592.61", 0x2000, 0x2000, 0xb3e748af , 0x7804db86 )
	ROM_LOAD( "6591.64", 0x4000, 0x2000, 0xc0e0f1a2 , 0x79fd26f7 )
	ROM_LOAD( "6590.63", 0x6000, 0x2000, 0x1d5eb106 , 0xbf858cad )
	ROM_LOAD( "6589.66", 0x8000, 0x2000, 0x90532037 , 0x5ac9d205 )
	ROM_LOAD( "6588.65", 0xA000, 0x2000, 0x1db3f33b , 0xdc931dbb )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "6546.117", 0x0000, 0x4000, 0x111f4dd1 , 0xa4785692 )
	ROM_LOAD( "6548.04", 0x4000, 0x4000, 0x75cd29ef , 0xbdf278c1 )
	ROM_LOAD( "6547.110", 0x8000, 0x4000, 0xe45abeae , 0x34451b08 )
	ROM_LOAD( "6549.05", 0xc000, 0x4000, 0x640faed7 , 0xd2057668 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "6559.120", 0x0000, 0x2000, 0x1b6c3eb6 , 0x5a1570ee )
ROM_END

ROM_START( seganinu_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "7149.116", 0x0000, 0x4000, 0x94ccb6ae , 0xcd9fade7 )
	ROM_LOAD( "7150.109", 0x4000, 0x4000, 0x5699ffdd , 0xc36351e2 )
	ROM_LOAD( "7151.96", 0x8000, 0x4000, 0x51e6fc7e , 0xf2eeb0d8 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6593.62", 0x0000, 0x2000, 0xc6859747 , 0x2af9eaeb )
	ROM_LOAD( "6592.61", 0x2000, 0x2000, 0xb3e748af , 0x7804db86 )
	ROM_LOAD( "6591.64", 0x4000, 0x2000, 0xc0e0f1a2 , 0x79fd26f7 )
	ROM_LOAD( "6590.63", 0x6000, 0x2000, 0x1d5eb106 , 0xbf858cad )
	ROM_LOAD( "6589.66", 0x8000, 0x2000, 0x90532037 , 0x5ac9d205 )
	ROM_LOAD( "6588.65", 0xA000, 0x2000, 0x1db3f33b , 0xdc931dbb )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "6546.117", 0x0000, 0x4000, 0x111f4dd1 , 0xa4785692 )
	ROM_LOAD( "6548.04", 0x4000, 0x4000, 0x75cd29ef , 0xbdf278c1 )
	ROM_LOAD( "6547.110", 0x8000, 0x4000, 0xe45abeae , 0x34451b08 )
	ROM_LOAD( "6549.05", 0xc000, 0x4000, 0x640faed7 , 0xd2057668 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "6559.120", 0x0000, 0x2000, 0x1b6c3eb6 , 0x5a1570ee )
ROM_END

ROM_START( nprinces_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr6550.116", 0x0000, 0x4000, 0x60474411 , 0x5f6d59f1 )	/* encrypted */
	ROM_LOAD( "epr6551.109", 0x4000, 0x4000, 0xc347bfb1 , 0x1af133b2 )	/* encrypted */
	ROM_LOAD( "7151.96", 0x8000, 0x4000, 0x51e6fc7e , 0xf2eeb0d8 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6593.62", 0x0000, 0x2000, 0xc6859747 , 0x2af9eaeb )
	ROM_LOAD( "epr6557.61", 0x2000, 0x2000, 0xb50edcea , 0x6eb131d0 )
	ROM_LOAD( "6591.64", 0x4000, 0x2000, 0xc0e0f1a2 , 0x79fd26f7 )
	ROM_LOAD( "epr6555.63", 0x6000, 0x2000, 0xb3d904ad , 0x7f669aac )
	ROM_LOAD( "6589.66", 0x8000, 0x2000, 0x90532037 , 0x5ac9d205 )
	ROM_LOAD( "epr6553.65", 0xA000, 0x2000, 0x17c145f9 , 0xeb82a8fe )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "6546.117", 0x0000, 0x4000, 0x111f4dd1 , 0xa4785692 )
	ROM_LOAD( "6548.04", 0x4000, 0x4000, 0x75cd29ef , 0xbdf278c1 )
	ROM_LOAD( "6547.110", 0x8000, 0x4000, 0xe45abeae , 0x34451b08 )
	ROM_LOAD( "6549.05", 0xc000, 0x4000, 0x640faed7 , 0xd2057668 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "6559.120", 0x0000, 0x2000, 0x1b6c3eb6 , 0x5a1570ee )
ROM_END

ROM_START( nprinceb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "nprinces.001", 0x0000, 0x4000, 0xc5d77061 , 0xe0de073c )	/* encrypted */
	ROM_LOAD( "nprinces.002", 0x4000, 0x4000, 0xdbaf37b9 , 0x27219c7f )	/* encrypted */
	ROM_LOAD( "7151.96", 0x8000, 0x4000, 0x51e6fc7e , 0xf2eeb0d8 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6593.62", 0x0000, 0x2000, 0xc6859747 , 0x2af9eaeb )
	ROM_LOAD( "epr6557.61", 0x2000, 0x2000, 0xb50edcea , 0x6eb131d0 )
	ROM_LOAD( "6591.64", 0x4000, 0x2000, 0xc0e0f1a2 , 0x79fd26f7 )
	ROM_LOAD( "epr6555.63", 0x6000, 0x2000, 0xb3d904ad , 0x7f669aac )
	ROM_LOAD( "6589.66", 0x8000, 0x2000, 0x90532037 , 0x5ac9d205 )
	ROM_LOAD( "epr6553.65", 0xA000, 0x2000, 0x17c145f9 , 0xeb82a8fe )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "6546.117", 0x0000, 0x4000, 0x111f4dd1 , 0xa4785692 )
	ROM_LOAD( "6548.04", 0x4000, 0x4000, 0x75cd29ef , 0xbdf278c1 )
	ROM_LOAD( "6547.110", 0x8000, 0x4000, 0xe45abeae , 0x34451b08 )
	ROM_LOAD( "6549.05", 0xc000, 0x4000, 0x640faed7 , 0xd2057668 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "6559.120", 0x0000, 0x2000, 0x1b6c3eb6 , 0x5a1570ee )
ROM_END

ROM_START( imsorry_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr6676.116", 0x0000, 0x4000, 0x2fbc048a , 0xeb087d7f )	/* encrypted */
	ROM_LOAD( "epr6677.109", 0x4000, 0x4000, 0xf469b72f , 0xbd244bee )	/* encrypted */
	ROM_LOAD( "epr6678.96", 0x8000, 0x4000, 0xa0d133c9 , 0x2e16b9fd )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6684.u62", 0x0000, 0x2000, 0x187fed0f , 0x2c8df377 )
	ROM_LOAD( "epr6683.u61", 0x2000, 0x2000, 0xb8bc525c , 0x89431c48 )
	ROM_LOAD( "epr6682.u64", 0x4000, 0x2000, 0xe2a11725 , 0x256a9246 )
	ROM_LOAD( "epr6681.u63", 0x6000, 0x2000, 0x30eda313 , 0x6974d189 )
	ROM_LOAD( "epr6680.u66", 0x8000, 0x2000, 0x86eeddee , 0x10a629d6 )
	ROM_LOAD( "epr6674.u65", 0xA000, 0x2000, 0xe49ee552 , 0x143d883c )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "epr66xx.117", 0x0000, 0x4000, 0x74bbf555 , 0x1ba167ee )
	ROM_LOAD( "epr66xx.u04", 0x4000, 0x4000, 0x0d3cb5da , 0xedda7ad6 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr6656.113", 0x0000, 0x2000, 0x50a6ee34 , 0x25e3d685 )
ROM_END

ROM_START( imsorryj_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "6647.116", 0x0000, 0x4000, 0xc09620e6 , 0xcc5d915d )	/* encrypted */
	ROM_LOAD( "6648.109", 0x4000, 0x4000, 0x92b8ab6c , 0x37574d60 )	/* encrypted */
	ROM_LOAD( "6649.96", 0x8000, 0x4000, 0x20ce0e08 , 0x5f59bdee )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6655.62", 0x0000, 0x2000, 0x64084486 , 0xbe1f762f )
	ROM_LOAD( "6654.61", 0x2000, 0x2000, 0x356ddad7 , 0xed5f7fc8 )
	ROM_LOAD( "6653.64", 0x4000, 0x2000, 0xe49dfdc5 , 0x8b4845a7 )
	ROM_LOAD( "6652.63", 0x6000, 0x2000, 0xe01d697d , 0x001d68cb )
	ROM_LOAD( "6651.66", 0x8000, 0x2000, 0x88b2da3e , 0x4ee9b5e6 )
	ROM_LOAD( "6650.65", 0xA000, 0x2000, 0x3dbe9502 , 0x3fca4414 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "epr66xx.117", 0x0000, 0x4000, 0x74bbf555 , 0x1ba167ee )
	ROM_LOAD( "epr66xx.u04", 0x4000, 0x4000, 0x0d3cb5da , 0xedda7ad6 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr6656.113", 0x0000, 0x2000, 0x50a6ee34 , 0x25e3d685 )
ROM_END

ROM_START( teddybb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "6768.116", 0x0000, 0x4000, 0x3cf801d6 , 0x5939817e )	/* encrypted */
	ROM_LOAD( "6769.109", 0x4000, 0x4000, 0x81ca459c , 0x14a98ddd )	/* encrypted */
	ROM_LOAD( "6770.96", 0x8000, 0x4000, 0xb23d4b9b , 0x67b0c7c2 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6776.62", 0x0000, 0x2000, 0x49ceeb9a , 0xa0e5aca7 )
	ROM_LOAD( "6775.61", 0x2000, 0x2000, 0xa1971e53 , 0xcdb77e51 )
	ROM_LOAD( "6774.64", 0x4000, 0x2000, 0xa18e5f9c , 0x0cab75c3 )
	ROM_LOAD( "6773.63", 0x6000, 0x2000, 0xfca42ca6 , 0x0ef8d2cd )
	ROM_LOAD( "6772.66", 0x8000, 0x2000, 0xbe626a10 , 0xc33062b5 )
	ROM_LOAD( "6771.65", 0xA000, 0x2000, 0xd8e8b816 , 0xc457e8c5 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "6735.117", 0x0000, 0x4000, 0x80439145 , 0x1be35a97 )
	ROM_LOAD( "6737.004", 0x4000, 0x4000, 0x1db4d610 , 0x6b53aa7a )
	ROM_LOAD( "6736.110", 0x8000, 0x4000, 0x8759c703 , 0x565c25d0 )
	ROM_LOAD( "6738.005", 0xc000, 0x4000, 0x57a9291d , 0xe116285f )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "6748.120", 0x0000, 0x2000, 0xe5fa8a54 , 0xc2a1b89d )
ROM_END

/* This is the first System 8 game to have extended ROM space */
ROM_START( hvymetal_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "epra6790.1", 0x00000, 0x8000, 0xc7b4e9ae , 0x59195bb9 )	/* encrypted */
	ROM_LOAD( "epra6789.2", 0x10000, 0x8000, 0xd7edd4ed , 0x83e1d18a )
	ROM_LOAD( "epra6788.3", 0x18000, 0x8000, 0xc3b4cfc6 , 0x6ecefd57 )

	ROM_REGION_DISPOSE(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6795.62", 0x00000, 0x4000, 0x361894ca , 0x58a3d038 )
	ROM_LOAD( "epr6796.61", 0x04000, 0x4000, 0x32fde07b , 0xd8b08a55 )
	ROM_LOAD( "epr6793.64", 0x08000, 0x4000, 0xa9b9b26b , 0x487407c2 )
	ROM_LOAD( "epr6794.63", 0x0c000, 0x4000, 0xd0bc4542 , 0x89eb3793 )
	ROM_LOAD( "epr6791.66", 0x10000, 0x4000, 0x54e071c4 , 0xa7dcd042 )
	ROM_LOAD( "epr6792.65", 0x14000, 0x4000, 0xdf83ca4d , 0xd0be5e33 )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "epr6778.117", 0x00000, 0x8000, 0xc80535a9 , 0x0af61aee )
	ROM_LOAD( "epr6777.110", 0x08000, 0x8000, 0xb49c4224 , 0x91d7a197 )
	ROM_LOAD( "epr6780.4", 0x10000, 0x8000, 0xa9f0543c , 0x55b31df5 )
	ROM_LOAD( "epr6779.5", 0x18000, 0x8000, 0x6a5844fc , 0xe03a2b28 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr6787.120", 0x0000, 0x8000, 0x4399342f , 0xb64ac7f0 )

	ROM_REGION(0x0300)	/* color proms */
	ROM_LOAD( "pr7036.3", 0x0000, 0x0100, 0xebee0b08 , 0x146f16fb ) /* palette red component */
	ROM_LOAD( "pr7035.2", 0x0100, 0x0100, 0xd2d8000c , 0x50b201ed ) /* palette green component */
	ROM_LOAD( "pr7034.1", 0x0200, 0x0100, 0x5d550303 , 0xdfb5f139 ) /* palette blue component */
ROM_END

ROM_START( myhero_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr6963b.116", 0x0000, 0x4000, 0x7a64afc6 , 0x4daf89d4 )
	ROM_LOAD( "epr6964a.109", 0x4000, 0x4000, 0xffc41534 , 0xc26188e5 )
	ROM_LOAD( "epr6965.96", 0x8000, 0x4000, 0x0dd388ef , 0x3cbbaf64 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6966.u62", 0x0000, 0x2000, 0x6917061f , 0x157f0401 )
	ROM_LOAD( "epr6961.u61", 0x2000, 0x2000, 0x3b22387e , 0xbe53ce47 )
	ROM_LOAD( "epr6960.u64", 0x4000, 0x2000, 0xe2befe90 , 0xbd381baa )
	ROM_LOAD( "epr6959.u63", 0x6000, 0x2000, 0xce1dde31 , 0xbc04e79a )
	ROM_LOAD( "epr6958.u66", 0x8000, 0x2000, 0x9972d01a , 0x714f2c26 )
	ROM_LOAD( "epr6958.u65", 0xA000, 0x2000, 0x3888973e , 0x80920112 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "epr6921.117", 0x0000, 0x4000, 0x8fbbe309 , 0xf19e05a1 )
	ROM_LOAD( "epr6923.u04", 0x4000, 0x4000, 0x475b7a49 , 0x7988adc3 )
	ROM_LOAD( "epr6922.110", 0x8000, 0x4000, 0xb0224d0a , 0x37f77a78 )
	ROM_LOAD( "epr6924.u05", 0xc000, 0x4000, 0x18f73c5d , 0x42bdc8f6 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr69xx.120", 0x0000, 0x2000, 0x49f0862c , 0x0039e1e9 )
ROM_END

ROM_START( myheroj_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	/* all the three program ROMs have bits 0-1 swapped */
	/* when decoded, 07 is identical to the unencrypted one. The other */
	/* two are different. */
	ROM_LOAD( "ry-11.rom", 0x0000, 0x4000, 0x6077b465 , 0x6f4c8ee5 )	/* encrypted */
	ROM_LOAD( "ry-09.rom", 0x4000, 0x4000, 0x1f450dc5 , 0x369302a1 )	/* encrypted */
	ROM_LOAD( "ry-07.rom", 0x8000, 0x4000, 0x208b88ef , 0xb8e9922e )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	/* all three gfx ROMs have address lines A4 and A5 swapped, also #1 and #3 */
	/* have data lines D0 and D6 swapped, while #2 has data lines D1 and D5 swapped. */
	ROM_LOAD( "ry-04.rom", 0x0000, 0x4000, 0x9ed25b08 , 0xdfb75143 )
	ROM_LOAD( "ry-03.rom", 0x4000, 0x4000, 0x5c509e4e , 0xcf68b4a2 )
	ROM_LOAD( "ry-02.rom", 0x8000, 0x4000, 0x5b22fdb8 , 0xd100eaef )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "epr6921.117", 0x0000, 0x4000, 0x8fbbe309 , 0xf19e05a1 )
	ROM_LOAD( "epr6923.u04", 0x4000, 0x4000, 0x475b7a49 , 0x7988adc3 )
	ROM_LOAD( "epr6922.110", 0x8000, 0x4000, 0xb0224d0a , 0x37f77a78 )
	ROM_LOAD( "epr6924.u05", 0xc000, 0x4000, 0x18f73c5d , 0x42bdc8f6 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "ry-01.rom", 0x0000, 0x2000, 0x939e9d86 , 0xaf467223 )
ROM_END

ROM_START( chplft_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "7124.90", 0x00000, 0x8000, 0xe67e1670 , 0x678d5c41 )
	ROM_LOAD( "7125.91", 0x10000, 0x8000, 0x56d3a903 , 0xf5283498 )
	ROM_LOAD( "7126.92", 0x18000, 0x8000, 0x2f4c41fa , 0xdbd192ab )

	ROM_REGION_DISPOSE(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7127.4", 0x00000, 0x8000, 0x684454da , 0x1e708f6d )
	ROM_LOAD( "7128.5", 0x08000, 0x8000, 0x60640aac , 0xb922e787 )
	ROM_LOAD( "7129.6", 0x10000, 0x8000, 0x0e274493 , 0xbd3b6e6e )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "7121.87", 0x00000, 0x8000, 0xa4f7add7 , 0xf2b88f73 )
	ROM_LOAD( "7120.86", 0x08000, 0x8000, 0xb59f8d71 , 0x517d7fd3 )
	ROM_LOAD( "7123.89", 0x10000, 0x8000, 0x23911e63 , 0x8f16a303 )
	ROM_LOAD( "7122.88", 0x18000, 0x8000, 0x087b75e1 , 0x7c93f160 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "7130.126", 0x0000, 0x8000, 0xf3ed7509 , 0x346af118 )

	ROM_REGION(0x0300)	/* color proms */
	ROM_LOAD( "pr7119.20", 0x0000, 0x0100, 0xd3a30307 , 0xb2a8260f ) /* palette red component */
	ROM_LOAD( "pr7118.14", 0x0100, 0x0100, 0xb8d9080f , 0x693e20c7 ) /* palette green component */
	ROM_LOAD( "pr7117.8", 0x0200, 0x0100, 0xfafd090d , 0x4124307e ) /* palette blue component */
ROM_END

ROM_START( chplftb_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "7152.90", 0x00000, 0x8000, 0x59a80b20 , 0xfe49d83e )
	ROM_LOAD( "7153.91", 0x10000, 0x8000, 0xb6d3a903 , 0x48697666 )
	ROM_LOAD( "7154.92", 0x18000, 0x8000, 0x0c4c50fa , 0x56d6222a )

	ROM_REGION_DISPOSE(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7127.4", 0x00000, 0x8000, 0x684454da , 0x1e708f6d )
	ROM_LOAD( "7128.5", 0x08000, 0x8000, 0x60640aac , 0xb922e787 )
	ROM_LOAD( "7129.6", 0x10000, 0x8000, 0x0e274493 , 0xbd3b6e6e )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "7121.87", 0x00000, 0x8000, 0xa4f7add7 , 0xf2b88f73 )
	ROM_LOAD( "7120.86", 0x08000, 0x8000, 0xb59f8d71 , 0x517d7fd3 )
	ROM_LOAD( "7123.89", 0x10000, 0x8000, 0x23911e63 , 0x8f16a303 )
	ROM_LOAD( "7122.88", 0x18000, 0x8000, 0x087b75e1 , 0x7c93f160 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "7130.126", 0x0000, 0x8000, 0xf3ed7509 , 0x346af118 )

	ROM_REGION(0x0300)	/* color proms */
	ROM_LOAD( "pr7119.20", 0x0000, 0x0100, 0xd3a30307 , 0xb2a8260f ) /* palette red component */
	ROM_LOAD( "pr7118.14", 0x0100, 0x0100, 0xb8d9080f , 0x693e20c7 ) /* palette green component */
	ROM_LOAD( "pr7117.8", 0x0200, 0x0100, 0xfafd090d , 0x4124307e ) /* palette blue component */
ROM_END

ROM_START( chplftbl_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "7124bl.90", 0x00000, 0x8000, 0xaa09ffb1 , 0x71a37932 )
	ROM_LOAD( "7125.91", 0x10000, 0x8000, 0x56d3a903 , 0xf5283498 )
	ROM_LOAD( "7126.92", 0x18000, 0x8000, 0x2f4c41fa , 0xdbd192ab )

	ROM_REGION_DISPOSE(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7127.4", 0x00000, 0x8000, 0x684454da , 0x1e708f6d )
	ROM_LOAD( "7128.5", 0x08000, 0x8000, 0x60640aac , 0xb922e787 )
	ROM_LOAD( "7129.6", 0x10000, 0x8000, 0x0e274493 , 0xbd3b6e6e )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "7121.87", 0x00000, 0x8000, 0xa4f7add7 , 0xf2b88f73 )
	ROM_LOAD( "7120.86", 0x08000, 0x8000, 0xb59f8d71 , 0x517d7fd3 )
	ROM_LOAD( "7123.89", 0x10000, 0x8000, 0x23911e63 , 0x8f16a303 )
	ROM_LOAD( "7122.88", 0x18000, 0x8000, 0x087b75e1 , 0x7c93f160 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "7130.126", 0x0000, 0x8000, 0xf3ed7509 , 0x346af118 )

	ROM_REGION(0x0300)	/* color proms */
	ROM_LOAD( "pr7119.20", 0x0000, 0x0100, 0xd3a30307 , 0xb2a8260f ) /* palette red component */
	ROM_LOAD( "pr7118.14", 0x0100, 0x0100, 0xb8d9080f , 0x693e20c7 ) /* palette green component */
	ROM_LOAD( "pr7117.8", 0x0200, 0x0100, 0xfafd090d , 0x4124307e ) /* palette blue component */
ROM_END

ROM_START( fdwarrio_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "4d.116", 0x0000, 0x4000, 0x8261e97f , 0x546d1bc7 )	/* encrypted */
	ROM_LOAD( "4d.109", 0x4000, 0x4000, 0xa0d7938d , 0xf1074ec3 )	/* encrypted */
	ROM_LOAD( "4d.96", 0x8000, 0x4000, 0xf3ddce83 , 0x387c1e8f )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "4d.62", 0x0000, 0x2000, 0x0428bda8 , 0xf31b2e09 )
	ROM_LOAD( "4d.61", 0x2000, 0x2000, 0x3dee387e , 0x5430e925 )
	ROM_LOAD( "4d.64", 0x4000, 0x2000, 0x43c0a3a6 , 0x9f442351 )
	ROM_LOAD( "4d.63", 0x6000, 0x2000, 0xa9ada417 , 0x633232bd )
	ROM_LOAD( "4d.66", 0x8000, 0x2000, 0x2853676f , 0x52bfa2ed )
	ROM_LOAD( "4d.65", 0xA000, 0x2000, 0x11c3b5e9 , 0xe9ba4658 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "4d.117", 0x0000, 0x4000, 0x0247b191 , 0x436e4141 )
	ROM_LOAD( "4d.04", 0x4000, 0x4000, 0xd8f9a681 , 0x8b7cecef )
	ROM_LOAD( "4d.110", 0x8000, 0x4000, 0xa789e6d1 , 0x6ec5990a )
	ROM_LOAD( "4d.05", 0xc000, 0x4000, 0x412f7991 , 0xf31a1e6a )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "4d.120", 0x0000, 0x2000, 0x81a67906 , 0x5241c009 )
ROM_END

ROM_START( brain_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "brain.1", 0x00000, 0x8000, 0x1fbe0c62 , 0x2d2aec31 )
	ROM_LOAD( "brain.2", 0x10000, 0x8000, 0x43aa91dc , 0x810a8ab5 )
	ROM_LOAD( "brain.3", 0x18000, 0x8000, 0x501df841 , 0x9a225634 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "brain.62", 0x0000, 0x4000, 0x50c638f2 , 0x7dce2302 )
	ROM_LOAD( "brain.64", 0x4000, 0x4000, 0x6c14b6c2 , 0x7ce03fd3 )
	ROM_LOAD( "brain.66", 0x8000, 0x4000, 0x59959253 , 0xea54323f )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "brain.117", 0x00000, 0x8000, 0x97c54879 , 0x92ff71a4 )
	ROM_LOAD( "brain.110", 0x08000, 0x8000, 0x59b14c67 , 0xa1b847ec )
	ROM_LOAD( "brain.4", 0x10000, 0x8000, 0x29e2f272 , 0xfd2ea53b )
	/* 18000-1ffff empty */

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "brain.120", 0x0000, 0x8000, 0x9f3369d9 , 0xc7e50278 )
ROM_END

ROM_START( wboy_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "b-1.bin", 0x0000, 0x4000, 0x91dcaa98 , 0x51d27534 )	/* encrypted */
	ROM_LOAD( "b-2.bin", 0x4000, 0x4000, 0xa584a3e6 , 0xe29d1cd1 )	/* encrypted */
	ROM_LOAD( "epr7491.96", 0x8000, 0x4000, 0xfc09e47f , 0x1f7d0efe )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6497.62", 0x0000, 0x2000, 0xfc185d64 , 0x08d609ca )
	ROM_LOAD( "epr7496.61", 0x2000, 0x2000, 0x1ccca0e4 , 0x6f61fdf1 )
	ROM_LOAD( "epr7495.64", 0x4000, 0x2000, 0xfe09faaf , 0x6a0d2c2d )
	ROM_LOAD( "epr7494.63", 0x6000, 0x2000, 0x1605383f , 0xa8e281c7 )
	ROM_LOAD( "epr7493.66", 0x8000, 0x2000, 0xd1dd427b , 0x89305df4 )
	ROM_LOAD( "epr7492.65", 0xA000, 0x2000, 0xebb3ae77 , 0x60f806b1 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "epr7485.117", 0x0000, 0x4000, 0x52dcb9e2 , 0xc2891722 )
	ROM_LOAD( "epr7487.04", 0x4000, 0x4000, 0x0833e9fd , 0x2d3a421b )
	ROM_LOAD( "epr7486.110", 0x8000, 0x4000, 0x556d3fa9 , 0x8d622c50 )
	ROM_LOAD( "epr7488.05", 0xc000, 0x4000, 0x744aca0e , 0x007c2f1b )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "ic120_98.bin", 0x0000, 0x2000, 0xb6d5585b , 0x78ae1e7b )
ROM_END

ROM_START( wboy2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "wb_1", 0x0000, 0x4000, 0x7820981c , 0xbd6fef49 )	/* encrypted */
	ROM_LOAD( "wb_2", 0x4000, 0x4000, 0x0e7ab2c0 , 0x4081b624 )	/* encrypted */
	ROM_LOAD( "wb_3", 0x8000, 0x4000, 0xca3d2dd5 , 0xc48a0e36 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6497.62", 0x0000, 0x2000, 0xfc185d64 , 0x08d609ca )
	ROM_LOAD( "epr7496.61", 0x2000, 0x2000, 0x1ccca0e4 , 0x6f61fdf1 )
	ROM_LOAD( "epr7495.64", 0x4000, 0x2000, 0xfe09faaf , 0x6a0d2c2d )
	ROM_LOAD( "epr7494.63", 0x6000, 0x2000, 0x1605383f , 0xa8e281c7 )
	ROM_LOAD( "epr7493.66", 0x8000, 0x2000, 0xd1dd427b , 0x89305df4 )
	ROM_LOAD( "epr7492.65", 0xA000, 0x2000, 0xebb3ae77 , 0x60f806b1 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "epr7485.117", 0x0000, 0x4000, 0x52dcb9e2 , 0xc2891722 )
	ROM_LOAD( "epr7487.04", 0x4000, 0x4000, 0x0833e9fd , 0x2d3a421b )
	ROM_LOAD( "epr7486.110", 0x8000, 0x4000, 0x556d3fa9 , 0x8d622c50 )
	ROM_LOAD( "epr7488.05", 0xc000, 0x4000, 0x744aca0e , 0x007c2f1b )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "ic120_98.bin", 0x0000, 0x2000, 0xb6d5585b , 0x78ae1e7b )
ROM_END

ROM_START( wboy3_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr7489.116", 0x0000, 0x4000, 0xaa6bad05 , 0x130f4b70 )	/* encrypted */
	ROM_LOAD( "epr7490.109", 0x4000, 0x4000, 0xe495cb13 , 0x9e656733 )	/* encrypted */
	ROM_LOAD( "epr7491.96", 0x8000, 0x4000, 0xfc09e47f , 0x1f7d0efe )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6497.62", 0x0000, 0x2000, 0xfc185d64 , 0x08d609ca )
	ROM_LOAD( "epr7496.61", 0x2000, 0x2000, 0x1ccca0e4 , 0x6f61fdf1 )
	ROM_LOAD( "epr7495.64", 0x4000, 0x2000, 0xfe09faaf , 0x6a0d2c2d )
	ROM_LOAD( "epr7494.63", 0x6000, 0x2000, 0x1605383f , 0xa8e281c7 )
	ROM_LOAD( "epr7493.66", 0x8000, 0x2000, 0xd1dd427b , 0x89305df4 )
	ROM_LOAD( "epr7492.65", 0xA000, 0x2000, 0xebb3ae77 , 0x60f806b1 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "epr7485.117", 0x0000, 0x4000, 0x52dcb9e2 , 0xc2891722 )
	ROM_LOAD( "epr7487.04", 0x4000, 0x4000, 0x0833e9fd , 0x2d3a421b )
	ROM_LOAD( "epr7486.110", 0x8000, 0x4000, 0x556d3fa9 , 0x8d622c50 )
	ROM_LOAD( "epr7488.05", 0xc000, 0x4000, 0x744aca0e , 0x007c2f1b )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epra7498.120", 0x0000, 0x2000, 0xb2d5545b , 0xc198205c )
ROM_END

ROM_START( wboy4_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic129", 0x0000, 0x2000, 0xadd78619 , 0x1bbb7354 )	/* encrypted */
	ROM_LOAD( "ic130", 0x2000, 0x2000, 0xfbcf7dc1 , 0x21007413 )	/* encrypted */
	ROM_LOAD( "ic131", 0x4000, 0x2000, 0xe168c68c , 0x44b30433 )	/* encrypted */
	ROM_LOAD( "ic132", 0x6000, 0x2000, 0x474e18c2 , 0xbb525a0b )	/* encrypted */
	ROM_LOAD( "ic133", 0x8000, 0x2000, 0x63bc5696 , 0x8379aa23 )
	ROM_LOAD( "ic134", 0xa000, 0x2000, 0x5664d752 , 0xc767a5d7 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6497.62", 0x0000, 0x2000, 0xfc185d64 , 0x08d609ca )
	ROM_LOAD( "epr7496.61", 0x2000, 0x2000, 0x1ccca0e4 , 0x6f61fdf1 )
	ROM_LOAD( "epr7495.64", 0x4000, 0x2000, 0xfe09faaf , 0x6a0d2c2d )
	ROM_LOAD( "epr7494.63", 0x6000, 0x2000, 0x1605383f , 0xa8e281c7 )
	ROM_LOAD( "epr7493.66", 0x8000, 0x2000, 0xd1dd427b , 0x89305df4 )
	ROM_LOAD( "epr7492.65", 0xA000, 0x2000, 0xebb3ae77 , 0x60f806b1 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "epr7485.117", 0x0000, 0x4000, 0x52dcb9e2 , 0xc2891722 )
	ROM_LOAD( "epr7487.04", 0x4000, 0x4000, 0x0833e9fd , 0x2d3a421b )
	ROM_LOAD( "epr7486.110", 0x8000, 0x4000, 0x556d3fa9 , 0x8d622c50 )
	ROM_LOAD( "epr7488.05", 0xc000, 0x4000, 0x744aca0e , 0x007c2f1b )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr7502", 0x0000, 0x2000, 0x369e1e2a , 0xc92484b3 )
ROM_END

ROM_START( wboyu_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic116_89.bin", 0x0000, 0x4000, 0x508e75d4 , 0x73d8cef0 )
	ROM_LOAD( "ic109_90.bin", 0x4000, 0x4000, 0xf938a950 , 0x29546828 )
	ROM_LOAD( "ic096_91.bin", 0x8000, 0x4000, 0x56aa06ca , 0xc7145c2a )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6497.62", 0x0000, 0x2000, 0xfc185d64 , 0x08d609ca )
	ROM_LOAD( "epr7496.61", 0x2000, 0x2000, 0x1ccca0e4 , 0x6f61fdf1 )
	ROM_LOAD( "epr7495.64", 0x4000, 0x2000, 0xfe09faaf , 0x6a0d2c2d )
	ROM_LOAD( "epr7494.63", 0x6000, 0x2000, 0x1605383f , 0xa8e281c7 )
	ROM_LOAD( "epr7493.66", 0x8000, 0x2000, 0xd1dd427b , 0x89305df4 )
	ROM_LOAD( "epr7492.65", 0xA000, 0x2000, 0xebb3ae77 , 0x60f806b1 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "ic117_85.bin", 0x0000, 0x4000, 0xfca06cf0 , 0x1ee96ae8 )
	ROM_LOAD( "ic004_87.bin", 0x4000, 0x4000, 0x37267b00 , 0x119735bb )
	ROM_LOAD( "ic110_86.bin", 0x8000, 0x4000, 0x2d6b5bab , 0x26d0fac4 )
	ROM_LOAD( "ic005_88.bin", 0xc000, 0x4000, 0x43b5917d , 0x2602e519 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "ic120_98.bin", 0x0000, 0x2000, 0xb6d5585b , 0x78ae1e7b )
ROM_END

ROM_START( wboy4u_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic129_02.bin", 0x0000, 0x2000, 0x2c061050 , 0x32c4b709 )
	ROM_LOAD( "ic130_03.bin", 0x2000, 0x2000, 0x620b2985 , 0x56463ede )
	ROM_LOAD( "ic131_04.bin", 0x4000, 0x2000, 0x8b959389 , 0x775ed392 )
	ROM_LOAD( "ic132_05.bin", 0x6000, 0x2000, 0x95cc4dd6 , 0x7b922708 )
	ROM_LOAD( "ic133", 0x8000, 0x2000, 0x63bc5696 , 0x8379aa23 )
	ROM_LOAD( "ic134", 0xa000, 0x2000, 0x5664d752 , 0xc767a5d7 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6497.62", 0x0000, 0x2000, 0xfc185d64 , 0x08d609ca )
	ROM_LOAD( "epr7496.61", 0x2000, 0x2000, 0x1ccca0e4 , 0x6f61fdf1 )
	ROM_LOAD( "epr7495.64", 0x4000, 0x2000, 0xfe09faaf , 0x6a0d2c2d )
	ROM_LOAD( "epr7494.63", 0x6000, 0x2000, 0x1605383f , 0xa8e281c7 )
	ROM_LOAD( "epr7493.66", 0x8000, 0x2000, 0xd1dd427b , 0x89305df4 )
	ROM_LOAD( "epr7492.65", 0xA000, 0x2000, 0xebb3ae77 , 0x60f806b1 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "epr7485.117", 0x0000, 0x4000, 0x52dcb9e2 , 0xc2891722 )
	ROM_LOAD( "epr7487.04", 0x4000, 0x4000, 0x0833e9fd , 0x2d3a421b )
	ROM_LOAD( "epr7486.110", 0x8000, 0x4000, 0x556d3fa9 , 0x8d622c50 )
	ROM_LOAD( "epr7488.05", 0xc000, 0x4000, 0x744aca0e , 0x007c2f1b )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epra7498.120", 0x0000, 0x2000, 0xb2d5545b , 0xc198205c )
ROM_END

ROM_START( wbdeluxe_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "wbd1.bin", 0x0000, 0x2000, 0xfc881b62 , 0xa1bedbd7 )
	ROM_LOAD( "ic130_03.bin", 0x2000, 0x2000, 0x620b2985 , 0x56463ede )
	ROM_LOAD( "wbd3.bin", 0x4000, 0x2000, 0x16949c5e , 0x6fcdbd4c )
	ROM_LOAD( "ic132_05.bin", 0x6000, 0x2000, 0x95cc4dd6 , 0x7b922708 )
	ROM_LOAD( "wbd5.bin", 0x8000, 0x2000, 0xbb73e2a9 , 0xf6b02902 )
	ROM_LOAD( "wbd6.bin", 0xa000, 0x2000, 0xc57f0e3b , 0x43df21fe )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr6497.62", 0x0000, 0x2000, 0xfc185d64 , 0x08d609ca )
	ROM_LOAD( "epr7496.61", 0x2000, 0x2000, 0x1ccca0e4 , 0x6f61fdf1 )
	ROM_LOAD( "epr7495.64", 0x4000, 0x2000, 0xfe09faaf , 0x6a0d2c2d )
	ROM_LOAD( "epr7494.63", 0x6000, 0x2000, 0x1605383f , 0xa8e281c7 )
	ROM_LOAD( "epr7493.66", 0x8000, 0x2000, 0xd1dd427b , 0x89305df4 )
	ROM_LOAD( "epr7492.65", 0xA000, 0x2000, 0xebb3ae77 , 0x60f806b1 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "epr7485.117", 0x0000, 0x4000, 0x52dcb9e2 , 0xc2891722 )
	ROM_LOAD( "epr7487.04", 0x4000, 0x4000, 0x0833e9fd , 0x2d3a421b )
	ROM_LOAD( "epr7486.110", 0x8000, 0x4000, 0x556d3fa9 , 0x8d622c50 )
	ROM_LOAD( "epr7488.05", 0xc000, 0x4000, 0x744aca0e , 0x007c2f1b )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epra7498.120", 0x0000, 0x2000, 0xb2d5545b , 0xc198205c )
ROM_END

ROM_START( gardia_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "epr10255.1", 0x00000, 0x8000, 0xd27181fd , 0x89282a6b )
	ROM_LOAD( "epr10254.2", 0x10000, 0x8000, 0xe87cfa94 , 0x2826b6d8 )
	ROM_LOAD( "epr10253.3", 0x18000, 0x8000, 0x53bf8ae1 , 0x7911260f )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr10249.61", 0x0000, 0x4000, 0x6031fc8d , 0x4e0ad0f2 )
	ROM_LOAD( "epr10248.64", 0x4000, 0x4000, 0x5b84a076 , 0x3515d124 )
	ROM_LOAD( "epr10247.66", 0x8000, 0x4000, 0x33d62274 , 0x541e1555 )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "epr10234.117", 0x00000, 0x8000, 0xa667b1f1 , 0x8a6aed33 )
	ROM_LOAD( "epr10233.110", 0x08000, 0x8000, 0x90f0a54e , 0xc52784d3 )
	ROM_LOAD( "epr10236.04", 0x10000, 0x8000, 0x2999f48b , 0xb35ab227 )
	ROM_LOAD( "epr10235.5", 0x18000, 0x8000, 0xa12ac992 , 0x006a3151 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr10243.120", 0x0000, 0x8000, 0x6463ec3f , 0x726a3fdb )

	ROM_REGION(0x0300)	/* color proms */
	ROM_LOAD( "bprom.3", 0x0000, 0x0100, 0xc6c00600 , 0x8eee0f72 ) /* palette red component */
	ROM_LOAD( "bprom.2", 0x0100, 0x0100, 0x46400200 , 0x3e7babd7 ) /* palette green component */
	ROM_LOAD( "bprom.1", 0x0200, 0x0100, 0x2e200e00 , 0x371c44a6 ) /* palette blue component */
ROM_END

ROM_START( blockgal_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "bg.116", 0x0000, 0x4000, 0xd9f83d2c , 0xa99b231a )	/* encrypted */
	ROM_LOAD( "bg.109", 0x4000, 0x4000, 0x59967640 , 0xa6b573d5 )	/* encrypted */
	ROM_LOAD( "bg.96", 0x8000, 0x4000, 0x0dd388ef , 0x3cbbaf64 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bg.62", 0x0000, 0x2000, 0xd5026988 , 0x7e3ea4eb )
	ROM_LOAD( "bg.61", 0x2000, 0x2000, 0x8b9b42c7 , 0x4dd3d39d )
	ROM_LOAD( "bg.64", 0x4000, 0x2000, 0x9cd3875b , 0x17368663 )
	ROM_LOAD( "bg.63", 0x6000, 0x2000, 0x226375d7 , 0x0c8bc404 )
	ROM_LOAD( "bg.66", 0x8000, 0x2000, 0x74b15211 , 0x2b7dc4fa )
	ROM_LOAD( "bg.65", 0xA000, 0x2000, 0xd59b48c3 , 0xed121306 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "bg.117", 0x0000, 0x4000, 0xca3fb861 , 0xe99cc920 )
	ROM_LOAD( "bg.04", 0x4000, 0x4000, 0xf6589fc8 , 0x213057f8 )
	ROM_LOAD( "bg.110", 0x8000, 0x4000, 0x11d569a3 , 0x064c812c )
	ROM_LOAD( "bg.05", 0xc000, 0x4000, 0xe813a0e9 , 0x02e0b040 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "bg.120", 0x0000, 0x2000, 0x3dc04530 , 0xd848faff )
ROM_END

ROM_START( tokisens_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "epr10961.90", 0x00000, 0x8000, 0x7f57d065 , 0x1466b61d )
	ROM_LOAD( "epr10962.91", 0x10000, 0x8000, 0xab34cede , 0xa8479f91 )
	ROM_LOAD( "epr10963.92", 0x18000, 0x8000, 0x5fe40000 , 0xb7193b39 )

	ROM_REGION_DISPOSE(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr10964.4", 0x00000, 0x8000, 0xd83cdf96 , 0x9013b85c )
	ROM_LOAD( "epr10965.5", 0x08000, 0x8000, 0x716ac43c , 0xe4755cc6 )
	ROM_LOAD( "epr10966.6", 0x10000, 0x8000, 0xe380cfc6 , 0x5bbfbdcc )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "epr10958.87", 0x00000, 0x8000, 0xce667e66 , 0xfc2bcbd7 )
	ROM_LOAD( "epr10957.86", 0x08000, 0x8000, 0x01edf6a3 , 0x4ec56860 )
	ROM_LOAD( "epr10960.89", 0x10000, 0x8000, 0xdc1b08a9 , 0x880e0d44 )
	ROM_LOAD( "epr10959.88", 0x18000, 0x8000, 0xaf05c491 , 0x4deda48f )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr10967.126", 0x0000, 0x8000, 0xdc1a8dba , 0x97966bf2 )

	ROM_REGION(0x0300)	/* color proms */
	ROM_LOAD( "bprom.20", 0x0000, 0x0100, 0xc6c00600 , 0x8eee0f72 ) /* palette red component */
	ROM_LOAD( "bprom.14", 0x0100, 0x0100, 0x46400200 , 0x3e7babd7 ) /* palette green component */
	ROM_LOAD( "bprom.8", 0x0200, 0x0100, 0x2e200e00 , 0x371c44a6 ) /* palette blue component */
ROM_END

ROM_START( dakkochn_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "epr11224.90", 0x00000, 0x8000, 0x77ecb8d0 , 0x9fb1972b )	/* encrypted */
	ROM_LOAD( "epr11225.91", 0x10000, 0x8000, 0xd4c496aa , 0xc540f9e2 )	/* encrypted */
	/* 18000-1ffff empty */

	ROM_REGION_DISPOSE(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr11226.4", 0x00000, 0x8000, 0x13dd3b9f , 0x3dbc2f78 )
	ROM_LOAD( "epr11227.5", 0x08000, 0x8000, 0xe0053dab , 0x34156e8d )
	ROM_LOAD( "epr11228.6", 0x10000, 0x8000, 0x0fa54759 , 0xfdd5323f )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "epr11221.87", 0x00000, 0x8000, 0x7d28997e , 0xf9a44916 )
	ROM_LOAD( "epr11220.86", 0x08000, 0x8000, 0x4c316dd3 , 0xfdd25d8a )
	ROM_LOAD( "epr11223.89", 0x10000, 0x8000, 0x8ad626b0 , 0x538adc55 )
	ROM_LOAD( "epr11222.88", 0x18000, 0x8000, 0xee708cb2 , 0x33fab0b2 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr11229.126", 0x0000, 0x8000, 0xf4346f90 , 0xc11648d0 )

	ROM_REGION(0x0300)	/* color proms */
	ROM_LOAD( "pr11219.20", 0x0000, 0x0100, 0x7d090d07 , 0x45e252d9 ) /* palette red component */
	ROM_LOAD( "pr11218.14", 0x0100, 0x0100, 0xf081050d , 0x3eda3a1b ) /* palette green component */
	ROM_LOAD( "pr11217.8", 0x0200, 0x0100, 0xc2cb0a03 , 0x49dbde88 ) /* palette blue component */
ROM_END

ROM_START( ufosensi_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "epr11661.90", 0x00000, 0x8000, 0x625be2af , 0xf3e394e2 )	/* encrypted */
	ROM_LOAD( "epr11662.91", 0x10000, 0x8000, 0x7784e02a , 0x0c2e4120 )	/* encrypted */
	ROM_LOAD( "epr11663.92", 0x18000, 0x8000, 0xeb0c5be8 , 0x4515ebae )	/* encrypted */

	ROM_REGION_DISPOSE(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr11664.4", 0x00000, 0x8000, 0xb953b363 , 0x1b1bc3d5 )
	ROM_LOAD( "epr11665.5", 0x08000, 0x8000, 0xfef4c036 , 0x3659174a )
	ROM_LOAD( "epr11666.6", 0x10000, 0x8000, 0x280d0c83 , 0x99dcc793 )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "epr11658.87", 0x00000, 0x8000, 0xb0c81114 , 0x3b5a20f7 )
	ROM_LOAD( "epr11657.86", 0x08000, 0x8000, 0x475fc523 , 0x010f81a9 )
	ROM_LOAD( "epr11660.89", 0x10000, 0x8000, 0x92c921fd , 0xe1e2e7c5 )
	ROM_LOAD( "epr11659.88", 0x18000, 0x8000, 0xdf74e5f8 , 0x286c7286 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr11667.126", 0x0000, 0x8000, 0xdf98aede , 0x110baba9 )

	ROM_REGION(0x0300)	/* color proms */
	ROM_LOAD( "pr11656.20", 0x0000, 0x0100, 0x5e760a04 , 0x640740eb ) /* palette red component */
	ROM_LOAD( "pr11655.14", 0x0100, 0x0100, 0xb0af0301 , 0xa0c3fa77 ) /* palette green component */
	ROM_LOAD( "pr11654.8", 0x0200, 0x0100, 0xc2d20706 , 0xba624305 ) /* palette blue component */
ROM_END

ROM_START( wbml_rom )
	ROM_REGION(0x40000)	/* 256k for code */
	ROM_LOAD( "wbml.01", 0x20000, 0x8000, 0xa5329b82 , 0x66482638 )	/* Unencrypted opcodes */
	ROM_CONTINUE(        0x00000, 0x8000 )              /* Now load the operands in RAM */
	ROM_LOAD( "wbml.02", 0x30000, 0x8000, 0xcdafb89f , 0x48746bb6 )	/* Unencrypted opcodes */
	ROM_CONTINUE(        0x10000, 0x8000 )
	ROM_LOAD( "wbml.03", 0x38000, 0x8000, 0x31cd6733 , 0xd57ba8aa )	/* Unencrypted opcodes */
	ROM_CONTINUE(        0x18000, 0x8000 )

	ROM_REGION_DISPOSE(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "wbml.08", 0x00000, 0x8000, 0xef89ac27 , 0xbbea6afe )
	ROM_LOAD( "wbml.09", 0x08000, 0x8000, 0x60a6011e , 0x77567d41 )
	ROM_LOAD( "wbml.10", 0x10000, 0x8000, 0x563078ba , 0xa52ffbdd )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "wbml.05", 0x00000, 0x8000, 0x7d7dd491 , 0xaf0b3972 )
	ROM_LOAD( "wbml.04", 0x08000, 0x8000, 0x5ba6090a , 0x277d8f1d )
	ROM_LOAD( "wbml.07", 0x10000, 0x8000, 0xf3a6277a , 0xf05ffc76 )
	ROM_LOAD( "wbml.06", 0x18000, 0x8000, 0x387db381 , 0xcedc9c61 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "wbml.11", 0x0000, 0x8000, 0x8057f681 , 0x7a4ee585 )

	ROM_REGION(0x0300)	/* color proms */
	ROM_LOAD( "prom3.bin", 0x0000, 0x0100, 0xbbbe0406 , 0x27057298 )
	ROM_LOAD( "prom2.bin", 0x0100, 0x0100, 0x6261050f , 0x41e4d86b )
	ROM_LOAD( "prom1.bin", 0x0200, 0x0100, 0xb0bd0401 , 0xbad2bda1 )
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



static int starjack_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xc0e1],"\x00\x00\x03",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xc0e1],30);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xc0db] = RAM[0xc0e1];
                        RAM[0xc0dc] = RAM[0xc0e2];
                        RAM[0xc0dd] = RAM[0xc0e3];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void starjack_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xc0e1],30);
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


static int mrviking_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xd300],"\x00\x00\x02",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xd300],21);
                        osd_fread(f,&RAM[0xd42c],21);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xc086] = RAM[0xd300];
                        RAM[0xc087] = RAM[0xd301];
                        RAM[0xc088] = RAM[0xd302];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void mrviking_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xd300],21);
                osd_fwrite(f,&RAM[0xd42c],21);
		osd_fclose(f);
	}
}


static int flicky_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xe700],"\x00\x10\x00",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xe700],49);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xC0d5] = RAM[0xe700];
                        RAM[0xC0d6] = RAM[0xe701];
                        RAM[0xC0d7] = RAM[0xe702];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void flicky_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xe700],49);
		osd_fclose(f);
	}
}


static int bullfgtj_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xd300],"\x00\x00\x02",3) == 0 &&
			memcmp(&RAM[0xd339],"\x48\x2e\x49",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xd300],6*10);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xC014] = RAM[0xD300];
                        RAM[0xC015] = RAM[0xD301];
                        RAM[0xC016] = RAM[0xD302];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void bullfgtj_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xd300],6*10);
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


static int seganinj_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xEF2E],"\x54\x41\x43\x00",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xEF01],48);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void seganinj_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xEF01],48);
		osd_fclose(f);
	}
}


static int imsorry_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xd300],"\x00\x20\x00",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xd300],0x16*10);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xC017] = RAM[0xD300];
                        RAM[0xC018] = RAM[0xD301];
                        RAM[0xC019] = RAM[0xD302];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void imsorry_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xd300],0x16*10);
		osd_fclose(f);
	}
}

static int fdwarrio_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xd300],"\x00\x00\x02",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xd300],0x16*10);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xC017] = RAM[0xD300];
                        RAM[0xC018] = RAM[0xD301];
                        RAM[0xC019] = RAM[0xD302];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static int teddybb_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xef03],"\x00\x10\x00",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xef03],49);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xC578] = RAM[0xef05];
                        RAM[0xC579] = RAM[0xef04];
                        RAM[0xC57a] = RAM[0xef03];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void teddybb_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xef03],49);
		osd_fclose(f);
	}
}


static int myhero_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xd339],"\x59\x2E\x49\x00",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xD300],61);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
			RAM[0xC017] = RAM[0xD300];
			RAM[0xC018] = RAM[0xD301];
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void myhero_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xD300],61);
		osd_fclose(f);
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


static int chplft_hiload(void)
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

static void chplft_hisave(void)
{
	void *f;
	unsigned char *choplifter_ram = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&choplifter_ram[0xEF00],49);
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



#define BASE_CREDITS "Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)"

struct GameDriver starjack_driver =
{
	__FILE__,
	0,
	"starjack",
	"Star Jacker (Sega)",
	"1983",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_small_machine_driver,

	starjack_rom,
	0, 0,					/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	starjack_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	starjack_hiload, starjack_hisave
};

struct GameDriver starjacs_driver =
{
	__FILE__,
	&starjack_driver,
	"starjacs",
	"Star Jacker (Stern)",
	"1983",
	"Stern",
	BASE_CREDITS,
	0,
	&system8_small_machine_driver,

	starjacs_rom,
	0, 0,					/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	starjacs_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver regulus_driver =
{
	__FILE__,
	0,
	"regulus",
	"Regulus",
	"1983",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	regulus_rom,
	0, regulus_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	regulus_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver upndown_driver =
{
	__FILE__,
	0,
	"upndown",
	"Up'n Down",
	"1983",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	upndown_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	upndown_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	upndown_hiload, upndown_hisave
};

struct GameDriver mrviking_driver =
{
	__FILE__,
	0,
	"mrviking",
	"Mister Viking",
	"1984",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_small_machine_driver,

	mrviking_rom,
	0, mrviking_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	mrviking_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	mrviking_hiload, mrviking_hisave
};

struct GameDriver swat_driver =
{
	__FILE__,
	0,
	"swat",
	"SWAT",
	"1984",
	"Coreland / Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	swat_rom,
	0, swat_decode,			/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	swat_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver flicky_driver =
{
	__FILE__,
	0,
	"flicky",
	"Flicky (set 1)",
	"1984",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	flicky_rom,
	0, flicky_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	flicky_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	flicky_hiload, flicky_hisave
};

struct GameDriver flicky2_driver =
{
	__FILE__,
	&flicky_driver,
	"flicky2",
	"Flicky (set 2)",
	"1984",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	flicky2_rom,
	0, flicky_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	flicky_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	flicky_hiload, flicky_hisave
};

struct GameDriver bullfgtj_driver =
{
	__FILE__,
	0,
	"bullfgtj",
	"Bull Fight (Japan)",
	"1984",
	"Sega / Coreland",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	bullfgtj_rom,
	0, bullfgtj_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	bullfgtj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	bullfgtj_hiload, bullfgtj_hisave
};

struct GameDriver pitfall2_driver =
{
	__FILE__,
	0,
	"pitfall2",
	"Pitfall II",
	"1985",
	"Sega",
	BASE_CREDITS,
	0,
	&pitfall2_machine_driver,

	pitfall2_rom,
	0, pitfall2_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	pitfall2_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	pitfall2_hiload, pitfall2_hisave
};

struct GameDriver pitfallu_driver =
{
	__FILE__,
	&pitfall2_driver,
	"pitfallu",
	"Pitfall II (not encrypted)",
	"1985",
	"Sega",
	BASE_CREDITS,
	0,
	&pitfall2_machine_driver,

	pitfallu_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	pitfallu_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	pitfall2_hiload, pitfall2_hisave
};

struct GameDriver seganinj_driver =
{
	__FILE__,
	0,
	"seganinj",
	"Sega Ninja",
	"1985",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	seganinj_rom,
	0, seganinj_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	seganinj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	seganinj_hiload, seganinj_hisave
};

struct GameDriver seganinu_driver =
{
	__FILE__,
	&seganinj_driver,
	"seganinu",
	"Sega Ninja (not encrypted)",
	"1985",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	seganinu_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	seganinj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	seganinj_hiload, seganinj_hisave
};

struct GameDriver nprinces_driver =
{
	__FILE__,
	&seganinj_driver,
	"nprinces",
	"Ninja Princess",
	"1985",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	nprinces_rom,
	0, nprinces_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	seganinj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	seganinj_hiload, seganinj_hisave
};

struct GameDriver nprinceb_driver =
{
	__FILE__,
	&seganinj_driver,
	"nprinceb",
	"Ninja Princess (bootleg)",
	"1985",
	"bootleg?",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	nprinceb_rom,
	0, flicky_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	seganinj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	seganinj_hiload, seganinj_hisave
};

struct GameDriver imsorry_driver =
{
	__FILE__,
	0,
	"imsorry",
	"I'm Sorry (US)",
	"1985",
	"Coreland / Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	imsorry_rom,
	0, imsorry_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	imsorry_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	imsorry_hiload, imsorry_hisave
};

struct GameDriver imsorryj_driver =
{
	__FILE__,
	&imsorry_driver,
	"imsorryj",
	"I'm Sorry (Japan)",
	"1985",
	"Coreland / Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	imsorryj_rom,
	0, imsorry_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	imsorry_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	imsorry_hiload, imsorry_hisave
};

struct GameDriver teddybb_driver =
{
	__FILE__,
	0,
	"teddybb",
	"TeddyBoy Blues",
	"1985",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	teddybb_rom,
	0, teddybb_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	teddybb_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	teddybb_hiload, teddybb_hisave
};

struct GameDriver hvymetal_driver =
{
	__FILE__,
	0,
	"hvymetal",
	"Heavy Metal",
	"1985",
	"Sega",
	BASE_CREDITS,
	0,
	&hvymetal_machine_driver,

	hvymetal_rom,
	0, hvymetal_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	hvymetal_input_ports,

	PROM_MEMORY_REGION(4),0,0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver myhero_driver =
{
	__FILE__,
	0,
	"myhero",
	"My Hero",
	"1985",
	"Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	myhero_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	myhero_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	myhero_hiload, myhero_hisave
};

struct GameDriver myheroj_driver =
{
	__FILE__,
	&myhero_driver,
	"myheroj",
	"Seishun Scandal",
	"1985",
	"Coreland / Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	myheroj_rom,
	0, myheroj_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	myhero_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	myhero_hiload, myhero_hisave
};

struct GameDriver chplft_driver =
{
	__FILE__,
	0,
	"chplft",
	"Choplifter",
	"1985",
	"Sega",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&chplft_machine_driver,

	chplft_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	chplft_input_ports,

	PROM_MEMORY_REGION(4),0,0,
	ORIENTATION_DEFAULT,
	chplft_hiload, chplft_hisave
};

struct GameDriver chplftb_driver =
{
	__FILE__,
	&chplft_driver,
	"chplftb",
	"Choplifter (alternate)",
	"1985",
	"Sega",
	BASE_CREDITS,
	0,
	&chplft_machine_driver,

	chplftb_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	chplft_input_ports,

	PROM_MEMORY_REGION(4),0,0,
	ORIENTATION_DEFAULT,
	chplft_hiload, chplft_hisave
};

struct GameDriver chplftbl_driver =
{
	__FILE__,
	&chplft_driver,
	"chplftbl",
	"Choplifter (bootleg)",
	"1985",
	"bootleg",
	BASE_CREDITS,
	0,
	&chplft_machine_driver,

	chplftbl_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	chplft_input_ports,

	PROM_MEMORY_REGION(4),0,0,
	ORIENTATION_DEFAULT,
	chplft_hiload, chplft_hisave
};

struct GameDriver fdwarrio_driver =
{
	__FILE__,
	0,
	"4dwarrio",
	"4D Warriors",
	"1985",
	"Coreland / Sega",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	fdwarrio_rom,
	0, fdwarrio_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	fdwarrio_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	fdwarrio_hiload, imsorry_hisave
};

struct GameDriver brain_driver =
{
	__FILE__,
	0,
	"brain",
	"Brain",
	"1986",
	"Coreland / Sega",
	BASE_CREDITS,
	GAME_WRONG_COLORS,
	&brain_machine_driver,

	brain_rom,
	0, 0,		/* ROM decode and opcode decode functions */
	0,   		/* Sample names */
	0,

	brain_input_ports,

	0,0,0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver wboy_driver =
{
	__FILE__,
	0,
	"wboy",
	"Wonder Boy (set 1)",
	"1986",
	"Sega (Escape license)",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	wboy_rom,
	0, hvymetal_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wboy_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver wboy2_driver =
{
	__FILE__,
	&wboy_driver,
	"wboy2",
	"Wonder Boy (set 2)",
	"1986",
	"Sega (Escape license)",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	wboy2_rom,
	0, hvymetal_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wboy_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver wboy3_driver =
{
	__FILE__,
	&wboy_driver,
	"wboy3",
	"Wonder Boy (set 3)",
	"????",
	"?????",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&system8_machine_driver,

	wboy3_rom,
	0, wboy3_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wboy_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver wboy4_driver =
{
	__FILE__,
	&wboy_driver,
	"wboy4",
	"Wonder Boy (set 4)",
	"1986",
	"Sega (Escape license)",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	wboy4_rom,
	0, wboy4_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wboy_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver wboyu_driver =
{
	__FILE__,
	&wboy_driver,
	"wboyu",
	"Wonder Boy (not encrypted)",
	"1986",
	"Sega (Escape license)",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	wboyu_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wboyu_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver wboy4u_driver =
{
	__FILE__,
	&wboy_driver,
	"wboy4u",
	"Wonder Boy (set 4 not encrypted)",
	"1986",
	"Sega (Escape license)",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	wboy4u_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wboy_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver wbdeluxe_driver =
{
	__FILE__,
	&wboy_driver,
	"wbdeluxe",
	"Wonder Boy Deluxe",
	"1986",
	"Sega (Escape license)",
	BASE_CREDITS,
	0,
	&system8_machine_driver,

	wbdeluxe_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wboy_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	wbdeluxe_hiload, wbdeluxe_hisave
};

struct GameDriver gardia_driver =
{
	__FILE__,
	0,
	"gardia",
	"Gardia",
	"????",
	"????",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&brain_machine_driver,

	gardia_rom,
	0, gardia_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wboy_input_ports,

	PROM_MEMORY_REGION(4),0,0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver blockgal_driver =
{
	__FILE__,
	0,
	"blockgal",
	"Block Gal",
	"????",
	"????",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&system8_machine_driver,

	blockgal_rom,
	0, 0,					/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wboy_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,
	0, 0
};

struct GameDriver tokisens_driver =
{
	__FILE__,
	0,
	"tokisens",
	"Toki no Senshi",
	"1987",
	"Sega",
	BASE_CREDITS,
	0,
	&wbml_machine_driver,

	tokisens_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	tokisens_input_ports,

	PROM_MEMORY_REGION(4),0,0,
	ORIENTATION_ROTATE_90,
	0, 0
};

struct GameDriver dakkochn_driver =
{
	__FILE__,
	0,
	"dakkochn",
	"DakkoChan Jansoh",
	"????",
	"?????",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&chplft_machine_driver,

	dakkochn_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	chplft_input_ports,

	PROM_MEMORY_REGION(4),0,0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver ufosensi_driver =
{
	__FILE__,
	0,
	"ufosensi",
	"Ufo Senshi Yohko Chan",
	"????",
	"?????",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&chplft_machine_driver,

	ufosensi_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	chplft_input_ports,

	PROM_MEMORY_REGION(4),0,0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver wbml_driver =
{
	__FILE__,
	0,
	"wbml",
	"Wonder Boy in Monster Land",
	"1987",
	"bootleg",
	BASE_CREDITS,
	0,
	&wbml_machine_driver,

	wbml_rom,
	0, wbml_decode,   		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wbml_input_ports,

	PROM_MEMORY_REGION(4), 0, 0,
	ORIENTATION_DEFAULT,
	wbml_hiload, wbml_hisave
};
