/***************************************************************************

Crazy Climber memory map (preliminary)
as described by Lionel Theunissen (lionelth@ozemail.com.au)

Crazy Kong is very similar to Crazy Climber, there is an additional ROM at
5000-5fff and RAM is at 6000-6bff. Dip switches and input connections are
different as well.


0000h-4fffh ;20k program ROMs. ROM11=0000h
                               ROM10=1000h
                               ROM09=2000h
                               ROM08=3000h
                               ROM07=4000h

8000h-83ffh ;1k scratchpad RAM.
8800h-88ffh ;256 bytes Bigsprite RAM.
9000h-93ffh ;1k screen RAM.
9800h-981fh ;Column smooth scroll position. Corresponds to each char
             column.

9880h-989fh ;Sprite controls. 8 groups of 4 bytes:
  1st byte; code/attribute.
            Bits 0-5: sprite code.
            Bit    6: x invert.
            Bit    7: y invert.
  2nd byte ;color.
            Bits 0-3: colour. (palette scheme 0-15)
            Bit    4: 0=charset1, 1 =charset 2.
  3rd byte ;y position
  4th byte ;x position

98dc        bit 0  big sprite priority over sprites? (1 = less priority)
98ddh ;Bigsprite colour/attribute.
            Bit 0-2: Big sprite colour.
            bit 3  ??
            Bit   4: x invert.
            Bit   5: y invert.
98deh ;Bigsprite y position.
98dfh ;Bigsprite x position.

9c00h-9fffh ;1/2k colour RAM: Bits 0-3: colour. (palette scheme 0-15)
                              Bit    4: 0=charset1, 1=charset2.
                              Bit    5: (not used by CC)
                              Bit    6: x invert.
                              Bit    7: y invert. (not used by CC)

a000h ;RD: Player 1 controls.
            Bit 0: Left up
                1: Left down
                2: Left left
                3: Left right
                4: Right up
                5: Right down
                6: Right left
                7: Right right

a000h ;WR: Non Maskable interrupt.
            Bit 0: 0=NMI disable, 1=NMI enable.

a001h ;WR: Horizontal video direction (Crazy Kong sets it to 1).
            Bit 0: 0=Normal, 1=invert.

a002h ;WR: Vertical video direction (Crazy Kong sets it to 1).
            Bit 0: 0=Normal, 1=invert.

a004h ;WR: Sample trigger.
            Bit 0: 0=Trigger.

a800h ;RD: Player 2 controls (table model only).
            Bit 0: Left up
                1: Left down
                2: Left left
                3: Left right
                4: Right up
                5: Right down
                6: Right left
                7: Right right


a800h ;WR: Sample rate speed.
              Full byte value (0-255).

b000h ;RD: DIP switches.
            Bit 1,0: Number of climbers.
                     00=3, 01=4, 10=5, 11=6.
            Bit   2: Extra climber bonus.
                     0=30000, 1=50000.
            Bit   3: 1=Test Pattern
            Bit 5,4: Coins per credit.
                     00=1, 01=2, 10=3 11=4.
            Bit 7,6: Plays per credit.
                     00=1, 01=2, 10=3, 11=Freeplay.

b000h ;WR: Sample volume.
            Bits 0-5: Volume (0-31).

b800h ;RD: Machine switches.
            Bit 0: Coin 1.
            Bit 1: Coin 2.
            Bit 2: 1 Player start.
            Bit 3: 2 Player start.
            Bit 4: Upright/table select.
                   0=table, 1=upright.


I/O 8  ;AY-3-8910 Control Reg.
I/O 9  ;AY-3-8910 Data Write Reg.
I/O C  ;AY-3-8910 Data Read Reg.
        Port A of the 8910 selects the digital sample to play

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *cclimber_bsvideoram;
extern int cclimber_bsvideoram_size;
extern unsigned char *cclimber_bigspriteram;
extern unsigned char *cclimber_column_scroll;
void cclimber_flipscreen_w(int offset,int data);
void cclimber_colorram_w(int offset,int data);
void cclimber_bigsprite_videoram_w(int offset,int data);
void cclimber_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int cclimber_vh_start(void);
void cclimber_vh_stop(void);
void cclimber_vh_screenrefresh(struct osd_bitmap *bitmap);

void cclimber_sample_trigger_w(int offset,int data);
void cclimber_sample_rate_w(int offset,int data);
void cclimber_sample_volume_w(int offset,int data);
int cclimber_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },	/* 5000-5fff: Crazy Kong only */
	{ 0x6000, 0x6bff, MRA_RAM },	/* Crazy Kong only */
	{ 0x8000, 0x83ff, MRA_RAM },	/* Crazy Climber only */
	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9800, 0x981f, MRA_RAM },	/* column scroll registers */
	{ 0x9880, 0x989f, MRA_RAM },	/* sprite registers */
	{ 0x98dc, 0x98df, MRA_RAM },	/* bigsprite registers */
	{ 0x9c00, 0x9fff, MRA_RAM },	/* color RAM */
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },	/* IN1 */
	{ 0xb000, 0xb000, input_port_3_r },	/* DSW */
	{ 0xb800, 0xb800, input_port_2_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },	/* 5000-5fff: Crazy Kong only */
	{ 0x6000, 0x6bff, MWA_RAM },	/* Crazy Kong only */
	{ 0x8000, 0x83ff, MWA_RAM },	/* Crazy Climber only */
	{ 0x8800, 0x88ff, cclimber_bigsprite_videoram_w, &cclimber_bsvideoram, &cclimber_bsvideoram_size },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, videoram_w },	/* mirror address, used by Crazy Climber to draw windows */
	{ 0x9800, 0x981f, MWA_RAM, &cclimber_column_scroll },
	{ 0x9880, 0x989f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x98dc, 0x98df, MWA_RAM, &cclimber_bigspriteram },
	{ 0x9c00, 0x9fff, cclimber_colorram_w, &colorram },
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0xa001, 0xa002, cclimber_flipscreen_w },
	{ 0xa004, 0xa004, cclimber_sample_trigger_w },
	{ 0xa800, 0xa800, cclimber_sample_rate_w },
	{ 0xb000, 0xb000, cclimber_sample_volume_w },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x0c, 0x0c, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x08, 0x08, AY8910_control_port_0_w },
	{ 0x09, 0x09, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( cclimber_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY | IPF_COCKTAIL )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x10, 0x10, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x30, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xc0, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "Free Play" )
