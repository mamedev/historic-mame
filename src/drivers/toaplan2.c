/*****************************************************************************
 Some ToaPlan games from 1991 -1994
 See end of file for Toaplan game numbers.

 Driver by : Quench

 To Do / Unknowns
	- Whoopee/Teki Paki sometimes tests bit 5 of the territory port
		just after testing for vblank. Why ?
	- Whoppee is currently using the sound CPU ROM (Z80) from a differnt
		(pirate ?) version of Pipi and Bibis (Ryouta Kikaku copyright).
		It really has a HD647180 CPU, and its internal ROM needs to be dumped.
	- Pipi and Bibis (Ryouta Kikaku copyright) Romset not supported yet.
		CPU roms are encoded, and decoding needs to be worked out.

--- Game status ---
Teki Paki     Working, but no sound. Missing sound MCU dump
Ghox          Sprites are missing. Probably due to MCU dump missing.
Dogyuun       Working? but bad GFX. Requires support for two video controllers
Knuckle Bash  Working, but no sound. MCU dump exists, but it needs investigation
Tatsujin 2    Not Working.
Pipi & Bibis  Working.
Whoopee       Working. Missing sound MCU dump. Using bootleg sound CPU dump for now
Pipi & Bibis  (bootleg ?)  Not working. 68K CPU ROMs are encoded ?
FixEight      Not working. Sound MCU (type unknown) dump is missing.
Snow Bros. 2  Working



*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"


#define HD64X180 0

/**************** Machine stuff ******************/
#define CPU_2_NONE		0x00
#define CPU_2_Z80		0x5a
#define CPU_2_HD647180  0xa5
#define CPU_2_UNKNOWN   0xff

static unsigned char *toaplan2_shared_ram;
static int mcu_data = 0;
int toaplan2_sub_cpu = 0;


/**************** Video stuff ******************/
int  toaplan2_scrollregs_r(int offset);
void toaplan2_scrollregs_w(int offset, int data);
void toaplan2_offsetregs_w(int offset, int data);
void toaplan2_layers_offset_w(int offset, int data);
void toaplan2_voffs_w(int offset, int data);

int  toaplan2_videoram_r(int offset);
void toaplan2_videoram_w(int offset, int data);

void toaplan2_scroll_reg_select_w(int offset, int data);
void toaplan2_scroll_reg_data_w(int offset, int data);

int  toaplan2_vh_start(void);
void toaplan2_vh_stop(void);
void toaplan2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static int video_status = 0;



static void init_toaplan2(void)
{
	toaplan2_sub_cpu = CPU_2_HD647180;
	mcu_data = 0;
}

static void init_toaplan3(void)
{
	toaplan2_sub_cpu = CPU_2_UNKNOWN;
	mcu_data = 0;
}

static void init_pipibibs(void)
{
	toaplan2_sub_cpu = CPU_2_Z80;
}

static void init_snowbro2(void)
{
	toaplan2_sub_cpu = CPU_2_NONE;
}

static int toaplan2_interrupt(void)
{
	return MC68000_IRQ_4;
}

static void toaplan2_coin_w(int offset, int data)
{
	switch (data & 0x0f)
	{
		case 0x00:	coin_lockout_global_w(0,1); break;	/* Lock all coin slots */
		case 0x0c:	coin_lockout_global_w(0,0); break;	/* Unlock all coin slots */
		case 0x0d:	coin_counter_w(0,1); coin_counter_w(0,0);	/* Count slot A */
					if (errorlog) fprintf(errorlog,"Count coin slot A\n"); break;
		case 0x0e:	coin_counter_w(1,1); coin_counter_w(1,0);	/* Count slot B */
					if (errorlog) fprintf(errorlog,"Count coin slot B\n"); break;
	/* The following are coin counts after coin-lock active (faulty coin-lock ?) */
		case 0x01:	coin_counter_w(0,1); coin_counter_w(0,0); coin_lockout_w(0,1); break;
		case 0x02:	coin_counter_w(1,1); coin_counter_w(1,0); coin_lockout_global_w(0,1); break;

		default:	if (errorlog) fprintf(errorlog,"Writing unknown command (%04x) to coin control\n",data);
					break;
	}
	if (data > 0xf)
	{
		if (errorlog) fprintf(errorlog,"Writing unknown upper bits command (%04x) to coin control\n",data);
	}
}

static void oki_bankswitch_w(int offset, int data)
{
	OKIM6295_set_bank_base(0, ALL_VOICES, (data & 1) * 0x40000);
}

static int toaplan2_shared_r(int offset)
{
	return toaplan2_shared_ram[offset>>1];
}

static void toaplan2_shared_w(int offset, int data)
{
	toaplan2_shared_ram[offset>>1] = data;
}

static void toaplan2_sub_cpu_w(int offset, int data)
{
	/* Command sent to secondary CPU. Support for HD647180 will be required
	   when a ROM dump becomes available for this hardware */

	if (toaplan2_sub_cpu == CPU_2_Z80)		/* Whoopee */
	{
		toaplan2_shared_ram[0] = data;
	}
	else
	{
		mcu_data = data;
		if (errorlog) fprintf(errorlog,"PC:%08x Writing command (%04x) to secondary CPU shared port\n",cpu_getpreviouspc(),mcu_data);
	}
}

static int c2map_port_6_r(int offset)
{
	/* bit 4 high signifies secondary CPU is ready */
	/* bit 5 is tested low before V-Blank bit ??? */
	switch (toaplan2_sub_cpu)
	{
		case CPU_2_Z80:			mcu_data = toaplan2_shared_ram[0]; break;
		case CPU_2_HD647180:	mcu_data = 0xff; break;
		default:				mcu_data = 0x00; break;
	}
	if (mcu_data == 0xff) mcu_data = 0x10;
	else mcu_data = 0x00;
	return ( mcu_data | input_port_6_r(0) );
}

static int video_status_r(int offset)
{
	/* video busy if bit 8 is low, or bits 7-0 are high ??? */
	video_status += 0x100;
	video_status &= 0x100;
	return video_status;
}

static int kbash_sub_cpu_r(int offset)
{
/*	Knuckle Bash's  68000 reads HD64x180 cpu status via an I/O port.
	If a value of 2 is read, then HD64x180 cpu is busy.
	HD64x180 cpu must report 0xff when no longer busy, to signify that it
	has passed POST.
*/
	mcu_data=0xff;
	return mcu_data;
}

static int ghox_mcu_r(int offset)
{
	return 0xff;
}
static void ghox_mcu_w(int offset, int data)
{
	data &= 0xffff;
	mcu_data = data;
	if ((data >= 0xd0) && (data < 0xe0))
	{
		offset = ((data & 0x0f) * 4) + 0x38;
		WRITE_WORD (&toaplan2_shared_ram[offset  ],0x05);	/* Return address for */
		WRITE_WORD (&toaplan2_shared_ram[offset-2],0x56);	/*   RTS instruction */
	}
	else
	{
		if (errorlog) fprintf(errorlog,"PC:%08x Writing %08x to HD647180 cpu shared ram status port\n",cpu_getpreviouspc(),mcu_data);
	}
	WRITE_WORD (&toaplan2_shared_ram[0x56],0x4e);	/* Return a RTS instruction */
	WRITE_WORD (&toaplan2_shared_ram[0x58],0x75);
}

