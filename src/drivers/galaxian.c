/***************************************************************************

Galaxian memory map

Compiled from information provided by friends and Uncles on RGVAC.

            AAAAAA
            111111AAAAAAAAAA     DDDDDDDD   Schem   function
HEX         5432109876543210 R/W 76543210   name

0000-27FF                                           Game ROM
5000-57FF   01010AAAAAAAAAAA R/W DDDDDDDD   !Vram   Character ram
5800-583F   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Screen attributes
5840-585F   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Sprites
5860-5FFF   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Bullets

6000        0110000000000000 R   -------D   !SW0    coin1
6000        0110000000000000 R   ------D-   !SW0    coin2
6000        0110000000000000 R   -----D--   !SW0    p1 left
6000        0110000000000000 R   ----D---   !SW0    p1 right
6000        0110000000000000 R   ---D----   !SW0    p1shoot
6000        0110000000000000 R   --D-----   !SW0    table ??
6000        0110000000000000 R   -D------   !SW0    test
6000        0110000000000000 R   D-------   !SW0    service

6000        0110000000000001 W   -------D   !DRIVER lamp 1 ??
6001        0110000000000001 W   -------D   !DRIVER lamp 2 ??
6002        0110000000000010 W   -------D   !DRIVER lamp 3 ??
6003        0110000000000011 W   -------D   !DRIVER coin control
6004        0110000000000100 W   -------D   !DRIVER Background lfo freq bit0
6005        0110000000000101 W   -------D   !DRIVER Background lfo freq bit1
6006        0110000000000110 W   -------D   !DRIVER Background lfo freq bit2
6007        0110000000000111 W   -------D   !DRIVER Background lfo freq bit3

6800        0110100000000000 R   -------D   !SW1    1p start
6800        0110100000000000 R   ------D-   !SW1    2p start
6800        0110100000000000 R   -----D--   !SW1    p2 left
6800        0110100000000000 R   ----D---   !SW1    p2 right
6800        0110100000000000 R   ---D----   !SW1    p2 shoot
6800        0110100000000000 R   --D-----   !SW1    no used
6800        0110100000000000 R   -D------   !SW1    dip sw1
6800        0110100000000000 R   D-------   !SW1    dip sw2

6800        0110100000000000 W   -------D   !SOUND  reset background F1
                                                    (1=reset ?)
6801        0110100000000001 W   -------D   !SOUND  reset background F2
6802        0110100000000010 W   -------D   !SOUND  reset background F3
6803        0110100000000011 W   -------D   !SOUND  Noise on/off
6804        0110100000000100 W   -------D   !SOUND  not used
6805        0110100000000101 W   -------D   !SOUND  shoot on/off
6806        0110100000000110 W   -------D   !SOUND  Vol of f1
6807        0110100000000111 W   -------D   !SOUND  Vol of f2

7000        0111000000000000 R   -------D   !DIPSW  dip sw 3
7000        0111000000000000 R   ------D-   !DIPSW  dip sw 4
7000        0111000000000000 R   -----D--   !DIPSW  dip sw 5
7000        0111000000000000 R   ----D---   !DIPSW  dip s2 6

7001        0111000000000001 W   -------D   9Nregen NMIon
7004        0111000000000100 W   -------D   9Nregen stars on
7006        0111000000000110 W   -------D   9Nregen hflip
7007        0111000000000111 W   -------D   9Nregen vflip

Note: 9n reg,other bits  used on moon cresta for extra graphics rom control.

7800        0111100000000000 R   --------   !wdr    watchdog reset
7800        0111100000000000 W   DDDDDDDD   !pitch  Sound Fx base frequency


Notes:
-----

- The only code difference between 'galaxian' and 'galmidw' is that the
  'BONUS SHIP' text is printed on a different line.



Jump Bug memory map (preliminary)

0000-3fff ROM
4000-47ff RAM
4800-4bff Video RAM
4c00-4fff mirror address for video RAM
5000-50ff Object RAM
  5000-503f  screen attributes
  5040-505f  sprites
  5060-507f  bullets?
  5080-50ff  unused?
8000-a7ff ROM

read:
6000      IN0
6800      IN1
7000      IN2

write:
5800      8910 write port
5900      8910 control port
6002-6006 gfx bank select - see vidhrdw/jumpbug.c for details
7001      interrupt enable
7002      coin counter ????
7003      ?
7004      stars on
7005      ?
7006      screen vertical flip
7007      screen horizontal flip
7800      ?


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern int galaxian_bulletsram_size;
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void galaxian_flipx_w(int offset,int data);
void galaxian_flipy_w(int offset,int data);
void galaxian_attributes_w(int offset,int data);
void galaxian_stars_w(int offset,int data);
void scramble_background_w(int offset, int data);
int  galaxian_vh_start(void);
int  mooncrgx_vh_start(void);
int    pisces_vh_start(void);
int  scramble_vh_start(void);
int   jumpbug_vh_start(void);
int    zigzag_vh_start(void);
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  galaxian_vh_interrupt(void);
int  scramble_vh_interrupt(void);
int  devilfsg_vh_interrupt(void);
void jumpbug_gfxbank_w(int offset,int data);
void pisces_gfxbank_w(int offset,int data);

void mooncrst_pitch_w(int offset,int data);
void mooncrst_vol_w(int offset,int data);
void mooncrst_noise_w(int offset,int data);
void mooncrst_background_w(int offset,int data);
void mooncrst_shoot_w(int offset,int data);
void mooncrst_lfo_freq_w(int offset,int data);
int  mooncrst_sh_start(const struct MachineSound *msound);
void mooncrst_sh_stop(void);
void mooncrst_sh_update(void);

int scramblb_protection_1_r(int offset);
int scramblb_protection_2_r(int offset);


static void galaxian_coin_lockout_w(int offset, int data)
{
	coin_lockout_global_w(offset, data ^ 1);
}

static int galapx_funky_r(int offset)
{
	return 0xff;
}


static void galaxian_machine_init(void)
{
	install_mem_write_handler(0, 0x6002, 0x6002, galaxian_coin_lockout_w);
}

static void galapx_machine_init(void)
{
    /* for the title screen */
	install_mem_read_handler(0, 0x7800, 0x78ff, galapx_funky_r);
	galaxian_machine_init();
}


static int jumpbug_protection_r(int offset)
{
	switch (offset)
	{
	case 0x0114:  return 0x4f;
	case 0x0118:  return 0xd3;
	case 0x0214:  return 0xcf;
	case 0x0235:  return 0x02;
	case 0x0311:  return 0x00;  /* not checked */
	default:
		if (errorlog)  fprintf(errorlog, "Unknown protection read. Offset: %04X  PC=%04X\n",0xb000+offset,cpu_get_pc());
	}

	return 0;
}

