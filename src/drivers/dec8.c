/***************************************************************************

Data East 8 bit system:

	Cobra Command (c) 1988 							6809 + 6502, YM2203, YM3812

  Ghostbusters    										6809, Intel 8751 + 6502, YM2203, YM3812
  Super Real Darwin 									6809, Intel 8751 + 6502, YM2203, YM3812
  Gondomania      										6809, Intel 8751 + 6502(?)

  Oscar           									  6809, 6809, (Intel?) + 6502, YM2203, YM3812
  Last Mission 												6809, 6809, Intel 8751 + 6502, YM2203, YM3812
  Shackled                            6809, 6809, Intel 8751 + 6502, YM2203, YM3812

Games that might run on this system or similar:

	Mazehunter
  Express Raider  - Probably too early..
  Liberation
  Battle Ranger   - ???  Z80?  Doesn't look like others
  Kid Niki - z80

Emulation by Bryan McPhail, mish@tendril.force9.net

Nb: Only Cobra Command works at the moment!
Dip switches on CC not confirmed but seem reasonable.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/m6809.h"
#include "M6502/M6502.h"

int dec8_video_r(int offset);
void dec8_video_w(int offset, int data);
void dec8_pf1_w(int offset, int data);
void dec8_pf2_w(int offset, int data);
void dec8_scroll1_w(int offset, int data);
void dec8_scroll2_w(int offset, int data);
void dec8_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int dec8_vh_start(void);
void dec8_vh_stop(void);

int oscar_share_r(int offset);
void oscar_share_w(int offset,int data);

/******************************************************************************/

static int prota, protb;

int gondo_prot1_r(int offset)
{
	if (errorlog) fprintf(errorlog,"PC %06x - Read from 8751 low\n",cpu_getpc());

  /* Ghostbusters protection */
	if (protb==0xaa && prota==0) return 6;


  return 0x74;
}

int gondo_prot2_r(int offset)
{
	if (errorlog) fprintf(errorlog,"PC %06x - Read from 8751 low\n",cpu_getpc());

  return rand()%0x7;
}

int prot1_r(int offset)
{
	if (errorlog) fprintf(errorlog,"PC %06x - Read from 8751 low\n",cpu_getpc());

  /* Ghostbusters protection */
	if (protb==0xaa && prota==0) return 6;
  if (protb==0x1a && prota==2) return 6;

  /* Darwin */
  if (protb==0x00 && prota==0x00) return 0x00;
  /* */
  if (protb==0x00 && prota==0x40) return 0x40;


  return 0;

}

int prot2_r(int offset)
{
  if (errorlog) fprintf(errorlog,"PC %06x - Read from 8751 high\n",cpu_getpc());

  /* Ghostbusters protection */
  if (protb==0xaa && prota==0) return 0x55;
  if (protb==0x1a && prota==2) return 0xe5;

  /* Darwin */
  if (protb==0x00 && prota==0x00) return 0x00;
  if (protb==0x63 && prota==0x30) return 0x9c;
  if (protb==0x00 && prota==0x40) return 0x00;
  if (protb==0x00 && prota==0x50) return rand()%0xff; /* In NMI */

	return 0;
}

void prot1_w(int offset, int data)
{
	prota=data;
  if (errorlog) fprintf(errorlog,"PC %06x - Write %02x to 8751 low\n",cpu_getpc(),data);
}

void prot2_w(int offset, int data)
{
	protb=data;
  if (errorlog) fprintf(errorlog,"PC %06x - Write %02x to 8751 high\n",cpu_getpc(),data);
}

void dec8_bank_w(int offset, int data)
{
 	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (data & 0x0f) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
if (errorlog) fprintf(errorlog,"PC %06x - Bank switch %02x (%02x)\n",cpu_getpc(),data&0xf,data);
}


void cobra_sound_w(int offset, int data)
{
 	soundlatch_w(0,data);
  cpu_cause_interrupt (1, INT_NMI);


}

/******************************************************************************/

