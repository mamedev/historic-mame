/***************************************************************************

	BattleLane
	1986 Taito

	2x6809

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"


int battlane_vh_start(void);
void battlane_vh_stop(void);
void battlane_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern unsigned char *battlane_bitmap;
extern int battlane_bitmap_size;
extern unsigned char *battlane_spriteram;
extern int battlane_spriteram_size;

extern void battlane_bitmap_w(int, int);
extern int battlane_bitmap_r(int);
extern void battlane_video_ctrl_w(int, int);
extern int battlane_video_ctrl_r(int);


/*
BattleLane uses an unknown M6809 interrupt vector 0xfffa (this is actually the
SWI vector, so the following is completely wrong - NS).

Since the core doesn't support this, I will swap the FIRQ and unknown vectors
around (swapping keeps the self test happy)
*/

#define VECTOR_KLUDGE 1

#if VECTOR_KLUDGE
#define M6809_IRQ2_LINE M6809_FIRQ_LINE

void battlane_machine_init(void)
{
	unsigned char *SLAVERAM =
		Machine->memory_region[Machine->drv->cpu[1].memory_region];
	int b1, b2;
	b1=SLAVERAM[0xfff6];
	b2=SLAVERAM[0xfff7];
	SLAVERAM[0xfff6]=SLAVERAM[0xfffa];
	SLAVERAM[0xfff7]=SLAVERAM[0xfffb];
	SLAVERAM[0xfffa]=b1;
	SLAVERAM[0xfffb]=b2;
}
#else
void battlane_machine_init(void)
{
}
#endif

void battlane_shared_ram_w(int offset, int data)
{
	unsigned char *RAM =
		Machine->memory_region[Machine->drv->cpu[0].memory_region];
	RAM[offset]=data;
}

int battlane_shared_ram_r(int offset)
{
	unsigned char *RAM =
		Machine->memory_region[Machine->drv->cpu[0].memory_region];
	return RAM[offset];
}

static int battlane_cpu_control;

void cpu1_command_w(int offset, int data)
{
	battlane_cpu_control=data;
	/*if (errorlog)
	{
		fprintf(errorlog, "CPU# PC=%04x IRQ=%02x\n", cpu_get_pc(), data);
	}*/

	/*
	CPU control register

		0x80	= ???
		0x08	= ???
		0x04	= Unknown IRQ vector 0xfffa (1=Activate)
		0x02	= IRQ	(0=Activate)
		0x01	= ???
	*/


	/*
	No idea what is connected to the NMI lines yet
	*/
	//cpu_set_nmi_line(0,     data & 0x80 ? CLEAR_LINE : HOLD_LINE);
	//cpu_set_nmi_line(1,     data & 0x80 ? CLEAR_LINE : HOLD_LINE);


	/*
	Trigger an 6809 IRQ at vector 0xfffa
	*/
	cpu_set_irq_line(1, M6809_IRQ2_LINE,  (~data) & 0x04 ? CLEAR_LINE : HOLD_LINE);

	/*
	Slave function call (e.g. ROM test):
	FA7F: 86 03       LDA   #$03	; Function code
	FA81: 97 6B       STA   $6B
	FA83: 86 0E       LDA   #$0E
	FA85: 84 FD       ANDA  #$FD	; Trigger IRQ
	FA87: 97 66       STA   $66
	FA89: B7 1C 03    STA   $1C03	; Do Trigger
	FA8C: C6 40       LDB   #$40
	FA8E: D5 68       BITB  $68
	FA90: 27 FA       BEQ   $FA8C	; Wait for slave IRQ pre-function dispatch
	FA92: 96 68       LDA   $68
	FA94: 84 01       ANDA  #$01
	FA96: 27 FA       BEQ   $FA92	; Wait for bit to be set
	*/

	cpu_set_irq_line(1, M6809_IRQ_LINE,   data & 0x02 ? CLEAR_LINE : HOLD_LINE);
}

