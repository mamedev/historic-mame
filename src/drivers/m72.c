/***************************************************************************

IREM M72 board

                                   Board  Working?   8751?
R-Type                              M72      Y         N
Battle Chopper / Mr. Heli           M72      N         Y?
Ninja Spirit                        M72      N         Y?
Image Fight                         M72      N         Y?
Legend of Hero Tonma                M72      N         Y
X Multiply                          M72      N         Y?
Dragon Breed                        M81      N         Y
R-Type II                           M82*     Y         N
Major Title                         M84      Y         N
Hammerin' Harry	/ Daiku no Gensan   M82*     Y         N
Gallop - Armed Police Unit          M72**    Y         N
Pound for Pound                     M83?     N***      N

* multiple versions supported, running on different hardware
** there is also a M84 version of Gallop
*** might be close to working, but gfx ROMs are missing


Notes:

Most of the non working games are protected, at least some of them have a
8751 mcu.
bchopper, nspirit, imgfight, loht are patched to get into service mode.

Major Title is supposed to disable rowscroll after a shot, but I haven't found how

Sprite/tile priorities are not completely understood.

Sample playback works only in R-Type II and Hammerin' Harry.
The other games have similar hardware but I don't understand how the sound
CPU is supposed to get the command to play a sample.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/m72.h"


/* in vidhrdw/m72.c */
extern unsigned char *m72_videoram1,*m72_videoram2,*majtitle_rowscrollram;
void m72_init_machine(void);
void hharry_init_machine(void);
int m72_interrupt(void);
int m72_vh_start(void);
int rtype2_vh_start(void);
int majtitle_vh_start(void);
int hharry_vh_start(void);
void m72_vh_stop(void);
int m72_palette1_r(int offset);
int m72_palette2_r(int offset);
void m72_palette1_w(int offset,int data);
void m72_palette2_w(int offset,int data);
int m72_videoram1_r(int offset);
int m72_videoram2_r(int offset);
void m72_videoram1_w(int offset,int data);
void m72_videoram2_w(int offset,int data);
void majtitle_videoram2_w(int offset,int data);
void m72_irq_line_w(int offset,int data);
void m72_scrollx1_w(int offset,int data);
void m72_scrollx2_w(int offset,int data);
void m72_scrolly1_w(int offset,int data);
void m72_scrolly2_w(int offset,int data);
void m72_spritectrl_w(int offset,int data);
void hharry_spritectrl_w(int offset,int data);
void hharryu_spritectrl_w(int offset,int data);
void m72_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void rtype2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void majtitle_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static void bchopper_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	/* patch out RAM and ROM tests */
	RAM[0x2c24e] = 0x33;
	RAM[0x2c41c] = 0x33;
	RAM[0x2c455] = 0x33;
}
static void nspirit_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	/* patch out RAM and ROM tests */
	RAM[0x22ca9] = 0x33;
	RAM[0x22ea7] = 0x33;
	RAM[0x22edf] = 0x33;
}
static void imgfight_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	/* patch out RAM and ROM tests */
	RAM[0x7a2a7] = 0x33;
	RAM[0x7a4a5] = 0x33;
	RAM[0x7a4de] = 0x33;
}
static void loht_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	/* patch out RAM and ROM tests */
	RAM[0x01011] = 0x33;
	RAM[0x0123f] = 0x33;
	RAM[0x01275] = 0x33;
}


static unsigned char *soundram;


static int soundram_r(int offset)
{
	return soundram[offset];
}

static void soundram_w(int offset,int data)
{
	soundram[offset] = data;
}

static void m72_port02_w(int offset,int data)
{
	static int reset;


	if (offset != 0)
	{
if (errorlog && data) fprintf(errorlog,"write %02x to port 03\n",data);
		return;
	}
if (errorlog && (data & 0xec)) fprintf(errorlog,"write %02x to port 02\n",data);

	/* bits 0/1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 3 is used but unknown */

	/* bit 4 resets sound CPU (active low) */
	if (data & 0x10)
	{
		if (!reset)
		{
			cpu_reset(1);
			cpu_halt(1,1);
		}
	}
	else
	{
		cpu_halt(1,0);
	}

	reset = data & 0x10;

	/* other bits unknown */
}

static void rtype2_port02_w(int offset,int data)
{
	if (offset != 0)
	{
if (errorlog && data) fprintf(errorlog,"write %02x to port 03\n",data);
		return;
	}
if (errorlog && (data & 0xfc)) fprintf(errorlog,"write %02x to port 02\n",data);

	/* bits 0/1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* other bits unknown */
}



static struct MemoryReadAddress rtype_readmem[] =
{
	{ 0x00000, 0x3ffff, MRA_ROM },
	{ 0x40000, 0x43fff, MRA_RAM },
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xc8000, 0xc8bff, m72_palette1_r },
	{ 0xcc000, 0xccbff, m72_palette2_r },
	{ 0xd0000, 0xd3fff, m72_videoram1_r },
	{ 0xd8000, 0xdbfff, m72_videoram2_r },
	{ 0xe0000, 0xeffff, soundram_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress rtype_writemem[] =
{
	{ 0x00000, 0x3ffff, MWA_ROM },
	{ 0x40000, 0x43fff, MWA_RAM },	/* work RAM */
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8000, 0xc8bff, m72_palette1_w, &paletteram },
	{ 0xcc000, 0xccbff, m72_palette2_w, &paletteram_2 },
	{ 0xd0000, 0xd3fff, m72_videoram1_w, &m72_videoram1 },
	{ 0xd8000, 0xdbfff, m72_videoram2_w, &m72_videoram2 },
	{ 0xe0000, 0xeffff, soundram_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress bchopper_readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0xa0000, 0xa3fff, MRA_RAM },
	{ 0xb0000, 0xb0fff, MRA_RAM },	/* 8751? */
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xc8000, 0xc8bff, m72_palette1_r },
	{ 0xcc000, 0xccbff, m72_palette2_r },
	{ 0xd0000, 0xd3fff, m72_videoram1_r },
	{ 0xd8000, 0xdbfff, m72_videoram2_r },
	{ 0xe0000, 0xeffff, soundram_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress bchopper_writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0xa0000, 0xa3fff, MWA_RAM },
	{ 0xb0000, 0xb0fff, MWA_RAM },	/* 8751? */
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8000, 0xc8bff, m72_palette1_w, &paletteram },
	{ 0xcc000, 0xccbff, m72_palette2_w, &paletteram_2 },
	{ 0xd0000, 0xd3fff, m72_videoram1_w, &m72_videoram1 },
	{ 0xd8000, 0xdbfff, m72_videoram2_w, &m72_videoram2 },
	{ 0xe0000, 0xeffff, soundram_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress dbreed_readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0x90000, 0x93fff, MRA_RAM },
	{ 0xb0000, 0xb0fff, MRA_RAM },	/* 8751? */
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xc8000, 0xc8bff, m72_palette1_r },
	{ 0xcc000, 0xccbff, m72_palette2_r },
	{ 0xd0000, 0xd3fff, m72_videoram1_r },
	{ 0xd8000, 0xdbfff, m72_videoram2_r },
	{ 0xe0000, 0xeffff, soundram_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dbreed_writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0x90000, 0x93fff, MWA_RAM },
	{ 0xb0000, 0xb0fff, MWA_RAM },	/* 8751? */
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8000, 0xc8bff, m72_palette1_w, &paletteram },
	{ 0xcc000, 0xccbff, m72_palette2_w, &paletteram_2 },
	{ 0xd0000, 0xd3fff, m72_videoram1_w, &m72_videoram1 },
	{ 0xd8000, 0xdbfff, m72_videoram2_w, &m72_videoram2 },
	{ 0xe0000, 0xeffff, soundram_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress rtype2_readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xc8000, 0xc8bff, m72_palette1_r },
	{ 0xd0000, 0xd3fff, m72_videoram1_r },
	{ 0xd4000, 0xd7fff, m72_videoram2_r },
	{ 0xd8000, 0xd8bff, m72_palette2_r },
	{ 0xe0000, 0xe3fff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress rtype2_writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0xb0000, 0xb0001, m72_irq_line_w },
	{ 0xbc000, 0xbc001, m72_spritectrl_w },
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8000, 0xc8bff, m72_palette1_w, &paletteram },
	{ 0xd0000, 0xd3fff, m72_videoram1_w, &m72_videoram1 },
	{ 0xd4000, 0xd7fff, m72_videoram2_w, &m72_videoram2 },
	{ 0xd8000, 0xd8bff, m72_palette2_w, &paletteram_2 },
	{ 0xe0000, 0xe3fff, MWA_RAM },	/* work RAM */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress majtitle_readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0xa0000, 0xa03ff, MRA_RAM },	/* ?? */
	{ 0xa4000, 0xa4bff, m72_palette2_r },
	{ 0xac000, 0xaffff, m72_videoram1_r },
	{ 0xb0000, 0xbffff, m72_videoram2_r },
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xc8000, 0xc83ff, MRA_RAM },
	{ 0xcc000, 0xccbff, m72_palette1_r },
	{ 0xd0000, 0xd3fff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress majtitle_writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0xa0000, 0xa03ff, MWA_RAM, &majtitle_rowscrollram },
	{ 0xa4000, 0xa4bff, m72_palette2_w, &paletteram_2 },
	{ 0xac000, 0xaffff, m72_videoram1_w, &m72_videoram1 },
	{ 0xb0000, 0xbffff, majtitle_videoram2_w, &m72_videoram2 },
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8000, 0xc83ff, MWA_RAM, &spriteram_2 },
	{ 0xcc000, 0xccbff, m72_palette1_w, &paletteram },
	{ 0xd0000, 0xd3fff, MWA_RAM },	/* work RAM */
	{ 0xe0000, 0xe0001, m72_irq_line_w },