static struct MemoryReadAddress cobra_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1fff, dec8_video_r },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x2800, 0x2fff, MRA_RAM },
	{ 0x3000, 0x31ff, MRA_RAM },
	{ 0x3200, 0x37ff, MRA_RAM }, /* Unknown, probably unused in this game */
	{ 0x3800, 0x3800, input_port_0_r }, /* Player 1 */
	{ 0x3801, 0x3801, input_port_1_r }, /* Player 2 */
	{ 0x3802, 0x3802, input_port_3_r }, /* Dip 1 */
	{ 0x3803, 0x3803, input_port_4_r }, /* Dip 2 */
	{ 0x3a00, 0x3a00, input_port_2_r }, /* VBL & coins */
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress cobra_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x1fff, dec8_video_w },
	{ 0x2000, 0x27ff, MWA_RAM, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, MWA_RAM, &spriteram },
	{ 0x3000, 0x31ff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
	{ 0x3200, 0x37ff, MWA_RAM }, /* Unknown, probably unused in this game */
	{ 0x3800, 0x3806, dec8_pf1_w },
	{ 0x3810, 0x3813, dec8_scroll1_w },
	{ 0x3a00, 0x3a06, dec8_pf2_w },
	{ 0x3a10, 0x3a13, dec8_scroll2_w },
	{ 0x3c00, 0x3c00, dec8_bank_w },
	{ 0x3c02, 0x3c02, MWA_NOP }, /* Lots of 1s written here, don't think it's a watchdog */
	{ 0x3e00, 0x3e00, cobra_sound_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


#if 0
static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
  { 0x1000, 0x13ff, MRA_RAM },
  { 0x1400, 0x17ff, MRA_RAM },

  //  { 0x1000, 0x1fff, dec8_video_r },
//  { 0x2000, 0x27ff, MRA_RAM },
//  { 0x2800, 0x2fff, MRA_RAM },
//  { 0x3000, 0x31ff, MRA_RAM },
//  { 0x3200, 0x37ff, MRA_RAM }, /* Unknown */

  { 0x2000, 0x2000, prot1_r }, // gb
  { 0x2001, 0x2001, prot2_r }, // gb


  { 0x3840, 0x3840, prot1_r }, // gb
  { 0x3860, 0x3860, prot2_r }, // gb

	{ 0x3800, 0x3800, input_port_0_r }, /* Player 1 */
	{ 0x3801, 0x3801, input_port_1_r }, /* Player 2 */


  { 0x3802, 0x3802, input_port_2_r }, /* VBL>>> */


  { 0x3803, 0x3803, input_port_4_r }, /* Dip 2 */

//  { 0x3a00, 0x3a00, input_port_2_r }, /* VBL & coins */

	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
  { 0x0000, 0x0fff, MWA_RAM },
 	{ 0x1000, 0x13ff, MWA_RAM }, /* Main RAM */
  { 0x1400, 0x17ff, MWA_RAM },

  { 0x1800, 0x1800, prot1_w },
  { 0x1801, 0x1801, prot2_w },


// 	{ 0x1000, 0x1fff, dec8_video_w },
//  { 0x2000, 0x27ff, MWA_RAM, &videoram, &videoram_size },
//  { 0x2800, 0x2fff, MWA_RAM, &spriteram },
//	{ 0x3000, 0x31ff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
//  { 0x3200, 0x37ff, MWA_RAM }, /* Unknown */

	{ 0x2840, 0x288f, MWA_RAM },       /* g b */
	{ 0x3040, 0x308f, MWA_RAM },  /* r*/


  { 0x3860, 0x3860, prot1_w }, /* Not used by CC */
  { 0x3861, 0x3861, prot2_w }, /* Not used by CC */


	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};
#endif

static struct MemoryReadAddress ghostb_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
  { 0x1000, 0x13ff, MRA_RAM },
  { 0x1400, 0x37ff, MRA_RAM },

  { 0x3840, 0x3840, prot1_r }, // gb
  { 0x3860, 0x3860, prot2_r }, // gb

	{ 0x3820, 0x3820, input_port_0_r }, /* Player 1 */
//	{ 0x3801, 0x3801, input_port_1_r }, /* Player 2 */

//  { 0x3802, 0x3802, input_port_2_r }, /* VBL>>> */

  { 0x3803, 0x3803, input_port_4_r }, /* Dip 2 */

//  { 0x3a00, 0x3a00, input_port_2_r }, /* VBL & coins */

	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ghostb_writemem[] =
{
  { 0x0000, 0x0fff, MWA_RAM },
 	{ 0x1000, 0x13ff, MWA_RAM }, /* Main RAM */
  { 0x1400, 0x37ff, MWA_RAM },

//	{ 0x2840, 0x288f, MWA_RAM },       /* g b */
//	{ 0x3040, 0x308f, MWA_RAM },  /* r*/

//CPU #0 PC 80bb: warning - write 82 to unmapped memory address 3820
//CPU #0 PC 80c0: warning - write 03 to unmapped memory address 3822


//CPU #0 PC 80c5: warning - write 00 to unmapped memory address 3824


//CPU #0 PC 80ca: warning - write 01 to unmapped memory address 3826




  { 0x3860, 0x3860, prot1_w },
  { 0x3861, 0x3861, prot2_w },

	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress gondo_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
  { 0x1000, 0x13ff, MRA_RAM },
  { 0x1400, 0x1fff, MRA_RAM },

  //  { 0x1000, 0x1fff, dec8_video_r },
  { 0x2000, 0x27ff, MRA_RAM },
  { 0x2800, 0x2fff, MRA_RAM },
//  { 0x3000, 0x31ff, MRA_RAM },
  { 0x3000, 0x37ff, MRA_RAM }, /* Unknown */

  { 0x3838, 0x3838, gondo_prot2_r },
  { 0x3839, 0x3839, gondo_prot1_r }, // gb

	{ 0x3800, 0x3800, input_port_0_r }, /* Player 1 */
	{ 0x3801, 0x3801, input_port_1_r }, /* Player 2 */

  { 0x380e, 0x380e, input_port_2_r }, /* VBL>>> */

  { 0x3803, 0x3803, input_port_4_r }, /* Dip 2 */

//  { 0x3a00, 0x3a00, input_port_2_r }, /* VBL & coins */

	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
	Gondomania Protection notes:

  Each interrupt value read from 0x3838, used as key to a lookup table
  of functions:




  3830, nmi mask??


*/

static struct MemoryWriteAddress gondo_writemem[] =
{
  { 0x0000, 0x0fff, MWA_RAM },
 	{ 0x1000, 0x13ff, MWA_RAM }, /* Main RAM */
  { 0x1400, 0x1fff, MWA_RAM },


// 	{ 0x1000, 0x1fff, dec8_video_w },
  { 0x2000, 0x27ff, MWA_RAM, &videoram, &videoram_size },
  { 0x2800, 0x2fff, MWA_RAM, &spriteram },  /* palette */
//	{ 0x3000, 0x31ff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
  { 0x3000, 0x37ff, MWA_RAM }, /* Unknown */

//	{ 0x2840, 0x288f, MWA_RAM },       /* g b */
//	{ 0x3040, 0x308f, MWA_RAM },  /* r*/


//  { 0x3830, 0x3830, dec8_bank_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress oscar_readmem[] =
{
	{ 0x0000, 0x07ff, oscar_share_r },
  { 0x0800, 0x0fff, MRA_RAM },
//  { 0x1000, 0x13ff, MRA_RAM },
//  { 0x1400, 0x1fff, MRA_RAM },

  { 0x1000, 0x1fff, dec8_video_r },
  { 0x2000, 0x27ff, MRA_RAM },
  { 0x2800, 0x2fff, MRA_RAM },
  { 0x3000, 0x39ff, MRA_RAM }, /* Unknown */
  { 0x3a00, 0x3bff, MRA_RAM },

//  { 0x3c00, 0x3c00, input_port_0_r },
//  { 0x3c01, 0x3c01, input_port_1_r },
  { 0x3c02, 0x3c02, input_port_2_r }, /* VBL & coins */
  { 0x3c03, 0x3c03, input_port_3_r }, /* Dip 1 - CONFIRMED */
 // { 0x3c04, 0x3c04, input_port_4_r },

	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


int sub_int=0;

void oscar_sub_int(int offset, int data)
{

	sub_int=1;

 	//cpu_cause_interrupt (1, INT_IRQ);
}

static struct MemoryWriteAddress oscar_writemem[] =
{
	{ 0x0000, 0x07ff, oscar_share_w },
	{ 0x0800, 0x0fff, MWA_RAM },
	{ 0x1000, 0x1fff, dec8_video_w },
	{ 0x2000, 0x27ff, MWA_RAM, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, MWA_RAM, &spriteram },  /* palette */
	{ 0x3000, 0x39ff, MWA_RAM }, /* Unknown */
	{ 0x3a00, 0x3bff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },

 //	{ 0x3800, 0x3806, dec8_pf1_w }, /* pf1 control?  MSB of 3800 is reverse screen */
 // { 0x3810, 0x3813, dec8_scroll1_w },
 // { 0x3a00, 0x3a06, dec8_pf2_w }, /* pf2 control? */
 // { 0x3a10, 0x3a13, dec8_scroll2_w },


      // bpx 8059   821d   8220   0x800 bytes shared..

 	// 3c01 - playfield 1

  { 0x3c80, 0x3c80, oscar_sub_int },
  { 0x3d00, 0x3d00, dec8_bank_w },
// 	{ 0x3e00, 0x3e00, cobra_sound_w },

	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



// bpx  faa7
// bpx f4f3 cpu 1

#if 0
static struct MemoryReadAddress oscar_sub_readmem[] =
{
	{ 0x0000, 0x07ff, oscar_share_r },
  { 0x0800, 0x0fff, MRA_RAM },
  { 0x1000, 0x1fff, dec8_video_r },

//	{ 0x3c02, 0x3c02, input_port_2_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress oscar_sub_writemem[] =
{
  { 0x0000, 0x07ff, oscar_share_w },
  { 0x0800, 0x0fff, MWA_RAM },

  { 0x1000, 0x1fff, dec8_video_w },

	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};
#endif


static struct MemoryReadAddress lastmiss_readmem[] =
{
	{ 0x0000, 0x07ff, oscar_share_r },
  { 0x0800, 0x17ff, MRA_RAM },

//  180d bank switch  1802 vbl
  { 0x1802, 0x1802, input_port_2_r },


//  { 0x1000, 0x1fff, dec8_video_r },

  { 0x2000, 0x27ff, MRA_RAM },
  { 0x2800, 0x2fff, MRA_RAM },
  { 0x3000, 0x3fff, dec8_video_r },
//  { 0x3a00, 0x3bff, MRA_RAM },

//  { 0x3c00, 0x3c00, input_port_0_r },
//  { 0x3c01, 0x3c01, input_port_1_r },
   /* VBL & coins */
  { 0x3c03, 0x3c03, input_port_3_r }, /* Dip 1 */
 // { 0x3c04, 0x3c04, input_port_4_r },

	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress lastmiss_writemem[] =
{
  { 0x0000, 0x07ff, oscar_share_w },
  { 0x0800, 0x17ff, MWA_RAM },
// 	{ 0x1000, 0x1fff,  },

  { 0x180d, 0x180d, dec8_bank_w },

  { 0x2000, 0x27ff, MWA_RAM, &videoram, &videoram_size },
  { 0x2800, 0x2fff, MWA_RAM, &spriteram },  /* palette */
  { 0x3000, 0x3fff, dec8_video_w },

//	{ 0x3a00, 0x3bff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
 //	{ 0x3800, 0x3806, dec8_pf1_w }, /* pf1 control?  MSB of 3800 is reverse screen */
 // { 0x3810, 0x3813, dec8_scroll1_w },
 // { 0x3a00, 0x3a06, dec8_pf2_w }, /* pf2 control? */
 // { 0x3a10, 0x3a13, dec8_scroll2_w },


      // bpx 8059   821d   8220   0x800 bytes shared..

 	// 3c01 - playfield 1
//  { 0x3c80, 0x3c80, oscar_sub_int },
//
// 	{ 0x3e00, 0x3e00, cobra_sound_w },

	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

/******************************************************************************/

static struct MemoryReadAddress cobra_s_readmem[] =
{


	{ 0x0000, 0x05ff, MRA_RAM},


	{ 0x6000, 0x6000, soundlatch_r },


	{ 0x8000, 0xffff, MRA_ROM },


	{ -1 }  /* end of table */


};





static struct MemoryWriteAddress cobra_s_writemem[] =


{


 	{ 0x0000, 0x05ff, MWA_RAM},


	{ 0x2000, 0x2000, YM2203_control_port_0_w },


  { 0x2001, 0x2001, YM2203_write_port_0_w },


  { 0x4000, 0x4000, YM3812_control_port_0_w },


	{ 0x4001, 0x4001, YM3812_write_port_0_w },


 	{ 0x8000, 0xffff, MWA_ROM },


	{ -1 }  /* end of table */


};

/******************************************************************************/

INPUT_PORTS_START( input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )


	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )


	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )


	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )


	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )


	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )


	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

 	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )


	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )


	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )


	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )


	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )


	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )


	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )



	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )


	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )


	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )


	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )


	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )


	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )


	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )


	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )


	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )


	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )


	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )


	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )


  PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Test mode on other games */


	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )


	PORT_DIPSETTING(    0x00, "Off" )


	PORT_DIPSETTING(    0x20, "On" )


	PORT_DIPNAME( 0x40, 0x40, "Screen Rotation", IP_KEY_NONE )


	PORT_DIPSETTING(    0x40, "Normal" )


	PORT_DIPSETTING(    0x00, "Reverse" )


	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )


	PORT_DIPSETTING(    0x80, "Cocktail" )


	PORT_DIPSETTING(    0x00, "Upright" )





	PORT_START	/* Dip switch bank 2 */


	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )


	PORT_DIPSETTING(    0x01, "5" )


	PORT_DIPSETTING(    0x03, "3" )


	PORT_DIPSETTING(    0x02, "4" )


  PORT_DIPSETTING(    0x00, "Infinite" )


	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )


	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )


	PORT_DIPSETTING(    0x04, "Easy" )


	PORT_DIPSETTING(    0x0c, "Normal" )


	PORT_DIPSETTING(    0x08, "Hard" )


	PORT_DIPSETTING(    0x00, "Hardest" )


	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )


	PORT_DIPSETTING(    0x00, "No" )


	PORT_DIPSETTING(    0x10, "Yes" )


  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )


  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )


  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )


