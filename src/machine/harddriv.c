/***************************************************************************

	Hard Drivin' machine hardware

****************************************************************************/

#include "machine/atarigen.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "cpu/adsp2100/adsp2100.h"
#include "sndhrdw/atarijsa.h"


/*************************************
 *
 *	Constants and macros
 *
 *************************************/

#define DUART_CLOCK TIME_IN_HZ(36864000)

/* debugging tools */
#define LOG_COMMANDS		0



/*************************************
 *
 *	External definitions
 *
 *************************************/

/* externally accessible */
data16_t *hdmsp_ram;
data16_t *hd68k_slapstic_base;
data16_t *hddsk_ram;
data16_t *hddsk_rom;
data16_t *hddsk_zram;

data16_t *hdgsp_speedup_addr[2];
offs_t hdgsp_speedup_pc;

data16_t *hdmsp_speedup_addr;
offs_t hdmsp_speedup_pc;

offs_t hdadsp_speedup_pc;


/* from slapstic.c */
int slapstic_tweak(offs_t offset);
void slapstic_reset(void);


/* from vidhrdw */
extern UINT8 *hdgsp_vram;



/*************************************
 *
 *	Static globals
 *
 *************************************/

static UINT8 gsp_cpu;
static UINT8 msp_cpu;
static UINT8 adsp_cpu;

static UINT8 irq_state;
static UINT8 gsp_irq_state;
static UINT8 msp_irq_state;
static UINT8 adsp_irq_state;
static UINT8 duart_irq_state;

static UINT8 duart_read_data[16];
static UINT8 duart_write_data[16];
static UINT8 duart_output_port;
static void *duart_timer;

static UINT8 pedal_value;

static UINT8 last_gsp_shiftreg;

static UINT8 m68k_zp1, m68k_zp2;
static UINT8 m68k_adsp_buffer_bank;

static UINT8 adsp_halt, adsp_br;
static UINT8 adsp_xflag;

static UINT16 adsp_sim_address;
static UINT16 adsp_som_address;
static UINT32 adsp_eprom_base;

static data16_t *sim_memory;
static data16_t *som_memory;
static data16_t *adsp_data_memory;
static data32_t *adsp_pgm_memory;
static data16_t *adsp_pgm_memory_word;

static UINT8 ds3_gcmd, ds3_gflag, ds3_g68irqs, ds3_g68flag, ds3_send;
static UINT16 ds3_gdata, ds3_g68data;
static UINT32 ds3_sim_address;

static UINT16 adc_control;
static UINT8 adc8_select;
static UINT8 adc8_data;
static UINT8 adc12_select;
static UINT8 adc12_byte;
static UINT16 adc12_data;



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (msp_irq_state)
		newstate = 1;
	if (adsp_irq_state)
		newstate = 2;
	if (gsp_irq_state)
		newstate = 3;
	if (atarigen_sound_int_state)	/* /LINKIRQ on STUN Runner */
		newstate = 4;
	if (irq_state)
		newstate = 5;
	if (duart_irq_state)
		newstate = 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


void harddriv_init_machine(void)
{
	int has_6502 = 0;
	int i;

	/* generic reset */
	atarigen_eeprom_reset();
	slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);

	/* determine the CPU numbers */
	gsp_cpu = msp_cpu = adsp_cpu = 0;
	for (i = 1; i < MAX_CPU; i++)
		switch (Machine->drv->cpu[i].cpu_type & ~CPU_FLAGS_MASK)
		{
			case CPU_TMS34010:
				if (gsp_cpu == 0)
					gsp_cpu = i;
				else
					msp_cpu = i;
				break;

			case CPU_ADSP2100:
			case CPU_ADSP2105:
				adsp_cpu = i;
				break;

			case CPU_M6502:
				has_6502 = 1;
				break;
		}

	/* if we found a 6502, reset the JSA board */
	if (has_6502)
		atarijsa_reset();

	/* predetermine memory regions */
	sim_memory = (data16_t *)memory_region(REGION_USER1);
	som_memory = (data16_t *)memory_region(REGION_USER2);
	adsp_data_memory = (data16_t *)(memory_region(REGION_CPU1 + adsp_cpu) + ADSP2100_DATA_OFFSET);
	adsp_pgm_memory = (data32_t *)(memory_region(REGION_CPU1 + adsp_cpu) + ADSP2100_PGM_OFFSET);
	adsp_pgm_memory_word = (data16_t *)((UINT8 *)adsp_pgm_memory + 1);

	/* set up the mirrored GSP banks */
	if (gsp_cpu)
	{
		cpu_setbank(1, hdgsp_vram);
	}

	/* set up the mirrored MSP banks */
	if (msp_cpu)
	{
		cpu_setbank(2, hdmsp_ram);
		cpu_setbank(3, hdmsp_ram);
	}

	/* halt the ADSP to start */
	cpu_set_halt_line(adsp_cpu, ASSERT_LINE);

	last_gsp_shiftreg = 0;

	m68k_adsp_buffer_bank = 0;

	irq_state = gsp_irq_state = msp_irq_state = adsp_irq_state = duart_irq_state = 0;

	memset(duart_read_data, 0, sizeof(duart_read_data));
	memset(duart_write_data, 0, sizeof(duart_write_data));
	duart_output_port = 0;
	duart_timer = NULL;

	adsp_halt = 1;
	adsp_br = 0;
	adsp_xflag = 0;

	pedal_value = 0;
}



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

int hd68k_vblank_gen(void)
{
	/* update the pedals once per frame */
	if (readinputport(5) & 1)
	{
		if (pedal_value < 255 - 16)
			pedal_value += 16;
	}
	else
	{
		if (pedal_value > 16)
			pedal_value -= 16;
	}
	return atarigen_video_int_gen();
}


int hd68k_irq_gen(void)
{
	irq_state = 1;
	atarigen_update_interrupts();
	return 0;
}


WRITE16_HANDLER( hd68k_irq_ack_w )
{
	irq_state = 0;
	atarigen_update_interrupts();
}


void hdgsp_irq_gen(int state)
{
	gsp_irq_state = state;
	atarigen_update_interrupts();
}


