/***************************************************************************

Halley's Comet, 1986 Taito

Notes:
	- video hardware uses independent two bitmapped layers
	- source graphics can be 2bpp or 4bpp


CPU:	MC68B09EP Z80A(SOUND)
Sound:	YM2149F x 4
OSC:	19.968MHz 6MHz

J1100021	sound daughterboard

Z80A	6MHz
2016
A62-13
A62-14
YM2149
YM2149
YM2149

J1100018

	  sw1		sw2  sw3  sw4

          A62-2  A62-15
68B09EP   A62-1
                 A62-4
                 2016    YM2149
                 2016

J1100020

 4116 4116 4116 4116     MB15511
 4116 4116 4116 4116
 4116 4116 4116 4116
 4116 4116 4116 4116
 4116 4116 4116 4116     MB15512
 4116 4116 4116 4116
 4116 4116 4116 4116
 4116 4116 4116 4116

 MB15510  MB15510
                          2016
                          2016

J1100019


                 A62-9  A62-10
 A26-13          A62-11 A62-12
 A26-13          A62-5  A62-16
 A26-13          A62-7  A62-8



                                 HALLEY'S COMET
                                   Taito 1986

                          Pinout and DIP switch settings
                    typed in by Pelle Einarsson, pelle@mensa.se

NOTE: I had to guess the meaning of some Japanese text concerning the DIP
      switch settings, but I think I got it right.

      * = default settings


              T                                       G
    PARTS          SOLDER                  PARTS              SOLDER
----------------------------------    ---------------------------------------
       GND   1 A   GND                         GND   1 A   GND
       GND   2 B   GND                               2 B
       GND   3 C   GND                               3 C
       GND   4 D   GND                               4 D
 Video GND   5 E   Video GND            Sound out+   5 E   Sound Out -
Video sync   6 F   Video sync                 Post   6 F   Post
      Post   7 H   Post                              7 H   Power on reset
   Video R   8 J   Video R               Coin sw A   8 J   Coin sw B
   Video G   9 K   Video G          Coin counter A   9 K   Coin counter B
   Video B  10 L   Video B          Coin lockout A  10 L   Coin lockout B
       -5V  11 M   -5V                  Service sw  11 M   Tilt sw
       -5V  12 N   -5V                    Select 1  12 N   Select 2
      +12V  13 P   +12V                      1P up  13 P   2P up
      +12V  14 R   +12V                    1P down  14 R   2P down
       +5V  15 S   +5V                    1P right  15 S   2P right
       +5V  16 T   +5V                     1P left  16 T   2P left
       +5V  17 U   +5V                              17 U
       +5V  18 V   +5V                              18 V
                                                    19 W
                                                    20 X
      H                                   1P shoot  21 Y   2p shoot
   --------                          1P hyperspace  22 Z   2P hyperspace
    1  GND
    2  GND
    3  GND
    4  GND
    5  +5V
    6  +5V
    7  +5V
    8  -5V
    9  +13V (a typo?)            DIP switch 1                Option    1-5   6
    10  Post                     ----------------------------------------------
    11  +12V                     (single/dual controls?)    *1-way     off  on
    12  +12V                                                 2-way          off



DIP switch 2      Option            1    2    3    4    5    6    7    8
-----------------------------------------------------------------------------
Free play         *off             off  off                 off       off
                   on                   on
-----------------------------------------------------------------------------
Start round        *1                        off  off  off
                    4                        on
                    7                        off  on
                   10                        on
                   13                        off  off  on
                   16                        on
                   19                        off  on
                   22                        on
-----------------------------------------------------------------------------
Collision         Normal                                        off
detection         No hit                                        on
-----------------------------------------------------------------------------




DIP switch 3      Option            1    2    3    4    5    6    7    8
-----------------------------------------------------------------------------
Difficulty           B             off  off                      off  off
                     A             on
                    *C             off  on
                     D             on
-----------------------------------------------------------------------------
Bonus ships  20000/60000/680000              off  off
             20000/80000/840000              on
            *20000/100000/920000             off  on
             10000/50000/560000              on
-----------------------------------------------------------------------------
Ships at start      *3                                 off  off
                     2                                 on
                     4                                 off  on
                     4 with no bonus ships             on
-----------------------------------------------------------------------------



DIP switch 4      Option            1    2    3    4    5    6    7    8
-----------------------------------------------------------------------------
Cabinet type      Table            off
                  Upright          on
-----------------------------------------------------------------------------
Screen flip                             off
                                        on
-----------------------------------------------------------------------------
Test mode                                    off
                                             on
-----------------------------------------------------------------------------
Demo sound                                   off
                                                  on
-----------------------------------------------------------------------------
Coin A           1 coin / 1 credit                     off  off
                 1 coin / 2 credits                    on
                 2 coins / 1 credit                    off  on
                 2 coins / 3 credits                   on
-----------------------------------------------------------------------------
Coin B           1 coin / 1 credit                               off  off
                 1 coin / 2 credits                              on
                 2 coins / 1 credit                              off  on
                 2 coins / 3 credits                             on
-----------------------------------------------------------------------------

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"

struct mame_bitmap *tmpbitmap2;
static int sndnmi_disable = 1;
static data8_t *blitter_ram;

PALETTE_INIT( halleys )
{
	int i;

	for (i = 0; i < 32; i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* blue component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* red component */
		bit0 = (color_prom[i] >> 6) & 0x01;
		bit1 = (color_prom[i] >> 7) & 0x01;
		b = 0x55 * bit0 + 0xaa * bit1;

		palette_set_color(i,r,g,b);
	}
} /* halleys */

