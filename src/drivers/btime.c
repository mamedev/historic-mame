/***************************************************************************

Burger Time hardware description:

Actually Lock'n'Chase is (C)1981 while Burger Time is (C)1982, so it might
be more accurate to say 'Lock'n'Chase hardware'.

This hardware is pretty straightforward, but has a couple of interesting
twists. There are two ports to the video and color RAMs, one normal access,
and one with X and Y coordinates swapped. The sprite RAM occupies the
first row of the swapped area, so it appears in the regular video RAM as
the first column of on the left side.

These games don't have VBLANK interrupts, but instead an IRQ or NMI
(depending on the particular board) is generated when a coin is inserted.

Some of the games also have a background playfield which, in the
case of Bump 'n' Jump and Zoar, can be scrolled vertically.

These boards use two 8910's for sound, controlled by a dedicated 6502 (except
in Eggs, where the main processor handles the sound). The main processor
triggers an IRQ request when writing a command to the sound CPU.

Main clock: XTAL = 12 MHz
Horizontal video frequency: HSYNC = XTAL/768?? = 15.625 kHz ??
Video frequency: VSYNC = HSYNC/272 = 57.44 Hz ?
VBlank duration: 1/VSYNC * (24/272) = 1536 us ?


Note on Lock'n'Chase:

The watchdog test prints "WATCHDOG TEST ER". Just by looking at the code,
I can't see how it could print anything else, there is only one path it
can take. Should the game reset????

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6502/M6502.h"

extern unsigned char *lnc_charbank;
extern unsigned char *bnj_backgroundram;
extern int bnj_backgroundram_size;
extern unsigned char *zoar_scrollram;

void btime_vh_convert_color_prom (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void lnc_vh_convert_color_prom (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void zoar_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

void lnc_init_machine (void);

int  bnj_vh_start (void);

void bnj_vh_stop (void);

void btime_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void bnj_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void lnc_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void eggs_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void zoar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void btime_paletteram_w(int offset,int data);
void bnj_background_w(int offset, int data);
void bnj_scroll1_w(int offset, int data);
void bnj_scroll2_w(int offset, int data);
int  btime_mirrorvideoram_r(int offset);
void btime_mirrorvideoram_w(int offset,int data);
int  btime_mirrorcolorram_r(int offset);
void btime_mirrorcolorram_w(int offset,int data);
void lnc_videoram_w(int offset,int data);
void lnc_mirrorvideoram_w(int offset,int data);
void zoar_video_control_w(int offset,int data);
void btime_video_control_w(int offset,int data);
void bnj_video_control_w(int offset,int data);
void lnc_video_control_w(int offset,int data);

int lnc_sound_interrupt(void);

static void sound_command_w(int offset,int data);

static void btime_decrypt(void)
{
	int A,A1;
	extern unsigned char *RAM;


	/* the encryption is a simple bit rotation: 76543210 -> 65342710, but */
	/* with a catch: it is only applied if the previous instruction did a */
	/* memory write. Also, only opcodes at addresses with this bit pattern: */
	/* xxxx xxx1 xxxx x1xx are encrypted. */

	/* get the address of the next opcode */
	A = cpu_getpc();

	/* however if the previous instruction was JSR (which caused a write to */
	/* the stack), fetch the address of the next instruction. */
	A1 = cpu_getpreviouspc();
	if (ROM[A1] == 0x20)	/* JSR $xxxx */
		A = cpu_readop_arg(A1+1) + 256 * cpu_readop_arg(A1+2);

	/* If the address of the next instruction is xxxx xxx1 xxxx x1xx, decode it. */
	if ((A & 0x0104) == 0x0104)
	{
		/* 76543210 -> 65342710 bit rotation */
		ROM[A] = (RAM[A] & 0x13) | ((RAM[A] & 0x80) >> 5) | ((RAM[A] & 0x64) << 1)
			   | ((RAM[A] & 0x08) << 2);
	}
}

static void btime_ram_w(int offset,int data)
{
	extern unsigned char *RAM;


	if (offset <= 0x07ff)
		RAM[offset] = data;
	else if (offset >= 0x0c00 && offset <= 0x0c0f)
		btime_paletteram_w(offset - 0x0c00,data);
	else if (offset >= 0x1000 && offset <= 0x13ff)
		videoram_w(offset - 0x1000,data);
	else if (offset >= 0x1400 && offset <= 0x17ff)
		colorram_w(offset - 0x1400,data);
	else if (offset >= 0x1800 && offset <= 0x1bff)
		btime_mirrorvideoram_w(offset - 0x1800,data);
	else if (offset >= 0x1c00 && offset <= 0x1fff)
		btime_mirrorcolorram_w(offset - 0x1c00,data);
	else if (offset == 0x4002)
		btime_video_control_w(0,data);
	else if (offset == 0x4003)
		sound_command_w(0,data);
	else if (offset == 0x4004)
		bnj_scroll1_w(0,data);

	btime_decrypt();
}

static void zoar_ram_w(int offset,int data)
{
	int good = 0;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (offset <= 0x07ff)
		{ RAM[offset] = data; good = 1; }
	else if (offset >= 0x8000 && offset <= 0x83ff)
		{ videoram_w(offset - 0x8000,data);	good = 1; }
	else if (offset >= 0x8400 && offset <= 0x87ff)
		{ colorram_w(offset - 0x8400,data); good = 1; }
	else if (offset >= 0x8800 && offset <= 0x8bff)
		{ btime_mirrorvideoram_w(offset - 0x8800,data); good = 1; }
	else if (offset >= 0x8c00 && offset <= 0x8fff)
		{ btime_mirrorcolorram_w(offset - 0x8c00,data); good = 1; }
	else if (offset == 0x9000)
		{ zoar_video_control_w(0, data); good = 1; }
	else if (offset >= 0x9800 && offset <= 0x9803)
		{ zoar_scrollram[offset - 0x9800] = data; good = 1; }
	else if (offset == 0x9804)
		{ bnj_scroll2_w(0,data); good = 1; }
	else if (offset == 0x9805)
		{ bnj_scroll1_w(0,data); good = 1; }
	else if (offset == 0x9806)
		{ sound_command_w(0,data); good = 1; }

	btime_decrypt();

	if (!good && errorlog)
	{
		fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to unmapped memory address %04x\n",cpu_getactivecpu(),cpu_getpc(),data,offset);
	}
}


