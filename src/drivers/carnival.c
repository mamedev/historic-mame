/***************************************************************************

Carnival memory map (preliminary)

0000-3fff ROM mirror image (not used, apart from startup and NMI)
4000-7fff ROM
e000-e3ff Video RAM + other
e400-e7ff RAM
e800-efff Character RAM

I/O ports:
read:
00        IN0
          bit 4 = ?

01        IN1
          bit 3 = vblank
          bit 4 = LEFT
          bit 5 = RIGHT

02        IN2
          bit 4 = START 1
          bit 5 = FIRE

03        IN3
          bit 3 = COIN (must reset the CPU to make the game acknowledge it)
          bit 5 = START 2

write:
01		  bit 0 = fire gun
 		  bit 1 = hit object     	* See Port 2
          bit 2 = duck 1
          bit 3 = duck 2
          bit 4 = duck 3
          bit 5 = hit pipe
          bit 6 = bonus
          bit 7 = hit background

02        bit 2 = Switch effect for hit object - Bear
          bit 3 = Music On/Off
          bit 4 = ? (may be used as signal to sound processor)
          bit 5 = Ranking (not implemented)

08        ?

40        This should be a port to select the palette bank, however it is
          never accessed by Carnival.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


int carnival_IN1_r(int offset);
int carnival_interrupt(void);

extern unsigned char *carnival_characterram;
void carnival_characterram_w(int offset,int data);
void carnival_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void carnival_vh_screenrefresh(struct osd_bitmap *bitmap);

void carnival_sh_port1_w(int offset, int data);			/* MJC */
void carnival_sh_port2_w(int offset, int data);			/* MJC */
void carnival_sh_update(void);

static struct MemoryReadAddress readmem[] =
{
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
	{ 0xe400, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xefff, carnival_characterram_w, &carnival_characterram },
	{ 0x4000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x01, 0x01, carnival_sh_port1_w },						/* MJC */
  	{ 0x02, 0x02, carnival_sh_port2_w },						/* MJC */
	{ -1 }	/* end of table */
};


static struct InputPort input_ports[] =
{
	{       /* IN0 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN1 */
		0xff,
		{ 0, 0, 0, IPB_VBLANK, OSD_KEY_LEFT, OSD_KEY_RIGHT, 0, 0 },
		{ 0, 0, 0, 0, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, 0 }
	},
	{       /* IN2 */
		0xff,
		{ 0, 0, 0, 0, OSD_KEY_1, OSD_KEY_CONTROL, 0, 0 },
		{ 0, 0, 0, 0, 0, OSD_JOY_FIRE, 0, 0 }
	},
	{       /* IN3 */
		0xff,
		{ 0, 0, 0, OSD_KEY_3, 0, OSD_KEY_2, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 1, 4, "MOVE LEFT"  },
        { 1, 5, "MOVE RIGHT" },
        { 2, 5, "JUMP" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 0, 0x10, "IN0 BIT 4", { "0", "1" } },
	{ -1 }
};


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0xe800, &charlayout, 0, 32 },	/* the game dynamically modifies this */
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* U49: palette */
	0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60,0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60,
	0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60,0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,readport,writeport,
			carnival_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	64,32*2,
	carnival_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	carnival_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	carnival_sh_update											/* MJC */
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( carnival_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "651u33.cpu", 0x0000, 0x0400, 0x661fa2ef )
	ROM_RELOAD(             0x4000, 0x0400 )
	ROM_LOAD( "652u32.cpu", 0x4400, 0x0400, 0x5172c958 )
	ROM_LOAD( "653u31.cpu", 0x4800, 0x0400, 0x3771ff13 )
	ROM_LOAD( "654u30.cpu", 0x4c00, 0x0400, 0x6b0caeb6 )
	ROM_LOAD( "655u29.cpu", 0x5000, 0x0400, 0x15283224 )
	ROM_LOAD( "656u28.cpu", 0x5400, 0x0400, 0xec2fd83b )
	ROM_LOAD( "657u27.cpu", 0x5800, 0x0400, 0x9a30851e )
	ROM_LOAD( "658u26.cpu", 0x5c00, 0x0400, 0x914e4bf2 )
	ROM_LOAD( "659u8.cpu",  0x6000, 0x0400, 0x22c6547c )
	ROM_LOAD( "660u7.cpu",  0x6400, 0x0400, 0xba76241e )
	ROM_LOAD( "661u6.cpu",  0x6800, 0x0400, 0x3bab3df7 )
	ROM_LOAD( "662u5.cpu",  0x6c00, 0x0400, 0xc315eb83 )
	ROM_LOAD( "663u4.cpu",  0x7000, 0x0400, 0x434d30f7 )
	ROM_LOAD( "664u3.cpu",  0x7400, 0x0400, 0x75f1b261 )
	ROM_LOAD( "665u2.cpu",  0x7800, 0x0400, 0xecdd5165 )
	ROM_LOAD( "666u1.cpu",  0x7c00, 0x0400, 0x24809182 )
ROM_END



static const char *carnival_sample_names[] =					/* MJC */
{
	"C1.SAM",	/* Fire Gun 1 */
	"C2.SAM",	/* Hit Target */
	"C3.SAM",	/* Duck 1 */
	"C4.SAM",	/* Duck 2 */
	"C5.SAM",	/* Duck 3 */
	"C6.SAM",	/* Hit Pipe */
	"C7.SAM",	/* BONUS */
	"C8.SAM",	/* Hit bonus box */
    "C9.SAM",	/* Fire Gun 2 */
    "C10.SAM",	/* Hit Bear */
	0       	/* end of array */
};



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0xE397],"\x00\x00\x00",3) == 0) &&
		(memcmp(&RAM[0xE5A2],"   ",3) == 0))
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			/* Read the scores */
                        fread(&RAM[0xE397],1,2*30,f);
			/* Read the initials */
                        fread(&RAM[0xE5A2],1,9,f);
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
		/* Save the scores */
                fwrite(&RAM[0xE397],1,2*30,f);
		/* Save the initials */
                fwrite(&RAM[0xE5A2],1,9,f);
		fclose(f);
	}

}



struct GameDriver carnival_driver =
{
	"Carnival",
	"carnival",
	"MIKE COATES\nRICHARD DAVIES\nNICOLA SALMORIA\nMIKE BALFOUR",
	&machine_driver,

	carnival_rom,
	0, 0,
	carnival_sample_names,										/* MJC */

	input_ports, 0, trak_ports, dsw, keys,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
