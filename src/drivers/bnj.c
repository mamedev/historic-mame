/***************************************************************************

Bump 'n' Jump memory map (preliminary)

MAIN BOARD:

0500-0640 Hi Scores
a000-ffff ROM

read:
1000      DSW1
1001      DSW2
1002      IN0
1003      IN1
1004      coin

write

IN0  Player 1 Joystick
7\
6 |
5 |
4 |  Jump
3 |  Down
2 |  Up
1 |  Left
0/   Right

IN1  Player 2 Joystick (TABLE only)
7\
6 |
5 |
4 |  Jump
3 |  Down
2 |  Up
1 |  Left
0/   Right

Coin slot
7\   Coin Right side
6 |  Coin Left side
5 |
4 |  Player 2 start
3 |  Player 1 start
2 |
1 |
0/

DSW1
7    Screen type.
6    Control type.
5\
4/   Diagnostics.
3\
2/   Credit base for slot 2
1\
0/   Credit base for slot 1

DSW2
7
6
5
4    Difficulty
3    Normal/Continue
2\
1/   Bonus life.
0    Number of lives.


interrupts:
A NMI causes coin check.


SOUND BOARD:
0000-03ff RAM
f000-ffff ROM

read:
a000      command from CPU board / interrupt acknowledge

write:
2000      8910 #1  write
4000      8910 #1  control
6000      8910 #2  write
8000      8910 #2  control
c000      NMI enable

interrupts (Probably the same as Burgertime):
NMI used for timing (music etc), clocked at (I think) 8V
IRQ triggered by commands sent by the main CPU.

***************************************************************************/


/*
 *  Required Headers.
 */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



/*
 *  Externals.
 */

extern void               bnj_background_w     (int offset,int data);
extern int                bnj_vh_start         (void);
extern void               bnj_vh_stop          (void);
extern void               bnj_vh_screenrefresh (struct osd_bitmap *bitmap);
extern void               bnj_bg_w             (int offset, int data);
extern unsigned char     *bnj_bgram;
extern int                bnj_bgram_size;
extern unsigned char     *bnj_scroll1;
extern unsigned char     *bnj_scroll2;

void btime_paletteram_w(int offset,int data);
int btime_mirrorvideoram_r(int offset);
int btime_mirrorcolorram_r(int offset);
void btime_mirrorvideoram_w(int offset,int data);
void btime_mirrorcolorram_w(int offset,int data);
void btime_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

void btime_sh_interrupt_enable_w(int offset,int data);
int  btime_sh_interrupt(void);
int  bnj_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x1000, input_port_3_r },	/* DSW1 */
	{ 0x1001, 0x1001, input_port_4_r },	/* DSW2 */
	{ 0x1002, 0x1002, input_port_0_r },	/* IN0 */
	{ 0x1003, 0x1003, input_port_1_r },	/* IN1 */
	{ 0x1004, 0x1004, input_port_2_r },	/* coin */
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x4800, 0x4bff, btime_mirrorvideoram_r },
	{ 0x4c00, 0x4fff, btime_mirrorcolorram_r },
	{ 0xa000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1002, 0x1002, sound_command_w },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, colorram_w, &colorram },
	{ 0x4800, 0x4bff, btime_mirrorvideoram_w },
	{ 0x4c00, 0x4fff, btime_mirrorcolorram_w },
	{ 0x5000, 0x51ff, bnj_bg_w, &bnj_bgram, &bnj_bgram_size },
	{ 0x5400, 0x5400, MWA_RAM, &bnj_scroll1 },
	{ 0x5800, 0x5800, MWA_RAM, &bnj_scroll2 },
	{ 0x5c00, 0x5c0f, btime_paletteram_w },
	{ 0xa000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ 0xa000, 0xa000, sound_command_latch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2000, AY8910_write_port_0_w },
	{ 0x4000, 0x4000, AY8910_control_port_0_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x8000, 0x8000, AY8910_control_port_1_w },
	{ 0xc000, 0xc000, btime_sh_interrupt_enable_w },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



/***************************************************************************

  Bump 'n Jump doesn't have VBlank interrupts.
  Interrupts are still used by the game, coin insertion generates a NMI.

***************************************************************************/
int bnj_interrupt(void)
{
	static int coin;


	if ((readinputport(2) & 0xc0) != 0xc0)	/* Coin */
	{
		if (coin == 0)
		{
			coin = 1;
			return nmi_interrupt();
		}
		else return ignore_interrupt();
	}
	else
	{
		coin = 0;
		return ignore_interrupt();
	}
}



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* DSW1 */
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
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START	/* DSW2 */
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "every 30k" )
	PORT_DIPSETTING(    0x04, "every 70k" )
	PORT_DIPSETTING(    0x02, "20k only"  )
	PORT_DIPSETTING(    0x00, "30k only"  )
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x10, 0x10, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 256*16*16, 2*256*16*16 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 characters */
	64,    /* 64 characters */
	3,	/* 3 bits per pixel */
    { 2*64*16*16+4, 0, 4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
    { 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
			2*16*8+3, 2*16*8+2, 2*16*8+1, 2*16*8+0, 3*16*8+3, 3*16*8+2, 3*16*8+1, 3*16*8+0 },
	64*8	/* every tile takes 64 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },	/* char set #1 */
	{ 1, 0x0000, &spritelayout, 0, 1 },	/* sprites */
	{ 1, 0x6000, &tilelayout,   8, 1 },	/* background tiles */
	{ -1 } /* end of array */
};



