#include "driver.h"

static unsigned char *ram;
extern unsigned char *toki_foreground_videoram;
extern unsigned char *toki_foreground_paletteram;
extern unsigned char *toki_background1_videoram;
extern unsigned char *toki_background1_paletteram;
extern unsigned char *toki_background2_videoram;
extern unsigned char *toki_background2_paletteram;
extern unsigned char *toki_sprites_paletteram;
extern unsigned char *toki_sprites_dataram;

int  toki_vh_start(void);
void toki_vh_stop(void);
int  toki_sh_start(void);
void toki_sh_stop(void);
void toki_soundcommand_w(int offset,int data);
void toki_vh_screenrefresh(struct osd_bitmap *bitmap);
int  toki_foreground_paletteram_r(int offset);
void toki_foreground_paletteram_w(int offset,int data);
int  toki_foreground_videoram_r(int offset);
void toki_foreground_videoram_w(int offset,int data);
int  toki_background1_paletteram_r(int offset);
void toki_background1_paletteram_w(int offset,int data);
int  toki_background1_videoram_r(int offset);
void toki_background1_videoram_w(int offset,int data);
int  toki_background2_paletteram_r(int offset);
void toki_background2_paletteram_w(int offset,int data);
int  toki_background2_videoram_r(int offset);
void toki_background2_videoram_w(int offset,int data);
int  toki_sprites_paletteram_r(int offset);
void toki_sprites_paletteram_w(int offset,int data);
void toki_scrollxy_w(int offset,int data);


int toki_read_ports(int offset)
{
    switch(offset)
    {
	case 0:
		return(input_port_3_r(0)+(input_port_2_r(0)<<8));
	case 2:
		return(input_port_1_r(0));
	case 4:
		return(input_port_0_r(0));
	default:
		return 0;
    }
}

int toki_interrupt(void)
{
	return 1;  /*Interrupt vector 1*/
}

