/***************************************************************************

							-= Kaneko 16 Bit Games =-

					driver by	Luca Elia (l.elia@tin.it)


CPU    :  68000  +  MCU [Optional]

SOUND  :  OKI-M6295 x (1 | 2) + YM2149 x (0 | 2)
   OR  :  Z80  +  YM2151

OTHER  :  93C46 EEPROM [Optional]

CUSTOM :  VU-001 046A                  (48pin PQFP)
          VU-002 052 151021            (160pin PQFP)	<- Sprites
          VU-003 										<- High Colour Background
          VIEW2-CHIP 23160-509 9047EAI (144pin PQFP)	<- Tilemaps
          MUX2-CHIP                    (64pin PQFP)
          HELP1-CHIP
          IU-001 9045KP002             (44pin PQFP)
          I/O JAMMA MC-8282 047        (46pin)			<- Inputs


---------------------------------------------------------------------------
Year + Game					PCB			Notes
---------------------------------------------------------------------------
91	The Berlin Wall						Encrypted ROMs -> wrong background colors
	Magical Crystals		Z00FC-02
92	Bakuretsu Breaker					Incomplete dump (gfx+sfx missing)
	Blaze On							2 Sprites Chips !?
	Sand Scorpion (by Face)				MCU protection (collision detection etc.)
	Shogun Warriors						MCU protection (68k code snippets, NOT WORKING)
94	Great 1000 Miles Rally				MCU protection (EEPROM handling etc.)
9?	Great 1000 Miles Rally 2			Incomplete dump (code missing!)
---------------------------------------------------------------------------

Note: gtmr manual shows "Compatible with AX Kaneko System Board"

To Do:

[berlwall]

- Fix colors of the high color background (encrypted)

[gtmr]

- Stage 4: The layers' scrolling is very jerky for a couple of seconds
  in the middle of this level (probably interrupt related)

- The layers' colours are not initialised when showing the self test
  screen and the very first screen (with the Kaneko logo in the middle).
  They're probably supposed to be disabled in those occasions, but the
  relevant registers aren't changed throughout the game (?)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "kaneko16.h"

/* Variables only used here: */

static int shogwarr_mcu_status, shogwarr_mcu_command_offset;
static data16_t *mcu_ram, gtmr_mcu_com[4];


/***************************************************************************


							Machine Initialisation


***************************************************************************/

void kaneko16_init_machine(void)
{
	kaneko16_sprite_type  = 0;

	kaneko16_sprite_xoffs = 0;
	kaneko16_sprite_yoffs = 0;

/*
	Sx = Sprites with priority x, x = tiles with priority x,
	Sprites - Tiles Order (bottom -> top):

	0	S0	1	2	3
	0	1	S1	2	3
	0	1	2	S2	3
	0	1	2	3	S3
*/
	kaneko16_priority.tile[0] = 0;
	kaneko16_priority.tile[1] = 1;
	kaneko16_priority.tile[2] = 2;
	kaneko16_priority.tile[3] = 3;

	kaneko16_priority.sprite[0] = 0xfffc;	// above tile[0],   below the others
	kaneko16_priority.sprite[1] = 0xfff0;	// above tile[0-1], below the others
	kaneko16_priority.sprite[2] = 0xff00;	// above tile[0-2], below the others
	kaneko16_priority.sprite[3] = 0x0000;	// above all
}

static void berlwall_init_machine(void)
{
	kaneko16_init_machine();

	kaneko16_sprite_type = 2;	// like type 0, but using 16 instead of 8 bytes
}

static void blazeon_init_machine(void)
{
	kaneko16_init_machine();

	kaneko16_sprite_xoffs = 0x10000 - 0x680;
	kaneko16_sprite_yoffs = 0x000;

/*
	Sx = Sprites with priority x, x = tiles with priority x,
	Sprites - Tiles Order (bottom -> top):

	0	S0	1	2	3
	0	1	S1	2	3
	0	1	2	3	S2
	0	1	2	3	S3
*/
	kaneko16_priority.sprite[0] = 0xfffc;	// above tile[0], below the others
	kaneko16_priority.sprite[1] = 0xfff0;	// above tile[0-1], below the others
	kaneko16_priority.sprite[2] = 0x0000;	// above all
	kaneko16_priority.sprite[3] = 0x0000;	// ""
}

static void gtmr_init_machine(void)
{
	kaneko16_init_machine();

	kaneko16_sprite_type = 1;

	memset(gtmr_mcu_com, 0, 4 * sizeof( data16_t) );
}

static void mgcrystl_init_machine (void)
{
	kaneko16_init_machine();
/*
	Sx = Sprites with priority x, x = tiles with priority x,
	Sprites - Tiles Order:

	S0:	below 0 2

	S1:	over  2
		below 0

	S2:	over 0 2

	S3:	over all

	tiles of the 2nd VIEW2 chip always behind sprites?

*/
	kaneko16_priority.tile[0] = 2;	// priorty mask = 1
	kaneko16_priority.tile[1] = 0;	// priorty mask = 2
	kaneko16_priority.tile[2] = 3;	// priorty mask = 4
	kaneko16_priority.tile[3] = 1;	// priorty mask = 8

	kaneko16_priority.sprite[0] = 0xfffe;	// below all
	kaneko16_priority.sprite[1] = 0xfffc;	// above tile[0], below the others
	kaneko16_priority.sprite[2] = 0x0000;	// above all
	kaneko16_priority.sprite[3] = 0x0000;	// ""
}

static void sandscrp_init_machine(void)
{
	kaneko16_init_machine();

	kaneko16_sprite_type = 3;	// "different" sprites layout

	watchdog_reset16_r(0,0);	// start with an armed watchdog
}

static void shogwarr_init_machine(void)
{
	kaneko16_init_machine();

	shogwarr_mcu_status = 0;
	shogwarr_mcu_command_offset = 0;
}


/***************************************************************************


							MCU Code Simulation


***************************************************************************/

/***************************************************************************
							Great 1000 Miles Rally
***************************************************************************/

const struct GameDriver driver_gtmr;
const struct GameDriver driver_gtmre;
const struct GameDriver driver_gtmrusa;

/*

	MCU Tasks:

	- Write and ID string to shared RAM.
	- Access the EEPROM
	- Read the DSWs

*/

void gtmr_mcu_run(void)
{
	data16_t mcu_command	=	mcu_ram[0x0010/2];
	data16_t mcu_offset		=	mcu_ram[0x0012/2] / 2;
	data16_t mcu_data		=	mcu_ram[0x0014/2];

	logerror("CPU #0 PC %06X : MCU executed command: %04X %04X %04X\n",cpu_get_pc(),mcu_command,mcu_offset*2,mcu_data);

	switch (mcu_command >> 8)
	{

		case 0x02:	// Read from NVRAM
		{
			void *f;
			if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_NVRAM,0)) != 0)
			{
				osd_fread(f,&mcu_ram[mcu_offset], 128);
				osd_fclose(f);
			}
		}
		break;

		case 0x42:	// Write to NVRAM
		{
			void *f;
			if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_NVRAM,1)) != 0)
			{
				osd_fwrite(f,&mcu_ram[mcu_offset], 128);
				osd_fclose(f);
			}
		}
		break;

		case 0x03:	// DSW
		{
			mcu_ram[mcu_offset] = readinputport(4);
		}
		break;

		case 0x04:	// TEST (2 versions)
		{
			if (Machine->gamedrv == &driver_gtmr)
			{
				/* MCU writes the string "MM0525-TOYBOX199" to shared ram */
				mcu_ram[mcu_offset+0] = 0x4d4d;
				mcu_ram[mcu_offset+1] = 0x3035;
				mcu_ram[mcu_offset+2] = 0x3235;
				mcu_ram[mcu_offset+3] = 0x2d54;
				mcu_ram[mcu_offset+4] = 0x4f59;
				mcu_ram[mcu_offset+5] = 0x424f;
				mcu_ram[mcu_offset+6] = 0x5831;
				mcu_ram[mcu_offset+7] = 0x3939;
			}
			else if ( (Machine->gamedrv == &driver_gtmre)  ||
					  (Machine->gamedrv == &driver_gtmrusa) )
			{
				/* MCU writes the string "USMM0713-TB1994 " to shared ram */
				mcu_ram[mcu_offset+0] = 0x5553;
				mcu_ram[mcu_offset+1] = 0x4d4d;
				mcu_ram[mcu_offset+2] = 0x3037;
				mcu_ram[mcu_offset+3] = 0x3133;
				mcu_ram[mcu_offset+4] = 0x2d54;
				mcu_ram[mcu_offset+5] = 0x4231;
				mcu_ram[mcu_offset+6] = 0x3939;
				mcu_ram[mcu_offset+7] = 0x3420;
			}
		}
		break;
	}

}