/*
 *  The machine specification.
 */
static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,    /* 1.5 Mhz ???? */
			0,
			readmem,writemem,0,0,
			bnj_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1000000,	/* 1.000 khz */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			btime_sh_interrupt,30	/* / 2 = 15 (??) NMI interrupts per frame */
		}
	},
	60,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	16,2*8,
	btime_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	bnj_vh_start,
	bnj_vh_stop,
	bnj_vh_screenrefresh,

	/* sound hardware */
	0,
	bnj_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bnj_rom )
    ROM_REGION(0x10000)	/* 64k for code */
    ROM_LOAD( "bnj12b.bin", 0xa000, 0x2000, 0x9ce25062 )
    ROM_LOAD( "bnj12c.bin", 0xc000, 0x2000, 0x9e763206 )
    ROM_LOAD( "bnj12d.bin", 0xe000, 0x2000, 0xc5a135b9 )

    ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "bnj4h.bin",  0x0000, 0x2000, 0xc141332f )
    ROM_LOAD( "bnj4f.bin",  0x2000, 0x2000, 0x5035d3c9 )
    ROM_LOAD( "bnj4e.bin",  0x4000, 0x2000, 0x97b719fb )
    ROM_LOAD( "bnj10e.bin", 0x6000, 0x1000, 0x43556767 )
    ROM_LOAD( "bnj10f.bin", 0x7000, 0x1000, 0xc1bfbfbf )

    ROM_REGION(0x10000)	/* 64k for the audio CPU */
    ROM_LOAD( "bnj6c.bin",  0xf000, 0x1000, 0x80f9e12b )
ROM_END

ROM_START( brubber_rom )
    ROM_REGION(0x10000)	/* 64k for code */
	/* a000-bfff space for the service ROM */
    ROM_LOAD( "brubber.12c", 0xc000, 0x2000, 0x2e85db11 )
    ROM_LOAD( "brubber.12d", 0xe000, 0x2000, 0x1d905348 )

    ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "brubber.4h",  0x0000, 0x2000, 0xc141332f )
    ROM_LOAD( "brubber.4f",  0x2000, 0x2000, 0x5035d3c9 )
    ROM_LOAD( "brubber.4e",  0x4000, 0x2000, 0x97b719fb )
    ROM_LOAD( "brubber.10e", 0x6000, 0x1000, 0x43556767 )
    ROM_LOAD( "brubber.10f", 0x7000, 0x1000, 0xc1bfbfbf )

    ROM_REGION(0x10000)	/* 64k for the audio CPU */
    ROM_LOAD( "brubber.6c",  0xf000, 0x1000, 0x80f9e12b )
ROM_END



static void bnj_decode (void)
{
    int A;


    for (A = 0;A < 0x10000;A++)
    {
        /*
         *  The encryption is dead simple.  Just swap bits 5 & 6 for opcodes
         *  only.
         */
        ROM[A] =
            ((RAM[A] & 0x40) >> 1) | ((RAM[A] & 0x20) << 1) | (RAM[A] & 0x9f);
    }
}



static int hiload(void)
{
    /*
     *   Get the RAM pointer (this game is multiCPU so we can't assume the
     *   global RAM pointer is pointing to the right place)
     */
    unsigned char *RAM = Machine->memory_region[0];


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
    else
    {
        /*
         *  The hi scores can't be loaded yet.
         */
        return 0;
    }
}

static void hisave(void)
{
    void *f;

    /*
     *   Get the RAM pointer (this game is multiCPU so we can't assume the
     *   global RAM pointer is pointing to the right place)
     */
    unsigned char *RAM = Machine->memory_region[0];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
	    osd_fwrite(f,&RAM[0x0500],640);
	    osd_fclose(f);
    }
}



struct GameDriver bnj_driver =
{
    "Bump 'n Jump",
    "bnj",
    "Kevin Brisley (MAME driver)\nMirko Buffoni (Audio/Add. code)",
    &machine_driver,

    bnj_rom,
    0, bnj_decode,
    0,
    0,	/* sound_prom */

    0/*TBR*/, input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

    0, 0, 0,
    ORIENTATION_DEFAULT,

    hiload, hisave
};

struct GameDriver brubber_driver =
{
    "Burnin' Rubber",
    "brubber",
    "Kevin Brisley (MAME driver)\nMirko Buffoni (Audio/Add. code)",
    &machine_driver,

    brubber_rom,
    0, bnj_decode,
    0,
    0,	/* sound_prom */

    0/*TBR*/, input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

    0, 0, 0,
    ORIENTATION_DEFAULT,

    hiload, hisave
};
