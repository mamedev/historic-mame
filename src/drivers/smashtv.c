/*************************************************************************

  Driver for Williams/Midway games using the TMS34010 processor.

  Created by Alex Pasadyn and Zsolt Vasvari with some help from Kurt Mahan

  Currently playable:
  ------------------

  	 - Smash Tv
	 - Total Carnage
	 - Mortal Kombat
	 - Narc
	 - Strike Force
	 - Trog (prototype and release versions)
	 - Hi Impact Football
     - Terminator 2


  Not Playable:
  ------------

	 - Mortal Kombat II (protection, some bankswitching)
	 - NBA Jam          (protection)
	 - Super Hi Impact  (plays end immaturely, TMS34010 core problem?)


  Known Bugs:
  ----------

     - High scores don't display in MK and Trog. We have no clue right now
	   as to why.

	 - Strike Force hangs after beating the mother alien. Might be a
	   protecion issue, but it's purely a speculation.

	 - Once in a while the "Milky Way" portion of the background in
	   Strike Force is miscolored

	 - When the Porsche spins in Narc, the wheels are missing for a single
	   frame. This actually might be there on the original, because if the
	   game runs over 60% (as it does now on my PII 266) it's very hard to
	   notice. With 100% framerate, it would be invisible.

	 - Save state is commented out because it only works without sound

	 - Sound has some problems, especially in Smash TV. Since we cannot
	   run these games at 100% with sound on, it's hard to test for
	   accuracy


  To Do:
  -----

     - Check for auto-erase more than once per frame
	   (not sure if this feature is actually used)

	 - Verify screen sizes

	 - Verify unknown DIP switches

	 - Verify inputs

	 - More cleanups


  Theory:
  ------

     - BANK1 = program ROM
	 - BANK2 = RAM
	 - BANK4 = RAM (palette RAM included here)
	 - BANK5 = sound ROM
	 - BANK6 = sound ROM
	 - BANK8 = graphics ROM

**************************************************************************/
#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "machine/6821pia.h"

/* Variables in vidhrdw/smashtv.c */
extern unsigned char *wms_videoram;
extern           int wms_videoram_size;

/* Variables in machine/smashtv.c */
extern unsigned char *wms_cmos_ram;
extern           int wms_bank2_size;
static           int wms_bank4_size;
extern           int wms_code_rom_size;
extern           int wms_gfx_rom_size;
static           int wms_paletteram_size;

/* Functions in vidhrdw/smashtv.c */
int wms_vh_start(void);
int wms_t_vh_start(void);
void wms_vh_stop (void);
void wms_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void wms_vram_w(int offset, int data);
int wms_vram_r(int offset);

/* Functions in machine/smashtv.c */
void smashtv_init_machine(void);
void mk_init_machine(void);
void term2_init_machine(void);
void trog_init_machine(void);
void narc_init_machine(void);
void mk2_init_machine(void);
void nbajam_init_machine(void);

void wms_sysreg_w(int offset, int data);
void wms_sysreg2_w(int offset, int data);

void wms_cmos_w(int offset, int data);
int wms_cmos_r(int offset);

void wms_01c00060_w(int offset, int data);
int wms_01c00060_r(int offset);

void wms_unk1_w(int offset, int data);
void wms_unk2_w(int offset, int data);

int wms_dma_r(int offset);
void wms_dma_w(int offset, int data);
void wms_dma2_w(int offset, int data);

int wms_input_r (int offset);

void narc_music_bank_select_w (int offset,int data);
void narc_digitizer_bank_select_w (int offset,int data);
void smashtv_sound_bank_select_w (int offset,int data);
void mk_sound_bank_select_w (int offset,int data);

void narc_driver_init(void);
void smashtv_driver_init(void);
void smashtv4_driver_init(void);
void trog_driver_init(void);
void trog3_driver_init(void);
void trogp_driver_init(void);
void mk_driver_init(void);
void mkla1_driver_init(void);
void mkla2_driver_init(void);
void mk2_driver_init(void);
void mk2r14_driver_init(void);
void nbajam_driver_init(void);
void totcarn_driver_init(void);
void totcarnp_driver_init(void);
void hiimpact_driver_init(void);
void shimpact_driver_init(void);
void strkforc_driver_init(void);
void term2_driver_init(void);

/* Functions in sndhrdw/smashtv.c */
void smashtv_ym2151_int (int irq);
void narc_ym2151_int (int irq);
void mk_sound_talkback_w (int offset,int data);
int  narc_DAC_r(int offset);
void narc_slave_DAC_w (int offset,int data);
void narc_slave_cmd_w (int offset,int data);

unsigned int wms_rom_loaded;

/* This just causes the init_machine to copy the images again */
static void wms_decode(void)
{
	wms_rom_loaded = 0;
}

static struct MemoryReadAddress smashtv_readmem[] =
{
	{ TOBYTE(0x00000000), TOBYTE(0x001fffff), wms_vram_r }, /* VRAM */
	{ TOBYTE(0x01000000), TOBYTE(0x010fffff), MRA_BANK2, 0, &wms_bank2_size }, /* RAM */
	{ TOBYTE(0x01400000), TOBYTE(0x0140ffff), wms_cmos_r }, /* CMOS RAM */
/*	{ TOBYTE(0x0181f000), TOBYTE(0x0181ffff), paletteram_word_r }, */
/*	{ TOBYTE(0x01810000), TOBYTE(0x0181ffff), paletteram_word_r }, */
/*	{ TOBYTE(0x01800000), TOBYTE(0x0181ffff), paletteram_word_r }, */
	{ TOBYTE(0x01800000), TOBYTE(0x019fffff), MRA_BANK4, 0, &wms_bank4_size }, /* RAM */
	{ TOBYTE(0x01a80000), TOBYTE(0x01a8001f), wms_dma_r },
	{ TOBYTE(0x01c00000), TOBYTE(0x01c0005f), wms_input_r },
	{ TOBYTE(0x01c00060), TOBYTE(0x01c0007f), wms_01c00060_r },
	{ TOBYTE(0x02000000), TOBYTE(0x05ffffff), MRA_BANK8, 0, &wms_gfx_rom_size }, /* GFX ROMS */
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MRA_BANK1, 0, &wms_code_rom_size },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress smashtv_writemem[] =
{
	{ TOBYTE(0x00000000), TOBYTE(0x001fffff), wms_vram_w }, /* VRAM */
	{ TOBYTE(0x01000000), TOBYTE(0x010fffff), MWA_BANK2 }, /* RAM */
	{ TOBYTE(0x01400000), TOBYTE(0x0140ffff), wms_cmos_w }, /* CMOS RAM */
/*	{ TOBYTE(0x0181f000), TOBYTE(0x0181ffff), paletteram_xRRRRRGGGGGBBBBB_word_w, 0, &wms_paletteram_size }, */
/*	{ TOBYTE(0x01810000), TOBYTE(0x0181ffff), paletteram_xRRRRRGGGGGBBBBB_word_w, 0, &wms_paletteram_size }, */
/*	{ TOBYTE(0x01800000), TOBYTE(0x0181ffff), paletteram_xRRRRRGGGGGBBBBB_word_w, 0, &wms_paletteram_size }, */
	{ TOBYTE(0x01800000), TOBYTE(0x019fffff), MWA_BANK4 }, /* RAM */
	{ TOBYTE(0x01a00000), TOBYTE(0x01a0009f), wms_dma_w },
	{ TOBYTE(0x01a80000), TOBYTE(0x01a8009f), wms_dma_w },
	{ TOBYTE(0x01c00060), TOBYTE(0x01c0007f), wms_01c00060_w },
	{ TOBYTE(0x01e00000), TOBYTE(0x01e0001f), MWA_NOP }, /* sound */
/*	{ TOBYTE(0x01e00000), TOBYTE(0x01e0001f), smashtv_sound_w }, */
/*	{ TOBYTE(0x01e00000), TOBYTE(0x01e0001f), mk_sound_w },		 */
/*	{ TOBYTE(0x01e00000), TOBYTE(0x01e0001f), narc_sound_w },	 */
	{ TOBYTE(0x01f00000), TOBYTE(0x01f0001f), wms_sysreg_w },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress mk2_readmem[] =
{
	{ TOBYTE(0x00000000), TOBYTE(0x003fffff), wms_vram_r }, /* VRAM */
	{ TOBYTE(0x01000000), TOBYTE(0x013fffff), MRA_BANK2, 0, &wms_bank2_size }, /* Sratch RAM UJ4/5/6/7 */
	{ TOBYTE(0x01400000), TOBYTE(0x0141ffff), wms_cmos_r },
	{ TOBYTE(0x01600000), TOBYTE(0x016000ff), wms_input_r },
//	{ TOBYTE(0x01800000), TOBYTE(0x0181ffff), paletteram_word_r },
	{ TOBYTE(0x01800000), TOBYTE(0x019fffff), MRA_BANK4, 0, &wms_bank4_size }, /* RAM */
	{ TOBYTE(0x01a80000), TOBYTE(0x01a8001f), wms_dma_r },
	{ TOBYTE(0x01b14000), TOBYTE(0x01b23fff), MRA_BANK3}, /* ???? */
//	{ TOBYTE(0x01d00000), TOBYTE(0x01d0005f), MRA_NOP }, /* ??? */
	{ TOBYTE(0x01d00000), TOBYTE(0x01d0001f), MRA_NOP }, /* ??? */
	/* checks 1d00000 for 0x8000 */
	{ TOBYTE(0x02000000), TOBYTE(0x07ffffff), MRA_BANK8, 0, &wms_gfx_rom_size }, /* GFX ROMS */
	{ TOBYTE(0x04000000), TOBYTE(0x05ffffff), MRA_BANK7}, /* banked GFX ROMS */
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MRA_BANK1, 0, &wms_code_rom_size },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress mk2_writemem[] =
{
	{ TOBYTE(0x00000000), TOBYTE(0x003fffff), wms_vram_w }, /* VRAM */
	{ TOBYTE(0x01000000), TOBYTE(0x013fffff), MWA_BANK2 }, /* Scratch RAM */
	{ TOBYTE(0x01400000), TOBYTE(0x0141ffff), wms_cmos_w }, /* ??? */
//	{ TOBYTE(0x01480000), TOBYTE(0x0148001f), MWA_NOP },  /* w from ffa4d3a0 (mk2) */
//	{ TOBYTE(0x014fffe0), TOBYTE(0x014fffff), MWA_NOP }, /* w from ff9daed0 (nbajam) */
//	{ TOBYTE(0x01800000), TOBYTE(0x0181ffff), paletteram_xRRRRRGGGGGBBBBB_word_w, 0, &wms_paletteram_size },
	{ TOBYTE(0x01800000), TOBYTE(0x019fffff), MWA_BANK4 }, /* RAM */
	{ TOBYTE(0x01a80000), TOBYTE(0x01a800ff), wms_dma2_w },
	{ TOBYTE(0x01b00000), TOBYTE(0x01b0001f), wms_unk1_w }, /* sysreg (mk2) */
	{ TOBYTE(0x01b14000), TOBYTE(0x01b23fff), MWA_BANK3}, /* ???? */
	{ TOBYTE(0x01c00060), TOBYTE(0x01c0007f), wms_01c00060_w },
	{ TOBYTE(0x01d01000), TOBYTE(0x01d010ff), wms_unk2_w }, /* ???? */
	{ TOBYTE(0x01d01020), TOBYTE(0x01d0103f), MWA_NOP }, /* sound */
	{ TOBYTE(0x01d81060), TOBYTE(0x01d8107f), MWA_NOP }, /* ???? */
	/* 1d01070, 1d81070 == watchdog?? */
	{ TOBYTE(0x01f00000), TOBYTE(0x01f0001f), wms_sysreg2_w },  /* only nbajam */
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_w },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress narc_music_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM},
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2800, 0x2800, soundlatch3_w }, /* talkback port */
	{ 0x2c00, 0x2c00, narc_slave_cmd_w }, /* COMM (for slave) */
	{ 0x3000, 0x3000, DAC_data_w },
	{ 0x3800, 0x3800, narc_music_bank_select_w },
	{ 0x3c00, 0x3c00, MWA_NOP}, /* SYNC */
	{ 0x4000, 0xffff, MWA_ROM},
	{ -1 }
};
static struct MemoryReadAddress narc_music_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM},
	{ 0x2001, 0x2001, YM2151_status_port_0_r },
	{ 0x3000, 0x3000, narc_DAC_r },
	{ 0x3400, 0x3400, soundlatch_r},
	{ 0x4000, 0xbfff, MRA_BANK5},
	{ 0xc000, 0xffff, MRA_ROM},
	  { -1 }
};
static struct MemoryWriteAddress narc_digitizer_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM},
	{ 0x2000, 0x2000, CVSD_clock_w },
	{ 0x2400, 0x2400, CVSD_dig_and_clk_w },
	{ 0x3000, 0x3000, narc_slave_DAC_w },
	{ 0x3800, 0x3800, narc_digitizer_bank_select_w },
	{ 0x3c00, 0x3c00, MWA_NOP}, /* SYNC */
	{ 0x4000, 0xffff, MWA_ROM},
	{ -1 }
};
static struct MemoryReadAddress narc_digitizer_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM},
	{ 0x3000, 0x3000, narc_DAC_r },
	{ 0x3400, 0x3400, soundlatch2_r},
	{ 0x4000, 0xbfff, MRA_BANK6},
	{ 0xc000, 0xffff, MRA_ROM},
	  { -1 }
};

