/***************************************************************************

  Tumblepop             (c) 1991 Data East Corporation (Bootleg 1)
  Tumblepop             (c) 1991 Data East Corporation (Bootleg 2)

Driver notes:

  Original romset would be nice :)
  There is music in the top half of the sample rom.  Don't know how it's
accessed.
  Sound is not quite correct yet (Nothing on bootleg 2).
  Dip switches are totally wrong.  They seem different to usual Deco dips.

  Emulation by Bryan McPhail, mish@tendril.force9.net

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int  tumblep_vh_start(void);
void tumblep_vh_stop(void);
void tumblep_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void tumblep_pf2_data_w(int offset,int data);
void tumblep_pf3_data_w(int offset,int data);

void tumblep_control_0_w(int offset,int data);

extern unsigned char *tumblep_pf2_data,*tumblep_pf3_data;

/******************************************************************************/

static void tumblep_oki_w(int offset, int data)
{
	OKIM6295_data_0_w(0,data&0xff);
    /* STUFF IN OTHER BYTE TOO..*/
}

static int tumblep_prot_r(int offset)
{
	return 0xffff;
}

/******************************************************************************/

static int tumblepop_controls_read(int offset)
{
 	switch (offset)
	{
		case 0: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));
		case 2: /* Dips */
			return (readinputport(3) + (readinputport(4) << 8));
		case 8: /* Credits */
			return readinputport(2);
		case 10: /* Looks like remains of protection... */
		case 12:
        	return 0;
	}

	if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - read unmapped control address %06x\n",cpu_get_pc(),offset);
	return 0xffff;
}

/******************************************************************************/

static struct MemoryReadAddress tumblepop_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x100001, tumblep_prot_r },
	{ 0x120000, 0x123fff, MRA_BANK1 },
	{ 0x140000, 0x1407ff, paletteram_word_r },
	{ 0x160000, 0x1607ff, MRA_BANK5 },
	{ 0x180000, 0x18000f, tumblepop_controls_read },
	{ 0x1a0000, 0x1a07ff, MRA_BANK2 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress tumblepop_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x100001, tumblep_oki_w },
	{ 0x120000, 0x123fff, MWA_BANK1 },
	{ 0x140000, 0x1407ff, paletteram_xxxxBBBBGGGGRRRR_word_w, &paletteram },
	{ 0x160000, 0x160807, MWA_BANK5, &spriteram },
	{ 0x18000c, 0x18000d, MWA_NOP }, /* Looks like remains of protection */
	{ 0x1a0000, 0x1a07ff, MWA_BANK2 },

	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x320fff, tumblep_pf3_data_w, &tumblep_pf3_data },
	{ 0x322000, 0x322fff, tumblep_pf2_data_w, &tumblep_pf2_data },
	{ 0x340000, 0x3401ff, MWA_NOP }, /* Unused row scroll */
	{ 0x340400, 0x34047f, MWA_NOP }, /* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA_NOP },
	{ 0x342400, 0x34247f, MWA_NOP },
	{ -1 }  /* end of table */
};

/******************************************************************************/

