/****************************************************************************
 *   Twin Cobra																*
 *   Communications and memory functions between shared CPU memory spaces	*
 ****************************************************************************/

#include "driver.h"
#include "cpu/tms32010/tms32010.h"


unsigned char *twincobr_68k_dsp_ram;
unsigned char *twincobr_7800c;
unsigned char *twincobr_sharedram;

extern int twincobr_fg_rom_bank;
extern int twincobr_bg_ram_bank;
extern int twincobr_display_on;
extern int twincobr_flip_screen;

static int dsp_exec = 0;
static unsigned int dsp_addr_w = 0;
int intenable = 0;



int twincobr_dsp_in(int offset)
{
	/* DSP can read data from 68000 main RAM via DSP IO port 1 */

	unsigned int input_data = 0;
	input_data = READ_WORD(&twincobr_68k_dsp_ram[dsp_addr_w]);
	if (errorlog) fprintf(errorlog,"DSP PC:%04x IO read %08x at %08x (port 1)\n",cpu_getpreviouspc(),input_data,dsp_addr_w + 0x30000);
	return input_data;
}

void twincobr_dsp_out(int fnction,int data)
{
	if (fnction == 0) {
		/* This sets the 68000 main RAM address the DSP should */
		/*		read/write, via the DSP IO port 0 */
		/* Top three bits of data need to be shifted left 3 places */
		/*		to select which 64K memory bank from 68000 space to use */
		/*		Here though, it only uses memory bank 0x03xxxx, so they */
		/*		are masked for simplicity */
		/* All of this data is shifted left one position to move it */
		/*		to the 68000 word boundary equivalent */

		dsp_addr_w = ((data & 0x01fff) << 1);
		if (errorlog) fprintf(errorlog,"DSP PC:%04x IO write %04x (%06x) at port 0\n",cpu_getpreviouspc(),data,dsp_addr_w);
	}
	if (fnction == 1) {
		/* Data written to 68000 main RAM via DSP IO port 1*/
		if ((data == 0) && (dsp_addr_w <= 2)) dsp_exec = 1;
		else dsp_exec = 0;
		WRITE_WORD(&twincobr_68k_dsp_ram[dsp_addr_w],data);
		if (errorlog) fprintf(errorlog,"DSP PC:%04x IO write %04x at %06x (port 1)\n",cpu_getpreviouspc(),data,dsp_addr_w + 0x30000);
	}
	if (fnction == 3) {
		/* data 0xffff means inhibit BIO line to DSP and enable */
		/*		communication to 68000 processor */
		/*		Actually only DSP data bit 15 controls this */
		/* data 0x0000 means set DSP BIO line active and disable */
		/*		communication to 68000 processor*/
		if (errorlog) fprintf(errorlog,"DSP PC:%04x IO write %04x at port 3\n",cpu_getpreviouspc(),data);
		if (data & 0x8000) {
#if NEW_INTERRUPT_SYSTEM
		cpu_set_irq_line(2, TMS320C10_ACTIVE_BIO, CLEAR_LINE);
#else
		cpu_cause_interrupt(2,TMS320C10_IGNORE_BIO);
#endif
		}
		else {
			if (dsp_exec) {
				if (errorlog) fprintf(errorlog,"Turning 68000 on\n");
				cpu_halt(0,1);
				dsp_exec = 0;
			}
#if NEW_INTERRUPT_SYSTEM
		cpu_set_irq_line(2, TMS320C10_ACTIVE_BIO, ASSERT_LINE);
#else
		cpu_cause_interrupt(2,TMS320C10_ACTIVE_BIO);
#endif
		}
	}
}

int twincobr_68k_dsp_r(int offset)
{
	return READ_WORD(&twincobr_68k_dsp_ram[offset]);
}

void twincobr_68k_dsp_w(int offset,int data)
{
	if (errorlog) if (offset < 9) fprintf(errorlog,"68K:%06x write %08x at %08x\n",cpu_getpreviouspc(),data,0x30000+offset);
	COMBINE_WORD_MEM(&twincobr_68k_dsp_ram[offset],data);
}

int twincobr_7800c_r(int offset)
{
	return READ_WORD(&twincobr_7800c[offset]);
}

void twincobr_7800c_w(int offset,int data)
{
	WRITE_WORD(&twincobr_7800c[offset],data);
	if (errorlog) fprintf(errorlog,"68K Writing %08x to %06x.\n",data,0x7800c + offset);
	switch (data) {
		case 0x0004: intenable = 0; break;
		case 0x0005: intenable = 1; break;
		case 0x0006: twincobr_flip_screen = 0; break;
		case 0x0007: twincobr_flip_screen = 1; break;
		case 0x0008: twincobr_bg_ram_bank = 0x0000; break;
		case 0x0009: twincobr_bg_ram_bank = 0x2000; break;
		case 0x000a: twincobr_fg_rom_bank = 0x0000; break;
		case 0x000b: twincobr_fg_rom_bank = 0x1000; break;
		case 0x000e: twincobr_display_on  = 0x0000; break; /* Turn display off */
		case 0x000f: twincobr_display_on  = 0x0001; break; /* Turn display on */
		case 0x000c: if (twincobr_display_on) {
				/* This means assert the INT line to the DSP */
				if (errorlog) fprintf(errorlog,"Turning DSP on and 68000 off\n");
				cpu_halt(2,1);
#if NEW_INTERRUPT_SYSTEM
				cpu_set_irq_line(2, TMS320C10_ACTIVE_INT, ASSERT_LINE);
#else
				cpu_cause_interrupt(2,TMS320C10_ACTIVE_INT);
#endif
				cpu_halt(0,0);
			}
			break;
		case 0x000d: if (twincobr_display_on) {
				/* This means inhibit the INT line to the DSP */
				if (errorlog) fprintf(errorlog,"Turning DSP off\n");
#if NEW_INTERRUPT_SYSTEM
				cpu_set_irq_line(2, TMS320C10_ACTIVE_INT, CLEAR_LINE);
#else
				cpu_clear_pending_interrupts(2)
#endif
				cpu_halt(2,0);
			}
			break;
	}
}


int twincobr_sharedram_r(int offset)
{
	return twincobr_sharedram[offset / 2];
}

void twincobr_sharedram_w(int offset,int data)
{
	twincobr_sharedram[offset / 2] = data;
}
