/***************************************************************************

	Art & Magic hardware

	driver by Aaron Giles

	Games supported:
		* Cheese Chase
		* Ultimate Tennis
		* Stone Ball

	Known bugs:
		* blitter data format not understood
		* sound banking not working yet
		* inputs not fully mapped
		* protection on Ultimate Tennis/Stone Ball not figured out

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "artmagic.h"


/*************************************
 *
 *	Interrupts
 *
 *************************************/

static void m68k_gen_int(int state)
{
	cpu_set_irq_line(0, 4, state ? ASSERT_LINE : CLEAR_LINE);
}



/*************************************
 *
 *	TMS34010 interface
 *
 *************************************/

static READ16_HANDLER( tms_host_r )
{
	return tms34010_host_r(1, offset);
}


static WRITE16_HANDLER( tms_host_w )
{
	tms34010_host_w(1, offset, data);
}



/*************************************
 *
 *	Unknown memory accesses
 *
 *************************************/

static data16_t *unknown;
static WRITE16_HANDLER( unknown_w )
{
	COMBINE_DATA(&unknown[offset]);
	logerror("%06X:unknown_w(%d) = %04X\n", activecpu_get_pc(), offset, data);
}



/*************************************
 *
 *	Ultimate Tennis protection workarounds
 *
 *************************************/

static READ16_HANDLER( ultennis_protection_r )
{
	/* IRQ5 points to: jsr (a5); rte */
	if (activecpu_get_pc() == 0x18c2)	// hack for ultennis
		cpu_set_irq_line(0, 5, PULSE_LINE);
	return readinputport(0);
}



/*************************************
 *
 *	Stone Ball protection workarounds
 *
 *************************************/

static UINT8 prot_input[16];
static UINT8 prot_input_index;
static UINT8 prot_output[16];
static UINT8 prot_output_index;
static UINT8 prot_output_bit;
static UINT8 prot_bit_index;

static void (*protection_handler)(void);


static void ultennis_protection(void)
{
	/* check the command byte */
	switch (prot_input[0])
	{
		case 0x00:	/* reset */
			logerror("protection command 00: reset\n");
			prot_input_index = prot_output_index = 0;
			break;

		case 0x01:	/* 01 aaaa bbbb cccc dddd (xxxx) */
			/* initial inputs...

				Stone Ball:
					0000 0000 00A0 69CE
					FFEB FFFE 00A0 68E1
					FFE5 FFFD 00A0 686C
					FFE2 FFFB 00A0 6800
					FFE0 FFF9 00A0 6795
					FFDF FFF7 00A0 672B
					FFDF FFF5 00A0 66C2
					FFE0 FFF3 00A1 665A
					FFE1 FFF1 00A1 65F2
					FFE3 FFEF 00A1 658C
					FFE4 FFED 00A1 6526
					FFE6 FFEB 00A1 64C1
					FFE9 FFEA 00A1 6464
					FFEB FFE9 00A1 6401
					FFED FFE7 00A1 639E

				Ultimate Tennis:
					7CA4 7B20 0D87 00C5
					4443 34A0 0D87 FFD8
					7CA4 7B2D 0D87 00C5
					455B 34BE 0D87 FFD9
					7CA4 7B39 0D87 00C5
					4673 34D9 0D87 FFDB
					7CA4 7B46 0D87 00C6
					4793 34DE 0D87 FFDB
					7CA4 7B52 0D87 00C6
					48B3 34E0 0D87 FFDB
					7CA4 7B5F 0D87 00C6
					496B 34E0 0D87 FFDB
			*/
			if (prot_input_index == 9)
			{
				UINT16 a = prot_input[1] | (prot_input[2] << 8);
				UINT16 b = prot_input[3] | (prot_input[4] << 8);
				UINT16 c = prot_input[5] | (prot_input[6] << 8);
				UINT16 d = prot_input[7] | (prot_input[8] << 8);
				UINT16 x = 0xa5a5;
//				usrintf_showmessage("01: %04X %04X %04X %04X", a, b, c, d);
				logerror("protection command 01: %04X %04X %04X %04X -> %04X\n", a, b, c, d, x);
				prot_output[0] = x;
				prot_output[1] = x >> 8;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 11)
				prot_input_index = 0;
			break;

		case 0x02:	/* 02 aaaa bbbb cccc (xxxxxxxx) */
			/* initial inputs...

				Ultimate Tennis:
					0041 0084 00C8
			*/
			if (prot_input_index == 7)
			{
				UINT16 a = prot_input[1] | (prot_input[2] << 8);
				UINT16 b = prot_input[3] | (prot_input[4] << 8);
				UINT16 c = prot_input[5] | (prot_input[6] << 8);
				UINT32 x = 0xa5a5a5a5;
//				usrintf_showmessage("02: %04X %04X %04X", a, b, c);
				logerror("protection command 02: %04X %04X %04X -> %08X\n", a, b, c, x);
				prot_output[0] = x;
				prot_output[1] = x >> 8;
				prot_output[2] = x >> 16;
				prot_output[3] = x >> 24;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 11)
				prot_input_index = 0;
			break;

		case 0x03:	/* 03 (xxxx) */
			if (prot_input_index == 1)
			{
				UINT16 x = 0xa5a5;
				logerror("protection command 03: -> %04X\n", x);
				prot_output[0] = x;
				prot_output[1] = x >> 8;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 3)
				prot_input_index = 0;
			break;

		case 0x04:	/* 04 aaaa */
			if (prot_input_index == 3)
			{
				UINT16 a = prot_input[1] | (prot_input[2] << 8);
				logerror("protection command 04: %04X\n", a);
				prot_input_index = prot_output_index = 0;
			}
			break;

		default:
			logerror("protection command %02X: unknown\n", prot_input[0]);
			prot_input_index = prot_output_index = 0;
			break;
	}
}