static struct MemoryReadAddress smashtv_sound_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x2001, 0x2001, YM2151_status_port_0_r },
	{ 0x4000, 0x4003, pia_0_r },
	{ 0x8000, 0xffff, MRA_BANK5},
	  { -1 }
};
static struct MemoryWriteAddress smashtv_sound_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM},
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x4000, 0x4003, pia_0_w },
	{ 0x6000, 0x6000, CVSD_dig_and_clk_w },
	{ 0x6800, 0x6800, CVSD_clock_w },
	{ 0x7800, 0x7800, smashtv_sound_bank_select_w },
	{ 0x8000, 0xffff, MWA_ROM},
	{ -1 }
};

static struct MemoryReadAddress mk_sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2401, 0x2401, YM2151_status_port_0_r },
	{ 0x2c00, 0x2c00, OKIM6295_status_0_r },
	{ 0x3000, 0x3000, soundlatch_r },
	{ 0x4000, 0xbfff, MRA_BANK5},
	{ 0xc000, 0xffff, MRA_BANK6},
	{ -1 }
};

void mk_adpcm_bs_w(int offset, int data);

static struct MemoryWriteAddress mk_sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM},
	{ 0x2000, 0x2000, mk_sound_bank_select_w },
	{ 0x2400, 0x2400, YM2151_register_port_0_w },
	{ 0x2401, 0x2401, YM2151_data_port_0_w },
	{ 0x2800, 0x2800, DAC_data_w },
	{ 0x2c00, 0x2c00, OKIM6295_data_0_w },
	{ 0x3400, 0x3400, mk_adpcm_bs_w }, /* PCM-BS */
	{ 0x3c00, 0x3c00, mk_sound_talkback_w }, /* talkback port? */
	{ 0x4000, 0xffff, MWA_ROM},
	{ -1 }
};

INPUT_PORTS_START( narc_input_ports )

	PORT_START      /* IN0 - player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )

	PORT_START      /* IN1 - player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START	    /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Advance", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Vault Switch", KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START	    /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) /* T/B strobe */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* memory protect */
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0xc0, "Language" )
	PORT_DIPSETTING(    0xc0, "English" )
	PORT_DIPSETTING(    0x80, "French" )
	PORT_DIPSETTING(    0x40, "German" )
	PORT_DIPSETTING(    0x00, "unknown" )

INPUT_PORTS_END

INPUT_PORTS_START( trog_input_ports )

	PORT_START      /* IN0 - player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 - player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Service Credit", KEYCODE_9, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 ) /* coin3 */

	PORT_START	    /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) /* video freeze */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )

	PORT_START	    /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN5 */
	PORT_DIPNAME( 0xff, 0xff, "IN5" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* DS1 */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, "Coinage Select" )
	PORT_DIPSETTING(    0x40, "Dipswitch Coinage" )
	PORT_DIPSETTING(    0x00, "CMOS Coinage" )
	PORT_DIPNAME( 0x38, 0x38, "Coinage" )
	PORT_DIPSETTING(    0x38, "1" )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x30, "ECA" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x07, 0x07, "Unused" )
	PORT_DIPSETTING(    0x07, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* DS2 */
	PORT_DIPNAME( 0xc0, 0xc0, "Country" )
	PORT_DIPSETTING(    0xc0, "USA" )
	PORT_DIPSETTING(    0x80, "French" )
	PORT_DIPSETTING(    0x40, "German" )
	PORT_DIPSETTING(    0x00, "Unused" )
	PORT_DIPNAME( 0x20, 0x00, "Powerup Test" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPNAME( 0x10, 0x00, "Counters" )
	PORT_DIPSETTING(    0x10, "One Counter" )
	PORT_DIPSETTING(    0x00, "Two Counters" )
	PORT_DIPNAME( 0x0c, 0x0c, "Players" )
	PORT_DIPSETTING(    0x0c, "4 Players" )
	PORT_DIPSETTING(    0x04, "3 Players" )
	PORT_DIPSETTING(    0x08, "2 Players" )
	PORT_DIPSETTING(    0x00, "1 Player" )
	PORT_DIPNAME( 0x02, 0x02, "Video Freeze" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* IN8 */
	PORT_DIPNAME( 0xff, 0xff, "IN8" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_START	    /* IN9 */
	PORT_DIPNAME( 0xff, 0xff, "IN9" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

INPUT_PORTS_END

INPUT_PORTS_START( smashtv_input_ports )

	PORT_START      /* IN0 - player 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP, "Move Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN, "Move Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT, "Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT, "Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP, "Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN, "Fire Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT, "Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT, "Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* IN1 - player 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_PLAYER2, "2 Move Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_PLAYER2, "2 Move Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_PLAYER2, "2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_PLAYER2, "2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_PLAYER2, "2 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2, "2 Fire Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_PLAYER2, "2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER2, "2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START	    /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Service Credit", KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 ) /* coin3 */

	PORT_START	    /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) /* video freeze */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 ) /* coin4 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* DS1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* DS2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN8 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN9 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END

INPUT_PORTS_START( strkforc_input_ports )

	PORT_START      /* IN0 - player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 - player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Service Credit", KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 ) /* coin3 */

	PORT_START	    /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN4 */
	PORT_DIPNAME( 0xff, 0xff, "IN4" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_START	    /* IN5 */
	PORT_DIPNAME( 0xff, 0xff, "IN5" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* DS1 */
	PORT_DIPNAME( 0xe0, 0xe0, "Difficulty" )
	PORT_DIPSETTING(    0xe0, "7?" )
	PORT_DIPSETTING(    0xc0, "6?" )
	PORT_DIPSETTING(    0xa0, "5?" )
	PORT_DIPSETTING(    0x80, "4?" )
	PORT_DIPSETTING(    0x60, "3?" )
	PORT_DIPSETTING(    0x40, "2?" )
	PORT_DIPSETTING(    0x20, "1?" )
	PORT_DIPSETTING(    0x00, "0?" )
	PORT_DIPNAME( 0x10, 0x10, "Ships" )
	PORT_DIPSETTING(    0x10, "1?" )
	PORT_DIPSETTING(    0x00, "0?" )
	PORT_DIPNAME( 0x0c, 0x0c, "Points for Ship" )
	PORT_DIPSETTING(    0x0c, "c?" )
	PORT_DIPSETTING(    0x08, "8?" )
	PORT_DIPSETTING(    0x04, "4?" )
	PORT_DIPSETTING(    0x00, "0?" )
	PORT_DIPNAME( 0x02, 0x02, "Credits to Start" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x01, 0x01, "Coin Meter" )
	PORT_DIPSETTING(    0x01, "1?" )
	PORT_DIPSETTING(    0x00, "0?" )

	PORT_START	    /* DS2 */
	PORT_DIPNAME( 0x80, 0x80, "Test Switch" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x78, 0x78, "Coin 1" )
	PORT_DIPSETTING(    0x78, "????" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x07, 0x07, "Coin 2" )
	PORT_DIPSETTING(    0x07, "???" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* IN8 */
	PORT_DIPNAME( 0xff, 0xff, "IN8" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* IN9 */
	PORT_DIPNAME( 0xff, 0xff, "IN9" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

INPUT_PORTS_END

INPUT_PORTS_START( mk_input_ports )

	PORT_START      /* IN0 - player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 - player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Service Credit", KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 ) /* coin3 */

	PORT_START	    /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN4 ) /* video freeze */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )

	PORT_START	    /* IN4 */
	PORT_DIPNAME( 0xff, 0xff, "IN4" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_START	    /* IN5 */
	PORT_DIPNAME( 0xff, 0xff, "IN5" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* DS1 */
	PORT_DIPNAME( 0x80, 0x80, "Violence" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Blood" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Low Blows" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Attract Sound" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Comic Book Offer" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )

	PORT_START	    /* DS2 */
	PORT_DIPNAME( 0x80, 0x00, "Coinage Source" )
	PORT_DIPSETTING(    0x80, "Dipswitch" )
	PORT_DIPSETTING(    0x00, "CMOS" )
	PORT_DIPNAME( 0x7c, 0x7c, "Coinage" )
	PORT_DIPSETTING(    0x7c, "USA-1" )
	PORT_DIPSETTING(    0x3c, "USA-2" )
	PORT_DIPSETTING(    0x5c, "USA-3" )
	PORT_DIPSETTING(    0x1c, "USA-4" )
	PORT_DIPSETTING(    0x6c, "USA-ECA" )
	PORT_DIPSETTING(    0x74, "German-1" )
	PORT_DIPSETTING(    0x34, "German-2" )
	PORT_DIPSETTING(    0x54, "German-3" )
	PORT_DIPSETTING(    0x14, "German-4" )
	PORT_DIPSETTING(    0x64, "German-5" )
	PORT_DIPSETTING(    0x78, "French-1" )
	PORT_DIPSETTING(    0x38, "French-2" )
	PORT_DIPSETTING(    0x58, "French-3" )
	PORT_DIPSETTING(    0x18, "French-4" )
	PORT_DIPSETTING(    0x68, "French-ECA" )
	PORT_DIPSETTING(    0x0c, "Free Play" )
	PORT_DIPNAME( 0x02, 0x00, "Counters" )
	PORT_DIPSETTING(    0x02, "One" )
	PORT_DIPSETTING(    0x00, "Two" )
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* IN8 */
	PORT_DIPNAME( 0xff, 0xff, "IN8" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* IN9 */
	PORT_DIPNAME( 0xff, 0xff, "IN9" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

INPUT_PORTS_END

INPUT_PORTS_START( term2_input_ports )

	PORT_START      /* IN0 - player 1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) /* trigger */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) /* bomb */
	PORT_BIT( 0xcf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 - player 2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* trigger */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) /* bomb */
	PORT_BIT( 0xcf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Service Credit", KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 ) /* coin3 */

	PORT_START	    /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) /* video freeze */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 ) /* coin4 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN4 */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X|IPF_REVERSE | IPF_PLAYER1, 20, 10, 0x0f, 0, 0xff)

	PORT_START	    /* IN5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* DS1 */
	PORT_DIPNAME( 0x80, 0x00, "Normal Display" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Dipswitch Coinage" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x38, 0x38, "Coinage" )
	PORT_DIPSETTING(    0x38, "1" )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x30, "USA ECA" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x07, 0x03, "Credits" )
	PORT_DIPSETTING(    0x07, "2 Start/1 Continue" )
	PORT_DIPSETTING(    0x06, "4 Start/1 Continue" )
	PORT_DIPSETTING(    0x05, "2 Start/2 Continue" )
	PORT_DIPSETTING(    0x04, "4 Start/2 Continue" )
	PORT_DIPSETTING(    0x03, "1 Start/1 Continue" )
	PORT_DIPSETTING(    0x02, "3 Start/2 Continue" )
	PORT_DIPSETTING(    0x01, "3 Start/1 Continue" )
	PORT_DIPSETTING(    0x00, "3 Start/3 Continue" )

	PORT_START	    /* DS2 */
	PORT_DIPNAME( 0xc0, 0xc0, "Country" )
	PORT_DIPSETTING(    0xc0, "USA" )
	PORT_DIPSETTING(    0x80, "French" )
	PORT_DIPSETTING(    0x40, "German" )
	PORT_DIPSETTING(    0x00, "Unused" )
	PORT_DIPNAME( 0x20, 0x00, "Powerup Test" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPNAME( 0x10, 0x00, "Two Counters" )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Players" )
	PORT_DIPSETTING(    0x08, "2 Players" )
	PORT_DIPSETTING(    0x00, "1 Player" )
	PORT_DIPNAME( 0x04, 0x04, "Unused" )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Video Freeze" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* IN8 */
	PORT_DIPNAME( 0xff, 0xff, "IN8" )
	PORT_DIPSETTING(    0xff, "On" )
	PORT_DIPSETTING(    0x00, "Off" )

	PORT_START	    /* IN9 */
	PORT_DIPNAME( 0xff, 0xff, "IN9" )
	PORT_DIPSETTING(    0xff, "On" )
	PORT_DIPSETTING(    0x00, "Off" )

	PORT_START	    /* IN10 */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER1, 20, 10, 0x0f, 0, 0xff)

	PORT_START	    /* IN11 */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER2, 20, 10, 0x0f, 0, 0xff)

	PORT_START	    /* IN12 */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER2, 20, 10, 0x0f, 0, 0xff)

INPUT_PORTS_END

INPUT_PORTS_START( totcarn_input_ports )

	PORT_START      /* IN0 - player 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP, "Move Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN, "Move Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT, "Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT, "Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP, "Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN, "Fire Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT, "Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT, "Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* IN1 - player 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_PLAYER2, "2 Move Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_PLAYER2, "2 Move Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_PLAYER2, "2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_PLAYER2, "2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_PLAYER2, "2 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2, "2 Fire Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_PLAYER2, "2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER2, "2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START	    /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Service Credit", KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 ) /* coin3 */

	PORT_START	    /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) /* video freeze */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 ) /* coin4 */
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* DS1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* DS2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN8 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN9 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END

INPUT_PORTS_START( mk2_input_ports )

	PORT_START      /* IN0 - player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 - player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Service Credit", KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 ) /* coin3 */

	PORT_START	    /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* volume down */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* volume up */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )

	PORT_START	    /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN5 */
	PORT_DIPNAME( 0xff, 0xff, "IN5" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* DS1 */
	PORT_DIPNAME( 0x80, 0x00, "Coinage Source" )
	PORT_DIPSETTING(    0x80, "Dipswitch" )
	PORT_DIPSETTING(    0x00, "CMOS" )
	PORT_DIPNAME( 0x7c, 0x7c, "Coinage" )
	PORT_DIPSETTING(    0x7c, "USA-1" )
	PORT_DIPSETTING(    0x3c, "USA-2" )
	PORT_DIPSETTING(    0x5c, "USA-3" )
	PORT_DIPSETTING(    0x1c, "USA-4" )
	PORT_DIPSETTING(    0x6c, "USA-ECA" )
	PORT_DIPSETTING(    0x74, "German-1" )
	PORT_DIPSETTING(    0x34, "German-2" )
	PORT_DIPSETTING(    0x54, "German-3" )
	PORT_DIPSETTING(    0x14, "German-4" )
	PORT_DIPSETTING(    0x64, "German-5" )
	PORT_DIPSETTING(    0x78, "French-1" )
	PORT_DIPSETTING(    0x38, "French-2" )
	PORT_DIPSETTING(    0x58, "French-3" )
	PORT_DIPSETTING(    0x18, "French-4" )
	PORT_DIPSETTING(    0x68, "French-ECA" )
	PORT_DIPSETTING(    0x0c, "Free Play" )
	PORT_DIPNAME( 0x02, 0x00, "Counters" )
	PORT_DIPSETTING(    0x02, "One" )
	PORT_DIPSETTING(    0x00, "Two" )
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* DS2 */
	PORT_DIPNAME( 0x80, 0x80, "Violence" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Blood" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Low Blows" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Attract Sound" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Comic Book Offer" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Bill Validator" )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Powerup Test" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPNAME( 0x01, 0x01, "Circuit Boards" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "1" )


	PORT_START	    /* IN8 */
	PORT_DIPNAME( 0xff, 0xff, "IN8" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_START	    /* IN9 */
	PORT_DIPNAME( 0xff, 0xff, "IN9" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

INPUT_PORTS_END

INPUT_PORTS_START( nbajam_input_ports )

	PORT_START      /* IN0 - player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 - player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Service Credit", KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 ) /* coin3 */

	PORT_START	    /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* volume down */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* volume up */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )

	PORT_START	    /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	    /* DS1 */
	PORT_DIPNAME( 0x80, 0x80, "Players" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPNAME( 0x40, 0x40, "Validator" )
	PORT_DIPSETTING(    0x00, "Installed" )
	PORT_DIPSETTING(    0x40, "None" )
	PORT_DIPNAME( 0x20, 0x20, "Video" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "Show" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x02, 0x00, "Powerup Test" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	    /* DS2 */
	PORT_DIPNAME( 0x80, 0x00, "Coinage Source" )
	PORT_DIPSETTING(    0x80, "Dipswitch" )
	PORT_DIPSETTING(    0x00, "CMOS" )
	PORT_DIPNAME( 0x7c, 0x7c, "Coinage" )
	PORT_DIPSETTING(    0x7c, "USA-1" )
	PORT_DIPSETTING(    0x3c, "USA-2" )
	PORT_DIPSETTING(    0x5c, "USA-3" )
	PORT_DIPSETTING(    0x1c, "USA-4" )
	PORT_DIPSETTING(    0x6c, "USA-ECA" )
	PORT_DIPSETTING(    0x74, "German-1" )
	PORT_DIPSETTING(    0x34, "German-2" )
	PORT_DIPSETTING(    0x54, "German-3" )
	PORT_DIPSETTING(    0x14, "German-4" )
	PORT_DIPSETTING(    0x64, "German-5" )
	PORT_DIPSETTING(    0x78, "French-1" )
	PORT_DIPSETTING(    0x38, "French-2" )
	PORT_DIPSETTING(    0x58, "French-3" )
	PORT_DIPSETTING(    0x18, "French-4" )
	PORT_DIPSETTING(    0x68, "French-ECA" )
	PORT_DIPSETTING(    0x0c, "Free Play" )
	PORT_DIPNAME( 0x03, 0x00, "Coin Counters" )
	PORT_DIPSETTING(    0x03, "1 Counter, 1 count/coin" )
	PORT_DIPSETTING(    0x02, "1 Counter, Totalizing" )
	PORT_DIPSETTING(    0x01, "2 Counters, 1 count/coin" )
	PORT_DIPSETTING(    0x00, "1 Counter, 1 count/coin" )


	PORT_START	    /* IN8 */
	PORT_DIPNAME( 0xff, 0xff, "IN8" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_START	    /* IN9 */
	PORT_DIPNAME( 0xff, 0xff, "IN9" )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

INPUT_PORTS_END

/*
 *   Sound interface
 */

static struct CVSDinterface cvsd_interface =
{
	1,          /* 1 chip */
	{ 30 }
};

static struct DACinterface dac_interface =
{
	1,          /* 1 chip */
	{ 30 }
};
static struct DACinterface narc_dac_interface =
{
	2,          /* DAC, DAC2 */
	{ 30, 30 }
};
static struct YM2151interface ym2151_interface =
{
	1,          /* 1 chip */
	3579545,    /* 3.579545 MHz */
	{ YM3012_VOL(30,MIXER_PAN_LEFT,30,MIXER_PAN_RIGHT) },
	{ smashtv_ym2151_int }
};
static struct YM2151interface narc_ym2151_interface =
{
	1,          /* 1 chip */
	3579545,    /* 3.579545 MHz */
	{ YM3012_VOL(30,MIXER_PAN_LEFT,30,MIXER_PAN_RIGHT) },
	{ narc_ym2151_int }
};
static struct OKIM6295interface okim6295_interface =
{
	1,          /* 1 chip */
	{ 8000 },       /* 8000 Hz frequency */
	{ 3 },          /* memory region 4 */
	{ 50 }
};

void mk_adpcm_bs_w(int offset, int data)
{
	if (!(data&0x04))
	{
		okim6295_interface.region[0]=3;
	}
	else
	{
		if (data&0x01)
		{
			okim6295_interface.region[0]=4;
		}
		else
		{
			okim6295_interface.region[0]=5;
		}
	}
	if (errorlog) fprintf(errorlog, "adpcm-bs 0x%x --> 0x%x\n", data, okim6295_interface.region[0]);
}

/* Y-unit games */
static struct MachineDriver smashtv_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS34010,
			50000000,	/* 50 Mhz */
			0,
			smashtv_readmem,smashtv_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			8000000,	/* 8 Mhz */
			2,
			smashtv_sound_readmem,smashtv_sound_writemem,0,0,
			ignore_interrupt,0
		},
	},
	57, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	smashtv_init_machine,

	/* video hardware */
	512, 288, { 0, 394, 20, 275 },

	0,
	65536,0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT,
	0,
	wms_vh_start,
	wms_vh_stop,
	wms_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_HC55516,
			&cvsd_interface
		}
	}
};

/* Z-Unit */
static struct MachineDriver narc_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS34010,
			48000000,	/* 48 Mhz */
			0,
			smashtv_readmem,smashtv_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			narc_music_readmem,narc_music_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			3,
			narc_digitizer_readmem,narc_digitizer_writemem,0,0,
			ignore_interrupt,0
		},
	},
	57, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	narc_init_machine,

	/* video hardware */
    512, 432, { 0, 511, 27, 426 },

	0,
	65536,0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT,
	0,
	wms_vh_start,
	wms_vh_stop,
	wms_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_DAC,
			&narc_dac_interface
		},
		{
			SOUND_YM2151,
			&narc_ym2151_interface
		},
		{
			SOUND_HC55516,
			&cvsd_interface
		}
	}
};