static struct MemoryReadAddress cpu1_readmem[] =
{
	{ 0x0000, 0x0fff, battlane_shared_ram_r },
	{ 0x1000, 0x17ff, MRA_RAM }, /* Tile RAM  */
	{ 0x1800, 0x18ff, MRA_RAM },
	{ 0x1c00, 0x1c00, input_port_0_r },
	{ 0x1c01, 0x1c01, input_port_1_r },
	{ 0x1c02, 0x1c02, input_port_2_r },
	{ 0x1c03, 0x1c03, input_port_3_r },
	{ 0x1c04, 0x1c04, YM3526_status_port_0_r },
	{ 0x2000, 0x3fff, battlane_bitmap_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress cpu1_writemem[] =
{
	{ 0x0000, 0x0fff, battlane_shared_ram_w },
	{ 0x1000, 0x17ff, MWA_RAM },  /* Tile RAM  */
	{ 0x1800, 0x18ff, MWA_RAM, &battlane_spriteram, &battlane_spriteram_size},
	{ 0x1c00, 0x1c00, battlane_video_ctrl_w },
	{ 0x1c03, 0x1c03, cpu1_command_w },
	{ 0x1c04, 0x1c04, YM3526_control_port_0_w },
	{ 0x1c05, 0x1c05, YM3526_write_port_0_w },
	{ 0x1e00, 0x1e3f, MWA_RAM }, /* Palette ??? */
	{ 0x2000, 0x3fff, battlane_bitmap_w, &battlane_bitmap, &battlane_bitmap_size },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


int battlane_cpu1_interrupt(void)
{

#ifdef MAME_DEBUG
	if (keyboard_pressed(KEYCODE_F))
	{
		FILE *fp;
		while (keyboard_pressed(KEYCODE_F)) ;
		fp=fopen("RAM.DMP", "w+b");
		if (fp)
		{
			unsigned char *RAM =
			Machine->memory_region[Machine->drv->cpu[0].memory_region];

			fwrite(RAM, 0x4000, 1, fp);
			fclose(fp);
		}
	}
#endif


	if (cpu_getiloops()!=0)
	{
		return interrupt();
	}
	else
	{
		/*
		Hit N like crazy to get the demo loop running. Both must be triggered
		at the same time since the slave NMI waits for the master to complete.
		NMI is used to update the sprites.
		*/
		if (keyboard_pressed(KEYCODE_N))
		{
			static int NMI=0;
			NMI=~NMI;
			cpu_set_nmi_line(0,  NMI ? CLEAR_LINE : HOLD_LINE);
			cpu_set_nmi_line(1,  NMI ? CLEAR_LINE : HOLD_LINE);
		}


		/*
		FIRQ seems to drive the music & coin inputs. I have no idea what it is
		attached to
		*/

		return M6809_INT_FIRQ;
	}
}

int battlane_cpu2_interrupt(void)
{
	/* CPU2's interrupts are generated on-demand by CPU1 */
	return ignore_interrupt();
}


INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )	/* VBLank ? */

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C )  )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Easy"  )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard"  )
	PORT_DIPSETTING(    0x00, "Very Hard" )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX(0,        0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "20k & every 50k" )
	PORT_DIPSETTING(    0x08, "20k & every 70k" )
	PORT_DIPSETTING(    0x04, "20k & every 90k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )
INPUT_PORTS_END

static struct GfxLayout spritelayout =
{
	16,16,    /* 16*16 sprites */
	512,    /* ??? sprites */
	6,      /* 6 bits per pixel ??!!! */
    { 0, 8, 0x08000*8,0x08000*8+8, 0x10000*8, 0x10000*8+8},
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
	  0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8,
	  16*8*2+0*8, 16*8*2+2*8, 16*8*2+4*8, 16*8*2+6*8,
	  16*8*2+8*8, 16*8*2+10*8, 16*8*2+12*8, 16*8*2+14*8},
	16*8*2*2     /* every char takes 16*8 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16 ,    /* 16*16 tiles */
	256,    /* 256 tiles */
	3,      /* 3 bits per pixel */
        {  0x8000*8+4, 4, 0},    /* plane offset */
	{
	16+8+0, 16+8+1, 16+8+2, 16+8+3,
	16+0, 16+1, 16+2,   16+3,
	8+0,    8+1,    8+2,    8+3,
	0,       1,    2,      3,
	},
	{ 0*8*4, 1*8*4,  2*8*4,  3*8*4,  4*8*4,  5*8*4,  6*8*4,  7*8*4,
	  8*8*4, 9*8*4, 10*8*4, 11*8*4, 12*8*4, 13*8*4, 14*8*4, 15*8*4
	},
	8*8*4*2     /* every char takes consecutive bytes */
};