static struct MemoryReadAddress btime_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x17ff, MRA_RAM },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_r },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_r },
	{ 0x4000, 0x4000, input_port_0_r },     /* IN0 */
	{ 0x4001, 0x4001, input_port_1_r },     /* IN1 */
	{ 0x4002, 0x4002, input_port_2_r },     /* coin */
	{ 0x4003, 0x4003, input_port_3_r },     /* DSW1 */
	{ 0x4004, 0x4004, input_port_4_r },     /* DSW2 */
	{ 0xb000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress btime_writemem[] =
{
	{ 0x0000, 0xffff, btime_ram_w },	/* override the following entries to */
										/* support ROM decryption */
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0c00, 0x0c0f, btime_paletteram_w, &paletteram },
	{ 0x1000, 0x13ff, videoram_w, &videoram, &videoram_size },
	{ 0x1400, 0x17ff, colorram_w, &colorram },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_w },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_w },
	{ 0x4000, 0x4000, MWA_NOP },
	{ 0x4002, 0x4002, btime_video_control_w },
	{ 0x4003, 0x4003, sound_command_w },
	{ 0x4004, 0x4004, bnj_scroll1_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress zoar_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x9800, 0x9800, input_port_3_r },     /* DSW 1 */
	{ 0x9801, 0x9801, input_port_4_r },     /* DSW 2 */
	{ 0x9802, 0x9802, input_port_0_r },     /* IN 0 */
	{ 0x9803, 0x9803, input_port_1_r },     /* IN 1 */
	{ 0x9804, 0x9804, input_port_2_r },     /* coin */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress zoar_writemem[] =
{
	{ 0x0000, 0xffff, zoar_ram_w },	/* override the following entries to */
									/* support ROM decryption */
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, colorram_w, &colorram },
	{ 0x8800, 0x8bff, btime_mirrorvideoram_w },
	{ 0x8c00, 0x8fff, btime_mirrorcolorram_w },
	{ 0x9000, 0x9000, zoar_video_control_w },
	{ 0x9800, 0x9803, MWA_RAM, &zoar_scrollram },
	{ 0x9805, 0x9805, bnj_scroll2_w },
	{ 0x9805, 0x9805, bnj_scroll1_w },
	{ 0x9806, 0x9806, sound_command_w },
  //{ 0x9807, 0x9807, MWA_RAM },  // Marked as ACK on schematics (Board 2 Pg 5)
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress eggs_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x17ff, MRA_RAM },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_r },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_r },
	{ 0x2000, 0x2000, input_port_2_r },     /* DSW1 */
	{ 0x2001, 0x2001, input_port_3_r },     /* DSW2 */
	{ 0x2002, 0x2002, input_port_0_r },     /* IN0 */
	{ 0x2003, 0x2003, input_port_1_r },     /* IN1 */
	{ 0x3000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },    /* reset/interrupt vectors */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress eggs_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1000, 0x13ff, videoram_w, &videoram, &videoram_size },
	{ 0x1400, 0x17ff, colorram_w, &colorram },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_w },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_w },
	{ 0x2000, 0x2000, btime_video_control_w },
	{ 0x2001, 0x2001, MWA_NOP },
	{ 0x2004, 0x2004, AY8910_control_port_0_w },
	{ 0x2005, 0x2005, AY8910_write_port_0_w },
	{ 0x2006, 0x2006, AY8910_control_port_1_w },
	{ 0x2007, 0x2007, AY8910_write_port_1_w },
	{ 0x3000, 0x7fff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress lnc_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_RAM },
	{ 0x7c00, 0x7fff, btime_mirrorvideoram_r },
	{ 0x8000, 0x8000, input_port_3_r },     /* DSW1 */
	{ 0x8001, 0x8001, input_port_4_r },     /* DSW2 */
	{ 0x9000, 0x9000, input_port_0_r },     /* IN0 */
	{ 0x9001, 0x9001, input_port_1_r },     /* IN1 */
	{ 0x9002, 0x9002, input_port_2_r },     /* coin */
	{ 0xc000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress lnc_writemem[] =
{
	{ 0x0000, 0x3bff, MWA_RAM },
	{ 0x3c00, 0x3fff, lnc_videoram_w, &videoram, &videoram_size },
	{ 0x7800, 0x7bff, MWA_RAM, &colorram },  // Dummy
	{ 0x7c00, 0x7fff, lnc_mirrorvideoram_w },
	{ 0x8000, 0x8000, MWA_NOP },            /* ??? */
	{ 0x8001, 0x8001, lnc_video_control_w },
	{ 0x8003, 0x8003, MWA_RAM, &lnc_charbank },
	{ 0x9000, 0x9000, MWA_NOP },            /* ??? */
	{ 0x9002, 0x9002, sound_command_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress bnj_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x1000, input_port_3_r },     /* DSW1 */
	{ 0x1001, 0x1001, input_port_4_r },     /* DSW2 */
	{ 0x1002, 0x1002, input_port_0_r },     /* IN0 */
	{ 0x1003, 0x1003, input_port_1_r },     /* IN1 */
	{ 0x1004, 0x1004, input_port_2_r },     /* coin */
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x4800, 0x4bff, btime_mirrorvideoram_r },
	{ 0x4c00, 0x4fff, btime_mirrorcolorram_r },
	{ 0xa000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress bnj_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1001, 0x1001, bnj_video_control_w },
	{ 0x1002, 0x1002, sound_command_w },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, colorram_w, &colorram },
	{ 0x4800, 0x4bff, btime_mirrorvideoram_w },
	{ 0x4c00, 0x4fff, btime_mirrorcolorram_w },
	{ 0x5000, 0x51ff, bnj_background_w, &bnj_backgroundram, &bnj_backgroundram_size },
	{ 0x5400, 0x5400, bnj_scroll1_w },
	{ 0x5800, 0x5800, bnj_scroll2_w },
	{ 0x5c00, 0x5c0f, btime_paletteram_w, &paletteram },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ 0xa000, 0xafff, soundlatch_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2fff, AY8910_write_port_0_w },
	{ 0x4000, 0x4fff, AY8910_control_port_0_w },
	{ 0x6000, 0x6fff, AY8910_write_port_1_w },
	{ 0x8000, 0x8fff, AY8910_control_port_1_w },
	{ 0xc000, 0xcfff, interrupt_enable_w },
	{ -1 }  /* end of table */
};