static int checkmaj_protection_r(int offset)
{
	switch (cpu_get_pc())
	{
	case 0x0f15:  return 0xf5;
	case 0x0f8f:  return 0x7c;
	case 0x10b3:  return 0x7c;
	case 0x10e0:  return 0x00;
	case 0x10f1:  return 0xaa;
	case 0x1402:  return 0xaa;
	default:
		if (errorlog)  fprintf(errorlog, "Unknown protection read. PC=%04X\n",cpu_get_pc());
	}

	return 0;
}

/* Send sound data to the sound cpu and cause an nmi */
static void checkmaj_sound_command_w (int offset, int data)
{
	soundlatch_w (0,data);
	cpu_cause_interrupt (1, Z80_NMI_INT);
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },	/* not all games use all the space */
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x5000, 0x5fff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* DSW */
	{ 0x7800, 0x7800, watchdog_reset_r },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress scramblb_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },	/* RAM and video RAM */
	{ 0x5000, 0x507f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* IN2 */
	{ 0x7800, 0x7800, watchdog_reset_r },
	{ 0x8102, 0x8102, scramblb_protection_1_r },
	{ 0x8202, 0x8202, scramblb_protection_2_r },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress jumpbug_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },	/* RAM, Video RAM */
	{ 0x4c00, 0x4fff, videoram_r },	/* mirror address for Video RAM*/
	{ 0x5000, 0x507f, MRA_RAM },	/* screen attributes, sprites */
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* DSW0 */
	{ 0x8000, 0xafff, MRA_ROM },
	{ 0xb000, 0xbfff, jumpbug_protection_r },
	{ 0xfff0, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },	/* not all games use all the space */
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x5000, 0x53ff, videoram_w, &videoram, &videoram_size },
	{ 0x5800, 0x583f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5840, 0x585f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5860, 0x587f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x6000, 0x6001, osd_led_w },
	{ 0x6004, 0x6007, mooncrst_lfo_freq_w },
	{ 0x6800, 0x6802, mooncrst_background_w },
	{ 0x6803, 0x6803, mooncrst_noise_w },
	{ 0x6805, 0x6805, mooncrst_shoot_w },
	{ 0x6806, 0x6807, mooncrst_vol_w },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7004, 0x7004, galaxian_stars_w },
	{ 0x7006, 0x7006, galaxian_flipx_w },
	{ 0x7007, 0x7007, galaxian_flipy_w },
	{ 0x7800, 0x7800, mooncrst_pitch_w },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress scramblb_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x5000, 0x503f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x6000, 0x6001, MWA_NOP },  /* sound triggers */
	{ 0x6004, 0x6007, mooncrst_lfo_freq_w },
	{ 0x6800, 0x6802, mooncrst_background_w },
	{ 0x6803, 0x6803, mooncrst_noise_w },
	{ 0x6805, 0x6805, mooncrst_shoot_w },
	{ 0x6806, 0x6807, mooncrst_vol_w },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7002, 0x7002, coin_counter_w },
	{ 0x7003, 0x7003, scramble_background_w },
	{ 0x7004, 0x7004, galaxian_stars_w },
	{ 0x7006, 0x7006, galaxian_flipx_w },
	{ 0x7007, 0x7007, galaxian_flipy_w },
	{ 0x7800, 0x7800, mooncrst_pitch_w },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress jumpbug_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x4c00, 0x4fff, videoram_w },	/* mirror address for Video RAM */
	{ 0x5000, 0x503f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5800, 0x5800, AY8910_write_port_0_w },
	{ 0x5900, 0x5900, AY8910_control_port_0_w },
	{ 0x6002, 0x6006, jumpbug_gfxbank_w },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7002, 0x7002, coin_counter_w },
	{ 0x7004, 0x7004, galaxian_stars_w },
	{ 0x7006, 0x7006, galaxian_flipx_w },
	{ 0x7007, 0x7007, galaxian_flipy_w },
	{ 0x8000, 0xafff, MWA_ROM },
	{ 0xfff0, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress checkmaj_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },	/* not all games use all the space */
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x5000, 0x53ff, videoram_w, &videoram, &videoram_size },
	{ 0x5800, 0x583f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5840, 0x585f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5860, 0x587f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7006, 0x7006, galaxian_flipx_w },
	{ 0x7007, 0x7007, galaxian_flipy_w },
	{ 0x7800, 0x7800, checkmaj_sound_command_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress checkmaj_sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x8000, 0x81ff, MRA_RAM },
	{ 0xa002, 0xa002, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress checkmaj_sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x8000, 0x81ff, MWA_RAM },
	{ 0xa000, 0xa000, AY8910_control_port_0_w },
	{ 0xa001, 0xa001, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};

/* Zig Zag can swap ROMs 2 and 3 as a form of copy protection */
static void zigzag_sillyprotection_w(int offset,int data)
{
	unsigned char *RAM = memory_region(REGION_CPU1);


	if (data)
	{
		/* swap ROM 2 and 3! */
		cpu_setbank(1,&RAM[0x3000]);
		cpu_setbank(2,&RAM[0x2000]);
	}
	else
	{
		cpu_setbank(1,&RAM[0x2000]);
		cpu_setbank(2,&RAM[0x3000]);
	}
}

/* but the way the 8910 is hooked up is even sillier! */
static int latch;

static void zigzag_8910_latch(int offset,int data)
{
	latch = offset;
}

static void zigzag_8910_data_trigger(int offset,int data)
{
	AY8910_write_port_0_w(0,latch);
}

static void zigzag_8910_control_trigger(int offset,int data)
{
	AY8910_control_port_0_w(0,latch);
}

static struct MemoryReadAddress zigzag_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x2fff, MRA_BANK1 },
	{ 0x3000, 0x3fff, MRA_BANK2 },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x5000, 0x5fff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* DSW */
	{ 0x7800, 0x7800, watchdog_reset_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress zigzag_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4800, MWA_NOP },	/* part of the 8910 interface */
	{ 0x4801, 0x4801, zigzag_8910_data_trigger },
	{ 0x4803, 0x4803, zigzag_8910_control_trigger },
	{ 0x4900, 0x49ff, zigzag_8910_latch },
	{ 0x4a00, 0x4a00, MWA_NOP },	/* part of the 8910 interface */
	{ 0x5000, 0x53ff, videoram_w, &videoram, &videoram_size },
	{ 0x5800, 0x583f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5840, 0x587f, MWA_RAM, &spriteram, &spriteram_size },	/* no bulletsram, all sprites */
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7002, 0x7002, zigzag_sillyprotection_w },
	{ 0x7006, 0x7006, galaxian_flipx_w },
	{ 0x7007, 0x7007, galaxian_flipy_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( galaxian )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "7000" )
	PORT_DIPSETTING(    0x01, "10000" )
	PORT_DIPSETTING(    0x02, "12000" )
	PORT_DIPSETTING(    0x03, "20000" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( superg )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "4000" )
	PORT_DIPSETTING(    0x02, "5000" )
	PORT_DIPSETTING(    0x03, "7000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( pisces )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 2C/1C  B 1C/2C 2C/5C" )
	PORT_DIPSETTING(    0x00, "A 1C/1C  B 1C/5C" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( warofbug )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP   | IPF_8WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )
/* 0x80 gives 2 Coins/1 Credit */

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "500000" )
	PORT_DIPSETTING(    0x00, "750000" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( redufo )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, "A 2C/1C  B 1C/3C" )
	PORT_DIPSETTING(    0x00, "A 1C/1C  B 1C/6C" )
	PORT_DIPSETTING(    0x80, "A 1C/2C  B 1C/12C" )
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "4000" )
	PORT_DIPSETTING(    0x02, "5000" )
	PORT_DIPSETTING(    0x03, "7000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( pacmanbl )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( devilfsg )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x01, "15000" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )   /* Probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( zigzag )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000 60000" )
	PORT_DIPSETTING(    0x04, "20000 60000" )
	PORT_DIPSETTING(    0x08, "30000 60000" )
	PORT_DIPSETTING(    0x0c, "40000 60000" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( mooncrgx )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "30000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )   /* probably unused */
  /*PORT_DIPNAME( 0x04, 0x00, "Language" )    This version is always in English */
													  	/* Code has been commented out at 0x2f4b */
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )   /* probably unused */
INPUT_PORTS_END

INPUT_PORTS_START( scramblb )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP   | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_BITX( 0,       0x0c, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )   /* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )   /* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )   /* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )   /* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( jumpbug )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP   | IPF_8WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x40, 0x00, "Difficulty ?" )
	PORT_DIPSETTING(    0x00, "Hard?" )
	PORT_DIPSETTING(    0x40, "Easy?" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, "2/1 2/1" )
	PORT_DIPSETTING(    0x08, "2/1 1/3" )
	PORT_DIPSETTING(    0x00, "1/1 1/1" )
	PORT_DIPSETTING(    0x0c, "1/1 1/6" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( levers )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )  /* probably unused */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )	/* used */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )  /* probably unused */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( azurian )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		/* used */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x80, "5" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )		/* used */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		/* used */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( orbitron )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 2C/1C  B 1C/3C" )
	PORT_DIPSETTING(    0x40, "A 1C/1C  B 1C/6C" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( checkmaj )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL) /* p2 tiles right */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL) /* p2 tiles left */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x40, "A 2/1 B 1/3" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DSW */
 	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "100000" )
	PORT_DIPSETTING(    0x04, "200000" )
	PORT_DIPNAME( 0x08, 0x00, "Difficulty Increases At Level" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* p1 tiles right */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* p1 tiles left */
INPUT_PORTS_END

INPUT_PORTS_START( swarm )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_DIPSETTING(    0x03, "40000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( streakng )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x40, "10000" )
	PORT_DIPSETTING(    0x80, "15000" )
	PORT_DIPSETTING(    0xc0, "20000" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( blkhole )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )	/* Bonus Life? */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


static struct GfxLayout galaxian_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout galaxian_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

/* twice as many characters and sprites */
static struct GfxLayout pisces_charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout pisces_spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout jumpbug_charlayout =
{
	8,8,	/* 8*8 characters */
	768,	/* 768 characters */
	2,	/* 2 bits per pixel */
	{ 0, 768*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout jumpbug_spritelayout =
{
	16,16,	/* 16*16 sprites */
	192,	/* 192 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 192*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	3,1,	/* 3*1 line */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 2, 2, 2 },	/* I "know" that this bit of the */
	{ 0 },			/* graphics ROMs is 1 */
	0	/* no use */
};
static struct GfxLayout scramble_bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	7,1,	/* it's just 1 pixel, but we use 7*1 to position it correctly */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 3, 0, 0, 0, 0, 0, 0 },	/* I "know" that this bit of the */
	{ 0 },						/* graphics ROMs is 1 */
	0	/* no use */
};
static struct GfxLayout backgroundlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	8,8,
	32,	/* one for each column */
	7,	/* 128 colors max */
	{ 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8*8, 1*8*8, 2*8*8, 3*8*8, 4*8*8, 5*8*8, 6*8*8, 7*8*8 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	8*8*8	/* each character takes 64 bytes */
};


static struct GfxDecodeInfo galaxian_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &galaxian_charlayout,    0, 8 },
	{ REGION_GFX1, 0x0000, &galaxian_spritelayout,  0, 8 },
	{ REGION_GFX1, 0x0000, &bulletlayout,         8*4, 2 },
	{ 0,           0x0000, &backgroundlayout, 8*4+2*2, 1 },	/* this will be dynamically created */
	{ -1 } /* end of array */
};
static struct GfxDecodeInfo pisces_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &pisces_charlayout,      0, 8 },
	{ REGION_GFX1, 0x0000, &pisces_spritelayout,    0, 8 },
	{ REGION_GFX1, 0x0000, &bulletlayout,         8*4, 2 },
	{ 0,           0x0000, &backgroundlayout, 8*4+2*2, 1 },	/* this will be dynamically created */
	{ -1 } /* end of array */
};
/* seperate character and sprite ROMs */
static struct GfxDecodeInfo pacmanbl_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &galaxian_charlayout,    0, 8 },
	{ REGION_GFX1, 0x1000, &galaxian_spritelayout,  0, 8 },
	{ REGION_GFX1, 0x0000, &bulletlayout,         8*4, 2 },
	{ 0,           0x0000, &backgroundlayout, 8*4+2*2, 1 },	/* this will be dynamically created */
	{ -1 } /* end of array */
};
static struct GfxDecodeInfo scramble_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &galaxian_charlayout,    0, 8 },
	{ REGION_GFX1, 0x0000, &galaxian_spritelayout,  0, 8 },
	{ REGION_GFX1, 0x0000, &scramble_bulletlayout,8*4, 1 },	/* 1 color code instead of 2, so all */
													/* shots will be yellow */
	{ 0,           0x0000, &backgroundlayout, 8*4+2*2, 1 },	/* this will be dynamically created */
	{ -1 } /* end of array */
};
static struct GfxDecodeInfo jumpbug_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &jumpbug_charlayout,     0, 8 },
	{ REGION_GFX1, 0x0000, &jumpbug_spritelayout,   0, 8 },
	{ REGION_GFX1, 0x0000, &bulletlayout,         8*4, 2 },
	{ 0,           0x0000, &backgroundlayout, 8*4+2*2, 1 },	/* this will be dynamically created */
	{ -1 } /* end of array */
};


static struct CustomSound_interface custom_interface =
{
	mooncrst_sh_start,
	mooncrst_sh_stop,
	mooncrst_sh_update
};