INPUT_PORTS_END

/* several differences with cclimber: note that IN2 bits are ACTIVE_LOW, while in */
/* cclimber they are ACTIVE_HIGH. */
INPUT_PORTS_START( ckong_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "7000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x08, "15000" )
	PORT_DIPSETTING(    0x0c, "20000" )
	PORT_DIPNAME( 0x70, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x50, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x60, "1 Coin/4 Credits" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
INPUT_PORTS_END




static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters (256 in Crazy Climber) */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout bscharlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites (64 in Crazy Climber) */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,      0, 16 },	/* char set #1 */
	{ 1, 0x2000, &charlayout,      0, 16 },	/* char set #2 */
	{ 1, 0x4000, &bscharlayout, 4*16,  8 },	/* big sprite char set */
	{ 1, 0x0000, &spritelayout,    0, 16 },	/* sprite set #1 */
	{ 1, 0x2000, &spritelayout,    0, 16 },	/* sprite set #2 */
	{ -1 } /* end of array */
};



static unsigned char cclimber_color_prom[] =
{
	/* char palette */
	0x00,0x79,0x04,0x87,0x00,0xb7,0xff,0x5f,0x00,0xc0,0xe8,0xf4,0x00,0x3f,0x04,0x38,
	0x00,0x0d,0x7a,0xb7,0x00,0x07,0x26,0x02,0x00,0x27,0x16,0x30,0x00,0xb7,0xf4,0x0c,
	0x00,0x4f,0xf6,0x24,0x00,0xb6,0xff,0x5f,0x00,0x33,0x00,0xb7,0x00,0x66,0x00,0x3a,
	0x00,0xc0,0x3f,0xb7,0x00,0x20,0xf4,0x16,0x00,0xff,0x7f,0x87,0x00,0xb6,0xf4,0xc0,
	/* bigsprite palette */
	0x00,0xff,0x18,0xc0,0x00,0xff,0xc6,0x8f,0x00,0x0f,0xff,0x1e,0x00,0xff,0xc0,0x67,
	0x00,0x47,0x7f,0x80,0x00,0x88,0x47,0x7f,0x00,0x7f,0x88,0x47,0x00,0x40,0x08,0xff
};

