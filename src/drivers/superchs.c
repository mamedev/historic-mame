/****************************************************************************

	Super Chase  						(c) 1992 Taito

	Driver by Bryan McPhail & David Graves.

	Board Info:

		CPU board:
		68000
		68020
		TC0570SPC (Taito custom)
		TC0470LIN (Taito custom)
		TC0510NIO (Taito custom)
		TC0480SCP (Taito custom)
		TC0650FDA (Taito custom)
		ADC0809CCN

		X2=26.686MHz
		X1=40MHz
		X3=32MHz

		Sound board:
		68000
		68681
		MB8421 (x2)
		MB87078
		Ensoniq 5510
		Ensoniq 5505

	(Acknowledgments and thanks to Richard Bush and the Raine team
	for their preliminary Super Chase driver.)

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"
#include "machine/eeprom.h"

int superchs_vh_start (void);
void superchs_vh_stop (void);
void superchs_vh_screenrefresh  (struct osd_bitmap *bitmap,int full_refresh);

static UINT16 coin_word;
static data32_t *superchs_ram;
static data32_t *shared_ram;
extern data32_t *f3_shared_ram;

static int steer=0;

/* from sndhrdw/taito_f3.c */
READ16_HANDLER(f3_68000_share_r);
WRITE16_HANDLER(f3_68000_share_w);
READ16_HANDLER(f3_68681_r);
WRITE16_HANDLER(f3_68681_w);
READ16_HANDLER(es5510_dsp_r);
WRITE16_HANDLER(es5510_dsp_w);
WRITE16_HANDLER(f3_volume_w);
WRITE16_HANDLER(f3_es5505_bank_w);
void f3_68681_reset(void);
extern WRITE16_HANDLER( es5505_bank_w ); /* drivers/f3 */

/*********************************************************************/

static READ16_HANDLER( shared_ram_r )
{
	if ((offset&1)==0) return (shared_ram[offset/2]&0xffff0000)>>16;
	return (shared_ram[offset/2]&0x0000ffff);
}

static WRITE16_HANDLER( shared_ram_w )
{
	if ((offset&1)==0) {
		if (ACCESSING_MSB)
			shared_ram[offset/2]=(shared_ram[offset/2]&0x00ffffff)|((data&0xff00)<<16);
		if (ACCESSING_LSB)
			shared_ram[offset/2]=(shared_ram[offset/2]&0xff00ffff)|((data&0x00ff)<<16);
	} else {
		if (ACCESSING_MSB)
			shared_ram[offset/2]=(shared_ram[offset/2]&0xffff00ff)|((data&0xff00)<< 0);
		if (ACCESSING_LSB)
			shared_ram[offset/2]=(shared_ram[offset/2]&0xffffff00)|((data&0x00ff)<< 0);
	}
}

static WRITE32_HANDLER( cpua_ctrl_w )
{
	/*
	CPUA writes 0x00, 22, 72, f2 in that order.
	f2 seems to be the standard in-game value.
	..x...x.
	.xxx..x.
	xxxx..x.
	is there an irq enable in the top nibble?
	*/

	if (ACCESSING_MSB) {
		cpu_set_reset_line(2,(data &0x200) ? CLEAR_LINE : ASSERT_LINE);
		if (data&0x8000) cpu_cause_interrupt(0,3); /* Guess */
	}
	if (ACCESSING_LSB) {
		/* Lamp control bits of some sort in the lsb */
	}
}

static WRITE32_HANDLER( superchs_palette_w )
{
	int a,r,g,b;
	COMBINE_DATA(&paletteram32[offset]);

	a = paletteram32[offset];
	r = (a &0xff0000) >> 16;
	g = (a &0xff00) >> 8;
	b = (a &0xff);

	palette_change_color(offset,r,g,b);
}

static READ32_HANDLER( superchs_input_r )
{
	switch (offset)
	{
		case 0x00:
			return (input_port_0_word_r(0,0) << 16) | input_port_1_word_r(0,0) |
				  (EEPROM_read_bit() << 7);

		case 0x01:
			return coin_word<<16;
 	}

	return 0xffffffff;
}

