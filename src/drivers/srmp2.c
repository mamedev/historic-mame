/***************************************************************************

Super Real Mahjong P2
-------------------------------------
driver by Yochizo and Takahiro Nogi

  Yochizo took charge of video and I/O part.
  Takahiro Nogi took charge of sound, I/O and NVRAM part.

  ... and this is based on "seta.c" driver written by Luca Elia.

  Thanks for your reference, Takahiro Nogi and Luca Elia.


Supported games :
==================
 Super Real Mahjong Part2     (C) 1987 Seta
 Super Real Mahjong Part3     (C) 1988 Seta
 Mahjong Yuugi (set 1)        (C) 1990 Visco
 Mahjong Yuugi (set 2)        (C) 1990 Visco


Not supported game :
=====================
 Super Real Mahjong Part1 (not dumped)


System specs :
===============
   CPU       : 68000 (8MHz)
   Sound     : AY8910 + MSM5205
   Chips     : X1-001, X1-002A, X1-003, X1-004x2, X0-005 x2
           X1-001, X1-002A  : Sprites
           X1-003           : Video output
           X1-004           : ???
           X1-005           : ???


Known issues :
===============
 - I/O port isn't fully analized. Currently avoid I/O error message with hack.
 - AY-3-8910 sound may be wrong.
 - CPU clock of srmp3 does not match the real machine.
 - MSM5205 clock frequency in srmp3 is wrong.


Note:
======
 - In mjyuugi and mjyuugia, DSW3 (Debug switch) is available if you
   turn on the cheat switch.


****************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"


/***************************************************************************

  Variables

***************************************************************************/

void srmp2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void srmp2_vh_screenrefresh     (struct mame_bitmap *bitmap,int full_refresh);
void srmp3_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void srmp3_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
void mjyuugi_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);


extern int srmp2_color_bank;
extern int srmp3_gfx_bank;
extern int mjyuugi_gfx_bank;

static int srmp2_adpcm_bank;
static int srmp2_adpcm_data;
static unsigned long srmp2_adpcm_sptr;
static unsigned long srmp2_adpcm_eptr;

static int srmp2_port_select;

static unsigned short	*srmp2_nvram;
static size_t			srmp2_nvram_size;
static unsigned char	*srmp3_nvram;
static size_t			srmp3_nvram_size;


/***************************************************************************

  Interrupt(s)

***************************************************************************/

static int srmp2_interrupt(void)
{
	switch (cpu_getiloops())
	{
		case 0:		return 4;	/* vblank */
		default:	return 2;	/* sound */
	}
}

static int srmp3_interrupt(void)
{
	return interrupt();
}


static void init_srmp2(void)
{
	data16_t *RAM = (data16_t *) memory_region(REGION_CPU1);

	/* Fix "ERROR BACK UP" and "ERROR IOX" */
	RAM[0x20c80 / 2] = 0x4e75;								// RTS
}

static void init_srmp3(void)
{
	data8_t *RAM = memory_region(REGION_CPU1);

	/* BANK ROM (0x08000 - 0x1ffff) Check skip [MAIN ROM side] */
	RAM[0x00000 + 0x7b69] = 0x00;							// NOP
	RAM[0x00000 + 0x7b6a] = 0x00;							// NOP

	/* MAIN ROM (0x00000 - 0x07fff) Check skip [BANK ROM side] */
	RAM[0x08000 + 0xc10b] = 0x00;							// NOP
	RAM[0x08000 + 0xc10c] = 0x00;							// NOP
	RAM[0x08000 + 0xc10d] = 0x00;							// NOP
	RAM[0x08000 + 0xc10e] = 0x00;							// NOP
	RAM[0x08000 + 0xc10f] = 0x00;							// NOP
	RAM[0x08000 + 0xc110] = 0x00;							// NOP
	RAM[0x08000 + 0xc111] = 0x00;							// NOP

	/* "ERR IOX" Check skip [MAIN ROM side] */
	RAM[0x00000 + 0x784e] = 0x00;							// NOP
	RAM[0x00000 + 0x784f] = 0x00;							// NOP
	RAM[0x00000 + 0x7850] = 0x00;							// NOP
}

static void init_mjyuugi(void)
{
	data16_t *RAM = (data16_t *) memory_region(REGION_CPU1);

	/* Sprite RAM check skip */
	RAM[0x0276e / 2] = 0x4e75;								// RTS
}


static void srmp2_init_machine(void)
{
	srmp2_port_select = 0;
}

static void srmp3_init_machine(void)
{
	srmp2_port_select = 0;
}


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

static void srmp2_nvram_handler(void *file, int read_or_write)
{
	if (read_or_write)
	{
		osd_fwrite(file, srmp2_nvram, srmp2_nvram_size);
	}
	else
	{
		if (file)
		{
			osd_fread(file, srmp2_nvram, srmp2_nvram_size);
		}
		else
		{
			memset(srmp2_nvram, 0, srmp2_nvram_size);
		}
	}
}