static int ghox_shared_ram_r(int offset)
{
	/* Ghox 68K reads data from MCU shared RAM and writes it to main RAM.
	   It then subroutine jumps to main RAM and executes this code.
	   Here, i'm just returning a RTS instruction for now.
	   See above ghox_mcu_w routine.

	   Offset $56 and $58 is accessed around PC:F814

	   Offset $38 and $36 is accessed from around PC:DA7C
	   Offset $3c and $3a is accessed from around PC:2E3C
	   Offset $40 and $3E is accessed from around PC:103EE
	   Offset $44 and $42 is accessed from around PC:FB52
	   Offset $48 and $46 is accessed from around PC:6776
	*/

	int data = READ_WORD (&toaplan2_shared_ram[offset]);

	return data;
}
static void ghox_shared_ram_w(int offset, int data)
{
	WRITE_WORD (&toaplan2_shared_ram[offset],data);
}


static int shared_ram_r(int offset)
{
/*	Other games using a HD647180 cpu, have shared memory between the 68000
	and the HD647180 cpu. The 68000 reads the status of the HD647180
	via a location of the shared memory.
	Be careful not to return HD647180 cpu POST codes when 68000 is
	performing a RAM test on the shared memory.
*/
	int data;

	if (offset == 0x1000)
	{
		if (mcu_data == 0x800000aa) mcu_data = 0xff;		/* dogyuun */
		if (mcu_data == 0x00) mcu_data = 0x800000aa;		/* dogyuun */

		if (mcu_data == 0x8000ffaa) mcu_data = 0xffff;		/* fixeight */
		if (mcu_data == 0xffaa) mcu_data = 0x8000ffaa;		/* fixeight */
		if (mcu_data == 0xff00) mcu_data = 0xffaa;			/* fixeight */

		if (errorlog) fprintf(errorlog,"PC:%08x reading %08x from HD647180 cpu shared ram status port\n",cpu_getpreviouspc(),mcu_data);
		data = mcu_data & 0x0000ffff;
	}
	else
	{
		data = READ_WORD (&toaplan2_shared_ram[offset]);
	}
	return data;
}

static void shared_ram_w(int offset, int data)
{

	if (offset == 0x1000)
	{
		mcu_data = data;
		if (errorlog) fprintf(errorlog,"PC:%08x Writing command (%04x) to secondary CPU shared port\n",cpu_getpreviouspc(),mcu_data);
	}

	if (offset == 0x9e8)
	{
		WRITE_WORD (&toaplan2_shared_ram[offset + 2],data);
	}
	if (offset == 0xff8)
	{
		WRITE_WORD (&toaplan2_shared_ram[offset + 2],data);
		if (errorlog) fprintf(errorlog,"PC:%08x Writing  (%04x) to secondary CPU\n",cpu_getpreviouspc(),data);
		if ((data & 0xffff) == 0x81) data = 0x01;
	}
	WRITE_WORD (&toaplan2_shared_ram[offset],data);
}



static struct MemoryReadAddress tekipaki_readmem[] =
{
	{ 0x000000, 0x01ffff, MRA_ROM },
	{ 0x020000, 0x03ffff, MRA_ROM },				/* extra for Whoopee */
	{ 0x080000, 0x082fff, MRA_BANK1 },
	{ 0x0c0000, 0x0c0fff, paletteram_word_r },
	{ 0x140004, 0x140007, toaplan2_videoram_r },
	{ 0x14000c, 0x14000d, input_port_0_r },			/* VBlank */
	{ 0x180000, 0x180001, input_port_4_r },			/* Dip Switch A */
	{ 0x180010, 0x180011, input_port_5_r },			/* Dip Switch B */
	{ 0x180020, 0x180021, input_port_3_r },			/* Coin/System inputs */
	{ 0x180030, 0x180031, c2map_port_6_r },			/* CPU 2 busy and Territory Jumper block */
	{ 0x180050, 0x180051, input_port_1_r },			/* Player 1 controls */
	{ 0x180060, 0x180061, input_port_2_r },			/* Player 2 controls */
	{ -1 }
};

static struct MemoryWriteAddress tekipaki_writemem[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x020000, 0x03ffff, MWA_ROM },				/* extra for Whoopee */
	{ 0x080000, 0x082fff, MWA_BANK1 },
	{ 0x0c0000, 0x0c0fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x140000, 0x140001, toaplan2_voffs_w },
	{ 0x140004, 0x140007, toaplan2_videoram_w },	/* Tile/Sprite VideoRAM */
	{ 0x140008, 0x140009, toaplan2_scroll_reg_select_w },
	{ 0x14000c, 0x14000d, toaplan2_scroll_reg_data_w },
	{ 0x180040, 0x180041, toaplan2_coin_w },		/* Coin count/lock */
	{ 0x180070, 0x180071, toaplan2_sub_cpu_w },
	{ -1 }
};

static struct MemoryReadAddress ghox_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x083fff, MRA_BANK1 },
	{ 0x0c0000, 0x0c0fff, paletteram_word_r },
	{ 0x140004, 0x140007, toaplan2_videoram_r },
	{ 0x14000c, 0x14000d, input_port_0_r },			/* VBlank */
	{ 0x180000, 0x180001, ghox_mcu_r },				/* really part of shared RAM */
	{ 0x180006, 0x180007, input_port_4_r },			/* Dip Switch A */
	{ 0x180008, 0x180009, input_port_5_r },			/* Dip Switch B */
	{ 0x180010, 0x180011, input_port_3_r },			/* Coin/System inputs */
	{ 0x18000c, 0x18000d, input_port_1_r },			/* Player 1 controls */
	{ 0x18000e, 0x18000f, input_port_2_r },			/* Player 2 controls */
	{ 0x180500, 0x180fff, ghox_shared_ram_r },
	{ 0x18100c, 0x18100d, input_port_6_r },			/* Territory Jumper block */
	{ -1 }
};

static struct MemoryWriteAddress ghox_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x080000, 0x083fff, MWA_BANK1 },
	{ 0x0c0000, 0x0c0fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x140000, 0x140001, toaplan2_voffs_w },
	{ 0x140004, 0x140007, toaplan2_videoram_w },	/* Tile/Sprite VideoRAM */
	{ 0x140008, 0x140009, toaplan2_scroll_reg_select_w },
	{ 0x14000c, 0x14000d, toaplan2_scroll_reg_data_w },
	{ 0x180000, 0x180001, ghox_mcu_w },				/* really part of shared RAM */
	{ 0x180500, 0x180fff, ghox_shared_ram_w, &toaplan2_shared_ram },
	{ 0x181000, 0x181001, toaplan2_coin_w },
	{ -1 }
};

