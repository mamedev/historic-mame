#include "driver.h"
#include "system16.h"

data16_t *sys16_workingram;
data16_t *sys16_workingram2;
data16_t *sys16_extraram;
data16_t *sys16_extraram2;
data16_t *sys16_extraram3;

static void patch_codeX( int offset, int data, int cpu ){
	int aligned_offset = offset&0xfffffe;
	data16_t *mem = (data16_t *)memory_region(REGION_CPU1+cpu);
	int old_word = mem[aligned_offset/2];

	if( offset&1 )
		data = (old_word&0xff00)|data;
	else
		data = (old_word&0x00ff)|(data<<8);

	mem[aligned_offset/2] = data;
}

void sys16_patch_code( int offset, int data ){
	patch_codeX(offset,data,0);
}


MACHINE_INIT( sys16_onetime ){
	sys16_bg1_trans=0;
	sys16_rowscroll_scroll=0;
	sys18_splittab_bg_x=0;
	sys18_splittab_bg_y=0;
	sys18_splittab_fg_x=0;
	sys18_splittab_fg_y=0;
}

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

struct GfxDecodeInfo sys16_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0, &charlayout,	0, 1024 },
	{ -1 } /* end of array */
};


/* sound */

static void sound_cause_nmi( int chip ){
	/* upd7759 callback */
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}


struct YM2151interface sys16_ym2151_interface = {
	1,			/* 1 chip */
	4000000,	/* 3.58 MHz ? */
	{ YM3012_VOL(32,MIXER_PAN_LEFT,32,MIXER_PAN_RIGHT) },
	{ 0 }
};

struct DACinterface sys16_7751_dac_interface =
{
	1,
	{ 80 }
};


struct upd7759_interface sys16_upd7759_interface =
{
	1,			/* 1 chip */
	{ UPD7759_STANDARD_CLOCK },
	{ 48 }, 	/* volumes */
	{ 0 },			/* memory region 3 contains the sample data */
	{ sound_cause_nmi },
};

struct RF5C68interface sys18_rf5c68_interface = {
  8000000,
  100
};

struct YM2612interface sys18_ym3438_interface =
{
	2,	/* 2 chips */
	8000000,
	{ YM3012_VOL(40,MIXER_PAN_CENTER,40,MIXER_PAN_CENTER),
			YM3012_VOL(40,MIXER_PAN_CENTER,40,MIXER_PAN_CENTER) },	/* Volume */
	{ 0 },	{ 0 },	{ 0 },	{ 0 }
};

int sys18_sound_info[4*2];