static struct GfxLayout tilelayout2 =
{
	16,16 ,    /* 16*16 tiles */
	256,    /* 256 tiles */
	3,      /* 3 bits per pixel */
	{ 0x4000*8, 0x4000*8+4, 0x8000*8 },    /* plane offset */
	{
	16+8+0, 16+8+1, 16+8+2, 16+8+3,
	16+0, 16+1, 16+2,   16+3,
	8+0,    8+1,    8+2,    8+3,
	0,       1,    2,      3,
	},
	{ 0*8*4, 1*8*4,  2*8*4,  3*8*4,  4*8*4,  5*8*4,  6*8*4,  7*8*4,
	  8*8*4, 9*8*4, 10*8*4, 11*8*4, 12*8*4, 13*8*4, 14*8*4, 15*8*4
	},
	8*8*4*2     /* every char takes consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { 1, 0x00000, &spritelayout,     0, 32},
    { 1, 0x18000, &tilelayout,       8, 32},
    { 1, 0x18000, &tilelayout2,      8, 32},
	{ -1 } /* end of array */
};

static struct YM3526interface ym3526_interface =
{
    1,              /* 1 chip (no more supported) */
    3600000,        /* 3.600000 MHz ? (partially supported) */
    { 255 }         /* (not supported) */
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,        /* 1.25 Mhz ? */
			0,
			cpu1_readmem,cpu1_writemem,0,0,
            battlane_cpu1_interrupt,3
		},
		{
			CPU_M6809,
			1250000,        /* 1.25 Mhz ? */
			2,      /* memory region #2 */
			cpu1_readmem,cpu1_writemem,0,0,
            battlane_cpu2_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
    1,      /* 3 CPU slices per frame */
	battlane_machine_init,      /* init machine */

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 1*8, 31*8-1 },       /* not sure */
	gfxdecodeinfo,
        0x40,0x40,
        NULL,

        VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	battlane_vh_start,
	battlane_vh_stop,
	battlane_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
	    {
			SOUND_YM3526,
			&ym3526_interface,
	    }
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( battlane_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	/* first half of da00-5 will be copied at 0x4000-0x7fff */
	ROM_LOAD( "da01-5",    0x8000, 0x8000, 0x7a6c3f02 )

	ROM_REGION_DISPOSE(0x24000)     /* temporary space for graphics */
	ROM_LOAD( "da05",      0x00000, 0x8000, 0x834829d4 ) /* Sprites Plane 1+2 */
	ROM_LOAD( "da04",      0x08000, 0x8000, 0xf083fd4c ) /* Sprites Plane 3+4 */
	ROM_LOAD( "da03",      0x10000, 0x8000, 0xcf187f25 ) /* Sprites Plane 5+6 */

	ROM_LOAD( "da06",      0x18000, 0x8000, 0x9c6a51b3 ) /* Tiles*/
	ROM_LOAD( "da07",      0x20000, 0x4000, 0x56df4077 ) /* Tiles*/

	ROM_REGION(0x10000)     /* 64K for slave CPU */
	ROM_LOAD( "da00-5",    0x00000, 0x8000, 0x85b4ed73 )	/* ...second half goes here */
	ROM_LOAD( "da02-2",    0x08000, 0x8000, 0x69d8dafe )

	ROM_REGION(0x0040)     /* PROMs (function unknown) */
	ROM_LOAD( "82s123.7h", 0x00000, 0x0020, 0xb9933663 )
	ROM_LOAD( "82s123.9n", 0x00020, 0x0020, 0x06491e53 )
ROM_END

ROM_START( battlan2_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	/* first half of da00-3 will be copied at 0x4000-0x7fff */
	ROM_LOAD( "da01-3",    0x8000, 0x8000, 0xd9e40800 )

	ROM_REGION_DISPOSE(0x24000)     /* temporary space for graphics */
	ROM_LOAD( "da05",      0x00000, 0x8000, 0x834829d4 ) /* Sprites Plane 1+2 */
	ROM_LOAD( "da04",      0x08000, 0x8000, 0xf083fd4c ) /* Sprites Plane 3+4 */
	ROM_LOAD( "da03",      0x10000, 0x8000, 0xcf187f25 ) /* Sprites Plane 5+6 */

	ROM_LOAD( "da06",      0x18000, 0x8000, 0x9c6a51b3 ) /* Tiles*/
	ROM_LOAD( "da07",      0x20000, 0x4000, 0x56df4077 ) /* Tiles*/

	ROM_REGION(0x10000)     /* 64K for slave CPU */
	ROM_LOAD( "da00-3",    0x00000, 0x8000, 0x7a0a5d58 )
	ROM_LOAD( "da02-2",    0x08000, 0x8000, 0x69d8dafe )

	ROM_REGION(0x0040)     /* PROMs (function unknown) */
	ROM_LOAD( "82s123.7h", 0x00000, 0x0020, 0xb9933663 )
	ROM_LOAD( "82s123.9n", 0x00020, 0x0020, 0x06491e53 )
ROM_END

ROM_START( battlan3_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	/* first half of bl_04.rom will be copied at 0x4000-0x7fff */
	ROM_LOAD( "bl_05.rom", 0x8000, 0x8000, 0x001c4bbe )

	ROM_REGION_DISPOSE(0x24000)     /* temporary space for graphics */
	ROM_LOAD( "da05",      0x00000, 0x8000, 0x834829d4 ) /* Sprites Plane 1+2 */
	ROM_LOAD( "da04",      0x08000, 0x8000, 0xf083fd4c ) /* Sprites Plane 3+4 */
	ROM_LOAD( "da03",      0x10000, 0x8000, 0xcf187f25 ) /* Sprites Plane 5+6 */

	ROM_LOAD( "da06",      0x18000, 0x8000, 0x9c6a51b3 ) /* Tiles*/
	ROM_LOAD( "da07",      0x20000, 0x4000, 0x56df4077 ) /* Tiles*/

	ROM_REGION(0x10000)     /* 64K for slave CPU */
	ROM_LOAD( "bl_04.rom", 0x00000, 0x8000, 0x5681564c )	/* ...second half goes here */
	ROM_LOAD( "da02-2",    0x08000, 0x8000, 0x69d8dafe )

	ROM_REGION(0x0040)     /* PROMs (function unknown) */
	ROM_LOAD( "82s123.7h", 0x00000, 0x0020, 0xb9933663 )
	ROM_LOAD( "82s123.9n", 0x00020, 0x0020, 0x06491e53 )
ROM_END



static void battlane_decode(void)
{
	unsigned char *src,*dest;
	int A;

	/* no encryption, but one ROM is shared among two CPUs. We loaded it into the */
	/* second CPU address space, let's copy it to the first CPU's one */
	src = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	dest = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	for(A = 0;A < 0x4000;A++)
		dest[A + 0x4000] = src[A];
}



struct GameDriver battlane_driver =
{
	__FILE__,
	0,
	"battlane",
	"Battle Lane Vol. 5 (set 1)",
	"1986",
	"Technos (Taito license)",
	"Paul Leaman\nKim Greenblatt",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	battlane_rom,
	battlane_decode, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver battlan2_driver =
{
	__FILE__,
	&battlane_driver,
	"battlan2",
	"Battle Lane Vol. 5 (set 2)",
	"1986",
	"Technos (Taito license)",
	"Paul Leaman\nKim Greenblatt",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	battlan2_rom,
	battlane_decode, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver battlan3_driver =
{
	__FILE__,
	&battlane_driver,
	"battlan3",
	"Battle Lane Vol. 5 (set 3)",
	"1986",
	"Technos (Taito license)",
	"Paul Leaman\nKim Greenblatt",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	battlan3_rom,
	battlane_decode, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};
