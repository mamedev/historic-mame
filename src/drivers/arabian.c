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
bit 6 - - coins per credit
bit 5 - -
bit 4 - /
bit 3 - 
bit 2 - 
bit 1 - 
bit 0 - number of lives 0=2 lives; 1=4 lives

write:
c800 - AY-3-8910 control
ca00 - AY-3-8910 write

ON AY PORTS there are two things :

port 0x0e (write only) to do ...
port 0x0f (write only) to do ... switch off the switches ???

interrupts:
standart IM 1 interrupt mode (rst #38 every vblank)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"
#include "sndhrdw/8910intf.h"

extern int arabian_vh_start(void);
extern void arabian_vh_stop(void);
extern void arabian_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int arabian_d7f6(int offset);
extern int arabian_d7f8(int offset);
extern int arabian_interrupt(void);

extern void arabian_spriteramw(int offset, int val);
extern void arabian_videoramw(int offset, int val);

extern int arabian_sh_start(void);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_RAM },	
        { 0xc000, 0xc000, input_port_6_r },
        { 0xc200, 0xc200, input_port_7_r },
        { 0xd000, 0xd7ef, MRA_RAM },
        { 0xd7f0, 0xd7f0, input_port_0_r },
        { 0xd7f1, 0xd7f1, input_port_1_r },
        { 0xd7f2, 0xd7f2, input_port_2_r },
        { 0xd7f3, 0xd7f3, input_port_3_r },
        { 0xd7f4, 0xd7f4, input_port_4_r },
        { 0xd7f5, 0xd7f5, input_port_5_r },
        { 0xd7f6, 0xd7f6, arabian_d7f6 },
        { 0xd7f7, 0xd7f7, MRA_RAM },
        { 0xd7f8, 0xd7f8, arabian_d7f8 },
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


void moja(int port, int val)
{

  Z80_Regs regs;
  static int lastr;
  Z80_GetRegs(&regs);

  if (regs.BC.D==0xc800)
  {
    AY8910_control_port_0_w(port,val);
    lastr=val;
  }
  else
  {
    if ( (lastr==0x0e) || (lastr==0x0f) )
    {
/*      if (errorlog) fprintf(errorlog,"AY reg:%02x, val: %02x\n", lastr, val );*/
    }
    else
      AY8910_write_port_0_w(port,val);
  }

}

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, moja },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
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
		{ OSD_KEY_CONTROL, 0, 0, 0, 0, 0, 0, 0 },
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
		{ OSD_KEY_CONTROL, 0, 0, 0, 0, 0, 0, 0 },
		{ OSD_JOY_FIRE, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN5 */
		0x0f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
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
	{ -1 }  /* end of table */
};


static struct DSW dsw[] =
{
	{ 7, 0x01, "LIVES", { "02", "04" } },
	{ 7, 0x02, "THING 0", { "SET A", "SET B" } },
	{ 7, 0x04, "THING 1", { "SET A", "SET B" } },
	{ 7, 0x08, "THING 2", { "SET A", "SET B" } },
	{ 7, 0x30, "COIN SELECT 1", { "SET A", "SET B", "SET C", "SET D" } },
	{ 7, 0xc0, "COIN SELECT 2", { "SET A", "SET B", "SET C", "SET D" } },
	{ -1 }
};

static struct GfxLayout charlayout =
{
	7,8,	/* 7*8 characters */
	48,	/* 40 characters */
	1,	/* 1 bit per pixel */
	{ 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8 },
	{ 0, 1, 2, 3, 4, 5, 6 ,7 },	/* pretty straightforward layout */
	7*8	/* every char takes 8 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 0, 0x1201, &charlayout,   0, 16 },   /*fonts*/
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0x49,0x00,0x00,	/* DKRED1 */
	0x92,0x00,0x00,	/* DKRED2 */
	0xff,0x00,0x00,	/* RED */
	0x00,0x24,0x00,	/* DKGRN1 */
	0x92,0x24,0x00,	/* DKBRN1 */
	0xb6,0x24,0x00,	/* DKBRN2 */
	0xff,0x24,0x00,	/* LTRED1 */
	0xdb,0x49,0x00,	/* BROWN */
	0x00,0x6c,0x00,	/* DKGRN2 */
	0xff,0x6c,0x00,	/* LTORG1 */
	0x00,0x92,0x00,	/* DKGRN3 */
	0x92,0x92,0x00,	/* DKYEL */
	0xdb,0x92,0x00,	/* DKORG */
	0xff,0x92,0x00,	/* ORANGE */
	0x00,0xdb,0x00,	/* GREEN1 */
	0x6d,0xdb,0x00,	/* LTGRN1 */
	0x00,0xff,0x00,	/* GREEN2 */
	0x49,0xff,0x00,	/* LTGRN2 */
	0xff,0xff,0x00,	/* YELLOW */
	0x00,0x00,0x55,	/* DKBLU1 */
	0xff,0x00,0x55,	/* DKPNK1 */
	0xff,0x24,0x55,	/* DKPNK2 */
	0xff,0x6d,0x55,	/* LTRED2 */
	0xdb,0x92,0x55,	/* LTBRN */
	0xff,0x92,0x55,	/* LTORG2 */
	0x24,0xff,0x55,	/* LTGRN3 */
	0x49,0xff,0x55,	/* LTGRN4 */
	0xff,0xff,0x55,	/* LTYEL */
	0x00,0x00,0xaa,	/* DKBLU2 */
	0xff,0x00,0xaa,	/* PINK1 */
	0x00,0x24,0xaa,	/* DKBLU3 */
	0xff,0x24,0xaa,	/* PINK2 */
	0xdb,0xdb,0xaa,	/* CREAM */
	0xff,0xdb,0xaa,	/* LTORG3 */
	0x00,0x00,0xff,	/* BLUE */
	0xdb,0x00,0xff,	/* PURPLE */
	0x00,0xb6,0xff,	/* LTBLU1 */
	0x92,0xdb,0xff,	/* LTBLU2 */
	0xdb,0xdb,0xff,	/* WHITE1 */
	0xff,0xff,0xff	/* WHITE2 */
};