#define GTMR_MCU_COM_W(_n_) \
WRITE16_HANDLER( gtmr_mcu_com##_n_##_w ) \
{ \
	COMBINE_DATA(&gtmr_mcu_com[_n_]); \
	if (gtmr_mcu_com[0] != 0xFFFF)	return; \
	if (gtmr_mcu_com[1] != 0xFFFF)	return; \
	if (gtmr_mcu_com[2] != 0xFFFF)	return; \
	if (gtmr_mcu_com[3] != 0xFFFF)	return; \
\
	memset(gtmr_mcu_com, 0, 4 * sizeof( data16_t ) ); \
	gtmr_mcu_run(); \
}

GTMR_MCU_COM_W(0)
GTMR_MCU_COM_W(1)
GTMR_MCU_COM_W(2)
GTMR_MCU_COM_W(3)


/***************************************************************************
								Sand Scorpion
***************************************************************************/

/*

	MCU Tasks:

	- Collision detection (test if 2 rectangles overlap)
	- Multiply 2 words, obtaining a long word
	- Return a random value?

*/

static READ16_HANDLER( sandscrp_mcu_ram_r )
{
	switch( offset )
	{
		case 0x04/2:	// Bit 0: collision detection
		{
			/* First rectangle */
			int x10		=	mcu_ram[0x00/2];
			int x11		=	mcu_ram[0x02/2] + x10;
			int y10		=	mcu_ram[0x04/2];
			int y11		=	mcu_ram[0x06/2] + y10;

			/* Second rectangle */
			int x20		=	mcu_ram[0x08/2];
			int x21		=	mcu_ram[0x0a/2] + x20;
			int y20		=	mcu_ram[0x0c/2];
			int y21		=	mcu_ram[0x0e/2] + y20;

			/* Sign extend the words */
			x10 = (x10 & 0x7fff) - (x10 & 0x8000);
			x11 = (x11 & 0x7fff) - (x11 & 0x8000);
			y10 = (y10 & 0x7fff) - (y10 & 0x8000);
			y11 = (y11 & 0x7fff) - (y11 & 0x8000);
			x20 = (x20 & 0x7fff) - (x20 & 0x8000);
			x21 = (x21 & 0x7fff) - (x21 & 0x8000);
			y20 = (y20 & 0x7fff) - (y20 & 0x8000);
			y21 = (y21 & 0x7fff) - (y21 & 0x8000);

			/* Check if they overlap */
			if	(	( x10 > x21 ) || ( x11 < x20 ) ||
					( y10 > y21 ) || ( y11 < y20 )	)
				return 0;
			else
				return 1;
		}
		break;

		case 0x10/2:	// Multiply 2 words, obtain a long word.
		case 0x12/2:
		{
			int res = mcu_ram[0x10/2] * mcu_ram[0x12/2];
			if (offset == 0x10/2)	return (res >> 16) & 0xffff;
			else					return (res >>  0) & 0xffff;
		}
		break;

		case 0x14/2:	// Random?
			return (rand() & 0xffff);
	}

	logerror("CPU #0 PC %06X : Unknown MCU word %04X read\n",cpu_get_pc(),offset*2);
	return mcu_ram[offset];
}

static WRITE16_HANDLER( sandscrp_mcu_ram_w )
{
	COMBINE_DATA(&mcu_ram[offset]);
}


/***************************************************************************
								Shogun Warriors
***************************************************************************/

/* Preliminary simulation: the game doesn't work */

/*

	MCU Tasks:

	- Read the DSWs
	- Supply code snippets to the 68000

*/

void shogwarr_mcu_run(void)
{
	data16_t mcu_command;

	if ( shogwarr_mcu_status != (1|2|4|8) )	return;

	mcu_command = mcu_ram[shogwarr_mcu_command_offset + 0];

	if (mcu_command == 0) return;

	logerror("CPU #0 PC %06X : MCU executed command at %04X: %04X\n",
	 	cpu_get_pc(),shogwarr_mcu_command_offset*2,mcu_command);

	switch (mcu_command)
	{

		case 0x00ff:
		{
			int param1 = mcu_ram[shogwarr_mcu_command_offset + 1];
			int param2 = mcu_ram[shogwarr_mcu_command_offset + 2];
			int param3 = mcu_ram[shogwarr_mcu_command_offset + 3];
//			int param4 = mcu_ram[shogwarr_mcu_command_offset + 4];
			int param5 = mcu_ram[shogwarr_mcu_command_offset + 5];
//			int param6 = mcu_ram[shogwarr_mcu_command_offset + 6];
//			int param7 = mcu_ram[shogwarr_mcu_command_offset + 7];

			// clear old command (handshake to main cpu)
			mcu_ram[shogwarr_mcu_command_offset] = 0x0000;

			// execute the command:

			mcu_ram[param1 / 2] = ~readinputport(4);	// DSW
			mcu_ram[param2 / 2] = 0xffff;				// ? -1 / anything else

			shogwarr_mcu_command_offset = param3 / 2;	// where next command will be written?
			// param 4?
			mcu_ram[param5 / 2] = 0x8ee4;				// MCU Rom Checksum!
			// param 6&7 = address.l
/*

First code snippet provided by the MCU:

207FE0: 48E7 FFFE                movem.l D0-D7/A0-A6, -(A7)

207FE4: 3039 00A8 0000           move.w  $a80000.l, D0
207FEA: 4279 0020 FFFE           clr.w   $20fffe.l

207FF0: 41F9 0020 0000           lea     $200000.l, A0
207FF6: 7000                     moveq   #$0, D0

207FF8: 43E8 01C6                lea     ($1c6,A0), A1
207FFC: 7E02                     moveq   #$2, D7
207FFE: D059                     add.w   (A1)+, D0
208000: 51CF FFFC                dbra    D7, 207ffe

208004: 43E9 0002                lea     ($2,A1), A1
208008: 7E04                     moveq   #$4, D7
20800A: D059                     add.w   (A1)+, D0
20800C: 51CF FFFC                dbra    D7, 20800a

208010: 4640                     not.w   D0
208012: 5340                     subq.w  #1, D0
208014: 0068 0030 0216           ori.w   #$30, ($216,A0)

20801A: B07A 009A                cmp.w   ($9a,PC), D0; ($2080b6)
20801E: 670A                     beq     20802a

208020: 0268 000F 0216           andi.w  #$f, ($216,A0)
208026: 4268 0218                clr.w   ($218,A0)

20802A: 5468 0216                addq.w  #2, ($216,A0)
20802E: 42A8 030C                clr.l   ($30c,A0)
208032: 117C 0020 030C           move.b  #$20, ($30c,A0)

208038: 3E3C 0001                move.w  #$1, D7

20803C: 0C68 0008 0218           cmpi.w  #$8, ($218,A0)
208042: 6C00 0068                bge     2080ac

208046: 117C 0080 0310           move.b  #$80, ($310,A0)
20804C: 117C 0008 0311           move.b  #$8, ($311,A0)
208052: 317C 7800 0312           move.w  #$7800, ($312,A0)
208058: 5247                     addq.w  #1, D7
20805A: 0C68 0040 0216           cmpi.w  #$40, ($216,A0)
208060: 6D08                     blt     20806a

208062: 5468 0218                addq.w  #2, ($218,A0)
208066: 6000 0044                bra     2080ac

20806A: 117C 0041 0314           move.b  #$41, ($314,A0)

208070: 0C39 0001 0010 2E12      cmpi.b  #$1, $102e12.l
208078: 6606                     bne     208080

20807A: 117C 0040 0314           move.b  #$40, ($314,A0)

208080: 117C 000C 0315           move.b  #$c, ($315,A0)
208086: 317C 7000 0316           move.w  #$7000, ($316,A0)
20808C: 5247                     addq.w  #1, D7

20808E: 0839 0001 0010 2E15      btst    #$1, $102e15.l	; service mode
208096: 6714                     beq     2080ac

208098: 117C 0058 0318           move.b  #$58, ($318,A0)
20809E: 117C 0006 0319           move.b  #$6, ($319,A0)
2080A4: 317C 6800 031A           move.w  #$6800, ($31a,A0)
2080AA: 5247                     addq.w  #1, D7

2080AC: 3147 030A                move.w  D7, ($30a,A0)
2080B0: 4CDF 7FFF                movem.l (A7)+, D0-D7/A0-A6
2080B4: 4E73                     rte

2080B6: C747
*/
		}
		break;


		case 0x0001:
		{
//			int param1 = mcu_ram[shogwarr_mcu_command_offset + 1];
			int param2 = mcu_ram[shogwarr_mcu_command_offset + 2];

			// clear old command (handshake to main cpu)
			mcu_ram[shogwarr_mcu_command_offset] = 0x0000;

			// execute the command:

			// param1 ?
			mcu_ram[param2/2 + 0] = 0x0000;		// ?
			mcu_ram[param2/2 + 1] = 0x0000;		// ?
			mcu_ram[param2/2 + 2] = 0x0000;		// ?
			mcu_ram[param2/2 + 3] = 0x0000;		// ? addr.l
			mcu_ram[param2/2 + 4] = 0x00e0;		// 0000e0: 4e73 rte

		}
		break;


		case 0x0002:
		{
//			int param1 = mcu_ram[shogwarr_mcu_command_offset + 1];
//			int param2 = mcu_ram[shogwarr_mcu_command_offset + 2];
//			int param3 = mcu_ram[shogwarr_mcu_command_offset + 3];
//			int param4 = mcu_ram[shogwarr_mcu_command_offset + 4];
//			int param5 = mcu_ram[shogwarr_mcu_command_offset + 5];
//			int param6 = mcu_ram[shogwarr_mcu_command_offset + 6];
//			int param7 = mcu_ram[shogwarr_mcu_command_offset + 7];

			// clear old command (handshake to main cpu)
			mcu_ram[shogwarr_mcu_command_offset] = 0x0000;

			// execute the command:

		}
		break;

	}

}



WRITE16_HANDLER( shogwarr_mcu_ram_w )
{
	COMBINE_DATA(&mcu_ram[offset]);
	shogwarr_mcu_run();
}



#define SHOGWARR_MCU_COM_W(_n_) \
WRITE16_HANDLER( shogwarr_mcu_com##_n_##_w ) \
{ \
	shogwarr_mcu_status |= (1 << _n_); \
	shogwarr_mcu_run(); \
}

SHOGWARR_MCU_COM_W(0)
SHOGWARR_MCU_COM_W(1)
SHOGWARR_MCU_COM_W(2)
SHOGWARR_MCU_COM_W(3)


/***************************************************************************


						Misc Machine Emulation Routines


***************************************************************************/

READ16_HANDLER( kaneko16_rnd_r )
{
	return rand() & 0xffff;
}

WRITE16_HANDLER( kaneko16_coin_lockout_w )
{
	if (ACCESSING_MSB)
	{
		coin_counter_w(0,   data  & 0x0100);
		coin_counter_w(1,   data  & 0x0200);
		coin_lockout_w(0, (~data) & 0x0400 );
		coin_lockout_w(1, (~data) & 0x0800 );
	}
}

/* Sand Scorpion */

static UINT8 sprite_irq;
static UINT8 unknown_irq;
static UINT8 vblank_irq;


/* Update the IRQ state based on all possible causes */
static void update_irq_state(void)
{
	if (vblank_irq || sprite_irq || unknown_irq)
		cpu_set_irq_line(0, 1, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 1, CLEAR_LINE);
}


/* Called once/frame to generate the VBLANK interrupt */
static int sandscrp_interrupt(void)
{
	vblank_irq = 1;
	update_irq_state();
	return ignore_interrupt();
}


static void sandscrp_eof_callback(void)
{
	sprite_irq = 1;
	update_irq_state();
}

/* Reads the cause of the interrupt */
static READ16_HANDLER( sandscrp_irq_cause_r )
{
	return 	( sprite_irq  ?  0x08  : 0 ) |
			( unknown_irq ?  0x10  : 0 ) |
			( vblank_irq  ?  0x20  : 0 ) ;
}


/* Clear the cause of the interrupt */
static WRITE16_HANDLER( sandscrp_irq_cause_w )
{
	if (ACCESSING_LSB)
	{
		kaneko16_sprite_flipx	=	data & 1;
		kaneko16_sprite_flipy	=	data & 1;

		if (data & 0x08)	sprite_irq  = 0;
		if (data & 0x10)	unknown_irq = 0;
		if (data & 0x20)	vblank_irq  = 0;
	}

	update_irq_state();
}


/***************************************************************************


									Sound


***************************************************************************/

WRITE16_HANDLER( kaneko16_soundlatch_w )
{
	if (ACCESSING_MSB)
	{
		soundlatch_w(0, (data & 0xff00) >> 8 );
		cpu_set_nmi_line(1,PULSE_LINE);
	}
}

/* Two identically mapped YM2149 chips */

READ16_HANDLER( kaneko16_YM2149_0_r )
{
	/* Each 2149 register is mapped to a different address */
	AY8910_control_port_0_w(0,offset);
	return AY8910_read_port_0_r(0);
}
READ16_HANDLER( kaneko16_YM2149_1_r )
{
	/* Each 2149 register is mapped to a different address */
	AY8910_control_port_1_w(0,offset);
	return AY8910_read_port_1_r(0);
}

WRITE16_HANDLER( kaneko16_YM2149_0_w )
{
	/* Each 2149 register is mapped to a different address */
	AY8910_control_port_0_w(0,offset);
	/* The registers are mapped to odd addresses, except one! */
	if (ACCESSING_LSB)	AY8910_write_port_0_w(0, data       & 0xff);
	else				AY8910_write_port_0_w(0,(data >> 8) & 0xff);
}
WRITE16_HANDLER( kaneko16_YM2149_1_w )
{
	/* Each 2149 register is mapped to a different address */
	AY8910_control_port_1_w(0,offset);
	/* The registers are mapped to odd addresses, except one! */
	if (ACCESSING_LSB)	AY8910_write_port_1_w(0, data       & 0xff);
	else				AY8910_write_port_1_w(0,(data >> 8) & 0xff);
}


/***************************************************************************


									EEPROM


***************************************************************************/

READ_HANDLER( kaneko16_eeprom_r )
{
	return EEPROM_read_bit() & 1;
}

WRITE_HANDLER( kaneko16_eeprom_reset_w )
{
	// reset line asserted: reset.
	EEPROM_set_cs_line((data & 0x01) ? CLEAR_LINE : ASSERT_LINE );
}

WRITE16_HANDLER( kaneko16_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		// latch the bit
		EEPROM_write_bit(data & 0x02);

		// reset line asserted: reset.
//		EEPROM_set_cs_line((data & 0x00) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x01) ? ASSERT_LINE : CLEAR_LINE );
	}
}


/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/

/***************************************************************************
								The Berlin Wall
***************************************************************************/

static MEMORY_READ16_START( berlwall_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM					},	// ROM
	{ 0x200000, 0x20ffff, MRA16_RAM					},	// Work RAM
	{ 0x30e000, 0x30ffff, MRA16_RAM					},	// Sprites
	{ 0x400000, 0x400fff, MRA16_RAM					},	// Palette
//	{ 0x480000, 0x480001, MRA16_RAM					},	// ?
	{ 0x500000, 0x500001, kaneko16_bg15_reg_r		},	// High Color Background
	{ 0x580000, 0x580001, kaneko16_bg15_select_r	},
	{ 0x600000, 0x60003f, MRA16_RAM					},	// Sprites Regs
	{ 0x680000, 0x680001, input_port_0_word_r		},	// Inputs
	{ 0x680002, 0x680003, input_port_1_word_r		},
	{ 0x680004, 0x680005, input_port_2_word_r		},
//	{ 0x680006, 0x680007, input_port_3_word_r		},
	{ 0x780000, 0x780001, watchdog_reset16_r		},	// Watchdog
	{ 0x800000, 0x80001f, kaneko16_YM2149_0_r		},	// Sound
	{ 0x800200, 0x80021f, kaneko16_YM2149_1_r		},
	{ 0x800400, 0x800401, OKIM6295_status_0_lsb_r	},
	{ 0xc00000, 0xc03fff, MRA16_RAM					},	// Layers
	{ 0xd00000, 0xd0001f, MRA16_RAM					},	// Layers Regs
MEMORY_END

static MEMORY_WRITE16_START( berlwall_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM											},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM											},	// Work RAM
	{ 0x30e000, 0x30ffff, MWA16_RAM, &spriteram16, &spriteram_size			},	// Sprites
	{ 0x400000, 0x400fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16	},	// Palette
