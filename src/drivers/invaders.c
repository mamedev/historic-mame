/***************************************************************************

Space Invaders memory map (preliminary)

0000-1fff ROM
2000-23ff RAM
2400-3fff Video RAM
4000-57ff ROM (some clones use more, others less)


I/O ports:
read:
01        IN0
02        IN1
03        bit shift register read

write:
02        shift amount
03        ?
04        shift data
05        ?
06        ?

Space Invaders
--------------
Input:
Port 1

   bit 0 = CREDIT (0 if deposit)
   bit 1 = 2P start(1 if pressed)
   bit 2 = 1P start(1 if pressed)
   bit 3 = 0 if TILT
   bit 4 = shot(1 if pressed)
   bit 5 = left(1 if pressed)
   bit 6 = right(1 if pressed)
   bit 7 = Always 1

Port 2
   bit 0 = 00 = 3 ships  10 = 5 ships
   bit 1 = 01 = 4 ships  11 = 6 ships
   bit 2 = Always 0 (1 if TILT)
   bit 3 = 0 = extra ship at 1500, 1 = extra ship at 1000
   bit 4 = shot player 2 (1 if pressed)
   bit 5 = left player 2 (1 if pressed)
   bit 6 = right player 2 (1 if pressed)
   bit 7 = Coin info if 0, last screen


Space Invaders Deluxe
---------------------

This info was gleaned from examining the schematics of the SID
manual which Mitchell M. Rohde keeps at:

http://www.eecs.umich.edu/~bovine/space_invaders/index.html

Input:
Port 0
   bit 0 = SW 8 unused
   bit 1 = Always 0
   bit 2 = Always 1
   bit 3 = Always 0
   bit 4 = Always 1
   bit 5 = Always 1
   bit 6 = 1, Name Reset when set to zero
   bit 7 = Always 1

Port 1
   bit 0 = CREDIT (0 if deposit)
   bit 1 = 2P start(1 if pressed)
   bit 2 = 1P start(1 if pressed)
   bit 3 = 0 if TILT
   bit 4 = shot(1 if pressed)
   bit 5 = left(1 if pressed)
   bit 6 = right(1 if pressed)
   bit 7 = Always 1

Port 2
   bit 0 = Number of bases 0 - 3 , 1 - 4  (SW 4)
   bit 1 = 1 or 0 connected to SW 3 not used
   bit 2 = Always 0  (1 if TILT)
   bit 3 = Preset         (SW 2)
   bit 4 = shot player 2 (1 if pressed)  These are also used
   bit 5 = left player 2 (1 if pressed)  to enter the name in the
   bit 6 = right player 2 (1 if pressed) high score field.
   bit 7 = Coin info if 0 (SW 1) last screen

Invaders and Invaders Deluxe both write to I/O port 6 but
I don't know why.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern int invaders_shift_data_r(int offset);
extern void invaders_shift_amount_w(int offset,int data);
extern void invaders_shift_data_w(int offset,int data);
extern int invaders_interrupt(void);

extern unsigned char *invaders_videoram;
extern void invaders_videoram_w(int offset,int data);
extern void lrescue_videoram_w(int offset,int data);    /* V.V */
extern void invrvnge_videoram_w(int offset,int data);   /* V.V */
extern void invaders_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void invaders_sh_port3_w(int offset,int data);
extern void invaders_sh_port5_w(int offset,int data);
extern void invaders_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x57ff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, invaders_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress lrescue_writemem[] = /* V.V */ /* Whole function */
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, lrescue_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress invrvnge_writemem[] = /* V.V */ /* Whole function */
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, invrvnge_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct IOReadPort readport[] =
{
	{ 0x01, 0x01, input_port_0_r },
	{ 0x02, 0x02, input_port_1_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};


static struct IOReadPort invdelux_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

/* Catch the write to unmapped I/O port 6 */
void invaders_dummy_write(int offset,int data)
{
}

static struct IOWritePort writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
        { 0x03, 0x03, invaders_sh_port3_w },
	{ 0x04, 0x04, invaders_shift_data_w },
        { 0x05, 0x05, invaders_sh_port5_w },
        { 0x06, 0x06, invaders_dummy_write },
	{ -1 }  /* end of table */
};

