/****************************************************************************

Kung Fu Master memory map (Doc by Ishmair)

ROMS mapping

KF1 ?

KF2yKF3   Adpcm  4bit samples

0000h->KF4      Z80   IM 1.   It doesn't use NMIs.
4000h->KF5      Z80


Gfx KF6,7,8->Background CharSet 1024 Chars.3 planes 8 colors.
    KF9,11,14->Sprites 1   ;3 planes
    KF10,12,13->Sprites 2  ;3 planes

    Sprites are in the same easy format that chars.

; Video RAM  (Char window)
D000h-D7FFh character number (0-255)  64x32 Size (two screens for scroll)

                              HFlip
                               |
D800h-DFFFh character ATTR   11000000
                             \/
                             banco
;                            char

;sprites
4096 char sprites (16 veces 256) de 8 colores.


SCROLL ;  Scroll Byte 1  AT (A000h) -> two's complement.
          Scroll Byte 2  AT (B000h) -> This seems to be scroll related,
                                       but i don't know how it works.

SPRITES
           00   01   02   03   04   05   06   07
           |         Y     ?   AnL  AnH  X     ?
           |              YH    |             Parte alta de XH?
           |                         |
           0F color                 03h bank
                                     |
           F0 ? 1 en silvia         40h Flip

All the X and Y are two's complements , i think.

Start at C000h
I use 256 sprites.      Maybe all my sprites handle is wrong.



Ports IN ;                    ;(0=On 1=Off)
              (00) INTERFACE 0          7 6 5 4 3 2 1 0
                                                | | | |
                                                | | | 1Up Start
                                                | | 2Up Start
                                                | Coin
                                                Coin
                                        Fire1
                                        |
              (01) INTERFACE 1          7 6 5 4 3 2 1 0
                                            |   | | | |
                                            |   | | | Right
                                            |   | | Left
                                            |   | Down
                                            |   Up
                                            Fire2

              (02) INTERFACE 2          7 6 5 4 3 2 1 0
                                        2nd player controls

              (03) DSW 1                7 6 5 4 3 2 1 0

              (04) DSW 2                7 6 5 4 3 2 1 0
                                        |
                                        Test Mode

Puertos OUT;  (00)      ;Sound related. (Look at subrutine 0DE5h)
                        ;Bit 7 at 1 , 7 low bits  .. sound number.
                        ;digital sound will be easy to implement.
              (01)       ?

****************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"
#include "sndhrdw/8910intf.h"
#include "vidhrdw/generic.h"


extern unsigned char *kungfum_scroll_low;
extern unsigned char *kungfum_scroll_high;
extern int kungfum_vh_start(void);
extern void kungfum_vh_stop(void);
extern void kungfum_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int c1942_sh_start(void);
static unsigned char interrupt_enable_1;



void kungfum_interrupt_enable_w(int offset,int data)
{
	interrupt_enable_1 = data;
}
int kungfum_interrupt(void)
{
	if (interrupt_enable_1) return Z80_IGNORE_INT;
	else return Z80_NMI_INT;
}
void kungfum_sample_w(int offset,int data)
{
        if (Machine->samples && (data >= 0x80)) {
          data &= 0x7f;
          if (data == 0)
            osd_stop_sample(7);
          else if ((data < 8) && Machine->samples->sample[data]) {
          fprintf(errorlog, "Data: %x\n", data);
                osd_play_sample(7,Machine->samples->sample[data]->data,
                        Machine->samples->sample[data]->length,
                        Machine->samples->sample[data]->smpfreq,
                        Machine->samples->sample[data]->volume,0);
          }
        }
}


static struct MemoryReadAddress readmem[] =
{
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xd000, 0xdfff, MRA_RAM },         /* Video and Color ram */
	{ 0x0000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xd000, 0xd7ff, videoram_w, &videoram },
	{ 0xd800, 0xdfff, colorram_w, &colorram },
	{ 0xc020, 0xc0df, MWA_RAM, &spriteram },
	{ 0xa000, 0xa000, MWA_RAM, &kungfum_scroll_low },
	{ 0xb000, 0xb000, MWA_RAM, &kungfum_scroll_high },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
        { 0x2ab4, 0x2ab4, MRA_RAM },
        { 0x304b, 0x304b, MRA_RAM },
        { 0x414b, 0x414b, MRA_RAM },
        { 0x514b, 0x514b, MRA_RAM },
        { 0x6a4b, 0x6a4b, MRA_RAM },
	{ 0xe000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
        { 0x2ab4, 0x2ab4, MWA_RAM },  /* incremental... TIMER? */
        { 0x304b, 0x304b, MWA_RAM },
        { 0x404b, 0x404b, MWA_RAM },  /* writes 6A */
        { 0x414b, 0x414b, MWA_RAM },  /* writes 6A */
        { 0x4802, 0x4802, MWA_RAM },  /* writes 77 */
        { 0x514b, 0x514b, MWA_RAM },
        { 0xe003, 0xe003, kungfum_interrupt_enable_w },
	{ 0xe000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },   /* coin */
	{ 0x01, 0x01, input_port_1_r },   /* player 1 control */
	{ 0x02, 0x02, input_port_2_r },   /* player 2 control */
	{ 0x03, 0x03, input_port_3_r },   /* DSW 1 */
	{ 0x04, 0x04, input_port_4_r },   /* DSW 2 */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
        { 0x00, 0x00, kungfum_sample_w },
        { 0x01, 0x01, IOWP_NOP },       /* always 0 ? */
	{ -1 }	/* end of table */
};