/***************************************************************************

These games don't have VBlank interrupts.
Interrupts are still used by the game, coin insertion generates an IRQ.

***************************************************************************/
static int btime_interrupt(int(*generated_interrupt)(void), int active_high)
{
	static int coin;
	int port;

	port = readinputport(2) & 0xc0;
	if (active_high) port ^= 0xc0;

	if (port != 0xc0)    /* Coin */
	{
		if (coin == 0)
		{
			coin = 1;
			return generated_interrupt();
		}
	}
	else coin = 0;

	return ignore_interrupt();
}

static int btime_irq_interrupt(void)
{
	return btime_interrupt(interrupt, 1);
}

static int zoar_irq_interrupt(void)
{
	return btime_interrupt(interrupt, 0);
}

static int btime_nmi_interrupt(void)
{
	return btime_interrupt(nmi_interrupt, 0);
}

static void sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,INT_IRQ);
}


INPUT_PORTS_START( btime_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH,IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH,IPT_COIN2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Cross Hatch Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "10,000" )
	PORT_DIPSETTING(    0x04, "15,000" )
	PORT_DIPSETTING(    0x02, "20,000"  )
	PORT_DIPSETTING(    0x00, "30,000"  )
	PORT_DIPNAME( 0x08, 0x08, "Enemies", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x10, 0x10, "End of Level Pepper", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
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


INPUT_PORTS_START( zoar_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credits" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )   /* almost certainly unused */
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	// Service mode doesn't work because of missing ROMs
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK  )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "5,000" )
	PORT_DIPSETTING(    0x04, "10,000" )
	PORT_DIPSETTING(    0x02, "15,000"  )
	PORT_DIPSETTING(    0x00, "20,000"  )
	PORT_DIPNAME( 0x08, 0x08, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x10, 0x00, "Weapon Select", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Manual" )
	PORT_DIPSETTING(    0x10, "Auto" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )  // These 3 switches
	PORT_DIPSETTING(    0x00, "Off" )					// have to do with
	PORT_DIPSETTING(    0x20, "On" )					// coinage.
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )	// See code at $d234.
	PORT_DIPSETTING(    0x00, "Off" )					// Feel free to figure
	PORT_DIPSETTING(    0x40, "On" )					// them out.
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END


INPUT_PORTS_START( eggs_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
	PORT_BIT( 0x30, 0x30, IPT_UNKNOWN )     /* almost certainly unused */
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x04, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "30,000" )
	PORT_DIPSETTING(    0x02, "50,000" )
	PORT_DIPSETTING(    0x00, "70,000"  )
	PORT_DIPSETTING(    0x06, "Never"  )
	PORT_BIT( 0x78, 0x78, IPT_UNKNOWN )     /* almost certainly unused */
	PORT_DIPNAME( 0x80, 0x80, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
INPUT_PORTS_END


INPUT_PORTS_START( lnc_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x30, 0x30, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Off" )
	PORT_DIPSETTING(    0x00, "RAM Test Only" )
	PORT_DIPSETTING(    0x20, "Watchdog Test Only" )
	PORT_DIPSETTING(    0x10, "All Tests" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "15,000" )
	PORT_DIPSETTING(    0x04, "20,000" )
	PORT_DIPSETTING(    0x02, "30,000" )
	PORT_DIPSETTING(    0x00, "Never" )
	PORT_DIPNAME( 0x08, 0x08, "Game Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Slow" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )	 // According to the manual
INPUT_PORTS_END


INPUT_PORTS_START( bnj_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_BITX(      0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START      /* DSW2 */
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "Every 30,000" )
	PORT_DIPSETTING(    0x04, "Every 70,000" )
	PORT_DIPSETTING(    0x02, "20,000 Only"  )
	PORT_DIPSETTING(    0x00, "30,000 Only"  )
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x10, 0x10, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
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



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	3,      /* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },    /* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	3,      /* 3 bits per pixel */
	{ 0, 256*16*16, 2*256*16*16 },  /* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
	  16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout zoar_spritelayout =
{
	16,16,  /* 16*16 sprites */
	128,    /* 256 sprites */
	3,      /* 3 bits per pixel */
	{ 0, 128*16*16, 2*128*16*16 },  /* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
	  16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout btime_tilelayout =
{
	16,16,  /* 16*16 characters */
	64,    /* 64 characters */
	3,      /* 3 bits per pixel */
	{ 0, 64*16*16, 64*2*16*16 },    /* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
	  16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8    /* every tile takes 32 consecutive bytes */
};

static struct GfxLayout bnj_tilelayout =
{
	16,16,  /* 16*16 characters */
	64, /* 64 characters */
	3,  /* 3 bits per pixel */
	{ 2*64*16*16+4, 0, 4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  2*16*8+3, 2*16*8+2, 2*16*8+1, 2*16*8+0, 3*16*8+3, 3*16*8+2, 3*16*8+1, 3*16*8+0 },
	64*8    /* every tile takes 64 consecutive bytes */
};

static struct GfxDecodeInfo btime_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 1 }, /* char set #1 */
	{ 1, 0x0000, &spritelayout,     0, 1 }, /* sprites */
	{ 1, 0x6000, &btime_tilelayout, 8, 1 }, /* background tiles */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo lnc_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },     /* char set #1 */
	{ 1, 0x0000, &spritelayout, 0, 1 },     /* sprites */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo bnj_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 1 }, /* char set #1 */
	{ 1, 0x0000, &spritelayout,     0, 1 }, /* sprites */
	{ 1, 0x6000, &bnj_tilelayout,   8, 1 }, /* background tiles */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo zoar_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 8 }, /* char set #1 */
	{ 1, 0x7800, &zoar_spritelayout,0, 8 }, /* sprites */
	{ 1, 0x6000, &btime_tilelayout, 0, 8 }, /* background tiles */
	{ -1 } /* end of array */
};