void hdmsp_irq_gen(int state)
{
	msp_irq_state = state;
	atarigen_update_interrupts();
}



/*************************************
 *
 *	I/O read dispatch.
 *
 *************************************/

READ16_HANDLER( hd68k_port0_r )
{
	/* port is as follows:

		0x0001 = DIAGN
		0x0002 = /HSYNCB
		0x0004 = /VSYNCB
		0x0008 = EOC12
		0x0010 = EOC8
		0x0020 = SELF-TEST
		0x0040 = COIN2
		0x0080 = COIN1
		0x0100 = SW1 #8
		0x0200 = SW1 #7
			.....
		0x8000 = SW1 #1
	*/
	int temp = readinputport(0);
	if (atarigen_get_hblank()) temp ^= 0x0002;
	temp ^= 0x0018;		/* both EOCs always high for now */
	return temp;
}



/*************************************
 *
 *	GSP I/O register writes
 *
 *************************************/

WRITE16_HANDLER( hdgsp_io_w )
{
	/* detect an enabling of the shift register and force yielding */
	if (offset == REG_DPYCTL)
	{
		UINT8 new_shiftreg = (data >> 11) & 1;
		if (new_shiftreg != last_gsp_shiftreg)
		{
			last_gsp_shiftreg = new_shiftreg;
			if (new_shiftreg)
				cpu_yield();
		}
	}
	tms34010_io_register_w(offset, data, mem_mask);
}



/*************************************
 *
 *	GSP I/O memory handlers
 *
 *************************************/

READ16_HANDLER( hd68k_gsp_io_r )
{
	offset = (offset / 2) ^ 1;
	return tms34010_host_r(gsp_cpu, offset);
}

WRITE16_HANDLER( hd68k_gsp_io_w )
{
	offset = (offset / 2) ^ 1;
	tms34010_host_w(gsp_cpu, offset, data);
}



/*************************************
 *
 *	MSP I/O memory handlers
 *
 *************************************/

READ16_HANDLER( hd68k_msp_io_r )
{
	offset = (offset / 2) ^ 1;
	return msp_cpu ? tms34010_host_r(msp_cpu, offset) : 0xffff;
}

WRITE16_HANDLER( hd68k_msp_io_w )
{
	offset = (offset / 2) ^ 1;
	if (msp_cpu)
		tms34010_host_w(msp_cpu, offset, data);
}



/*************************************
 *
 *	ADSP program memory handlers
 *
 *************************************/

READ16_HANDLER( hd68k_adsp_program_r )
{
	UINT32 word = adsp_pgm_memory[offset/2];
	return (!(offset & 1)) ? (word >> 16) : (word & 0xffff);
}

WRITE16_HANDLER( hd68k_adsp_program_w )
{
	UINT32 *base = &adsp_pgm_memory[offset/2];
	UINT32 oldword = *base;
	data16_t temp;

	if (!(offset & 1))
	{
		temp = oldword >> 16;
		COMBINE_DATA(&temp);
		oldword = (oldword & 0x0000ffff) | (temp << 16);
	}
	else
	{
		temp = oldword & 0xffff;
		COMBINE_DATA(&temp);
		oldword = (oldword & 0xffff0000) | temp;
	}
	ADSP2100_WRPGM(base, oldword);
}



/*************************************
 *
 *	ADSP data memory handlers
 *
 *************************************/

READ16_HANDLER( hd68k_adsp_data_r )
{
	return adsp_data_memory[offset];
}

WRITE16_HANDLER( hd68k_adsp_data_w )
{
	COMBINE_DATA(&adsp_data_memory[offset]);

	/* any write to $1FFF is taken to be a trigger; synchronize the CPUs */
	if (offset == 0x1fff)
	{
		logerror("ADSP sync address written (%04X)\n", data);
		timer_set(TIME_NOW, 0, 0);
		cpu_trigger(554433);
	}
}



/*************************************
 *
 *	ADSP buffer memory handlers
 *
 *************************************/

READ16_HANDLER( hd68k_adsp_buffer_r )
{
/*	logerror("hd68k_adsp_buffer_r(%04X)\n", offset);*/
	return som_memory[m68k_adsp_buffer_bank * 0x2000 + offset];
}

WRITE16_HANDLER( hd68k_adsp_buffer_w )
{
	COMBINE_DATA(&som_memory[m68k_adsp_buffer_bank * 0x2000 + offset]);
}



/*************************************
 *
 *	ADSP control memory handlers
 *
 *************************************/

static void deferred_adsp_bank_switch(int data)
{
#if LOG_COMMANDS
	if (m68k_adsp_buffer_bank != data && keyboard_pressed(KEYCODE_L))
	{
		static FILE *commands;
		if (!commands) commands = fopen("commands.log", "w");
		if (commands)
		{
			INT16 *base = (INT16 *)&som_memory[data * 0x2000];
			INT16 *end = base + (UINT16)*base++;
			INT16 *current = base;
			INT16 *table = base + (UINT16)*current++;

			fprintf(commands, "\n---------------\n");

			while ((current + 5) < table)
			{
				int offset = (int)(current - base);
				int c1 = *current++;
				int c2 = *current++;
				int c3 = *current++;
				int c4 = *current++;
				fprintf(commands, "Cmd @ %04X = %04X  %d-%d @ %d\n", offset, c1, c2, c3, c4);
				while (current < table)
				{
					UINT32 rslope, lslope;
					rslope = (UINT16)*current++,
					rslope |= *current++ << 16;
					if (rslope == 0xffffffff)
					{
						fprintf(commands, "  (end)\n");
						break;
					}
					lslope = (UINT16)*current++,
					lslope |= *current++ << 16;
					fprintf(commands, "  L=%08X R=%08X count=%d\n",
							(int)lslope, (int)rslope, (int)*current++);
				}
			}
			fprintf(commands, "\nTable:\n");
			current = table;
			while (current < end)
				fprintf(commands, "  %04X\n", *current++);
		}
	}
#endif
	m68k_adsp_buffer_bank = data;
}

