/***************************************************************************

68000:

00000-2ffff ROM
30000-33fff RAM part of this (starting from 40000) is shared with the
                TI TMS320C10NL-14 protection microcontroller
40000-40fff RAM
50000-50fff RAM
7a000-7afff RAM shared with Z80; 16-bit on this side, 8-bit on Z80 side

read:
78009       bit 7 vblank
7e000-7e005 ???

write:
70000-70003 scroll ???
70004-70007 ???
72000-72003 scroll ???
72004-72007 ???
74000-74003 scroll ???
74004-74007 ???
76000-76003 scroll ???
7800c       goes to the TI TMS320C10NL-14 protection microcontroller. Bit 0
            might be interrupt enable, but might also be a signal to the chip
            which then triggers the IRQ. The game writes 0c-0d in succession
            to this register when it expects the protection chip to do its
            work with the shared RAM at 30000.
7e000-7e007 ???

Z80:
0000-7fff ROM
8000-87ff shared with 68000; 8-bit on this side, 16-bit on 68000 side

in:
00        YM3812 status
10        IN
40        DSW1
50        DSW2

out:
00        YM3812 control
01        YM3812 data
20        ??

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M68000/M68000.h"


static unsigned char *twincobr_sharedram;


int twincobr_vh_start(void)
{
	return 0;
}
void twincobr_vh_stop(void)
{
}
void twincobr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
static int base = 0;
static int bose = 0;
if (osd_key_pressed(OSD_KEY_Z))
{
	while (osd_key_pressed(OSD_KEY_Z)) ;
	base -= 0x400;
}
if (osd_key_pressed(OSD_KEY_X))
{
	while (osd_key_pressed(OSD_KEY_X)) ;
	base += 0x400;
}
if (osd_key_pressed(OSD_KEY_C))
{
	while (osd_key_pressed(OSD_KEY_C)) ;
	bose -= 256;
}
if (osd_key_pressed(OSD_KEY_V))
{
	while (osd_key_pressed(OSD_KEY_V)) ;
	bose += 256;
}

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < 0x400;offs++)
	{
		int sx,sy;


		sx = offs % 32;
		sy = offs / 32;

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs+base]+bose,
				0,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}


static int intenable;

int twincobr_interrupt(void)
{
	if (intenable) return MC68000_IRQ_4;
	else return MC68000_INT_NONE;
}

void twincobr_7800c_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"PC %06x - write %02x to 7800c\n",cpu_getpc(),data);
	intenable = data & 1;
}

int twincobr_vblank_r(int offset)
{
	switch (offset)
	{
		case 0:
			return input_port_0_r(offset);
		default:
if (errorlog) fprintf(errorlog,"PC %06x - read input port %06x\n",cpu_getpc(),0x78008+offset);
			return input_port_0_r(offset);
	}
}

int twincobr_sharedram_r(int offset)
{
	return twincobr_sharedram[offset / 2];
}

void twincobr_sharedram_w(int offset,int data)
{
	twincobr_sharedram[offset / 2] = data;
}

static unsigned char *ppp;
static int plap(int offset)
{
if (errorlog) fprintf(errorlog,"plap %06x %04x\n",cpu_getpc(),offset);
if (cpu_getpc() == 0x23a9e)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23aac)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23ad0)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23c1a)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23c3e)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23c62)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23c80)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
	WRITE_WORD(&ppp[0x000e],0x0000);	/* ??? */
}
if (cpu_getpc() == 0x23cb6)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
	WRITE_WORD(&ppp[0x000e],0x0076);
}
	return READ_WORD(&ppp[offset]);
}
static void plop(int offset,int data)
{
if (errorlog) fprintf(errorlog,"plop %06x %04x %02x\n",cpu_getpc(),offset,data);
	COMBINE_WORD_MEM(&ppp[offset],data);
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x00000, 0x2ffff, MRA_ROM },
	{ 0x30000, 0x3001f, plap },
	{ 0x30020, 0x33fff, MRA_BANK1 },
	{ 0x40000, 0x40fff, MRA_BANK2 },
	{ 0x50000, 0x50fff, MRA_BANK3 },
	{ 0x78008, 0x7800b, twincobr_vblank_r },
	{ 0x7a000, 0x7afff, twincobr_sharedram_r },	/* 16-bit on 68000 side, 8-bit on Z80 side */
	{ 0x7e000, 0x7e007, MRA_BANK4 },	/* ??? */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x00000, 0x2ffff, MWA_ROM },
	{ 0x30000, 0x3001f, plop, &ppp },
	{ 0x30020, 0x33fff, MWA_BANK1 },
	{ 0x40000, 0x40fff, MWA_BANK2 },
	{ 0x50000, 0x50fff, MWA_BANK3, &videoram },
	{ 0x70000, 0x70003, MWA_NOP },	/* scroll ??? */
	{ 0x70004, 0x70007, MWA_NOP },	/* ??? */
	{ 0x72000, 0x72003, MWA_NOP },	/* scroll ??? */
	{ 0x72004, 0x72007, MWA_NOP },	/* ??? */
	{ 0x74000, 0x74003, MWA_NOP },	/* scroll ??? */
	{ 0x74004, 0x74007, MWA_NOP },	/* ??? */
	{ 0x76000, 0x76003, MWA_NOP },	/* scroll ??? */
	{ 0x7800c, 0x7800f, twincobr_7800c_w },
	{ 0x7a000, 0x7afff, twincobr_sharedram_w },	/* 16-bit on 68000 side, 8-bit on Z80 side */
	{ 0x7e000, 0x7e007, MWA_BANK4 },	/* ??? */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM, &twincobr_sharedram },
	{ -1 }	/* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, YM3812_status_port_0_r },
	{ 0x10, 0x10, input_port_1_r },
	{ 0x40, 0x40, input_port_2_r },
	{ 0x50, 0x50, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x01, 0x01, YM3812_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )	/* ? could be wrong */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x30, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "4 Coins/1 Credit" )
	PORT_DIPNAME( 0xc0, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "50000 150000" )
	PORT_DIPSETTING(    0x04, "70000 200000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x0c, "100000" )
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_BITX(    0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 0*1024*8*8, 1*1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout1 =
{
	16,16,	/* 16*16 tiles */
	2048,	/* 2048 tiles */
	4,	/* 4 bits per pixel */
	{ 0*2048*32*8, 1*2048*32*8, 2*2048*32*8, 3*2048*32*8 },	/* the bitplanes are separated */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout2 =
{
	16,16,	/* 16*16 tiles */
	1024,	/* 1024 tiles */
	4,	/* 4 bits per pixel */
	{ 0*1024*32*8, 1*1024*32*8, 2*1024*32*8, 3*1024*32*8 },	/* the bitplanes are separated */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,	/* 4 bits per pixel */
	{ 0*2048*32*8, 1*2048*32*8, 2*2048*32*8, 3*2048*32*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,       0, 16 },
	{ 1, 0x06000, &tilelayout1,       0, 16 },
	{ 1, 0x46000, &tilelayout2,       0, 16 },
	{ 1, 0x66000, &spritelayout,       0, 16 },
	{ -1 } /* end of array */
};



static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 255 }		/* (not supported) */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 MHz (?) */
			0,
			readmem,writemem,0,0,
			twincobr_interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			interrupt,60	/* ??? */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,	/* 100 CPU slices per frame */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER,
	0,
	twincobr_vh_start,
	twincobr_vh_stop,
	twincobr_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( twincobr_rom )
	ROM_REGION(0x30000)	/* 3*64k for code */
	ROM_LOAD_EVEN( "tc16", 0x00000, 0x10000, 0x0f9aa12e )
	ROM_LOAD_ODD ( "tc14", 0x00000, 0x10000, 0x1f34c798 )
	ROM_LOAD_EVEN( "tc15", 0x20000, 0x08000, 0x1b87c579 )
	ROM_LOAD_ODD ( "tc13", 0x20000, 0x08000, 0x114ed0b6 )

	ROM_REGION(0xa6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tc03", 0x00000, 0x02000, 0xca1e8a46 )	/* chars */
	ROM_LOAD( "tc04", 0x02000, 0x02000, 0xbb1f3515 )
	ROM_LOAD( "tc11", 0x04000, 0x02000, 0x61353201 )
	ROM_LOAD( "tc01", 0x06000, 0x10000, 0x977e859c )	/* fg tiles */
	ROM_LOAD( "tc02", 0x16000, 0x10000, 0x99f6aed8 )
	ROM_LOAD( "tc05", 0x26000, 0x10000, 0x0ab22680 )
	ROM_LOAD( "tc06", 0x36000, 0x10000, 0xe0022950 )
	ROM_LOAD( "tc07", 0x46000, 0x08000, 0x0a073c15 )	/* bg tiles */
	ROM_LOAD( "tc08", 0x4e000, 0x08000, 0x4ac424d4 )
	ROM_LOAD( "tc09", 0x56000, 0x08000, 0xa8ef725f )
	ROM_LOAD( "tc10", 0x5e000, 0x08000, 0xa3acafa4 )
	ROM_LOAD( "tc17", 0x66000, 0x10000, 0x2f78c36e )	/* sprites */
	ROM_LOAD( "tc18", 0x76000, 0x10000, 0x3a5d943b )
	ROM_LOAD( "tc19", 0x86000, 0x10000, 0xd4dccef2 )
	ROM_LOAD( "tc20", 0x96000, 0x10000, 0x1bf59e49 )

	ROM_REGION(0x10000)	/* 64k for second CPU */
	ROM_LOAD( "tc12", 0x0000, 0x08000, 0x1603fba3 )
ROM_END



struct GameDriver twincobr_driver =
{
	__FILE__,
	0,
	"twincobr",
	"Twin Cobra",
	"????",
	"?????",
	"Nicola Salmoria",
	0,
	&machine_driver,

	twincobr_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};