static struct MemoryReadAddress dogyuun_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK1 },
	{ 0x200010, 0x200011, input_port_1_r },			/* Player 1 controls */
	{ 0x200014, 0x200015, input_port_2_r },			/* Player 2 controls */
	{ 0x200018, 0x200019, input_port_3_r },			/* Coin/System inputs */
	{ 0x21e000, 0x21ffff, shared_ram_r },			/* 21f000 */
	{ 0x300004, 0x300007, toaplan2_videoram_r },	/* tile layers */
	{ 0x30000c, 0x30000d, input_port_0_r },			/* VBlank */
	{ 0x400000, 0x400fff, paletteram_word_r },
	/***** The following in 0x50000x are for video controller 2 ??? ******/
	{ 0x500004, 0x500007, toaplan2_videoram_r },	/* tile layers 2 */
	{ 0x700000, 0x700001, video_status_r },			/* test bit 8 */
	{ -1 }
};

static struct MemoryWriteAddress dogyuun_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK1 },
	{ 0x200008, 0x200009, OKIM6295_data_0_w },
	{ 0x20001c, 0x20001d, toaplan2_coin_w },
	{ 0x21e000, 0x21ffff, shared_ram_w, &toaplan2_shared_ram },
	{ 0x300000, 0x300001, toaplan2_voffs_w },		/* VideoRAM selector/offset */
	{ 0x300004, 0x300007, toaplan2_videoram_w },	/* Tile/Sprite VideoRAM */
	{ 0x300008, 0x300009, toaplan2_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	/***** The following in 0x50000x are for video controller 2 ??? ******/
	{ 0x500000, 0x500001, toaplan2_voffs_w },		/* VideoRAM selector/offset */
	{ 0x500004, 0x500007, toaplan2_videoram_w },	/* Tile/Sprite VideoRAM */
	{ 0x500008, 0x500009, toaplan2_scroll_reg_select_w },
	{ 0x50000c, 0x50000d, toaplan2_scroll_reg_data_w },
	{ -1 }
};

static struct MemoryReadAddress kbash_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK1 },
	{ 0x200000, 0x200001, kbash_sub_cpu_r },
	{ 0x200004, 0x200005, input_port_4_r },			/* Dip Switch A */
	{ 0x200006, 0x200007, input_port_5_r },			/* Dip Switch B */
	{ 0x200008, 0x200009, input_port_6_r },			/* Territory Jumper block */
	{ 0x208010, 0x208011, input_port_1_r },			/* Player 1 controls */
	{ 0x208014, 0x208015, input_port_2_r },			/* Player 2 controls */
	{ 0x208018, 0x208019, input_port_3_r },			/* Coin/System inputs */
	{ 0x300004, 0x300007, toaplan2_videoram_r },	/* tile layers */
	{ 0x30000c, 0x30000d, input_port_0_r },			/* VBlank */
	{ 0x400000, 0x400fff, paletteram_word_r },
	{ 0x700000, 0x700001, video_status_r },			/* test bit 8 */
	{ -1 }
};

static struct MemoryWriteAddress kbash_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK1 },
#if 0
	{ 0x200000, 0x200001, kbash_sub_cpu_w },		/* sound number to play */
	{ 0x200002, 0x200003, kbash_sub_cpu_w2 },		/* ??? */
#endif
	{ 0x20801c, 0x20801d, toaplan2_coin_w },
	{ 0x300000, 0x300001, toaplan2_voffs_w },
	{ 0x300004, 0x300007, toaplan2_videoram_w },
	{ 0x300008, 0x300009, toaplan2_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ -1 }
};

static struct MemoryReadAddress tatsujn2_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },
	{ 0x200004, 0x200007, toaplan2_videoram_r },
	{ 0x20000c, 0x20000d, input_port_0_r },
	{ 0x300000, 0x300fff, paletteram_word_r },
	{ 0x400000, 0x403fff, MRA_BANK2 },
	{ 0x500000, 0x50ffff, MRA_BANK3 },
	{ 0x600000, 0x600001, video_status_r },
	{ 0x700000, 0x700001, input_port_4_r },			/* Dip Switch A */
	{ 0x700004, 0x700005, input_port_5_r },			/* Dip Switch B */
	{ 0x700006, 0x700007, input_port_6_r },			/* Territory Jumper block */
	{ 0x700008, 0x700009, input_port_3_r },			/* Coin/System inputs */
	{ 0x70000a, 0x70000b, input_port_0_r },		/* ??? whats this ? */
	{ 0x700016, 0x700017, YM2151_status_port_0_r },
	{ -1 }
};

static struct MemoryWriteAddress tatsujn2_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1 },
	{ 0x200000, 0x200001, toaplan2_voffs_w },		/* VideoRAM selector/offset */
	{ 0x200004, 0x200007, toaplan2_videoram_w },	/* Tile/Sprite VideoRAM */
	{ 0x200008, 0x200009, toaplan2_scroll_reg_select_w },
	{ 0x20000c, 0x20000d, toaplan2_scroll_reg_data_w },
	{ 0x300000, 0x300fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x400000, 0x403fff, MWA_BANK2 },
	{ 0x500000, 0x50ffff, MWA_BANK3 },
//	{ 0x700010, 0x700011, OKIM6295_data_0_w },
	{ 0x700014, 0x700015, YM2151_register_port_0_w },
	{ 0x700016, 0x700017, YM2151_data_port_0_w },
	{ 0x70001e, 0x70001f, toaplan2_coin_w },		/* Coin count/lock */
	{ -1 }
};

static struct MemoryReadAddress pipibibs_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x082fff, MRA_BANK1 },
	{ 0x0c0000, 0x0c0fff, paletteram_word_r },
	{ 0x140004, 0x140007, toaplan2_videoram_r },
	{ 0x14000c, 0x14000d, input_port_0_r },			/* VBlank */
	{ 0x190000, 0x190fff, toaplan2_shared_r },
	{ 0x19c020, 0x19c021, input_port_4_r },			/* Dip Switch A */
	{ 0x19c024, 0x19c025, input_port_5_r },			/* Dip Switch B */
	{ 0x19c028, 0x19c029, input_port_6_r },			/* Territory Jumper block */
	{ 0x19c02c, 0x19c02d, input_port_3_r },			/* Coin/System inputs */
	{ 0x19c030, 0x19c031, input_port_1_r },			/* Player 1 controls */
	{ 0x19c034, 0x19c035, input_port_2_r },			/* Player 2 controls */
	{ -1 }
};