//	{ 0xe4000, 0xe4001, MWA_RAM },	/* playfield enable? 1 during screen transitions, 0 otherwise */
	{ 0xec000, 0xec001, hharryu_spritectrl_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress hharry_readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0xa0000, 0xa3fff, MRA_RAM },
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xc8000, 0xc8bff, m72_palette1_r },
	{ 0xcc000, 0xccbff, m72_palette2_r },
	{ 0xd0000, 0xd3fff, m72_videoram1_r },
	{ 0xd8000, 0xdbfff, m72_videoram2_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress hharry_writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0xa0000, 0xa3fff, MWA_RAM },	/* work RAM */
	{ 0xb0ffe, 0xb0fff, MWA_RAM },	/* ?? */
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8000, 0xc8bff, m72_palette1_w, &paletteram },
	{ 0xcc000, 0xccbff, m72_palette2_w, &paletteram_2 },
	{ 0xd0000, 0xd3fff, m72_videoram1_w, &m72_videoram1 },
	{ 0xd8000, 0xdbfff, m72_videoram2_w, &m72_videoram2 },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress hharryu_readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0xa0000, 0xa0bff, m72_palette1_r },
	{ 0xa8000, 0xa8bff, m72_palette2_r },
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xd0000, 0xd3fff, m72_videoram1_r },
	{ 0xd4000, 0xd7fff, m72_videoram2_r },
	{ 0xe0000, 0xe3fff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress hharryu_writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0xa0000, 0xa0bff, m72_palette1_w, &paletteram },
	{ 0xa8000, 0xa8bff, m72_palette2_w, &paletteram_2 },
	{ 0xb0000, 0xb0001, m72_irq_line_w },
	{ 0xbc000, 0xbc001, hharryu_spritectrl_w },
	{ 0xb0ffe, 0xb0fff, MWA_RAM },	/* ?? */
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd0000, 0xd3fff, m72_videoram1_w, &m72_videoram1 },
	{ 0xd4000, 0xd7fff, m72_videoram2_w, &m72_videoram2 },
	{ 0xe0000, 0xe3fff, MWA_RAM },	/* work RAM */
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
	{ 0x05, 0x05, input_port_5_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, m72_port02_w },	/* coin counters, reset sound cpu, other stuff? */
	{ 0x04, 0x05, m72_spritectrl_w },
	{ 0x06, 0x07, m72_irq_line_w },
	{ 0x80, 0x81, m72_scrolly1_w },
	{ 0x82, 0x83, m72_scrollx1_w },
	{ 0x84, 0x85, m72_scrolly2_w },
	{ 0x86, 0x87, m72_scrollx2_w },
//	{ 0xc0, 0xc0,  }, trigger sample, it is then played by the sound CPU
	{ -1 }  /* end of table */
};

static struct IOWritePort rtype2_writeport[] =
{
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, rtype2_port02_w },
	{ 0x80, 0x81, m72_scrolly1_w },
	{ 0x82, 0x83, m72_scrollx1_w },
	{ 0x84, 0x85, m72_scrolly2_w },
	{ 0x86, 0x87, m72_scrollx2_w },
	{ -1 }  /* end of table */
};

static struct IOWritePort hharry_writeport[] =
{
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, rtype2_port02_w },	/* coin counters, reset sound cpu, other stuff? */
	{ 0x04, 0x04, hharry_spritectrl_w },
	{ 0x06, 0x07, m72_irq_line_w },
	{ 0x80, 0x81, m72_scrolly1_w },
	{ 0x82, 0x83, m72_scrollx1_w },
	{ 0x84, 0x85, m72_scrolly2_w },
	{ 0x86, 0x87, m72_scrollx2_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xffff, MWA_RAM, &soundram },
	{ -1 }	/* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x02, 0x02, soundlatch_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x06, 0x06, m72_sound_irq_ack_w },
	{ -1 }  /* end of table */
};

static struct IOReadPort rtype2_sound_readport[] =
{
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x80, 0x80, soundlatch_r },
	{ 0x84, 0x84, m72_sample_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort rtype2_sound_writeport[] =
{
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x80, 0x81, m72_sample_addr_w },
	{ 0x82, 0x82, m72_sample_w },
	{ 0x83, 0x83, m72_sound_irq_ack_w },
	{ -1 }  /* end of table */
};