INPUT_PORTS_END

INPUT_PORTS_START( darwin_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )


	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )


	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )


	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )


	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )


	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )


	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )



  /* VBL

  reads, 2 shifts <<


  */


	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )


	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_VBLANK )


	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_VBLANK )


	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_VBLANK ) /* real one? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_BITX(0,        0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "99", IP_KEY_NONE, IP_JOY_NONE, 0)
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x10, "On" )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE, "Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin C", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,


	1024,


	2,


	{ 0x4000*8,0x0000*8 },


	{ 0, 1, 2, 3, 4, 5, 6, 7 },


	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },


	8*8	/* every sprite takes 8 consecutive bytes */


};

static struct GfxLayout gondo_charlayout =
{
	8,8,


	1024,


	3,


	{ 0x6000*8,0x4000*8,0x2000*8 },


	{ 0, 1, 2, 3, 4, 5, 6, 7 },


	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },


	8*8	/* every sprite takes 8 consecutive bytes */


};

static struct GfxLayout oscar_charlayout =
{
	8,8,


	1024,


	3,


	{ 0x3000*8,0x2000*8,0x1000*8 },


	{ 0, 1, 2, 3, 4, 5, 6, 7 },


	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },


	8*8	/* every sprite takes 8 consecutive bytes */


};

/* 16x16 tiles, 4 Planes, each plane is 0x10000 bytes */
static struct GfxLayout tiles =