static struct MemoryWriteAddress pipibibs_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x080000, 0x082fff, MWA_BANK1 },
	{ 0x0c0000, 0x0c0fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x140000, 0x140001, toaplan2_voffs_w },
	{ 0x140004, 0x140007, toaplan2_videoram_w },	/* Tile/Sprite VideoRAM */
	{ 0x140008, 0x140009, toaplan2_scroll_reg_select_w },
	{ 0x14000c, 0x14000d, toaplan2_scroll_reg_data_w },
	{ 0x190000, 0x190fff, toaplan2_shared_w, &toaplan2_shared_ram },
	{ 0x19c01c, 0x19c01d, toaplan2_coin_w },		/* Coin count/lock */
	{ -1 }
};

static struct MemoryReadAddress fixeight_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK1 },
	{ 0x200000, 0x200001, input_port_4_r },			/* Dip Switch A */
	{ 0x200004, 0x200005, input_port_5_r },			/* Dip Switch B */
	{ 0x200010, 0x200011, input_port_3_r },			/* Coin/System inputs */
	{ 0x280000, 0x28dfff, MRA_BANK2 },				/* part of shared ram ? */
	{ 0x28e000, 0x28ffff, shared_ram_r },
	{ 0x300004, 0x300007, toaplan2_videoram_r },
	{ 0x30000c, 0x30000d, input_port_0_r },
	{ 0x400000, 0x400fff, paletteram_word_r },
	{ 0x500000, 0x501fff, MRA_BANK4 },
	{ 0x502000, 0x6021ff, MRA_BANK5 },
	{ 0x503000, 0x6031ff, MRA_BANK6 },
	{ 0x600000, 0x60ffff, MRA_BANK7 },
	{ 0x800000, 0x800001, video_status_r },
	{ -1 }
};

static struct MemoryWriteAddress fixeight_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK1 },
	{ 0x20001c, 0x20001d, toaplan2_coin_w },		/* Coin count/lock */
	{ 0x280000, 0x28dfff, MWA_BANK2 },				/* part of shared ram ? */
	{ 0x28e000, 0x28ffff, shared_ram_w, &toaplan2_shared_ram },
	{ 0x300000, 0x300001, toaplan2_voffs_w },		/* VideoRAM selector/offset */
	{ 0x300004, 0x300007, toaplan2_videoram_w },	/* Tile/Sprite VideoRAM */
	{ 0x300008, 0x300009, toaplan2_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x500000, 0x501fff, MWA_BANK4 },
	{ 0x502000, 0x6021ff, MWA_BANK5 },
	{ 0x503000, 0x6031ff, MWA_BANK6 },
	{ 0x600000, 0x60ffff, MWA_BANK7 },
	{ -1 }
};

static struct MemoryReadAddress snowbro2_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },
	{ 0x300004, 0x300007, toaplan2_videoram_r },	/* tile layers */
	{ 0x30000c, 0x30000d, input_port_0_r },			/* VBlank */
	{ 0x400000, 0x400fff, paletteram_word_r },
	{ 0x500002, 0x500003, YM2151_status_port_0_r },
	{ 0x600000, 0x600001, OKIM6295_status_0_r },
	{ 0x700000, 0x700001, input_port_8_r },			/* Territory Jumper block */
	{ 0x700004, 0x700005, input_port_6_r },			/* Dip Switch A */
	{ 0x700008, 0x700009, input_port_7_r },			/* Dip Switch B */
	{ 0x70000c, 0x70000d, input_port_1_r },			/* Player 1 controls */
	{ 0x700010, 0x700011, input_port_2_r },			/* Player 2 controls */
	{ 0x700014, 0x700015, input_port_3_r },			/* Player 3 controls */
	{ 0x700018, 0x700019, input_port_4_r },			/* Player 4 controls */
	{ 0x70001c, 0x70001d, input_port_5_r },			/* Coin/System inputs */
	{ -1 }
};

static struct MemoryWriteAddress snowbro2_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1 },
	{ 0x300000, 0x300001, toaplan2_voffs_w },		/* VideoRAM selector/offset */
	{ 0x300004, 0x300007, toaplan2_videoram_w },	/* Tile/Sprite VideoRAM */
	{ 0x300008, 0x300009, toaplan2_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x500000, 0x500001, YM2151_register_port_0_w },
	{ 0x500002, 0x500003, YM2151_data_port_0_w },
	{ 0x600000, 0x600001, OKIM6295_data_0_w },
	{ 0x700030, 0x700031, oki_bankswitch_w },		/* Sample bank switch */
	{ 0x700034, 0x700035, toaplan2_coin_w },		/* Coin count/lock */
	{ -1 }
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xe000, 0xe000, YM3812_status_port_0_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM, &toaplan2_shared_ram },
	{ 0xe000, 0xe000, YM3812_control_port_0_w },
	{ 0xe001, 0xe001, YM3812_write_port_0_w },
	{ -1 }  /* end of table */
};

#if HD64X180
static struct MemoryReadAddress hd641180_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xfe00, 0xffff, MRA_RAM },			/* Internal 512 bytes of RAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress hd641180_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xfe00, 0xffff, MWA_RAM },			/* Internal 512 bytes of RAM */
	{ -1 }  /* end of table */
};
#endif

/*****************************************************************************
	Input Port definitions
*****************************************************************************/

#define  TOAPLAN2_PLAYER_INPUT( player, button3 )								\
	PORT_START 																	\
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | player )	\
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | player )	\
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | player )	\
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | player )	\
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 | player)						\
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 | player)						\
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, button3     )								\
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

#define  TOAPLAN2_SYSTEM_INPUTS						\
	PORT_START										\
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_COIN3 ) 	\
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_TILT )	\
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_UNKNOWN )	\
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN1 )	\
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_COIN2 )	\
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_START1 )	\
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START2 )	\
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

#define  TOAPLAN2_DSW_A												\
	PORT_START														\
	PORT_DIPNAME( 0x01,	0x00, DEF_STR( Unused ) )					\
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )						\
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )						\
	PORT_DIPNAME( 0x02,	0x00, DEF_STR( Flip_Screen ) )				\
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )						\
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )						\
	PORT_SERVICE( 0x04,	IP_ACTIVE_HIGH )		/* Service Mode */	\
	PORT_DIPNAME( 0x08,	0x00, DEF_STR( Demo_Sounds ) )				\
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )						\
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )						\
	PORT_DIPNAME( 0x30,	0x00, DEF_STR( Coin_A ) )					\
	PORT_DIPSETTING(	0x30, DEF_STR( 4C_1C ) )					\
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )					\
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )					\
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )					\
	PORT_DIPNAME( 0xc0,	0x00, DEF_STR( Coin_B ) )					\
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_2C ) )					\
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ) )					\
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_4C ) )					\
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_6C ) )					\
/*	Non-European territories coin setups							\
	PORT_DIPNAME( 0x30,	0x00, DEF_STR( Coin_A ) )					\
	PORT_DIPSETTING(	0x20, DEF_STR( 2C_1C ) )					\
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )					\
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_3C ) )					\
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )					\
	PORT_DIPNAME( 0xc0,	0x00, DEF_STR( Coin_B ) )					\
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ) )					\
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )					\
	PORT_DIPSETTING(	0xc0, DEF_STR( 2C_3C ) )					\
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )					\
*/




