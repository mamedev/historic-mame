/***************************************************************************

Birdie King II Memory Map
-------------------------

0000-7fff ROM
8000-83ff Scratch RAM
8400-8fff (Scratch RAM again, address lines AB10, AB11 ignored)
9000-9fff Video RAM?
A000-Bfff Unused?


NOTE:  ROM DM03 is missing from all known ROM sets.  This is a color palette.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

extern void bking2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void bking2_xld1_w(int offset, int data);
extern void bking2_yld1_w(int offset, int data);
extern void bking2_xld2_w(int offset, int data);
extern void bking2_yld2_w(int offset, int data);
extern void bking2_xld3_w(int offset, int data);
extern void bking2_yld3_w(int offset, int data);
extern void bking2_msk_w(int offset, int data);
extern void bking2_safe_w(int offset, int data);
extern void bking2_cont1_w(int offset, int data);
extern void bking2_cont2_w(int offset, int data);
extern void bking2_cont3_w(int offset, int data);
extern void bking2_eport1_w(int offset, int data);
extern void bking2_eport2_w(int offset, int data);
extern void bking2_hitclr_w(int offset, int data);
extern int bking2_pos_r(int offset);

static unsigned char *bking2_ram;

static int bking2_ram_r(int offset)
{
	return bking2_ram[offset];
}

static void bking2_ram_w(int offset, int data)
{
	bking2_ram[offset] = data;
}


static int sndnmi_enable = 1;

static int bking2_sndnmi_disable_r(int offset)
{
	sndnmi_enable = 0;
	return 0;
}

static void bking2_sndnmi_enable_w(int offset,int data)
{
	sndnmi_enable = 1;
}

static void bking2_soundlatch_w(int offset,int data)
{
	int i,code;


	code = 0;
	for (i = 0;i < 8;i++)
		if (data & (1 << i)) code |= 0x80 >> i;

	soundlatch_w(offset,code);
	if (sndnmi_enable) cpu_cause_interrupt(1,Z80_NMI_INT);
}


static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x7fff, MRA_ROM },
    { 0x8000, 0x83ff, bking2_ram_r },
    { 0x8400, 0x87ff, bking2_ram_r },
    { 0x8800, 0x8bff, bking2_ram_r },
    { 0x8c00, 0x8fff, bking2_ram_r },
    { 0x9000, 0x97ff, MRA_RAM },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
    { 0x0000, 0x7fff, MWA_ROM },
    { 0x8000, 0x83ff, bking2_ram_w, &bking2_ram },
    { 0x8400, 0x87ff, bking2_ram_w },
    { 0x8800, 0x8bff, bking2_ram_w },
    { 0x8c00, 0x8fff, bking2_ram_w },
    { 0x9000, 0x97ff, videoram_w, &videoram, &videoram_size },
    { -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
    { 0x00, 0x00, input_port_0_r },
    { 0x01, 0x01, input_port_1_r },
    { 0x02, 0x02, input_port_2_r },
    { 0x03, 0x03, input_port_3_r },
    { 0x04, 0x04, input_port_4_r },
    { 0x05, 0x05, input_port_5_r },
    { 0x06, 0x06, input_port_6_r },
	{ 0x07, 0x1f, bking2_pos_r },
    { -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
    { 0x00, 0x00, bking2_xld1_w },
    { 0x01, 0x01, bking2_yld1_w },
    { 0x02, 0x02, bking2_xld2_w },
    { 0x03, 0x03, bking2_yld2_w },
    { 0x04, 0x04, bking2_xld3_w },
    { 0x05, 0x05, bking2_yld3_w },
    { 0x06, 0x06, bking2_msk_w },
    { 0x07, 0x07, bking2_safe_w },
    { 0x08, 0x08, bking2_cont1_w },
    { 0x09, 0x09, bking2_cont2_w },
    { 0x0a, 0x0a, bking2_cont3_w },
    { 0x0b, 0x0b, bking2_soundlatch_w },
    { 0x0c, 0x0c, bking2_eport2_w },
    { 0x0d, 0x0d, bking2_hitclr_w },
    { -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
    { 0x0000, 0x1fff, MRA_ROM },
    { 0x4000, 0x43ff, MRA_RAM },
	{ 0x4401, 0x4401, AY8910_read_port_0_r },
	{ 0x4403, 0x4403, AY8910_read_port_1_r },
    { 0x4800, 0x4800, soundlatch_r },
    { 0x4802, 0x4802, bking2_sndnmi_disable_r },
    { 0xe000, 0xefff, MRA_ROM },   /* space for some other ROM???
									  It's checked if there is valid code there */
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
    { 0x0000, 0x1fff, MWA_ROM },
    { 0x4000, 0x43ff, MWA_RAM },
	{ 0x4400, 0x4400, AY8910_control_port_0_w },
	{ 0x4401, 0x4401, AY8910_write_port_0_w },
	{ 0x4402, 0x4402, AY8910_control_port_1_w },
	{ 0x4403, 0x4403, AY8910_write_port_1_w },
    { 0x4802, 0x4802, bking2_sndnmi_enable_w },
    { -1 }  /* end of table */
};

