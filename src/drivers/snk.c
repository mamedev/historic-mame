/*
snk.c
various SNK triple Z80 games

To Do:
- add remaining alternate ROM sets (Bermuda Triangle, Iron Tank)
- finish input ports for the sports games

****************************************************************************
Sound is still messed up; because of incorrect emulation of the IRQ control
register at 0xf800.

ym3526
Aso, Tank

ym3526x2
Athena, Ikari, Fighting Golf

ym3526 + y8950
Victory Road, Psycho Soldier, Bermuda Triangle, Touchdown Fever, Guerilla War

ym3812 + y8950
Legofair, Chopper1

y8950
Fighting Soccer

(in alphabetical order)
#define CREDITS	"Ernesto Corvi\n" \
				"Carlos A. Lozano\n" \
				"Jarek Parchanski\n" \
				"Phil Stroffolino\n" \
				"Victor Trucco\n" \
				"Marco Cassili\n"

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

enum {
	MEM_COLOR = 3,
	MEM_GFX_CHARS,
	MEM_GFX_TILES,
	MEM_GFX_SPRITES,
	MEM_GFX_BIGSPRITES,
	MEM_SAMPLES
};

extern void snk_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void aso_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void ikari_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

extern int snk_vh_start( void );
extern void snk_vh_stop( void );

extern void tnk3_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void ikari_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void tdfever_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void gwar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void psychos_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/*********************************************************************/

static struct YM3526interface ym3526_interface = {
	1,			/* number of chips */
	4000000,	/* 4 MHz */
	{ 50 }		/* mixing level */
};
static struct YM3526interface dual_ym3526_interface = {
	2,			/* number of chips */
	4000000,	/* 4 MHz */
	{ 50,50 }	/* mixing level */
};
static struct Y8950interface y8950_interface = {
	1,			/* number of chips */
	4000000,	/* 4 MHz */
	{ 50 },		/* mixing level */
	{ 0 },
	{ MEM_SAMPLES }	/* memory region */
};
static struct YM3812interface ym3812_interface = {
	1,			/* number of chips */
	4000000,	/* 4 MHz */
	{ 50,50 }	/* mixing level */
};

static int snk_soundcommand;

static int snk_irq_status;
/*
	This 4 bit register is mapped at 0xf800.

	Writes to this register always contain 0x0f in the lower nibble.
	The upper nibble contains a mask, which is typically used to clear a
	single bit.

	bit 0:	this is related to the status of the sound chips (timer?)
	bit 1:	cleared at end of interrupt handler (irq ack?)
	bit 2:	cleared after new sound command is processed
	bit 3:	indicates that a sound command has been sent by the main CPU;
			cleared after the sound command has been read
*/

int sound_interrupt( void ){
	if( snk_irq_status & 0x02 ){
		return ignore_interrupt();
	}
	else {
		snk_irq_status |= 0x2; /* cleared at the end of the interrupt handler */
		snk_irq_status |= 0x1;
		return interrupt();
	}
}

static void snk_soundlatch_w( int offset, int data ){
	snk_soundcommand = data;
	snk_irq_status |= 0x8; /* pending sound command */
}

static int snk_soundlatch_r( int offset ){
	return snk_soundcommand;
}

static int snk_soundlatch_clear( int offset ){
	snk_soundcommand = 0x00;
	return 0x00;
}

static void snk_irq_ctrl_w( int offset, int data ){
	snk_irq_status &= (data>>4);
}

static int snk_irq_ctrl_r( int offset ){
	return snk_irq_status;
}

static struct MemoryReadAddress tnk3_readmem_sound[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, snk_soundlatch_r },
	{ 0xc000, 0xc000, snk_soundlatch_clear },
	{ 0xe000, 0xe000, YM3526_status_port_0_r },
	{ -1 }
};

static struct MemoryWriteAddress tnk3_writemem_sound[] = {
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xe000, 0xe000, YM3526_control_port_0_w },
	{ 0xe001, 0xe001, YM3526_write_port_0_w },
	{ -1 }
};

static struct MemoryReadAddress athena_readmem_sound[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xe000, 0xe000, snk_soundlatch_r },
	{ 0xe800, 0xe800, YM3526_status_port_0_r },
	{ 0xf000, 0xf000, YM3526_status_port_1_r },
	{ 0xf800, 0xf800, snk_irq_ctrl_r },
	{ -1 }
};

static struct MemoryWriteAddress athena_writemem_sound[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xe800, 0xe800, YM3526_control_port_0_w },
	{ 0xec00, 0xec00, YM3526_write_port_0_w },
	{ 0xf000, 0xf000, YM3526_control_port_1_w },
	{ 0xf400, 0xf400, YM3526_write_port_1_w },
	{ 0xf800, 0xf800, snk_irq_ctrl_w },
	{ -1 }
};

static struct MemoryReadAddress dogsoke_readmem_sound[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xe000, 0xe000, snk_soundlatch_r },
	{ 0xe800, 0xe800, YM3526_status_port_0_r }, /* ? */
	{ 0xf000, 0xf000, Y8950_status_port_0_r },
	{ 0xf800, 0xf800, snk_irq_ctrl_r },
	{ -1 }
};

static struct MemoryWriteAddress dogsoke_writemem_sound[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xe800, 0xe800, YM3526_control_port_0_w }, /* ? */
	{ 0xec00, 0xec00, YM3526_write_port_0_w }, /* ? */
	{ 0xf000, 0xf000, Y8950_control_port_0_w },
	{ 0xf400, 0xf400, Y8950_write_port_0_w },
	{ 0xf800, 0xf800, snk_irq_ctrl_w },
	{ -1 }
};

static struct MemoryReadAddress chopper1_readmem_sound[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xe000, 0xe000, snk_soundlatch_r },
	{ 0xe800, 0xe800, YM3812_status_port_0_r },
	{ 0xf000, 0xf000, Y8950_status_port_0_r },
	{ 0xf800, 0xf800, snk_irq_ctrl_r },
	{ -1 }
};

static struct MemoryWriteAddress chopper1_writemem_sound[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xe800, 0xe800, YM3812_control_port_0_w },
	{ 0xec00, 0xec00, YM3812_write_port_0_w },
	{ 0xf000, 0xf000, Y8950_control_port_0_w },
	{ 0xf400, 0xf400, Y8950_write_port_0_w },
	{ 0xf800, 0xf800, snk_irq_ctrl_w },
	{ -1 }
};

#if 0
/* TBA */
static struct MemoryReadAddress ftsoccer_readmem_sound[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xe000, 0xe000, snk_soundlatch_r },
	{ 0xf000, 0xf000, Y8950_status_port_0_r },
	{ 0xf800, 0xf800, snk_irq_ctrl_r },
	{ -1 }
};

static struct MemoryWriteAddress ftsoccer_writemem_sound[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xf000, 0xf000, Y8950_control_port_0_w },
	{ 0xf400, 0xf400, Y8950_write_port_0_w },
	{ 0xf800, 0xf800, snk_irq_ctrl_w },
	{ -1 }
};
#endif

/**********************  Tnk3, Athena, Fighting Golf ********************/

static int hard_flags;

#define SNK_NMI_ENABLE	1
#define SNK_NMI_PENDING	2

static enum {
	DIAL_NONE,
	DIAL_BOOTLEG,
	DIAL_NORMAL
} dial_type;

static int cpuA_latch, cpuB_latch;
static unsigned char *shared_ram, *io_ram, *shared_auxram;

static int snk_read_joy( int which ){
	int result = readinputport(which+1);

	if( dial_type==DIAL_BOOTLEG ){
		static int dial_8[8]   = { 0xf0,0x30,0x10,0x50,0x40,0xc0,0x80,0xa0 };
		result |= dial_8[readinputport(which+7) * 8 / 256];
	}
	else if( dial_type==DIAL_NORMAL ){
		static int dial_12[12] = { 0xb0,0xa0,0x90,0x80,0x70,0x60,0x50,0x40,0x30,0x20,0x10,0x00 };
		result |= dial_12[readinputport(which+7) * 12 / 256];
	}

	return result;
}

static int shared_ram_r( int offset ){
	return shared_ram[offset];
}
static void shared_ram_w( int offset, int data ){
	shared_ram[offset] = data;
}

static int shared_auxram_r( int offset ){
	return shared_auxram[offset];
}
static void shared_auxram_w( int offset, int data ){
	shared_auxram[offset] = data;
}

static int cpuA_io_r( int offset ){
	switch( offset ){
		case 0x000: return input_port_0_r( 0 );
		case 0x100: return snk_read_joy( 0 );
		case 0x200: return snk_read_joy( 1 );
		case 0x300: return input_port_3_r( 0 );

		case 0x400: return input_port_4_r( 0 );
		case 0x500: return input_port_5_r( 0 );
		case 0x600: return input_port_6_r( 0 );

		case 0x700:
		if( cpuB_latch & SNK_NMI_ENABLE ){
			cpu_cause_interrupt( 1, Z80_NMI_INT );
			cpuB_latch = 0;
		}
		else {
			cpuB_latch |= SNK_NMI_PENDING;
		}
		return 0xff;

		/* "Hard Flags" */
		case 0xe00:
		case 0xe20:
		case 0xe40:
		case 0xe60:
		case 0xe80:
		case 0xea0:
		case 0xee0: if( hard_flags ) return 0xff;
	}
	return io_ram[offset];
}

/* Ikari Warriors, Victory Road, Guerrilla War */
static int cpuA_io_r2( int offset ){
	switch( offset ){
		case 0x000: return input_port_0_r( 0 );
		case 0x100: return snk_read_joy( 0 );
		case 0x200: return snk_read_joy( 1 );
		case 0x300: return input_port_3_r( 0 );

		case 0x500: return input_port_4_r( 0 );
		case 0x600: return input_port_5_r( 0 );

		case 0x700:
			if( cpuB_latch & SNK_NMI_ENABLE ){
				cpu_cause_interrupt( 1, Z80_NMI_INT );
				cpuB_latch = 0;
			}
			else {
				cpuB_latch |= SNK_NMI_PENDING;
			}
			return 0xff;

		/* "Hard Flags" */
		case 0xe00:
		case 0xe20:
		case 0xe40:
		case 0xe60:
		case 0xe80:
		case 0xea0:
		case 0xee0:
			if( hard_flags ) return 0xff;
			break;

		default:
			if (errorlog) fprintf(errorlog,"%04x: cpuA_io_r2 %03x\n",cpu_get_pc(),offset);
			break;
	}
	return io_ram[offset];
}

static void cpuA_io_w( int offset, int data ){
	switch( offset ){
		case 0x000:
		break;

		case 0x400:
		io_ram[offset] = data; /* ? */
		snk_soundlatch_w( 0, data );
		break;

		case 0x700:
		if( cpuA_latch&SNK_NMI_PENDING ){
			cpu_cause_interrupt( 0, Z80_NMI_INT );
			cpuA_latch = 0;
		}
		else {
			cpuA_latch |= SNK_NMI_ENABLE;
		}
		break;

		default:
		io_ram[offset] = data;
		break;
	}
}