//	{ 0x480000, 0x480001, MWA16_RAM											},	// ?
	{ 0x500000, 0x500001, kaneko16_bg15_reg_w,     &kaneko16_bg15_reg		},	// High Color Background
	{ 0x580000, 0x580001, kaneko16_bg15_select_w,  &kaneko16_bg15_select	},
	{ 0x600000, 0x60003f, kaneko16_sprites_regs_w, &kaneko16_sprites_regs	},	// Sprites Regs
	{ 0x700000, 0x700001, kaneko16_coin_lockout_w							},	// Coin Lockout
	{ 0x800000, 0x80001f, kaneko16_YM2149_0_w								},	// Sound
	{ 0x800200, 0x80021f, kaneko16_YM2149_1_w								},
	{ 0x800400, 0x800401, OKIM6295_data_0_lsb_w								},
	{ 0xc00000, 0xc00fff, kaneko16_vram_1_w, &kaneko16_vram_1				},	// Layers
	{ 0xc01000, 0xc01fff, kaneko16_vram_0_w, &kaneko16_vram_0				},	//
	{ 0xc02000, 0xc03fff, MWA16_RAM											},	//
	{ 0xd00000, 0xd0001f, kaneko16_layers_0_regs_w, &kaneko16_layers_0_regs	},	// Layers Regs
MEMORY_END


/***************************************************************************
							Bakuretsu Breaker
***************************************************************************/

static MEMORY_READ16_START( bakubrkr_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	// ROM
	{ 0x100000, 0x10ffff, MRA16_RAM					},	// Work RAM
	{ 0x400000, 0x40001f, kaneko16_YM2149_0_r		},	// Sound
	{ 0x400200, 0x40021f, kaneko16_YM2149_1_r		},	//
	{ 0x400400, 0x400401, OKIM6295_status_0_lsb_r	},	//
	{ 0x500000, 0x503fff, MRA16_RAM					},	// Layers 1
	{ 0x580000, 0x583fff, MRA16_RAM					},	// Layers 0
	{ 0x600000, 0x601fff, MRA16_RAM					},	// Sprites
	{ 0x700000, 0x700fff, MRA16_RAM					},	// Palette
	{ 0x800000, 0x80001f, MRA16_RAM					},	// Layers 0 Regs
	{ 0x900000, 0x90001f, MRA16_RAM					},	// Sprites Regs
	{ 0xa80000, 0xa80001, watchdog_reset16_r		},	// Watchdog
	{ 0xb00000, 0xb0001f, MRA16_RAM					},	// Layers 1 Regs
	{ 0xe00000, 0xe00001, input_port_0_word_r		},	// Inputs
	{ 0xe00002, 0xe00003, input_port_1_word_r		},
	{ 0xe00004, 0xe00005, input_port_2_word_r		},
	{ 0xe00006, 0xe00007, input_port_3_word_r		},
MEMORY_END

static MEMORY_WRITE16_START( bakubrkr_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM											},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM											},	// Work RAM
	{ 0x400000, 0x40001f, kaneko16_YM2149_0_w								},	// Sound
	{ 0x400200, 0x40021f, kaneko16_YM2149_1_w								},	//
	{ 0x500000, 0x500fff, kaneko16_vram_1_w, &kaneko16_vram_1				},	// Layers 0
	{ 0x501000, 0x501fff, kaneko16_vram_0_w, &kaneko16_vram_0				},	//
	{ 0x502000, 0x503fff, MWA16_RAM											},	//
	{ 0x580000, 0x580fff, kaneko16_vram_3_w, &kaneko16_vram_3				},	// Layers 1
	{ 0x581000, 0x581fff, kaneko16_vram_2_w, &kaneko16_vram_2				},	//
	{ 0x582000, 0x583fff, MWA16_RAM											},	//
	{ 0x600000, 0x601fff, MWA16_RAM, &spriteram16, &spriteram_size			},	// Sprites
	{ 0x700000, 0x700fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x900000, 0x90001f, kaneko16_sprites_regs_w,  &kaneko16_sprites_regs	},	// Sprites Regs
	{ 0x800000, 0x80001f, kaneko16_layers_1_regs_w, &kaneko16_layers_1_regs	},	// Layers 0 Regs
	{ 0xb00000, 0xb0001f, kaneko16_layers_0_regs_w, &kaneko16_layers_0_regs	},	// Layers 1 Regs
	{ 0xd00000, 0xd00001, kaneko16_eeprom_w									},	// EEPROM
MEMORY_END


/***************************************************************************
									Blaze On
***************************************************************************/

static MEMORY_READ16_START( blazeon_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x300000, 0x30ffff, MRA16_RAM				},	// Work RAM
	{ 0x500000, 0x500fff, MRA16_RAM				},	// Palette
	{ 0x600000, 0x603fff, MRA16_RAM				},	// Layers 0
	{ 0x700000, 0x700fff, MRA16_RAM				},	// Sprites
/**/{ 0x800000, 0x80001f, MRA16_RAM				},	// Layers 0 Regs
/**/{ 0x900000, 0x90001f, MRA16_RAM				},	// Sprites Regs #1
/**/{ 0x980000, 0x98001f, MRA16_RAM				},	// Sprites Regs #2
	{ 0xc00000, 0xc00001, input_port_0_word_r	},	// Inputs
	{ 0xc00002, 0xc00003, input_port_1_word_r	},
	{ 0xc00004, 0xc00005, input_port_2_word_r	},
	{ 0xc00006, 0xc00007, input_port_3_word_r	},
	{ 0xe00000, 0xe00001, MRA16_NOP				},	// IRQ Ack ?
	{ 0xe40000, 0xe40001, MRA16_NOP				},	// IRQ Ack ?
//	{ 0xe80000, 0xe80001, MRA16_NOP				},	// IRQ Ack ?
	{ 0xec0000, 0xec0001, MRA16_NOP				},	// Lev 4 IRQ Ack ?
MEMORY_END

static MEMORY_WRITE16_START( blazeon_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM											},	// ROM
	{ 0x300000, 0x30ffff, MWA16_RAM											},	// Work RAM
	{ 0x500000, 0x500fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x600000, 0x600fff, kaneko16_vram_1_w, &kaneko16_vram_1				},	// Layers 0
	{ 0x601000, 0x601fff, kaneko16_vram_0_w, &kaneko16_vram_0				},	//
	{ 0x602000, 0x603fff, MWA16_RAM											},	//
	{ 0x700000, 0x700fff, MWA16_RAM, &spriteram16,	&spriteram_size			},	// Sprites
	{ 0x800000, 0x80001f, kaneko16_layers_0_regs_w,	&kaneko16_layers_0_regs	},	// Layers 1 Regs
	{ 0x900000, 0x90001f, kaneko16_sprites_regs_w,	&kaneko16_sprites_regs	},	// Sprites Regs #1
	{ 0x980000, 0x98001f, MWA16_RAM											},	// Sprites Regs #2
	{ 0xd00000, 0xd00001, kaneko16_coin_lockout_w							},	// Coin Lockout
	{ 0xe00000, 0xe00001, kaneko16_soundlatch_w								},
MEMORY_END


/***************************************************************************
							Great 1000 Miles Rally
***************************************************************************/


READ16_HANDLER( gtmr_wheel_r )
{
	if ( (readinputport(4) & 0x1800) == 0x10)	// DSW setting
		return	readinputport(5)<<8;			// 360° Wheel
	else
		return	readinputport(5);				// 270° Wheel
}

static int bank0;
WRITE16_HANDLER( gtmr_oki_0_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, 0x10000 * (data & 0xF) );
		bank0 = (data & 0xF);
//		logerror("CPU #0 PC %06X : OKI0 bank %08X\n",cpu_get_pc(),data);
	}
}

WRITE16_HANDLER( gtmr_oki_1_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(1, 0x40000 * (data & 0x1) );
//		logerror("CPU #0 PC %06X : OKI1 bank %08X\n",cpu_get_pc(),data);
	}
}

/*
	If you look at the samples ROM for the OKI chip #0, you'll see
	it's divided into 16 chunks, each chunk starting with the header
	holding the samples	addresses. But, except for chunk 0, the first
	$100 bytes ($20 samples) of each chunk are empty, and despite that,
	samples in the range $0-1f are played. So, whenever a samples in
	this range is requested, we use the address and sample from chunk 0,
	otherwise we use those from the selected bank. By using this scheme
	the sound improves, but I wouldn't bet it's correct..
*/

WRITE16_HANDLER( gtmr_oki_0_data_w )
{
	static int pend = 0;

	if (ACCESSING_LSB)
	{

		if (pend)	pend = 0;
		else
		{
			if (data & 0x80)
			{
				int samp = data &0x7f;

				pend = 1;
				if (samp < 0x20)
				{
					OKIM6295_set_bank_base(0, 0);
//					logerror("Setting OKI0 bank to zero\n");
				}
				else
					OKIM6295_set_bank_base(0, 0x10000 * bank0 );
			}
		}

		OKIM6295_data_0_w(0,data);
//		logerror("CPU #0 PC %06X : OKI0 <- %08X\n",cpu_get_pc(),data);

	}

}

WRITE16_HANDLER( gtmr_oki_1_data_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_data_1_w(0,data);
//		logerror("CPU #0 PC %06X : OKI1 <- %08X\n",cpu_get_pc(),data);
	}
}


static MEMORY_READ16_START( gtmr_readmem )
	{ 0x000000, 0x0ffffd, MRA16_ROM					},	// ROM
	{ 0x0ffffe, 0x0fffff, gtmr_wheel_r				},	// Wheel Value
	{ 0x100000, 0x10ffff, MRA16_RAM					},	// Work RAM
	{ 0x200000, 0x20ffff, MRA16_RAM					},	// Shared With MCU
	{ 0x300000, 0x30ffff, MRA16_RAM					},	// Palette
	{ 0x310000, 0x327fff, MRA16_RAM					},	//
	{ 0x400000, 0x401fff, MRA16_RAM					},	// Sprites
	{ 0x500000, 0x503fff, MRA16_RAM					},	// Layers 0
	{ 0x580000, 0x583fff, MRA16_RAM					},	// Layers 1
	{ 0x600000, 0x60000f, MRA16_RAM					},	// Layers 0 Regs
	{ 0x680000, 0x68000f, MRA16_RAM					},	// Layers 1 Regs
	{ 0x700000, 0x70001f, kaneko16_sprites_regs_r	},	// Sprites Regs
	{ 0x800000, 0x800001, OKIM6295_status_0_lsb_r	},	// Samples
	{ 0x880000, 0x880001, OKIM6295_status_1_lsb_r	},
	{ 0x900014, 0x900015, kaneko16_rnd_r			},	// Random Number ?
	{ 0xa00000, 0xa00001, watchdog_reset16_r		},	// Watchdog
	{ 0xb00000, 0xb00001, input_port_0_word_r		},	// Inputs
	{ 0xb00002, 0xb00003, input_port_1_word_r		},
	{ 0xb00004, 0xb00005, input_port_2_word_r		},
	{ 0xb00006, 0xb00007, input_port_3_word_r		},
	{ 0xd00000, 0xd00001, MRA16_NOP					},	// ? (bit 0)
MEMORY_END