VIDEO_START( halleys )
{
	tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmap2 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	if( tmpbitmap && tmpbitmap2 )
	{
		return 0;
	}
	return -1; /* error */
} /* halleys */


VIDEO_UPDATE( halleys )
{
	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
	copybitmap(bitmap,tmpbitmap2,0,0,0,0,cliprect,TRANSPARENCY_PEN,0);

	/* There are several busy loops that poll 0xff0b, expecting it to increment.
	 * I haven't found any interrupt code which writes to 0xff0b, so do change it
	 * here as a side effect of each vblank.
	 */
	memory_region( REGION_CPU1 )[0xff0b]++;
} /* halleys */





static READ_HANDLER( vector_r )
{
	/*
	there are two vector arrays, one at ffe0 the other at fff0. Which vector
	to pick depends on dip switches and additional logic.

	halleys:
	-------
	address    vector
	fff0       effd/effd  always 1
	fff2       eff0/eff0  dipswitch 1
	fff4       effd/effd  dipswitch 2
	fff6 FIRQ  eff4/eff7  unknown source
	fff8 IRQ   eff1/effd  unknown source but always 0 due to additional logic
	fffa       effd/effd  dipswitch 3
	fffc NMI   effa/effa  dipswitch 4
	fffe RESET effd/effd  dipswitch 5

	The only one we have to worry about is FIRQ. The two possible vectors point
	to the different entry points for the same routine: one inverts FF9E, the
	other doesn't.

	benberop:
	--------
	the only vector that varies is the one for IRQ (and both routines do very
	little).
	*/
	int vector_base = 0xffe0;

	if ((offset & ~1) == 6)
	{
		if (rand()&1) vector_base = 0xfff0;
	}

	return memory_region(REGION_CPU1)[vector_base + offset];
} /* vector_r */

static int
nthbit( const data8_t *pMem, int offs )
{
	pMem += offs/8;
	if( ((*pMem)<<(offs&7))&0x80 ) return 1;
	return 0;
} /* nthbit */