{


	16,16,


	2048,


	4,


 	{ 0x30000*8,0x20000*8,0x10000*8,0x00000*8 },


	{ 16*8, 1+(16*8), 2+(16*8), 3+(16*8), 4+(16*8), 5+(16*8), 6+(16*8), 7+(16*8),


  	0,1,2,3,4,5,6,7 },


  { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},


	16*16


};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,     0, 2*64  },
	{ 1, 0x08000, &tiles,  0, 2*64 },
	{ 1, 0x48000, &tiles,  0, 2*64 },
	{ 1, 0x88000, &tiles,  0, 2*64 },
	{ -1 } /* end of array */
};

#if 0
static struct GfxLayout gcharlayout =
{


	8,8,	/* 8*8 chars */


	4096,


	1,		/* 4 bits per pixel  */


	{ 0x00000*8, 0x10000*8, 0x8000*8, 0x18000*8 },


	{ 0, 1, 2, 3, 4, 5, 6, 7 },


	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },


	8*8	/* every char takes 8 consecutive bytes */


};
#endif



static struct GfxDecodeInfo gondo_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &gondo_charlayout,     0, 2*64  },
	{ 1, 0x08000, &tiles,  0, 2*64 },
	{ 1, 0x48000, &tiles,  0, 2*64 },
	{ 1, 0x88000, &tiles,  0, 2*64 },
  { 1, 0xc8000, &tiles,  0, 2*64 },
 	{ -1 } /* end of array */
};

