/***************************************************************************

MCR/I memory map

0000-0fff ROM 0
1000-1fff ROM 1
2000-2fff ROM 2
3000-3fff ROM 3
4000-4fff ROM 4
5000-5fff ROM 5
7000-7fff RAM
f000-f3ff sprite ram
f400-f81f palette ram
fc00-ff7f tiles

IN0

bit 0 : left coin
bit 1 : right coin
bit 2 : 1 player
bit 3 : 2 player
bit 4 : trigger button
bit 5 : tilt
bit 6 : ?
bit 7 : service

IN1 (kick)

spinner knob

IN1 (solar fox)

bit 0: left
bit 1: right
bit 2: up
bit 3: down

IN2

DSW1

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"
#include "Z80.h"

void mcr1_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void solarfox_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void mcr1_vh_screenrefresh(struct osd_bitmap *bitmap);
void mcr1_palette_w(int offset,int data);
extern unsigned char *mcr1_paletteram;

void mcr_init_machine(void);
int mcr_interrupt(void);
extern int mcr_loadnvram;

void mcr_writeport(int port,int value);
int mcr_readport(int port);
void mcr_soundstatus_w (int offset,int data);
int mcr_soundlatch_r (int offset);

int mcr_sh_interrupt(void);
int mcr_sh_start(void);

static struct MemoryReadAddress mcr1_readmem[] =
{
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x7fff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress mcr1_writemem[] =
{
	{ 0x7000, 0x77ff, MWA_RAM },
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0xf000, 0xf1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xfc00, 0xffff, videoram_w, &videoram, &videoram_size },
	{ 0xf400, 0xf81f, mcr1_palette_w, &mcr1_paletteram },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x9003, mcr_soundlatch_r },
	{ 0xa001, 0xa001, AY8910_read_port_0_r },
	{ 0xb001, 0xb001, AY8910_read_port_1_r },
	{ 0xf000, 0xf000, input_port_5_r },
	{ 0xe000, 0xe000, MRA_NOP },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xa000, 0xa000, AY8910_control_port_0_w },
	{ 0xa002, 0xa002, AY8910_write_port_0_w },
	{ 0xb000, 0xb000, AY8910_control_port_1_w },
	{ 0xb002, 0xb002, AY8910_write_port_1_w },
	{ 0xc000, 0xc000, mcr_soundstatus_w },
	{ 0xe000, 0xe000, MWA_NOP },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct IOWritePort writeport[] =
{
   { 0, 0xFF, mcr_writeport },
   { -1 }	/* end of table */
};



static struct IOReadPort readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, input_port_1_r },
   { 0x02, 0x02, input_port_2_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOReadPort kick_readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, input_port_1_r },
   { 0x02, 0x02, input_port_2_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

INPUT_PORTS_START( kick_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 -- this is the Kick spinner input.  */
	PORT_ANALOG ( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 100, 5, 0, 0 )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
 	PORT_DIPNAME( 0x01, 0x00, "Music", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

INPUT_PORTS_START( solarfox_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout kick_charlayout =
{
	16, 16,
	256,	/* 256 characters */
	4,
	{ 256*16*8, 256*16*8+1, 0, 1 },	/* bit planes */
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14 },
	{ 0, 0, 2*8, 2*8, 4*8, 4*8, 6*8, 6*8, 8*8, 8*8, 10*8, 10*8, 12*8, 12*8, 14*8, 14*8 },
	16*8	/* every char takes 16 bytes */
};

#define X (64*128*8)
#define Y (2*X)
#define Z (3*X)
static struct GfxLayout kick_spritelayout =
{
   32,32,	/* 32x32 sprites */
   64,	        /* 128 sprites */
   4,	        /* 4 bit planes */
   { 0, 1, 2, 3 },
   {  Z+0, Z+4, Y+0, Y+4, X+0, X+4, 0, 4, Z+8, Z+12, Y+8, Y+12, X+8, X+12, 8, 12,
      Z+16, Z+20, Y+16, Y+20, X+16, X+20, 16, 20, Z+24, Z+28, Y+24, Y+28,
      X+24, X+28, 24, 28 },
   {  0, 32, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7, 32*8, 32*9, 32*10, 32*11,
      32*12, 32*13, 32*14, 32*15, 32*16, 32*17, 32*18, 32*19, 32*20, 32*21,
      32*22, 32*23, 32*24, 32*25, 32*26, 32*27, 32*28, 32*29, 32*30, 32*31 },
   128*8	/* bits per sprite per plane */
};
#undef X
#undef Y
#undef Z

static struct GfxDecodeInfo kick_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &kick_charlayout,   0, 2 },
	{ 1, 0x2000, &kick_spritelayout, 0, 2 },
	{ -1 } /* end of array */
};

static struct GfxLayout solarfox_charlayout =
{
	16, 16,
	256,	/* 256 characters */
	4,
	{ 256*16*8, 256*16*8+1, 0, 1 },	/* bit planes */
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14 },
	{ 0, 0, 2*8, 2*8, 4*8, 4*8, 6*8, 6*8, 8*8, 8*8, 10*8, 10*8, 12*8, 12*8, 14*8, 14*8 },
	16*8	/* every char takes 16 bytes */
};