static WRITE32_HANDLER( superchs_input_w )
{

	#if 0
	{
	char t[64];
	static data32_t mem[2];
	COMBINE_DATA(&mem[offset]);
	sprintf(t,"%08x %08x",mem[0],mem[1]);
	//usrintf_showmessage(t);
	}
	#endif

	switch (offset)
	{
		case 0x00:
		{
			if (ACCESSING_MSB32)	/* $300000 is watchdog */
			{
				watchdog_reset_w(0,data >> 24);
			}

			if (ACCESSING_LSB32)
			{
				EEPROM_set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
				EEPROM_write_bit(data & 0x40);
				EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
				return;
			}

			return;
		}

		/* there are 'vibration' control bits somewhere! */

		case 0x01:
		{
			if (ACCESSING_MSB32)
			{
				coin_lockout_w(0,~data & 0x01000000);
				coin_lockout_w(1,~data & 0x02000000);
				coin_counter_w(0, data & 0x04000000);
				coin_counter_w(1, data & 0x08000000);
				coin_word=(data >> 16) &0xffff;
			}
		}
	}
}

static READ32_HANDLER( superchs_stick_r )
{
	int fake = input_port_6_word_r(0,0);
	int accel;

	if (!(fake &0x10))	/* Analogue steer (the real control method) */
	{
		steer = input_port_2_word_r(0,0);
	}
	else	/* Digital steer, with smoothing - speed depends on how often stick_r is called */
	{
		int delta;
		int goal = 0x80;
		if (fake &0x04) goal = 0xff;		/* pressing left */
		if (fake &0x08) goal = 0x0;		/* pressing right */

		if (steer!=goal)
		{
			delta = goal - steer;
			if (steer < goal)
			{
				if (delta >2) delta = 2;
			}
			else
			{
				if (delta < (-2)) delta = -2;
			}
			steer += delta;
		}
	}

	/* Accelerator is an analogue input but the game treats it as digital (on/off) */
	if (input_port_6_word_r(0,0) & 0x1)	/* pressing B1 */
		accel = 0x0;
	else
		accel = 0xff;

	/* Todo: Verify brake - and figure out other input */
	return (steer << 24) | (accel << 16) | (input_port_4_word_r(0,0) << 8) | input_port_5_word_r(0,0);
}

static WRITE32_HANDLER( superchs_stick_w )
{
	/* This is guess work - the interrupts are in groups of 4, with each writing to a
		different byte in this long word before the RTE.  I assume all but the last
		(top) byte cause an IRQ with the final one being an ACK.  (Total guess but it works). */
	if (mem_mask!=0x00ffffff)
		cpu_cause_interrupt(0,3);
}

/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ32_START( superchs_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x11ffff, MRA32_RAM },	/* main CPUA ram */
	{ 0x140000, 0x141fff, MRA32_RAM },	/* Sprite ram */
	{ 0x180000, 0x18ffff, TC0480SCP_long_r },
	{ 0x1b0000, 0x1b002f, TC0480SCP_ctrl_long_r },
	{ 0x200000, 0x20ffff, MRA32_RAM },	/* Shared ram */
	{ 0x280000, 0x287fff, MRA32_RAM },	/* Palette ram */
	{ 0x2c0000, 0x2c07ff, MRA32_RAM },	/* Sound shared ram */
	{ 0x300000, 0x300007, superchs_input_r },
	{ 0x340000, 0x340003, superchs_stick_r },	/* stick coord read */
MEMORY_END

static MEMORY_WRITE32_START( superchs_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x11ffff, MWA32_RAM, &superchs_ram },
	{ 0x140000, 0x141fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x180000, 0x18ffff, TC0480SCP_long_w },
	{ 0x1b0000, 0x1b002f, TC0480SCP_ctrl_long_w },
	{ 0x200000, 0x20ffff, MWA32_RAM, &shared_ram },
	{ 0x240000, 0x240003, cpua_ctrl_w },
	{ 0x280000, 0x287fff, superchs_palette_w, &paletteram32 },
	{ 0x2c0000, 0x2c07ff, MWA32_RAM, &f3_shared_ram },
	{ 0x300000, 0x300007, superchs_input_w },	/* eerom etc. */
	{ 0x340000, 0x340003, superchs_stick_w },	/* stick int request */
MEMORY_END