/* The original Burger Time has color RAM, but the bootleg uses a PROM. */
static unsigned char hamburge_color_prom[] =
{
	0x00,0xFF,0x2F,0x3F,0x07,0x38,0x1E,0x2B,0x00,0xAD,0xF8,0xC0,0xFF,0x07,0x3F,0xC7,
	0x00,0xFF,0x2F,0x3F,0x07,0x38,0x1E,0x2B,0x00,0xAD,0xF8,0xC0,0xFF,0x07,0x3F,0xC7
};

static unsigned char eggs_color_prom[] =
{
	/* screggco.c6 */
	0x21,0x07,0x00,0x3F,0xC0,0x26,0x91,0xFF,0x21,0x07,0x00,0x3F,0x00,0x26,0x00,0xFF,
	0x21,0x07,0x00,0x3F,0xC0,0x26,0x91,0xFF,0x21,0x07,0x00,0x3F,0xC0,0x26,0x91,0xFF,
};


static unsigned char zoar_color_prom[] =
{
	// z20-1l
	0x00,0x2c,0x21,0x6d,0x25,0x0b,0xc0,0x80,0x00,0x07,0x30,0x3f,0x00,0x25,0xb6,0xff,
	0x00,0x26,0x22,0x6d,0x1d,0x0a,0x80,0x80,0x00,0x07,0x30,0x3f,0x00,0x25,0xb6,0xff,

	// z21-2l
	0xff,0x42,0x6b,0x2c,0x14,0x01,0x40,0x40,0x00,0x07,0x30,0x3f,0x00,0x25,0xb6,0xff,
	0xff,0x60,0x75,0x2c,0x14,0x00,0x40,0x40,0x00,0x07,0x30,0x3f,0x00,0x25,0xb6,0xff
};

static unsigned char lnc_color_prom[] =
{
	/* palette SC-5M */
    0x00,0xdf,0x51,0x1c,0xa7,0xe0,0xfc,0xff

	/* PROM SB-4C. This PROM is in the RAS/CAS logic. Nothing to do with
	   colors, but I'm leaving it here for documentation purposed
	0xf7,0xf7,0xf5,0xfd,0xf9,0xf9,0xf0,0xf6,0xf6,0xf6,0xf6,0xf4,0xfc,0xf8,0xf8,0xf6,
	0xf6,0xf6,0xf4,0xfc,0xf8,0xf8,0xf6,0xf6,0xf6,0xf6,0xf6,0xf4,0xfc,0xf8,0xf8,0xf6 */
};


static struct AY8910interface ay8910_interface =
{
	2,      /* 2 chips */
	1500000,        /* 1.5 MHz ? (hand tuned) */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

/********************************************************************
*
*  Machine Driver macro
*  ====================
*
*  Abusing the pre-processor.
*
********************************************************************/

#define bnj_vh_convert_color_prom    0
#define eggs_vh_convert_color_prom   btime_vh_convert_color_prom
#define zoar_vh_convert_color_prom   btime_vh_convert_color_prom

#define btime_init_machine  0
#define bnj_init_machine    0
#define zoar_init_machine   0

#define btime_vh_start  generic_vh_start
#define zoar_vh_start   generic_vh_start
#define eggs_vh_start   generic_vh_start
#define lnc_vh_start    generic_vh_start

#define btime_vh_stop   generic_vh_stop
#define zoar_vh_stop    generic_vh_stop
#define eggs_vh_stop    generic_vh_stop
#define lnc_vh_stop     generic_vh_stop

#define MACHINE_DRIVER(GAMENAME, CLOCK, MAIN_IRQ, SOUND_IRQ, GFX, COLOR)   \
																	\
static struct MachineDriver GAMENAME##_machine_driver =             \
{                                                                   \
	/* basic machine hardware */                                	\
	{		                                                        \
		{	  	                                                    \
			CPU_M6502,                                  			\
			CLOCK,													\
			0,                                          			\
			GAMENAME##_readmem,GAMENAME##_writemem,0,0, 			\
			MAIN_IRQ,1                                  			\
		},		                                                    \
		{		                                                    \
			CPU_M6502 | CPU_AUDIO_CPU,                  			\
			500000, /* 500 kHz */                       			\
			2,      /* memory region #2 */              			\
			sound_readmem,sound_writemem,0,0,           			\
			SOUND_IRQ,16   /* IRQs are triggered by the main CPU */ \
		}                                                   		\
	},                                                          	\
	57, 3072,        /* frames per second, vblank duration */   	\
	1,      /* 1 CPU slice per frame - interleaving is forced when a sound command is written */ \
	GAMENAME##_init_machine,		                               	\
																	\
	/* video hardware */                                        	\
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },                   	\
	GFX,                                                        	\
	COLOR,COLOR,                                                	\
	GAMENAME##_vh_convert_color_prom,                           	\
																	\
	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,                   	\
	0,                                                          	\
	GAMENAME##_vh_start,                                        	\
	GAMENAME##_vh_stop,                                         	\
	GAMENAME##_vh_screenrefresh,                                   	\
																	\
	/* sound hardware */                                        	\
	0,0,0,0,                                                    	\
	{                                                           	\
		{                                                   		\
			SOUND_AY8910,                               			\
			&ay8910_interface                           			\
		}                                                   		\
	}                                                           	\
}

#define EGGS_MACHINE_DRIVER(GAMENAME, CLOCK, MAIN_IRQ, SOUND_IRQ, GFX, COLOR)   \
																	\
