/***************************************************************************

Fire Trap memory map

driver by Nicola Salmoria

Z80:
0000-7fff ROM
8000-bfff Banked ROM (4 banks)
c000-cfff RAM
d000-d7ff bg #1 video/color RAM (alternating pages 0x100 long)
d000-dfff bg #2 video/color RAM (alternating pages 0x100 long)
e000-e3ff fg video RAM
e400-e7ff fg color RAM
e800-e97f sprites RAM

memory mapped ports:
read:
f010      IN0
f011      IN1
f012      IN2
f013      DSW0
f014      DSW1
f015      from pin 10 of 8751 controller
f016      from port #1 of 8751 controller

write:
f000      IRQ acknowledge
f001      sound command (also causes NMI on sound CPU)
f002      ROM bank selection
f003      flip screen
f004      NMI disable
f005      to port #2 of 8751 controller (signal on P3.2)
f008-f009 bg #1 x scroll
f00a-f00b bg #1 y scroll
f00c-f00d bg #2 x scroll
f00e-f00f bg #2 y scroll

interrupts:
VBlank triggers NMI.
the 8751 triggers IRQ

6502:
0000-07ff RAM
4000-7fff Banked ROM (2 banks)
8000-ffff ROM

read:
3400      command from the main cpu

write:
1000-1001 YM3526
2000      ADPCM data for the MSM5205 chip
2400      bit 0 = to sound chip MSM5205 (1 = play sample); bit 1 = IRQ enable
2800      ROM bank select

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"



extern unsigned char *firetrap_bg1videoram;
extern unsigned char *firetrap_bg2videoram;
extern unsigned char *firetrap_fgvideoram;

WRITE_HANDLER( firetrap_fgvideoram_w );
WRITE_HANDLER( firetrap_bg1videoram_w );
WRITE_HANDLER( firetrap_bg2videoram_w );
WRITE_HANDLER( firetrap_bg1_scrollx_w );
WRITE_HANDLER( firetrap_bg1_scrolly_w );
WRITE_HANDLER( firetrap_bg2_scrollx_w );
WRITE_HANDLER( firetrap_bg2_scrolly_w );
VIDEO_START( firetrap );
PALETTE_INIT( firetrap );
VIDEO_UPDATE( firetrap );


static int firetrap_irq_enable = 0;
static int firetrap_nmi_enable;

static WRITE_HANDLER( firetrap_nmi_disable_w )
{
	firetrap_nmi_enable=~data & 1;
}

static WRITE_HANDLER( firetrap_bankselect_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	bankaddress = 0x10000 + (data & 0x03) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}

static READ_HANDLER( firetrap_8751_bootleg_r )
{
	/* Check for coin insertion */
	/* the following only works in the bootleg version, which doesn't have an */
	/* 8751 - the real thing is much more complicated than that. */
	if ((readinputport(2) & 0x70) != 0x70) return 0xff;
	return 0;
}

static int i8751_return,i8751_current_command;

static MACHINE_INIT( firetrap )
{
	i8751_current_command=0;
}

static READ_HANDLER( firetrap_8751_r )
{
	//logerror("PC:%04x read from 8751\n",activecpu_get_pc());
	return i8751_return;
}

