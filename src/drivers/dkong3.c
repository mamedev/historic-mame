/***************************************************************************

Donkey Kong 3 memory map (preliminary):

0000-5fff ROM
6000-6fff RAM
7000-73ff ?
7400-77ff Video RAM
8000-9fff ROM

read:
7c00      IN0
7c80      IN1
7d00      DSW2
7d80      DSW1

*
 * IN0 (bits NOT inverted)
 * bit 7 : TEST
 * bit 6 : START 2
 * bit 5 : START 1
 * bit 4 : JUMP player 1
 * bit 3 : ? DOWN player 1 ?
 * bit 2 : ? UP player 1 ?
 * bit 1 : LEFT player 1
 * bit 0 : RIGHT player 1
 *
*
 * IN1 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : COIN 2
 * bit 5 : COIN 1
 * bit 4 : JUMP player 2
 * bit 3 : ? DOWN player 2 ?
 * bit 2 : ? UP player 2 ?
 * bit 1 : LEFT player 2
 * bit 0 : RIGHT player 2
 *
*
 * DSW1 (bits NOT inverted)
 * bit 7 : \ difficulty
 * bit 6 : / 00 = easy  01 = medium  10 = hard  11 = hardest
 * bit 5 : \ bonus
 * bit 4 : / 00 = 20000  01 = 30000  10 = 40000  11 = none
 * bit 3 : \ coins per play
 * bit 2 : /
 * bit 1 : \ 00 = 3 lives  01 = 4 lives
 * bit 0 : / 10 = 5 lives  11 = 6 lives
 *

write:
7d00      ?
7d80      ?
7e00      ?
7e80
7e81      char bank selector
7e82-7e83 ?
7e84      interrupt enable
7e85      ?
7e86      \  bit 1  Seleziona probabilmente la musica per lo schema
7e87      /  bit 0


I/O ports

write:
00        ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern void dkong3_gfxbank_w(int offset,int data);
extern int  dkong3_vh_start(void);
extern void dkong3_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x7400, 0x77ff, MRA_RAM },	/* video RAM */
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_ROM },
	{ 0x7c00, 0x7c00, input_port_0_r },	/* IN0 */
	{ 0x7c80, 0x7c80, input_port_1_r },	/* IN1 */
	{ 0x7d00, 0x7d00, input_port_2_r },	/* DSW2 */
	{ 0x7d80, 0x7d80, input_port_3_r },	/* DSW1 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x6000, 0x68ff, MWA_RAM },
	{ 0x6a80, 0x6fff, MWA_RAM },
	{ 0x6900, 0x6a7f, MWA_RAM, &spriteram },
	{ 0x7400, 0x77ff, videoram_w, &videoram },
	{ 0x7e81, 0x7e81, dkong3_gfxbank_w },
	{ 0x7e84, 0x7e84, interrupt_enable_w },
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0x9fff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, OSD_KEY_1, OSD_KEY_2, OSD_KEY_F1 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE, 0, 0, 0 },
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, OSD_KEY_3, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 1, "MOVE LEFT"  },
        { 0, 0, "MOVE RIGHT" },
        { 0, 3, "MOVE DOWN" },
        { 0, 4, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x03, "LIVES", { "3", "4", "5", "6" } },
	{ 3, 0x0c, "BONUS", { "30000", "40000", "50000", "NONE" } },
	{ 3, 0x30, "ADDITIONAL BONUS", { "30000", "40000", "50000", "NONE" } },
	{ 3, 0xc0, "DIFFICULTY", { "EASY", "MEDIUM", "HARD", "HARDEST" } },
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

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 256*16*16 },	/* the two bitplanes are separated */
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* the two halves of the sprite are separated */
			128*16*16+0, 128*16*16+1, 128*16*16+2, 128*16*16+3, 128*16*16+4, 128*16*16+5, 128*16*16+6, 128*16*16+7 },
	16*8	/* every sprite takes 16 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,      0, 64 },
	{ 1, 0x2000, &spritelayout, 64*4, 16 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xdb,0x00,0x00,	/* RED */
	0xdb,0x92,0x49,	/* BROWN */
	0xff,0xb6,0xdb,	/* PINK */
	0x00,0xdb,0x00,	/* UNUSED */
	0x00,0xdb,0xdb,	/* CYAN */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,0xb6,0x49,	/* DKORANGE */
	0x88,0x88,0x88,	/* UNUSED */
	0xdb,0xdb,0x00,	/* YELLOW */
	0xff,0x00,0xdb,	/* UNUSED */
	0x24,0x24,0xdb,	/* BLUE */
	0x00,0xdb,0x00,	/* GREEN */
	0x49,0xb6,0x92,	/* DKGREEN */
	0xff,0xb6,0x92,	/* LTORANGE */
	0xdb,0xdb,0xdb	/* GREY */
};