enum {BLACK,DKRED1,DKRED2,RED,DKGRN1,DKBRN1,DKBRN2,LTRED1,BROWN,DKGRN2,
	LTORG1,DKGRN3,DKYEL,DKORG,ORANGE,GREEN1,LTGRN1,GREEN2,LTGRN2,YELLOW,
	DKBLU1,DKPNK1,DKPNK2,LTRED2,LTBRN,LTORG2,LTGRN3,LTGRN4,LTYEL,DKBLU2,
	PINK1,DKBLU3,PINK2,CREAM,LTORG3,BLUE,PURPLE,LTBLU1,LTBLU2,WHITE1,
	WHITE2};

static unsigned char colortable[] =
{
	/* characters and sprites */
	BLACK,PINK1,RED,DKGRN3,               /* 1st level */
	BLACK,LTORG1,WHITE1,LTRED1,             /* pauline with kong */
	BLACK,RED,CREAM,BLUE,		/* Mario */
	BLACK,PINK1,RED,DKGRN3,                  /* 3rd level */
	BLACK,BLUE,LTBLU1,LTYEL,                /* 4th lvl */
	BLACK,BLUE,LTYEL,LTBLU1,               /* 2nd level */
	BLACK,RED,CREAM,BLUE,                  /* blue text */
	BLACK,LTYEL,BROWN,WHITE1,	/* hammers */
	BLACK,LTBRN,BROWN,CREAM,               /* kong */
	BLACK,RED,LTRED1,YELLOW,             /* oil flame */
	BLACK,LTBRN,CREAM,LTRED1,              /* pauline */
	BLACK,LTYEL,BLUE,BROWN,		/* barrels */
	BLACK,CREAM,LTBLU2,BLUE,	/* "oil" barrel */
	BLACK,YELLOW,BLUE,RED,               /* small mario, spring */
	BLACK,DKGRN3,LTBLU1,BROWN,            /* scared flame */
	BLACK,LTRED1,YELLOW,BLUE,            /* flame */
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
	50,
	0,

	/* video hardware */
	32*8, 32*8, { 0x0b, 0xfa, 0, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

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
	ROM_LOAD( "ic1.87",  0x0000, 0x2000 )  /* fonts are messed up with */
	ROM_LOAD( "ic2.88",  0x2000, 0x2000 )  /* program code */
	ROM_LOAD( "ic3.89",  0x4000, 0x2000 )
	ROM_LOAD( "ic4.90",  0x6000, 0x2000 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic84.91",  0x0000, 0x2000 )	/*this is not used at all*/
                                                /* might be removed */
	ROM_REGION(0x10000) /* space for graphics roms */
	ROM_LOAD( "ic84.91",  0x0000, 0x2000 )	/* because of very rare way */
	ROM_LOAD( "ic85.92",  0x2000, 0x2000 )  /* CRT controller uses these roms */
	ROM_LOAD( "ic86.93",  0x4000, 0x2000 )  /* there's no way, but to decode */
	ROM_LOAD( "ic87.94",  0x6000, 0x2000 )	/* it at runtime - which is SLOW */
                                                
ROM_END


struct GameDriver arabian_driver =
{
	"arabian",
	&machine_driver,

	arabian_rom,
	0, 0,
	0,

	input_ports, dsw,

	0, palette, colortable,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	0, 14,
	8*13, 8*16,14,

	0, 0
};
