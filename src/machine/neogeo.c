#include "driver.h"
#include "machine/pd4990a.h"
#include <time.h>

data16_t *neogeo_ram16;
data16_t *neogeo_sram16;

static int sram_locked;
static offs_t sram_protection_hack;

extern int neogeo_has_trackball;
extern int neogeo_irq2type;


/***************** MEMCARD GLOBAL VARIABLES ******************/
int mcd_action=0;
int mcd_number=0;
int memcard_status=0;		/* 1=Inserted 0=No card */
int memcard_number=0;		/* 000...999, -1=None */
int memcard_manager=0;		/* 0=Normal boot 1=Call memcard manager */
UINT8 *neogeo_memcard;		/* Pointer to 2kb RAM zone */

/*************** MEMCARD FUNCTION PROTOTYPES *****************/
READ16_HANDLER( neogeo_memcard16_r );
WRITE16_HANDLER( neogeo_memcard16_w );
int neogeo_memcard_load(int);
void neogeo_memcard_save(void);
void neogeo_memcard_eject(void);
int neogeo_memcard_create(int);



static void neogeo_custom_memory(void);


/* This function is called on every reset */
void neogeo_init_machine(void)
{
	data16_t src, res, *mem16= (data16_t *)memory_region(REGION_USER1);
	time_t ltime;
	struct tm *today;


	/* Reset variables & RAM */
	memset (neogeo_ram16, 0, 0x10000);

	/* Set up machine country */
	src = readinputport(5);
	res = src & 0x3;

	/* Console/arcade mode */
	if (src & 0x04)
		res |= 0x8000;

	/* write the ID in the system BIOS ROM */
	mem16[0x0200] = res;

	if (memcard_manager==1)
	{
		memcard_manager=0;
		mem16[0x11b1a >> 1] = 0x500a;
	}
	else
	{
		mem16[0x11b1a >> 1] = 0x1b6a;
	}

	time(&ltime);
	today = localtime(&ltime);

	pd4990a.seconds = ((today->tm_sec/10)<<4) + (today->tm_sec%10);
	pd4990a.minutes = ((today->tm_min/10)<<4) + (today->tm_min%10);
	pd4990a.hours = ((today->tm_hour/10)<<4) + (today->tm_hour%10);
	pd4990a.days = ((today->tm_mday/10)<<4) + (today->tm_mday%10);
	pd4990a.month = (today->tm_mon + 1);
	pd4990a.year = ((today->tm_year/10)<<4) + (today->tm_year%10);
	pd4990a.weekday = today->tm_wday;
}