static struct GfxDecodeInfo oscar_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &oscar_charlayout,     0, 2*64  },
	{ 1, 0x08000, &tiles,  0, 2*64 },
	{ 1, 0x48000, &tiles,  0, 2*64 },
	{ 1, 0x88000, &tiles,  0, 2*64 },
  { 1, 0xc8000, &tiles,  0, 2*64 },
 	{ -1 } /* end of array */
};

static struct GfxDecodeInfo lastmiss_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &gondo_charlayout,     0, 2*64  },
	{ 1, 0x08000, &tiles,  0, 2*64 },
	{ 1, 0x48000, &tiles,  0, 2*64 },
	{ 1, 0x88000, &tiles,  0, 2*64 },
  { 1, 0xc8000, &tiles,  0, 2*64 },
 	{ -1 } /* end of array */
};
/******************************************************************************/

static struct YM2203interface ym2203_interface =
{
	1,
	1500000,	/* Unknown */
	{ YM2203_VOL(140,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 255 }		/* (not supported) */
};

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 255 }		/* (not supported) */
};

/******************************************************************************/

static struct MachineDriver cobra_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M6809,
			1250000,
			0,
			cobra_readmem,cobra_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			cobra_s_readmem,cobra_s_writemem,0,0,
      interrupt,8     /* Set by hand. */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
  //64*8, 64*8, { 0*8, 64*8-1, 1*8, 64*8-1 },

	gfxdecodeinfo,
	256,2*64*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	dec8_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};

int ghost_interrupt(void)
{


	static int a=0;





//  return 0;








  if (a) {a=0; return M6809_INT_NMI;}


  a=1; return M6809_INT_IRQ;


}

static struct MachineDriver gondo_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M6809,
			1250000,
			0,
			gondo_readmem,gondo_writemem,0,0,
			interrupt,1   /* CHECK */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			cobra_s_readmem,cobra_s_writemem,0,0,
      interrupt,8     /* Set by hand. */
		}
	},
	60, 2000, //DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
  //64*8, 64*8, { 0*8, 64*8-1, 1*8, 64*8-1 },

	gondo_gfxdecodeinfo,
	256,2*64*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	dec8_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};

int oscar_sub_interrupt(void)
{
	return sub_int;
}


static struct MachineDriver oscar_machine_driver =
{
	/* basic machine hardware */
	{
 /*	  {
			CPU_M6809,
			1250000,
			3,
			oscar_sub_readmem,oscar_sub_writemem,0,0,
//						oscar_readmem,oscar_writemem,0,0,
			oscar_sub_interrupt,1
		},   */

 	 	{
			CPU_M6809,
			1250000,
			0,
			oscar_readmem,oscar_writemem,0,0,
			interrupt,1
		},

		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			cobra_s_readmem,cobra_s_writemem,0,0,
      interrupt,8     /* Set by hand. */
		}
	},
	60, 2000, //DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
  //64*8, 64*8, { 0*8, 64*8-1, 1*8, 64*8-1 },

	oscar_gfxdecodeinfo,
	256,2*64*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	dec8_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};

static struct MachineDriver lastmiss_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M6809,
			1250000,
			0,
			lastmiss_readmem,lastmiss_writemem,0,0,
			interrupt,1
		},
 /*   {
			CPU_M6809,
			1250000,
			3,
			oscar_sub_readmem,oscar_sub_writemem,0,0,
     //	oscar_readmem,oscar_writemem,0,0,
			interrupt,1
		},  */
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			cobra_s_readmem,cobra_s_writemem,0,0,
      interrupt,8     /* Set by hand. */
		}
	},
	60, 2000, //DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
  //64*8, 64*8, { 0*8, 64*8-1, 1*8, 64*8-1 },

	lastmiss_gfxdecodeinfo,
	256,2*64*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	dec8_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};


static struct MachineDriver ghostb_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,
			0,
			ghostb_readmem,ghostb_writemem,0,0,
			interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			cobra_s_readmem,cobra_s_writemem,0,0,
      interrupt,8     /* Set by hand. */
		}
	},
	60, 2000, //DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 1*8, 32*8-1, 0*8, 31*8-1 },

	gfxdecodeinfo,
	256,2*64*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	dec8_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};