static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
				0, OSD_KEY_CONTROL, 0, OSD_KEY_ALT },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
				0, OSD_JOY_FIRE1, 0, OSD_JOY_FIRE2 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0xfd,
		{ 0, 0, 0, 0, 0, 0, 0, OSD_KEY_F1 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
	{ 1, 3, "JUMP" },
	{ 1, 1, "MOVE LEFT"  },
	{ 1, 0, "MOVE RIGHT" },
	{ 1, 2, "CROUCH" },
	{ 1, 5, "PUNCH" },
	{ 1, 7, "KICK" },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x0c, "LIVES", { "5", "4", "2", "3" }, 1 },
	{ 3, 0x01, "DIFFICULTY", { "HARD", "EASY" }, 1 },
	{ 3, 0x02, "TIMER", { "FAST", "SLOW" }, 1 },
	{ 4, 0x40, "DEMO MODE", { "ON", "OFF" }, 1 },
	{ 4, 0x20, "LEVEL SELECT", { "ON", "OFF" }, 1 },
	{ 4, 0x08, "SW4B", { "ON", "OFF" }, 1 },
	{ 4, 0x10, "SW5B", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 2 bits per pixel */
	{ 0x4000*8, 0x2000*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 0x8000*8, 0x4000*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,           0, 32 },
	{ 1, 0x06000, &spritelayout,      32*8, 32 },
	{ 1, 0x08000, &spritelayout,      32*8, 32 },
	{ 1, 0x12000, &spritelayout,      32*8, 32 },
	{ 1, 0x14000, &spritelayout,      32*8, 32 },
	{ -1 } /* end of array */
};