static struct MachineDriver GAMENAME##_machine_driver =             \
{                                                                   \
	/* basic machine hardware */                                	\
	{		                                                        \
		{	  	                                                    \
			CPU_M6502,                                  			\
			CLOCK,													\
			0,                                          			\
			GAMENAME##_readmem,GAMENAME##_writemem,0,0, 			\
			MAIN_IRQ,1                                  			\
		}                                                   		\
	},                                                          	\
	57, 3072,        /* frames per second, vblank duration */   	\
	1,      /* 1 CPU slice per frame - interleaving is forced when a sound command is written */ \
	0,						                                    	\
																	\
	/* video hardware */                                        	\
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },                   	\
	GFX,                                                        	\
	COLOR,COLOR,                                                	\
	GAMENAME##_vh_convert_color_prom,                           	\
																	\
	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,                   	\
	0,                                                          	\
	GAMENAME##_vh_start,                                        	\
	GAMENAME##_vh_stop,                                         	\
	GAMENAME##_vh_screenrefresh,                                   	\
																	\
	/* sound hardware */                                        	\
	0,0,0,0,                                                    	\
	{                                                           	\
		{                                                   		\
			SOUND_AY8910,                               			\
			&ay8910_interface                           			\
		}                                                   		\
	}                                                           	\
}


MACHINE_DRIVER(btime, 1500000, btime_irq_interrupt, nmi_interrupt, btime_gfxdecodeinfo, 16);

EGGS_MACHINE_DRIVER(eggs, 1500000, interrupt, nmi_interrupt, lnc_gfxdecodeinfo, 8);

MACHINE_DRIVER(lnc, 1500000, btime_nmi_interrupt, lnc_sound_interrupt, lnc_gfxdecodeinfo, 8);

MACHINE_DRIVER(bnj, 750000, btime_nmi_interrupt, nmi_interrupt, bnj_gfxdecodeinfo, 16);

MACHINE_DRIVER(zoar, 1500000, zoar_irq_interrupt, nmi_interrupt, zoar_gfxdecodeinfo, 64);

/***************************************************************************

	Game driver(s)

***************************************************************************/

ROM_START( btime_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ab05a1.12b", 0xb000, 0x1000, 0x1d9da777 )
	ROM_LOAD( "ab04.9b",    0xc000, 0x1000, 0x9dcc9634 )
	ROM_LOAD( "ab06.13b",   0xd000, 0x1000, 0x8fe2de1c )
	ROM_LOAD( "ab05.10b",   0xe000, 0x1000, 0x24560b1c )
	ROM_LOAD( "ab07.15b",   0xf000, 0x1000, 0x7e750c89 )

	ROM_REGION(0x7800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ab8.13k",    0x0000, 0x1000, 0xccc1f7cf )    /* charset #1 */
	ROM_LOAD( "ab9.15k",    0x1000, 0x1000, 0xfcee0000 )
	ROM_LOAD( "ab10.10k",   0x2000, 0x1000, 0x9f7eaf02 )
	ROM_LOAD( "ab11.12k",   0x3000, 0x1000, 0xf491ffff )
	ROM_LOAD( "ab12.7k",    0x4000, 0x1000, 0x85b738a9 )
	ROM_LOAD( "ab13.9k" ,   0x5000, 0x1000, 0xf5faff00 )
	ROM_LOAD( "ab02.4b",    0x6000, 0x0800, 0x644f1331 )    /* charset #2 */
	ROM_LOAD( "ab01.3b",    0x6800, 0x0800, 0x18000000 )
	ROM_LOAD( "ab00.1b",    0x7000, 0x0800, 0xea53eb39 )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "ab14.12h",   0xf000, 0x1000, 0x06b5888d )

	ROM_REGION(0x0800)      /* background graphics */
	ROM_LOAD( "ab03.6b",    0x0000, 0x0800, 0x8d200806 )
ROM_END

ROM_START( btimea_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "aa04.9b",    0xc000, 0x1000, 0x3d080936 )
	ROM_LOAD( "aa06.13b",   0xd000, 0x1000, 0xe2db880f )
	ROM_LOAD( "aa05.10b",   0xe000, 0x1000, 0x34bec090 )
	ROM_LOAD( "aa07.15b",   0xf000, 0x1000, 0xd993a235 )

	ROM_REGION(0x7800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "aa8.13k",    0x0000, 0x1000, 0xf994fcc4 )    /* charset #1 */
	ROM_LOAD( "ab9.15k",    0x1000, 0x1000, 0xfcee0000 )
	ROM_LOAD( "ab10.10k",   0x2000, 0x1000, 0x9f7eaf02 )
	ROM_LOAD( "ab11.12k",   0x3000, 0x1000, 0xf491ffff )
	ROM_LOAD( "aa12.7k",    0x4000, 0x1000, 0x7f8438a2 )
	ROM_LOAD( "ab13.9k" ,   0x5000, 0x1000, 0xf5faff00 )
	ROM_LOAD( "ab02.4b",    0x6000, 0x0800, 0x644f1331 )    /* charset #2 */
	ROM_LOAD( "ab01.3b",    0x6800, 0x0800, 0x18000000 )
	ROM_LOAD( "ab00.1b",    0x7000, 0x0800, 0xea53eb39 )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "ab14.12h",   0xf000, 0x1000, 0x06b5888d )

	ROM_REGION(0x0800)      /* background graphics */
	ROM_LOAD( "ab03.6b",    0x0000, 0x0800, 0x8d200806 )
ROM_END

ROM_START( hamburge_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	/* the following might be wrong - ROMs seem encrypted */
	ROM_LOAD( "inc.1", 0xc000, 0x2000, 0x5e7aae64 )
	ROM_LOAD( "inc.2", 0xe000, 0x2000, 0x57e824c8 )

	ROM_REGION(0x7800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "inc.9", 0x0000, 0x2000, 0x2cd80018 ) /* charset #1 */
	ROM_LOAD( "inc.8", 0x2000, 0x2000, 0x940f50fd )
	ROM_LOAD( "inc.7", 0x4000, 0x2000, 0xdd7cc6dc )
	ROM_LOAD( "inc.5", 0x6000, 0x0800, 0x241199e7 ) /* garbage?? */
	ROM_CONTINUE(      0x6000, 0x0800 )             /* charset #2 */
	ROM_LOAD( "inc.4", 0x6800, 0x0800, 0x02c16917 ) /* garbage?? */
	ROM_CONTINUE(      0x6800, 0x0800 )
	ROM_LOAD( "inc.3", 0x7000, 0x0800, 0xc8b931bf ) /* garbage?? */
	ROM_CONTINUE(      0x7000, 0x0800 )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "inc.6", 0x0000, 0x1000, 0x3c760fb8 ) /* starts at 0000, not f000; 0000-01ff is RAM */
	ROM_RELOAD(        0xf000, 0x1000 )     /* for the reset/interrupt vectors */

	ROM_REGION(0x0800)      /* background graphics */
	/* this ROM is missing? maybe the hardware works differently */