WRITE16_HANDLER( hd68k_adsp_irq_clear_w )
{
	adsp_irq_state = 0;
	atarigen_update_interrupts();
}

WRITE16_HANDLER( hd68k_adsp_control_w )
{
	int val = (offset >> 3) & 1;

	switch (offset & 7)
	{
		case 0:
		case 1:
			/* LEDs */
			break;

		case 3:
			timer_set(TIME_NOW, val, deferred_adsp_bank_switch);
			break;

		case 5:
			/* connected to the /BR (bus request) line; this effectively halts */
			/* the ADSP at the next instruction boundary */
			adsp_br = !val;
			if (adsp_br || adsp_halt)
				cpu_set_halt_line(adsp_cpu, ASSERT_LINE);
			else
			{
				cpu_set_halt_line(adsp_cpu, CLEAR_LINE);
				/* a yield in this case is not enough */
				/* we would need to increase the interleaving otherwise */
				/* note that this only affects the test mode */
				cpu_spin();
			}
			break;

		case 6:
			/* connected to the /HALT line; this effectively halts */
			/* the ADSP at the next instruction boundary */
			adsp_halt = !val;
			if (adsp_br || adsp_halt)
				cpu_set_halt_line(adsp_cpu, ASSERT_LINE);
			else
			{
				cpu_set_halt_line(adsp_cpu, CLEAR_LINE);
				/* a yield in this case is not enough */
				/* we would need to increase the interleaving otherwise */
				/* note that this only affects the test mode */
				cpu_spin();
			}
			break;

		case 7:
			cpu_set_reset_line(adsp_cpu, val ? CLEAR_LINE : ASSERT_LINE);
			cpu_yield();
			break;

		default:
			logerror("ADSP control %02X = %04X\n", offset, data);
			break;
	}
}

READ16_HANDLER( hd68k_adsp_irq_state_r )
{
	int result = 0xfffd;
	if (adsp_xflag) result ^= 2;
	if (adsp_irq_state) result ^= 1;
	return result;
}


/*************************************
 *
 *	DS III I/O
 *
 *************************************/

WRITE16_HANDLER( hd68k_ds3_control_w )
{
	int val = (offset >> 3) & 1;

	switch (offset & 7)
	{
		case 2:
			/* connected to the /BR (bus request) line; this effectively halts */
			/* the ADSP at the next instruction boundary */
			adsp_br = !val;
			if (adsp_br)
				cpu_set_halt_line(adsp_cpu, ASSERT_LINE);
			else
			{
				cpu_set_halt_line(adsp_cpu, CLEAR_LINE);
				/* a yield in this case is not enough */
				/* we would need to increase the interleaving otherwise */
				/* note that this only affects the test mode */
				cpu_spin();
			}
			break;

		case 3:
			cpu_set_reset_line(adsp_cpu, val ? CLEAR_LINE : ASSERT_LINE);
			cpu_yield();
			break;

		case 7:
			/* LED */
			break;

		default:
			logerror("DS III control %02X = %04X\n", offset, data);
			break;
	}
}

READ16_HANDLER( hd68k_ds3_irq_state_r )
{
	int result = 0x0fff;
	if (ds3_g68flag) result ^= 0x8000;
	if (ds3_gflag) result ^= 0x4000;
	if (ds3_g68irqs) result ^= 0x2000;
	if (!adsp_irq_state) result ^= 0x1000;
	return result;
}


/*************************************
 *
 *	DS III internal I/O
 *
 *************************************/

static void update_ds3_irq(void)
{
	if ((ds3_g68flag || !ds3_g68irqs) && (!ds3_gflag || ds3_g68irqs))
		cpu_set_irq_line(adsp_cpu, ADSP2100_IRQ2, ASSERT_LINE);
	else
		cpu_set_irq_line(adsp_cpu, ADSP2100_IRQ2, CLEAR_LINE);
}

READ16_HANDLER( hdds3_special_r )
{
	int result;

	switch (offset)
	{
		case 0:
			ds3_g68flag = 0;
			update_ds3_irq();
			return ds3_g68data;

		case 1:
			result = 0x0fff;
			if (ds3_gcmd) result ^= 0x8000;
			if (ds3_g68flag) result ^= 0x4000;
			if (ds3_gflag) result ^= 0x2000;
			return result;

		case 6:
			return sim_memory[ds3_sim_address];
	}
	return 0;
}

WRITE16_HANDLER( hdds3_special_w )
{
	switch (offset)
	{
		case 0:
			ds3_gdata = data;
			ds3_gflag = 1;
			update_ds3_irq();
			break;

		case 1:
			adsp_irq_state = (data >> 9) & 1;
			update_interrupts();
			break;

		case 2:
			ds3_send = (data >> 8) & 1;
			break;

		case 3:
			ds3_g68irqs = (~data >> 9) & 1;
			break;

		case 4:
			ds3_sim_address = (ds3_sim_address & 0xffff0000) | (data & 0xffff);
			break;

		case 5:
			ds3_sim_address = (ds3_sim_address & 0xffff) | ((data << 16) & 0x00070000);
			break;
	}
}



/*************************************
 *
 *	DS III program memory handlers
 *
 *************************************/

READ16_HANDLER( hd68k_ds3_program_r )
{
	UINT32 *base = &adsp_pgm_memory[offset & 0x1fff];
	UINT32 word = *base;
	return (!(offset & 0x2000)) ? (word >> 16) : (word & 0xffff);
}

WRITE16_HANDLER( hd68k_ds3_program_w )
{
	UINT32 *base = &adsp_pgm_memory[offset & 0x1fff];
	UINT32 oldword = *base;
	UINT16 temp;

	if (!(offset & 0x2000))
	{
		temp = oldword >> 16;
		COMBINE_DATA(&temp);
		oldword = (oldword & 0x0000ffff) | (temp << 16);
	}
	else
	{
		temp = oldword & 0xffff;
		COMBINE_DATA(&temp);
		oldword = (oldword & 0xffff0000) | temp;
	}
	ADSP2100_WRPGM(base, oldword);
}