/******************************************************************************/

ROM_START( ghostb_rom )
	ROM_REGION(0x50000)

  /* NOTE!!! 2nd 32k of dz-01 appears to be same as 1st, so only load 1st */
 	ROM_LOAD( "dz-01.rom", 0x08000, 0x08000, 0x463d0ead )
 	ROM_LOAD( "dz-02.rom", 0x10000, 0x10000, 0xc76c007a )
  ROM_LOAD( "dz-03.rom", 0x20000, 0x10000, 0xc76c007a )
  ROM_LOAD( "dz-04.rom", 0x30000, 0x10000, 0xc76c007a )
  ROM_LOAD( "dz-05.rom", 0x40000, 0x10000, 0xc76c007a )

	ROM_REGION(0xc8000)	/* temporary space for graphics */
	ROM_LOAD( "dz-00.rom", 0x00000, 0x8000, 0x1b793a0b )	/* characters */

	/* sprites bank 1 */
  ROM_LOAD( "dz-15.rom", 0x08000, 0x10000, 0xe7ca5c28 )
	ROM_LOAD( "dz-12.rom", 0x18000, 0x10000, 0x50691d89 )
	ROM_LOAD( "dz-17.rom", 0x28000, 0x10000, 0x92f174b5 )
  ROM_LOAD( "dz-11.rom", 0x38000, 0x10000, 0xe7ca5c28 )

	/* sprites bank 2 */
	ROM_LOAD( "dz-14.rom", 0x48000, 0x10000, 0xe7ca5c28 )
	ROM_LOAD( "dz-16.rom", 0x58000, 0x10000, 0x50691d89 )
	ROM_LOAD( "dz-18.rom", 0x68000, 0x10000, 0x92f174b5 )
  ROM_LOAD( "dz-13.rom", 0x78000, 0x10000, 0x50691d89 )

  /* tiles 1 */
  ROM_LOAD( "dz-07.rom", 0x88000, 0x10000, 0xe7ca5c28 )
 	ROM_LOAD( "dz-08.rom", 0x98000, 0x10000, 0x50691d89 )
 	ROM_LOAD( "dz-09.rom", 0xa8000, 0x10000, 0x92f174b5 )
  ROM_LOAD( "dz-10.rom", 0xb8000, 0x10000, 0x92f174b5 )

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "dz-06.rom", 0x8000, 0x8000, 0x364efd26 )
ROM_END


ROM_START( srdarwin_rom )
	ROM_REGION(0x44000)
 	ROM_LOAD( "dy_01.rom", 0x10000, 0x08000, 0x463d0ead )
  ROM_CONTINUE(	0x8000, 0x8000 )

 	ROM_LOAD( "dy_02.rom", 0x18000, 0x10000, 0xc76c007a )
//  ROM_LOAD( "dy_03.rom", 0x28000, 0x10000, 0xc76c007a )
//  ROM_LOAD( "dy_01.rom", 0x38000, 0x08000, 0xc76c007a )
//  ROM_LOAD( "dy_05.rom", 0x40000, 0x04000, 0xc76c007a )

  /* 4 6 */

	ROM_REGION(0xc8000)	/* temporary space for graphics */
	ROM_LOAD( "dy_00.rom", 0x00000, 0x8000, 0x1b793a0b )	/* characters */

	/* sprites bank 1 */
  ROM_LOAD( "dy_06.rom", 0x08000, 0x8000, 0xe7ca5c28 )
	ROM_LOAD( "dy_07.rom", 0x18000, 0x8000, 0x50691d89 )
	ROM_LOAD( "dy_08.rom", 0x28000, 0x8000, 0x92f174b5 )
//  ROM_LOAD( "dz-11.rom", 0x38000, 0x10000, 0xe7ca5c28 )

	/* sprites bank 2 */
	ROM_LOAD( "dy_09.rom", 0x48000, 0x8000, 0xe7ca5c28 )
	ROM_LOAD( "dy_10.rom", 0x58000, 0x8000, 0x50691d89 )
	ROM_LOAD( "dy_11.rom", 0x68000, 0x8000, 0x92f174b5 )
//  ROM_LOAD( "dz-13.rom", 0x78000, 0x10000, 0x50691d89 )


	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "dy_04.rom", 0x8000, 0x8000, 0x364efd26 )
ROM_END