static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1789750,	/* 1.78975 MHz? */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct AY8910interface checkmaj_ay8910_interface =
{
	1,	/* 1 chip */
	1620000,	/* 1.62 MHz? (Used the same as Moon Cresta) */
	{ 50 },
	{ soundlatch_r },
	{ 0 },
	{ 0 },
	{ 0 }
};


#define MACHINE_DRIVER(NAME, INTERRUPT, MACHINEINIT, GFXDECODEINFO, VHSTART)	\
																				\
static struct MachineDriver machine_driver_##NAME =								\
{																				\
	/* basic machine hardware */												\
	{																			\
		{																		\
			CPU_Z80,															\
			18432000/6,	/* 3.072 Mhz */											\
			readmem,writemem,0,0,												\
			INTERRUPT##_vh_interrupt,1											\
		}																		\
	},																			\
	60, 2500,	/* frames per second, vblank duration */						\
	1,	/* single CPU, no need for interleaving */								\
	MACHINEINIT,																\
																				\
	/* video hardware */														\
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },									\
	GFXDECODEINFO##_gfxdecodeinfo,												\
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */	\
	galaxian_vh_convert_color_prom,												\
																				\
	VIDEO_TYPE_RASTER,															\
	0,																			\
	VHSTART##_vh_start,															\
	generic_vh_stop,															\
	galaxian_vh_screenrefresh,													\
																				\
	/* sound hardware */														\
	0,0,0,0,																	\
	{																			\
		{																		\
			SOUND_CUSTOM,														\
			&custom_interface													\
		}																		\
	}																			\
};

/*						 INTERRUPT	MACHINE_INIT           GFXDECODE  VH_START */

MACHINE_DRIVER(galaxian, galaxian,  galaxian_machine_init, galaxian,  galaxian)
MACHINE_DRIVER(warofbug, galaxian,  0,                     galaxian,  galaxian)
MACHINE_DRIVER(galapx,   galaxian,  galapx_machine_init,   galaxian,  galaxian)
MACHINE_DRIVER(pisces,   galaxian,  0,                     pisces,    pisces)
MACHINE_DRIVER(mooncrgx, galaxian,  0,                     pisces,    mooncrgx)
MACHINE_DRIVER(pacmanbl, galaxian,  0,                     pacmanbl,  galaxian)
MACHINE_DRIVER(devilfsg, devilfsg,  0,                     pacmanbl,  galaxian)
MACHINE_DRIVER(azurian,  galaxian,  0,                     scramble,  galaxian)

static struct MachineDriver machine_driver_zigzag =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			zigzag_readmem,zigzag_writemem,0,0,
			nmi_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	pacmanbl_gfxdecodeinfo,
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	zigzag_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_scramblb =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			scramblb_readmem,scramblb_writemem,0,0,
			scramble_vh_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	scramble_gfxdecodeinfo,
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
		}
	}
};

static struct MachineDriver machine_driver_jumpbug =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			jumpbug_readmem,jumpbug_writemem,0,0,
			scramble_vh_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	jumpbug_gfxdecodeinfo,
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	jumpbug_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_checkmaj =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			readmem,checkmaj_writemem,0,0,
			galaxian_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1620000,	/* 1.62 MHz? (used the same as Moon Cresta) */
			checkmaj_sound_readmem,checkmaj_sound_writemem,0,0,
			interrupt,32	/* NMIs are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	galaxian_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&checkmaj_ay8910_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( galaxian )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "galmidw.u",    0x0000, 0x0800, 0x745e2d61 )
	ROM_LOAD( "galmidw.v",    0x0800, 0x0800, 0x9c999a40 )
	ROM_LOAD( "galmidw.w",    0x1000, 0x0800, 0xb5894925 )
	ROM_LOAD( "galmidw.y",    0x1800, 0x0800, 0x6b3ca10b )
	ROM_LOAD( "7l",           0x2000, 0x0800, 0x1b933207 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "1h",           0x0000, 0x0800, 0x39fb43a4 )
	ROM_LOAD( "1k",           0x0800, 0x0800, 0x7e3f56a2 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( galmidw )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "galmidw.u",    0x0000, 0x0800, 0x745e2d61 )
	ROM_LOAD( "galmidw.v",    0x0800, 0x0800, 0x9c999a40 )
	ROM_LOAD( "galmidw.w",    0x1000, 0x0800, 0xb5894925 )
	ROM_LOAD( "galmidw.y",    0x1800, 0x0800, 0x6b3ca10b )
	ROM_LOAD( "galmidw.z",    0x2000, 0x0800, 0xcb24f797 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "galmidw.1j",   0x0000, 0x0800, 0x84decf98 )
	ROM_LOAD( "galmidw.1k",   0x0800, 0x0800, 0xc31ada9e )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( superg )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "superg.u",     0x0000, 0x0800, 0xe8f3aa67 )
	ROM_LOAD( "superg.v",     0x0800, 0x0800, 0xf58283e3 )
	ROM_LOAD( "superg.w",     0x1000, 0x0800, 0xddeabdae )
	ROM_LOAD( "superg.y",     0x1800, 0x0800, 0x9463f753 )
	ROM_LOAD( "superg.z",     0x2000, 0x0800, 0xe6312e35 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "galmidw.1j",   0x0000, 0x0800, 0x84decf98 )
	ROM_LOAD( "galmidw.1k",   0x0800, 0x0800, 0xc31ada9e )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( galaxb )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "superg.u",     0x0000, 0x0800, 0xe8f3aa67 )
	ROM_LOAD( "superg.v",     0x0800, 0x0800, 0xf58283e3 )
	ROM_LOAD( "cp3",          0x1000, 0x0800, 0x4c7031c0 )
	ROM_LOAD( "cp4",          0x1800, 0x0800, 0x097d92a2 )
	ROM_LOAD( "cp5",          0x2000, 0x0800, 0x5341d75a )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "cp7e",         0x0000, 0x0800, 0xd0ba22c9 )   /* logo was removed */
	ROM_LOAD( "cp6e",         0x0800, 0x0800, 0x977e37cf )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( galapx )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "galx.u",       0x0000, 0x0800, 0x79e4007d )
	ROM_LOAD( "galx.v",       0x0800, 0x0800, 0xbc16064e )
	ROM_LOAD( "galx.w",       0x1000, 0x0800, 0x72d2d3ee )
	ROM_LOAD( "galx.y",       0x1800, 0x0800, 0xafe397f3 )
	ROM_LOAD( "galx.z",       0x2000, 0x0800, 0x778c0d3c )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "galx.1h",      0x0000, 0x0800, 0xe8810654 )
	ROM_LOAD( "galx.1k",      0x0800, 0x0800, 0xcbe84a76 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( galap1 )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "superg.u",     0x0000, 0x0800, 0xe8f3aa67 )
	ROM_LOAD( "superg.v",     0x0800, 0x0800, 0xf58283e3 )
	ROM_LOAD( "cp3",          0x1000, 0x0800, 0x4c7031c0 )
	ROM_LOAD( "galx_1_4.rom", 0x1800, 0x0800, 0xe71e1d9e )
	ROM_LOAD( "galx_1_5.rom", 0x2000, 0x0800, 0x6e65a3b2 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "galmidw.1j",   0x0000, 0x0800, 0x84decf98 )
	ROM_LOAD( "galmidw.1k",   0x0800, 0x0800, 0xc31ada9e )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( galap4 )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "galnamco.u",   0x0000, 0x0800, 0xacfde501 )
	ROM_LOAD( "galnamco.v",   0x0800, 0x0800, 0x65cf3c77 )
	ROM_LOAD( "galnamco.w",   0x1000, 0x0800, 0x9eef9ae6 )
	ROM_LOAD( "galnamco.y",   0x1800, 0x0800, 0x56a5ddd1 )
	ROM_LOAD( "galnamco.z",   0x2000, 0x0800, 0xf4bc7262 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "galx_4c1.rom", 0x0000, 0x0800, 0xd5e88ab4 )
	ROM_LOAD( "galx_4c2.rom", 0x0800, 0x0800, 0xa57b83e4 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( galturbo )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "superg.u",     0x0000, 0x0800, 0xe8f3aa67 )
	ROM_LOAD( "galx.v",       0x0800, 0x0800, 0xbc16064e )
	ROM_LOAD( "superg.w",     0x1000, 0x0800, 0xddeabdae )
	ROM_LOAD( "galturbo.y",   0x1800, 0x0800, 0xa44f450f )
	ROM_LOAD( "galturbo.z",   0x2000, 0x0800, 0x3247f3d4 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "galturbo.1h",  0x0000, 0x0800, 0xa713fd1a )
	ROM_LOAD( "galturbo.1k",  0x0800, 0x0800, 0x28511790 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( swarm )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "swarm1.bin",    0x0000, 0x0800, 0x21eba3d0 )
	ROM_LOAD( "swarm2.bin",    0x0800, 0x0800, 0xf3a436cd )
	ROM_LOAD( "swarm3.bin",    0x1000, 0x0800, 0x2915e38b )
	ROM_LOAD( "swarm4.bin",    0x1800, 0x0800, 0x8bbbf486 )
	ROM_LOAD( "swarm5.bin",    0x2000, 0x0800, 0xf1b1987e )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "swarma.bin",    0x0000, 0x0800, 0xef8657bb )
	ROM_LOAD( "swarmb.bin",    0x0800, 0x0800, 0x60c4bd31 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( pisces )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "pisces.a1",    0x0000, 0x0800, 0x856b8e1f )
	ROM_LOAD( "pisces.a2",    0x0800, 0x0800, 0x055f9762 )
	ROM_LOAD( "pisces.b2",    0x1000, 0x0800, 0x5540f2e4 )
	ROM_LOAD( "pisces.c1",    0x1800, 0x0800, 0x44aaf525 )
	ROM_LOAD( "pisces.d1",    0x2000, 0x0800, 0xfade512b )
	ROM_LOAD( "pisces.e2",    0x2800, 0x0800, 0x5ab2822f )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "pisces.1j",    0x0000, 0x1000, 0x2dba9e0e )
	ROM_LOAD( "pisces.1k",    0x1000, 0x1000, 0xcdc5aa26 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "6331-1j.86",   0x0000, 0x0020, 0x24652bc4 ) /* very close to Galaxian */
