/***************************************************************************

Popeye memory map (preliminary)

0000-7fff  ROM

8000-87ff  RAM
8c00       background x position
8c01       background y position
8c02       ?
8c03       bit 3: background palette bank
           bit 0-2: sprite palette bank
8c04-8e7f  sprites
8f00-8fff  RAM (stack)

a000-a3ff  Text video ram
a400-a7ff  Text Attribute

c000-cfff  Background bitmap. Accessed as nibbles: bit 7 selects which of
           the two nibbles should be written to.


I/O 0  ;AY-3-8910 Control Reg.
I/O 1  ;AY-3-8910 Data Write Reg.
        write to port B: select bit of DSW2 to read in bit 7 of port A (0-2-4-6-8-a-c-e)
I/O 3  ;AY-3-8910 Data Read Reg.
        read from port A: bit 0-5 = DSW1  bit 7 = bit of DSW1 selected by port B

        DSW1
		bit 0-3 = coins per play (0 = free play)
		bit 4-5 = ?

		DSW2
		bit 0-1 = lives
		bit 2-3 = difficulty
		bit 4-5 = bonus
		bit 6 = demo sounds
		bit 7 = cocktail/upright (0 = upright)

I/O 2  ;bit 0 Coin in 1
        bit 1 Coin in 2
        bit 2 Coin in 3 = 5 credits
        bit 3
        bit 4 Start 2 player game
        bit 5 Start 1 player game
        bit 6
        bit 7

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *popeye_videoram;
extern int popeye_videoram_size;
extern unsigned char *popeye_background_pos,*popeye_palette_bank;
void popeye_backgroundram_w(int offset,int data);
void popeye_videoram_w(int offset,int data);
void popeye_colorram_w(int offset,int data);
void popeye_palettebank_w(int offset,int data);
void popeye_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void popeye_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  popeye_vh_start(void);
void popeye_vh_stop(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8c00, 0x8e7f, MRA_RAM },
	{ 0x8f00, 0x8fff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8c04, 0x8e7f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x8f00, 0x8fff, MWA_RAM },
	{ 0xa000, 0xa3ff, videoram_w, &videoram, &videoram_size },
	{ 0xa400, 0xa7ff, colorram_w, &colorram },
	{ 0xc000, 0xcfff, popeye_videoram_w, &popeye_videoram, &popeye_videoram_size },
	{ 0x8c00, 0x8c01, MWA_RAM, &popeye_background_pos },
	{ 0x8c03, 0x8c03, popeye_palettebank_w, &popeye_palette_bank },
	{ -1 }	/* end of table */
};



static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* probably unused */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x08, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x00, "Free play" )
/*  0x0e = 2 Coins/1 Credit
    0x07, 0x0c = 1 Coin/1 Credit
    0x01, 0x0b, 0x0d = 1 Coin/2 Credits */
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 7 scans DSW1 one bit at a time */

	PORT_START	/* DSW1 (FAKE - appears as bit 7 of DSW0, see code below) */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x08, "Medium" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "40000" )
	PORT_DIPSETTING(    0x20, "60000" )
	PORT_DIPSETTING(    0x10, "80000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	16,16,	/* 16*16 characters (8*8 doubled) */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel (there are two bitplanes in the ROM, but only one is used) */
	{ 0 },
	{ 7,7, 6,6, 5,5, 4,4, 3,3, 2,2, 1,1, 0,0 },	/* pretty straightforward layout */
	{ 0*8,0*8, 1*8,1*8, 2*8,2*8, 3*8,3*8, 4*8,4*8, 5*8,5*8, 6*8,6*8, 7*8,7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 0x4000*8 },	/* the two bitplanes are separated in different files */
	{7+(0x2000*8),6+(0x2000*8),5+(0x2000*8),4+(0x2000*8),
     3+(0x2000*8),2+(0x2000*8),1+(0x2000*8),0+(0x2000*8),
   7,6,5,4,3,2,1,0 },
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
    7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8, },
	16*8	/* every sprite takes 16 consecutive bytes */
};
/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the background color table. */
static struct GfxLayout fakelayout =
{
	1,1,
	0,
	1,
	{ 0 },
	{ 0 },
	{ 0 },
	0
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0800, &charlayout,        32, 16 },	/* chars */
	{ 1, 0x1000, &spritelayout, 32+16*2, 64 },	/* sprites */
	{ 1, 0x2000, &spritelayout, 32+16*2, 64 },	/* sprites */
	{ 0, 0,      &fakelayout,         0, 32 },	/* background bitmap */
	{ -1 } /* end of array */
};



static int dswbit;

static void popeye_portB_w(int offset,int data)
{
	/* bit 0 does something - RV in the schematics */

	/* bits 1-3 select DSW1 bit to read */
	dswbit = (data & 0x0e) >> 1;
}

static int popeye_portA_r(int offset)
{
	int res;


	res = input_port_3_r(offset);
	res |= (input_port_4_r(offset) << (7-dswbit)) & 0x80;

	return res;
}