static struct IOWritePort invdelux_writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
        { 0x03, 0x03, invaders_sh_port3_w },
	{ 0x04, 0x04, invaders_shift_data_w },
        { 0x05, 0x05, invaders_sh_port5_w },
        { 0x06, 0x06, invaders_dummy_write },
	{ -1 }  /* end of table */
};


static struct InputPort input_ports[] =
{
	{       /* IN0 */
		0x81,
		{ OSD_KEY_3, OSD_KEY_2, OSD_KEY_1, 0,
				OSD_KEY_CONTROL, OSD_KEY_LEFT, OSD_KEY_RIGHT, 0 },
		{ 0, 0, 0, 0,
				OSD_JOY_FIRE, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0 }
	},
	{       /* IN1 */
		0x00,
		{ 0, 0, OSD_KEY_T, 0,
				OSD_KEY_CONTROL, OSD_KEY_LEFT, OSD_KEY_RIGHT, 0 },
		{ 0, 0, 0, 0,
				OSD_JOY_FIRE, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0 }
	},
	{ -1 }
};

static struct InputPort invdelux_input_ports[] =
{
	{       /* IN0 */
		0xf4,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_N, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN1 */
		0x81,
		{ OSD_KEY_3, OSD_KEY_2, OSD_KEY_1, 0,
				OSD_KEY_CONTROL, OSD_KEY_LEFT, OSD_KEY_RIGHT, 0 },
		{ 0, 0, 0, 0,
				OSD_JOY_FIRE, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0 }
	},
	{       /* IN2 */
		0x00,
		{ 0, 0, OSD_KEY_T, 0,
                	    OSD_KEY_CONTROL, OSD_KEY_LEFT, OSD_KEY_RIGHT, 0 },
		{ 0, 0, 0, 0, OSD_JOY_FIRE,    OSD_JOY_LEFT, OSD_JOY_RIGHT, 0 }
	},
	{ -1 }
};



static struct KEYSet keys[] =
{
	{ 0, 5, "PL1 MOVE LEFT"  },
	{ 0, 6, "PL1 MOVE RIGHT" },
	{ 0, 4, "PL1 FIRE"       },
	{ 1, 5, "PL1 MOVE LEFT"  },
	{ 1, 6, "PL1 MOVE RIGHT" },
	{ 1, 4, "PL1 FIRE"       },
	{ -1 }
};

static struct KEYSet invaders_keys[] =
{
	{ 0, 5, "PL1 MOVE LEFT"  },
	{ 0, 6, "PL1 MOVE RIGHT" },
	{ 0, 4, "PL1 FIRE"       },
	{ 1, 5, "PL2 MOVE LEFT"  },
	{ 1, 6, "PL2 MOVE RIGHT" },
	{ 1, 4, "PL2 FIRE"       },
	{ -1 }
};

/* Deluxe uses set two to input the name */
static struct KEYSet invdelux_keys[] =
{
	{ 1, 5, "PL1 MOVE LEFT"  },
	{ 1, 6, "PL1 MOVE RIGHT" },
	{ 1, 4, "PL1 FIRE"       },
	{ 2, 5, "PL2 MOVE LEFT"  },
	{ 2, 6, "PL2 MOVE RIGHT" },
	{ 2, 4, "PL2 FIRE"       },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 1, 0x03, "LIVES", { "3", "4", "5", "6" } },
	{ 1, 0x08, "BONUS", { "1500", "1000" }, 1 },
	{ -1 }
};