enum { BLACK,RED,BROWN,PINK,UNUSED1,CYAN,DKCYAN,DKORANGE,UNUSED2,YELLOW,
        UNUSED3,BLUE,GREEN,DKGREEN,LTORANGE,GREY };

/* Used for common colors (much easier to change them here!) */
#define DK_LEAVE_MIDDLE     11
#define DK_VINES_EDGE       13
#define LEAVE_EDGE          12
#define LEAVE_MIDDLE_LEV2   6
#define LEAVE_EDGE_LEV2     11
#define TRUNK_LEV2          13

static unsigned char colortable[] =
{
    /* chars */
    /* (#0-3) NUMBERS, 1UP/2UP, TREE TRUNK on Level 2. */
    BLACK,UNUSED1,UNUSED2,GREY, /* #0. ?,?,0123 */
    BLACK,UNUSED1,UNUSED2,GREY, /* #1. ?,?,4567 */
    BLACK,UNUSED1,RED,GREY,     /* #2. ?,color of part of "1UP",89 */

    /* 2=color of tree trunk on level 2, 3=color of "UP 2UP",
       4=tree trunk on level 2  */
    BLACK,LEAVE_EDGE_LEV2,RED,TRUNK_LEV2,

    /* (#4-12) A-Z, "TOP", & top vines on Level 1. */
    BLACK,RED,RED,RED,           /* #4. ABC  */
    BLACK,RED,RED,RED,           /* #5. DEFG */
    BLACK,RED,RED,RED,           /* #6. HIJK */
    BLACK,RED,RED,RED,           /* #7. LMNO */
    BLACK,RED,RED,RED,           /* #8. PQRS */
    BLACK,RED,RED,RED,           /* #9. TUVW */
    BLACK,DK_VINES_EDGE,RED,RED, /* #10. "T" in TOP, XYZ */

    /* 2="OP" & top vines in level 1, 3="OP" & top vines in level 1
       4=top vines in level 1.  */
    BLACK,DK_VINES_EDGE,DK_VINES_EDGE,DK_VINES_EDGE,

    /* 2=top-vines, 3=???, 4=top-vines */
    BLACK,DK_VINES_EDGE,UNUSED3,DK_VINES_EDGE,

    /* (#13-15) TIME display and the 2-color box around it. */
    BLACK,RED,BLUE,GREY,        /* #13. outerborder, innerborder, ??? */
    BLACK,BLUE,BLUE,GREY,       /* #14. "TIME" display, line under time, ??? */
    BLACK,RED,BLUE,GREY,        /* #15. outerborder, innerborder, ??? */

    /* (#16-22) 2=shadow of "3", 3=outline of "3" & vines, 4=middle of "3" */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,  /* 3=vertical vines  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,  /* 3=horizontal vines */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,  /* 2=box on bottom, 3=vines in box. */

    /* 2=shadow of "3" & middle of DK-logo & middle of box on bottom
       3=outline of "3" & outline of DK-logo & vines in box on bottom
       4=middle of "3"  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#23-47) DK-LOGO
       2=middle of DK-logo & line on top of boxes on side of levels,
       3=boxes on side of level & outline of DK-logo, 4=boxes on side of levels  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#28) 2=middle of DK-logo & leaves, 3=outline of DK-logo, 4=edge of leaves */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#32) 2=middle of leaves & shadow of "3", 3=outline of "3",
       4=edge of leaves & middle of "3"  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#33) 2=middle of leaves & shadow of "3", 4=edge of leaves */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#34-36) 2=middle of DK-logo & middle of leaves, 3=outline of DK-logo,
       4=edge of leaves */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#37-38) 2=middle of DK-logo, 3=outline of DK-logo */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#39) 2=middle of DK-logo & color of DK's hanging vines, 3=outline of
       DK-logo, 4=color of DK's hanging vines (mixed w/color 2).  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#40-41) 2=middle of DK-logo & boxes near bottom, 3=outline of DK-logo &
       lines in boxes near bottom. */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#42) 2=middle of DK-logo & boxes near bottom, 3=outline of DK-logo &
       lines in boxes near bottom, 4=boxes on side of levels  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#43-46) DK-Logo, ANGLED pieces on side of levels.
       2=middle of DK-logo, 3=outline of DK-logo, 4=boxes on side of levels */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#47) 2=leave middle, 4=leave edge & boxes on side of levels */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#48-53) LEAVES on Level 1, LETTERS on title screen.
       2=leave middle, 4=leave edges & some letters  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#54-57) LEAVES on Level 1, (56-57) LETTERS on title screen
       2=leave middle, 4=leave edges  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    /* 2=some letters on title screen  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#58) 2=tree trunk & some letters on title screen & angled pieces
       on Level 2, 4=tree trunk & tops of angulars on Level 2.  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#59) 2=some letters on title screen */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#60) 2=some letters on title screen & vines on level 2, 3=side bar,
       4=vines on level 2.  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#61) 2=some letters on title screen & angulars on level 2,
     4=top of angled pieces on level 2.  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    /* (#62-63) LEAVES ON LEVEL 2.
       2=middle of leaves on level 2, 4=edges of leaves on level 2.  */
    BLACK,LEAVE_MIDDLE_LEV2,UNUSED3,LEAVE_EDGE_LEV2,
    BLACK,LEAVE_MIDDLE_LEV2,UNUSED3,LEAVE_EDGE_LEV2,

    /* sprites */
    /* #0. Donkey Kong's head. */
    BLACK,RED,BROWN,GREY,

    BLACK,UNUSED1,UNUSED2,UNUSED3,       /* ???? */
    BLACK,UNUSED2,UNUSED1,UNUSED3,       /* ???? */

    /* #3. Middle-vertical-vines on level 2. */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,LEAVE_EDGE,

    BLACK,UNUSED3,UNUSED2,UNUSED1,       /* ???? */

    /* #5. Box on the bottom-middle of level #1, 2=box color, 3=lines in box, 4=???  */
    BLACK,DK_LEAVE_MIDDLE,DK_VINES_EDGE,GREY,

    BLACK,UNUSED1,UNUSED3,UNUSED2,       /* ???? */

    /* #7. Mario's body? */
    BLACK,DKGREEN,RED,BLUE,
    /* #8. Mario's head */
    BLACK,LTORANGE,BROWN,BLUE,

    /* #9. Mario's BULLETS (weak gun) */
    BLACK,RED,GREEN,BLUE,

    /* #10. Bee hives & 2-hit bugs (level 3) (EYES,WINGS/FEET,BODY/ANTENNA)
            also color of SHOTS & PLAYER when player gets spray bottle. */
    BLACK,RED,BROWN,YELLOW,

    /* #11. Bugs-common ones (EYES,WINGS/FEET,BODY/ANTENNA) */
    BLACK,DKGREEN,GREEN,BLUE,

    /* #12. Bugs */
    BLACK,DKGREEN,BLUE,GREEN,

    /* #13. Worm (BODY,EYES/STRIPES,STRIPES) */
    BLACK,DKCYAN,RED,YELLOW,

    /* #14. Spray Bottle & flowers? (near DK on level 1) */
    BLACK,RED,GREEN,GREY,

    /* #15. Donkey Kong's body. (BODY,CHEST,EDGES), also BALL on level 2. */
    BLACK,RED,BROWN,RED
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz ? */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	dkong3_vh_start,
	generic_vh_stop,
	dkong3_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( dkong3_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "dk3c.7b",  0x0000, 0x2000 )
	ROM_LOAD( "dk3c.7c",  0x2000, 0x2000 )
	ROM_LOAD( "dk3c.7d",  0x4000, 0x2000 )
	ROM_LOAD( "dk3c.7e",  0x8000, 0x2000 )

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dk3v.3n",  0x0000, 0x1000 )
	ROM_LOAD( "dk3v.3p",  0x1000, 0x1000 )
	ROM_LOAD( "dk3v.7c",  0x2000, 0x1000 )
	ROM_LOAD( "dk3v.7d",  0x3000, 0x1000 )
	ROM_LOAD( "dk3v.7e",  0x4000, 0x1000 )
	ROM_LOAD( "dk3v.7f",  0x5000, 0x1000 )

	ROM_REGION(0x4000)	/* sound? */
	ROM_LOAD( "dk3c.5l",  0x0000, 0x2000 )
	ROM_LOAD( "dk3c.6h",  0x2000, 0x2000 )
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x6b1d],"\x00\x20\x01",3) == 0 &&
			memcmp(&RAM[0x6ba5],"\x00\x32\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x6b00],1,34*5,f);	/* hi scores */
			RAM[0x68f3] = RAM[0x6b1f];
			RAM[0x68f4] = RAM[0x6b1e];
			RAM[0x68f5] = RAM[0x6b1d];
			fread(&RAM[0x6c20],1,0x40,f);	/* distributions */
			fread(&RAM[0x6c16],1,4,f);
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
		fwrite(&RAM[0x6b00],1,34*5,f);	/* hi scores */
		fwrite(&RAM[0x6c20],1,0x40,f);	/* distribution */
		fwrite(&RAM[0x6c16],1,4,f);
		fclose(f);
	}
}



struct GameDriver dkong3_driver =
{
	"Donkey Kong 3",
	"dkong3",
	"MIRKO BUFFONI\nNICOLA SALMORIA\nMATTHEW HILLMER",
	&machine_driver,

	dkong3_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

	hiload, hisave
};
