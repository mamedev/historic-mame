/***************************************************************************

Tube Panic
(c)1984 Nichibutsu

Driver by Jarek Burczynski.

It wouldn't be possible without help from following people:
Al Kossow helped with finding TTL chips' numbers and made PCB scans.
Tim made some nice screenshots.
Dox lent the Tube Panic PCB to me - I have drawn the schematics using the PCB,
this allowed me to emulate the background-drawing circuit.

----
Tube Panic
Nichibutsu 1984

CPU
84P0100B

tp-b 6->1          19.968MHz

                  tp-2 tp-1  2147 2147 2147 2147 2147 2147 2147 2147

               +------ daughter board ------+
               tp-p 5->8 6116  6116 tp-p 4->1
               +----------------------------+

   z80a              z80a                     z80a

                8910 8910 8910     6116  - tp-s 2->1

       16MHz

 VID
 84P101B

   6MHz                                        +------+ daughter board
                                  6116          tp-c 1
   MS2010-A                                     tp-c 2
                              tp-g 3            tp-c 3
   tp-g 6                                       tp-c 4
                              tp-g 4
   tp-g 5                                       tp-c 8
                                                tp-c 7
   6116                                         tp-c 6
                                       tp-g 1   tp-c 5
                                               +------+
     tp-g 7
                                       tp-g 2
     tp-g 8
                                             4164 4164 4164 4164
                                        4164 4164 4164 4164
  2114

----

Roller Jammer
Nichibutsu 1985

84P0501A

               SW1      SW2                      16A

Z80   6116                        TP-B.5         16B     6116
TP-S.1 TP-S.2 TP-S.3 TP-B.1  8212 TP-B.2 TP-B.3          TP-B.4


 TP-P.1 TP-P.2 TP-P.3 TP-P.4 6116 6116 TP-P.5 TP-P.6 TP-P.7 TP-P.8    6116


       8910 8910 8910         Z80A      Z80A

                               16MHz                       19.968MHz



                      --------------------------------

  6MHz
                                     6116
                                                     TP-C.8
  MS2010-A                     TP-G.4                TP-C.7
                                                     TP-C.6
  TP-G.8                        TP-G.3               TP-C.5

  TP-G.7                                 TP-G.2
                                                     TP-C.4
  6116                                   TP-G.1      TP-C.3
                                                     TP-C.2
                                                     TP-C.1
   TP-G.6

   TP-G.5                                         4164 4164 4164 4164
                                             4164 4164 4164 4164
 2114

----




***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

#include "tubep.h"


static data8_t *cpu_sharedram;
static data8_t *tubep_sprite_sharedram;

static int sound_latch;


/*************************** Main CPU on main PCB **************************/


static WRITE_HANDLER ( cpu_sharedram_w )
{
	cpu_sharedram[offset] = data;
}
static READ_HANDLER ( cpu_sharedram_r )
{
	return cpu_sharedram[offset];
}

static WRITE_HANDLER ( tubep_sprite_sharedram_w )
{
	tubep_sprite_sharedram[offset] = data;
}
static READ_HANDLER ( tubep_sprite_sharedram_r )
{
	return tubep_sprite_sharedram[offset];
}
static WRITE_HANDLER ( tubep_sprite_colorsharedram_w )
{
	tubep_sprite_colorsharedram[offset] = data;
}
static READ_HANDLER ( tubep_sprite_colorsharedram_r )
{
	return tubep_sprite_colorsharedram[offset];
}


static WRITE_HANDLER( tubep_LS259_w )
{
	switch(offset)
	{
		case 0:
		case 1:
				/*
					port b0: bit0 - coin 1 counter
					port b1	 bit0 - coin 2 counter
				*/
				coin_counter_w(offset,data&1);
				break;
		case 2:
				//something...
				break;
		case 5:
				//screen_flip_w(offset,data&1);	/* bit 0 = screen flip, active high */
				break;
		case 6:
				tubep_background_romselect_w(offset,data);	/* bit0 = 0->select roms: B1,B3,B5; bit0 = 1->select roms: B2,B4,B6 */
				break;
		case 7:
				tubep_colorproms_A4_line_w(offset,data);	/* bit0 = line A4 (color proms address) state */
				break;
		default:
				break;
	}
}


