/***************************************************************************

Green Beret memory map (preliminary)

0000-bfff ROM
c000-c7ff Color RAM
c800-cfff Video RAM
d000-d0c0 Sprites (bank 0)
d100-d1c0 Sprites (bank 1)
d200-dfff RAM
e000-e01f ZRAM1 line scroll registers
e020-e03f ZRAM2 bit 8 of line scroll registers

read:
f200      DSW2
          bit 0-1 lives
          bit 2   cocktail/upright cabinet (0 = upright)
          bit 3-4 bonus
          bit 5-6 difficulty
          bit 7   demo sounds
f400      DSW3
          bit 0 = screen flip
		  bit 1 = single/dual upright controls
f600      DSW1 coins per play
f601      IN1 player 2 controls
f602      IN0 player 1 controls
f603      IN2
          bit 0-1-2 coin  bit 3 1 player  start  bit 4 2 players start

write:
e040      ?
e041      ?
e042      ?
e043      bit 3 = sprite RAM bank select; other bits = ?
e044      bit 0 = nmi enable, other bits = ?
f000      ?
f200      sound
f400      sound (always used together with f200)
f600      watchdog reset (?)

interrupts:
The game uses both IRQ (mode 1) and NMI.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *gberet_interrupt_enable;
extern int gberet_interrupt(void);

extern unsigned char *gberet_scroll;
extern unsigned char *gberet_spritebank;
extern void gberet_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int gberet_vh_start(void);
extern void gberet_vh_stop(void);
extern void gberet_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void gberet_sound1_w(int offset,int data);
extern int gberet_sh_start(void);
extern void gberet_sh_stop(void);
extern void gberet_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0xc000, 0xe03f, MRA_RAM },
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xf603, 0xf603, input_port_2_r },	/* IN2 */
	{ 0xf602, 0xf602, input_port_0_r },	/* IN0 */
	{ 0xf601, 0xf601, input_port_1_r },	/* IN1 */
	{ 0xf600, 0xf600, input_port_3_r },	/* DSW1 */
	{ 0xf200, 0xf200, input_port_4_r },	/* DSW2 */
	{ 0xf400, 0xf400, input_port_5_r },	/* DSW3 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xd000, 0xd0bf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd100, 0xd1bf, MWA_RAM, &spriteram_2 },
	{ 0xd200, 0xdfff, MWA_RAM },
	{ 0xc000, 0xc7ff, colorram_w, &colorram },
	{ 0xc800, 0xcfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xe03f, MWA_RAM, &gberet_scroll },
	{ 0xe043, 0xe043, MWA_RAM, &gberet_spritebank },
	{ 0xe044, 0xe044, MWA_RAM, &gberet_interrupt_enable },
	{ 0xf200, 0xf200, gberet_sound1_w },
	{ 0xf400, 0xf400, MWA_NOP },
	{ 0xf000, 0xf000, MWA_NOP },
	{ 0xf600, 0xf600, MWA_NOP },
	{ 0x0000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, OSD_KEY_ALT, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, 0, 0 },
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x7a,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW3 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 0, "MOVE LEFT"  },
        { 0, 1, "MOVE RIGHT" },
        { 0, 3, "MOVE DOWN" },
        { 0, 4, "KNIFE" },
        { 0, 5, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 4, 0x03, "LIVES", { "7", "5", "3", "2" }, 1 },
	{ 4, 0x18, "BONUS", { "50000 200000", "50000 100000", "40000 80000", "30000 70000" }, 1 },
	{ 4, 0x60, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 4, 0x80, "DEMO SOUNDS", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
		32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		64*8+0*32, 64*8+1*32, 64*8+2*32, 64*8+3*32, 64*8+4*32, 64*8+5*32, 64*8+6*32, 64*8+7*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   16*16, 16 },
	{ 1, 0x04000, &spritelayout,     0, 16 },
	{ 1, 0x0c000, &spritelayout,     0, 16 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* 577h09 - palette */
	0x00,0x1A,0x26,0x1C,0xB6,0x74,0x0A,0x52,0xA4,0xD0,0xE8,0xAD,0x3F,0x06,0xFF,0x40,
	0x00,0x05,0x04,0x02,0x88,0x37,0xAA,0xAC,0x16,0xAE,0x24,0x10,0x52,0xA4,0xF6,0xFF,
	/* 577h10 - sprites */
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x0D,0x0C,0x0F,0x00,0x0D,0x0C,0x0F,0x00,0x0D,0x0C,0x0F,0x00,0x0D,0x0C,0x0F,
	0x00,0x01,0x03,0x03,0x03,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x02,0x0F,
	0x00,0x06,0x02,0x04,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x01,0x00,0x03,0x04,0x05,0x06,0x0E,0x00,0x09,0x0A,0x0B,0x07,0x0D,0x00,0x0F,
	0x00,0x01,0x02,0x0B,0x04,0x05,0x01,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x01,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0C,0x0D,0x0F,0x0F,
	0x00,0x01,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x00,0x01,0x01,0x01,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x04,0x0F,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0D,0x0B,0x0E,0x0F,
	0x00,0x0D,0x00,0x00,0x00,0x0D,0x0D,0x0D,0x0D,0x00,0x0D,0x0D,0x0D,0x00,0x00,0x00,
	0x00,0x05,0x00,0x00,0x00,0x05,0x05,0x05,0x05,0x00,0x05,0x05,0x05,0x00,0x00,0x00,
	0x00,0x00,0x02,0x03,0x04,0x09,0x06,0x04,0x02,0x09,0x0D,0x0E,0x0F,0x0D,0x0E,0x0F,
	0x00,0x00,0x02,0x03,0x04,0x09,0x06,0x07,0x08,0x09,0x0D,0x0E,0x0F,0x0D,0x0E,0x0F,
	0x00,0x0E,0x00,0x00,0x00,0x0E,0x0E,0x0E,0x0E,0x00,0x0E,0x0E,0x0E,0x00,0x00,0x00,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0D,0x0A,0x0E,0x0F,
	/* 577h11 - characters */
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x07,0x07,0x07,0x07,0x0E,0x0E,0x0E,0x0E,0x0D,0x0D,0x0D,0x0D,0x0C,0x0C,0x0C,0x0C,
	0x07,0x0F,0x0E,0x0D,0x07,0x0F,0x0E,0x0D,0x07,0x0F,0x0E,0x0D,0x07,0x0F,0x0E,0x0D,
	0x00,0x00,0x00,0x00,0x0E,0x0E,0x0E,0x0E,0x0D,0x0D,0x0D,0x0D,0x0C,0x0C,0x0C,0x0C,
	0x00,0x01,0x02,0x03,0x00,0x01,0x02,0x03,0x00,0x01,0x02,0x03,0x00,0x01,0x02,0x03,
	0x00,0x01,0x0B,0x08,0x00,0x01,0x0B,0x08,0x00,0x01,0x0B,0x08,0x00,0x01,0x0B,0x08,
	0x07,0x07,0x07,0x07,0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,
	0x00,0x0D,0x04,0x0C,0x00,0x0D,0x04,0x0C,0x00,0x0D,0x04,0x0C,0x00,0x0D,0x04,0x0C,
	0x00,0x0D,0x0A,0x0B,0x00,0x0D,0x0A,0x0B,0x00,0x0D,0x0A,0x0B,0x00,0x0D,0x0A,0x0B,
	0x00,0x06,0x07,0x0C,0x00,0x06,0x07,0x0C,0x00,0x06,0x07,0x0C,0x00,0x06,0x07,0x0C,
	0x00,0x0F,0x0E,0x0D,0x00,0x0F,0x0E,0x0D,0x00,0x0F,0x0E,0x0D,0x00,0x0F,0x0E,0x0D,
	0x00,0x0E,0x0D,0x0C,0x00,0x0E,0x0D,0x0C,0x00,0x0E,0x0D,0x0C,0x00,0x0E,0x0D,0x0C,
	0x00,0x00,0x00,0x00,0x0A,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x0B,0x0C,0x0C,0x0C,0x0C,
	0x0D,0x0D,0x0D,0x0D,0x0E,0x0E,0x0E,0x0E,0x0D,0x0D,0x0D,0x0D,0x0C,0x0C,0x0C,0x0C,
	0x00,0x05,0x08,0x0E,0x00,0x05,0x08,0x0E,0x00,0x05,0x08,0x0E,0x00,0x05,0x08,0x0E,
	0x00,0x08,0x03,0x09,0x00,0x08,0x03,0x09,0x00,0x08,0x03,0x09,0x00,0x08,0x03,0x09
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz?? */
			0,
			readmem,writemem,0,0,
			gberet_interrupt,16
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	32,2*16*16,
	gberet_vh_convert_color_prom,

	0,
	gberet_vh_start,
	gberet_vh_stop,
	gberet_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	gberet_sh_start,
	gberet_sh_stop,
	gberet_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gberet_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "c10_l03.bin", 0x0000, 0x4000 )
	ROM_LOAD( "c08_l02.bin", 0x4000, 0x4000 )
	ROM_LOAD( "c07_l01.bin", 0x8000, 0x4000 )

	ROM_REGION(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "f03_l07.bin", 0x00000, 0x4000 )
	ROM_LOAD( "e05_l06.bin", 0x04000, 0x4000 )
	ROM_LOAD( "e04_l05.bin", 0x08000, 0x4000 )
	ROM_LOAD( "f04_l08.bin", 0x0c000, 0x4000 )
	ROM_LOAD( "e03_l04.bin", 0x10000, 0x4000 )