/* This function is only called once per game. */
void init_neogeo(void)
{
	extern struct YM2610interface neogeo_ym2610_interface;
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	data8_t *mem08;

	if (memory_region(REGION_SOUND2))
	{
		logerror("using memory region %d for Delta T samples\n",REGION_SOUND2);
		neogeo_ym2610_interface.pcmromb[0] = REGION_SOUND2;
	}
	else
	{
		logerror("using memory region %d for Delta T samples\n",REGION_SOUND1);
		neogeo_ym2610_interface.pcmromb[0] = REGION_SOUND1;
	}

	/* Allocate ram banks */
	neogeo_ram16 = malloc (0x10000);
	cpu_setbank(1, neogeo_ram16);

	/* Set the biosbank */
	cpu_setbank(3, memory_region(REGION_USER1));

	/* Set the 2nd ROM bank */
	mem16 = (data16_t *)memory_region(REGION_CPU1);
	if (memory_region_length(REGION_CPU1) > 0x100000)
	{
		cpu_setbank(4, &mem16[0x80000]);
	}
	else
	{
		cpu_setbank(4, &mem16[0x00000]);
	}

	/* Set the sound CPU ROM banks */
	mem08 = memory_region(REGION_CPU2);
	cpu_setbank(5,&mem08[0x08000]);
	cpu_setbank(6,&mem08[0x0c000]);
	cpu_setbank(7,&mem08[0x0e000]);
	cpu_setbank(8,&mem08[0x0f000]);

	/* Allocate and point to the memcard - bank 5 */
	neogeo_memcard = calloc (0x800, 1);
	memcard_status=0;
	memcard_number=0;


	mem16 = (data16_t *)memory_region(REGION_USER1);

	if (mem16[0x11b00 >> 1] == 0x4eba)
	{
		/* standard bios */
		neogeo_has_trackball = 0;

		/* Remove memory check for now */
		mem16[0x11b00 >> 1] = 0x4e71;
		mem16[0x11b02 >> 1] = 0x4e71;
		mem16[0x11b16 >> 1] = 0x4ef9;
		mem16[0x11b18 >> 1] = 0x00c1;
		mem16[0x11b1a >> 1] = 0x1b6a;

		/* Patch bios rom, for Calendar errors */
		mem16[0x11c14 >> 1] = 0x4e71;
		mem16[0x11c16 >> 1] = 0x4e71;
		mem16[0x11c1c >> 1] = 0x4e71;
		mem16[0x11c1e >> 1] = 0x4e71;

		/* Rom internal checksum fails for now.. */
		mem16[0x11c62 >> 1] = 0x4e71;
		mem16[0x11c64 >> 1] = 0x4e71;
	}
	else
	{
		/* special bios with trackball support */
		neogeo_has_trackball = 1;

		/* TODO: check the memcard manager patch in neogeo_init_machine(), */
		/* it probably has to be moved as well */

		/* Remove memory check for now */
		mem16[0x10c2a >> 1] = 0x4e71;
		mem16[0x10c2c >> 1] = 0x4e71;
		mem16[0x10c40 >> 1] = 0x4ef9;
		mem16[0x10c42 >> 1] = 0x00c1;
		mem16[0x10c44 >> 1] = 0x0c94;

		/* Patch bios rom, for Calendar errors */
		mem16[0x10d3e >> 1] = 0x4e71;
		mem16[0x10d40 >> 1] = 0x4e71;
		mem16[0x10d46 >> 1] = 0x4e71;
		mem16[0x10d48 >> 1] = 0x4e71;

		/* Rom internal checksum fails for now.. */
		mem16[0x10d8c >> 1] = 0x4e71;
		mem16[0x10d8e >> 1] = 0x4e71;
	}

	/* Install custom memory handlers */
	neogeo_custom_memory();


	/* Flag how to handle IRQ2 raster effect */
	/* 0=write 0,2	 1=write2,0 */
	if (!strcmp(Machine->gamedrv->name,"neocup98") ||
		!strcmp(Machine->gamedrv->name,"ssideki3") ||
		!strcmp(Machine->gamedrv->name,"ssideki4"))
		neogeo_irq2type = 1;
}

/******************************************************************************/

static int prot_data;

static READ16_HANDLER( fatfury2_protection_16_r )
{
	data16_t res = (prot_data >> 24) & 0xff;

	switch (offset)
	{
	case 0x55550 >> 1:
	case 0xffff0 >> 1:
	case 0x00000 >> 1:
	case 0xff000 >> 1:
	case 0x36000 >> 1:
	case 0x36008 >> 1:
		return res;

	case 0x36004 >> 1:
	case 0x3600c >> 1:
		return ((res & 0xf0) >> 4) | ((res & 0x0f) << 4);

	default:
logerror("unknown protection read at pc %06x, offset %08x\n",cpu_get_pc(),offset<<1);
		return 0;
	}
}

static WRITE16_HANDLER( fatfury2_protection_16_w )
{
	switch (offset)
	{
	case 0x55552 >> 1:	 /* data == 0x5555; read back from 55550, ffff0, 00000, ff000 */
		prot_data = 0xff00ff00;
		break;

	case 0x56782 >> 1:	 /* data == 0x1234; read back from 36000 *or* 36004 */
		prot_data = 0xf05a3601;
		break;

	case 0x42812 >> 1:	 /* data == 0x1824; read back from 36008 *or* 3600c */
		prot_data = 0x81422418;
		break;

	case 0x55550 >> 1:
	case 0xffff0 >> 1:
	case 0xff000 >> 1:
	case 0x36000 >> 1:
	case 0x36004 >> 1:
	case 0x36008 >> 1:
	case 0x3600c >> 1:
		prot_data <<= 8;
		break;

	default:
logerror("unknown protection write at pc %06x, offset %08x, data %02x\n",cpu_get_pc(),offset,data);
		break;
	}
}

static READ16_HANDLER( popbounc_sfix_16_r )
{
	if (cpu_get_pc()==0x6b10)
		return 0;
	return neogeo_ram16[0x4fbc >> 1];
}

