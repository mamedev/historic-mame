/***************************************************************************
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"

void rastan_paletteram_w(int offset,int data);
int rastan_paletteram_r(int offset);
void rastan_spriteram_w(int offset,int data);
int rastan_spriteram_r(int offset);
void rastan_videoram1_w(int offset,int data);
int rastan_videoram1_r(int offset);
void rastan_videoram2_w(int offset,int data);
int rastan_videoram2_r(int offset);
void rastan_videoram3_w(int offset,int data);
int rastan_videoram3_r(int offset);
void rastan_videoram4_w(int offset,int data);
int rastan_videoram4_r(int offset);

void rastan_scrollY_w(int offset,int data);
void rastan_scrollX_w(int offset,int data);

void rastan_ram_w(int offset,int data);
int rastan_ram_r(int offset);

int rastan_interrupt(void);

void rastan_background_w(int offset,int data);
void rastan_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void rastan_vh_screenrefresh(struct osd_bitmap *bitmap);

int  rastan_vh_start(void);
void rastan_vh_stop(void);

int rastan_s_interrupt(void);

void rastan_sound_port_w(int offset,int data);
int rastan_sound_port_r(int offset);
void rastan_sound_comm_w(int offset,int data);
int rastan_sound_comm_r(int offset);


int rYMport(int offset);
int rYMdata(int offset);
void wYMport(int offset,int data);
void wYMdata(int offset,int data);

int r_rd_a000(int offset);
int r_rd_a001(int offset);
void r_wr_a000(int offset,int data);
void r_wr_a001(int offset,int data);

/*void osd_ym2151_update(void); */

int  rastan_sh_init(const char *gamename);
void r_wr_b000(int offset,int data);
void r_wr_c000(int offset,int data);
void r_wr_d000(int offset,int data);

void rastan_machine_init(void);

static struct MemoryReadAddress rastan_readmem[] =
{
	{ 0x200000, 0x20ffff, rastan_paletteram_r },
	{ 0xc00000, 0xc03fff, rastan_videoram1_r},
	{ 0xc04000, 0xc07fff, rastan_videoram2_r},
	{ 0xc08000, 0xc0bfff, rastan_videoram3_r},
	{ 0xc0c000, 0xc0ffff, rastan_videoram4_r},
	{ 0xd00000, 0xd0ffff, rastan_spriteram_r},

//	{ 0x3e0001, 0x3e0001, rastan_sound_port_r },
	{ 0x3e0003, 0x3e0003, rastan_sound_comm_r },

	{ 0x390001, 0x390001, input_port_0_r },
	{ 0x390003, 0x390003, input_port_0_r },
	{ 0x390005, 0x390005, input_port_1_r },
	{ 0x390007, 0x390007, input_port_2_r },

