/***************************************************************************

  Leland driver

  2 x Z80
  1 x AY8910 FM chip
  1 x AY8912 FM chip
  1 x 10 BIT DAC
  6 x 8 BIT DAC
  1 x 16 BIT DAC
  1 x 80186

  To get into service mode:
  offroad  = Service & nitro 3 button (blue)
  basebal2 = Service & 1P start
  strkzone = Service & 1P start
  dblplay  = Service & 1P start

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


//#define NOISY_CPU


/* Helps document the input ports. */
#define IPT_SLAVEHALT IPT_UNKNOWN

#define LELAND "The Leland Corp."

#include "cpu\z80\z80.h"

int cpu_get_halt_line(int num)
{
    return rand() & 0x01;
	if (num==1)
	{
		int halt=cpunum_get_reg(1, Z80_HALT);
#ifdef NOISY_CPU
		if (errorlog && halt)
		{
			fprintf(errorlog, "HALT=%d\n", halt);
		}
#endif
		return halt;
	}
	return 0;
}

void cpu_set_reset_line(int num, int reset)
{
	if (reset==HOLD_LINE )
	{
#ifdef NOISY_CPU
		/* Hold reset line (drop it low). Suspend CPU. */
		if (errorlog)
		{
			fprintf(errorlog, "CPU#%d (PC=%04x) RESET LINE HELD\n", num, cpu_get_pc());
		}
#endif
		if (cpu_getstatus(num))
		{
			cpu_reset(num);
			cpu_halt(num, 0);
		}
	}
	else
	{
		if (!cpu_getstatus(num))
		{
#ifdef NOISY_CPU
			/* Resume CPU when reset line has been raised */
			if (errorlog)
			{
				fprintf(errorlog, "CPU#%d RESET LINE CLEARED... RESUMING\n", num);
			}
#endif
			cpu_halt(num, 1);
		}
	}
}

void cpu_set_test_line(int num, int test)
{
	/* TEST flag for 80186 sync instrucntion */
	if (test==HOLD_LINE )
	{
		/* Set test line */
	}
	else
	{
		/* Reset test line */
	}
}

/* Video routines */
extern int leland_vh_start(void);
extern void leland_vh_stop(void);
extern void leland_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void pigout_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* Video RAM ports */
extern void leland_master_video_addr_w(int offset, int data);
extern void leland_slave_video_addr_w(int offset, int data);
extern void leland_mvram_port_w(int offset, int data);
extern void leland_svram_port_w(int offset, int data);
extern int  leland_mvram_port_r(int offset);
extern int  leland_svram_port_r(int offset);

extern void leland_sound_port_w(int offset, int data);
extern int  leland_sound_port_r(int offset);

extern void leland_bk_xlow_w(int offset, int data);
extern void leland_bk_xhigh_w(int offset, int data);
extern void leland_bk_ylow_w(int offset, int data);
extern void leland_bk_yhigh_w(int offset, int data);

extern unsigned char *leland_palette_ram;
extern int            leland_palette_ram_size;

extern void leland_palette_ram_w(int offset, int data);
extern int  leland_palette_ram_r(int offset);

static int  leland_slave_cmd;

void (*leland_update_master_bank)(void);


/***********************************************************************

   ATAXX

************************************************************************/

extern int ataxx_vram_port_r(int offset, int num);
extern void ataxx_vram_port_w(int offset, int data, int num);
extern void ataxx_mvram_port_w(int offset, int data);
extern void ataxx_svram_port_w(int offset, int data);
extern int  ataxx_mvram_port_r(int offset);
extern int  ataxx_svram_port_r(int offset);

/***********************************************************************

   BATTERY BACKED RAM

   Looks like it may be accessed in different ways by various games.

   (0xa000-0xafff)

************************************************************************/

#define leland_battery_ram_size 0x4000
static unsigned char leland_battery_ram[leland_battery_ram_size];


void leland_battery_w(int offset, int data)
{
	leland_battery_ram[offset]=data;
}

static void leland_hisave (void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
	if (f)
	{
		osd_fwrite (f, leland_battery_ram, leland_battery_ram_size);
		osd_fclose (f);
	}
}

static int leland_hiload (void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f, leland_battery_ram, leland_battery_ram_size);
		osd_fclose (f);
	}
	else
	{
		memset(leland_battery_ram, 0, leland_battery_ram_size);
	}

	return 1;
}

#ifdef MAME_DEBUG
void leland_debug_dump_driver(void)
{
	if (osd_key_pressed(OSD_KEY_F))
	{
		FILE *fp=fopen("MASTER.DMP", "w+b");
		if (fp)
		{
			unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
			fwrite(RAM, 0x10000, 1, fp);
			fclose(fp);
		}
		fp=fopen("SLAVE.DMP", "w+b");
        if (fp)
		{
			unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
			fwrite(RAM, 0x10000, 1, fp);
			fclose(fp);
		}
		fp=fopen("SOUND.DMP", "w+b");
        if (fp)
		{
			unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[2].memory_region];
			int size=Machine->memory_region_length[Machine->drv->cpu[2].memory_region];
			if (size != 1)
			{
				fwrite(RAM, size, 1, fp);
			}
			fclose(fp);
		}
        fp=fopen("BATTERY.DMP", "w+b");
        if (fp)
		{
            fwrite(leland_battery_ram, leland_battery_ram_size, 1, fp);
			fclose(fp);
		}

	}
}
#else
#define leland_debug_dump_driver()
#endif

#define MACHINE_DRIVER(DRV, MRP, MWP, MR, MW, INITMAC, GFXD, VRF, SLR, SLW)\
static struct MachineDriver DRV =    \
{                                                \
		{                                          \
				{                                   \
						CPU_Z80,        /* Master game processor */ \
						6000000,        /* 6.000 Mhz  */           \
						0,                                         \
						MR,MW,            \
						MRP,MWP,                                   \
						leland_master_interrupt,16                 \
				},                                                 \
				{                                                  \
						CPU_Z80, /* Slave graphics processor*/     \
						6000000, /* 6.000 Mhz */                   \
						2,       /* memory region #2 */            \
						SLR,SLW,              \
						slave_readport,slave_writeport,            \
						leland_slave_interrupt,1                   \
				},                                                 \
				{                                                  \
						CPU_I86,         /* Sound processor */     \
						16000000,        /* 16 Mhz  */             \
						3,                                         \
						leland_i86_readmem,leland_i86_writemem,\
						leland_i86_readport,leland_i86_writeport, \
						leland_i86_interrupt,1,                    \
						0,0,&leland_addrmask                    \
				},                                                 \
		},                                                         \
		60, 2000,2,                                                \
		INITMAC,                                       \
		0x28*8, 0x20*8, { 0*8, 0x28*8-1, 0*8, 0x1e*8-1 },              \
		GFXD,                                             \
		1024,1024,                                                 \
		0,                                                         \
		VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,                \
		0,                                                         \
		leland_vh_start,leland_vh_stop,VRF,    \
		0,0,0,0,\
		{   \
			{SOUND_AY8910, &ay8910_interface }, \
			{SOUND_DAC,    &dac_interface }     \
		}                 \
}

#define MACHINE_DRIVER_NO_SOUND(DRV, MRP, MWP, MR, MW, INITMAC, GFXD, VRF, SLR, SLW)\
static struct MachineDriver DRV =    \
{                                                \
		{                                          \
				{                                   \
						CPU_Z80,        /* Master game processor */ \
						6000000,        /* 6.000 Mhz  */           \
						0,                                         \
						MR,MW,            \
						MRP,MWP,                                   \
						leland_master_interrupt,16                 \
				},                                                 \
				{                                                  \
						CPU_Z80, /* Slave graphics processor*/     \
						6000000, /* 6.000 Mhz */                   \
						2,       /* memory region #2 */            \
						SLR, SLW,               \
						slave_readport,slave_writeport,            \
						leland_slave_interrupt,1                   \
				},                                                 \
		},                                                         \
		60, 2000,2,                                                \
		INITMAC,                                       \
		0x28*8, 0x20*8, { 0*8, 0x28*8-1, 0*8, 0x1e*8-1 },              \
		GFXD,                                             \
		1024,1024,                                                 \
		0,                                                         \
		VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,                \
		0,                                                         \
		leland_vh_start,leland_vh_stop,VRF,    \
		0,0,0,0,\
		{   \
			{SOUND_AY8910, &ay8910_interface }, \
			{SOUND_DAC,    &dac_interface }     \
		}                 \
}


/*
   2 AY8910 chips - Actually, one of these is an 8912
   (8910 with only 1 output port)

   Port A of both chips are connected to a banking control
   register.
*/

static struct AY8910interface ay8910_interface =
{
	2,
	10000000/6, /* 1.666 MHz */
	{ 0x20ff, 0x20ff },
	{ leland_sound_port_r, leland_sound_port_r },
	{ 0 },
	{ leland_sound_port_w, leland_sound_port_w },
	{ 0 }
};

/*
There are:
 2x  8 bit DACs (connected to video board)
 6x  8 bit DACs (on I/O daughter board) Ataxx uses 3x8bit DACs
 1x 10 bit DAC  (on I/O daughter board)
*/

static struct DACinterface dac_interface =
{
	MAX_DAC,  /* 8 chips */
	{255,255,255,255,},    /* Volume */
};

/***********************************************************************

   SLAVE (16MHz 80186 MUSIC CPU)

   6 x  8 bit DAC
   1 x 10 bit DAC

************************************************************************/

static int leland_sound_comm;       /* Port 0xf2 */
static int leland_sound_cmd;        /* Port 0xf4 */

static int leland_i86_sound_comm;       /* Port 0xf2 */


void leland_sound_comm_w(int offset, int data)
{
#ifdef NOISY_CPU
	if (errorlog)
	{
			fprintf(errorlog, "CPU#%d Sound COMM_W = %02x\n ", cpu_getactivecpu(), data);
	}
#endif
	leland_sound_comm=data;
}

void leland_i86_sound_comm_w(int offset, int data)
{
#ifdef NOISY_CPU
	if (errorlog)
	{
			fprintf(errorlog, "CPU#%d I86 Sound COMM_W = %02x\n ", cpu_getactivecpu(), data);
	}
#endif
	leland_sound_comm=data;
}

int leland_i86_sound_comm_r(int offset)
{
#ifdef NOISY_CPU
	if (errorlog)
	{
		fprintf(errorlog, "CPU#%d Sound COMM_R = %02x\n ", cpu_getactivecpu(), leland_sound_comm);
	}
#endif
	return leland_sound_comm;
}


int leland_sound_comm_r(int offset)
{
#ifdef NOISY_CPU
	if (errorlog)
	{
		fprintf(errorlog, "CPU#%d Sound COMM_R = %02x\n ", cpu_getactivecpu(), leland_i86_sound_comm);
	}
#endif
	return leland_sound_comm;
}

void leland_sound_cmd_w(int offset, int data)
{
		leland_sound_cmd=data;
}

int leland_sound_cmd_r(int offset)
{
		return leland_sound_cmd;
}

void leland_sound_cpu_control_w(int data)
{
	int intnum;
	/*
	Sound control = data & 0xf7
	===========================
		0x80 = CPU Reset
		0x40 = NMI
		0x20 = INT0
		0x10 = TEST
		0x08 = INT1
	*/

	cpu_set_reset_line(2, data&0x80  ? CLEAR_LINE : HOLD_LINE);
//    cpu_set_nmi_line(2,   data&0x40  ? CLEAR_LINE : HOLD_LINE);
//    cpu_set_test_line(2,  data&0x20  ? CLEAR_LINE : HOLD_LINE);

	/* No idea about the int 0 and int 1 pins (do they give int number?) */
	intnum=(data&0x20)>>6;  /* Int 0 */
	intnum|=(data&0x08)>>2; /* Int 1 */

#ifdef NOISY_CPU
	if (errorlog)
	{
		 fprintf(errorlog, "PC=%04x Sound CPU intnum=%02x\n",
			cpu_get_pc(), intnum);
	}
#endif
}

/***********************************************************************

   DAC

	There are 6x8 Bit DACs. 2 bytes for each
	(high=volume, low=value).

************************************************************************/

void leland_dac_w(int offset, int data)
{
	int dacnum=(offset/2)+1;
	if (dacnum >= MAX_DAC)
	{
		/* Exceeded DAC */
		return;
	}
	if (offset & 0x01)
	{
		/* Writing to volume register */
		DAC_set_volume(dacnum, data, 255);
	}
	else
	{
		/* Writing to DAC */
		DAC_data_w(dacnum, data);
	}
}

int leland_i86_interrupt(void)
{
	return ignore_interrupt();
}