ROM_END

ROM_START( eggs_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "e14.bin", 0x3000, 0x1000, 0x996dbebf )
	ROM_LOAD( "d14.bin", 0x4000, 0x1000, 0xbbbce334 )
	ROM_LOAD( "c14.bin", 0x5000, 0x1000, 0xc89bff1d )
	ROM_LOAD( "b14.bin", 0x6000, 0x1000, 0x86e4f94e )
	ROM_LOAD( "a14.bin", 0x7000, 0x1000, 0x9beb93e9 )
	ROM_RELOAD(          0xf000, 0x1000 )   /* for reset/interrupt vectors */

	ROM_REGION(0x6000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "g12.bin",  0x0000, 0x1000, 0x4beb2eab )
	ROM_LOAD( "g10.bin",  0x1000, 0x1000, 0x61460352 )
	ROM_LOAD( "h12.bin",  0x2000, 0x1000, 0x9c23f42b )
	ROM_LOAD( "h10.bin",  0x3000, 0x1000, 0x77b53ac7 )
	ROM_LOAD( "j12.bin",  0x4000, 0x1000, 0x27ab70f5 )
	ROM_LOAD( "j10.bin",  0x5000, 0x1000, 0x8786e8c4 )
ROM_END

ROM_START( scregg_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "scregg.e14", 0x3000, 0x1000, 0x84a288b2 )
	ROM_LOAD( "scregg.d14", 0x4000, 0x1000, 0x8ab8d190 )
	ROM_LOAD( "scregg.c14", 0x5000, 0x1000, 0xf212ac78 )
	ROM_LOAD( "scregg.b14", 0x6000, 0x1000, 0xdc095969 )
	ROM_LOAD( "scregg.a14", 0x7000, 0x1000, 0x1afd9ddb )
	ROM_RELOAD(             0xf000, 0x1000 )        /* for reset/interrupt vectors */

	ROM_REGION(0x6000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "scregg.g12",  0x0000, 0x1000, 0xa3b0e2d6 )
	ROM_LOAD( "scregg.g10",  0x1000, 0x1000, 0xeb7f0865 )
	ROM_LOAD( "scregg.h12",  0x2000, 0x1000, 0xc48207f2 )
	ROM_LOAD( "scregg.h10",  0x3000, 0x1000, 0x007800b8 )
	ROM_LOAD( "scregg.j12",  0x4000, 0x1000, 0xe5077119 )
	ROM_LOAD( "scregg.j10",  0x5000, 0x1000, 0xc41a7b5e )
ROM_END

/* There is a flyer with a screen shot for Lock'n'Chase at:
   http://www.gamearchive.com/flyers/video/taito/locknchase_f.jpg  */

ROM_START( lnc_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "s3-3d", 0xc000, 0x1000, 0xa8106830 )
	ROM_LOAD( "s2-3c", 0xd000, 0x1000, 0x1c274145 )
	ROM_LOAD( "s1-3b", 0xe000, 0x1000, 0xa41f02d5 )
	ROM_LOAD( "s0-3a", 0xf000, 0x1000, 0xee021c06 )

	ROM_REGION(0x6000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "s8-15l",  0x0000, 0x1000, 0x04ac8f32 )
	ROM_LOAD( "s9-15m",  0x1000, 0x1000, 0x0845b92f )
	ROM_LOAD( "s6-13l",  0x2000, 0x1000, 0x7550c9d6 )
	ROM_LOAD( "s7-13m",  0x3000, 0x1000, 0x23457281 )
	ROM_LOAD( "s4-11l",  0x4000, 0x1000, 0x4b6f09bf )
	ROM_LOAD( "s5-11m",  0x5000, 0x1000, 0x7aa5ddab )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "sa-1h",  0xf000, 0x1000, 0x2c680040 )
ROM_END

ROM_START( bnj_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "bnj12b.bin", 0xa000, 0x2000, 0x9ce25062 )
	ROM_LOAD( "bnj12c.bin", 0xc000, 0x2000, 0x9e763206 )
	ROM_LOAD( "bnj12d.bin", 0xe000, 0x2000, 0xc5a135b9 )

	ROM_REGION(0x8000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bnj4h.bin",  0x0000, 0x2000, 0xc141332f )
	ROM_LOAD( "bnj4f.bin",  0x2000, 0x2000, 0x5035d3c9 )
	ROM_LOAD( "bnj4e.bin",  0x4000, 0x2000, 0x97b719fb )
	ROM_LOAD( "bnj10e.bin", 0x6000, 0x1000, 0x43556767 )
	ROM_LOAD( "bnj10f.bin", 0x7000, 0x1000, 0xc1bfbfbf )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "bnj6c.bin",  0xf000, 0x1000, 0x80f9e12b )
ROM_END