static unsigned char ckong_color_prom[] =
{
	/* char palette */
	0x00,0x3F,0x16,0xFF,0x00,0xC0,0xFF,0xA7,0x00,0xC8,0xE8,0x3F,0x00,0x27,0x16,0x2F,
	0x00,0x1F,0x37,0xFF,0x00,0xD0,0xC0,0xE8,0x00,0x07,0x27,0xF6,0x00,0x2F,0xF7,0xA7,
	0x00,0x2F,0xC0,0x16,0x00,0x07,0x27,0xD0,0x00,0x17,0x27,0xE8,0x00,0x07,0x1F,0xFF,
	0x00,0xE8,0xD8,0x07,0x00,0x3D,0xFF,0xE8,0x00,0x07,0x3F,0xD2,0x00,0xFF,0xD0,0xE0,
	/* bigsprite palette */
	0x00,0xFF,0x18,0xC0,0x00,0xFF,0xC6,0x8F,0x00,0x16,0x27,0x2F,0x00,0xFF,0xC0,0x67,
	0x00,0x47,0x7F,0x88,0x00,0x88,0x47,0x7F,0x00,0x7F,0x88,0x47,0x00,0x40,0x08,0xFF,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,readport,writeport,
			nmi_interrupt,1
		}
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	96,16*4+8*4,
	cclimber_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	cclimber_vh_start,
	cclimber_vh_stop,
	cclimber_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	cclimber_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( cclimber_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cc11", 0x0000, 0x1000, 0xda9892bc )
	ROM_LOAD( "cc10", 0x1000, 0x1000, 0x154b349b )
	ROM_LOAD( "cc09", 0x2000, 0x1000, 0x6b5227fc )
	ROM_LOAD( "cc08", 0x3000, 0x1000, 0x4a92862c )
	ROM_LOAD( "cc07", 0x4000, 0x1000, 0xc6a5d53b )

	ROM_REGION(0x5000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cc06", 0x0000, 0x0800, 0x8ceda6c7 )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc04", 0x1000, 0x0800, 0xda323f5a )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc05", 0x2000, 0x0800, 0xa26db1cf )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc03", 0x3000, 0x0800, 0x8eb7e34d )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc02", 0x4000, 0x0800, 0x25f097b6 )
	ROM_LOAD( "cc01", 0x4800, 0x0800, 0xb90b75dd )

	ROM_REGION(0x2000)	/* samples */
	ROM_LOAD( "cc13", 0x0000, 0x1000, 0xf33cfa4a )
	ROM_LOAD( "cc12", 0x1000, 0x1000, 0xe3e5f257 )
ROM_END