static void stonebal_protection(void)
{
	/* check the command byte */
	switch (prot_input[0])
	{
		case 0x02:	/* 02 aaaa (xx) */
			if (prot_input_index == 3)
			{
				UINT16 a = prot_input[1] | (prot_input[2] << 8);
				UINT8 x = 0xa5;
				logerror("protection command 02: %04X -> %02X\n", a, x);
				prot_output[0] = x;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 4)
				prot_input_index = 0;
			break;

		default:
			ultennis_protection();
			break;
	}
}


static READ16_HANDLER( protection_r )
{
//	logerror("%06X:prot_protection_r(%d)\n", activecpu_get_pc(), offset);
	return (readinputport(5) & ~0x0001) | prot_output_bit;
}


static WRITE16_HANDLER( protection_w )
{
//	logerror("%06X:protection_w(%d,%04X)\n", activecpu_get_pc(), offset, data);

	/* shift in the new bit based on the offset */
	prot_input[prot_input_index] <<= 1;
	prot_input[prot_input_index] |= offset;

	/* clock out the next bit based on the offset */
	prot_output_bit = prot_output[prot_output_index] & 0x01;
	prot_output[prot_output_index] >>= 1;

	/* are we done with a whole byte? */
	if (++prot_bit_index == 8)
	{
		/* add the data and process it */
		prot_input_index++;
		prot_output_index++;
		prot_bit_index = 0;

		/* update the protection state */
		(*protection_handler)();
	}
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x220000, 0x23ffff, MRA16_RAM },
	{ 0x240000, 0x240fff, MRA16_RAM },
	{ 0x300000, 0x300001, input_port_0_word_r },
	{ 0x300002, 0x300003, input_port_1_word_r },
	{ 0x300004, 0x300005, input_port_2_word_r },
	{ 0x300006, 0x300007, input_port_3_word_r },
	{ 0x300008, 0x300009, input_port_4_word_r },
	{ 0x30000a, 0x30000b, input_port_5_word_r },
	{ 0x360000, 0x360001, OKIM6295_status_0_lsb_r },
	{ 0x380000, 0x380007, tms_host_r },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x220000, 0x23ffff, MWA16_RAM },
	{ 0x240000, 0x240fff, MWA16_RAM, (data16_t **)&generic_nvram, &generic_nvram_size },
	{ 0x300000, 0x30000f, unknown_w, &unknown },
	{ 0x360000, 0x360001, OKIM6295_data_0_lsb_w },
	{ 0x380000, 0x380007, tms_host_w },
MEMORY_END


static MEMORY_READ16_START( stonebal_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x200000, 0x27ffff, MRA16_RAM },
	{ 0x280000, 0x280fff, MRA16_RAM },
	{ 0x300000, 0x300001, input_port_0_word_r },
	{ 0x300002, 0x300003, input_port_1_word_r },
	{ 0x300004, 0x300005, input_port_2_word_r },
	{ 0x300006, 0x300007, input_port_3_word_r },
	{ 0x300008, 0x300009, input_port_4_word_r },
	{ 0x30000a, 0x30000b, protection_r },
	{ 0x340000, 0x340001, OKIM6295_status_0_lsb_r },
	{ 0x380000, 0x380007, tms_host_r },
MEMORY_END