static struct MachineDriver trog_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS34010,
			50000000,	/* 50 Mhz */
			0,
			smashtv_readmem,smashtv_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			8000000,	/* 8 Mhz */
			2,
			smashtv_sound_readmem,smashtv_sound_writemem,0,0,
			ignore_interrupt,0
		},
	},
	57, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	trog_init_machine,

	/* video hardware */
	512, 288, { 0, 395, 20, 275 },

	0,
	65536,0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT,
	0,
	wms_vh_start,
	wms_vh_stop,
	wms_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_HC55516,
			&cvsd_interface
		}
	}
};

/* Y-Unit */
static struct MachineDriver mk_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS34010,
			50000000,	/* 50 Mhz */
			0,
			smashtv_readmem,smashtv_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			8000000,	/* 8 Mhz */
			2,
			mk_sound_readmem,mk_sound_writemem,0,0,
			ignore_interrupt,0
		},
	},
	57, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1, /* cpu slices */
	mk_init_machine,

	/* video hardware */
	512, 304, { 0, 399, 27, 281 },

	0,
	65536,0,
    0,

    VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT,
	0,
	wms_vh_start,
	wms_vh_stop,
	wms_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_YM2151,
			&narc_ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/* Y-Unit */
static struct MachineDriver term2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS34010,
			50000000,	/* 50 Mhz */
			0,
			smashtv_readmem,smashtv_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			8000000,	/* 8 Mhz */
			2,
			mk_sound_readmem,mk_sound_writemem,0,0,
			ignore_interrupt,0
		},
	},
	57, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1, /* cpu slices */
	term2_init_machine,

	/* video hardware */
	512, 304, { 0, 399, 27, 281 },

	0,
	65536,0,
    0,

    VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT,
	0,
	wms_vh_start,
	wms_vh_stop,
	wms_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_YM2151,
			&narc_ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/* T-Unit */
static struct MachineDriver mk2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS34010,
			50000000,	/* 50 Mhz */
			0,
			mk2_readmem,mk2_writemem,0,0,
			ignore_interrupt,0
		},
	},
	57, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	mk2_init_machine,

	/* video hardware */
	512, 512, { 54, 452, 0, 255 },

	0,
	65536,0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT,
	0,
	wms_t_vh_start,
	wms_vh_stop,
	wms_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

/* T-Unit */
static struct MachineDriver nbajam_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS34010,
			50000000,	/* 50 Mhz */
			0,
			mk2_readmem,mk2_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			8000000,	/* 2 Mhz */
			2,
			mk_sound_readmem,mk_sound_writemem,0,0,
			ignore_interrupt,0
		},
	},
	57, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	nbajam_init_machine,

	/* video hardware */
	512, 512, { 54, 452, 0, 255 },

	0,
	65536,0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT,
	0,
	wms_t_vh_start,
	wms_vh_stop,
	wms_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_YM2151,
			&narc_ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};


/***************************************************************************

  High score save/load

***************************************************************************/

static int hiload (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f,wms_cmos_ram,0x8000);
		osd_fclose (f);
	}

	return 1;
}

static void hisave (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
	if (f)
	{
		osd_fwrite(f,wms_cmos_ram,0x8000);
		osd_fclose (f);
	}
}

void wms_stateload(void)
{
	int i;
	void *f;
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_STATE,0)) != 0)
	{
		if (errorlog) fprintf(errorlog,"Loading State...\n");
		TMS34010_State_Load(0,f);
		osd_fread(f,wms_videoram,wms_videoram_size);
		osd_fread(f,cpu_bankbase[2],wms_bank2_size);
		//osd_fread(f,cpu_bankbase[4],wms_bank4_size);
		osd_fread(f,wms_cmos_ram,0x8000);
		osd_fread(f,cpu_bankbase[8],wms_gfx_rom_size);
		osd_fread(f,paletteram,wms_paletteram_size);
		for (i=0;i<wms_paletteram_size;i+=2)
		{
			paletteram_xRRRRRGGGGGBBBBB_word_w(i,READ_WORD(&paletteram[i]));
		}
		if (errorlog) fprintf(errorlog,"State loaded\n");
		osd_fclose(f);
	}
}