static void cclimber_decode(void)
{
/*
	translation mask is layed out like this:

	  0 1 2 3 4 5 6 7 8 9 a b c d e f
	0 <------A------> <------A------>
	1 <------B------> <------B------>
	2 <------A------> <------A------>
	3 <------B------> <------B------>
	4 <------C------> <------C------>
	5 <------D------> <------D------>
	6 <------C------> <------C------>
	7 <------D------> <------D------>
	8 <------E------> <------E------>
	9 <------F------> <------F------>
	a <------E------> <------E------>
	b <------F------> <------F------>
	c <------G------> <------G------>
	d <------H------> <------H------>
	e <------G------> <------G------>
	f <------H------> <------H------>

	Where <------A------> etc. are groups of 8 unrelated values.

	therefore in the following table we only keep track of <--A-->, <--B--> etc.
*/
	static const unsigned char xortable[2][64] =
	{
		/* -1 marks spots which are unused and therefore unknown */
		{
			0x44,0x15,0x45,0x11,0x50,0x15,0x15,0x41,
			0x01,0x50,0x15,0x41,0x11,0x45,0x45,0x11,
			0x11,0x41,0x01,0x55,0x04,0x10,0x51,0x05,
			0x15,0x55,0x51,0x05,0x55,0x40,0x01,0x55,
			0x54,0x50,0x51,0x05,0x11,0x40,0x14,  -1,
			0x54,0x10,0x40,0x51,0x05,0x54,0x14,  -1,
			0x44,0x14,0x01,0x40,0x14,  -1,0x41,0x50,
			0x50,0x41,0x41,0x45,0x14,  -1,0x10,0x01
		},
		{
			0x44,0x11,0x04,0x50,0x11,0x50,0x41,0x05,
			0x10,0x50,0x54,0x01,0x54,0x44,  -1,0x40,
			0x54,0x04,0x51,0x15,0x55,0x15,0x14,0x05,
			0x51,0x05,0x55,  -1,0x50,0x50,0x40,0x54,
			  -1,0x55,  -1,  -1,0x10,0x55,0x50,0x04,
			0x41,0x10,0x05,0x51,  -1,0x55,0x51,0x54,
			0x01,0x51,0x11,0x45,0x44,0x10,0x14,0x40,
			0x55,0x15,0x41,0x15,0x45,0x10,0x44,0x41
		}
	};
	int A;


	for (A = 0x0000;A < 0x10000;A++)
	{
		int i,j;
		unsigned char src;


		src = RAM[A];

		/* pick the translation table from bit 0 of the address */
		i = A & 1;

		/* pick the offset in the table from bits 012467 of the source data */
		j = (src & 0x07) + ((src & 0x10) >> 1) + ((src & 0xc0) >> 2);

		/* decode the opcodes */
		ROM[A] = src ^ xortable[i][j];
	}
}

ROM_START( ccjap_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cc11j.bin", 0x0000, 0x1000, 0x9ac39aa9 )
	ROM_LOAD( "cc10j.bin", 0x1000, 0x1000, 0x878c61ca )
	ROM_LOAD( "cc09j.bin", 0x2000, 0x1000, 0x32fdd4f5 )
	ROM_LOAD( "cc08j.bin", 0x3000, 0x1000, 0x398cbfc0 )
	ROM_LOAD( "cc07j.bin", 0x4000, 0x1000, 0xbe3cc484 )

	ROM_REGION(0x5000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cc06j.bin", 0x0000, 0x0800, 0x8ceda6c7 )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc04j.bin", 0x1000, 0x0800, 0xda323f5a )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc05j.bin", 0x2000, 0x0800, 0xa26db1cf )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc03j.bin", 0x3000, 0x0800, 0x8eb7e34d )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc02j.bin", 0x4000, 0x0800, 0x25f097b6 )
	ROM_LOAD( "cc01j.bin", 0x4800, 0x0800, 0xb90b75dd )

	ROM_REGION(0x2000)	/* samples */
	ROM_LOAD( "cc13j.bin", 0x0000, 0x1000, 0x9f4339e5 )
	ROM_LOAD( "cc12j.bin", 0x1000, 0x1000, 0xe921f6f5 )
ROM_END