static void neogeo_custom_memory(void)
{
	/* Individual games can go here... */

	/* kludges */

	if (!strcmp(Machine->gamedrv->name,"gururin"))
	{
		/* Fix a really weird problem. The game clears the video RAM but goes */
		/* beyond the tile RAM, corrupting the zoom control RAM. After that it */
		/* initializes the control RAM, but then corrupts it again! */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0x1328 >> 1] = 0x4e71;
		mem16[0x132a >> 1] = 0x4e71;
		mem16[0x132c >> 1] = 0x4e71;
		mem16[0x132e >> 1] = 0x4e71;
	}

	if (!Machine->sample_rate &&
			!strcmp(Machine->gamedrv->name,"popbounc"))
	/* the game hangs after a while without this patch */
		install_mem_read16_handler(0, 0x104fbc, 0x104fbd, popbounc_sfix_16_r);

	/* hacks to make the games which do protection checks run in arcade mode */
	/* we write protect a SRAM location so it cannot be set to 1 */
	sram_protection_hack = ~0;
	if (!strcmp(Machine->gamedrv->name,"fatfury3") ||
			 !strcmp(Machine->gamedrv->name,"samsho3") ||
			 !strcmp(Machine->gamedrv->name,"samsho4") ||
			 !strcmp(Machine->gamedrv->name,"aof3") ||
			 !strcmp(Machine->gamedrv->name,"rbff1") ||
			 !strcmp(Machine->gamedrv->name,"rbffspec") ||
			 !strcmp(Machine->gamedrv->name,"kof95") ||
			 !strcmp(Machine->gamedrv->name,"kof96") ||
			 !strcmp(Machine->gamedrv->name,"kof97") ||
			 !strcmp(Machine->gamedrv->name,"kof98") ||
			 !strcmp(Machine->gamedrv->name,"kof99") ||
			 !strcmp(Machine->gamedrv->name,"kizuna") ||
			 !strcmp(Machine->gamedrv->name,"lastblad") ||
			 !strcmp(Machine->gamedrv->name,"lastbld2") ||
			 !strcmp(Machine->gamedrv->name,"rbff2") ||
			 !strcmp(Machine->gamedrv->name,"mslug2") ||
			 !strcmp(Machine->gamedrv->name,"garou"))
		sram_protection_hack = 0x100 >> 1;

	if (!strcmp(Machine->gamedrv->name,"pulstar"))
		sram_protection_hack = 0x35a >> 1;

	if (!strcmp(Machine->gamedrv->name,"ssideki"))
	{
		/* patch out protection check */
		/* the protection routines are at 0x25dcc and involve reading and writing */
		/* addresses in the 0x2xxxxx range */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0x2240 >> 1] = 0x4e71;
	}

	/* Hacks the program rom of Fatal Fury 2, needed either in arcade or console mode */
	/* otherwise at level 2 you cannot hit the opponent and other problems */
	if (!strcmp(Machine->gamedrv->name,"fatfury2"))
	{
		/* there seems to also be another protection check like the countless ones */
		/* patched above by protecting a SRAM location, but that trick doesn't work */
		/* here (or maybe the SRAM location to protect is different), so I patch out */
		/* the routine which trashes memory. Without this, the game goes nuts after */
		/* the first bonus stage. */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0xb820 >> 1] = 0x4e71;
		mem16[0xb822 >> 1] = 0x4e71;

		/* again, the protection involves reading and writing addresses in the */
		/* 0x2xxxxx range. There are several checks all around the code. */
		install_mem_read16_handler(0, 0x200000, 0x2fffff, fatfury2_protection_16_r);
		install_mem_write16_handler(0, 0x200000, 0x2fffff, fatfury2_protection_16_w);
	}

	if (!strcmp(Machine->gamedrv->name,"fatfury3"))
	{
		/* patch the first word, it must be 0x0010 not 0x0000 (initial stack pointer) */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0x0000 >> 1] = 0x0010;
	}

	if (!strcmp(Machine->gamedrv->name,"mslugx"))
	{
		/* patch out protection checks */
		int i;
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);

		for (i = 0;i < (0x100000 >> 1) - 4;i++)
		{
			if (mem16[i+0] == 0x0243 &&
				mem16[i+1] == 0x0001 && 	/* andi.w  #$1, D3 */
				mem16[i+2] == 0x6600)		/* bne xxxx */
			{
				mem16[i+2] = 0x4e71;
				mem16[i+3] = 0x4e71;
			}
		}

		mem16[0x3bdc >> 1] = 0x4e71;
		mem16[0x3bde >> 1] = 0x4e71;
		mem16[0x3be0 >> 1] = 0x4e71;
		mem16[0x3c0c >> 1] = 0x4e71;
		mem16[0x3c0e >> 1] = 0x4e71;
		mem16[0x3c10 >> 1] = 0x4e71;

		mem16[0x3c36 >> 1] = 0x4e71;
		mem16[0x3c38 >> 1] = 0x4e71;
	}
}