static MEMORY_WRITE16_START( stonebal_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x200000, 0x27ffff, MWA16_RAM },
	{ 0x280000, 0x280fff, MWA16_RAM, (data16_t **)&generic_nvram, &generic_nvram_size },
	{ 0x300000, 0x30000f, unknown_w, &unknown },
	{ 0x340000, 0x340001, OKIM6295_data_0_lsb_w },
	{ 0x380000, 0x380007, tms_host_w },
MEMORY_END



/*************************************
 *
 *	Slave CPU memory handlers
 *
 *************************************/

static struct tms34010_config tms_config =
{
	1,								/* halt on reset */
	m68k_gen_int,					/* generate interrupt */
	artmagic_to_shiftreg,			/* write to shiftreg function */
	artmagic_from_shiftreg,			/* read from shiftreg function */
	0								/* display offset update function */
};


static MEMORY_READ16_START( tms_readmem )
	{ TOBYTE(0x00000000), TOBYTE(0x001fffff), MRA16_RAM },
	{ TOBYTE(0x00400000), TOBYTE(0x005fffff), MRA16_RAM },
	{ TOBYTE(0x00800000), TOBYTE(0x0080000f), artmagic_blitter_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xffe00000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( tms_writemem )
	{ TOBYTE(0x00000000), TOBYTE(0x001fffff), MWA16_RAM, &artmagic_vram0 },
	{ TOBYTE(0x00400000), TOBYTE(0x005fffff), MWA16_RAM, &artmagic_vram1 },
	{ TOBYTE(0x00800000), TOBYTE(0x0080007f), artmagic_blitter_w },
	{ TOBYTE(0x00c00000), TOBYTE(0x00c0001f), artmagic_paletteram_w },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_w },
	{ TOBYTE(0xffe00000), TOBYTE(0xffffffff), MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( stonebal_tms_readmem )
	{ TOBYTE(0x00000000), TOBYTE(0x001fffff), MRA16_RAM },
	{ TOBYTE(0x00400000), TOBYTE(0x005fffff), MRA16_RAM },
	{ TOBYTE(0x00800000), TOBYTE(0x0080000f), artmagic_blitter_r },
	{ TOBYTE(0x00c00000), TOBYTE(0x00c0003f), artmagic_paletteram_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xffc00000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( stonebal_tms_writemem )
	{ TOBYTE(0x00000000), TOBYTE(0x001fffff), MWA16_RAM, &artmagic_vram0 },
	{ TOBYTE(0x00400000), TOBYTE(0x005fffff), MWA16_RAM, &artmagic_vram1 },
	{ TOBYTE(0x00800000), TOBYTE(0x0080007f), artmagic_blitter_w },
	{ TOBYTE(0x00c00000), TOBYTE(0x00c0003f), artmagic_paletteram_w },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_w },
	{ TOBYTE(0xffc00000), TOBYTE(0xffffffff), MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( cheesech )
	PORT_START	/* 300000 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 300002 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 300004 */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0006, 0x0004, "Language" )
	PORT_DIPSETTING(      0x0000, "French" )
	PORT_DIPSETTING(      0x0002, "Italian" )
	PORT_DIPSETTING(      0x0004, "English" )
	PORT_DIPSETTING(      0x0006, "German" )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Lives ))
	PORT_DIPSETTING(      0x0008, "3" )
	PORT_DIPSETTING(      0x0018, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPSETTING(      0x0010, "6" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x00c0, 0x0040, DEF_STR( Difficulty ))
	PORT_DIPSETTING(      0x00c0, "Easy" )
	PORT_DIPSETTING(      0x0040, "Normal" )
	PORT_DIPSETTING(      0x0080, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )

	PORT_START	/* 300006 */
	PORT_DIPNAME( 0x0007, 0x0007, "Right Coinage" )
	PORT_DIPSETTING(      0x0002, DEF_STR( 6C_1C ))
	PORT_DIPSETTING(      0x0006, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ))
	PORT_DIPNAME( 0x0038, 0x0038, "Left Coinage" )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* 300008 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 30000a */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )	// tight loop @013220
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( ultennis )
	PORT_START	/* 300000 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 300002 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 300004 */
	PORT_DIPNAME( 0x0001, 0x0001, "Button Layout" )
	PORT_DIPSETTING(      0x0001, "Triangular" )
	PORT_DIPSETTING(      0x0000, "Linear" )
	PORT_DIPNAME( 0x0002, 0x0002, "Start set at" )
	PORT_DIPSETTING(      0x0000, "0-0" )
	PORT_DIPSETTING(      0x0002, "4-4" )
	PORT_DIPNAME( 0x0004, 0x0004, "Sets per Match" )
	PORT_DIPSETTING(      0x0004, "1" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0018, 0x0008, "Game Duration" )
	PORT_DIPSETTING(      0x0018, "5 lost points" )
	PORT_DIPSETTING(      0x0008, "6 lost points" )
	PORT_DIPSETTING(      0x0010, "7 lost points" )
	PORT_DIPSETTING(      0x0000, "8 lost points" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x00c0, 0x0040, DEF_STR( Difficulty ))
	PORT_DIPSETTING(      0x00c0, "Easy" )
	PORT_DIPSETTING(      0x0040, "Normal" )
	PORT_DIPSETTING(      0x0080, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )

	PORT_START	/* 300006 */
	PORT_DIPNAME( 0x0007, 0x0007, "Right Coinage" )
	PORT_DIPSETTING(      0x0002, DEF_STR( 6C_1C ))
	PORT_DIPSETTING(      0x0006, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ))
	PORT_DIPNAME( 0x0038, 0x0038, "Left Coinage" )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* 300008 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 30000a */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )	// tight loop @013220
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( stonebal )
	PORT_START	/* 300000 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 300002 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 300004 */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Free_Play ))
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x001c, 0x001c, "Left Coinage" )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0x00e0, 0x00e0, "Right Coinage" )
	PORT_DIPSETTING(      0x0040, DEF_STR( 6C_1C ))
	PORT_DIPSETTING(      0x0060, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x00a0, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(      0x00c0, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ))

	PORT_START	/* 300006 */
	PORT_DIPNAME( 0x0003, 0x0002, DEF_STR( Difficulty ))
	PORT_DIPSETTING(      0x0003, "Easy" )
	PORT_DIPSETTING(      0x0002, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0038, 0x0038, "Match Time" )
	PORT_DIPSETTING(      0x0030, "60s" )
	PORT_DIPSETTING(      0x0028, "70s" )
	PORT_DIPSETTING(      0x0020, "80s" )
	PORT_DIPSETTING(      0x0018, "90s" )
	PORT_DIPSETTING(      0x0038, "100s" )
	PORT_DIPSETTING(      0x0010, "110s" )
	PORT_DIPSETTING(      0x0008, "120s" )
	PORT_DIPSETTING(      0x0000, "130s" )
	PORT_DIPNAME( 0x0040, 0x0040, "Free Match Time" )
	PORT_DIPSETTING(      0x0040, "Normal" )
	PORT_DIPSETTING(      0x0000, "Short" )
	PORT_DIPNAME( 0x0080, 0x0080, "Game Mode" )
	PORT_DIPSETTING(      0x0080, "4 Players" )
	PORT_DIPSETTING(      0x0000, "2 Players" )

	PORT_START	/* 300008 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 30000a */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )	// tight loop @013220
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Sound definition
 *
 *************************************/

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 10000000/10/132 },	/* ??? */
	{ REGION_SOUND1 },
	{ 100 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

MACHINE_DRIVER_START( artmagic )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 12000000)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)

	MDRV_CPU_ADD_TAG("tms", TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_MEMORY(tms_readmem,tms_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((1000000 * (312 - 256)) / (60 * 312))
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 255)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(artmagic)
	MDRV_VIDEO_UPDATE(artmagic)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( stonebal )
	MDRV_IMPORT_FROM(artmagic)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(stonebal_readmem,stonebal_writemem)

	MDRV_CPU_MODIFY("tms")
	MDRV_CPU_MEMORY(stonebal_tms_readmem,stonebal_tms_writemem)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( cheesech )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "u102",     0x00000, 0x40000, 0x1d6e07c5 )
	ROM_LOAD16_BYTE( "u101",     0x00001, 0x40000, 0x30ae9f95 )

	ROM_REGION( TOBYTE(0x00200000), REGION_CPU2, 0 )	/* dummy TMS34010 region */

	ROM_REGION16_LE( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "u134", 0x00000, 0x80000, 0x354ba4a6 )
	ROM_LOAD16_BYTE( "u135", 0x00001, 0x80000, 0x97348681 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "u151", 0x00000, 0x80000, 0x65d5ebdb )
