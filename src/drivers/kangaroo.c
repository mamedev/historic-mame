/* Kangaroo driver
Kangaroo (c) Atari Games / Sun Electronics Corp 1982

   Changes:
   97/06/19 - mod to ensure it's really safe to load the scores.

   97/06/17 - added the coin counter output so the error log will
              not get cluttered with so much garbage about unknown locations.
            - added music on/off to dip switch settings.
              Thanks to S. Joe Dunkle for mentioning the game should
              have music. btw. this is not really a dip switch on the PCB,
              it's a pin on the edge connector and the damn manual doesn't
              really tell what it does, so I hadn't ever tried it out...
            - added high score saving/loading. Since I try to avoid disassembling
              the code at all costs I'm not sure it's correct - seems to work tho' ;-) -V-

   97/05/07 - changed to conform the new AUDIO_CPU code
            - changed the audio command read to use the latched version.

   97/04/xx - created, based on the Arabian driver,
            The two games (arabian & kangaroo) are both by Sun Electronics
            and run on very similar hardware.
            Kangaroo PCB is constructed from two boards:
            TVG-1-CPU-B , which houses program ROMS, two Z80 CPUs,
             a GI-AY-8910, a custom microcontroller and the logic chips connecting these.
            TVG-1-VIDEO-B is obviously the video board. On this one
             you can find the graphics ROMS (tvg83 -tvg86), some logic
             chips and 4 banks of 8 4116 RAM chips.

            */
/* TODO */
/* This is still a bit messy after my cut/paste from the arabian driver */
/* will have to clean up && correct the sound problems */

/***************************************************************************

Kangaroo memory map (preliminary)
(these are for CPU#0)

0000-0fff tvg75
1000-1fff tvg76
2000-2fff tvg77
3000-3fff tvg78
4000-4fff tvg79
5000-5fff tvg80

8000-bfff VIDEO RAM

e000-e3ff RAM

e800-e80a CRT controller  ? blitter ?


memory mapped ports:
read:
ec00 - IN 0
ed00 - IN 1
ee00 - IN 2

efxx - security chip in/out
       it seems to work like a clock.

e400 - DSW 0

DSW 0
bit 7 = ?
bit 6 = ?
bit 5 = ?
bit 4 = ?
bit 3 = ?
bit 2 = 1 - test mode
bit 1 = COIN 2
bit 0 = COIN 1
---------------------------------------------------------------------------
(these are for CPU#1)
0x0000-0x0fff tvg81

0x4000-0x43ff RAM
0x6000        sound commands from CPU#0

8000 - AY-3-8910 control
7000 - AY-3-8910 write
---------------------------------------------------------------------------

interrupts:
(CPU#0) standard IM 1 interrupt mode (rst #38 every vblank)
(CPU#1) same here
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"
#include "sndhrdw/8910intf.h"
#include "sndhrdw/generic.h"



/* machine */
extern int  kangaroo_sec_chip_r(int offset);
extern void kangaroo_sec_chip_w(int offset,int val);
extern int  kangaroo_interrupt(void);

/* vidhrdw */
extern int  kangaroo_vh_start(void);
extern void kangaroo_vh_stop(void);
extern void kangaroo_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void kangaroo_spriteramw(int offset, int val);
extern void kangaroo_videoramw(int offset, int val);
extern void kangaroo_color_shadew(int offset, int val);

/*sndhrdw*/
extern int kangaroo_sh_start(void);

static struct MemoryReadAddress readmem[] =
{
        { 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_RAM },
        { 0xee00, 0xee00, input_port_3_r },
        { 0xe400, 0xe400, input_port_7_r },
        { 0xe000, 0xe3ff, MRA_RAM },
        { 0xec00, 0xec00, input_port_0_r },
        { 0xed00, 0xed00, input_port_1_r },
        { 0xef00, 0xef00, kangaroo_sec_chip_r },
        { 0xe800, 0xe80a, MRA_RAM },
	{ -1 }  /* end of table */
};
static struct MemoryReadAddress sh_readmem[] =
{
        { 0x0000, 0x0fff, MRA_ROM },
        { 0x4000, 0x43ff, MRA_RAM },
        { 0x6000, 0x6000, sound_command_latch_r },
        { -1 }
};
static struct MemoryWriteAddress writemem[] =
{
        { 0x8000, 0xbfff, kangaroo_videoramw, &videoram },
        { 0xe000, 0xe3ff, MWA_RAM },
        { 0xe800, 0xe80a, kangaroo_spriteramw, &spriteram },
        { 0xef00, 0xefff, kangaroo_sec_chip_w, &videoram },
        { 0xec00, 0xec00, sound_command_w, &videoram },
        { 0xed00, 0xed00, MWA_NOP },
        { 0x0000, 0x5fff, MWA_ROM },
        { -1 }  /* end of table */
};
static struct MemoryWriteAddress sh_writemem[] =
{
        { 0x4000, 0x43ff, MWA_RAM },
        { 0x0000, 0x0fff, MWA_ROM },
        { 0x6000, 0x6000, MWA_ROM },
        { -1 }
};