/*************************************
 *
 *	I/O write latches
 *
 *************************************/

READ16_HANDLER( hd68k_sound_reset_r )
{
	atarijsa_reset();
	return ~0;
}


WRITE16_HANDLER( hd68k_nwr_w )
{
	data = (offset >> 3) & 1;

	switch (offset & 7)
	{
		case 0:	/* CR2 */
			set_led_status(0, data);
/*			logerror("Write to CR2(%d)\n", data);*/
			break;
		case 1:	/* CR1 */
			set_led_status(1, data);
/*			logerror("Write to CR1(%d)\n", data);*/
			break;
		case 2:	/* LC1 */
/*			logerror("Write to LC1(%d)\n", data);*/
			break;
		case 3:	/* LC2 */
			logerror("Write to LC2(%d)\n", data);
			break;
		case 4:	/* ZP1 */
			m68k_zp1 = data;
			break;
		case 5:	/* ZP2 */
			m68k_zp2 = data;
			break;
		case 6:	/* /GSPRES */
			logerror("Write to /GSPRES(%d)\n", data);
			cpu_set_reset_line(gsp_cpu, data ? CLEAR_LINE : ASSERT_LINE);
			break;
		case 7:	/* /MSPRES */
			logerror("Write to /MSPRES(%d)\n", data);
			if (msp_cpu)
				cpu_set_reset_line(msp_cpu, data ? CLEAR_LINE : ASSERT_LINE);
			break;
	}
}



/*************************************
 *
 *	Input reading
 *
 *************************************/

READ16_HANDLER( hd68k_adc8_r )
{
	return adc8_data;
}


READ16_HANDLER( hd68k_adc12_r )
{
	return adc12_byte ? ((adc12_data >> 8) & 0x0f) : (adc12_data & 0xff);
}



/*************************************
 *
 *	ZRAM I/O
 *
 *************************************/

READ16_HANDLER( hd68k_zram_r )
{
	return atarigen_eeprom[offset];
}


WRITE16_HANDLER( hd68k_zram_w )
{
	if (m68k_zp1 == 0 && m68k_zp2 == 1)
		COMBINE_DATA(&atarigen_eeprom[offset]);
}



/*************************************
 *
 *	ADSP memory-mapped I/O
 *
 *************************************/

READ16_HANDLER( hdadsp_special_r )
{
	switch (offset)
	{
		case 0:	/* /SIMBUF */
			return sim_memory[adsp_eprom_base + adsp_sim_address++];

		case 1:	/* /SIMLD */
			break;

		case 2:	/* /SOMO */
			break;

		case 3:	/* /SOMLD */
			break;

		default:
			logerror("%04X:hdadsp_special_r(%04X)\n", cpu_getpreviouspc(), offset);
			break;
	}
	return 0;
}


WRITE16_HANDLER( hdadsp_special_w )
{
	switch (offset)
	{
		case 1:	/* /SIMCLK */
			adsp_sim_address = data;
			break;

		case 2:	/* SOMLATCH */
			som_memory[(m68k_adsp_buffer_bank ^ 1) * 0x2000 + (adsp_som_address++ & 0x1fff)] = data;
			break;

		case 3:	/* /SOMCLK */
			adsp_som_address = data;
			break;

		case 5:	/* /XOUT */
			adsp_xflag = data & 1;
			break;

		case 6:	/* /GINT */
			logerror("%04X:ADSP signals interrupt\n", cpu_getpreviouspc());
			adsp_irq_state = 1;
			atarigen_update_interrupts();
			break;

		case 7:	/* /MP */
			adsp_eprom_base = 0x10000 * data;
			break;

		default:
			logerror("%04X:hdadsp_special_w(%04X)=%04X\n", cpu_getpreviouspc(), offset, data);
			break;
	}
}


/*************************************
 *
 *	DUART memory handlers
 *
 *************************************/

/*
									DUART registers

			Read								Write
			----------------------------------	-------------------------------------------
	0x00 = 	Mode Register A (MR1A, MR2A) 		Mode Register A (MR1A, MR2A)
	0x02 =	Status Register A (SRA) 			Clock-Select Register A (CSRA)
	0x04 =	Clock-Select Register A 1 (CSRA) 	Command Register A (CRA)
	0x06 = 	Receiver Buffer A (RBA) 			Transmitter Buffer A (TBA)
	0x08 =	Input Port Change Register (IPCR) 	Auxiliary Control Register (ACR)
	0x0a = 	Interrupt Status Register (ISR) 	Interrupt Mask Register (IMR)
	0x0c = 	Counter Mode: Current MSB of 		Counter/Timer Upper Register (CTUR)
					Counter (CUR)
	0x0e = 	Counter Mode: Current LSB of 		Counter/Timer Lower Register (CTLR)
					Counter (CLR)
	0x10 = Mode Register B (MR1B, MR2B) 		Mode Register B (MR1B, MR2B)
	0x12 = Status Register B (SRB) 				Clock-Select Register B (CSRB)
	0x14 = Clock-Select Register B 2 (CSRB) 	Command Register B (CRB)
	0x16 = Receiver Buffer B (RBB) 				Transmitter Buffer B (TBB)
	0x18 = Interrupt-Vector Register (IVR) 		Interrupt-Vector Register (IVR)
	0x1a = Input Port (IP) 						Output Port Configuration Register (OPCR)
	0x1c = Start-Counter Command 3				Output Port Register (OPR): Bit Set Command 3
	0x1e = Stop-Counter Command 3				Output Port Register (OPR): Bit Reset Command 3
*/

INLINE double duart_clock_period(void)
{
	int mode = (duart_write_data[0x04] >> 4) & 7;
	if (mode != 3)
		logerror("DUART: unsupported clock mode %d\n", mode);
	return DUART_CLOCK * 16.0;
}


static void duart_callback(int param)
{
	logerror("DUART timer fired\n");
	if (duart_write_data[0x05] & 0x08)
	{
		logerror("DUART interrupt generated\n");
		duart_read_data[0x05] |= 0x08;
		duart_irq_state = (duart_read_data[0x05] & duart_write_data[0x05]) != 0;
		atarigen_update_interrupts();
	}
	duart_timer = timer_set(duart_clock_period() * 65536.0, 0, duart_callback);
}