static int cpuB_io_r( int offset ){
	switch( offset ){
		case 0x000:
		case 0x700:
		if( cpuA_latch & SNK_NMI_ENABLE ){
			cpu_cause_interrupt( 0, Z80_NMI_INT );
			cpuA_latch = 0;
		}
		else {
			cpuA_latch |= SNK_NMI_PENDING;
		}
		return 0xff;

		/* "Hard Flags" they are needed here, otherwise ikarijp/b doesn't work right */
		case 0xe00:
		case 0xe20:
		case 0xe40:
		case 0xe60:
		case 0xe80:
		case 0xea0:
		case 0xee0: if( hard_flags ) return 0xff;
        }
	return io_ram[offset];
}

static void cpuB_io_w( int offset, int data ){
	if( offset==0 || offset==0x700 ){
		if( cpuB_latch&SNK_NMI_PENDING ){
			cpu_cause_interrupt( 1, Z80_NMI_INT );
			cpuB_latch = 0;
		}
		else {
			cpuB_latch |= SNK_NMI_ENABLE;
		}
		return;
	}
	io_ram[offset] = data;
}

/**********************  Tnk3, Athena, Fighting Golf ********************/

static struct MemoryReadAddress tnk3_readmem_cpuA[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, cpuA_io_r },
	{ 0xd000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_RAM },
	{ -1 }
};
static struct MemoryWriteAddress tnk3_writemem_cpuA[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, cpuA_io_w, &io_ram },
	{ 0xd000, 0xf7ff, MWA_RAM, &shared_auxram },
	{ 0xf800, 0xffff, MWA_RAM, &shared_ram },
	{ -1 }
};

static struct MemoryReadAddress tnk3_readmem_cpuB[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, cpuB_io_r },
	{ 0xc800, 0xefff, shared_auxram_r },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, shared_ram_r },
	{ -1 }
};
static struct MemoryWriteAddress tnk3_writemem_cpuB[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, cpuB_io_w },
	{ 0xc800, 0xefff, shared_auxram_w },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xffff, shared_ram_w },
	{ -1 }
};


/* Chopper I, T.D.Fever, Psycho S., Bermuda T. */

static struct MemoryReadAddress readmem_cpuA[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, cpuA_io_r },
	{ 0xd000, 0xffff, MRA_RAM },
	{ -1 }
};

/* Ikari Warriors, Victroy Road, Guerrilla War */
static struct MemoryReadAddress readmem2_cpuA[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, cpuA_io_r2 },
	{ 0xd000, 0xffff, MRA_RAM },
	{ -1 }
};

static struct MemoryWriteAddress writemem_cpuA[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, cpuA_io_w, &io_ram },
	{ 0xd000, 0xffff, MWA_RAM, &shared_ram },
	{ -1 }
};

static struct MemoryReadAddress readmem_cpuB[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, cpuB_io_r },
	{ 0xd000, 0xffff, shared_ram_r },
	{ -1 }
};
static struct MemoryWriteAddress writemem_cpuB[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, cpuB_io_w },
	{ 0xd000, 0xffff, shared_ram_w },
	{ -1 }
};

/*********************************************************************/

static struct GfxLayout char512 =
{
	8,8,
	512,
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	256
};

static struct GfxLayout char1024 =
{
	8,8,
	1024,
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	256
};

static struct GfxLayout tile1024 =
{
	16,16,
	1024,
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
		32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24, },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static struct GfxLayout tile2048 =
{
	16,16,
	2048,
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
		32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24, },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static struct GfxLayout tdfever_tiles =
{
	16,16,
	512*5,
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
		32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24, },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static struct GfxLayout sprite512 =
{
	16,16,
	512,
	3,
	{ 2*1024*256, 1*1024*256, 0*1024*256 },
	{ 7,6,5,4,3,2,1,0, 15,14,13,12,11,10,9,8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	256
};

static struct GfxLayout sprite1024 =
{
	16,16,
	1024,
	3,
	{ 2*1024*256,1*1024*256,0*1024*256 },
	{ 7,6,5,4,3,2,1,0, 15,14,13,12,11,10,9,8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	256
};

static struct GfxLayout big_sprite512 =
{
	32,32,
	512,
	3,
	{ 2*2048*256,1*2048*256,0*2048*256 },
	{
		7,6,5,4,3,2,1,0,
		15,14,13,12,11,10,9,8,
		23,22,21,20,19,18,17,16,
		31,30,29,28,27,26,25,24
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
		16*32+0*32, 16*32+1*32, 16*32+2*32, 16*32+3*32,
		16*32+4*32, 16*32+5*32, 16*32+6*32, 16*32+7*32,
		16*32+8*32, 16*32+9*32, 16*32+10*32, 16*32+11*32,
		16*32+12*32, 16*32+13*32, 16*32+14*32, 16*32+15*32,
	},
	16*32*2
};

static struct GfxLayout gwar_sprite1024 =
{
	16,16,
	1024,
	4,
	{ 3*2048*256,2*2048*256,1*2048*256,0*2048*256 },
	{
		8,9,10,11,12,13,14,15,
		0,1,2,3,4,5,6,7
	},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	256
};

static struct GfxLayout gwar_sprite2048 =
{
	16,16,
	2048,
	4,
	{  3*2048*256,2*2048*256,1*2048*256,0*2048*256 },
	{ 8,9,10,11,12,13,14,15, 0,1,2,3,4,5,6,7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	256
};

static struct GfxLayout gwar_big_sprite1024 =
{
	32,32,
	1024,
	4,
	{ 3*1024*1024, 2*1024*1024, 1*1024*1024, 0*1024*1024 },
	{
		24,25,26,27,28,29,30,31,
		16,17,18,19,20,21,22,23,
		8,9,10,11,12,13,14,15,
		0,1,2,3,4,5,6,7
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
		16*32+0*32, 16*32+1*32, 16*32+2*32, 16*32+3*32,
		16*32+4*32, 16*32+5*32, 16*32+6*32, 16*32+7*32,
		16*32+8*32, 16*32+9*32, 16*32+10*32, 16*32+11*32,
		16*32+12*32, 16*32+13*32, 16*32+14*32, 16*32+15*32,
	},
	1024
};

static struct GfxLayout tdfever_big_sprite1024 =
{
	32,32,
	1024,
	4,
	{ 3*1024*1024, 2*1024*1024, 1*1024*1024, 0*1024*1024 },
	{
		7,6,5,4,3,2,1,0,
		15,14,13,12,11,10,9,8,
		23,22,21,20,19,18,17,16,
		31,30,29,28,27,26,25,24
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
		16*32+0*32, 16*32+1*32, 16*32+2*32, 16*32+3*32,
		16*32+4*32, 16*32+5*32, 16*32+6*32, 16*32+7*32,
		16*32+8*32, 16*32+9*32, 16*32+10*32, 16*32+11*32,
		16*32+12*32, 16*32+13*32, 16*32+14*32, 16*32+15*32,
	},
	1024
};

/*********************************************************************/

static struct GfxDecodeInfo tnk3_gfxdecodeinfo[] =
{
	{ MEM_GFX_CHARS,      0x0, &char512,	128*3,  8 },
	{ MEM_GFX_TILES,      0x0, &char1024,	128*1, 16 },
	{ MEM_GFX_SPRITES,    0x0, &sprite512,	128*0, 16 },
	{ -1 }
};

static struct GfxDecodeInfo athena_gfxdecodeinfo[] =
{
	/* colors 512-1023 are currently unused, I think they are a second bank */
	{ MEM_GFX_CHARS,      0x0, &char512,	128*3,  8 },	/* colors 384..511 */
	{ MEM_GFX_TILES,      0x0, &char1024,   128*1, 16 },	/* colors 128..383 */
	{ MEM_GFX_SPRITES,    0x0, &sprite1024,		0, 16 },	/* colors   0..127 */
	{ -1 }
};

static struct GfxDecodeInfo ikari_gfxdecodeinfo[] =
{
	{ MEM_GFX_CHARS,      0x0, &char512,             256, 16 },
	{ MEM_GFX_TILES,      0x0, &tile1024,            256, 16 },
	{ MEM_GFX_SPRITES,    0x0, &sprite1024,            0, 16 },
	{ MEM_GFX_BIGSPRITES, 0x0, &big_sprite512,       128, 16 },
	{ -1 }
};

static struct GfxDecodeInfo gwar_gfxdecodeinfo[] =
{
	{ MEM_GFX_CHARS,      0x0, &char1024,             256*0, 16 },
	{ MEM_GFX_TILES,      0x0, &tile2048,             256*3, 16 },
	{ MEM_GFX_SPRITES,    0x0, &gwar_sprite2048,      256*1, 16 },
	{ MEM_GFX_BIGSPRITES, 0x0, &gwar_big_sprite1024,  256*2, 16 },
	{ -1 }
};

static struct GfxDecodeInfo bermudat_gfxdecodeinfo[] =
{
	{ MEM_GFX_CHARS,      0x0, &char1024,             256*0, 16 },
	{ MEM_GFX_TILES,      0x0, &tile2048,             256*3, 16 },
	{ MEM_GFX_SPRITES,    0x0, &gwar_sprite1024,      256*1, 16 },
	{ MEM_GFX_BIGSPRITES, 0x0, &gwar_big_sprite1024,  256*2, 16 },
	{ -1 }
};

static struct GfxDecodeInfo psychos_gfxdecodeinfo[] =
{
	{ MEM_GFX_CHARS,      0x0, &char1024,             256*0, 16 },
	{ MEM_GFX_TILES,      0x0, &tile2048,             256*3, 16 },
	{ MEM_GFX_SPRITES,    0x0, &gwar_sprite1024,      256*1, 16 },
	{ MEM_GFX_BIGSPRITES, 0x0, &gwar_big_sprite1024,  256*2, 16 },
	{ -1 }
};

static struct GfxDecodeInfo tdfever_gfxdecodeinfo[] =
{
	{ MEM_GFX_CHARS,      0x0, &char1024,            	0, 16 },
	{ MEM_GFX_TILES,      0x0, &tdfever_tiles,        512, 16 },
	{ MEM_GFX_SPRITES,    0x0, &tdfever_big_sprite1024,  256, 16 },
	{ -1 }
};

/**********************************************************************/

static struct MachineDriver machine_driver_tnk3 =
{
	{
		{
			CPU_Z80,
			4000000, /* ? */
			tnk3_readmem_cpuA,tnk3_writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000, /* ? */
			tnk3_readmem_cpuB,tnk3_writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			tnk3_readmem_sound,tnk3_writemem_sound,0,0,
			interrupt,2	/* ?? generated by the ym3526 maybe? */
		},

	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	36*8, 28*8, { 0*8, 36*8-1, 1*8, 28*8-1 },

	tnk3_gfxdecodeinfo,
	1024,1024,
	aso_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	tnk3_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&ym3526_interface
	    }
	}
};

static struct MachineDriver machine_driver_athena =
{
	{
		{
			CPU_Z80,
			4000000, /* ? */
			tnk3_readmem_cpuA,tnk3_writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000, /* ? */
			tnk3_readmem_cpuB,tnk3_writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			athena_readmem_sound,athena_writemem_sound,0,0,
			interrupt,1
		},

	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	600,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	//36*8, 256, { 0*8, 36*8-1, 16, 255-16 },
	36*8, 28*8, { 0*8, 36*8-1, 1*8, 28*8-1 },

	athena_gfxdecodeinfo,
	1024,1024,
	aso_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	tnk3_vh_screenrefresh,  //athena_vh...

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&dual_ym3526_interface
	    }
	}
};

static struct MachineDriver machine_driver_ikari =
{
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem2_cpuA,writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuB,writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			athena_readmem_sound,athena_writemem_sound,0,0,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	  36*8, 28*8, { 0*8, 36*8-1, 1*8, 28*8-1 },

	ikari_gfxdecodeinfo,
	1024,1024,
	ikari_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	ikari_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&dual_ym3526_interface
	    }
	}
};