ROM_END

ROM_START( uniwars )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "f07_1a.bin",   0x0000, 0x0800, 0xd975af10 )
	ROM_LOAD( "h07_2a.bin",   0x0800, 0x0800, 0xb2ed14c3 )
	ROM_LOAD( "k07_3a.bin",   0x1000, 0x0800, 0x945f4160 )
	ROM_LOAD( "m07_4a.bin",   0x1800, 0x0800, 0xddc80bc5 )
	ROM_LOAD( "d08p_5a.bin",  0x2000, 0x0800, 0x62354351 )
	ROM_LOAD( "gg6",          0x2800, 0x0800, 0x270a3f4d )
	ROM_LOAD( "m08p_7a.bin",  0x3000, 0x0800, 0xc9245346 )
	ROM_LOAD( "n08p_8a.bin",  0x3800, 0x0800, 0x797d45c7 )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "egg10",        0x0000, 0x0800, 0x012941e0 )
	ROM_LOAD( "h01_2.bin",    0x0800, 0x0800, 0xc26132af )
	ROM_LOAD( "egg9",         0x1000, 0x0800, 0xfc8b58fd )
	ROM_LOAD( "k01_2.bin",    0x1800, 0x0800, 0xdcc2b33b )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "uniwars.clr",  0x0000, 0x0020, 0x25c79518 )
ROM_END

ROM_START( gteikoku )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "f07_1a.bin",   0x0000, 0x0800, 0xd975af10 )
	ROM_LOAD( "h07_2a.bin",   0x0800, 0x0800, 0xb2ed14c3 )
	ROM_LOAD( "k07_3a.bin",   0x1000, 0x0800, 0x945f4160 )
	ROM_LOAD( "m07_4a.bin",   0x1800, 0x0800, 0xddc80bc5 )
	ROM_LOAD( "d08p_5a.bin",  0x2000, 0x0800, 0x62354351 )
	ROM_LOAD( "e08p_6a.bin",  0x2800, 0x0800, 0xd915a389 )
	ROM_LOAD( "m08p_7a.bin",  0x3000, 0x0800, 0xc9245346 )
	ROM_LOAD( "n08p_8a.bin",  0x3800, 0x0800, 0x797d45c7 )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "h01_1.bin",    0x0000, 0x0800, 0x8313c959 )
	ROM_LOAD( "h01_2.bin",    0x0800, 0x0800, 0xc26132af )
	ROM_LOAD( "k01_1.bin",    0x1000, 0x0800, 0xc9d4537e )
	ROM_LOAD( "k01_2.bin",    0x1800, 0x0800, 0xdcc2b33b )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( spacbatt )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "f07_1a.bin",   0x0000, 0x0800, 0xd975af10 )
	ROM_LOAD( "h07_2a.bin",   0x0800, 0x0800, 0xb2ed14c3 )
	ROM_LOAD( "sb.3",         0x1000, 0x0800, 0xc25ce4c1 )
	ROM_LOAD( "sb.4",         0x1800, 0x0800, 0x8229835c )
	ROM_LOAD( "sb.5",         0x2000, 0x0800, 0xf51ef930 )
	ROM_LOAD( "e08p_6a.bin",  0x2800, 0x0800, 0xd915a389 )
	ROM_LOAD( "m08p_7a.bin",  0x3000, 0x0800, 0xc9245346 )
	ROM_LOAD( "sb.8",         0x3800, 0x0800, 0xe59ff1ae )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "h01_1.bin",    0x0000, 0x0800, 0x8313c959 )
	ROM_LOAD( "h01_2.bin",    0x0800, 0x0800, 0xc26132af )
	ROM_LOAD( "k01_1.bin",    0x1000, 0x0800, 0xc9d4537e )
	ROM_LOAD( "k01_2.bin",    0x1800, 0x0800, 0xdcc2b33b )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( warofbug )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "warofbug.u",   0x0000, 0x0800, 0xb8dfb7e3 )
	ROM_LOAD( "warofbug.v",   0x0800, 0x0800, 0xfd8854e0 )
	ROM_LOAD( "warofbug.w",   0x1000, 0x0800, 0x4495aa14 )
	ROM_LOAD( "warofbug.y",   0x1800, 0x0800, 0xc14a541f )
	ROM_LOAD( "warofbug.z",   0x2000, 0x0800, 0xc167fe55 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "warofbug.1k",  0x0000, 0x0800, 0x8100fa85 )
	ROM_LOAD( "warofbug.1j",  0x0800, 0x0800, 0xd1220ae9 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "warofbug.clr", 0x0000, 0x0020, 0x8688e64b )