static struct IOReadPort leland_i86_readport[] =
{
/*    { 0x0000, 0x0000, leland_sound_cmd_r },
	{ 0x0080, 0x0080, leland_sound_comm_r },*/
	{ -1 }  /* end of table */
};

static struct IOWritePort leland_i86_writeport[] =
{
	{ 0x0000, 0x000c, leland_dac_w },       /* 6x8 bit DACs */
	/*
	{ 0x0081, 0x0081, leland_sound_comm_w },
	*/
	{ -1 }  /* end of table */
};



int leland_i86_ram_r(int offset)
{
	/*
	Not very tidy, but it works for now...
	*/
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[2].memory_region];
	return RAM[0x1c000+offset];
}

void leland_i86_ram_w(int offset, int data)
{
	/*
	Not very tidy, but it works for now...
	*/
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[2].memory_region];
	RAM[0x1c000+offset]=data;
}



static struct MemoryReadAddress leland_i86_readmem[] =
{
	{ 0x020080, 0x020080, leland_i86_sound_comm_r },
	{ 0x000000, 0x00ffff, leland_i86_ram_r },   /* vectors live here */
	{ 0x01c000, 0x01ffff, MRA_RAM },
	{ 0x020000, 0x0203ff, MRA_RAM },    /* Don't know */
	{ 0x080000, 0x0fffff, MRA_ROM },    /* Program ROM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress leland_i86_writemem[] =
{
	{ 0x020080, 0x020080, leland_i86_sound_comm_w },
	{ 0x000000, 0x00ffff, leland_i86_ram_w },   /* vectors live here */
	{ 0x01c000, 0x01ffff, MWA_RAM }, /* 64K RAM (also appears at 0x00000) */
	{ 0x020000, 0x0203ff, MWA_RAM }, /* Unknown */
	{ 0x080000, 0x0fffff, MWA_ROM }, /* Program ROM */
	{ -1 }  /* end of table */
};

/***********************************************************************

   MASTER (Z80 MAIN CPU)

   There appear to be 16 interrupts per frame. The interrupts
   are generated by the graphics hardware (probably every 16 scan
   lines).

   There is a video refresh flag in one of the input ports (0x02 of
   port 0x51 in Pigout). When this is set (clear?), the game resets
   the interrupt counter and performs some extra processing.

   The interrupt routine increments a counter every time an interrupt
   is triggered. The counter wraps around at 16. When it reaches 0,
   it does the same as if the refresh flag was set. I don't think that
   it ever reaches that far (it doesn't make sense to do all that expensive
   processing twice in quick succession).

************************************************************************/

int leland_raster_count;

int leland_master_interrupt(void)
{
	leland_raster_count++;
	leland_raster_count&=0x0f;

	/* Debug dumping */
	leland_debug_dump_driver();

	/* Generate an interrupt */
	return interrupt();
}

void leland_slave_cmd_w(int offset, int data)
{
	cpu_set_reset_line(1, data&0x01  ? CLEAR_LINE : HOLD_LINE);
	/* 0x02=colour write */
	cpu_set_nmi_line(1,    data&0x04 ? CLEAR_LINE : HOLD_LINE);
	cpu_set_irq_line(1, 0, data&0x08 ? CLEAR_LINE : HOLD_LINE);

	/*
	0x10, 0x20, 0x40 = Unknown (connected to bit 0 of control read
	halt detect bit for slave CPU)
	*/
	leland_slave_cmd=data;
}

INLINE int leland_slavebit_r(int bit)
{
	int ret=input_port_0_r(0);
	int halt=0;
	if (cpu_get_halt_line(1))
	{
		halt=bit;  /* CPU halted */
	}
	ret=(ret&(~bit))|halt;
	return ret;
}

int leland_slavebit0_r(int offset)
{
	return leland_slavebit_r(0x01);
}

int leland_slavebit1_r(int offset)
{
	return leland_slavebit_r(0x02);
}

static int leland_current_analog;

void leland_analog_w(int offset, int data)
{
	leland_current_analog=data&0x0f;
/*
	if (errorlog)
	{
		fprintf(errorlog, "Analog joystick %02x (device#%d)\n", data, leland_current_analog);
	}
*/
}

int leland_analog_r(int offset)
{
	return readinputport(leland_current_analog+4+offset);
}


int leland_analog_control_r(int offset)
{
	/*
	Game reads this to ensure that the analog/digital converter
	is ready. Return 0 in the high bit
	*/
	return 0x00;
}


/***********************************************************************

   SLAVE (Z80 GFX CPU)

   A big graphics blitter.

************************************************************************/

int leland_slave_interrupt(void)
{
	/* Slave's interrupts are driven by the master */
	return ignore_interrupt();
}

int leland_slave_cmd_r(int offset)
{
	return leland_slave_cmd;
}

void leland_slave_banksw_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	int bankaddress;
	bankaddress=0x10000+0xc000*(data&0x0f);
	cpu_setbank(7, &RAM[bankaddress]);
	cpu_setbank(8, &RAM[bankaddress+0x2000]);
#ifdef NOISY_CPU
	if (errorlog)
	{
		fprintf(errorlog, "CPU #1 %04x BANK SWITCH %02x\n",
			cpu_get_pc(), data);
	}
#endif
}

void leland_slave_large_banksw_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	int bankaddress;
    bankaddress=0x10000+0x8000*(data&0x0f);
    cpu_setbank(7, &RAM[0x2000]);
    cpu_setbank(8, &RAM[bankaddress]);

#ifdef NOISY_CPU
	if (errorlog)
	{
		fprintf(errorlog, "CPU #1 %04x BIG BANK SWITCH %02x\n",
			cpu_get_pc(), data);
	}
#endif
}


static struct MemoryReadAddress slave_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },        /* Resident program ROM */
	{ 0x2000, 0x3fff, MRA_BANK7 },      /* Paged graphics ROM */
	{ 0x4000, 0xdfff, MRA_BANK8 },      /* Paged graphics ROM */
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xf400, leland_palette_ram_r },
	{ 0xf802, 0xf802, leland_slave_cmd_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress slave_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0xc000, 0xc000, leland_slave_large_banksw_w }, /* Pigout */
	{ 0x2000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xf400, leland_palette_ram_w },
	{ 0xf800, 0xf801, leland_slave_video_addr_w },
	{ 0xf803, 0xf803, leland_slave_banksw_w },
	{ 0xfffc, 0xfffd, leland_slave_video_addr_w }, /* Ataxx */
	{ -1 }  /* end of table */
};

static struct IOReadPort slave_readport[] =
{
	{ 0x00, 0x1f, leland_svram_port_r }, /* Video ports (some games) */
	{ 0x40, 0x5f, leland_svram_port_r }, /* Video ports (other games) */
	{ 0x60, 0x77, ataxx_svram_port_r },  /* Ataxx ports */
	{ -1 }  /* end of table */
};

static struct IOWritePort slave_writeport[] =
{
	{ 0x00, 0x1f, leland_svram_port_w }, /* Video ports (some games) */
	{ 0x40, 0x5f, leland_svram_port_w }, /* Video ports (other games) */
	{ 0x60, 0x77, ataxx_svram_port_w },  /* Ataxx ports */
	{ -1 }  /* end of table */
};

static struct GfxLayout bklayout =
{
	8,8,    /* 8*8 characters */
	4096, /* ??? characters */
	3,      /* 3 bits per pixel */
	{ 0*0x08000*8,1*0x08000*8,2*0x08000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8    /* every char takes 8*8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/*   start    pointer       colour start   number of colours */
	{ 1, 0x000000, &bklayout,          0, 32 },    /* 32 */
	{ -1 } /* end of array */
};


int leland_rearrange_bank(unsigned char *pData, int count)
{
	unsigned char *p;
	int i;
	p=malloc(0x8000);
	if (p)
	{
	   for (i=0; i<count; i++)
	   {
			memcpy(p, pData+0x2000, 0x6000);
			memcpy(p+0x6000, pData, 0x2000);
			memcpy(pData, p, 0x8000);
			pData+=0x8000;
		}
		free(p);
		return 1;
	}
	return 0;
}

/* 80186 1 MB address mask */
int leland_addrmask=0x0fffff;



void leland_init_machine(void)
{
	   leland_update_master_bank=NULL;     /* No custom master banking */
}








/***************************************************************************

  Game driver(s)

***************************************************************************/

/***************************************************************************

  Strike Zone

***************************************************************************/

INPUT_PORTS_START( input_ports_strkzone )
	PORT_START      /* 0x41 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", OSD_KEY_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* 0x40 */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK ) /* Double play */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 | IPF_PLAYER1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* 0x50 */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1  )

	PORT_START      /* Analog joystick 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 0, 0, 255 )
	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 0, 0, 255 )
	PORT_START      /* Analog joystick 2 */
	PORT_START
	PORT_START      /* Analog joystick 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER2, 100, 0, 0, 255 )
	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER2, 100, 0, 0, 255 )
INPUT_PORTS_END

int leland_bank;

void strkzone_update_bank(void)
{
	int bankaddress, batteryaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	if (leland_bank & 0x80 )
	{
		if (leland_bank & 0x40)
		{
			bankaddress=0x30000;
		}
		else
		{
			bankaddress=0x28000;
		}
	}
	else
	{
		if (leland_sound_port_r(0) & 0x04)
		{
			bankaddress=0x1c000;
		}
		else
		{
			bankaddress=0x10000;
		}
	}
	cpu_setbank(1, &RAM[bankaddress]);

	if (1) // || leland_sound_port_r(0) & 0x20 && !(leland_bank & 0x80))
	{
		 cpu_setbank(2, &RAM[bankaddress+(0xa000-0x2000)]);
	}
	else
	{
#ifdef NOISY_CPU
		if (errorlog)
		{
			fprintf(errorlog, "BATTERY RAM\n");
		}
#endif
		cpu_setbank(2, leland_battery_ram);
	}

/*
	{
		char baf[80];
		sprintf(baf,"Bank Address=%06x\n", bankaddress);
		usrintf_showmessage(baf);
	}
*/
}

void strkzone_banksw_w(int offset, int data)
{
	leland_bank=data;
	strkzone_update_bank();
}

static struct MemoryReadAddress strkzone_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0xa000, 0xdfff, MRA_BANK2 },
	{ 0x2000, 0xdfff, MRA_BANK1 },
	{ 0xf000, 0xf400, leland_palette_ram_r },
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress strkzone_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM   },
	{ 0xa000, 0xdfff, MWA_BANK2 }, /* Battery RAM */
	{ 0x2000, 0xdfff, MWA_BANK1 },
	{ 0xf000, 0xf400, leland_palette_ram_w, &leland_palette_ram, &leland_palette_ram_size },
	{ 0xf800, 0xf801, leland_master_video_addr_w },
	{ 0xe000, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};

int strkzone_joystick_r(int offset)
{
	return 0x34;
}

static struct IOReadPort strkzone_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */
	{ 0x40, 0x40, input_port_1_r },
	{ 0x41, 0x41, leland_slavebit0_r },
	{ 0x43, 0x43, AY8910_read_port_0_r },
	{ 0x50, 0x50, input_port_3_r },
	{ 0x51, 0x51, input_port_2_r },

	{ 0x80, 0x9f, leland_mvram_port_r }, /* Video RAM ports (double play) */

	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xff, 0xff, leland_analog_control_r },
	{ -1 }  /* end of table */
};

void leland_unknown_w(int offset, int data)
{
	/* Unknown port, just toggles a cople of high bits */
}

static struct IOWritePort strkzone_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0x49, 0x49, leland_slave_cmd_w },
	{ 0x4a, 0x4a, AY8910_control_port_0_w },
	{ 0x4b, 0x4b, AY8910_write_port_0_w },
	{ 0x4c, 0x4c, leland_bk_xlow_w },
	{ 0x4d, 0x4d, leland_bk_xhigh_w },
	{ 0x4e, 0x4e, leland_bk_ylow_w },
	{ 0x4f, 0x4f, leland_bk_yhigh_w },

	{ 0x80, 0x9f, leland_mvram_port_w }, /* Video RAM ports (double play) */

	{ 0xfd, 0xfd, leland_analog_w },
	{ 0xfe, 0xfe, strkzone_banksw_w },
	{ 0xff, 0xff, leland_unknown_w },
	{ -1 }  /* end of table */
};

/*
Rearrange the banks into a more managable
format
*/

void leland_rearrange_banks(int cpu)
{
	int i;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];
	unsigned char *p=&RAM[0x10000];
	char *buffer=malloc(0x18000);
	if (buffer)
	{
		memcpy(buffer, p, 0x18000);
		for (i=0; i<6; i++)
		{
			memcpy(p,         buffer+0x4000*i,         0x2000);
			memcpy(p+0x0c000, buffer+0x4000*i+0x02000, 0x2000);
			p+=0x2000;
		}
		free(buffer);
	}
	else
	{
		/* should really abort */
	}
}

