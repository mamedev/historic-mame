#include "driver.h"

static unsigned char *biosbank;
//static unsigned char *memcard;
unsigned char *neogeo_ram;
unsigned char *neogeo_sram;

extern int neogeo_irq2;

extern void install_mem_read_handler(int cpu, int start, int end, int (*handler)(int));
static void neogeo_custom_memory(void);

/* This function is called on every reset */
void neogeo_init_machine (void)
{
	/* Reset variables & RAM */
	neogeo_irq2=0;
	memset (neogeo_ram, 0, 0x10000);

	/* Credits are stored in the SRAM in case of power failures, we don't really
		want this behaviour in Mame, so zero credit entry on bootup.  Without
        this a game will go straight to intro screen on bootup if credits are
        in the system */
	WRITE_WORD(&neogeo_sram[0x34],0x0000);
}

/* This function is only called once per game. */
void neogeo_onetime_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    void *f;

    /* Allocate ram banks */
	neogeo_ram = malloc (0x10000);
    neogeo_sram = malloc (0x10000);
	cpu_setbank (1, neogeo_ram);
	cpu_setbank (2, neogeo_sram);

	/* Set the biosbank */
	cpu_setbank (3, Machine->memory_region[4]);

	/* Set the 2nd ROM bank */
    RAM = Machine->memory_region[0];
	cpu_setbank (4, &RAM[0x100000]);

	/* Allocate and point to the memcard - bank 5 */
 //	memcard = calloc (0x1000, 1);
 //	cpu_setbank (5, memcard);

	/* Patch bios rom, for Calendar errors */
	RAM = Machine->memory_region[4];
    WRITE_WORD(&RAM[0x11c14],0x4e71);
    WRITE_WORD(&RAM[0x11c16],0x4e71);
    WRITE_WORD(&RAM[0x11c1c],0x4e71);
    WRITE_WORD(&RAM[0x11c1e],0x4e71);

    /* Rom internal checksum fails for now.. */
    WRITE_WORD(&RAM[0x11c62],0x4e71);
    WRITE_WORD(&RAM[0x11c64],0x4e71);

	/* Remove memory check for now */
    WRITE_WORD(&RAM[0x11b00],0x4ef9);
    WRITE_WORD(&RAM[0x11b02],0x00c1);
    WRITE_WORD(&RAM[0x11b04],0x1b6a);

    /* Load the SRAM settings for this game */
	memset (neogeo_sram, 0, 0x10000);
	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f, neogeo_sram, 0x2000);
		osd_fclose (f);
	}
    else {
        /* If we have no SRAM settings we need to set up machine defaults */
		WRITE_WORD(&neogeo_sram[0x3a],0x0101); /* Coin slot 1: 1 coin 1 credit */
		WRITE_WORD(&neogeo_sram[0x3c],0x0102); /* Coin slot 2: 1 coin 2 credits */
        /* Without above two settings, coin inputs won't work at all! */

		WRITE_WORD(&neogeo_sram[0x3e],0x0101); /* Service switch parameters? */

		WRITE_WORD(&neogeo_sram[0x40],0x0103); /* Unknown */

		WRITE_WORD(&neogeo_sram[0x42],0x0000); /* Game start only when credited */
		WRITE_WORD(&neogeo_sram[0x44],0x303b); /* Game start compulsion is 1st byte */
		WRITE_WORD(&neogeo_sram[0x46],0x0000); /* Attract mode sound set per game */
    }

	/* Install custom memory handlers */
	neogeo_custom_memory();
}

/******************************************************************************/

static int read_softdip (int offset)
{
	int ret=0,src;

//if (errorlog) fprintf(errorlog,"CPU %04x - Read soft dip\n",cpu_getpc());

    /* Memory check *
    if (cpu_getpc()==0xc13154) return result; */

	/* Set up machine country */
    src = readinputport(5);
    ret |= src&0x3;

    /* Console/arcade mode */
	if (src & 0x04)
		ret |= 0x8000;

	return ret;
}

static int bios_cycle_skip_r(int offset)
{
	cpu_spinuntil_int();
	return 0;
}

/******************************************************************************/
/* Routines to speed up the main processor 				      */
/******************************************************************************/
static int puzzledp_cycle_r(int offset)
{
	if (cpu_getpc()==0x12f2) {cpu_spinuntil_int(); return 1;}
	return READ_WORD(&neogeo_ram[0]);
}