static WRITE_HANDLER( tubep_backgroundram_w )
{
	tubep_backgroundram[offset] = data;
}

static MEMORY_READ_START( tubep_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xa000, 0xa7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( tubep_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xa000, 0xa7ff, MWA_RAM },
	{ 0xc000, 0xc7ff, tubep_textram_w, &tubep_textram },	/* RAM on GFX PCB @B13 */
	{ 0xe000, 0xe7ff, cpu_sharedram_w },
	{ 0xe800, 0xebff, tubep_backgroundram_w },				/* row of 8 x 2147 RAMs on main PCB */
MEMORY_END

static PORT_READ_START( tubep_readport )
	{ 0x80, 0x80, input_port_3_r },
	{ 0x90, 0x90, input_port_4_r },
	{ 0xa0, 0xa0, input_port_5_r },

	{ 0xb0, 0xb0, input_port_2_r },
	{ 0xc0, 0xc0, input_port_1_r },
	{ 0xd0, 0xd0, input_port_0_r },
PORT_END



static WRITE_HANDLER( main_cpu_irq_line_clear_w )
{
//	cpu_set_irq_line(0,CLEAR_LINE);
//not used - handled by MAME anyway (because it is usual Vblank int)
	return;
}

static WRITE_HANDLER( tubep_soundlatch_w )
{
	sound_latch = (data&0x7f) | 0x80;
}

static PORT_WRITE_START( tubep_writeport )
	{ 0x80, 0x80, main_cpu_irq_line_clear_w },
	{ 0xb0, 0xb7, tubep_LS259_w },
	{ 0xd0, 0xd0, tubep_soundlatch_w },
PORT_END




/************************** Slave CPU on main PCB ****************************/

static MEMORY_READ_START( tubep_g_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xe7ff, cpu_sharedram_r },
	{ 0xf800, 0xffff, tubep_sprite_sharedram_r },
MEMORY_END


static MEMORY_WRITE_START( tubep_g_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xa000, 0xa000, tubep_background_a000_w },
	{ 0xc000, 0xc000, tubep_background_c000_w },
	{ 0xe000, 0xe7ff, cpu_sharedram_w, &cpu_sharedram },	/* 6116 #1 */
	{ 0xe800, 0xebff, MWA_RAM, &tubep_backgroundram },		/* row of 8 x 2147 RAMs on main PCB */
	{ 0xf000, 0xf3ff, tubep_sprite_colorsharedram_w },		/* sprites color lookup table */
	{ 0xf800, 0xffff, tubep_sprite_sharedram_w },			/* program copies here part of shared ram ?? */
MEMORY_END

static READ_HANDLER( tubep_soundlatch_r )
{
 	int res;

	res = sound_latch;
	sound_latch = 0; /* "=0" ????  or "&= 0x7f" ?????  works either way */

	/*logerror("SOUND COMM READ %2x\n",res);*/

	return res;
}

static READ_HANDLER( tubep_sound_irq_ack )
{
	cpu_set_irq_line(2, 0, CLEAR_LINE);
	return 0;
}

static WRITE_HANDLER( tubep_sound_unknown )
{
	/*logerror("Sound CPU writes to port 0x07 - unknown function\n");*/
	return;
}


static MEMORY_READ_START( tubep_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xd000, 0xd000, tubep_sound_irq_ack },
	{ 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( tubep_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0xe000, 0xe7ff, MWA_RAM },		/* 6116 #3 */
MEMORY_END

static PORT_READ_START( tubep_sound_readport )
	{ 0x06, 0x06, tubep_soundlatch_r },
PORT_END

static PORT_WRITE_START( tubep_sound_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x02, 0x02, AY8910_control_port_1_w },
	{ 0x03, 0x03, AY8910_write_port_1_w },
	{ 0x04, 0x04, AY8910_control_port_2_w },
	{ 0x05, 0x05, AY8910_write_port_2_w },
	{ 0x07, 0x07, tubep_sound_unknown },
PORT_END

static void scanline_callback(int scanline)
{
	/* interrupt is generated whenever line V6 from video part goes lo->hi */
	/* that is when scanline is 64 and 192 accordingly */

	cpu_set_irq_line(2,0,ASSERT_LINE);	/* sound cpu interrupt (music tempo) */

	scanline += 128;
	scanline &= 255;

	timer_set( cpu_getscanlinetime( scanline ), scanline, scanline_callback );
}

static MACHINE_INIT( tubep )
{
	timer_set(cpu_getscanlinetime( 64 ), 64, scanline_callback );
}











/****************************************************************/

static WRITE_HANDLER( rjammer_LS259_w )
{
	switch(offset)
	{
		case 0:
		case 1:
				coin_counter_w(offset,data&1);	/* bit 0 = coin counter */
				break;
		case 5:
				//screen_flip_w(offset,data&1);	/* bit 0 = screen flip, active high */
				break;
		default:
				break;
	}
}

static WRITE_HANDLER( rjammer_soundlatch_w )
{
	sound_latch = data;
	cpu_set_nmi_line(2, PULSE_LINE);
}

static PORT_READ_START( rjammer_readport )
	{ 0x00, 0x00, input_port_2_r },	/* a bug in game code (during attract mode) */
	{ 0x80, 0x80, input_port_2_r },
	{ 0x90, 0x90, input_port_3_r },
	{ 0xa0, 0xa0, input_port_4_r },
	{ 0xb0, 0xb0, input_port_0_r },
	{ 0xc0, 0xc0, input_port_1_r },
PORT_END

static PORT_WRITE_START( rjammer_writeport )
	{ 0xd0, 0xd7, rjammer_LS259_w },
	{ 0xe0, 0xe0, main_cpu_irq_line_clear_w },	/* clear IRQ interrupt */
	{ 0xf0, 0xf0, rjammer_soundlatch_w },
PORT_END


static MEMORY_READ_START( rjammer_readmem )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xe000, 0xe7ff, cpu_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( rjammer_writemem )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa7ff, MWA_RAM },						/* MB8416 SRAM on daughterboard on main PCB (there are two SRAMs, this is the one on the left) */
	{ 0xc000, 0xc7ff, tubep_textram_w, &tubep_textram },/* RAM on GFX PCB @B13 */
	{ 0xe000, 0xe7ff, cpu_sharedram_w },				/* MB8416 SRAM on daughterboard (the one on the right) */
MEMORY_END



static PORT_WRITE_START( rjammer_slave_writeport )
	{ 0xb0, 0xb0, rjammer_background_page_w },
	{ 0xd0, 0xd0, rjammer_background_LS377_w },
PORT_END

static MEMORY_READ_START( rjammer_slave_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xa000, 0xa7ff, MRA_RAM },			/* M5M5117P @21G */
	{ 0xe000, 0xe7ff, cpu_sharedram_r },	/* MB8416 on daughterboard (the one on the right) */
	{ 0xe800, 0xefff, MRA_RAM },			/* M5M5117P @19B (background) */
	{ 0xf800, 0xffff, tubep_sprite_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( rjammer_slave_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xa000, 0xa7ff, MWA_RAM },						/* M5M5117P @21G */
	{ 0xe000, 0xe7ff, cpu_sharedram_w, &cpu_sharedram },/* MB8416 on daughterboard (the one on the right) */
	{ 0xe800, 0xefff, MWA_RAM, &rjammer_backgroundram },/* M5M5117P @19B (background) */
	{ 0xf800, 0xffff, tubep_sprite_sharedram_w },
MEMORY_END


/* MS2010-A CPU (equivalent to NSC8105 with one new opcode: 0xec) on graphics PCB */
static MEMORY_READ_START( nsc_readmem )
	{ 0x0000, 0x03ff, tubep_sprite_colorsharedram_r },
	{ 0x0800, 0x0fff, tubep_sprite_sharedram_r },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( nsc_writemem )
	{ 0x0000, 0x03ff, tubep_sprite_colorsharedram_w, &tubep_sprite_colorsharedram },
	{ 0x0800, 0x0fff, tubep_sprite_sharedram_w, &tubep_sprite_sharedram },
	{ 0x2000, 0x2009, tubep_sprite_control_w },
	{ 0x200a, 0x200b, MWA_NOP }, /* not used by the games - perhaps designed for debugging */
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END





/****************************** Sound CPU *******************************/




static READ_HANDLER( rjammer_soundlatch_r )
{
 	int res = sound_latch;
	return res;
}

static WRITE_HANDLER( rjammer_voice_startstop_w )
{
	/* bit 0 of data selects voice start/stop (reset pin on MSM5205)*/
	// 0 -stop; 1-start
	MSM5205_reset_w (0, (data&1)^1 );

	return;
}
static WRITE_HANDLER( rjammer_voice_frequency_select_w )
{
	/* bit 0 of data selects voice frequency on MSM5205 */
	// 0 -4 KHz; 1- 8KHz
	if (data&1)
		MSM5205_playmode_w(0,MSM5205_S48_4B);	/* 8 KHz */
	else
		MSM5205_playmode_w(0,MSM5205_S96_4B);	/* 4 KHz */

	return;
}

static int ls74 = 0;
static int ls377 = 0;

static void rjammer_adpcm_vck (int data)
{
	ls74 = (ls74+1) & 1;

	if (ls74==1)
	{
		MSM5205_data_w(0, (ls377>>0) & 15 );
		cpu_set_irq_line(2, 0, ASSERT_LINE );
	}
	else
	{
		MSM5205_data_w(0, (ls377>>4) & 15 );
	}

}

static WRITE_HANDLER( rjammer_voice_input_w )
{
	/* 8 bits of adpcm data for MSM5205 */
	/* need to buffer the data, and switch two nibbles on two following interrupts*/

	ls377 = data;


	/* NOTE: game resets interrupt line on ANY access to ANY I/O port.
			I do it here because this port (0x80) is first one accessed
			in the interrupt routine.
	*/
	cpu_set_irq_line(2, 0, CLEAR_LINE );
	return;
}

static WRITE_HANDLER( rjammer_voice_intensity_control_w )
{
	/* 4 LSB bits select the intensity (analog circuit that alters the output from MSM5205) */
	// need to buffer the data
	return;
}

static MEMORY_READ_START( rjammer_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( rjammer_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xe000, 0xe7ff, MWA_RAM },	/* M5M5117P (M58125P @2C on schematics) */
MEMORY_END

static PORT_READ_START( rjammer_sound_readport )
	{ 0x00, 0x00, rjammer_soundlatch_r },
PORT_END

static PORT_WRITE_START( rjammer_sound_writeport )
	{ 0x10, 0x10, rjammer_voice_startstop_w },
	{ 0x18, 0x18, rjammer_voice_frequency_select_w },
	{ 0x80, 0x80, rjammer_voice_input_w },
	{ 0x90, 0x90, AY8910_control_port_0_w },
	{ 0x91, 0x91, AY8910_write_port_0_w },
	{ 0x92, 0x92, AY8910_control_port_1_w },
	{ 0x93, 0x93, AY8910_write_port_1_w },
	{ 0x94, 0x94, AY8910_control_port_2_w },
	{ 0x95, 0x95, AY8910_write_port_2_w },
	{ 0x96, 0x96, rjammer_voice_intensity_control_w },
PORT_END


static WRITE_HANDLER( ay8910_portA_0_w )
{
		//analog sound control
}
static WRITE_HANDLER( ay8910_portB_0_w )
{
		//analog sound control
}
static WRITE_HANDLER( ay8910_portA_1_w )
{
		//analog sound control
}
static WRITE_HANDLER( ay8910_portB_1_w )
{
		//analog sound control
}
static WRITE_HANDLER( ay8910_portA_2_w )
{
		//analog sound control
}
static WRITE_HANDLER( ay8910_portB_2_w )
{
		//analog sound control
}



INPUT_PORTS_START( tubep )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Coin, Start */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "40000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x04, "60000" )
	PORT_DIPSETTING(    0x00, "80000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "In Game Sounds" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END




INPUT_PORTS_START( rjammer )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Bonus Time" )
	PORT_DIPSETTING(    0x02, "100" )
	PORT_DIPSETTING(    0x00, "200" )
	PORT_DIPNAME( 0x0c, 0x0c, "Clear Men" )
	PORT_DIPSETTING(    0x0c, "20" )
	PORT_DIPSETTING(    0x08, "30" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Time" )
	PORT_DIPSETTING(    0x20, "40" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8, 8,	/* 8*8 characters */
	512,	/* 512 characters */
	1,		/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};
static struct GfxDecodeInfo tubep_gfxdecodeinfo[] =
{
	{ REGION_GFX1,      0, &charlayout,       0, 32 },	/* 32 color codes */
	{ -1 }
};

static struct GfxDecodeInfo rjammer_gfxdecodeinfo[] =
{
	{ REGION_GFX1,      0, &charlayout,       0, 16 },	/* 16 color codes */
	{ -1 }
};

static struct AY8910interface ay8910_interface =
{
	3,					/* 3 chips */
	19968000 / 8 / 2,	/* Xtal3 div by LS669 Q2, div by LS669 Q0 (signal RH1) */
	{ 15, 15, 15 },		/* volume */
	{ 0, 0, 0 },		/* read port A */
	{ 0, 0, 0 },		/* read port B */
	{ ay8910_portA_0_w, ay8910_portA_1_w, ay8910_portA_2_w }, /* write port A */
	{ ay8910_portB_0_w, ay8910_portB_1_w, ay8910_portB_2_w }  /* write port B */
};

static struct MSM5205interface msm5205_interface =
{
	1,								/* 1 chip */
	384000, 						/* 384 KHz */
	{ rjammer_adpcm_vck },			/* VCK function */
	{ MSM5205_S48_4B},				/* 8 KHz (changes at run time) */
	{ 100 }							/* volume */
};



static MACHINE_DRIVER_START( tubep )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,16000000 / 4)	/* 4 MHz */
	MDRV_CPU_MEMORY(tubep_readmem,tubep_writemem)
	MDRV_CPU_PORTS(tubep_readport,tubep_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000 / 4)	/* 4 MHz */
	MDRV_CPU_MEMORY(tubep_g_readmem,tubep_g_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,19968000 / 8)	/* X2 19968000 Hz divided by LS669 (on Qc output) (signal RH0) */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(tubep_sound_readmem,tubep_sound_writemem)
	MDRV_CPU_PORTS(tubep_sound_readport,tubep_sound_writeport)

	MDRV_CPU_ADD(NSC8105,6000000/4)	/* 6 MHz Xtal - divided internally ??? */
	MDRV_CPU_MEMORY(nsc_readmem,nsc_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(tubep)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(tubep_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32 + 256*64)
	MDRV_COLORTABLE_LENGTH(32*2)

	MDRV_PALETTE_INIT(tubep)
	MDRV_VIDEO_START(tubep)
	MDRV_VIDEO_EOF(tubep_eof)
	MDRV_VIDEO_UPDATE(tubep)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rjammer )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,16000000 / 4)	/* 4 MHz */
	MDRV_CPU_MEMORY(rjammer_readmem,rjammer_writemem)
	MDRV_CPU_PORTS(rjammer_readport,rjammer_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000 / 4)	/* 4 MHz */
	MDRV_CPU_MEMORY(rjammer_slave_readmem,rjammer_slave_writemem)
	MDRV_CPU_PORTS(0,rjammer_slave_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,19968000 / 8)	/* Xtal3 divided by LS669 (on Qc output) (signal RH0) */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(rjammer_sound_readmem,rjammer_sound_writemem)
	MDRV_CPU_PORTS(rjammer_sound_readport,rjammer_sound_writeport)

	MDRV_CPU_ADD(NSC8105,6000000/4)	/* 6 MHz Xtal - divided internally ??? */
	MDRV_CPU_MEMORY(nsc_readmem,nsc_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)


	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(rjammer_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(2*16 + 16*2)

	MDRV_PALETTE_INIT(rjammer)
	MDRV_VIDEO_START(tubep)
	MDRV_VIDEO_EOF(tubep_eof)
	MDRV_VIDEO_UPDATE(rjammer)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END




ROM_START( tubep )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* Z80 (master) cpu code */
	ROM_LOAD( "tp-p.5", 0x0000, 0x2000, 0xd5e0cc2f )
	ROM_LOAD( "tp-p.6", 0x2000, 0x2000, 0x97b791a0 )
	ROM_LOAD( "tp-p.7", 0x4000, 0x2000, 0xadd9983e )
	ROM_LOAD( "tp-p.8", 0x6000, 0x2000, 0xb3793cb5 )

	ROM_REGION( 0x10000,REGION_CPU2, 0 ) /* Z80 (slave) cpu code */
	ROM_LOAD( "tp-p.1", 0x0000, 0x2000, 0xb4020fcc )
	ROM_LOAD( "tp-p.2", 0x2000, 0x2000, 0xa69862d6 )
	ROM_LOAD( "tp-p.3", 0x4000, 0x2000, 0xf1d86e00 )
	ROM_LOAD( "tp-p.4", 0x6000, 0x2000, 0x0a1027bc )

	ROM_REGION( 0x10000,REGION_CPU3, 0 ) /* Z80 (sound) cpu code */
	ROM_LOAD( "tp-s.1", 0x0000, 0x2000, 0x78964fcc )
	ROM_LOAD( "tp-s.2", 0x2000, 0x2000, 0x61232e29 )

	ROM_REGION( 0x10000,REGION_CPU4, 0 ) /* 64k for the custom CPU */
	ROM_LOAD( "tp-g5.e1", 0xc000, 0x2000, 0x9f375b27 )
	ROM_LOAD( "tp-g6.d1", 0xe000, 0x2000, 0x3ea127b8 )

	ROM_REGION( 0xc000, REGION_USER1, 0 ) /* background data */
	ROM_LOAD( "tp-b.1", 0x0000, 0x2000, 0xfda355e0 )
	ROM_LOAD( "tp-b.2", 0x2000, 0x2000, 0xcbe30149 )
	ROM_LOAD( "tp-b.3", 0x4000, 0x2000, 0xf5d118e7 )
	ROM_LOAD( "tp-b.4", 0x6000, 0x2000, 0x01952144 )
	ROM_LOAD( "tp-b.5", 0x8000, 0x2000, 0x4dabea43 )
	ROM_LOAD( "tp-b.6", 0xa000, 0x2000, 0x01952144 )

	ROM_REGION( 0x18000,REGION_USER2, 0 )
	ROM_LOAD( "tp-c.1", 0x0000, 0x2000, 0xec002af2 )
	ROM_LOAD( "tp-c.2", 0x2000, 0x2000, 0xc44f7128 )
	ROM_LOAD( "tp-c.3", 0x4000, 0x2000, 0x4146b0c9 )
	ROM_LOAD( "tp-c.4", 0x6000, 0x2000, 0x552b58cf )
	ROM_LOAD( "tp-c.5", 0x8000, 0x2000, 0x2bb481d7 )
	ROM_LOAD( "tp-c.6", 0xa000, 0x2000, 0xc07a4338 )
	ROM_LOAD( "tp-c.7", 0xc000, 0x2000, 0x87b8700a )
	ROM_LOAD( "tp-c.8", 0xe000, 0x2000, 0xa6497a03 )
	ROM_LOAD( "tp-g4.d10", 0x10000, 0x1000, 0x40a1fe00 ) /* 2732 eprom is used, but the PCB is prepared for 2764 eproms */
	ROM_RELOAD(            0x11000, 0x1000 )
	ROM_LOAD( "tp-g1.e13", 0x12000, 0x1000, 0x4a7407a2 )
	ROM_LOAD( "tp-g2.f13", 0x13000, 0x1000, 0xf0b26c2e )

	ROM_LOAD( "tp-g7.h2",  0x14000, 0x2000, 0x105cb9e4 )
	ROM_LOAD( "tp-g8.i2",  0x16000, 0x2000, 0x27e5e6c1 )

	ROM_REGION( 0x1000, REGION_GFX1, 0 )
	ROM_LOAD( "tp-g3.c10", 0x0000, 0x1000, 0x657a465d )	/* text characters */

	ROM_REGION( 0x40,   REGION_PROMS, 0 ) /* color proms */
	ROM_LOAD( "tp-2.c12", 0x0000, 0x0020, 0xac7e582f ) /* text and sprites palette */
	ROM_LOAD( "tp-1.c13", 0x0020, 0x0020, 0xcd0910d6 ) /* color control prom */
ROM_END


ROM_START( rjammer )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* Z80 (master) cpu code */
	ROM_LOAD( "tp-p.1", 0x0000, 0x2000, 0x93eeed67 )
	ROM_LOAD( "tp-p.2", 0x2000, 0x2000, 0xed2830c4 )
	ROM_LOAD( "tp-p.3", 0x4000, 0x2000, 0xe29f25e3 )
	ROM_LOAD( "tp-p.4", 0x8000, 0x2000, 0x6ed71fbc )
	ROM_CONTINUE(       0x6000, 0x2000 )

	ROM_REGION( 0x10000,REGION_CPU2, 0 ) /* Z80 (slave) cpu code */
	ROM_LOAD( "tp-p.8", 0x0000, 0x2000, 0x388b9c66 )
	ROM_LOAD( "tp-p.7", 0x2000, 0x2000, 0x595030bb )
	ROM_LOAD( "tp-p.6", 0x4000, 0x2000, 0xb5aa0f89 )
	ROM_LOAD( "tp-p.5", 0x6000, 0x2000, 0x56eae9ac )

	ROM_REGION( 0x10000,REGION_CPU3, 0 ) /* Z80 (sound) cpu code */
	ROM_LOAD( "tp-b1.6d", 0x0000, 0x2000, 0xb1c2525c )
	ROM_LOAD( "tp-s3.4d", 0x2000, 0x2000, 0x90c9d0b9 )
	ROM_LOAD( "tp-s2.2d", 0x4000, 0x2000, 0x444b6a1d )
	ROM_LOAD( "tp-s1.1d", 0x6000, 0x2000, 0x391097cd )

	ROM_REGION( 0x10000,REGION_CPU4, 0 ) /* 64k for the custom CPU */
	ROM_LOAD( "tp-g7.e1",  0xc000, 0x2000, 0x9f375b27 )
	ROM_LOAD( "tp-g8.d1",  0xe000, 0x2000, 0x2e619fec )

	ROM_REGION( 0x7000, REGION_USER1, 0 ) /* background data */
	ROM_LOAD( "tp-b3.13d", 0x0000, 0x1000, 0xb80ef399 )
	ROM_LOAD( "tp-b5.11b", 0x1000, 0x2000, 0x0f260bfe )
	ROM_LOAD( "tp-b2.11d", 0x3000, 0x2000, 0x8cd2c917 )
	ROM_LOAD( "tp-b4.19c", 0x5000, 0x2000, 0x6600f306 )

	ROM_REGION( 0x18000,REGION_USER2, 0 )
	ROM_LOAD( "tp-c.8", 0x0000, 0x2000, 0x9f31ecb5 )
	ROM_LOAD( "tp-c.7", 0x2000, 0x2000, 0xcbf093f1 )
	ROM_LOAD( "tp-c.6", 0x4000, 0x2000, 0x11f9752b )
	ROM_LOAD( "tp-c.5", 0x6000, 0x2000, 0x513f8777 )
	ROM_LOAD( "tp-c.1", 0x8000, 0x2000, 0xef573117 )
	ROM_LOAD( "tp-c.2", 0xa000, 0x2000, 0x1d29f1e6 )
	ROM_LOAD( "tp-c.3", 0xc000, 0x2000, 0x086511a7 )
	ROM_LOAD( "tp-c.4", 0xe000, 0x2000, 0x49f372ea )
	ROM_LOAD( "tp-g3.d10", 0x10000, 0x1000, 0x1f2abec5 )	/* 2732 eprom is used, but the PCB is prepared for 2764 eproms */
	ROM_RELOAD(            0x11000, 0x1000 )
	ROM_LOAD( "tp-g2.e13", 0x12000, 0x1000, 0x4a7407a2 )
	ROM_LOAD( "tp-g1.f13", 0x13000, 0x1000, 0xf0b26c2e )
	ROM_LOAD( "tp-g6.h2",  0x14000, 0x2000, 0x105cb9e4 )
	ROM_LOAD( "tp-g5.i2",  0x16000, 0x2000, 0x27e5e6c1 )

	ROM_REGION( 0x1000, REGION_GFX1, 0 )
	ROM_LOAD( "tp-g4.c10", 0x0000, 0x1000, 0x99e72549 )	/* text characters */

	ROM_REGION( 0x40,   REGION_PROMS, 0 ) /* color proms */
	ROM_LOAD( "16b", 0x0000, 0x0020, 0x9a12873a ) /* text palette, sprites palette */
	ROM_LOAD( "16a", 0x0020, 0x0020, 0x90222a71 ) /* background palette */
ROM_END

/*     year  rom      parent  machine  inp   init */
GAME( 1984, tubep,   0,      tubep,   tubep,   0, ROT0, "Nichibutsu + Fujitek", "Tube Panic" )
GAME( 1984, rjammer, 0,      rjammer, rjammer, 0, ROT0, "Nichibutsu + Alice", "Roller Jammer" )