static struct IOReadPort poundfor_sound_readport[] =
{
	{ 0x41, 0x41, YM2151_status_port_0_r },
	{ 0x42, 0x42, soundlatch_r },
//	{ 0x84, 0x84, m72_sample_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort poundfor_sound_writeport[] =
{
	{ 0x40, 0x40, YM2151_register_port_0_w },
	{ 0x41, 0x41, YM2151_data_port_0_w },
	{ 0x42, 0x42, m72_sound_irq_ack_w },
//	{ 0x80, 0x81, m72_sample_addr_w },
//	{ 0x82, 0x82, m72_sample_w },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( rtype_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* Probably Bonus Life */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
/* Coin Mode 1, todo Mode 2 */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, "8 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, "5 Coins/3 Credits" )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* Probably Difficulty */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", 0, 0 )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

/* Demo sound is inverted, other dips still to test */
INPUT_PORTS_START( rtypej_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* Probably Bonus Life */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
/* Coin Mode 1, todo Mode 2 */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, "8 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, "5 Coins/3 Credits" )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* Probably Difficulty */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", 0, 0 )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( rtype2_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
/* Coin Mode 1, todo Mode 2 */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, "8 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, "5 Coins/3 Credits" )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright 1 Player" )
	PORT_DIPSETTING(    0x18, "Upright 2 Players" )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	/* PORT_DIPSETTING(    0x10, "Upright 2 Players" ) */
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( hharry_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( gallop_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( unknown_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



#define TILELAYOUT(NUM) static struct GfxLayout tilelayout_##NUM =  \
{                                                                   \
	8,8,	/* 8*8 characters */                                    \
	NUM,	/* NUM characters */                                    \
	4,	/* 4 bits per pixel */                                      \
	{ 3*NUM*8*8, 2*NUM*8*8, NUM*8*8, 0 },                           \
	{ 0, 1, 2, 3, 4, 5, 6, 7 },                                     \
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },                     \
	8*8	/* every char takes 8 consecutive bytes */                  \
}

TILELAYOUT(4096);
TILELAYOUT(16384);
TILELAYOUT(32768);

#define SPRITELAYOUT(NUM) static struct GfxLayout spritelayout_##NUM =         \
{                                                                              \
	16,16,	/* 16*16 sprites */                                                \
	NUM,	/* NUM sprites */                                                  \
	4,	/* 4 bits per pixel */                                                 \
	{ 3*NUM*32*8, 2*NUM*32*8, NUM*32*8, 0 },                                   \
	{ 0, 1, 2, 3, 4, 5, 6, 7,                                                  \
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },  \
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,                                  \
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },                    \
	32*8	/* every sprite takes 32 consecutive bytes */                      \
}

SPRITELAYOUT(4096);
SPRITELAYOUT(8192);

static struct GfxDecodeInfo rtype_gfxdecodeinfo[] =
{
	{ 1, 0x40000, &spritelayout_4096,    0, 32 },
	{ 1, 0x00000, &tilelayout_4096,    512, 32 },
	{ 1, 0x20000, &tilelayout_4096,    512, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo bchopper_gfxdecodeinfo[] =
{
	{ 1, 0x100000, &spritelayout_4096,     0, 32 },
	{ 1, 0x000000, &tilelayout_16384,    512, 32 },
	{ 1, 0x080000, &tilelayout_16384,    512, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rtype2_gfxdecodeinfo[] =
{
	{ 1, 0x100000, &spritelayout_4096,     0, 32 },
	{ 1, 0x000000, &tilelayout_32768,    512, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo majtitle_gfxdecodeinfo[] =
{
	{ 1, 0x080000, &spritelayout_8192,     0, 32 },
	{ 1, 0x000000, &tilelayout_16384,    512, 32 },
	{ 1, 0x180000, &spritelayout_4096,     0, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo hharry_gfxdecodeinfo[] =
{
	{ 1, 0x080000, &spritelayout_4096,     0, 32 },
	{ 1, 0x000000, &tilelayout_16384,    512, 32 },
	{ -1 } /* end of array */
};



static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* ??? */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ m72_ym2151_irq_handler },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	1,	/* 1 channel */
	{ 50 }
};



static struct MachineDriver rtype_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			16000000,	/* ?? */
			0,
			rtype_readmem,rtype_writemem,readport,writeport,
			m72_interrupt,256
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579645,		   /* 3.579645 MHz? (Vigilante) */
			2,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* no NMIs unlike the other games */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	55, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	m72_init_machine,

	/* video hardware */
	512, 512, { 8*8, (64-8)*8-1, 16*8, (64-16)*8-1 },
	rtype_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	m72_vh_start,
	m72_vh_stop,
	m72_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};

static struct MachineDriver bchopper_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			16000000,	/* ?? */
			0,
			bchopper_readmem,bchopper_writemem,readport,writeport,
			m72_interrupt,256
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579645,		   /* 3.579645 MHz? (Vigilante) */
			2,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	55, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	m72_init_machine,

	/* video hardware */
	512, 512, { 8*8, (64-8)*8-1, 16*8, (64-16)*8-1 },
	bchopper_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	m72_vh_start,
	m72_vh_stop,
	m72_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver dbreed_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			16000000,	/* ?? */
			0,
			dbreed_readmem,dbreed_writemem,readport,writeport,
			m72_interrupt,256
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579645,		   /* 3.579645 MHz? (Vigilante) */
			2,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	55, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	m72_init_machine,

	/* video hardware */
	512, 512, { 8*8, (64-8)*8-1, 16*8, (64-16)*8-1 },
	bchopper_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	m72_vh_start,
	m72_vh_stop,
	m72_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver rtype2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			16000000,	/* ?? */
			0,
			rtype2_readmem,rtype2_writemem,readport,rtype2_writeport,
			m72_interrupt,256
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579645,		   /* 3.579645 MHz? (Vigilante) */
			2,
			sound_readmem,sound_writemem,rtype2_sound_readport,rtype2_sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	55, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	m72_init_machine,

	/* video hardware */
	512, 512, { 8*8, (64-8)*8-1, 16*8, (64-16)*8-1 },
	rtype2_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	rtype2_vh_start,
	m72_vh_stop,
	rtype2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver majtitle_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			16000000,	/* ?? */
			0,
			majtitle_readmem,majtitle_writemem,readport,rtype2_writeport,
			m72_interrupt,256
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579645,		   /* 3.579645 MHz? (Vigilante) */
			2,
			sound_readmem,sound_writemem,rtype2_sound_readport,rtype2_sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	55, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	m72_init_machine,

	/* video hardware */
	512, 512, { 8*8, (64-8)*8-1, 16*8, (64-16)*8-1 },
	majtitle_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	majtitle_vh_start,
	m72_vh_stop,
	majtitle_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver hharry_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			16000000,	/* ?? */
			0,
			hharry_readmem,hharry_writemem,readport,hharry_writeport,
			m72_interrupt,256
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579645,		   /* 3.579645 MHz? (Vigilante) */
			2,
			sound_readmem,sound_writemem,rtype2_sound_readport,rtype2_sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	55, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	hharry_init_machine,

	/* video hardware */
	512, 512, { 8*8, (64-8)*8-1, 16*8, (64-16)*8-1 },
	hharry_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	hharry_vh_start,
	m72_vh_stop,
	m72_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver hharryu_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			16000000,	/* ?? */
			0,
			hharryu_readmem,hharryu_writemem,readport,rtype2_writeport,
			m72_interrupt,256
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579645,		   /* 3.579645 MHz? (Vigilante) */
			2,
			sound_readmem,sound_writemem,rtype2_sound_readport,rtype2_sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	55, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	hharry_init_machine,

	/* video hardware */
	512, 512, { 8*8, (64-8)*8-1, 16*8, (64-16)*8-1 },
	hharry_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	rtype2_vh_start,
	m72_vh_stop,
	m72_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver poundfor_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			16000000,	/* ?? */
			0,
			rtype2_readmem,rtype2_writemem,readport,rtype2_writeport,
			m72_interrupt,256
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579645,		   /* 3.579645 MHz? (Vigilante) */
			2,
			sound_readmem,sound_writemem,poundfor_sound_readport,poundfor_sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	55, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	hharry_init_machine,

	/* video hardware */
	512, 512, { 8*8, (64-8)*8-1, 16*8, (64-16)*8-1 },
	rtype2_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	rtype2_vh_start,
	m72_vh_stop,
	m72_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rtype_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "aud-h0.bin",   0x00000, 0x10000, 0x36008a4e )
	ROM_LOAD_V20_ODD ( "aud-l0.bin",   0x00000, 0x10000, 0x4aaa668e )
	ROM_LOAD_V20_EVEN( "aud-h1.bin",   0x20000, 0x10000, 0x7ebb2a53 )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "aud-l1.bin",   0x20000, 0x10000, 0xc28b103b )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0xc0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cpu-a0.bin",   0x00000, 0x08000, 0x4e212fb0 )	/* tiles #1 */
	ROM_LOAD( "cpu-a1.bin",   0x08000, 0x08000, 0x8a65bdff )
	ROM_LOAD( "cpu-a2.bin",   0x10000, 0x08000, 0x5a4ae5b9 )
	ROM_LOAD( "cpu-a3.bin",   0x18000, 0x08000, 0x73327606 )
	ROM_LOAD( "cpu-b0.bin",   0x20000, 0x08000, 0xa7b17491 )	/* tiles #2 */
	ROM_LOAD( "cpu-b1.bin",   0x28000, 0x08000, 0xb9709686 )
	ROM_LOAD( "cpu-b2.bin",   0x30000, 0x08000, 0x433b229a )
	ROM_LOAD( "cpu-b3.bin",   0x38000, 0x08000, 0xad89b072 )
	ROM_LOAD( "cpu-00.bin",   0x40000, 0x10000, 0xdad53bc0 )	/* sprites */
	ROM_LOAD( "cpu-01.bin",   0x50000, 0x10000, 0xb28d1a60 )
	ROM_LOAD( "cpu-10.bin",   0x60000, 0x10000, 0xd6a66298 )
	ROM_LOAD( "cpu-11.bin",   0x70000, 0x10000, 0xbb182f1a )
	ROM_LOAD( "cpu-20.bin",   0x80000, 0x10000, 0xfc247c8a )
	ROM_LOAD( "cpu-21.bin",   0x90000, 0x10000, 0x5b41f5f3 )
	ROM_LOAD( "cpu-30.bin",   0xa0000, 0x10000, 0xeb02a1cb )
	ROM_LOAD( "cpu-31.bin",   0xb0000, 0x10000, 0x2bec510a )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */
ROM_END

ROM_START( rtypeb_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "r-7.8b",       0x00000, 0x10000, 0xeacc8024 )
	ROM_LOAD_V20_ODD ( "r-1.7b",       0x00000, 0x10000, 0x2e5fe27b )
	ROM_LOAD_V20_EVEN( "r-8.8c",       0x20000, 0x10000, 0x22cc4950 )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "r-2.7c",       0x20000, 0x10000, 0xada7b90e )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0xc0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cpu-a0.bin",   0x00000, 0x08000, 0x4e212fb0 )	/* tiles #1 */
	ROM_LOAD( "cpu-a1.bin",   0x08000, 0x08000, 0x8a65bdff )
	ROM_LOAD( "cpu-a2.bin",   0x10000, 0x08000, 0x5a4ae5b9 )
	ROM_LOAD( "cpu-a3.bin",   0x18000, 0x08000, 0x73327606 )
	ROM_LOAD( "cpu-b0.bin",   0x20000, 0x08000, 0xa7b17491 )	/* tiles #2 */
	ROM_LOAD( "cpu-b1.bin",   0x28000, 0x08000, 0xb9709686 )
	ROM_LOAD( "cpu-b2.bin",   0x30000, 0x08000, 0x433b229a )
	ROM_LOAD( "cpu-b3.bin",   0x38000, 0x08000, 0xad89b072 )
	ROM_LOAD( "cpu-00.bin",   0x40000, 0x10000, 0xdad53bc0 )	/* sprites */
	ROM_LOAD( "cpu-01.bin",   0x50000, 0x10000, 0xb28d1a60 )
	ROM_LOAD( "cpu-10.bin",   0x60000, 0x10000, 0xd6a66298 )
	ROM_LOAD( "cpu-11.bin",   0x70000, 0x10000, 0xbb182f1a )
	ROM_LOAD( "cpu-20.bin",   0x80000, 0x10000, 0xfc247c8a )
	ROM_LOAD( "cpu-21.bin",   0x90000, 0x10000, 0x5b41f5f3 )
	ROM_LOAD( "cpu-30.bin",   0xa0000, 0x10000, 0xeb02a1cb )
	ROM_LOAD( "cpu-31.bin",   0xb0000, 0x10000, 0x2bec510a )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */
ROM_END

ROM_START( rtypej_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "db_b1.bin",   0x00000, 0x10000, 0xc1865141 )
	ROM_LOAD_V20_ODD ( "db_a1.bin",   0x00000, 0x10000, 0x5ad2bd90 )
	ROM_LOAD_V20_EVEN( "db_b2.bin",   0x20000, 0x10000, 0xb4f6407e )
	ROM_RELOAD_V20_EVEN(              0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "db_a2.bin",   0x20000, 0x10000, 0x6098d86f )
	ROM_RELOAD_V20_ODD (              0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0xc0000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cpu-a0.bin",   0x00000, 0x08000, 0x4e212fb0 )	/* tiles #1 */
	ROM_LOAD( "cpu-a1.bin",   0x08000, 0x08000, 0x8a65bdff )
	ROM_LOAD( "cpu-a2.bin",   0x10000, 0x08000, 0x5a4ae5b9 )
	ROM_LOAD( "cpu-a3.bin",   0x18000, 0x08000, 0x73327606 )
	ROM_LOAD( "cpu-b0.bin",   0x20000, 0x08000, 0xa7b17491 )	/* tiles #2 */
	ROM_LOAD( "cpu-b1.bin",   0x28000, 0x08000, 0xb9709686 )
	ROM_LOAD( "cpu-b2.bin",   0x30000, 0x08000, 0x433b229a )
	ROM_LOAD( "cpu-b3.bin",   0x38000, 0x08000, 0xad89b072 )
	ROM_LOAD( "cpu-00.bin",   0x40000, 0x10000, 0xdad53bc0 )	/* sprites */
	ROM_LOAD( "cpu-01.bin",   0x50000, 0x10000, 0xb28d1a60 )
	ROM_LOAD( "cpu-10.bin",   0x60000, 0x10000, 0xd6a66298 )
	ROM_LOAD( "cpu-11.bin",   0x70000, 0x10000, 0xbb182f1a )
	ROM_LOAD( "cpu-20.bin",   0x80000, 0x10000, 0xfc247c8a )
	ROM_LOAD( "cpu-21.bin",   0x90000, 0x10000, 0x5b41f5f3 )
	ROM_LOAD( "cpu-30.bin",   0xa0000, 0x10000, 0xeb02a1cb )
	ROM_LOAD( "cpu-31.bin",   0xb0000, 0x10000, 0x2bec510a )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */
ROM_END

ROM_START( bchopper_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "c-h0-b.rom",   0x00000, 0x10000, 0xf2feab16 )
	ROM_LOAD_V20_ODD ( "c-l0-b.rom",   0x00000, 0x10000, 0x9f887096 )
	ROM_LOAD_V20_EVEN( "c-h1-b.rom",   0x20000, 0x10000, 0xa995d64f )
	ROM_LOAD_V20_ODD ( "c-l1-b.rom",   0x20000, 0x10000, 0x41dda999 )
	ROM_LOAD_V20_EVEN( "c-h3-b.rom",   0x60000, 0x10000, 0xab9451ca )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "c-l3-b.rom",   0x60000, 0x10000, 0x11562221 )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b-a0-b.rom",   0x000000, 0x10000, 0xe46ed7bf )	/* tiles #1 */
	ROM_LOAD( "b-a1-b.rom",   0x020000, 0x10000, 0x590605ff )
	ROM_LOAD( "b-a2-b.rom",   0x040000, 0x10000, 0xf8158226 )
	ROM_LOAD( "b-a3-b.rom",   0x060000, 0x10000, 0x0f07b9b7 )
	ROM_LOAD( "b-b0-.rom",    0x080000, 0x10000, 0xb5b95776 )	/* tiles #2 */
	ROM_LOAD( "b-b1-.rom",    0x0a0000, 0x10000, 0x74ca16ee )
	ROM_LOAD( "b-b2-.rom",    0x0c0000, 0x10000, 0xb82cca04 )
	ROM_LOAD( "b-b3-.rom",    0x0e0000, 0x10000, 0xa7afc920 )
	ROM_LOAD( "c-00-a.rom",   0x100000, 0x10000, 0xf6e6e660 )	/* sprites */
	ROM_LOAD( "c-01-b.rom",   0x110000, 0x10000, 0x708cdd37 )
	ROM_LOAD( "c-10-a.rom",   0x120000, 0x10000, 0x292c8520 )
	ROM_LOAD( "c-11-b.rom",   0x130000, 0x10000, 0x20904cf3 )
	ROM_LOAD( "c-20-a.rom",   0x140000, 0x10000, 0x1ab50c23 )
	ROM_LOAD( "c-21-b.rom",   0x150000, 0x10000, 0xc823d34c )
	ROM_LOAD( "c-30-a.rom",   0x160000, 0x10000, 0x11f6c56b )
	ROM_LOAD( "c-31-b.rom",   0x170000, 0x10000, 0x23134ec5 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION(0x10000)	/* samples */
	ROM_LOAD( "c-v0-b.rom",   0x00000, 0x10000, 0xd0c27e58 )
ROM_END

ROM_START( mrheli_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "mh-c-h0.bin",  0x00000, 0x10000, 0xe2ca5646 )
	ROM_LOAD_V20_ODD ( "mh-c-l0.bin",  0x00000, 0x10000, 0x643e23cd )
	ROM_LOAD_V20_EVEN( "mh-c-h1.bin",  0x20000, 0x10000, 0x8974e84d )
	ROM_LOAD_V20_ODD ( "mh-c-l1.bin",  0x20000, 0x10000, 0x5f8bda69 )
	ROM_LOAD_V20_EVEN( "mh-c-h3.bin",  0x60000, 0x10000, 0x143f596e )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "mh-c-l3.bin",  0x60000, 0x10000, 0xc0982536 )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mh-b-a0.bin",  0x000000, 0x10000, 0x6a0db256 )	/* tiles #1 */
	ROM_LOAD( "mh-b-a1.bin",  0x020000, 0x10000, 0x14ec9795 )
	ROM_LOAD( "mh-b-a2.bin",  0x040000, 0x10000, 0xdfcb510e )
	ROM_LOAD( "mh-b-a3.bin",  0x060000, 0x10000, 0x957e329b )
	ROM_LOAD( "b-b0-.rom",    0x080000, 0x10000, 0xb5b95776 )	/* tiles #2 */
	ROM_LOAD( "b-b1-.rom",    0x0a0000, 0x10000, 0x74ca16ee )
	ROM_LOAD( "b-b2-.rom",    0x0c0000, 0x10000, 0xb82cca04 )
	ROM_LOAD( "b-b3-.rom",    0x0e0000, 0x10000, 0xa7afc920 )
	ROM_LOAD( "mh-c-00.bin",  0x100000, 0x20000, 0xdec4e121 )	/* sprites */
	ROM_LOAD( "mh-c-10.bin",  0x120000, 0x20000, 0x7aaa151e )
	ROM_LOAD( "mh-c-20.bin",  0x140000, 0x20000, 0xeae0de74 )
	ROM_LOAD( "mh-c-30.bin",  0x160000, 0x20000, 0x01d5052f )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION(0x10000)	/* samples */
	ROM_LOAD( "c-v0-b.rom",   0x00000, 0x10000, 0xd0c27e58 )
ROM_END

ROM_START( nspirit_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "nin-c-h0.rom", 0x00000, 0x10000, 0x035692fa )
	ROM_LOAD_V20_ODD ( "nin-c-l0.rom", 0x00000, 0x10000, 0x9a405898 )
	ROM_LOAD_V20_EVEN( "nin-c-h1.rom", 0x20000, 0x10000, 0xcbc10586 )
	ROM_LOAD_V20_ODD ( "nin-c-l1.rom", 0x20000, 0x10000, 0xb75c9a4d )
	ROM_LOAD_V20_EVEN( "nin-c-h2.rom", 0x40000, 0x10000, 0x8ad818fa )
	ROM_LOAD_V20_ODD ( "nin-c-l2.rom", 0x40000, 0x10000, 0xc52ca78c )
	ROM_LOAD_V20_EVEN( "nin-c-h3.rom", 0x60000, 0x10000, 0x501104ef )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "nin-c-l3.rom", 0x60000, 0x10000, 0xfd7408b8 )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nin-b-a0.rom", 0x000000, 0x10000, 0x63f8f658 )	/* tiles #1 */
	ROM_LOAD( "nin-b-a1.rom", 0x020000, 0x10000, 0x75eb8306 )
	ROM_LOAD( "nin-b-a2.rom", 0x040000, 0x10000, 0xdf532172 )
	ROM_LOAD( "nin-b-a3.rom", 0x060000, 0x10000, 0x4dedd64c )
	ROM_LOAD( "nin-b0.rom",   0x080000, 0x10000, 0x1b0e08a6 )	/* tiles #2 */
	ROM_LOAD( "nin-b1.rom",   0x0a0000, 0x10000, 0x728727f0 )
	ROM_LOAD( "nin-b2.rom",   0x0c0000, 0x10000, 0xf87efd75 )
	ROM_LOAD( "nin-b3.rom",   0x0e0000, 0x10000, 0x98856cb4 )
	ROM_LOAD( "nin-r00.rom",  0x100000, 0x20000, 0x5f61d30b )	/* sprites */
	ROM_LOAD( "nin-r10.rom",  0x120000, 0x20000, 0x0caad107 )
	ROM_LOAD( "nin-r20.rom",  0x140000, 0x20000, 0xef3617d3 )
	ROM_LOAD( "nin-r30.rom",  0x160000, 0x20000, 0x175d2a24 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION(0x10000)	/* samples */
	ROM_LOAD( "nin-v0.rom",      0x00000, 0x10000, 0xa32e8caf )
ROM_END

ROM_START( nspiritj_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "c-h0",         0x00000, 0x10000, 0x8603fab2 )
	ROM_LOAD_V20_ODD ( "c-l0",         0x00000, 0x10000, 0xe520fa35 )
	ROM_LOAD_V20_EVEN( "nin-c-h1.rom", 0x20000, 0x10000, 0xcbc10586 )
	ROM_LOAD_V20_ODD ( "nin-c-l1.rom", 0x20000, 0x10000, 0xb75c9a4d )
	ROM_LOAD_V20_EVEN( "nin-c-h2.rom", 0x40000, 0x10000, 0x8ad818fa )
	ROM_LOAD_V20_ODD ( "nin-c-l2.rom", 0x40000, 0x10000, 0xc52ca78c )
	ROM_LOAD_V20_EVEN( "c-h3",         0x60000, 0x10000, 0x95b63a61 )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "c-l3",         0x60000, 0x10000, 0xe754a87a )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nin-b-a0.rom", 0x000000, 0x10000, 0x63f8f658 )	/* tiles #1 */
	ROM_LOAD( "nin-b-a1.rom", 0x020000, 0x10000, 0x75eb8306 )
	ROM_LOAD( "nin-b-a2.rom", 0x040000, 0x10000, 0xdf532172 )
	ROM_LOAD( "nin-b-a3.rom", 0x060000, 0x10000, 0x4dedd64c )
	ROM_LOAD( "nin-b0.rom",   0x080000, 0x10000, 0x1b0e08a6 )	/* tiles #2 */
	ROM_LOAD( "nin-b1.rom",   0x0a0000, 0x10000, 0x728727f0 )
	ROM_LOAD( "nin-b2.rom",   0x0c0000, 0x10000, 0xf87efd75 )
	ROM_LOAD( "nin-b3.rom",   0x0e0000, 0x10000, 0x98856cb4 )
	ROM_LOAD( "nin-r00.rom",  0x100000, 0x20000, 0x5f61d30b )	/* sprites */
	ROM_LOAD( "nin-r10.rom",  0x120000, 0x20000, 0x0caad107 )
	ROM_LOAD( "nin-r20.rom",  0x140000, 0x20000, 0xef3617d3 )
	ROM_LOAD( "nin-r30.rom",  0x160000, 0x20000, 0x175d2a24 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION(0x10000)	/* samples */
	ROM_LOAD( "nin-v0.rom",      0x00000, 0x10000, 0xa32e8caf )
ROM_END

ROM_START( imgfight_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "if-c-h0.bin",  0x00000, 0x10000, 0x592d2d80 )
	ROM_LOAD_V20_ODD ( "if-c-l0.bin",  0x00000, 0x10000, 0x61f89056 )
	ROM_LOAD_V20_EVEN( "if-c-h3.bin",  0x40000, 0x20000, 0xea030541 )
	ROM_RELOAD_V20_EVEN(               0xc0000, 0x20000 )
	ROM_LOAD_V20_ODD ( "if-c-l3.bin",  0x40000, 0x20000, 0xc66ae348 )
	ROM_RELOAD_V20_ODD (               0xc0000, 0x20000 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "if-a-a0.bin",  0x000000, 0x10000, 0x34ee2d77 )	/* tiles #1 */
	ROM_LOAD( "if-a-a1.bin",  0x020000, 0x10000, 0x6bd2845b )
	ROM_LOAD( "if-a-a2.bin",  0x040000, 0x10000, 0x090d50e5 )
	ROM_LOAD( "if-a-a3.bin",  0x060000, 0x10000, 0x3a8e3083 )
	ROM_LOAD( "if-a-b0.bin",  0x080000, 0x10000, 0xb425c829 )	/* tiles #2 */
	ROM_LOAD( "if-a-b1.bin",  0x0a0000, 0x10000, 0xe9bfe23e )
	ROM_LOAD( "if-a-b2.bin",  0x0c0000, 0x10000, 0x256e50f2 )
	ROM_LOAD( "if-a-b3.bin",  0x0e0000, 0x10000, 0x4c682785 )
	ROM_LOAD( "if-c-00.bin",  0x100000, 0x20000, 0x745e6638 )	/* sprites */
	ROM_LOAD( "if-c-10.bin",  0x120000, 0x20000, 0xb7108449 )
	ROM_LOAD( "if-c-20.bin",  0x140000, 0x20000, 0xaef33cba )
	ROM_LOAD( "if-c-30.bin",  0x160000, 0x20000, 0x1f98e695 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION(0x20000)	/* samples */
	ROM_LOAD( "if-c-v0.bin",  0x00000, 0x10000, 0xcb64a194 )
	ROM_LOAD( "if-c-v1.bin",  0x10000, 0x10000, 0x45b68bf5 )
ROM_END

ROM_START( loht_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "tom_c-h0.rom", 0x00000, 0x20000, 0xa63204b6 )
	ROM_LOAD_V20_ODD ( "tom_c-l0.rom", 0x00000, 0x20000, 0xe788002f )
	ROM_LOAD_V20_EVEN( "tom_c-h3.rom", 0x40000, 0x20000, 0x714778b5 )
	ROM_RELOAD_V20_EVEN(               0xc0000, 0x20000 )
	ROM_LOAD_V20_ODD ( "tom_c-l3.rom", 0x40000, 0x20000, 0x2f049b03 )
	ROM_RELOAD_V20_ODD (               0xc0000, 0x20000 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tom_m21.rom",  0x000000, 0x10000, 0x3ca3e771 )	/* tiles #1 */
	ROM_LOAD( "tom_m22.rom",  0x020000, 0x10000, 0x7a05ee2f )
	ROM_LOAD( "tom_m20.rom",  0x040000, 0x10000, 0x79aa2335 )
	ROM_LOAD( "tom_m23.rom",  0x060000, 0x10000, 0x789e8b24 )
	ROM_LOAD( "tom_m26.rom",  0x080000, 0x10000, 0x44626bf6 )	/* tiles #2 */
	ROM_LOAD( "tom_m27.rom",  0x0a0000, 0x10000, 0x464952cf )
	ROM_LOAD( "tom_m25.rom",  0x0c0000, 0x10000, 0x3db9b2c7 )
	ROM_LOAD( "tom_m24.rom",  0x0e0000, 0x10000, 0xf01fe899 )
	ROM_LOAD( "tom_m53.rom",  0x100000, 0x20000, 0x0b83265f )	/* sprites */
	ROM_LOAD( "tom_m51.rom",  0x120000, 0x20000, 0x8ec5f6f3 )
	ROM_LOAD( "tom_m49.rom",  0x140000, 0x20000, 0xa41d3bfd )
	ROM_LOAD( "tom_m47.rom",  0x160000, 0x20000, 0x9d81a25b )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION(0x10000)	/* samples */
	ROM_LOAD( "tom_m44.rom",  0x00000, 0x10000, 0x3ed51d1f )
ROM_END

ROM_START( xmultipl_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "ch3.h3",       0x00000, 0x20000, 0x20685021 )
	ROM_LOAD_V20_ODD ( "cl3.l3",       0x00000, 0x20000, 0x93fdd200 )
	ROM_LOAD_V20_EVEN( "ch0.h0",       0x40000, 0x10000, 0x9438dd8a )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "cl0.l0",       0x40000, 0x10000, 0x06a9e213 )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0x200000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "t53.a0",       0x000000, 0x20000, 0x1a082494 )	/* tiles #1 */
	ROM_LOAD( "t54.a1",       0x020000, 0x20000, 0x076c16c5 )
	ROM_LOAD( "t55.a2",       0x040000, 0x20000, 0x25d877a5 )
	ROM_LOAD( "t56.a3",       0x060000, 0x20000, 0x5b1213f5 )
	ROM_LOAD( "t57.b0",       0x080000, 0x20000, 0x0a84e0c7 )	/* tiles #2 */
	ROM_LOAD( "t58.b1",       0x0a0000, 0x20000, 0xa874121d )
	ROM_LOAD( "t59.b2",       0x0c0000, 0x20000, 0x69deb990 )
	ROM_LOAD( "t60.b3",       0x0e0000, 0x20000, 0x14c69f99 )
	ROM_LOAD( "t44.00",       0x100000, 0x20000, 0xdb45186e )	/* sprites */
	ROM_LOAD( "t45.01",       0x120000, 0x20000, 0x4d0764d4 )
	ROM_LOAD( "t46.10",       0x140000, 0x20000, 0xf0c465a4 )
	ROM_LOAD( "t47.11",       0x160000, 0x20000, 0x1263b24b )
	ROM_LOAD( "t48.20",       0x180000, 0x20000, 0x4129944f )
	ROM_LOAD( "t49.21",       0x1a0000, 0x20000, 0x2346e6f9 )
	ROM_LOAD( "t50.30",       0x1c0000, 0x20000, 0xe322543e )
	ROM_LOAD( "t51.31",       0x1e0000, 0x20000, 0x229bf7b1 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION(0x20000)	/* samples */
	ROM_LOAD( "t52.v0",       0x00000, 0x20000, 0x2db1bd80 )
ROM_END

ROM_START( dbreed_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "db_c-h3.rom",  0x00000, 0x20000, 0x4bf3063c )
	ROM_LOAD_V20_ODD ( "db_c-l3.rom",  0x00000, 0x20000, 0xe4b89b79 )
	ROM_LOAD_V20_EVEN( "db_c-h0.rom",  0x40000, 0x10000, 0x5aa79fb2 )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "db_c-l0.rom",  0x40000, 0x10000, 0xed0f5e06 )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "db_a0m.rom",   0x000000, 0x20000, 0x4c83e92e )	/* tiles #1 */
	ROM_LOAD( "db_a1m.rom",   0x020000, 0x20000, 0x835ef268 )
	ROM_LOAD( "db_a2m.rom",   0x040000, 0x20000, 0x5117f114 )
	ROM_LOAD( "db_a3m.rom",   0x060000, 0x20000, 0x8eb0c978 )
	ROM_LOAD( "db_b0m.rom",   0x080000, 0x20000, 0x4c83e92e )	/* tiles #2 */
	ROM_LOAD( "db_b1m.rom",   0x0a0000, 0x20000, 0x835ef268 )
	ROM_LOAD( "db_b2m.rom",   0x0c0000, 0x20000, 0x5117f114 )
	ROM_LOAD( "db_b3m.rom",   0x0e0000, 0x20000, 0x8eb0c978 )
	ROM_LOAD( "db_k800m.rom", 0x100000, 0x20000, 0xc027a8cf )	/* sprites */
	ROM_LOAD( "db_k801m.rom", 0x120000, 0x20000, 0x093faf33 )
	ROM_LOAD( "db_k802m.rom", 0x140000, 0x20000, 0x055b4c59 )
	ROM_LOAD( "db_k803m.rom", 0x160000, 0x20000, 0x8ed63922 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION(0x20000)	/* samples */
	ROM_LOAD( "db_c-v0.rom",  0x00000, 0x20000, 0x312f7282 )
ROM_END

ROM_START( rtype2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "ic54.8d",      0x00000, 0x20000, 0xd8ece6f4 )
	ROM_LOAD_V20_ODD ( "ic60.9d",      0x00000, 0x20000, 0x32cfb2e4 )
	ROM_LOAD_V20_EVEN( "ic53.8b",      0x40000, 0x20000, 0x4f6e9b15 )
	ROM_RELOAD_V20_EVEN(               0xc0000, 0x20000 )
	ROM_LOAD_V20_ODD ( "ic59.9b",      0x40000, 0x20000, 0x0fd123bf )
	ROM_RELOAD_V20_ODD (               0xc0000, 0x20000 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic50.7s",      0x000000, 0x20000, 0xf3f8736e )	/* tiles */
	ROM_LOAD( "ic51.7u",      0x020000, 0x20000, 0xb4c543af )
	ROM_LOAD( "ic56.8s",      0x040000, 0x20000, 0x4cb80d66 )
	ROM_LOAD( "ic57.8u",      0x060000, 0x20000, 0xbee128e0 )
	ROM_LOAD( "ic65.9r",      0x080000, 0x20000, 0x2dc9c71a )
	ROM_LOAD( "ic66.9u",      0x0a0000, 0x20000, 0x7533c428 )
	ROM_LOAD( "ic63.9m",      0x0c0000, 0x20000, 0xa6ad67f2 )
	ROM_LOAD( "ic64.9p",      0x0e0000, 0x20000, 0x3686d555 )
	ROM_LOAD( "ic31.6l",      0x100000, 0x20000, 0x2cd8f913 )	/* sprites */
	ROM_LOAD( "ic21.4l",      0x120000, 0x20000, 0x5033066d )
	ROM_LOAD( "ic32.6m",      0x140000, 0x20000, 0xec3a0450 )
	ROM_LOAD( "ic22.4m",      0x160000, 0x20000, 0xdb6176fc )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ic17.4f",      0x0000, 0x10000, 0x73ffecb4 )

	ROM_REGION(0x20000)	/* samples */
	ROM_LOAD( "ic14.4c",      0x00000, 0x20000, 0x637172d5 )
ROM_END

ROM_START( majtitle_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "mt_m0.bin",    0x00000, 0x20000, 0xb9682c70 )
	ROM_LOAD_V20_ODD ( "mt_l0.bin",    0x00000, 0x20000, 0x702c9fd6 )
	ROM_LOAD_V20_EVEN( "mt_m1.bin",    0x40000, 0x20000, 0xd9e97c30 )
	ROM_RELOAD_V20_EVEN(               0xc0000, 0x20000 )
	ROM_LOAD_V20_ODD ( "mt_l1.bin",    0x40000, 0x20000, 0x8dbd91b5 )
	ROM_RELOAD_V20_ODD (               0xc0000, 0x20000 )

	ROM_REGION_DISPOSE(0x200000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mt_c0.bin",    0x000000, 0x20000, 0x780e7a02 )	/* tiles */
	ROM_LOAD( "mt_c1.bin",    0x020000, 0x20000, 0x45ad1381 )
	ROM_LOAD( "mt_c2.bin",    0x040000, 0x20000, 0x5df5856d )
	ROM_LOAD( "mt_c3.bin",    0x060000, 0x20000, 0xf5316cc8 )
	ROM_LOAD( "mt_n0.bin",    0x080000, 0x40000, 0x5618cddc )	/* sprites #1 */
	ROM_LOAD( "mt_n1.bin",    0x0c0000, 0x40000, 0x483b873b )
	ROM_LOAD( "mt_n2.bin",    0x100000, 0x40000, 0x4f5d665b )
	ROM_LOAD( "mt_n3.bin",    0x140000, 0x40000, 0x83571549 )
	ROM_LOAD( "mt_f0.bin",    0x180000, 0x20000, 0x2d5e05d5 )	/* sprites #2 */
	ROM_LOAD( "mt_f1.bin",    0x1a0000, 0x20000, 0xc68cd65f )
	ROM_LOAD( "mt_f2.bin",    0x1c0000, 0x20000, 0xa71feb2d )
	ROM_LOAD( "mt_f3.bin",    0x1e0000, 0x20000, 0x179f7562 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "mt_sp.bin",    0x0000, 0x10000, 0xe44260a9 )

	ROM_REGION(0x20000)	/* samples */
	ROM_LOAD( "mt_vo.bin",    0x00000, 0x20000, 0xeb24bb2c )
ROM_END

ROM_START( hharry_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "a-h0-v.rom",   0x00000, 0x20000, 0xc52802a5 )
	ROM_LOAD_V20_ODD ( "a-l0-v.rom",   0x00000, 0x20000, 0xf463074c )
	ROM_LOAD_V20_EVEN( "a-h1-0.rom",   0x60000, 0x10000, 0x3ae21335 )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "a-l1-0.rom",   0x60000, 0x10000, 0xbc6ac5f9 )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0x100000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hh_a0.rom",    0x000000, 0x20000, 0xc577ba5f )	/* tiles */
	ROM_LOAD( "hh_a1.rom",    0x020000, 0x20000, 0x429d12ab )
	ROM_LOAD( "hh_a2.rom",    0x040000, 0x20000, 0xb5b163b0 )
	ROM_LOAD( "hh_a3.rom",    0x060000, 0x20000, 0x8ef566a1 )
	ROM_LOAD( "hh_00.rom",    0x080000, 0x20000, 0xec5127ef )	/* sprites */
	ROM_LOAD( "hh_10.rom",    0x0a0000, 0x20000, 0xdef65294 )
	ROM_LOAD( "hh_20.rom",    0x0c0000, 0x20000, 0xbb0d6ad4 )
	ROM_LOAD( "hh_30.rom",    0x0e0000, 0x20000, 0x4351044e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a-sp-0.rom",   0x0000, 0x10000, 0x80e210e7 )

	ROM_REGION(0x20000)	/* samples */
	ROM_LOAD( "a-v0-0.rom",   0x00000, 0x20000, 0xfaaacaff )
ROM_END

ROM_START( hharryu_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "a-ho-u.8d",    0x00000, 0x20000, 0xede7f755 )
	ROM_LOAD_V20_ODD ( "a-lo-u.9d",    0x00000, 0x20000, 0xdf0726ae )
	ROM_LOAD_V20_EVEN( "a-h1-f.8b",    0x60000, 0x10000, 0x31b741c5 )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "a-l1-f.9b",    0x60000, 0x10000, 0xb23e966c )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0x100000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hh_a0.rom",    0x000000, 0x20000, 0xc577ba5f )	/* tiles */
	ROM_LOAD( "hh_a1.rom",    0x020000, 0x20000, 0x429d12ab )
	ROM_LOAD( "hh_a2.rom",    0x040000, 0x20000, 0xb5b163b0 )
	ROM_LOAD( "hh_a3.rom",    0x060000, 0x20000, 0x8ef566a1 )
	ROM_LOAD( "hh_00.rom",    0x080000, 0x20000, 0xec5127ef )	/* sprites */
	ROM_LOAD( "hh_10.rom",    0x0a0000, 0x20000, 0xdef65294 )
	ROM_LOAD( "hh_20.rom",    0x0c0000, 0x20000, 0xbb0d6ad4 )
	ROM_LOAD( "hh_30.rom",    0x0e0000, 0x20000, 0x4351044e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a-sp-0.rom",   0x0000, 0x10000, 0x80e210e7 )

	ROM_REGION(0x20000)	/* samples */
	ROM_LOAD( "a-v0-0.rom",   0x00000, 0x20000, 0xfaaacaff )
ROM_END

ROM_START( dkgensan_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "gen-a-h0.bin", 0x00000, 0x20000, 0x07a45f6d )
	ROM_LOAD_V20_ODD ( "gen-a-l0.bin", 0x00000, 0x20000, 0x46478fea )
	ROM_LOAD_V20_EVEN( "gen-a-h1.bin", 0x60000, 0x10000, 0x54e5b73c )
	ROM_RELOAD_V20_EVEN(               0xe0000, 0x10000 )
	ROM_LOAD_V20_ODD ( "gen-a-l1.bin", 0x60000, 0x10000, 0x894f8a9f )
	ROM_RELOAD_V20_ODD (               0xe0000, 0x10000 )

	ROM_REGION_DISPOSE(0x100000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hh_a0.rom",    0x000000, 0x20000, 0xc577ba5f )	/* tiles */
	ROM_LOAD( "hh_a1.rom",    0x020000, 0x20000, 0x429d12ab )
	ROM_LOAD( "hh_a2.rom",    0x040000, 0x20000, 0xb5b163b0 )
	ROM_LOAD( "hh_a3.rom",    0x060000, 0x20000, 0x8ef566a1 )
	ROM_LOAD( "hh_00.rom",    0x080000, 0x20000, 0xec5127ef )	/* sprites */
	ROM_LOAD( "hh_10.rom",    0x0a0000, 0x20000, 0xdef65294 )
	ROM_LOAD( "hh_20.rom",    0x0c0000, 0x20000, 0xbb0d6ad4 )
	ROM_LOAD( "hh_30.rom",    0x0e0000, 0x20000, 0x4351044e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "gen-a-sp.bin", 0x0000, 0x10000, 0xe83cfc2c )

	ROM_REGION(0x20000)	/* samples */
	ROM_LOAD( "gen-vo.bin",   0x00000, 0x20000, 0xd8595c66 )
ROM_END

ROM_START( gallop_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "cc-c-h0.bin",  0x00000, 0x20000, 0x2217dcd0 )
	ROM_LOAD_V20_ODD ( "cc-c-l0.bin",  0x00000, 0x20000, 0xff39d7fb )
	ROM_LOAD_V20_EVEN( "cc-c-h3.bin",  0x40000, 0x20000, 0x9b2bbab9 )
	ROM_RELOAD_V20_EVEN(               0xc0000, 0x20000 )
	ROM_LOAD_V20_ODD ( "cc-c-l3.bin",  0x40000, 0x20000, 0xacd3278e )
	ROM_RELOAD_V20_ODD (               0xc0000, 0x20000 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cc-b-a0.bin",  0x000000, 0x10000, 0xa33472bd )	/* tiles #1 */
	ROM_LOAD( "cc-b-a1.bin",  0x020000, 0x10000, 0x118b1f2d )
	ROM_LOAD( "cc-b-a2.bin",  0x040000, 0x10000, 0x83cebf48 )
	ROM_LOAD( "cc-b-a3.bin",  0x060000, 0x10000, 0x572903fc )
	ROM_LOAD( "cc-b-b0.bin",  0x080000, 0x10000, 0x0df5b439 )	/* tiles #2 */
	ROM_LOAD( "cc-b-b1.bin",  0x0a0000, 0x10000, 0x010b778f )
	ROM_LOAD( "cc-b-b2.bin",  0x0c0000, 0x10000, 0xbda9f6fb )
	ROM_LOAD( "cc-b-b3.bin",  0x0e0000, 0x10000, 0xd361ba3f )
	ROM_LOAD( "cc-c-00.bin",  0x100000, 0x20000, 0x9d99deaa )	/* sprites */
	ROM_LOAD( "cc-c-10.bin",  0x120000, 0x20000, 0x7eb083ed )
	ROM_LOAD( "cc-c-20.bin",  0x140000, 0x20000, 0x9421489e )
	ROM_LOAD( "cc-c-30.bin",  0x160000, 0x20000, 0x920ec735 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION(0x20000)	/* samples */
	ROM_LOAD( "cc-c-v0.bin",  0x00000, 0x20000, 0x6247bade )
ROM_END

ROM_START( poundfor_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "ppa-ho-a.9e",  0x00000, 0x20000, 0xff4c83a4 )
	ROM_LOAD_V20_ODD ( "ppa-lo-a.9d",  0x00000, 0x20000, 0x3374ce8f )
	ROM_LOAD_V20_EVEN( "ppa-h1.9f",    0x40000, 0x20000, 0xf6c82f48 )
	ROM_RELOAD_V20_EVEN(               0xc0000, 0x20000 )
	ROM_LOAD_V20_ODD ( "ppa-l1.9c",    0x40000, 0x20000, 0x5b07b087 )
	ROM_RELOAD_V20_ODD (               0xc0000, 0x20000 )

	ROM_REGION_DISPOSE(0x180000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tiles",        0x000000, 0x100000, 0x00000000 )	/* tiles */
	ROM_LOAD( "sprites",      0x080000, 0x080000, 0x00000000 )	/* sprites */
	ROM_LOAD( "rt2_c0.bin",   0x000000, 0x40000, 0xf5bad5f2 )	/* tiles */
	ROM_LOAD( "rt2_c1.bin",   0x040000, 0x40000, 0x71451778 )
	ROM_LOAD( "rt2_c2.bin",   0x080000, 0x40000, 0xc6b0c352 )
	ROM_LOAD( "rt2_c3.bin",   0x0c0000, 0x40000, 0x6d530a32 )
	ROM_LOAD( "rt2_n0.bin",   0x100000, 0x20000, 0x2cd8f913 )	/* sprites */
	ROM_LOAD( "rt2_n1.bin",   0x120000, 0x20000, 0x5033066d )
	ROM_LOAD( "rt2_n2.bin",   0x140000, 0x20000, 0xec3a0450 )
	ROM_LOAD( "rt2_n3.bin",   0x160000, 0x20000, 0xdb6176fc )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ppa-sp.4j",    0x0000, 0x10000, 0x3f458a5b )

	ROM_REGION(0x20000)	/* samples */
	ROM_LOAD( "samples",      0x00000, 0x20000, 0x00000000 )
ROM_END



struct GameDriver rtype_driver =
{
	__FILE__,
	0,
	"rtype",
	"R-Type (US)",
	"1987",
	"Irem (Nintendo of America license)",
	"Nicola Salmoria",
	0,
	&rtype_machine_driver,
	0,

	rtype_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	rtype_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver rtypeb_driver =
{
	__FILE__,
	&rtype_driver,
	"rtypeb",
	"R-Type (bootleg)",
	"1987",
	"bootleg",
	"Nicola Salmoria",
	0,
	&rtype_machine_driver,
	0,

	rtypeb_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	rtype_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver rtypej_driver =
{
	__FILE__,
	&rtype_driver,
	"rtypej",
	"R-Type (Japan)",
	"1987",
	"Irem",
	"Nicola Salmoria",
	0,
	&rtype_machine_driver,
	0,

	rtypej_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	rtypej_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver bchopper_driver =
{
	__FILE__,
	0,
	"bchopper",
	"Battle Chopper",
	"1987",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&bchopper_machine_driver,
	0,

	bchopper_rom,
	bchopper_patch, 0,
	0,
	0,	/* sound_prom */

	unknown_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver mrheli_driver =
{
	__FILE__,
	&bchopper_driver,
	"mrheli",
	"Mr. Heli",
	"1987",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&bchopper_machine_driver,
	0,

	mrheli_rom,
	bchopper_patch, 0,
	0,
	0,	/* sound_prom */

	unknown_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver nspirit_driver =
{
	__FILE__,
	0,
	"nspirit",
	"Ninja Spirit",
	"1988",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&bchopper_machine_driver,
	0,

	nspirit_rom,
	nspirit_patch, 0,
	0,
	0,	/* sound_prom */

	unknown_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver nspiritj_driver =
{
	__FILE__,
	&nspirit_driver,
	"nspiritj",
	"Ninja Spirit (Japan)",
	"1988",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&bchopper_machine_driver,
	0,

	nspiritj_rom,
	nspirit_patch, 0,
	0,
	0,	/* sound_prom */

	unknown_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver imgfight_driver =
{
	__FILE__,
	0,
	"imgfight",
	"Image Fight",
	"1988",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&bchopper_machine_driver,
	0,

	imgfight_rom,
	imgfight_patch, 0,
	0,
	0,	/* sound_prom */

	unknown_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver loht_driver =
{
	__FILE__,
	0,
	"loht",
	"Legend of Hero Tonma",
	"1989",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&bchopper_machine_driver,
	0,

	loht_rom,
	loht_patch, 0,
	0,
	0,	/* sound_prom */

	unknown_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver xmultipl_driver =
{
	__FILE__,
	0,
	"xmultipl",
	"X Multiply (Japan)",
	"1989",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&bchopper_machine_driver,
	0,

	xmultipl_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	unknown_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver dbreed_driver =
{
	__FILE__,
	0,
	"dbreed",
	"Dragon Breed",
	"1989",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&dbreed_machine_driver,
	0,

	dbreed_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	unknown_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver rtype2_driver =
{
	__FILE__,
	0,
	"rtype2",
	"R-Type II",
	"1989",
	"Irem",
	"Nicola Salmoria",
	0,
	&rtype2_machine_driver,
	0,

	rtype2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	rtype2_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver majtitle_driver =
{
	__FILE__,
	0,
	"majtitle",
	"Major Title (Japan)",
	"1990",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&majtitle_machine_driver,
	0,

	majtitle_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	rtype2_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver hharry_driver =
{
	__FILE__,
	0,
	"hharry",
	"Hammerin' Harry (World)",
	"1990",
	"Irem",
	"Nicola Salmoria",
	0,
	&hharry_machine_driver,
	0,

	hharry_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	hharry_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver hharryu_driver =
{
	__FILE__,
	&hharry_driver,
	"hharryu",
	"Hammerin' Harry (US)",
	"1990",
	"Irem America",
	"Nicola Salmoria",
	0,
	&hharryu_machine_driver,
	0,

	hharryu_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	hharry_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver dkgensan_driver =
{
	__FILE__,
	&hharry_driver,
	"dkgensan",
	"Daiku no Gensan (Japan)",
	"1990",
	"Irem",
	"Nicola Salmoria",
	0,
	&hharryu_machine_driver,
	0,

	dkgensan_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	hharry_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver gallop_driver =
{
	__FILE__,
	0,
	"gallop",
	"Gallop - Armed police Unit (Japan)",
	"1991",
	"Irem",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&bchopper_machine_driver,
	0,

	gallop_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	gallop_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver poundfor_driver =
{
	__FILE__,
	0,
	"poundfor",
	"Pound for Pound",
	"????",
	"?????",
	"Nicola Salmoria",
	0,
	&poundfor_machine_driver,
	0,

	poundfor_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	unknown_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};