ROM_END

ROM_START( rushatck_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "rush_h03.10c", 0x0000, 0x4000 )
	ROM_LOAD( "rush_h02.8c",  0x4000, 0x4000 )
	ROM_LOAD( "rush_h01.7c",  0x8000, 0x4000 )

	ROM_REGION(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "rush_h07.3f",  0x00000, 0x4000 )
	ROM_LOAD( "rush_h06.5e",  0x04000, 0x4000 )
	ROM_LOAD( "rush_h05.4e",  0x08000, 0x4000 )
	ROM_LOAD( "rush_h08.4f",  0x0c000, 0x4000 )
	ROM_LOAD( "rush_h04.3e",  0x10000, 0x4000 )
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xd900],"\x03\x30\x00",3) == 0 &&
			memcmp(&RAM[0xd91b],"\x01\x00\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0xd900],1,6*10,f);
			RAM[0xdb06] = RAM[0xd900];
			RAM[0xdb07] = RAM[0xd901];
			RAM[0xdb08] = RAM[0xd902];
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
		fwrite(&RAM[0xd900],1,6*10,f);
		fclose(f);
	}
}



struct GameDriver gberet_driver =
{
	"Green Beret",
	"gberet",
	"NICOLA SALMORIA\nCHRIS HARDY\nPAUL SWAN",
	&machine_driver,

	gberet_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	hiload, hisave
};

struct GameDriver rushatck_driver =
{
	"Rush'n Attack",
	"rushatck",
	"NICOLA SALMORIA\nCHRIS HARDY\nPAUL SWAN",
	&machine_driver,

	rushatck_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	hiload, hisave
};