static MEMORY_WRITE16_START( gtmr_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM					},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM					},	// Work RAM
	{ 0x200000, 0x20ffff, MWA16_RAM, &mcu_ram		},	// Shared With MCU
	{ 0x2a0000, 0x2a0001, gtmr_mcu_com0_w			},	// To MCU ?
	{ 0x2b0000, 0x2b0001, gtmr_mcu_com1_w			},
	{ 0x2c0000, 0x2c0001, gtmr_mcu_com2_w			},
	{ 0x2d0000, 0x2d0001, gtmr_mcu_com3_w			},
	{ 0x300000, 0x30ffff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x310000, 0x327fff, MWA16_RAM												},	//
	{ 0x400000, 0x401fff, MWA16_RAM, &spriteram16, &spriteram_size			},	// Sprites
	{ 0x500000, 0x500fff, kaneko16_vram_1_w, &kaneko16_vram_1				},	// Layers 0
	{ 0x501000, 0x501fff, kaneko16_vram_0_w, &kaneko16_vram_0				},	//
	{ 0x502000, 0x503fff, MWA16_RAM											},	//
	{ 0x580000, 0x580fff, kaneko16_vram_3_w, &kaneko16_vram_3				},	// Layers 1
	{ 0x581000, 0x581fff, kaneko16_vram_2_w, &kaneko16_vram_2				},	//
	{ 0x582000, 0x583fff, MWA16_RAM											},	//
	{ 0x600000, 0x60000f, kaneko16_layers_0_regs_w, &kaneko16_layers_0_regs	},	// Layers 0 Regs
	{ 0x680000, 0x68000f, kaneko16_layers_1_regs_w, &kaneko16_layers_1_regs	},	// Layers 1 Regs
	{ 0x700000, 0x70001f, kaneko16_sprites_regs_w, &kaneko16_sprites_regs	},	// Sprites Regs
	{ 0x800000, 0x800001, gtmr_oki_0_data_w			},	// Samples
	{ 0x880000, 0x880001, gtmr_oki_1_data_w			},
	{ 0xa00000, 0xa00001, watchdog_reset16_w		},	// Watchdog
	{ 0xb80000, 0xb80001, kaneko16_coin_lockout_w	},	// Coin Lockout
//	{ 0xc00000, 0xc00001, MWA16_NOP					},	// ?
	{ 0xe00000, 0xe00001, gtmr_oki_0_bank_w			},	// Samples Bankswitching
	{ 0xe80000, 0xe80001, gtmr_oki_1_bank_w			},
MEMORY_END


/***************************************************************************
								Magical Crystal
***************************************************************************/

static MEMORY_READ16_START( mgcrystl_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	// ROM
	{ 0x300000, 0x30ffff, MRA16_RAM					},	// Work RAM
	{ 0x400000, 0x40001f, kaneko16_YM2149_0_r		},	// Sound
	{ 0x400200, 0x40021f, kaneko16_YM2149_1_r		},
	{ 0x400400, 0x400401, OKIM6295_status_0_lsb_r	},
	{ 0x500000, 0x500fff, MRA16_RAM					},	// Palette
	{ 0x600000, 0x603fff, MRA16_RAM					},	// Layers 0
	{ 0x680000, 0x683fff, MRA16_RAM					},	// Layers 1
	{ 0x700000, 0x701fff, MRA16_RAM					},	// Sprites
	{ 0x800000, 0x80000f, MRA16_RAM					},	// Layers 0 Regs
	{ 0x900000, 0x90001f, MRA16_RAM					},	// Sprites Regs
	{ 0xb00000, 0xb0000f, MRA16_RAM					},	// Layers 1 Regs
	{ 0xa00000, 0xa00001, watchdog_reset16_r		},	// Watchdog
	{ 0xc00000, 0xc00001, input_port_0_word_r		},	// Inputs
	{ 0xc00002, 0xc00003, input_port_1_word_r		},	//
	{ 0xc00004, 0xc00005, input_port_2_word_r		},	//
MEMORY_END

static MEMORY_WRITE16_START( mgcrystl_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM											},	// ROM
	{ 0x300000, 0x30ffff, MWA16_RAM											},	// Work RAM
	{ 0x400000, 0x40001f, kaneko16_YM2149_0_w								},	// Sound
	{ 0x400200, 0x40021f, kaneko16_YM2149_1_w								},
	{ 0x400400, 0x400401, OKIM6295_data_0_lsb_w								},
	{ 0x500000, 0x500fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x600000, 0x600fff, kaneko16_vram_1_w, &kaneko16_vram_1				},	// Layers 0
	{ 0x601000, 0x601fff, kaneko16_vram_0_w, &kaneko16_vram_0				},	//
	{ 0x602000, 0x603fff, MWA16_RAM											},	//
	{ 0x680000, 0x680fff, kaneko16_vram_3_w, &kaneko16_vram_3				},	// Layers 1
	{ 0x681000, 0x681fff, kaneko16_vram_2_w, &kaneko16_vram_2				},	//
	{ 0x682000, 0x683fff, MWA16_RAM											},	//
	{ 0x700000, 0x701fff, MWA16_RAM, &spriteram16, &spriteram_size			},	// Sprites
	{ 0x800000, 0x80001f, kaneko16_layers_0_regs_w, &kaneko16_layers_0_regs	},	// Layers 0 Regs
	{ 0x900000, 0x90001f, kaneko16_sprites_regs_w,  &kaneko16_sprites_regs	},	// Sprites Regs
	{ 0xb00000, 0xb0001f, kaneko16_layers_1_regs_w, &kaneko16_layers_1_regs	},	// Layers 1 Regs
	{ 0xd00000, 0xd00001, kaneko16_eeprom_w									},	// EEPROM
MEMORY_END


/***************************************************************************
								Sand Scorpion
***************************************************************************/

WRITE16_HANDLER( sandscrp_coin_counter_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0,   data  & 0x0001);
		coin_counter_w(1,   data  & 0x0002);
	}
}

static data8_t latch1_full;
static data8_t latch2_full;

static READ16_HANDLER( sandscrp_latchstatus_word_r )
{
	return	(latch1_full ? 0x80 : 0) |
			(latch2_full ? 0x40 : 0) ;
}

static WRITE16_HANDLER( sandscrp_latchstatus_word_w )
{
	if (ACCESSING_LSB)
	{
		latch1_full = data & 0x80;
		latch2_full = data & 0x40;
	}
}

static READ16_HANDLER( sandscrp_soundlatch_word_r )
{
	latch2_full = 0;
	return soundlatch2_r(0);
}

static WRITE16_HANDLER( sandscrp_soundlatch_word_w )
{
	if (ACCESSING_LSB)
	{
		latch1_full = 1;
		soundlatch_w(0, data & 0xff);
		cpu_set_nmi_line(1,PULSE_LINE);
		cpu_spinuntil_time(TIME_IN_USEC(100));	// Allow the other cpu to reply
	}
}

static MEMORY_READ16_START( sandscrp_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM						},	// ROM
	{ 0x700000, 0x70ffff, MRA16_RAM						},	// RAM
	{ 0x200000, 0x20001f, sandscrp_mcu_ram_r			},	// Protection
	{ 0x300000, 0x30000f, MRA16_RAM						},	// Scroll
	{ 0x400000, 0x401fff, MRA16_RAM						},	// Layers
	{ 0x402000, 0x403fff, MRA16_RAM						},	//
	{ 0x500000, 0x501fff, MRA16_RAM						},	// Sprites
	{ 0x600000, 0x600fff, MRA16_RAM						},	// Palette
	{ 0xb00000, 0xb00001, input_port_0_word_r			},	// Inputs
	{ 0xb00002, 0xb00003, input_port_1_word_r			},	//
	{ 0xb00004, 0xb00005, input_port_2_word_r			},	//
	{ 0xb00006, 0xb00007, input_port_3_word_r			},	//
	{ 0xec0000, 0xec0001, watchdog_reset16_r			},	//
	{ 0x800000, 0x800001, sandscrp_irq_cause_r			},	// IRQ Cause
	{ 0xe00000, 0xe00001, sandscrp_soundlatch_word_r	},	// From Sound CPU
	{ 0xe40000, 0xe40001, sandscrp_latchstatus_word_r	},	//
MEMORY_END

static MEMORY_WRITE16_START( sandscrp_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM								},	// ROM
	{ 0x200000, 0x20001f, sandscrp_mcu_ram_w, &mcu_ram			},	// Protection
	{ 0x700000, 0x70ffff, MWA16_RAM								},	// RAM
	{ 0x300000, 0x30000f, kaneko16_layers_0_regs_w, &kaneko16_layers_0_regs	},	// Layers 0 Regs
	{ 0x100000, 0x100001, sandscrp_irq_cause_w					},	// IRQ Ack
	{ 0x400000, 0x400fff, kaneko16_vram_1_w, &kaneko16_vram_1	},	// Layers 0
	{ 0x401000, 0x401fff, kaneko16_vram_0_w, &kaneko16_vram_0	},	//
	{ 0x402000, 0x403fff, MWA16_RAM								},	//
	{ 0x500000, 0x501fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Sprites
	{ 0x600000, 0x600fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16	},	// Palette
	{ 0xa00000, 0xa00001, sandscrp_coin_counter_w				},	// Coin Counters (Lockout unused)
	{ 0xe00000, 0xe00001, sandscrp_soundlatch_word_w			},	// To Sound CPU
	{ 0xe40000, 0xe40001, sandscrp_latchstatus_word_w			},	//
MEMORY_END


/***************************************************************************
								Shogun Warriors
***************************************************************************/

/* Untested */
WRITE16_HANDLER( shogwarr_oki_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, 0x10000 * ((data >> 0) & 0x3) );
		OKIM6295_set_bank_base(1, 0x10000 * ((data >> 4) & 0x3) );
	}
}

static MEMORY_READ16_START( shogwarr_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM					},	// ROM
	{ 0x100000, 0x10ffff, MRA16_RAM					},	// Work RAM
	{ 0x200000, 0x20ffff, MRA16_RAM					},	// Shared With MCU
	{ 0x380000, 0x380fff, MRA16_RAM					},	// Palette
	{ 0x400000, 0x400001, OKIM6295_status_0_lsb_r	},	// Samples
	{ 0x480000, 0x480001, OKIM6295_status_1_lsb_r	},
	{ 0x580000, 0x581fff, MRA16_RAM					},	// Sprites
	{ 0x600000, 0x603fff, MRA16_RAM					},	// Layers 0
	{ 0x800000, 0x80000f, MRA16_RAM					},	// Layers 0 Regs
	{ 0x900000, 0x90001f, MRA16_RAM					},	// Sprites Regs
	{ 0xa00014, 0xa00015, kaneko16_rnd_r			},	// Random Number ?
	{ 0xa80000, 0xa80001, watchdog_reset16_r		},	// Watchdog
	{ 0xb80000, 0xb80001, input_port_0_word_r		},	// Inputs
	{ 0xb80002, 0xb80003, input_port_1_word_r		},
	{ 0xb80004, 0xb80005, input_port_2_word_r		},
	{ 0xb80006, 0xb80007, input_port_3_word_r		},
	{ 0xd00000, 0xd00001, MRA16_NOP					},	// ? (bit 0)
MEMORY_END

static MEMORY_WRITE16_START( shogwarr_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM								},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM								},	// Work RAM
	{ 0x200000, 0x20ffff, shogwarr_mcu_ram_w, &mcu_ram			},	// Shared With MCU
	{ 0x280000, 0x280001, shogwarr_mcu_com0_w					},	// To MCU ?
	{ 0x290000, 0x290001, shogwarr_mcu_com1_w					},
	{ 0x2b0000, 0x2b0001, shogwarr_mcu_com2_w					},
	{ 0x2d0000, 0x2d0001, shogwarr_mcu_com3_w					},
	{ 0x380000, 0x380fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x400000, 0x400001, OKIM6295_data_0_lsb_w					},	// Samples
	{ 0x480000, 0x480001, OKIM6295_data_1_lsb_w					},
	{ 0x580000, 0x581fff, MWA16_RAM, &spriteram16, &spriteram_size			},	// Sprites
	{ 0x600000, 0x600fff, kaneko16_vram_1_w, &kaneko16_vram_1				},	// Layers 0
	{ 0x601000, 0x601fff, kaneko16_vram_0_w, &kaneko16_vram_0				},	//
	{ 0x602000, 0x603fff, MWA16_RAM											},	//
	{ 0x800000, 0x80000f, kaneko16_layers_0_regs_w, &kaneko16_layers_0_regs	},	// Layers 1 Regs
	{ 0x900000, 0x90001f, kaneko16_sprites_regs_w, &kaneko16_sprites_regs		},	// Sprites Regs
	{ 0xa80000, 0xa80001, watchdog_reset16_w					},	// Watchdog
	{ 0xd00000, 0xd00001, MWA16_NOP								},	// ?
	{ 0xe00000, 0xe00001, shogwarr_oki_bank_w					},	// Samples Bankswitching