READ16_HANDLER( hd68k_duart_r )
{
	switch (offset)
	{
		case 0x00:		/* Mode Register A (MR1A, MR2A) */
		case 0x08:		/* Mode Register B (MR1B, MR2B) */
			return (duart_write_data[0x00] << 8) | 0x00ff;
		case 0x01:		/* Status Register A (SRA) */
		case 0x02:		/* Clock-Select Register A 1 (CSRA) */
		case 0x03:		/* Receiver Buffer A (RBA) */
		case 0x04:		/* Input Port Change Register (IPCR) */
		case 0x05:		/* Interrupt Status Register (ISR) */
		case 0x06:		/* Counter Mode: Current MSB of Counter (CUR) */
		case 0x07:		/* Counter Mode: Current LSB of Counter (CLR) */
		case 0x09:		/* Status Register B (SRB) */
		case 0x0a:		/* Clock-Select Register B 2 (CSRB) */
		case 0x0b:		/* Receiver Buffer B (RBB) */
		case 0x0c:		/* Interrupt-Vector Register (IVR) */
		case 0x0d:		/* Input Port (IP) */
			return (duart_read_data[offset] << 8) | 0x00ff;
		case 0x0e:		/* Start-Counter Command 3 */
		{
			int reps = (duart_write_data[0x06] << 8) | duart_write_data[0x07];
			if (duart_timer)
				timer_remove(duart_timer);
			duart_timer = timer_set(duart_clock_period() * (double)reps, 0, duart_callback);
			logerror("DUART timer started (period=%f)\n", duart_clock_period() * (double)reps);
			return 0x00ff;
		}
		case 0x0f:		/* Stop-Counter Command 3 */
			if (duart_timer)
			{
				int reps = timer_timeleft(duart_timer) / duart_clock_period();
				timer_remove(duart_timer);
				duart_read_data[0x06] = reps >> 8;
				duart_read_data[0x07] = reps & 0xff;
				logerror("DUART timer stopped (final count=%04X)\n", reps);
			}
			duart_timer = NULL;
			duart_read_data[0x05] &= ~0x08;
			duart_irq_state = (duart_read_data[0x05] & duart_write_data[0x05]) != 0;
			atarigen_update_interrupts();
			return 0x00ff;
	}
	return 0x00ff;
}


WRITE16_HANDLER( hd68k_duart_w )
{
	if (ACCESSING_MSB)
	{
		int newdata = (data >> 8) & 0xff;
		duart_write_data[offset] = newdata;

		switch (offset)
		{
			case 0x00:		/* Mode Register A (MR1A, MR2A) */
			case 0x01:		/* Clock-Select Register A (CSRA) */
			case 0x02:		/* Command Register A (CRA) */
			case 0x03:		/* Transmitter Buffer A (TBA) */
			case 0x04:		/* Auxiliary Control Register (ACR) */
			case 0x05:		/* Interrupt Mask Register (IMR) */
			case 0x06:		/* Counter/Timer Upper Register (CTUR) */
			case 0x07:		/* Counter/Timer Lower Register (CTLR) */
			case 0x08:		/* Mode Register B (MR1B, MR2B) */
			case 0x09:		/* Clock-Select Register B (CSRB) */
			case 0x0a:		/* Command Register B (CRB) */
			case 0x0b:		/* Transmitter Buffer B (TBB) */
			case 0x0c:		/* Interrupt-Vector Register (IVR) */
			case 0x0d:		/* Output Port Configuration Register (OPCR) */
				break;
			case 0x0e:		/* Output Port Register (OPR): Bit Set Command 3 */
				duart_output_port |= newdata;
				break;
			case 0x0f:		/* Output Port Register (OPR): Bit Reset Command 3 */
				duart_output_port &= ~newdata;
				break;
		}
		logerror("DUART write %02X @ %02X\n", (data >> 8) & 0xff, offset);
	}
	else
		logerror("Unexpected DUART write %02X @ %02X\n", data, offset);
}



/*************************************
 *
 *	Misc. I/O and latches
 *
 *************************************/

WRITE16_HANDLER( hd68k_wr0_write )
{
	if (offset == 1) 		{ //	logerror("SEL1 low\n");
	} else if (offset == 2) { //	logerror("SEL2 low\n");
	} else if (offset == 3) { //	logerror("SEL3 low\n");
	} else if (offset == 4) { //	logerror("SEL4 low\n");
	} else if (offset == 6) { //	logerror("CC1 off\n");
	} else if (offset == 7) { //	logerror("CC2 off\n");
	} else if (offset == 9) { //	logerror("SEL1 high\n");
	} else if (offset == 10) { //	logerror("SEL2 high\n");
	} else if (offset == 11) { //	logerror("SEL3 high\n");
	} else if (offset == 12) { //	logerror("SEL4 high\n");
	} else if (offset == 14) { //	logerror("CC1 on\n");
	} else if (offset == 15) { //	logerror("CC2 on\n");
	} else { 				logerror("/WR1(%04X)=%02X\n", offset, data);
	}
}


WRITE16_HANDLER( hd68k_wr1_write )
{
	if (offset == 0) { //	logerror("Shifter Interface Latch = %02X\n", data);
	} else { 				logerror("/WR1(%04X)=%02X\n", offset, data);
	}
}


WRITE16_HANDLER( hd68k_wr2_write )
{
	if (offset == 0) { //	logerror("Steering Wheel Latch = %02X\n", data);
	} else { 				logerror("/WR2(%04X)=%02X\n", offset, data);
	}
}


WRITE16_HANDLER( hd68k_adc_control_w )
{
	COMBINE_DATA(&adc_control);

	/* handle a write to the 8-bit ADC address select */
	if (adc_control & 0x08)
	{
		adc8_select = adc_control & 0x07;
		adc8_data = readinputport(2 + adc8_select);
//		logerror("8-bit ADC select %d\n", adc8_select, data);
	}

	/* handle a write to the 12-bit ADC address select */
	if (adc_control & 0x40)
	{
		adc12_select = (adc_control >> 4) & 0x03;
		adc12_data = readinputport(10 + adc12_select) << 4;
//		logerror("12-bit ADC select %d\n", adc12_select);
	}
	adc12_byte = (adc_control >> 7) & 1;
}