INPUT_PORTS_START( tekipaki )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	TOAPLAN2_DSW_A

	PORT_START		/* (5) DSWB */
	PORT_DIPNAME( 0x03,	0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Medium" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x04,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40,	0x00, "Game Mode" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x40, "Stop" )
	PORT_DIPNAME( 0x80,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x0f,	0x02, "Territory" )
	PORT_DIPSETTING(	0x02, "Europe" )
	PORT_DIPSETTING(	0x01, "USA" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x03, "Hong Kong" )
	PORT_DIPSETTING(	0x05, "Taiwan" )
	PORT_DIPSETTING(	0x04, "Korea" )
	PORT_DIPSETTING(	0x07, "USA (Romstar)" )
	PORT_DIPSETTING(	0x08, "Hong Kong (Honest Trading Co.)" )
	PORT_DIPSETTING(	0x06, "Taiwan (Spacy Co. Ltd)" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ghox )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	TOAPLAN2_DSW_A

	PORT_START		/* (5) DSWB */
	PORT_DIPNAME( 0x03,	0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Medium" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c,	0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "100K, every 200K" )
	PORT_DIPSETTING(	0x04, "100K, every 300K" )
	PORT_DIPSETTING(	0x08, "100K" )
	PORT_DIPSETTING(	0x0c, "None" )
	PORT_DIPNAME( 0x30,	0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "5" )
	PORT_DIPNAME( 0x40,	0x00, "Game Mode" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x40, "Debug" )
	PORT_DIPNAME( 0x80,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x0f,	0x02, "Territory" )
	PORT_DIPSETTING(	0x02, "Europe" )
	PORT_DIPSETTING(	0x01, "USA" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x04, "Korea" )
	PORT_DIPSETTING(	0x03, "Hong Kong (Honest Trading Co." )
	PORT_DIPSETTING(	0x05, "Taiwan" )
	PORT_DIPSETTING(	0x06, "Spain & Portugal (APM Electronics SA)" )
	PORT_DIPSETTING(	0x07, "Italy (Star Electronica SRL)" )
	PORT_DIPSETTING(	0x08, "UK (JP Leisure Ltd)" )
	PORT_DIPSETTING(	0x0a, "Europe (Nova Apparate GMBH & Co)" )
	PORT_DIPSETTING(	0x0d, "Europe (Taito Corporation Japan)" )
	PORT_DIPSETTING(	0x09, "USA (Romstar)" )
	PORT_DIPSETTING(	0x0b, "USA (Taito America Corporation)" )
	PORT_DIPSETTING(	0x0c, "USA (Taito Corporation Japan)" )
	PORT_DIPSETTING(	0x0e, "Japan (Taito Corporation)" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( kbash )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_BUTTON3 | IPF_PLAYER1 )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_BUTTON3 | IPF_PLAYER2 )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, "Continue Mode" )
	PORT_DIPSETTING(		0x0000, "Normal" )
	PORT_DIPSETTING(		0x0001, "Discount" )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(		0x0030, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(		0x0020, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x00c0,	0x0000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(		0x0080, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(		0x00c0, DEF_STR( 1C_6C ) )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )
/*	Non-European territories coin setup
	PORT_DIPNAME( 0x0030, 0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(		0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x0030, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0,	0x0000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(		0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x00c0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_2C ) )
*/

	PORT_START		/* (5) DSWB */
	PORT_DIPNAME( 0x0003,	0x0000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(		0x0001, "Easy" )
	PORT_DIPSETTING(		0x0000, "Medium" )
	PORT_DIPSETTING(		0x0002, "Hard" )
	PORT_DIPSETTING(		0x0003, "Hardest" )
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0000, "100K, every 400K" )
	PORT_DIPSETTING(		0x0004, "100K" )
	PORT_DIPSETTING(		0x0008, "200K" )
	PORT_DIPSETTING(		0x000c, "None" )
	PORT_DIPNAME( 0x0030,	0x0020, DEF_STR( Lives ) )
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0000, "2" )
	PORT_DIPSETTING(		0x0020, "3" )
	PORT_DIPSETTING(		0x0010, "4" )
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0300,	0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(		0x0000, "0" )
	PORT_DIPSETTING(		0x0100, "1" )
	PORT_DIPSETTING(		0x0200, "2" )
	PORT_DIPSETTING(		0x0300, "3" )
	PORT_BIT( 0xfc00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x000f,	0x000a, "Territory" )
	PORT_DIPSETTING(		0x000a, "Europe" )
	PORT_DIPSETTING(		0x0009, "USA" )
	PORT_DIPSETTING(		0x0000, "Japan" )
	PORT_DIPSETTING(		0x0003, "Korea" )
	PORT_DIPSETTING(		0x0004, "Hong Kong" )
	PORT_DIPSETTING(		0x0007, "Taiwan" )
	PORT_DIPSETTING(		0x0006, "South East Asia" )
	PORT_DIPSETTING(		0x0002, "Europe, USA (Atari License)" )
	PORT_DIPSETTING(		0x0001, "USA, Europe (Atari License)" )
	PORT_BIT( 0xfff0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( pipibibs )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	TOAPLAN2_DSW_A

	PORT_START		/* (5) DSWB */
	PORT_DIPNAME( 0x03,	0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Medium" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c,	0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x04, "150K, every 200K" )
	PORT_DIPSETTING(	0x00, "200K, every 300K" )
	PORT_DIPSETTING(	0x08, "200K" )
	PORT_DIPSETTING(	0x0c, "None" )
	PORT_DIPNAME( 0x30,	0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "5" )
	PORT_DIPNAME( 0x40,	0x00, "Game Mode" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x40, "Debug" )
	PORT_DIPNAME( 0x80,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x07,	0x06, "Territory" )
	PORT_DIPSETTING(	0x06, "Europe" )
	PORT_DIPSETTING(	0x04, "USA" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x02, "Hong Kong" )
	PORT_DIPSETTING(	0x03, "Taiwan" )
	PORT_DIPSETTING(	0x01, "Asia" )
	PORT_DIPSETTING(	0x07, "Europe (Nova Apparate GMBH & Co)" )
	PORT_DIPSETTING(	0x05, "USA (Romstar)" )
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( whoopee )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x01,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02,	0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x08,	0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30,	0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0,	0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )
/*	Non-European territories coin setups
	PORT_DIPNAME( 0x30,	0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0,	0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_6C ) )
*/

	PORT_START		/* (5) DSWB */
	PORT_DIPNAME( 0x03,	0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Medium" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c,	0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x04, "150K, every 200K" )
	PORT_DIPSETTING(	0x00, "200K, every 300K" )
	PORT_DIPSETTING(	0x08, "200K" )
	PORT_DIPSETTING(	0x0c, "None" )
	PORT_DIPNAME( 0x30,	0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "5" )
	PORT_DIPNAME( 0x40,	0x00, "Game Mode" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x40, "Debug" )
	PORT_DIPNAME( 0x80,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x07,	0x00, "Territory" )
	PORT_DIPSETTING(	0x06, "Europe" )
	PORT_DIPSETTING(	0x04, "USA" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x02, "Hong Kong" )
	PORT_DIPSETTING(	0x03, "Taiwan" )
	PORT_DIPSETTING(	0x01, "Asia" )
	PORT_DIPSETTING(	0x07, "Europe (Nova Apparate GMBH & Co)" )
	PORT_DIPSETTING(	0x05, "USA (Romstar)" )
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* bit 0x10 sound ready */
INPUT_PORTS_END

INPUT_PORTS_START( snowbro2 )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER3, IPT_START3 )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER4, IPT_START4 )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (6) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, "Continue Mode" )
	PORT_DIPSETTING(		0x0000, "Normal" )
	PORT_DIPSETTING(		0x0001, "Discount" )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(		0x0030, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(		0x0020, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x00c0,	0x0000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(		0x0080, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(		0x00c0, DEF_STR( 1C_6C ) )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )
/*	Non-European territories coin setups
	PORT_DIPNAME( 0x0030, 0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(		0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x0030, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0,	0x0000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(		0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x00c0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_2C ) )
*/

	PORT_START		/* (7) DSWB */
	PORT_DIPNAME( 0x0003,	0x0000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(		0x0001, "Easy" )
	PORT_DIPSETTING(		0x0000, "Medium" )
	PORT_DIPSETTING(		0x0002, "Hard" )
	PORT_DIPSETTING(		0x0003, "Hardest" )
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0004, "100K, every 500K" )
	PORT_DIPSETTING(		0x0000, "100K" )
	PORT_DIPSETTING(		0x0008, "200K" )
	PORT_DIPSETTING(		0x000c, "None" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "4" )
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Maximum Players" )
	PORT_DIPSETTING(		0x0080, "2" )
	PORT_DIPSETTING(		0x0000, "4" )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (8) Territory Jumper block */
	PORT_DIPNAME( 0x1c00,	0x0800, "Territory" )
	PORT_DIPSETTING(		0x0800, "Europe" )
	PORT_DIPSETTING(		0x0400, "USA" )
	PORT_DIPSETTING(		0x0000, "Japan" )
	PORT_DIPSETTING(		0x0c00, "Korea" )
	PORT_DIPSETTING(		0x1000, "Hong Kong" )
	PORT_DIPSETTING(		0x1400, "Taiwan" )
	PORT_DIPSETTING(		0x1800, "South East Asia" )
	PORT_DIPSETTING(		0x1c00, DEF_STR( Unused ) )
	PORT_DIPNAME( 0x2000,	0x0000, "Show All Rights Reserved" )
	PORT_DIPSETTING(		0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(		0x2000, DEF_STR( Yes ) )
	PORT_BIT( 0xc3ff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END




#define TILELAYOUT(NUM) static struct GfxLayout tilelayout_##NUM =			\
{																			\
	16,16,	/* 16x16 */														\
	NUM,	/* Number of tiles */											\
	4,		/* 4 bits per pixel */											\
	{ 8*NUM*64+8, 8*NUM*64, 8, 0 },											\
	{ 0, 1, 2, 3, 4, 5, 6, 7,												\
		8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },	\
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,						\
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },			\
	8*4*16																	\
}

TILELAYOUT(8192);
TILELAYOUT(16384);
TILELAYOUT(24576);
TILELAYOUT(32768);
TILELAYOUT(49152);
TILELAYOUT(65536);


#define SPRITELAYOUT(NUM) static struct GfxLayout spritelayout_##NUM =		\
{																			\
	8,8,	/* 8x8 */														\
	NUM,	/* Number of 8x8 sprites */										\
	4,		/* 4 bits per pixel */											\
	{ 8*NUM*16+8, 8*NUM*16, 8, 0 },											\
	{ 0, 1, 2, 3, 4, 5, 6, 7 },												\
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },						\
	8*16																	\
}

SPRITELAYOUT(32768);
SPRITELAYOUT(65536);
SPRITELAYOUT(98304);
SPRITELAYOUT(131072);
SPRITELAYOUT(196608);
SPRITELAYOUT(262144);

static struct GfxDecodeInfo rom_1mb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout_8192,			0, 128 },
	{ REGION_GFX1, 0, &spritelayout_32768,		0,  64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rom_2mb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout_16384,			0, 128 },
	{ REGION_GFX1, 0, &spritelayout_65536,		0,  64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rom_3mb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout_24576,			0, 128 },
	{ REGION_GFX1, 0, &spritelayout_98304,		0,  64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rom_4mb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout_32768,			0, 128 },
	{ REGION_GFX1, 0, &spritelayout_131072,		0,  64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rom_6mb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout_49152,			0, 128 },
	{ REGION_GFX1, 0, &spritelayout_196608,		0,  64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rom_8mb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout_65536,			0, 128 },
	{ REGION_GFX1, 0, &spritelayout_262144,		0,  64 },
	{ -1 } /* end of array */
};