MEMORY_END



/***************************************************************************


							Memory Maps - Sound CPU


***************************************************************************/

/***************************************************************************
									Blaze On
***************************************************************************/

static WRITE_HANDLER( blazeon_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bank = data & 7;
	cpu_setbank(15, &RAM[bank * 0x10000 + 0x1000]);
}

static MEMORY_READ_START( blazeon_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM					},	// ROM
	{ 0xc000, 0xdfff, MRA_RAM					},	// RAM
MEMORY_END
static MEMORY_WRITE_START( blazeon_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM					},	// ROM
	{ 0xc000, 0xdfff, MWA_RAM					},	// RAM
MEMORY_END

static PORT_READ_START( blazeon_sound_readport )
	{ 0x03, 0x03, YM2151_status_port_0_r	},
	{ 0x06, 0x06, soundlatch_r				},
PORT_END
static PORT_WRITE_START( blazeon_sound_writeport )
	{ 0x02, 0x02, YM2151_register_port_0_w	},
	{ 0x03, 0x03, YM2151_data_port_0_w		},
PORT_END


/***************************************************************************
								Sand Scorpion
***************************************************************************/

WRITE_HANDLER( sandscrp_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bank = data & 0x07;

	if ( bank != data )	logerror("CPU #1 - PC %04X: Bank %02X\n",cpu_get_pc(),data);

	if (bank < 3)	RAM = &RAM[0x4000 * bank];
	else			RAM = &RAM[0x4000 * (bank-3) + 0x10000];

	cpu_setbank(1, RAM);
}

static READ_HANDLER( sandscrp_latchstatus_r )
{
	return	(latch2_full ? 0x80 : 0) |	// swapped!?
			(latch1_full ? 0x40 : 0) ;
}

static READ_HANDLER( sandscrp_soundlatch_r )
{
	latch1_full = 0;
	return soundlatch_r(0);
}

static WRITE_HANDLER( sandscrp_soundlatch_w )
{
	latch2_full = 1;
	soundlatch2_w(0,data);
}

static MEMORY_READ_START( sandscrp_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM					},	// ROM
	{ 0x8000, 0xbfff, MRA_BANK1					},	// Banked ROM
	{ 0xc000, 0xdfff, MRA_RAM					},	// RAM
MEMORY_END
static MEMORY_WRITE_START( sandscrp_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM					},	// ROM
	{ 0x8000, 0xbfff, MWA_ROM					},	// Banked ROM
	{ 0xc000, 0xdfff, MWA_RAM					},	// RAM
MEMORY_END

static PORT_READ_START( sandscrp_sound_readport )
	{ 0x02, 0x02, YM2203_status_port_0_r		},	// YM2203
	{ 0x03, 0x03, YM2203_read_port_0_r			},	// PORTA/B read
	{ 0x07, 0x07, sandscrp_soundlatch_r			},	//
	{ 0x08, 0x08, sandscrp_latchstatus_r		},	//
PORT_END
static PORT_WRITE_START( sandscrp_sound_writeport )
	{ 0x00, 0x00, sandscrp_bankswitch_w		},	// ROM Bank
	{ 0x02, 0x02, YM2203_control_port_0_w	},	// YM2203
	{ 0x03, 0x03, YM2203_write_port_0_w		},	//
	{ 0x04, 0x04, OKIM6295_data_0_w			},	// OKIM6295
	{ 0x06, 0x06, sandscrp_soundlatch_w		},	//
PORT_END




/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
							Bakuretsu Breaker
***************************************************************************/

INPUT_PORTS_START( bakubrkr )
	PORT_START	// IN0 - Player 1 + DSW - e00000.w
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - e00002.b
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins - e00004.b
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT_IMPULSE( 0x0400, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x0800, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)	// pause
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START	// IN3 - Seems unused ! - e00006.b
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
							The Berlin Wall (set 1)
***************************************************************************/

INPUT_PORTS_START( berlwall )
	PORT_START	// IN0 - Player 1 - 680000.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - 680002.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins - 680004.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT_IMPULSE( 0x0400, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x0800, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START	// IN3 - ? - 680006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW 1 - $200018.b <- ! $80001d.b
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Reserved" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START	// IN5 - DSW 2 - $200019.b <- $80001f.b
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy"    )
	PORT_DIPSETTING(    0x03, "Normal"  )
	PORT_DIPSETTING(    0x01, "Hard"    )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )	// 1p lives at 202982.b
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Country"   )
	PORT_DIPSETTING(    0x30, "England" )
	PORT_DIPSETTING(    0x20, "Italy"   )
	PORT_DIPSETTING(    0x10, "Germany" )
	PORT_DIPSETTING(    0x00, "Freeze Screen" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


/***************************************************************************
							The Berlin Wall (set 2)
***************************************************************************/

//	Same as berlwall, but for a different lives setting

INPUT_PORTS_START( berlwalt )
	PORT_START	// IN0 - Player 1 - 680000.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - 680002.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins - 680004.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT_IMPULSE( 0x0400, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x0800, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START	// IN3 - ? - 680006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW 1 - $80001d.b
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Reserved" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START	// IN5 - DSW 2 - $80001f.b
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy"    )
	PORT_DIPSETTING(    0x03, "Normal"  )
	PORT_DIPSETTING(    0x01, "Hard"    )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPNAME( 0x30, 0x30, "Country"   )
	PORT_DIPSETTING(    0x30, "England" )
	PORT_DIPSETTING(    0x20, "Italy"   )
	PORT_DIPSETTING(    0x10, "Germany" )
	PORT_DIPSETTING(    0x00, "Freeze Screen" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


/***************************************************************************
									Blaze On
***************************************************************************/

INPUT_PORTS_START( blazeon )
	PORT_START	// IN0 - Player 1 + DSW - c00000.w
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy"    )
	PORT_DIPSETTING(      0x0003, "Normal"  )
	PORT_DIPSETTING(      0x0001, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0008, "4" )
	PORT_DIPSETTING(      0x0004, "5" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT_IMPULSE( 0x8000, IP_ACTIVE_LOW, IPT_COIN1, 2 )

	PORT_START	// IN1 - Player 2 - c00002.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0005, "6 Coins/3 Credits" )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, "5 Coins/6 Credits" )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_3C ) )
//	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )

	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0050, "6 Coins/3 Credits" )
	PORT_DIPSETTING(      0x0090, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
//	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, "5 Coins/6 Credits" )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_3C ) )
//	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT_IMPULSE( 0x8000, IP_ACTIVE_LOW, IPT_COIN2, 2 )

	PORT_START	// IN2 - ? - c00004.w
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )	// unused?

	PORT_START	// IN3 - Other Buttons - c00006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX( 0x2000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_TILT  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_SERVICE1 )
INPUT_PORTS_END


/***************************************************************************
							Great 1000 Miles Rally
***************************************************************************/

INPUT_PORTS_START( gtmr )
	PORT_START	// IN0 - Player 1 - b00000.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) // swapped for consistency:
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) // button1 is usually accel.
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - b00002.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) // swapped for consistency:
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) // button1 is usually accel.
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins - b00004.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT_IMPULSE( 0x0400, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x0800, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START	// IN3 - Seems unused ! - b00006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW from the MCU - 101265.b <- 206000.b
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Cabinet )  )
	PORT_DIPSETTING(      0x0400, DEF_STR( Upright )  )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x1800, 0x1800, "Controls"    )
	PORT_DIPSETTING(      0x1800, "1 Joystick"  )
	PORT_DIPSETTING(      0x0800, "2 Joysticks" )
	PORT_DIPSETTING(      0x1000, "Wheel (360)" )
	PORT_DIPSETTING(      0x0000, "Wheel (270)" )
	PORT_DIPNAME( 0x2000, 0x2000, "Use Brake"    )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On )  )
	PORT_DIPNAME( 0xc000, 0xc000, "National Anthem & Flag" )
	PORT_DIPSETTING(      0xc000, "Use Memory"  )
	PORT_DIPSETTING(      0x8000, "Anthem Only" )
	PORT_DIPSETTING(      0x4000, "Flag Only"   )
	PORT_DIPSETTING(      0x0000, "None"        )

	PORT_START	// IN5 - Wheel - 100015.b <- ffffe.b
	PORT_ANALOG ( 0x00ff, 0x0080, IPT_AD_STICK_X | IPF_CENTER, 30, 1, 0x00, 0xff )
INPUT_PORTS_END


/***************************************************************************
								Magical Crystal
***************************************************************************/

INPUT_PORTS_START( mgcrystl )
	PORT_START	// IN0 - Player 1 + DSW - c00000.w
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	// TESTED!
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - c00002.b
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Other Buttons - c00004.b
	PORT_BIT ( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT ( 0x0100, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT ( 0x0200, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT_IMPULSE( 0x0400, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x0800, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT ( 0x2000, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT ( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
								Sand Scorpion
***************************************************************************/

INPUT_PORTS_START( sandscrp )
	PORT_START	// IN0 - $b00000.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	| IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $b00002.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	| IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $b00004.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $b00006.w
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW 1 read by the Z80 through the sound chip
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bombs" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, "Easy"    )
	PORT_DIPSETTING(    0x20, "Normal"  )
	PORT_DIPSETTING(    0x10, "Hard"    )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x80, "100K, 300K" )
	PORT_DIPSETTING(    0xc0, "200K, 500K" )
	PORT_DIPSETTING(    0x40, "500K, 1000K" )
	PORT_DIPSETTING(    0x00, "1000K, 3000K" )

	PORT_START	// IN5 - DSW 2 read by the Z80 through the sound chip
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


/***************************************************************************
								Shogun Warriors
***************************************************************************/

INPUT_PORTS_START( shogwarr )
	PORT_START	// IN0 - - b80000.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	// ? tested

	PORT_START	// IN1 - - b80002.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	// ? tested

	PORT_START	// IN2 - Coins - b80004.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT_IMPULSE( 0x0400, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x0800, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)	// ? tested

	PORT_START	// IN3 - ? - b80006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW from the MCU - 102e15.b <- 200059.b
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x38, "1" )	// easy
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0x40, 0x40, "Can Join During Game" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )	//	2 credits		winner vs computer
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )	//	1 credit		game over
	PORT_DIPNAME( 0x80, 0x80, "Special Continue Mode" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



/***************************************************************************


								Graphics Layouts


***************************************************************************/


/*
	16x16x4 made of 4 8x8x4 blocks arrenged like:		 	01
 	(nibbles are swapped for tiles, not for sprites)		23
*/
static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP8(8*8*4*0,4),   STEP8(8*8*4*1,4)   },
	{ STEP8(8*8*4*0,8*4), STEP8(8*8*4*2,8*4) },
	16*16*4
};

/*
	16x16x8 made of 4 8x8x8 blocks arrenged like:	01
													23
*/
static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8),   STEP8(8*8*8*1,8)   },
	{ STEP8(0,8*8), STEP8(8*8*8*2,8*8) },
	16*16*8
};

static struct GfxDecodeInfo kaneko16_gfx_1x4bit_1x4bit[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0,			0x40 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4, 0x40 * 16,	0x40 }, // [1] Layers
	{ -1 }
};
static struct GfxDecodeInfo kaneko16_gfx_1x4bit_2x4bit[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0,			0x40 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4, 0x40 * 16,	0x40 }, // [1] Layers
	{ REGION_GFX3, 0, &layout_16x16x4, 0x40 * 16,	0x40 }, // [2] Layers
	{ -1 }
};
static struct GfxDecodeInfo kaneko16_gfx_1x8bit_2x4bit[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8,	0x40 * 256,	0x40 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4,	0,			0x40 }, // [1] Layers
	{ REGION_GFX3, 0, &layout_16x16x4,	0,			0x40 }, // [2] Layers
	{ -1 }
};

/* 16x16x4 tiles (made of four 8x8 tiles) */
static struct GfxLayout layout_16x16x4_2 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP4(8*8*4*0 + 3*4, -4), STEP4(8*8*4*0 + 7*4, -4),
	  STEP4(8*8*4*1 + 3*4, -4), STEP4(8*8*4*1 + 7*4, -4) },
	{ STEP8(8*8*4*0, 8*4),     STEP8(8*8*4*2, 8*4) },
	16*16*4
};
static struct GfxDecodeInfo sandscrp_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4,   0x000, 0x10 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4_2, 0x400, 0x40 }, // [1] Layers
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