static struct DSW invaders_dsw[] =
{
	{ 1, 0x03, "BASES", { "3", "4", "5", "6" } },
	{ 1, 0x08, "BONUS", { "1500", "1000" }, 1 },
	{ 1, 0x80, "COIN INFO", { "ON", "OFF" }, 0 },
	{ -1 }
};

static struct DSW invdelux_dsw[] =
{
	{ 2, 0x01, "BASES", { "3", "4" }, 0},
	{ 2, 0x08, "PRESET MODE", { "OFF", "ON" }, 0 },
	{ 2, 0x80, "COIN INFO", { "ON", "OFF" }, 0 },
	{ -1 }
};



static unsigned char palette[] = /* V.V */ /* Smoothed pure colors, overlays are not so contrasted */
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0x20,0x20, /* RED */
	0x20,0xff,0x20, /* GREEN */
	0xff,0xff,0x20, /* YELLOW */
	0xff,0xff,0xff, /* WHITE */
	0x20,0xff,0xff, /* CYAN */
	0xff,0x20,0xff  /* PURPLE */
};

enum { BLACK,RED,GREEN,YELLOW,WHITE,CYAN,PURPLE }; /* V.V */



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,        /* 1 Mhz? */
			0,
			readmem,writemem,readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60,
        0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	invaders_sh_update
};


static struct MachineDriver lrescue_machine_driver = /* V.V */ /* Whole function */
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,        /* 2 Mhz? */
			0,
			readmem,lrescue_writemem,readport,writeport, /* V.V */
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60,
        0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	invaders_sh_update
};


static struct MachineDriver invrvnge_machine_driver = /* V.V */ /* Whole function */
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,        /* 2 Mhz? */
			0,
			readmem,invrvnge_writemem,readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	invaders_sh_update
};



static struct MachineDriver invdelux_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,        /* 2 Mhz? */
			0,
			readmem,writemem,invdelux_readport, invdelux_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60,
        0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	invaders_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( invaders_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invaders.h", 0x0000, 0x0800)
	ROM_LOAD( "invaders.g", 0x0800, 0x0800)
	ROM_LOAD( "invaders.f", 0x1000, 0x0800)
	ROM_LOAD( "invaders.e", 0x1800, 0x0800)
ROM_END

ROM_START( spaceatt_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "spaceatt.h", 0x0000, 0x0800)
	ROM_LOAD( "spaceatt.g", 0x0800, 0x0800)
	ROM_LOAD( "spaceatt.f", 0x1000, 0x0800)
	ROM_LOAD( "spaceatt.e", 0x1800, 0x0800)
ROM_END

ROM_START( invrvnge_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invrvnge.h", 0x0000, 0x0800)
	ROM_LOAD( "invrvnge.g", 0x0800, 0x0800)
	ROM_LOAD( "invrvnge.f", 0x1000, 0x0800)
	ROM_LOAD( "invrvnge.e", 0x1800, 0x0800)
ROM_END

ROM_START( invdelux_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invdelux.h", 0x0000, 0x0800)
	ROM_LOAD( "invdelux.g", 0x0800, 0x0800)
	ROM_LOAD( "invdelux.f", 0x1000, 0x0800)
	ROM_LOAD( "invdelux.e", 0x1800, 0x0800)
	ROM_LOAD( "invdelux.d", 0x4000, 0x0800)
ROM_END

ROM_START( galxwars_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "galxwars.0", 0x0000, 0x0400)
	ROM_LOAD( "galxwars.1", 0x0400, 0x0400)
	ROM_LOAD( "galxwars.2", 0x0800, 0x0400)
	ROM_LOAD( "galxwars.3", 0x0c00, 0x0400)
	ROM_LOAD( "galxwars.4", 0x4000, 0x0400)
	ROM_LOAD( "galxwars.5", 0x4400, 0x0400)
ROM_END

ROM_START( lrescue_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "lrescue.1", 0x0000, 0x0800)
	ROM_LOAD( "lrescue.2", 0x0800, 0x0800)
	ROM_LOAD( "lrescue.3", 0x1000, 0x0800)
	ROM_LOAD( "lrescue.4", 0x1800, 0x0800)
	ROM_LOAD( "lrescue.5", 0x4000, 0x0800)
	ROM_LOAD( "lrescue.6", 0x4800, 0x0800)
ROM_END

ROM_START( desterth_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "36_h.bin", 0x0000, 0x0800)
	ROM_LOAD( "35_g.bin", 0x0800, 0x0800)
	ROM_LOAD( "34_f.bin", 0x1000, 0x0800)
	ROM_LOAD( "33_e.bin", 0x1800, 0x0800)
	ROM_LOAD( "32_d.bin", 0x4000, 0x0800)
	ROM_LOAD( "31_c.bin", 0x4800, 0x0800)
	ROM_LOAD( "42_b.bin", 0x5000, 0x0800)
ROM_END



static const char *invaders_sample_names[] =
{
	"0.SAM",
	"1.SAM",
	"2.SAM",
	"3.SAM",
	"4.SAM",
	"5.SAM",
	"6.SAM",
	"7.SAM",
	"8.SAM",
	0       /* end of array */
};



static int invaders_hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20f4],"\x00\x00",2) == 0)
	{
		FILE *f;

		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x20f4],1,2,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void invaders_hisave(const char *name)
{
	FILE *f;

	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x20f4],1,2,f);
		fclose(f);
	}
}