ROM_START( cobracom_rom )
	ROM_REGION(0x30000)
 	ROM_LOAD( "eh-11.rom", 0x08000, 0x08000, 0x4443d8d7 )
 	ROM_LOAD( "eh-12.rom", 0x10000, 0x10000, 0x600d4151 )
 	ROM_LOAD( "eh-13.rom", 0x20000, 0x10000, 0xb10ad1ce )

	ROM_REGION(0xc0000)	/* temporary space for graphics */
	ROM_LOAD( "eh-14.rom", 0x00000, 0x8000, 0x323bdb89 )	/* Characters */
  ROM_LOAD( "eh-00.rom", 0x08000, 0x10000, 0xf648425c ) /* Sprites */
	ROM_LOAD( "eh-01.rom", 0x18000, 0x10000, 0xa3e62bd2 )
	ROM_LOAD( "eh-02.rom", 0x28000, 0x10000, 0x8f7fdbbd )
  ROM_LOAD( "eh-03.rom", 0x38000, 0x10000, 0xa6b316f3 )
	ROM_LOAD( "eh-05.rom", 0x48000, 0x10000, 0x6b305826 ) /* Tiles */
	ROM_LOAD( "eh-06.rom", 0x58000, 0x10000, 0x0b1b4b7d )
	ROM_LOAD( "eh-04.rom", 0x68000, 0x10000, 0x43e9af03 )
  ROM_LOAD( "eh-07.rom", 0x78000, 0x10000, 0x92c8787e )
  ROM_LOAD( "eh-08.rom", 0x88000, 0x08000, 0xea3d163b ) /* Tiles 2 */
  ROM_CONTINUE(0x98000,0x8000)
	ROM_LOAD( "eh-09.rom", 0xa8000, 0x08000, 0x3847cd1f )
  ROM_CONTINUE(0xb8000,0x8000)

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "eh-10.rom", 0x8000, 0x8000, 0xf38bbfdf )
ROM_END

ROM_START( gondo_rom )
	ROM_REGION(0x44000)
 	ROM_LOAD( "dt-00.256", 0x08000, 0x08000, 0x463d0ead )
 	ROM_LOAD( "dt-01.512", 0x10000, 0x10000, 0xc76c007a )
  ROM_LOAD( "dt-02.512", 0x20000, 0x10000, 0xc76c007a )
  ROM_LOAD( "dt-03.512", 0x30000, 0x10000, 0xc76c007a )

	ROM_REGION(0x100000)	/* temporary space for graphics */
  ROM_LOAD( "dt-14.256", 0x00000, 0x08000, 0x434ccb9e )

  ROM_LOAD( "dt-15.512", 0x08000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dt-16.512", 0x18000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dt-19.512", 0x28000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dt-21.512", 0x38000, 0x10000, 0x434ccb9e )





  ROM_LOAD( "dt-17.256", 0x48000, 0x8000, 0x434ccb9e )


  ROM_LOAD( "dt-18.256", 0x58000, 0x8000, 0x434ccb9e )


  ROM_LOAD( "dt-20.256", 0x68000, 0x8000, 0x434ccb9e )


  ROM_LOAD( "dt-22.256", 0x78000, 0x8000, 0x434ccb9e )





  ROM_LOAD( "dt-06.512", 0x88000, 0x10000, 0x434ccb9e )
  ROM_LOAD( "dt-08.512", 0x98000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dt-10.512", 0xa8000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dt-12.512", 0xb8000, 0x10000, 0x434ccb9e )

  ROM_LOAD( "dt-09.256", 0xc8000, 0x8000, 0x434ccb9e )
  ROM_LOAD( "dt-11.256", 0xd8000, 0x8000, 0x434ccb9e )


  ROM_LOAD( "dt-13.256", 0xe8000, 0x8000, 0x434ccb9e )


  ROM_LOAD( "dt-07.256", 0xf8000, 0x8000, 0x434ccb9e )



	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "dt-05.256", 0x8000, 0x8000, 0x364efd26 )
ROM_END

ROM_START( mazeh_rom )
	ROM_REGION(0x20000)
 	ROM_LOAD( "dw-01.rom", 0x08000, 0x08000, 0x463d0ead )
 	ROM_LOAD( "dw-02.rom", 0x10000, 0x10000, 0xc76c007a )

  // 3 4   6 7 8 9 to place

	ROM_REGION(0xc8000)	/* temporary space for graphics */
  ROM_LOAD( "dw-00.rom", 0x00000, 0x8000, 0x1b793a0b )	/* characters */

  ROM_LOAD( "dw-12.rom", 0x08000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dw-13.rom", 0x18000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dw-15.rom", 0x28000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dw-17.rom", 0x38000, 0x10000, 0x434ccb9e )





  ROM_LOAD( "dw-10.rom", 0x48000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dw-11.rom", 0x58000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dw-14.rom", 0x68000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dw-16.rom", 0x78000, 0x10000, 0x434ccb9e )





	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "dw-05.rom", 0x8000, 0x8000, 0x364efd26 )
ROM_END


ROM_START( oscar_rom )
	ROM_REGION(0x20000)
 	ROM_LOAD( "du_10.rom", 0x08000, 0x08000, 0x463d0ead )
 	ROM_LOAD( "du_09.rom", 0x10000, 0x10000, 0xc76c007a )

	ROM_REGION(0xc8000)	/* temporary space for graphics */
  ROM_LOAD( "du_08.rom", 0x00000, 0x4000, 0x1b793a0b )	/* characters */

  ROM_LOAD( "du_04.rom", 0x08000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "du_05.rom", 0x18000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "du_06.rom", 0x28000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "du_07.rom", 0x38000, 0x10000, 0x434ccb9e )





  ROM_LOAD( "du_00.rom", 0x48000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "du_01.rom", 0x58000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "du_02.rom", 0x68000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "du_03.rom", 0x78000, 0x10000, 0x434ccb9e )





	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "du_12.rom", 0x8000, 0x8000, 0x364efd26 )

  ROM_REGION(0x10000)	/* CPU 2, 1st 16k is empty */
	ROM_LOAD( "du_11.rom", 0x0000, 0x10000, 0x364efd26 )