static void
blit( const data8_t *pMem )
{
	struct mame_bitmap *pDest;
	const data8_t *pGfx1 = memory_region( REGION_GFX1 );
	const data8_t *pGfx2 = &pGfx1[0x10000];

//	pMem[0x5]	?
//	pMem[0x6]	color
//	pMem[0x7]	bit 0x80 selects layer to draw to

	int y		= pMem[0x8];
	int x		= pMem[0x9];
	int w		= pMem[0xa];
	int h		= pMem[0xb];
	int bitOffs = pMem[0xf]+pMem[0xe]*256;

	int xpos,ypos,pen;
	int clut[4] = { 0x0,0xf,0xe,0x6 };
	int code;

	bitOffs &= 0x3fff; /* ? */
	code = pMem[0x7];
	if( code&0x80 )
	{
		pDest = tmpbitmap;
	}
	else
	{
		pDest = tmpbitmap2;
	}

	if( bitOffs ) bitOffs = 8*(0x10000 - bitOffs);

	for( ypos=0; ypos<h; ypos++ )
	{
		for( xpos=0; xpos<w; xpos++ )
		{
			pen = 0;
			if( bitOffs )
			{
				--bitOffs;
				if( nthbit( pGfx1, bitOffs ) ) pen+=1;
				if( nthbit( pGfx2, bitOffs ) ) pen+=2;
			}
			plot_pixel( pDest, (x+xpos)&0xff, (y+ypos)&0xff, clut[pen] );
		}
	}
} /* blit */

static WRITE_HANDLER( blitter_w )
{
	int pc = activecpu_get_pc();

	blitter_ram[offset] = data;
	if( pc==0x8025 ) return; /* return if this is part of the memory wipe on startup */

	if( (offset&0xf)==0 )
	{
		blit( &blitter_ram[offset] );

		logerror( "%04x BLT[%04x]: %02x:%02x:%02x y=%02x x=%02x w=%02x h=%02x %02x%02x.%02x%02x\n",
			activecpu_get_pc(),
			offset,
			blitter_ram[offset+0x5],
			blitter_ram[offset+0x6],
			blitter_ram[offset+0x7],
			blitter_ram[offset+0x8],
			blitter_ram[offset+0x9],
			blitter_ram[offset+0xa],
			blitter_ram[offset+0xb],
			blitter_ram[offset+0xc],
			blitter_ram[offset+0xd],
			blitter_ram[offset+0xe],
			blitter_ram[offset+0xf] );
	}
} /* blitter_w */

static READ_HANDLER( zero_r )
{
	if( activecpu_get_pc()==0x8017 )
	{ /* hack; trick SRAM test on startup */
		return 0x55;
	}
	return 0x00;
} /* zero_r */

static READ_HANDLER( unk_r )
{
	return rand();
} /* unk_r */

static READ_HANDLER( mirror_r )
{
	switch( offset )
	{
	case 0: return readinputport(3);
	case 1: return readinputport(4);
	case 2: return readinputport(5);
	default: return 0x00;
	}
} /* mirror_r */



static WRITE_HANDLER( halleys_nmi_line_clear_w )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
}



static WRITE_HANDLER( halleys_sndnmi_msk_w )
{
	sndnmi_disable = data & 0x01;
}

static WRITE_HANDLER( halleys_soundcommand_w )
{
	soundlatch_w(offset,data);
	/*if (!sndnmi_disable)*/ cpu_set_irq_line(1,IRQ_LINE_NMI,ASSERT_LINE);
}
static READ_HANDLER( halleys_soundcommand_r )
{
	cpu_set_irq_line(1,IRQ_LINE_NMI,CLEAR_LINE);
	return soundlatch_r(offset);
}