static WRITE_HANDLER( firetrap_8751_w )
{
	static int i8751_init_ptr=0;
	static const data8_t i8751_init_data[]={
		0xf5,0xd5,0xdd,0x21,0x05,0xc1,0x87,0x5f,0x87,0x83,0x5f,0x16,0x00,0xdd,0x19,0xd1,
		0xf1,0xc9,0xf5,0xd5,0xfd,0x21,0x2f,0xc1,0x87,0x5f,0x16,0x00,0xfd,0x19,0xd1,0xf1,
		0xc9,0xe3,0xd5,0xc5,0xf5,0xdd,0xe5,0xfd,0xe5,0xe9,0xe1,0xfd,0xe1,0xdd,0xe1,0xf1,
		0xc1,0xd1,0xe3,0xc9,0xf5,0xc5,0xe5,0xdd,0xe5,0xc5,0x78,0xe6,0x0f,0x47,0x79,0x48,
		0x06,0x00,0xdd,0x21,0x00,0xd0,0xdd,0x09,0xe6,0x0f,0x6f,0x26,0x00,0x29,0x29,0x29,
		0x29,0xeb,0xdd,0x19,0xc1,0x78,0xe6,0xf0,0x28,0x05,0x11,0x00,0x02,0xdd,0x19,0x79,
		0xe6,0xf0,0x28,0x05,0x11,0x00,0x04,0xdd,0x19,0xdd,0x5e,0x00,0x01,0x00,0x01,0xdd,
		0x09,0xdd,0x56,0x00,0xdd,0xe1,0xe1,0xc1,0xf1,0xc9,0xf5,0x3e,0x01,0x32,0x04,0xf0,
		0xf1,0xc9,0xf5,0x3e,0x00,0x32,0x04,0xf0,0xf1,0xc9,0xf5,0xd5,0xdd,0x21,0x05,0xc1,
		0x87,0x5f,0x87,0x83,0x5f,0x16,0x00,0xdd,0x19,0xd1,0xf1,0xc9,0xf5,0xd5,0xfd,0x21,
		0x2f,0xc1,0x87,0x5f,0x16,0x00,0xfd,0x19,0xd1,0xf1,0xc9,0xe3,0xd5,0xc5,0xf5,0xdd,
		0xe5,0xfd,0xe5,0xe9,0xe1,0xfd,0xe1,0xdd,0xe1,0xf1,0xc1,0xd1,0xe3,0xc9,0xf5,0xc5,
		0xe5,0xdd,0xe5,0xc5,0x78,0xe6,0x0f,0x47,0x79,0x48,0x06,0x00,0xdd,0x21,0x00,0xd0,
		0xdd,0x09,0xe6,0x0f,0x6f,0x26,0x00,0x29,0x29,0x29,0x29,0xeb,0xdd,0x19,0xc1,0x78,
		0xe6,0xf0,0x28,0x05,0x11,0x00,0x02,0xdd,0x19,0x79,0xe6,0xf0,0x28,0x05,0x11,0x00,
		0x04,0xdd,0x19,0xdd,0x5e,0x00,0x01,0x00,0x01,0xdd,0x09,0xdd,0x56,0x00,0xdd,0x00
	};
	static const int i8751_coin_data[]={ 0x00, 0xb7 };
	static const int i8751_36_data[]={ 0x00, 0xbc };

	/* End of command - important to note, as coin input is supressed while commands are pending */
	if (data==0x26) {
		i8751_current_command=0;
		i8751_return=0xff; /* This value is XOR'd and must equal 0 */
		cpu_set_irq_line_and_vector(0,0,HOLD_LINE,0xff);
		return;
	}

	/* Init sequence command */
	else if (data==0x13) {
		if (!i8751_current_command)
			i8751_init_ptr=0;
		i8751_return=i8751_init_data[i8751_init_ptr++];
	}

	/* Used to calculate a jump address when coins are inserted */
	else if (data==0xbd) {
		if (!i8751_current_command)
			i8751_init_ptr=0;
		i8751_return=i8751_coin_data[i8751_init_ptr++];
	}

	else if (data==0x36) {
		if (!i8751_current_command)
			i8751_init_ptr=0;
		i8751_return=i8751_36_data[i8751_init_ptr++];
	}

	/* Static value commands */
	else if (data==0x14)
		i8751_return=1;
	else if (data==0x02)
		i8751_return=0;
	else if (data==0x72)
		i8751_return=3;
	else if (data==0x69)
		i8751_return=2;
	else if (data==0xcb)
		i8751_return=0;
	else if (data==0x49)
		i8751_return=1;
	else if (data==0x17)
		i8751_return=2;
	else if (data==0x88)
		i8751_return=3;
	else {
		i8751_return=0xff;
		logerror("%04x: Unknown i8751 command %02x!\n",activecpu_get_pc(),data);
	}

	/* Signal main cpu task is complete */
	cpu_set_irq_line_and_vector(0,0,HOLD_LINE,0xff);
	i8751_current_command=data;
}

static WRITE_HANDLER( firetrap_sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}

static WRITE_HANDLER( firetrap_sound_2400_w )
{
	MSM5205_reset_w(offset,~data & 0x01);
	firetrap_irq_enable = data & 0x02;
}