static struct MemoryReadAddress toki_readmem[] =
{
	{ 0x000000, 0x05FFFF, MRA_ROM },
	{ 0x060000, 0x06DFFF, MRA_BANK1, &ram },
	{ 0x06E000, 0x06E1FF, toki_sprites_paletteram_r, &toki_sprites_paletteram },
	{ 0x06E200, 0x06E3FF, toki_foreground_paletteram_r, &toki_foreground_paletteram },
	{ 0x06E400, 0x06E5FF, toki_background1_paletteram_r, &toki_background1_paletteram },
	{ 0x06E600, 0x06E7FF, toki_background2_paletteram_r, &toki_background2_paletteram },
	{ 0x06E800, 0x06EFFF, toki_background1_videoram_r, &toki_background1_videoram },
	{ 0x06F000, 0x06F7FF, toki_background2_videoram_r, &toki_background2_videoram },
	{ 0x06F800, 0x06FFFF, toki_foreground_videoram_r, &toki_foreground_videoram },
	{ 0x070000, 0x07180D, MRA_BANK2 },
	{ 0x07180E, 0x07200D, MRA_BANK3, &toki_sprites_dataram },
	{ 0x07200E, 0x074FFF, MRA_BANK4 },
	{ 0x075000, 0x07500B, MRA_NOP },
	{ 0x07500C, 0x0800FF, MRA_BANK5 },
	{ 0x0A0000, 0x0A00FF, MRA_BANK6 },
	{ 0x0C0000, 0x0C0005, toki_read_ports },
	{ 0x0C0006, 0x0C00FF, MRA_BANK7 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress toki_writemem[] =
{
	{ 0x000000, 0x05FFFF, MWA_ROM },
	{ 0x060000, 0x06DFFF, MWA_BANK1 },
	{ 0x06E000, 0x06E1FF, toki_sprites_paletteram_w },
	{ 0x06E200, 0x06E3FF, toki_foreground_paletteram_w },
	{ 0x06E400, 0x06E5FF, toki_background1_paletteram_w },
	{ 0x06E600, 0x06E7FF, toki_background2_paletteram_w },
	{ 0x06E800, 0x06EFFF, toki_background1_videoram_w },
	{ 0x06F000, 0x06F7FF, toki_background2_videoram_w },
	{ 0x06F800, 0x06FFFF, toki_foreground_videoram_w },
	{ 0x070000, 0x07180D, MWA_BANK2 },
	{ 0x07180E, 0x07200D, MWA_BANK3 },
	{ 0x07200E, 0x074FFF, MWA_BANK4 },
	{ 0x075000, 0x075003, toki_soundcommand_w },
	{ 0x075004, 0x07500B, toki_scrollxy_w },
	{ 0x07500C, 0x0800FF, MWA_BANK5 },
	{ 0x0A0000, 0x0A00FF, MWA_BANK6 },		// SOUND DATA ???
	{ 0x0C0000, 0x0C0005, MWA_NOP },
	{ 0x0C0006, 0x0C00FF, MWA_BANK7 },
	{ -1 }  /* end of table */
};

#if 0
static struct MemoryReadAddress toki_sndreadmem[] =
{
	{ 0x000000, 0x00DFFF, MRA_ROM },
	{ 0x00E000, 0x00F7FF, MRA_RAM },
	{ 0x00F800, 0x00FFFF, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress toki_sndwritemem[] =
{
	{ 0x000000, 0x00DFFF, MWA_ROM },
	{ 0x00E000, 0x00F7FF, MWA_RAM },
	{ 0x00F800, 0x00FFFF, MWA_ROM },
	{ -1 }  /* end of table */
};
#endif

INPUT_PORTS_START( input_ports )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x0C, 0x0C, "Extra Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "50000 150000" )
	PORT_DIPSETTING(    0x00, "70000 140000 210000" )
	PORT_DIPSETTING(    0x0C, "70000" )
	PORT_DIPSETTING(    0x04, "100000 200000" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Medium" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x1F, 0x1F, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x15, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0x17, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x19, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x1B, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "8 Coins/3 Credits" )
	PORT_DIPSETTING(    0x1D, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "5 Coins/3 Credits" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x1F, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x13, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x11, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0F, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0D, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0B, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x1E, "A 1/1 B 1/2" )
	PORT_DIPSETTING(    0x14, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x0A, "A 3/1 B 1/5" )
	PORT_DIPSETTING(    0x00, "A 5/1 B 1/6" )
	PORT_DIPSETTING(    0x01, "Free Play" )
	PORT_DIPNAME( 0x20, 0x20, "Joysticks", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8 by 8 */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
        {4096*8*8*3,4096*8*8*2,4096*8*8*1,4096*8*8*0 },	/* planes */
	{ 0, 1,  2,  3,  4,  5,  6,  7},		/* x bit */
	{ 0, 8, 16, 24, 32, 40, 48, 56},		/* y bit */
	8*8
};

static struct GfxLayout backgroundlayout =
{
	16,16,	/* 16 by 16 */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
        { 4096*16*16*3,4096*16*16*2,4096*16*16*1,4096*16*16*0 },	/* planes */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  0x8000*8+0, 0x8000*8+1, 0x8000*8+2, 0x8000*8+3, 0x8000*8+4,
	  0x8000*8+5, 0x8000*8+6, 0x8000*8+7 },				/* x bit */
	{
	  0,8,16,24,32,40,48,56,
	  0x10000*8+ 0, 0x10000*8+ 8, 0x10000*8+16, 0x10000*8+24, 0x10000*8+32,
	  0x10000*8+40, 0x10000*8+48, 0x10000*8+56 },			/* y bit */
	8*8
};

static struct GfxLayout spriteslayout =
{
	16,16,	/* 16 by 16 */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
        { 8192*16*16*3,8192*16*16*2,8192*16*16*1,8192*16*16*0 },	/* planes */
	{    0,     1,     2,     3,     4,     5,     6,     7,
	 128+0, 128+1, 128+2, 128+3, 128+4, 128+5, 128+6, 128+7 },	/* x bit */
	{ 0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120 },		/* y bit */
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       	0,  16 },
	{ 2, 0x0000, &spriteslayout,        16*16,  16 },
	{ 3, 0x0000, &backgroundlayout,     32*16,  16 },
	{ 4, 0x0000, &backgroundlayout,	    48*16,  16 },
	{ -1 } /* end of array */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,	/* 16 MHz ? */
			0,
			toki_readmem,toki_writemem,
			0,0,
			toki_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,
	/* video hardware */
	32*8, 32*8,
	{ 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256, 4*256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT,
	0,
	toki_vh_start,
	toki_vh_stop,
	toki_vh_screenrefresh,

	/* sound hardware; only samples because we still haven't YM 3812 */
	0,
	toki_sh_start,
	toki_sh_stop,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( toki_rom )

	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "TOKI.E3", 0x00000, 0x20000, 0xe2f5431f )
	ROM_LOAD_ODD ( "TOKI.E5", 0x00000, 0x20000, 0x4404439e )
	ROM_LOAD_EVEN( "TOKI.E2", 0x40000, 0x10000, 0x1fe23624 )
	ROM_LOAD_ODD ( "TOKI.E4", 0x40000, 0x10000, 0x1678f760 )

	ROM_REGION(0x20000)	/* 2*64k for foreground tiles */
	ROM_LOAD( "TOKI.E21", 0x00000, 0x8000, 0x62bbb4d3 )
	ROM_LOAD( "TOKI.E13", 0x08000, 0x8000, 0x47775ce5 )
	ROM_LOAD( "TOKI.E22", 0x10000, 0x8000, 0xf6e030fc )
	ROM_LOAD( "TOKI.E7" , 0x18000, 0x8000, 0xc2a6d2f0 )

	ROM_REGION(0x100000)	/* 16*64k for sprites */
	ROM_LOAD( "TOKI.E26", 0x00000, 0x20000, 0xf240d60a )
	ROM_LOAD( "TOKI.E28", 0x20000, 0x20000, 0x1525bd97 )
	ROM_LOAD( "TOKI.E34", 0x40000, 0x20000, 0x93776021 )
	ROM_LOAD( "TOKI.E36", 0x60000, 0x20000, 0x14d49b24 )
	ROM_LOAD( "TOKI.E30", 0x80000, 0x20000, 0x0b00822e )
	ROM_LOAD( "TOKI.E32", 0xA0000, 0x20000, 0x625a59fa )
	ROM_LOAD( "TOKI.E38", 0xC0000, 0x20000, 0xf062260c )
	ROM_LOAD( "TOKI.E40", 0xE0000, 0x20000, 0xb78b0da3 )

	ROM_REGION(0x80000)	/* 8*64k for background #1 tiles */
	ROM_LOAD( "TOKI.E23", 0x00000, 0x00800, 0xeafea71c )
	ROM_CONTINUE(	      0x10000, 0x00800)
	ROM_CONTINUE(	      0x08000, 0x00800)
	ROM_CONTINUE(	      0x18000, 0x00800)
	ROM_CONTINUE(	      0x00800, 0x00800)
	ROM_CONTINUE(	      0x10800, 0x00800)
	ROM_CONTINUE(	      0x08800, 0x00800)
	ROM_CONTINUE(	      0x18800, 0x00800)
	ROM_CONTINUE(	      0x01000, 0x00800)
	ROM_CONTINUE(	      0x11000, 0x00800)
	ROM_CONTINUE(	      0x09000, 0x00800)
	ROM_CONTINUE(	      0x19000, 0x00800)
	ROM_CONTINUE(	      0x01800, 0x00800)
	ROM_CONTINUE(	      0x11800, 0x00800)
	ROM_CONTINUE(	      0x09800, 0x00800)
	ROM_CONTINUE(	      0x19800, 0x00800)
	ROM_CONTINUE(	      0x02000, 0x00800)
	ROM_CONTINUE(	      0x12000, 0x00800)
	ROM_CONTINUE(	      0x0A000, 0x00800)
	ROM_CONTINUE(	      0x1A000, 0x00800)
	ROM_CONTINUE(	      0x02800, 0x00800)
	ROM_CONTINUE(	      0x12800, 0x00800)
	ROM_CONTINUE(	      0x0A800, 0x00800)
	ROM_CONTINUE(	      0x1A800, 0x00800)
	ROM_CONTINUE(	      0x03000, 0x00800)
	ROM_CONTINUE(	      0x13000, 0x00800)
	ROM_CONTINUE(	      0x0B000, 0x00800)
	ROM_CONTINUE(	      0x1B000, 0x00800)
	ROM_CONTINUE(	      0x03800, 0x00800)
	ROM_CONTINUE(	      0x13800, 0x00800)
	ROM_CONTINUE(	      0x0B800, 0x00800)
	ROM_CONTINUE(	      0x1B800, 0x00800)
	ROM_LOAD( "TOKI.E24", 0x04000, 0x00800, 0x1515fcb5 )
	ROM_CONTINUE(	      0x14000, 0x00800)
	ROM_CONTINUE(	      0x0C000, 0x00800)
	ROM_CONTINUE(	      0x1C000, 0x00800)
	ROM_CONTINUE(	      0x04800, 0x00800)
	ROM_CONTINUE(	      0x14800, 0x00800)
	ROM_CONTINUE(	      0x0C800, 0x00800)
	ROM_CONTINUE(	      0x1C800, 0x00800)
	ROM_CONTINUE(	      0x05000, 0x00800)
	ROM_CONTINUE(	      0x15000, 0x00800)
	ROM_CONTINUE(	      0x0D000, 0x00800)
	ROM_CONTINUE(	      0x1D000, 0x00800)
	ROM_CONTINUE(	      0x05800, 0x00800)
	ROM_CONTINUE(	      0x15800, 0x00800)
	ROM_CONTINUE(	      0x0D800, 0x00800)
	ROM_CONTINUE(	      0x1D800, 0x00800)
	ROM_CONTINUE(	      0x06000, 0x00800)
	ROM_CONTINUE(	      0x16000, 0x00800)
	ROM_CONTINUE(	      0x0E000, 0x00800)
	ROM_CONTINUE(	      0x1E000, 0x00800)
	ROM_CONTINUE(	      0x06800, 0x00800)
	ROM_CONTINUE(	      0x16800, 0x00800)
	ROM_CONTINUE(	      0x0E800, 0x00800)
	ROM_CONTINUE(	      0x1E800, 0x00800)
	ROM_CONTINUE(	      0x07000, 0x00800)
	ROM_CONTINUE(	      0x17000, 0x00800)
	ROM_CONTINUE(	      0x0F000, 0x00800)
	ROM_CONTINUE(	      0x1F000, 0x00800)
	ROM_CONTINUE(	      0x07800, 0x00800)
	ROM_CONTINUE(	      0x17800, 0x00800)
	ROM_CONTINUE(	      0x0F800, 0x00800)
	ROM_CONTINUE(	      0x1F800, 0x00800)
	ROM_LOAD( "TOKI.E15", 0x20000, 0x00800, 0x63200f60 )
	ROM_CONTINUE(	      0x30000, 0x00800)
	ROM_CONTINUE(	      0x28000, 0x00800)
	ROM_CONTINUE(	      0x38000, 0x00800)
	ROM_CONTINUE(	      0x20800, 0x00800)
	ROM_CONTINUE(	      0x30800, 0x00800)
	ROM_CONTINUE(	      0x28800, 0x00800)
	ROM_CONTINUE(	      0x38800, 0x00800)
	ROM_CONTINUE(	      0x21000, 0x00800)
	ROM_CONTINUE(	      0x31000, 0x00800)
	ROM_CONTINUE(	      0x29000, 0x00800)
	ROM_CONTINUE(	      0x39000, 0x00800)
	ROM_CONTINUE(	      0x21800, 0x00800)
	ROM_CONTINUE(	      0x31800, 0x00800)
	ROM_CONTINUE(	      0x29800, 0x00800)
	ROM_CONTINUE(	      0x39800, 0x00800)
	ROM_CONTINUE(	      0x22000, 0x00800)
	ROM_CONTINUE(	      0x32000, 0x00800)
	ROM_CONTINUE(	      0x2A000, 0x00800)
	ROM_CONTINUE(	      0x3A000, 0x00800)
	ROM_CONTINUE(	      0x22800, 0x00800)
	ROM_CONTINUE(	      0x32800, 0x00800)
	ROM_CONTINUE(	      0x2A800, 0x00800)
	ROM_CONTINUE(	      0x3A800, 0x00800)
	ROM_CONTINUE(	      0x23000, 0x00800)
	ROM_CONTINUE(	      0x33000, 0x00800)
	ROM_CONTINUE(	      0x2B000, 0x00800)
	ROM_CONTINUE(	      0x3B000, 0x00800)
	ROM_CONTINUE(	      0x23800, 0x00800)
	ROM_CONTINUE(	      0x33800, 0x00800)
	ROM_CONTINUE(	      0x2B800, 0x00800)
	ROM_CONTINUE(	      0x3B800, 0x00800)
	ROM_LOAD( "TOKI.E16", 0x24000, 0x00800, 0x328cb95c )
	ROM_CONTINUE(	      0x34000, 0x00800)
	ROM_CONTINUE(	      0x2C000, 0x00800)
	ROM_CONTINUE(	      0x3C000, 0x00800)
	ROM_CONTINUE(	      0x24800, 0x00800)
	ROM_CONTINUE(	      0x34800, 0x00800)
	ROM_CONTINUE(	      0x2C800, 0x00800)
	ROM_CONTINUE(	      0x3C800, 0x00800)
	ROM_CONTINUE(	      0x25000, 0x00800)
	ROM_CONTINUE(	      0x35000, 0x00800)
	ROM_CONTINUE(	      0x2D000, 0x00800)
	ROM_CONTINUE(	      0x3D000, 0x00800)
	ROM_CONTINUE(	      0x25800, 0x00800)
	ROM_CONTINUE(	      0x35800, 0x00800)
	ROM_CONTINUE(	      0x2D800, 0x00800)
	ROM_CONTINUE(	      0x3D800, 0x00800)
	ROM_CONTINUE(	      0x26000, 0x00800)
	ROM_CONTINUE(	      0x36000, 0x00800)
	ROM_CONTINUE(	      0x2E000, 0x00800)
	ROM_CONTINUE(	      0x3E000, 0x00800)
	ROM_CONTINUE(	      0x26800, 0x00800)
	ROM_CONTINUE(	      0x36800, 0x00800)
	ROM_CONTINUE(	      0x2E800, 0x00800)
	ROM_CONTINUE(	      0x3E800, 0x00800)
	ROM_CONTINUE(	      0x27000, 0x00800)
	ROM_CONTINUE(	      0x37000, 0x00800)
	ROM_CONTINUE(	      0x2F000, 0x00800)
	ROM_CONTINUE(	      0x3F000, 0x00800)
	ROM_CONTINUE(	      0x27800, 0x00800)
	ROM_CONTINUE(	      0x37800, 0x00800)
	ROM_CONTINUE(	      0x2F800, 0x00800)
	ROM_CONTINUE(	      0x3F800, 0x00800)
	ROM_LOAD( "TOKI.E17", 0x40000, 0x00800, 0xa4908982 )
	ROM_CONTINUE(	      0x50000, 0x00800)
	ROM_CONTINUE(	      0x48000, 0x00800)
	ROM_CONTINUE(	      0x58000, 0x00800)
	ROM_CONTINUE(	      0x40800, 0x00800)
	ROM_CONTINUE(	      0x50800, 0x00800)
	ROM_CONTINUE(	      0x48800, 0x00800)
	ROM_CONTINUE(	      0x58800, 0x00800)
	ROM_CONTINUE(	      0x41000, 0x00800)
	ROM_CONTINUE(	      0x51000, 0x00800)
	ROM_CONTINUE(	      0x49000, 0x00800)
	ROM_CONTINUE(	      0x59000, 0x00800)
	ROM_CONTINUE(	      0x41800, 0x00800)
	ROM_CONTINUE(	      0x51800, 0x00800)
	ROM_CONTINUE(	      0x49800, 0x00800)
	ROM_CONTINUE(	      0x59800, 0x00800)
	ROM_CONTINUE(	      0x42000, 0x00800)
	ROM_CONTINUE(	      0x52000, 0x00800)
	ROM_CONTINUE(	      0x4A000, 0x00800)
	ROM_CONTINUE(	      0x5A000, 0x00800)
	ROM_CONTINUE(	      0x42800, 0x00800)
	ROM_CONTINUE(	      0x52800, 0x00800)
	ROM_CONTINUE(	      0x4A800, 0x00800)
	ROM_CONTINUE(	      0x5A800, 0x00800)
	ROM_CONTINUE(	      0x43000, 0x00800)
	ROM_CONTINUE(	      0x53000, 0x00800)
	ROM_CONTINUE(	      0x4B000, 0x00800)
	ROM_CONTINUE(	      0x5B000, 0x00800)
	ROM_CONTINUE(	      0x43800, 0x00800)
	ROM_CONTINUE(	      0x53800, 0x00800)
	ROM_CONTINUE(	      0x4B800, 0x00800)
	ROM_CONTINUE(	      0x5B800, 0x00800)
	ROM_LOAD( "TOKI.E18", 0x44000, 0x00800, 0xe5de7236 )
	ROM_CONTINUE(	      0x54000, 0x00800)
	ROM_CONTINUE(	      0x4C000, 0x00800)
	ROM_CONTINUE(	      0x5C000, 0x00800)
	ROM_CONTINUE(	      0x44800, 0x00800)
	ROM_CONTINUE(	      0x54800, 0x00800)
	ROM_CONTINUE(	      0x4C800, 0x00800)
	ROM_CONTINUE(	      0x5C800, 0x00800)
	ROM_CONTINUE(	      0x45000, 0x00800)
	ROM_CONTINUE(	      0x55000, 0x00800)
	ROM_CONTINUE(	      0x4D000, 0x00800)
	ROM_CONTINUE(	      0x5D000, 0x00800)
	ROM_CONTINUE(	      0x45800, 0x00800)
	ROM_CONTINUE(	      0x55800, 0x00800)
	ROM_CONTINUE(	      0x4D800, 0x00800)
	ROM_CONTINUE(	      0x5D800, 0x00800)
	ROM_CONTINUE(	      0x46000, 0x00800)
	ROM_CONTINUE(	      0x56000, 0x00800)
	ROM_CONTINUE(	      0x4E000, 0x00800)
	ROM_CONTINUE(	      0x5E000, 0x00800)
	ROM_CONTINUE(	      0x46800, 0x00800)
	ROM_CONTINUE(	      0x56800, 0x00800)
	ROM_CONTINUE(	      0x4E800, 0x00800)
	ROM_CONTINUE(	      0x5E800, 0x00800)
	ROM_CONTINUE(	      0x47000, 0x00800)
	ROM_CONTINUE(	      0x57000, 0x00800)
	ROM_CONTINUE(	      0x4F000, 0x00800)
	ROM_CONTINUE(	      0x5F000, 0x00800)
	ROM_CONTINUE(	      0x47800, 0x00800)
	ROM_CONTINUE(	      0x57800, 0x00800)
	ROM_CONTINUE(	      0x4F800, 0x00800)
	ROM_CONTINUE(	      0x5F800, 0x00800)
	ROM_LOAD( "TOKI.E8",  0x60000, 0x00800, 0x12e1d7f9 )
	ROM_CONTINUE(	      0x70000, 0x00800)
	ROM_CONTINUE(	      0x68000, 0x00800)
	ROM_CONTINUE(	      0x78000, 0x00800)
	ROM_CONTINUE(	      0x60800, 0x00800)
	ROM_CONTINUE(	      0x70800, 0x00800)
	ROM_CONTINUE(	      0x68800, 0x00800)
	ROM_CONTINUE(	      0x78800, 0x00800)
	ROM_CONTINUE(	      0x61000, 0x00800)
	ROM_CONTINUE(	      0x71000, 0x00800)
	ROM_CONTINUE(	      0x69000, 0x00800)
	ROM_CONTINUE(	      0x79000, 0x00800)
	ROM_CONTINUE(	      0x61800, 0x00800)
	ROM_CONTINUE(	      0x71800, 0x00800)
	ROM_CONTINUE(	      0x69800, 0x00800)
	ROM_CONTINUE(	      0x79800, 0x00800)
	ROM_CONTINUE(	      0x62000, 0x00800)
	ROM_CONTINUE(	      0x72000, 0x00800)
	ROM_CONTINUE(	      0x6A000, 0x00800)
	ROM_CONTINUE(	      0x7A000, 0x00800)
	ROM_CONTINUE(	      0x62800, 0x00800)
	ROM_CONTINUE(	      0x72800, 0x00800)
	ROM_CONTINUE(	      0x6A800, 0x00800)
	ROM_CONTINUE(	      0x7A800, 0x00800)
	ROM_CONTINUE(	      0x63000, 0x00800)
	ROM_CONTINUE(	      0x73000, 0x00800)
	ROM_CONTINUE(	      0x6B000, 0x00800)
	ROM_CONTINUE(	      0x7B000, 0x00800)
	ROM_CONTINUE(	      0x63800, 0x00800)
	ROM_CONTINUE(	      0x73800, 0x00800)
	ROM_CONTINUE(	      0x6B800, 0x00800)
	ROM_CONTINUE(	      0x7B800, 0x00800)
	ROM_LOAD( "TOKI.E9" , 0x64000, 0x00800, 0x14318607 )
	ROM_CONTINUE(	      0x74000, 0x00800)
	ROM_CONTINUE(	      0x6C000, 0x00800)
	ROM_CONTINUE(	      0x7C000, 0x00800)
	ROM_CONTINUE(	      0x64800, 0x00800)
	ROM_CONTINUE(	      0x74800, 0x00800)
	ROM_CONTINUE(	      0x6C800, 0x00800)
	ROM_CONTINUE(	      0x7C800, 0x00800)
	ROM_CONTINUE(	      0x65000, 0x00800)
	ROM_CONTINUE(	      0x75000, 0x00800)
	ROM_CONTINUE(	      0x6D000, 0x00800)
	ROM_CONTINUE(	      0x7D000, 0x00800)
	ROM_CONTINUE(	      0x65800, 0x00800)
	ROM_CONTINUE(	      0x75800, 0x00800)
	ROM_CONTINUE(	      0x6D800, 0x00800)
	ROM_CONTINUE(	      0x7D800, 0x00800)
	ROM_CONTINUE(	      0x66000, 0x00800)
	ROM_CONTINUE(	      0x76000, 0x00800)
	ROM_CONTINUE(	      0x6E000, 0x00800)
	ROM_CONTINUE(	      0x7E000, 0x00800)
	ROM_CONTINUE(	      0x66800, 0x00800)
	ROM_CONTINUE(	      0x76800, 0x00800)
	ROM_CONTINUE(	      0x6E800, 0x00800)
	ROM_CONTINUE(	      0x7E800, 0x00800)
	ROM_CONTINUE(	      0x67000, 0x00800)
	ROM_CONTINUE(	      0x77000, 0x00800)
	ROM_CONTINUE(	      0x6F000, 0x00800)
	ROM_CONTINUE(	      0x7F000, 0x00800)
	ROM_CONTINUE(	      0x67800, 0x00800)
	ROM_CONTINUE(	      0x77800, 0x00800)
	ROM_CONTINUE(	      0x6F800, 0x00800)
	ROM_CONTINUE(	      0x7F800, 0x00800)

	ROM_REGION(0x80000)	/* 8*64k for background #2 tiles */
	ROM_LOAD( "TOKI.E25", 0x00000, 0x00800, 0x7f2775f5 )
	ROM_CONTINUE(	      0x10000, 0x00800)
	ROM_CONTINUE(	      0x08000, 0x00800)
	ROM_CONTINUE(	      0x18000, 0x00800)
	ROM_CONTINUE(	      0x00800, 0x00800)
	ROM_CONTINUE(	      0x10800, 0x00800)
	ROM_CONTINUE(	      0x08800, 0x00800)
	ROM_CONTINUE(	      0x18800, 0x00800)
	ROM_CONTINUE(	      0x01000, 0x00800)
	ROM_CONTINUE(	      0x11000, 0x00800)
	ROM_CONTINUE(	      0x09000, 0x00800)
	ROM_CONTINUE(	      0x19000, 0x00800)
	ROM_CONTINUE(	      0x01800, 0x00800)
	ROM_CONTINUE(	      0x11800, 0x00800)
	ROM_CONTINUE(	      0x09800, 0x00800)
	ROM_CONTINUE(	      0x19800, 0x00800)
	ROM_CONTINUE(	      0x02000, 0x00800)
	ROM_CONTINUE(	      0x12000, 0x00800)
	ROM_CONTINUE(	      0x0A000, 0x00800)
	ROM_CONTINUE(	      0x1A000, 0x00800)
	ROM_CONTINUE(	      0x02800, 0x00800)
	ROM_CONTINUE(	      0x12800, 0x00800)
	ROM_CONTINUE(	      0x0A800, 0x00800)
	ROM_CONTINUE(	      0x1A800, 0x00800)
	ROM_CONTINUE(	      0x03000, 0x00800)
	ROM_CONTINUE(	      0x13000, 0x00800)
	ROM_CONTINUE(	      0x0B000, 0x00800)
	ROM_CONTINUE(	      0x1B000, 0x00800)
	ROM_CONTINUE(	      0x03800, 0x00800)
	ROM_CONTINUE(	      0x13800, 0x00800)
	ROM_CONTINUE(	      0x0B800, 0x00800)
	ROM_CONTINUE(	      0x1B800, 0x00800)
	ROM_LOAD( "TOKI.E20", 0x04000, 0x00800, 0x82ed13fd )
	ROM_CONTINUE(	      0x14000, 0x00800)
	ROM_CONTINUE(	      0x0C000, 0x00800)
	ROM_CONTINUE(	      0x1C000, 0x00800)
	ROM_CONTINUE(	      0x04800, 0x00800)
	ROM_CONTINUE(	      0x14800, 0x00800)
	ROM_CONTINUE(	      0x0C800, 0x00800)
	ROM_CONTINUE(	      0x1C800, 0x00800)
	ROM_CONTINUE(	      0x05000, 0x00800)
	ROM_CONTINUE(	      0x15000, 0x00800)
	ROM_CONTINUE(	      0x0D000, 0x00800)
	ROM_CONTINUE(	      0x1D000, 0x00800)
	ROM_CONTINUE(	      0x05800, 0x00800)
	ROM_CONTINUE(	      0x15800, 0x00800)
	ROM_CONTINUE(	      0x0D800, 0x00800)
	ROM_CONTINUE(	      0x1D800, 0x00800)
	ROM_CONTINUE(	      0x06000, 0x00800)
	ROM_CONTINUE(	      0x16000, 0x00800)
	ROM_CONTINUE(	      0x0E000, 0x00800)
	ROM_CONTINUE(	      0x1E000, 0x00800)
	ROM_CONTINUE(	      0x06800, 0x00800)
	ROM_CONTINUE(	      0x16800, 0x00800)
	ROM_CONTINUE(	      0x0E800, 0x00800)
	ROM_CONTINUE(	      0x1E800, 0x00800)
	ROM_CONTINUE(	      0x07000, 0x00800)
	ROM_CONTINUE(	      0x17000, 0x00800)
	ROM_CONTINUE(	      0x0F000, 0x00800)
	ROM_CONTINUE(	      0x1F000, 0x00800)
	ROM_CONTINUE(	      0x07800, 0x00800)
	ROM_CONTINUE(	      0x17800, 0x00800)
	ROM_CONTINUE(	      0x0F800, 0x00800)
	ROM_CONTINUE(	      0x1F800, 0x00800)
	ROM_LOAD( "TOKI.E11", 0x20000, 0x00800, 0x262c3c66 )
	ROM_CONTINUE(	      0x30000, 0x00800)
	ROM_CONTINUE(	      0x28000, 0x00800)
	ROM_CONTINUE(	      0x38000, 0x00800)
	ROM_CONTINUE(	      0x20800, 0x00800)
	ROM_CONTINUE(	      0x30800, 0x00800)
	ROM_CONTINUE(	      0x28800, 0x00800)
	ROM_CONTINUE(	      0x38800, 0x00800)
	ROM_CONTINUE(	      0x21000, 0x00800)
	ROM_CONTINUE(	      0x31000, 0x00800)
	ROM_CONTINUE(	      0x29000, 0x00800)
	ROM_CONTINUE(	      0x39000, 0x00800)
	ROM_CONTINUE(	      0x21800, 0x00800)
	ROM_CONTINUE(	      0x31800, 0x00800)
	ROM_CONTINUE(	      0x29800, 0x00800)
	ROM_CONTINUE(	      0x39800, 0x00800)
	ROM_CONTINUE(	      0x22000, 0x00800)
	ROM_CONTINUE(	      0x32000, 0x00800)
	ROM_CONTINUE(	      0x2A000, 0x00800)
	ROM_CONTINUE(	      0x3A000, 0x00800)
	ROM_CONTINUE(	      0x22800, 0x00800)
	ROM_CONTINUE(	      0x32800, 0x00800)
	ROM_CONTINUE(	      0x2A800, 0x00800)
	ROM_CONTINUE(	      0x3A800, 0x00800)
	ROM_CONTINUE(	      0x23000, 0x00800)
	ROM_CONTINUE(	      0x33000, 0x00800)
	ROM_CONTINUE(	      0x2B000, 0x00800)
	ROM_CONTINUE(	      0x3B000, 0x00800)
	ROM_CONTINUE(	      0x23800, 0x00800)
	ROM_CONTINUE(	      0x33800, 0x00800)
	ROM_CONTINUE(	      0x2B800, 0x00800)
	ROM_CONTINUE(	      0x3B800, 0x00800)
	ROM_LOAD( "TOKI.E12", 0x24000, 0x00800, 0xe2f714a9 )
	ROM_CONTINUE(	      0x34000, 0x00800)
	ROM_CONTINUE(	      0x2C000, 0x00800)
	ROM_CONTINUE(	      0x3C000, 0x00800)
	ROM_CONTINUE(	      0x24800, 0x00800)
	ROM_CONTINUE(	      0x34800, 0x00800)
	ROM_CONTINUE(	      0x2C800, 0x00800)
	ROM_CONTINUE(	      0x3C800, 0x00800)
	ROM_CONTINUE(	      0x25000, 0x00800)
	ROM_CONTINUE(	      0x35000, 0x00800)
	ROM_CONTINUE(	      0x2D000, 0x00800)
	ROM_CONTINUE(	      0x3D000, 0x00800)
	ROM_CONTINUE(	      0x25800, 0x00800)
	ROM_CONTINUE(	      0x35800, 0x00800)
	ROM_CONTINUE(	      0x2D800, 0x00800)
	ROM_CONTINUE(	      0x3D800, 0x00800)
	ROM_CONTINUE(	      0x26000, 0x00800)
	ROM_CONTINUE(	      0x36000, 0x00800)
	ROM_CONTINUE(	      0x2E000, 0x00800)
	ROM_CONTINUE(	      0x3E000, 0x00800)
	ROM_CONTINUE(	      0x26800, 0x00800)
	ROM_CONTINUE(	      0x36800, 0x00800)
	ROM_CONTINUE(	      0x2E800, 0x00800)
	ROM_CONTINUE(	      0x3E800, 0x00800)
	ROM_CONTINUE(	      0x27000, 0x00800)
	ROM_CONTINUE(	      0x37000, 0x00800)
	ROM_CONTINUE(	      0x2F000, 0x00800)
	ROM_CONTINUE(	      0x3F000, 0x00800)
	ROM_CONTINUE(	      0x27800, 0x00800)
	ROM_CONTINUE(	      0x37800, 0x00800)
	ROM_CONTINUE(	      0x2F800, 0x00800)
	ROM_CONTINUE(	      0x3F800, 0x00800)
	ROM_LOAD( "TOKI.E19", 0x40000, 0x00800, 0x8d6314f9 )
	ROM_CONTINUE(	      0x50000, 0x00800)
	ROM_CONTINUE(	      0x48000, 0x00800)
	ROM_CONTINUE(	      0x58000, 0x00800)
	ROM_CONTINUE(	      0x40800, 0x00800)
	ROM_CONTINUE(	      0x50800, 0x00800)
	ROM_CONTINUE(	      0x48800, 0x00800)
	ROM_CONTINUE(	      0x58800, 0x00800)
	ROM_CONTINUE(	      0x41000, 0x00800)
	ROM_CONTINUE(	      0x51000, 0x00800)
	ROM_CONTINUE(	      0x49000, 0x00800)
	ROM_CONTINUE(	      0x59000, 0x00800)
	ROM_CONTINUE(	      0x41800, 0x00800)
	ROM_CONTINUE(	      0x51800, 0x00800)
	ROM_CONTINUE(	      0x49800, 0x00800)
	ROM_CONTINUE(	      0x59800, 0x00800)
	ROM_CONTINUE(	      0x42000, 0x00800)
	ROM_CONTINUE(	      0x52000, 0x00800)
	ROM_CONTINUE(	      0x4A000, 0x00800)
	ROM_CONTINUE(	      0x5A000, 0x00800)
	ROM_CONTINUE(	      0x42800, 0x00800)
	ROM_CONTINUE(	      0x52800, 0x00800)
	ROM_CONTINUE(	      0x4A800, 0x00800)
	ROM_CONTINUE(	      0x5A800, 0x00800)
	ROM_CONTINUE(	      0x43000, 0x00800)
	ROM_CONTINUE(	      0x53000, 0x00800)
	ROM_CONTINUE(	      0x4B000, 0x00800)
	ROM_CONTINUE(	      0x5B000, 0x00800)
	ROM_CONTINUE(	      0x43800, 0x00800)
	ROM_CONTINUE(	      0x53800, 0x00800)
	ROM_CONTINUE(	      0x4B800, 0x00800)
	ROM_CONTINUE(	      0x5B800, 0x00800)
	ROM_LOAD( "TOKI.E14", 0x44000, 0x00800, 0xdf55f5cb )
	ROM_CONTINUE(	      0x54000, 0x00800)
	ROM_CONTINUE(	      0x4C000, 0x00800)
	ROM_CONTINUE(	      0x5C000, 0x00800)
	ROM_CONTINUE(	      0x44800, 0x00800)
	ROM_CONTINUE(	      0x54800, 0x00800)
	ROM_CONTINUE(	      0x4C800, 0x00800)
	ROM_CONTINUE(	      0x5C800, 0x00800)
	ROM_CONTINUE(	      0x45000, 0x00800)
	ROM_CONTINUE(	      0x55000, 0x00800)
	ROM_CONTINUE(	      0x4D000, 0x00800)
	ROM_CONTINUE(	      0x5D000, 0x00800)
	ROM_CONTINUE(	      0x45800, 0x00800)
	ROM_CONTINUE(	      0x55800, 0x00800)
	ROM_CONTINUE(	      0x4D800, 0x00800)
	ROM_CONTINUE(	      0x5D800, 0x00800)
	ROM_CONTINUE(	      0x46000, 0x00800)
	ROM_CONTINUE(	      0x56000, 0x00800)
	ROM_CONTINUE(	      0x4E000, 0x00800)
	ROM_CONTINUE(	      0x5E000, 0x00800)
	ROM_CONTINUE(	      0x46800, 0x00800)
	ROM_CONTINUE(	      0x56800, 0x00800)
	ROM_CONTINUE(	      0x4E800, 0x00800)
	ROM_CONTINUE(	      0x5E800, 0x00800)
	ROM_CONTINUE(	      0x47000, 0x00800)
	ROM_CONTINUE(	      0x57000, 0x00800)
	ROM_CONTINUE(	      0x4F000, 0x00800)
	ROM_CONTINUE(	      0x5F000, 0x00800)
	ROM_CONTINUE(	      0x47800, 0x00800)
	ROM_CONTINUE(	      0x57800, 0x00800)
	ROM_CONTINUE(	      0x4F800, 0x00800)
	ROM_CONTINUE(	      0x5F800, 0x00800)
	ROM_LOAD( "TOKI.E10",  0x60000, 0x00800, 0xa06d2c4d )
	ROM_CONTINUE(	      0x70000, 0x00800)
	ROM_CONTINUE(	      0x68000, 0x00800)
	ROM_CONTINUE(	      0x78000, 0x00800)
	ROM_CONTINUE(	      0x60800, 0x00800)
	ROM_CONTINUE(	      0x70800, 0x00800)
	ROM_CONTINUE(	      0x68800, 0x00800)
	ROM_CONTINUE(	      0x78800, 0x00800)
	ROM_CONTINUE(	      0x61000, 0x00800)
	ROM_CONTINUE(	      0x71000, 0x00800)
	ROM_CONTINUE(	      0x69000, 0x00800)
	ROM_CONTINUE(	      0x79000, 0x00800)
	ROM_CONTINUE(	      0x61800, 0x00800)
	ROM_CONTINUE(	      0x71800, 0x00800)
	ROM_CONTINUE(	      0x69800, 0x00800)
	ROM_CONTINUE(	      0x79800, 0x00800)
	ROM_CONTINUE(	      0x62000, 0x00800)
	ROM_CONTINUE(	      0x72000, 0x00800)
	ROM_CONTINUE(	      0x6A000, 0x00800)
	ROM_CONTINUE(	      0x7A000, 0x00800)
	ROM_CONTINUE(	      0x62800, 0x00800)
	ROM_CONTINUE(	      0x72800, 0x00800)
	ROM_CONTINUE(	      0x6A800, 0x00800)
	ROM_CONTINUE(	      0x7A800, 0x00800)
	ROM_CONTINUE(	      0x63000, 0x00800)
	ROM_CONTINUE(	      0x73000, 0x00800)
	ROM_CONTINUE(	      0x6B000, 0x00800)
	ROM_CONTINUE(	      0x7B000, 0x00800)
	ROM_CONTINUE(	      0x63800, 0x00800)
	ROM_CONTINUE(	      0x73800, 0x00800)
	ROM_CONTINUE(	      0x6B800, 0x00800)
	ROM_CONTINUE(	      0x7B800, 0x00800)
	ROM_LOAD( "TOKI.E6" , 0x64000, 0x00800, 0xa5a88cec )
	ROM_CONTINUE(	      0x74000, 0x00800)
	ROM_CONTINUE(	      0x6C000, 0x00800)
	ROM_CONTINUE(	      0x7C000, 0x00800)
	ROM_CONTINUE(	      0x64800, 0x00800)
	ROM_CONTINUE(	      0x74800, 0x00800)
	ROM_CONTINUE(	      0x6C800, 0x00800)
	ROM_CONTINUE(	      0x7C800, 0x00800)
	ROM_CONTINUE(	      0x65000, 0x00800)
	ROM_CONTINUE(	      0x75000, 0x00800)
	ROM_CONTINUE(	      0x6D000, 0x00800)
	ROM_CONTINUE(	      0x7D000, 0x00800)
	ROM_CONTINUE(	      0x65800, 0x00800)
	ROM_CONTINUE(	      0x75800, 0x00800)
	ROM_CONTINUE(	      0x6D800, 0x00800)
	ROM_CONTINUE(	      0x7D800, 0x00800)
	ROM_CONTINUE(	      0x66000, 0x00800)
	ROM_CONTINUE(	      0x76000, 0x00800)
	ROM_CONTINUE(	      0x6E000, 0x00800)
	ROM_CONTINUE(	      0x7E000, 0x00800)
	ROM_CONTINUE(	      0x66800, 0x00800)
	ROM_CONTINUE(	      0x76800, 0x00800)
	ROM_CONTINUE(	      0x6E800, 0x00800)
	ROM_CONTINUE(	      0x7E800, 0x00800)
	ROM_CONTINUE(	      0x67000, 0x00800)
	ROM_CONTINUE(	      0x77000, 0x00800)
	ROM_CONTINUE(	      0x6F000, 0x00800)
	ROM_CONTINUE(	      0x7F000, 0x00800)
	ROM_CONTINUE(	      0x67800, 0x00800)
	ROM_CONTINUE(	      0x77800, 0x00800)
	ROM_CONTINUE(	      0x6F800, 0x00800)
	ROM_CONTINUE(	      0x7F800, 0x00800)

	ROM_REGION(0x10000)	/* 64k for Z80 code */
	ROM_LOAD( "TOKI.E1", 0x00000, 0x10000, 0x391e1c32 )

ROM_END

static int hiload(void)
{
	void *f;

	/* check if the hi score table has already been initialized */

	if (memcmp(&ram[0x6BB6],"D TA",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&ram[0x6B66],180);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}


static void hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&ram[0x6B66],180);
		osd_fclose(f);
	}
}


struct GameDriver toki_driver =
{
	"Toki",
	"toki",
	"Jarek Parchanski (MAME driver)\nRichard Bush     (hardware info)\nRomek Bacza      (digi-sound)\n\nGame authors:\n\nKitahara Haruki\nNishizawa Takashi\nSakuma Akira\nAoki Tsukasa\nKakiuchi Hiroyuki",
	&machine_driver,
	toki_rom,
	0, 0,
	0,
	0,			/* sound_prom */
	input_ports,
	0, 0, 0, 	  	/* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

