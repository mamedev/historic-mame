/***************************************************************************

	Seibu Sound System v1.02, designed 1986 by Seibu Kaihatsu

	The Seibu sound system comprises of a Z80A, a YM3812, a YM3931*, and
	an Oki MSM6205.  As well as sound the Z80 can controls coins and pass
	data to the main cpu.  There are a few little quirks that make it
	worthwhile emulating in a seperate file:

	* The YM3812 generates interrupt RST10, by asserting the interrupt line,
	and placing 0xd7 on the data bus.

	* The main cpu generates interrupt RST18, by asserting the interrupt line,
	and placing 0xdf on the data bus.

	A problem can occur if both the YM3812 and the main cpu try to assert
	the interrupt line at the same time.  The effect in the old Mame
	emulation would be for sound to stop playing - this is because a RST18
	cancelled out a RST10, and if a single RST10 is dropped sound stops
	as the YM3812 timer is not reset.  The problem occurs because even
	if both interrupts happen at the same time, there can only be one value
	on the data bus.  Obviously the real hardware must have some circuit
	to prevent this.  It is emulated by user timers to control the z80
	interrupt vector.

	(*What on earth _is_ the YM3931??  There are no unknown memory writes)

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "sndhrdw/seibu.h"



/*
	Games using encrypted sound cpu:

	Air Raid         1987	"START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Cabal            1988	"Michel/Seibu    sound 11/04/88"
	Dead Angle       1988?	"START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Dynamite Duke    1989	"START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Toki             1989	"START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Raiden (alt)     1990	"START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."

	raiden and the decrypted raidena are not identical, there are vast sections of different data.
	However, there are a few different bytes in the middle of identical data, suggesting a possible
	error in the decryption scheme: they all require an additional XOR with 0x20 and are located at
	similar addresses.
	00002422: 03 23
	000024A1: 00 20
	000024A2: 09 29
	00002822: 48 68
	000028A1: 06 26
	00002A21: 17 37
	00002A22: 00 20
	00002AA1: 12 32
	00002C21: 02 22
	00002CA1: 02 22
	00002CA2: 17 37
*/


static UINT8 decrypt_data(int a,int src)
{
	if ( BIT(a,9)  &  BIT(a,8))             src ^= 0x80;
	if ( BIT(a,11) &  BIT(a,4) &  BIT(a,1)) src ^= 0x40;
	if ( BIT(a,11) & ~BIT(a,8) &  BIT(a,1)) src ^= 0x04;
	if ( BIT(a,13) & ~BIT(a,6) &  BIT(a,4)) src ^= 0x02;
	if (~BIT(a,11) &  BIT(a,9) &  BIT(a,2)) src ^= 0x01;

	if (BIT(a,13) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,3,2,0,1);
	if (BIT(a, 8) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,2,3,1,0);

	return src;
}

static UINT8 decrypt_opcode(int a,int src)
{
	if ( BIT(a,9)  &  BIT(a,8))             src ^= 0x80;
	if ( BIT(a,11) &  BIT(a,4) &  BIT(a,1)) src ^= 0x40;
	if (~BIT(a,13) & BIT(a,12))             src ^= 0x20;
	if (~BIT(a,6)  &  BIT(a,1))             src ^= 0x10;
	if (~BIT(a,12) &  BIT(a,2))             src ^= 0x08;
	if ( BIT(a,11) & ~BIT(a,8) &  BIT(a,1)) src ^= 0x04;
	if ( BIT(a,13) & ~BIT(a,6) &  BIT(a,4)) src ^= 0x02;
	if (~BIT(a,11) &  BIT(a,9) &  BIT(a,2)) src ^= 0x01;

	if (BIT(a,13) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,3,2,0,1);
	if (BIT(a, 8) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,2,3,1,0);
	if (BIT(a,12) &  BIT(a,9)) src = BITSWAP8(src,7,6,4,5,3,2,1,0);
	if (BIT(a,11) & ~BIT(a,6)) src = BITSWAP8(src,6,7,5,4,3,2,1,0);

	return src;
}

void seibu_sound_decrypt(int cpu_region,int length)
{
	UINT8 *rom = memory_region(cpu_region);
	int diff =  memory_region_length(cpu_region)/2;
	int i;

	memory_set_opcode_base(cpu_region-REGION_CPU1,rom+diff);

	for (i = 0;i < length;i++)
	{
		UINT8 src = rom[i];

		rom[i]      = decrypt_data(i,src);
		rom[i+diff] = decrypt_opcode(i,src);
	}
}