/*************************************
 *
 *	Race Drivin' RAM handlers
 *
 *************************************/

READ16_HANDLER( hddsk_ram_r )
{
	return hddsk_ram[offset];
}


WRITE16_HANDLER( hddsk_ram_w )
{
	COMBINE_DATA(&hddsk_ram[offset]);
}


READ16_HANDLER( hddsk_zram_r )
{
	return hddsk_zram[offset];
}


WRITE16_HANDLER( hddsk_zram_w )
{
	COMBINE_DATA(&hddsk_zram[offset]);
}


READ16_HANDLER( hddsk_rom_r )
{
	return hddsk_rom[offset];
}



/*************************************
 *
 *	Race Drivin' slapstic handling
 *
 *************************************/

WRITE16_HANDLER( racedriv_68k_slapstic_w )
{
	slapstic_tweak(offset & 0x3fff);
}


READ16_HANDLER( racedriv_68k_slapstic_r )
{
	int bank = slapstic_tweak(offset & 0x3fff) * 0x4000;
	return hd68k_slapstic_base[bank + (offset & 0x3fff)];
}



/*************************************
 *
 *	Race Drivin' ASIC 65 handling
 *
 *	(ASIC 65 is actually a TMS32C015
 *	DSP chip clocked at 20MHz)
 *
 *************************************/

static UINT16 asic65_command;
static UINT16 asic65_param[32];
static UINT8  asic65_param_index;
static UINT16 asic65_result[32];
static UINT8  asic65_result_index;

static FILE * asic65_log;

