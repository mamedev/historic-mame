#include "driver.h"

static unsigned char *biosbank;
//static unsigned char *memcard;
unsigned char *neogeo_ram;
unsigned char *neogeo_sram;

int neogeo_game_fix=-1;

extern int neogeo_irq2;

extern void install_mem_read_handler(int cpu, int start, int end, int (*handler)(int));
static void neogeo_custom_memory(void);

/* This function is called on every reset */
void neogeo_init_machine (void)
{
	/* Reset variables & RAM */
	neogeo_irq2=0;
	memset (neogeo_ram, 0, 0x10000);
}

/* This function is only called once per game. */
void neogeo_onetime_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    void *f;
	extern struct YM2610interface neogeo_ym2610_interface;


	if (Machine->memory_region[7])
	{
if (errorlog) fprintf(errorlog,"using memory region 7 for Delta T samples\n");
		neogeo_ym2610_interface.pcmroma[0] = 7;
	}
	else
	{
if (errorlog) fprintf(errorlog,"using memory region 6 for Delta T samples\n");
		neogeo_ym2610_interface.pcmroma[0] = 6;
	}

    /* Allocate ram banks */
	neogeo_ram = malloc (0x10000);
    neogeo_sram = malloc (0x10000);
	cpu_setbank (1, neogeo_ram);
	cpu_setbank (2, neogeo_sram);

	/* Set the biosbank */
	cpu_setbank (3, Machine->memory_region[4]);

	/* Set the 2nd ROM bank */
    RAM = Machine->memory_region[0];
	if (Machine->memory_region_length[0] > 0x100000)
	{
		cpu_setbank (4, &RAM[0x100000]);
	}
	else
	{
		cpu_setbank (4, &RAM[0]);
	}

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
 *	Also sound revision no 3.0, but different types.
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

static int artfight_cycle_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x0143) {
		cpu_spinuntil_int();
		return RAM[0xfef3];
	}
	return RAM[0xfef3];
}

/*
 *	Sound V2.0
 *
 *	Used by puzzle Bobble and Goal Goal Goal
 *
 */

static int cycle_v2_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x0143) {
		cpu_spinuntil_int();
		return RAM[0xfeef];
	}
	return RAM[0xfeef];
}

static int vwpoint_cycle_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x0143) {
		cpu_spinuntil_int();
		return RAM[0xfe46];
	}
	return RAM[0xfe46];
}

/*
 *	Sound revision no 1.5, and some 2.0 versions,
 *	are not fit for speedups, it results in sound drops !
 *	Games that use this one are : Ghost Pilots, Joy Joy, Nam 1975
 */

/*
static int cycle_v15_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x013D) {
		cpu_spinuntil_int();
		return RAM[0xfe46];
	}
	return RAM[0xfe46];
}
*/

/*
 *	Magician Lord uses a different sound core from all other
 *	Neo Geo Games.
 */

static int maglord_cycle_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0xd487) {
		cpu_spinuntil_int();
		return RAM[0xfb91];
	}
	return RAM[0xfb91];
}

/******************************************************************************/

static void neogeo_custom_memory(void)
{
	/* Country/Console select, used by all games */
	install_mem_read_handler(0, 0x10fd82, 0x10fd83, read_softdip);

	/* NeoGeo intro screen cycle skip, used by all games */
	install_mem_read_handler(0, 0x10fe8c, 0x10fe8d, bios_cycle_skip_r);

    /* Individual games can go here... */

#if 1
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
	if (!strcmp(Machine->gamedrv->name,"artfight")) install_mem_read_handler(1, 0xfef3, 0xfef3, artfight_cycle_sr);

	if (!strcmp(Machine->gamedrv->name,"pbobble")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);
	if (!strcmp(Machine->gamedrv->name,"goalx3")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);
	if (!strcmp(Machine->gamedrv->name,"fatfury1")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);
	if (!strcmp(Machine->gamedrv->name,"mutnat")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);

	if (!strcmp(Machine->gamedrv->name,"maglord")) install_mem_read_handler(1, 0xfb91, 0xfb91, maglord_cycle_sr);
	if (!strcmp(Machine->gamedrv->name,"vwpoint")) install_mem_read_handler(1, 0xfe46, 0xfe46, vwpoint_cycle_sr);

//	if (!strcmp(Machine->gamedrv->name,"joyjoy")) install_mem_read_handler(1, 0xfe46, 0xfe46, cycle_v15_sr);
//	if (!strcmp(Machine->gamedrv->name,"nam_1975")) install_mem_read_handler(1, 0xfe46, 0xfe46, cycle_v15_sr);
//	if (!strcmp(Machine->gamedrv->name,"gpilots")) install_mem_read_handler(1, 0xfe46, 0xfe46, cycle_v15_sr);

	/* kludges */
	if (!strcmp(Machine->gamedrv->name,"blazstar")) neogeo_game_fix=0;
	if (!strcmp(Machine->gamedrv->name,"gowcaizr")) neogeo_game_fix=1;
	if (!strcmp(Machine->gamedrv->name,"realbou2")) neogeo_game_fix=2;
	if (!strcmp(Machine->gamedrv->name,"samsho3")) neogeo_game_fix=3;
	if (!strcmp(Machine->gamedrv->name,"overtop")) neogeo_game_fix=4;
	if (!strcmp(Machine->gamedrv->name,"kof97")) neogeo_game_fix=5;
	if (!strcmp(Machine->gamedrv->name,"miexchng")) neogeo_game_fix=6;
	if (!strcmp(Machine->gamedrv->name,"gururin")) neogeo_game_fix=7;
}