ROM_START( ccboot_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "m11.bin", 0x0000, 0x1000, 0xdd73b251 )
	ROM_LOAD( "m10.bin", 0x1000, 0x1000, 0x890ef772 )
	ROM_LOAD( "m09.bin", 0x2000, 0x1000, 0x32fdd4f5 )
	ROM_LOAD( "m08.bin", 0x3000, 0x1000, 0x068011fe )
	ROM_LOAD( "m07.bin", 0x4000, 0x1000, 0xbe3cc484 )

	ROM_REGION(0x5000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "m06.bin", 0x0000, 0x0800, 0x8ceda6c7 )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "m04.bin", 0x1000, 0x0800, 0x91305aa8 )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "m05.bin", 0x2000, 0x0800, 0xe883dff7 )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "m03.bin", 0x3000, 0x0800, 0xd4cd8d75 )
	/* empy hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "m02.bin", 0x4000, 0x0800, 0x2bed2d1d )
	ROM_LOAD( "m01.bin", 0x4800, 0x0800, 0x82637d0f )

	ROM_REGION(0x2000)	/* samples */
	ROM_LOAD( "m13.bin", 0x0000, 0x1000, 0x9f4339e5 )
	ROM_LOAD( "m12.bin", 0x1000, 0x1000, 0xe921f6f5 )
ROM_END

static void ccjap_decode(void)
{
/*
	translation mask is layed out like this:

	  0 1 2 3 4 5 6 7 8 9 a b c d e f
	0 <------A------> <------A------>
	1 <------B------> <------B------>
	2 <------A------> <------A------>
	3 <------B------> <------B------>
	4 <------C------> <------C------>
	5 <------D------> <------D------>
	6 <------C------> <------C------>
	7 <------D------> <------D------>
	8 <------E------> <------E------>
	9 <------F------> <------F------>
	a <------E------> <------E------>
	b <------F------> <------F------>
	c <------G------> <------G------>
	d <------H------> <------H------>
	e <------G------> <------G------>
	f <------H------> <------H------>

	Where <------A------> etc. are groups of 8 unrelated values.

	therefore in the following table we only keep track of <--A-->, <--B--> etc.
*/
	static const unsigned char xortable[2][64] =
	{
		{
			0x41,0x55,0x44,0x10,0x55,0x11,0x04,0x55,
			0x15,0x01,0x51,0x45,0x15,0x40,0x10,0x01,
			0x04,0x50,0x55,0x01,0x44,0x15,0x15,0x10,
			0x45,0x11,0x55,0x41,0x50,0x10,0x55,0x10,
			0x14,0x40,0x05,0x54,0x05,0x41,0x04,0x55,
			0x14,0x41,0x01,0x51,0x45,0x50,0x40,0x01,
			0x51,0x01,0x05,0x10,0x10,0x50,0x54,0x41,
			0x40,0x51,0x14,0x50,0x01,0x50,0x15,0x40
		},
		{
			0x50,0x10,0x10,0x51,0x44,0x50,0x50,0x50,
			0x41,0x05,0x11,0x55,0x51,0x11,0x54,0x11,
			0x14,0x54,0x54,0x50,0x54,0x40,0x44,0x04,
			0x14,0x50,0x15,0x44,0x54,0x14,0x05,0x50,
			0x01,0x04,0x55,0x51,0x45,0x40,0x11,0x15,
			0x44,0x41,0x11,0x15,0x41,0x05,0x55,0x51,
			0x51,0x54,0x05,0x01,0x15,0x51,0x41,0x45,
			0x14,0x11,0x41,0x45,0x50,0x55,0x05,0x01
		}
	};
	int A;


	for (A = 0x0000;A < 0x10000;A++)
	{
		int i,j;
		unsigned char src;


		src = RAM[A];

		/* pick the translation table from bit 0 of the address */
		i = A & 1;

		/* pick the offset in the table from bits 012467 of the source data */
		j = (src & 0x07) + ((src & 0x10) >> 1) + ((src & 0xc0) >> 2);

		/* decode the opcodes */
		ROM[A] = src ^ xortable[i][j];
	}
}