ROM_END

ROM_START( redufo )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "ru1a",         0x0000, 0x0800, 0x5a8e4f37 )
	ROM_LOAD( "ru2a",         0x0800, 0x0800, 0xc624f52d )
	ROM_LOAD( "ru3a",         0x1000, 0x0800, 0xe1030d1c )
	ROM_LOAD( "ru4a",         0x1800, 0x0800, 0x7692069e )
	ROM_LOAD( "ru5a",         0x2000, 0x0800, 0xcb648ff3 )
	ROM_LOAD( "ru6a",         0x2800, 0x0800, 0xe1a9f58e )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "ruhja",        0x0000, 0x0800, 0x8a422b0d )
	ROM_LOAD( "rukla",        0x0800, 0x0800, 0x1eb84cb1 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( pacmanbl )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "blpac1b",      0x0000, 0x0800, 0x6718df42 )
	ROM_LOAD( "blpac2b",      0x0800, 0x0800, 0x33be3648 )
	ROM_LOAD( "blpac3b",      0x1000, 0x0800, 0xf98c0ceb )
	ROM_LOAD( "blpac4b",      0x1800, 0x0800, 0xa9cd0082 )
	ROM_LOAD( "blpac5b",      0x2000, 0x0800, 0x6d475afc )
	ROM_LOAD( "blpac6b",      0x2800, 0x0800, 0xcbe863d3 )
	ROM_LOAD( "blpac7b",      0x3000, 0x0800, 0x7daef758 )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "blpac12b",     0x0000, 0x0800, 0xb2ed320b )
	ROM_LOAD( "blpac11b",     0x0800, 0x0800, 0xab88b2c4 )
	ROM_LOAD( "blpac10b",     0x1000, 0x0800, 0x44a45b72 )
	ROM_LOAD( "blpac9b",      0x1800, 0x0800, 0xfa84659f )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "6331-1j.86",   0x0000, 0x0020, 0x24652bc4 ) /* same as pisces */
ROM_END

ROM_START( devilfsg )
	ROM_REGIONX( 0x10000, REGION_CPU1 )     /* 64k for code */
	ROM_LOAD( "dfish1.7f",    0x2000, 0x0800, 0x2ab19698 )
	ROM_CONTINUE(             0x0000, 0x0800 )
	ROM_LOAD( "dfish2.7h",    0x2800, 0x0800, 0x4e77f097 )
	ROM_CONTINUE(             0x0800, 0x0800 )
	ROM_LOAD( "dfish3.7k",    0x3000, 0x0800, 0x3f16a4c6 )
	ROM_CONTINUE(             0x1000, 0x0800 )
	ROM_LOAD( "dfish4.7m",    0x3800, 0x0800, 0x11fc7e59 )
	ROM_CONTINUE(             0x1800, 0x0800 )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "dfish5.1h",    0x1000, 0x0800, 0xace6e31f )
	ROM_CONTINUE(             0x0000, 0x0800 )
	ROM_LOAD( "dfish6.1k",    0x1800, 0x0800, 0xd7a6c4c4 )
	ROM_CONTINUE(             0x0800, 0x0800 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )
ROM_END

ROM_START( zigzag )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "zz_d1.bin",    0x0000, 0x1000, 0x8cc08d81 )
	ROM_LOAD( "zz_d2.bin",    0x1000, 0x1000, 0x326d8d45 )
	ROM_LOAD( "zz_d4.bin",    0x2000, 0x1000, 0xa94ed92a )
	ROM_LOAD( "zz_d3.bin",    0x3000, 0x1000, 0xce5e7a00 )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "zz_6_h1.bin",  0x0000, 0x0800, 0x780c162a )
	ROM_CONTINUE(             0x1000, 0x0800 )
	ROM_LOAD( "zz_5.bin",     0x0800, 0x0800, 0xf3cdfec5 )
	ROM_CONTINUE(             0x1800, 0x0800 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "zzbp_e9.bin",  0x0000, 0x0020, 0xaa486dd0 )
ROM_END

ROM_START( zigzag2 )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "z1",           0x0000, 0x1000, 0x4c28349a )
	ROM_LOAD( "zz_d2.bin",    0x1000, 0x1000, 0x326d8d45 )
	ROM_LOAD( "zz_d4.bin",    0x2000, 0x1000, 0xa94ed92a )
	ROM_LOAD( "zz_d3.bin",    0x3000, 0x1000, 0xce5e7a00 )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "zz_6_h1.bin",  0x0000, 0x0800, 0x780c162a )
	ROM_CONTINUE(             0x1000, 0x0800 )
	ROM_LOAD( "zz_5.bin",     0x0800, 0x0800, 0xf3cdfec5 )
	ROM_CONTINUE(             0x1800, 0x0800 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "zzbp_e9.bin",  0x0000, 0x0020, 0xaa486dd0 )
ROM_END