WRITE16_HANDLER( neogeo_sram16_lock_w )
{
	sram_locked = 1;
}

WRITE16_HANDLER( neogeo_sram16_unlock_w )
{
	sram_locked = 0;
}

READ16_HANDLER( neogeo_sram16_r )
{
	return neogeo_sram16[offset];
}

WRITE16_HANDLER( neogeo_sram16_w )
{
	if (sram_locked)
	{
logerror("PC %06x: warning: write %02x to SRAM %04x while it was protected\n",cpu_get_pc(),data,offset<<1);
	}
	else
	{
		if (offset == sram_protection_hack)
		{
			if (ACCESSING_LSB && (data & 0xff) == 0x01)
				return; /* fake protection pass */
		}

		COMBINE_DATA(&neogeo_sram16[offset]);
	}
}

void neogeo_nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
	{
		/* Save the SRAM settings */
		osd_fwrite_msbfirst(file,neogeo_sram16,0x2000);

		/* save the memory card */
		neogeo_memcard_save();
	}
	else
	{
		/* Load the SRAM settings for this game */
		if (file)
			osd_fread_msbfirst(file,neogeo_sram16,0x2000);
		else
			memset(neogeo_sram16,0,0x10000);

		/* load the memory card */
		neogeo_memcard_load(memcard_number);
	}
}



/*
	INFORMATION:

	Memory card is a 2kb battery backed RAM.
	It is accessed thru 0x800000-0x800FFF.
	Even bytes are always 0xFF
	Odd bytes are memcard data (0x800 bytes)

	Status byte at 0x380000: (BITS ARE ACTIVE *LOW*)

	0 PAD1 START
	1 PAD1 SELECT
	2 PAD2 START
	3 PAD2 SELECT
	4 --\  MEMORY CARD
	5 --/  INSERTED
	6 MEMORY CARD WRITE PROTECTION
	7 UNUSED (?)
*/




/********************* MEMCARD ROUTINES **********************/
READ16_HANDLER( neogeo_memcard16_r )
{
	if (memcard_status==1)
		return neogeo_memcard[offset] | 0xff00;
	else
		return ~0;
}

WRITE16_HANDLER( neogeo_memcard16_w )
{
	if (ACCESSING_MSB)
		return;

	if (memcard_status==1)
		neogeo_memcard[offset] = data & 0xff;
}

int neogeo_memcard_load(int number)
{
	char name[16];
	void *f;

	sprintf(name, "MEMCARD.%03d", number);
	if ((f=osd_fopen(0, name, OSD_FILETYPE_MEMCARD,0))!=0)
	{
		osd_fread(f,neogeo_memcard,0x800);
		osd_fclose(f);
		return 1;
	}
	return 0;
}

void neogeo_memcard_save(void)
{
	char name[16];
	void *f;

	if (memcard_number!=-1)
	{
		sprintf(name, "MEMCARD.%03d", memcard_number);
		if ((f=osd_fopen(0, name, OSD_FILETYPE_MEMCARD,1))!=0)
		{
			osd_fwrite(f,neogeo_memcard,0x800);
			osd_fclose(f);
		}
	}
}

void neogeo_memcard_eject(void)
{
   if (memcard_number!=-1)
   {
	   neogeo_memcard_save();
	   memset(neogeo_memcard, 0, 0x800);
	   memcard_status=0;
	   memcard_number=-1;
   }
}

int neogeo_memcard_create(int number)
{
	char buf[0x800];
	char name[16];
	void *f1, *f2;

	sprintf(name, "MEMCARD.%03d", number);
	if ((f1=osd_fopen(0, name, OSD_FILETYPE_MEMCARD,0))==0)
	{
		if ((f2=osd_fopen(0, name, OSD_FILETYPE_MEMCARD,1))!=0)
		{
			osd_fwrite(f2,buf,0x800);
			osd_fclose(f2);
			return 1;
		}
	}
	else
		osd_fclose(f1);

	return 0;
}