static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	1,				/* 1 chip  */
	3500000,		/* 3.5 MHz */
	{ 100 },		/* volume */
	{ irqhandler },
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.58 MHZ ? */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 22050 },		/* frequency (Hz) */
	{ 2 },			/* memory region */
	{ 47 }
};

static struct MachineDriver machine_driver_tekipaki =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			tekipaki_readmem,tekipaki_writemem,0,0,
			toaplan2_interrupt,1
		},
#if HD64X180
		{
			CPU_Z80,			/* HD647180 CPU actually */
			27000000/8,			/* 3.37Mh ??? */
			hd647180_readmem,hd647180_writemem,0,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	rom_1mb_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	0,
	toaplan2_vh_start,
	toaplan2_vh_stop,
	toaplan2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	}
};

static struct MachineDriver machine_driver_ghox =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			ghox_readmem,ghox_writemem,0,0,
			toaplan2_interrupt,1
		},
#if HD64X180
		{
			CPU_Z80,			/* HD647180 CPU actually */
			27000000/8,			/* 3.37Mh ??? */
			hd647180_readmem,hd647180_writemem,0,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	rom_1mb_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	0,
	toaplan2_vh_start,
	toaplan2_vh_stop,
	toaplan2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};

static struct MachineDriver machine_driver_dogyuun =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,
			dogyuun_readmem,dogyuun_writemem,0,0,
			toaplan2_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	rom_6mb_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	0,
	toaplan2_vh_start,
	toaplan2_vh_stop,
	toaplan2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_kbash =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,
			kbash_readmem,kbash_writemem,0,0,
			toaplan2_interrupt,1
		},