void wms_statesave(void)
{
	void *f;
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_STATE,1))!= 0)
	{
		if (errorlog) fprintf(errorlog,"Saving State...\n");
		TMS34010_State_Save(0,f);
		osd_fwrite(f,wms_videoram,wms_videoram_size);
		osd_fwrite(f,cpu_bankbase[2],wms_bank2_size);
		//osd_fwrite(f,cpu_bankbase[4],wms_bank4_size);
		osd_fwrite(f,wms_cmos_ram,0x8000);
		osd_fwrite(f,cpu_bankbase[8],wms_gfx_rom_size);
		osd_fwrite(f,paletteram,wms_paletteram_size);
		if (errorlog) fprintf(errorlog,"State saved\n");
		osd_fclose(f);
	}
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( narc_rom )
	ROM_REGION(0x100000)     /*34010 code */
	ROM_LOAD_ODD ( "u42",  0x80000, 0x20000, 0xd1111b76 )  /* even */
	ROM_LOAD_EVEN( "u24",  0x80000, 0x20000, 0xaa0d3082 )  /* odd  */
	ROM_LOAD_ODD ( "u41",  0xc0000, 0x20000, 0x3903191f )  /* even */
	ROM_LOAD_EVEN( "u23",  0xc0000, 0x20000, 0x7a316582 )  /* odd  */

	ROM_REGION(0x800000)      /* graphics (mapped as code) */
	ROM_LOAD ( "u94",  0x000000, 0x10000, 0xca3194e4 )  /* even */
	ROM_LOAD ( "u76",  0x200000, 0x10000, 0x1cd897f4 )  /* odd  */
	ROM_LOAD ( "u93",  0x010000, 0x10000, 0x0ed7f7f5 )  /* even */
	ROM_LOAD ( "u75",  0x210000, 0x10000, 0x78abfa01 )  /* odd  */
	ROM_LOAD ( "u92",  0x020000, 0x10000, 0x40d2fc66 )  /* even */
	ROM_LOAD ( "u74",  0x220000, 0x10000, 0x66d2a234 )  /* odd  */
	ROM_LOAD ( "u91",  0x030000, 0x10000, 0xf39325e0 )  /* even */
	ROM_LOAD ( "u73",  0x230000, 0x10000, 0xefa5cd4e )  /* odd  */
	ROM_LOAD ( "u90",  0x040000, 0x10000, 0x0132aefa )  /* even */
	ROM_LOAD ( "u72",  0x240000, 0x10000, 0x70638eb5 )  /* odd  */
	ROM_LOAD ( "u89",  0x050000, 0x10000, 0xf7260c9e )  /* even */
	ROM_LOAD ( "u71",  0x250000, 0x10000, 0x61226883 )  /* odd  */
	ROM_LOAD ( "u88",  0x060000, 0x10000, 0xedc19f42 )  /* even */
	ROM_LOAD ( "u70",  0x260000, 0x10000, 0xc808849f )  /* odd  */
	ROM_LOAD ( "u87",  0x070000, 0x10000, 0xd9b42ff9 )  /* even */
	ROM_LOAD ( "u69",  0x270000, 0x10000, 0xe7f9c34f )  /* odd  */
	ROM_LOAD ( "u86",  0x080000, 0x10000, 0xaf7daad3 )  /* even */
	ROM_LOAD ( "u68",  0x280000, 0x10000, 0x88a634d5 )  /* odd  */
	ROM_LOAD ( "u85",  0x090000, 0x10000, 0x095fae6b )  /* even */
	ROM_LOAD ( "u67",  0x290000, 0x10000, 0x4ab8b69e )  /* odd  */
	ROM_LOAD ( "u84",  0x0a0000, 0x10000, 0x3fdf2057 )  /* even */
	ROM_LOAD ( "u66",  0x2a0000, 0x10000, 0xe1da4b25 )  /* odd  */
	ROM_LOAD ( "u83",  0x0b0000, 0x10000, 0xf2d27c9f )  /* even */
	ROM_LOAD ( "u65",  0x2b0000, 0x10000, 0x6df0d125 )  /* odd  */
	ROM_LOAD ( "u82",  0x0c0000, 0x10000, 0x962ce47c )  /* even */
	ROM_LOAD ( "u64",  0x2c0000, 0x10000, 0xabab1b16 )  /* odd  */
	ROM_LOAD ( "u81",  0x0d0000, 0x10000, 0x00fe59ec )  /* even */
	ROM_LOAD ( "u63",  0x2d0000, 0x10000, 0x80602f31 )  /* odd  */
	ROM_LOAD ( "u80",  0x0e0000, 0x10000, 0x147ba8e9 )  /* even */
	ROM_LOAD ( "u62",  0x2e0000, 0x10000, 0xc2a476d1 )  /* odd  */

	ROM_LOAD ( "u58",  0x400000, 0x10000, 0x8a7501e3 )  /* even */
	ROM_LOAD ( "u40",  0x600000, 0x10000, 0x7fcaebc7 )  /* odd  */
	ROM_LOAD ( "u57",  0x410000, 0x10000, 0xa504735f )  /* even */
	ROM_LOAD ( "u39",  0x610000, 0x10000, 0x7db5cf52 )  /* odd  */
	ROM_LOAD ( "u56",  0x420000, 0x10000, 0x55f8cca7 )  /* even */
	ROM_LOAD ( "u38",  0x620000, 0x10000, 0x3f9f3ef7 )  /* odd  */
	ROM_LOAD ( "u55",  0x430000, 0x10000, 0xd3c932c1 )  /* even */
	ROM_LOAD ( "u37",  0x630000, 0x10000, 0xed81826c )  /* odd  */
	ROM_LOAD ( "u54",  0x440000, 0x10000, 0xc7f4134b )  /* even */
	ROM_LOAD ( "u36",  0x640000, 0x10000, 0xe5d855c0 )  /* odd  */
	ROM_LOAD ( "u53",  0x450000, 0x10000, 0x6be4da56 )  /* even */
	ROM_LOAD ( "u35",  0x650000, 0x10000, 0x3a7b1329 )  /* odd  */
	ROM_LOAD ( "u52",  0x460000, 0x10000, 0x1ea36a4a )  /* even */
	ROM_LOAD ( "u34",  0x660000, 0x10000, 0xfe982b0e )  /* odd  */
	ROM_LOAD ( "u51",  0x470000, 0x10000, 0x9d4b0324 )  /* even */
	ROM_LOAD ( "u33",  0x670000, 0x10000, 0x6bc7eb0f )  /* odd  */
	ROM_LOAD ( "u50",  0x480000, 0x10000, 0x6f9f0c26 )  /* even */
	ROM_LOAD ( "u32",  0x680000, 0x10000, 0x5875a6d3 )  /* odd  */
	ROM_LOAD ( "u49",  0x490000, 0x10000, 0x80386fce )  /* even */
	ROM_LOAD ( "u31",  0x690000, 0x10000, 0x2fa4b8e5 )  /* odd  */
	ROM_LOAD ( "u48",  0x4a0000, 0x10000, 0x05c16185 )  /* even */
	ROM_LOAD ( "u30",  0x6a0000, 0x10000, 0x7e4bb8ee )  /* odd  */
	ROM_LOAD ( "u47",  0x4b0000, 0x10000, 0x4c0151f1 )  /* even */
	ROM_LOAD ( "u29",  0x6b0000, 0x10000, 0x45136fd9 )  /* odd  */
	ROM_LOAD ( "u46",  0x4c0000, 0x10000, 0x5670bfcb )  /* even */
	ROM_LOAD ( "u28",  0x6c0000, 0x10000, 0xd6cdac24 )  /* odd  */
	ROM_LOAD ( "u45",  0x4d0000, 0x10000, 0x27f10d98 )  /* even */
	ROM_LOAD ( "u27",  0x6d0000, 0x10000, 0x4d33bbec )  /* odd  */
	ROM_LOAD ( "u44",  0x4e0000, 0x10000, 0x93b8eaa4 )  /* even */
	ROM_LOAD ( "u26",  0x6e0000, 0x10000, 0xcb19f784 )  /* odd  */

	ROM_REGION(0x30000)     /* sound CPU */
	ROM_LOAD ( "u5-snd", 0x00000, 0x10000, 0xe551e5e3 )
	ROM_RELOAD (         0x10000, 0x10000 )
	ROM_LOAD ( "u4-snd", 0x20000, 0x10000, 0x450a591a )

	ROM_REGION(0x50000)     /* slave sound CPU */
	ROM_LOAD ( "u38-snd", 0x00000, 0x10000, 0x09b03b80 )
	ROM_RELOAD (          0x10000, 0x10000 )
	ROM_LOAD ( "u37-snd", 0x20000, 0x10000, 0x29dbeffd )
	ROM_LOAD ( "u36-snd", 0x30000, 0x10000, 0x16cdbb13 )
	ROM_LOAD ( "u35-snd", 0x40000, 0x10000, 0x81295892 )

ROM_END

ROM_START( trog_rom )	/* released version */
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "trogu105.bin",  0xc0000, 0x20000, 0xe6095189 ) /* even */
	ROM_LOAD_EVEN( "trogu89.bin",   0xc0000, 0x20000, 0xfdd7cc65 ) /* odd */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "trogu111.bin",  0x000000, 0x20000, 0x9ded08c1 )  /* even */
	ROM_LOAD ( "trogu112.bin",  0x020000, 0x20000, 0x42293843 )  /* even */
	ROM_LOAD ( "trogu113.bin",  0x040000, 0x20000, 0x77f50cbb )  /* even */

	ROM_LOAD (  "trogu95.bin",  0x200000, 0x20000, 0xf3ba2838 )  /* odd  */
	ROM_LOAD (  "trogu96.bin",  0x220000, 0x20000, 0xcfed2e77 )  /* odd  */
	ROM_LOAD (  "trogu97.bin",  0x240000, 0x20000, 0x3262d1f8 )  /* odd  */

	ROM_LOAD ( "trogu106.bin",  0x080000, 0x20000, 0xaf2eb0d8 )  /* even */
 	ROM_LOAD ( "trogu107.bin",  0x0a0000, 0x20000, 0x88a7b3f6 )  /* even */

	ROM_LOAD (  "trogu90.bin",  0x280000, 0x20000, 0x16e06753 )  /* odd  */
	ROM_LOAD (  "trogu91.bin",  0x2a0000, 0x20000, 0x880a02c7 )  /* odd  */

	ROM_REGION(0x70000) /* sound CPU */
	ROM_LOAD (   "trogu4.bin", 0x10000, 0x10000, 0x759d0bf4 )
	ROM_LOAD (  "trogu19.bin", 0x30000, 0x10000, 0x960c333d )
	ROM_LOAD (  "trogu20.bin", 0x50000, 0x10000, 0x67f1658a )

ROM_END

ROM_START( trog3_rom )	/* released version */
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "u105-la3",  0xc0000, 0x20000, 0xd09cea97 ) /* even */
	ROM_LOAD_EVEN( "u89-la3",   0xc0000, 0x20000, 0xa61e3572 ) /* odd */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "trogu111.bin",  0x000000, 0x20000, 0x9ded08c1 )  /* even */
	ROM_LOAD ( "trogu112.bin",  0x020000, 0x20000, 0x42293843 )  /* even */
	ROM_LOAD ( "trogu113.bin",  0x040000, 0x20000, 0x77f50cbb )  /* even */

	ROM_LOAD (  "trogu95.bin",  0x200000, 0x20000, 0xf3ba2838 )  /* odd  */
	ROM_LOAD (  "trogu96.bin",  0x220000, 0x20000, 0xcfed2e77 )  /* odd  */
	ROM_LOAD (  "trogu97.bin",  0x240000, 0x20000, 0x3262d1f8 )  /* odd  */

	ROM_LOAD ( "trogu106.bin",  0x080000, 0x20000, 0xaf2eb0d8 )  /* even */
 	ROM_LOAD ( "trogu107.bin",  0x0a0000, 0x20000, 0x88a7b3f6 )  /* even */

	ROM_LOAD (  "trogu90.bin",  0x280000, 0x20000, 0x16e06753 )  /* odd  */
	ROM_LOAD (  "trogu91.bin",  0x2a0000, 0x20000, 0x880a02c7 )  /* odd  */

	ROM_REGION(0x70000) /* sound CPU */
	ROM_LOAD (   "trogu4.bin", 0x10000, 0x10000, 0x759d0bf4 )
	ROM_LOAD (  "trogu19.bin", 0x30000, 0x10000, 0x960c333d )
	ROM_LOAD (  "trogu20.bin", 0x50000, 0x10000, 0x67f1658a )

ROM_END