static int invdelux_hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2340],"\x1b\x1b",2) == 0)
	{
		FILE *f;

		if ((f = fopen(name,"rb")) != 0)
		{
	 /* Load the actual score */
		   fread(&RAM[0x20f4],1, 0x2,f);
	 /* Load the name */
		   fread(&RAM[0x2340],1, 0xa,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void invdelux_hisave(const char *name)
{
	FILE *f;

	if ((f = fopen(name,"wb")) != 0)
	{
      /* Save the actual score */
		fwrite(&RAM[0x20f4],1, 0x2,f);
      /* Save the name */
		fwrite(&RAM[0x2340],1, 0xa,f);
		fclose(f);
	}
}

static int lrescue_hiload(const char *name)     /* V.V */ /* Whole function */
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20CF],"\x1b\x1b",2) == 0)
	{
		FILE *f;

		if ((f = fopen(name,"rb")) != 0)
	       {
	 /* Load the actual score */
		   fread(&RAM[0x20F4],1, 0x2,f);
	 /* Load the name */
		   fread(&RAM[0x20CF],1, 0xa,f);
	 /* Load the high score length */
		   fread(&RAM[0x20DB],1, 0x1,f);
		   fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */

}

static int desterth_hiload(const char *name)     /* V.V */ /* Whole function */
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20CF],"\x1b\x07",2) == 0)
	{
		FILE *f;

		if ((f = fopen(name,"rb")) != 0)
	       {
	 /* Load the actual score */
		   fread(&RAM[0x20F4],1, 0x2,f);
	 /* Load the name */
		   fread(&RAM[0x20CF],1, 0xa,f);
	 /* Load the high score length */
		   fread(&RAM[0x20DB],1, 0x1,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */

}



static void lrescue_hisave(const char *name)    /* V.V */ /* Whole function */
{
	FILE *f;

	if ((f = fopen(name,"wb")) != 0)
	{
      /* Save the actual score */
		fwrite(&RAM[0x20F4],1,0x02,f);
      /* Save the name */
		fwrite(&RAM[0x20CF],1,0xa,f);
      /* Save the high score length */
		fwrite(&RAM[0x20DB],1,0x1,f);
		fclose(f);
	}
}



struct GameDriver invaders_driver =
{
	"Space Invaders",
	"invaders",
	"MICHAEL STRUTTS\nNICOLA SALMORIA\nTORMOD TJABERG\nMIRKO BUFFONI\nVALERIO VERRANDO",    /* V.V */
	&machine_driver,

	invaders_rom,
	0, 0,
	invaders_sample_names,

	input_ports, invaders_dsw, invaders_keys,

	0, palette, 0,
	8*13, 8*16,

	invaders_hiload, invaders_hisave
};

struct GameDriver earthinv_driver =
{
	"Super Earth Invasion",
	"earthinv",
	"MICHAEL STRUTTS\nNICOLA SALMORIA\nTORMOD TJABERG\nMIRKO BUFFONI\nVALERIO VERRANDO",    /* V.V */
	&machine_driver,

	invaders_rom,
	0, 0,
	invaders_sample_names,

	input_ports, dsw, keys,

	0, palette, 0,
	8*13, 8*16,

	0, 0
};


struct GameDriver spaceatt_driver =
{
	"Space Attack II",
	"spaceatt",
	"MICHAEL STRUTTS\nNICOLA SALMORIA\nTORMOD TJABERG\nMIRKO BUFFONI\nVALERIO VERRANDO",    /* V.V */
	&machine_driver,

	spaceatt_rom,
	0, 0,
	invaders_sample_names,

	input_ports, dsw, keys,

	0, palette, 0,
	8*13, 8*16,

	0, 0
};

struct GameDriver invrvnge_driver =
{
	"Invaders Revenge",
	"invrvnge",
	"MICHAEL STRUTTS\nNICOLA SALMORIA\nTORMOD TJABERG\nMIRKO BUFFONI",
	&invrvnge_machine_driver,

	invrvnge_rom,
	0, 0,
	invaders_sample_names,

	input_ports, dsw, keys,

	0, palette, 0,
	8*13, 8*16,

	0, 0
};

struct GameDriver invdelux_driver =
{
	"Invaders Deluxe",
	"invdelux",
	"MICHAEL STRUTTS\nNICOLA SALMORIA\nTORMOD TJABERG\nMIRKO BUFFONI\nVALERIO VERRANDO",    /* V.V */
	&invdelux_machine_driver,

	invdelux_rom,
	0, 0,
	invaders_sample_names,

	invdelux_input_ports, invdelux_dsw, invdelux_keys,

	0, palette, 0,
	8*13, 8*16,

	invdelux_hiload, invdelux_hisave
};

struct GameDriver galxwars_driver =
{
	"Galaxy Wars",
	"galxwars",
	"MICHAEL STRUTTS\nNICOLA SALMORIA\nTORMOD TJABERG\nMIRKO BUFFONI\nVALERIO VERRANDO",    /* V.V */
	&machine_driver,

	galxwars_rom,
	0, 0,
	invaders_sample_names,

	input_ports, dsw, keys,

	0, palette, 0,
	8*13, 8*16,

	0, 0
};

struct GameDriver lrescue_driver =
{
	"Lunar Rescue",
	"lrescue",
	"MICHAEL STRUTTS\nNICOLA SALMORIA\nTORMOD TJABERG\nMIRKO BUFFONI\nVALERIO VERRANDO",    /* V.V */
	&lrescue_machine_driver,

	lrescue_rom,
	0, 0,
	invaders_sample_names,

	input_ports, dsw, keys,

	0, palette, 0,
	8*13, 8*16,

	lrescue_hiload, lrescue_hisave  /* V.V */
};


struct GameDriver desterth_driver =
{
	"Destination Earth",
	"desterth",
	"MICHAEL STRUTTS\nNICOLA SALMORIA\nTORMOD TJABERG\nMIRKO BUFFONI\nVALERIO VERRANDO",    /* V.V */
	&machine_driver,

	desterth_rom,
	0, 0,
	invaders_sample_names,

	input_ports, dsw, keys,

	0, palette, 0,
	8*13, 8*16,

	desterth_hiload, lrescue_hisave
};