#if HD64X180
		{
			CPU_Z80,			/* HD64x180 CPU actually */
			3500000,
			hd641180_readmem,hd641180_writemem,0,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	rom_8mb_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	0,
	toaplan2_vh_start,
	toaplan2_vh_stop,
	toaplan2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_tatsujn2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,
			tatsujn2_readmem,tatsujn2_writemem,0,0,
			toaplan2_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	rom_2mb_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	0,
	toaplan2_vh_start,
	toaplan2_vh_stop,
	toaplan2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_pipibibs =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			pipibibs_readmem,pipibibs_writemem,0,0,
			toaplan2_interrupt,1
		},
		{
			CPU_Z80,
			3500000,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	rom_2mb_gfxdecodeinfo,
	(128*16), (128*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	0,
	toaplan2_vh_start,
	toaplan2_vh_stop,
	toaplan2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	}
};

static struct MachineDriver machine_driver_whoopee =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,		/* ??? */
			tekipaki_readmem,tekipaki_writemem,0,0,
			toaplan2_interrupt,1
		},
		{
			CPU_Z80,		/* This should probably be a HD647180 */
			3500000,		/* ??? */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	rom_2mb_gfxdecodeinfo,
	(128*16), (128*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	0,
	toaplan2_vh_start,
	toaplan2_vh_stop,
	toaplan2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	}
};

static struct MachineDriver machine_driver_fixeight =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,
			fixeight_readmem,fixeight_writemem,0,0,
			toaplan2_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	rom_4mb_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	0,
	toaplan2_vh_start,
	toaplan2_vh_stop,
	toaplan2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};