#define KANEKO16_INTERRUPTS_NUM	3
int kaneko16_interrupt(void)
{
	switch ( cpu_getiloops() )
	{
		case 2:  return 3;
		case 1:  return 4;
		case 0:  return 5;
		default: return 0;
	}
}

static struct OKIM6295interface okim6295_intf_8kHz =
{
	1,
	{ 8000 },
	{ REGION_SOUND1 },
	{ 100 }
};
static struct OKIM6295interface okim6295_intf_12kHz =
{
	1,
	{ 12000 },
	{ REGION_SOUND1 },
	{ 100 }
};
static struct OKIM6295interface okim6295_intf_2x12kHz =
{
	2,
	{ 12000, 12000 },
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 100, 100 }
};

static struct AY8910interface ay8910_intf_2x1MHz_DSW =
{
	2,
	1000000,	/* ? */
	{ MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) },
	{ input_port_4_r, 0 },	/* input A: DSW 1 */
	{ input_port_5_r, 0 },	/* input B: DSW 2 */
	{ 0, 0 },
	{ 0, 0 }
};
static struct AY8910interface ay8910_intf_2x2MHz_EEPROM =
{
	2,
	2000000,	/* ? */
	{ MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) },
	{ 0, kaneko16_eeprom_r },		/* inputs  A:  0,EEPROM bit read */
	{ 0, 0 },						/* inputs  B */
	{ 0, 0 },						/* outputs A */
	{ 0, kaneko16_eeprom_reset_w }	/* outputs B:  0,EEPROM reset */
};

static struct YM2151interface ym2151_intf_blazeon =
{
	1,
	4000000,			/* ? */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 },				/* irq handler */
	{ 0 }				/* port_write */
};


/***************************************************************************
								The Berlin Wall
***************************************************************************/

/*
	Berlwall interrupts:

	1-3]	e8c:
	4]		e54:
	5]		de4:
	6-7]	rte
*/

static const struct MachineDriver machine_driver_berlwall =
{
	{
		{
			CPU_M68000,
			12000000,	/* MC68000P12 */
			berlwall_readmem,berlwall_writemem,0,0,
			kaneko16_interrupt, KANEKO16_INTERRUPTS_NUM
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	berlwall_init_machine,

	/* video hardware */
	256, 256, { 0, 256-1, 16, 240-1},
	kaneko16_gfx_1x4bit_1x4bit,
	2048 + 32768, 0,	/* 32768 static colors for the bg */
	berlwall_init_palette,

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK,	// mangled sprites otherwise
	0,
	berlwall_vh_start,
	berlwall_vh_stop,
	kaneko16_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_AY8910,	&ay8910_intf_2x1MHz_DSW	},
		{	SOUND_OKIM6295,	&okim6295_intf_8kHz		}
	}
};


/***************************************************************************
							Bakuretsu Breaker
***************************************************************************/

static struct MachineDriver machine_driver_bakubrkr =
{
	{
		{
			CPU_M68000,
			16000000,	/* ? */
			bakubrkr_readmem,bakubrkr_writemem,0,0,
			kaneko16_interrupt, KANEKO16_INTERRUPTS_NUM
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	kaneko16_init_machine,

	/* video hardware */
	256, 256, { 0, 256-1, 16, 240-1},
	kaneko16_gfx_1x4bit_2x4bit,
	2048, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK,	// mangled sprites otherwise
	0,
	kaneko16_vh_start_2xVIEW2,
	kaneko16_vh_stop,
	kaneko16_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{	SOUND_AY8910,	&ay8910_intf_2x2MHz_EEPROM		},
		{	SOUND_OKIM6295,	&okim6295_intf_8kHz				}
	},

	nvram_handler_93C46
};


/***************************************************************************
									Blaze On
***************************************************************************/

/*
	Blaze On:
		1]		busy loop
		2]		does nothing
		3]		rte
		4]		drives the game
		5]		== 2
		6-7]	busy loop
*/

static struct MachineDriver machine_driver_blazeon =
{
	{
		{
			CPU_M68000,	/* TMP68HC000-12 */
			12000000,
			blazeon_readmem,blazeon_writemem,0,0,
			kaneko16_interrupt, KANEKO16_INTERRUPTS_NUM
		},
		{
			CPU_Z80,	/* D780C-2 */
			4000000,	/* ? */
			blazeon_sound_readmem, blazeon_sound_writemem,
			blazeon_sound_readport,blazeon_sound_writeport,
			ignore_interrupt, 1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	blazeon_init_machine,

	/* video hardware */
	320, 240, { 0, 320-1, 0, 240-1 -8},
	kaneko16_gfx_1x4bit_1x4bit,
	2048, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK,
	0,
	kaneko16_vh_start_1xVIEW2,
	kaneko16_vh_stop,
	kaneko16_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM2151,	&ym2151_intf_blazeon	}
	}
};


/***************************************************************************
							Great 1000 Miles Rally
***************************************************************************/

/*
	gtmr interrupts:

	3] 476:			time, input ports, scroll registers
	4] 466->258e:	set sprite ram
	5] 438:			set sprite colors

	VIDEO_UPDATE_AFTER_VBLANK fixes the mangled/wrong colored sprites
*/

static const struct MachineDriver machine_driver_gtmr =
{
	{
		{
			CPU_M68000,
			16000000,	/* ? Most likely a 68000-HC16 */
			gtmr_readmem,gtmr_writemem,0,0,
			kaneko16_interrupt, KANEKO16_INTERRUPTS_NUM
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	gtmr_init_machine,

	/* video hardware */
	320, 240, { 0, 320-1, 0, 240-1 },
	kaneko16_gfx_1x8bit_2x4bit,
	32768, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK,
	0,
	kaneko16_vh_start_2xVIEW2,
	kaneko16_vh_stop,
	kaneko16_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{	SOUND_OKIM6295,	&okim6295_intf_2x12kHz	}
	}
};


/***************************************************************************
								Magical Crystal
***************************************************************************/

static const struct MachineDriver machine_driver_mgcrystl =
{
	{
		{
			CPU_M68000,
			12000000,
			mgcrystl_readmem,mgcrystl_writemem,0,0,
			kaneko16_interrupt, KANEKO16_INTERRUPTS_NUM
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	mgcrystl_init_machine,

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 256-16-1},
	kaneko16_gfx_1x4bit_2x4bit,
	2048, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK,
	0,
	kaneko16_vh_start_2xVIEW2,
	kaneko16_vh_stop,
	kaneko16_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{	SOUND_AY8910,	&ay8910_intf_2x2MHz_EEPROM	},
		{	SOUND_OKIM6295,	&okim6295_intf_8kHz			}
	},

	nvram_handler_93C46
};


/***************************************************************************
								Sand Scorpion
***************************************************************************/

/* YM3014B + YM2203C */

static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_intf_sandscrp =
{
	1,
	4000000,	/* ? */
	{ YM2203_VOL(100,100) },
	{ input_port_4_r },	/* Port A Read - DSW 1 */
	{ input_port_5_r },	/* Port B Read - DSW 2 */
	{ 0 },	/* Port A Write */
	{ 0 },	/* Port B Write */
	{ irq_handler },	/* IRQ handler */
};


static const struct MachineDriver machine_driver_sandscrp =
{
	{
		{
			CPU_M68000,	/* TMP68HC000N-12 */
			12000000,
			sandscrp_readmem, sandscrp_writemem,0,0,
			sandscrp_interrupt, 1
		},
		{
			CPU_Z80,	/* Z8400AB1, Reads the DSWs: it can't be disabled */
			4000000,
			sandscrp_sound_readmem,  sandscrp_sound_writemem,
			sandscrp_sound_readport, sandscrp_sound_writeport,
			ignore_interrupt, 1	/* IRQ by YM2203, NMI by Main CPU */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	// eof callback
	1,
	sandscrp_init_machine,

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 256-16-1 },
	sandscrp_gfxdecodeinfo,
	2048, 0,
	0,

	VIDEO_TYPE_RASTER,
	sandscrp_eof_callback,
	sandscrp_vh_start_1xVIEW2,
	kaneko16_vh_stop,
	kaneko16_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_OKIM6295,	&okim6295_intf_12kHz	},
		{	SOUND_YM2203,	&ym2203_intf_sandscrp	},
	}
};


/***************************************************************************
								Shogun Warriors
***************************************************************************/

/*
	shogwarr interrupts:

	2] 100:	rte
	3] 102:
	4] 136:
		movem.l D0-D7/A0-A6, -(A7)
		movea.l $207808.l, A0	; from mcu?
		jmp     ($4,A0)

	other: busy loop
*/
#define SHOGWARR_INTERRUPTS_NUM	3
int shogwarr_interrupt(void)
{
	switch ( cpu_getiloops() )
	{
		case 2:  return 2;
		case 1:  return 3;
//		case 0:  return 4;
		default: return 0;
	}
}

static const struct MachineDriver machine_driver_shogwarr =
{
	{
		{
			CPU_M68000,
			12000000,
			shogwarr_readmem,shogwarr_writemem,0,0,
			shogwarr_interrupt, SHOGWARR_INTERRUPTS_NUM
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	shogwarr_init_machine,

	/* video hardware */
	320, 240, { 0, 320-1, 0, 240-1 },
	kaneko16_gfx_1x4bit_1x4bit,
	2048, 0,
	0,

	VIDEO_TYPE_RASTER,
	0,
	kaneko16_vh_start_1xVIEW2,
	kaneko16_vh_stop,
	kaneko16_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{	SOUND_OKIM6295,	&okim6295_intf_2x12kHz	}
	}
};


/***************************************************************************


								ROMs Loading


***************************************************************************/

/*
 Sprites and tiles are stored in the ROMs using the same layout. But tiles
 have the even and odd pixels swapped. So we use this function to untangle
 them and have one single gfxlayout for both tiles and sprites.
*/
void kaneko16_unscramble_tiles(int region)
{
	unsigned char *RAM	=	memory_region(region);
	int size			=	memory_region_length(region);
	int i;

	if (RAM == NULL)	return;

	for (i = 0; i < size; i ++)
	{
		RAM[i] = ((RAM[i] & 0xF0)>>4) + ((RAM[i] & 0x0F)<<4);
	}
}

void init_kaneko16(void)
{
	kaneko16_unscramble_tiles(REGION_GFX2);
	kaneko16_unscramble_tiles(REGION_GFX3);
}

void init_berlwall(void)
{
	kaneko16_unscramble_tiles(REGION_GFX2);
}


/***************************************************************************

								Bakuretsu Breaker

	USES TOSHIBA 68000 CPU W/TWO YM2149 SOUND

	LOCATION    TYPE
	------------------
	U38         27C040
	U37         "
	U36         27C020
	U19         "
	U18         "

***************************************************************************/

ROM_START( bakubrkr )
 	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "u18", 0x000000, 0x040000, 0x8cc0a4fd )
	ROM_LOAD16_BYTE( "u19", 0x000001, 0x040000, 0xaea92195 )