static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
  0xFF,0x00,0x00, /* RED */
	0xdb,0x92,0x49, /* BROWN */
	0xff,0xb6,0xdb, /* PINK */
	0xFF,0xFF,0xFF, /* WHITE */
	0x00,0xFF,0xFF, /* CYAN */
	0x49,0xb6,0xdb, /* DKCYAN */
  0xFF,0x60,0x00, /* DKORANGE */
	0x00,0x00,0x96, /* DKBLUE */
  0xFF,0xFF,0x00, /* YELLOW */
	0x03,0x96,0xd2, /* LTBLUE */
	0x24,0x24,0xdb, /* BLUE */
	0x00,0xdb,0x00, /* GREEN */
	0x49,0xb6,0x92, /* DKGREEN */
	0xff,0xb6,0x92, /* LTORANGE */
	0xb6,0xb6,0xb6, /* GRAY */
	0x19,0x96,0x62, /* VDKGREEN */
  0xC0,0x00,0x00, /* DKRED */

  0xFF,0x00,0xFF, /* CUSTOM1 magenta*/
  0x80,0xC0,0xFF, /* CUSTOM2 blue plane*/
  0xFF,0xE0,0x00, /* CUSTOM3 */
  0xFF,0xC0,0x40, /* CUSTOM4 */

  0xc0,0xff,0x00, /* GREEN1 */
  0x80,0xe0,0x00, /* GREEN2 */
  0x40,0xc0,0x00, /* GREEN3 */

  0x00,0x00,0x80, /* BACK1 dark blue*/
  0x00,0x00,0xC0, /* BACK2 Blue */
  0x40,0xA0,0xFF, /* BACK3 */
  0x60,0xA0,0xE0, /* BACK4 */
  0xA0,0xD0,0xF0, /* BACK5 */
  0xC0,0xE0,0xFF, /* BACK6 */
  0x00,0x60,0xC0, /* BACK7 */
  0xE0,0x80,0xE0, /* BACK8 */
  0x50,0x90,0xD0, /* BACK9 */
  0x40,0x80,0xC0, /* BACK10 */
  0x80,0xB0,0xE0, /* BACK11 */

  0x00,0x00,0xFF, /* BLUE1 */
  0x00,0x00,0xC0, /* BLUE2 */
  0x00,0x00,0x80, /* BLUE3 */

  0xFF,0xFF,0xFF, /* YELLOW1 */
  0xFF,0xFF,0x00, /* YELLOW2 */
  0xFF,0xE0,0x00, /* YELLOW3 */
  0xE0,0xC0,0x00, /* YELLOW4 */
  0xD0,0xA0,0x00, /* YELLOW5 */

  0xA0,0x00,0x00, /* DKRED2 */
  0x80,0x00,0x00, /* DKRED3 */

  0x80,0xA0,0xC0, /* GRAY1 */
  0x90,0xA0,0xC0, /* GRAY2 */
  0xC0,0xE0,0xFF, /* GRAY3 */
  0xA0,0xC0,0xE0, /* GRAY4 */
  0x80,0xA0,0xC0, /* GRAY5 */
  0x60,0x80,0xA0, /* GRAY6 */
};

enum {BLACK,RED,BROWN,PINK,WHITE,CYAN,DKCYAN,DKORANGE,
		DKBLUE,YELLOW,LTBLUE,BLUE,GREEN,DKGREEN,LTORANGE,GRAY,
		VDKGREEN,DKRED,
		CUSTOM1,CUSTOM2,CUSTOM3,CUSTOM4,
		GREEN1,GREEN2,GREEN3,
		BACK1,BACK2,BACK3,BACK4,BACK5,BACK6,BACK7,BACK8,BACK9,BACK10,BACK11,
		BLUE1,BLUE2,BLUE3,
		YELLOW1,YELLOW2,YELLOW3,YELLOW4,YELLOW5,
		DKRED2,DKRED3,GRAY1,GRAY2,GRAY3,GRAY4,GRAY5,GRAY6};

static unsigned char colortable[] =
{
	/* chars */
	0,1,2,3,4,5,6,7,
	0,2,3,4,5,6,7,1,
	0,3,4,5,6,7,1,2,
	0,4,5,6,7,1,2,3,
	0,5,6,7,1,2,3,4,
	0,6,7,1,2,3,4,5,
	0,7,1,2,3,4,5,6,
	0,1,2,3,4,5,6,7,
	0,1,2,3,4,5,6,7,
	0,2,3,4,5,6,7,1,
	0,3,4,5,6,7,1,2,
	0,4,5,6,7,1,2,3,
	0,5,6,7,1,2,3,4,
	0,6,7,1,2,3,4,5,
	0,7,1,2,3,4,5,6,
	0,1,2,3,4,5,6,7,
	0,1,2,3,4,5,6,7,
	0,2,3,4,5,6,7,1,
	0,3,4,5,6,7,1,2,
	0,4,5,6,7,1,2,3,
	0,5,6,7,1,2,3,4,
	0,6,7,1,2,3,4,5,
	0,7,1,2,3,4,5,6,
	0,1,2,3,4,5,6,7,
	0,1,2,3,4,5,6,7,
	0,2,3,4,5,6,7,1,
	0,3,4,5,6,7,1,2,
	0,4,5,6,7,1,2,3,
	0,5,6,7,1,2,3,4,
	0,6,7,1,2,3,4,5,
	0,7,1,2,3,4,5,6,
	0,1,2,3,4,5,6,7,

	/* sprites */
	0,8,9,10,11,12,13,14,
	0,9,10,11,12,13,14,8,
	0,10,11,12,13,14,8,9,
	0,11,12,13,14,8,9,10,
	0,12,13,14,8,9,10,11,
	0,13,14,8,9,10,11,12,
	0,14,8,9,10,11,12,13,
	0,8,9,10,11,12,13,14,
	0,8,9,10,11,12,13,14,
	0,9,10,11,12,13,14,8,
	0,10,11,12,13,14,8,9,
	0,11,12,13,14,8,9,10,
	0,12,13,14,8,9,10,11,
	0,13,14,8,9,10,11,12,
	0,14,8,9,10,11,12,13,
	0,8,9,10,11,12,13,14,
	0,8,9,10,11,12,13,14,
	0,9,10,11,12,13,14,8,
	0,10,11,12,13,14,8,9,
	0,11,12,13,14,8,9,10,
	0,12,13,14,8,9,10,11,
	0,13,14,8,9,10,11,12,
	0,14,8,9,10,11,12,13,
	0,8,9,10,11,12,13,14,
	0,8,9,10,11,12,13,14,
	0,9,10,11,12,13,14,8,
	0,10,11,12,13,14,8,9,
	0,11,12,13,14,8,9,10,
	0,12,13,14,8,9,10,11,
	0,13,14,8,9,10,11,12,
	0,14,8,9,10,11,12,13,
	0,8,9,10,11,12,13,14
};


