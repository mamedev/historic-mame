/******************************************************************************

Sega System 1 / System 2

driver by Jarek Parchanski, Nicola Salmoria, Mirko Buffoni


Up'n Down, Mister Viking, Flicky, SWAT, Water Match and Bull Fight are known
to run on IDENTICAL hardware (they were sold by Bally-Midway as ROM swaps).

TODO: - background is misplaced in wbmlju
	  - sprites stick in Pitfall II
	  - sprite priorities are probably wrong
	  - remove patch in noboranb if possible and fully understand the
	    ports involved in the protection

******************************************************************************/

#include "driver.h"
#include "vidhrdw/system1.h"
#include "cpu/z80/z80.h"
#include "machine/segacrpt.h"



static MACHINE_INIT( system1 )
{
	/* skip the long IC CHECK in Teddyboy Blues and Choplifter */
	/* this is not a ROM patch, the game checks a RAM location */
	/* before doing the test */
//	memory_region(REGION_CPU1)[0xeffe] = 0x4f;
//	memory_region(REGION_CPU1)[0xefff] = 0x4b;

	system1_define_background_memory(system1_BACKGROUND_MEMORY_SINGLE);
}

static MACHINE_INIT( wbml )
{
	/* skip the long IC CHECK in Teddyboy Blues and Choplifter */
	/* this is not a ROM patch, the game checks a RAM location */
	/* before doing the test */
//	memory_region(REGION_CPU1)[0xeffe] = 0x4f;
//	memory_region(REGION_CPU1)[0xefff] = 0x4b;

	system1_define_background_memory(system1_BACKGROUND_MEMORY_BANKED);
}

/* noboranb

there seems to be some protection? involving reads / writes to ports in the 2x region

*/

static int inport23_step;

static MACHINE_INIT( noboranb )
{
	system1_define_background_memory(system1_BACKGROUND_MEMORY_SINGLE);
	inport23_step = 0;

}


static READ_HANDLER( inport16_r )
{
//	logerror("IN  $16 : pc = %04x - data = ??\n",activecpu_get_pc());

	return(0);
}

static READ_HANDLER( inport1c_r )
{
//	logerror("IN  $1c : pc = %04x - data = ??\n",activecpu_get_pc());

	return(0x80);	// infinite loop (at 0x0fb3) until bit 7 is set
}

static READ_HANDLER( inport22_r )
{
//	logerror("IN  $22 : pc = %04x - data = ??\n",activecpu_get_pc());

	return (0);
}

static READ_HANDLER( inport23_r )
{
	int data = inport23_step;

//	logerror("IN  $23 : pc = %04x - step = %02x\n",activecpu_get_pc(),inport23_step);

	inport23_step = (inport23_step + 1) & 0xff;

	return (data);
}

static WRITE_HANDLER( outport16_w )
{
//	logerror("OUT $16 : pc = %04x - data = %02x\n",activecpu_get_pc(),data);
}

static WRITE_HANDLER( outport17_w )
{
//	logerror("OUT $17 : pc = %04x - data = %02x\n",activecpu_get_pc(),data);
}

static WRITE_HANDLER( outport24_w )
{
//	logerror("OUT $24 : pc = %04x - data = %02x\n",activecpu_get_pc(),data);
}

/* end noboranb */

WRITE_HANDLER( hvymetal_videomode_w )
{
	int bankaddress;
	unsigned char *rom = memory_region(REGION_CPU1);

	/* patch out the obnoxiously long startup RAM tests */
//	rom[0x4a55 + memory_region_length(REGION_CPU1) / 2] = 0xc3;
//	rom[0x4a56] = 0xb6;
//	rom[0x4a57] = 0x4a;

	bankaddress = 0x10000 + (((data & 0x04)>>2) * 0x4000) + (((data & 0x40)>>5) * 0x4000);
	cpu_setbank(1,&rom[bankaddress]);

	system1_videomode_w(0, data);
}

WRITE_HANDLER( brain_videomode_w )
{
	int bankaddress;
	unsigned char *rom = memory_region(REGION_CPU1);


	bankaddress = 0x10000 + (((data & 0x04)>>2) * 0x4000) + (((data & 0x40)>>5) * 0x4000);
	cpu_setbank(1,&rom[bankaddress]);

	system1_videomode_w(0, data);
}

WRITE_HANDLER( chplft_videomode_w )
{
	int bankaddress;
	unsigned char *rom = memory_region(REGION_CPU1);


	bankaddress = 0x10000 + (((data & 0x0c)>>2) * 0x4000);
	cpu_setbank(1,&rom[bankaddress]);

	system1_videomode_w(0, data);
}


WRITE_HANDLER( system1_soundport_w )
{
	soundlatch_w(0,data);
	cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
	/* spin for a while to let the Z80 read the command (fixes hanging sound in Regulus) */
	cpu_spinuntil_time(TIME_IN_USEC(50));
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xf3ff, MRA_RAM },
	{ 0xf800, 0xfbff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAMROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xddff, system1_paletteram_w, &paletteram },
	{ 0xe000, 0xe7ff, system1_backgroundram_w, &system1_backgroundram, &system1_backgroundram_size },
	{ 0xe800, 0xeeff, MWA_RAM, &system1_videoram, &system1_videoram_size },
	{ 0xefbd, 0xefbd, MWA_RAM, &system1_scroll_y },
	{ 0xeffc, 0xeffd, MWA_RAM, &system1_scroll_x },
	{ 0xf000, 0xf3ff, system1_background_collisionram_w, &system1_background_collisionram },
	{ 0xf800, 0xfbff, system1_sprites_collisionram_w, &system1_sprites_collisionram },
MEMORY_END

static MEMORY_WRITE_START( blockgal_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAMROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xddff, system1_paletteram_w, &paletteram },
	{ 0xe800, 0xeeff, system1_backgroundram_w, &system1_backgroundram, &system1_backgroundram_size },
	{ 0xe000, 0xe6ff, MWA_RAM, &system1_videoram, &system1_videoram_size },
	{ 0xe7bd, 0xe7bd, MWA_RAM, &system1_scroll_y },	// ???
	{ 0xe7c0, 0xe7c1, MWA_RAM, &system1_scroll_x },
	{ 0xf000, 0xf3ff, system1_background_collisionram_w, &system1_background_collisionram },
	{ 0xf800, 0xfbff, system1_sprites_collisionram_w, &system1_sprites_collisionram },
MEMORY_END

static MEMORY_READ_START( brain_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xf3ff, MRA_RAM },
	{ 0xf800, 0xfbff, MRA_RAM },
MEMORY_END

static MEMORY_READ_START( wbml_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, wbml_paged_videoram_r },
	{ 0xf020, 0xf03f, MRA_RAM },
	{ 0xf800, 0xfbff, MRA_RAM },
MEMORY_END


static MEMORY_WRITE_START( wbml_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAMROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xddff, system1_paletteram_w, &paletteram },
	{ 0xe000, 0xefff, wbml_paged_videoram_w },
	{ 0xf000, 0xf3ff, system1_background_collisionram_w, &system1_background_collisionram },
	{ 0xf800, 0xfbff, system1_sprites_collisionram_w, &system1_sprites_collisionram },
MEMORY_END

static MEMORY_WRITE_START( chplft_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAMROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xddff, system1_paletteram_w, &paletteram },
	{ 0xe7c0, 0xe7ff, choplifter_scroll_x_w, &system1_scrollx_ram },
	{ 0xe000, 0xe7ff, MWA_RAM, &system1_videoram, &system1_videoram_size },
	{ 0xe800, 0xeeff, system1_backgroundram_w, &system1_backgroundram, &system1_backgroundram_size },
	{ 0xf000, 0xf3ff, system1_background_collisionram_w, &system1_background_collisionram },
	{ 0xf800, 0xfbff, system1_sprites_collisionram_w, &system1_sprites_collisionram },
MEMORY_END

static MEMORY_WRITE_START( nobo_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc3ff, system1_background_collisionram_w, &system1_background_collisionram },
	{ 0xc800, 0xcbff, system1_sprites_collisionram_w, &system1_sprites_collisionram },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd200, 0xd7ff, MWA_RAM },
	{ 0xd800, 0xddff, system1_paletteram_w, &paletteram },
	{ 0xde00, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, system1_backgroundram_w, &system1_backgroundram, &system1_backgroundram_size },
	{ 0xe800, 0xeeff, MWA_RAM, &system1_videoram, &system1_videoram_size },
	{ 0xefbd, 0xefbd, MWA_RAM, &system1_scroll_y },
	{ 0xeffc, 0xeffd, MWA_RAM, &system1_scroll_x },
	{ 0xf000, 0xffff, MWA_RAM },

	/* These addresses are written during P.O.S.T. but don't seem to be used after */
	{ 0xc400, 0xc7ff, MWA_RAM },
	{ 0xcc00, 0xcfff, MWA_RAM },
	{ 0xd200, 0xd7ff, MWA_RAM },
	{ 0xde00, 0xdfff, MWA_RAM },
	{ 0xef00, 0xefbc, MWA_RAM },
	{ 0xefbf, 0xeffb, MWA_RAM },
	{ 0xeffe, 0xefff, MWA_RAM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r }, /* joy1 */
	{ 0x04, 0x04, input_port_1_r }, /* joy2 */
	{ 0x08, 0x08, input_port_2_r }, /* coin,start */
	{ 0x0c, 0x0c, input_port_3_r }, /* DIP2 */
	{ 0x0e, 0x0e, input_port_3_r }, /* DIP2 blckgalb reads it from here */
	{ 0x0d, 0x0d, input_port_4_r }, /* DIP1 some games read it from here... */
	{ 0x10, 0x10, input_port_4_r }, /* DIP1 ... and some others from here */
									/* but there are games which check BOTH! */
	{ 0x15, 0x15, system1_videomode_r },
	{ 0x19, 0x19, system1_videomode_r },    /* mirror address */
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x14, 0x14, system1_soundport_w },    /* sound commands */
	{ 0x15, 0x15, system1_videomode_w },    /* video control and (in some games) bank switching */
	{ 0x18, 0x18, system1_soundport_w },    /* mirror address */
	{ 0x19, 0x19, system1_videomode_w },    /* mirror address */
PORT_END

static PORT_READ_START( wbml_readport )
	{ 0x00, 0x00, input_port_0_r }, /* joy1 */
	{ 0x04, 0x04, input_port_1_r }, /* joy2 */
	{ 0x08, 0x08, input_port_2_r }, /* coin,start */
	{ 0x0c, 0x0c, input_port_3_r }, /* DIP2 */
	{ 0x0d, 0x0d, input_port_4_r }, /* DIP1 some games read it from here... */
	{ 0x10, 0x10, input_port_4_r }, /* DIP1 ... and some others from here */
									/* but there are games which check BOTH! */
	{ 0x15, 0x15, system1_videomode_r },
	{ 0x16, 0x16, wbml_videoram_bank_latch_r },
	{ 0x19, 0x19, system1_videomode_r },  /* mirror address */
PORT_END

static PORT_READ_START( nobo_readport )
	{ 0x00, 0x00, input_port_0_r },	/* Player 1 inputs */
	{ 0x04, 0x04, input_port_1_r },	/* Player 2 inputs */
	{ 0x08, 0x08, input_port_2_r },	/* System inputs */
	{ 0x0c, 0x0c, input_port_3_r },	/* DSW1 */
	{ 0x0d, 0x0d, input_port_4_r },	/* DSW0 */
	{ 0x15, 0x15, system1_videomode_r },
	{ 0x16, 0x16, inport16_r },			/* Used - check code at 0x05cb */
	{ 0x1c, 0x1c, inport1c_r },			/* Shouldn't be called ! */
	{ 0x22, 0x22, inport22_r },			/* Used - check code at 0xb253 */
	{ 0x23, 0x23, inport23_r },			/* Used - check code at 0xb275 and 0xb283 */

PORT_END

static PORT_WRITE_START( wbml_writeport )
	{ 0x14, 0x14, system1_soundport_w },    /* sound commands */
	{ 0x15, 0x15, chplft_videomode_w },
	{ 0x16, 0x16, wbml_videoram_bank_latch_w },
PORT_END

static PORT_WRITE_START( hvymetal_writeport )
	{ 0x18, 0x18, system1_soundport_w },    /* sound commands */
	{ 0x19, 0x19, hvymetal_videomode_w },
PORT_END

static PORT_WRITE_START( brain_writeport )
	{ 0x18, 0x18, system1_soundport_w },    /* sound commands */
	{ 0x19, 0x19, brain_videomode_w },
PORT_END

static PORT_WRITE_START( chplft_writeport )
	{ 0x14, 0x14, system1_soundport_w },    /* sound commands */
	{ 0x15, 0x15, chplft_videomode_w },
PORT_END

static PORT_WRITE_START( nobo_writeport )
	{ 0x14, 0x14, system1_soundport_w },	/* sound commands ? */
	{ 0x15, 0x15, brain_videomode_w },	/* video control + bank switching */
	{ 0x16, 0x16, outport16_w },			/* Used - check code at 0x05cb */
	{ 0x17, 0x17, outport17_w },			/* Not handled in emul. of other SS1/2 games */
	{ 0x24, 0x24, outport24_w },			/* Used - check code at 0xb24e and 0xb307 */
PORT_END


static unsigned char *work_ram;

static READ_HANDLER( work_ram_r )
{
	return work_ram[offset];
}

static WRITE_HANDLER( work_ram_w )
{
	work_ram[offset] = data;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, work_ram_r },
	{ 0x8800, 0x8fff, work_ram_r },
	{ 0xe000, 0xe000, soundlatch_r },
	{ 0xffff, 0xffff, soundlatch_r },   /* 4D warriors reads also from here - bug? */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, work_ram_w, &work_ram },
	{ 0x8800, 0x8fff, work_ram_w },
	{ 0xa000, 0xa003, SN76496_0_w },    /* Choplifter writes to the four addresses */
	{ 0xc000, 0xc003, SN76496_1_w },    /* in sequence */
MEMORY_END