static WRITE_HANDLER( bank_control_w )
{
	memory_region( REGION_CPU1 )[0xff8d] = data;	/*store in RAM*/
	logerror("bank_ctrl_w: data=%2x\n",data);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x0fff, MRA_RAM },		/* blitter RAM */
	{ 0x1000, 0xefff, MRA_ROM },
	{ 0xf000, 0xfeff, MRA_RAM },		/* work ram */
	/* note that SRAM is tested from 0xf000 to 0xff7f,
	 * but several of the addresses in 0xff00..0xff7f
	 * behave as ports during normal game operation.
	 */


	{ 0xff16, 0xff16, zero_r },			/* blitter busy */
	{ 0xff71, 0xff71, zero_r },			/* blitter busy */


	{ 0xff80, 0xff83, mirror_r },
	{ 0xff8b, 0xff8b, MRA_RAM },		/* IRQ enable */

	{ 0xff90, 0xff90, input_port_3_r }, /* coin/start */
	{ 0xff91, 0xff91, input_port_4_r }, /* player 1 */
	{ 0xff92, 0xff92, input_port_5_r }, /* player 2 */
	{ 0xff93, 0xff93, zero_r },
	{ 0xff94, 0xff94, unk_r }, // important!
		// 0x01 ?
		// 0x02 ?
		// 0x04 ?
		// 0x08 ?
	{ 0xff95, 0xff95, input_port_0_r }, /* dipswitch 4 */
	{ 0xff96, 0xff96, input_port_1_r }, /* dipswitch 3 */
	{ 0xff97, 0xff97, input_port_2_r }, /* dipswitch 2 */

	{ 0xff00, 0xffef, MRA_RAM },	/* RAM - all other addresses (comment this line for debugging) */

	{ 0xfff0, 0xffff, vector_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0fff, blitter_w, &blitter_ram },
	{ 0x1000, 0xefff, MWA_ROM },
	{ 0xf000, 0xfeff, MWA_RAM }, /* work ram */
	/* note that SRAM is tested from 0xf000 to 0xff7f,
	 * but several of the addresses in 0xff00..0xff7f
	 * behave as ports during normal game operation.
	 */

	{ 0xff00, 0xff7f, MWA_RAM },

	//{ 0xff72, 0xff72, MWA_RAM },
	//{ 0xff76, 0xff76, watchdog_w },

	{ 0xff8a, 0xff8a, halleys_soundcommand_w },

	{ 0xff8d, 0xff8d, bank_control_w },	//bank AND ram simulateously

	{ 0xffa2, 0xffa2, MWA_RAM }, // bg scrolly?
	{ 0xffa3, 0xffa3, MWA_RAM }, // bg scrollx?
	{ 0xff98, 0xff98, MWA_RAM },		// IRQ ack
	{ 0xff9b, 0xff9b, halleys_nmi_line_clear_w },	// NMI line reset
	{ 0xff9c, 0xff9c, MWA_RAM },			//FIRQ line reset
	{ 0xff9e, 0xff9e, MWA_RAM }, // flipscreen
	{ 0xfff0, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x4801, 0x4801, AY8910_read_port_1_r },
	{ 0x4803, 0x4803, AY8910_read_port_2_r },
	{ 0x4805, 0x4805, AY8910_read_port_3_r },
	{ 0x5000, 0x5000, halleys_soundcommand_r },
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4800, AY8910_control_port_1_w },
	{ 0x4801, 0x4801, AY8910_write_port_1_w },
	{ 0x4802, 0x4802, AY8910_control_port_2_w },
	{ 0x4803, 0x4803, AY8910_write_port_2_w },
	{ 0x4804, 0x4804, AY8910_control_port_3_w },
	{ 0x4805, 0x4805, AY8910_write_port_3_w },
	{ 0xe000, 0xefff, MWA_ROM },
MEMORY_END

/*
Halley's Comet
Taito/Coin-it 1986

Coin mechs system can be optioned by setting dip sw 1 position 6 on
for single coin selector. Position 6 off for twin coin selector.

Dip sw 2 is not used and all contacts should be set off.
*/
INPUT_PORTS_START( halleys )
	PORT_START /* 0xff95 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )

	PORT_START /* 0xff96 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "200k/500k" )
	PORT_DIPSETTING(    0x04, "200k/800k" )
	PORT_DIPSETTING(    0x08, "200k/1000k" )
	PORT_DIPSETTING(    0x0c, "100k/500k" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x00, "Record Data" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* 0xff97 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x00, "Start Round" )
	PORT_DIPSETTING( 0x00, "1" )
	PORT_DIPSETTING( 0x04, "4" )
	PORT_DIPSETTING( 0x08, "7" )
	PORT_DIPSETTING( 0x0c, "10" )
	PORT_DIPSETTING( 0x10, "13" )
	PORT_DIPSETTING( 0x14, "16" )
	PORT_DIPSETTING( 0x18, "19" )
	PORT_DIPSETTING( 0x1c, "22" )
	PORT_DIPNAME( 0x20, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Unknown7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Unknown8" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* 0xff90 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START /* 0xff91 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT| IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* WARP */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* FIRE */

	PORT_START /* 0xff92 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY |IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT| IPF_8WAY |IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY |IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   | IPF_8WAY |IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 |IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 |IPF_PLAYER2 )
INPUT_PORTS_END

/* we don't actually use gfx elements; we unpack only to help visualize the gfxdata roms */
static struct GfxLayout gfx_layout1 =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(1,2), RGN_FRAC(0,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};
static struct GfxLayout gfx_layout2 =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2),RGN_FRAC(1,2)+1,RGN_FRAC(0,2),RGN_FRAC(0,2)+1 },
	{ 0*2, 1*2, 2*2, 3*2, 4*2, 5*2, 6*2, 7*2 },
	{ 0*8*2, 1*8*2, 2*8*2, 3*8*2, 4*8*2, 5*8*2, 6*8*2, 7*8*2 },
	8*8*2
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &gfx_layout1, 0, 32/4 },
	{ REGION_GFX1, 0, &gfx_layout2, 0, 32/16 },
	{ -1 }
};