ROM_START( trogp_rom )   /* prototype version */
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "trog105.dat",  0xc0000, 0x20000, 0x526a3f5b ) /* even */
	ROM_LOAD_EVEN( "trog89.dat",   0xc0000, 0x20000, 0x38d68685 ) /* odd */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "trogu111.bin",  0x000000, 0x20000, 0x9ded08c1 )  /* even */
	ROM_LOAD ( "trogu112.bin",  0x020000, 0x20000, 0x42293843 )  /* even */
	ROM_LOAD ( "trog113.dat",  0x040000, 0x20000, 0x2980a56f )  /* even */

	ROM_LOAD (  "trogu95.bin",  0x200000, 0x20000, 0xf3ba2838 )  /* odd  */
	ROM_LOAD (  "trogu96.bin",  0x220000, 0x20000, 0xcfed2e77 )  /* odd  */
	ROM_LOAD (  "trog97.dat",  0x240000, 0x20000, 0xf94b77c1 )  /* odd  */

	ROM_LOAD ( "trogu106.bin",  0x080000, 0x20000, 0xaf2eb0d8 )  /* even */
 	ROM_LOAD ( "trogu107.bin",  0x0a0000, 0x20000, 0x88a7b3f6 )  /* even */

	ROM_LOAD (  "trogu90.bin",  0x280000, 0x20000, 0x16e06753 )  /* odd  */
	ROM_LOAD (  "trogu91.bin",  0x2a0000, 0x20000, 0x880a02c7 )  /* odd  */

	ROM_REGION(0x70000) /* sound CPU */
	ROM_LOAD (   "trogu4.bin", 0x10000, 0x10000, 0x759d0bf4 )
	ROM_LOAD (  "trogu19.bin", 0x30000, 0x10000, 0x960c333d )
	ROM_LOAD (  "trogu20.bin", 0x50000, 0x10000, 0x67f1658a )

ROM_END

ROM_START( smashtv_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "u105.l8",  0xc0000, 0x20000, 0x48cd793f ) /* even */
	ROM_LOAD_EVEN( "u89.l8",   0xc0000, 0x20000, 0x8e7fe463 ) /* odd */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "u111.gam",  0x000000, 0x20000, 0x72f0ba84 )  /* even */
	ROM_LOAD ( "u112.gam",  0x020000, 0x20000, 0x436f0283 )  /* even */
	ROM_LOAD ( "u113.gam",  0x040000, 0x20000, 0x4a4b8110 )  /* even */

	ROM_LOAD (  "u95.gam",  0x200000, 0x20000, 0xe864a44b )  /* odd  */
	ROM_LOAD (  "u96.gam",  0x220000, 0x20000, 0x15555ea7 )  /* odd  */
	ROM_LOAD (  "u97.gam",  0x240000, 0x20000, 0xccac9d9e )  /* odd  */

 	ROM_LOAD ( "u106.gam",  0x400000, 0x20000, 0x5c718361 )  /* even */
 	ROM_LOAD ( "u107.gam",  0x420000, 0x20000, 0x0fba1e36 )  /* even */
 	ROM_LOAD ( "u108.gam",  0x440000, 0x20000, 0xcb0a092f )  /* even */

	ROM_REGION(0x70000) /* sound CPU */
	ROM_LOAD (  "u4.snd", 0x10000, 0x10000, 0x29d3f6c8 )
	ROM_LOAD ( "u19.snd", 0x30000, 0x10000, 0xac5a402a )
	ROM_LOAD ( "u20.snd", 0x50000, 0x10000, 0x875c66d9 )

ROM_END

ROM_START( smashtv6_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "la6-u105",  0xc0000, 0x20000, 0xf1666017 ) /* even */
	ROM_LOAD_EVEN( "la6-u89",   0xc0000, 0x20000, 0x908aca5d ) /* odd */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "u111.gam",  0x000000, 0x20000, 0x72f0ba84 )  /* even */
	ROM_LOAD ( "u112.gam",  0x020000, 0x20000, 0x436f0283 )  /* even */
	ROM_LOAD ( "u113.gam",  0x040000, 0x20000, 0x4a4b8110 )  /* even */

	ROM_LOAD (  "u95.gam",  0x200000, 0x20000, 0xe864a44b )  /* odd  */
	ROM_LOAD (  "u96.gam",  0x220000, 0x20000, 0x15555ea7 )  /* odd  */
	ROM_LOAD (  "u97.gam",  0x240000, 0x20000, 0xccac9d9e )  /* odd  */

 	ROM_LOAD ( "u106.gam",  0x400000, 0x20000, 0x5c718361 )  /* even */
 	ROM_LOAD ( "u107.gam",  0x420000, 0x20000, 0x0fba1e36 )  /* even */
 	ROM_LOAD ( "u108.gam",  0x440000, 0x20000, 0xcb0a092f )  /* even */

	ROM_REGION(0x70000) /* sound CPU */
	ROM_LOAD (  "u4.snd", 0x10000, 0x10000, 0x29d3f6c8 )
	ROM_LOAD ( "u19.snd", 0x30000, 0x10000, 0xac5a402a )
	ROM_LOAD ( "u20.snd", 0x50000, 0x10000, 0x875c66d9 )

ROM_END

ROM_START( smashtv5_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "u105-v5",  0xc0000, 0x20000, 0x81f564b9 ) /* even */
	ROM_LOAD_EVEN( "u89-v5",   0xc0000, 0x20000, 0xe5017d25 ) /* odd */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "u111.gam",  0x000000, 0x20000, 0x72f0ba84 )  /* even */
	ROM_LOAD ( "u112.gam",  0x020000, 0x20000, 0x436f0283 )  /* even */
	ROM_LOAD ( "u113.gam",  0x040000, 0x20000, 0x4a4b8110 )  /* even */

	ROM_LOAD (  "u95.gam",  0x200000, 0x20000, 0xe864a44b )  /* odd  */
	ROM_LOAD (  "u96.gam",  0x220000, 0x20000, 0x15555ea7 )  /* odd  */
	ROM_LOAD (  "u97.gam",  0x240000, 0x20000, 0xccac9d9e )  /* odd  */

 	ROM_LOAD ( "u106.gam",  0x400000, 0x20000, 0x5c718361 )  /* even */
 	ROM_LOAD ( "u107.gam",  0x420000, 0x20000, 0x0fba1e36 )  /* even */
 	ROM_LOAD ( "u108.gam",  0x440000, 0x20000, 0xcb0a092f )  /* even */

	ROM_REGION(0x70000) /* sound CPU */
	ROM_LOAD (  "u4.snd", 0x10000, 0x10000, 0x29d3f6c8 )
	ROM_LOAD ( "u19.snd", 0x30000, 0x10000, 0xac5a402a )
	ROM_LOAD ( "u20.snd", 0x50000, 0x10000, 0x875c66d9 )

ROM_END

ROM_START( smashtv4_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "la4-u105",  0xc0000, 0x20000, 0xa50ccb71 ) /* even */
	ROM_LOAD_EVEN( "la4-u89",   0xc0000, 0x20000, 0xef0b0279 ) /* odd */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "u111.gam",  0x000000, 0x20000, 0x72f0ba84 )  /* even */
	ROM_LOAD ( "u112.gam",  0x020000, 0x20000, 0x436f0283 )  /* even */
	ROM_LOAD ( "u113.gam",  0x040000, 0x20000, 0x4a4b8110 )  /* even */

	ROM_LOAD (  "u95.gam",  0x200000, 0x20000, 0xe864a44b )  /* odd  */
	ROM_LOAD (  "u96.gam",  0x220000, 0x20000, 0x15555ea7 )  /* odd  */
	ROM_LOAD (  "u97.gam",  0x240000, 0x20000, 0xccac9d9e )  /* odd  */

 	ROM_LOAD ( "u106.gam",  0x400000, 0x20000, 0x5c718361 )  /* even */
 	ROM_LOAD ( "u107.gam",  0x420000, 0x20000, 0x0fba1e36 )  /* even */
 	ROM_LOAD ( "u108.gam",  0x440000, 0x20000, 0xcb0a092f )  /* even */

	ROM_REGION(0x70000) /* sound CPU */
	ROM_LOAD (  "u4.snd", 0x10000, 0x10000, 0x29d3f6c8 )
	ROM_LOAD ( "u19.snd", 0x30000, 0x10000, 0xac5a402a )
	ROM_LOAD ( "u20.snd", 0x50000, 0x10000, 0x875c66d9 )

ROM_END

ROM_START( hiimpact_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "la3u105.bin",  0xc0000, 0x20000, 0xb9190c4a ) /* even */
	ROM_LOAD_EVEN( "la3u89.bin",   0xc0000, 0x20000, 0x1cbc72a5 ) /* odd */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "la1u111.bin",  0x000000, 0x20000, 0x49560560 )  /* even */
	ROM_LOAD ( "la1u112.bin",  0x020000, 0x20000, 0x4dd879dc )  /* even */
	ROM_LOAD ( "la1u113.bin",  0x040000, 0x20000, 0xb67aeb70 )  /* even */
	ROM_LOAD ( "la1u114.bin",  0x060000, 0x20000, 0x9a4bc44b )  /* even */

	ROM_LOAD (  "la1u95.bin",  0x200000, 0x20000, 0xe1352dc0 )  /* odd  */
	ROM_LOAD (  "la1u96.bin",  0x220000, 0x20000, 0x197d0f34 )  /* odd  */
	ROM_LOAD (  "la1u97.bin",  0x240000, 0x20000, 0x908ea575 )  /* odd  */
	ROM_LOAD (  "la1u98.bin",  0x260000, 0x20000, 0x6dcbab11 )  /* odd  */

 	ROM_LOAD ( "la1u106.bin",  0x400000, 0x20000, 0x7d0ead0d )  /* even */
	ROM_LOAD ( "la1u107.bin",  0x420000, 0x20000, 0xef48e8fa )  /* even */
	ROM_LOAD ( "la1u108.bin",  0x440000, 0x20000, 0x5f363e12 )  /* even */
	ROM_LOAD ( "la1u109.bin",  0x460000, 0x20000, 0x3689fbbc )  /* even */

	ROM_REGION(0x70000) /* sound CPU */
	ROM_LOAD (  "sl1u4.bin", 0x10000, 0x20000, 0x28effd6a )
	ROM_LOAD ( "sl1u19.bin", 0x30000, 0x20000, 0x0ea22c89 )
	ROM_LOAD ( "sl1u20.bin", 0x50000, 0x20000, 0x4e747ab5 )

ROM_END

ROM_START( shimpact_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "shiu105.bin",  0xc0000, 0x20000, 0xf2cf8de3 ) /* even */
	ROM_LOAD_EVEN( "shiu89.bin",   0xc0000, 0x20000, 0xf97d9b01 ) /* odd */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "shiu111.bin",  0x000000, 0x40000, 0x80ae2a86 ) /* even */
	ROM_LOAD ( "shiu112.bin",  0x040000, 0x40000, 0x3ffc27e9 ) /* even */
	ROM_LOAD ( "shiu113.bin",  0x080000, 0x40000, 0x01549d00 ) /* even */
	ROM_LOAD ( "shiu114.bin",  0x0c0000, 0x40000, 0xa68af319 ) /* even */

	ROM_LOAD (  "shiu95.bin",  0x200000, 0x40000, 0xe8f56ef5 ) /* odd  */
	ROM_LOAD (  "shiu96.bin",  0x240000, 0x40000, 0x24ed04f9 ) /* odd  */
	ROM_LOAD (  "shiu97.bin",  0x280000, 0x40000, 0xdd7f41a9 ) /* odd  */
	ROM_LOAD (  "shiu98.bin",  0x2c0000, 0x40000, 0x23ef65dd ) /* odd  */

	ROM_LOAD ( "shiu106.bin",  0x400000, 0x40000, 0x6f5bf337 ) /* even */
	ROM_LOAD ( "shiu107.bin",  0x440000, 0x40000, 0xa8815dad ) /* even */
	ROM_LOAD ( "shiu108.bin",  0x480000, 0x40000, 0xd39685a3 ) /* even */
	ROM_LOAD ( "shiu109.bin",  0x4c0000, 0x40000, 0x36e0b2b2 ) /* even */

	ROM_REGION(0x70000) /* sound CPU */
	ROM_LOAD (   "shiu4.bin", 0x10000, 0x20000, 0x1e5a012c )
	ROM_LOAD (  "shiu19.bin", 0x30000, 0x20000, 0x10f9684e )
	ROM_LOAD (  "shiu20.bin", 0x50000, 0x20000, 0x1b4a71c1 )

ROM_END

