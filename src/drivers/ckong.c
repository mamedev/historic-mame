/***************************************************************************

Crazy Kong memory map

Very similar to Crazy Climber, there is an additional ROM at 5000-5fff.
RAM is at 6000-6bff

*
 * DSW1 (bits NOT inverted)
 * bit 7 : COCKTAIL or UPRIGHT cabinet (1 = UPRIGHT)
 * bit 6 : \ 000 = 1 coin 1 play   001 = 2 coins 1 play  010 = 1 coin 2 plays
 * bit 5 : | 011 = 3 coins 1 play  100 = 1 coin 3 plays  101 = 4 coins 1 play
 * bit 4 : / 110 = 1 coin 4 plays  111 = 5 coins 1 play
 * bit 3 : \bonus at
 * bit 2 : / 00 = 7000  01 = 10000  10 = 15000  11 = 20000
 * bit 1 : \ 00 = 3 lives  01 = 4 lives
 * bit 0 : / 10 = 5 lives  11 = 6 lives
 *

Notable differences are: (thanks to Ville Laitinen)
 address	bits		r/w
 0x1234	76543210
 ----------------------------------------------------------------
 0xA000	76543xxx	r	player 1 control:
 		10000000	r	right
 		01000000	r	left
 		00100000	r	down
 		00010000	r	up
 		00001000	r	jump
 0xA800	76543xxx	r	player 2 controls, same as 0xa000

 0xb800	xxxx321x	r	coin/start: (inverted input - not like CC)
 		11110111	r	2 pl start
 		11111011	r	1 pl start
 		11111101	r	insert coin

 0xb000	76543210	r	dipswitches
 		10000000	r	cabinet upright (0 for cocktail)
 		00001100	r	bonus at:
 					00 -> 7000
 					else 5000 + 5000 * ((0xb000) >> 2)&0x03
 		01110000	r	coins/credits
 					000 1 coin  / 1 credit
 					001 1 coin  / 2 cr
 					010 1 coin  / 3 cr
 					011 1 coin  / 4 cr
 					100 2 coins / 1 cr
 					101 3 coins / 1 cr
 					110 4 coins / 1 cr
 					111 5 coins / 1 cr
 		xxxxx10	r	no of jumpmen (actually (3 + (0xb000)&0x03) )

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *ckong_bsvideoram;
extern unsigned char *ckong_bigspriteram;
extern unsigned char *ckong_row_scroll;
extern void ckong_colorram_w(int offset,int data);
extern void ckong_bigsprite_videoram_w(int offset,int data);
extern void cclimber_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int ckong_vh_start(void);
extern void ckong_vh_stop(void);
extern void ckong_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void cclimber_sample_trigger_w(int offset,int data);
extern void cclimber_sample_rate_w(int offset,int data);
extern void cclimber_sample_volume_w(int offset,int data);
extern int cclimber_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x6000, 0x6bff, MRA_RAM },
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x9880, 0x989f, MRA_RAM },	/* sprite registers */
	{ 0x98dc, 0x98df, MRA_RAM },	/* bigsprite registers */
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },	/* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },	/* DSW1 */
	{ 0xb800, 0xb800, input_port_3_r },	/* IN2 */
	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9c00, 0x9fff, MRA_RAM },	/* color RAM */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x6000, 0x6bff, MWA_RAM },
	{ 0x9880, 0x989f, MWA_RAM, &spriteram },
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0x9000, 0x93ff, videoram_w, &videoram },
	{ 0x9c00, 0x9fff, ckong_colorram_w, &colorram },
	{ 0x8800, 0x88ff, ckong_bigsprite_videoram_w, &ckong_bsvideoram },
	{ 0x98dc, 0x98df, MWA_RAM, &ckong_bigspriteram },
	{ 0x9800, 0x981f, MWA_RAM, &ckong_row_scroll },
	{ 0xa004, 0xa004, cclimber_sample_trigger_w },
	{ 0xb000, 0xb000, cclimber_sample_volume_w },
	{ 0xa800, 0xa800, cclimber_sample_rate_w },
	{ 0xa001, 0xa002, MWA_NOP },
	{ 0x0000, 0x5fff, MWA_ROM },
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



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ 0, 0, 0, OSD_KEY_CONTROL,
				OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT },
		{ 0, 0, 0, OSD_JOY_FIRE,
				OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x84,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 2, 0x03, "LIVES", { "3", "4", "5", "6" } },
	{ 2, 0x0c, "BONUS", { "7000", "10000", "15000", "20000" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout bscharlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
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



static unsigned char color_prom[] =
{
	/* char palette */
	0x00,0x3F,0x16,0xFF,0x00,0xC0,0xFF,0xA7,0x00,0xC8,0xE8,0x3F,0x00,0x27,0x16,0x2F,
	0x00,0x1F,0x37,0xFF,0x00,0xD0,0xC0,0xE8,0x00,0x07,0x27,0xF6,0x00,0x2F,0xF7,0xA7,
	0x00,0x2F,0xC0,0x16,0x00,0x07,0x27,0xD0,0x00,0x17,0x27,0xE8,0x00,0x07,0x1F,0xFF,
	0x00,0xE8,0xD8,0x07,0x00,0x3D,0xFF,0xE8,0x00,0x07,0x3F,0xD2,0x00,0xFF,0xD0,0xE0,
	/* bigsprite palette */
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x16,0x27,0x2F,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x40,0x08,0xFF
/*	0x00,0xFF,0xC6,0x8F,0x00,0xFF,0xC6,0x8F,0x00,0xFF,0xC0,0x67,0x00,0xFF,0xC0,0x67, */
/*	0x00,0x88,0x47,0x7F,0x00,0x88,0x47,0x7F,0x00,0x40,0x08,0xFF,0x00,0x40,0x08,0xFF, */
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
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	96,4*24,
	cclimber_vh_convert_color_prom,

	0,
	ckong_vh_start,
	ckong_vh_stop,
	ckong_vh_screenrefresh,

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

ROM_START( ckong_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "7.dat",  0x0000, 0x1000 )
	ROM_LOAD( "8.dat",  0x1000, 0x1000 )
	ROM_LOAD( "9.dat",  0x2000, 0x1000 )
	ROM_LOAD( "10.dat", 0x3000, 0x1000 )
	ROM_LOAD( "11.dat", 0x4000, 0x1000 )
	ROM_LOAD( "12.dat", 0x5000, 0x1000 )

	ROM_REGION(0x5000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6.dat",  0x0000, 0x1000 )
	ROM_LOAD( "4.dat",  0x1000, 0x1000 )
	ROM_LOAD( "5.dat",  0x2000, 0x1000 )
	ROM_LOAD( "3.dat",  0x3000, 0x1000 )
	ROM_LOAD( "2.dat",  0x4000, 0x0800 )
	ROM_LOAD( "1.dat",  0x4800, 0x0800 )

	ROM_REGION(0x2000)	/* samples */
	ROM_LOAD( "14.dat", 0x0000, 0x1000 )
	ROM_LOAD( "13.dat", 0x1000, 0x1000 )
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x611d],"\x50\x76\x00",3) == 0 &&
			memcmp(&RAM[0x61a5],"\x00\x43\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x6100],1,34*5,f);
			RAM[0x60b8] = RAM[0x611d];
			RAM[0x60b9] = RAM[0x611e];
			RAM[0x60ba] = RAM[0x611f];
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x6100],1,34*5,f);
		fclose(f);
	}
}



struct GameDriver ckong_driver =
{
	"ckong",
	&machine_driver,

	ckong_rom,
	0, 0,
	0,

	input_ports, dsw,

	color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,	/* letters */
		0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a },
	0x00, 0x02,
	8*13, 8*16, 0x0c,

	hiload, hisave
};
