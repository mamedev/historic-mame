/***************************************************************************
Xain'd Sleena (TECHNOS), Solar Warrior (TAITO).
By Carlos A. Lozano & Rob Rosenbrock & Phil Stroffolino

	- MC68B09EP (2)
        - 6809EP (1)
        - 68705 (only in Solar Warrior)
        - ym2203 (2)

Remaining Issues:

       - Fix the random loops. (yet???)

       - Get better music.

       - Don't understood the timers.

       - Implement the sprite-plane2 priorities.

       - 68705 in Solar Warrior. (partial missing sprites)

       - Optimize. (For example, pallete updates)

       - CPU speed may be wrong

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "m6809/m6809.h"

unsigned char *xain_sharedram;
static int xain_timer = 0xff;

unsigned char waitIRQA;
unsigned char waitIRQB;
unsigned char waitIRQsound;

void xain_vh_screenrefresh(struct osd_bitmap *bitmap);
int xain_vh_start(void);
void xain_vh_stop(void);
void xain_scrollxP2_w(int offset,int data);
void xain_scrollyP2_w(int offset,int data);
void xain_scrollxP3_w(int offset,int data);
void xain_scrollyP3_w(int offset,int data);
void videoram2_w(int offset,int data);

extern unsigned char *xain_videoram;
extern int xain_videoram_size;
extern unsigned char *xain_videoram2;
extern int xain_videoram2_size;
extern unsigned char *xain_paletteram;


void xain_init_machine(void)
{
   //unsigned char *RAM = Machine->memory_region[2];
   //RAM[0x4b90] = 0x00;
}

int xain_sharedram_r(int offset)
{
	return xain_sharedram[offset];
}

void xain_sharedram_w(int offset, int data)
{
	xain_sharedram[offset] = data;
}

void xainCPUA_bankswitch_w(int offset,int data)
{
        if (data&0x08) {cpu_setbank(1,&RAM[0x10000]);}
        else {cpu_setbank(1,&RAM[0x4000]);}
}

void xainCPUB_bankswitch_w(int offset,int data)
{
        if (data&0x1) {cpu_setbank(2,&RAM[0x10000]);}
        else {cpu_setbank(2,&RAM[0x4000]);}
}

int xain_timer_r(int offset)
{
      return (xain_timer);
}

int solarwar_slapstic_r(int offset)
{
      return (0x4d);
}

void xainB_forcedIRQ_w(int offset,int data)
{
    waitIRQB = 1;
    cpu_spin();  // Forced change of CPU
}

void xainA_forcedIRQ_w(int offset,int data)
{
    waitIRQA = 1;
    cpu_spin(); // Forced change of CPU
}

void xainA_writesoundcommand_w(int offset, int data)
{
    waitIRQsound = 1;
    soundlatch_w(offset,data);
}

static struct MemoryReadAddress readmem[] =
{
        { 0x0000, 0x1fff, MRA_RAM, &xain_sharedram},
	{ 0x2000, 0x39ff, MRA_RAM },
        { 0x3a00, 0x3a00, input_port_0_r },
        { 0x3a01, 0x3a01, input_port_1_r },
        { 0x3a02, 0x3a02, input_port_2_r },
        { 0x3a03, 0x3a03, input_port_3_r },
        { 0x3a04, 0x3a04, solarwar_slapstic_r},
        { 0x3a05, 0x3a05, xain_timer_r},       /* how?? */
        { 0x3a06, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
        { 0x0000, 0x1fff, MWA_RAM, &xain_sharedram},
        { 0x2000, 0x27ff, MWA_RAM, &xain_videoram, &xain_videoram_size },
	{ 0x2800, 0x2fff, videoram2_w, &xain_videoram2, &xain_videoram2_size },
	{ 0x3000, 0x37ff, videoram_w, &videoram, &videoram_size },
        { 0x3800, 0x397f, MWA_RAM, &spriteram, &spriteram_size },
        { 0x3980, 0x39ff, MWA_RAM },
        { 0x3a00, 0x3a01, xain_scrollxP2_w},
        { 0x3a02, 0x3a03, xain_scrollyP2_w},
        { 0x3a04, 0x3a05, xain_scrollxP3_w},
        { 0x3a06, 0x3a07, xain_scrollyP3_w},
        { 0x3a08, 0x3a08, xainA_writesoundcommand_w},
        { 0x3a09, 0x3a0b, MWA_RAM },
        { 0x3a0c, 0x3a0c, xainB_forcedIRQ_w},
        { 0x3a0d, 0x3a0e, MWA_RAM },
        { 0x3a0f, 0x3a0f, xainCPUA_bankswitch_w},
        { 0x3a10, 0x3bff, MWA_RAM },
        { 0x3c00, 0x3fff, MWA_RAM, &xain_paletteram },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmemB[] =
{
	{ 0x0000, 0x1fff, xain_sharedram_r },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writememB[] =
{
	{ 0x0000, 0x1fff, xain_sharedram_w },
        { 0x2000, 0x27ff, xainA_forcedIRQ_w},
	{ 0x2800, 0x2fff, MWA_RAM },
        { 0x3000, 0x37ff, xainCPUB_bankswitch_w},
        { 0x3800, 0x3fff, MWA_RAM },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_sound[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x1000, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_sound[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
        { 0x2800, 0x2800, YM2203_control_port_0_w },
	{ 0x2801, 0x2801, YM2203_write_port_0_w },
	{ 0x3000, 0x3000, YM2203_control_port_1_w },
	{ 0x3001, 0x3001, YM2203_write_port_1_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static int xainA_interrupt(void)
{
     xain_timer ^= 0x38;
     if (waitIRQA)
     { waitIRQA = 0;
       return (M6809_INT_IRQ);}
     return (M6809_INT_FIRQ | M6809_INT_NMI);
}

static int xainB_interrupt(void)
{
     if (waitIRQB)
     { waitIRQB = 0;
       return (M6809_INT_IRQ);}
     return ignore_interrupt();
}

static int xain_sound_interrupt(void)
{
     if (waitIRQsound)
     { waitIRQsound = 0;
       return (M6809_INT_IRQ);}
     return (M6809_INT_FIRQ);
}

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* DSW0 */
        PORT_DIPNAME( 0x80, 0x80, "Screen Invert", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")

        PORT_DIPNAME( 0x40, 0x40, "Screen Type", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off")

        PORT_DIPNAME( 0x20, 0x20, "Continue Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x20, "On")

        PORT_DIPNAME( 0x10, 0x10, "Demo Sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x10, "On")

        PORT_DIPNAME( 0x0c, 0x0c, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "2 coin 1 credit")
        PORT_DIPSETTING(    0x04, "1 coin 3 credit")
        PORT_DIPSETTING(    0x08, "1 coin 2 credit")
        PORT_DIPSETTING(    0x0c, "1 coin 1 credit")

        PORT_DIPNAME( 0x03, 0x03, "Coin B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "2 coin 1 credit")
        PORT_DIPSETTING(    0x01, "1 coin 3 credit")
        PORT_DIPSETTING(    0x02, "1 coin 2 credit")
        PORT_DIPSETTING(    0x03, "1 coin 1 credit")

	PORT_START	/* DSW1 */
        PORT_DIPNAME( 0xC0, 0xC0, "Number Lifes", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "No Limit")
        PORT_DIPSETTING(    0x40, "5")
        PORT_DIPSETTING(    0x80, "3")
        PORT_DIPSETTING(    0xC0, "2")

        PORT_DIPNAME( 0x30, 0x30, "Bonus Lifes", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "20000 70000 70000" )
	PORT_DIPSETTING(    0x20, "30000 80000 80000" )
	PORT_DIPSETTING(    0x10, "20000 80000" )
	PORT_DIPSETTING(    0x00, "30000 80000" )

        PORT_DIPNAME( 0x0c, 0x0c, "Game Time", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Very Fast")
        PORT_DIPSETTING(    0x04, "Fast")
        PORT_DIPSETTING(    0x08, "Normal")
        PORT_DIPSETTING(    0x0c, "Slow")

        PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Very Difficulty")
        PORT_DIPSETTING(    0x01, "Difficulty")
        PORT_DIPSETTING(    0x02, "Normal")
        PORT_DIPSETTING(    0x03, "Easy")
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	// 8*8 chars
	1024,	// 1024 characters
	4,	// 4 bits per pixel
	{ 0, 2, 4, 6 },	// plane offset
	{ 1, 0, 65, 64, 129, 128, 193, 192 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8	// every char takes 32 consecutive bytes
};

static struct GfxLayout tileslayout =
{
	16,16,	/* 8*8 chars */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
        { 0x8000*8+0, 0x8000*8+4, 0, 4 },	/* plane offset */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
          32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
          8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8	/* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* 8x8 text */
	{ 1, 0x00000, &charlayout,       0, 128 },

        /* 16x16 Background */
        { 1, 0x08000, &tileslayout,      256, 128 },
        { 1, 0x18000, &tileslayout,      256, 128 },
        { 1, 0x28000, &tileslayout,      256, 128 },
        { 1, 0x38000, &tileslayout,      256, 128 },

        /* 16x16 Background */
        { 1, 0x48000, &tileslayout,      384, 128 },
        { 1, 0x58000, &tileslayout,      384, 128 },
        { 1, 0x68000, &tileslayout,      384, 128 },
        { 1, 0x78000, &tileslayout,      384, 128 },

        /* Sprites */
        { 1, 0x88000, &tileslayout,      128, 128 },
        { 1, 0x98000, &tileslayout,      128, 128 },
        { 1, 0xa8000, &tileslayout,      128, 128 },
        { 1, 0xb8000, &tileslayout,      128, 128 },
	{ -1 } // end of array
};

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz (?) */      /* Real unknown */
	{ YM2203_VOL(100,0x20ff), YM2203_VOL(100,0x20ff) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MachineDriver xain_machine_driver =
{
	/* basic machine hardware */
	{
		{
 			CPU_M6809,
			2000000,	/* 2 Mhz (?) */
			0,
			readmem,writemem,0,0,
			xainA_interrupt,4      /* Number Unknown */
		},
		{
 			CPU_M6809,
			2000000,	/* 2 Mhz (?) */
			2,
			readmemB,writememB,0,0,
			xainB_interrupt,4      /* Number Unknown */
		},
		{
 			CPU_M6809 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz (?) */
			3,
			readmem_sound,writemem_sound,0,0,
			xain_sound_interrupt,16 /* Number Unknown */
		},

	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	4*60, 	/* 240 CPU slices per frame */
	xain_init_machine,

	/* video hardware */
	32*8, 32*8, { 0, 32*8, 8, 30*8 },
	gfxdecodeinfo,
	128*4,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT,
	0,
	xain_vh_start,
	xain_vh_stop,
	xain_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( xain_rom )
	ROM_REGION(0x14000)	/* 64k for code */
	ROM_LOAD( "1.ROM", 0x8000, 0x8000, 0x3ec33443 )
	ROM_LOAD( "2.ROM", 0x4000, 0x4000, 0xdb85edf1 )
	ROM_CONTINUE(           0x10000, 0x4000 )

	ROM_REGION(0xc8000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "12.ROM",  0x00000, 0x8000, 0x332dd307 ) /* Characters */
	ROM_LOAD( "21.ROM",  0x08000, 0x8000, 0xc5ffe72d ) /* Characters */
	ROM_LOAD( "13.ROM",  0x10000, 0x8000, 0x8149446f ) /* Characters */
	ROM_LOAD( "22.ROM",  0x18000, 0x8000, 0xd98cbc6e ) /* Characters */
	ROM_LOAD( "14.ROM",  0x20000, 0x8000, 0x9541a9e5 ) /* Characters */
	ROM_LOAD( "23.ROM",  0x28000, 0x8000, 0x8de2d670 ) /* Characters */
	ROM_LOAD( "15.ROM",  0x30000, 0x8000, 0x982dcaa3 ) /* Characters */
	ROM_LOAD( "24.ROM",  0x38000, 0x8000, 0xa6e371a3 ) /* Characters */
	ROM_LOAD( "16.ROM",  0x40000, 0x8000, 0xfc3769e7 ) /* Characters */

	ROM_LOAD( "6.ROM",  0x48000, 0x8000, 0xe557058d ) /* Characters */
	ROM_LOAD( "7.ROM",  0x50000, 0x8000, 0x89af64ff ) /* Characters */
	ROM_LOAD( "5.ROM",  0x58000, 0x8000, 0x547824e6 ) /* Characters */
	ROM_LOAD( "8.ROM",  0x60000, 0x8000, 0x16f96c2d ) /* Characters */
	ROM_LOAD( "4.ROM",  0x68000, 0x8000, 0x6ce496ca ) /* Characters */
	ROM_LOAD( "9.ROM",  0x70000, 0x8000, 0xfdac576e ) /* Characters */

	ROM_LOAD( "25.ROM",  0x88000, 0x8000, 0x5d057a13 ) /* Sprites */
	ROM_LOAD( "17.ROM",  0x90000, 0x8000, 0xceb1ebc9 ) /* Sprites */
	ROM_LOAD( "26.ROM",  0x98000, 0x8000, 0x08031ddd ) /* Sprites */
	ROM_LOAD( "18.ROM",  0xa0000, 0x8000, 0x778ecaa8 ) /* Sprites */
	ROM_LOAD( "27.ROM",  0xa8000, 0x8000, 0xd65c0e86 ) /* Sprites */
	ROM_LOAD( "19.ROM",  0xb0000, 0x8000, 0xb92747b1 ) /* Sprites */
	ROM_LOAD( "28.ROM",  0xb8000, 0x8000, 0xbc3c6520 ) /* Sprites */
	ROM_LOAD( "20.ROM",  0xc0000, 0x8000, 0x12ec4e96 ) /* Sprites */

	ROM_REGION(0x14000)	/* 64k for code */
	ROM_LOAD( "10.ROM", 0x8000, 0x8000, 0x27cac38c )
	ROM_LOAD( "11.ROM", 0x4000, 0x4000, 0x153a4f82 )
	ROM_CONTINUE(           0x10000, 0x4000 )

	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "3.ROM", 0x8000, 0x8000, 0x1e173e1f )
ROM_END

ROM_START( solarwar_rom )
	ROM_REGION(0x14000)	/* 64k for code */
	ROM_LOAD( "P9-0.BIN", 0x8000, 0x8000, 0xc3e68da0 )
	ROM_LOAD( "PA-0.BIN", 0x4000, 0x4000, 0x748e938e )
	ROM_CONTINUE(           0x10000, 0x4000 )

	ROM_REGION(0xc8000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "PB-01.BIN", 0x00000, 0x8000, 0x332dd307 ) /* Characters */
	ROM_LOAD( "PK-0.BIN",  0x08000, 0x8000, 0x7cff502d ) /* Characters */
	ROM_LOAD( "PC-0.BIN",  0x10000, 0x8000, 0x8149446f ) /* Characters */
	ROM_LOAD( "PL-0.BIN",  0x18000, 0x8000, 0xe88cb36e ) /* Characters */
	ROM_LOAD( "PD-0.BIN",  0x20000, 0x8000, 0x9541a9e5 ) /* Characters */
	ROM_LOAD( "PM-0.BIN",  0x28000, 0x8000, 0x81e22270 ) /* Characters */
	ROM_LOAD( "PE-0.BIN",  0x30000, 0x8000, 0x982dcaa3 ) /* Characters */
	ROM_LOAD( "PN-0.BIN",  0x38000, 0x8000, 0x624d8eeb ) /* Characters */
	ROM_LOAD( "PF-0.BIN",  0x40000, 0x8000, 0xa4919a67 ) /* Characters */

	ROM_LOAD( "P5-0.BIN",  0x48000, 0x8000, 0x24573a8d ) /* Characters */
	ROM_LOAD( "P6-0.BIN",  0x50000, 0x8000, 0xb9af54ff ) /* Characters */
	ROM_LOAD( "P4-0.BIN",  0x58000, 0x8000, 0xbd784de6 ) /* Characters */
	ROM_LOAD( "P7-0.BIN",  0x60000, 0x8000, 0x16f96c2d ) /* Characters */
	ROM_LOAD( "P3-0.BIN",  0x68000, 0x8000, 0x6be469ca ) /* Characters */
	ROM_LOAD( "P8-0.BIN",  0x70000, 0x8000, 0xfcaca86e ) /* Characters */

	ROM_LOAD( "PO-0.BIN",  0x88000, 0x8000, 0x5d057a13 ) /* Sprites */
	ROM_LOAD( "PG-0.BIN",  0x90000, 0x8000, 0xcdb1eac9 ) /* Sprites */
	ROM_LOAD( "PP-0.BIN",  0x98000, 0x8000, 0x08031ddd ) /* Sprites */
	ROM_LOAD( "PH-0.BIN",  0xa0000, 0x8000, 0x778ecaa8 ) /* Sprites */
	ROM_LOAD( "PQ-0.BIN",  0xa8000, 0x8000, 0xd65c0e86 ) /* Sprites */
	ROM_LOAD( "PI-0.BIN",  0xb0000, 0x8000, 0xb92747b1 ) /* Sprites */
	ROM_LOAD( "PR-0.BIN",  0xb8000, 0x8000, 0xbc3c6520 ) /* Sprites */
	ROM_LOAD( "PJ-0.BIN",  0xc0000, 0x8000, 0x12ec4e96 ) /* Sprites */

	ROM_REGION(0x14000)	/* 64k for code */
	ROM_LOAD( "P1-0.BIN", 0x8000, 0x8000, 0xddf55cd3 )
	ROM_LOAD( "P0-0.BIN", 0x4000, 0x4000, 0x7bc8ebda )
	ROM_CONTINUE(           0x10000, 0x4000 )

	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "P2-0.BIN", 0x8000, 0x8000, 0x2817281f )
ROM_END



struct GameDriver xsleena_driver =
{
	"Xain'd Sleena",
	"xsleena",
	"Carlos A. Lozano\nRob Rosenbrock\nPhil Stroffolino\n",
	&xain_machine_driver,

	xain_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver solarwar_driver =
{
	"Solar Warrior",
	"solarwar",
	"Carlos A. Lozano\nRob Rosenbrock\nPhil Stroffolino\n",
	&xain_machine_driver,

	solarwar_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