ROM_START( strkforc_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "sfu105.bin",  0xc0000, 0x20000, 0x7895e0e3 ) /* even */
	ROM_LOAD_EVEN( "sfu89.bin",   0xc0000, 0x20000, 0x26114d9e ) /* odd */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "sfu111.bin",  0x000000, 0x20000, 0x878efc80 )  /* even */
	ROM_LOAD ( "sfu112.bin",  0x020000, 0x20000, 0x93394399 )  /* even */
	ROM_LOAD ( "sfu113.bin",  0x040000, 0x20000, 0x9565a79b )  /* even */
	ROM_LOAD ( "sfu114.bin",  0x060000, 0x20000, 0xb71152da )  /* even */

	ROM_LOAD (  "sfu95.bin",  0x200000, 0x20000, 0x519cb2b4 )  /* odd  */
	ROM_LOAD (  "sfu96.bin",  0x220000, 0x20000, 0x61214796 )  /* odd  */
	ROM_LOAD (  "sfu97.bin",  0x240000, 0x20000, 0xeb5dee5f )  /* odd  */
	ROM_LOAD (  "sfu98.bin",  0x260000, 0x20000, 0xc5c079e7 )  /* odd  */

 	ROM_LOAD ( "sfu106.bin",  0x080000, 0x20000, 0xa394d4cf )  /* even */
 	ROM_LOAD ( "sfu107.bin",  0x0a0000, 0x20000, 0xedef1419 )  /* even */

	ROM_LOAD (  "sfu90.bin",  0x280000, 0x20000, 0x607bcdc0 )  /* odd  */
	ROM_LOAD (  "sfu91.bin",  0x2a0000, 0x20000, 0xda02547e )  /* odd  */

	ROM_REGION(0x70000) /* sound CPU */
	ROM_LOAD (  "sfu4.bin", 0x10000, 0x10000, 0x8f747312 )
	ROM_LOAD ( "sfu19.bin", 0x30000, 0x10000, 0xafb29926 )
	ROM_LOAD ( "sfu20.bin", 0x50000, 0x10000, 0x1bc9b746 )

ROM_END

ROM_START( mk_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "mkg-u105.rom",  0x00000, 0x80000, 0x2ce843c5 )  /* even */
	ROM_LOAD_EVEN(  "mkg-u89.rom",  0x00000, 0x80000, 0x49a46e10 )  /* odd  */

    ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "mkg-u111.rom",  0x000000, 0x80000, 0xd17096c4 )  /* even */
	ROM_LOAD ( "mkg-u112.rom",  0x080000, 0x80000, 0x993bc2e4 )  /* even */
	ROM_LOAD ( "mkg-u113.rom",  0x100000, 0x80000, 0x6fb91ede )  /* even */
	ROM_LOAD ( "mkg-u114.rom",  0x180000, 0x80000, 0xed1ff88a )  /* even */

 	ROM_LOAD (  "mkg-u95.rom",  0x200000, 0x80000, 0xa002a155 )  /* odd  */
 	ROM_LOAD (  "mkg-u96.rom",  0x280000, 0x80000, 0xdcee8492 )  /* odd  */
	ROM_LOAD (  "mkg-u97.rom",  0x300000, 0x80000, 0xde88caef )  /* odd  */
	ROM_LOAD (  "mkg-u98.rom",  0x380000, 0x80000, 0x37eb01b4 )  /* odd  */

	ROM_LOAD ( "mkg-u106.rom",  0x400000, 0x80000, 0x45acaf21 )  /* even */
 	ROM_LOAD ( "mkg-u107.rom",  0x480000, 0x80000, 0x2a6c10a0 )  /* even */
  	ROM_LOAD ( "mkg-u108.rom",  0x500000, 0x80000, 0x23308979 )  /* even */
 	ROM_LOAD ( "mkg-u109.rom",  0x580000, 0x80000, 0xcafc47bb )  /* even */

	ROM_REGION(0x50000) /* sound CPU */
	ROM_LOAD (  "mks-u3.rom", 0x10000, 0x40000, 0xc615844c )

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "mks-u12.rom", 0x00000, 0x40000, 0x258bd7f9 )
/*	ROM_LOAD ( "mks-u12.rom", 0x40000, 0x40000, 0x258bd7f9 ) */

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "mks-u13.rom", 0x00000, 0x20000, 0x7b7ec3b6 )
	ROM_CONTINUE            ( 0x40000, 0x20000 )
/*	ROM_LOAD ( "mks-u12.rom", 0x60000, 0x20000, 0x258bd7f9 ) */
/*	ROM_CONTINUE            ( 0x20000, 0x20000 ) */

	ROM_REGION(0x80000) /* ADPCM samples */
/*	ROM_LOAD ( "mks-u13.rom", 0x40000, 0x20000, 0x7b7ec3b6 ) */
/*	ROM_CONTINUE            ( 0x00000, 0x20000 ) */
/*	ROM_LOAD ( "mks-u12.rom", 0x60000, 0x20000, 0x258bd7f9 ) */
/*	ROM_CONTINUE            ( 0x20000, 0x20000 ) */

ROM_END

ROM_START( mkla1_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "mkg-u105.la1",  0x00000, 0x80000, 0xe1f7b4c9 )  /* even */
	ROM_LOAD_EVEN(  "mkg-u89.la1",  0x00000, 0x80000, 0x9d38ac75 )  /* odd  */

    ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "mkg-u111.rom",  0x000000, 0x80000, 0xd17096c4 )  /* even */
	ROM_LOAD ( "mkg-u112.rom",  0x080000, 0x80000, 0x993bc2e4 )  /* even */
	ROM_LOAD ( "mkg-u113.rom",  0x100000, 0x80000, 0x6fb91ede )  /* even */
	ROM_LOAD ( "mkg-u114.rom",  0x180000, 0x80000, 0xed1ff88a )  /* even */

 	ROM_LOAD (  "mkg-u95.rom",  0x200000, 0x80000, 0xa002a155 )  /* odd  */
 	ROM_LOAD (  "mkg-u96.rom",  0x280000, 0x80000, 0xdcee8492 )  /* odd  */
	ROM_LOAD (  "mkg-u97.rom",  0x300000, 0x80000, 0xde88caef )  /* odd  */
	ROM_LOAD (  "mkg-u98.rom",  0x380000, 0x80000, 0x37eb01b4 )  /* odd  */

	ROM_LOAD ( "mkg-u106.rom",  0x400000, 0x80000, 0x45acaf21 )  /* even */
 	ROM_LOAD ( "mkg-u107.rom",  0x480000, 0x80000, 0x2a6c10a0 )  /* even */
  	ROM_LOAD ( "mkg-u108.rom",  0x500000, 0x80000, 0x23308979 )  /* even */
 	ROM_LOAD ( "mkg-u109.rom",  0x580000, 0x80000, 0xcafc47bb )  /* even */

	ROM_REGION(0x50000) /* sound CPU */
	ROM_LOAD (  "mks-u3.rom", 0x10000, 0x40000, 0xc615844c )

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "mks-u12.rom", 0x00000, 0x40000, 0x258bd7f9 )
/*	ROM_LOAD ( "mks-u12.rom", 0x40000, 0x40000, 0x258bd7f9 ) */

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "mks-u13.rom", 0x00000, 0x20000, 0x7b7ec3b6 )
	ROM_CONTINUE            ( 0x40000, 0x20000 )
/*	ROM_LOAD ( "mks-u12.rom", 0x60000, 0x20000, 0x258bd7f9 ) */
/*	ROM_CONTINUE            ( 0x20000, 0x20000 ) */

	ROM_REGION(0x80000) /* ADPCM samples */
/*	ROM_LOAD ( "mks-u13.rom", 0x40000, 0x20000, 0x7b7ec3b6 ) */
/*	ROM_CONTINUE            ( 0x00000, 0x20000 ) */
/*	ROM_LOAD ( "mks-u12.rom", 0x60000, 0x20000, 0x258bd7f9 ) */
/*	ROM_CONTINUE            ( 0x20000, 0x20000 ) */

ROM_END

ROM_START( mkla2_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "mkg-u105.la2",  0x00000, 0x80000, 0x8531d44e )  /* even */
	ROM_LOAD_EVEN(  "mkg-u89.la2",  0x00000, 0x80000, 0xb88dc26e )  /* odd  */

    ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "mkg-u111.rom",  0x000000, 0x80000, 0xd17096c4 )  /* even */
	ROM_LOAD ( "mkg-u112.rom",  0x080000, 0x80000, 0x993bc2e4 )  /* even */
	ROM_LOAD ( "mkg-u113.rom",  0x100000, 0x80000, 0x6fb91ede )  /* even */
	ROM_LOAD ( "mkg-u114.rom",  0x180000, 0x80000, 0xed1ff88a )  /* even */

 	ROM_LOAD (  "mkg-u95.rom",  0x200000, 0x80000, 0xa002a155 )  /* odd  */
 	ROM_LOAD (  "mkg-u96.rom",  0x280000, 0x80000, 0xdcee8492 )  /* odd  */
	ROM_LOAD (  "mkg-u97.rom",  0x300000, 0x80000, 0xde88caef )  /* odd  */
	ROM_LOAD (  "mkg-u98.rom",  0x380000, 0x80000, 0x37eb01b4 )  /* odd  */

	ROM_LOAD ( "mkg-u106.rom",  0x400000, 0x80000, 0x45acaf21 )  /* even */
 	ROM_LOAD ( "mkg-u107.rom",  0x480000, 0x80000, 0x2a6c10a0 )  /* even */
  	ROM_LOAD ( "mkg-u108.rom",  0x500000, 0x80000, 0x23308979 )  /* even */
 	ROM_LOAD ( "mkg-u109.rom",  0x580000, 0x80000, 0xcafc47bb )  /* even */

	ROM_REGION(0x50000) /* sound CPU */
	ROM_LOAD (  "mks-u3.rom", 0x10000, 0x40000, 0xc615844c )

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "mks-u12.rom", 0x00000, 0x40000, 0x258bd7f9 )
/*	ROM_LOAD ( "mks-u12.rom", 0x40000, 0x40000, 0x258bd7f9 ) */

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "mks-u13.rom", 0x00000, 0x20000, 0x7b7ec3b6 )
	ROM_CONTINUE            ( 0x40000, 0x20000 )
/*	ROM_LOAD ( "mks-u12.rom", 0x60000, 0x20000, 0x258bd7f9 ) */
/*	ROM_CONTINUE            ( 0x20000, 0x20000 ) */

	ROM_REGION(0x80000) /* ADPCM samples */
/*	ROM_LOAD ( "mks-u13.rom", 0x40000, 0x20000, 0x7b7ec3b6 ) */
/*	ROM_CONTINUE            ( 0x00000, 0x20000 ) */
/*	ROM_LOAD ( "mks-u12.rom", 0x60000, 0x20000, 0x258bd7f9 ) */
/*	ROM_CONTINUE            ( 0x20000, 0x20000 ) */

ROM_END

ROM_START( term2_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "t2.105",  0x00000, 0x80000, 0x34142b28 )  /* even */
	ROM_LOAD_EVEN( "t2.89",   0x00000, 0x80000, 0x5ffea427 )  /* odd  */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "t2.111",  0x000000, 0x80000, 0x916d0197 )  /* even */
	ROM_LOAD ( "t2.112",  0x080000, 0x80000, 0x39ae1c87 )  /* even */
	ROM_LOAD ( "t2.113",  0x100000, 0x80000, 0xcb5084e5 )  /* even */
	ROM_LOAD ( "t2.114",  0x180000, 0x80000, 0x53c516ec )  /* even */

	ROM_LOAD (  "t2.95",  0x200000, 0x80000, 0xdd39cf73 )  /* odd  */
	ROM_LOAD (  "t2.96",  0x280000, 0x80000, 0x31f4fd36 )  /* odd  */
	ROM_LOAD (  "t2.97",  0x300000, 0x80000, 0x7f72e775 )  /* odd  */
	ROM_LOAD (  "t2.98",  0x380000, 0x80000, 0x1a20ce29 )  /* odd  */

	ROM_LOAD ( "t2.106",  0x400000, 0x80000, 0xf08a9536 )  /* even */
 	ROM_LOAD ( "t2.107",  0x480000, 0x80000, 0x268d4035 )  /* even */
 	ROM_LOAD ( "t2.108",  0x500000, 0x80000, 0x379fdaed )  /* even */
 	ROM_LOAD ( "t2.109",  0x580000, 0x80000, 0x306a9366 )  /* even */

	ROM_REGION(0x50000) /* sound CPU */
	ROM_LOAD (  "t2_snd.3", 0x10000, 0x20000, 0x73c3f5c4 )
	ROM_RELOAD (            0x30000, 0x20000 )

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "t2_snd.12", 0x00000, 0x40000, 0xe192a40d )
/*	ROM_LOAD ( "t2_snd.12", 0x40000, 0x40000, 0xe192a40d ) */

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "t2_snd.13", 0x00000, 0x20000, 0x956fa80b )
	ROM_CONTINUE          ( 0x40000, 0x20000             )