static WRITE_HANDLER( firetrap_sound_bankselect_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU2);

	bankaddress = 0x10000 + (data & 0x01) * 0x4000;
	cpu_setbank(2,&RAM[bankaddress]);
}

static int msm5205next;

static void firetrap_adpcm_int (int data)
{
	static int toggle=0;

	MSM5205_data_w (0,msm5205next>>4);
	msm5205next<<=4;

	toggle ^= 1;
	if (firetrap_irq_enable && toggle)
		cpu_set_irq_line (1, M6502_IRQ_LINE, HOLD_LINE);
}

static WRITE_HANDLER( firetrap_adpcm_data_w )
{
	msm5205next = data;
}

static WRITE_HANDLER( flip_screen_w )
{
	flip_screen_set(data);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xe97f, MRA_RAM },
	{ 0xf010, 0xf010, input_port_0_r },
	{ 0xf011, 0xf011, input_port_1_r },
	{ 0xf012, 0xf012, input_port_2_r },
	{ 0xf013, 0xf013, input_port_3_r },
	{ 0xf014, 0xf014, input_port_4_r },
	{ 0xf016, 0xf016, firetrap_8751_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, firetrap_bg1videoram_w, &firetrap_bg1videoram },
	{ 0xd800, 0xdfff, firetrap_bg2videoram_w, &firetrap_bg2videoram },
	{ 0xe000, 0xe7ff, firetrap_fgvideoram_w,  &firetrap_fgvideoram },
	{ 0xe800, 0xe97f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf000, MWA_NOP },	/* IRQ acknowledge */
	{ 0xf001, 0xf001, firetrap_sound_command_w },
	{ 0xf002, 0xf002, firetrap_bankselect_w },
	{ 0xf003, 0xf003, flip_screen_w },
	{ 0xf004, 0xf004, firetrap_nmi_disable_w },
	{ 0xf005, 0xf005, firetrap_8751_w },
	{ 0xf008, 0xf009, firetrap_bg1_scrollx_w },
	{ 0xf00a, 0xf00b, firetrap_bg1_scrolly_w },
	{ 0xf00c, 0xf00d, firetrap_bg2_scrollx_w },
	{ 0xf00e, 0xf00f, firetrap_bg2_scrolly_w },
MEMORY_END

static MEMORY_READ_START( readmem_bootleg )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xe97f, MRA_RAM },
	{ 0xf010, 0xf010, input_port_0_r },
	{ 0xf011, 0xf011, input_port_1_r },
	{ 0xf012, 0xf012, input_port_2_r },
	{ 0xf013, 0xf013, input_port_3_r },
	{ 0xf014, 0xf014, input_port_4_r },
	{ 0xf016, 0xf016, firetrap_8751_bootleg_r },
	{ 0xf800, 0xf8ff, MRA_ROM },	/* extra ROM in the bootleg with unprotection code */
MEMORY_END