ROM_END

ROM_START( lastmiss_rom )
	ROM_REGION(0x20000)
 	ROM_LOAD( "lm_dl03.rom", 0x08000, 0x08000, 0x463d0ead )
 	ROM_LOAD( "lm_dl04.rom", 0x10000, 0x10000, 0xc76c007a )

	ROM_REGION(0xc8000)	/* temporary space for graphics */
  ROM_LOAD( "lm_dl01.rom", 0x00000, 0x8000, 0x1b793a0b )	/* characters */

  ROM_LOAD( "lm_dl10.rom", 0x08000, 0x8000, 0x434ccb9e )
  ROM_LOAD( "lm_dl11.rom", 0x18000, 0x8000, 0x434ccb9e )


  ROM_LOAD( "lm_dl12.rom", 0x28000, 0x8000, 0x434ccb9e )


  ROM_LOAD( "lm_dl13.rom", 0x38000, 0x8000, 0x434ccb9e )



  ROM_LOAD( "lm_dl06.rom", 0x48000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "lm_dl07.rom", 0x58000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "lm_dl08.rom", 0x68000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "lm_dl09.rom", 0x78000, 0x10000, 0x434ccb9e )

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "lm_dl05.rom", 0x8000, 0x8000, 0x364efd26 )

  ROM_REGION(0x10000)	/* CPU 2, 1st 16k is empty */
	ROM_LOAD( "lm_dl02.rom", 0x0000, 0x10000, 0x364efd26 )
ROM_END

ROM_START( shackled_rom )
	ROM_REGION(0x20000)
 	ROM_LOAD( "dk-02.rom", 0x08000, 0x08000, 0x463d0ead )
 //	ROM_LOAD( "dk-11.rom", 0x10000, 0x10000, 0xc76c007a )

// 3 4 5 6

	ROM_REGION(0xc8000)	/* temporary space for graphics */
  ROM_LOAD( "dk-00.rom", 0x00000, 0x8000, 0x1b793a0b )	/* characters */

  ROM_LOAD( "dk-12.rom", 0x08000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dk-14.rom", 0x18000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dk-16.rom", 0x28000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dk-18.rom", 0x38000, 0x10000, 0x434ccb9e )





  ROM_LOAD( "dk-13.rom", 0x48000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dk-15.rom", 0x58000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dk-17.rom", 0x68000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dk-19.rom", 0x78000, 0x10000, 0x434ccb9e )





  ROM_LOAD( "dk-08.rom", 0x88000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dk-09.rom", 0x98000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dk-10.rom", 0xa8000, 0x10000, 0x434ccb9e )


  ROM_LOAD( "dk-11.rom", 0xb8000, 0x10000, 0x434ccb9e )





	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "dk-07.rom", 0x8000, 0x8000, 0x364efd26 )

  ROM_REGION(0x10000)	/* CPU 2, 1st 16k is empty */
	ROM_LOAD( "dk-01.rom", 0x0000, 0x10000, 0x364efd26 )
ROM_END

struct GameDriver ghostb_driver =
{
	__FILE__,
	0,
	"ghostb",
	"Ghostbusters",
	"????",
	"?????",
	"bm",
	0,
	&ghostb_machine_driver,

	ghostb_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver srdarwin_driver =
{
	__FILE__,
	0,
	"srdarwin",
	"Super Real Darwin",
	"????",
	"?????",
	"bm",
	0,
	&ghostb_machine_driver,

	srdarwin_rom,
	0, 0,
	0,
	0,

	darwin_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver cobracom_driver =
{
	__FILE__,
	0,
	"cobracom",
	"Cobra Command",
	"1988",
	"Data East Corporation",
	"Bryan McPhail",
	0,
	&cobra_machine_driver,

	cobracom_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver gondo_driver =
{
	__FILE__,
	0,
	"gondo",
	"Gondomania",
	"????",
	"?????",
	"Bm",
	0,
	&gondo_machine_driver,

	gondo_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

struct GameDriver oscar_driver =
{
	__FILE__,
	0,
	"oscar",
	"Oscar",
	"????",
	"?????",
	"Bm",
	0,
	&oscar_machine_driver,

	oscar_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver lastmiss_driver =
{
	__FILE__,
	0,
	"lastmiss",
	"lastmiss",
	"????",
	"?????",
	"Bm",
	0,
	&lastmiss_machine_driver,

	lastmiss_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

struct GameDriver shackled_driver =
{
	__FILE__,
	0,
	"shackled",
	"shackled",
	"????",
	"?????",
	"Bm",
	0,
	&lastmiss_machine_driver,

	shackled_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver mazeh_driver =
{
	__FILE__,
	0,
	"mazeh",
	"mazeh",
	"????",
	"?????",
	"Bm",
	0,
	&ghostb_machine_driver,

	mazeh_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};