/***************************************************************************/


static int sound_cpu;

enum
{
	VECTOR_INIT,
	RST10_ASSERT,
	RST10_CLEAR,
	RST18_ASSERT,
	RST18_CLEAR
};

static void setvector_callback(int param)
{
	static int irq1,irq2;

	switch(param)
	{
		case VECTOR_INIT:
			irq1 = irq2 = 0xff;
			break;

		case RST10_ASSERT:
			irq1 = 0xd7;
			break;

		case RST10_CLEAR:
			irq1 = 0xff;
			break;

		case RST18_ASSERT:
			irq2 = 0xdf;
			break;

		case RST18_CLEAR:
			irq2 = 0xff;
			break;
	}

	cpu_irq_line_vector_w(sound_cpu,0,irq1 & irq2);
	if ((irq1 & irq2) == 0xff)	/* no IRQs pending */
		cpu_set_irq_line(sound_cpu,0,CLEAR_LINE);
	else	/* IRQ pending */
		cpu_set_irq_line(sound_cpu,0,ASSERT_LINE);
}

WRITE_HANDLER( seibu_irq_clear_w )
{
	setvector_callback(VECTOR_INIT);
}

WRITE_HANDLER( seibu_rst10_ack_w )
{
	/* Unused for now */
}

WRITE_HANDLER( seibu_rst18_ack_w )
{
	timer_set(TIME_NOW,RST18_CLEAR,setvector_callback);
}

void seibu_ym3812_irqhandler(int linestate)
{
	if (linestate)
		timer_set(TIME_NOW,RST10_ASSERT,setvector_callback);
	else
		timer_set(TIME_NOW,RST10_CLEAR,setvector_callback);
}

void seibu_ym2151_irqhandler(int linestate)
{
	if (linestate)
		timer_set(TIME_NOW,RST10_ASSERT,setvector_callback);
	else
		timer_set(TIME_NOW,RST10_CLEAR,setvector_callback);
}

void seibu_ym2203_irqhandler(int linestate)
{
	if (linestate)
		timer_set(TIME_NOW,RST10_ASSERT,setvector_callback);
	else
		timer_set(TIME_NOW,RST10_CLEAR,setvector_callback);
}

/***************************************************************************/

/* Use this if the sound cpu is cpu 1 */
MACHINE_INIT( seibu_sound_1 )
{
	sound_cpu=1;
	setvector_callback(VECTOR_INIT);
}

/* Use this if the sound cpu is cpu 2 */
MACHINE_INIT( seibu_sound_2 )
{
	sound_cpu=2;
	setvector_callback(VECTOR_INIT);
}

/***************************************************************************/

static UINT8 main2sub[2],sub2main[2];
static int main2sub_pending,sub2main_pending;

WRITE_HANDLER( seibu_bank_w )
{
	UINT8 *rom = memory_region(REGION_CPU1+sound_cpu);

	cpu_setbank(1,rom + 0x10000 + 0x8000 * (data & 1));
}

WRITE_HANDLER( seibu_coin_w )
{
	coin_counter_w(0,data & 1);
	coin_counter_w(1,data & 2);
}

READ_HANDLER( seibu_soundlatch_r )
{
	return main2sub[offset];
}

READ_HANDLER( seibu_main_data_pending_r )
{
	return sub2main_pending ? 1 : 0;
}

WRITE_HANDLER( seibu_main_data_w )
{
	sub2main[offset] = data;
}

WRITE_HANDLER( seibu_pending_w )
{
	/* just a guess */
	main2sub_pending = 0;
	sub2main_pending = 1;
}

READ16_HANDLER( seibu_main_word_r )
{
	//logerror("%06x: seibu_main_word_r(%x)\n",activecpu_get_pc(),offset);
	switch (offset)
	{
		case 2:
		case 3:
			return sub2main[offset-2];
		case 5:
			return main2sub_pending ? 1 : 0;
		default:
			//logerror("%06x: seibu_main_word_r(%x)\n",activecpu_get_pc(),offset);
			return 0xffff;
	}
}

WRITE16_HANDLER( seibu_main_word_w )
{
	//logerror("%06x: seibu_main_word_w(%x,%02x)\n",activecpu_get_pc(),offset,data);
	if (ACCESSING_LSB)
	{
		switch (offset)
		{
			case 0:
			case 1:
				main2sub[offset] = data;
				break;
			case 4:
				timer_set(TIME_NOW,RST18_ASSERT,setvector_callback);
				break;
			case 6:
				/* just a guess */
				sub2main_pending = 0;
				main2sub_pending = 1;
				break;
			default:
				//logerror("%06x: seibu_main_word_w(%x,%02x)\n",activecpu_get_pc(),offset,data);
				break;
		}
	}
}