static struct MachineDriver machine_driver_victroad =
{
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem2_cpuA,writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuB,writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			dogsoke_readmem_sound,dogsoke_writemem_sound,0,0,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	  36*8, 28*8, { 0*8, 36*8-1, 1*8, 28*8-1 },

	ikari_gfxdecodeinfo,
	1024,1024,
	ikari_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	ikari_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&ym3526_interface
	    },
		{
			SOUND_Y8950,
			&y8950_interface
		}
	}
};

static struct MachineDriver machine_driver_gwar =
{
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem2_cpuA,writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuB,writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			dogsoke_readmem_sound,dogsoke_writemem_sound,0,0,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	384, 240, { 16, 383,0, 239-16 },
	gwar_gfxdecodeinfo,
	1024,1024,
	snk_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	gwar_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&ym3526_interface
	    },
		{
			SOUND_Y8950,
			&y8950_interface
		}
	}
};

static struct MachineDriver machine_driver_bermudat =
{
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuA,writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuB,writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			dogsoke_readmem_sound,dogsoke_writemem_sound,0,0,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	384, 240, { 16, 383,0, 239-16 },
	bermudat_gfxdecodeinfo,
	1024,1024,
	snk_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	gwar_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&ym3526_interface
	    },
		{
			SOUND_Y8950,
			&y8950_interface
		}
	}
};

static struct MachineDriver machine_driver_psychos =
{
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuA,writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuB,writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			dogsoke_readmem_sound,dogsoke_writemem_sound,0,0,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	384, 240, { 0, 383, 0, 239-16 },
	psychos_gfxdecodeinfo,
	1024,1024,
	snk_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	gwar_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&ym3526_interface
	    },
		{
			SOUND_Y8950,
			&y8950_interface
		}
	}
};

static struct MachineDriver machine_driver_chopper1 =
{
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuA,writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuB,writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			chopper1_readmem_sound,chopper1_writemem_sound,0,0,
			sound_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	384, 240, { 0, 383, 0, 239-16 },
	psychos_gfxdecodeinfo,
	1024,1024,
	snk_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	gwar_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
	    },
		{
			SOUND_Y8950,
			&y8950_interface
		}
	}
};

static struct MachineDriver machine_driver_tdfever =
{
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuA,writemem_cpuA,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			readmem_cpuB,writemem_cpuB,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz (?) */
			dogsoke_readmem_sound, dogsoke_writemem_sound,0,0,
			interrupt,1
		},

	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,	/* CPU slices per frame */
	0, /* init_machine */

	/* video hardware */
	384,240, { 0, 383, 0, 239-16 },
	tdfever_gfxdecodeinfo,
	1024,1024,
	snk_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	snk_vh_start,
	snk_vh_stop,
	tdfever_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&ym3526_interface
	    },
		{
			SOUND_Y8950,
			&y8950_interface
		}
	}
};

/***********************************************************************/

ROM_START( tnk3 )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "tnk3-p1.bin",  0x0000, 0x4000, 0x0d2a8ca9 )
	ROM_LOAD( "tnk3-p2.bin",  0x4000, 0x4000, 0x0ae0a483 )
	ROM_LOAD( "tnk3-p3.bin",  0x8000, 0x4000, 0xd16dd4db )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "tnk3-p4.bin",  0x0000, 0x4000, 0x01b45a90 )
	ROM_LOAD( "tnk3-p5.bin",  0x4000, 0x4000, 0x60db6667 )
	ROM_LOAD( "tnk3-p6.bin",  0x8000, 0x4000, 0x4761fde7 )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "tnk3-p10.bin",  0x0000, 0x4000, 0x7bf0a517 )
	ROM_LOAD( "tnk3-p11.bin",  0x4000, 0x4000, 0x0569ce27 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "7122.2",  0x000, 0x400, 0x34c06bc6 )
	ROM_LOAD( "7122.1",  0x400, 0x400, 0x6d0ac66a )
	ROM_LOAD( "7122.0",  0x800, 0x400, 0x4662b4c8 )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "tnk3-p14.bin", 0x0000, 0x2000, 0x1fd18c43 )
	ROM_RELOAD(               0x2000, 0x2000 )

	ROM_REGION_DISPOSE( 0x8000 ) /* background tiles */
	ROM_LOAD( "tnk3-p12.bin", 0x0000, 0x4000, 0xff495a16 )
	ROM_LOAD( "tnk3-p13.bin", 0x4000, 0x4000, 0xf8344843 )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "tnk3-p7.bin", 0x00000, 0x4000, 0x06b92c88 )
	ROM_LOAD( "tnk3-p8.bin", 0x08000, 0x4000, 0x63d0e2eb )
	ROM_LOAD( "tnk3-p9.bin", 0x10000, 0x4000, 0x872e3fac )
ROM_END

ROM_START( tnk3j )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "p1.4e",  0x0000, 0x4000, 0x03aca147 )
	ROM_LOAD( "tnk3-p2.bin",  0x4000, 0x4000, 0x0ae0a483 )
	ROM_LOAD( "tnk3-p3.bin",  0x8000, 0x4000, 0xd16dd4db )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "tnk3-p4.bin",  0x0000, 0x4000, 0x01b45a90 )
	ROM_LOAD( "tnk3-p5.bin",  0x4000, 0x4000, 0x60db6667 )
	ROM_LOAD( "tnk3-p6.bin",  0x8000, 0x4000, 0x4761fde7 )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "tnk3-p10.bin",  0x0000, 0x4000, 0x7bf0a517 )
	ROM_LOAD( "tnk3-p11.bin",  0x4000, 0x4000, 0x0569ce27 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "7122.2",  0x000, 0x400, 0x34c06bc6 )
	ROM_LOAD( "7122.1",  0x400, 0x400, 0x6d0ac66a )
	ROM_LOAD( "7122.0",  0x800, 0x400, 0x4662b4c8 )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "p14.1e", 0x0000, 0x2000, 0x6bd575ca )
	ROM_RELOAD(         0x2000, 0x2000 )

	ROM_REGION_DISPOSE( 0x8000 ) /* background tiles */
	ROM_LOAD( "tnk3-p12.bin", 0x0000, 0x4000, 0xff495a16 )
	ROM_LOAD( "tnk3-p13.bin", 0x4000, 0x4000, 0xf8344843 )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "tnk3-p7.bin", 0x00000, 0x4000, 0x06b92c88 )
	ROM_LOAD( "tnk3-p8.bin", 0x08000, 0x4000, 0x63d0e2eb )
	ROM_LOAD( "tnk3-p9.bin", 0x10000, 0x4000, 0x872e3fac )
ROM_END

/***********************************************************************/

ROM_START( athena )
	ROM_REGIONX( 0x10000, REGION_CPU1 ) /* 64k for cpuA code */
	ROM_LOAD( "up02_p4.rom",  0x0000, 0x4000,  0x900a113c )
	ROM_LOAD( "up02_m4.rom",  0x4000, 0x8000,  0x61c69474 )

	ROM_REGIONX(  0x10000 , REGION_CPU2 ) /* 64k for cpuB code */
	ROM_LOAD( "up02_p8.rom",  0x0000, 0x4000, 0xdf50af7e )
	ROM_LOAD( "up02_m8.rom",  0x4000, 0x8000, 0xf3c933df )

	ROM_REGIONX( 0x10000, REGION_CPU3 ) /* 64k for sound code */
	ROM_LOAD( "up02_g6.rom",  0x0000, 0x4000, 0x42dbe029 )
	ROM_LOAD( "up02_k6.rom",  0x4000, 0x8000, 0x596f1c8a )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "up02_c2.rom",  0x000, 0x400, 0x294279ae )
	ROM_LOAD( "up02_b1.rom",  0x400, 0x400, 0xd25c9099 )
	ROM_LOAD( "up02_c1.rom",  0x800, 0x400, 0xa4a4e7dc )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "up01_d2.rom",  0x0000, 0x4000,  0x18b4bcca )

	ROM_REGION_DISPOSE( 0x8000 ) /* background tiles */
	ROM_LOAD( "up01_b2.rom",  0x0000, 0x8000,  0xf269c0eb )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "up01_p2.rom",  0x00000, 0x8000, 0xc63a871f )//0,1
	ROM_LOAD( "up01_s2.rom",  0x08000, 0x8000, 0x760568d8 )//8,0
	ROM_LOAD( "up01_t2.rom",  0x10000, 0x8000, 0x57b35c73 )//10,8
ROM_END

/***********************************************************************/

ROM_START( fitegolf )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "gu2",    0x0000, 0x4000, 0x19be7ad6 )
	ROM_LOAD( "gu1",    0x4000, 0x8000, 0xbc32568f )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "gu6",    0x0000, 0x4000, 0x2b9978c5 )
	ROM_LOAD( "gu5",    0x4000, 0x8000, 0xea3d138c )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "gu3",    0x0000, 0x4000, 0x811b87d7 )
	ROM_LOAD( "gu4",    0x4000, 0x8000, 0x2d998e2b )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "82s137.2c",  0x00000, 0x00400, 0x6e4c7836 )
	ROM_LOAD( "82s137.1b",  0x00400, 0x00400, 0x29e7986f )
	ROM_LOAD( "82s137.1c",  0x00800, 0x00400, 0x27ba9ff9 )

	ROM_REGION_DISPOSE(0x4000) /* characters */
	ROM_LOAD( "gu8",   0x0000, 0x4000, 0xf1628dcf )

	ROM_REGION_DISPOSE( 0x8000 ) /* background tiles */
	ROM_LOAD( "gu7",  0x0000, 0x8000, 0x4655f94e )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "gu9",   0x00000, 0x8000, 0xd4957ec5 )
	ROM_LOAD( "gu10",  0x08000, 0x8000, 0xb3acdac2 )
	ROM_LOAD( "gu11",  0x10000, 0x8000, 0xb99cf73b )
ROM_END

/***********************************************************************/