ROM_START( mooncrgx )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "1",            0x0000, 0x0800, 0x84cf420b )
	ROM_LOAD( "2",            0x0800, 0x0800, 0x4c2a61a1 )
	ROM_LOAD( "3",            0x1000, 0x0800, 0x1962523a )
	ROM_LOAD( "4",            0x1800, 0x0800, 0x75dca896 )
	ROM_LOAD( "5",            0x2000, 0x0800, 0x32483039 )
	ROM_LOAD( "6",            0x2800, 0x0800, 0x43f2ab89 )
	ROM_LOAD( "7",            0x3000, 0x0800, 0x1e9c168c )
	ROM_LOAD( "8",            0x3800, 0x0800, 0x5e09da94 )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "1h_1_10.bin",  0x0000, 0x0800, 0x528da705 )
	ROM_LOAD( "12.chr",       0x0800, 0x0800, 0x5a4b17ea )
	ROM_LOAD( "9.chr",        0x1000, 0x0800, 0x70df525c )
	ROM_LOAD( "11.chr",       0x1800, 0x0800, 0xe0edccbd )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( scramblb )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "scramble.1k",  0x0000, 0x0800, 0x9e025c4a )
	ROM_LOAD( "scramble.2k",  0x0800, 0x0800, 0x306f783e )
	ROM_LOAD( "scramble.3k",  0x1000, 0x0800, 0x0500b701 )
	ROM_LOAD( "scramble.4k",  0x1800, 0x0800, 0xdd380a22 )
	ROM_LOAD( "scramble.5k",  0x2000, 0x0800, 0xdf0b9648 )
	ROM_LOAD( "scramble.1j",  0x2800, 0x0800, 0xb8c07b3c )
	ROM_LOAD( "scramble.2j",  0x3000, 0x0800, 0x88ac07a0 )
	ROM_LOAD( "scramble.3j",  0x3800, 0x0800, 0xc67d57ca )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "5f.k",         0x0000, 0x0800, 0x4708845b )
	ROM_LOAD( "5h.k",         0x0800, 0x0800, 0x11fd2887 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )
ROM_END

ROM_START( jumpbug )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "jb1",          0x0000, 0x1000, 0x415aa1b7 )
	ROM_LOAD( "jb2",          0x1000, 0x1000, 0xb1c27510 )
	ROM_LOAD( "jb3",          0x2000, 0x1000, 0x97c24be2 )
	ROM_LOAD( "jb4",          0x3000, 0x1000, 0x66751d12 )
	ROM_LOAD( "jb5",          0x8000, 0x1000, 0xe2d66faf )
	ROM_LOAD( "jb6",          0x9000, 0x1000, 0x49e0bdfd )
	ROM_LOAD( "jb7",          0xa000, 0x0800, 0x83d71302 )

	ROM_REGIONX( 0x3000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "jbl",          0x0000, 0x0800, 0x9a091b0a )
	ROM_LOAD( "jbm",          0x0800, 0x0800, 0x8a0fc082 )
	ROM_LOAD( "jbn",          0x1000, 0x0800, 0x155186e0 )
	ROM_LOAD( "jbi",          0x1800, 0x0800, 0x7749b111 )
	ROM_LOAD( "jbj",          0x2000, 0x0800, 0x06e8d7df )
	ROM_LOAD( "jbk",          0x2800, 0x0800, 0xb8dbddf3 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( jumpbugb )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "jb1",          0x0000, 0x1000, 0x415aa1b7 )
	ROM_LOAD( "jb2",          0x1000, 0x1000, 0xb1c27510 )
	ROM_LOAD( "jb3b",         0x2000, 0x1000, 0xcb8b8a0f )
	ROM_LOAD( "jb4",          0x3000, 0x1000, 0x66751d12 )
	ROM_LOAD( "jb5b",         0x8000, 0x1000, 0x7553b5e2 )
	ROM_LOAD( "jb6b",         0x9000, 0x1000, 0x47be9843 )
	ROM_LOAD( "jb7b",         0xa000, 0x0800, 0x460aed61 )

	ROM_REGIONX( 0x3000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "jbl",          0x0000, 0x0800, 0x9a091b0a )
	ROM_LOAD( "jbm",          0x0800, 0x0800, 0x8a0fc082 )
	ROM_LOAD( "jbn",          0x1000, 0x0800, 0x155186e0 )
	ROM_LOAD( "jbi",          0x1800, 0x0800, 0x7749b111 )
	ROM_LOAD( "jbj",          0x2000, 0x0800, 0x06e8d7df )
	ROM_LOAD( "jbk",          0x2800, 0x0800, 0xb8dbddf3 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( levers )
	ROM_REGIONX( 0x10000, REGION_CPU1 )       /* 64k for code */
	ROM_LOAD( "g96059.a8", 	  0x0000, 0x1000, 0x9550627a )
	ROM_LOAD( "g96060.d8", 	  0x2000, 0x1000, 0x5ac64646 )
	ROM_LOAD( "g96061.e8", 	  0x3000, 0x1000, 0x9db8e520 )
	ROM_LOAD( "g96062.h8", 	  0x8000, 0x1000, 0x7c8e8b3a )
	ROM_LOAD( "g96063.j8", 	  0x9000, 0x1000, 0xfa61e793 )
	ROM_LOAD( "g96064.l8", 	  0xa000, 0x1000, 0xf797f389 )

	ROM_REGIONX( 0x3000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "g95948.n1", 	  0x0000, 0x0800, 0xd8a0c692 )
							/*0x0800- 0x0fff empty */
	ROM_LOAD( "g95949.s1", 	  0x1000, 0x0800, 0x3660a552 )
	ROM_LOAD( "g95946.j1", 	  0x1800, 0x0800, 0x73b61b2d )
							/*0x2000- 0x27ff empty */
	ROM_LOAD( "g95947.m1", 	  0x2800, 0x0800, 0x72ff67e2 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "g960lev.clr",  0x0000, 0x0020, 0x01febbbe )
ROM_END

ROM_START( azurian )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "pgm.1",        0x0000, 0x1000, 0x17a0fca7 )
	ROM_LOAD( "pgm.2",        0x1000, 0x1000, 0x14659848 )
	ROM_LOAD( "pgm.3",        0x2000, 0x1000, 0x8f60fb97 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "gfx.1",        0x0000, 0x0800, 0xf5afb803 )
	ROM_LOAD( "gfx.2",        0x0800, 0x0800, 0xae96e5d1 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( orbitron )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "orbitron.3",   0x0600, 0x0200, 0x419f9c9b )
	ROM_CONTINUE(			  0x0400, 0x0200)
	ROM_CONTINUE(			  0x0200, 0x0200)
	ROM_CONTINUE(			  0x0000, 0x0200)
	ROM_LOAD( "orbitron.4",   0x0e00, 0x0200, 0x44ad56ac )
	ROM_CONTINUE(			  0x0c00, 0x0200)
	ROM_CONTINUE(			  0x0a00, 0x0200)
	ROM_CONTINUE(			  0x0800, 0x0200)
	ROM_LOAD( "orbitron.1",   0x1600, 0x0200, 0xda3f5168 )
	ROM_CONTINUE(			  0x1400, 0x0200)
	ROM_CONTINUE(			  0x1200, 0x0200)
	ROM_CONTINUE(			  0x1000, 0x0200)
	ROM_LOAD( "orbitron.2",   0x1e00, 0x0200, 0xa3b813fc )
	ROM_CONTINUE(			  0x1c00, 0x0200)
	ROM_CONTINUE(			  0x1a00, 0x0200)
	ROM_CONTINUE(			  0x1800, 0x0200)
	ROM_LOAD( "orbitron.5",   0x2000, 0x0800, 0x20cd8bb8 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "orbitron.6",   0x0000, 0x0800, 0x2c91b83f )
	ROM_LOAD( "orbitron.7",   0x0800, 0x0800, 0x46f4cca4 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( checkmaj )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "cm_1.bin",     0x0000, 0x1000, 0x456a118f )
	ROM_LOAD( "cm_2.bin",     0x1000, 0x1000, 0x146b2c44 )
	ROM_LOAD( "cm_3.bin",     0x2000, 0x0800, 0x73e1c945 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "cm_6.bin",     0x0000, 0x0800, 0x476a7cc3 )
	ROM_LOAD( "cm_5.bin",     0x0800, 0x0800, 0xb3df2b5f )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "checkman.clr", 0x0000, 0x0020, 0x57a45057 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for sound code */
	ROM_LOAD( "cm_4.bin",     0x0000, 0x1000, 0x923cffa1 )