static struct AY8910interface ay8910_interface =
{
	4,      /* 4 chips */
	6000000/4,      /* 1.5 MHz */
	{ 15, 15, 15, MIXERG(15,MIXER_GAIN_2x,MIXER_PAN_CENTER) },
	{ /*input_port_6_r*/0, 0, 0, 0 },            /* port Aread */
	{ /*input_port_7_r*/0, 0, 0, 0 },            /* port Bread */
	{ 0, /*DAC_0_data_w*/0, 0, 0 },              /* port Awrite */
	{ 0, 0, 0, halleys_sndnmi_msk_w }       /* port Bwrite */
};

static INTERRUPT_GEN( halleys_interrupt )
{
	static int which;

	which = 1-which;


	if ((readinputport(3) & 0xc0) != 0x00)
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);


//	if( which )
//	if( cpu_getiloops()==0 )
//	{
//		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
//	}
//	else
	{
		cpu_set_irq_line(0, M6809_FIRQ_LINE, HOLD_LINE);
	}
}

static MACHINE_DRIVER_START( halleys )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,2000000)		/* 2 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)
	MDRV_CPU_PERIODIC_INT(halleys_interrupt,1)

	MDRV_CPU_ADD(Z80,6000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)      /* 3 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
			/* interrupts: */
			/* - no interrupts synced with vblank ?????? */
			/* - NMI triggered by the main CPU */
			/* - periodic IRQ, with frequency 6000000/(4*16*16*10*16) = 36.621 Hz, ?????? */
			/*   that is a period of 27306666.6666 ns ??????????? */
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,27306667)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_PALETTE_LENGTH(32)
	MDRV_PALETTE_INIT(halleys)

	MDRV_VIDEO_START(halleys)
	MDRV_VIDEO_UPDATE(halleys)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)

MACHINE_DRIVER_END



/**************************************************************************/


ROM_START( benberob )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) //MAIN PRG
	ROM_LOAD( "a26_01.31",   0x4000, 0x4000, 0x9ed566ba )
	ROM_LOAD( "a26_02.52",   0x8000, 0x4000, 0xa563a033 )
	ROM_LOAD( "a26_03.50",   0xc000, 0x4000, 0x975849ef )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) //SOUND
	ROM_LOAD( "a26_12.5",    0x0000, 0x4000, 0x7fd728f3 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) //CHR
	ROM_LOAD( "a26_04.80",   0x00000, 0x4000, 0x77d10723 )
	ROM_LOAD( "a26_05.79",   0x04000, 0x4000, 0x098eebcc )
	ROM_LOAD( "a26_06.78",   0x08000, 0x4000, 0x79ae2e58 )
	ROM_LOAD( "a26_07.77",   0x0c000, 0x4000, 0xfe976343 )
	ROM_LOAD( "a26_08.91",   0x10000, 0x4000, 0x7e63059d )
	ROM_LOAD( "a26_09.90",   0x14000, 0x4000, 0xebd9a16c )
	ROM_LOAD( "a26_10.89",   0x18000, 0x4000, 0x3d406060 )
	ROM_LOAD( "a26_11.88",   0x1c000, 0x4000, 0x4561836a )

	ROM_REGION( 0x0060, REGION_PROMS, 0 ) //COLOR (all identical!)
	ROM_LOAD( "a26_13.r",    0x0000, 0x0020, 0xec449aee )
	ROM_LOAD( "a26_13.g",    0x0020, 0x0020, 0xec449aee )
	ROM_LOAD( "a26_13.b",    0x0040, 0x0020, 0xec449aee )