	{ 0x390009, 0x390009, input_port_3_r },	/* DSW 1 */
	{ 0x39000b, 0x39000b, input_port_4_r },	/* DSW 2 */
	{ 0x10c000, 0x10ffff, rastan_ram_r },	/* RAM */
	{ 0x000000, 0x05ffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rastan_writemem[] =
{
	{ 0x200000, 0x20ffff, rastan_paletteram_w },
	{ 0xc00000, 0xc03fff, rastan_videoram1_w, &videoram, &videoram_size }, /*this is just a fake */
	{ 0xc04000, 0xc07fff, rastan_videoram2_w },
	{ 0xc08000, 0xc0bfff, rastan_videoram3_w },
	{ 0xc0c000, 0xc0ffff, rastan_videoram4_w },

	{ 0xc20000, 0xc20003, rastan_scrollY_w },  /* scroll Y  1st.w plane1  2nd.w plane2 */
	{ 0xc40000, 0xc40003, rastan_scrollX_w },  /* scroll X  1st.w plane1  2nd.w plane2 */
	{ 0xd00000, 0xd0ffff, rastan_spriteram_w },

	{ 0x3e0001, 0x3e0001, rastan_sound_port_w },
	{ 0x3e0003, 0x3e0003, rastan_sound_comm_w },

	{ 0xc50000, 0xc50001, MWA_NOP },     /* 0 only (rarely)*/
	{ 0x350008, 0x350009, MWA_NOP },     /* 0 only (often) ? */
	{ 0x380000, 0x380001, MWA_NOP },     /*0000,0060,0063,0063b   ? */
	{ 0x3c0000, 0x3c0001, MWA_NOP },     /*0000,0020,0063,0992,1753 (very often) watchdog? */
	{ 0x10c000, 0x10ffff, rastan_ram_w },
	{ 0x000000, 0x05ffff, MWA_ROM },
	{ -1 }  /* end of table */
};




static struct MemoryReadAddress rastan_s_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
        { 0x9000, 0x9000, rYMport },
        { 0x9001, 0x9001, rYMdata },
        { 0x9002, 0x9100, MRA_RAM },
        { 0xa000, 0xa000, r_rd_a000 },
        { 0xa001, 0xa001, r_rd_a001 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rastan_s_writemem[] =
{
	{ 0x8000, 0x8fff, MWA_RAM },
        { 0xa000, 0xa000, r_wr_a000 },
        { 0xa001, 0xa001, r_wr_a001 },
	{ 0x9000, 0x9000, wYMport },
	{ 0x9001, 0x9001, wYMdata },
        { 0xb000, 0xb000, r_wr_b000 },
        { 0xc000, 0xc000, r_wr_c000 },
        { 0xd000, 0xd000, r_wr_d000 },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }  /* end of table */
};




static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_CONTROL, OSD_KEY_ALT, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff-0x40-0x20 -0x80,
		{ OSD_KEY_7, 0, OSD_KEY_6, OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, OSD_KEY_4, OSD_KEY_8 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }  /* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct KEYSet keys[] =
{
        { 0, 0, "MOVE UP" },
        { 0, 2, "MOVE LEFT"  },
        { 0, 3, "MOVE RIGHT" },
        { 0, 1, "MOVE DOWN" },
        { 0, 4, "FIRE" },
        { 0, 5, "WARP" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x04, "TEST MODE", { "ON", "OFF" } },
	{ 4, 0x03, "DIFFICULTY", { "HARDEST", "DIFFICULT", "EASIEST", "EASY" } },
		/* not a mistake, EASIEST and EASY are swapped */
	{ 4, 0x0c, "BONUS PLAYER", { "250 000 PTS", "200 000 PTS", "150 000 PTS", "100 000 PTS" } },
	{ 4, 0x30, "LIVES", { "6", "5", "4", "3" } },
	{ 4, 0x40, "CONTINUE MODE", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout spritelayout1 =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
        { 0, 4, 0x10000*8+0 ,0x10000*8+4, 8+0, 8+4, 0x10000*8+8+0, 0x10000*8+8+4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout2 =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
{
0, 4, 0x10000*8+0 ,0x10000*8+4,
8+0, 8+4, 0x10000*8+8+0, 0x10000*8+8+4,
16+0, 16+4, 0x10000*8+16+0, 0x10000*8+16+4,
24+0, 24+4, 0x10000*8+24+0, 0x10000*8+24+4
},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the remapped color table and dynamically build the real one. */

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
	{ 1, 0x00000, &spritelayout1,  0, 0x50 },	/* sprites 8x8*/
	{ 1, 0x20000, &spritelayout1,  0, 0x50 },	/* sprites 8x8*/
	{ 1, 0x40000, &spritelayout2,  0, 0x50 },	/* sprites 16x16*/
	{ 1, 0x60000, &spritelayout2,  0, 0x50 },	/* sprites 16x16*/
	{ 0, 0,      &fakelayout,    0x50*16, 0xff },
	{ -1 } /* end of array */
};



/* RASTAN doesn't have a color PROM, it uses a RAM to generate colors */
/* and change them during the game. Here is the list of all the colors is uses. */
/* We cannot do it this time :( I've checked : RASTAN uses more than 512 colors*/


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 Mhz */
			0,
			rastan_readmem,rastan_writemem,0,0,
			rastan_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz */
			2,
			rastan_s_readmem,rastan_s_writemem,0,0,
			rastan_s_interrupt,4
		}
	},
	60,
	rastan_machine_init,

	/* video hardware */
	40*8, 30*8, { 0*8, 40*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	256,0x50*16+256, /* looking on palette it seems that RASTAN uses 0x0-0x4f color schemes 16 colors each*/
	rastan_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	rastan_vh_start,
	rastan_vh_stop,
	rastan_vh_screenrefresh,

	/* sound hardware */
	0,
	rastan_sh_init,
	0, //rastan_sh_start,
	0, //AY8910_sh_stop,
	0  //osd_ym2151_update  //AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( rastan_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD( "IC19_38.bin", 0x00000, 0x10000, 0 )
	ROM_LOAD( "IC07_37.bin", 0x10000, 0x10000, 0 )
	ROM_LOAD( "IC20_40.bin", 0x20000, 0x10000, 0 )
	ROM_LOAD( "IC08_39.bin", 0x30000, 0x10000, 0 )
	ROM_LOAD( "IC21_42.bin", 0x40000, 0x10000, 0 )
	ROM_LOAD( "IC09_43.bin", 0x50000, 0x10000,0  )

	ROM_REGION(0x80000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "IC40_01.bin",  0x00000, 0x10000, 0 )	/* 8x8 0 */
	ROM_LOAD( "IC67_02.bin",  0x10000, 0x10000, 0 )	/* 8x8 1 */
	ROM_LOAD( "IC39_03.bin",  0x20000, 0x10000, 0 )	/* 8x8 0 */
	ROM_LOAD( "IC66_04.bin",  0x30000, 0x10000, 0 )	/* 8x8 1 */
	ROM_LOAD( "IC15_05.bin",  0x40000, 0x10000, 0 )	/* sprites 1a */
	ROM_LOAD( "IC28_06.bin",  0x50000, 0x10000, 0 )	/* sprites 1b */
	ROM_LOAD( "IC14_07.bin",  0x60000, 0x10000, 0 )	/* sprites 2a */
	ROM_LOAD( "IC27_08.bin",  0x70000, 0x10000, 0 )	/* sprites 2b */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "IC49_19.bin", 0x0000, 0x10000, 0 )   /* Audio CPU is a Z80  */
                                                     /* sound chip is YM2151*/
	ROM_REGION(0x10000)	/* 64k for the samples */
	ROM_LOAD( "IC76_20.bin", 0x0000, 0x10000, 0 ) /* samples are 4bit ADPCM */
ROM_END



struct GameDriver rastan_driver =
{
	"RASTAN",
	"rastan",
	"JAREK BURCZYNSKI",
	&machine_driver,

	rastan_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