ROM_START( brubber_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	/* a000-bfff space for the service ROM */
	ROM_LOAD( "brubber.12c", 0xc000, 0x2000, 0x2e85db11 )
	ROM_LOAD( "brubber.12d", 0xe000, 0x2000, 0x1d905348 )

	ROM_REGION(0x8000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bnj4h.bin",  0x0000, 0x2000, 0xc141332f )
	ROM_LOAD( "bnj4f.bin",  0x2000, 0x2000, 0x5035d3c9 )
	ROM_LOAD( "bnj4e.bin",  0x4000, 0x2000, 0x97b719fb )
	ROM_LOAD( "bnj10e.bin", 0x6000, 0x1000, 0x43556767 )
	ROM_LOAD( "bnj10f.bin", 0x7000, 0x1000, 0xc1bfbfbf )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "bnj6c.bin",  0xf000, 0x1000, 0x80f9e12b )
ROM_END

ROM_START( caractn_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	/* a000-bfff space for the service ROM */
	ROM_LOAD( "brubber.12c", 0xc000, 0x2000, 0x2e85db11 )
	ROM_LOAD( "caractn.a6",  0xe000, 0x2000, 0x105bf9d5 )

	ROM_REGION(0x8000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "caractn.a2", 0x0000, 0x2000, 0xa72dd54b )
	ROM_LOAD( "caractn.a1", 0x2000, 0x2000, 0x3582b118 )
	ROM_LOAD( "caractn.a0", 0x4000, 0x2000, 0x6da52fab )
	ROM_LOAD( "bnj10e.bin", 0x6000, 0x1000, 0x43556767 )
	ROM_LOAD( "bnj10f.bin", 0x7000, 0x1000, 0xc1bfbfbf )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "bnj6c.bin",  0xf000, 0x1000, 0x80f9e12b )
ROM_END

ROM_START( zoar_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "zoar15",     0xd000, 0x1000, 0xa03c8544 )
	ROM_LOAD( "zoar16",     0xe000, 0x1000, 0x5bb44a00 )
	ROM_LOAD( "zoar17",     0xf000, 0x1000, 0x5f438e27 )

	ROM_REGION(0xa800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "zoar06",     0x0000, 0x1000, 0x1d88e394 )
	ROM_LOAD( "zoar07",     0x1000, 0x1000, 0x1f03000f )
	ROM_LOAD( "zoar03",     0x2000, 0x1000, 0x8e0e75b6 )
	ROM_LOAD( "zoar04",     0x3000, 0x1000, 0xd84f692d )
	ROM_LOAD( "zoar00",     0x4000, 0x1000, 0x209f31c1 )
	ROM_LOAD( "zoar01",     0x5000, 0x1000, 0xbed82d5a )

	ROM_LOAD( "zoar12",     0x6000, 0x0800, 0xe0a986e5 )
	ROM_LOAD( "zoar11",     0x6800, 0x0800, 0x01d2fca0 )
	ROM_LOAD( "zoar10",     0x7000, 0x0800, 0xae683efc )

	ROM_LOAD( "zoar08",     0x7800, 0x1000, 0xd7226052 )
	ROM_LOAD( "zoar05",     0x8800, 0x1000, 0xc94cd6a8 )
	ROM_LOAD( "zoar02",     0x9800, 0x1000, 0x40a9bd9f )

	ROM_REGION(0x10000)      /* 64k for the audio CPU */
	ROM_LOAD( "zoar09",     0xf000, 0x1000, 0xd9658801 )

	ROM_REGION(0x1000)      /* background graphics */
	ROM_LOAD( "zoar13",     0x0000, 0x1000, 0x82532837 )
ROM_END


static void btime_decode(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* For now, just copy the RAM array over to ROM. Decryption will happen */
	/* at run time, since the CPU applies the decryption only if the previous */
	/* instruction did a memory write. */
	memcpy(ROM,RAM,0x10000);
}

static void zoar_decode(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	// At location 0xD50A is what looks like an undocumented opcode. I tried
	// implemting it given what opcode 0x23 should do, but it still didn't
	// work in demo mode. So this could be another protection or a bad ROM read.
	// I'm NOPing it out for now.
	memset(&RAM[0xd50a],0xea,8);

	/* For now, just copy the RAM array over to ROM. Decryption will happen */
	/* at run time, since the CPU applies the decryption only if the previous */
	/* instruction did a memory write. */
	memcpy(ROM,RAM,0x10000);
}

static void lnc_decode (void)
{
	static int encrypttable[] = { 0x00, 0x10, 0x40, 0x50, 0x20, 0x30, 0x60, 0x70,
								  0x80, 0x90, 0xc0, 0xd0, 0xa0, 0xb0, 0xe0, 0xf0};

	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0;A < 0x10000;A++)
	{
		/*
		 *  The encryption is dead simple.  Just swap bits 5 & 6 for opcodes
		 *  only.
		 */
		ROM[A] = (RAM[A] & 0x0f) | encrypttable[RAM[A] >> 4];
	}
}



static int btime_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0036],"\x00\x80\x02",3) == 0 &&
		memcmp(&RAM[0x0042],"\x50\x48\00",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0036],6*6);
			RAM[0x0033] = RAM[0x0036];
			RAM[0x0034] = RAM[0x0037];
			RAM[0x0035] = RAM[0x0038];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void btime_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0036],6*6);
		osd_fclose(f);
	}
}


static int eggs_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0400],"\x17\x25\x19",3)==0) &&
		(memcmp(&RAM[0x041B],"\x00\x47\x00",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0400],0x1E);
			/* Fix hi score at top */
			memcpy(&RAM[0x0015],&RAM[0x0403],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void eggs_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0400],0x1E);
		osd_fclose(f);
	}
}

static int lnc_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/*   Check if the hi score table has already been initialized.
	 */
	if (RAM[0x02b7] == 0xff)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int banksave, i;

			osd_fread(f,&RAM[0x0008],0x03);
			osd_fread(f,&RAM[0x0294],0x0f);
			osd_fread(f,&RAM[0x02a6],0x0f);

			// Put high score on screen as we missed when it was done
			// by the program
			banksave = *lnc_charbank;
			*lnc_charbank = 0;

			lnc_videoram_w(0x004c, 0x0b);
			lnc_videoram_w(0x0052, (RAM[0x0008] & 0x0f) + 1);
			lnc_videoram_w(0x0051, (RAM[0x0008] >> 4) + 1);
			lnc_videoram_w(0x0050, (RAM[0x0009] & 0x0f) + 1);
			lnc_videoram_w(0x004f, (RAM[0x0009] >> 4) + 1);
			lnc_videoram_w(0x004e, (RAM[0x000a] & 0x0f) + 1);
			lnc_videoram_w(0x004d, (RAM[0x000a] >> 4) + 1);

			// Remove leading zeros
			for (i = 0; i < 5; i++)
			{
				if (videoram_r(0x004d + i) != 0x01) break;

				lnc_videoram_w(0x004c + i, 0x00);
				lnc_videoram_w(0x004d + i, 0x0b);
			}

			*lnc_charbank = banksave;

			osd_fclose(f);
		}

		return 1;
	}

	/*
	 *  The hi scores can't be loaded yet.
	 */
	return 0;
}