#define X (64*128*8)
#define Y (2*X)
#define Z (3*X)
static struct GfxLayout solarfox_spritelayout =
{
   32,32,	/* 32x32 sprites */
   64,	        /* 128 sprites */
   4,	        /* 4 bit planes */
   { 0, 1, 2, 3 },
   {  Z+0, Z+4, Y+0, Y+4, X+0, X+4, 0, 4, Z+8, Z+12, Y+8, Y+12, X+8, X+12, 8, 12,
      Z+16, Z+20, Y+16, Y+20, X+16, X+20, 16, 20, Z+24, Z+28, Y+24, Y+28,
      X+24, X+28, 24, 28 },
   {  0, 32, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7, 32*8, 32*9, 32*10, 32*11,
      32*12, 32*13, 32*14, 32*15, 32*16, 32*17, 32*18, 32*19, 32*20, 32*21,
      32*22, 32*23, 32*24, 32*25, 32*26, 32*27, 32*28, 32*29, 32*30, 32*31 },
   128*8	/* bits per sprite per plane */
};
#undef X
#undef Y
#undef Z

static struct GfxDecodeInfo solarfox_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &solarfox_charlayout,   0, 2 },
	{ 1, 0x2000, &solarfox_spritelayout, 0, 2 },
	{ -1 } /* end of array */
};

static struct MachineDriver kick_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2500000,	/* 2.5 Mhz */
			0,
			mcr1_readmem,mcr1_writemem,kick_readport,writeport,
			mcr_interrupt,32
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			mcr_sh_interrupt,26
		}
	},
	30,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	mcr_init_machine,

	/* video hardware */
	32*16, 29*16, { 0, 32*16-1, 0, 29*16-1 },
	kick_gfxdecodeinfo,
	8*16, 8*16,
	mcr1_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr1_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	mcr_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};


static struct MachineDriver solarfox_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2500000,	/* 2.5 Mhz */
			0,
			mcr1_readmem,mcr1_writemem,readport,writeport,
			mcr_interrupt,32
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			mcr_sh_interrupt,26
		}
	},
	30,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	mcr_init_machine,

	/* video hardware */
	32*16, 29*16, { 0, 32*16-1, 0, 29*16-1 },
	solarfox_gfxdecodeinfo,
	8*16, 8*16,
	solarfox_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr1_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	mcr_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  High score save/load

***************************************************************************/

static int mcr1_hiload(int addr, int len)
{
   unsigned char *RAM = Machine->memory_region[0];

   /* see if it's okay to load */
   if (mcr_loadnvram)
   {
      void *f;

		f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
      if (f)
      {
			osd_fread(f,&RAM[addr],len);
			osd_fclose (f);
      }
      return 1;
   }
   else return 0;	/* we can't load the hi scores yet */
}

static void mcr1_hisave(int addr, int len)
{
   unsigned char *RAM = Machine->memory_region[0];
   void *f;

	f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);
   if (f)
   {
      osd_fwrite(f,&RAM[addr],len);
      osd_fclose (f);
   }
}

static int  kick_hiload(void)     { return mcr1_hiload(0x7000, 0x91); }
static void kick_hisave(void)     {        mcr1_hisave(0x7000, 0x91); }