static MEMORY_READ16_START( superchs_cpub_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x200000, 0x20ffff, MRA16_RAM },	/* local ram */
	{ 0x800000, 0x80ffff, shared_ram_r },
	{ 0xa00000, 0xa001ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( superchs_cpub_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x200000, 0x20ffff, MWA16_RAM },
	{ 0x600000, 0x60ffff, TC0480SCP_word_w }, /* Only written upon errors */
	{ 0x800000, 0x80ffff, shared_ram_w },
	{ 0xa00000, 0xa001ff, MWA16_RAM },	/* Extra road control?? */
MEMORY_END

/******************************************************************************/

static MEMORY_READ16_START( sound_readmem )
	{ 0x000000, 0x03ffff, MRA16_RAM },
	{ 0x140000, 0x140fff, f3_68000_share_r },
	{ 0x200000, 0x20001f, ES5505_data_0_r },
	{ 0x260000, 0x2601ff, es5510_dsp_r },
	{ 0x280000, 0x28001f, f3_68681_r },
	{ 0xc00000, 0xcfffff, MRA16_BANK1 },
	{ 0xff8000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( sound_writemem )
	{ 0x000000, 0x03ffff, MWA16_RAM },
	{ 0x140000, 0x140fff, f3_68000_share_w },
	{ 0x200000, 0x20001f, ES5505_data_0_w },
	{ 0x260000, 0x2601ff, es5510_dsp_w },
	{ 0x280000, 0x28001f, f3_68681_w },
	{ 0x300000, 0x30003f, f3_es5505_bank_w },
	{ 0x340000, 0x340003, f3_volume_w }, /* 8 channel volume control */
	{ 0xc00000, 0xcfffff, MWA16_ROM },
	{ 0xff8000, 0xffffff, MWA16_RAM },
MEMORY_END

/***********************************************************/

INPUT_PORTS_START( superchs )
	PORT_START      /* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* Freeze input */
	PORT_BITX(0x0010, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* reserved for EEROM */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_BUTTON5 | IPF_PLAYER1 )	/* seat center (cockpit only) */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BITX(0x1000, IP_ACTIVE_LOW,  IPT_BUTTON3, "Nitro", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x2000, IP_ACTIVE_LOW,  IPT_BUTTON4, "Gear Shift", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x4000, IP_ACTIVE_LOW,  IPT_BUTTON2, "Brake", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START	/* IN 2, steering wheel */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 25, 15, 0, 0xff )

	PORT_START	/* IN 3, accel [effectively also brake for the upright] */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER1, 20, 10, 0, 0xff)

	PORT_START	/* IN 4, sound volume */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER2, 20, 10, 0, 0xff)

	PORT_START	/* IN 5, unknown */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER2, 20, 10, 0, 0xff)

	PORT_START	/* IN 6, inputs and DSW all fake */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x00, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END

/***********************************************************
				GFX DECODING
**********************************************************/

static struct GfxLayout tile16x16_layout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64,  2*64,  3*64,  4*64,  5*64,  6*64,  7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	64*16	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	16,16,    /* 16*16 characters */
	RGN_FRAC(1,1),
	4,        /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 5*4, 4*4, 3*4, 2*4, 7*4, 6*4, 9*4, 8*4, 13*4, 12*4, 11*4, 10*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8     /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo superchs_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x16_layout,  0, 512 },
	{ REGION_GFX1, 0x0, &charlayout,        0, 512 },
	{ -1 } /* end of array */
};


/***********************************************************
			     MACHINE DRIVERS
***********************************************************/

static void superchs_machine_reset(void)
{
	/* Sound cpu program loads to 0xc00000 so we use a bank */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU2);
	cpu_setbank(1,&RAM[0x80000]);

	RAM[0]=RAM[0x80000]; /* Stack and Reset vectors */
	RAM[1]=RAM[0x80001];
	RAM[2]=RAM[0x80002];
	RAM[3]=RAM[0x80003];

	f3_68681_reset();
}

static struct ES5505interface es5505_interface =
{
	1,					/* total number of chips */
	{ 13343000 },		/* freq - 26.686MHz/2??  May be 16MHz but Nancy sounds too high-pitched */
	{ REGION_SOUND1 },	/* Bank 0: Unused by F3 games? */
	{ REGION_SOUND1 },	/* Bank 1: All games seem to use this */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },		/* master volume */
	{ 0 }				/* irq callback */
};

static struct EEPROM_interface superchs_eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/* read command */
	"0101",			/* write command */
	"0111",			/* erase command */
	"0100000000",	/* unlock command */
	"0100110000",	/* lock command */
};