void strkzone_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	leland_update_master_bank=strkzone_update_bank;
	leland_rearrange_banks(0);
	/* Chip 4 is banked in */
	memcpy(&RAM[0x30000], &RAM[0x28000], 0x6000);
	memcpy(&RAM[0x34000], &RAM[0x2e000], 0x2000);
	leland_rearrange_banks(1);
}

MACHINE_DRIVER_NO_SOUND(strkzone_machine, strkzone_readport, strkzone_writeport,
	strkzone_readmem, strkzone_writemem, strkzone_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( strkzone_rom )
	ROM_REGION(0x03c000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("strkzone.101",   0x00000, 0x04000, 0x8d83a611 ) /* 0x0000 */

	ROM_LOAD("strkzone.102",   0x10000, 0x04000, 0x3859e67d ) /* 0x2000 (2 pages) */
	ROM_LOAD("strkzone.103",   0x14000, 0x04000, 0xcdd83bfb ) /* 0x4000 (2 pages)*/
	ROM_LOAD("strkzone.104",   0x18000, 0x04000, 0xbe280212 ) /* 0x6000 (2 pages)*/
	ROM_LOAD("strkzone.105",   0x1c000, 0x04000, 0xafb63390 ) /* 0x8000 (2 pages)*/
	ROM_LOAD("strkzone.106",   0x20000, 0x04000, 0xe853b9f6 ) /* 0xA000 (2 pages)*/
	ROM_LOAD("strkzone.107",   0x24000, 0x04000, 0x1b4b6c2d ) /* 0xC000 (2 pages)*/
	/* Extra banks (referred to as the "top" board). Probably an add-on */
	ROM_LOAD("strkzone.U2T",   0x28000, 0x02000, 0x8e0af06f ) /* 2000-3fff (1 page) */
	ROM_LOAD("strkzone.U3T",   0x2a000, 0x02000, 0x909d35f3 ) /* 4000-5fff (1 page) */
	ROM_LOAD("strkzone.U4T",   0x2c000, 0x04000, 0x9b1e72e9 ) /* 6000-7fff (2 pages) */
	/* Remember to leave 0xc000 bytes here for paging */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("strkzone.U93", 0x00000, 0x04000, 0x8ccb1404 )
	ROM_LOAD("strkzone.U94", 0x08000, 0x04000, 0x9941a55b )
	ROM_LOAD("strkzone.U95", 0x10000, 0x04000, 0xb68baf47 )

	ROM_REGION(0x28000)     /* Z80 slave CPU */
	ROM_LOAD("strkzone.U3",  0x00000, 0x02000, 0x40258fbe ) /* 0000-1fff */
	ROM_LOAD("strkzone.U4",  0x10000, 0x04000, 0xdf7f2604 ) /* 2000-3fff */
	ROM_LOAD("strkzone.U5",  0x14000, 0x04000, 0x37885206 ) /* 4000-3fff */
	ROM_LOAD("strkzone.U6",  0x18000, 0x04000, 0x6892dc4f ) /* 6000-3fff */
	ROM_LOAD("strkzone.U7",  0x1c000, 0x04000, 0x6ac8f87c ) /* 8000-3fff */
	ROM_LOAD("strkzone.U8",  0x20000, 0x04000, 0x4b6d3725 ) /* a000-3fff */
	ROM_LOAD("strkzone.U9",  0x24000, 0x04000, 0xab3aac49 ) /* c000-3fff */

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* U70 = Empty */
	ROM_LOAD( "strkzone.U92",  0x04000, 0x4000, 0x2508a9ad )
	ROM_LOAD( "strkzone.U69",  0x08000, 0x4000, 0xb123a28e )
	/* U91 = Empty */
	ROM_LOAD( "strkzone.U68",  0x10000, 0x4000, 0xa1a51383 )
	ROM_LOAD( "strkzone.U90",  0x14000, 0x4000, 0xef01d997 )
	ROM_LOAD( "strkzone.U67",  0x18000, 0x4000, 0x976334e6 )
	/* 89 = Empty */
ROM_END

struct GameDriver strkzone_driver =
{
	__FILE__,
	0,
	"strkzone",
	"Strike Zone",
	"1988",
	LELAND,
	"Paul Leaman",
	0,
	&strkzone_machine,
	0,

	strkzone_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_strkzone,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

/***************************************************************************

  DblPlay

***************************************************************************/

ROM_START( dblplay_rom )
	ROM_REGION(0x03c000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("15018-01.101",   0x00000, 0x02000, 0x17b6af29 )
	ROM_LOAD("15019-01.102",   0x10000, 0x04000, 0x9fc8205e ) /* 0x2000 */
	ROM_LOAD("15020-01.103",   0x14000, 0x04000, 0x4edcc091 ) /* 0x4000 */
	ROM_LOAD("15021-01.104",   0x18000, 0x04000, 0xa0eba1c7 ) /* 0x6000 */
	ROM_LOAD("15022-01.105",   0x1c000, 0x04000, 0x7bbfe0b7 ) /* 0x8000 */
	ROM_LOAD("15023-01.106",   0x20000, 0x04000, 0xbbedae34 ) /* 0xA000 */
	ROM_LOAD("15024-01.107",   0x24000, 0x04000, 0x02afcf52 ) /* 0xC000 */
	/* Extra banks (referred to as the "top" board). Probably an add-on */
	ROM_LOAD("15025-01.U2T",   0x28000, 0x02000, 0x1c959895 ) /* 2000-3fff (1 page) */
	ROM_LOAD("15026-01.U3T",   0x2a000, 0x02000, 0xed5196d6 ) /* 4000-5fff (1 page) */
	ROM_LOAD("15027-01.U4T",   0x2c000, 0x04000, 0x9b1e72e9 ) /* 6000-7fff (2 pages) */
	/* Remember to leave 0xc000 bytes here for paging */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("15015-01.U93", 0x00000, 0x04000, 0x8ccb1404 )
	ROM_LOAD("15016-01.U94", 0x08000, 0x04000, 0x9941a55b )
	ROM_LOAD("15017-01.U95", 0x10000, 0x04000, 0xb68baf47 )

	ROM_REGION(0x26000)     /* Z80 slave CPU */
	ROM_LOAD("15000-01.U03",  0x00000, 0x02000, 0x208a920a )
	ROM_LOAD("15001-01.U04",  0x10000, 0x04000, 0x751c40d6 )
	ROM_LOAD("14402-01.U05",  0x14000, 0x04000, 0x5ffaec36 )
	ROM_LOAD("14403-01.U06",  0x18000, 0x04000, 0x48d6d9d3 )
	ROM_LOAD("15004-01.U07",  0x1c000, 0x04000, 0x6a7acebc )
	ROM_LOAD("15005-01.U08",  0x20000, 0x04000, 0x69d487c9 )
	ROM_LOAD("15006-01.U09",  0x22000, 0x04000, 0xab3aac49 )

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* U70 = Empty */
	ROM_LOAD( "15014-01.U92",  0x04000, 0x4000, 0x2508a9ad )
	ROM_LOAD( "15009-01.U69",  0x08000, 0x4000, 0xb123a28e )
	/* U91 = Empty */
	ROM_LOAD( "15008-01.U68",  0x10000, 0x4000, 0xa1a51383 )
	/* U90 = Empty */
	ROM_LOAD( "15007-01.U67",  0x18000, 0x4000, 0x976334e6 )
	/* 89 = Empty */
ROM_END

struct GameDriver dblplay_driver =
{
	__FILE__,
	0,
	"dblplay",
	"Double Play",
	"1988",
	LELAND,
	"Paul Leaman",
	0,
	&strkzone_machine,
	0,

	dblplay_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_strkzone,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

/***************************************************************************

	World Series

***************************************************************************/

static struct IOReadPort wseries_readport[] =
{
	{ 0x40, 0x5f, leland_mvram_port_r }, /* Video RAM ports */
	{ 0x80, 0x80, input_port_1_r },
	{ 0x81, 0x81, leland_slavebit0_r },
	{ 0x83, 0x83, AY8910_read_port_0_r },
	{ 0x90, 0x90, input_port_3_r },
	{ 0x91, 0x91, input_port_2_r },

	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xff, 0xff, leland_analog_control_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort wseries_writeport[] =
{
	{ 0x40, 0x5f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0x89, 0x89, leland_slave_cmd_w },
	{ 0x8a, 0x8a, AY8910_control_port_0_w },
	{ 0x8b, 0x8b, AY8910_write_port_0_w },
	{ 0x8c, 0x8c, leland_bk_xlow_w },
	{ 0x8d, 0x8d, leland_bk_xhigh_w },
	{ 0x8e, 0x8e, leland_bk_ylow_w },
	{ 0x8f, 0x8f, leland_bk_yhigh_w },

	{ 0xfd, 0xfd, leland_analog_w },
	{ 0xfe, 0xfe, strkzone_banksw_w },
	{ 0xff, 0xff, leland_unknown_w },
	{ -1 }  /* end of table */
};


MACHINE_DRIVER_NO_SOUND(wseries_machine, wseries_readport, wseries_writeport,
	strkzone_readmem, strkzone_writemem, strkzone_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( wseries_rom )
	ROM_REGION(0x032000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("13409-01.101",   0x00000, 0x02000, 0xb5eccf5c )
	ROM_LOAD("13410-01.102",   0x10000, 0x04000, 0xdd1ec091 ) /* 0x2000 */
	ROM_LOAD("13411-01.103",   0x14000, 0x04000, 0xec867a0e ) /* 0x4000 */
	ROM_LOAD("13412-01.104",   0x18000, 0x04000, 0x2977956d ) /* 0x6000 */
	ROM_LOAD("13413-01.105",   0x1c000, 0x04000, 0x569468a6 ) /* 0x8000 */
	ROM_LOAD("13414-01.106",   0x20000, 0x04000, 0xb178632d ) /* 0xA000 */
	ROM_LOAD("13415-01.107",   0x24000, 0x04000, 0x20b92eff ) /* 0xC000 */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("13401-00.U93", 0x00000, 0x04000, 0x4ea3e641 )
	ROM_LOAD("13402-00.U94", 0x08000, 0x04000, 0x71a8a56c )
	ROM_LOAD("13403-00.U95", 0x10000, 0x04000, 0x8077ae25 )

	ROM_REGION(0x28000)     /* Z80 slave CPU */
	ROM_LOAD("13416-00.U3",  0x00000, 0x02000, 0x37c960cf )
	ROM_LOAD("13417-00.U4",  0x10000, 0x04000, 0x97f044b5 )
	ROM_LOAD("13418-00.U5",  0x14000, 0x04000, 0x0931cfc0 )
	ROM_LOAD("13419-00.U6",  0x18000, 0x04000, 0xa7962b5a )
	ROM_LOAD("13420-00.U7",  0x1c000, 0x04000, 0x3c275262 )
	ROM_LOAD("13421-00.U8",  0x20000, 0x04000, 0x86f57c80 )
	ROM_LOAD("13422-00.U9",  0x24000, 0x04000, 0x222e8405 )

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* U70 = Empty */
	ROM_LOAD( "13404-00.U92",  0x04000, 0x4000, 0x22da40aa )
	ROM_LOAD( "13405-00.U69",  0x08000, 0x4000, 0x6f65b313 )
	/* U91 = Empty */
	ROM_LOAD( "13406-00.U68",  0x10000, 0x4000, 0x00000000 )   /* ??? BAD */
	ROM_LOAD( "13407-00.U90",  0x14000, 0x4000, 0xe46ca57f )
	ROM_LOAD( "13408-00.U67",  0x18000, 0x4000, 0xbe637305 )
	/* 89 = Empty */
ROM_END

struct GameDriver wseries_driver =
{
	__FILE__,
	0,
	"wseries",
	"World Series Baseball",
	"1988",
	LELAND,
	"Paul Leaman",
	0,
	&wseries_machine,
	0,

	wseries_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_strkzone,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

/***************************************************************************

  Baseball 2

***************************************************************************/

static struct IOReadPort basebal2_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0xc0, 0xc0, input_port_1_r },
	{ 0xc1, 0xc1, leland_slavebit0_r },
	{ 0xc3, 0xc3, AY8910_read_port_0_r },
	{ 0xd0, 0xd0, input_port_3_r },
	{ 0xd1, 0xd1, input_port_2_r },
	{ 0xf2, 0xf2, leland_sound_comm_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xff, 0xff, leland_analog_control_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort basebal2_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0xc9, 0xc9, leland_slave_cmd_w },
	{ 0xca, 0xca, AY8910_control_port_0_w },
	{ 0xcb, 0xcb, AY8910_write_port_0_w },

	{ 0xcc, 0xcc, leland_bk_xlow_w },
	{ 0xcd, 0xcd, leland_bk_xhigh_w },
	{ 0xce, 0xce, leland_bk_ylow_w },
	{ 0xcf, 0xcf, leland_bk_yhigh_w },

	{ 0xf2, 0xf2, leland_sound_comm_w },
	{ 0xf4, 0xf4, leland_sound_cmd_w },

	{ 0xfe, 0xfe, strkzone_banksw_w },
	{ 0xfd, 0xfd, leland_analog_w },
	{ 0xff, 0xff, leland_unknown_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER_NO_SOUND(basebal2_machine, basebal2_readport, basebal2_writeport,
	strkzone_readmem, strkzone_writemem, strkzone_init_machine, gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( basebal2_rom )
	ROM_REGION(0x032000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("14115-00.101",   0x00000, 0x02000, 0x05231fee )
	ROM_LOAD("14116-00.102",   0x10000, 0x04000, 0xe1482ea3 ) /* 0x2000 */
	ROM_LOAD("14117-01.103",   0x14000, 0x04000, 0x677181dd ) /* 0x4000 */
	ROM_LOAD("14118-01.104",   0x18000, 0x04000, 0x5f570264 ) /* 0x6000 */
	ROM_LOAD("14119-01.105",   0x1c000, 0x04000, 0x90822145 ) /* 0x8000 */
	ROM_LOAD("14120-00.106",   0x20000, 0x04000, 0x4d2b7217 ) /* 0xA000 */
	ROM_LOAD("14121-01.107",   0x24000, 0x04000, 0xb987b97c ) /* 0xC000 */

	/* Extra banks (referred to as the "top" board). Probably an add-on */
	ROM_LOAD("14122-01.U2T",   0x28000, 0x02000, 0xa89882d8 ) /* 2000-3fff (1 page) */
	ROM_LOAD("14123-01.U3T",   0x2a000, 0x02000, 0xf9c51e5a ) /* 4000-5fff (1 page) */
							/* 6000-7fff (2 pages) - EMPTY */
	/* Remember to leave 0xc000 bytes here for paging */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("14112-00.U93", 0x00000, 0x04000, 0x8ccb1404 )
	ROM_LOAD("14113-00.U94", 0x08000, 0x04000, 0x9941a55b )
	ROM_LOAD("14114-00.U95", 0x10000, 0x04000, 0xb68baf47 )

	ROM_REGION(0x28000)     /* Z80 slave CPU */
	ROM_LOAD("14100-01.U3",  0x00000, 0x02000, 0x1dffbdaf )
	ROM_LOAD("14101-01.U4",  0x10000, 0x04000, 0xc585529c )
	ROM_LOAD("14102-01.U5",  0x14000, 0x04000, 0xace3f918 )
	ROM_LOAD("14103-01.U6",  0x18000, 0x04000, 0xcd41cf7a )
	ROM_LOAD("14104-01.U7",  0x1c000, 0x04000, 0x9b169e78 )
	ROM_LOAD("14105-01.U8",  0x20000, 0x04000, 0xec596b43 )
	ROM_LOAD("14106-01.U9",  0x24000, 0x04000, 0xb9656baa )

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* U70 = Empty */
	ROM_LOAD( "14111-01.U92",  0x04000, 0x4000, 0x2508a9ad )
	ROM_LOAD( "14109-00.U69",  0x08000, 0x4000, 0xb123a28e )
	/* U91 = Empty */
	ROM_LOAD( "14108-01.U68",  0x10000, 0x4000, 0xa1a51383 )
	ROM_LOAD( "14110-01.U90",  0x14000, 0x4000, 0xef01d997 )
	ROM_LOAD( "14107-00.U67",  0x18000, 0x4000, 0x976334e6 )
	/* 89 = Empty */
ROM_END

struct GameDriver basebal2_driver =
{
	__FILE__,
	0,
	"basebal2",
	"Baseball Season 2",
	"1988",
	LELAND,
	"Paul Leaman",
	0,
	&basebal2_machine,
	0,

	basebal2_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_strkzone,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

/***************************************************************************

  Pigout

***************************************************************************/


INPUT_PORTS_START( input_ports_pigout )
	PORT_START      /* GIN1 (0x41) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* GIN0 (0x40) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service", OSD_KEY_F2, IP_JOY_NONE )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START      /* GIN1 (0x50) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2  )

	PORT_START      /* GIN3 (0x7f) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3|IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1  )


INPUT_PORTS_END

void pigout_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	leland_init_machine();
	leland_rearrange_bank(&RAM[0x10000], 4);
}

void pigout_banksw_w(int offset, int data)
{
	int bank;
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	unsigned char *battery_bank=&RAM[0xa000];

	bank=data&0x07;

	if (bank<=1)
	{
		/* 0 = resident */
		bankaddress =0x2000;
		/* 1 = battery RAM */
		if (bank==1)
		{
			battery_bank=leland_battery_ram;
		}
	}
	else
	{
		bank-=2;
		bankaddress = 0x10000 + bank * 0x8000;
	}

	cpu_setbank(1,&RAM[bankaddress]);    /* 0x2000-0x9fff */
	cpu_setbank(3,battery_bank);            /* 0xa000-0xdfff */

	leland_sound_cpu_control_w(data);
}

static struct MemoryReadAddress master_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },      /* Resident ROM */
	{ 0x2000, 0x9fff, MRA_BANK1 },
	{ 0xa000, 0xdfff, MRA_BANK3 },  /* BATTERY RAM / ROM */
	{ 0xf000, 0xf400, leland_palette_ram_r },
	{ 0xe000, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress master_writemem[] =
{
	{ 0x0000, 0xdfff, MWA_NOP },
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xdfff, leland_battery_w },  /* BATTERY RAM */

	{ 0xf000, 0xf400, leland_palette_ram_w },
	{ 0xf800, 0xf801, leland_master_video_addr_w },
	{ 0xe000, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};


static struct IOReadPort pigout_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0x40, 0x40, input_port_1_r },
	{ 0x41, 0x41, leland_slavebit0_r },
	{ 0x43, 0x43, AY8910_read_port_0_r },
	{ 0x50, 0x50, input_port_3_r },
	{ 0x51, 0x51, input_port_2_r },

	{ 0xf2, 0xf2, leland_sound_comm_r },
	{ 0x7f, 0x7f, input_port_4_r },         /* Player 1 */
	{ -1 }  /* end of table */
};

static struct IOWritePort pigout_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */
	{ 0x80, 0x9f, leland_mvram_port_w }, /* track pack Video RAM ports */

	{ 0x40, 0x46, leland_mvram_port_w }, /* (stray) video RAM ports */
	{ 0x49, 0x49, leland_slave_cmd_w },
	{ 0x4a, 0x4a, AY8910_control_port_0_w },
	{ 0x4b, 0x4b, AY8910_write_port_0_w },
	{ 0x4c, 0x4c, leland_bk_xlow_w },
	{ 0x4d, 0x4d, leland_bk_xhigh_w },
	{ 0x4e, 0x4e, leland_bk_ylow_w },
	{ 0x4f, 0x4f, leland_bk_yhigh_w },

	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },

	{ 0xf0, 0xf0, pigout_banksw_w },
	{ 0xf2, 0xf2, leland_sound_comm_w },
	{ 0xf4, 0xf4, leland_sound_cmd_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER(pigout_machine, pigout_readport, pigout_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	pigout_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( pigout_rom )
	ROM_REGION(0x040000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("POUTU58T.BIN",  0x00000, 0x10000, 0x8fe4b683 ) /* CODE */
	ROM_LOAD("POUTU59T.BIN",  0x10000, 0x10000, 0xab907762 ) /* Banked code */
	ROM_LOAD("POUTU57T.BIN",  0x20000, 0x10000, 0xc22be0ff ) /* Banked code */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("POUTU93.BIN", 0x000000, 0x08000, 0xf102a04d ) /* Plane 3 */
	ROM_LOAD("POUTU94.BIN", 0x008000, 0x08000, 0xec63c015 ) /* Plane 2 */
	ROM_LOAD("POUTU95.BIN", 0x010000, 0x08000, 0xba6e797e ) /* Plane 1 */

	ROM_REGION(0x80000)     /* Z80 slave CPU */
	ROM_LOAD("POUTU3.BIN",   0x00000, 0x02000, 0xaf213cb7 )
	ROM_LOAD("POUTU2T.BIN",  0x10000, 0x10000, 0xb23164c6 )
	ROM_LOAD("POUTU3T.BIN",  0x20000, 0x10000, 0xd93f105f )
	ROM_LOAD("POUTU4T.BIN",  0x30000, 0x10000, 0xb7c47bfe )
	ROM_LOAD("POUTU5T.BIN",  0x40000, 0x10000, 0xd9b9dfbf )
	ROM_LOAD("POUTU6T.BIN",  0x50000, 0x10000, 0x728c7c1a )
	ROM_LOAD("POUTU7T.BIN",  0x60000, 0x10000, 0x393bd990 )
	ROM_LOAD("POUTU8T.BIN",  0x70000, 0x10000, 0xcb9ffaad )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("POUTU25T.BIN", 0x080000, 0x10000, 0x92cd2617 )
	ROM_LOAD_ODD ("POUTU13T.BIN", 0x080000, 0x10000, 0x9448c389 )
	ROM_LOAD_EVEN("POUTU26T.BIN", 0x0a0000, 0x10000, 0xab57de8f )
	ROM_LOAD_ODD ("POUTU14T.BIN", 0x0a0000, 0x10000, 0x30678e93 )
	ROM_LOAD_EVEN("POUTU27T.BIN", 0x0e0000, 0x10000, 0x37a8156e )
	ROM_LOAD_ODD ("POUTU15T.BIN", 0x0e0000, 0x10000, 0x1c60d58b )

	ROM_REGION(0x40000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	ROM_LOAD( "POUTU70.BIN",  0x00000, 0x4000, 0x7db4eaa1 )
	ROM_LOAD( "POUTU92.BIN",  0x04000, 0x4000, 0x20fa57bb )
	ROM_LOAD( "POUTU69.BIN",  0x08000, 0x4000, 0xa16886f3 )
	ROM_LOAD( "POUTU91.BIN",  0x0c000, 0x4000, 0x482a3581 )
	ROM_LOAD( "POUTU68.BIN",  0x10000, 0x4000, 0x7b62a3ed )
	ROM_LOAD( "POUTU90.BIN",  0x14000, 0x4000, 0x9615d710 )
	ROM_LOAD( "POUTU67.BIN",  0x18000, 0x4000, 0xaf85ce79 )
	ROM_LOAD( "POUTU89.BIN",  0x1c000, 0x4000, 0x6c874a05 )
ROM_END

struct GameDriver pigout_driver =
{
	__FILE__,
	0,
	"pigout",
	"pigout",
	"1990",
	LELAND,
	"Paul Leaman",
	0,
	&pigout_machine,
	0,

	pigout_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_pigout,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

ROM_START( pigoutj_rom )
	ROM_REGION(0x040000)     /* 64k for code + banked ROMs images */
	ROM_LOAD( "03-29020.01", 0x00000, 0x10000, 0x6c815982 ) /* CODE */
	ROM_LOAD( "03-29021.01", 0x10000, 0x10000, 0x9de7a763 ) /* Banked code */
	ROM_LOAD("POUTU57T.BIN", 0x20000, 0x10000, 0xc22be0ff ) /* Banked code */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("POUTU95.BIN", 0x000000, 0x08000, 0xba6e797e )
	ROM_LOAD("POUTU94.BIN", 0x008000, 0x08000, 0xec63c015 )
	ROM_LOAD("POUTU93.BIN", 0x010000, 0x08000, 0xf102a04d )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
	ROM_LOAD("POUTU3.BIN",   0x00000, 0x02000, 0xaf213cb7 )
	ROM_LOAD("POUTU2T.BIN",  0x10000, 0x10000, 0xb23164c6 )
	ROM_LOAD("POUTU3T.BIN",  0x20000, 0x10000, 0xd93f105f )
	ROM_LOAD("POUTU4T.BIN",  0x30000, 0x10000, 0xb7c47bfe )
	ROM_LOAD("POUTU5T.BIN",  0x40000, 0x10000, 0xd9b9dfbf )
	ROM_LOAD("POUTU6T.BIN",  0x50000, 0x10000, 0x728c7c1a )
	ROM_LOAD("POUTU7T.BIN",  0x60000, 0x10000, 0x393bd990 )
	ROM_LOAD("POUTU8T.BIN",  0x70000, 0x10000, 0xcb9ffaad )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("POUTU25T.BIN", 0x080000, 0x10000, 0x92cd2617 )
	ROM_LOAD_ODD ("POUTU13T.BIN", 0x080000, 0x10000, 0x9448c389 )
	ROM_LOAD_EVEN("POUTU26T.BIN", 0x0a0000, 0x10000, 0xab57de8f )
	ROM_LOAD_ODD ("POUTU14T.BIN", 0x0a0000, 0x10000, 0x30678e93 )
	ROM_LOAD_EVEN("POUTU27T.BIN", 0x0e0000, 0x10000, 0x37a8156e )
	ROM_LOAD_ODD ("POUTU15T.BIN", 0x0e0000, 0x10000, 0x1c60d58b )

	ROM_REGION(0x40000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	ROM_LOAD( "POUTU70.BIN",  0x00000, 0x4000, 0x7db4eaa1 ) /* 00 */
	ROM_LOAD( "POUTU92.BIN",  0x04000, 0x4000, 0x20fa57bb )
	ROM_LOAD( "POUTU69.BIN",  0x08000, 0x4000, 0xa16886f3 )
	ROM_LOAD( "POUTU91.BIN",  0x0c000, 0x4000, 0x482a3581 )
	ROM_LOAD( "POUTU68.BIN",  0x10000, 0x4000, 0x7b62a3ed )
	ROM_LOAD( "POUTU90.BIN",  0x14000, 0x4000, 0x9615d710 )
	ROM_LOAD( "POUTU67.BIN",  0x18000, 0x4000, 0xaf85ce79 )
	ROM_LOAD( "POUTU89.BIN",  0x1c000, 0x4000, 0x6c874a05 )
ROM_END

struct GameDriver pigoutj_driver =
{
	__FILE__,
	&pigout_driver,
	"pigoutj",
	"Pigout (Japanese)",
	"1990",
	LELAND,
	"Paul Leaman",
	0,
	&pigout_machine,
	0,

	pigoutj_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_pigout,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

/***************************************************************************

  Super Offroad

***************************************************************************/

INPUT_PORTS_START( input_ports_offroad )
	PORT_START      /* IN3 (0xc1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* Unused */

	PORT_START      /* IN0 (0xc0)*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START      /* IN1 (0xd1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK ) /* Track pack */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, "Service", OSD_KEY_F2, IP_JOY_NONE )

	PORT_START      /* in2 (0xd0) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Analog pedal 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 0, 0, 255 )
	PORT_START      /* Analog pedal 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER2, 100, 0, 0, 255 )
	PORT_START      /* Analog pedal 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER3, 100, 0, 0, 255 )
	PORT_START      /* Analog wheel 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 0, 0, 255 )
	PORT_START      /* Analog wheel 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER2, 100, 0, 0, 255 )
	PORT_START      /* Analog wheel 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER3, 100, 0, 0, 255 )
INPUT_PORTS_END


static struct IOReadPort offroad_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0xc0, 0xc0, input_port_1_r },
	{ 0xc1, 0xc1, leland_slavebit0_r },
	{ 0xc3, 0xc3, AY8910_read_port_0_r },
	{ 0xd0, 0xd0, input_port_3_r },
	{ 0xd1, 0xd1, input_port_2_r },
	{ 0xf2, 0xf2, leland_sound_comm_r },
	{ 0xf9, 0xf9, input_port_7_r },
	{ 0xfb, 0xfb, input_port_9_r },
	{ 0xf8, 0xf8, input_port_8_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xfe, 0xfe, leland_analog_control_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort offroad_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0xc9, 0xc9, leland_slave_cmd_w },
	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },
	{ 0xca, 0xca, AY8910_control_port_0_w },
	{ 0xcb, 0xcb, AY8910_write_port_0_w },
	{ 0xcc, 0xcc, leland_bk_xlow_w },
	{ 0xcd, 0xcd, leland_bk_xhigh_w },
	{ 0xce, 0xce, leland_bk_ylow_w },
	{ 0xcf, 0xcf, leland_bk_yhigh_w },

	{ 0xf0, 0xf0, pigout_banksw_w },
	{ 0xf2, 0xf2, leland_sound_comm_w },
	{ 0xf4, 0xf4, leland_sound_cmd_w },
	{ 0xfd, 0xfd, MWA_NOP },            /* Reset analog ? */
	{ 0xfe, 0xfe, leland_analog_w },
	{ -1 }  /* end of table */
};


MACHINE_DRIVER(offroad_machine, offroad_readport, offroad_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	pigout_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( offroad_rom )
	ROM_REGION(0x040000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("22121-04.U58",   0x00000, 0x10000, 0xc5790988 )
	ROM_LOAD("22122-03.U59",   0x10000, 0x10000, 0xae862fdc )
	ROM_LOAD("22120-01.U57",   0x20000, 0x10000, 0xe9f0f175 )
	ROM_LOAD("22119-02.U56",   0x30000, 0x10000, 0x38642f22 )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("22105-01.U93", 0x00000, 0x08000, 0x4426e367 )
	ROM_LOAD("22106-02.U94", 0x08000, 0x08000, 0x687dc1fc )
	ROM_LOAD("22107-02.U95", 0x10000, 0x08000, 0xcee6ee5f )

	ROM_REGION(0x60000)     /* Z80 slave CPU */
	ROM_LOAD("22100-01.U2",  0x00000, 0x02000, 0x08c96a4b )
	ROM_LOAD("22108-02.U4",  0x10000, 0x10000, 0x0d72780a )
	ROM_LOAD("22109-02.U5",  0x20000, 0x10000, 0x5429ce2c )
	ROM_LOAD("22110-02.U6",  0x30000, 0x10000, 0xf97bad5c )
	ROM_LOAD("22111-01.U7",  0x40000, 0x10000, 0xf79157a1 )
	ROM_LOAD("22112-01.U8",  0x50000, 0x10000, 0x3eef38d3 )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("22116-03.U25", 0x080000, 0x10000, 0x95bb31d3 )
	ROM_LOAD_ODD ("22113-03.U13", 0x080000, 0x10000, 0x71b28df6 )
	ROM_LOAD_EVEN("22117-03.U26", 0x0a0000, 0x10000, 0x703d81ce )
	ROM_LOAD_ODD ("22114-03.U14", 0x0a0000, 0x10000, 0xf8b31bf8 )
	ROM_LOAD_EVEN("22118-03.U27", 0x0e0000, 0x10000, 0x806ccf8b )
	ROM_LOAD_ODD ("22115-03.U15", 0x0e0000, 0x10000, 0xc8439a7a )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* 70 = empty */
	ROM_LOAD( "22104-01.U92",  0x04000, 0x4000, 0x03e0497d )
	ROM_LOAD( "22102-01.U69",  0x08000, 0x4000, 0xc3f2e443 )
	/* 91 = empty */
	/* 68 = empty */
	ROM_LOAD( "22103-02.U90",  0x14000, 0x4000, 0x2266757a )
	ROM_LOAD( "22101-01.U67",  0x18000, 0x4000, 0xecab0527 )
	/* 89 = empty */
ROM_END

struct GameDriver offroad_driver =
{
	__FILE__,
	0,
	"offroad",
	"Super Off-road Racer",
	"????",
	LELAND,
	"Paul Leaman",
	0,
	&offroad_machine,
	0,

	offroad_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_offroad,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

ROM_START( offroada_rom )
	ROM_REGION(0x040000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("22121-04.U58",   0x00000, 0x10000, 0xc5790988 )
	ROM_LOAD("22122-03.U59",   0x10000, 0x10000, 0xae862fdc )
	ROM_LOAD("22120-01.U57",   0x20000, 0x10000, 0xe9f0f175 )
	ROM_LOAD("22119-02.U56",   0x30000, 0x10000, 0x38642f22 )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("SOF_B.93", 0x00000, 0x04000, 0xa24351bc ) /* Different (length?) */
	ROM_LOAD("SOF_B.94", 0x08000, 0x04000, 0xc0a44c36 ) /* Different (length?) */
	ROM_LOAD("SOF_B.95", 0x10000, 0x04000, 0xcf074561 ) /* Different (length?) */

	ROM_REGION(0x60000)     /* Z80 slave CPU */
	ROM_LOAD("SOF_B.3",      0x00000, 0x04000, 0x7dea667a ) /* Different (length?) */
	ROM_LOAD("22108-02.U4",  0x10000, 0x10000, 0x0d72780a )
	ROM_LOAD("22109-02.U5",  0x20000, 0x10000, 0x5429ce2c )
	ROM_LOAD("22110-02.U6",  0x30000, 0x10000, 0xf97bad5c )
	ROM_LOAD("22111-01.U7",  0x40000, 0x10000, 0xf79157a1 )
	ROM_LOAD("22112-01.U8",  0x50000, 0x10000, 0x3eef38d3 )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("22116-03.U25", 0x080000, 0x10000, 0x95bb31d3 )
	ROM_LOAD_ODD ("22113-03.U13", 0x080000, 0x10000, 0x71b28df6 )
	ROM_LOAD_EVEN("22117-03.U26", 0x0a0000, 0x10000, 0x703d81ce )
	ROM_LOAD_ODD ("22114-03.U14", 0x0a0000, 0x10000, 0xf8b31bf8 )
	ROM_LOAD_EVEN("22118-03.U27", 0x0e0000, 0x10000, 0x806ccf8b )
	ROM_LOAD_ODD ("22115-03.U15", 0x0e0000, 0x10000, 0xc8439a7a )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* 70 = empty */
	ROM_LOAD( "22104-01.U92",  0x04000, 0x4000, 0x03e0497d )
	ROM_LOAD( "22102-01.U69",  0x08000, 0x4000, 0xc3f2e443 )
	/* 91 = empty */
	/* 68 = empty */
	ROM_LOAD( "22103-02.U90",  0x14000, 0x4000, 0x2266757a )
	ROM_LOAD( "22101-01.U67",  0x18000, 0x4000, 0xecab0527 )
	/* 89 = empty */
ROM_END

struct GameDriver offroada_driver =
{
	__FILE__,
	&offroad_driver,
	"offroada",
	"Super Off-road Racer (probably bad set)",
	"????",
	LELAND,
	"Paul Leaman",
	0,
	&offroad_machine,
	0,

	offroada_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_offroad,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};




ROM_START( offroadt_rom )
	ROM_REGION(0x048000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("ORTPU58.BIN",   0x00000, 0x10000, 0xadbc6211 )
	ROM_LOAD("ORTPU59.BIN",   0x10000, 0x10000, 0x296dd3b6 )
	ROM_LOAD("ORTPU57.BIN",   0x20000, 0x10000, 0xe9f0f175 )  /* Identical to offroad */
	ROM_LOAD("ORTPU56.BIN",   0x30000, 0x10000, 0x2c1a22b3 )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("ORTPU93B.BIN", 0x00000, 0x08000, 0xf0c1d8b0 )
	ROM_LOAD("ORTPU94B.BIN", 0x08000, 0x08000, 0x7460d8c0 )
	ROM_LOAD("ORTPU95B.BIN", 0x10000, 0x08000, 0x081ee7a8 )

	ROM_REGION(0x90000)     /* Z80 slave CPU */
	ROM_LOAD("ORTPU3B.BIN", 0x00000, 0x02000, 0x95abb9f1 )
	ROM_LOAD("ORTPU2.BIN",  0x10000, 0x10000, 0xc46c1627 )
	ROM_LOAD("ORTPU3.BIN",  0x20000, 0x10000, 0x2276546f )
	ROM_LOAD("ORTPU4.BIN",  0x30000, 0x10000, 0xaa4b5975 )
	ROM_LOAD("ORTPU5.BIN",  0x40000, 0x10000, 0x69100b06 )
	ROM_LOAD("ORTPU6.BIN",  0x50000, 0x10000, 0xb75015b8 )
	ROM_LOAD("ORTPU7.BIN",  0x60000, 0x10000, 0xa5af5b4f )
	ROM_LOAD("ORTPU8.BIN",  0x70000, 0x10000, 0x0f735078 )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("ORTPU25.BIN", 0x080000, 0x10000, 0xf952f800 )
	ROM_LOAD_ODD ("ORTPU13.BIN", 0x080000, 0x10000, 0x7beec9fc )
	ROM_LOAD_EVEN("ORTPU26.BIN", 0x0c0000, 0x10000, 0x6227ea94 )
	ROM_LOAD_ODD ("ORTPU14.BIN", 0x0c0000, 0x10000, 0x0a44331d )
	ROM_LOAD_EVEN("ORTPU27.BIN", 0x0e0000, 0x10000, 0xb80c5f99 )
	ROM_LOAD_ODD ("ORTPU15.BIN", 0x0e0000, 0x10000, 0x2a1a1c3c )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* 70 = empty */
	ROM_LOAD( "ORTPU92B.BIN",  0x04000, 0x4000, 0xf9988e28 )
	ROM_LOAD( "ORTPU69B.BIN",  0x08000, 0x4000, 0xfe5f8d8f )
	/* 91 = empty */
	/* 68 = empty */
	ROM_LOAD( "ORTPU90B.BIN",  0x14000, 0x4000, 0xbda2ecb1 )
	ROM_LOAD( "ORTPU67B.BIN",  0x1c000, 0x4000, 0x38c9bf29 )
	/* 89 = empty */
ROM_END

struct GameDriver offroadt_driver =
{
	__FILE__,
	0,
	"offroadt",
	"Super Off-road Racer (Track Pack)",
	"????",
	"leland Corp.",
	"Paul Leaman",
	0,
	&pigout_machine,
	0,

	offroadt_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_offroad,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};


/***************************************************************************

  Team QB

***************************************************************************/
//
INPUT_PORTS_START( input_ports_teamqb )
	PORT_START      /* GIN1 (0x81) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* GIN0 (0x80) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0xee, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* (0x91) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service", OSD_KEY_F2, IP_JOY_NONE)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START      /* GIN1 (0x90) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* Analog spring stick 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 0, 0, 255 )
	PORT_START      /* Analog spring stick 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 0, 0, 255 )
	PORT_START      /* Analog spring stick 3 */
	PORT_START      /* Analog spring stick 4 */
	PORT_START      /* Analog spring stick 5 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER2, 100, 0, 0, 255 )
	PORT_START      /* Analog spring stick 5 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER2, 100, 0, 0, 255 )

	PORT_START      /* GIN3 (0x7c) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* GIN3 (0x7f) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1  | IPF_PLAYER1)
INPUT_PORTS_END

static struct IOReadPort teamqb_readport[] =
{
	{ 0x40, 0x5f, leland_mvram_port_r }, /* Video RAM ports */
	{ 0x80, 0x80, input_port_1_r },
	{ 0x81, 0x81, leland_slavebit0_r },
	{ 0x83, 0x83, AY8910_read_port_0_r },
	{ 0x90, 0x90, input_port_3_r },
	{ 0x91, 0x91, input_port_2_r },
	{ 0x7c, 0x7c, input_port_7_r },
	{ 0x7f, 0x7f, input_port_8_r },
	{ 0xf2, 0xf2, leland_sound_comm_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xfe, 0xfe, leland_analog_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort teamqb_writeport[] =
{
	{ 0x40, 0x5f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0x89, 0x89, leland_slave_cmd_w },
	{ 0x8a, 0x8a, AY8910_control_port_0_w },
	{ 0x8b, 0x8b, AY8910_write_port_0_w },
	{ 0x8c, 0x8c, leland_bk_xlow_w },
	{ 0x8d, 0x8d, leland_bk_xhigh_w },
	{ 0x8e, 0x8e, leland_bk_ylow_w },
	{ 0x8f, 0x8f, leland_bk_yhigh_w },

	{ 0xf0, 0xf0, pigout_banksw_w },
	{ 0xf2, 0xf2, leland_sound_comm_w },
	{ 0xf4, 0xf4, leland_sound_cmd_w },
	{ 0xfd, 0xfd, leland_analog_w },
	{ 0xfe, 0xfe, leland_analog_w },

	{ -1 }  /* end of table */
};

MACHINE_DRIVER(teamqb_machine, teamqb_readport, teamqb_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( teamqb_rom )
	ROM_REGION(0x048000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("15618-03.58T",   0x00000, 0x10000, 0xb32568dc )
	ROM_LOAD("15619-02.59T",   0x10000, 0x10000, 0x6d533714 )
	ROM_LOAD("15619-03.59T",   0x20000, 0x10000, 0x40b3319f )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("15615-01.U93", 0x00000, 0x04000, 0xa7ea6a87 )
	ROM_LOAD("15616-01.U94", 0x08000, 0x04000, 0x4a9b3900 )
	ROM_LOAD("15617-01.U95", 0x10000, 0x04000, 0x2cd95edb )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
	ROM_LOAD("15600-01.U3",   0x00000, 0x02000, 0x46615844 )
	ROM_LOAD("15601-01.U2T",  0x10000, 0x10000, 0x8e523c58 )
	ROM_LOAD("15602-01.U3T",  0x20000, 0x10000, 0x545b27a1 )
	ROM_LOAD("15603-01.U4T",  0x30000, 0x10000, 0xcdc9c09d )
	ROM_LOAD("15604-01.U5T",  0x40000, 0x10000, 0x3c03e92e )
	ROM_LOAD("15605-01.U6T",  0x50000, 0x10000, 0xcdf7d19c )
	ROM_LOAD("15606-01.U7T",  0x60000, 0x10000, 0x8eeb007c )
	ROM_LOAD("15607-01.U8T",  0x70000, 0x10000, 0x57cb6d2d )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("15623-01.25T", 0x080000, 0x10000, 0x710bdc76 )
	ROM_LOAD_ODD ("15620-01.13T", 0x080000, 0x10000, 0x7e5cb8ad )
	ROM_LOAD_EVEN("15624-01.26T", 0x0a0000, 0x10000, 0xdd090d33 )
	ROM_LOAD_ODD ("15621-01.14T", 0x0a0000, 0x10000, 0xf68c68c9 )
	ROM_LOAD_EVEN("15625-01.27T", 0x0e0000, 0x10000, 0xac442523 )
	ROM_LOAD_ODD ("15622-01.15T", 0x0e0000, 0x10000, 0x9e84509a )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	ROM_LOAD( "15611-01.U70",  0x00000, 0x4000, 0xbf2695fb )
	ROM_LOAD( "15614-01.U92",  0x04000, 0x4000, 0xc93fd870 )
	ROM_LOAD( "15610-01.U69",  0x08000, 0x4000, 0x3e5b786f )
	/* 91 = empty */
	ROM_LOAD( "15609-01.U68",  0x10000, 0x4000, 0x0319aec7 )
	ROM_LOAD( "15613-01.U90",  0x14000, 0x4000, 0x4805802e )
	ROM_LOAD( "15608-01.U67",  0x18000, 0x4000, 0x78f0fd2b )
	/* 89 = empty */
ROM_END

struct GameDriver teamqb_driver =
{
	__FILE__,
	0,
	"teamqb",
	"Team Quaterback",
	"1988",
	LELAND,
	"Paul Leaman",
	0,
	&teamqb_machine,
	0,

	teamqb_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_teamqb,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	leland_hiload,leland_hisave
};

INPUT_PORTS_START( input_ports_redlin2p )
	PORT_START      /* IN3 (0xc1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", OSD_KEY_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_START1 )  /* Unused */

	PORT_START      /* IN0 (0xc0)*/
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON1 )  /* Unused */

	PORT_START      /* IN1 (0xd1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* in2 (0xd0) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Analog pedal 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 0, 0, 255 )
	PORT_START      /* Analog pedal 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER2, 100, 0, 0, 255 )
	PORT_START      /* Analog pedal 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER3, 100, 0, 0, 255 )
	PORT_START      /* Analog wheel 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 0, 0, 255 )
	PORT_START      /* Analog wheel 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER2, 100, 0, 0, 255 )
	PORT_START      /* Analog wheel 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER3, 100, 0, 0, 255 )
INPUT_PORTS_END

void redlin2p_banksw_w(int offset, int data)
{
	int bank;
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	unsigned char *battery_bank=&RAM[0xa000];

	bank=data&0x03;

	if (bank==0x02)
	{
		/* 0 = resident */
		bankaddress =0x2000;
	}
	else
	{
		bankaddress  = 0x10000 + bank * 0x8000;
	}

	cpu_setbank(1,&RAM[bankaddress]);    /* 0x2000-0x9fff */
	cpu_setbank(3,battery_bank);            /* 0xa000-0xdfff */

	leland_sound_cpu_control_w(data);
}

static int redlin2p_kludge_r(int offset)
{
static int s;
s=~s;
return s;
}


static struct IOReadPort redlin2p_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0xc0, 0xc0, input_port_1_r },
	{ 0xc1, 0xc1, leland_slavebit0_r },
	{ 0xc3, 0xc3, AY8910_read_port_0_r },
	{ 0xd0, 0xd0, input_port_3_r },
	{ 0xd1, 0xd1, input_port_2_r },
	{ 0xf2, 0xf2, redlin2p_kludge_r },
//    { 0xf2, 0xf2, leland_sound_comm_r },
	{ 0xf9, 0xf9, input_port_7_r },
	{ 0xfb, 0xfb, input_port_9_r },
	{ 0xf8, 0xf8, input_port_8_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xfe, 0xfe, leland_analog_control_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort redlin2p_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0xc9, 0xc9, leland_slave_cmd_w },
	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },
	{ 0xca, 0xca, AY8910_control_port_0_w },
	{ 0xcb, 0xcb, AY8910_write_port_0_w },
	{ 0xcc, 0xcc, leland_bk_xlow_w },
	{ 0xcd, 0xcd, leland_bk_xhigh_w },
	{ 0xce, 0xce, leland_bk_ylow_w },
	{ 0xcf, 0xcf, leland_bk_yhigh_w },

	{ 0xf0, 0xf0, redlin2p_banksw_w },
	{ 0xf2, 0xf2, leland_sound_comm_w },
	{ 0xf4, 0xf4, leland_sound_cmd_w },
	{ 0xfd, 0xfd, MWA_NOP },            /* Reset analog ? */
	{ 0xfe, 0xfe, leland_analog_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER_NO_SOUND(redlin2p_machine, redlin2p_readport, redlin2p_writeport,
	master_readmem, master_writemem, pigout_init_machine, gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);


ROM_START( redlin2p_rom )
	ROM_REGION(0x048000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("13932-01.23T", 0x00000, 0x10000, 0xecdf0fbe )
	ROM_LOAD("13931-01.22T", 0x10000, 0x10000, 0x16d01978 )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("13930-01.U93", 0x00000, 0x04000, 0x0721f42e )
	ROM_LOAD("13929-01.U94", 0x08000, 0x04000, 0x1522e7b2 )
	ROM_LOAD("13928-01.U95", 0x10000, 0x04000, 0xc321b5d1 )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
	ROM_LOAD("13907-01.U3",  0x00000, 0x04000, 0xb760d63e )
	ROM_LOAD("13908-01.U4",  0x10000, 0x04000, 0xa30739d3 )
	ROM_LOAD("13909-01.U5",  0x10000, 0x04000, 0xaaf16ad7 )
	ROM_LOAD("13910-01.U6",  0x10000, 0x04000, 0xd03469eb )
	ROM_LOAD("13911-01.U7",  0x10000, 0x04000, 0x8ee1f547 )
	ROM_LOAD("13912-01.U8",  0x10000, 0x04000, 0xe5b57eac )
	ROM_LOAD("13913-01.U9",  0x10000, 0x04000, 0x02886071 )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("28T",    0x0e0000, 0x10000, 0x7aa21b2c )
	ROM_LOAD_ODD ("17T",    0x0e0000, 0x10000, 0x8d26f221 )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	ROM_LOAD( "13920-01.U70",  0x00000, 0x4000, 0xf343d34a )
	ROM_LOAD( "13921-01.U92",  0x04000, 0x4000, 0xc9ba8d41 )
	ROM_LOAD( "13922-01.U69",  0x08000, 0x4000, 0x276cfba0 )
	ROM_LOAD( "13923-01.U91",  0x0c000, 0x4000, 0x4a88ea34 )
	ROM_LOAD( "13924-01.U68",  0x10000, 0x4000, 0x3995cb7e )
	/* 90 = empty / missing */
	ROM_LOAD( "13926-01.U67",  0x18000, 0x4000, 0xdaa30add )
	ROM_LOAD( "13927-01.U89",  0x1c000, 0x4000, 0x30e60fb5 )
ROM_END

struct GameDriver redlin2p_driver =
{
	__FILE__,
	0,
	"redlin2p",
	"Redline Racer (2 player version)",
	"1987",
	"Cinematronics",
	"Paul Leaman",
	0,
	&redlin2p_machine,
	0,

	redlin2p_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_redlin2p,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	leland_hiload,leland_hisave
};

INPUT_PORTS_START( input_ports_viper )
	PORT_START      /* IN3 (0xc1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", OSD_KEY_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_START1 )  /* Unused */

	PORT_START      /* IN0 (0xc0)*/
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON1 )  /* Unused */

	PORT_START      /* IN1 (0xd1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* in2 (0xd0) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Analog pedal 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 0, 0, 255 )
	PORT_START      /* Analog pedal 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER2, 100, 0, 0, 255 )
	PORT_START      /* Analog pedal 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER3, 100, 0, 0, 255 )
	PORT_START      /* Analog wheel 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 0, 0, 255 )
	PORT_START      /* Analog wheel 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER2, 100, 0, 0, 255 )
	PORT_START      /* Analog wheel 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER3, 100, 0, 0, 255 )
INPUT_PORTS_END

void viper_banksw_w(int offset, int data)
{
	int bank;
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	unsigned char *battery_bank=&RAM[0xa000];

	bank=data&0x07;
#ifdef NOISY_CPU
	if (errorlog)
	{
		fprintf(errorlog, "BANK=%02x\n", bank);
	}
#endif
	if (!bank)
	{
		/* 0 = resident */
		bankaddress =0x2000;
	}
	else
	{
		bank--;
		bankaddress  = 0x10000 + bank * 0x8000;
	}

	cpu_setbank(1,&RAM[bankaddress]);    /* 0x2000-0x9fff */
	cpu_setbank(3,battery_bank);            /* 0xa000-0xdfff */

	leland_sound_cpu_control_w(data);
}

static struct IOReadPort viper_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0xc0, 0xc0, input_port_1_r },
	{ 0xc1, 0xc1, leland_slavebit0_r },
	{ 0xc3, 0xc3, AY8910_read_port_0_r },
	{ 0xd0, 0xd0, input_port_3_r },
	{ 0xd1, 0xd1, input_port_2_r },
	{ 0xf2, 0xf2, leland_sound_comm_r },
	{ 0xf9, 0xf9, input_port_7_r },
	{ 0xfb, 0xfb, input_port_9_r },
	{ 0xf8, 0xf8, input_port_8_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xfe, 0xfe, leland_analog_control_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort viper_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0xc9, 0xc9, leland_slave_cmd_w },
	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },
	{ 0xca, 0xca, AY8910_control_port_0_w },
	{ 0xcb, 0xcb, AY8910_write_port_0_w },
	{ 0xcc, 0xcc, leland_bk_xlow_w },
	{ 0xcd, 0xcd, leland_bk_xhigh_w },
	{ 0xce, 0xce, leland_bk_ylow_w },
	{ 0xcf, 0xcf, leland_bk_yhigh_w },

	{ 0xf0, 0xf0, viper_banksw_w },
	{ 0xf2, 0xf2, leland_sound_comm_w },
	{ 0xf4, 0xf4, leland_sound_cmd_w },
	{ 0xfd, 0xfd, MWA_NOP },            /* Reset analog ? */
	{ 0xfe, 0xfe, leland_analog_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER_NO_SOUND(viper_machine, viper_readport, viper_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( viper_rom )
	ROM_REGION(0x050000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("15617-03.49T",   0x00000, 0x10000, 0x7e4688a6 )
	ROM_LOAD("15616-03.48T",   0x10000, 0x10000, 0x3fe2f0bf )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("15609-01.U93", 0x00000, 0x04000, 0x08ad92e9 )
	ROM_LOAD("15610-01.U94", 0x08000, 0x04000, 0xd4e56dfb )
	ROM_LOAD("15611-01.U95", 0x10000, 0x04000, 0x3a2c46fb )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
	ROM_LOAD("15600-02.U3", 0x00000, 0x02000, 0x0f57f68a )
	ROM_LOAD("VIPER.U2T",   0x10000, 0x10000, 0x4043d4ee )
	ROM_LOAD("VIPER.U3T",   0x20000, 0x10000, 0x213bc02b )
	ROM_LOAD("VIPER.U4T",   0x30000, 0x10000, 0xce0b95b4 )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN( "15620-02.45T", 0x0a0000, 0x10000, 0x7380ece1 )
	ROM_LOAD_ODD ( "15623-02.62T", 0x0a0000, 0x10000, 0x2921d8f9 )
	ROM_LOAD_EVEN( "15619-02.44T", 0x0c0000, 0x10000, 0xc8507cc2 )
	ROM_LOAD_ODD ( "15622-02.61T", 0x0c0000, 0x10000, 0x32dfda37 )
	ROM_LOAD_EVEN( "15618-02.43T", 0x0e0000, 0x10000, 0x5562e0c3 )
	ROM_LOAD_ODD ( "15621-02.60T", 0x0e0000, 0x10000, 0xcb468f2b )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	ROM_LOAD( "15604-01.U70",  0x00000, 0x4000, 0x7e3b0cce )
	ROM_LOAD( "15608-01.U92",  0x04000, 0x4000, 0xa9bde0ef )
	ROM_LOAD( "15603-01.U69",  0x08000, 0x4000, 0xaecc9516 )
	ROM_LOAD( "15607-01.U91",  0x0c000, 0x4000, 0x14f06f88 )
	ROM_LOAD( "15602-01.U68",  0x10000, 0x4000, 0x4ef613ad )
	ROM_LOAD( "15606-01.U90",  0x14000, 0x4000, 0x3c2e8e76 )
	ROM_LOAD( "15601-01.U67",  0x18000, 0x4000, 0xdc7006cd )
	ROM_LOAD( "15605-01.U89",  0x1c000, 0x4000, 0x4aa9c788 )
ROM_END

struct GameDriver viper_driver =
{
	__FILE__,
	0,
	"viper",
	"Viper",
	"1988",
	LELAND,
	"Paul Leaman",
	0,
	&viper_machine,
	0,

	viper_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_viper,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

INPUT_PORTS_START( input_ports_aafb )
	PORT_START      /* GIN1 (0x41) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", OSD_KEY_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* GIN0 (0x40) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START      /* GIN1 (0x50) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2  )

	PORT_START      /* GIN3 (0x7c) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static struct IOReadPort aafb_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* offroad video RAM ports */

	{ 0x40, 0x40, input_port_1_r },
	{ 0x41, 0x41, leland_slavebit0_r },
	{ 0x43, 0x43, AY8910_read_port_0_r },
	{ 0x50, 0x50, input_port_3_r },
	{ 0x51, 0x51, input_port_2_r },
	{ 0x7c, 0x7c, input_port_4_r },

	{ 0xf2, 0xf2, leland_sound_comm_r },
	{ -1 }  /* end of table */
};


static struct IOWritePort aafb_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* track pack Video RAM ports */

	{ 0x40, 0x46, leland_mvram_port_w }, /* (stray) video RAM ports */
	{ 0x49, 0x49, leland_slave_cmd_w },
	{ 0x4a, 0x4a, AY8910_control_port_0_w },
	{ 0x4b, 0x4b, AY8910_write_port_0_w },
	{ 0x4c, 0x4c, leland_bk_xlow_w },
	{ 0x4d, 0x4d, leland_bk_xhigh_w },
	{ 0x4e, 0x4e, leland_bk_ylow_w },
	{ 0x4f, 0x4f, leland_bk_yhigh_w },

	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },



	{ 0xf0, 0xf0, viper_banksw_w },
	{ 0xf2, 0xf2, leland_sound_comm_w },
	{ 0xf4, 0xf4, leland_sound_cmd_w },
	{ -1 }  /* end of table */
};



MACHINE_DRIVER(aafb_machine, aafb_readport, aafb_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);


ROM_START( aafb_rom )
	ROM_REGION(0x048000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("24014-02.U58",   0x00000, 0x10000, 0x5db4a3d0 ) /* SUSPECT */
	ROM_LOAD("24015-02.U59",   0x10000, 0x10000, 0x00000000 ) /* SUSPECT */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("24011-02.U93", 0x00000, 0x08000, 0x011c0235 )  /* SUSPECT */
	ROM_LOAD("24012-02.U94", 0x08000, 0x08000, 0x376199a2 )  /* SUSPECT */
	ROM_LOAD("24013-02.U95", 0x10000, 0x08000, 0x0a604e0d )  /* SUSPECT */

	ROM_REGION(0x80000)     /* Z80 slave CPU */
	ROM_LOAD("24000-02.U3",   0x00000, 0x02000, 0x52df0354 ) /* SUSPECT */
	ROM_LOAD("24001-02.U2T",  0x10000, 0x10000, 0x9b20697d ) /* SUSPECT */
	ROM_LOAD("24002-02.U3T",  0x20000, 0x10000, 0xbbb92184 )
	ROM_LOAD("15603-01.U4T",  0x30000, 0x10000, 0xcdc9c09d )
	ROM_LOAD("15604-01.U5T",  0x40000, 0x10000, 0x3c03e92e )
	ROM_LOAD("15605-01.U6T",  0x50000, 0x10000, 0xcdf7d19c )
	ROM_LOAD("15606-01.U7T",  0x60000, 0x10000, 0x8eeb007c )
	ROM_LOAD("24002-02.U8T",  0x70000, 0x10000, 0x3d9747c9 )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("24019-01.U25", 0x080000, 0x10000, 0x9e344768 )
	ROM_LOAD_ODD ("24016-01.U13", 0x080000, 0x10000, 0x6997025f )
	ROM_LOAD_EVEN("24020-01.U26", 0x0a0000, 0x10000, 0x0788f2a5 )
	ROM_LOAD_ODD ("24017-01.U14", 0x0a0000, 0x10000, 0xa48bd721 )
	ROM_LOAD_EVEN("24021-01.U27", 0x0e0000, 0x10000, 0x94081899 )
	ROM_LOAD_ODD ("24018-01.U15", 0x0e0000, 0x10000, 0x76eb6077 )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	ROM_LOAD( "24007-01.U70",  0x00000, 0x4000, 0x40e46aa4 )
	ROM_LOAD( "24010-01.U92",  0x04000, 0x4000, 0x78705f42 )
	ROM_LOAD( "24006-01.U69",  0x08000, 0x4000, 0x6a576aa9 )
	ROM_LOAD( "24009-02.U91",  0x0c000, 0x4000, 0xb857a1ad )
	ROM_LOAD( "24005-02.U68",  0x10000, 0x4000, 0x8ea75319 )
	ROM_LOAD( "24008-01.U90",  0x14000, 0x4000, 0x4538bc58 )
	ROM_LOAD( "24004-02.U67",  0x18000, 0x4000, 0xcd7a3338 )
	/* 89 = empty */
ROM_END



struct GameDriver aafb_driver =
{
	__FILE__,
	0,
	"aafb",
	"All American Football",
	"????",
	LELAND,
	"Paul Leaman",
	0,
	&aafb_machine,
	0,

	aafb_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_aafb,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	leland_hiload,leland_hisave
};


ROM_START( aafb2p_rom )
	ROM_REGION(0x048000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("aafb2p.58T",   0x00000, 0x10000, 0x79fd14cd )
	ROM_LOAD("aafb2p.59T",   0x10000, 0x10000, 0x3b0382f0 )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("aafb2p.U93", 0x00000, 0x04000, 0x5c333ef3 )
	ROM_LOAD("aafb2p.U94", 0x08000, 0x04000, 0x2e7a1af0 )
	ROM_LOAD("aafb2p.U95", 0x10000, 0x04000, 0x7ede8bf2 )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
	ROM_LOAD("aafb2p.U3",   0x00000, 0x04000, 0x7a8259c4 )
	ROM_LOAD("aafb2p.U2T",  0x10000, 0x10000, 0xf118b9b4 )
	ROM_LOAD("aafb2p.U3T",  0x20000, 0x10000, 0xbbb92184 )
	ROM_LOAD("aafb2p.U4T",  0x30000, 0x10000, 0xcdc9c09d )
	ROM_LOAD("aafb2p.U5T",  0x40000, 0x10000, 0x3c03e92e )
	ROM_LOAD("aafb2p.U6T",  0x50000, 0x10000, 0xcdf7d19c )
	ROM_LOAD("aafb2p.U7T",  0x60000, 0x10000, 0x8eeb007c )
	ROM_LOAD("aafb2p.U8T",  0x70000, 0x10000, 0x3d9747c9 )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("aafb2p.25T", 0x080000, 0x10000, 0x9e344768 )
	ROM_LOAD_ODD ("aafb2p.13T", 0x080000, 0x10000, 0x6997025f )
	ROM_LOAD_EVEN("aafb2p.26T", 0x0a0000, 0x10000, 0x0788f2a5 )
	ROM_LOAD_ODD ("aafb2p.14T", 0x0a0000, 0x10000, 0xa48bd721 )
	ROM_LOAD_EVEN("aafb2p.27T", 0x0e0000, 0x10000, 0x94081899 )
	ROM_LOAD_ODD ("aafb2p.15T", 0x0e0000, 0x10000, 0x76eb6077 )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	ROM_LOAD( "aafb2p.U70",  0x00000, 0x4000, 0x40e46aa4 )
	ROM_LOAD( "aafb2p.U92",  0x04000, 0x4000, 0x78705f42 )
	ROM_LOAD( "aafb2p.U69",  0x08000, 0x4000, 0x6a576aa9 )
	ROM_LOAD( "aafb2p.U91",  0x0c000, 0x4000, 0xb857a1ad )
	ROM_LOAD( "aafb2p.U68",  0x10000, 0x4000, 0x8ea75319 )
	ROM_LOAD( "aafb2p.U90",  0x14000, 0x4000, 0x4538bc58 )
	ROM_LOAD( "aafb2p.U67",  0x18000, 0x4000, 0xcd7a3338 )
	/* 89 = empty */
ROM_END

struct GameDriver aafb2p_driver =
{
	__FILE__,
	0,
	"aafb2p",
	"All American Football",
	"????",
	LELAND,
	"Paul Leaman",
	0,
	&aafb_machine,
	0,

	aafb2p_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_aafb,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	leland_hiload,leland_hisave
};


/***************************************************************************

  Ataxx

***************************************************************************/

extern int ataxx_vh_start(void);
extern void ataxx_vh_stop(void);
extern void ataxx_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void leland_graphics_ram_w(int offset, int data);
extern unsigned char *ataxx_bk_ram;
extern unsigned char *ataxx_tram;
extern int ataxx_tram_size;
extern unsigned char *ataxx_qram1;
extern unsigned char *ataxx_qram2;

INPUT_PORTS_START( input_ports_ataxx )
	PORT_START /* (0xf7) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START  /* (0xf6) */
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, "Service", OSD_KEY_F2, IP_JOY_NONE)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START
    PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_X, 50, 0, 0, 15 ) /* Sensitivity, clip, min, max */

	PORT_START
    PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_Y, 50, 0, 0, 15 )

	PORT_START
    PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 0, 0, 0 ) /* Sensitivity, clip, min, max */

	PORT_START
    PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 0, 0, 0 )
INPUT_PORTS_END



static struct GfxLayout ataxx_tilelayout =
{
  8,8,  /* 8 wide by 8 high */
  16*1024, /* 128k/8 characters */
  6,    /* 6 bits per pixel, each ROM holds one bit */
  { 8*0xa0000, 8*0x80000, 8*0x60000, 8*0x40000, 8*0x20000, 8*0x00000 }, /* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo ataxx_gfxdecodeinfo[] =
{
  { 1, 0x00000, &ataxx_tilelayout, 0, 64 },
  { -1 } /* end of array */
};

void ataxx_slave_cmd_w(int offset, int data)
{
	cpu_set_irq_line(1, 0, data&0x01 ? CLEAR_LINE : HOLD_LINE);
	cpu_set_nmi_line(1,    data&0x04 ? CLEAR_LINE : HOLD_LINE);
	cpu_set_reset_line(1,  data&0x10 ? CLEAR_LINE : HOLD_LINE);
	leland_slave_cmd=data;
}

void ataxx_banksw_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int bank=data & 0x03;
#ifdef NOISY_CPU
	if (errorlog)
	{
        fprintf(errorlog, "Ataxx bank %02x\n", data);
	}
#endif
    /* Program ROM bank */
	if (!bank)
	{
		cpu_setbank(1, &RAM[0x2000]);
	}
	else
	{
        cpu_setbank(1, &RAM[0x8000*bank]);
	}

    /* Battery / QRAM bank */
    cpu_setbank(3, &RAM[0xa000]);
    if (data & 0x10)
    {
        cpu_setbank(3, &leland_battery_ram[0]);
    }
    else
    {
        if (data & 0x20)
        {
            cpu_setbank(3, &ataxx_qram1[0]);
        }
        if (data & 0x40)
        {
            cpu_setbank(3, &ataxx_qram2[0]);
        }
    }
}

static struct MemoryReadAddress ataxx_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x9fff, MRA_BANK1 },
    { 0xa000, 0xdfff, MRA_BANK3 },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xfff7, leland_palette_ram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ataxx_writemem[] =
{
    { 0xa000, 0xdfff, MWA_BANK3 },
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xefff, MWA_RAM },
    { 0xf000, 0xf7ff, MWA_RAM, &ataxx_tram, &ataxx_tram_size },
	{ 0xf800, 0xfff7, leland_palette_ram_w, &leland_palette_ram, &leland_palette_ram_size },
	{ 0xfff8, 0xfff9, leland_master_video_addr_w },
	{ -1 }  /* end of table */
};

void ataxx_init_machine(void)
{
    unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

    leland_init_machine();

    leland_rearrange_bank(&RAM[0x10000], 2);
}

int ataxx_unknown_port_r(int offset)
{
	static int s;
	s=s^0x01;
	return s;
}


int ataxx_unknown_r(int offset)
{
	static int s;
	s=~s;
	return s;
}

void ataxx_sound_control_w(int offset, int data)
{
	/*
		0x01=Reset
		0x02=NMI
		0x04=Int0
		0x08=Int1
		0x10=Test
	*/
}

static struct IOReadPort ataxx_readport[] =
{
	{ 0x00, 0x00, input_port_2_r },  /* Player 1 Track X */
	{ 0x00, 0x01, input_port_3_r },  /* Player 1 Track Y */
	{ 0x00, 0x02, input_port_4_r },  /* Player 2 Track X */
	{ 0x00, 0x03, input_port_5_r },  /* Player 2 Track Y */
	{ 0x04, 0x04, leland_sound_comm_r },    /* = 0xf2 */
	{ 0x20, 0x20, ataxx_unknown_port_r },
	{ 0xd0, 0xe7, ataxx_mvram_port_r },
	{ 0xf6, 0xf6, input_port_1_r },
	{ 0xf7, 0xf7, leland_slavebit0_r },     /* Slave block (slvblk) */
	{ -1 }  /* end of table */
};

static struct IOWritePort ataxx_writeport[] =
{
	{ 0x06, 0x06, leland_sound_comm_w },    /* = 0xf2 */
	{ 0x05, 0x05, leland_sound_cmd_w },     /* = 0xf4 */
	{ 0x0c, 0x0c, ataxx_sound_control_w },  /* = Sound control register */
	{ 0x20, 0x20, leland_unknown_w },       /* = Unknown output port (0xff) */
	{ 0xd0, 0xe7, ataxx_mvram_port_w },
	{ 0xf0, 0xf0, leland_bk_xlow_w },       /* = Probably ... */
	{ 0xf1, 0xf1, leland_bk_xhigh_w },
	{ 0xf2, 0xf2, leland_bk_ylow_w },
	{ 0xf3, 0xf3, leland_bk_yhigh_w },
	{ 0xf4, 0xf4, ataxx_banksw_w },         /* = Bank switch */
	{ 0xf5, 0xf5, ataxx_slave_cmd_w },      /* = Slave output (slvo) */
	{ 0xf8, 0xf8, MWA_NOP },                /* = Unknown */
	{ -1 }  /* end of table */
};

void ataxx_slave_banksw_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	int bankaddress;
    int bank;
    /* This is wrong! */

    switch (data & 0x0f)
    {
        case 5:
            bank=4;
            break;
        case 4:
            bank=3;
            break;
        case 3:
            bank=2;
            break;
        case 2:
            bank=1;
            break;
        default:
            bank=0;
            break;
    }
    bank=2;
    bankaddress =0x10000*bank;
    bankaddress+=0x8000*(~data&0x10);
    cpu_setbank(7, &RAM[bankaddress]);
    cpu_setbank(8, &RAM[bankaddress+0x2000]);
}


static struct MemoryReadAddress ataxx_slave_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },        /* Resident program ROM */
    { 0x2000, 0x7fff, MRA_BANK7 },      /* Paged graphics ROM */
    { 0x8000, 0x9fff, MRA_BANK8 },      /* Paged graphics ROM */
	{ 0xa000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf800, 0xfff7, leland_palette_ram_r },
	{ 0xfffe, 0xfffe, ataxx_unknown_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ataxx_slave_writemem[] =
{
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf800, 0xfff7, leland_palette_ram_w },
	{ 0xfffc, 0xfffd, leland_slave_video_addr_w },
    { 0xffff, 0xffff, ataxx_slave_banksw_w },
	{ -1 }  /* end of table */
};

#if 0
                {                                                  \
                        CPU_I86,         /* Sound processor */     \
                        16000000,        /* 16 Mhz  */             \
                        3,                                         \
                        leland_i86_readmem,leland_i86_writemem,\
                        leland_i86_readport,leland_i86_writeport, \
                        leland_i86_interrupt,1,                    \
                        0,0,&leland_addrmask                    \
                },                                                 \

#endif

#define ATAXX_MACHINE_DRIVER(DRV, MRP, MWP, MR, MW, INITMAC, GFXD, VRF, SLR, SLW)\
static struct MachineDriver DRV =    \
{                                                      \
        {                                           \
                {                                    \
                        CPU_Z80,        /* Master game processor */  \
                        6000000,        /* 6.000 Mhz  */             \
                        0,                                          \
                        MR,MW,            \
                        MRP,MWP,                                   \
                        leland_master_interrupt,16                 \
                },                                                 \
                {                                                  \
                        CPU_Z80, /* Slave graphics processor*/     \
                        6000000, /* 6.000 Mhz */                   \
                        2,       /* memory region #2 */            \
                        SLR,SLW,              \
                        slave_readport,slave_writeport,            \
                        leland_slave_interrupt,1                   \
                },                                                 \
        },                                                         \
        60, 2000,2,                                                \
        INITMAC,                                       \
        0x28*8, 0x20*8, { 0*8, 0x28*8-1, 0*8, 0x1e*8-1 },              \
        GFXD,                                             \
        1024,1024,                                                 \
        0,                                                         \
        VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,                \
        0,                                                         \
        ataxx_vh_start,ataxx_vh_stop,VRF,    \
        0,0,0,0,\
        {   \
            {SOUND_AY8910, &ay8910_interface }, \
            {SOUND_DAC,    &dac_interface }     \
        }                 \
}

ATAXX_MACHINE_DRIVER(ataxx_machine,ataxx_readport, ataxx_writeport,
	ataxx_readmem, ataxx_writemem, ataxx_init_machine, ataxx_gfxdecodeinfo,
	ataxx_vh_screenrefresh, ataxx_slave_readmem, ataxx_slave_writemem);

ROM_START( ataxx_rom )
	ROM_REGION(0x20000)
	ROM_LOAD( "ataxx.038",   0x00000, 0x20000, 0x0e1cf6236)

	ROM_REGION_DISPOSE(0xC0000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ataxx.098",  0x00000, 0x20000, 0x059d0f2ae )
	ROM_LOAD( "ataxx.099",  0x20000, 0x20000, 0x06ab7db25 )
	ROM_LOAD( "ataxx.100",  0x40000, 0x20000, 0x02352849e )
	ROM_LOAD( "ataxx.101",  0x60000, 0x20000, 0x04c31e02b )
	ROM_LOAD( "ataxx.102",  0x80000, 0x20000, 0x0a951228c )
	ROM_LOAD( "ataxx.103",  0xa0000, 0x20000, 0x0ed326164 )

	ROM_REGION(0x060000) /* 1M for secondary cpu */
	ROM_LOAD( "ataxx.111",  0x00000, 0x20000, 0x09a3297cc )
	ROM_LOAD( "ataxx.112",  0x20000, 0x20000, 0x07e7c3e2f )
	ROM_LOAD( "ataxx.113",  0x40000, 0x20000, 0x08cf3e101 )

	ROM_REGION(0x100000) /* 1M for sound cpu */
	ROM_LOAD_EVEN( "ataxx.015",  0x80000, 0x20000, 0x08bb3233b )
	ROM_LOAD_ODD ( "ataxx.001",  0x80000, 0x20000, 0x0728d75f2 )
	ROM_LOAD_EVEN( "ataxx.016",  0xC0000, 0x20000, 0x0f2bdff48 )
	ROM_LOAD_ODD ( "ataxx.002",  0xC0000, 0x20000, 0x0ca06a394 )
ROM_END

struct GameDriver ataxx_driver =
{
	__FILE__,
	0,
	"ataxx",
	"Ataxx",
	"1990",
    LELAND,
	"Paul Leaman\nScott Kelley",
	0,
	&ataxx_machine,
	0,
	ataxx_rom,
	0, 0,
	0,
	0,

	input_ports_ataxx,

	0, 0, 0,
	ORIENTATION_DEFAULT,
    leland_hiload, leland_hisave
};