	ROM_REGION( 0x240000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "u37",  0x000000, 0x080000, 0x70b66e7e )
	ROM_RELOAD(       0x100000, 0x080000             )
	ROM_LOAD( "u38",  0x080000, 0x080000, 0xa7a94143 )
	ROM_RELOAD(       0x180000, 0x080000             )
	ROM_LOAD( "u36",  0x200000, 0x040000, 0x611271e6 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles */
	ROM_LOAD( "layer1", 0x000000, 0x040000, 0x00000000 )
	ROM_FILL(           0x000000, 0x040000, 0x00 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles */
	ROM_LOAD( "layer2", 0x000000, 0x040000, 0x00000000 )
	ROM_FILL(           0x000000, 0x040000, 0x00 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "samples", 0x000000, 0x040000, 0x00000000 )
ROM_END


/***************************************************************************

								The Berlin Wall

The Berlin Wall, Kaneko 1991, BW-002

----

BW-004 BW-008                    VU-003
BW-005 BW-009                    VU-003
BW-006 BW-00A                    VU-003
BW-007 BW-00B                          6116-90
                                       6116-90
BW-003                           52256  52256
                                 BW101A BW100A
5864
5864                   MUX2      68000
            VIEW2
BW300
BW-002
BW-001                      42101
                            42101
41464 41464      VU-002
41464 41464                      YM2149  IU-004
41464 41464                      YM2149
                           SWB             BW-000  6295
                           SWA


PALs : BW-U47, BW-U48 (backgrounds encryption)

***************************************************************************/

ROM_START( berlwall )
 	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "bw100a", 0x000000, 0x020000, 0xe6bcb4eb )
	ROM_LOAD16_BYTE( "bw101a", 0x000001, 0x020000, 0x38056fb2 )

	ROM_REGION( 0x120000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bw001",  0x000000, 0x080000, 0xbc927260 )
	ROM_LOAD( "bw002",  0x080000, 0x080000, 0x223f5465 )
	ROM_LOAD( "bw300",  0x100000, 0x020000, 0xb258737a )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "bw003",  0x000000, 0x080000, 0xfbb4b72d )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )	/* High Color Background */
	ROM_LOAD16_BYTE( "bw004",  0x000000, 0x080000, 0x5300c34d )
	ROM_LOAD16_BYTE( "bw008",  0x000001, 0x080000, 0x9aaf2f2f ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw005",  0x100000, 0x080000, 0x16db6d43 )
	ROM_LOAD16_BYTE( "bw009",  0x100001, 0x080000, 0x1151a0b0 ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw006",  0x200000, 0x080000, 0x73a35d1f )
	ROM_LOAD16_BYTE( "bw00a",  0x200001, 0x080000, 0xf447dfc2 ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw007",  0x300000, 0x080000, 0x97f85c87 )
	ROM_LOAD16_BYTE( "bw00b",  0x300001, 0x080000, 0xb0a48225 ) // FIXED BITS (xxxxxxx0)

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "bw000",  0x000000, 0x040000, 0xd8fe869d )
ROM_END

ROM_START( berlwalt )
 	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "u23_01.bin", 0x000000, 0x020000, 0x76b526ce )
	ROM_LOAD16_BYTE( "u39_01.bin", 0x000001, 0x020000, 0x78fa7ef2 )

	ROM_REGION( 0x120000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bw001",  0x000000, 0x080000, 0xbc927260 )
	ROM_LOAD( "bw002",  0x080000, 0x080000, 0x223f5465 )
	ROM_LOAD( "bw300",  0x100000, 0x020000, 0xb258737a )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "bw003",  0x000000, 0x080000, 0xfbb4b72d )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )	/* High Color Background */
	ROM_LOAD16_BYTE( "bw004",  0x000000, 0x080000, 0x5300c34d )
	ROM_LOAD16_BYTE( "bw008",  0x000001, 0x080000, 0x9aaf2f2f ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw005",  0x100000, 0x080000, 0x16db6d43 )
	ROM_LOAD16_BYTE( "bw009",  0x100001, 0x080000, 0x1151a0b0 ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw006",  0x200000, 0x080000, 0x73a35d1f )
	ROM_LOAD16_BYTE( "bw00a",  0x200001, 0x080000, 0xf447dfc2 ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw007",  0x300000, 0x080000, 0x97f85c87 )
	ROM_LOAD16_BYTE( "bw00b",  0x300001, 0x080000, 0xb0a48225 ) // FIXED BITS (xxxxxxx0)

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "bw000",  0x000000, 0x040000, 0xd8fe869d )
ROM_END


/***************************************************************************

							Blaze On (Japan version)

CPU:          TMP68HC000-12/D780C-2(Z80)
SOUND:        YM2151
OSC:          13.3330/16.000MHz
CUSTOM:       KANEKO VU-002 x2
              KANEKO 23160-509 VIEW2-CHIP
              KANEKO MUX2-CHIP
              KANEKO HELP1-CHIP

---------------------------------------------------
 filemanes          devices       kind
---------------------------------------------------
 BZ_PRG1.U80        27C020        68000 main prg.
 BZ_PRG2.U81        27C020        68000 main prg.
 3.U45              27C010        Z80 sound prg.
 BZ_BG.U2           57C8200       BG CHR
 BZ_SP1.U20         27C8001       OBJ
 BZ_SP2.U21         27C8001       OBJ
 BZ_SP1.U68 ( == BZ_SP1.U20)
 BZ_SP2.U86 ( == BZ_SP2.U21)

***************************************************************************/

ROM_START( blazeon )
 	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "bz_prg1.u80", 0x000000, 0x040000, 0x8409e31d )
	ROM_LOAD16_BYTE( "bz_prg2.u81", 0x000001, 0x040000, 0xb8a0a08b )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* Z80 Code */
	ROM_LOAD( "3.u45", 0x000000, 0x020000, 0x52fe4c94 )	// 1xxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bz_sp1.u20", 0x000000, 0x100000, 0x0d5809a1 )
	ROM_LOAD( "bz_sp2.u21", 0x100000, 0x100000, 0x56ead2bd )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "bz_bg.u2", 0x000000, 0x100000, 0xfc67f19f )
ROM_END


/***************************************************************************

							Great 1000 Miles Rally

GMMU2+1	512K * 2	68k
GMMU23	1M		OKI6295: 00000-2ffff + chunks of 0x10000 with headers
GMMU24	1M		OKI6295: chunks of 0x40000 with headers - FIRST AND SECOND HALF IDENTICAL

GMMU27	2M		sprites
GMMU28	2M		sprites
GMMU29	2M		sprites
GMMU30	512k	sprites

GMMU64	1M		sprites - FIRST AND SECOND HALF IDENTICAL
GMMU65	1M		sprites - FIRST AND SECOND HALF IDENTICAL

GMMU52	2M		tiles


---------------------------------------------------------------------------
								Game code
---------------------------------------------------------------------------

100000.b	<- (!b00000.b) & 7f	[1p]
    01.b	previous value of the above
    02.b	bits gone high

100008.b	<- (!b00002.b) & 7f	[2p]

100010.b	<- !b00004.b [coins]
    11.b	previous value of the above
    12.b	bits gone high

100013.b	<- b00006.b	(both never accessed again?)

100015.b	<- wheel value

600000.w	<- 100a20.w + 100a30.w		600002.w	<- 100a22.w + 100a32.w
600004.w	<- 100a24.w + 100a34.w		600006.w	<- 100a26.w + 100a36.w

680000.w	<- 100a28.w + 100a38.w		680002.w	<- 100a2a.w + 100a3a.w
680004.w	<- 100a2c.w + 100a3c.w		680006.w	<- 100a2e.w + 100a3e.w

101265.b	<- DSW (from 206000)
101266		<- Settings from NVRAM (0x80 bytes from 208000)

1034f8.b	credits
103502.b	coins x ..
103503.b	.. credits

1035ec.l	*** Time (BCD: seconds * 10000) ***
103e64.w	*** Speed << 4 ***

10421a.b	bank for the oki mapped at 800000
104216.b	last value of the above

10421c.b	bank for the oki mapped at 880000
104218.b	last value of the above

ROUTINES:

dd6	print string: a2->scr ; a1->string ; d1.l = xpos.w<<6|ypos.w<<6

Trap #2 = 43a0 ; d0.w = routine index ; (where not specified: 43c0):
1:  43C4	2:  43F8	3:  448E	4:  44EE
5:  44D2	6:  4508	7:  453A	10: 0AF6
18: 4580	19: 4604
20> 2128	writes 700000-70001f
21: 21F6
24> 2346	clears 400000-401407 (641*8 = $281*8)
30> 282A	writes 600008/9/b/e-f, 680008/9/b/e-f
31: 295A
32> 2B36	100a30-f <- 100a10-f
34> 2B4C	clears 500000-503fff, 580000-583fff
35> 2B9E	d1.w = selects between:	500000;501000;580000;581000.
			Fill 0x1000 bytes from there with d2.l

70: 2BCE>	11d8a
71: 2BD6
74: 2BDE	90: 3D44
91> 3D4C	wait for bit 0 of d00000 to be 0
92> 3D5C	200010.w<-D1	200012.w<-D2	200014.w<-D3
f1: 10F6

***************************************************************************/

/*	This version displays:

	tb05mm-eu "1000 miglia"
	master up= 94/07/18 15:12:35			*/

ROM_START( gtmr )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "u2.bin", 0x000000, 0x080000, 0x031799f7 )
	ROM_LOAD16_BYTE( "u1.bin", 0x000001, 0x080000, 0x6238790a )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "mcu_code.u12",  0x000000, 0x020000, 0x00000000 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	/* fill the 0x700000-7fffff range first, with the second of the identical halves */
//	ROM_LOAD16_BYTE( "gmmu64.bin",  0x600000, 0x100000, 0x57d77b33 )	// HALVES IDENTICAL
//	ROM_LOAD16_BYTE( "gmmu65.bin",  0x600001, 0x100000, 0x05b8bdca )	// HALVES IDENTICAL
	ROM_LOAD( "gmmu27.bin",  0x000000, 0x200000, 0xc0ab3efc )
	ROM_LOAD( "gmmu28.bin",  0x200000, 0x200000, 0xcf6b23dc )
	ROM_LOAD( "gmmu29.bin",  0x400000, 0x200000, 0x8f27f5d3 )
	ROM_LOAD( "gmmu30.bin",  0x600000, 0x080000, 0xe9747c8c )
	/* codes 6800-7fff are explicitly skipped */
	/* wrong tiles: 	gtmr	77e0 ; gtmralt	81c4 81e0 81c4 */
	ROM_LOAD( "sprites",     0x700000, 0x100000, 0x00000000 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "gmmu52.bin",  0x000000, 0x200000, 0xb15f6b7f )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "gmmu52.bin",  0x000000, 0x200000, 0xb15f6b7f )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "gmmu23.bin",  0x000000, 0x100000, 0xb9cbfbee )	// 16 x $10000

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "gmmu24.bin",  0x000000, 0x100000, 0x380cdc7c )	//  2 x $40000 - HALVES IDENTICAL
ROM_END


/*	This version displays:

	tb05mm-eu "1000 miglia"
	master up= 94/09/06 14:49:19			*/

ROM_START( gtmre )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "gmmu2.bin", 0x000000, 0x080000, 0x36dc4aa9 )
	ROM_LOAD16_BYTE( "gmmu1.bin", 0x000001, 0x080000, 0x8653c144 )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "mcu_code.u12",  0x000000, 0x020000, 0x00000000 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	/* fill the 0x700000-7fffff range first, with the second of the identical halves */
	ROM_LOAD16_BYTE( "gmmu64.bin",  0x600000, 0x100000, 0x57d77b33 )	// HALVES IDENTICAL
	ROM_LOAD16_BYTE( "gmmu65.bin",  0x600001, 0x100000, 0x05b8bdca )	// HALVES IDENTICAL
	ROM_LOAD( "gmmu27.bin",  0x000000, 0x200000, 0xc0ab3efc )
	ROM_LOAD( "gmmu28.bin",  0x200000, 0x200000, 0xcf6b23dc )
	ROM_LOAD( "gmmu29.bin",  0x400000, 0x200000, 0x8f27f5d3 )
	ROM_LOAD( "gmmu30.bin",  0x600000, 0x080000, 0xe9747c8c )
	/* codes 6800-6fff are explicitly skipped */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "gmmu52.bin",  0x000000, 0x200000, 0xb15f6b7f )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "gmmu52.bin",  0x000000, 0x200000, 0xb15f6b7f )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "gmmu23.bin",  0x000000, 0x100000, 0xb9cbfbee )	// 16 x $10000

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "gmmu24.bin",  0x000000, 0x100000, 0x380cdc7c )	//  2 x $40000 - HALVES IDENTICAL
ROM_END


/*	This version displays:

	tb05mm-eu "1000 miglia"
	master up= 94/09/06 20:30:39			*/