static const char *kungfum_sample_names[] =
{
        "kf_fx_00.sam",
	"kf_fx_01.sam",
	"kf_fx_02.sam",
	"kf_fx_03.sam",
	"kf_fx_04.sam",
	"kf_fx_05.sam",
	"kf_fx_06.sam",
	"kf_fx_07.sam",
	0	/* end of array */
};


int bootleg_init_machine(const char *gamename)
{
	/* fix the ROMs to pass the RAM and ROM tests with the bootleg version */
	RAM[0x001e]=0x61;
	RAM[0x770a]=0x18;
	RAM[0x7716]=0x18;

	return 0;
}

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,readport, writeport,
			interrupt,1
		},
                {
                        CPU_Z80 | CPU_AUDIO_CPU,
                        4000000,
                        2,
                        sound_readmem,sound_writemem,0,0,
                        kungfum_interrupt,1
                }
	},
	60,
	bootleg_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	kungfum_vh_start,
	kungfum_vh_stop,
	kungfum_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	c1942_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kungfum_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "kf4", 0x0000, 0x4000 )
	ROM_LOAD( "kf5", 0x4000, 0x4000 )

	ROM_REGION(0x24000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "kf6",  0x00000, 0x2000 )	/* characters plane 0 */
	ROM_LOAD( "kf7",  0x02000, 0x2000 )	/* characters plane 1 */
	ROM_LOAD( "kf8",  0x04000, 0x2000 )	/* characters plane 2 */
	ROM_LOAD( "kf9",  0x06000, 0x4000 )	/* Sprites 1 plane 0 */
	ROM_LOAD( "kf11", 0x0a000, 0x4000 )	/* Sprites 1 plane 1 */
	ROM_LOAD( "kf14", 0x0e000, 0x4000 )	/* Sprites 1 plane 2 */
	ROM_LOAD( "kf10", 0x12000, 0x4000 )	/* Sprites 2 plane 0 */
	ROM_LOAD( "kf12", 0x16000, 0x4000 )	/* Sprites 2 plane 1 */
	ROM_LOAD( "kf13", 0x1a000, 0x4000 )	/* Sprites 2 plane 2 */
	ROM_LOAD( "kf3",  0x1e000, 0x2000 )	/* Voices, ADPCM 4-bit */
	ROM_LOAD( "kf2",  0x22000, 0x2000 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "kf1", 0x0000, 0x2000 )
	ROM_LOAD( "kf1", 0xe000, 0x2000 )
ROM_END



struct GameDriver kungfum_driver =
{
	"kungfum",
	&machine_driver,

	kungfum_rom,
	0, 0,
	kungfum_sample_names,

	input_ports, dsw, keys,

	0, palette, colortable,
	{ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	25, 26,
	8*13, 8*16, 22,

	0, 0
};