ROM_END

ROM_START( halleys )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) //MAIN PRG
	ROM_LOAD( "a62_01.30",   0x0000, 0x4000, 0xa5e82b3e )
	ROM_LOAD( "a62_02.31",   0x4000, 0x4000, 0x25f5bcd3 )
	ROM_LOAD( "a62-15.52",   0x8000, 0x4000, 0xe65d8312 )
	ROM_LOAD( "a62_04.50",   0xc000, 0x4000, 0xfad74dfe )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) //SOUND
	ROM_LOAD( "a62_13.5",    0x0000, 0x2000, 0x7ce290db )
	ROM_LOAD( "a62_14.4",    0x2000, 0x2000, 0xea74b1a2 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD( "a62_12.78",   0x00000, 0x4000, 0xc5834a7a )
	ROM_LOAD( "a62_10.77",   0x04000, 0x4000, 0x3ae7231e )
	ROM_LOAD( "a62_08.80",   0x08000, 0x4000, 0xb9210dbe )
	ROM_LOAD( "a62-16.79",   0x0c000, 0x4000, 0x1165a622 ) /* alpha */

	ROM_LOAD( "a62_11.89",   0x10000, 0x4000, 0xd0e9974e )
	ROM_LOAD( "a62_09.88",   0x14000, 0x4000, 0xe93ef281 )
	ROM_LOAD( "a62_07.91",   0x18000, 0x4000, 0x64c95e8b )
	ROM_LOAD( "a62_05.90",   0x1c000, 0x4000, 0xc3c877ef )

	ROM_REGION( 0x0060, REGION_PROMS, 0 ) //COLOR (all identical!)
	ROM_LOAD( "a26-13.109",  0x0000, 0x0020, 0xec449aee )
	ROM_LOAD( "a26-13.110",  0x0020, 0x0020, 0xec449aee )
	ROM_LOAD( "a26-13.111",  0x0040, 0x0020, 0xec449aee )
ROM_END

ROM_START( halleycj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) //MAIN PRG
	ROM_LOAD( "a62_01.30",   0x0000, 0x4000, 0xa5e82b3e )
	ROM_LOAD( "a62_02.31",   0x4000, 0x4000, 0x25f5bcd3 )
	ROM_LOAD( "a62_03-1.52", 0x8000, 0x4000, 0xe2fffbe4 )
	ROM_LOAD( "a62_04.50",   0xc000, 0x4000, 0xfad74dfe )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) //SOUND
	ROM_LOAD( "a62_13.5",    0x0000, 0x2000, 0x7ce290db )
	ROM_LOAD( "a62_14.4",    0x2000, 0x2000, 0xea74b1a2 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) //CHR
	ROM_LOAD( "a62_08.80",   0x00000, 0x4000, 0xb9210dbe )
	ROM_LOAD( "a62_06.79",   0x04000, 0x4000, 0x600be9ca )
	ROM_LOAD( "a62_12.78",   0x08000, 0x4000, 0xc5834a7a )
	ROM_LOAD( "a62_10.77",   0x0c000, 0x4000, 0x3ae7231e )
	ROM_LOAD( "a62_07.91",   0x10000, 0x4000, 0x64c95e8b )
	ROM_LOAD( "a62_05.90",   0x14000, 0x4000, 0xc3c877ef )
	ROM_LOAD( "a62_11.89",   0x18000, 0x4000, 0xd0e9974e )
	ROM_LOAD( "a62_09.88",   0x1c000, 0x4000, 0xe93ef281 )

	ROM_REGION( 0x0060, REGION_PROMS, 0 ) //COLOR
	ROM_LOAD( "a26-13.109",  0x0000, 0x0020, 0xec449aee )
	ROM_LOAD( "a26-13.110",  0x0020, 0x0020, 0xec449aee )
	ROM_LOAD( "a26-13.111",  0x0040, 0x0020, 0xec449aee )