ROM_START( ckong_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "7.dat",  0x0000, 0x1000, 0xc6efa047 )
	ROM_LOAD( "8.dat",  0x1000, 0x1000, 0xb6c21834 )
	ROM_LOAD( "9.dat",  0x2000, 0x1000, 0xa71d0d79 )
	ROM_LOAD( "10.dat", 0x3000, 0x1000, 0x68fee770 )
	ROM_LOAD( "11.dat", 0x4000, 0x1000, 0x18a93c23 )
	ROM_LOAD( "12.dat", 0x5000, 0x1000, 0xe72c50f6 )

	ROM_REGION(0x5000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6.dat",  0x0000, 0x1000, 0x29097247 )
	ROM_LOAD( "4.dat",  0x1000, 0x1000, 0xd8e27c6e )
	ROM_LOAD( "5.dat",  0x2000, 0x1000, 0xbb5521c9 )
	ROM_LOAD( "3.dat",  0x3000, 0x1000, 0x8aef534f )
	ROM_LOAD( "2.dat",  0x4000, 0x0800, 0x9c1f9d15 )
	ROM_LOAD( "1.dat",  0x4800, 0x0800, 0x9cbf41cb )

	ROM_REGION(0x2000)	/* samples */
	ROM_LOAD( "14.dat", 0x0000, 0x1000, 0x9f4339e5 )
	ROM_LOAD( "13.dat", 0x1000, 0x1000, 0xe921f6f5 )
ROM_END

ROM_START( ckonga_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "D05-07.bin", 0x0000, 0x1000, 0xc6efa047 )
	ROM_LOAD( "F05-08.bin", 0x1000, 0x1000, 0xb6c21834 )
	ROM_LOAD( "H05-09.bin", 0x2000, 0x1000, 0xa71d0d79 )
	ROM_LOAD( "K05-10.bin", 0x3000, 0x1000, 0x1cfee570 )
	ROM_LOAD( "L05-11.bin", 0x4000, 0x1000, 0x18a93c23 )
	ROM_LOAD( "N05-12.bin", 0x5000, 0x1000, 0xe72c50f6 )

	ROM_REGION(0x5000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "N11-06.bin", 0x0000, 0x1000, 0x29097247 )
	ROM_LOAD( "K11-04.bin", 0x1000, 0x1000, 0xd8e27c6e )
	ROM_LOAD( "L11-05.bin", 0x2000, 0x1000, 0xbb5521c9 )
	ROM_LOAD( "H11-03.bin", 0x3000, 0x1000, 0x8aef534f )
	ROM_LOAD( "C11-02.bin", 0x4000, 0x0800, 0x9c1f9d15 )
	ROM_LOAD( "A11-01.bin", 0x4800, 0x0800, 0x9cbf41cb )

	ROM_REGION(0x2000)	/* samples */
	ROM_LOAD( "S05-14.bin", 0x0000, 0x1000, 0x9f4339e5 )
	ROM_LOAD( "R05-13.bin", 0x1000, 0x1000, 0xe921f6f5 )
ROM_END

ROM_START( ckongjeu_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "7.dat",  0x0000, 0x1000, 0xc6efa047 )
	ROM_LOAD( "8.dat",  0x1000, 0x1000, 0xb6c21834 )
	ROM_LOAD( "9.dat",  0x2000, 0x1000, 0xa71d0d79 )
	ROM_LOAD( "10.dat", 0x3000, 0x1000, 0x5beeee78 )
	ROM_LOAD( "11.dat", 0x4000, 0x1000, 0x18a93c23 )
	ROM_LOAD( "12.dat", 0x5000, 0x1000, 0x10bfbb61 )

	ROM_REGION(0x5000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6.dat",  0x0000, 0x1000, 0x29097247 )
	ROM_LOAD( "4.dat",  0x1000, 0x1000, 0xd8e27c6e )
	ROM_LOAD( "5.dat",  0x2000, 0x1000, 0xbb5521c9 )
	ROM_LOAD( "3.dat",  0x3000, 0x1000, 0x8aef534f )
	ROM_LOAD( "2.dat",  0x4000, 0x0800, 0x9c1f9d15 )
	ROM_LOAD( "1.dat",  0x4800, 0x0800, 0x9cbf41cb )

	ROM_REGION(0x2000)	/* samples */
	ROM_LOAD( "14.dat", 0x0000, 0x1000, 0x9f4339e5 )
	ROM_LOAD( "13.dat", 0x1000, 0x1000, 0xe921f6f5 )
ROM_END



static int cclimber_hiload(void)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8083],"\x02\x00\x00",3) == 0 &&
			memcmp(&RAM[0x808f],"\x02\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8083],17*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void cclimber_hisave(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8083],17*5);
		osd_fclose(f);
	}
}



