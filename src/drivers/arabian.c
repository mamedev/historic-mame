/***************************************************************************

Arabian memory map (preliminary)

0000-1fff ROM 0
2000-3fff ROM 1
4000-5fff ROM 2
6000-7fff ROM 3
8000-bfff VIDEO RAM

d000-dfff RAM
e000-e100 CRT controller  ? blitter ?

memory mapped ports:
read:
d7f0 - IN 0
d7f1 - IN 1
d7f2 - IN 2
d7f3 - IN 3
d7f4 - IN 4
d7f5 - IN 5
d7f6 - clock HI  ?
d7f7 - clock set ?
d7f8 - clock LO  ?

c000 - DSW 0
c200 - DSW 1

DSW 0
bit 7 = ?
bit 6 = ?
bit 5 = ?
bit 4 = ?
bit 3 = ?
bit 2 = 1 - test mode
bit 1 = COIN 2
bit 0 = COIN 1

DSW 1
bit 7 - \
bit 6 - - coins per credit    (ALL=1 free play)
bit 5 - -
bit 4 - /
bit 3 - carry bowls / don't carry bowls to next level (0 DON'T CARRY)
bit 2 - PIC NORMAL or FLIPPED (0 NORMAL)
bit 1 - COCKTAIL or UPRIGHT   (0 COCKTAIL)
bit 0 - LIVES 0=3 lives 1=5 lives

write:
c800 - AY-3-8910 control
ca00 - AY-3-8910 write

ON AY PORTS there are two things :

port 0x0e (write only) to do ...

port 0x0f (write only) controls:
BIT 0 -
BIT 1 -
BIT 2 -
BIT 3 -
BIT 4 - 0-read RAM  1-read switches(ports)
BIT 5 -
BIT 6 -
BIT 7 -

interrupts:
standart IM 1 interrupt mode (rst #38 every vblank)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"
#include "sndhrdw/8910intf.h"

int arabian_vh_start(void);
void arabian_vh_stop(void);
void arabian_vh_screenrefresh(struct osd_bitmap *bitmap);
void arabian_spriteramw(int offset, int val);
void arabian_videoramw(int offset, int val);

void moja(int port, int val);
int arabian_interrupt(void);
int arabian_input_port(int offset);

int arabian_sh_start(void);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_RAM },
        { 0xc000, 0xc000, input_port_0_r },
        { 0xc200, 0xc200, input_port_1_r },
        { 0xd000, 0xd7ef, MRA_RAM },
        { 0xd7f0, 0xd7f8, arabian_input_port },
        { 0xd7f9, 0xdfff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0xbfff, arabian_videoramw, &videoram },
        { 0xd000, 0xd7ff, MWA_RAM },
        { 0xe000, 0xe07f, arabian_spriteramw, &spriteram },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, moja },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN6 */
		0x00,
		{ OSD_KEY_3, OSD_KEY_4, OSD_KEY_F1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN0 */
		0x00,
		{ 0, OSD_KEY_1, OSD_KEY_2, OSD_KEY_F2, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},

	{	/* IN1 */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				0, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ OSD_KEY_LCONTROL, 0, 0, 0, 0, 0, 0, 0 },
		{ OSD_JOY_FIRE, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN3 */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				0, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				0, 0, 0, 0 }
	},
	{	/* IN4 */
		0x00,
		{ OSD_KEY_LCONTROL, 0, 0, 0, 0, 0, 0, 0 },
		{ OSD_JOY_FIRE, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN5 */
		0x04,
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
        { 3, 2, "PL1 MOVE UP" },
        { 3, 1, "PL1 MOVE LEFT"  },
        { 3, 0, "PL1 MOVE RIGHT" },
        { 3, 3, "PL1 MOVE DOWN" },
        { 4, 0, "PL1 KICK" },
        { 5, 2, "PL2 MOVE UP" },
        { 5, 1, "PL2 MOVE LEFT"  },
        { 5, 0, "PL2 MOVE RIGHT" },
        { 5, 3, "PL2 MOVE DOWN" },
        { 6, 0, "PL2 KICK" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 1, 0x01, "LIVES       ", { "  3", "  5" } },
	{ 1, 0x08, "BOWLS       ", { "CARRY TO NEXT LEVEL",
				     "       DO NOT CARRY" } },
	{ 1, 0xf0, "COIN SELECT ", { "1 COIN 1 CREDIT", "SET 2", "SET 3", "SET 4",
				    "SET 5", "SET 6", "SET 7", "FREE PLAY"} },
	{ 7, 0x02, "ATTRACT SOUND", { " ON", "OFF" } },
	{ 7, 0x0c, "BONUS        ", { "   NO BONUS", "20000 1 MEN", "30000 1 MEN", "FORGET BONUS"} },
	{ -1 }
};




static unsigned char palette[] =
{

/*colors for plane 1*/
	0   , 0   , 0,
	0   , 37*4, 53*4,
	0   , 40*4, 63*4,
	0   , 44*4, 63*4,
	48*4, 22*4, 0,
	63*4, 39*4, 51*4,
	63*4, 56*4, 0,
	60*4, 60*4, 60*4,
	0   , 0   , 0,
	32*4, 12*4, 0,
	39*4, 18*4, 0,
	0   , 24*4, 51*4,
	45*4, 20*4, 1*4,
	63*4, 36*4, 51*4,
	57*4, 41*4, 10*4,
	63*4, 39*4, 51*4,
/*colors for plane 2*/
	0   , 0   , 0,
	0   , 28*4, 0,
	0   , 11*4, 0,
	0   , 40*4, 0,
	48*4, 22*4, 0,
	58*4, 48*4, 0,
	44*4, 18*4, 0,
	60*4, 60*4, 60*4,
	25*4, 6*4 , 0,
	28*4, 21*4, 0,
	26*4, 18*4, 0,
	0   , 24*4, 0,
	45*4, 20*4, 1*4,
	51*4, 30*4, 5*4,
	57*4, 41*4, 10*4,
	63*4, 53*4, 16*4,
};


enum {BLACK,BLUE1,BLUE2,BLUE3,YELLOW };

static unsigned char colortable[] =
{
	/* characters and sprites */
	BLACK,BLUE1,BLACK,YELLOW,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,writeport,
			arabian_interrupt,1
		}
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0x0b, 0xf2, 0, 32*8-1 },
        0,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER,
	0,
	arabian_vh_start,
	arabian_vh_stop,
	arabian_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	arabian_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( arabian_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic1.87", 0x0000, 0x2000, 0x71cadc52 )
	ROM_LOAD( "ic2.88", 0x2000, 0x2000, 0xc24b12f9 )
	ROM_LOAD( "ic3.89", 0x4000, 0x2000, 0x7aac9abe )
	ROM_LOAD( "ic4.90", 0x6000, 0x2000, 0xcd8565b1 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_OBSOLETELOAD( "ic84.91", 0x0000, 0x2000 )	/*this is not used at all*/
                                                /* might be removed */
	ROM_REGION(0x10000) /* space for graphics roms */
	ROM_LOAD( "ic84.91", 0x0000, 0x2000, 0xb8edb68d )	/* because of very rare way */
	ROM_LOAD( "ic85.92", 0x2000, 0x2000, 0x3e0048d8 )  /* CRT controller uses these roms */
	ROM_LOAD( "ic86.93", 0x4000, 0x2000, 0xc871f425 )  /* there's no way, but to decode */
	ROM_LOAD( "ic87.94", 0x6000, 0x2000, 0x46faa604 )	/* it at runtime - which is SLOW */

ROM_END



static int arabian_hiload(void)
{
  unsigned char *RAM = Machine->memory_region[0];
  void *f;

  /* Wait for hiscore table initialization to be done. */
  if (memcmp(&RAM[0xd384], "\x00\x00\x00\x01\x00\x00", 6) != 0)
    return 0;

  if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
    {
      /* Load and set hiscore table. */
      osd_fread(f,&RAM[0xd384],6*10);
      osd_fclose(f);
    }

  return 1;
}



static void arabian_hisave(void)
{
  unsigned char *RAM = Machine->memory_region[0];
  void *f;

  if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
      /* Write hiscore table. */
      osd_fwrite(f,&RAM[0xd384],6*10);
      osd_fclose(f);
    }
}



struct GameDriver arabian_driver =
{
	"Arabian",
	"arabian",
	"JAREK BURCZYNSKI",
	&machine_driver,

	arabian_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	arabian_hiload, arabian_hisave
};