static struct MachineDriver machine_driver_snowbro2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,
			snowbro2_readmem,snowbro2_writemem,0,0,
			toaplan2_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	rom_3mb_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	0,
	toaplan2_vh_start,
	toaplan2_vh_stop,
	toaplan2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tekipaki )
	ROM_REGION( 0x20000, REGION_CPU1 )				/* Main 68K code */
	ROM_LOAD_EVEN( "tp020-1.bin",  0x00000, 0x10000, 0xd8420bd5 )
	ROM_LOAD_ODD ( "tp020-2.bin",  0x00000, 0x10000, 0x7222de8e )

	ROM_REGION( 0x10000, REGION_CPU2 )	/* Sound 647180 code */
	/* sound CPU is a HD647180 (Z180) with internal ROM - not yet supported */
	ROM_LOAD( "hd647180.020",  0x00000, 0x08000, 0x00000000 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tp020-4.bin", 0x000000, 0x80000, 0x3ebbe41e )
	ROM_LOAD( "tp020-3.bin", 0x080000, 0x80000, 0x2d5e2201 )
ROM_END

ROM_START( ghox )
	ROM_REGION( 0x40000, REGION_CPU1 )				/* Main 68K code */
	ROM_LOAD_EVEN( "tp021-01.u10",  0x00000, 0x20000, 0x9e56ac67 )
	ROM_LOAD_ODD ( "tp021-02.u11",  0x00000, 0x20000, 0x15cac60f )

	ROM_REGION( 0x10000, REGION_CPU2 )	/* Sound 647180 code */
	/* sound CPU is a HD647180 (Z180) with internal ROM - not yet supported */
	ROM_LOAD( "hd647180.021",  0x00000, 0x08000, 0x00000000 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tp021-03.u36", 0x000000, 0x80000, 0xa15d8e9d )
	ROM_LOAD( "tp021-04.u37", 0x080000, 0x80000, 0x26ed1c9a )
ROM_END

ROM_START( dogyuun )
	ROM_REGION( 0x080000, REGION_CPU1 )			/* Main 68K code */
	ROM_LOAD_WIDE( "tp022_1.r16",  0x00000, 0x80000, 0x72f18907 )

	ROM_REGION( 0x10000, REGION_CPU2 )				/* Sound CPU code */
	/* sound CPU is a Toaplan marked chip ??? */
	ROM_LOAD( "tp022.mcu",  0x00000, 0x08000, 0x00000000 )

	ROM_REGION( 0x600000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD_GFX_SWAP( "tp022_3.r16", 0x000000,  0x100000, 0x191b595f )
	ROM_LOAD_GFX_SWAP( "tp022_5.r16", 0x100000,  0x200000, 0xd4c1db45 )
	ROM_LOAD_GFX_SWAP( "tp022_4.r16", 0x300000,  0x100000, 0xd58d29ca )
	ROM_LOAD_GFX_SWAP( "tp022_6.r16", 0x400000,  0x200000, 0xd48dc74f )

	ROM_REGION( 0x40000, REGION_USER1 )	/* what is this */
	ROM_LOAD( "tp022_2.rom", 0x000000, 0x40000, 0x043271b3 )
ROM_END

ROM_START( kbash )
	ROM_REGION( 0x080000, REGION_CPU1 )			/* Main 68K code */
	ROM_LOAD_WIDE_SWAP( "kbash01.bin",  0x00000, 0x80000, 0x2965f81d )

	ROM_REGION( 0x10000, REGION_CPU2 )				/* Sound HD64x180 code */
	ROM_LOAD( "kbash02.bin", 0x000000, 0x8000, 0x4cd882a1 )

	ROM_REGION( 0x800000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "kbash03.bin", 0x000000,  0x200000, 0x32ad508b )
	ROM_LOAD( "kbash05.bin", 0x200000,  0x200000, 0xb84c90eb )
	ROM_LOAD( "kbash04.bin", 0x400000,  0x200000, 0xe493c077 )
	ROM_LOAD( "kbash06.bin", 0x600000,  0x200000, 0x9084b50a )

	ROM_REGION( 0x40000, REGION_USER1 )	/* what is this */
	ROM_LOAD( "kbash07.bin", 0x000000, 0x40000, 0x3732318f )
ROM_END

ROM_START( tatsujn2 )
	ROM_REGION( 0x080000, REGION_CPU1 )			/* Main 68K code */
	ROM_LOAD_WIDE( "tsj2rom1.bin",  0x00000, 0x80000, 0xf5cfe6ee )

	ROM_REGION( 0x10000, REGION_CPU2 )				/* Sound CPU code */
	/* sound CPU is a Toaplan marked chip ??? */
//	ROM_LOAD( "tp024.mcu",  0x00000, 0x08000, 0x00000000 )

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tsj2rom4.bin", 0x000000, 0x100000, 0x805c449e )
	ROM_LOAD( "tsj2rom3.bin", 0x100000, 0x100000, 0x47587164 )

	ROM_REGION( 0x80000, REGION_USER1 )	/* what is this */
	ROM_LOAD( "tsj2rom2.bin", 0x00000, 0x80000, 0xf2f6cae4 )
ROM_END

ROM_START( pipibibs )
	ROM_REGION( 0x40000, REGION_CPU1 )				/* Main 68K code */
	ROM_LOAD_EVEN( "tp025-1.bin",  0x00000, 0x20000, 0xb2ea8659 )
	ROM_LOAD_ODD ( "tp025-2.bin",  0x00000, 0x20000, 0xdc53b939 )

	ROM_REGION( 0x10000, REGION_CPU2 )				/* Sound Z80 code */
	ROM_LOAD( "tp025-5.bin",   0x0000, 0x8000, 0xbf8ffde5 )

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tp025-4.bin", 0x000000, 0x100000, 0xab97f744 )
	ROM_LOAD( "tp025-3.bin", 0x100000, 0x100000, 0x7b16101e )
ROM_END

ROM_START( whoopee )
	ROM_REGION( 0x40000, REGION_CPU1 )				/* Main 68K code */
	ROM_LOAD_EVEN( "whoopee.1",  0x00000, 0x20000, 0x28882e7e )
	ROM_LOAD_ODD ( "whoopee.2",  0x00000, 0x20000, 0x6796f133 )

	ROM_REGION( 0x10000, REGION_CPU2 )				/* Sound Z80 code */
	/* sound CPU is a HD647180 (Z180) with internal ROM - not yet supported */
	/* use the Z80 version from the bootleg Pipi & Bibis set for now */
	ROM_LOAD( "hd647180.025",   0x0000, 0x04000, BADCRC( 0x7670d612 ) )

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tp025-4.bin", 0x000000, 0x100000, 0xab97f744 )
	ROM_LOAD( "tp025-3.bin", 0x100000, 0x100000, 0x7b16101e )
ROM_END

ROM_START( pipibibi )
	ROM_REGION( 0x40000, REGION_CPU1 )				/* Main 68K code */
	ROM_LOAD_EVEN( "ppbb05.bin",  0x00000, 0x20000, 0x3d51133c )
	ROM_LOAD_ODD ( "ppbb06.bin",  0x00000, 0x20000, 0x14c92515 )

	ROM_REGION( 0x10000, REGION_CPU2 )				/* Sound Z80 code */
	ROM_LOAD( "ppbb08.bin",   0x0000, 0x04000, 0x7670d612 )

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD_GFX_EVEN( "ppbb02.bin", 0x100000, 0x080000, 0x8bfcdf87 )
	ROM_LOAD_GFX_ODD ( "ppbb01.bin", 0x100000, 0x080000, 0x0fcae44b )
	ROM_LOAD_GFX_EVEN( "ppbb03.bin", 0x000000, 0x080000, 0xabdd2b8b )
	ROM_LOAD_GFX_ODD ( "ppbb04.bin", 0x000000, 0x080000, 0x70faa734 )

	ROM_REGION( 0x08000, REGION_USER1 )	/* what is this */
	ROM_LOAD( "ppbb07.bin",   0x0000, 0x08000, 0x456dd16e )
ROM_END

ROM_START( fixeight )
	ROM_REGION( 0x080000, REGION_CPU1 )			/* Main 68K code */
	ROM_LOAD_WIDE_SWAP( "tp026.01",  0x00000, 0x80000, 0xf7b1746a )

	ROM_REGION( 0x10000, REGION_CPU2 )				/* Sound CPU code */
	/* sound CPU is a Toaplan marked chip, (TS-001-Turbo  TOA PLAN) */
	ROM_LOAD( "tp026.mcu",  0x00000, 0x08000, 0x00000000 )

	ROM_REGION( 0x400000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tp026.03", 0x000000, 0x200000, 0xe5578d98 )
	ROM_LOAD( "tp026.04", 0x200000, 0x200000, 0xb760cb53 )

	ROM_REGION( 0x80000, REGION_USER1 )	/* what is this */
	ROM_LOAD( "tp026.02",  0x00000, 0x80000, 0x76fab9f4 )
ROM_END

ROM_START( snowbro2 )
	ROM_REGION( 0x080000, REGION_CPU1 )			/* Main 68K code */
	ROM_LOAD_WIDE_SWAP( "pro-4",  0x00000, 0x80000, 0x4c7ee341 )

	ROM_REGION( 0x300000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "rom2-l", 0x000000,  0x100000, 0xe9d366a9 )
	ROM_LOAD( "rom2-h", 0x100000,  0x080000, 0x9aab7a62 )
	ROM_LOAD( "rom3-l", 0x180000,  0x100000, 0xeb06e332 )
	ROM_LOAD( "rom3-h", 0x280000,  0x080000, 0xdf4a952a )

	ROM_REGION( 0x80000, REGION_USER1 )	/* what is this */
	ROM_LOAD( "rom4", 0x000000, 0x80000, 0x638f341e )
ROM_END


/*
Toaplan board game numbers:
Teki Paki     TP-020
Ghox          TP-021
Dogyuun       TP-022
Knuckle Bash  TP-023
Tatsujin 2    TP-024
Pipi & Bibis  TP-025
FixEight      TP-026
Snow Bros. 2  TP-???
*/

GAMEX( 1991, tekipaki, 0,        tekipaki, tekipaki, toaplan2, ROT0,       "Toaplan", "Teki Paki", GAME_NO_SOUND )
GAMEX( 1991, ghox,     0,        ghox,     ghox,     toaplan2, ROT270,     "Toaplan", "Ghox", GAME_NOT_WORKING )
GAMEX( 1992, dogyuun,  0,        dogyuun,  tekipaki, toaplan3, ROT270,     "Toaplan", "Dogyuun", GAME_NOT_WORKING )
GAMEX( 1993, kbash,    0,        kbash,    kbash,    toaplan2, ROT0_16BIT, "Toaplan", "Knuckle Bash", GAME_NO_SOUND )
GAMEX( 1992, tatsujn2, 0,        tatsujn2, tekipaki, toaplan3, ROT270,     "Toaplan", "Truxton II / Tatsujin II (Japan)", GAME_NOT_WORKING )
GAME ( 1991, pipibibs, 0,        pipibibs, pipibibs, pipibibs, ROT0,       "Toaplan", "Pipi & Bibis / Whoopee (Japan)" )
GAMEX( 1991, pipibibi, pipibibs, pipibibs, pipibibs, pipibibs, ROT0,       "bootleg ?", "Pipi & Bibis / Whoopee (Japan) [bootleg ?]", GAME_NOT_WORKING )
	/* Whoopee machine to be changed to Teki Paki when hd647180 is dumped */
GAME ( 1991, whoopee,  pipibibs, whoopee,  whoopee,  pipibibs, ROT0,       "Toaplan", "Whoopee (Japan) / Pipi & Bibis (World)" )
GAMEX( 1992, fixeight, 0,        fixeight, tekipaki, toaplan3, ROT270,     "Toaplan", "FixEight", GAME_NOT_WORKING )
GAME ( 1994, snowbro2, 0,        snowbro2, snowbro2, snowbro2, ROT0_16BIT, "[Toaplan] Hanafram", "Snow Bros. 2 - With New Elves" )