void kanga_moja(int port, int val)
{
/* does this really work, or is there a problem with the PSG code,
   it seems like one channel is completely missing from the output
*/

  Z80_Regs regs;
  Z80_GetRegs(&regs);

  if (regs.BC.D==0x8000)
    AY8910_control_port_0_w(port,val);
  else /* it must be 0x7000 ;-) -V- */
    AY8910_write_port_0_w(port,val);

}
int kanga_rdport(int port)
{
/* this is actually not used*/
        return AY8910_read_port_0_r(port);
}
static struct IOWritePort sh_writeport[] =
{
        { 0x00, 0x00, kanga_moja },
	{ -1 }	/* end of table */
};
static struct IOReadPort sh_readport[] =
{
        { 0x00, 0x00, kanga_rdport },
        { -1 }
};


static struct InputPort k_input_ports[] =
{
	{	/* IN0 */
                0x20,
                { OSD_KEY_4, OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},

	{	/* IN1 */
                0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
                                OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
                                OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
                { 0 , 0, 0, 0, 0, 0, 0, 0 },
                { 0 , 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN3 */
                0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
                                OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
                                OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN4 */
		0x00,
                { 0 , 0, 0, 0, 0, 0, 0, 0 },
                { 0 , 0, 0, 0, 0, 0, 0, 0 }
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


static struct KEYSet keys[] =
{
        { 1, 2, "PL1 MOVE UP" },
        { 1, 1, "PL1 MOVE LEFT"  },
        { 1, 0, "PL1 MOVE RIGHT" },
        { 1, 3, "PL1 MOVE DOWN" },
        { 1, 4, "PL1 FIRE" },
        { 3, 2, "PL2 MOVE UP" },
        { 3, 1, "PL2 MOVE LEFT"  },
        { 3, 0, "PL2 MOVE RIGHT" },
        { 3, 3, "PL2 MOVE DOWN" },
        { 3, 4, "PL2 FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
        { 7, 0x01, "LIVES", { "03", "05" } },
        { 7, 0x02, "DIFFICULTY", { "EASY", "HARD" } },
        { 7, 0x0c, "BONUS", { "NO BONUS", "AT 10 000", "AT 10 AND EVERY 30 000", "AT 20 AND EVERY 40 000" } },
        { 0, 0x20, "MUSIC", { "ON ", "OFF" } }, /* 970617 -V- */
        { 7, 0xf0, "COIN SELECT 1", { "1/1 1/1", "2/1 2/1", "2/1 1/3", "1/1 1/2",\
                        "1/1 1/3", "1/1 1/4", "1/1 1/5", "1/1 1/6",\
                        "1/2 1/2", "1/2 1/4", "1/2 1/5", "1/2 1/10",\
                        "1/2 1/11", "1/2 1/12", "1/2 1/6", "FREE FREE" } },
       /* { 7, 0xc0, "COIN SELECT 2", { "SET A", "SET B", "SET C", "SET D" } },*/
	{ -1 }
};

static struct GfxLayout charlayout =
{
        5,8,    /* 7*8 characters */
	48,	/* 40 characters */
	1,	/* 1 bit per pixel */
	{ 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8 },
	{ 0, 1, 2, 3, 4, 5, 6 ,7 },	/* pretty straightforward layout */
        5*8     /* every char takes 8 consecutive bytes */
};
static struct GfxLayout sprlayout =
{
        1,4,    /* 4 * 1 lines */
        8192,   /* 8192 "characters" */
        4,      /* 4 bpp        */
        { 0x2000*8, 0x2000*8+4, 0, 4  },       /* 4 and 8192 bytes between planes */
        { 0,1,2,3 },
        { 0,1,2,3 },
        1*8
};
static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 0, 3670, &charlayout,   0, 16 },   /*fonts*/
        { 2, 0 , &sprlayout, 32, 1 },
        { -1 } /* end of array */
};


static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
        0x00,0x00,42*4, /* DKRED1 */
        0x00,42*4,0x00, /* DKRED2 */
        0x00,42*4,42*4, /* RED */
        42*4,00,00, /* DKGRN1 */
        42*4,00,42*4, /* DKBRN1 */
        42*4,21*4,00, /* DKBRN2 */
        42*4,42*4,42*4, /* LTRED1 */
        21*4,21*4,21*4, /* BROWN */
        21*4,21*4,63*4, /* DKGRN2 */
        21*4,63*4,21*4, /* LTORG1 */
        21*4,63*4,63*4, /* DKGRN3 */
        63*4,21*4,21*4, /* DKYEL */
        63*4,21*4,63*4, /* DKORG */
        63*4,63*4,21*4, /* ORANGE */
        63*4,63*4,63*4, /* GREEN1 */
	0x00,0x00,0x00,	/* BLACK */
        0x00,0x00,42*4, /* DKRED1 */
        0x00,42*4,0x00, /* DKRED2 */
        0x00,42*4,42*4, /* RED */
        42*4,00,00, /* DKGRN1 */
        42*4,00,42*4, /* DKBRN1 */
        42*4,21*4,00, /* DKBRN2 */
        42*4,42*4,42*4, /* LTRED1 */
        21*4,21*4,21*4, /* BROWN */
        21*4,21*4,63*4, /* DKGRN2 */
        21*4,63*4,21*4, /* LTORG1 */
        21*4,63*4,63*4, /* DKGRN3 */
        63*4,21*4,21*4, /* DKYEL */
        63*4,21*4,63*4, /* DKORG */
        63*4,63*4,21*4, /* ORANGE */
        63*4,63*4,63*4, /* GREEN1 */

};

enum {BLACK,DKRED1,DKRED2,RED,DKGRN1,DKBRN1,DKBRN2,LTRED1,BROWN,DKGRN2,
	LTORG1,DKGRN3,DKYEL,DKORG,ORANGE,GREEN1,LTGRN1,GREEN2,LTGRN2,YELLOW,
	DKBLU1,DKPNK1,DKPNK2,LTRED2,LTBRN,LTORG2,LTGRN3,LTGRN4,LTYEL,DKBLU2,
	PINK1,DKBLU3,PINK2,CREAM,LTORG3,BLUE,PURPLE,LTBLU1,LTBLU2,WHITE1,
	WHITE2};

static unsigned char colortable[] =
{
	/* characters and sprites */
        BLACK,0,0,1,               /* 1st level */
        0,2,0,3,             /* pauline with kong */
        0,4,0,5,           /* Mario */
        0,6,0,7,                  /* 3rd level */
        BLACK,8,0,9,                /* 4th lvl */
        BLACK,10,0,11,               /* 2nd level */
        BLACK,12,0,13,                  /* blue text */
        BLACK,14,0,15,       /* hammers */
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
};

/* 971706 -V- */
static int kangaroo_hiload(const char *name)
{
        /* Ok, we need to explicitly tell what RAM to read...*/
        /* realizing this was necessary took me quite a long time :( -V- */

        unsigned char *RAM = Machine->memory_region[0];

        /* just a guess really... */
        if ( RAM[0xe1da] == 0x50 )
        {
                FILE *f;
                if (( f = fopen(name, "rb")) != 0)
                {
                        fread(&RAM[0xe1a0], 1, 0x40, f);
                        /* is this enough ??? */
                        fclose(f);
                }
                return 1;
         }
         else return 0; /* didn't load them yet, do stop by later ;-) -V- */
}
/* 970617 -V- */
static void kangaroo_hisave(const char *name)
{
        FILE *f;

        unsigned char *RAM = Machine->memory_region[0];

        if ((f = fopen(name , "wb")) != 0)
        {
                fwrite(&RAM[0xe1a0], 1, 0x40, f);
                fclose(f);
        }
}



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
                        3000000,        /* 3 Mhz */
			0,
                        readmem,writemem,0,0,
                        interrupt,1
                },
                {
                        CPU_Z80 | CPU_AUDIO_CPU,
                        2500000,
                        3,
                        sh_readmem,sh_writemem,sh_readport,sh_writeport,
                        kangaroo_interrupt,1
                }
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0x0b, 0xfa, 0, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
        kangaroo_vh_start,
        kangaroo_vh_stop,
        kangaroo_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
        kangaroo_sh_start,
	AY8910_sh_stop,
        AY8910_sh_update
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kangaroo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
        ROM_LOAD( "tvg75.bin",  0x0000, 0x1000 )  /* fonts are messed up with */
        ROM_LOAD( "tvg76.bin",  0x1000, 0x1000 )  /* program code */
        ROM_LOAD( "tvg77.bin",  0x2000, 0x1000 )
        ROM_LOAD( "tvg78.bin",  0x3000, 0x1000 )
        ROM_LOAD( "tvg79.bin",  0x4000, 0x1000 )
        ROM_LOAD( "tvg80.bin",  0x5000, 0x1000 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "tvg83.bin",  0x0000, 0x1000 )  /*this is not used at all*/
                                                /* might be removed */
	ROM_REGION(0x10000) /* space for graphics roms */
        ROM_LOAD( "tvg84.bin",  0x0000, 0x1000 )  /* because of very rare way */
        ROM_LOAD( "tvg86.bin",  0x1000, 0x1000 )  /* CRT controller uses these roms */
        ROM_LOAD( "tvg83.bin",  0x2000, 0x1000 )  /* there's no way, but to decode */
        ROM_LOAD( "tvg85.bin",  0x3000, 0x1000 )  /* it at runtime - which is SLOW */

        ROM_REGION(0x10000) /*sound*/
        ROM_LOAD( "tvg81.bin", 0x0000, 0x1000 )
ROM_END


struct GameDriver kangaroo_driver =
{
	"Kangaroo",
	"kangaroo",
	"VILLE LAITINEN",
	&machine_driver,

        kangaroo_rom,
	0, 0,
	0,

        k_input_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

        kangaroo_hiload, kangaroo_hisave
};