static void lnc_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0008],0x03);
		osd_fwrite(f,&RAM[0x0294],0x0f);
		osd_fwrite(f,&RAM[0x02a6],0x0f);
		osd_fclose(f);
	}
}

static int bnj_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/*   Check if the hi score table has already been initialized.
	 */
	if (RAM[0x0640] == 0x4d)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0500],640);
			osd_fclose(f);
		}

		return 1;
	}

	/*
	 *  The hi scores can't be loaded yet.
	 */
	return 0;
}

static void bnj_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0500],640);
		osd_fclose(f);
	}
}


static int zoar_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x034b],"\x20",1) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x02dd],3);
			osd_fread(f,&RAM[0x02e5],5*3);
			osd_fread(f,&RAM[0x034b],3);
			osd_fread(f,&RAM[0x0356],3);
			osd_fread(f,&RAM[0x0361],3);
			osd_fread(f,&RAM[0x036c],3);
			osd_fread(f,&RAM[0x0377],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void zoar_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x02dd],3);
		osd_fwrite(f,&RAM[0x02e5],5*3);
		osd_fwrite(f,&RAM[0x034b],3);
		osd_fwrite(f,&RAM[0x0356],3);
		osd_fwrite(f,&RAM[0x0361],3);
		osd_fwrite(f,&RAM[0x036c],3);
		osd_fwrite(f,&RAM[0x0377],3);
		osd_fclose(f);
	}
}



struct GameDriver btime_driver =
{
	__FILE__,
	0,
	"btime",
	"Burger Time (Midway)",
	"1982",
	"Data East (Bally Midway license)",
	"Kevin Brisley (Replay emulator)\nMirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)\nZsolt Vasvari (ROM decryption)",
	0,
	&btime_machine_driver,

	btime_rom,
	0, btime_decode,
	0,
	0,	/* sound_prom */

	btime_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	btime_hiload, btime_hisave
};

struct GameDriver btimea_driver =
{
	__FILE__,
	&btime_driver,
	"btimea",
	"Burger Time (Data East)",
	"1982",
	"Data East Corporation",
	"Kevin Brisley (Replay emulator)\nMirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)\nZsolt Vasvari (ROM decryption)",
	0,
	&btime_machine_driver,

	btimea_rom,
	0, btime_decode,
	0,
	0,	/* sound_prom */

	btime_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	btime_hiload, btime_hisave
};

struct GameDriver hamburge_driver =
{
	__FILE__,
	&btime_driver,
	"hamburge",
	"Hamburger",
	"1982",
	"bootleg",
	"Kevin Brisley (Replay emulator)\nMirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)",
	GAME_NOT_WORKING,
	&btime_machine_driver,

	hamburge_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	btime_input_ports,

	hamburge_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	btime_hiload, btime_hisave
};

struct GameDriver eggs_driver =
{
	__FILE__,
	0,
	"eggs",
	"Eggs",
	"1983",
	"Universal USA",
	"Nicola Salmoria (MAME driver)\nMike Balfour (high score save)",
	0,
	&eggs_machine_driver,

	eggs_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	eggs_input_ports,

	eggs_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	eggs_hiload, eggs_hisave
};

struct GameDriver scregg_driver =
{
	__FILE__,
	&eggs_driver,
	"scregg",
	"Scrambled Egg",
	"1983",
	"Technos",
	"Nicola Salmoria (MAME driver)\nMike Balfour (high score save)",
	0,
	&eggs_machine_driver,

	scregg_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	eggs_input_ports,

	eggs_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	eggs_hiload, eggs_hisave
};

struct GameDriver lnc_driver =
{
	__FILE__,
	0,
	"lnc",
	"Lock'n'Chase",
	"1981",
	"Data East Corporation",
	"Zsolt Vasvari\nKevin Brisley (Bump 'n' Jump driver)\nMirko Buffoni (Audio/Add. code)",
	0,
	&lnc_machine_driver,

	lnc_rom,
	0, lnc_decode,
	0,
	0,	/* sound_prom */

	lnc_input_ports,

	lnc_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	lnc_hiload, lnc_hisave
};

struct GameDriver bnj_driver =
{
	__FILE__,
	0,
	"bnj",
	"Bump 'n' Jump",
	"1982",
	"Data East USA (Bally Midway license)",
	"Kevin Brisley (MAME driver)\nMirko Buffoni (Audio/Add. code)",
	0,
	&bnj_machine_driver,

	bnj_rom,
	0, lnc_decode,
	0,
	0,	/* sound_prom */

	bnj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	bnj_hiload, bnj_hisave
};

struct GameDriver brubber_driver =
{
	__FILE__,
	&bnj_driver,
	"brubber",
	"Burnin' Rubber",
	"1982",
	"Data East",
	"Kevin Brisley (MAME driver)\nMirko Buffoni (Audio/Add. code)",
	0,
	&bnj_machine_driver,

	brubber_rom,
	0, lnc_decode,
	0,
	0,	/* sound_prom */

	bnj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	bnj_hiload, bnj_hisave
};

struct GameDriver caractn_driver =
{
	__FILE__,
	&bnj_driver,
	"caractn",
	"Car Action",
	"1983",
	"bootleg",
	"Ivan Mackintosh\nKevin Brisley (Bump 'n' Jump driver)\nMirko Buffoni (Audio/Add. code)",
	0,
	&bnj_machine_driver,

	caractn_rom,
	0, lnc_decode,
	0,
	0,	/* sound_prom */

	bnj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	bnj_hiload, bnj_hisave
};

struct GameDriver zoar_driver =
{
	__FILE__,
	0,
	"zoar",
	"Zoar",
	"1982",
	"Data East USA",
	"Zsolt Vasvari\nKevin Brisley (Bump 'n' Jump driver)",
	0,
	&zoar_machine_driver,

	zoar_rom,
	0, zoar_decode,
	0,
	0,	/* sound_prom */

	zoar_input_ports,

	zoar_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	zoar_hiload, zoar_hisave
};