INPUT_PORTS_START( tumblep_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	/* All dips are wrong! */

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Energy" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "2.5" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout tcharlayout =
{
	8,8,	/* 8*8 chars */
	4096,
	4,		/* 4 bits per pixel  */
	{ 0x40000*8 , 0x00000*8, 0x60000*8 , 0x20000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tlayout =
{
	16,16,
	0x2000,
	4,
	{ 0x00000*8 , 0x40000*8, 0x80000*8, 0xc0000*8 },
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout tlayout3 =
{
	16,16,
	4096,
	4,
	{  0x40000*8 , 0x00000*8, 0x60000*8 , 0x20000*8 },
	{
			0, 1, 2, 3, 4, 5, 6, 7,
           16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x100000, &tcharlayout,  256, 16 },	/* Characters 8x8 */
	{ 1, 0x100000, &tlayout3,     512, 16 }, 	/* Tiles 16x16 */
	{ 1, 0x100000, &tlayout3,     256, 16 },	/* Tiles 16x16 */
	{ 1, 0x000000, &tlayout,    	0, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	{ 8000 },           /* 8000Hz frequency */
	{ 2 },          /* memory region 3 */
	{ 70 }
};

static struct MachineDriver tumblepop_machine_driver =
{
	/* basic machine hardware */
	{
	 	{
			CPU_M68000,
			12000000,
			0,
			tumblepop_readmem,tumblepop_writemem,0,0,
			m68_level6_irq,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	tumblep_vh_start,
	tumblep_vh_stop,
	tumblep_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
  	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/******************************************************************************/

ROM_START( tumblepop_rom )
	ROM_REGION(0x80000) /* 68000 code */
	ROM_LOAD_EVEN ("thumbpop.12", 0x00000, 0x40000, 0x0c984703 )
	ROM_LOAD_ODD ( "thumbpop.13", 0x00000, 0x40000, 0x864c4053 )

 	ROM_REGION(0x180000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "thumbpop.15",  0x00000,  0x40000, 0xac3d8349 )
	ROM_LOAD( "thumbpop.14",  0x40000,  0x40000, 0x79a29725 )
	ROM_LOAD( "thumbpop.17",  0x80000,  0x40000, 0x87cffb06 )
	ROM_LOAD( "thumbpop.16",  0xc0000,  0x40000, 0xee91db18 )

	ROM_LOAD( "thumbpop.19",  0x100000, 0x40000, 0x0795aab4 )
	ROM_LOAD( "thumbpop.18",  0x140000, 0x40000, 0xad58df43 )

	ROM_REGION(0x80000) /* Oki samples */
	ROM_LOAD( "thumbpop.snd",  0x00000,  0x80000, 0xfabbf15d )
ROM_END

ROM_START( tumblepop2_rom )
	ROM_REGION(0x80000) /* 68000 code */
	ROM_LOAD_EVEN ("thumbpop.2", 0x00000, 0x40000, 0x34b016e1 )
	ROM_LOAD_ODD ( "thumbpop.3", 0x00000, 0x40000, 0x89501c71 )

	ROM_REGION(0x180000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "thumbpop.5",   0x00000,  0x40000, 0xdda8932e )
	ROM_LOAD( "thumbpop.14",  0x40000,  0x40000, 0x79a29725 )
	ROM_LOAD( "thumbpop.17",  0x80000,  0x40000, 0x87cffb06 )
	ROM_LOAD( "thumbpop.16",  0xc0000,  0x40000, 0xee91db18 )

	ROM_LOAD( "thumbpop.19",  0x100000, 0x40000, 0x0795aab4 )
	ROM_LOAD( "thumbpop.18",  0x140000, 0x40000, 0xad58df43 )

	ROM_REGION(0x80000) /* Oki samples */
	ROM_LOAD( "thumbpop.snd",  0x00000,  0x80000, 0xfabbf15d )
ROM_END

/******************************************************************************/

static void t_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];
	int i,x,a;
    char z[64];

	/* Hmm, characters are stored in wrong word endian-ness for sequential graphics
		decode!  Very bad...  */
	RAM = Machine->memory_region[1];

	for (a=0; a<4; a++) {
		for (i=32; i<0x2800; i+=32) {
            for (x=0; x<16; x++)
            	z[x]=RAM[i + x + 0x100000+(a*0x20000)];
            for (x=0; x<16; x++)
                RAM[i + x + 0x100000+(a*0x20000)]=RAM[i+x+16+0x100000+(a*0x20000)];
            for (x=0; x<16; x++)
				RAM[i+x+16+0x100000+(a*0x20000)]=z[x];
    	}
    }
}

/******************************************************************************/

struct GameDriver tumblep_driver =
{
	__FILE__,
	0,
	"tumblep",
	"Tumble Pop (bootleg set 1)",
	"1991",
	"bootleg",
	"Bryan McPhail",
	0,
	&tumblepop_machine_driver,
	0,

	tumblepop_rom,
	t_patch, 0,
	0,
	0,	/* sound_prom */

	tumblep_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver tumblep2_driver =
{
	__FILE__,
	&tumblep_driver,
	"tumblep2",
	"Tumble Pop (bootleg set 2)",
	"1991",
	"bootleg",
	"Bryan McPhail",
	0,
	&tumblepop_machine_driver,
	0,

	tumblepop2_rom,
	t_patch, 0,
	0,
	0,	/* sound_prom */

	tumblep_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};