static void srmp3_nvram_handler(void *file, int read_or_write)
{
	if (read_or_write)
	{
		osd_fwrite(file, srmp3_nvram, srmp3_nvram_size);
	}
	else
	{
		if (file)
		{
			osd_fread(file, srmp3_nvram, srmp3_nvram_size);
		}
		else
		{
			memset(srmp3_nvram, 0, srmp3_nvram_size);
		}
	}
}


static WRITE16_HANDLER( srmp2_flags_w )
{
/*
	---- ---x : Coin Counter
	---x ---- : Coin Lock Out
	--x- ---- : ADPCM Bank
	x--- ---- : Palette Bank
*/

	coin_counter_w( 0, ((data & 0x01) >> 0) );
	coin_lockout_w( 0, (((~data) & 0x10) >> 4) );
	srmp2_adpcm_bank = ( (data & 0x20) >> 5 );
	srmp2_color_bank = ( (data & 0x80) >> 7 );
}


static WRITE16_HANDLER( mjyuugi_flags_w )
{
/*
	---- ---x : Coin Counter
	---x ---- : Coin Lock Out
*/

	coin_counter_w( 0, ((data & 0x01) >> 0) );
	coin_lockout_w( 0, (((~data) & 0x10) >> 4) );
}


static WRITE16_HANDLER( mjyuugi_adpcm_bank_w )
{
/*
	---- xxxx : ADPCM Bank
	--xx ---- : GFX Bank
*/
	srmp2_adpcm_bank = (data & 0x0f);
	mjyuugi_gfx_bank = ((data >> 4) & 0x03);
}


static WRITE16_HANDLER( srmp2_adpcm_code_w )
{
/*
	- Received data may be playing ADPCM number.
	- 0x000000 - 0x0000ff and 0x010000 - 0x0100ff are offset table.
	- When the hardware receives the ADPCM number, it refers the offset
	  table and plays the ADPCM for itself.
*/

	unsigned char *ROM = memory_region(REGION_SOUND1);

	srmp2_adpcm_sptr = (ROM[((srmp2_adpcm_bank * 0x10000) + (data << 2) + 0)] << 8);
	srmp2_adpcm_eptr = (ROM[((srmp2_adpcm_bank * 0x10000) + (data << 2) + 1)] << 8);
	srmp2_adpcm_eptr  = (srmp2_adpcm_eptr - 1) & 0x0ffff;

	srmp2_adpcm_sptr += (srmp2_adpcm_bank * 0x10000);
	srmp2_adpcm_eptr += (srmp2_adpcm_bank * 0x10000);

	MSM5205_reset_w(0, 0);
	srmp2_adpcm_data = -1;
}


static WRITE_HANDLER( srmp3_adpcm_code_w )
{
/*
	- Received data may be playing ADPCM number.
	- 0x000000 - 0x0000ff and 0x010000 - 0x0100ff are offset table.
	- When the hardware receives the ADPCM number, it refers the offset
	  table and plays the ADPCM for itself.
*/

	unsigned char *ROM = memory_region(REGION_SOUND1);

	srmp2_adpcm_sptr = (ROM[((srmp2_adpcm_bank * 0x10000) + (data << 2) + 0)] << 8);
	srmp2_adpcm_eptr = (ROM[((srmp2_adpcm_bank * 0x10000) + (data << 2) + 1)] << 8);
	srmp2_adpcm_eptr  = (srmp2_adpcm_eptr - 1) & 0x0ffff;

	srmp2_adpcm_sptr += (srmp2_adpcm_bank * 0x10000);
	srmp2_adpcm_eptr += (srmp2_adpcm_bank * 0x10000);

	MSM5205_reset_w(0, 0);
	srmp2_adpcm_data = -1;
}


static void srmp2_adpcm_int(int num)
{
	unsigned char *ROM = memory_region(REGION_SOUND1);

	if (srmp2_adpcm_sptr)
	{
		if (srmp2_adpcm_data == -1)
		{
			srmp2_adpcm_data = ROM[srmp2_adpcm_sptr];

			if (srmp2_adpcm_sptr >= srmp2_adpcm_eptr)
			{
				MSM5205_reset_w(0, 1);
				srmp2_adpcm_data = 0;
				srmp2_adpcm_sptr = 0;
			}
			else
			{
				MSM5205_data_w(0, ((srmp2_adpcm_data >> 4) & 0x0f));
			}
		}
		else
		{
			MSM5205_data_w(0, ((srmp2_adpcm_data >> 0) & 0x0f));
			srmp2_adpcm_sptr++;
			srmp2_adpcm_data = -1;
		}
	}
	else
	{
		MSM5205_reset_w(0, 1);
	}
}


static READ16_HANDLER( srmp2_cchip_status_0_r )
{
	return 0x01;
}


static READ16_HANDLER( srmp2_cchip_status_1_r )
{
	return 0x01;
}