ROM_START( ikari )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* CPU A */
	ROM_LOAD( "1.rom",  0x0000, 0x10000, 0x52a8b2dd )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* CPU B */
	ROM_LOAD( "2.rom",  0x0000, 0x10000, 0x45364d55 )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* Sound CPU */
	ROM_LOAD( "3.rom",  0x0000, 0x10000, 0x56a26699 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "7122er.prm",  0x000, 0x400, 0xb9bf2c2c )
	ROM_LOAD( "7122eg.prm",  0x400, 0x400, 0x0703a770 )
	ROM_LOAD( "7122eb.prm",  0x800, 0x400, 0x0a11cdde )

	ROM_REGION_DISPOSE(0x4000) /* characters */
	ROM_LOAD( "7.rom",    0x00000, 0x4000, 0xa7eb4917 )	/* characters */

	ROM_REGION( 0x20000 ) /* background tiles */
	ROM_LOAD( "17.rom", 0x00000, 0x8000, 0xe0dba976 )
	ROM_LOAD( "18.rom", 0x08000, 0x8000, 0x24947d5f )
	ROM_LOAD( "19.rom", 0x10000, 0x8000, 0x9ee59e91 )
	ROM_LOAD( "20.rom", 0x18000, 0x8000, 0x5da7ec1a )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "8.rom",  0x00000, 0x8000, 0x9827c14a )
	ROM_LOAD( "9.rom",  0x08000, 0x8000, 0x545c790c )
	ROM_LOAD( "10.rom", 0x10000, 0x8000, 0xec9ba07e )

	ROM_REGION_DISPOSE( 0x30000 ) /* 32x32 sprites */
	ROM_LOAD( "11.rom", 0x00000, 0x8000, 0x5c75ea8f )
	ROM_LOAD( "14.rom", 0x08000, 0x8000, 0x3293fde4 )
	ROM_LOAD( "12.rom", 0x10000, 0x8000, 0x95138498 )
	ROM_LOAD( "15.rom", 0x18000, 0x8000, 0x65a61c99 )
	ROM_LOAD( "13.rom", 0x20000, 0x8000, 0x315383d7 )
	ROM_LOAD( "16.rom", 0x28000, 0x8000, 0xe9b03e07 )
ROM_END

ROM_START( ikarijp ) /* unconfirmed */
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "up03_l4.rom",  0x0000, 0x4000, 0xcde006be )
	ROM_LOAD( "up03_k4.rom",  0x4000, 0x8000, 0x26948850 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "ik3",  0x0000, 0x4000, 0x9bb385f8 )
	ROM_LOAD( "ik4",  0x4000, 0x8000, 0x3a144bca )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "ik5",  0x0000, 0x4000, 0x863448fa )
	ROM_LOAD( "ik6",  0x4000, 0x8000, 0x9b16aa57 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "7122er.prm",  0x000, 0x400, 0xb9bf2c2c ) /* red */
	ROM_LOAD( "7122eg.prm",  0x400, 0x400, 0x0703a770 ) /* green */
	ROM_LOAD( "7122eb.prm",  0x800, 0x400, 0x0a11cdde ) /* blue */

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "ik7",    0x00000, 0x4000, 0x9e88f536 )	/* characters */

	ROM_REGION_DISPOSE( 0x20000 ) /* background tiles */
	ROM_LOAD( "17.rom", 0x00000, 0x8000, 0xe0dba976 ) /* same as 17.rom */
	ROM_LOAD( "18.rom", 0x08000, 0x8000, 0x24947d5f ) /* same as 18.rom */
	ROM_LOAD( "ik19", 0x10000, 0x8000, 0x566242ec )
	ROM_LOAD( "20.rom", 0x18000, 0x8000, 0x5da7ec1a ) /* same as 20.rom */

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "ik8",  0x00000, 0x8000, 0x75d796d0 )
	ROM_LOAD( "ik9",  0x08000, 0x8000, 0x2c34903b )
	ROM_LOAD( "ik10", 0x10000, 0x8000, 0xda9ccc94 )

	ROM_REGION_DISPOSE( 0x30000 ) /* 32x32 sprites */
	ROM_LOAD( "11.rom", 0x00000, 0x8000, 0x5c75ea8f )
	ROM_LOAD( "14.rom", 0x08000, 0x8000, 0x3293fde4 )
	ROM_LOAD( "12.rom", 0x10000, 0x8000, 0x95138498 )
	ROM_LOAD( "15.rom", 0x18000, 0x8000, 0x65a61c99 )
	ROM_LOAD( "13.rom", 0x20000, 0x8000, 0x315383d7 )
	ROM_LOAD( "16.rom", 0x28000, 0x8000, 0xe9b03e07 )
ROM_END

ROM_START( ikarijpb )
	ROM_REGIONX( 0x10000, REGION_CPU1 ) /* CPU A */
	ROM_LOAD( "ik1",	  0x00000, 0x4000, 0x2ef87dce )
	ROM_LOAD( "up03_k4.rom",  0x04000, 0x8000, 0x26948850 )

	ROM_REGIONX( 0x10000, REGION_CPU2 ) /* CPU B code */
	ROM_LOAD( "ik3",    0x0000, 0x4000, 0x9bb385f8 )
	ROM_LOAD( "ik4",    0x4000, 0x8000, 0x3a144bca )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "ik5",    0x0000, 0x4000, 0x863448fa )
	ROM_LOAD( "ik6",    0x4000, 0x8000, 0x9b16aa57 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "7122er.prm", 0x000, 0x400, 0xb9bf2c2c )
	ROM_LOAD( "7122eg.prm", 0x400, 0x400, 0x0703a770 )
	ROM_LOAD( "7122eb.prm", 0x800, 0x400, 0x0a11cdde )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "ik7", 0x0000, 0x4000, 0x9e88f536 )

	ROM_REGION_DISPOSE( 0x20000 ) /* background tiles */
	ROM_LOAD( "17.rom", 0x00000, 0x8000, 0xe0dba976 ) /* tiles */
	ROM_LOAD( "18.rom", 0x08000, 0x8000, 0x24947d5f )
	ROM_LOAD( "ik19",   0x10000, 0x8000, 0x566242ec )
	ROM_LOAD( "20.rom", 0x18000, 0x8000, 0x5da7ec1a )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "ik8",    0x00000, 0x8000, 0x75d796d0 )
	ROM_LOAD( "ik9",    0x08000, 0x8000, 0x2c34903b )
	ROM_LOAD( "ik10",   0x10000, 0x8000, 0xda9ccc94 )

	ROM_REGION_DISPOSE( 0x30000 ) /* 32x32 sprites */
	ROM_LOAD( "11.rom", 0x00000, 0x8000, 0x5c75ea8f )
	ROM_LOAD( "14.rom", 0x08000, 0x8000, 0x3293fde4 )
	ROM_LOAD( "12.rom", 0x10000, 0x8000, 0x95138498 )
	ROM_LOAD( "15.rom", 0x18000, 0x8000, 0x65a61c99 )
	ROM_LOAD( "13.rom", 0x20000, 0x8000, 0x315383d7 )
	ROM_LOAD( "16.rom", 0x28000, 0x8000, 0xe9b03e07 )
ROM_END

/***********************************************************************/

ROM_START( victroad )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* CPU A code */
	ROM_LOAD( "p1",  0x0000, 0x10000,  0xe334acef )

	ROM_REGIONX(  0x10000 , REGION_CPU2 )	/* CPU B code */
	ROM_LOAD( "p2",  0x00000, 0x10000, 0x907fac83 )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* sound code */
	ROM_LOAD( "p3",  0x00000, 0x10000, 0xbac745f6 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "mb7122e.1k", 0x000, 0x400, 0x491ab831 )
	ROM_LOAD( "mb7122e.2l", 0x400, 0x400, 0x8feca424 )
	ROM_LOAD( "mb7122e.1l", 0x800, 0x400, 0x220076ca )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "p7",  0x0000, 0x4000,  0x2b6ed95b )

	ROM_REGION_DISPOSE( 0x20000 ) /* background tiles */
	ROM_LOAD( "p17",  0x00000, 0x8000, 0x19d4518c )
	ROM_LOAD( "p18",  0x08000, 0x8000, 0xd818be43 )
	ROM_LOAD( "p19",  0x10000, 0x8000, 0xd64e0f89 )
	ROM_LOAD( "p20",  0x18000, 0x8000, 0xedba0f31 )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "p8",  0x00000, 0x8000, 0xdf7f252a )
	ROM_LOAD( "p9",  0x08000, 0x8000, 0x9897bc05 )
	ROM_LOAD( "p10", 0x10000, 0x8000, 0xecd3c0ea )

	ROM_REGION( 0x40000 ) /* 32x32 sprites */
	ROM_LOAD( "p11", 0x00000, 0x8000, 0x668b25a4 )
	ROM_LOAD( "p14", 0x08000, 0x8000, 0xa7031d4a )
	ROM_LOAD( "p12", 0x10000, 0x8000, 0xf44e95fa )
	ROM_LOAD( "p15", 0x18000, 0x8000, 0x120d2450 )
	ROM_LOAD( "p13", 0x20000, 0x8000, 0x980ca3d8 )
	ROM_LOAD( "p16", 0x28000, 0x8000, 0x9f820e8a )

	ROM_REGION(0x20000)	/* ADPCM Samples */
	ROM_LOAD( "p4",  0x00000, 0x10000, 0xe10fb8cc )
	ROM_LOAD( "p5",  0x10000, 0x10000, 0x93e5f110 )
ROM_END