static data8_t default_eeprom[128]={
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x53,0x00,0x2e,0x00,0x43,0x00,0x00,
	0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0xff,0xff,0xff,0xff,0x00,0x01,
	0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x01,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x80,0xff,0xff,0xff,0xff,
	0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff
};

static void nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&superchs_eeprom_interface);

		if (file)
			EEPROM_load(file);
		else
			EEPROM_set_data(default_eeprom,128);  /* Default the wheel setup values */
	}
}

static struct MachineDriver machine_driver_superchs =
{
	{
		{
			CPU_M68EC020,
			16000000,	/* 16 MHz */
			superchs_readmem,superchs_writemem,0,0,
			m68_level2_irq,1 /* VBL */
		},
		{
			CPU_M68000 | CPU_AUDIO_CPU,
			16000000,	/* 16 MHz */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_M68000,
			16000000,	/* 16 MHz */
			superchs_cpub_readmem,superchs_cpub_writemem,0,0,
			m68_level4_irq,1 /* VBL */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	8,	/* CPU slices - Need to interleave Cpu's 1 & 3 */
	superchs_machine_reset,

	/* video hardware */
	40*8, 32*8, { 0, 40*8-1, 2*8, 32*8-1 },

	superchs_gfxdecodeinfo,
	8192, 8192,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN,
	0,
	superchs_vh_start,
	superchs_vh_stop,
	superchs_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_ES5505,
			&es5505_interface
		}
	},

	nvram_handler
};

/***************************************************************************/

ROM_START( superchs )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1024K for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "d46-35.27", 0x00000, 0x40000, 0x1575c9a7 )
	ROM_LOAD32_BYTE( "d46-34.25", 0x00001, 0x40000, 0xc72a4d2b )
	ROM_LOAD32_BYTE( "d46-33.23", 0x00002, 0x40000, 0x3094bcd0 )
	ROM_LOAD32_BYTE( "d46-31.21", 0x00003, 0x40000, 0x38b983a3 )

	ROM_REGION( 0x140000, REGION_CPU2, 0 )	/* Sound cpu */
	ROM_LOAD16_BYTE( "d46-37.8up", 0x100000, 0x20000, 0x60b51b91 )
	ROM_LOAD16_BYTE( "d46-36.7lo", 0x100001, 0x20000, 0x8f7aa276 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "d46-24.127", 0x00000, 0x20000, 0xa006baa1 )
	ROM_LOAD16_BYTE( "d46-23.112", 0x00001, 0x20000, 0x9a69dbd0 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "d46-05.87", 0x00000, 0x100000, 0x150d0e4c )	/* SCR 16x16 tiles */
	ROM_LOAD16_BYTE( "d46-06.88", 0x00001, 0x100000, 0x321308be )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "d46-01.64", 0x000003, 0x200000, 0x5c2ae92d )	/* OBJ 16x16 tiles: each rom has 1 bitplane */
	ROM_LOAD32_BYTE( "d46-02.65", 0x000002, 0x200000, 0xa83ca82e )
	ROM_LOAD32_BYTE( "d46-03.66", 0x000001, 0x200000, 0xe0e9cbfd )
	ROM_LOAD32_BYTE( "d46-04.67", 0x000000, 0x200000, 0x832769a9 )

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "d46-07.34", 0x00000, 0x80000, 0xc3b8b093 )	/* STY, used to create big sprites on the fly */

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "d46-10.2", 0x400000, 0x200000, 0x306256be )
	ROM_LOAD16_BYTE( "d46-12.4", 0x000000, 0x200000, 0xa24a53a8 )
	ROM_LOAD16_BYTE( "d46-11.5", 0xc00000, 0x200000, 0xd4ea0f56 )
ROM_END

static READ32_HANDLER( main_cycle_r )
{
	if (cpu_get_pc()==0x702)
		cpu_spinuntil_int();

	return superchs_ram[0];
}

static READ16_HANDLER( sub_cycle_r )
{
	if (cpu_get_pc()==0x454)
		cpu_spinuntil_int();

	return superchs_ram[2]&0xffff;
}

static void init_superchs(void)
{
	/* Speedup handlers */
	install_mem_read32_handler(0, 0x100000, 0x100003, main_cycle_r);
	install_mem_read16_handler(2, 0x80000a, 0x80000b, sub_cycle_r);
}

GAME( 1992, superchs, 0, superchs, superchs, superchs, ROT0_16BIT, "Taito America Corporation", "Super Chase - Criminal Termination (US)" )