ROM_END

ROM_START( halleysc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) //MAIN PRG
	ROM_LOAD( "a62_01.30",   0x0000, 0x4000, 0xa5e82b3e )
	ROM_LOAD( "a62_02.31",   0x4000, 0x4000, 0x25f5bcd3 )
	ROM_LOAD( "a62_03.52",   0x8000, 0x4000, 0x8e90a97b )
	ROM_LOAD( "a62_04.50",   0xc000, 0x4000, 0xfad74dfe )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) //SOUND
	ROM_LOAD( "a62_13.5",    0x0000, 0x2000, 0x7ce290db )
	ROM_LOAD( "a62_14.4",    0x2000, 0x2000, 0xea74b1a2 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) //CHR
	ROM_LOAD( "a62_08.80",   0x00000, 0x4000, 0xb9210dbe )
	ROM_LOAD( "a62_06.79",   0x04000, 0x4000, 0x600be9ca )
	ROM_LOAD( "a62_12.78",   0x08000, 0x4000, 0xc5834a7a )
	ROM_LOAD( "a62_10.77",   0x0c000, 0x4000, 0x3ae7231e )
	ROM_LOAD( "a62_07.91",   0x10000, 0x4000, 0x64c95e8b )
	ROM_LOAD( "a62_05.90",   0x14000, 0x4000, 0xc3c877ef )
	ROM_LOAD( "a62_11.89",   0x18000, 0x4000, 0xd0e9974e )
	ROM_LOAD( "a62_09.88",   0x1c000, 0x4000, 0xe93ef281 )

	ROM_REGION( 0x0060, REGION_PROMS, 0 ) //COLOR
	ROM_LOAD( "a26-13.109",  0x0000, 0x0020, 0xec449aee )
	ROM_LOAD( "a26-13.110",  0x0020, 0x0020, 0xec449aee )
	ROM_LOAD( "a26-13.111",  0x0040, 0x0020, 0xec449aee )
ROM_END


static DRIVER_INIT( halleys )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 *buf;

/*
	The data lines between the program ROMs and the CPU are swapped.
	ROM -> CPU
	D0	D7
	D1	D3
	D2	D1
	D3	D0
	D4	D2
	D5	D4
	D6	D5
	D7	D6
*/

/*
	Address lines between the program ROMs and the CPU are swapped.
	CPU -> ROM
	A0	A8
	A1	A9
	A2	A0
	A3	A4
	A4	A7 (via additional logic !!!)
	A5	A6
	A6	A5
	A7	A3
	A8	A2
	A9	A1
	A10	A10
	A11	A11
	A12	A12
	A13	A13
	A14	n/a
	A15	n/a
*/

	buf = malloc(0x10000);
	if (buf)
	{
		int i;

		for (i = 0; i < 0x10000; i++)
		{
			UINT32 addr;

			addr = BITSWAP16(i,15,14,13,12,11,10,1,0,4,5,6,3,7,8,9,2);
			buf[i] = BITSWAP8(rom[addr],0,7,6,5,1,4,2,3);
		}

		memcpy(rom, buf, 0x10000);

		free(buf);
	}
}

GAME( 1984, benberob, 0,       halleys, halleys, halleys, ROT0,  "Taito", "Ben Bero Beh (Japan)" )
GAME( 1986, halleys,  0,       halleys, halleys, halleys, ROT90, "Taito", "Halley's Comet (set 1)" )
GAME( 1986, halleysc, halleys, halleys, halleys, halleys, ROT90, "Taito", "Halley's Comet (set 2)" )
GAME( 1986, halleycj, halleys, halleys, halleys, halleys, ROT90, "Taito", "Halley's Comet (Japan)" )
