/***************************************************************************

Food Fight Memory Map
-----------------------------------

Function                           Address        R/W  DATA
-------------------------------------------------------------
Program ROM                        000000-00FFFF  R    D0-D15
Program RAM                        014000-01BFFF  R/W  D0-D15
Motion Object RAM                  01C000-01CFFF  R/W  D0-D15

Motion Objects:
  Vertical Position                xxxx00              D0-D7
  Horizontal Position              xxxx00              D8-D15
  Picture                          xxxx10              D0-D7
  Color                            xxxx10              D8-D13
  VFlip                            xxxx10              D14
  HFlip                            xxxx10              D15

Playfield                          800000-8007FF  R/W  D0-D15
  Picture                          xxxxx0              D0-D7+D15
  Color                            xxxxx0              D8-D13

NVRAM                              900000-9001FF  R/W  D0-D3
Analog In                          940000-940007  R    D0-D7
Analog Out                         944000-944007  W

Coin 1 (Digital In)                948000         R    D0
Coin 2                                            R    D1
Start 1                                           R    D2
Start 2                                           R    D3
Coin Aux                                          R    D4
Throw 1                                           R    D5
Throw 2                                           R    D6
Test                                              R    D7

PFFlip                             948000         W    D0
Update                                            W    D1
INT3RST                                           W    D2
INT4RST                                           W    D3
LED 1                                             W    D4
LED 2                                             W    D5
COUNTERL                                          W    D6
COUNTERR                                          W    D7

Color RAM                          950000-9503FF  W    D0-D7
Recall                             954000         W
Watchdog                           958000         W
Audio 1                            A40000-A4001F  R/W  D0-D7
Audio 0                            A80000-A8001F  R/W  D0-D7
Audio 2                            AC0000-AC001F  R/W  D0-D7

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/pokyintf.h"

extern int foodf_nvram_size;
extern int foodf_playfieldram_size;
extern int foodf_spriteram_size;
extern int foodf_paletteram_size;

extern unsigned char *foodf_spriteram;

int foodf_playfieldram_r (int offset);
int foodf_nvram_r (int offset);
int foodf_analog_r (int offset);
int foodf_digital_r (int offset);
int foodf_pokey1_r (int offset);
int foodf_pokey2_r (int offset);
int foodf_pokey3_r (int offset);

void foodf_playfieldram_w (int offset, int data);
void foodf_nvram_w (int offset, int data);
void foodf_analog_w (int offset, int data);
void foodf_digital_w (int offset, int data);
void foodf_paletteram_w (int offset, int data);
void foodf_pokey1_w (int offset, int data);
void foodf_pokey2_w (int offset, int data);
void foodf_pokey3_w (int offset, int data);

unsigned char *foodf_nvram_base (void);

int foodf_interrupt(void);

int foodf_vh_start(void);
void foodf_vh_stop(void);

void foodf_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void foodf_vh_screenrefresh(struct osd_bitmap *bitmap);

int foodf_sh_start (void);


static struct MemoryReadAddress foodf_readmem[] =
{
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x014000, 0x01bfff, MRA_RAM },
	{ 0x01c000, 0x01cfff, MRA_RAM, &foodf_spriteram, &foodf_spriteram_size },
	{ 0x800000, 0x8007ff, foodf_playfieldram_r, 0, &foodf_playfieldram_size },
	{ 0x900000, 0x9001ff, foodf_nvram_r, 0, &foodf_nvram_size },
	{ 0x940000, 0x940007, foodf_analog_r },
	{ 0x948000, 0x948003, foodf_digital_r },
	{ 0x94c000, 0x94c003, MRA_NOP }, /* Used from PC 0x776E */
	{ 0x958000, 0x958003, MRA_NOP },
	{ 0xa40000, 0xa4001f, foodf_pokey1_r },
	{ 0xa80000, 0xa8001f, foodf_pokey2_r },
	{ 0xac0000, 0xac001f, foodf_pokey3_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress foodf_writemem[] =
{
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x014000, 0x01bfff, MWA_RAM },
	{ 0x01c000, 0x01cfff, MWA_RAM },
	{ 0x800000, 0x8007ff, foodf_playfieldram_w },
	{ 0x900000, 0x9001ff, foodf_nvram_w },
	{ 0x944000, 0x944007, foodf_analog_w },
	{ 0x948000, 0x948003, foodf_digital_w },
	{ 0x950000, 0x9503ff, foodf_paletteram_w, 0, &foodf_paletteram_size },
	{ 0x954000, 0x954003, MWA_NOP },
	{ 0x958000, 0x958003, MWA_NOP },
	{ 0xa40000, 0xa4001f, foodf_pokey1_w },
	{ 0xa80000, 0xa8001f, foodf_pokey2_w },
	{ 0xac0000, 0xac001f, foodf_pokey3_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( foodf_ports )
	PORT_START	/* IN0 */
	PORT_ANALOG ( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER1 | IPF_REVERSE, 100, 0, 0, 255 )

	PORT_START	/* IN1 */
	PORT_ANALOG ( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER2 | IPF_REVERSE | IPF_COCKTAIL, 100, 0, 0, 255 )

	PORT_START	/* IN2 */
	PORT_ANALOG ( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_PLAYER1, 100, 0, 0, 255 )

	PORT_START	/* IN3 */
	PORT_ANALOG ( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_COCKTAIL, 100, 0, 0, 255 )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	512,	/* 512 chars */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*16	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 of them */
	2,		/* 2 bits per pixel */
	{ 8*0x2000, 0 },
	{ 8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*32	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x4000, &charlayout,      0, 64 },		/* characters 8x8 */
	{ 1, 0x0000, &spritelayout,    0, 64 },		/* sprites & playfield */
	{ -1 } /* end of array */
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 6 Mhz really, but use 8 to account for fake 68k timing */
			0,
			foodf_readmem,foodf_writemem,0,0,
			foodf_interrupt,4
		},
	},
	60,
	1,
	0,

	/* video hardware */
	32*8, 28*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	256,4*64,
	foodf_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
	0,
	foodf_vh_start,
	foodf_vh_stop,
	foodf_vh_screenrefresh,

	/* sound hardware */
	0,
	foodf_sh_start,
	pokey_sh_stop,
	pokey_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( foodf_rom )
	ROM_REGION(0x20000)	/* 128k for 68000 code + RAM */
	ROM_LOAD_EVEN( "foodf.9c",  0x00000, 0x02000, 0x428f3769 )
	ROM_LOAD_ODD ( "foodf.8c",  0x00000, 0x02000, 0x073c9d32 )
	ROM_LOAD_EVEN( "foodf.9d",  0x04000, 0x02000, 0xce746fa0 )
	ROM_LOAD_ODD ( "foodf.8d",  0x04000, 0x02000, 0x689c7232 )
	ROM_LOAD_EVEN( "foodf.9e",  0x08000, 0x02000, 0xbe570453 )
	ROM_LOAD_ODD ( "foodf.8e",  0x08000, 0x02000, 0x827981db )
	ROM_LOAD_EVEN( "foodf.9f",  0x0c000, 0x02000, 0x294af2da )
	ROM_LOAD_ODD ( "foodf.8f",  0x0c000, 0x02000, 0x02a55787 )

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "foodf.4d",  0x0000, 0x2000, 0x0a70cc90 )
	ROM_LOAD( "foodf.4e",  0x2000, 0x2000, 0x87997597 )
	ROM_LOAD( "foodf.6lm", 0x4000, 0x2000, 0x1ed008ec )
