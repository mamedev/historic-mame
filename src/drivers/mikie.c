/***************************************************************************

Mikie memory map (preliminary)

MAIN BOARD:
2800-288f Sprite RAM (288f, not 287f - quite unusual)
3800-3bff Color RAM
3c00-3fff Video RAM
4000-5fff ROM (?)
5ff0	  Watchdog (?)
6000-ffff ROM


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/M6809.h"



void mikie_palettebank_w(int offset,int data);
void mikie_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void mikie_vh_screenrefresh(struct osd_bitmap *bitmap);



void mikie_init_machine(void)
{
        /* Set optimization flags for M6809 */
        m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}

int mikie_sh_timer_r(int offset)
{
	int clock;

#define TIMER_RATE 512

	clock = cpu_gettotalcycles() / TIMER_RATE;

	return clock;
}

void mikie_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 0 && data == 1)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x00ff, MRA_RAM },	/* ???? */
	{ 0x2400, 0x2400, input_port_0_r },	/* coins + selftest */
	{ 0x2401, 0x2401, input_port_1_r },	/* player 1 controls */
	{ 0x2402, 0x2402, input_port_2_r },	/* player 2 controls */
	{ 0x2403, 0x2403, input_port_3_r },	/* flip */
	{ 0x2500, 0x2500, input_port_4_r },	/* Dipswitch settings */
	{ 0x2501, 0x2501, input_port_5_r },	/* Dipswitch settings */
	{ 0x2800, 0x2fff, MRA_RAM },	/* RAM BANK 2 */
	{ 0x3000, 0x37ff, MRA_RAM },	/* RAM BANK 3 */
	{ 0x3800, 0x3fff, MRA_RAM },	/* video RAM */
	{ 0x4000, 0x5fff, MRA_ROM },    /* Machine checks for extra rom */
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x2002, 0x2002, mikie_sh_irqtrigger_w },
	{ 0x2007, 0x2007, interrupt_enable_w },
	{ 0x2100, 0x2100, MWA_NOP },		/* Watchdog */
	{ 0x2200, 0x2200, mikie_palettebank_w },
	{ 0x2400, 0x2400, soundlatch_w },
	{ 0x2800, 0x288f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x2890, 0x37ff, MWA_RAM },
	{ 0x3800, 0x3bff, colorram_w, &colorram },
	{ 0x3c00, 0x3fff, videoram_w, &videoram, &videoram_size },
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x8003, 0x8003, soundlatch_r },
	{ 0x8005, 0x8005, mikie_sh_timer_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x8000, 0x8000, MWA_NOP },	/* sound command latch */
	{ 0x8001, 0x8001, MWA_NOP },	/* ??? */
	{ 0x8002, 0x8002, SN76496_0_w },	/* trigger read of latch */
	{ 0x8004, 0x8004, SN76496_1_w },	/* trigger read of latch */
	{ 0x8079, 0x8079, MWA_NOP },	/* ??? */