/*	ROM_LOAD ( "t2_snd.12", 0x60000, 0x20000, 0xe192a40d ) */
/*	ROM_CONTINUE          ( 0x20000, 0x20000             ) */

	ROM_REGION(0x80000) /* ADPCM samples */
/*	ROM_LOAD ( "t2_snd.13", 0x40000, 0x20000, 0x956fa80b ) */
/*	ROM_CONTINUE          ( 0x00000, 0x20000             ) */
/*	ROM_LOAD ( "t2_snd.12", 0x60000, 0x20000, 0xe192a40d ) */
/*	ROM_CONTINUE          ( 0x20000, 0x20000             ) */

ROM_END

ROM_START( totcarn_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "tcu105.bin",  0x80000, 0x40000, 0x7c651047 )  /* even */
	ROM_LOAD_EVEN( "tcu89.bin",   0x80000, 0x40000, 0x6761daf3 )  /* odd  */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "tcu111.bin",  0x000000, 0x40000, 0x13f3f231 )  /* even */
	ROM_LOAD ( "tcu112.bin",  0x040000, 0x40000, 0x72e45007 )  /* even */
	ROM_LOAD ( "tcu113.bin",  0x080000, 0x40000, 0x2c8ec753 )  /* even */
	ROM_LOAD ( "tcu114.bin",  0x0c0000, 0x40000, 0x6210c36c )  /* even */

	ROM_LOAD (  "tcu95.bin",  0x200000, 0x40000, 0x579caeba )  /* odd  */
	ROM_LOAD (  "tcu96.bin",  0x240000, 0x40000, 0xf43f1ffe )  /* odd  */
	ROM_LOAD (  "tcu97.bin",  0x280000, 0x40000, 0x1675e50d )  /* odd  */
	ROM_LOAD (  "tcu98.bin",  0x2c0000, 0x40000, 0xab06c885 )  /* odd  */

	ROM_LOAD ( "tcu106.bin",  0x400000, 0x40000, 0x146e3863 )  /* even */
 	ROM_LOAD ( "tcu107.bin",  0x440000, 0x40000, 0x95323320 )  /* even */
 	ROM_LOAD ( "tcu108.bin",  0x480000, 0x40000, 0xed152acc )  /* even */
 	ROM_LOAD ( "tcu109.bin",  0x4c0000, 0x40000, 0x80715252 )  /* even */

	ROM_REGION(0x50000) /* sound CPU */
	ROM_LOAD (  "tcu3.bin", 0x10000, 0x20000, 0x5bdb4665 )
	ROM_RELOAD (            0x30000, 0x20000 )

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "tcu12.bin", 0x00000, 0x40000, 0xd0000ac7 )
/*	ROM_LOAD ( "tcu12.bin", 0x40000, 0x40000, 0xd0000ac7 ) */

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "tcu13.bin", 0x00000, 0x20000, 0xe48e6f0c )
	ROM_CONTINUE          ( 0x40000, 0x20000             )
/*	ROM_LOAD ( "tcu12.bin", 0x60000, 0x20000, 0xd0000ac7 ) */
/*	ROM_CONTINUE          ( 0x20000, 0x20000             ) */

	ROM_REGION(0x80000) /* ADPCM samples */
/*	ROM_LOAD ( "tcu13.bin", 0x40000, 0x20000, 0xe48e6f0c ) */
/*	ROM_CONTINUE          ( 0x00000, 0x20000             ) */
/*	ROM_LOAD ( "tcu12.bin", 0x60000, 0x20000, 0xd0000ac7 ) */
/*	ROM_CONTINUE          ( 0x20000, 0x20000             ) */

ROM_END

ROM_START( totcarnp_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "u105",  0x80000, 0x40000, 0x7a782cae )  /* even */
	ROM_LOAD_EVEN( "u89",   0x80000, 0x40000, 0x1c899a8d )  /* odd  */

	ROM_REGION(0x800000)      /* graphics */
	ROM_LOAD ( "tcu111.bin",  0x000000, 0x40000, 0x13f3f231 )  /* even */
	ROM_LOAD ( "tcu112.bin",  0x040000, 0x40000, 0x72e45007 )  /* even */
	ROM_LOAD ( "tcu113.bin",  0x080000, 0x40000, 0x2c8ec753 )  /* even */
	ROM_LOAD ( "tcu114.bin",  0x0c0000, 0x40000, 0x6210c36c )  /* even */

	ROM_LOAD (  "tcu95.bin",  0x200000, 0x40000, 0x579caeba )  /* odd  */
	ROM_LOAD (  "tcu96.bin",  0x240000, 0x40000, 0xf43f1ffe )  /* odd  */
	ROM_LOAD (  "tcu97.bin",  0x280000, 0x40000, 0x1675e50d )  /* odd  */
	ROM_LOAD (  "tcu98.bin",  0x2c0000, 0x40000, 0xab06c885 )  /* odd  */

	ROM_LOAD ( "tcu106.bin",  0x400000, 0x40000, 0x146e3863 )  /* even */
 	ROM_LOAD ( "tcu107.bin",  0x440000, 0x40000, 0x95323320 )  /* even */
 	ROM_LOAD ( "tcu108.bin",  0x480000, 0x40000, 0xed152acc )  /* even */
 	ROM_LOAD ( "tcu109.bin",  0x4c0000, 0x40000, 0x80715252 )  /* even */

	ROM_REGION(0x50000) /* sound CPU */
	ROM_LOAD (  "tcu3.bin", 0x10000, 0x20000, 0x5bdb4665 )
	ROM_RELOAD (            0x30000, 0x20000 )

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "tcu12.bin", 0x00000, 0x40000, 0xd0000ac7 )
/*	ROM_LOAD ( "tcu12.bin", 0x40000, 0x40000, 0xd0000ac7 ) */

	ROM_REGION(0x80000) /* ADPCM samples */
	ROM_LOAD ( "tcu13.bin", 0x00000, 0x20000, 0xe48e6f0c )
	ROM_CONTINUE          ( 0x40000, 0x20000             )
/*	ROM_LOAD ( "tcu12.bin", 0x60000, 0x20000, 0xd0000ac7 ) */
/*	ROM_CONTINUE          ( 0x20000, 0x20000             ) */

	ROM_REGION(0x80000) /* ADPCM samples */
/*	ROM_LOAD ( "tcu13.bin", 0x40000, 0x20000, 0xe48e6f0c ) */
/*	ROM_CONTINUE          ( 0x00000, 0x20000             ) */
/*	ROM_LOAD ( "tcu12.bin", 0x60000, 0x20000, 0xd0000ac7 ) */
/*	ROM_CONTINUE          ( 0x20000, 0x20000             ) */

ROM_END

ROM_START( mk2_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "uj12.l31",  0x00000, 0x80000, 0xcf100a75 )  /* even */
	ROM_LOAD_EVEN( "ug12.l31",  0x00000, 0x80000, 0x582c7dfd )  /* odd  */

	ROM_REGION(0xc00000)      /* graphics */
	ROM_LOAD ( "ug14-vid",  0x000000, 0x100000, 0x01e73af6 )  /* even */
	ROM_LOAD ( "ug16-vid",  0x200000, 0x100000, 0x8ba6ae18 )  /* even */
	ROM_LOAD ( "ug17-vid",  0x100000, 0x100000, 0x937d8620 )  /* even */

	ROM_LOAD ( "uj14-vid",  0x300000, 0x100000, 0xd4985cbb )  /* odd  */
	ROM_LOAD ( "uj16-vid",  0x500000, 0x100000, 0x39d885b4 )  /* odd  */
	ROM_LOAD ( "uj17-vid",  0x400000, 0x100000, 0x218de160 )  /* odd  */

	ROM_LOAD ( "ug19-vid",  0x600000, 0x100000, 0xfec137be )  /* even */
	ROM_LOAD ( "ug20-vid",  0x800000, 0x100000, 0x809118c1 )  /* even */
	ROM_LOAD ( "ug22-vid",  0x700000, 0x100000, 0x154d53b1 )  /* even */

	ROM_LOAD ( "uj19-vid",  0x900000, 0x100000, 0x2d763156 )  /* odd  */
	ROM_LOAD ( "uj20-vid",  0xb00000, 0x100000, 0xb96824f0 )  /* odd  */
	ROM_LOAD ( "uj22-vid",  0xa00000, 0x100000, 0x8891d785 )  /* odd  */

	ROM_REGION_OPTIONAL(0x100000) /* sound */
	ROM_LOAD (   "su2.l1",  0x000000, 0x80000, 0x5f23d71d )
	ROM_LOAD (   "su3.l1",  0x080000, 0x80000, 0xd6d92bf9 )
	ROM_LOAD (   "su4.l1",  0x000000, 0x80000, 0xeebc8e0f )
	ROM_LOAD (   "su5.l1",  0x080000, 0x80000, 0x2b0b7961 )
	ROM_LOAD (   "su6.l1",  0x000000, 0x80000, 0xf694b27f )
	ROM_LOAD (   "su7.l1",  0x080000, 0x80000, 0x20387e0a )

ROM_END

ROM_START( mk2r32_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "uj12.l32",  0x00000, 0x80000, 0x43f773a6 )  /* even */
	ROM_LOAD_EVEN( "ug12.l32",  0x00000, 0x80000, 0xdcde9619 )  /* odd  */

	ROM_REGION(0xc00000)      /* graphics */
	ROM_LOAD ( "ug14-vid",  0x000000, 0x100000, 0x01e73af6 )  /* even */
	ROM_LOAD ( "ug16-vid",  0x200000, 0x100000, 0x8ba6ae18 )  /* even */
	ROM_LOAD ( "ug17-vid",  0x100000, 0x100000, 0x937d8620 )  /* even */

	ROM_LOAD ( "uj14-vid",  0x300000, 0x100000, 0xd4985cbb )  /* odd  */
	ROM_LOAD ( "uj16-vid",  0x500000, 0x100000, 0x39d885b4 )  /* odd  */
	ROM_LOAD ( "uj17-vid",  0x400000, 0x100000, 0x218de160 )  /* odd  */

	ROM_LOAD ( "ug19-vid",  0x600000, 0x100000, 0xfec137be )  /* even */
	ROM_LOAD ( "ug20-vid",  0x800000, 0x100000, 0x809118c1 )  /* even */
	ROM_LOAD ( "ug22-vid",  0x700000, 0x100000, 0x154d53b1 )  /* even */

	ROM_LOAD ( "uj19-vid",  0x900000, 0x100000, 0x2d763156 )  /* odd  */
	ROM_LOAD ( "uj20-vid",  0xb00000, 0x100000, 0xb96824f0 )  /* odd  */
	ROM_LOAD ( "uj22-vid",  0xa00000, 0x100000, 0x8891d785 )  /* odd  */

	ROM_REGION_OPTIONAL(0x100000) /* sound */
	ROM_LOAD (   "su2.l1",  0x000000, 0x80000, 0x5f23d71d )
	ROM_LOAD (   "su3.l1",  0x080000, 0x80000, 0xd6d92bf9 )
	ROM_LOAD (   "su4.l1",  0x000000, 0x80000, 0xeebc8e0f )
	ROM_LOAD (   "su5.l1",  0x080000, 0x80000, 0x2b0b7961 )
	ROM_LOAD (   "su6.l1",  0x000000, 0x80000, 0xf694b27f )
	ROM_LOAD (   "su7.l1",  0x080000, 0x80000, 0x20387e0a )

ROM_END

ROM_START( mk2r14_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "uj12.l14",  0x00000, 0x80000, 0x6d43bc6d )  /* even */
	ROM_LOAD_EVEN( "ug12.l14",  0x00000, 0x80000, 0x42b0da21 )  /* odd  */

	ROM_REGION(0xc00000)      /* graphics */
	ROM_LOAD ( "ug14-vid",  0x000000, 0x100000, 0x01e73af6 )  /* even */
	ROM_LOAD ( "ug16-vid",  0x200000, 0x100000, 0x8ba6ae18 )  /* even */
	ROM_LOAD ( "ug17-vid",  0x100000, 0x100000, 0x937d8620 )  /* even */

	ROM_LOAD ( "uj14-vid",  0x300000, 0x100000, 0xd4985cbb )  /* odd  */
	ROM_LOAD ( "uj16-vid",  0x500000, 0x100000, 0x39d885b4 )  /* odd  */
	ROM_LOAD ( "uj17-vid",  0x400000, 0x100000, 0x218de160 )  /* odd  */

	ROM_LOAD ( "ug19-vid",  0x600000, 0x100000, 0xfec137be )  /* even */
	ROM_LOAD ( "ug20-vid",  0x800000, 0x100000, 0x809118c1 )  /* even */
	ROM_LOAD ( "ug22-vid",  0x700000, 0x100000, 0x154d53b1 )  /* even */

	ROM_LOAD ( "uj19-vid",  0x900000, 0x100000, 0x2d763156 )  /* odd  */
	ROM_LOAD ( "uj20-vid",  0xb00000, 0x100000, 0xb96824f0 )  /* odd  */
	ROM_LOAD ( "uj22-vid",  0xa00000, 0x100000, 0x8891d785 )  /* odd  */

	ROM_REGION_OPTIONAL(0x100000) /* sound */
	ROM_LOAD (   "su2.l1",  0x000000, 0x80000, 0x5f23d71d )
	ROM_LOAD (   "su3.l1",  0x080000, 0x80000, 0xd6d92bf9 )
	ROM_LOAD (   "su4.l1",  0x000000, 0x80000, 0xeebc8e0f )
	ROM_LOAD (   "su5.l1",  0x080000, 0x80000, 0x2b0b7961 )
	ROM_LOAD (   "su6.l1",  0x000000, 0x80000, 0xf694b27f )
	ROM_LOAD (   "su7.l1",  0x080000, 0x80000, 0x20387e0a )