ROM_END

ROM_START( streakng )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "sk1",          0x0000, 0x1000, 0xc8866ccb )
	ROM_LOAD( "sk2",          0x1000, 0x1000, 0x7caea29b )
	ROM_LOAD( "sk3",          0x2000, 0x1000, 0x7b4bfa76 )
	ROM_LOAD( "sk4",          0x3000, 0x1000, 0x056fc921 )

	ROM_REGIONX( 0x2000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "sk5",          0x0000, 0x1000, 0xd27f1e0c )
	ROM_LOAD( "sk6",          0x1000, 0x1000, 0xa7089588 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "sk.bpr",       0x0000, 0x0020, 0xbce79607 )
ROM_END

ROM_START( blkhole )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "bh1",          0x0000, 0x0800, 0x64998819 )
	ROM_LOAD( "bh2",          0x0800, 0x0800, 0x26f26ce4 )
	ROM_LOAD( "bh3",          0x1000, 0x0800, 0x3418bc45 )
	ROM_LOAD( "bh4",          0x1800, 0x0800, 0x735ff481 )
	ROM_LOAD( "bh5",          0x2000, 0x0800, 0x3f657be9 )
	ROM_LOAD( "bh6",          0x2800, 0x0800, 0xa057ab35 )

	ROM_REGIONX( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bh7",          0x0000, 0x0800, 0x975ba821 )
	ROM_LOAD( "bh8",          0x0800, 0x0800, 0x03d11020 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END


static void init_pisces(void)
{
	install_mem_write_handler(0, 0x6002, 0x6002, pisces_gfxbank_w);
}

static void init_checkmaj(void)
{
    /* for the title screen */
	install_mem_read_handler(0, 0x3800, 0x3800, checkmaj_protection_r);
}


GAME( 1979, galaxian, 0,        galaxian, galaxian, 0,        ROT90,  "Namco", "Galaxian (Namco)" )
GAME( 1979, galmidw,  galaxian, galaxian, galaxian, 0,        ROT90,  "[Namco] (Midway license)", "Galaxian (Midway)" )
GAME( 1979, superg,   galaxian, galaxian, superg,   0,        ROT90,  "hack", "Super Galaxians" )
GAME( 1979, galaxb,   galaxian, galaxian, superg,   0,        ROT90,  "bootleg", "Galaxian (bootleg)" )
GAME( 1979, galapx,   galaxian, galapx,   superg,   0,        ROT90,  "hack", "Galaxian Part X" )
GAME( 1979, galap1,   galaxian, galaxian, superg,   0,        ROT90,  "hack", "Space Invaders Galactica" )
GAME( 1979, galap4,   galaxian, galaxian, superg,   0,        ROT90,  "hack", "Galaxian Part 4" )
GAME( 1979, galturbo, galaxian, galaxian, superg,   0,        ROT90,  "hack", "Galaxian Turbo" )
GAME( 1979, swarm,    galaxian, galaxian, swarm,    0,        ROT90,  "hack", "Swarm" )
GAME( ????, pisces,   0,        pisces,   pisces,   pisces,   ROT90,  "<unknown>", "Pisces" )
GAME( 1980, uniwars,  0,        pisces,   superg,   pisces,   ROT90,  "Irem", "UniWar S" )
GAME( 1980, gteikoku, uniwars,  pisces,   superg,   pisces,   ROT90,  "Irem", "Gingateikoku No Gyakushu" )
GAME( 1980, spacbatt, uniwars,  pisces,   superg,   pisces,   ROT90,  "bootleg", "Space Battle" )
GAME( 1981, warofbug, 0,        warofbug, warofbug, 0,        ROT90,  "Armenia", "War of the Bugs" )
GAME( ????, redufo,   0,        warofbug, redufo,   0,        ROT90,  "Hara Industries??", "Defend the Terra Attack on the Red UFO" )
GAME( 1981, pacmanbl, pacman,   pacmanbl, pacmanbl, 0,        ROT270, "bootleg", "Pac-Man (bootleg on Galaxian hardware)" )
GAME( 1984, devilfsg, devilfsh, devilfsg, devilfsg, 0,        ROT270, "Vision / Artic", "Devil Fish (Galaxian hardware, bootleg?)" )
GAME( 1982, zigzag,   0,        zigzag,   zigzag,   0,        ROT90,  "LAX", "Zig Zag (Galaxian hardware, set 1)" )
GAME( 1982, zigzag2,  zigzag,   zigzag,   zigzag,   0,        ROT90,  "LAX", "Zig Zag (Galaxian hardware, set 2)" )
GAME( 1980, mooncrgx, mooncrst, mooncrgx, mooncrgx, 0,        ROT270, "bootleg", "Moon Cresta (bootleg on Galaxian hardware)" )
GAME( 1981, scramblb, scramble, scramblb, scramblb, 0,        ROT90,  "bootleg", "Scramble (bootleg on Galaxian hardware)" )
GAME( 1981, jumpbug,  0,        jumpbug,  jumpbug,  0,        ROT90,  "Rock-ola", "Jump Bug" )
GAME( 1981, jumpbugb, jumpbug,  jumpbug,  jumpbug,  0,        ROT90,  "bootleg", "Jump Bug (bootleg)" )
GAME( 1983, levers,   0,        jumpbug,  levers,   0,        ROT90,  "Rock-ola", "Levers" )
GAME( 1982, azurian,  0,        azurian,  azurian,  0,        ROT90,  "Rait Electronics Ltd", "Azurian Attack" )
GAME( ????, orbitron, 0,        azurian,  orbitron, 0,        ROT270, "Signatron USA", "Orbitron" )
GAME( 1982, checkmaj, checkman, checkmaj, checkmaj, checkmaj, ROT90,  "Jaleco", "Checkman (Japan)" )
GAME( 1980, streakng, 0,        pacmanbl, streakng, 0,        ROT90,  "Shoei", "Streaking" )
GAME( ????, blkhole,  0,        galaxian, blkhole,  0,        ROT90,  "<unknown>", "Black Hole" )