static READ16_HANDLER( srmp2_input_1_r )
{
/*
	---x xxxx : Key code
	--x- ---- : Player 1 and 2 side flag
*/

	if (!ACCESSING_LSB)
	{
		return 0xffff;
	}

	if (srmp2_port_select != 2)			/* Panel keys */
	{
		int i, j, t;

		for (i = 0x00 ; i < 0x20 ; i += 8)
		{
			j = (i / 0x08) + 3;

			for (t = 0 ; t < 8 ; t ++)
			{
				if (!(readinputport(j) & ( 1 << t )))
				{
					return (i + t);
				}
			}
		}
	}
	else								/* Analizer and memory reset keys */
	{
		return readinputport(7);
	}

	return 0xffff;
}


static READ16_HANDLER( srmp2_input_2_r )
{
	if (!ACCESSING_LSB)
	{
		return 0x0001;
	}

	/* Always return 1, otherwise freeze. Maybe read I/O status */
	return 0x0001;
}


static WRITE16_HANDLER( srmp2_input_1_w )
{
	if (data != 0x0000)
	{
		srmp2_port_select = 1;
	}
	else
	{
		srmp2_port_select = 0;
	}
}


static WRITE16_HANDLER( srmp2_input_2_w )
{
	if (data == 0x0000)
	{
		srmp2_port_select = 2;
	}
	else
	{
		srmp2_port_select = 0;
	}
}


static WRITE_HANDLER( srmp3_rombank_w )
{
/*
	---x xxxx : MAIN ROM bank
	xxx- ---- : ADPCM ROM bank
*/

	unsigned char *ROM = memory_region(REGION_CPU1);
	int addr;

	srmp2_adpcm_bank = ((data & 0xe0) >> 5);

	if (data & 0x1f) addr = ((0x10000 + (0x2000 * (data & 0x0f))) - 0x8000);
	else addr = 0x10000;

	cpu_setbank(1, &ROM[addr]);
}

/**************************************************************************

  Memory Map(s)

**************************************************************************/


static MEMORY_READ16_START( srmp2_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x0c0000, 0x0c3fff, MRA16_RAM },
	{ 0x140000, 0x143fff, MRA16_RAM },				/* Sprites Code + X + Attr */
	{ 0x180000, 0x180607, MRA16_RAM },				/* Sprites Y */
	{ 0x900000, 0x900001, input_port_0_word_r },	/* Coinage */
	{ 0xa00000, 0xa00001, srmp2_input_1_r },		/* I/O port 1 */
	{ 0xa00002, 0xa00003, srmp2_input_2_r },		/* I/O port 2 */
	{ 0xb00000, 0xb00001, srmp2_cchip_status_0_r },	/* Custom chip status ??? */
	{ 0xb00002, 0xb00003, srmp2_cchip_status_1_r },	/* Custom chip status ??? */
	{ 0xf00000, 0xf00001, AY8910_read_port_0_lsb_r },
MEMORY_END

static MEMORY_WRITE16_START( srmp2_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0c0000, 0x0c3fff, MWA16_RAM, &srmp2_nvram, &srmp2_nvram_size },
	{ 0x140000, 0x143fff, MWA16_RAM, &spriteram16_2 },	/* Sprites Code + X + Attr */
	{ 0x180000, 0x180609, MWA16_RAM, &spriteram16 },	/* Sprites Y */
	{ 0x1c0000, 0x1c0001, MWA16_NOP },					/* ??? */
	{ 0x800000, 0x800001, srmp2_flags_w },				/* ADPCM bank, Color bank, etc. */
	{ 0x900000, 0x900001, MWA16_NOP },					/* ??? */
	{ 0xa00000, 0xa00001, srmp2_input_1_w },			/* I/O ??? */
	{ 0xa00002, 0xa00003, srmp2_input_2_w },			/* I/O ??? */
	{ 0xb00000, 0xb00001, srmp2_adpcm_code_w },			/* ADPCM number */
	{ 0xc00000, 0xc00001, MWA16_NOP },					/* ??? */
	{ 0xd00000, 0xd00001, MWA16_NOP },					/* ??? */
	{ 0xe00000, 0xe00001, MWA16_NOP },					/* ??? */
	{ 0xf00000, 0xf00001, AY8910_control_port_0_lsb_w },
	{ 0xf00002, 0xf00003, AY8910_write_port_0_lsb_w },
MEMORY_END