ROM_END


ROM_START( ultennis )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "utu102.bin", 0x00000, 0x40000, 0xec31385e )
	ROM_LOAD16_BYTE( "utu101.bin", 0x00001, 0x40000, 0x08a7f655 )

	ROM_REGION( TOBYTE(0x00200000), REGION_CPU2, 0 )	/* dummy TMS34010 region */

	ROM_REGION16_LE( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "utu133.bin", 0x000000, 0x200000, 0x29d9204d )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "utu151.bin", 0x00000,  0x40000, 0x4e19ca89 )
ROM_END


ROM_START( stonebal )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "u102",     0x00000, 0x40000, 0x712feda1 )
	ROM_LOAD16_BYTE( "u101",     0x00001, 0x40000, 0x4f1656a9 )

	ROM_REGION( TOBYTE(0x00200000), REGION_CPU2, 0 )	/* dummy TMS34010 region */

	ROM_REGION16_LE( 0x400000, REGION_GFX1, 0 )
	ROM_LOAD( "u1600.bin", 0x000000, 0x200000, 0xd2ffe9ff )
	ROM_LOAD( "u1601.bin", 0x200000, 0x200000, 0xdbe893f0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "u1801.bin", 0x00000, 0x80000, 0xd98f7378 )
ROM_END


ROM_START( stoneba2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "u102.bin", 0x00000, 0x40000, 0xb3c4f64f )
	ROM_LOAD16_BYTE( "u101.bin", 0x00001, 0x40000, 0xfe373f74 )

	ROM_REGION( TOBYTE(0x00200000), REGION_CPU2, 0 )	/* dummy TMS34010 region */

	ROM_REGION16_LE( 0x400000, REGION_GFX1, 0 )
	ROM_LOAD( "u1600.bin", 0x000000, 0x200000, 0xd2ffe9ff )
	ROM_LOAD( "u1601.bin", 0x200000, 0x200000, 0xdbe893f0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "u1801.bin", 0x00000, 0x80000, 0xd98f7378 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void decrypt_ultennis(void)
{
	int i;

	/* set up the parameters for the blitter data decryption which will happen at runtime */
	for (i = 0;i < 16;i++)
	{
		artmagic_xor[i] = 0x0462;
		if (i & 1) artmagic_xor[i] ^= 0x0011;
		if (i & 2) artmagic_xor[i] ^= 0x2200;
		if (i & 4) artmagic_xor[i] ^= 0x4004;
		if (i & 8) artmagic_xor[i] ^= 0x0880;
	}
}


static void decrypt_cheesech(void)
{
	int i;

	/* set up the parameters for the blitter data decryption which will happen at runtime */
	for (i = 0;i < 16;i++)
	{
		artmagic_xor[i] = 0x0891;
		if (i & 1) artmagic_xor[i] ^= 0x1100;
		if (i & 2) artmagic_xor[i] ^= 0x0022;
		if (i & 4) artmagic_xor[i] ^= 0x0440;
		if (i & 8) artmagic_xor[i] ^= 0x8008;
	}
}


static DRIVER_INIT( ultennis )
{
	decrypt_ultennis();
	artmagic_no_onelinedecrypt = 0;

	install_mem_read16_handler(0, 0x300000, 0x300001, ultennis_protection_r);

	/* set up protection */
	install_mem_write16_handler(0, 0x300004, 0x300007, protection_w);
	install_mem_read16_handler(0, 0x30000a, 0x30000b, protection_r);

	protection_handler = ultennis_protection;
}


static DRIVER_INIT( stonebal )
{
	decrypt_ultennis();
	artmagic_no_onelinedecrypt = 1;	/* blits 1 line high are NOT encrypted */

	/* set up protection */
	install_mem_write16_handler(0, 0x300004, 0x300007, protection_w);
	install_mem_read16_handler(0, 0x30000a, 0x30000b, protection_r);

	protection_handler = stonebal_protection;
}


static DRIVER_INIT( cheesech )
{
	decrypt_cheesech();
	artmagic_no_onelinedecrypt = 0;

	install_mem_read16_handler(0, 0x300000, 0x300001, ultennis_protection_r);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEX( 1993, ultennis, 0,        artmagic, ultennis, ultennis, ROT0, "Art & Magic", "Ultimate Tennis", GAME_IMPERFECT_GRAPHICS | GAME_UNEMULATED_PROTECTION )
GAMEX( 1994, cheesech, 0,        artmagic, cheesech, cheesech, ROT0, "Art & Magic", "Cheese Chase", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1994, stonebal, 0,        stonebal, stonebal, stonebal, ROT0, "Art & Magic", "Stone Ball (4 Players)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_UNEMULATED_PROTECTION )
GAMEX( 1994, stoneba2, stonebal, stonebal, stonebal, stonebal, ROT0, "Art & Magic", "Stone Ball (2 Players)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_UNEMULATED_PROTECTION )