static MEMORY_WRITE_START( writemem_bootleg )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, firetrap_bg1videoram_w, &firetrap_bg1videoram },
	{ 0xd800, 0xdfff, firetrap_bg2videoram_w, &firetrap_bg2videoram },
	{ 0xe000, 0xe7ff, firetrap_fgvideoram_w,  &firetrap_fgvideoram },
	{ 0xe800, 0xe97f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf000, MWA_NOP },	/* IRQ acknowledge */
	{ 0xf001, 0xf001, firetrap_sound_command_w },
	{ 0xf002, 0xf002, firetrap_bankselect_w },
	{ 0xf003, 0xf003, flip_screen_w },
	{ 0xf004, 0xf004, firetrap_nmi_disable_w },
	{ 0xf005, 0xf005, MWA_NOP },
	{ 0xf008, 0xf009, firetrap_bg1_scrollx_w },
	{ 0xf00a, 0xf00b, firetrap_bg1_scrolly_w },
	{ 0xf00c, 0xf00d, firetrap_bg2_scrollx_w },
	{ 0xf00e, 0xf00f, firetrap_bg2_scrolly_w },
	{ 0xf800, 0xf8ff, MWA_ROM },	/* extra ROM in the bootleg with unprotection code */
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x3400, 0x3400, soundlatch_r },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1000, 0x1000, YM3526_control_port_0_w },
	{ 0x1001, 0x1001, YM3526_write_port_0_w },
	{ 0x2000, 0x2000, firetrap_adpcm_data_w },	/* ADPCM data for the MSM5205 chip */
	{ 0x2400, 0x2400, firetrap_sound_2400_w },
	{ 0x2800, 0x2800, firetrap_sound_bankselect_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( firetrap )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
//	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "50000 70000" )
	PORT_DIPSETTING(    0x20, "60000 80000" )
	PORT_DIPSETTING(    0x10, "80000 100000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START      /* Connected to i8751 directly */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
INPUT_PORTS_END

INPUT_PORTS_START( firetpbl )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )	/* bootleg only */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )	/* bootleg only */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )	/* bootleg only */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
//	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "50000 70000" )
	PORT_DIPSETTING(    0x20, "60000 80000" )
	PORT_DIPSETTING(    0x10, "80000 100000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ 0, 4 },
	{ 3, 2, 1, 0, RGN_FRAC(1,2)+3, RGN_FRAC(1,2)+2, RGN_FRAC(1,2)+1, RGN_FRAC(1,2)+0 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8
};
static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 3, 2, 1, 0, RGN_FRAC(1,4)+3, RGN_FRAC(1,4)+2, RGN_FRAC(1,4)+1, RGN_FRAC(1,4)+0,
			16*8+3, 16*8+2, 16*8+1, 16*8+0, RGN_FRAC(1,4)+16*8+3, RGN_FRAC(1,4)+16*8+2, RGN_FRAC(1,4)+16*8+1, RGN_FRAC(1,4)+16*8+0 },
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	32*8
};
static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x00, 16 },	/* colors 0x00-0x3f */
	{ REGION_GFX2, 0, &tilelayout,   0x80,  4 },	/* colors 0x80-0xbf */
	{ REGION_GFX3, 0, &tilelayout,   0xc0,  4 },	/* colors 0xc0-0xff */
	{ REGION_GFX4, 0, &spritelayout, 0x40,  4 },	/* colors 0x40-0x7f */
	{ -1 } /* end of array */
};



static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3.000000 MHz ? */
	{ 100 }		/* volume */
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip             */
	384000,				/* 384KHz ?           */
	{ firetrap_adpcm_int },/* interrupt function */
	{ MSM5205_S48_4B},	/* 8KHz ?             */
	{ 30 }
};

static INTERRUPT_GEN( firetrap )
{
	static int latch=0;
	static int coin_command_pending=0;

	/* Check for coin IRQ */
	if (cpu_getiloops()) {
		if ((readinputport(5) & 0x7) != 0x7 && !latch) {
			coin_command_pending=~readinputport(5);
			latch=1;
		}
		if ((readinputport(5) & 0x7) == 0x7)
			latch=0;

		/* Make sure coin IRQ's aren't generated when another command is pending, the main cpu
			definitely doesn't expect them as it locks out the coin routine */
		if (coin_command_pending && !i8751_current_command) {
			i8751_return=coin_command_pending;
			cpu_set_irq_line_and_vector(0,0,HOLD_LINE,0xff);
			coin_command_pending=0;
		}
	}

	if (firetrap_nmi_enable && !cpu_getiloops())
		cpu_set_irq_line (0, IRQ_LINE_NMI, PULSE_LINE);
}

static INTERRUPT_GEN( bootleg )
{
	if (firetrap_nmi_enable)
		cpu_set_irq_line (0, IRQ_LINE_NMI, PULSE_LINE);
}