READ_HANDLER( seibu_main_v30_r )
{
	return seibu_main_word_r(offset/2,0) >> (8 * (offset & 1));
}

WRITE_HANDLER( seibu_main_v30_w )
{
	seibu_main_word_w(offset/2,data << (8 * (offset & 1)),0xff00 >> (8 * (offset & 1)));
}

WRITE16_HANDLER( seibu_main_mustb_w )
{
	main2sub[0] = data&0xff;
	main2sub[1] = data>>8;

//	logerror("seibu_main_mustb_w: %x -> %x %x\n", data, main2sub[0], main2sub[1]);

	timer_set(TIME_NOW,RST18_ASSERT,setvector_callback);
}

/***************************************************************************/

MEMORY_READ_START( seibu_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4008, 0x4008, YM3812_status_port_0_r },
	{ 0x4010, 0x4011, seibu_soundlatch_r },
	{ 0x4012, 0x4012, seibu_main_data_pending_r },
	{ 0x4013, 0x4013, input_port_0_r },
	{ 0x6000, 0x6000, OKIM6295_status_0_r },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

MEMORY_WRITE_START( seibu_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x4000, 0x4000, seibu_pending_w },
	{ 0x4001, 0x4001, seibu_irq_clear_w },
	{ 0x4002, 0x4002, seibu_rst10_ack_w },
	{ 0x4003, 0x4003, seibu_rst18_ack_w },
	{ 0x4007, 0x4007, seibu_bank_w },
	{ 0x4008, 0x4008, YM3812_control_port_0_w },
	{ 0x4009, 0x4009, YM3812_write_port_0_w },
	{ 0x4018, 0x4019, seibu_main_data_w },
	{ 0x401b, 0x401b, seibu_coin_w },
	{ 0x6000, 0x6000, OKIM6295_data_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

MEMORY_READ_START( seibu2_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4009, 0x4009, YM2151_status_port_0_r },
	{ 0x4010, 0x4011, seibu_soundlatch_r },
	{ 0x4012, 0x4012, seibu_main_data_pending_r },
	{ 0x4013, 0x4013, input_port_0_r },
	{ 0x6000, 0x6000, OKIM6295_status_0_r },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

MEMORY_WRITE_START( seibu2_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x4000, 0x4000, seibu_pending_w },
	{ 0x4001, 0x4001, seibu_irq_clear_w },
	{ 0x4002, 0x4002, seibu_rst10_ack_w },
	{ 0x4003, 0x4003, seibu_rst18_ack_w },
	{ 0x4007, 0x4007, seibu_bank_w },
	{ 0x4008, 0x4008, YM2151_register_port_0_w },
	{ 0x4009, 0x4009, YM2151_data_port_0_w },
	{ 0x4018, 0x4019, seibu_main_data_w },
	{ 0x401b, 0x401b, seibu_coin_w },
	{ 0x6000, 0x6000, OKIM6295_data_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

MEMORY_READ_START( seibu3_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4008, 0x4008, YM2203_status_port_0_r },
	{ 0x4009, 0x4009, YM2203_read_port_0_r },
	{ 0x4010, 0x4011, seibu_soundlatch_r },
	{ 0x4012, 0x4012, seibu_main_data_pending_r },
	{ 0x4013, 0x4013, input_port_0_r },
	{ 0x6008, 0x6008, YM2203_status_port_1_r },
	{ 0x6009, 0x6009, YM2203_read_port_1_r },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

MEMORY_WRITE_START( seibu3_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x4000, 0x4000, seibu_pending_w },
	{ 0x4001, 0x4001, seibu_irq_clear_w },
	{ 0x4002, 0x4002, seibu_rst10_ack_w },
	{ 0x4003, 0x4003, seibu_rst18_ack_w },
	{ 0x4007, 0x4007, seibu_bank_w },
	{ 0x4008, 0x4008, YM2203_control_port_0_w },
	{ 0x4009, 0x4009, YM2203_write_port_0_w },
	{ 0x4018, 0x4019, seibu_main_data_w },
	{ 0x401b, 0x401b, seibu_coin_w },
	{ 0x6008, 0x6008, YM2203_control_port_1_w },
	{ 0x6009, 0x6009, YM2203_write_port_1_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END