//	{ 0xa003, 0xa003, MWA_RAM },
	{ -1 }
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Controls", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Single" )
	PORT_DIPSETTING(    0x02, "Dual" )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x50, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x10, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Disabled" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "20000 50000" )
	PORT_DIPSETTING(    0x10, "30000 60000" )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed */
	{ 7*4*8, 6*4*8, 5*4*8, 4*4*8, 3*4*8, 2*4*8, 1*4*8, 0*4*8 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	8*4*8     /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	     /* 16*16 sprites */
	256,	        /* 256 sprites */
	4,	           /* 4 bits per pixel */
	{ 0, 4, 256*128*8+0, 256*128*8+4 },
	{ 39*16, 38*16, 37*16, 36*16, 35*16, 34*16, 33*16, 32*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			0, 1, 2, 3, 48*8+0, 48*8+1, 48*8+2, 48*8+3 },
	128*8	/* every sprite takes 64 bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,         0, 16*8 },
	{ 1, 0x4000, &spritelayout, 16*8*16, 16*8 },
	{ 1, 0x4001, &spritelayout, 16*8*16, 16*8 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* 469D19.1I - palette red component */
	0x00,0x0E,0x0E,0x01,0x0E,0x00,0x0E,0x0B,0x0E,0xFE,0xF4,0xF0,0xFF,0xFA,0x0D,0x05,
	0x00,0x0E,0x0A,0x0C,0x09,0x0D,0x0E,0x0C,0x0A,0x00,0x0A,0x00,0x05,0x00,0x0D,0x0F,
	0x00,0x0E,0x0D,0x01,0x0E,0x00,0x0E,0xCB,0xFE,0xFE,0x84,0x00,0x0F,0x0A,0x0D,0x05,
	0x70,0x0E,0x0C,0x78,0x70,0x20,0x9D,0x1B,0xF4,0xC0,0x7F,0x00,0x03,0x25,0x78,0x7F,
	0x20,0x2E,0x95,0x70,0x7E,0x20,0x9E,0x3B,0x1E,0xFE,0xC4,0xC0,0x5F,0x5A,0x5D,0xB5,
	0x50,0xDE,0x5B,0xED,0x5D,0x4E,0x5E,0x88,0x5A,0xE0,0x28,0x00,0x9C,0x38,0x44,0x2F,
	0x00,0x0E,0x2E,0x71,0x1E,0x00,0xEE,0xEB,0xBE,0x2E,0x04,0xA0,0x7F,0x7A,0x2D,0x25,
	0xF0,0x2E,0x2F,0x05,0x2A,0xFE,0xCF,0x3B,0x5E,0x00,0x2A,0xE0,0xC9,0x37,0x5B,0x06,
	0x00,0x1E,0xCE,0x31,0x5E,0x00,0x0E,0x0B,0xCE,0x3E,0x54,0x90,0x3F,0x0A,0x9D,0x35,
	0x00,0xAE,0xE5,0x57,0x2C,0x5E,0x2E,0xEB,0x27,0x90,0x5C,0xE0,0xEA,0xE6,0x25,0x0F,
	0x90,0xCE,0x55,0x2D,0xFE,0x10,0x0E,0x0B,0x1E,0x7E,0xB4,0x20,0xFF,0xCA,0x5D,0x25,
	0xF0,0x1E,0xAE,0x2D,0x0E,0xCE,0x3F,0x8D,0x7B,0xE0,0x0A,0xE0,0x0B,0x2D,0x09,0xCF,
	0x50,0x2E,0x0E,0x39,0xFE,0x10,0x0E,0xCB,0x4E,0x2E,0x04,0x30,0x0F,0x2A,0xCD,0x65,
	0x20,0xEE,0xCF,0x28,0x0F,0xCF,0x3F,0x1B,0x36,0x00,0xAF,0x00,0x03,0x38,0xF6,0x20,
	0x10,0xFE,0x3E,0xFE,0xD0,0x50,0xC0,0x3B,0xFA,0x00,0x40,0xC0,0x1F,0x9F,0xCF,0x2F,
	0xD0,0xCE,0x0E,0x0E,0x10,0x10,0x70,0x2B,0xCA,0x60,0x20,0xF0,0x0F,0x9F,0x9F,0x5F,
	/* 469D21.3I - palette green component */
	0x00,0x08,0x09,0x0A,0x00,0x08,0x0E,0x0B,0x0E,0xF0,0xF3,0xF6,0xFD,0xF2,0x08,0x05,
	0x00,0x0E,0x0A,0x0C,0x05,0x0B,0x0C,0x0C,0x02,0x04,0x07,0x0A,0x05,0x08,0x08,0x00,
	0x00,0x08,0x0D,0x0A,0x00,0x08,0x0E,0xCB,0xFE,0xF0,0x83,0x06,0x0D,0x02,0x08,0x05,
	0x70,0x0E,0x04,0x78,0x77,0x2B,0x9D,0x1B,0xF4,0xC4,0x77,0x08,0x0A,0x26,0x79,0x70,
	0x20,0x28,0x9B,0x78,0x70,0x28,0x9E,0x3B,0x1E,0xF0,0xC3,0xC6,0x5D,0x52,0x58,0xB5,
	0x50,0xDE,0x5B,0xE6,0x5F,0x48,0x59,0x88,0x56,0xE4,0x24,0x08,0x9A,0x3C,0x44,0x20,
	0x00,0x08,0x29,0x7A,0x10,0x08,0xEE,0xEB,0xBE,0x20,0x03,0xA6,0x7D,0x72,0x28,0x25,
	0xF0,0x2E,0x29,0x0A,0x25,0xFF,0xCC,0x33,0x52,0x04,0x2E,0xE8,0xC6,0x3C,0x5B,0x06,
	0x00,0x18,0xC9,0x3A,0x50,0x08,0x0E,0x0B,0xCE,0x30,0x53,0x96,0x3D,0x02,0x98,0x35,
	0x00,0xAE,0xEA,0x59,0x28,0x5A,0x2B,0xEB,0x27,0x94,0x55,0xEA,0xEA,0xE6,0x25,0x00,
	0x90,0xC8,0x5B,0x2C,0xF0,0x18,0x0E,0x0B,0x1E,0x70,0xB3,0x26,0xFD,0xC2,0x58,0x25,
	0xF0,0x1E,0xA6,0x2C,0x0B,0xC8,0x39,0x8A,0x76,0xE4,0x02,0xEA,0x02,0x23,0x06,0xC0,
	0x50,0x28,0x09,0x38,0xF0,0x18,0x0E,0xCB,0x4E,0x20,0x03,0x36,0x0D,0x22,0xC8,0x60,
	0x20,0xEE,0xCC,0x28,0x00,0xC9,0x3F,0x1B,0x36,0x04,0xAC,0x08,0x0A,0x3D,0xF9,0x27,
	0x10,0xFE,0x3F,0xFF,0xDF,0x5A,0xCC,0x3B,0xF0,0x08,0x40,0xCA,0x17,0x95,0xC3,0x20,
	0xD0,0xCE,0x0F,0x0F,0x1F,0x1A,0x7C,0x2B,0xC0,0x60,0x29,0xFA,0x07,0x95,0x93,0x50,
	/* 469D20.2I - palette blue component */
	0x00,0x06,0x04,0x08,0x0B,0x0C,0x0E,0x0B,0x00,0xF2,0xF0,0xF9,0xFA,0xF2,0x03,0x05,
	0x00,0x0E,0x0A,0x0C,0x03,0x05,0x06,0x00,0x02,0x0B,0x05,0x0C,0x09,0x05,0x03,0x00,
	0x00,0x06,0x08,0x08,0x0B,0x0C,0x0E,0xCB,0xF0,0xF2,0x80,0x09,0x0A,0x02,0x03,0x05,
	0x70,0x0E,0x04,0x78,0x70,0x2F,0x98,0x1B,0xF4,0xCB,0x70,0x0F,0x09,0x27,0x7A,0x70,
	0x20,0x26,0x98,0x76,0x78,0x2C,0x9E,0x3B,0x10,0xF2,0xC0,0xC9,0x5A,0x52,0x53,0xB5,
	0x50,0xDE,0x5B,0xE6,0x5C,0x49,0x5A,0x88,0x54,0xEB,0x23,0x0F,0x96,0x3D,0x44,0x20,
	0x00,0x06,0x24,0x78,0x1B,0x0C,0xEE,0xEB,0xB0,0x22,0x00,0xA9,0x7A,0x72,0x23,0x25,
	0xF0,0x2E,0x28,0x07,0x27,0xF7,0xC6,0x33,0x52,0x0B,0x20,0xEF,0xC8,0x3E,0x5B,0x06,
	0x00,0x16,0xC4,0x38,0x5B,0x0C,0x0E,0x0B,0xC0,0x32,0x50,0x99,0x3A,0x02,0x93,0x35,
	0x00,0xAE,0xEC,0x5F,0x25,0x57,0x28,0xED,0x27,0x9B,0x53,0xEF,0xE9,0xEB,0x28,0x00,
	0x90,0xC6,0x58,0x29,0xF8,0x1C,0x0E,0x0B,0x10,0x72,0xB0,0x29,0xFA,0xC2,0x53,0x25,
	0xF0,0x1E,0xA7,0x29,0x08,0xC5,0x36,0x85,0x74,0xEB,0x02,0xEF,0x04,0x21,0x03,0xC0,
	0x50,0x26,0x04,0x3A,0xFB,0x1C,0x0E,0xCB,0x40,0x22,0x00,0x39,0x0A,0x22,0xC3,0x65,
	0x20,0xEE,0xC8,0x28,0x00,0xC8,0x39,0x1B,0x36,0x0B,0xA0,0x0F,0x09,0x3F,0xF5,0x20,
	0x10,0xFE,0x39,0xF0,0xD0,0x54,0xC9,0x3B,0xFB,0x0F,0x4F,0xCF,0x19,0x97,0xC5,0x20,
	0xD0,0xCE,0x09,0x00,0x10,0x14,0x79,0x2B,0xCB,0x6F,0x29,0xFF,0x09,0x97,0x95,0x50,
	/* 469D22.12H - character lookup table */
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x01,0x04,0x03,0x0B,0x02,0x0F,0x07,0x08,0x05,0x0A,0x06,0x0C,0x0B,0x09,0x0B,
	0x00,0x09,0x02,0x03,0x0F,0x05,0x06,0x07,0x08,0x01,0x0A,0x0C,0x0C,0x0D,0x0E,0x04,
	0x00,0x02,0x01,0x04,0x05,0x07,0x06,0x08,0x0A,0x09,0x0B,0x0C,0x0D,0x0E,0x0F,0x03,
	0x00,0x04,0x01,0x03,0x05,0x07,0x06,0x02,0x08,0x0B,0x09,0x0A,0x0C,0x0D,0x0E,0x0F,
	0x00,0x0C,0x02,0x03,0x0C,0x0E,0x05,0x07,0x08,0x09,0x0F,0x0B,0x04,0x0D,0x0E,0x0F,
	0x00,0x0D,0x05,0x06,0x04,0x02,0x07,0x03,0x08,0x09,0x0E,0x0B,0x0D,0x0D,0x0E,0x0F,
	0x00,0x03,0x05,0x06,0x0D,0x02,0x03,0x07,0x08,0x09,0x0A,0x0B,0x04,0x08,0x0D,0x0F,
	0x00,0x0E,0x02,0x03,0x0D,0x0C,0x07,0x07,0x08,0x09,0x0A,0x0B,0x04,0x08,0x0D,0x0F,
	0x00,0x05,0x06,0x03,0x05,0x04,0x03,0x07,0x08,0x09,0x0A,0x0B,0x0E,0x0D,0x0C,0x0F,
	0x00,0x06,0x02,0x03,0x0C,0x07,0x04,0x07,0x08,0x09,0x0A,0x0B,0x09,0x0D,0x0E,0x0F,
	0x00,0x01,0x0B,0x03,0x0B,0x0B,0x06,0x07,0x08,0x0B,0x0A,0x0B,0x0B,0x0B,0x0B,0x0B,
	0x00,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x00,0x0F,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
	0x00,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
	0x00,0x0B,0x0C,0x0D,0x0E,0x0F,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,
	/* 469D18.F9 - sprite lookup table */
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x01,0x0D,0x03,0x08,0x05,0x06,0x07,0x04,0x02,0x0A,0x0B,0x0C,0x0E,0x09,0x0F,
	0x00,0x01,0x02,0x03,0x05,0x04,0x06,0x07,0x0D,0x0B,0x0A,0x08,0x0C,0x0F,0x09,0x0E,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x0F,0x08,0x0A,0x0D,0x0C,0x0E,0x0B,0x09,
	0x00,0x01,0x04,0x09,0x05,0x02,0x06,0x07,0x08,0x0B,0x0A,0x03,0x0C,0x0D,0x0E,0x0F,
	0x00,0x01,0x05,0x0B,0x0E,0x03,0x06,0x07,0x08,0x0D,0x0A,0x02,0x0C,0x09,0x04,0x0F,
	0x00,0x01,0x02,0x03,0x01,0x05,0x09,0x07,0x03,0x09,0x0A,0x03,0x0C,0x0A,0x01,0x03,
	0x00,0x01,0x02,0x03,0x0A,0x05,0x0A,0x07,0x00,0x09,0x0A,0x01,0x0C,0x00,0x00,0x0A,
	0x00,0x01,0x02,0x03,0x04,0x05,0x08,0x0C,0x08,0x09,0x0A,0x09,0x0C,0x0D,0x0E,0x0F,
	0x00,0x0E,0x0F,0x00,0x07,0x0C,0x0D,0x0C,0x07,0x08,0x0F,0x08,0x02,0x0C,0x0A,0x03,
	0x0F,0x04,0x00,0x05,0x00,0x0F,0x0F,0x0C,0x0E,0x0F,0x0B,0x02,0x05,0x07,0x0F,0x07,
	0x00,0x0E,0x0F,0x00,0x07,0x0C,0x0D,0x0C,0x07,0x08,0x0F,0x08,0x02,0x05,0x0A,0x03,
	0x0B,0x0B,0x0F,0x08,0x0D,0x0F,0x01,0x0F,0x0E,0x00,0x09,0x02,0x0F,0x0D,0x07,0x00,
	0x00,0x0E,0x04,0x00,0x07,0x0C,0x0D,0x0C,0x07,0x08,0x0F,0x08,0x02,0x05,0x00,0x03,
	0x02,0x01,0x08,0x0F,0x02,0x0F,0x02,0x04,0x0A,0x05,0x0E,0x06,0x07,0x0B,0x07,0x00,
	0x0D,0x0F,0x0B,0x08,0x02,0x0B,0x0F,0x08,0x0D,0x05,0x01,0x0F,0x0E,0x00,0x09,0x02
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	1789750,	/* 1.78975 Mhz ??? */
	{ 255, 255 }
};