ROM_END


/***************************************************************************

  High score save/load

***************************************************************************/

static int hiload (void)
{
  	unsigned char *nvram = foodf_nvram_base ();
   void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
   if (f)
   {
		osd_fread (f, nvram, foodf_nvram_size);
		osd_fclose (f);
   }
   else
   {
   	static unsigned char factory_nvram[] =
   	{
   		0x10,0x00,0x10,0x00,0xf1,0x00,0xca,0xb8,0x00,0x00,0x10,0x20,0x00,0x00,0x00,0x00,
   		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xa1,0x20,0x00,0x00,0xcd,0x10,
   		0x14,0xa4,0x49,0x10,0x02,0x75,0x45,0x84,0x00,0xa4,0x24,0x25,0xe0,0x00,0xf0,0x00,
   		0x10,0x00,0xd0,0x00,0x50,0x00,0x10,0x00,0x10,0x00,0x41,0x00,0x10,0x00,0x10,0x00,
   		0x00,0x00,0x00,0x00,0x08,0x08,0x00,0x00,0x08,0x08,0x08,0x08,0x00,0x00,0x08,0x08
   	};
   	int i;

   	/* reset to factory defaults */
   	memset (nvram, 0, foodf_nvram_size);
   	for (i = 0; i < foodf_nvram_size; i += 4)
   	{
   		nvram[i+1] = factory_nvram[i/4] >> 4;
   		nvram[i+3] = factory_nvram[i/4] & 0x0f;
   	}
   }
   return 1;
}

static void hisave (void)
{
   void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
   if (f)
   {
      osd_fwrite (f, foodf_nvram_base (), foodf_nvram_size);
      osd_fclose (f);
   }
}


struct GameDriver foodf_driver =
{
	"Food Fight",
	"foodf",
	"Aaron Giles (MAME driver)\nMike Balfour (Hardware info)\nAlan J. McCormick (Sound info)",
	&machine_driver,

	foodf_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	0, foodf_ports, 0, 0, 0,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};