ROM_START( dogosoke ) /* Victory Road Japan */
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* CPU A code */
	ROM_LOAD( "up03_p4.rom",  0x0000, 0x10000,  0x37867ad2 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* CPU B code */
	ROM_LOAD( "p2",  0x00000, 0x10000, 0x907fac83 )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* sound code */
	ROM_LOAD( "up03_k7.rom",  0x00000, 0x10000, 0x173fa571 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "up03_k1.rom",  0x000, 0x400, 0x10a2ce2b )
	ROM_LOAD( "up03_l2.rom",  0x400, 0x400, 0x99dc9792 )
	ROM_LOAD( "up03_l1.rom",  0x800, 0x400, 0xe7213160 )

	ROM_REGION_DISPOSE( 0x4000 ) /* characters */
	ROM_LOAD( "up02_b3.rom",  0x0000, 0x4000,  0x51a4ec83 )

	ROM_REGION_DISPOSE( 0x20000 ) /* background tiles */
	ROM_LOAD( "p17",  0x00000, 0x8000, 0x19d4518c )
	ROM_LOAD( "p18",  0x08000, 0x8000, 0xd818be43 )
	ROM_LOAD( "p19",  0x10000, 0x8000, 0xd64e0f89 )
	ROM_LOAD( "p20",  0x18000, 0x8000, 0xedba0f31 )

	ROM_REGION_DISPOSE( 0x18000 ) /* 16x16 sprites */
	ROM_LOAD( "up02_d3.rom",  0x00000, 0x8000, 0xd43044f8 )
	ROM_LOAD( "up02_e3.rom",  0x08000, 0x8000, 0x365ed2d8 )
	ROM_LOAD( "up02_g3.rom",  0x10000, 0x8000, 0x92579bf3 )

	ROM_REGION_DISPOSE( 0x30000 ) /* 32x32 sprites */
	ROM_LOAD( "p11", 0x00000, 0x8000, 0x668b25a4 )
	ROM_LOAD( "p14", 0x08000, 0x8000, 0xa7031d4a )
	ROM_LOAD( "p12", 0x10000, 0x8000, 0xf44e95fa )
	ROM_LOAD( "p15", 0x18000, 0x8000, 0x120d2450 )
	ROM_LOAD( "p13", 0x20000, 0x8000, 0x980ca3d8 )
	ROM_LOAD( "p16", 0x28000, 0x8000, 0x9f820e8a )

	ROM_REGION(0x20000) /* ADPCM Samples */
	ROM_LOAD( "up03_f5.rom", 0x00000, 0x10000, 0x5b43fe9f )
	ROM_LOAD( "up03_g5.rom", 0x10000, 0x10000, 0xaae30cd6 )
ROM_END

/***********************************************************************/

ROM_START( gwar )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "g01",  0x00000, 0x10000, 0xce1d3c80 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "g02",  0x00000, 0x10000, 0x86d931bf )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "g03",  0x00000, 0x10000, 0xeb544ab9 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "guprom.3", 0x000, 0x400, 0x090236a3 ) /* red */ // up03_k1.rom
	ROM_LOAD( "guprom.2", 0x400, 0x400, 0x9147de69 ) /* green */ // up03_l1.rom
	ROM_LOAD( "guprom.1", 0x800, 0x400, 0x7f9c839e ) /* blue */ // up03_k2.rom

	ROM_REGION_DISPOSE(0x8000) /* characters */
	ROM_LOAD( "g05",  0x0000, 0x08000, 0x80f73e2e )

	ROM_REGION_DISPOSE(0x40000) /* background tiles */
	ROM_LOAD( "g06",  0x00000, 0x10000, 0xf1dcdaef )
	ROM_LOAD( "g07",  0x10000, 0x10000, 0x326e4e5e )
	ROM_LOAD( "g08",  0x20000, 0x10000, 0x0aa70967 )
	ROM_LOAD( "g09",  0x30000, 0x10000, 0xb7686336 )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "g10",  0x00000, 0x10000, 0x58600f7d )
	ROM_LOAD( "g11",  0x10000, 0x10000, 0xa3f9b463 )
	ROM_LOAD( "g12",  0x20000, 0x10000, 0x092501be )
	ROM_LOAD( "g13",  0x30000, 0x10000, 0x25801ea6 )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "g20",  0x00000, 0x10000, 0x2b46edff )
	ROM_LOAD( "g21",  0x10000, 0x10000, 0xbe19888d )
	ROM_LOAD( "g18",  0x20000, 0x10000, 0x2d653f0c )
	ROM_LOAD( "g19",  0x30000, 0x10000, 0xebbf3ba2 )
	ROM_LOAD( "g16",  0x40000, 0x10000, 0xaeb3707f )
	ROM_LOAD( "g17",  0x50000, 0x10000, 0x0808f95f )
	ROM_LOAD( "g14",  0x60000, 0x10000, 0x8dfc7b87 )
	ROM_LOAD( "g15",  0x70000, 0x10000, 0x06822aac )

	ROM_REGION(0x10000)	/* ADPCM Samples */
	ROM_LOAD( "g04",  0x00000, 0x10000, 0x2255f8dd )
ROM_END

/***********************************************************************/

ROM_START( bermudat )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "bt_p1.rom",  0x0000, 0x10000,  0x43dec5e9 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "bt_p2.rom",  0x00000, 0x10000, 0x0e193265 )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "bt_p3.rom",  0x00000, 0x10000, 0x53a82e50 )    /* YM3526 */

	ROM_REGIONX( 0x1400, REGION_PROMS )
	ROM_LOAD( "btj_01r.prm", 0x0000, 0x0400, 0xf4b54d06 ) /* red */
	ROM_LOAD( "btj_02g.prm", 0x0400, 0x0400, 0xbaac139e ) /* green */
	ROM_LOAD( "btj_03b.prm", 0x0800, 0x0400, 0x2edf2e0b ) /* blue */
	ROM_LOAD( "btj_h.prm",   0x0c00, 0x0400, 0xc20b197b ) /* ? */
	ROM_LOAD( "btj_v.prm",   0x1000, 0x0400, 0x5d0c617f ) /* ? */

	ROM_REGION_DISPOSE( 0x8000 ) /* characters */
	ROM_LOAD( "bt_p10.rom", 0x0000, 0x8000,  0xd3650211 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "bt_p22.rom", 0x00000, 0x10000, 0x8daf7df4 )
	ROM_LOAD( "bt_p21.rom", 0x10000, 0x10000, 0xb7689599 )
	ROM_LOAD( "bt_p20.rom", 0x20000, 0x10000, 0xab6217b7 )
	ROM_LOAD( "bt_p19.rom", 0x30000, 0x10000, 0x8ed759a0 )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "bt_p6.rom",  0x00000, 0x8000, 0x8ffdf969 )
	ROM_LOAD( "bt_p7.rom",  0x10000, 0x8000, 0x268d10df )
	ROM_LOAD( "bt_p8.rom",  0x20000, 0x8000, 0x3e39e9dd )
	ROM_LOAD( "bt_p9.rom",  0x30000, 0x8000, 0xbf56da61 )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "bt_p11.rom", 0x00000, 0x10000, 0xaae7410e )
	ROM_LOAD( "bt_p12.rom", 0x10000, 0x10000, 0x18914f70 )
	ROM_LOAD( "bt_p13.rom", 0x20000, 0x10000, 0xcd79ce81 )
	ROM_LOAD( "bt_p14.rom", 0x30000, 0x10000, 0xedc57117 )
	ROM_LOAD( "bt_p15.rom", 0x40000, 0x10000, 0x448bf9f4 )
	ROM_LOAD( "bt_p16.rom", 0x50000, 0x10000, 0x119999eb )
	ROM_LOAD( "bt_p17.rom", 0x60000, 0x10000, 0xb5462139 )
	ROM_LOAD( "bt_p18.rom", 0x70000, 0x10000, 0xcb416227 )

	ROM_REGION(0x20000)	/* ADPCM Samples */
	ROM_LOAD( "bt_p4.rom",  0x00000, 0x10000, 0x4bc83229 )
	ROM_LOAD( "bt_p5.rom",  0x10000, 0x10000, 0x817bd62c )
ROM_END

/***********************************************************************/

ROM_START( bermudaj )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "btj_p01.bin", 0x0000, 0x10000,  0xeda75f36 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "bt_p2.rom",   0x00000, 0x10000, 0x0e193265 )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "btj_p03.bin", 0x00000, 0x10000, 0xfea8a096 )    /* YM3526 */

	ROM_REGIONX( 0x1400, REGION_PROMS )
	ROM_LOAD( "btj_01r.prm", 0x0000, 0x0400, 0xf4b54d06 ) /* red */
	ROM_LOAD( "btj_02g.prm", 0x0400, 0x0400, 0xbaac139e ) /* green */
	ROM_LOAD( "btj_03b.prm", 0x0800, 0x0400, 0x2edf2e0b ) /* blue */
	ROM_LOAD( "btj_h.prm",   0x0c00, 0x0400, 0xc20b197b ) /* ? */
	ROM_LOAD( "btj_v.prm",   0x1000, 0x0400, 0x5d0c617f ) /* ? */

	ROM_REGION_DISPOSE( 0x8000 ) /* characters */
	ROM_LOAD( "bt_p10.rom",  0x0000, 0x8000,  0xd3650211 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "bt_p22.rom",  0x00000, 0x10000, 0x8daf7df4 )
	ROM_LOAD( "bt_p21.rom",  0x10000, 0x10000, 0xb7689599 )
	ROM_LOAD( "bt_p20.rom",  0x20000, 0x10000, 0xab6217b7 )
	ROM_LOAD( "bt_p19.rom",  0x30000, 0x10000, 0x8ed759a0 )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "bt_p6.rom",   0x00000, 0x8000, 0x8ffdf969 )
	ROM_LOAD( "bt_p7.rom",   0x10000, 0x8000, 0x268d10df )
	ROM_LOAD( "bt_p8.rom",   0x20000, 0x8000, 0x3e39e9dd )
	ROM_LOAD( "bt_p9.rom",   0x30000, 0x8000, 0xbf56da61 )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "bt_p11.rom",  0x00000, 0x10000, 0xaae7410e )
	ROM_LOAD( "bt_p12.rom",  0x10000, 0x10000, 0x18914f70 )
	ROM_LOAD( "bt_p13.rom",  0x20000, 0x10000, 0xcd79ce81 )
	ROM_LOAD( "bt_p14.rom",  0x30000, 0x10000, 0xedc57117 )
	ROM_LOAD( "bt_p15.rom",  0x40000, 0x10000, 0x448bf9f4 )
	ROM_LOAD( "bt_p16.rom",  0x50000, 0x10000, 0x119999eb )
	ROM_LOAD( "bt_p17.rom",  0x60000, 0x10000, 0xb5462139 )
	ROM_LOAD( "bt_p18.rom",  0x70000, 0x10000, 0xcb416227 )

	ROM_REGION(0x20000)	/* ADPCM Samples */
	ROM_LOAD( "btj_p04.bin", 0x00000, 0x10000, 0xb2e01129 )
	ROM_LOAD( "btj_p05.bin", 0x10000, 0x10000, 0x924c24f7 )
ROM_END

/***********************************************************************/

ROM_START( psychos )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "p7",  0x00000, 0x10000, 0x562809f4 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "up03_m8.rom",  0x00000, 0x10000, 0x5f426ddb )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "p5",  0x0000, 0x10000,  0x64503283 )

	ROM_REGIONX( 0x1400, REGION_PROMS )
	ROM_LOAD( "up03_k1.rom",  0x00000, 0x00400, 0x27b8ca8c ) /* red */
	ROM_LOAD( "up03_l1.rom",  0x00400, 0x00400, 0x40e78c9e ) /* green */
	ROM_LOAD( "up03_k2.rom",  0x00800, 0x00400, 0xd845d5ac ) /* blue */
	ROM_LOAD( "mb7122e.8j",   0x0c00, 0x400, 0xc20b197b ) /* ? */
	ROM_LOAD( "mb7122e.8k",   0x1000, 0x400, 0x5d0c617f ) /* ? */

	ROM_REGION_DISPOSE( 0x8000 ) /* characters */
	ROM_LOAD( "up02_a3.rom",  0x0000, 0x8000,  0x11a71919 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "up01_f1.rom",  0x00000, 0x10000, 0x167e5765 )
	ROM_LOAD( "up01_d1.rom",  0x10000, 0x10000, 0x8b0fe8d0 )
	ROM_LOAD( "up01_c1.rom",  0x20000, 0x10000, 0xf4361c50 )
	ROM_LOAD( "up01_a1.rom",  0x30000, 0x10000, 0xe4b0b95e )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "up02_f3.rom",  0x00000, 0x8000, 0xf96f82db ) //ps12
	ROM_LOAD( "up02_e3.rom",  0x10000, 0x8000, 0x2b007733 ) //ps11
	ROM_LOAD( "up02_c3.rom",  0x20000, 0x8000, 0xefa830e1 ) //ps10
	ROM_LOAD( "up02_b3.rom",  0x30000, 0x8000, 0x24559ee1 ) //ps9

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "up01_f10.rom",  0x00000, 0x10000, 0x2bac250e ) //ps17
	ROM_LOAD( "up01_h10.rom",  0x10000, 0x10000, 0x5e1ba353 ) //ps18
	ROM_LOAD( "up01_j10.rom",  0x20000, 0x10000, 0x9ff91a97 ) //ps19
	ROM_LOAD( "up01_l10.rom",  0x30000, 0x10000, 0xae1965ef ) //ps20
	ROM_LOAD( "up01_m10.rom",  0x40000, 0x10000, 0xdf283b67 ) //ps21
	ROM_LOAD( "up01_n10.rom",  0x50000, 0x10000, 0x914f051f ) //ps22
	ROM_LOAD( "up01_r10.rom",  0x60000, 0x10000, 0xc4488472 ) //ps23
	ROM_LOAD( "up01_s10.rom",  0x70000, 0x10000, 0x8ec7fe18 ) //ps24

	ROM_REGION(0x40000)	/* samples */
	ROM_LOAD( "p1",  0x00000, 0x10000, 0x58f1683f )
	ROM_LOAD( "p2",  0x10000, 0x10000, 0xda3abda1 )
	ROM_LOAD( "p3",  0x20000, 0x10000, 0xf3683ae8 )
	ROM_LOAD( "p4",  0x30000, 0x10000, 0x437d775a )