INPUT_PORTS_START( bking2_input_ports )
    PORT_START  /* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )

    PORT_START  /* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) /* Continue 1 */
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 ) /* Continue 2 */
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Not Connected? */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Not Connected? */

    PORT_START  /* IN2 - DIP Switch A */
    PORT_DIPNAME( 0x01, 0x00, "Holes Awarded", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Less" )
    PORT_DIPSETTING(    0x01, "More" )
    PORT_DIPNAME( 0x02, 0x02, "Holes Awarded", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Hole In One = 3" )
    PORT_DIPSETTING(    0x02, "Hole In One = 9" )
    PORT_DIPNAME( 0x04, 0x04, "Mode", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Free Play" )
    PORT_DIPSETTING(    0x04, "Normal" )
    PORT_DIPNAME( 0x18, 0x18, "# of Holes", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "9" )
    PORT_DIPSETTING(    0x08, "5" )
    PORT_DIPSETTING(    0x10, "4" )
    PORT_DIPSETTING(    0x18, "3" )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
    PORT_DIPSETTING(    0x40, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Upright" )
    PORT_DIPSETTING(    0x80, "Cocktail" )


    PORT_START  /* IN3 - DIP Switch B */
    PORT_DIPNAME( 0x0F, 0x00, "Coins/Credit (left)", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "1/1" )
    PORT_DIPSETTING(    0x01, "1/2" )
    PORT_DIPSETTING(    0x02, "1/3" )
    PORT_DIPSETTING(    0x03, "1/4" )
    PORT_DIPSETTING(    0x04, "1/5" )
    PORT_DIPSETTING(    0x05, "1/6" )
    PORT_DIPSETTING(    0x06, "1/7" )
    PORT_DIPSETTING(    0x07, "1/8" )
    PORT_DIPSETTING(    0x08, "2/1" )
    PORT_DIPSETTING(    0x09, "3/1" )
    PORT_DIPSETTING(    0x0A, "4/1" )
    PORT_DIPSETTING(    0x0B, "5/1" )
    PORT_DIPSETTING(    0x0C, "6/1" )
    PORT_DIPSETTING(    0x0D, "7/1" )
    PORT_DIPSETTING(    0x0E, "8/1" )
    PORT_DIPSETTING(    0x0F, "9/1" )
    PORT_DIPNAME( 0xF0, 0x00, "Coins/Credit (right)", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "1/1" )
    PORT_DIPSETTING(    0x10, "1/2" )
    PORT_DIPSETTING(    0x20, "1/3" )
    PORT_DIPSETTING(    0x30, "1/4" )
    PORT_DIPSETTING(    0x40, "1/5" )
    PORT_DIPSETTING(    0x50, "1/6" )
    PORT_DIPSETTING(    0x60, "1/7" )
    PORT_DIPSETTING(    0x70, "1/8" )
    PORT_DIPSETTING(    0x80, "2/1" )
    PORT_DIPSETTING(    0x90, "3/1" )
    PORT_DIPSETTING(    0xA0, "4/1" )
    PORT_DIPSETTING(    0xB0, "5/1" )
    PORT_DIPSETTING(    0xC0, "6/1" )
    PORT_DIPSETTING(    0xD0, "7/1" )
    PORT_DIPSETTING(    0xE0, "8/1" )
    PORT_DIPSETTING(    0xF0, "9/1" )

    PORT_START  /* IN4 - DIP Switch C */
    PORT_DIPNAME( 0x01, 0x01, "Crow Appearance", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Without" )
    PORT_DIPSETTING(    0x01, "With" )
    PORT_DIPNAME( 0x06, 0x06, "Crow Flight", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Pattern 1" )
    PORT_DIPSETTING(    0x02, "Pattern 2" )
    PORT_DIPSETTING(    0x04, "Pattern 3" )
    PORT_DIPSETTING(    0x06, "Pattern 4" )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_DIPNAME( 0x10, 0x10, "Coin Display", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "No Display" )
    PORT_DIPSETTING(    0x10, "Display" )
    PORT_DIPNAME( 0x20, 0x20, "Year Display", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "No Display" )
    PORT_DIPSETTING(    0x20, "Display" )
    PORT_DIPNAME( 0x40, 0x40, "Check", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Check" )
    PORT_DIPSETTING(    0x40, "Normal" )
    PORT_DIPNAME( 0x80, 0x80, "Coin System", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "1 Way" )
    PORT_DIPSETTING(    0x80, "2 Way" )

    PORT_START  /* IN5 */
    PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_X, 25, 0, 0, 0 ) /* Sensitivity, clip, min, max */

    PORT_START  /* IN6 */
    PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_REVERSE, 25, 0, 0, 0 ) /* Sensitivity, clip, min, max */
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	3,      /* 3 bits per pixel */
	{ 0*1024*8*8, 1*1024*8*8, 2*1024*8*8 }, /* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 }, /* reverse layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

struct GfxLayout crowlayout =
{
	16,32,	/* 16*32 characters */
	16,		/* 16 characters */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 3*32*8+3, 3*32*8+2, 3*32*8+1, 3*32*8+0,
			2*32*8+3, 2*32*8+2, 2*32*8+1, 2*32*8+0,
			32*8+3, 32*8+2, 32*8+1, 32*8+0,
			3, 2, 1, 0 }, /* reverse layout */
	{ 31*8, 30*8, 29*8, 28*8, 27*8, 26*8, 25*8, 24*8,
			23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	128*8    /* every sprite takes 128 consecutive bytes */
};

struct GfxLayout balllayout =
{
	8,16,  /* 8*16 sprites */
	8,     /* 8 sprites */
	1,  /* 1 bit per pixel */
	{ 0 },   /* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },   /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8    /* every sprite takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { 1, 0x0000, &charlayout,      0, 1 }, /* playfield */
    { 1, 0x6000, &crowlayout,      0, 1 }, /* crow */
    { 1, 0x6800, &balllayout,      8, 1 }, /* ball */
    { 1, 0x7000, &balllayout,      8, 1 }, /* ball */
    { -1 } /* end of array */
};

static unsigned char palette[] =
{
	0x00,0xff,0x00,
	0x80,0x40,0x00,
	0x00,0x00,0x00,
	0xff,0xff,0x00,
	0x00,0x80,0x00,
	0xff,0x00,0x00,
	0x00,0x00,0xff,
	0xff,0xff,0xff
};

static unsigned short colortable[] =
{
    0, 1, 2, 3, 4, 5, 6, 7,
	2, 7
};


static void portb_w(int offset,int data)
{
	/* don't know what this is... could be a filter */
	if (errorlog) if (data != 0x00) fprintf(errorlog,"portB = %02x\n",data);
}

static struct AY8910interface ay8910_interface =
{
	2,      /* 2 chips */
	2000000,	/* 2 MHz ??? */
	{ 25, 25 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, DAC_signed_data_w },
	{ 0, portb_w }
};

static struct DACinterface dac_interface =
{
	1,
	{ 25 }
};



static struct MachineDriver machine_driver =
{
    /* basic machine hardware */
    {
        {
            CPU_Z80,
			4000000,	/* 4 Mhz ??? */
            0,
            readmem,writemem,
            readport,writeport,
            interrupt,1
        },
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz ??? */
			2,
			sound_readmem,sound_writemem,0,0,
			/* interrupts (from Jungle King hardware, might be wrong): */
			/* - no interrupts synced with vblank */
			/* - NMI triggered by the main CPU */
			/* - periodic IRQ, with frequency 6000000/(4*16*16*10*16) = 36.621 Hz, */
			/*   that is a period of 27306666.6666 ns */
			0,0,
			interrupt,27306667
		}
    },
    60, DEFAULT_60HZ_VBLANK_DURATION,   /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
    0,  /* init machine */

    /* video hardware */
    32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
    gfxdecodeinfo,
    sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
    0,

    VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
    0,  /* video hardware init */
    generic_vh_start,
    generic_vh_stop,
    bking2_vh_screenrefresh,

    /* sound hardware */
    0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
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

ROM_START( bking2_rom )
	ROM_REGION(0x10000)
	ROM_LOAD( "01.13f",       0x0000, 0x1000, 0x078ada3f )
	ROM_LOAD( "02.11f",       0x1000, 0x1000, 0xc37d110a )
	ROM_LOAD( "03.10f",       0x2000, 0x1000, 0x2ba5c681 )
	ROM_LOAD( "04.8f",        0x3000, 0x1000, 0x8fad54e8 )
	ROM_LOAD( "05.7f",        0x4000, 0x1000, 0xb4de6b58 )
	ROM_LOAD( "06.5f",        0x5000, 0x1000, 0x9ac43b87 )
	ROM_LOAD( "07.4f",        0x6000, 0x1000, 0xb3ed40b7 )
	ROM_LOAD( "08.2f",        0x7000, 0x1000, 0x8fddb2e8 )

	ROM_REGION_DISPOSE(0x7800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "14.5a",        0x0000, 0x1000, 0x52636a94 )
	ROM_LOAD( "13.7a",        0x1000, 0x1000, 0x6b9e0564 )
	ROM_LOAD( "12.8a",        0x2000, 0x1000, 0xc6d685d9 )
	ROM_LOAD( "11.10a",       0x3000, 0x1000, 0x2b949987 )
	ROM_LOAD( "10.11a",       0x4000, 0x1000, 0xeb96f948 )
	ROM_LOAD( "09.13a",       0x5000, 0x1000, 0x595e3dd4 )
	ROM_LOAD( "17",           0x6000, 0x0800, 0xe5663f0b )
	ROM_LOAD( "18",           0x6800, 0x0800, 0xfc9cec31 )
	ROM_LOAD( "19",           0x7000, 0x0800, 0xfc9cec31 )

	ROM_REGION(0x10000)         /* Sound ROMs */
	ROM_LOAD( "15",           0x0000, 0x1000, 0xf045d0fe )
	ROM_LOAD( "16",           0x1000, 0x1000, 0x92d50410 )
ROM_END



struct GameDriver bking2_driver =
{
	__FILE__,
	0,
	"bking2",
	"Birdie King 2",
	"1983",
	"Taito",
	"Ed. Mueller\nMike Balfour\nZsolt Vasvari",
	0,
	&machine_driver,
	0,

	bking2_rom,
	0, 0,
	0,
	0,  /* sound_prom */

	bking2_input_ports,

	0, palette, colortable,
	ORIENTATION_ROTATE_90,

	0, 0
};