static MEMORY_READ16_START( mjyuugi_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x100001, input_port_0_word_r },	/* Coinage */
	{ 0x100010, 0x100011, MRA16_NOP },				/* ??? */
	{ 0x200000, 0x200001, MRA16_NOP },				/* ??? */
	{ 0x300000, 0x300001, MRA16_NOP },				/* ??? */
	{ 0x500000, 0x500001, input_port_8_word_r },	/* DSW 3-1 */
	{ 0x500010, 0x500011, input_port_9_word_r },	/* DSW 3-2 */
	{ 0x700000, 0x7003ff, paletteram16_word_r },
	{ 0x800000, 0x800001, MRA16_NOP },				/* ??? */
	{ 0x900000, 0x900001, srmp2_input_1_r },		/* I/O port 1 */
	{ 0x900002, 0x900003, srmp2_input_2_r },		/* I/O port 2 */
	{ 0xa00000, 0xa00001, srmp2_cchip_status_0_r },	/* custom chip status ??? */
	{ 0xa00002, 0xa00003, srmp2_cchip_status_1_r },	/* custom chip status ??? */
	{ 0xb00000, 0xb00001, AY8910_read_port_0_lsb_r },
	{ 0xd00000, 0xd00609, MRA16_RAM },				/* Sprites Y */
	{ 0xd02000, 0xd023ff, MRA16_RAM },				/* ??? */
	{ 0xe00000, 0xe03fff, MRA16_RAM },				/* Sprites Code + X + Attr */
	{ 0xffc000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( mjyuugi_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x100001, mjyuugi_flags_w },			/* Coin Counter */
	{ 0x100010, 0x100011, mjyuugi_adpcm_bank_w },		/* ADPCM bank, GFX bank */
	{ 0x700000, 0x7003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0x900000, 0x900001, srmp2_input_1_w },			/* I/O ??? */
	{ 0x900002, 0x900003, srmp2_input_2_w },			/* I/O ??? */
	{ 0xa00000, 0xa00001, srmp2_adpcm_code_w },			/* ADPCM number */
	{ 0xb00000, 0xb00001, AY8910_control_port_0_lsb_w },
	{ 0xb00002, 0xb00003, AY8910_write_port_0_lsb_w },
	{ 0xc00000, 0xc00001, MWA16_NOP },					/* ??? */
	{ 0xd00000, 0xd00609, MWA16_RAM, &spriteram16 },	/* Sprites Y */
	{ 0xd02000, 0xd023ff, MWA16_RAM },					/* ??? only writes $00fa */
	{ 0xe00000, 0xe03fff, MWA16_RAM, &spriteram16_2 },	/* Sprites Code + X + Attr */
	{ 0xffc000, 0xffffff, MWA16_RAM, &srmp2_nvram, &srmp2_nvram_size },
MEMORY_END


static READ_HANDLER( srmp3_cchip_status_0_r )
{
	return 0x01;
}

static READ_HANDLER( srmp3_cchip_status_1_r )
{
	return 0x01;
}

static WRITE_HANDLER( srmp3_input_1_w )
{
/*
	---- --x- : Player 1 side flag ?
	---- -x-- : Player 2 side flag ?
*/

	logerror("PC:%04X DATA:%02X  srmp3_input_1_w\n", cpu_get_pc(), data);

	srmp2_port_select = 0;

	{
		static int qqq01 = 0;
		static int qqq02 = 0;
		static int qqq49 = 0;
		static int qqqzz = 0;

		if (data == 0x01) qqq01++;
		else if (data == 0x02) qqq02++;
		else if (data == 0x49) qqq49++;
		else qqqzz++;

//		usrintf_showmessage("%04X %04X %04X %04X", qqq01, qqq02, qqq49, qqqzz);
	}
}

static WRITE_HANDLER( srmp3_input_2_w )
{

	/* Key matrix reading related ? */

	logerror("PC:%04X DATA:%02X  srmp3_input_2_w\n", cpu_get_pc(), data);

	srmp2_port_select = 1;

}

static READ_HANDLER( srmp3_input_r )
{
/*
	---x xxxx : Key code
	--x- ---- : Player 1 and 2 side flag
*/

	/* Currently I/O port of srmp3 is fully understood. */

	int keydata = 0xff;

	logerror("PC:%04X          srmp3_input_r\n", cpu_get_pc());

	// PC:0x8903	ROM:0xC903
	// PC:0x7805	ROM:0x7805

	if ((cpu_get_pc() == 0x8903) || (cpu_get_pc() == 0x7805))	/* Panel keys */
	{
		int i, j, t;

		for (i = 0x00 ; i < 0x20 ; i += 8)
		{
			j = (i / 0x08) + 3;

			for (t = 0 ; t < 8 ; t ++)
			{
				if (!(readinputport(j) & ( 1 << t )))
				{
					keydata = (i + t);
				}
			}
		}
	}

	// PC:0x8926	ROM:0xC926
	// PC:0x7822	ROM:0x7822

	if ((cpu_get_pc() == 0x8926) || (cpu_get_pc() == 0x7822))	/* Analizer and memory reset keys */
	{
		keydata = readinputport(7);
	}

	return keydata;
}

static WRITE_HANDLER( srmp3_flags_w )
{
/*
	---- ---x : Coin Counter
	---x ---- : Coin Lock Out
	xx-- ---- : GFX Bank
*/

	coin_counter_w( 0, ((data & 0x01) >> 0) );
	coin_lockout_w( 0, (((~data) & 0x10) >> 4) );
	srmp3_gfx_bank = (data >> 6) & 0x03;
}


static MEMORY_READ_START( srmp3_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK1 },						/* rom bank */
	{ 0xa000, 0xa7ff, MRA_RAM },						/* work ram */
	{ 0xb000, 0xb303, MRA_RAM },						/* Sprites Y */
	{ 0xc000, 0xdfff, MRA_RAM },						/* Sprites Code + X + Attr */
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( srmp3_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x9fff, MWA_ROM },						/* rom bank */
	{ 0xa000, 0xa7ff, MWA_RAM, &srmp3_nvram, &srmp3_nvram_size },	/* work ram */
	{ 0xa800, 0xa800, MWA_NOP },						/* flag ? */
	{ 0xb000, 0xb303, MWA_RAM, &spriteram },			/* Sprites Y */
	{ 0xb800, 0xb800, MWA_NOP },						/* flag ? */
	{ 0xc000, 0xdfff, MWA_RAM, &spriteram_2 },			/* Sprites Code + X + Attr */
	{ 0xe000, 0xffff, MWA_RAM, &spriteram_3 },
MEMORY_END

static PORT_READ_START( srmp3_readport )
	{ 0x40, 0x40, input_port_0_r },						/* coin, service */
	{ 0xa1, 0xa1, srmp3_cchip_status_0_r },				/* custom chip status ??? */
	{ 0xc0, 0xc0, srmp3_input_r },						/* key matrix */
	{ 0xc1, 0xc1, srmp3_cchip_status_1_r },				/* custom chip status ??? */
	{ 0xe2, 0xe2, AY8910_read_port_0_r },
PORT_END

static PORT_WRITE_START( srmp3_writeport )
	{ 0x20, 0x20, IOWP_NOP },							/* elapsed interrupt signal */
	{ 0x40, 0x40, srmp3_flags_w },						/* GFX bank, counter, lockout */
	{ 0x60, 0x60, srmp3_rombank_w },					/* ROM bank select */
	{ 0xa0, 0xa0, srmp3_adpcm_code_w },					/* ADPCM number */
	{ 0xc0, 0xc0, srmp3_input_1_w },					/* I/O ??? */
	{ 0xc1, 0xc1, srmp3_input_2_w },					/* I/O ??? */
	{ 0xe0, 0xe0, AY8910_control_port_0_w },
	{ 0xe1, 0xe1, AY8910_write_port_0_w },
PORT_END


/***************************************************************************

  Input Port(s)

***************************************************************************/

#define SETAMJCTRL_PORT3 \
	PORT_START	/* KEY MATRIX INPUT (3) */ \
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "P1 Small",       KEYCODE_BACKSPACE, IP_JOY_NONE ) \
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "P1 Double Up",   KEYCODE_RSHIFT,    IP_JOY_NONE ) \
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "P1 Big",         KEYCODE_ENTER,     IP_JOY_NONE ) \
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, 0, "P1 Take Score",  KEYCODE_RCONTROL,  IP_JOY_NONE ) \
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, 0, "P1 Flip",        KEYCODE_X,         IP_JOY_NONE ) \
	PORT_BITX( 0x0040, IP_ACTIVE_LOW, 0, "P1 Last Chance", KEYCODE_RALT,      IP_JOY_NONE ) \
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define SETAMJCTRL_PORT4 \
	PORT_START	/* KEY MATRIX INPUT (4) */ \
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, 0, "P1 K",   KEYCODE_K,     IP_JOY_NONE ) \
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z,     IP_JOY_NONE ) \
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "P1 G",   KEYCODE_G,     IP_JOY_NONE ) \
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE ) \
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, 0, "P1 C",   KEYCODE_C,     IP_JOY_NONE ) \
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BITX( 0x0040, IP_ACTIVE_LOW, 0, "P1 L",   KEYCODE_L,     IP_JOY_NONE ) \
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define SETAMJCTRL_PORT5 \
	PORT_START	/* KEY MATRIX INPUT (5) */ \
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, 0, "P1 H",     KEYCODE_H,        IP_JOY_NONE ) \
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "P1 Pon",   KEYCODE_LALT,     IP_JOY_NONE ) \
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "P1 D",     KEYCODE_D,        IP_JOY_NONE ) \
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "P1 Start", KEYCODE_1,        IP_JOY_NONE ) \
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, 0, "P1 I",     KEYCODE_I,        IP_JOY_NONE ) \
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, 0, "P1 Kan",   KEYCODE_LCONTROL, IP_JOY_NONE ) \
	PORT_BITX( 0x0040, IP_ACTIVE_LOW, 0, "P1 E",     KEYCODE_E,        IP_JOY_NONE ) \
	PORT_BITX( 0x0080, IP_ACTIVE_LOW, 0, "P1 M",     KEYCODE_M,        IP_JOY_NONE ) \
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define SETAMJCTRL_PORT6 \
	PORT_START	/* KEY MATRIX INPUT (6) */ \
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, 0, "P1 A",     KEYCODE_A,      IP_JOY_NONE ) \
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "P1 Bet",   KEYCODE_2,      IP_JOY_NONE ) \
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "P1 J",     KEYCODE_J,      IP_JOY_NONE ) \
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE ) \
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, 0, "P1 F",     KEYCODE_F,      IP_JOY_NONE ) \
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, 0, "P1 N",     KEYCODE_N,      IP_JOY_NONE ) \
	PORT_BITX( 0x0040, IP_ACTIVE_LOW, 0, "P1 B",     KEYCODE_B,      IP_JOY_NONE ) \
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( srmp2 )
	PORT_START			/* Coinnage (0) */
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* DSW (1) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xe0, "1 (Easy)" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )

	PORT_START			/* DSW (2) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SETAMJCTRL_PORT3	/* INPUT1 (3) */
	SETAMJCTRL_PORT4	/* INPUT1 (4) */
	SETAMJCTRL_PORT5	/* INPUT1 (5) */
	SETAMJCTRL_PORT6	/* INPUT1 (6) */

	PORT_START			/* INPUT1 (7) */
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( srmp3 )
	PORT_START			/* Coinnage (0) */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* DSW (1) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX   ( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Open Reach of CPU" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xe0, "1 (Easy)" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )

	PORT_START			/* DSW (2) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SETAMJCTRL_PORT3	/* INPUT1 (3) */
	SETAMJCTRL_PORT4	/* INPUT1 (4) */
	SETAMJCTRL_PORT5	/* INPUT1 (5) */
	SETAMJCTRL_PORT6	/* INPUT1 (6) */

	PORT_START			/* INPUT1 (7) */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( mjyuugi )
	PORT_START			/* Coinnage (0) */
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* DSW (1) */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Gal Score" )
	PORT_DIPSETTING(    0x10, "+0" )
	PORT_DIPSETTING(    0x00, "+1000" )
	PORT_DIPNAME( 0x20, 0x20, "Player Score" )
	PORT_DIPSETTING(    0x20, "+0" )
	PORT_DIPSETTING(    0x00, "+1000" )
	PORT_DIPNAME( 0x40, 0x40, "Item price initialize ?" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* DSW (2) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SETAMJCTRL_PORT3	/* INPUT1 (3) */
	SETAMJCTRL_PORT4	/* INPUT1 (4) */
	SETAMJCTRL_PORT5	/* INPUT1 (5) */
	SETAMJCTRL_PORT6	/* INPUT1 (6) */

	PORT_START			/* INPUT1 (7) */
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BITX( 0x0004, 0x0004, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* DSW (3-1) [Debug switch] */
	PORT_BITX( 0x0001, 0x0001, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug  0", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0002, 0x0002, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug  1", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0004, 0x0004, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug  2", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0008, 0x0008, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug  3", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0010, 0x0010, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug  4", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0020, 0x0020, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug  5", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0040, 0x0040, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug  6", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0080, 0x0080, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug  7", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START			/* DSW (3-2) [Debug switch] */
	PORT_BITX( 0x0001, 0x0001, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug  8", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0002, 0x0002, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug  9", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0004, 0x0004, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug 10", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0008, 0x0008, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug 11", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0010, 0x0010, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug 12", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0020, 0x0020, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug 13", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0040, 0x0040, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug 14", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BITX( 0x0080, 0x0080, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug 15", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************

  Machine Driver(s)

***************************************************************************/

static struct AY8910interface srmp2_ay8910_interface =
{
	1,
	20000000/16,					/* 1.25 MHz */
	{ 40 },
	{ input_port_2_r },				/* Input A: DSW 2 */
	{ input_port_1_r },				/* Input B: DSW 1 */
	{ 0 },
	{ 0 }
};

static struct AY8910interface srmp3_ay8910_interface =
{
	1,
	16000000/16,					/* 1.00 MHz */
	{ 20 },
	{ input_port_2_r },				/* Input A: DSW 2 */
	{ input_port_1_r },				/* Input B: DSW 1 */
	{ 0 },
	{ 0 }
};

static struct AY8910interface mjyuugi_ay8910_interface =
{
	1,
	16000000/16,					/* 1.00 MHz */
	{ 20 },
	{ input_port_2_r },				/* Input A: DSW 2 */
	{ input_port_1_r },				/* Input B: DSW 1 */
	{ 0 },
	{ 0 }
};


struct MSM5205interface srmp2_msm5205_interface =
{
	1,
	384000,
	{ srmp2_adpcm_int },			/* IRQ handler */
	{ MSM5205_S48_4B },				/* 8 KHz, 4 Bits  */
	{ 45 }
};

struct MSM5205interface srmp3_msm5205_interface =
{
	1,
//	455000,							/* 455 KHz */
	384000,							/* 384 KHz */
	{ srmp2_adpcm_int },			/* IRQ handler */
	{ MSM5205_S48_4B },				/* 8 KHz, 4 Bits  */
	{ 45 }
};


static struct GfxLayout charlayout =
{
	16, 16,
	RGN_FRAC(1, 2),
	4,
	{ RGN_FRAC(1, 2) + 8, RGN_FRAC(1, 2) + 0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 128, 129, 130, 131, 132, 133, 134, 135 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	16*16*2
};

static struct GfxDecodeInfo srmp2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 64 },
	{ -1 }
};

static struct GfxDecodeInfo srmp3_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 32 },
	{ -1 }
};


static struct MachineDriver machine_driver_srmp2 =
{
	{
		{
			CPU_M68000,
			16000000/2,				/* 8.00 MHz */
			srmp2_readmem, srmp2_writemem, 0, 0,
			srmp2_interrupt, 16		/* Interrupt times is not understood */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	srmp2_init_machine,

	/* video hardware */
	464, 256-16, { 16, 464-1, 8, 256-1-24 },
	srmp2_gfxdecodeinfo,
	1024, 1024,						/* sprites only */
	srmp2_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	0,								/* no need for a vh_start: no tilemaps */
	0,
	srmp2_vh_screenrefresh,			/* just draw the sprites */

	/* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_AY8910, &srmp2_ay8910_interface },
		{ SOUND_MSM5205, &srmp2_msm5205_interface }
	},
	srmp2_nvram_handler
};


static struct MachineDriver machine_driver_srmp3 =
{
	{
		{
			CPU_Z80,
			3500000,				/* 3.50 MHz ? */
	//		4000000,				/* 4.00 MHz ? */
			srmp3_readmem, srmp3_writemem, srmp3_readport, srmp3_writeport,
	//		interrupt, 1
			srmp3_interrupt, 1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	srmp3_init_machine,

	/* video hardware */
	400, 256-16, { 16, 400-1, 8, 256-1-24 },
	srmp3_gfxdecodeinfo,
	512, 0,	/* sprites only */
	srmp3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	0,								/* no need for a vh_start: no tilemaps */
	0,
	srmp3_vh_screenrefresh,			/* just draw the sprites */

	/* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_AY8910, &srmp3_ay8910_interface },
		{ SOUND_MSM5205, &srmp3_msm5205_interface }
	},
	srmp3_nvram_handler
};


static struct MachineDriver machine_driver_mjyuugi =
{
	{
		{
			CPU_M68000,
			16000000/2,				/* 8.00 MHz */
			mjyuugi_readmem, mjyuugi_writemem, 0, 0,
			srmp2_interrupt, 16		/* Interrupt times is not understood */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	srmp2_init_machine,

	/* video hardware */
	400, 256-16, { 16, 400-1, 0, 256-1-16 },
	srmp3_gfxdecodeinfo,
	512, 0,						/* sprites only */
	0,

	VIDEO_TYPE_RASTER,
	0,
	0,								/* no need for a vh_start: no tilemaps */
	0,
	mjyuugi_vh_screenrefresh,		/* just draw the sprites */

	/* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_AY8910, &mjyuugi_ay8910_interface },
		{ SOUND_MSM5205, &srmp2_msm5205_interface }
	},
	srmp2_nvram_handler
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( srmp2 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD16_BYTE( "uco-2.17", 0x000000, 0x020000, 0x0d6c131f )
	ROM_LOAD16_BYTE( "uco-3.18", 0x000001, 0x020000, 0xe9fdf5f8 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD       ( "ubo-4.60",  0x000000, 0x040000, 0xcb6f7cce )
	ROM_LOAD       ( "ubo-5.61",  0x040000, 0x040000, 0x7b48c540 )
	ROM_LOAD16_BYTE( "uco-8.64",  0x080000, 0x040000, 0x1ca1c7c9 )
	ROM_LOAD16_BYTE( "uco-9.65",  0x080001, 0x040000, 0xef75471b )
	ROM_LOAD       ( "ubo-6.62",  0x100000, 0x040000, 0x6c891ac5 )
	ROM_LOAD       ( "ubo-7.63",  0x140000, 0x040000, 0x60a45755 )
	ROM_LOAD16_BYTE( "uco-10.66", 0x180000, 0x040000, 0xcb6bd857 )
	ROM_LOAD16_BYTE( "uco-11.67", 0x180001, 0x040000, 0x199f79c0 )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "uco-1.19", 0x000000, 0x020000, 0xf284af8e )

	ROM_REGION( 0x000800, REGION_PROMS, 0 )					/* Color PROMs */
	ROM_LOAD( "uc-1o.12", 0x000000, 0x000400, 0xfa59b5cb )
	ROM_LOAD( "uc-2o.13", 0x000400, 0x000400, 0x50a33b96 )
ROM_END

ROM_START( srmp3 )
	ROM_REGION( 0x028000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD( "za0-10.bin", 0x000000, 0x008000, 0x939d126f )
	ROM_CONTINUE(           0x010000, 0x018000 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "za0-02.bin", 0x000000, 0x080000, 0x85691946 )
	ROM_LOAD16_BYTE( "za0-04.bin", 0x000001, 0x080000, 0xc06e7a96 )
	ROM_LOAD16_BYTE( "za0-01.bin", 0x100000, 0x080000, 0x95e0d87c )
	ROM_LOAD16_BYTE( "za0-03.bin", 0x100001, 0x080000, 0x7c98570e )
	ROM_LOAD16_BYTE( "za0-06.bin", 0x200000, 0x080000, 0x8b874b0a )
	ROM_LOAD16_BYTE( "za0-08.bin", 0x200001, 0x080000, 0x3de89d88 )
	ROM_LOAD16_BYTE( "za0-05.bin", 0x300000, 0x080000, 0x80d3b4e6 )
	ROM_LOAD16_BYTE( "za0-07.bin", 0x300001, 0x080000, 0x39d15129 )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "za0-11.bin", 0x000000, 0x080000, 0x2248c23f )

	ROM_REGION( 0x000400, REGION_PROMS, 0 )					/* Color PROMs */
	ROM_LOAD( "za0-12.prm", 0x000000, 0x000200, 0x1ac5387c )
	ROM_LOAD( "za0-13.prm", 0x000200, 0x000200, 0x4ea3d2fe )
ROM_END

ROM_START( mjyuugi )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD16_BYTE( "um001.001", 0x000000, 0x020000, 0x28d5340f )
	ROM_LOAD16_BYTE( "um001.003", 0x000001, 0x020000, 0x275197de )
	ROM_LOAD16_BYTE( "um001.002", 0x040000, 0x020000, 0xd5dd4710 )
	ROM_LOAD16_BYTE( "um001.004", 0x040001, 0x020000, 0xc5ddb567 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "maj-001.10",  0x000000, 0x080000, 0x3c08942a )
	ROM_LOAD16_BYTE( "maj-001.08",  0x000001, 0x080000, 0xe2444311 )
	ROM_LOAD16_BYTE( "maj-001.09",  0x100000, 0x080000, 0xa1974860 )
	ROM_LOAD16_BYTE( "maj-001.07",  0x100001, 0x080000, 0xb1f1d118 )
	ROM_LOAD16_BYTE( "maj-001.06",  0x200000, 0x080000, 0x4c60acdd )
	ROM_LOAD16_BYTE( "maj-001.04",  0x200001, 0x080000, 0x0a4b2de1 )
	ROM_LOAD16_BYTE( "maj-001.05",  0x300000, 0x080000, 0x6be7047a )
	ROM_LOAD16_BYTE( "maj-001.03",  0x300001, 0x080000, 0xc4fb6ea0 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "maj-001.01", 0x000000, 0x080000, 0x029a0b60 )
	ROM_LOAD( "maj-001.02", 0x080000, 0x080000, 0xeb28e641 )
ROM_END

ROM_START( mjyuugia )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD16_BYTE( "um_001.001", 0x000000, 0x020000, 0x76dc0594 )
	ROM_LOAD16_BYTE( "um001.003",  0x000001, 0x020000, 0x275197de )
	ROM_LOAD16_BYTE( "um001.002",  0x040000, 0x020000, 0xd5dd4710 )
	ROM_LOAD16_BYTE( "um001.004",  0x040001, 0x020000, 0xc5ddb567 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "maj-001.10", 0x000000, 0x080000, 0x3c08942a )
	ROM_LOAD16_BYTE( "maj-001.08", 0x000001, 0x080000, 0xe2444311 )
	ROM_LOAD16_BYTE( "maj-001.09", 0x100000, 0x080000, 0xa1974860 )
	ROM_LOAD16_BYTE( "maj-001.07", 0x100001, 0x080000, 0xb1f1d118 )
	ROM_LOAD16_BYTE( "maj-001.06", 0x200000, 0x080000, 0x4c60acdd )
	ROM_LOAD16_BYTE( "maj-001.04", 0x200001, 0x080000, 0x0a4b2de1 )
	ROM_LOAD16_BYTE( "maj-001.05", 0x300000, 0x080000, 0x6be7047a )
	ROM_LOAD16_BYTE( "maj-001.03", 0x300001, 0x080000, 0xc4fb6ea0 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "maj-001.01", 0x000000, 0x080000, 0x029a0b60 )
	ROM_LOAD( "maj-001.02", 0x080000, 0x080000, 0xeb28e641 )
ROM_END


GAME( 1987, srmp2,     0,        srmp2,    srmp2,    srmp2,   ROT0, "Seta", "Super Real Mahjong Part 2 (Japan)" )
GAME( 1988, srmp3,     0,        srmp3,    srmp3,    srmp3,   ROT0, "Seta", "Super Real Mahjong Part 3 (Japan)" )
GAME( 1990, mjyuugi,   0,        mjyuugi,  mjyuugi,  mjyuugi, ROT0, "Visco", "Mahjong Yuugi (Japan set 1)" )
GAME( 1990, mjyuugia,  mjyuugi,  mjyuugi,  mjyuugi,  mjyuugi, ROT0, "Visco", "Mahjong Yuugi (Japan set 2)" )