static int ckong_hiload(void)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x611d],"\x50\x76\x00",3) == 0 &&
			memcmp(&RAM[0x61a5],"\x00\x43\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x6100],34*5);
			RAM[0x60b8] = RAM[0x611d];
			RAM[0x60b9] = RAM[0x611e];
			RAM[0x60ba] = RAM[0x611f];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void ckong_hisave(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x6100],34*5);
		osd_fclose(f);
	}
}



struct GameDriver cclimber_driver =
{
	"Crazy Climber (US version)",
	"cclimber",
	"Lionel Theunissen (hardware info and ROM decryption)\nNicola Salmoria (MAME driver)",
	&machine_driver,

	cclimber_rom,
	0, cclimber_decode,
	0,

	0/*TBR*/,cclimber_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	cclimber_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	cclimber_hiload, cclimber_hisave
};

struct GameDriver ccjap_driver =
{
	"Crazy Climber (Japanese version)",
	"ccjap",
	"Lionel Theunissen (hardware info and ROM decryption)\nNicola Salmoria (MAME driver)",
	&machine_driver,

	ccjap_rom,
	0, ccjap_decode,
	0,

	0/*TBR*/,cclimber_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	cclimber_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	cclimber_hiload, cclimber_hisave
};

struct GameDriver ccboot_driver =
{
	"Crazy Climber (bootleg)",
	"ccboot",
	"Lionel Theunissen (hardware info and ROM decryption)\nNicola Salmoria (MAME driver)",
	&machine_driver,

	ccboot_rom,
	0, ccjap_decode,
	0,

	0/*TBR*/,cclimber_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	cclimber_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	cclimber_hiload, cclimber_hisave
};



struct GameDriver ckong_driver =
{
	"Crazy Kong (Crazy Climber hardware)",
	"ckong",
	"Nicola Salmoria (MAME driver)\nVille Laitinen (adaptation from Crazy Climber)\nDoug Jefferys (color info)\nTim Lindquist (color info)",
	&machine_driver,

	ckong_rom,
	0, 0,
	0,

	0/*TBR*/,ckong_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	ckong_color_prom, 0, 0,
	ORIENTATION_ROTATE_270,

	ckong_hiload, ckong_hisave
};

struct GameDriver ckonga_driver =
{
	"Crazy Kong (alternate version)",
	"ckonga",
	"Nicola Salmoria (MAME driver)\nVille Laitinen (adaptation from Crazy Climber)\nDoug Jefferys (color info)\nTim Lindquist (color info)",
	&machine_driver,

	ckonga_rom,
	0, 0,
	0,

	0/*TBR*/,ckong_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	ckong_color_prom, 0, 0,
	ORIENTATION_ROTATE_270,

	ckong_hiload, ckong_hisave
};

struct GameDriver ckongjeu_driver =
{
	"Crazy Kong (Jeutel bootleg)",
	"ckongjeu",
	"Nicola Salmoria (MAME driver)\nVille Laitinen (adaptation from Crazy Climber)\nDoug Jefferys (color info)\nTim Lindquist (color info)",
	&machine_driver,

	ckongjeu_rom,
	0, 0,
	0,

	0/*TBR*/,ckong_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	ckong_color_prom, 0, 0,
	ORIENTATION_ROTATE_270,

	ckong_hiload, ckong_hisave
};