WRITE16_HANDLER( racedriv_asic65_w )
{
	if (!asic65_log) asic65_log = fopen("asic65.log", "w");

	if (!ACCESSING_LSB || !ACCESSING_MSB)
		return;

	/* parameters go to offset 0 */
	if (offset == 0)
	{
		if (asic65_log) fprintf(asic65_log, " W=%04X", data);

		asic65_param[asic65_param_index++] = data;
		if (asic65_param_index >= 32)
			asic65_param_index = 32;
	}

	/* commands go to offset 1 */
	else
	{
		if (asic65_log) fprintf(asic65_log, "\n(%06X) %04X:", cpu_getpreviouspc(), data);

		asic65_command = data;
		asic65_result_index = asic65_param_index = 0;
	}

	/* update results */
	switch (asic65_command)
	{
		case 0x01:	/* reflect data */
			if (asic65_param_index >= 1)
			{
				asic65_result[0] = asic65_param[0];
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x02:	/* compute checksum (should be XX27) */
			asic65_result[0] = 0x0027;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x03:	/* get version (returns 1.3) */
			asic65_result[0] = 0x0013;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x04:	/* internal RAM test (result should be 0) */
			asic65_result[0] = 0;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x10:	/* matrix multiply */
			if (asic65_param_index >= 9+6)
			{
				INT64 element, result;

				element = (INT32)((asic65_param[9] << 16) | asic65_param[10]);
				result = element * (INT16)asic65_param[0] +
						 element * (INT16)asic65_param[1] +
						 element * (INT16)asic65_param[2];
				result >>= 14;
				asic65_result[0] = result >> 16;
				asic65_result[1] = result & 0xffff;

				element = (INT32)((asic65_param[11] << 16) | asic65_param[12]);
				result = element * (INT16)asic65_param[3] +
						 element * (INT16)asic65_param[4] +
						 element * (INT16)asic65_param[5];
				result >>= 14;
				asic65_result[2] = result >> 16;
				asic65_result[3] = result & 0xffff;

				element = (INT32)((asic65_param[13] << 16) | asic65_param[14]);
				result = element * (INT16)asic65_param[6] +
						 element * (INT16)asic65_param[7] +
						 element * (INT16)asic65_param[8];
				result >>= 14;
				asic65_result[4] = result >> 16;
				asic65_result[5] = result & 0xffff;
			}
			break;

		case 0x14:	/* ??? */
			if (asic65_param_index >= 1)
			{
				asic65_result[0] = asic65_param[0];
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x15:	/* ??? */
			if (asic65_param_index >= 1)
			{
				asic65_result[0] = asic65_param[0];
				asic65_result_index = asic65_param_index = 0;
			}
			break;
	}
}


READ16_HANDLER( racedriv_asic65_r )
{
	int result;

	if (!asic65_log) asic65_log = fopen("asic65.log", "w");
	if (asic65_log) fprintf(asic65_log, " (R=%04X)", asic65_result[asic65_result_index]);

	/* return the next result */
	result = asic65_result[asic65_result_index++];
	if (asic65_result_index >= 32)
		asic65_result_index = 32;
	return result;
}


READ16_HANDLER( racedriv_asic65_io_r )
{
	/* indicate that we always are ready to accept data and always ready to send */
	return 0x4000;
}



/*************************************
 *
 *	Race Drivin' ASIC 61 handling
 *
 *	(ASIC 61 is actually an AT&T/Lucent
 *	DSP32 chip clocked at 40MHz)
 *
 *************************************/

static UINT32 asic61_addr;
static FILE *asic61_log;

/* remove these when we get a real CPU to work with */
static UINT16 *asic61_ram_low;
static UINT16 *asic61_ram_mid;
static UINT16 *asic61_ram_high;

static WRITE16_HANDLER( asic61_w )
{
	if (!asic61_ram_low)
	{
		asic61_ram_low = malloc(2 * 0x1000);
		asic61_ram_mid = malloc(2 * 0x20000);
		asic61_ram_high = malloc(2 * 0x00800);
		if (!asic61_ram_low || !asic61_ram_mid || !asic61_ram_high)
			return;
	}
	if (offset >= 0x000000 && offset < 0x001000)
		asic61_ram_low[offset & 0xfff] = data;
	else if (offset >= 0x600000 && offset < 0x620000)
		asic61_ram_mid[offset & 0x1ffff] = data;
	else if (offset >= 0xfff800 && offset < 0x1000000)
		asic61_ram_high[offset & 0x7ff] = data;
	else
		logerror("Unexpected ASIC61 write to %06X\n", offset);
}


static READ16_HANDLER( asic61_r )
{
	if (!asic61_ram_low)
	{
		asic61_ram_low = malloc(2 * 0x1000);
		asic61_ram_mid = malloc(2 * 0x20000);
		asic61_ram_high = malloc(2 * 0x00800);
		if (!asic61_ram_low || !asic61_ram_mid || !asic61_ram_high)
			return 0;
	}
	if (offset >= 0x000000 && offset < 0x001000)
		return asic61_ram_low[offset & 0xfff];
	else if (offset >= 0x600000 && offset < 0x620000)
		return asic61_ram_mid[offset & 0x1ffff];
	else if (offset >= 0xfff800 && offset < 0x1000000)
		return asic61_ram_high[offset & 0x7ff];
	else
	{
		logerror("Unexpected ASIC61 read from %06X\n", offset);
		if (asic61_log) fprintf(asic61_log, "Unexpected ASIC61 read from %06X\n", offset);
	}
	return 0;
}


WRITE16_HANDLER( racedriv_asic61_w )
{
	if (!asic61_log) asic61_log = fopen("asic61.log", "w");

	switch (offset)
	{
		case 0x00:
//			if (asic61_log) fprintf(asic61_log, "%06X:mem addr lo = %04X\n", cpu_getpreviouspc(), data);
			asic61_addr = (asic61_addr & 0x00ff0000) | (data & 0xfffe);
			break;

		case 0x02:
//			if (asic61_log) fprintf(asic61_log, "%06X:mem W @ %08X=%04X\n", cpu_getpreviouspc(), asic61_addr, data);
			asic61_w(asic61_addr, data, mem_mask);
			asic61_addr = (asic61_addr & 0xffff0000) | ((asic61_addr+1) & 0x0000ffff);
			break;

		case 0x0b:
//			if (asic61_log) fprintf(asic61_log, "%06X:mem addr hi = %04X\n", cpu_getpreviouspc(), data);
			asic61_addr = ((data << 16) & 0x00ff0000) | (asic61_addr & 0xfffe);
			break;

		default:
			if (asic61_log) fprintf(asic61_log, "%06X:W@%02X = %04X\n", cpu_getpreviouspc(), offset, data);
			break;
	}
}


READ16_HANDLER( racedriv_asic61_r )
{
	UINT32 orig_addr = asic61_addr;

	if (!asic61_log) asic61_log = fopen("asic61.log", "w");

	switch (offset)
	{
		case 0x00:
//			if (asic61_log) fprintf(asic61_log, "%06X:read mem addr lo = %04X\n", cpu_getpreviouspc(), asic61_addr & 0xffff);
			return (asic61_addr & 0xfffe) | 1;

		case 0x02:
//			if (asic61_log) fprintf(asic61_log, "%06X:mem R @ %08X=%04X\n", cpu_getpreviouspc(), asic61_addr, asic61_r(asic61_addr));
			asic61_addr = (asic61_addr & 0xffff0000) | ((asic61_addr+1) & 0x0000ffff);

			/* special case: reading from 0x613e08 seems to be a flag */
			if (orig_addr == 0x613e08 && asic61_r(orig_addr,0) == 1)
				return 0;
			return asic61_r(orig_addr,0);

		default:
//			if (asic61_log) fprintf(asic61_log, "%06X:R@%02X\n", cpu_getpreviouspc(), offset);
			break;
	}
	return 0;
}



/*************************************
 *
 *	Steel Talons slapstic handling
 *
 *************************************/

static int steeltal_slapstic_tweak(offs_t offset)
{
	static int last_offset;
	static int bank = 0;

	if (last_offset == 0)
	{
		switch (offset)
		{
			case 0x78e8:
				bank = 0;
				break;
			case 0x6ca4:
				bank = 1;
				break;
			case 0x15ea:
				bank = 2;
				break;
			case 0x6b28:
				bank = 3;
				break;
		}
	}
	last_offset = offset;
	return bank;
}


WRITE16_HANDLER( steeltal_68k_slapstic_w )
{
	steeltal_slapstic_tweak(offset & 0x3fff);
}


READ16_HANDLER( steeltal_68k_slapstic_r )
{
	int bank = steeltal_slapstic_tweak(offset) * 0x4000;
	return hd68k_slapstic_base[bank + (offset & 0x3fff)];
}



/*************************************
 *
 *	ADSP Optimizations
 *
 *************************************/

#define READ_DATA(o) 	(adsp_data_memory[(o) & 0x3fff])
#ifndef ALIGN_SHORTS
#define READ_PGM(o)		(adsp_pgm_memory_word[((o) & 0x3fff) * 2])
#else
#define READ_PGM(o)		((adsp_pgm_memory[(o) & 0x3fff] >> 8) & 0xffff)
#endif


READ16_HANDLER( hdadsp_speedup_r )
{
	int data = READ_DATA(0x1fff);

	if (data == 0xffff && cpu_get_pc() <= 0x14 && cpu_getactivecpu() == adsp_cpu)
		cpu_spinuntil_trigger(554433);

	return data;
}


READ16_HANDLER( hdadsp_speedup2_r )
{
	UINT32 i5 = READ_DATA(0x0958);

	int pc = cpu_get_pc();
	if (pc >= 0x07c7 && pc <= 0x7d5)
	{
		UINT32 i4 = cpu_get_reg(ADSP2100_I4);
		INT32 ay = (READ_PGM(i4++) << 16) | (UINT16)cpu_get_reg(ADSP2100_AY1);
		INT32 sr = (cpu_get_reg(ADSP2100_SR1) << 16) | (UINT16)cpu_get_reg(ADSP2100_SR0);
		INT32 last_ay;

		if (ay < sr)
		{
			do
			{
				last_ay = ay;
				i4 = (UINT16)READ_PGM(i4);
				i5++;
				ay = (UINT16)READ_PGM(i4) | (READ_PGM(i4 + 1) << 16);
				i4 += 2;
			} while (sr >= last_ay);
			cpu_set_reg(ADSP2100_I4, i4);
			cpu_set_reg(ADSP2100_PC, pc + 0x7d0 - 0x7c7);
		}
		else
		{
			i4++;
			do
			{
				last_ay = ay;
				i4 = (UINT16)READ_PGM(i4);
				i5++;
				ay = (UINT16)READ_PGM(i4) | (READ_PGM(i4 + 1) << 16);
				i4 += 3;
			} while (last_ay >= sr);
			cpu_set_reg(ADSP2100_I4, i4);
			cpu_set_reg(ADSP2100_AR, i4 - 3);
			cpu_set_reg(ADSP2100_M4, 2);
			cpu_set_reg(ADSP2100_PC, pc + 0x7ec - 0x7c7);
		}
	}

	return i5;
}

#ifndef ALIGN_SHORTS
#define MATRIX(n)	(*(INT16 *)((UINT8 *)matrix + (n)*4 + 1))
#else
#define MATRIX(n)	((INT16)((matrix[n] >> 8) & 0xffff))
#endif

WRITE16_HANDLER( hdadsp_speedup2_w )
{
	int se = (INT8)cpu_get_reg(ADSP2100_SE);
	UINT32 pc = cpu_get_pc();
	data32_t *matrix;
	INT16 *vector;
	INT16 *trans;
	INT16 *result;
	INT64 mr;
	int count;

	COMBINE_DATA(&adsp_data_memory[0x0033]);
	if (pc != hdadsp_speedup_pc)
		return;
	adsp2100_icount -= 8;

	matrix = &adsp_pgm_memory[0x1010];
	trans  = (INT16 *)&adsp_data_memory[0x000c];
	result = (INT16 *)&adsp_data_memory[0x010d];
	vector = (INT16 *)&sim_memory[adsp_eprom_base + adsp_sim_address];

	count = READ_DATA(0x0032);
	adsp2100_icount -= (9 + 6*3) * count;
	while (count--)
	{
		INT16 temp[3];

		if (se < 0)
		{
			temp[0] = vector[0] >> -se;
			temp[1] = vector[1] >> -se;
			temp[2] = vector[2] >> -se;
		}
		else
		{
			temp[0] = vector[0] << se;
			temp[1] = vector[1] << se;
			temp[2] = vector[2] << se;
		}

		mr = temp[0] * MATRIX(0);
		mr += temp[1] * MATRIX(1);
		mr += temp[2] * MATRIX(2);
		*result++ = ((INT32)mr >> (16 - 1)) + trans[0];

		mr = temp[0] * MATRIX(3);
		mr += temp[1] * MATRIX(4);
		mr += temp[2] * MATRIX(5);
		*result++ = ((INT32)mr >> (16 - 1)) + trans[1];

		mr = temp[0] * MATRIX(6);
		mr += temp[1] * MATRIX(7);
		mr += temp[2] * MATRIX(8);
		*result++ = ((INT32)mr >> (16 - 1)) + trans[2];

		vector += 3;
	}

	count = READ_DATA(0x08da);
	adsp2100_icount -= (9 + 9*3) * count;
	while (count--)
	{
		mr = vector[0] * MATRIX(0);
		mr += vector[1] * MATRIX(1);
		mr += vector[2] * MATRIX(2);
		*result++ = ((INT32)(mr << 1) + 0x4000) >> 15;

		mr = vector[0] * MATRIX(3);
		mr += vector[1] * MATRIX(4);
		mr += vector[2] * MATRIX(5);
		*result++ = ((INT32)(mr << 1) + 0x4000) >> 15;

		mr = vector[0] * MATRIX(6);
		mr += vector[1] * MATRIX(7);
		mr += vector[2] * MATRIX(8);
		*result++ = ((INT32)(mr << 1) + 0x4000) >> 15;

		vector += 3;
	}
	cpu_set_reg(ADSP2100_SI, *vector++);
	adsp_sim_address = ((data16_t *)vector - sim_memory) - adsp_eprom_base;

	cpu_set_reg(ADSP2100_PC, pc + 0x165 - 0x139);
}



/*************************************
 *
 *	GSP Optimizations
 *
 *************************************/

READ16_HANDLER( hdgsp_speedup_r )
{
	int result = hdgsp_speedup_addr[0][offset];

	if (offset != 0 || result == 0xffff)
		return result;

	if (cpu_get_pc() == hdgsp_speedup_pc && hdgsp_speedup_addr[1][0] != 0xffff && cpu_getactivecpu() == gsp_cpu)
		cpu_spinuntil_int();

	return result;
}


WRITE16_HANDLER( hdgsp_speedup1_w )
{
	COMBINE_DATA(&hdgsp_speedup_addr[0][offset]);
	if (offset == 0 && hdgsp_speedup_addr[0][offset] == 0xffff)
		cpu_trigger(-2000 + gsp_cpu);
}


WRITE16_HANDLER( hdgsp_speedup2_w )
{
	COMBINE_DATA(&hdgsp_speedup_addr[1][offset]);
	if (offset == 0 && hdgsp_speedup_addr[1][offset] == 0xffff)
		cpu_trigger(-2000 + gsp_cpu);
}



/*************************************
 *
 *	MSP Optimizations
 *
 *************************************/

READ16_HANDLER( hdmsp_speedup_r )
{
	int result = hdmsp_speedup_addr[offset];

	if (offset != 0 || result != 0)
		return result;

	if (cpu_get_pc() == hdmsp_speedup_pc && cpu_getactivecpu() == msp_cpu)
		cpu_spinuntil_int();

	return result;
}


WRITE16_HANDLER( hdmsp_speedup_w )
{
	COMBINE_DATA(&hdmsp_speedup_addr[offset]);
	if (offset == 0 && hdmsp_speedup_addr[offset] != 0)
		cpu_trigger(-2000 + msp_cpu);
}