ROM_START( gtmrusa )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "gtmrusa.u2", 0x000000, 0x080000, 0x5be615c4 )
	ROM_LOAD16_BYTE( "gtmrusa.u1", 0x000001, 0x080000, 0xae853e4e )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code? */
	ROM_LOAD( "gtmrusa.u12",  0x000000, 0x020000, 0x2e1a06ff )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	/* fill the 0x700000-7fffff range first, with the second of the identical halves */
	ROM_LOAD16_BYTE( "gmmu64.bin",  0x600000, 0x100000, 0x57d77b33 )	// HALVES IDENTICAL
	ROM_LOAD16_BYTE( "gmmu65.bin",  0x600001, 0x100000, 0x05b8bdca )	// HALVES IDENTICAL
	ROM_LOAD( "gmmu27.bin",  0x000000, 0x200000, 0xc0ab3efc )
	ROM_LOAD( "gmmu28.bin",  0x200000, 0x200000, 0xcf6b23dc )
	ROM_LOAD( "gmmu29.bin",  0x400000, 0x200000, 0x8f27f5d3 )
	ROM_LOAD( "gmmu30.bin",  0x600000, 0x080000, 0xe9747c8c )
	/* codes 6800-6fff are explicitly skipped */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "gmmu52.bin",  0x000000, 0x200000, 0xb15f6b7f )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "gmmu52.bin",  0x000000, 0x200000, 0xb15f6b7f )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "gmmu23.bin",  0x000000, 0x100000, 0xb9cbfbee )	// 16 x $10000

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "gmmu24.bin",  0x000000, 0x100000, 0x380cdc7c )	//  2 x $40000 - HALVES IDENTICAL
ROM_END


/***************************************************************************

						Great 1000 Miles Rally 2

GMR2U48	1M		OKI6295: 00000-3ffff + chunks of 0x10000 with headers

GMR2U49	2M		sprites
GMR2U50	2M		sprites
GMR2U51	2M		sprites - FIRST AND SECOND HALF IDENTICAL

GMR2U89	2M		tiles
GMR1U90	2M		tiles
GMR2U90	IDENTICAL TO GMR1U90

***************************************************************************/

ROM_START( gtmr2 )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "maincode.1", 0x000000, 0x080000, 0x00000000 )
	ROM_LOAD16_BYTE( "maincode.2", 0x000001, 0x080000, 0x00000000 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "gmr2u49.bin",  0x000000, 0x200000, 0xd50f9d80 )
	ROM_LOAD( "gmr2u50.bin",  0x200000, 0x200000, 0x39b60a83 )
	ROM_LOAD( "gmr2u51.bin",  0x400000, 0x200000, 0xfd06b339 )
	ROM_LOAD( "gmr2u89.bin",  0x600000, 0x200000, 0x4dc42fbb )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "gmr1u90.bin", 0x000000, 0x200000, 0xf4e894f2 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "gmr2u90.bin", 0x000000, 0x200000, 0xf4e894f2 )	// IDENTICAL to gmr1u90

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "gmr2u48.bin", 0x000000, 0x100000, 0x1 )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "samples",  0x000000, 0x100000, 0x00000000 )
ROM_END


/***************************************************************************

								Magical Crystals

(c)1991 Kaneko/Atlus
Z00FC-02

CPU  : TMP68HC000N-12
Sound: YM2149Fx2 M6295
OSC  : 16.0000MHz(X1) 12.0000MHz(X2)

ROMs:
mc100j.u18 - Main programs
mc101j.u19 /

mc000.u38  - Graphics (32pin mask)
mc001.u37  | (32pin mask)
mc002j.u36 / (27c010)

mc010.u04 - Graphics (42pin mask)

mc020.u33 - Graphics (42pin mask)

mc030.u32 - Samples (32pin mask)

PALs (18CV8PC):
u08, u20, u41, u42, u50, u51, u54

Custom chips:
KANEKO VU-001 046A (u53, 48pin PQFP)
KANEKO VU-002 052 151021 (u60, 160pin PQFP)
KANEKO 23160-509 9047EAI VIEW2-CHIP (u24 & u34, 144pin PQFP)
KANEKO MUX2-CHIP (u28, 64pin PQFP)
KANEKO IU-001 9045KP002 (u22, 44pin PQFP)
KANEKO I/O JAMMA MC-8282 047 (u5, 46pin)
699206p (u09, 44pin PQFP)
699205p (u10, 44pin PQFP)

Other:
93C46 EEPROM

DIP settings:
1: Flip screen
2: Test mode
3: Unused
4: Unused

***************************************************************************/

ROM_START( mgcrystl )
 	ROM_REGION( 0x040000*2, REGION_CPU1, ROMREGION_ERASE )			/* 68000 Code */
	ROM_LOAD16_BYTE( "mc100j.u18", 0x000000, 0x020000, 0xafe5882d )
	ROM_LOAD16_BYTE( "mc101j.u19", 0x000001, 0x040000, 0x60da5492 )	//!!

	ROM_REGION( 0x280000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "mc000.u38",  0x000000, 0x100000, 0x28acf6f4 )
	ROM_LOAD( "mc001.u37",  0x100000, 0x080000, 0x005bc43d )
	ROM_RELOAD(             0x180000, 0x080000             )
	ROM_LOAD( "mc002j.u36", 0x200000, 0x020000, 0x27ac1056 )
	ROM_RELOAD(             0x220000, 0x020000             )
	ROM_RELOAD(             0x240000, 0x020000             )
	ROM_RELOAD(             0x260000, 0x020000             )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "mc010.u04",  0x000000, 0x100000, 0x85072772 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "mc020.u34",  0x000000, 0x100000, 0x1ea92ff1 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "mc030.u32",  0x000000, 0x040000, 0xc165962e )
ROM_END


/***************************************************************************

								Sand Scorpion

(C) FACE
68HC000N-12
Z8400AB1
OKI6295, YM2203C
OSC:  16.000mhz,   12.000mhz

SANDSC03.BIN     27C040
SANDSC04.BIN     27C040
SANDSC05.BIN     27C040
SANDSC06.BIN     27C040
SANDSC07.BIN     27C2001
SANDSC08.BIN     27C1001
SANDSC11.BIN     27C2001
SANDSC12.BIN     27C2001

***************************************************************************/

ROM_START( sandscrp )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "sandsc11.bin", 0x000000, 0x040000, 0x9b24ab40 )
	ROM_LOAD16_BYTE( "sandsc12.bin", 0x000001, 0x040000, 0xad12caee )

	ROM_REGION( 0x24000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "sandsc08.bin", 0x00000, 0x0c000, 0x6f3e9db1 )
	ROM_CONTINUE(             0x10000, 0x14000             )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "sandsc05.bin", 0x000000, 0x080000, 0x9bb675f6 )
	ROM_LOAD( "sandsc06.bin", 0x080000, 0x080000, 0x7df2f219 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layers */
	ROM_LOAD16_BYTE( "sandsc04.bin", 0x000000, 0x080000, 0xb9222ff2 )
	ROM_LOAD16_BYTE( "sandsc03.bin", 0x000001, 0x080000, 0xadf20fa0 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sandsc07.bin", 0x000000, 0x040000, 0x9870ab12 )
ROM_END

void init_sandscrp(void)
{
	data16_t *RAM = (data16_t *) memory_region(REGION_CPU1);

	// $100->0 (fix the bg x scroll being 100 pixels off during game only!)
	RAM[0x13ee/2] = 0x0000;
}


/***************************************************************************

								Shogun Warriors

Shogun Warriors, Kaneko 1992

   fb010.u65           fb040.u33
   fb011.u66
   rb012.u67
   rb013.u68

                         fb001.u43
     68000-12            fb000.u42  m6295
    51257     fb030.u61  fb002.u44  m6295
    51257     fb031.u62  fb003.u45


                fb021a.u3
                fb021b.u4
                fb022a.u5
   fb023.u7     fb022b.u6
   fb020a.u1    fb020b.u2



---------------------------------------------------------------------------
								Game code
---------------------------------------------------------------------------

102e04-7	<- !b80004-7
102e18.w	-> $800000
102e1c.w	-> $800002 , $800006
102e1a.w	-> $800004
102e20.w	-> $800008

ROUTINES:

6622	print ($600000)

***************************************************************************/

ROM_START( shogwarr )
 	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "fb030a.u61", 0x000000, 0x020000, 0xa04106c6 )
	ROM_LOAD16_BYTE( "fb031a.u62", 0x000001, 0x020000, 0xd1def5e2 )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "fb040a.u33",  0x000000, 0x020000, 0x4b62c4d9 )

	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "fb020a.u1",  0x000000, 0x080000, 0xda1b7373 )
	ROM_LOAD( "fb022a.u5",  0x080000, 0x080000, 0x60aa1282 )
	ROM_LOAD( "fb020b.u2",  0x100000, 0x100000, 0x276b9d7b )
	ROM_LOAD( "fb021a.u3",  0x200000, 0x100000, 0x7da15d37 )
	ROM_LOAD( "fb021b.u4",  0x300000, 0x100000, 0x6a512d7b )
	ROM_LOAD( "fb023.u7",   0x400000, 0x100000, 0x132794bd )
	ROM_LOAD( "fb022b.u6",  0x500000, 0x080000, 0xcd05a5c8 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "fb010.u65",  0x000000, 0x100000, 0x296ffd92 )
	ROM_LOAD( "fb011.u66",  0x100000, 0x080000, 0x500a0367 )	// ?!
	ROM_LOAD( "rb012.u67",  0x200000, 0x100000, 0xbfdbe0d1 )
	ROM_LOAD( "rb013.u68",  0x300000, 0x100000, 0x28c37fe8 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "fb000e.u42",  0x000000, 0x080000, 0x969f1465 )	// 2 x $40000
	ROM_LOAD( "fb001e.u43",  0x080000, 0x080000, 0xf524aaa1 )	// 2 x $40000

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "fb002.u44",   0x000000, 0x080000, 0x05d7c2a9 )	// 2 x $40000
	ROM_LOAD( "fb003.u45",   0x080000, 0x080000, 0x405722e9 )	// 2 x $40000
ROM_END


void init_shogwarr(void)
{
	/* Code patches */
#if 0
	data16_t *RAM = memory_region(REGION_CPU1);
	RAM[0x0039a / 2] = 0x4e71;	// 200000 test
	RAM[0x003e6 / 2] = 0x4e71;	// 20030a test
	RAM[0x223a8 / 2] = 0x6000;	// rom test
#endif

	init_kaneko16();

/*
	ROM test at 2237e:

	the chksum of 00000-03fffd = $657f is added to ($200042).w
	[from shared ram]. The result must be $f463 [=($3fffe).w]

	Now, $f463-$657f = $8ee4 = byte sum of FB040A.U33 !!

	So, there's probably the MCU's code in there, though
	I can't id what kind of CPU should run it :-(
*/
}


/***************************************************************************


								Game drivers


***************************************************************************/

/* Working games */

GAME( 1991, berlwall, 0,        berlwall, berlwall, berlwall, ROT0,  "Kaneko", "The Berlin Wall (set 1)" )
GAME( 1991, berlwalt, berlwall, berlwall, berlwalt, berlwall, ROT0,  "Kaneko", "The Berlin Wall (set 2)" )
GAME( 1991, mgcrystl, 0,        mgcrystl, mgcrystl, kaneko16, ROT0,  "Kaneko", "Magical Crystals (Japan)" )
GAME( 1992, blazeon,  0,        blazeon,  blazeon,  kaneko16, ROT0,  "Atlus",  "Blaze On (Japan)" )
GAME( 1992, sandscrp, 0,        sandscrp, sandscrp, sandscrp, ROT90, "Face",   "Sand Scorpion" )
GAME( 1994, gtmr,     0,        gtmr,     gtmr,     kaneko16, ROT0,  "Kaneko", "Great 1000 Miles Rally" )
GAME( 1994, gtmre,    gtmr,     gtmr,     gtmr,     kaneko16, ROT0,  "Kaneko", "Great 1000 Miles Rally (Evolution Model)" )
GAME( 1994, gtmrusa,  gtmr,     gtmr,     gtmr,     kaneko16, ROT0,  "Kaneko", "Great 1000 Miles Rally (USA)" )

/* Non-working games (mainly due to incomplete dumps) */

GAMEX(1992, bakubrkr, 0,        bakubrkr, bakubrkr, 0,        ROT90,      "Kaneko", "Bakuretsu Breaker",         GAME_NOT_WORKING  )
GAMEX(1992, shogwarr, 0,        shogwarr, shogwarr, shogwarr, ROT0,       "Kaneko", "Shogun Warriors",           GAME_NOT_WORKING  )
GAMEX(1995, gtmr2,    0,        gtmr,     gtmr,     kaneko16, ROT0,       "Kaneko", "Great 1000 Miles Rally 2",  GAME_NOT_WORKING  )