static int samsho4_cycle_r(int offset)
{
	if (cpu_getpc()==0xaffc) {cpu_spinuntil_int(); return 0;}
	return READ_WORD(&neogeo_ram[0x830c]);
}

static int karnov_r_cycle_r(int offset)
{
	if (cpu_getpc()==0x5b56) {cpu_spinuntil_int(); return 0;}
	return READ_WORD(&neogeo_ram[0x3466]);
}

static int wjammers_cycle_r(int offset)
{
	if (cpu_getpc()==0x1362e) {cpu_spinuntil_int(); return READ_WORD(&neogeo_ram[0x5a])&0x7fff;}
	return READ_WORD(&neogeo_ram[0x5a]);
}

static int strhoops_cycle_r(int offset)
{
	if (cpu_getpc()==0x29a) {cpu_spinuntil_int(); return 0;}
	return READ_WORD(&neogeo_ram[0x1200]);
}

static int magdrop3_cycle_r(int offset)
{
	if (cpu_getpc()==0xa378) {cpu_spinuntil_int(); return READ_WORD(&neogeo_ram[0x60])&0x7fff;}
	return READ_WORD(&neogeo_ram[0x60]);
}

static int neobombe_cycle_r(int offset)
{
	if (cpu_getpc()==0x9f2) {cpu_spinuntil_int(); return 0xffff;}
	return READ_WORD(&neogeo_ram[0x448c]);
}

/******************************************************************************/
/* Routines to speed up the sound processor AVDB 24-10-1998		      */
/******************************************************************************/

/*
 *	Sound V3.0
 *
 *	Used by puzzle de pon and Super Sidekicks 2
 *
 */
static int cycle_v3_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x0137) {
		cpu_spinuntil_int();
		return RAM[0xfeb1];
	}
	return RAM[0xfeb1];
}

/*
 *	Also sound revision no 3.0 weird :-).
 */
static int sidkicks_cycle_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x015A) {
		cpu_spinuntil_int();
		return RAM[0xfef3];
	}
	return RAM[0xfef3];
}

static int pbobble_cycle_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x0143) {
		cpu_spinuntil_int();
		return RAM[0xfeef];
	}
	return RAM[0xfeef];
}

static int joyjoy_cycle_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x0156) {
		cpu_spinuntil_int();
		return RAM[0xfeef];
	}
	return RAM[0x0156];
}

/******************************************************************************/

static void neogeo_custom_memory(void)
{
	/* Country/Console select, used by all games */
	install_mem_read_handler(0, 0x10fd82, 0x10fd83, read_softdip);

	/* NeoGeo intro screen cycle skip, used by all games */
	install_mem_read_handler(0, 0x10fe8c, 0x10fe8d, bios_cycle_skip_r);

    /* Individual games can go here... */

#if 0
	if (!strcmp(Machine->gamedrv->name,"puzzledp")) install_mem_read_handler(0, 0x100000, 0x100001, puzzledp_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"samsho4")) install_mem_read_handler(0, 0x10830c, 0x10830d, samsho4_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"karnov_r")) install_mem_read_handler(0, 0x103466, 0x103467, karnov_r_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"wjammers")) install_mem_read_handler(0, 0x10005a, 0x10005b, wjammers_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"strhoops")) install_mem_read_handler(0, 0x101200, 0x101201, strhoops_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"magdrop3")) install_mem_read_handler(0, 0x100060, 0x100061, magdrop3_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"neobombe")) install_mem_read_handler(0, 0x10448c, 0x10448d, neobombe_cycle_r);
#endif

	/* AVDB cpu spins based on sound processor status */
	if (!strcmp(Machine->gamedrv->name,"puzzledp")) install_mem_read_handler(1, 0xfeb1, 0xfeb1, cycle_v3_sr);
	if (!strcmp(Machine->gamedrv->name,"ssideki2")) install_mem_read_handler(1, 0xfeb1, 0xfeb1, cycle_v3_sr);
	if (!strcmp(Machine->gamedrv->name,"sidkicks")) install_mem_read_handler(1, 0xfef3, 0xfef3, sidkicks_cycle_sr);
	if (!strcmp(Machine->gamedrv->name,"pbobble")) install_mem_read_handler(1, 0xfeef, 0xfeef, pbobble_cycle_sr);
	if (!strcmp(Machine->gamedrv->name,"joyjoy")) install_mem_read_handler(1, 0xfe75, 0xfe75, joyjoy_cycle_sr);

}

/******************************************************************************/