ROM_END

ROM_START( psychosa ) /* USA set */
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "up03_m4.rom",  0x0000, 0x10000,  0x05dfb409 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "up03_m8.rom",  0x00000, 0x10000, 0x5f426ddb )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "up03_j6.rom",  0x00000, 0x10000, 0xbbd0a8e3 )

	ROM_REGIONX( 0x1400, REGION_PROMS )
	ROM_LOAD( "up03_k1.rom",  0x00000, 0x00400, 0x27b8ca8c ) /* red */
	ROM_LOAD( "up03_l1.rom",  0x00400, 0x00400, 0x40e78c9e ) /* green */
	ROM_LOAD( "up03_k2.rom",  0x00800, 0x00400, 0xd845d5ac ) /* blue */
	ROM_LOAD( "mb7122e.8j",   0x0c00, 0x400, 0xc20b197b ) /* ? */
	ROM_LOAD( "mb7122e.8k",   0x1000, 0x400, 0x5d0c617f ) /* ? */

	ROM_REGION_DISPOSE( 0x8000 ) /* characters */
	ROM_LOAD( "up02_a3.rom",  0x0000, 0x8000,  0x11a71919 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "up01_f1.rom",  0x00000, 0x10000, 0x167e5765 )
	ROM_LOAD( "up01_d1.rom",  0x10000, 0x10000, 0x8b0fe8d0 )
	ROM_LOAD( "up01_c1.rom",  0x20000, 0x10000, 0xf4361c50 )
	ROM_LOAD( "up01_a1.rom",  0x30000, 0x10000, 0xe4b0b95e )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "up02_f3.rom",  0x00000, 0x8000, 0xf96f82db )
	ROM_LOAD( "up02_e3.rom",  0x10000, 0x8000, 0x2b007733 )
	ROM_LOAD( "up02_c3.rom",  0x20000, 0x8000, 0xefa830e1 )
	ROM_LOAD( "up02_b3.rom",  0x30000, 0x8000, 0x24559ee1 )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "up01_f10.rom",  0x00000, 0x10000, 0x2bac250e )
	ROM_LOAD( "up01_h10.rom",  0x10000, 0x10000, 0x5e1ba353 )
	ROM_LOAD( "up01_j10.rom",  0x20000, 0x10000, 0x9ff91a97 )
	ROM_LOAD( "up01_l10.rom",  0x30000, 0x10000, 0xae1965ef )
	ROM_LOAD( "up01_m10.rom",  0x40000, 0x10000, 0xdf283b67 )
	ROM_LOAD( "up01_n10.rom",  0x50000, 0x10000, 0x914f051f )
	ROM_LOAD( "up01_r10.rom",  0x60000, 0x10000, 0xc4488472 )
	ROM_LOAD( "up01_s10.rom",  0x70000, 0x10000, 0x8ec7fe18 )

	ROM_REGION(0x40000)	/* Samples */
	ROM_LOAD( "up03_b5.rom",  0x00000, 0x10000, 0x0f8e8276 )
	ROM_LOAD( "up03_c5.rom",  0x10000, 0x10000, 0x34e41dfb )
	ROM_LOAD( "up03_d5.rom",  0x20000, 0x10000, 0xaa583c5e )
	ROM_LOAD( "up03_f5.rom",  0x30000, 0x10000, 0x7e8bce7a )
ROM_END

/***********************************************************************/

ROM_START( chopper )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "kk_01.rom",  0x0000, 0x10000,  0x8fa2f839 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "kk_04.rom",  0x00000, 0x10000, 0x004f7d9a )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "kk_03.rom",  0x00000, 0x10000, 0xdbaafb87 )   /* YM3526 */

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "up03_k1.rom",  0x0000, 0x0400, 0x7f07a45c ) /* red */
	ROM_LOAD( "up03_l1.rom",  0x0400, 0x0400, 0x15359fc3 ) /* green */
	ROM_LOAD( "up03_k2.rom",  0x0800, 0x0400, 0x79b50f7d ) /* blue */

	ROM_REGION_DISPOSE( 0x8000 ) /* characters */
	ROM_LOAD( "kk_05.rom",  0x0000, 0x8000, 0xdefc0987 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "kk_10.rom",  0x00000, 0x10000, 0x5cf4d22b )
	ROM_LOAD( "kk_11.rom",  0x10000, 0x10000, 0x9af4cad0 )
	ROM_LOAD( "kk_12.rom",  0x20000, 0x10000, 0x02fec778 )
	ROM_LOAD( "kk_13.rom",  0x30000, 0x10000, 0x2756817d )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "kk_09.rom",  0x00000, 0x08000, 0x653c4342 )
	ROM_LOAD( "kk_08.rom",  0x10000, 0x08000, 0x2da45894 )
	ROM_LOAD( "kk_07.rom",  0x20000, 0x08000, 0xa0ebebdf )
	ROM_LOAD( "kk_06.rom",  0x30000, 0x08000, 0x284fad9e )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "kk_18.rom",  0x00000, 0x10000, 0x6abbff36 )
	ROM_LOAD( "kk_19.rom",  0x10000, 0x10000, 0x5283b4d3 )
	ROM_LOAD( "kk_20.rom",  0x20000, 0x10000, 0x6403ddf2 )
	ROM_LOAD( "kk_21.rom",  0x30000, 0x10000, 0x9f411940 )
	ROM_LOAD( "kk_14.rom",  0x40000, 0x10000, 0x9bad9e25 )
	ROM_LOAD( "kk_15.rom",  0x50000, 0x10000, 0x89faf590 )
	ROM_LOAD( "kk_16.rom",  0x60000, 0x10000, 0xefb1fb6c )
	ROM_LOAD( "kk_17.rom",  0x70000, 0x10000, 0x6b7fb0a5 )

	ROM_REGION(0x10000)	/* Samples ?? */
	ROM_LOAD( "kk_02.rom",  0x00000, 0x10000, 0x06169ae0 )
ROM_END

ROM_START( legofair ) /* ChopperI (Japan) */
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "up03_m4.rom",  0x0000, 0x10000,  0x79a485c0 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "up03_m8.rom",  0x00000, 0x10000, 0x96d3a4d9 )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "kk_03.rom",  0x00000, 0x10000, 0xdbaafb87 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "up03_k1.rom",  0x0000, 0x0400, 0x7f07a45c ) /* red */
	ROM_LOAD( "up03_l1.rom",  0x0400, 0x0400, 0x15359fc3 ) /* green */
	ROM_LOAD( "up03_k2.rom",  0x0800, 0x0400, 0x79b50f7d ) /* blue */

	ROM_REGION_DISPOSE(0x8000) /* characters */
	ROM_LOAD( "kk_05.rom",  0x0000, 0x8000, 0xdefc0987 )

	ROM_REGION_DISPOSE( 0x40000 ) /* background tiles */
	ROM_LOAD( "kk_10.rom",  0x00000, 0x10000, 0x5cf4d22b )
	ROM_LOAD( "kk_11.rom",  0x10000, 0x10000, 0x9af4cad0 )
	ROM_LOAD( "kk_12.rom",  0x20000, 0x10000, 0x02fec778 )
	ROM_LOAD( "kk_13.rom",  0x30000, 0x10000, 0x2756817d )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */
	ROM_LOAD( "kk_09.rom",  0x00000, 0x08000, 0x653c4342 )
	ROM_LOAD( "kk_08.rom",  0x10000, 0x08000, 0x2da45894 )
	ROM_LOAD( "kk_07.rom",  0x20000, 0x08000, 0xa0ebebdf )
	ROM_LOAD( "kk_06.rom",  0x30000, 0x08000, 0x284fad9e )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "kk_18.rom",  0x00000, 0x10000, 0x6abbff36 )
	ROM_LOAD( "kk_19.rom",  0x10000, 0x10000, 0x5283b4d3 )
	ROM_LOAD( "kk_20.rom",  0x20000, 0x10000, 0x6403ddf2 )
	ROM_LOAD( "kk_21.rom",  0x30000, 0x10000, 0x9f411940 )
	ROM_LOAD( "kk_14.rom",  0x40000, 0x10000, 0x9bad9e25 )
	ROM_LOAD( "kk_15.rom",  0x50000, 0x10000, 0x89faf590 )
	ROM_LOAD( "kk_16.rom",  0x60000, 0x10000, 0xefb1fb6c )
	ROM_LOAD( "kk_17.rom",  0x70000, 0x10000, 0x6b7fb0a5 )

	ROM_REGION(0x10000)	/* ADPCM Samples */
	ROM_LOAD( "kk_02.rom",  0x00000, 0x10000, 0x06169ae0 )
ROM_END

/***********************************************************************/

ROM_START( ftsoccer )
	ROM_REGIONX( 0x10000, REGION_CPU1 )     /* 64k for cpuA code */
	ROM_LOAD( "ft-003.bin",  0x00000, 0x10000, 0x649d4448 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )     /* 64k for cpuB code */
	ROM_LOAD( "ft-001.bin",  0x00000, 0x10000, 0x2f68e38b )

	ROM_REGIONX( 0x10000, REGION_CPU3 )     /* 64k for sound code */
	ROM_LOAD( "ft-002.bin",  0x00000, 0x10000, 0x9ee54ea1 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "prom2.bin", 0x000, 0x400, 0xbf4ac706 ) /* red */
	ROM_LOAD( "prom1.bin", 0x400, 0x400, 0x1bac8010 ) /* green */
	ROM_LOAD( "prom3.bin", 0x800, 0x400, 0xdbeddb14 ) /* blue */

	ROM_REGION_DISPOSE(0x8000) /* characters */
	ROM_LOAD( "ft-013.bin",  0x0000, 0x08000, 0x0de7b7ad )

	ROM_REGION_DISPOSE(0x40000) /* background tiles */
	ROM_LOAD( "ft-014.bin",  0x00000, 0x10000, 0x38c38b40 )
	ROM_LOAD( "ft-015.bin",  0x10000, 0x10000, 0xa614834f )

	ROM_REGION_DISPOSE( 0x40000 ) /* 16x16 sprites */

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "ft-005.bin",  0x70000, 0x10000, 0xdef2f1d8 )
	ROM_LOAD( "ft-006.bin",  0x60000, 0x10000, 0x588d14b3 )
	ROM_LOAD( "ft-007.bin",  0x50000, 0x10000, 0xd584964b )
	ROM_LOAD( "ft-008.bin",  0x40000, 0x10000, 0x11156a7d )
	ROM_LOAD( "ft-009.bin",  0x30000, 0x10000, 0xd8112aa6 )
	ROM_LOAD( "ft-010.bin",  0x20000, 0x10000, 0xe42864d8 )
	ROM_LOAD( "ft-011.bin",  0x10000, 0x10000, 0x022f3e96 )
	ROM_LOAD( "ft-012.bin",  0x00000, 0x10000, 0xb2442c30 )

	ROM_REGION(0x10000)     /* Samples?? */
	ROM_LOAD( "ft-004.bin",  0x00000, 0x10000, 0x435c3716 )