#define IN0_PORT \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define DSW1_PORT \
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(	0x07, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(	0x08, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(	0x09, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(	0x05, "2 Coins/1 Credit 5/3 6/4" ) \
	PORT_DIPSETTING(	0x04, "2 Coins/1 Credit 4/3" ) \
	PORT_DIPSETTING(	0x0f, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(	0x03, "1 Coin/1 Credit 5/6" ) \
	PORT_DIPSETTING(	0x02, "1 Coin/1 Credit 4/5" ) \
	PORT_DIPSETTING(	0x01, "1 Coin/1 Credit 2/3" ) \
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(	0x0d, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(	0x0b, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_6C ) ) \
/*  PORT_DIPSETTING(	0x00, "1/1" ) */ \
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(	0x70, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(	0x80, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(	0x90, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(	0x50, "2 Coins/1 Credit 5/3 6/4" ) \
	PORT_DIPSETTING(	0x40, "2 Coins/1 Credit 4/3" ) \
	PORT_DIPSETTING(	0xf0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(	0x30, "1 Coin/1 Credit 5/6" ) \
	PORT_DIPSETTING(	0x20, "1 Coin/1 Credit 4/5" ) \
	PORT_DIPSETTING(	0x10, "1 Coin/1 Credit 2/3" ) \
	PORT_DIPSETTING(	0x60, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(	0xe0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(	0xd0, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(	0xb0, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(	0xa0, DEF_STR( 1C_6C ) )
/*  PORT_DIPSETTING(	0x00, "1/1" ) */

/* If you don't like the description, feel free to change it */
#define DSW0_BIT7 \
	PORT_DIPNAME( 0x80, 0x80, "SW 0 Read From" ) \
	PORT_DIPSETTING(	0x80, "Port $0D" ) \
	PORT_DIPSETTING(	0x00, "Port $10" )

INPUT_PORTS_START( starjack )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x06, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x38, 0x30, DEF_STR (Bonus_Life ) )
	PORT_DIPSETTING(	0x38, "Every 20k" )
	PORT_DIPSETTING(	0x28, "Every 30k" )
	PORT_DIPSETTING(	0x18, "Every 40k" )
	PORT_DIPSETTING(	0x08, "Every 50k" )
	PORT_DIPSETTING(	0x30, "20k, then every 30k" )
	PORT_DIPSETTING(	0x20, "30k, then every 40k" )
	PORT_DIPSETTING(	0x10, "40k, then every 50k" )
	PORT_DIPSETTING(	0x00, "50k, then every 60k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0xc0, "Easy" )
	PORT_DIPSETTING(	0x80, "Medium" )
	PORT_DIPSETTING(	0x40, "Hard" )
	PORT_DIPSETTING(	0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( starjacs )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x06, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x08, 0x08, "Ship" )
	PORT_DIPSETTING(	0x08, "Single" )
	PORT_DIPSETTING(	0x00, "Multi" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "30k, then every 40k" )
	PORT_DIPSETTING(	0x20, "40k, then every 50k" )
	PORT_DIPSETTING(	0x10, "50k, then every 60k" )
	PORT_DIPSETTING(	0x00, "60k, then every 70k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0xc0, "Easy" )
	PORT_DIPSETTING(	0x80, "Medium" )
	PORT_DIPSETTING(	0x40, "Hard" )
	PORT_DIPSETTING(	0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( regulus )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, "Unused SW 0-1" )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x10, "Unused SW 0-4" )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unused SW 0-5" )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(	0x80, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
INPUT_PORTS_END

/* Same as 'regulus', but no "Allow Continue" Dip Switch */
INPUT_PORTS_START( reguluso )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, "Unused SW 0-1" )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x10, "Unused SW 0-4" )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unused SW 0-5" )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unused SW 0-7" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( upndown )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x06, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x38, "10000" )
	PORT_DIPSETTING(	0x30, "20000" )
	PORT_DIPSETTING(	0x28, "30000" )
	PORT_DIPSETTING(	0x20, "40000" )
	PORT_DIPSETTING(	0x18, "50000" )
	PORT_DIPSETTING(	0x10, "60000" )
	PORT_DIPSETTING(	0x08, "70000" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0xc0, "Easy" )
	PORT_DIPSETTING(	0x80, "Medium" )
	PORT_DIPSETTING(	0x40, "Hard" )
	PORT_DIPSETTING(	0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( mrviking )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, "Maximum Credits" )
	PORT_DIPSETTING(	0x02, "9" )
	PORT_DIPSETTING(	0x00, "99" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "10k, 30k then every 30k" )
	PORT_DIPSETTING(	0x20, "20k, 40k then every 30k" )
	PORT_DIPSETTING(	0x10, "30k, then every 30k" )
	PORT_DIPSETTING(	0x00, "40k, then every 30k" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	DSW0_BIT7
INPUT_PORTS_END

/* Same as 'mrviking', but no "Maximum Credits" Dip Switch and "Difficulty" Dip Switch is
   handled by bit 7 instead of bit 6 (so bit 6 is unused) */
INPUT_PORTS_START( mrvikngj )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, "Unused SW 0-1" )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "10k, 30k then every 30k" )
	PORT_DIPSETTING(	0x20, "20k, 40k then every 30k" )
	PORT_DIPSETTING(	0x10, "30k, then every 30k" )
	PORT_DIPSETTING(	0x00, "40k, then every 30k" )
	PORT_DIPNAME( 0x40, 0x40, "Unused SW 0-6" )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x80, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
INPUT_PORTS_END

INPUT_PORTS_START( swat )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x06, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x38, "30000" )
	PORT_DIPSETTING(	0x30, "40000" )
	PORT_DIPSETTING(	0x28, "50000" )
	PORT_DIPSETTING(	0x20, "60000" )
	PORT_DIPSETTING(	0x18, "70000" )
	PORT_DIPSETTING(	0x10, "80000" )
	PORT_DIPSETTING(	0x08, "90000" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Unused SW 0-6" )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unused SW 0-7" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( flicky )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, "Unused SW 0-1" )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "30000 80000 160000" )
	PORT_DIPSETTING(	0x20, "30000 100000 200000" )
	PORT_DIPSETTING(	0x10, "40000 120000 240000" )
	PORT_DIPSETTING(	0x00, "40000 140000 280000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	DSW0_BIT7
INPUT_PORTS_END

INPUT_PORTS_START( wmatch )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* TURN P1 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL ) /* TURN P2 */

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Time" )
	PORT_DIPSETTING(	0x0c, "Normal" )
	PORT_DIPSETTING(	0x08, "Fast" )
	PORT_DIPSETTING(	0x04, "Faster" )
	PORT_DIPSETTING(	0x00, "Fastest" )
	PORT_DIPNAME( 0x10, 0x10, "Unused SW 0-4" )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unused SW 0-5" )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	DSW0_BIT7
INPUT_PORTS_END

INPUT_PORTS_START( bullfgt )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "30000" )
	PORT_DIPSETTING(	0x20, "50000" )
	PORT_DIPSETTING(	0x10, "70000" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Unused SW 0-6" )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	DSW0_BIT7
INPUT_PORTS_END

INPUT_PORTS_START( spatter )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x08, "2" )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "40k, 120k and 480k" )
	PORT_DIPSETTING(	0x20, "50k and 200k" )
	PORT_DIPSETTING(	0x10, "100k only" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Reset Timer/Objects On Life Loss" )
	PORT_DIPSETTING(	0x40, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
	DSW0_BIT7
INPUT_PORTS_END

INPUT_PORTS_START( pitfall2 )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x10, "20000 50000" )
	PORT_DIPSETTING(	0x00, "30000 70000" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue" )
	PORT_DIPSETTING(	0x20, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Time" )
	PORT_DIPSETTING(	0x00, "2 Minutes" )
	PORT_DIPSETTING(	0x40, "3 Minutes" )
	PORT_DIPNAME( 0x80, 0x80, "Unused SW 0-7" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( pitfallu )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x06, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x18, 0x18, "Starting Stage" )
	PORT_DIPSETTING(	0x18, "1" )
	PORT_DIPSETTING(	0x10, "2" )
	PORT_DIPSETTING(	0x08, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue" )
	PORT_DIPSETTING(	0x20, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Time" )
	PORT_DIPSETTING(	0x00, "2 Minutes" )
	PORT_DIPSETTING(	0x40, "3 Minutes" )
	PORT_DIPNAME( 0x80, 0x80, "Unused SW 0-7" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( seganinj )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x08, "2" )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x10, "20k 70k 120k 170k" )
	PORT_DIPSETTING(	0x00, "50k 100k 150k 200k" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue" )
	PORT_DIPSETTING(	0x20, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	DSW0_BIT7
INPUT_PORTS_END

INPUT_PORTS_START( imsorry )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0C, 0x0C, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0C, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "30000" )
	PORT_DIPSETTING(	0x20, "40000" )
	PORT_DIPSETTING(	0x10, "50000" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Unused SW 0-6" )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	DSW0_BIT7
INPUT_PORTS_END

INPUT_PORTS_START( teddybb )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x08, "2" )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "252", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "100k 400k" )
	PORT_DIPSETTING(	0x20, "200k 600k" )
	PORT_DIPSETTING(	0x10, "400k 800k" )
	PORT_DIPSETTING(	0x00, "600k" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	DSW0_BIT7
INPUT_PORTS_END

INPUT_PORTS_START( hvymetal )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "50000 100000" )
	PORT_DIPSETTING(	0x20, "60000 120000" )
	PORT_DIPSETTING(	0x10, "70000 150000" )
	PORT_DIPSETTING(	0x00, "100000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(	0x80, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( myhero )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "30000" )
	PORT_DIPSETTING(	0x20, "50000" )
	PORT_DIPSETTING(	0x10, "70000" )
	PORT_DIPSETTING(	0x00, "90000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	DSW0_BIT7
INPUT_PORTS_END

INPUT_PORTS_START( shtngmst )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "100k 500k" )
	PORT_DIPSETTING(	0x20, "150k 600k" )
	PORT_DIPSETTING(	0x10, "200k 700k" )
	PORT_DIPSETTING(	0x00, "300k 800k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(  0xc0, "Easy" )
	PORT_DIPSETTING(  0x80, "Medium" )
	PORT_DIPSETTING(  0x40, "Hard" )
	PORT_DIPSETTING(  0x00, "Hardest" )

	PORT_START	  /* DSW0 */
	DSW1_PORT
INPUT_PORTS_END

INPUT_PORTS_START( chplft )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x08, "2" )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "20000 70000" )
	PORT_DIPSETTING(	0x20, "50000 100000" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	  /* DSW0 */
	DSW1_PORT
INPUT_PORTS_END

INPUT_PORTS_START( 4dwarrio )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x06, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x38, "30000" )
	PORT_DIPSETTING(	0x30, "40000" )
	PORT_DIPSETTING(	0x28, "50000" )
	PORT_DIPSETTING(	0x20, "60000" )
	PORT_DIPSETTING(	0x18, "70000" )
	PORT_DIPSETTING(	0x10, "80000" )
	PORT_DIPSETTING(	0x08, "90000" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	DSW0_BIT7
INPUT_PORTS_END

INPUT_PORTS_START( brain )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( raflesia )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x30, "20k, 70k and 120k" )
	PORT_DIPSETTING(	0x20, "30k, 80k and 150k" )
	PORT_DIPSETTING(	0x10, "50k, 100k and 200k" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Unused SW 0-6" )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	DSW0_BIT7
INPUT_PORTS_END

INPUT_PORTS_START( wboy )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* down - unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* up - unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* down - unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* up - unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x10, "30k 100k 170k 240k" )
	PORT_DIPSETTING(	0x00, "30k 120k 210k 300k" )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* same as wboy, additional Energy Consumption switch */
INPUT_PORTS_START( wbdeluxe )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Has to be 0 otherwise the game resets */
												/* if you die after level 1. */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* down - unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* up - unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* down - unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* up - unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START	  /* DSW1 */
	DSW1_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x10, "30k 100k 170k 240k" )
	PORT_DIPSETTING(	0x00, "30k 120k 210k 300k" )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x40, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Energy Consumption" )
	PORT_DIPSETTING(	0x00, "Slow" )
	PORT_DIPSETTING(	0x80, "Fast" )
INPUT_PORTS_END

INPUT_PORTS_START( wboyu )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* down - unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* up - unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* down - unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* up - unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x06, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	  /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Mode" )
	PORT_DIPSETTING(	0xc0, "Normal Game" )
	PORT_DIPSETTING(	0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, "Test Mode" )
	PORT_DIPSETTING(	0x00, "Endless Game" )
INPUT_PORTS_END

/* Notes about the bootleg version (as this is the only "working" one) :

Coinage is almost the same as the other Sega games.

However, when you set DSW1 to 00, you enter a pseudo "free play" mode :

  - When you insert a coin, or press the "Service" button, you are given 2 credits
    and this number is NEVER incremented nor decremented
  - You are given 3 lives at start and this number is NEVER decremented
    (it can however be incremented depending on the "Bonus Life" Dip Switch)

If only one nibble is set to 0, it will give a standard 1C_1C.


There is an ingame bug with the "Bonus Life" Dip Switch, but I don't know if it's only
a "feature" of the bootleg :

  - Check routine at 0x2366, and you'll notice that 0xc02d (player 1) and 0xc02e (player 2)
    act like a "pointer" to the bonus life table (0x5ab6 or 0x5abb)
  - Once you get enough points, 1 life is added, and the pointer is incremented
  - There is however NO test to the limit of this pointer ! So, once you've got your 5th
    extra life at 150k, the pointed value will be 3 (= extra life at 30k), and as your
    score is over this value, you'll be given another extra life ... and so on ...


Bits 2 and 6 of DSW0 aren't tested in the game (I can't tell about the "test mode")


Useful addresses (to unprotect 'blockgal' ?) :

  - 0xc040 : credits (0x00-0x09)
  - 0xc019 : player 1 lives
  - 0xc021 : player 2 lives
  - 0xc018 : player 1 level (0x01-0x14)
  - 0xc020 : player 2 level (0x01-0x14)
  - 0xc02d : player 1 bonus life "pointer"
  - 0xc02e : player 1 bonus life "pointer"
  - 0xc01a - 0xc01c : player 1 score (BCD coded - LSB first)
  - 0xc022 - 0xc024 : player 1 score (BCD coded - LSB first)

  - 0xc050 : when == 01, "free play" mode
  - 0xc00c : when == 01, you end the level

*/

INPUT_PORTS_START( blockgal )
	PORT_START  /* IN1 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 60, 15, 0, 0)

	PORT_START  /* IN2 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_COCKTAIL, 60, 15, 0, 0)

	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unused SW 0-2" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x08, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x10, "10k 30k 60k 100k 150k" )
	PORT_DIPSETTING(	0x00, "30k 50k 100k 200k 300k" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue" )
	PORT_DIPSETTING(	0x20, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Unused SW 0-6" )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	  /* DSW1 */
	DSW1_PORT
INPUT_PORTS_END

INPUT_PORTS_START( tokisens )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x08, "2" )
	PORT_DIPSETTING(	0x0c, "3" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	  /* DSW1 */
	DSW1_PORT
INPUT_PORTS_END

INPUT_PORTS_START( wbml )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	IN0_PORT

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x0c, "4" )
	PORT_DIPSETTING(	0x08, "5" )
/* 0x00 gives 4 lives */
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x10, "30000 100000 200000" )
	PORT_DIPSETTING(	0x00, "50000 150000 250000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x20, "Easy" )
	PORT_DIPSETTING(	0x00, "Hard" )
	PORT_BITX(	0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Test Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	  /* DSW0 */
	DSW1_PORT
INPUT_PORTS_END

INPUT_PORTS_START( noboranb )
	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )				// shot
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )				// fly
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )		// shot
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )		// fly
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x20, "Easy" )
	PORT_DIPSETTING(	0x30, "Medium" )
	PORT_DIPSETTING(	0x10, "Hard" )
	PORT_DIPSETTING(	0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x02, "2" )
	PORT_DIPSETTING(	0x03, "3" )
	PORT_DIPSETTING(	0x01, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "99", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x08, "40k, 80k, 120k and 160k" )
	PORT_DIPSETTING(	0x0c, "50k, 100k and 150k" )
	PORT_DIPSETTING(	0x04, "60k, 120k and 180k" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unused SW 0-6" )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unused SW 0-7" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* sprites use colors 0-511, but are not defined here */
	{ REGION_GFX1, 0, &charlayout, 512, 128 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,      /* 2 chips */
	{ 2000000, 4000000 },   /* 8 MHz / 4 ?*/
	{ 50, 50 }
};



static MACHINE_DRIVER_START( system1 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3600000)	/* should be 4 MHz but that value makes the Pitfall II title screen disappear */
//	MDRV_CPU_ADD(Z80, 4000000)	/* My Hero has 2 OSCs 8 & 20 MHz (Cabbe Info) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)		 /* NMIs are caused by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(system1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1536)
	MDRV_COLORTABLE_LENGTH(1536)

	MDRV_PALETTE_INIT(system1)
	MDRV_VIDEO_START(system1)
	MDRV_VIDEO_UPDATE(system1)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END


/* driver with reduced visible area for scrolling games */
static MACHINE_DRIVER_START( small )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)    /* My Hero has 2 OSCs 8 & 20 MHz (Cabbe Info) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)		 /* NMIs are caused by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(system1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8+8, 32*8-1-8, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1536)
	MDRV_COLORTABLE_LENGTH(1536)

	MDRV_PALETTE_INIT(system1)
	MDRV_VIDEO_START(system1)
	MDRV_VIDEO_UPDATE(system1)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hvymetal )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)        	/* 4 MHz ? */
	MDRV_CPU_MEMORY(brain_readmem,writemem)
	MDRV_CPU_PORTS(wbml_readport,hvymetal_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        	/* 4 MHz ? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)		 /* NMIs are caused by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(system1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1536)
	MDRV_COLORTABLE_LENGTH(1536)

	MDRV_PALETTE_INIT(system1)
	MDRV_VIDEO_START(system1)
	MDRV_VIDEO_UPDATE(system1)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( chplft )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)        	/* 4 MHz ? */
	MDRV_CPU_MEMORY(brain_readmem,chplft_writemem)
	MDRV_CPU_PORTS(wbml_readport,chplft_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        	/* 4 MHz ? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)		 /* NMIs are caused by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(system1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1536)
	MDRV_COLORTABLE_LENGTH(1536)

	MDRV_PALETTE_INIT(system1)
	MDRV_VIDEO_START(system1)
	MDRV_VIDEO_UPDATE(choplifter)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( brain )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)    /* My Hero has 2 OSCs 8 & 20 MHz (Cabbe Info) */
	MDRV_CPU_MEMORY(brain_readmem,writemem)
	MDRV_CPU_PORTS(readport,brain_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)		 /* NMIs are caused by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(system1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1536)
	MDRV_COLORTABLE_LENGTH(1536)

	MDRV_PALETTE_INIT(system1)
	MDRV_VIDEO_START(system1)
	MDRV_VIDEO_UPDATE(system1)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( wbml )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)        	/* 4 MHz ? */
	MDRV_CPU_MEMORY(wbml_readmem,wbml_writemem)
	MDRV_CPU_PORTS(wbml_readport,wbml_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        	/* 4 MHz ? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)		 /* NMIs are caused by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(wbml)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1536)
	MDRV_COLORTABLE_LENGTH(1536)

	MDRV_PALETTE_INIT(system1)
	MDRV_VIDEO_START(system1)
	MDRV_VIDEO_UPDATE(wbml)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( noboranb )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 8000000)    /* ? guess ? */
	MDRV_CPU_MEMORY(brain_readmem,nobo_writemem)
	MDRV_CPU_PORTS(nobo_readport,nobo_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)		 /* NMIs are caused by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(noboranb)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1536)
	MDRV_COLORTABLE_LENGTH(1536)

	MDRV_PALETTE_INIT(system1)
	MDRV_VIDEO_START(system1)
	MDRV_VIDEO_UPDATE(system1)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( blockgal )

	MDRV_IMPORT_FROM(system1)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem,blockgal_writemem)

	MDRV_VIDEO_UPDATE(blockgal)

MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/* Since the standard System 1 PROM has part # 5317, Star Jacker, whose first */
/* ROM is #5318, is probably the first or second System 1 game produced */
ROM_START( starjack )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "epr5320b.129",	0x0000, 0x2000, 0x7ab72ecd )
	ROM_LOAD( "epr5321a.130",	0x2000, 0x2000, 0x38b99050 )
	ROM_LOAD( "epr5322a.131",	0x4000, 0x2000, 0x103a595b )
	ROM_LOAD( "epr-5323.132",	0x6000, 0x2000, 0x46af0d58 )
	ROM_LOAD( "epr-5324.133",	0x8000, 0x2000, 0x1e89efe2 )
	ROM_LOAD( "epr-5325.134",	0xa000, 0x2000, 0xd6e379a1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5332.3",		0x0000, 0x2000, 0x7a72ab3d )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5331.82",	0x0000, 0x2000, 0x251d898f )
	ROM_LOAD( "epr-5330.65",	0x2000, 0x2000, 0xeb048745 )
	ROM_LOAD( "epr-5329.81",	0x4000, 0x2000, 0x3e8bcaed )
	ROM_LOAD( "epr-5328.64",	0x6000, 0x2000, 0x9ed7849f )
	ROM_LOAD( "epr-5327.80",	0x8000, 0x2000, 0x79e92cb1 )
	ROM_LOAD( "epr-5326.63",	0xa000, 0x2000, 0xba7e2b47 )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5318.86",	0x0000, 0x4000, 0x6f2e1fd3 )
	ROM_LOAD( "epr-5319.93",	0x4000, 0x4000, 0xebee4999 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( starjacs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "sja1.129",		0x0000, 0x2000, 0x59a22a1f )
	ROM_LOAD( "sja1.130",		0x2000, 0x2000, 0x7f4597dc )
	ROM_LOAD( "sja1.131",		0x4000, 0x2000, 0x6074c046 )
	ROM_LOAD( "sja1.132",		0x6000, 0x2000, 0x1c48a3fa )
	ROM_LOAD( "sja1.133",		0x8000, 0x2000, 0x7598bd51 )
	ROM_LOAD( "sja1.134",		0xa000, 0x2000, 0xf66fa604 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5332.3",		0x0000, 0x2000, 0x7a72ab3d )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5331.82",	0x0000, 0x2000, 0x251d898f )
	ROM_LOAD( "sja1.65",		0x2000, 0x2000, 0x0ab1893c )
	ROM_LOAD( "epr-5329.81",	0x4000, 0x2000, 0x3e8bcaed )
	ROM_LOAD( "sja1.64",		0x6000, 0x2000, 0x7f628ae6 )
	ROM_LOAD( "epr-5327.80",	0x8000, 0x2000, 0x79e92cb1 )
	ROM_LOAD( "sja1.63",		0xa000, 0x2000, 0x5bcb253e )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	/* SJA1IC86 and SJA1IC93 in the original set were bad, so I'm using the ones */
	/* from the Sega version. However I suspect the real ones should be slightly */
	/* different. */
	ROM_LOAD( "epr-5318.86",	0x0000, 0x4000, BADCRC(0x6f2e1fd3) )
	ROM_LOAD( "epr-5319.93",	0x4000, 0x4000, BADCRC(0xebee4999) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )	/* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( regulus )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr5640a.129",	0x0000, 0x2000, 0xdafb1528 ) /* encrypted */
	ROM_LOAD( "epr5641a.130",	0x2000, 0x2000, 0x0fcc850e ) /* encrypted */
	ROM_LOAD( "epr5642a.131",	0x4000, 0x2000, 0x4feffa17 ) /* encrypted */
	ROM_LOAD( "epr5643a.132",	0x6000, 0x2000, 0xb8ac7eb4 ) /* encrypted */
	ROM_LOAD( "epr-5644.133",	0x8000, 0x2000, 0xffd05b7d )
	ROM_LOAD( "epr5645a.134",	0xa000, 0x2000, 0x6b4bf77c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5652.3",		0x0000, 0x2000, 0x74edcb98 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5651.82",	0x0000, 0x2000, 0xf07f3e82 )
	ROM_LOAD( "epr-5650.65",	0x2000, 0x2000, 0x84c1baa2 )
	ROM_LOAD( "epr-5649.81",	0x4000, 0x2000, 0x6774c895 )
	ROM_LOAD( "epr-5648.64",	0x6000, 0x2000, 0x0c69e92a )
	ROM_LOAD( "epr-5647.80",	0x8000, 0x2000, 0x9330f7b5 )
	ROM_LOAD( "epr-5646.63",	0xa000, 0x2000, 0x4dfacbbc )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5638.86",	0x0000, 0x4000, 0x617363dd )
	ROM_LOAD( "epr-5639.93",	0x4000, 0x4000, 0xa4ec5131 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( reguluso )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-5640.129",	0x0000, 0x2000, 0x8324d0d4 ) /* encrypted */
	ROM_LOAD( "epr-5641.130",	0x2000, 0x2000, 0x0a09f5c7 ) /* encrypted */
	ROM_LOAD( "epr-5642.131",	0x4000, 0x2000, 0xff27b2f6 ) /* encrypted */
	ROM_LOAD( "epr-5643.132",	0x6000, 0x2000, 0x0d867df0 ) /* encrypted */
	ROM_LOAD( "epr-5644.133",	0x8000, 0x2000, 0xffd05b7d )
	ROM_LOAD( "epr-5645.134",	0xa000, 0x2000, 0x57a2b4b4 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5652.3",		0x0000, 0x2000, 0x74edcb98 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5651.82",	0x0000, 0x2000, 0xf07f3e82 )
	ROM_LOAD( "epr-5650.65",	0x2000, 0x2000, 0x84c1baa2 )
	ROM_LOAD( "epr-5649.81",	0x4000, 0x2000, 0x6774c895 )
	ROM_LOAD( "epr-5648.64",	0x6000, 0x2000, 0x0c69e92a )
	ROM_LOAD( "epr-5647.80",	0x8000, 0x2000, 0x9330f7b5 )
	ROM_LOAD( "epr-5646.63",	0xa000, 0x2000, 0x4dfacbbc )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5638.86",	0x0000, 0x4000, 0x617363dd )
	ROM_LOAD( "epr-5639.93",	0x4000, 0x4000, 0xa4ec5131 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( regulusu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "epr-5950.129",	0x0000, 0x2000, 0x3b047b67 )
	ROM_LOAD( "epr-5951.130",	0x2000, 0x2000, 0xd66453ab )
	ROM_LOAD( "epr-5952.131",	0x4000, 0x2000, 0xf3d0158a )
	ROM_LOAD( "epr-5953.132",	0x6000, 0x2000, 0xa9ad4f44 )
	ROM_LOAD( "epr-5644.133",	0x8000, 0x2000, 0xffd05b7d )
	ROM_LOAD( "epr-5955.134",	0xa000, 0x2000, 0x65ddb2a3 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5652.3",		0x0000, 0x2000, 0x74edcb98 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5651.82",	0x0000, 0x2000, 0xf07f3e82 )
	ROM_LOAD( "epr-5650.65",	0x2000, 0x2000, 0x84c1baa2 )
	ROM_LOAD( "epr-5649.81",	0x4000, 0x2000, 0x6774c895 )
	ROM_LOAD( "epr-5648.64",	0x6000, 0x2000, 0x0c69e92a )
	ROM_LOAD( "epr-5647.80",	0x8000, 0x2000, 0x9330f7b5 )
	ROM_LOAD( "epr-5646.63",	0xa000, 0x2000, 0x4dfacbbc )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5638.86",	0x0000, 0x4000, 0x617363dd )
	ROM_LOAD( "epr-5639.93",	0x4000, 0x4000, 0xa4ec5131 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( upndown )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "epr5516a.129",	0x0000, 0x2000, 0x038c82da ) /* encrypted */
	ROM_LOAD( "epr5517a.130",	0x2000, 0x2000, 0x6930e1de ) /* encrypted */
	ROM_LOAD( "epr-5518.131",	0x4000, 0x2000, 0x2a370c99 ) /* encrypted */
	ROM_LOAD( "epr-5519.132",	0x6000, 0x2000, 0x9d664a58 ) /* encrypted */
	ROM_LOAD( "epr-5520.133",	0x8000, 0x2000, 0x208dfbdf )
	ROM_LOAD( "epr-5521.134",	0xa000, 0x2000, 0xe7b8d87a )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5535.3",		0x0000, 0x2000, 0xcf4e4c45 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5527.82",	0x0000, 0x2000, 0xb2d616f1 )
	ROM_LOAD( "epr-5526.65",	0x2000, 0x2000, 0x8a8b33c2 )
	ROM_LOAD( "epr-5525.81",	0x4000, 0x2000, 0xe749c5ef )
	ROM_LOAD( "epr-5524.64",	0x6000, 0x2000, 0x8b886952 )
	ROM_LOAD( "epr-5523.80",	0x8000, 0x2000, 0xdede35d9 )
	ROM_LOAD( "epr-5522.63",	0xa000, 0x2000, 0x5e6d9dff )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5514.86",	0x0000, 0x4000, 0xfcc0a88b )
	ROM_LOAD( "epr-5515.93",	0x4000, 0x4000, 0x60908838 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( upndownu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "epr-5679.129",	0x0000, 0x2000, 0xc4f2f9c2 )
	ROM_LOAD( "epr-5680.130",	0x2000, 0x2000, 0x837f021c )
	ROM_LOAD( "epr-5681.131",	0x4000, 0x2000, 0xe1c7ff7e )
	ROM_LOAD( "epr-5682.132",	0x6000, 0x2000, 0x4a5edc1e )
	ROM_LOAD( "epr-5520.133",	0x8000, 0x2000, 0x208dfbdf ) /* epr-5683.133 */
	ROM_LOAD( "epr-5684.133",	0xa000, 0x2000, 0x32fa95da )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5528.3",		0x0000, 0x2000, 0x00cd44ab )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5527.82",	0x0000, 0x2000, 0xb2d616f1 )
	ROM_LOAD( "epr-5526.65",	0x2000, 0x2000, 0x8a8b33c2 )
	ROM_LOAD( "epr-5525.81",	0x4000, 0x2000, 0xe749c5ef )
	ROM_LOAD( "epr-5524.64",	0x6000, 0x2000, 0x8b886952 )
	ROM_LOAD( "epr-5523.80",	0x8000, 0x2000, 0xdede35d9 )
	ROM_LOAD( "epr-5522.63",	0xa000, 0x2000, 0x5e6d9dff )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5514.86",	0x0000, 0x4000, 0xfcc0a88b )
	ROM_LOAD( "epr-5515.93",	0x4000, 0x4000, 0x60908838 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( mrviking )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-5873.129",	0x0000, 0x2000, 0x14d21624 ) /* encrypted */
	ROM_LOAD( "epr-5874.130",	0x2000, 0x2000, 0x6df7de87 ) /* encrypted */
	ROM_LOAD( "epr-5875.131",	0x4000, 0x2000, 0xac226100 ) /* encrypted */
	ROM_LOAD( "epr-5876.132",	0x6000, 0x2000, 0xe77db1dc ) /* encrypted */
	ROM_LOAD( "epr-5755.133",	0x8000, 0x2000, 0xedd62ae1 )
	ROM_LOAD( "epr-5756.134",	0xa000, 0x2000, 0x11974040 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5763.3",		0x0000, 0x2000, 0xd712280d )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5762.82",	0x0000, 0x2000, 0x4a91d08a )
	ROM_LOAD( "epr-5761.65",	0x2000, 0x2000, 0xf7d61b65 )
	ROM_LOAD( "epr-5760.81",	0x4000, 0x2000, 0x95045820 )
	ROM_LOAD( "epr-5759.64",	0x6000, 0x2000, 0x5f9bae4e )
	ROM_LOAD( "epr-5758.80",	0x8000, 0x2000, 0x808ee706 )
	ROM_LOAD( "epr-5757.63",	0xa000, 0x2000, 0x480f7074 )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5749.86",	0x0000, 0x4000, 0xe24682cd )
	ROM_LOAD( "epr-5750.93",	0x4000, 0x4000, 0x6564d1ad )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( mrvikngj )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-5751.129",	0x0000, 0x2000, 0xae97a4c5 ) /* encrypted */
	ROM_LOAD( "epr-5752.130",	0x2000, 0x2000, 0xd48e6726 ) /* encrypted */
	ROM_LOAD( "epr-5753.131",	0x4000, 0x2000, 0x28c60887 ) /* encrypted */
	ROM_LOAD( "epr-5754.132",	0x6000, 0x2000, 0x1f47ed02 ) /* encrypted */
	ROM_LOAD( "epr-5755.133",	0x8000, 0x2000, 0xedd62ae1 )
	ROM_LOAD( "epr-5756.134",	0xa000, 0x2000, 0x11974040 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5763.3",		0x0000, 0x2000, 0xd712280d )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5762.82",	0x0000, 0x2000, 0x4a91d08a )
	ROM_LOAD( "epr-5761.65",	0x2000, 0x2000, 0xf7d61b65 )
	ROM_LOAD( "epr-5760.81",	0x4000, 0x2000, 0x95045820 )
	ROM_LOAD( "epr-5759.64",	0x6000, 0x2000, 0x5f9bae4e )
	ROM_LOAD( "epr-5758.80",	0x8000, 0x2000, 0x808ee706 )
	ROM_LOAD( "epr-5757.63",	0xa000, 0x2000, 0x480f7074 )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5749.86",	0x0000, 0x4000, 0xe24682cd )
	ROM_LOAD( "epr-5750.93",	0x4000, 0x4000, 0x6564d1ad )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( swat )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr5807b.129",	0x0000, 0x2000, 0x93db9c9f ) /* encrypted */
	ROM_LOAD( "epr-5808.130",	0x2000, 0x2000, 0x67116665 ) /* encrypted */
	ROM_LOAD( "epr-5809.131",	0x4000, 0x2000, 0xfd792fc9 ) /* encrypted */
	ROM_LOAD( "epr-5810.132",	0x6000, 0x2000, 0xdc2b279d ) /* encrypted */
	ROM_LOAD( "epr-5811.133",	0x8000, 0x2000, 0x093e3ab1 )
	ROM_LOAD( "epr-5812.134",	0xa000, 0x2000, 0x5bfd692f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5819.3",		0x0000, 0x2000, 0xf6afd0fd )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5818.82",	0x0000, 0x2000, 0xb22033d9 )
	ROM_LOAD( "epr-5817.65",	0x2000, 0x2000, 0xfd942797 )
	ROM_LOAD( "epr-5816.81",	0x4000, 0x2000, 0x4384376d )
	ROM_LOAD( "epr-5815.64",	0x6000, 0x2000, 0x16ad046c )
	ROM_LOAD( "epr-5814.80",	0x8000, 0x2000, 0xbe721c99 )
	ROM_LOAD( "epr-5813.63",	0xa000, 0x2000, 0x0d42c27e )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5805.86",	0x0000, 0x4000, 0x5a732865 )
	ROM_LOAD( "epr-5806.93",	0x4000, 0x4000, 0x26ac258c )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( flicky )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr5978a.116",	0x0000, 0x4000, 0x296f1492 ) /* encrypted */
	ROM_LOAD( "epr5979a.109",	0x4000, 0x4000, 0x64b03ef9 ) /* encrypted */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5869.120",	0x0000, 0x2000, 0x6d220d4e )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5868.62",	0x0000, 0x2000, 0x7402256b )
	ROM_LOAD( "epr-5867.61",	0x2000, 0x2000, 0x2f5ce930 )
	ROM_LOAD( "epr-5866.64",	0x4000, 0x2000, 0x967f1d9a )
	ROM_LOAD( "epr-5865.63",	0x6000, 0x2000, 0x03d9a34c )
	ROM_LOAD( "epr-5864.66",	0x8000, 0x2000, 0xe659f358 )
	ROM_LOAD( "epr-5863.65",	0xa000, 0x2000, 0xa496ca15 )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5855.117",	0x0000, 0x4000, 0xb5f894a1 )
	ROM_LOAD( "epr-5856.110",	0x4000, 0x4000, 0x266af78f )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( flickyo )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-5857.bin",	0x0000, 0x2000, 0xa65ac88e ) /* encrypted */
	ROM_LOAD( "epr5858a.bin",	0x2000, 0x2000, 0x18b412f4 ) /* encrypted */
	ROM_LOAD( "epr-5859.bin",	0x4000, 0x2000, 0xa5558d7e ) /* encrypted */
	ROM_LOAD( "epr-5860.bin",	0x6000, 0x2000, 0x1b35fef1 ) /* encrypted */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-5869.120",	0x0000, 0x2000, 0x6d220d4e )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5868.62",	0x0000, 0x2000, 0x7402256b )
	ROM_LOAD( "epr-5867.61",	0x2000, 0x2000, 0x2f5ce930 )
	ROM_LOAD( "epr-5866.64",	0x4000, 0x2000, 0x967f1d9a )
	ROM_LOAD( "epr-5865.63",	0x6000, 0x2000, 0x03d9a34c )
	ROM_LOAD( "epr-5864.66",	0x8000, 0x2000, 0xe659f358 )
	ROM_LOAD( "epr-5863.65",	0xa000, 0x2000, 0xa496ca15 )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-5855.117",	0x0000, 0x4000, 0xb5f894a1 )
	ROM_LOAD( "epr-5856.110",	0x4000, 0x4000, 0x266af78f )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( wmatch )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "wm.129",			0x0000, 0x2000, 0xb6db4442 ) /* encrypted */
	ROM_LOAD( "wm.130",			0x2000, 0x2000, 0x59a0a7a0 ) /* encrypted */
	ROM_LOAD( "wm.131",			0x4000, 0x2000, 0x4cb3856a ) /* encrypted */
	ROM_LOAD( "wm.132",			0x6000, 0x2000, 0xe2e44b29 ) /* encrypted */
	ROM_LOAD( "wm.133",			0x8000, 0x2000, 0x43a36445 )
	ROM_LOAD( "wm.134",			0xa000, 0x2000, 0x5624794c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "wm.3",			0x0000, 0x2000, 0x50d2afb7 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "wm.82",			0x0000, 0x2000, 0x540f0bf3 )
	ROM_LOAD( "wm.65",			0x2000, 0x2000, 0x92c1e39e )
	ROM_LOAD( "wm.81",			0x4000, 0x2000, 0x6a01ff2a )
	ROM_LOAD( "wm.64",			0x6000, 0x2000, 0xaae6449b )
	ROM_LOAD( "wm.80",			0x8000, 0x2000, 0xfc3f0bd4 )
	ROM_LOAD( "wm.63",			0xa000, 0x2000, 0xc2ce9b93 )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "wm.86",			0x0000, 0x4000, 0x238ae0e5 )
	ROM_LOAD( "wm.93",			0x4000, 0x4000, 0xa2f19170 )

	ROM_REGION( 0x0100, REGION_USER1, 0 )	/* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.106",	0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( bullfgt )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-.129",		0x0000, 0x2000, 0x29f19156 ) /* encrypted */
	ROM_LOAD( "epr-.130",		0x2000, 0x2000, 0xe37d2b95 ) /* encrypted */
	ROM_LOAD( "epr-.131",		0x4000, 0x2000, 0xeaf5773d ) /* encrypted */
	ROM_LOAD( "epr-.132",		0x6000, 0x2000, 0x72c3c712 ) /* encrypted */
	ROM_LOAD( "epr-.133",		0x8000, 0x2000, 0x7d9fa4cd )
	ROM_LOAD( "epr-.134",		0xa000, 0x2000, 0x061f2797 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6077.120",	0x0000, 0x2000, 0x02a37602 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-.82",		0x0000, 0x2000, 0xb71c349f )
	ROM_LOAD( "epr-.65",		0x2000, 0x2000, 0x86deafa8 )
	ROM_LOAD( "epr-6087.64",	0x4000, 0x2000, 0x2677742c ) /* epr-6087.81 */
	ROM_LOAD( "epr-.64",		0x6000, 0x2000, 0x6f0a62be )
	ROM_LOAD( "epr-6085.66",	0x8000, 0x2000, 0x9c3ddc62 ) /* epr-6085.80 */
	ROM_LOAD( "epr-.63",		0xa000, 0x2000, 0xc0fce57c )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-6069.117",	0x0000, 0x4000, 0xfe691e41 ) /* epr-6069.86 */
	ROM_LOAD( "epr-6070.110",	0x4000, 0x4000, 0x34f080df ) /* epr-6070.93 */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
															 /* pr-5317.106 */
ROM_END

ROM_START( thetogyu )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-6071.116",	0x0000, 0x4000, 0x96b57df9 ) /* encrypted */
	ROM_LOAD( "epr-6072.109",	0x4000, 0x4000, 0xf7baadd0 ) /* encrypted */
	ROM_LOAD( "epr-6073.96",	0x8000, 0x4000, 0x721af166 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6077.120",	0x0000, 0x2000, 0x02a37602 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6089.62",	0x0000, 0x2000, 0xa183e5ff )
	ROM_LOAD( "epr-6088.61",	0x2000, 0x2000, 0xb919b4a6 )
	ROM_LOAD( "epr-6087.64",	0x4000, 0x2000, 0x2677742c )
	ROM_LOAD( "epr-6086.63",	0x6000, 0x2000, 0x76b5a084 )
	ROM_LOAD( "epr-6085.66",	0x8000, 0x2000, 0x9c3ddc62 )
	ROM_LOAD( "epr-6084.65",	0xa000, 0x2000, 0x90e1fa5f )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-6069.117",	0x0000, 0x4000, 0xfe691e41 )
	ROM_LOAD( "epr-6070.110",	0x4000, 0x4000, 0x34f080df )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( pitfall2 )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr6456a.116",	0x0000, 0x4000, 0xbcc8406b ) /* encrypted */
	ROM_LOAD( "epr6457a.109",	0x4000, 0x4000, 0xa016fd2a ) /* encrypted */
	ROM_LOAD( "epr6458a.96",	0x8000, 0x4000, 0x5c30b3e8 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6462.120",	0x0000, 0x2000, 0x86bb9185 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr6474a.62",	0x0000, 0x2000, 0x9f1711b9 )
	ROM_LOAD( "epr6473a.61",	0x2000, 0x2000, 0x8e53b8dd )
	ROM_LOAD( "epr6472a.64",	0x4000, 0x2000, 0xe0f34a11 )
	ROM_LOAD( "epr6471a.63",	0x6000, 0x2000, 0xd5bc805c )
	ROM_LOAD( "epr6470a.66",	0x8000, 0x2000, 0x1439729f )
	ROM_LOAD( "epr6469a.65",	0xa000, 0x2000, 0xe4ac6921 )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr6454a.117",	0x0000, 0x4000, 0xa5d96780 )
	ROM_LOAD( "epr-6455.05",	0x4000, 0x4000, 0x32ee64a1 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( pitfallu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "epr-6623.116",	0x0000, 0x4000, 0xbcb47ed6 )
	ROM_LOAD( "epr6624a.109",	0x4000, 0x4000, 0x6e8b09c1 )
	ROM_LOAD( "epr-6625.96",	0x8000, 0x4000, 0xdc5484ba )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6462.120",	0x0000, 0x2000, 0x86bb9185 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr6474a.62",	0x0000, 0x2000, 0x9f1711b9 )
	ROM_LOAD( "epr6473a.61",	0x2000, 0x2000, 0x8e53b8dd )
	ROM_LOAD( "epr6472a.64",	0x4000, 0x2000, 0xe0f34a11 )
	ROM_LOAD( "epr6471a.63",	0x6000, 0x2000, 0xd5bc805c )
	ROM_LOAD( "epr6470a.66",	0x8000, 0x2000, 0x1439729f )
	ROM_LOAD( "epr6469a.65",	0xa000, 0x2000, 0xe4ac6921 )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr6454a.117",	0x0000, 0x4000, 0xa5d96780 )
	ROM_LOAD( "epr-6455.05",	0x4000, 0x4000, 0x32ee64a1 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( seganinj )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-.116",		0x0000, 0x4000, 0xa5d0c9d0 ) /* encrypted */
	ROM_LOAD( "epr-.109",		0x4000, 0x4000, 0xb9e6775c ) /* encrypted */
	ROM_LOAD( "epr-6552.96",	0x8000, 0x4000, 0xf2eeb0d8 ) /* epr-7151.96 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6559.120",	0x0000, 0x2000, 0x5a1570ee )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6558.62",	0x0000, 0x2000, 0x2af9eaeb )
	ROM_LOAD( "epr-6592.61",	0x2000, 0x2000, 0x7804db86 )
	ROM_LOAD( "epr-6556.64",	0x4000, 0x2000, 0x79fd26f7 )
	ROM_LOAD( "epr-6590.63",	0x6000, 0x2000, 0xbf858cad )
	ROM_LOAD( "epr-6554.66",	0x8000, 0x2000, 0x5ac9d205 )
	ROM_LOAD( "epr-6588.65",	0xa000, 0x2000, 0xdc931dbb )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6546.117",	0x0000, 0x4000, 0xa4785692 )
	ROM_LOAD( "epr-6548.04",	0x4000, 0x4000, 0xbdf278c1 )
	ROM_LOAD( "epr-6547.110",	0x8000, 0x4000, 0x34451b08 )
	ROM_LOAD( "epr-6549.05",	0xc000, 0x4000, 0xd2057668 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( seganinu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "epr-7149.116",	0x0000, 0x4000, 0xcd9fade7 )
	ROM_LOAD( "epr-7150.109",	0x4000, 0x4000, 0xc36351e2 )
	ROM_LOAD( "epr-6552.96",	0x8000, 0x4000, 0xf2eeb0d8 ) /* epr-7151.96 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6559.120",	0x0000, 0x2000, 0x5a1570ee )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6558.62",	0x0000, 0x2000, 0x2af9eaeb )
	ROM_LOAD( "epr-6592.61",	0x2000, 0x2000, 0x7804db86 )
	ROM_LOAD( "epr-6556.64",	0x4000, 0x2000, 0x79fd26f7 )
	ROM_LOAD( "epr-6590.63",	0x6000, 0x2000, 0xbf858cad )
	ROM_LOAD( "epr-6554.66",	0x8000, 0x2000, 0x5ac9d205 )
	ROM_LOAD( "epr-6588.65",	0xa000, 0x2000, 0xdc931dbb )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6546.117",	0x0000, 0x4000, 0xa4785692 )
	ROM_LOAD( "epr-6548.04",	0x4000, 0x4000, 0xbdf278c1 )
	ROM_LOAD( "epr-6547.110",	0x8000, 0x4000, 0x34451b08 )
	ROM_LOAD( "epr-6549.05",	0xc000, 0x4000, 0xd2057668 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( nprinces )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-6612.129",	0x0000, 0x2000, 0x1b30976f ) /* encrypted */
	ROM_LOAD( "epr-6613.130",	0x2000, 0x2000, 0x18281f27 ) /* encrypted */
	ROM_LOAD( "epr-6614.131",	0x4000, 0x2000, 0x69fc3d73 ) /* encrypted */
	ROM_LOAD( "epr-6615.132",	0x6000, 0x2000, 0x1d0374c8 ) /* encrypted */
	ROM_LOAD( "epr-6577.133",	0x8000, 0x2000, 0x73616e03 ) /* epr-6616.133 */
	ROM_LOAD( "epr-6617.134",	0xa000, 0x2000, 0x20b6f895 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6559.120",	0x0000, 0x2000, 0x5a1570ee )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6558.62",	0x0000, 0x2000, 0x2af9eaeb ) /* epr-6558.82 */
	ROM_LOAD( "epr-6557.61",	0x2000, 0x2000, 0x6eb131d0 ) /* epr-6557.65 */
	ROM_LOAD( "epr-6556.64",	0x4000, 0x2000, 0x79fd26f7 ) /* epr-6556.81 */
	ROM_LOAD( "epr-6555.63",	0x6000, 0x2000, 0x7f669aac ) /* epr-6555.64 */
	ROM_LOAD( "epr-6554.66",	0x8000, 0x2000, 0x5ac9d205 ) /* epr-6554.80 */
	ROM_LOAD( "epr-6553.65",	0xa000, 0x2000, 0xeb82a8fe ) /* epr-6553.63 */

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6546.117",	0x0000, 0x4000, 0xa4785692 ) /* epr-6546.3 */
	ROM_LOAD( "epr-6548.04",	0x4000, 0x4000, 0xbdf278c1 ) /* epr-6548.1 */
	ROM_LOAD( "epr-6547.110",	0x8000, 0x4000, 0x34451b08 ) /* epr-6547.4 */
	ROM_LOAD( "epr-6549.05",	0xc000, 0x4000, 0xd2057668 ) /* epr-6549.2 */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
															 /* pr-5317.106 */
ROM_END

ROM_START( nprincso )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-6550.116",	0x0000, 0x4000, 0x5f6d59f1 ) /* encrypted */
	ROM_LOAD( "epr-6551.109",	0x4000, 0x4000, 0x1af133b2 ) /* encrypted */
	ROM_LOAD( "epr-6552.96",	0x8000, 0x4000, 0xf2eeb0d8 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6559.120",	0x0000, 0x2000, 0x5a1570ee )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6558.62",	0x0000, 0x2000, 0x2af9eaeb )
	ROM_LOAD( "epr-6557.61",	0x2000, 0x2000, 0x6eb131d0 )
	ROM_LOAD( "epr-6556.64",	0x4000, 0x2000, 0x79fd26f7 )
	ROM_LOAD( "epr-6555.63",	0x6000, 0x2000, 0x7f669aac )
	ROM_LOAD( "epr-6554.66",	0x8000, 0x2000, 0x5ac9d205 )
	ROM_LOAD( "epr-6553.65",	0xa000, 0x2000, 0xeb82a8fe )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6546.117",	0x0000, 0x4000, 0xa4785692 )
	ROM_LOAD( "epr-6548.04",	0x4000, 0x4000, 0xbdf278c1 )
	ROM_LOAD( "epr-6547.110",	0x8000, 0x4000, 0x34451b08 )
	ROM_LOAD( "epr-6549.05",	0xc000, 0x4000, 0xd2057668 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( nprincsu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "epr-6573.129",	0x0000, 0x2000, 0xd2919c7d )
	ROM_LOAD( "epr-6574.130",	0x2000, 0x2000, 0x5a132833 )
	ROM_LOAD( "epr-6575.131",	0x4000, 0x2000, 0xa94b0bd4 )
	ROM_LOAD( "epr-6576.132",	0x6000, 0x2000, 0x27d3bbdb )
	ROM_LOAD( "epr-6577.133",	0x8000, 0x2000, 0x73616e03 )
	ROM_LOAD( "epr-6578.134",	0xa000, 0x2000, 0xab68499f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6559.120",	0x0000, 0x2000, 0x5a1570ee )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6558.62",	0x0000, 0x2000, 0x2af9eaeb ) /* epr-6558.82 */
	ROM_LOAD( "epr-6557.61",	0x2000, 0x2000, 0x6eb131d0 ) /* epr-6557.65 */
	ROM_LOAD( "epr-6556.64",	0x4000, 0x2000, 0x79fd26f7 ) /* epr-6556.81 */
	ROM_LOAD( "epr-6555.63",	0x6000, 0x2000, 0x7f669aac ) /* epr-6555.64 */
	ROM_LOAD( "epr-6554.66",	0x8000, 0x2000, 0x5ac9d205 ) /* epr-6554.80 */
	ROM_LOAD( "epr-6553.65",	0xa000, 0x2000, 0xeb82a8fe ) /* epr-6553.63 */

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6546.117",	0x0000, 0x4000, 0xa4785692 ) /* epr-6546.3 */
	ROM_LOAD( "epr-6548.04",	0x4000, 0x4000, 0xbdf278c1 ) /* epr-6548.1 */
	ROM_LOAD( "epr-6547.110",	0x8000, 0x4000, 0x34451b08 ) /* epr-6547.4 */
	ROM_LOAD( "epr-6549.05",	0xc000, 0x4000, 0xd2057668 ) /* epr-6549.2 */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( nprincsb )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "nprinces.001",	0x0000, 0x4000, 0xe0de073c )  /* encrypted */
	ROM_LOAD( "nprinces.002",	0x4000, 0x4000, 0x27219c7f )  /* encrypted */
	ROM_LOAD( "epr-6552.96",	0x8000, 0x4000, 0xf2eeb0d8 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6559.120",	0x0000, 0x2000, 0x5a1570ee )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6558.62",	0x0000, 0x2000, 0x2af9eaeb )
	ROM_LOAD( "epr-6557.61",	0x2000, 0x2000, 0x6eb131d0 )
	ROM_LOAD( "epr-6556.64",	0x4000, 0x2000, 0x79fd26f7 )
	ROM_LOAD( "epr-6555.63",	0x6000, 0x2000, 0x7f669aac )
	ROM_LOAD( "epr-6554.66",	0x8000, 0x2000, 0x5ac9d205 )
	ROM_LOAD( "epr-6553.65",	0xa000, 0x2000, 0xeb82a8fe )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6546.117",	0x0000, 0x4000, 0xa4785692 )
	ROM_LOAD( "epr-6548.04",	0x4000, 0x4000, 0xbdf278c1 )
	ROM_LOAD( "epr-6547.110",	0x8000, 0x4000, 0x34451b08 )
	ROM_LOAD( "epr-6549.05",	0xc000, 0x4000, 0xd2057668 )

	ROM_REGION( 0x0220, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
	ROM_LOAD( "nprinces.129",	0x0100, 0x0100, 0xae765f62 ) /* decryption table (not used) */
	ROM_LOAD( "nprinces.123",	0x0200, 0x0020, 0xed5146e9 ) /* decryption table (not used) */
ROM_END

ROM_START( imsorry )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-6676.116",	0x0000, 0x4000, 0xeb087d7f ) /* encrypted */
	ROM_LOAD( "epr-6677.109",	0x4000, 0x4000, 0xbd244bee ) /* encrypted */
	ROM_LOAD( "epr-6678.96",	0x8000, 0x4000, 0x2e16b9fd )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6656.120",	0x0000, 0x2000, 0x25e3d685 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6684.62",	0x0000, 0x2000, 0x2c8df377 )
	ROM_LOAD( "epr-6683.61",	0x2000, 0x2000, 0x89431c48 )
	ROM_LOAD( "epr-6682.64",	0x4000, 0x2000, 0x256a9246 )
	ROM_LOAD( "epr-6681.63",	0x6000, 0x2000, 0x6974d189 )
	ROM_LOAD( "epr-6680.66",	0x8000, 0x2000, 0x10a629d6 )
	ROM_LOAD( "epr-6674.65",	0xa000, 0x2000, 0x143d883c )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-6645.117",	0x0000, 0x4000, 0x1ba167ee )
	ROM_LOAD( "epr-6646.04",	0x4000, 0x4000, 0xedda7ad6 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( imsorryj )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-6647.116",	0x0000, 0x4000, 0xcc5d915d ) /* encrypted */
	ROM_LOAD( "epr-6648.109",	0x4000, 0x4000, 0x37574d60 ) /* encrypted */
	ROM_LOAD( "epr-6649.96",	0x8000, 0x4000, 0x5f59bdee )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6656.120",	0x0000, 0x2000, 0x25e3d685 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6655.62",	0x0000, 0x2000, 0xbe1f762f )
	ROM_LOAD( "epr-6654.61",	0x2000, 0x2000, 0xed5f7fc8 )
	ROM_LOAD( "epr-6653.64",	0x4000, 0x2000, 0x8b4845a7 )
	ROM_LOAD( "epr-6652.63",	0x6000, 0x2000, 0x001d68cb )
	ROM_LOAD( "epr-6651.66",	0x8000, 0x2000, 0x4ee9b5e6 )
	ROM_LOAD( "epr-6650.65",	0xa000, 0x2000, 0x3fca4414 )

	ROM_REGION( 0x8000, REGION_GFX2, 0 ) /* 32k for sprites data */
	ROM_LOAD( "epr-6645.117",	0x0000, 0x4000, 0x1ba167ee )
	ROM_LOAD( "epr-6646.04",	0x4000, 0x4000, 0xedda7ad6 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( teddybb )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-6768.116",	0x0000, 0x4000, 0x5939817e ) /* encrypted */
	ROM_LOAD( "epr-6769.109",	0x4000, 0x4000, 0x14a98ddd ) /* encrypted */
	ROM_LOAD( "epr-6770.96",	0x8000, 0x4000, 0x67b0c7c2 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr6748x.120",	0x0000, 0x2000, 0xc2a1b89d )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6747.62",	0x0000, 0x2000, 0xa0e5aca7 ) /* epr-6776.62 */
	ROM_LOAD( "epr-6746.61",	0x2000, 0x2000, 0xcdb77e51 ) /* epr-6775.61 */
	ROM_LOAD( "epr-6745.64",	0x4000, 0x2000, 0x0cab75c3 ) /* epr-6774.64 */
	ROM_LOAD( "epr-6744.63",	0x6000, 0x2000, 0x0ef8d2cd ) /* epr-6773.63 */
	ROM_LOAD( "epr-6743.66",	0x8000, 0x2000, 0xc33062b5 ) /* epr-6772.66 */
	ROM_LOAD( "epr-6742.65",	0xa000, 0x2000, 0xc457e8c5 ) /* epr-6771.65 */

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6735.117",	0x0000, 0x4000, 0x1be35a97 )
	ROM_LOAD( "epr-6737.04",	0x4000, 0x4000, 0x6b53aa7a )
	ROM_LOAD( "epr-6736.110",	0x8000, 0x4000, 0x565c25d0 )
	ROM_LOAD( "epr-6738.05",	0xc000, 0x4000, 0xe116285f )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( teddybbo )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-6739.116",	0x0000, 0x4000, 0x81a37e69 ) /* encrypted */
	ROM_LOAD( "epr-6740.109",	0x4000, 0x4000, 0x715388a9 ) /* encrypted */
	ROM_LOAD( "epr-6741.96",	0x8000, 0x4000, 0xe5a74f5f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6748.120",	0x0000, 0x2000, 0x9325a1cf )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6747.62",	0x0000, 0x2000, 0xa0e5aca7 )
	ROM_LOAD( "epr-6746.61",	0x2000, 0x2000, 0xcdb77e51 )
	ROM_LOAD( "epr-6745.64",	0x4000, 0x2000, 0x0cab75c3 )
	ROM_LOAD( "epr-6744.63",	0x6000, 0x2000, 0x0ef8d2cd )
	ROM_LOAD( "epr-6743.66",	0x8000, 0x2000, 0xc33062b5 )
	ROM_LOAD( "epr-6742.65",	0xa000, 0x2000, 0xc457e8c5 )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6735.117",	0x0000, 0x4000, 0x1be35a97 )
	ROM_LOAD( "epr-6737.04",	0x4000, 0x4000, 0x6b53aa7a )
	ROM_LOAD( "epr-6736.110",	0x8000, 0x4000, 0x565c25d0 )
	ROM_LOAD( "epr-6738.05",	0xc000, 0x4000, 0xe116285f )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

/* This is the first System 1 game to have extended ROM space */
ROM_START( hvymetal )
	ROM_REGION( 2*0x20000, REGION_CPU1, 0 ) /* 128k for code + 128k for decrypted opcodes */
	ROM_LOAD( "epra6790.1",   0x00000, 0x8000, 0x59195bb9 ) /* encrypted */
	ROM_LOAD( "epra6789.2",   0x10000, 0x8000, 0x83e1d18a )
	ROM_LOAD( "epra6788.3",   0x18000, 0x8000, 0x6ecefd57 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr6787.120",  0x0000, 0x8000, 0xb64ac7f0 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr6795.62",   0x00000, 0x4000, 0x58a3d038 )
	ROM_LOAD( "epr6796.61",   0x04000, 0x4000, 0xd8b08a55 )
	ROM_LOAD( "epr6793.64",   0x08000, 0x4000, 0x487407c2 )
	ROM_LOAD( "epr6794.63",   0x0c000, 0x4000, 0x89eb3793 )
	ROM_LOAD( "epr6791.66",   0x10000, 0x4000, 0xa7dcd042 )
	ROM_LOAD( "epr6792.65",   0x14000, 0x4000, 0xd0be5e33 )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr6778.117",  0x00000, 0x8000, 0x0af61aee )
	ROM_LOAD( "epr6777.110",  0x08000, 0x8000, 0x91d7a197 )
	ROM_LOAD( "epr6780.4",    0x10000, 0x8000, 0x55b31df5 )
	ROM_LOAD( "epr6779.5",    0x18000, 0x8000, 0xe03a2b28 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "pr7036.3",     0x0000, 0x0100, 0x146f16fb ) /* palette red component */
	ROM_LOAD( "pr7035.2",     0x0100, 0x0100, 0x50b201ed ) /* palette green component */
	ROM_LOAD( "pr7034.1",     0x0200, 0x0100, 0xdfb5f139 ) /* palette blue component */
	ROM_LOAD( "pr5317p.4",    0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( myhero )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "epr6963b.116",	0x0000, 0x4000, 0x4daf89d4 )
	ROM_LOAD( "epr6964a.109",	0x4000, 0x4000, 0xc26188e5 )
	ROM_LOAD( "epr-6927.96",	0x8000, 0x4000, 0x3cbbaf64 ) /* epr-6965.96 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-69xx.120",	0x0000, 0x2000, 0x0039e1e9 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6966.62",	0x0000, 0x2000, 0x157f0401 )
	ROM_LOAD( "epr-6961.61",	0x2000, 0x2000, 0xbe53ce47 )
	ROM_LOAD( "epr-6960.64",	0x4000, 0x2000, 0xbd381baa )
	ROM_LOAD( "epr-6959.63",	0x6000, 0x2000, 0xbc04e79a )
	ROM_LOAD( "epr-6958.66",	0x8000, 0x2000, 0x714f2c26 )
	ROM_LOAD( "epr-6958.65",	0xa000, 0x2000, 0x80920112 )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6921.117",	0x0000, 0x4000, 0xf19e05a1 )
	ROM_LOAD( "epr-6923.04",	0x4000, 0x4000, 0x7988adc3 )
	ROM_LOAD( "epr-6922.110",	0x8000, 0x4000, 0x37f77a78 )
	ROM_LOAD( "epr-6924.05",	0xc000, 0x4000, 0x42bdc8f6 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( sscandal )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr6925b.116",	0x0000, 0x4000, 0xff54dcec ) /* encrypted */
	ROM_LOAD( "epr6926a.109",	0x4000, 0x4000, 0x5c41eea8 ) /* encrypted */
	ROM_LOAD( "epr-6927.96",	0x8000, 0x4000, 0x3cbbaf64 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6934.120",	0x0000, 0x2000, 0xaf467223 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6933.62",	0x0000, 0x2000, 0xe7304036 )
	ROM_LOAD( "epr-6932.61",	0x2000, 0x2000, 0xf5cfbfda )
	ROM_LOAD( "epr-6931.64",	0x4000, 0x2000, 0x599d7f87 )
	ROM_LOAD( "epr-6930.63",	0x6000, 0x2000, 0xcb6616c2 )
	ROM_LOAD( "epr-6929.66",	0x8000, 0x2000, 0x27a16856 )
	ROM_LOAD( "epr-6928.65",	0xa000, 0x2000, 0xc0c9cfa4 )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6921.117",	0x0000, 0x4000, 0xf19e05a1 )
	ROM_LOAD( "epr-6923.04",	0x4000, 0x4000, 0x7988adc3 )
	ROM_LOAD( "epr-6922.110",	0x8000, 0x4000, 0x37f77a78 )
	ROM_LOAD( "epr-6924.05",	0xc000, 0x4000, 0x42bdc8f6 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( myherok )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	/* all the three program ROMs have bits 0-1 swapped */
	/* when decoded, they are identical to the Japanese version */
	ROM_LOAD( "ry-11.rom",		0x0000, 0x4000, 0x6f4c8ee5 ) /* encrypted */
	ROM_LOAD( "ry-09.rom",		0x4000, 0x4000, 0x369302a1 ) /* encrypted */
	ROM_LOAD( "ry-07.rom",		0x8000, 0x4000, 0xb8e9922e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6934.120",	0x0000, 0x2000, 0xaf467223 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	/* all three gfx ROMs have address lines A4 and A5 swapped, also #1 and #3 */
	/* have data lines D0 and D6 swapped, while #2 has data lines D1 and D5 swapped. */
	ROM_LOAD( "ry-04.rom",		0x0000, 0x4000, 0xdfb75143 )
	ROM_LOAD( "ry-03.rom",		0x4000, 0x4000, 0xcf68b4a2 )
	ROM_LOAD( "ry-02.rom",		0x8000, 0x4000, 0xd100eaef )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6921.117",	0x0000, 0x4000, 0xf19e05a1 )
	ROM_LOAD( "epr-6923.04",	0x4000, 0x4000, 0x7988adc3 )
	ROM_LOAD( "epr-6922.110",	0x8000, 0x4000, 0x37f77a78 )
	ROM_LOAD( "epr-6924.05",	0xc000, 0x4000, 0x42bdc8f6 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( shtngmst )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 128k for code */
	ROM_LOAD( "epr7100",      0x00000, 0x8000, 0x45e64431 )
	ROM_LOAD( "epr7101",      0x10000, 0x8000, 0xebf5ff72 )
	ROM_LOAD( "epr7102",      0x18000, 0x8000, 0xc890a4ad )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr7043",      0x0000, 0x8000, 0x99a368ab )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr7040",      0x00000, 0x8000, 0xf30769fa )
	ROM_LOAD( "epr7041",      0x08000, 0x8000, 0xf3e273f9 )
	ROM_LOAD( "epr7042",      0x10000, 0x8000, 0x6841c917 )

	ROM_REGION( 0x38000, REGION_GFX2, 0 ) /* 224 for sprites data - PROBABLY WRONG! */
	ROM_LOAD( "epr7105",      0x00000, 0x8000, 0x13111729 )
	ROM_LOAD( "epr7104",      0x08000, 0x8000, 0x84a679c5 )
	ROM_LOAD( "epr7107",      0x10000, 0x8000, 0x8f50ea24 )
	ROM_LOAD( "epr7106",      0x18000, 0x8000, 0xae7ab7a2 )
	ROM_LOAD( "epr7109",      0x20000, 0x8000, 0x097f7481 )
	ROM_LOAD( "epr7108",      0x28000, 0x8000, 0x816180ac )
	ROM_LOAD( "epr7110",      0x30000, 0x8000, 0x5d1a5048 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "epr7113",      0x0000, 0x0100, 0x5c0e1360 ) /* palette red component */
	ROM_LOAD( "epr7112",      0x0100, 0x0100, 0x46fbd351 ) /* palette green component */
	ROM_LOAD( "epr7111",      0x0200, 0x0100, 0x8123b6b9 ) /* palette blue component */
	ROM_LOAD( "epr5317",      0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( chplft )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 128k for code */
	ROM_LOAD( "epr-7124.90",	0x00000, 0x8000, 0x678d5c41 )
	ROM_LOAD( "epr-7125.91",	0x10000, 0x8000, 0xf5283498 )
	ROM_LOAD( "epr-7126.92",	0x18000, 0x8000, 0xdbd192ab )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-7130.126",	0x0000, 0x8000, 0x346af118 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7127.4",		0x00000, 0x8000, 0x1e708f6d )
	ROM_LOAD( "epr-7128.5",		0x08000, 0x8000, 0xb922e787 )
	ROM_LOAD( "epr-7129.6",		0x10000, 0x8000, 0xbd3b6e6e )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr-7121.87",	0x00000, 0x8000, 0xf2b88f73 )
	ROM_LOAD( "epr-7120.86",	0x08000, 0x8000, 0x517d7fd3 )
	ROM_LOAD( "epr-7123.89",	0x10000, 0x8000, 0x8f16a303 )
	ROM_LOAD( "epr-7122.88",	0x18000, 0x8000, 0x7c93f160 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "pr7119.20",		0x0000, 0x0100, 0xb2a8260f ) /* palette red component */
	ROM_LOAD( "pr7118.14",		0x0100, 0x0100, 0x693e20c7 ) /* palette green component */
	ROM_LOAD( "pr7117.8",		0x0200, 0x0100, 0x4124307e ) /* palette blue component */
	ROM_LOAD( "pr5317.28",		0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( chplftb )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 128k for code */
	ROM_LOAD( "epr-7152.90",	0x00000, 0x8000, 0xfe49d83e )
	ROM_LOAD( "epr-7153.91",	0x10000, 0x8000, 0x48697666 )
	ROM_LOAD( "epr-7154.92",	0x18000, 0x8000, 0x56d6222a )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-7130.126",	0x0000, 0x8000, 0x346af118 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7127.4",		0x00000, 0x8000, 0x1e708f6d )
	ROM_LOAD( "epr-7128.5",		0x08000, 0x8000, 0xb922e787 )
	ROM_LOAD( "epr-7129.6",		0x10000, 0x8000, 0xbd3b6e6e )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr-7121.87",	0x00000, 0x8000, 0xf2b88f73 )
	ROM_LOAD( "epr-7120.86",	0x08000, 0x8000, 0x517d7fd3 )
	ROM_LOAD( "epr-7123.89",	0x10000, 0x8000, 0x8f16a303 )
	ROM_LOAD( "epr-7122.88",	0x18000, 0x8000, 0x7c93f160 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "pr7119.20",		0x0000, 0x0100, 0xb2a8260f ) /* palette red component */
	ROM_LOAD( "pr7118.14",		0x0100, 0x0100, 0x693e20c7 ) /* palette green component */
	ROM_LOAD( "pr7117.8",		0x0200, 0x0100, 0x4124307e ) /* palette blue component */
	ROM_LOAD( "pr5317.28",		0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( chplftbl )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 128k for code */
	ROM_LOAD( "ep7124bl.90",	0x00000, 0x8000, 0x71a37932 )
	ROM_LOAD( "epr-7125.91",	0x10000, 0x8000, 0xf5283498 )
	ROM_LOAD( "epr-7126.92",	0x18000, 0x8000, 0xdbd192ab )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-7130.126",	0x0000, 0x8000, 0x346af118 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7127.4",		0x00000, 0x8000, 0x1e708f6d )
	ROM_LOAD( "epr-7128.5",		0x08000, 0x8000, 0xb922e787 )
	ROM_LOAD( "epr-7129.6",		0x10000, 0x8000, 0xbd3b6e6e )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr-7121.87",	0x00000, 0x8000, 0xf2b88f73 )
	ROM_LOAD( "epr-7120.86",	0x08000, 0x8000, 0x517d7fd3 )
	ROM_LOAD( "epr-7123.89",	0x10000, 0x8000, 0x8f16a303 )
	ROM_LOAD( "epr-7122.88",	0x18000, 0x8000, 0x7c93f160 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "pr7119.20",		0x0000, 0x0100, 0xb2a8260f ) /* palette red component */
	ROM_LOAD( "pr7118.14",		0x0100, 0x0100, 0x693e20c7 ) /* palette green component */
	ROM_LOAD( "pr7117.8",		0x0200, 0x0100, 0x4124307e ) /* palette blue component */
	ROM_LOAD( "pr5317.28",		0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( 4dwarrio )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "4d.116",       0x0000, 0x4000, 0x546d1bc7 ) /* encrypted */
	ROM_LOAD( "4d.109",       0x4000, 0x4000, 0xf1074ec3 ) /* encrypted */
	ROM_LOAD( "4d.96",        0x8000, 0x4000, 0x387c1e8f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "4d.120",       0x0000, 0x2000, 0x5241c009 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "4d.62",        0x0000, 0x2000, 0xf31b2e09 )
	ROM_LOAD( "4d.61",        0x2000, 0x2000, 0x5430e925 )
	ROM_LOAD( "4d.64",        0x4000, 0x2000, 0x9f442351 )
	ROM_LOAD( "4d.63",        0x6000, 0x2000, 0x633232bd )
	ROM_LOAD( "4d.66",        0x8000, 0x2000, 0x52bfa2ed )
	ROM_LOAD( "4d.65",        0xa000, 0x2000, 0xe9ba4658 )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "4d.117",       0x0000, 0x4000, 0x436e4141 )
	ROM_LOAD( "4d.04",        0x4000, 0x4000, 0x8b7cecef )
	ROM_LOAD( "4d.110",       0x8000, 0x4000, 0x6ec5990a )
	ROM_LOAD( "4d.05",        0xc000, 0x4000, 0xf31a1e6a )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr5317.76",    0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( brain )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 128k for code */
	ROM_LOAD( "brain.1",      0x00000, 0x8000, 0x2d2aec31 )
	ROM_LOAD( "brain.2",      0x10000, 0x8000, 0x810a8ab5 )
	ROM_RELOAD(               0x08000, 0x8000 )	/* there's code falling through from 7fff */
												/* so I have to copy the ROM there */
	ROM_LOAD( "brain.3",      0x18000, 0x8000, 0x9a225634 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "brain.120",    0x0000, 0x8000, 0xc7e50278 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "brain.62",     0x0000, 0x4000, 0x7dce2302 )
	ROM_LOAD( "brain.64",     0x4000, 0x4000, 0x7ce03fd3 )
	ROM_LOAD( "brain.66",     0x8000, 0x4000, 0xea54323f )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "brain.117",    0x00000, 0x8000, 0x92ff71a4 )
	ROM_LOAD( "brain.110",    0x08000, 0x8000, 0xa1b847ec )
	ROM_LOAD( "brain.4",      0x10000, 0x8000, 0xfd2ea53b )
	/* 18000-1ffff empty */

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "bprom.3",      0x0000, 0x0100, BADCRC( 0x8eee0f72 ) ) /* palette red component */
	ROM_LOAD( "bprom.2",      0x0100, 0x0100, BADCRC( 0x3e7babd7 ) ) /* palette green component */
	ROM_LOAD( "bprom.1",      0x0200, 0x0100, BADCRC( 0x371c44a6 ) ) /* palette blue component */
	ROM_LOAD( "pr5317.76",    0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( raflesia )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-7411.116",	0x0000, 0x4000, 0x88a0c6c6 ) /* encrypted */
	ROM_LOAD( "epr-7412.109",	0x4000, 0x4000, 0xd3b8cddf ) /* encrypted */
	ROM_LOAD( "epr-7413.96",	0x8000, 0x4000, 0xb7e688b3 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-7420.120",	0x0000, 0x2000, 0x14387666 ) /* epr-7420.3 */

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7419.62",	0x0000, 0x2000, 0xbfd5f34c ) /* epr-7419.82 */
	ROM_LOAD( "epr-7418.61",	0x2000, 0x2000, 0xf8cbc9b6 ) /* epr-7418.65 */
	ROM_LOAD( "epr-7417.64",	0x4000, 0x2000, 0xe63501bc ) /* epr-7417.81 */
	ROM_LOAD( "epr-7416.63",	0x6000, 0x2000, 0x093e5693 ) /* epr-7416.64 */
	ROM_LOAD( "epr-7415.66",	0x8000, 0x2000, 0x1a8d6bd6 ) /* epr-7415.80 */
	ROM_LOAD( "epr-7414.65",	0xa000, 0x2000, 0x5d20f218 ) /* epr-7414.63 */

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-7407.117",	0x0000, 0x4000, 0xf09fc057 ) /* epr-7407.3 */
	ROM_LOAD( "epr-7409.04",	0x4000, 0x4000, 0x819fedb8 ) /* epr-7409.1 */
	ROM_LOAD( "epr-7408.110",	0x8000, 0x4000, 0x3189f33c ) /* epr-7408.4 */
	ROM_LOAD( "epr-7410.05",	0xc000, 0x4000, 0xced74789 ) /* epr-7410.2 */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
															 /* pr-5317.106 */
ROM_END

ROM_START( spatter )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-6392.116",	0x0000, 0x4000, 0x329b4506 ) /* encrypted */
	ROM_LOAD( "epr-6393.109",	0x4000, 0x4000, 0x3b56e25f ) /* encrypted */
	ROM_LOAD( "epr-6394.96",	0x8000, 0x4000, 0x647c1301 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6316.120",	0x0000, 0x2000, 0x1df95511 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6328.62",	0x0000, 0x2000, 0xa2bf2832 )
	ROM_LOAD( "epr-6397.61",	0x2000, 0x2000, 0xc60d4471 )
	ROM_LOAD( "epr-6326.64",	0x4000, 0x2000, 0x269fbb4c )
	ROM_LOAD( "epr-6396.63",	0x6000, 0x2000, 0xc15ccf3b )
	ROM_LOAD( "epr-6324.66",	0x8000, 0x2000, 0x8ab3b563 )
	ROM_LOAD( "epr-6395.65",	0xa000, 0x2000, 0x3f083065 )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6306.04",	0x0000, 0x4000, 0xe871e132 )
	ROM_LOAD( "epr-6308.117",	0x4000, 0x4000, 0x99c2d90e )
	ROM_LOAD( "epr-6307.05",	0x8000, 0x4000, 0x0a5ad543 )
	ROM_LOAD( "epr-6309.110",	0xc000, 0x4000, 0x7423ad98 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
															 /* pr-5317.106 */
ROM_END

ROM_START( ssanchan )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-6310.116",	0x0000, 0x4000, 0x26b43701 ) /* encrypted */
	ROM_LOAD( "epr-6311.109",	0x4000, 0x4000, 0xcb2bc620 ) /* encrypted */
	ROM_LOAD( "epr-6312.96",	0x8000, 0x4000, 0x71b15b47 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-6316.120",	0x0000, 0x2000, 0x1df95511 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6328.62",	0x0000, 0x2000, 0xa2bf2832 )
	ROM_LOAD( "epr-6327.61",	0x2000, 0x2000, 0x53298109 )
	ROM_LOAD( "epr-6326.64",	0x4000, 0x2000, 0x269fbb4c )
	ROM_LOAD( "epr-6325.63",	0x6000, 0x2000, 0xbf038745 )
	ROM_LOAD( "epr-6324.66",	0x8000, 0x2000, 0x8ab3b563 )
	ROM_LOAD( "epr-6323.65",	0xa000, 0x2000, 0x0394673c )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-6306.04",	0x0000, 0x4000, 0xe871e132 )
	ROM_LOAD( "epr-6308.117",	0x4000, 0x4000, 0x99c2d90e )
	ROM_LOAD( "epr-6307.05",	0x8000, 0x4000, 0x0a5ad543 )
	ROM_LOAD( "epr-6309.110",	0xc000, 0x4000, 0x7423ad98 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
															 /* pr-5317.106 */
ROM_END

ROM_START( wboy )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-7489.116",	0x0000, 0x4000, 0x130f4b70 ) /* encrypted */
	ROM_LOAD( "epr-7490.109",	0x4000, 0x4000, 0x9e656733 ) /* encrypted */
	ROM_LOAD( "epr-7491.96",	0x8000, 0x4000, 0x1f7d0efe )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-7498.120",	0x0000, 0x2000, 0x78ae1e7b )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7497.62",	0x0000, 0x2000, 0x08d609ca )
	ROM_LOAD( "epr-7496.61",	0x2000, 0x2000, 0x6f61fdf1 )
	ROM_LOAD( "epr-7495.64",	0x4000, 0x2000, 0x6a0d2c2d )
	ROM_LOAD( "epr-7494.63",	0x6000, 0x2000, 0xa8e281c7 )
	ROM_LOAD( "epr-7493.66",	0x8000, 0x2000, 0x89305df4 )
	ROM_LOAD( "epr-7492.65",	0xa000, 0x2000, 0x60f806b1 )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-7485.117",	0x0000, 0x4000, 0xc2891722 )
	ROM_LOAD( "epr-7487.04",	0x4000, 0x4000, 0x2d3a421b )
	ROM_LOAD( "epr-7486.110",	0x8000, 0x4000, 0x8d622c50 )
	ROM_LOAD( "epr-7488.05",	0xc000, 0x4000, 0x007c2f1b )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( wboyo )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-.116",		0x0000, 0x4000, 0x51d27534 ) /* encrypted */
	ROM_LOAD( "epr-.109",		0x4000, 0x4000, 0xe29d1cd1 ) /* encrypted */
	ROM_LOAD( "epr-7491.96",	0x8000, 0x4000, 0x1f7d0efe )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-7498.120",	0x0000, 0x2000, 0x78ae1e7b )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7497.62",	0x0000, 0x2000, 0x08d609ca )
	ROM_LOAD( "epr-7496.61",	0x2000, 0x2000, 0x6f61fdf1 )
	ROM_LOAD( "epr-7495.64",	0x4000, 0x2000, 0x6a0d2c2d )
	ROM_LOAD( "epr-7494.63",	0x6000, 0x2000, 0xa8e281c7 )
	ROM_LOAD( "epr-7493.66",	0x8000, 0x2000, 0x89305df4 )
	ROM_LOAD( "epr-7492.65",	0xa000, 0x2000, 0x60f806b1 )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-7485.117",	0x0000, 0x4000, 0xc2891722 )
	ROM_LOAD( "epr-7487.04",	0x4000, 0x4000, 0x2d3a421b )
	ROM_LOAD( "epr-7486.110",	0x8000, 0x4000, 0x8d622c50 )
	ROM_LOAD( "epr-7488.05",	0xc000, 0x4000, 0x007c2f1b )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( wboy2 )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr-7587.129",	0x0000, 0x2000, 0x1bbb7354 ) /* encrypted */
	ROM_LOAD( "epr-7588.130",	0x2000, 0x2000, 0x21007413 ) /* encrypted */
	ROM_LOAD( "epr-7589.131",	0x4000, 0x2000, 0x44b30433 ) /* encrypted */
	ROM_LOAD( "epr-7590.132",	0x6000, 0x2000, 0xbb525a0b ) /* encrypted */
	ROM_LOAD( "epr-7591.133",	0x8000, 0x2000, 0x8379aa23 )
	ROM_LOAD( "epr-7592.134",	0xa000, 0x2000, 0xc767a5d7 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-7498.120",	0x0000, 0x2000, 0x78ae1e7b ) /* epr-7498.3 */

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7497.62",	0x0000, 0x2000, 0x08d609ca ) /* epr-7497.82 */
	ROM_LOAD( "epr-7496.61",	0x2000, 0x2000, 0x6f61fdf1 ) /* epr-7496.65 */
	ROM_LOAD( "epr-7495.64",	0x4000, 0x2000, 0x6a0d2c2d ) /* epr-7495.81 */
	ROM_LOAD( "epr-7494.63",	0x6000, 0x2000, 0xa8e281c7 ) /* epr-7494.64 */
	ROM_LOAD( "epr-7493.66",	0x8000, 0x2000, 0x89305df4 ) /* epr-7493.80 */
	ROM_LOAD( "epr-7492.65",	0xa000, 0x2000, 0x60f806b1 ) /* epr-7492.63 */

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-7485.117",	0x0000, 0x4000, 0xc2891722 ) /* epr-7485.3 */
	ROM_LOAD( "epr-7487.04",	0x4000, 0x4000, 0x2d3a421b ) /* epr-7487.1 */
	ROM_LOAD( "epr-7486.110",	0x8000, 0x4000, 0x8d622c50 ) /* epr-7486.4 */
	ROM_LOAD( "epr-7488.05",	0xc000, 0x4000, 0x007c2f1b ) /* epr-7488.2 */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
															 /* pr-5317.106 */
ROM_END

ROM_START( wboy2u )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "ic129_02.bin",	0x0000, 0x2000, 0x32c4b709 )
	ROM_LOAD( "ic130_03.bin",	0x2000, 0x2000, 0x56463ede )
	ROM_LOAD( "ic131_04.bin",	0x4000, 0x2000, 0x775ed392 )
	ROM_LOAD( "ic132_05.bin",	0x6000, 0x2000, 0x7b922708 )
	ROM_LOAD( "epr-7591.133",	0x8000, 0x2000, 0x8379aa23 )
	ROM_LOAD( "epr-7592.134",	0xa000, 0x2000, 0xc767a5d7 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr7498a.3",		0x0000, 0x2000, 0xc198205c )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7497.62",	0x0000, 0x2000, 0x08d609ca ) /* epr-7497.82 */
	ROM_LOAD( "epr-7496.61",	0x2000, 0x2000, 0x6f61fdf1 ) /* epr-7496.65 */
	ROM_LOAD( "epr-7495.64",	0x4000, 0x2000, 0x6a0d2c2d ) /* epr-7495.81 */
	ROM_LOAD( "epr-7494.63",	0x6000, 0x2000, 0xa8e281c7 ) /* epr-7494.64 */
	ROM_LOAD( "epr-7493.66",	0x8000, 0x2000, 0x89305df4 ) /* epr-7493.80 */
	ROM_LOAD( "epr-7492.65",	0xa000, 0x2000, 0x60f806b1 ) /* epr-7492.63 */

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-7485.117",	0x0000, 0x4000, 0xc2891722 ) /* epr-7485.3 */
	ROM_LOAD( "epr-7487.04",	0x4000, 0x4000, 0x2d3a421b ) /* epr-7487.1 */
	ROM_LOAD( "epr-7486.110",	0x8000, 0x4000, 0x8d622c50 ) /* epr-7486.4 */
	ROM_LOAD( "epr-7488.05",	0xc000, 0x4000, 0x007c2f1b ) /* epr-7488.2 */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
															 /* pr-5317.106 */
ROM_END

ROM_START( wboy3 )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "wb_1",			0x0000, 0x4000, 0xbd6fef49 ) /* encrypted */
	ROM_LOAD( "wb_2",			0x4000, 0x4000, 0x4081b624 ) /* encrypted */
	ROM_LOAD( "wb_3",			0x8000, 0x4000, 0xc48a0e36 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-7498.120",	0x0000, 0x2000, 0x78ae1e7b )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7497.62",	0x0000, 0x2000, 0x08d609ca )
	ROM_LOAD( "epr-7496.61",	0x2000, 0x2000, 0x6f61fdf1 )
	ROM_LOAD( "epr-7495.64",	0x4000, 0x2000, 0x6a0d2c2d )
	ROM_LOAD( "epr-7494.63",	0x6000, 0x2000, 0xa8e281c7 )
	ROM_LOAD( "epr-7493.66",	0x8000, 0x2000, 0x89305df4 )
	ROM_LOAD( "epr-7492.65",	0xa000, 0x2000, 0x60f806b1 )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-7485.117",	0x0000, 0x4000, 0xc2891722 )
	ROM_LOAD( "epr-7487.04",	0x4000, 0x4000, 0x2d3a421b )
	ROM_LOAD( "epr-7486.110",	0x8000, 0x4000, 0x8d622c50 )
	ROM_LOAD( "epr-7488.05",	0xc000, 0x4000, 0x007c2f1b )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( wboyu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "ic116_89.bin",	0x0000, 0x4000, 0x73d8cef0 )
	ROM_LOAD( "ic109_90.bin",	0x4000, 0x4000, 0x29546828 )
	ROM_LOAD( "ic096_91.bin",	0x8000, 0x4000, 0xc7145c2a )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr-7498.120",	0x0000, 0x2000, 0x78ae1e7b )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7497.62",	0x0000, 0x2000, 0x08d609ca ) /* epr-7497.82 */
	ROM_LOAD( "epr-7496.61",	0x2000, 0x2000, 0x6f61fdf1 ) /* epr-7496.65 */
	ROM_LOAD( "epr-7495.64",	0x4000, 0x2000, 0x6a0d2c2d ) /* epr-7495.81 */
	ROM_LOAD( "epr-7494.63",	0x6000, 0x2000, 0xa8e281c7 ) /* epr-7494.64 */
	ROM_LOAD( "epr-7493.66",	0x8000, 0x2000, 0x89305df4 ) /* epr-7493.80 */
	ROM_LOAD( "epr-7492.65",	0xa000, 0x2000, 0x60f806b1 ) /* epr-7492.63 */

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "ic117_85.bin",	0x0000, 0x4000, 0x1ee96ae8 )
	ROM_LOAD( "ic004_87.bin",	0x4000, 0x4000, 0x119735bb )
	ROM_LOAD( "ic110_86.bin",	0x8000, 0x4000, 0x26d0fac4 )
	ROM_LOAD( "ic005_88.bin",	0xc000, 0x4000, 0x2602e519 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( wbdeluxe )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "wbd1.bin",		0x0000, 0x2000, 0xa1bedbd7 )
	ROM_LOAD( "ic130_03.bin",	0x2000, 0x2000, 0x56463ede )
	ROM_LOAD( "wbd3.bin",		0x4000, 0x2000, 0x6fcdbd4c )
	ROM_LOAD( "ic132_05.bin",	0x6000, 0x2000, 0x7b922708 )
	ROM_LOAD( "wbd5.bin",		0x8000, 0x2000, 0xf6b02902 )
	ROM_LOAD( "wbd6.bin",		0xa000, 0x2000, 0x43df21fe )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr7498a.3",		0x0000, 0x2000, 0xc198205c )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-7497.62",	0x0000, 0x2000, 0x08d609ca ) /* epr-7497.82 */
	ROM_LOAD( "epr-7496.61",	0x2000, 0x2000, 0x6f61fdf1 ) /* epr-7496.65 */
	ROM_LOAD( "epr-7495.64",	0x4000, 0x2000, 0x6a0d2c2d ) /* epr-7495.81 */
	ROM_LOAD( "epr-7494.63",	0x6000, 0x2000, 0xa8e281c7 ) /* epr-7494.64 */
	ROM_LOAD( "epr-7493.66",	0x8000, 0x2000, 0x89305df4 ) /* epr-7493.80 */
	ROM_LOAD( "epr-7492.65",	0xa000, 0x2000, 0x60f806b1 ) /* epr-7492.63 */

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "epr-7485.117",	0x0000, 0x4000, 0xc2891722 ) /* epr-7485.3 */
	ROM_LOAD( "epr-7487.04",	0x4000, 0x4000, 0x2d3a421b ) /* epr-7487.1 */
	ROM_LOAD( "epr-7486.110",	0x8000, 0x4000, 0x8d622c50 ) /* epr-7486.4 */
	ROM_LOAD( "epr-7488.05",	0xc000, 0x4000, 0x007c2f1b ) /* epr-7488.2 */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr-5317.76",		0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
															 /* pr-5317.106 */
ROM_END

ROM_START( gardia )
	ROM_REGION( 2*0x20000, REGION_CPU1, 0 ) /* 128k for code + 128k for decrypted opcodes */
	ROM_LOAD( "epr10255.1",   0x00000, 0x8000, 0x89282a6b ) /* encrypted */
	ROM_LOAD( "epr10254.2",   0x10000, 0x8000, 0x2826b6d8 )
	ROM_LOAD( "epr10253.3",   0x18000, 0x8000, 0x7911260f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr10243.120", 0x0000, 0x4000, 0x87220660 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr10249.61",  0x0000, 0x4000, 0x4e0ad0f2 )
	ROM_LOAD( "epr10248.64",  0x4000, 0x4000, 0x3515d124 )
	ROM_LOAD( "epr10247.66",  0x8000, 0x4000, 0x541e1555 )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr10234.117", 0x00000, 0x8000, 0x8a6aed33 )
	ROM_LOAD( "epr10233.110", 0x08000, 0x8000, 0xc52784d3 )
	ROM_LOAD( "epr10236.04",  0x10000, 0x8000, 0xb35ab227 )
	ROM_LOAD( "epr10235.5",   0x18000, 0x8000, 0x006a3151 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "bprom.3",      0x0000, 0x0100, 0x8eee0f72 ) /* palette red component */
	ROM_LOAD( "bprom.2",      0x0100, 0x0100, 0x3e7babd7 ) /* palette green component */
	ROM_LOAD( "bprom.1",      0x0200, 0x0100, 0x371c44a6 ) /* palette blue component */
	ROM_LOAD( "pr5317.4",     0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( gardiab )
	ROM_REGION( 2*0x20000, REGION_CPU1, 0 ) /* 128k for code + 128k for decrypted opcodes */
	ROM_LOAD( "gardiabl.5",   0x00000, 0x8000, 0x207f9cbb ) /* encrypted */
	ROM_LOAD( "gardiabl.6",   0x10000, 0x8000, 0xb2ed05dc )
	ROM_LOAD( "gardiabl.7",   0x18000, 0x8000, 0x0a490588 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr10243.120", 0x0000, 0x4000, 0x87220660 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gardiabl.8",   0x0000, 0x4000, 0x367c9a17 )
	ROM_LOAD( "gardiabl.9",   0x4000, 0x4000, 0x1540fd30 )
	ROM_LOAD( "gardiabl.10",  0x8000, 0x4000, 0xe5c9af10 )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr10234.117", 0x00000, 0x8000, 0x8a6aed33 )
	ROM_LOAD( "epr10233.110", 0x08000, 0x8000, 0xc52784d3 )
	ROM_LOAD( "epr10236.04",  0x10000, 0x8000, 0xb35ab227 )
	ROM_LOAD( "epr10235.5",   0x18000, 0x8000, 0x006a3151 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "bprom.3",      0x0000, 0x0100, 0x8eee0f72 ) /* palette red component */
	ROM_LOAD( "bprom.2",      0x0100, 0x0100, 0x3e7babd7 ) /* palette green component */
	ROM_LOAD( "bprom.1",      0x0200, 0x0100, 0x371c44a6 ) /* palette blue component */
	ROM_LOAD( "pr5317.4",     0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( blockgal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "bg.116",       0x0000, 0x4000, 0xa99b231a ) /* encrypted */
	ROM_LOAD( "bg.109",       0x4000, 0x4000, 0xa6b573d5 ) /* encrypted */
	/* 0x8000-0xbfff empty (was same as My Hero) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "bg.120",       0x0000, 0x2000, 0xd848faff )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bg.62",        0x0000, 0x2000, 0x7e3ea4eb )
	ROM_LOAD( "bg.61",        0x2000, 0x2000, 0x4dd3d39d )
	ROM_LOAD( "bg.64",        0x4000, 0x2000, 0x17368663 )
	ROM_LOAD( "bg.63",        0x6000, 0x2000, 0x0c8bc404 )
	ROM_LOAD( "bg.66",        0x8000, 0x2000, 0x2b7dc4fa )
	ROM_LOAD( "bg.65",        0xa000, 0x2000, 0xed121306 )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "bg.117",       0x0000, 0x4000, 0xe99cc920 )
	ROM_LOAD( "bg.04",        0x4000, 0x4000, 0x213057f8 )
	ROM_LOAD( "bg.110",       0x8000, 0x4000, 0x064c812c )
	ROM_LOAD( "bg.05",        0xc000, 0x4000, 0x02e0b040 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr5317.76",    0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( blckgalb )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "ic62",         0x10000, 0x8000, 0x65c47676 ) /* decrypted opcodes */
	ROM_CONTINUE(             0x00000, 0x8000 )             /* decrypted data */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "bg.120",       0x0000, 0x2000, 0xd848faff )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bg.62",        0x0000, 0x2000, 0x7e3ea4eb )
	ROM_LOAD( "bg.61",        0x2000, 0x2000, 0x4dd3d39d )
	ROM_LOAD( "bg.64",        0x4000, 0x2000, 0x17368663 )
	ROM_LOAD( "bg.63",        0x6000, 0x2000, 0x0c8bc404 )
	ROM_LOAD( "bg.66",        0x8000, 0x2000, 0x2b7dc4fa )
	ROM_LOAD( "bg.65",        0xa000, 0x2000, 0xed121306 )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* 64k for sprites data */
	ROM_LOAD( "bg.117",       0x0000, 0x4000, 0xe99cc920 )
	ROM_LOAD( "bg.04",        0x4000, 0x4000, 0x213057f8 )
	ROM_LOAD( "bg.110",       0x8000, 0x4000, 0x064c812c )
	ROM_LOAD( "bg.05",        0xc000, 0x4000, 0x02e0b040 )

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "pr5317.76",    0x0000, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( tokisens )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 128k for code */
	ROM_LOAD( "epr10961.90",  0x00000, 0x8000, 0x1466b61d )
	ROM_LOAD( "epr10962.91",  0x10000, 0x8000, 0xa8479f91 )
	ROM_LOAD( "epr10963.92",  0x18000, 0x8000, 0xb7193b39 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr10967.126", 0x0000, 0x8000, 0x97966bf2 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr10964.4",   0x00000, 0x8000, 0x9013b85c )
	ROM_LOAD( "epr10965.5",   0x08000, 0x8000, 0xe4755cc6 )
	ROM_LOAD( "epr10966.6",   0x10000, 0x8000, 0x5bbfbdcc )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr10958.87",  0x00000, 0x8000, 0xfc2bcbd7 )
	ROM_LOAD( "epr10957.86",  0x08000, 0x8000, 0x4ec56860 )
	ROM_LOAD( "epr10960.89",  0x10000, 0x8000, 0x880e0d44 )
	ROM_LOAD( "epr10959.88",  0x18000, 0x8000, 0x4deda48f )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "bprom.20",      0x0000, 0x0100, 0x8eee0f72 ) /* palette red component */
	ROM_LOAD( "bprom.14",      0x0100, 0x0100, 0x3e7babd7 ) /* palette green component */
	ROM_LOAD( "bprom.8",       0x0200, 0x0100, 0x371c44a6 ) /* palette blue component */
	ROM_LOAD( "bprom.28",      0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( wbml )
	ROM_REGION( 2*0x20000, REGION_CPU1, 0 ) /* 256k for code + 256k for decrypted opcodes */
	ROM_LOAD( "ep11031a.90",  0x00000, 0x8000, 0xbd3349e5 ) /* encrypted */
	ROM_LOAD( "epr11032.91",  0x10000, 0x8000, 0x9d03bdb2 ) /* encrypted */
	ROM_LOAD( "epr11033.92",  0x18000, 0x8000, 0x7076905c ) /* encrypted */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr11037.126", 0x0000, 0x8000, 0x7a4ee585 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr11034.4",   0x00000, 0x8000, 0x37a2077d )
	ROM_LOAD( "epr11035.5",   0x08000, 0x8000, 0xcdf2a21b )
	ROM_LOAD( "epr11036.6",   0x10000, 0x8000, 0x644687fa )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr11028.87",  0x00000, 0x8000, 0xaf0b3972 )
	ROM_LOAD( "epr11027.86",  0x08000, 0x8000, 0x277d8f1d )
	ROM_LOAD( "epr11030.89",  0x10000, 0x8000, 0xf05ffc76 )
	ROM_LOAD( "epr11029.88",  0x18000, 0x8000, 0xcedc9c61 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "pr11026.20",   0x0000, 0x0100, 0x27057298 )
	ROM_LOAD( "pr11025.14",   0x0100, 0x0100, 0x41e4d86b )
	ROM_LOAD( "pr11024.8",    0x0200, 0x0100, 0x08d71954 )
	ROM_LOAD( "pr5317.37",    0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( wbmljo )
	ROM_REGION( 2*0x20000, REGION_CPU1, 0 ) /* 256k for code + 256k for decrypted opcodes */
	ROM_LOAD( "epr11031.90",  0x00000, 0x8000, 0x497ebfb4 ) /* encrypted */
	ROM_LOAD( "epr11032.91",  0x10000, 0x8000, 0x9d03bdb2 ) /* encrypted */
	ROM_LOAD( "epr11033.92",  0x18000, 0x8000, 0x7076905c ) /* encrypted */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr11037.126", 0x0000, 0x8000, 0x7a4ee585 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr11034.4",   0x00000, 0x8000, 0x37a2077d )
	ROM_LOAD( "epr11035.5",   0x08000, 0x8000, 0xcdf2a21b )
	ROM_LOAD( "epr11036.6",   0x10000, 0x8000, 0x644687fa )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr11028.87",  0x00000, 0x8000, 0xaf0b3972 )
	ROM_LOAD( "epr11027.86",  0x08000, 0x8000, 0x277d8f1d )
	ROM_LOAD( "epr11030.89",  0x10000, 0x8000, 0xf05ffc76 )
	ROM_LOAD( "epr11029.88",  0x18000, 0x8000, 0xcedc9c61 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "pr11026.20",   0x0000, 0x0100, 0x27057298 )
	ROM_LOAD( "pr11025.14",   0x0100, 0x0100, 0x41e4d86b )
	ROM_LOAD( "pr11024.8",    0x0200, 0x0100, 0x08d71954 )
	ROM_LOAD( "pr5317.37",    0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( wbmljb )
	ROM_REGION( 2*0x20000, REGION_CPU1, 0 ) /* 256k for code + 256k for decrypted opcodes */
	ROM_LOAD( "wbml.01",      0x20000, 0x8000, 0x66482638 ) /* Unencrypted opcodes */
	ROM_CONTINUE(             0x00000, 0x8000 )             /* Now load the operands in RAM */
	ROM_LOAD( "m-6.bin",      0x30000, 0x8000, 0x8c08cd11 ) /* Unencrypted opcodes */
	ROM_CONTINUE(             0x10000, 0x8000 )
	ROM_LOAD( "m-7.bin",      0x38000, 0x8000, 0x11881703 ) /* Unencrypted opcodes */
	ROM_CONTINUE(             0x18000, 0x8000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr11037.126", 0x0000, 0x8000, 0x7a4ee585 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr11034.4",   0x00000, 0x8000, 0x37a2077d )
	ROM_LOAD( "epr11035.5",   0x08000, 0x8000, 0xcdf2a21b )
	ROM_LOAD( "epr11036.6",   0x10000, 0x8000, 0x644687fa )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr11028.87",  0x00000, 0x8000, 0xaf0b3972 )
	ROM_LOAD( "epr11027.86",  0x08000, 0x8000, 0x277d8f1d )
	ROM_LOAD( "epr11030.89",  0x10000, 0x8000, 0xf05ffc76 )
	ROM_LOAD( "epr11029.88",  0x18000, 0x8000, 0xcedc9c61 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "pr11026.20",   0x0000, 0x0100, 0x27057298 )
	ROM_LOAD( "pr11025.14",   0x0100, 0x0100, 0x41e4d86b )
	ROM_LOAD( "pr11024.8",    0x0200, 0x0100, 0x08d71954 )
	ROM_LOAD( "pr5317.37",    0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( wbmlb )
	ROM_REGION( 2*0x20000, REGION_CPU1, 0 ) /* 256k for code + 256k for decrypted opcodes */
	ROM_LOAD( "wbml.01",      0x20000, 0x8000, 0x66482638 ) /* Unencrypted opcodes */
	ROM_CONTINUE(             0x00000, 0x8000 )             /* Now load the operands in RAM */
	ROM_LOAD( "wbml.02",      0x30000, 0x8000, 0x48746bb6 ) /* Unencrypted opcodes */
	ROM_CONTINUE(             0x10000, 0x8000 )
	ROM_LOAD( "wbml.03",      0x38000, 0x8000, 0xd57ba8aa ) /* Unencrypted opcodes */
	ROM_CONTINUE(             0x18000, 0x8000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr11037.126", 0x0000, 0x8000, 0x7a4ee585 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "wbml.08",      0x00000, 0x8000, 0xbbea6afe )
	ROM_LOAD( "wbml.09",      0x08000, 0x8000, 0x77567d41 )
	ROM_LOAD( "wbml.10",      0x10000, 0x8000, 0xa52ffbdd )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr11028.87",  0x00000, 0x8000, 0xaf0b3972 )
	ROM_LOAD( "epr11027.86",  0x08000, 0x8000, 0x277d8f1d )
	ROM_LOAD( "epr11030.89",  0x10000, 0x8000, 0xf05ffc76 )
	ROM_LOAD( "epr11029.88",  0x18000, 0x8000, 0xcedc9c61 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "pr11026.20",   0x0000, 0x0100, 0x27057298 )
	ROM_LOAD( "pr11025.14",   0x0100, 0x0100, 0x41e4d86b )
	ROM_LOAD( "pr11024.8",    0x0200, 0x0100, 0x08d71954 )
	ROM_LOAD( "pr5317.37",    0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( dakkochn )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 128k for code */
	ROM_LOAD( "epr11224.90",  0x00000, 0x8000, 0x9fb1972b ) /* encrypted */
	ROM_LOAD( "epr11225.91",  0x10000, 0x8000, 0xc540f9e2 ) /* encrypted */
	/* 18000-1ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr11229.126", 0x0000, 0x8000, 0xc11648d0 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr11226.4",   0x00000, 0x8000, 0x3dbc2f78 )
	ROM_LOAD( "epr11227.5",   0x08000, 0x8000, 0x34156e8d )
	ROM_LOAD( "epr11228.6",   0x10000, 0x8000, 0xfdd5323f )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr11221.87",  0x00000, 0x8000, 0xf9a44916 )
	ROM_LOAD( "epr11220.86",  0x08000, 0x8000, 0xfdd25d8a )
	ROM_LOAD( "epr11223.89",  0x10000, 0x8000, 0x538adc55 )
	ROM_LOAD( "epr11222.88",  0x18000, 0x8000, 0x33fab0b2 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "pr11219.20",   0x0000, 0x0100, 0x45e252d9 ) /* palette red component */
	ROM_LOAD( "pr11218.14",   0x0100, 0x0100, 0x3eda3a1b ) /* palette green component */
	ROM_LOAD( "pr11217.8",    0x0200, 0x0100, 0x49dbde88 ) /* palette blue component */
	ROM_LOAD( "pr5317.37",    0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( ufosensi )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 128k for code */
	ROM_LOAD( "epr11661.90",  0x00000, 0x8000, 0xf3e394e2 ) /* encrypted */
	ROM_LOAD( "epr11662.91",  0x10000, 0x8000, 0x0c2e4120 ) /* encrypted */
	ROM_LOAD( "epr11663.92",  0x18000, 0x8000, 0x4515ebae ) /* encrypted */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu */
	ROM_LOAD( "epr11667.126", 0x0000, 0x8000, 0x110baba9 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr11664.4",   0x00000, 0x8000, 0x1b1bc3d5 )
	ROM_LOAD( "epr11665.5",   0x08000, 0x8000, 0x3659174a )
	ROM_LOAD( "epr11666.6",   0x10000, 0x8000, 0x99dcc793 )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "epr11658.87",  0x00000, 0x8000, 0x3b5a20f7 )
	ROM_LOAD( "epr11657.86",  0x08000, 0x8000, 0x010f81a9 )
	ROM_LOAD( "epr11660.89",  0x10000, 0x8000, 0xe1e2e7c5 )
	ROM_LOAD( "epr11659.88",  0x18000, 0x8000, 0x286c7286 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "pr11656.20",   0x0000, 0x0100, 0x640740eb ) /* palette red component */
	ROM_LOAD( "pr11655.14",   0x0100, 0x0100, 0xa0c3fa77 ) /* palette green component */
	ROM_LOAD( "pr11654.8",    0x0200, 0x0100, 0xba624305 ) /* palette blue component */
	ROM_LOAD( "pr5317.28",    0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

ROM_START( noboranb )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "nobo-t.bin", 0x00000, 0x8000, 0x176fd168 )
	ROM_LOAD( "nobo-r.bin", 0x10000, 0x8000, 0xd61cf3c9 )
	ROM_LOAD( "nobo-s.bin", 0x18000, 0x8000, 0xb0e7697f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "nobo-m.bin", 0x0000, 0x4000, 0x415adf76 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "nobo-j.bin", 0x08000, 0x8000, 0xf12df039 )
	ROM_LOAD( "nobo-k.bin", 0x00000, 0x8000, 0x446fbcdd )
	ROM_LOAD( "nobo-l.bin", 0x10000, 0x8000, 0x35f396df )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* 128k for sprites data */
	ROM_LOAD( "nobo-q.bin", 0x00000, 0x8000, 0x2442b86d )
	ROM_LOAD( "nobo-o.bin", 0x08000, 0x8000, 0xe33743a6 )
	ROM_LOAD( "nobo-p.bin", 0x10000, 0x8000, 0x7fbba01d )
	ROM_LOAD( "nobo-n.bin", 0x18000, 0x8000, 0x85e7a29f )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "nobo_pr.16d", 0x0000, 0x0100, 0x95010ac2 ) /* palette red component */
	ROM_LOAD( "nobo_pr.15d", 0x0100, 0x0100, 0xc55aac0c ) /* palette green component */
	ROM_LOAD( "nobo_pr.14d", 0x0200, 0x0100, 0xde394cee ) /* palette blue component */
	ROM_LOAD( "nobo_pr.13a", 0x0300, 0x0100, 0x648350b8 ) /* timing? (not used) */
ROM_END

static DRIVER_INIT( regulus )	{ regulus_decode(); }
static DRIVER_INIT( mrviking )	{ mrviking_decode(); }
static DRIVER_INIT( swat )		{ swat_decode(); }
static DRIVER_INIT( flicky )	{ flicky_decode(); }
static DRIVER_INIT( wmatch )	{ wmatch_decode(); }
static DRIVER_INIT( bullfgtj )	{ bullfgtj_decode(); }
static DRIVER_INIT( spatter )	{ spatter_decode(); }
static DRIVER_INIT( pitfall2 )	{ pitfall2_decode(); }
static DRIVER_INIT( nprinces )	{ nprinces_decode(); }
static DRIVER_INIT( seganinj )	{ seganinj_decode(); }
static DRIVER_INIT( imsorry )	{ imsorry_decode(); }
static DRIVER_INIT( teddybb )	{ teddybb_decode(); }
static DRIVER_INIT( hvymetal )	{ hvymetal_decode(); }
static DRIVER_INIT( myheroj )	{ myheroj_decode(); }
static DRIVER_INIT( 4dwarrio )	{ fdwarrio_decode(); }
static DRIVER_INIT( wboy )		{ astrofl_decode(); }
static DRIVER_INIT( wboy2 )		{ wboy2_decode(); }
static DRIVER_INIT( gardia )	{ gardia_decode(); }
static DRIVER_INIT( gardiab )	{ gardiab_decode(); }


DRIVER_INIT( myherok )
{
	int A;
	unsigned char *rom;

	/* additionally to the usual protection, all the program ROMs have data lines */
	/* D0 and D1 swapped. */
	rom = memory_region(REGION_CPU1);
	for (A = 0;A < 0xc000;A++)
		rom[A] = (rom[A] & 0xfc) | ((rom[A] & 1) << 1) | ((rom[A] & 2) >> 1);

	/* the tile gfx ROMs are mangled as well: */
	rom = memory_region(REGION_GFX1);

	/* the first ROM has data lines D0 and D6 swapped. */
	for (A = 0x0000;A < 0x4000;A++)
		rom[A] = (rom[A] & 0xbe) | ((rom[A] & 0x01) << 6) | ((rom[A] & 0x40) >> 6);

	/* the second ROM has data lines D1 and D5 swapped. */
	for (A = 0x4000;A < 0x8000;A++)
		rom[A] = (rom[A] & 0xdd) | ((rom[A] & 0x02) << 4) | ((rom[A] & 0x20) >> 4);

	/* the third ROM has data lines D0 and D6 swapped. */
	for (A = 0x8000;A < 0xc000;A++)
		rom[A] = (rom[A] & 0xbe) | ((rom[A] & 0x01) << 6) | ((rom[A] & 0x40) >> 6);

	/* also, all three ROMs have address lines A4 and A5 swapped. */
	for (A = 0;A < 0xc000;A++)
	{
		int A1;
		unsigned char temp;

		A1 = (A & 0xffcf) | ((A & 0x0010) << 1) | ((A & 0x0020) >> 1);
		if (A < A1)
		{
			temp = rom[A];
			rom[A] = rom[A1];
			rom[A1] = temp;
		}
	}

	myheroj_decode();
}

static DRIVER_INIT( bootleg )
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;

	memory_set_opcode_base(0,rom+diff);
}

static DRIVER_INIT( noboranb )
{
	/* Patch to get PRG ROMS ('T', 'R' and 'S) status as "GOOD" in the "test mode" */
	/* not really needed */

	data8_t *ROM = memory_region(REGION_CPU1);

//	ROM[0x3296] = 0x18;		// 'jr' instead of 'jr z' - 'T' (PRG Main ROM)
//	ROM[0x32be] = 0x18;		// 'jr' instead of 'jr z' - 'R' (Banked ROM 1)
//	ROM[0x32ea] = 0x18;		// 'jr' instead of 'jr z' - 'S' (Banked ROM 2)

	/* Patch to avoid the internal checksum that will hang the game after an amount of time
	   (check code at 0x3313 in 'R' (banked ROM 1)) */

	ROM[0x10000 + 0 * 0x8000 + 0x3347] = 0x18;	// 'jr' instead of 'jr z'
}





GAME( 1983, starjack, 0,        small,    starjack, 0,        ROT270, "Sega", "Star Jacker (Sega)" )
GAME( 1983, starjacs, starjack, small,    starjacs, 0,        ROT270, "Stern", "Star Jacker (Stern)" )
GAME( 1983, regulus,  0,        system1,  regulus,  regulus,  ROT270, "Sega", "Regulus (New Ver.)" )
GAME( 1983, reguluso, regulus,  system1,  reguluso, regulus,  ROT270, "Sega", "Regulus (Old Ver.)" )
GAME( 1983, regulusu, regulus,  system1,  regulus,  0,        ROT270, "Sega", "Regulus (not encrypted)" )
GAME( 1983, upndown,  0,        system1,  upndown,  nprinces, ROT270, "Sega", "Up'n Down" )
GAME( 1983, upndownu, upndown,  system1,  upndown,  0,        ROT270, "Sega", "Up'n Down (not encrypted)" )
GAME( 1984, mrviking, 0,        small,    mrviking, mrviking, ROT270, "Sega", "Mister Viking" )
GAME( 1984, mrvikngj, mrviking, small,    mrvikngj, mrviking, ROT270, "Sega", "Mister Viking (Japan)" )
GAME( 1984, swat,     0,        system1,  swat,     swat,     ROT270, "Coreland / Sega", "SWAT" )
GAME( 1984, flicky,   0,        system1,  flicky,   flicky,   ROT0,   "Sega", "Flicky (128k Ver.)" )
GAME( 1984, flickyo,  flicky,   system1,  flicky,   flicky,   ROT0,   "Sega", "Flicky (64k Ver.)" )
GAME( 1984, wmatch,   0,        system1,  wmatch,   wmatch,   ROT270, "Sega", "Water Match" )
GAME( 1984, bullfgt,  0,        system1,  bullfgt,  bullfgtj, ROT0,   "Coreland / Sega", "Bullfight" )
GAME( 1984, thetogyu, bullfgt,  system1,  bullfgt,  bullfgtj, ROT0,   "Coreland / Sega", "The Togyu (Japan)" )
GAME( 1984, spatter,  0,        small,    spatter,  spatter,  ROT0,   "Sega", "Spatter" )
GAME( 1984, ssanchan, spatter,  small,    spatter,  spatter,  ROT0,   "Sega", "Sanrin San Chan (Japan)" )
GAME( 1985, pitfall2, 0,        system1,  pitfall2, pitfall2, ROT0,   "Sega", "Pitfall II - The Lost Caverns" )
GAME( 1985, pitfallu, pitfall2, system1,  pitfallu, 0,        ROT0,   "Sega", "Pitfall II - The Lost Caverns (not encrypted)" )
GAME( 1985, seganinj, 0,        system1,  seganinj, seganinj, ROT0,   "Sega", "Sega Ninja" )
GAME( 1985, seganinu, seganinj, system1,  seganinj, 0,        ROT0,   "Sega", "Sega Ninja (not encrypted)" )
GAME( 1985, nprinces, seganinj, system1,  seganinj, flicky,   ROT0,   "bootleg?", "Ninja Princess (64k Ver. bootleg?)" )
GAME( 1985, nprincso, seganinj, system1,  seganinj, nprinces, ROT0,   "Sega", "Ninja Princess (128k Ver.)" )
GAME( 1985, nprincsu, seganinj, system1,  seganinj, 0,        ROT0,   "Sega", "Ninja Princess (64k Ver. not encrypted)" )
GAME( 1985, nprincsb, seganinj, system1,  seganinj, flicky,   ROT0,   "bootleg?", "Ninja Princess (128k Ver. bootleg?)" )
GAME( 1985, imsorry,  0,        system1,  imsorry,  imsorry,  ROT0,   "Coreland / Sega", "I'm Sorry (US)" )
GAME( 1985, imsorryj, imsorry,  system1,  imsorry,  imsorry,  ROT0,   "Coreland / Sega", "I'm Sorry (Japan)" )
GAME( 1985, teddybb,  0,        system1,  teddybb,  teddybb,  ROT0,   "Sega", "TeddyBoy Blues (New Ver.)" )
GAME( 1985, teddybbo, teddybb,  system1,  teddybb,  teddybb,  ROT0,   "Sega", "TeddyBoy Blues (Old Ver.)" )
GAME( 1985, hvymetal, 0,        hvymetal, hvymetal, hvymetal, ROT0,   "Sega", "Heavy Metal" )
GAME( 1985, myhero,   0,        system1,  myhero,   0,        ROT0,   "Sega", "My Hero (US)" )
GAME( 1985, sscandal, myhero,   system1,  myhero,   myheroj,  ROT0,   "Coreland / Sega", "Seishun Scandal (Japan)" )
GAME( 1985, myherok,  myhero,   system1,  myhero,   myherok,  ROT0,   "Coreland / Sega", "My Hero (Korea)" )
GAMEX(1985, shtngmst, 0,        chplft,   chplft,   0,        ROT0,   "Sega", "Shooting Master", GAME_UNEMULATED_PROTECTION )	/* 8751 protection, mcu = 315-5159 */
GAMEX(1985, chplft,   0,        chplft,   chplft,   0,        ROT0,   "Sega", "Choplifter", GAME_UNEMULATED_PROTECTION )	/* 8751 protection */
GAME( 1985, chplftb,  chplft,   chplft,   chplft,   0,        ROT0,   "Sega", "Choplifter (alternate)" )
GAME( 1985, chplftbl, chplft,   chplft,   chplft,   0,        ROT0,   "bootleg", "Choplifter (bootleg)" )
GAME( 1985, 4dwarrio, 0,        system1,  4dwarrio, 4dwarrio, ROT0,   "Coreland / Sega", "4-D Warriors" )
GAME( 1986, brain,    0,        brain,    brain,    0,        ROT0,   "Coreland / Sega", "Brain" )
GAME( 1986, raflesia, 0,        system1,  raflesia, 4dwarrio, ROT270, "Coreland / Sega", "Rafflesia" )
GAME( 1986, wboy,     0,        system1,  wboy,     wboy,     ROT0,   "Sega (Escape license)", "Wonder Boy (set 1, new encryption)" )
GAME( 1986, wboyo,    wboy,     system1,  wboy,     hvymetal, ROT0,   "Sega (Escape license)", "Wonder Boy (set 1, old encryption)" )
GAME( 1986, wboy2,    wboy,     system1,  wboy,     wboy2,    ROT0,   "Sega (Escape license)", "Wonder Boy (set 2)" )
GAME( 1986, wboy2u,   wboy,     system1,  wboy,     0,        ROT0,   "Sega (Escape license)", "Wonder Boy (set 2 not encrypted)" )
GAME( 1986, wboy3,    wboy,     system1,  wboy,     hvymetal, ROT0,   "Sega (Escape license)", "Wonder Boy (set 3)" )
GAME( 1986, wboyu,    wboy,     system1,  wboyu,    0,        ROT0,   "Sega (Escape license)", "Wonder Boy (not encrypted)" )
GAME( 1986, wbdeluxe, wboy,     system1,  wbdeluxe, 0,        ROT0,   "Sega (Escape license)", "Wonder Boy Deluxe" )
GAMEX(1986, gardia,   0,        brain,    wboy,     gardia,   ROT270, "Sega / Coreland", "Gardia", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAMEX(1986, gardiab,  gardia,   brain,    wboy,     gardiab,  ROT270, "bootleg", "Gardia (bootleg)", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAMEX(1986, noboranb, 0,        noboranb, noboranb, noboranb, ROT270, "bootleg", "Noboranka (Japan)", GAME_IMPERFECT_SOUND )
GAMEX(1987, blockgal, 0,        blockgal, blockgal, 0,        ROT90,  "Sega / Vic Tokai", "Block Gal", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAMEX(1987, blckgalb, blockgal, blockgal, blockgal, bootleg,  ROT90,  "bootleg", "Block Gal (bootleg)", GAME_NO_COCKTAIL )
GAMEX(1987, tokisens, 0,        wbml,     tokisens, 0,        ROT90,  "Sega", "Toki no Senshi - Chrono Soldier", GAME_NO_COCKTAIL )
GAMEX(1987, wbml,     0,        wbml,     wbml,     0,        ROT0,   "Sega / Westone", "Wonder Boy in Monster Land (Japan New Ver.)", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAMEX(1987, wbmljo,   wbml,     wbml,     wbml,     0,        ROT0,   "Sega / Westone", "Wonder Boy in Monster Land (Japan Old Ver.)", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAMEX(1987, wbmljb,   wbml,     wbml,     wbml,     bootleg,  ROT0,   "bootleg", "Wonder Boy in Monster Land (Japan not encrypted)", GAME_NO_COCKTAIL )
GAMEX(1987, wbmlb,    wbml,     wbml,     wbml,     bootleg,  ROT0,   "bootleg", "Wonder Boy in Monster Land", GAME_NO_COCKTAIL )
GAMEX(1987, dakkochn, 0,        chplft,   chplft,   0,        ROT0,   "Sega", "DakkoChan Jansoh", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAMEX(1988, ufosensi, 0,        chplft,   chplft,   0,        ROT0,   "Sega", "Ufo Senshi Yohko Chan", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