static struct MachineDriver mikie_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,        /* 1.25 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* ? */
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	mikie_init_machine,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,16*8*16+16*8*16,
	mikie_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	mikie_vh_screenrefresh,

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

ROM_START( mikie_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "mik_10c", 0x6000, 0x2000, 0xd00e7ee2 )
	ROM_LOAD( "mik_10d", 0x8000, 0x2000, 0x9a739a85 )
	ROM_LOAD( "mik_12a", 0xa000, 0x2000, 0x923c86e0 )
	ROM_LOAD( "mik_12c", 0xc000, 0x2000, 0x02a1e95f )
	ROM_LOAD( "mik_12d", 0xe000, 0x2000, 0xda0455ec )

	ROM_REGION(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mik_8i", 0x00000, 0x2000, 0x861ee5a2 )
	ROM_LOAD( "mik_9i", 0x02000, 0x2000, 0xc6e8f8f4 )
	ROM_LOAD( "mik_f1", 0x04000, 0x4000, 0x4b7d6439 )
	ROM_LOAD( "mik_f3", 0x08000, 0x4000, 0xf6cf4ab5 )
	ROM_LOAD( "mik_h1", 0x0c000, 0x4000, 0x3e86314c )
	ROM_LOAD( "mik_h3", 0x10000, 0x4000, 0xca74f5e0 )

	ROM_REGION(0x10000)
	ROM_LOAD( "mik_6e", 0x0000, 0x2000, 0xe1ef92bb )
ROM_END



static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x2a00],"\x1d\x2c\x1f\x00\x01",5) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{

                        osd_fread(f,&RAM[0x2a00],9*5);

				/* top score display */
                        memcpy(&RAM[0x29f0], &RAM[0x2a05], 4);

				/* 1p score display, which also displays the top score on startup */
				memcpy(&RAM[0x297c], &RAM[0x2a05], 4);

                        osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x2a00],9*5);
		osd_fclose(f);
	}
}



struct GameDriver mikie_driver =
{
	"Mikie",
	"mikie",
	"Allard Van Der Bas (MAME driver)\nMirko Buffoni (MAME driver)\nStefano Mozzi (MAME driver)\nMarco Cassili (dip switches)\nAl Kossow (color info)\nGerrit Van Goethem (high score save)",
	&mikie_machine_driver,

	mikie_rom,
	0, 0,
	0,
	0,

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