ROM_END

/***********************************************************************/

ROM_START( tdfever ) /* USA set - unconfirmed! */
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "td2-ver3.6c",  0x0000, 0x10000,  0x92138fe4 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "td1-ver3.2c",  0x00000, 0x10000, 0x798711f5 )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "td3-ver3.3j",  0x00000, 0x10000, 0x5d13e0b1 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "up03_e8.rom",  0x000, 0x00400, 0x67bdf8a0 )
	ROM_LOAD( "up03_d8.rom",  0x400, 0x00400, 0x9c4a9198 )
	ROM_LOAD( "up03_e9.rom",  0x800, 0x00400, 0xc93c18e8 )

	ROM_REGION_DISPOSE(0x8000) /* characters */
	ROM_LOAD( "td14ver3.4n",  0x0000, 0x8000,  0xe841bf1a )

	ROM_REGION_DISPOSE( 0x50000 )/* background tiles */
	ROM_LOAD( "up01_d8.rom",  0x00000, 0x10000, 0xad6e0927 )
	ROM_LOAD( "up01_e8.rom",  0x10000, 0x10000, 0x181db036 )
	ROM_LOAD( "up01_f8.rom",  0x20000, 0x10000, 0xc5decca3 )
	ROM_LOAD( "td18ver2.8gh", 0x30000, 0x10000, 0x3924da37 )
	ROM_LOAD( "up01_j8.rom",  0x40000, 0x10000, 0xbc17ea7f )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "up01_j2.rom",  0x10000, 0x10000, 0x9b6d4053 ) /* plane 0 */
	ROM_LOAD( "up01_n2.rom",  0x00000, 0x10000, 0xa8979657 )
	ROM_LOAD( "up01_l2.rom",  0x30000, 0x10000, 0x28f49182 ) /* plane 1 */
	ROM_LOAD( "up01_k2.rom",  0x20000, 0x10000, 0x72a5590d )
	ROM_LOAD( "up01_p2.rom",  0x50000, 0x10000, 0xc8c71c7b ) /* plane 2 */
	ROM_LOAD( "up01_r2.rom",  0x40000, 0x10000, 0xa0d53fbd )
	ROM_LOAD( "up01_s2.rom",  0x70000, 0x10000, 0xf6f83d63 ) /* plane 3 */
	ROM_LOAD( "up01_t2.rom",  0x60000, 0x10000, 0x88e2e819 )

	ROM_REGION( 1 ) /* dummy */

	ROM_REGION(0x20000)	/* ADPCM Samples */
	ROM_LOAD( "up02_n6.rom",  0x00000, 0x10000, 0x155e472e )
	ROM_LOAD( "up02_p6.rom",  0x10000, 0x10000, 0x04794557 )
ROM_END

ROM_START( tdfeverj )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for cpuA code */
	ROM_LOAD( "up02_c6.rom",  0x0000, 0x10000,  0x88d88ec4 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for cpuB code */
	ROM_LOAD( "up02_c2.rom",  0x00000, 0x10000, 0x191e6442 )

	ROM_REGIONX( 0x10000, REGION_CPU3 )	/* 64k for sound code */
	ROM_LOAD( "up02_j3.rom",  0x00000, 0x10000, 0x4e4d71c7 )

	ROM_REGIONX( 0x0c00, REGION_PROMS )
	ROM_LOAD( "up03_e8.rom",  0x000, 0x00400, 0x67bdf8a0 )
	ROM_LOAD( "up03_d8.rom",  0x400, 0x00400, 0x9c4a9198 )
	ROM_LOAD( "up03_e9.rom",  0x800, 0x00400, 0xc93c18e8 )

	ROM_REGION_DISPOSE(0x8000) /* characters */
	ROM_LOAD( "up01_n4.rom",  0x0000, 0x8000,  0xaf9bced5 )

	ROM_REGION_DISPOSE( 0x50000 )/* background tiles */
	ROM_LOAD( "up01_d8.rom",  0x00000, 0x10000, 0xad6e0927 )
	ROM_LOAD( "up01_e8.rom",  0x10000, 0x10000, 0x181db036 )
	ROM_LOAD( "up01_f8.rom",  0x20000, 0x10000, 0xc5decca3 )
	ROM_LOAD( "up01_g8.rom",  0x30000, 0x10000, 0x4512cdfb )
	ROM_LOAD( "up01_j8.rom",  0x40000, 0x10000, 0xbc17ea7f )

	ROM_REGION_DISPOSE( 0x80000 ) /* 32x32 sprites */
	ROM_LOAD( "up01_j2.rom",  0x70000, 0x10000, 0x9b6d4053 )
	ROM_LOAD( "up01_n2.rom",  0x60000, 0x10000, 0xa8979657 )
	ROM_LOAD( "up01_l2.rom",  0x50000, 0x10000, 0x28f49182 )
	ROM_LOAD( "up01_k2.rom",  0x40000, 0x10000, 0x72a5590d )
	ROM_LOAD( "up01_p2.rom",  0x30000, 0x10000, 0xc8c71c7b )
	ROM_LOAD( "up01_r2.rom",  0x20000, 0x10000, 0xa0d53fbd )
	ROM_LOAD( "up01_s2.rom",  0x10000, 0x10000, 0xf6f83d63 )
	ROM_LOAD( "up01_t2.rom",  0x00000, 0x10000, 0x88e2e819 )
ROM_REGION( 1 ) /* dummy */

	ROM_REGION(0x20000)	/* ADPCM Samples */
	ROM_LOAD( "up02_n6.rom",  0x00000, 0x10000, 0x155e472e )
	ROM_LOAD( "up02_p6.rom",  0x10000, 0x10000, 0x04794557 )
ROM_END


/***********************************************************************/

#define SNK_ROTARY \
	PORT_START \
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL, 25, 10, 0, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 ) \
	PORT_START \
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 25, 10, 0, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )

#define SNK_JOY1_PORT \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

#define SNK_JOY2_PORT \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED ) // dial CONTROL 4 bits

#define SNK_BUTTON_PORT \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define SNK_COINAGE \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