static MACHINE_DRIVER_START( firetrap )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* 6 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(firetrap,2)

	MDRV_CPU_ADD(M6502,3072000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 1.536 MHz? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
							/* IRQs are caused by the ADPCM chip */
							/* NMIs are caused by the main CPU */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(firetrap)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(firetrap)
	MDRV_VIDEO_START(firetrap)
	MDRV_VIDEO_UPDATE(firetrap)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3526, ym3526_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( firetpbl )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* 6 MHz */
	MDRV_CPU_MEMORY(readmem_bootleg,writemem_bootleg)
	MDRV_CPU_VBLANK_INT(bootleg,1)

	MDRV_CPU_ADD(M6502,3072000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 1.536 MHz? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
							/* IRQs are caused by the ADPCM chip */
							/* NMIs are caused by the main CPU */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(firetrap)
	MDRV_VIDEO_START(firetrap)
	MDRV_VIDEO_UPDATE(firetrap)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3526, ym3526_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( firetrap )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code + 64k for banked ROMs */
	ROM_LOAD( "di02.bin",     0x00000, 0x8000, 0x3d1e4bf7 )
	ROM_LOAD( "di01.bin",     0x10000, 0x8000, 0x9bbae38b )
	ROM_LOAD( "di00.bin",     0x18000, 0x8000, 0xd0dad7de )

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the sound CPU + 32k for banked ROMs */
	ROM_LOAD( "di17.bin",     0x08000, 0x8000, 0x8605f6b9 )
	ROM_LOAD( "di18.bin",     0x10000, 0x8000, 0x49508c93 )

	/* there's also a protected 8751 microcontroller with ROM onboard */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )	/* characters */
	ROM_LOAD( "di03.bin",     0x00000, 0x2000, 0x46721930 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD( "di06.bin",     0x00000, 0x2000, 0x441d9154 )
	ROM_CONTINUE(             0x08000, 0x2000 )
	ROM_CONTINUE(             0x02000, 0x2000 )
	ROM_CONTINUE(             0x0a000, 0x2000 )
	ROM_LOAD( "di04.bin",     0x04000, 0x2000, 0x8e6e7eec )
	ROM_CONTINUE(             0x0c000, 0x2000 )
	ROM_CONTINUE(             0x06000, 0x2000 )
	ROM_CONTINUE(             0x0e000, 0x2000 )
	ROM_LOAD( "di07.bin",     0x10000, 0x2000, 0xef0a7e23 )
	ROM_CONTINUE(             0x18000, 0x2000 )
	ROM_CONTINUE(             0x12000, 0x2000 )
	ROM_CONTINUE(             0x1a000, 0x2000 )
	ROM_LOAD( "di05.bin",     0x14000, 0x2000, 0xec080082 )
	ROM_CONTINUE(             0x1c000, 0x2000 )
	ROM_CONTINUE(             0x16000, 0x2000 )
	ROM_CONTINUE(             0x1e000, 0x2000 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "di09.bin",     0x00000, 0x2000, 0xd11e28e8 )
	ROM_CONTINUE(             0x08000, 0x2000 )
	ROM_CONTINUE(             0x02000, 0x2000 )
	ROM_CONTINUE(             0x0a000, 0x2000 )
	ROM_LOAD( "di08.bin",     0x04000, 0x2000, 0xc32a21d8 )
	ROM_CONTINUE(             0x0c000, 0x2000 )
	ROM_CONTINUE(             0x06000, 0x2000 )
	ROM_CONTINUE(             0x0e000, 0x2000 )
	ROM_LOAD( "di11.bin",     0x10000, 0x2000, 0x6424d5c3 )
	ROM_CONTINUE(             0x18000, 0x2000 )
	ROM_CONTINUE(             0x12000, 0x2000 )
	ROM_CONTINUE(             0x1a000, 0x2000 )
	ROM_LOAD( "di10.bin",     0x14000, 0x2000, 0x9b89300a )
	ROM_CONTINUE(             0x1c000, 0x2000 )
	ROM_CONTINUE(             0x16000, 0x2000 )
	ROM_CONTINUE(             0x1e000, 0x2000 )

	ROM_REGION( 0x20000, REGION_GFX4, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "di16.bin",     0x00000, 0x8000, 0x0de055d7 )
	ROM_LOAD( "di13.bin",     0x08000, 0x8000, 0x869219da )
	ROM_LOAD( "di14.bin",     0x10000, 0x8000, 0x6b65812e )
	ROM_LOAD( "di15.bin",     0x18000, 0x8000, 0x3e27f77d )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "firetrap.3b",  0x0000,  0x0100, 0x8bb45337 ) /* palette red and green component */
	ROM_LOAD( "firetrap.4b",  0x0100,  0x0100, 0xd5abfc64 ) /* palette blue component */
ROM_END

ROM_START( firetpbl )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code + 96k for banked ROMs */
	ROM_LOAD( "ft0d.bin",     0x00000, 0x8000, 0x793ef849 )
	ROM_LOAD( "ft0a.bin",     0x08000, 0x8000, 0x613313ee )	/* unprotection code */
	ROM_LOAD( "ft0c.bin",     0x10000, 0x8000, 0x5c8a0562 )
	ROM_LOAD( "ft0b.bin",     0x18000, 0x8000, 0xf2412fe8 )

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the sound CPU + 32k for banked ROMs */
	ROM_LOAD( "di17.bin",     0x08000, 0x8000, 0x8605f6b9 )
	ROM_LOAD( "di18.bin",     0x10000, 0x8000, 0x49508c93 )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )	/* characters */
	ROM_LOAD( "ft0e.bin",     0x00000, 0x2000, 0xa584fc16 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD( "di06.bin",     0x00000, 0x2000, 0x441d9154 )
	ROM_CONTINUE(             0x08000, 0x2000 )
	ROM_CONTINUE(             0x02000, 0x2000 )
	ROM_CONTINUE(             0x0a000, 0x2000 )
	ROM_LOAD( "di04.bin",     0x04000, 0x2000, 0x8e6e7eec )
	ROM_CONTINUE(             0x0c000, 0x2000 )
	ROM_CONTINUE(             0x06000, 0x2000 )
	ROM_CONTINUE(             0x0e000, 0x2000 )
	ROM_LOAD( "di07.bin",     0x10000, 0x2000, 0xef0a7e23 )
	ROM_CONTINUE(             0x18000, 0x2000 )
	ROM_CONTINUE(             0x12000, 0x2000 )
	ROM_CONTINUE(             0x1a000, 0x2000 )
	ROM_LOAD( "di05.bin",     0x14000, 0x2000, 0xec080082 )
	ROM_CONTINUE(             0x1c000, 0x2000 )
	ROM_CONTINUE(             0x16000, 0x2000 )
	ROM_CONTINUE(             0x1e000, 0x2000 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "di09.bin",     0x00000, 0x2000, 0xd11e28e8 )
	ROM_CONTINUE(             0x08000, 0x2000 )
	ROM_CONTINUE(             0x02000, 0x2000 )
	ROM_CONTINUE(             0x0a000, 0x2000 )
	ROM_LOAD( "di08.bin",     0x04000, 0x2000, 0xc32a21d8 )
	ROM_CONTINUE(             0x0c000, 0x2000 )
	ROM_CONTINUE(             0x06000, 0x2000 )
	ROM_CONTINUE(             0x0e000, 0x2000 )
	ROM_LOAD( "di11.bin",     0x10000, 0x2000, 0x6424d5c3 )
	ROM_CONTINUE(             0x18000, 0x2000 )
	ROM_CONTINUE(             0x12000, 0x2000 )
	ROM_CONTINUE(             0x1a000, 0x2000 )
	ROM_LOAD( "di10.bin",     0x14000, 0x2000, 0x9b89300a )
	ROM_CONTINUE(             0x1c000, 0x2000 )
	ROM_CONTINUE(             0x16000, 0x2000 )
	ROM_CONTINUE(             0x1e000, 0x2000 )

	ROM_REGION( 0x20000, REGION_GFX4, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "di16.bin",     0x00000, 0x8000, 0x0de055d7 )
	ROM_LOAD( "di13.bin",     0x08000, 0x8000, 0x869219da )
	ROM_LOAD( "di14.bin",     0x10000, 0x8000, 0x6b65812e )
	ROM_LOAD( "di15.bin",     0x18000, 0x8000, 0x3e27f77d )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "firetrap.3b",  0x0000,  0x0100, 0x8bb45337 ) /* palette red and green component */
	ROM_LOAD( "firetrap.4b",  0x0100,  0x0100, 0xd5abfc64 ) /* palette blue component */
ROM_END



GAME( 1986, firetrap, 0,        firetrap, firetrap, 0, ROT90, "Data East USA", "Fire Trap (US)" )
GAME( 1986, firetpbl, firetrap, firetpbl, firetpbl, 0, ROT90, "bootleg", "Fire Trap (Japan bootleg)" )