static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	2000000,	/* 2 MHz */
	{ 255 },
	{ popeye_portA_r },
	{ 0 },
	{ 0 },
	{ popeye_portB_w }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,readport,writeport,
			nmi_interrupt,2
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*16, 30*16, { 0*16, 32*16-1, 1*16, 29*16-1 },
	gfxdecodeinfo,
	32+16+256, 32+16*2+64*4,
	popeye_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	popeye_vh_start,
	popeye_vh_stop,
	popeye_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( popeye_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "c-7a",         0x0000, 0x2000, 0x9af7c821 )
	ROM_LOAD( "c-7b",         0x2000, 0x2000, 0xc3704958 )
	ROM_LOAD( "c-7c",         0x4000, 0x2000, 0x5882ebf9 )
	ROM_LOAD( "c-7e",         0x6000, 0x2000, 0xef8649ca )

	ROM_REGION_DISPOSE(0x9000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "v-5n",         0x0000, 0x1000, 0xcca61ddd )
	ROM_LOAD( "v-1e",         0x1000, 0x2000, 0x0f2cd853 )
	ROM_LOAD( "v-1f",         0x3000, 0x2000, 0x888f3474 )
	ROM_LOAD( "v-1j",         0x5000, 0x2000, 0x7e864668 )
	ROM_LOAD( "v-1k",         0x7000, 0x2000, 0x49e1d170 )

	ROM_REGION(0x0240)	/* color proms */
	ROM_LOAD( "popeye.pr1",   0x0000, 0x0020, 0xd138e8a4 ) /* background palette */
	ROM_LOAD( "popeye.pr2",   0x0020, 0x0020, 0x0f364007 ) /* char palette */
	ROM_LOAD( "popeye.pr3",   0x0040, 0x0100, 0xca4d7b6a ) /* sprite palette - low 4 bits */
	ROM_LOAD( "popeye.pr4",   0x0140, 0x0100, 0xcab9bc53 ) /* sprite palette - high 4 bits */
ROM_END

ROM_START( popeyebl_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "po1",          0x0000, 0x2000, 0xb14a07ca )
	ROM_LOAD( "po2",          0x2000, 0x2000, 0x995475ff )
	ROM_LOAD( "po3",          0x4000, 0x2000, 0x99d6a04a )
	ROM_LOAD( "po4",          0x6000, 0x2000, 0x548a6514 )
	ROM_LOAD( "po_d1-e1.bin", 0xe000, 0x0020, 0x8de22998 )	/* protection PROM */

	ROM_REGION_DISPOSE(0x9000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "v-5n",         0x0000, 0x1000, 0xcca61ddd )
	ROM_LOAD( "v-1e",         0x1000, 0x2000, 0x0f2cd853 )
	ROM_LOAD( "v-1f",         0x3000, 0x2000, 0x888f3474 )
	ROM_LOAD( "v-1j",         0x5000, 0x2000, 0x7e864668 )
	ROM_LOAD( "v-1k",         0x7000, 0x2000, 0x49e1d170 )

	ROM_REGION(0x0240)	/* color proms */
	ROM_LOAD( "popeye.pr1",   0x0000, 0x0020, 0xd138e8a4 ) /* background palette */
	ROM_LOAD( "popeye.pr2",   0x0020, 0x0020, 0x0f364007 ) /* char palette */
	ROM_LOAD( "popeye.pr3",   0x0040, 0x0100, 0xca4d7b6a ) /* sprite palette - low 4 bits */
	ROM_LOAD( "popeye.pr4",   0x0140, 0x0100, 0xcab9bc53 ) /* sprite palette - high 4 bits */
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8209],"\x00\x26\x03",3) == 0 &&
			memcmp(&RAM[0x8221],"\x50\x11\x02",3) == 0 &&
			memcmp(&RAM[0x8fed],"\x00\x26\x03",3) == 0)	/* high score */
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int i;


			osd_fread(f,&RAM[0x8200],6+6*5);

			i = RAM[0x8201];

			RAM[0x8fed] = RAM[0x8200+i-2];
			RAM[0x8fee] = RAM[0x8200+i-1];
			RAM[0x8fef] = RAM[0x8200+i];


			/* I think the first lValue of the next sentences *
			 * is unnecessary. Please confirm if you know it  */
			RAM[0x8f32] = RAM[0xA04F] = RAM[0x8200+i] >> 4;
			RAM[0x8f33] = RAM[0xA050] = RAM[0x8200+i] & 0x0f;
			RAM[0x8f34] = RAM[0xA051] = RAM[0x8200+i-1] >> 4;
			RAM[0x8f35] = RAM[0xA052] = RAM[0x8200+i-1] & 0x0f;
			RAM[0x8f36] = RAM[0xA053] = RAM[0x8200+i-2] >> 4;
			RAM[0x8f37] = RAM[0xA054] = RAM[0x8200+i-2] & 0x0f;

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8200],6+6*5);
		osd_fclose(f);
	}
}



/* The original doesn't work because the ROMs are encrypted. */
/* The encryption is based on a custom ALU and seems to be dynamically evolving */
/* (like Jr. PacMan). I think it decodes 16 bits at a time, bits 0-2 are (or can be) */
/* an opcode for the ALU and the others contain the data. */
struct GameDriver popeye_driver =
{
	__FILE__,
	0,
	"popeye",
	"Popeye",
	"1982?",
	"Nintendo",
	"Marc Lafontaine\nNicola Salmoria\nMarco Cassili",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	popeye_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload,hisave
};

struct GameDriver popeyebl_driver =
{
	__FILE__,
	&popeye_driver,
	"popeyebl",
	"Popeye (bootleg)",
	"1982?",
	"bootleg",
	"Marc Lafontaine\nNicola Salmoria\nMarco Cassili",
	0,
	&machine_driver,
	0,

	popeyebl_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload,hisave
};