#define SNK_UNUSED_PORT \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( ikari )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* sound CPU status */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	SNK_JOY1_PORT
	SNK_JOY2_PORT
	SNK_BUTTON_PORT

	PORT_START /* DSW 1 */
	PORT_DIPNAME( 0x01, 0x01, "Allow killing each other" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "P1 & P2 Fire Buttons" )
	PORT_DIPSETTING(    0x02, "Separate" )
	PORT_DIPSETTING(    0x00, "Common" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance" )
	PORT_DIPSETTING(    0x04, "1st & every 2nd" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	SNK_COINAGE

	PORT_START /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Demo Sounds On" )
	PORT_DIPSETTING(    0x04, "Freeze" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "50k 100k" )
	PORT_DIPSETTING(    0x20, "60k 120k" )
	PORT_DIPSETTING(    0x10, "100k 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	SNK_UNUSED_PORT
	SNK_ROTARY

INPUT_PORTS_END

INPUT_PORTS_START( ikarijp )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,IPT_UNKNOWN ) /* sound CPU status */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* tilt? */

	SNK_JOY1_PORT
	SNK_JOY2_PORT
	SNK_BUTTON_PORT

	PORT_START /* DSW 1 */
	PORT_DIPNAME( 0x01, 0x01, "Allow killing each other" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "P1 & P2 Fire Buttons" )
	PORT_DIPSETTING(    0x02, "Separate" )
	PORT_DIPSETTING(    0x00, "Common" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance" )
	PORT_DIPSETTING(    0x04, "1st & every 2nd" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	SNK_COINAGE

	PORT_START /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Demo Sounds On" )
	PORT_DIPSETTING(    0x04, "Freeze" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "50k 100k" )
	PORT_DIPSETTING(    0x20, "60k 120k" )
	PORT_DIPSETTING(    0x10, "100k 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SNK_UNUSED_PORT
	SNK_ROTARY

INPUT_PORTS_END


INPUT_PORTS_START( victroad )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) 	/* sound related ??? */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	SNK_JOY1_PORT
	SNK_JOY2_PORT
	SNK_BUTTON_PORT

	PORT_START /* DSW 1 */
	PORT_BITX( 0x01,    0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Kill friend & walk everywhere" ,0 ,0 )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "P1 & P2 Fire Buttons" )
	PORT_DIPSETTING(    0x02, "Separate" )
	PORT_DIPSETTING(    0x00, "Common" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance" )
	PORT_DIPSETTING(    0x04, "1st & every 2nd" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	SNK_COINAGE

	PORT_START /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Demo Sounds On" )
	PORT_DIPSETTING(    0x00, "Freeze" )
	PORT_BITX( 0,       0x04, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "50k 100k" )
	PORT_DIPSETTING(    0x20, "60k 120k" )
	PORT_DIPSETTING(    0x10, "100k 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	SNK_UNUSED_PORT
	SNK_ROTARY

INPUT_PORTS_END


INPUT_PORTS_START( gwar )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) 	/* sound related ??? */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* causes reset */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	SNK_JOY1_PORT
	SNK_JOY2_PORT
	SNK_BUTTON_PORT

	PORT_START /* DSW 1 */
	PORT_DIPNAME( 0x01, 0x01, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance" )
	PORT_DIPSETTING(    0x04, "1st & every 2nd" )
	PORT_DIPSETTING(    0x00, "1s7 & 2nd only" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	SNK_COINAGE

	PORT_START /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "30k 50k" )
	PORT_DIPSETTING(    0x20, "40k 80k" )
	PORT_DIPSETTING(    0x10, "50k 100k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40 ,0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SNK_UNUSED_PORT
	SNK_ROTARY

INPUT_PORTS_END

INPUT_PORTS_START( athena )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* sound CPU status */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SNK_UNUSED_PORT
	SNK_UNUSED_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurance" )
	PORT_DIPSETTING(    0x04, "1st & every 2nd" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	SNK_COINAGE

	PORT_START /* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "50k 100k" )
	PORT_DIPSETTING(    0x20, "80k 160k" )
	PORT_DIPSETTING(    0x10, "100k 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Energy" )
	PORT_DIPSETTING(    0x80, "12" )
	PORT_DIPSETTING(    0x00, "14" )
INPUT_PORTS_END


INPUT_PORTS_START( tnk3 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* sound CPU status */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED ) /*DIAL P1*/

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED ) /*DIAL P2*/

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SNK_UNUSED_PORT

	PORT_START	/* DSW1 */
	PORT_BITX( 0x01,    0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Walk everywhere", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coinage ) )
	/* 0x08 and 0x10: 1 Coin/1 Credit */
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xc0, "20k 60k" )
	PORT_DIPSETTING(    0x80, "40k 90k" )
	PORT_DIPSETTING(    0x40, "50k 120k" )
	PORT_DIPSETTING(    0x00, "None" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Bonus Occurance" )
	PORT_DIPSETTING(    0x01, "1st & every 2nd" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, "Easy?" )
	PORT_DIPSETTING(    0x04, "Normal?" )
	PORT_DIPSETTING(    0x02, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
	PORT_DIPNAME( 0x18, 0x10, "Game Mode" )
	PORT_DIPSETTING(    0x18, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x10, "Demo Sounds On" )
	PORT_DIPSETTING(    0x00, "Freeze" )
	PORT_BITX( 0,       0x08, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	SNK_ROTARY

INPUT_PORTS_END


INPUT_PORTS_START( bermudat )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* sound CPU status */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* tile? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	SNK_JOY1_PORT
	SNK_JOY2_PORT
	SNK_BUTTON_PORT
	SNK_UNUSED_PORT

	PORT_START  /* DSW 1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurrance" )
	PORT_DIPSETTING(    0x04, "1st & every 2nd" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	SNK_COINAGE

	PORT_START  /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x08, "Game Mode" )
	PORT_DIPSETTING(    0x08, "Demo Sounds On" )
	PORT_DIPSETTING(    0x0c, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x00, "Freeze" )
	PORT_BITX( 0,       0x04, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "50k 100k" )
	PORT_DIPSETTING(    0x20, "60k 120k" )
	PORT_DIPSETTING(    0x10, "100 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0xc0, "Game Style" )
	PORT_DIPSETTING(    0xc0, "Normal without continue" )
	PORT_DIPSETTING(    0x80, "Normal with continue" )
	PORT_DIPSETTING(    0x40, "Time attack 3 minutes" )
	PORT_DIPSETTING(    0x00, "Time attack 5 minutes" )

	SNK_ROTARY
INPUT_PORTS_END

INPUT_PORTS_START( psychos )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* sound related???*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* Reset */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )

	SNK_BUTTON_PORT
	SNK_UNUSED_PORT
	SNK_UNUSED_PORT

	PORT_START  /* DSW 1 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occurrance" )
	PORT_DIPSETTING(    0x00, "1st & every 2nd" )
	PORT_DIPSETTING(    0x04, "1st & 2nd only" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	SNK_COINAGE

	PORT_START  /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "50K 100K" )
	PORT_DIPSETTING(    0x20, "60K 120K" )
	PORT_DIPSETTING(    0x10, "100K 200K" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( legofair )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* sound CPU status */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )  /* Reset */
	PORT_BITX(0x08, 0x08, IPT_SERVICE, DEF_STR( Service_Mode), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SNK_UNUSED_PORT
	SNK_UNUSED_PORT

	PORT_START  /* DSW 1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Occourrance" )
	PORT_DIPSETTING(    0x00, "1st & every 2nd" )
	PORT_DIPSETTING(    0x04, "1st & 2nd only" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	SNK_COINAGE

	PORT_START  /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x0c, "Demo Sounds On" )
	PORT_DIPSETTING(    0x00, "Freeze" )
	PORT_BITX( 0,       0x04, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "50k 100k" )
	PORT_DIPSETTING(    0x20, "75k 150k" )
	PORT_DIPSETTING(    0x10, "100k 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x80,    0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability" , IP_KEY_NONE ,IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( fitegolf ) /* bogus */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* sound related */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SNK_UNUSED_PORT
	SNK_UNUSED_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Continue?" )
	PORT_DIPSETTING(    0x01, "Coin Up" )
	PORT_DIPSETTING(    0x00, "Standard" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen?" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bonus" )
	PORT_DIPSETTING(    0x04, "Every" )
	PORT_DIPSETTING(    0x00, "Only" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START /* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty?" )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "Demo Sounds?" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Bonus?" )
	PORT_DIPSETTING(    0x30, "50k 100k?" )
	PORT_DIPSETTING(    0x20, "60k 120k?" )
	PORT_DIPSETTING(    0x10, "100k 200k?" )
	PORT_DIPSETTING(    0x00, "None?" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ftsoccer )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* sound CPU status */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )

	SNK_UNUSED_PORT
	SNK_UNUSED_PORT
	SNK_UNUSED_PORT

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode?" )
	PORT_DIPSETTING(    0x08, "Normal?" )
	PORT_DIPSETTING(    0x0c, "Demo Sound On?" )
	PORT_DIPSETTING(    0x00, "Freeze?" )
	PORT_BITX( 0,       0x04, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Never Finish?", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x70, 0x70, "Play Time" )
	PORT_DIPSETTING(    0x00, "2:10" )
	PORT_DIPSETTING(    0x70, "2:00" )
	PORT_DIPSETTING(    0x20, "1:50" )
	PORT_DIPSETTING(    0x30, "1:40" )
	PORT_DIPSETTING(    0x40, "1:30" )
	PORT_DIPSETTING(    0x50, "1:20" )
	PORT_DIPSETTING(    0x60, "1:10" )
	PORT_DIPSETTING(    0x10, "1:00" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


INPUT_PORTS_START( tdfever )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 ) /* service? */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* sound CPU status */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 ) /* Single Coin */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )

	SNK_UNUSED_PORT
	SNK_UNUSED_PORT
	SNK_UNUSED_PORT

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Mode" )
	PORT_DIPSETTING(    0x08, "Normal?" )
	PORT_DIPSETTING(    0x0c, "Demo Sound On?" )
	PORT_DIPSETTING(    0x00, "Freeze" )
	PORT_BITX( 0,       0x04, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Never Finish?", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

/***********************************************************************/

static void init_ikari(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/*  Hack ROM test */
	RAM[0x11a6] = 0x00;
	RAM[0x11a7] = 0x00;
	RAM[0x11a8] = 0x00;

	/* Hack Incorrect port value */
	RAM[0x1003] = 0xc3;
	RAM[0x1004] = 0x02;
	RAM[0x1005] = 0x10;

	dial_type = DIAL_NORMAL;
	hard_flags = 1;
}

static void init_ikarijp(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	RAM[0x190b] = 0xc9; /* faster test */

	dial_type = DIAL_NORMAL;
	hard_flags = 1;
}

static void init_ikarijpb(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	RAM[0x190b] = 0xc9; /* faster test */

	dial_type = DIAL_BOOTLEG;
	hard_flags = 1;
}

static void init_victroad(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	/* Hack ROM test */
	RAM[0x17bd] = 0x00;
	RAM[0x17be] = 0x00;
	RAM[0x17bf] = 0x00;

	/* Hack Incorrect port value */
	RAM[0x161a] = 0xc3;
	RAM[0x161b] = 0x19;
	RAM[0x161c] = 0x16;

	dial_type = DIAL_NORMAL;
	hard_flags = 1;
}

static void init_dogosoke(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	/* Hack ROM test */
	RAM[0x179f] = 0x00;
	RAM[0x17a0] = 0x00;
	RAM[0x17a1] = 0x00;

	/* Hack Incorrect port value */
	RAM[0x15fc] = 0xc3;
	RAM[0x15fd] = 0xfb;
	RAM[0x15fe] = 0x15;

	dial_type = DIAL_NORMAL;
	hard_flags = 1;
}

static void init_gwar(void)
{
	dial_type = DIAL_NORMAL;
	hard_flags = 0;
}

static void init_chopper(void)
{
	dial_type = DIAL_NONE;
	hard_flags = 0;
}

static void init_bermudat(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	// Patch Turbo Error
	RAM[0x127e] = 0xc9;
	RAM[0x118d] = 0x00;
	RAM[0x118e] = 0x00;

	dial_type = DIAL_NORMAL;
	hard_flags = 0;
}

static void init_tdfever( void )
{
	dial_type = DIAL_NONE;
	hard_flags = 0;
}

static void init_tnk3( void )
{
	dial_type = DIAL_NORMAL;
	hard_flags = 0;
}

static void init_athena( void )
{
	dial_type = DIAL_NONE;
	hard_flags = 0;
}

static void init_psychos( void )
{
	dial_type = DIAL_NONE;
	hard_flags = 0;
}



GAME( 1985, tnk3,     0,        tnk3,     tnk3,     tnk3,     ROT270,       "SNK", "TNK III (US?)" )
GAME( 1985, tnk3j,    tnk3,     tnk3,     tnk3,     tnk3,     ROT270,       "SNK", "Tank (Japan)" )
GAME( 1986, athena,   0,        athena,   athena,   athena,   ROT0_16BIT,   "SNK", "Athena" )
GAMEX(1988, fitegolf, 0,        athena,   fitegolf, 0,        ROT0,         "SNK", "Fighting Golf", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAME( 1986, ikari,    0,        ikari,    ikari,    ikari,    ROT270,       "SNK", "Ikari Warriors (US)" )
GAME( 1986, ikarijp,  ikari,    ikari,    ikarijp,  ikarijp,  ROT270,       "SNK", "Ikari Warriors (Japan)" )
GAME( 1986, ikarijpb, ikari,    ikari,    ikarijp,  ikarijpb, ROT270,       "bootleg", "Ikari Warriors (Japan bootleg)" )
GAME( 1986, victroad, 0,        victroad, victroad, victroad, ROT270,       "SNK", "Victory Road" )
GAME( 1986, dogosoke, victroad, victroad, victroad, dogosoke, ROT270,       "SNK", "Dogou Souken" )
GAME( 1987, gwar,     0,        gwar,     gwar,     gwar,     ROT270,       "SNK", "Guerrilla War" )
GAMEX(1987, bermudat, 0,        bermudat, bermudat, bermudat, ROT270_16BIT, "SNK", "Bermuda Triangle (World?)", GAME_IMPERFECT_SOUND )
GAMEX(1987, bermudaj, bermudat, bermudat, bermudat, bermudat, ROT270_16BIT, "SNK", "Bermuda Triangle (Japan)", GAME_IMPERFECT_SOUND )
GAME( 1987, psychos,  0,        psychos,  psychos,  psychos,  ROT0_16BIT,   "SNK", "Psycho Soldier (US)" )
GAME( 1987, psychosa, psychos,  psychos,  psychos,  psychos,  ROT0_16BIT,   "SNK", "Psycho Soldier (Japan)" )
GAMEX(1988, chopper,  0,        chopper1, legofair, chopper,  ROT270_16BIT, "SNK", "Chopper I", GAME_IMPERFECT_SOUND )
GAMEX(1988, legofair, chopper,  chopper1, legofair, chopper,  ROT270_16BIT, "SNK", "Koukuu Kihei Monogatari - The Legend of Air Cavalry", GAME_IMPERFECT_SOUND )
GAMEX(1988, ftsoccer, 0,        psychos,  ftsoccer, 0,        ROT0_16BIT,   "SNK", "Fighting Soccer", GAME_IMPERFECT_SOUND )
GAMEX(1987, tdfever,  0,        tdfever,  tdfever,  tdfever,  ROT270,       "SNK", "TouchDown Fever", GAME_IMPERFECT_SOUND )
GAMEX(1987, tdfeverj, tdfever,  tdfever,  tdfever,  tdfever,  ROT270,       "SNK", "TouchDown Fever (Japan)", GAME_IMPERFECT_SOUND )