ROM_END

/*
    equivalences for the extension board version (same contents, split in half)

	ROM_LOAD ( "ug14.l1",   0x000000, 0x080000, 0x74f5aaf1 )
	ROM_LOAD ( "ug16.l11",  0x080000, 0x080000, 0x1cf58c4c )
	ROM_LOAD ( "u8.l1",     0x200000, 0x080000, 0x56e22ff5 )
	ROM_LOAD ( "u11.l1",    0x280000, 0x080000, 0x559ca4a3 )
	ROM_LOAD ( "ug17.l1",   0x100000, 0x080000, 0x4202d8bf )
	ROM_LOAD ( "ug18.l1",   0x180000, 0x080000, 0xa3deab6a )

	ROM_LOAD ( "uj14.l1",   0x300000, 0x080000, 0x869a3c55 )
	ROM_LOAD ( "uj16.l11",  0x380000, 0x080000, 0xc70cf053 )
	ROM_LOAD ( "u9.l1",     0x500000, 0x080000, 0x67da0769 )
	ROM_LOAD ( "u10.l1",    0x580000, 0x080000, 0x69000ac3 )
	ROM_LOAD ( "uj17.l1",   0x400000, 0x080000, 0xec3e1884 )
	ROM_LOAD ( "uj18.l1",   0x480000, 0x080000, 0xc9f5aef4 )

	ROM_LOAD ( "u6.l1",     0x600000, 0x080000, 0x8d4c496a )
	ROM_LOAD ( "u13.l11",   0x680000, 0x080000, 0x7fb20a45 )
	ROM_LOAD ( "ug19.l1",   0x800000, 0x080000, 0xd6c1f75e )
	ROM_LOAD ( "ug20.l1",   0x880000, 0x080000, 0x19a33cff )
	ROM_LOAD ( "ug22.l1",   0x700000, 0x080000, 0xdb6cfa45 )
	ROM_LOAD ( "ug23.l1",   0x780000, 0x080000, 0xbfd8b656 )

	ROM_LOAD ( "u7.l1",     0x900000, 0x080000, 0x3988aac8 )
	ROM_LOAD ( "u12.l11",   0x980000, 0x080000, 0x2ef12cc6 )
	ROM_LOAD ( "uj19.l1",   0xb00000, 0x080000, 0x4eed6f18 )
	ROM_LOAD ( "uj20.l1",   0xb80000, 0x080000, 0x337b1e20 )
	ROM_LOAD ( "uj22.l1",   0xa00000, 0x080000, 0xa6546b15 )
	ROM_LOAD ( "uj23.l1",   0xa80000, 0x080000, 0x45867c6f )
*/


ROM_START( nbajam_rom )
	ROM_REGION(0x100000)     /* 34010 code */
	ROM_LOAD_ODD ( "nbauj12.bin",  0x00000, 0x80000, 0xb93e271c )  /* even */
	ROM_LOAD_EVEN( "nbaug12.bin",  0x00000, 0x80000, 0x407d3390 )  /* odd  */

	ROM_REGION(0xc00000)      /* graphics */
	ROM_LOAD ( "nbaug14.bin",  0x000000, 0x80000, 0x04bb9f64 )  /* even */
	ROM_LOAD ( "nbaug16.bin",  0x080000, 0x80000, 0x8591c572 )  /* even */
	ROM_LOAD ( "nbaug17.bin",  0x100000, 0x80000, 0x6f921886 )  /* even */
	ROM_LOAD ( "nbaug18.bin",  0x180000, 0x80000, 0x5162d3d6 )  /* even */

	ROM_LOAD ( "nbauj14.bin",  0x300000, 0x80000, 0xb34b7af3 )  /* odd  */
	ROM_LOAD ( "nbauj16.bin",  0x380000, 0x80000, 0xd2e554f1 )  /* odd  */
	ROM_LOAD ( "nbauj17.bin",  0x400000, 0x80000, 0xb2e14981 )  /* odd  */
	ROM_LOAD ( "nbauj18.bin",  0x480000, 0x80000, 0xfdee0037 )  /* odd  */

	ROM_LOAD ( "nbaug19.bin",  0x600000, 0x80000, 0xa8f22fbb )  /* even */
	ROM_LOAD ( "nbaug20.bin",  0x680000, 0x80000, 0x44fd6221 )  /* even */
	ROM_LOAD ( "nbaug22.bin",  0x700000, 0x80000, 0xab05ed89 )  /* even */
	ROM_LOAD ( "nbaug23.bin",  0x780000, 0x80000, 0x7b934c7a )  /* even */

	ROM_LOAD ( "nbauj19.bin",  0x900000, 0x80000, 0x8130a8a2 )  /* odd  */
	ROM_LOAD ( "nbauj20.bin",  0x980000, 0x80000, 0xf9cebbb6 )  /* odd  */
	ROM_LOAD ( "nbauj22.bin",  0xa00000, 0x80000, 0x59a95878 )  /* odd  */
	ROM_LOAD ( "nbauj23.bin",  0xa80000, 0x80000, 0x427d2eee )  /* odd  */

	ROM_REGION(0x50000) /* sound CPU */
	ROM_LOAD (  "nbau3.bin",  0x010000, 0x20000, 0x3a3ea480 )
	ROM_RELOAD (              0x030000, 0x20000 )

	ROM_REGION(0x100000)
	ROM_LOAD ( "nbau12.bin",  0x000000, 0x80000, 0xb94847f1 )
	ROM_LOAD ( "nbau13.bin",  0x080000, 0x80000, 0xb6fe24bd )

ROM_END


#define BASE_CREDITS  "Alex Pasadyn\nZsolt Vasvari\nKurt Mahan (hardware info)"

struct GameDriver narc_driver =
{
	__FILE__,
	0,
	"narc",
	"Narc (rev 7.00)",
	"1988",
	"Williams",
	BASE_CREDITS,
	0,
	&narc_machine_driver,
	narc_driver_init,

	narc_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	narc_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver trog_driver =
{
	__FILE__,
	0,
	"trog",
	"Trog (rev LA4 03/11/91)",
	"1990",
	"Midway",
	BASE_CREDITS,
	0,
	&trog_machine_driver,
	trog_driver_init,

	trog_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	trog_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver trog3_driver =
{
	__FILE__,
	&trog_driver,
	"trog3",
	"Trog (rev LA3 02/14/91)",
	"1990",
	"Midway",
	BASE_CREDITS,
	0,
	&trog_machine_driver,
	trog3_driver_init,

	trog3_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	trog_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver trogp_driver =
{
	__FILE__,
	&trog_driver,
	"trogp",
	"Trog (prototype, rev 4.00 07/27/90)",
	"1990",
	"Midway",
	BASE_CREDITS,
	0,
	&trog_machine_driver,
	trogp_driver_init,

	trogp_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	trog_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver smashtv_driver =
{
	__FILE__,
	0,
	"smashtv",
	"Smash T.V. (rev 8.00)",
	"1990",
	"Williams",
	BASE_CREDITS,
	0,
	&smashtv_machine_driver,
	smashtv_driver_init,

	smashtv_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	smashtv_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver smashtv6_driver =
{
	__FILE__,
	&smashtv_driver,
	"smashtv6",
	"Smash T.V. (rev 6.00)",
	"1990",
	"Williams",
	BASE_CREDITS,
	0,
	&smashtv_machine_driver,
	smashtv_driver_init,

	smashtv6_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	smashtv_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver smashtv5_driver =
{
	__FILE__,
	&smashtv_driver,
	"smashtv5",
	"Smash T.V. (rev 5.00)",
	"1990",
	"Williams",
	BASE_CREDITS,
	0,
	&smashtv_machine_driver,
	smashtv_driver_init,

	smashtv5_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	smashtv_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver smashtv4_driver =
{
	__FILE__,
	&smashtv_driver,
	"smashtv4",
	"Smash T.V. (rev 4.00)",
	"1990",
	"Williams",
	BASE_CREDITS,
	0,
	&smashtv_machine_driver,
	smashtv4_driver_init,

	smashtv4_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	smashtv_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver hiimpact_driver =
{
	__FILE__,
	0,
	"hiimpact",
	"High Impact Football (rev LA3 12/27/90)",
	"1990",
	"Williams",
	BASE_CREDITS,
	0,
	&smashtv_machine_driver,
	hiimpact_driver_init,

	hiimpact_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	trog_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver shimpact_driver =
{
	__FILE__,
	0,
	"shimpact",
	"Super High Impact (rev LA1 09/30/91)",
	"1991",
	"Midway",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&smashtv_machine_driver,
	shimpact_driver_init,

	shimpact_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	trog_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver strkforc_driver =
{
	__FILE__,
	0,
	"strkforc",
	"Strike Force (rev 1 02/25/91)",
	"1991",
	"Midway",
	BASE_CREDITS,
	0,
	&trog_machine_driver,
	strkforc_driver_init,

	strkforc_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	strkforc_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver mk_driver =
{
	__FILE__,
	0,
	"mk",
	"Mortal Kombat (rev 3.0 08/31/92)",
	"1992",
	"Midway",
	BASE_CREDITS,
	0,
	&mk_machine_driver,
	mk_driver_init,

	mk_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	mk_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};
struct GameDriver mkla1_driver =
{
	__FILE__,
	&mk_driver,
	"mkla1",
	"Mortal Kombat (rev 1.0 08/08/92)",
	"1992",
	"Midway",
	BASE_CREDITS,
	0,
	&mk_machine_driver,
	mkla1_driver_init,

	mkla1_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	mk_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};
struct GameDriver mkla2_driver =
{
	__FILE__,
	&mk_driver,
	"mkla2",
	"Mortal Kombat (rev 2.0 08/18/92)",
	"1992",
	"Midway",
	BASE_CREDITS,
	0,
	&mk_machine_driver,
	mkla2_driver_init,

	mkla2_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	mk_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver term2_driver =
{
	__FILE__,
	0,
	"term2",
	"Terminator 2 - Judgment Day (rev LA3 03/27/92)",
	"1991",
	"Midway",
	BASE_CREDITS,
	0,
	&term2_machine_driver,
	term2_driver_init,

	term2_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	term2_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver totcarn_driver =
{
	__FILE__,
	0,
	"totcarn",
	"Total Carnage (rev LA1 03/10/92)",
	"1992",
	"Midway",
	BASE_CREDITS,
	0,
	&mk_machine_driver,
	totcarn_driver_init,

	totcarn_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	totcarn_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver totcarnp_driver =
{
	__FILE__,
	&totcarn_driver,
	"totcarnp",
	"Total Carnage (prototype, rev 1.0 01/25/92)",
	"1992",
	"Midway",
	BASE_CREDITS,
	0,
	&mk_machine_driver,
	totcarnp_driver_init,

	totcarnp_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	totcarn_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver mk2_driver =
{
	__FILE__,
	0,
	"mk2",
	"Mortal Kombat II (rev L3.1)",
	"1993",
	"Midway",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&mk2_machine_driver,
	mk2_driver_init,

	mk2_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	mk2_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver mk2r32_driver =
{
	__FILE__,
	&mk2_driver,
	"mk2r32",
	"Mortal Kombat II (rev L3.2 (European))",
	"1993",
	"Midway",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&mk2_machine_driver,
	mk2_driver_init,

	mk2r32_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	mk2_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver mk2r14_driver =
{
	__FILE__,
	&mk2_driver,
	"mk2r14",
	"Mortal Kombat II (rev L1.4)",
	"1993",
	"Midway",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&mk2_machine_driver,
	mk2r14_driver_init,

	mk2r14_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	mk2_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver nbajam_driver =
{
	__FILE__,
	0,
	"nbajam",
	"NBA Jam",
	"1993",
	"Midway",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&nbajam_machine_driver,
	nbajam_driver_init,

	nbajam_rom,
	wms_decode, 0,
	0,
	0,	/* sound_prom */

	nbajam_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};