static int  solarfox_hiload(void)
{
   unsigned char *RAM = Machine->memory_region[0];

   /* see if it's okay to load */
   if (mcr_loadnvram)
   {
      void *f;

		f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
      if (f)
      {
			osd_fread(f,&RAM[0x7000],0x86);
			osd_fclose (f);
      }
      else
      {
      	/* leaving RAM all-zero is not a happy thing for solarfox */
      	static unsigned char init[] = { 0,0,1,1,1,1,1,3,3,3,7,0,0,0,0,0 };
      	memcpy (&RAM[0x7000], init, sizeof (init));
      }
      return 1;
   }
   else return 0;	/* we can't load the hi scores yet */
}
static void solarfox_hisave(void) {        mcr1_hisave(0x7000, 0x86); }


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kick_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cpu_b3", 0x0000, 0x1000, 0xb8eacdee )
	ROM_LOAD( "cpu_b4", 0x1000, 0x1000, 0x34b3c82d )
	ROM_LOAD( "cpu_b5", 0x2000, 0x1000, 0x962e11c0 )
	ROM_LOAD( "cpu_d4", 0x3000, 0x1000, 0xb8cf9c4b )
	ROM_LOAD( "cpu_d5", 0x4000, 0x1000, 0x59d87236 )
	ROM_LOAD( "cpu_d6", 0x5000, 0x1000, 0x59cd7833 )

	ROM_REGION(0x0a000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cpu_g4", 0x0000, 0x1000, 0x4bdb4c47 )
	ROM_LOAD( "cpu_g5", 0x1000, 0x1000, 0xbfe7b19f )
	ROM_LOAD( "vid_a1", 0x2000, 0x2000, 0x7dbb39e5 )
	ROM_LOAD( "vid_b1", 0x4000, 0x2000, 0xdc44b7e0 )
	ROM_LOAD( "vid_d1", 0x6000, 0x2000, 0x9abb5055 )
	ROM_LOAD( "vid_e1", 0x8000, 0x2000, 0x6d2149ad )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "sio_a7", 0x0000, 0x1000, 0x02d981f3 )
	ROM_LOAD( "sio_a8", 0x1000, 0x1000, 0xc5223442 )
	ROM_LOAD( "sio_a9", 0x2000, 0x1000, 0xbe9ef560 )
	ROM_LOAD( "sio_a10", 0x3000, 0x1000, 0x1667420b )
ROM_END

ROM_START( solarfox_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sfcpu.3b", 0x0000, 0x1000, 0x1b63bbb9 )
	ROM_LOAD( "sfcpu.4b", 0x1000, 0x1000, 0x9f1f7dc7 )
	ROM_LOAD( "sfcpu.5b", 0x2000, 0x1000, 0x4b7d8bc1 )
	ROM_LOAD( "sfcpu.4d", 0x3000, 0x1000, 0xae2ca80e )
	ROM_LOAD( "sfcpu.5d", 0x4000, 0x1000, 0x426dcdab )
	ROM_LOAD( "sfcpu.6d", 0x5000, 0x1000, 0x508903cd )
	ROM_LOAD( "sfcpu.7d", 0x6000, 0x1000, 0x51468c4e )

	ROM_REGION(0x0a000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sfcpu.4g", 0x0000, 0x1000, 0x44e0e0c0 )
	ROM_LOAD( "sfcpu.5g", 0x1000, 0x1000, 0xb69f2685 )
	ROM_LOAD( "sfvid.1a", 0x8000, 0x2000, 0x1e42fb6a )
	ROM_LOAD( "sfvid.1b", 0x6000, 0x2000, 0xb92428f4 )
	ROM_LOAD( "sfvid.1d", 0x4000, 0x2000, 0x6eeff5c5 )
	ROM_LOAD( "sfvid.1e", 0x2000, 0x2000, 0x803cf0ae )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "sfsnd.7a", 0x0000, 0x1000, 0xc3945494 )
	ROM_LOAD( "sfsnd.8a", 0x1000, 0x1000, 0xd43589ef )
	ROM_LOAD( "sfsnd.9a", 0x2000, 0x1000, 0x9f5bd101 )

ROM_END

struct GameDriver kick_driver =
{
	"Kick",
	"kick",
	"CHRISTOPHER KIRMSE\nAARON GILES\nNICOLA SALMORIA\nBRAD OLIVER\nJOHN BUTLER",
	&kick_machine_driver,

	kick_rom,
	0, 0,
	0,

	0/*TBR*/, kick_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, 0,0,
	ORIENTATION_SWAP_XY,

	kick_hiload,kick_hisave
};

struct GameDriver solarfox_driver =
{
	"Solar Fox",
	"solarfox",
	"CHRISTOPHER KIRMSE\nAARON GILES\nNICOLA SALMORIA\nBRAD OLIVER",
	&solarfox_machine_driver,

	solarfox_rom,
	0, 0,
	0,

	0/*TBR*/, solarfox_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, 0,0,
	ORIENTATION_SWAP_XY,

	solarfox_hiload,solarfox_hisave
};

