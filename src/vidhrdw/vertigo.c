/*************************************************************************

    Exidy Vertigo hardware

 The Vertigo vector CPU consists of four AMD 2901 bit slice
 processors. The microcode for these CPUs is stored in 13 bipolar
 proms for a total of 512 52 bit wide micro instructions. The
 microcode not only crontrols the 2901s but also loading and storing
 of operands and results, program flow control and vector generation.


 55 444444 4444 3333 333333222 2 2 2 2 222 11 11 1 1 11110 000000000
 21 987654 3210 9876 543210987 6 5 4 3 210 98 76 5 4 32109 876543210

    xxxxxx aaaa bbbb iiiiiiiii c m r r ooo ii oo   j jjjjj mmmmmmmmm
    543210 3210 3210 876543210 n r s w fff ff aa   p 43210 aaaaaaaaa
                                 e e r 210 10 10   o       876543210
                                 q l i             s
                                     t
                                     e
 x: 64 words of 16 bit wide SRAM
 a: A register index
 b: B register index
 i: 2901 instruction
 cn: carry bit
 mreq, rsel, rwrite: signals for memory access
 of: vector generator
 if: vector RAM/ROM data select
 oa: vector RAM/ROM address select
 jpos: jump condition inverted
 j: jump condition and type
 m: jump address

*************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "vertigo.h"

static UINT16 *vertigo_vectorrom;
static UINT64 *mcode;

UINT16 *vertigo_vectorram;

#define MC_X      ((mcode[vs.pc] >> 44) & 0x3f)
#define MC_A      ((mcode[vs.pc] >> 40) & 0xf)
#define MC_B      ((mcode[vs.pc] >> 36) & 0xf)

#define MC_DST    ((mcode[vs.pc] >> 33) & 0x7)
#define QREG  0
#define NOP   1
#define RAMA  2
#define RAMF  3
#define RAMQD 4
#define RAMD  5
#define RAMQU 6
#define RAMU  7

#define MC_SRC_FUNC ((mcode[vs.pc] >> 27) & 077)
#define MC_CN     ((mcode[vs.pc] >> 26) & 0x1)
#define MC_MREQ   ((mcode[vs.pc] >> 25) & 0x1)
#define MC_RSEL   (MC_RWRITE & ((mcode[vs.pc] >> 24) & 0x1))
#define MC_RWRITE ((mcode[vs.pc] >> 23) & 0x1)
#define MC_RAMACC (MC_RWRITE+MC_RSEL)
#define S_RAMW 0
#define S_RAMR 1
#define S_RAM0 2

#define MC_OF     ((mcode[vs.pc] >> 20) & 0x7)
#define MC_IF     ((mcode[vs.pc] >> 18) & 0x3)
#define S_ROMDE 0
#define S_RAMDE 1

#define MC_OA     ((mcode[vs.pc] >> 16) & 0x3)
#define S_SREG 0
#define S_ROMA 1
#define S_RAMD 2

#define MC_JPOS   ((mcode[vs.pc] >> 14) & 0x1)
#define MC_JMP    ((mcode[vs.pc] >> 12) & 0x3)
#define MC_JCON   ((mcode[vs.pc] >> 9) & 0x7)
#define MC_MA     ( mcode[vs.pc] & 0x1ff)

#define ADD(r,s)   f = (r + s + MC_CN) & 0xffff
#define SUBR(r,s)  f = (~r  + s + MC_CN) & 0xffff
#define SUBS(r,s)  f = (r + ~s  + MC_CN) & 0xffff
#define OR(r,s)    f = r | s
#define AND(r,s)   f = r & s
#define NOTRS(r,s) f = ~r & s
#define EXOR(r,s)  f = r ^ s
#define EXNOR(r,s) f = ~(r ^ s)


static struct vproc_state
{
	int reg[16];
	int ram[64];
	int ramff;
	int pc;
} vs;

void vertigo_vproc_init(void)
{
	int i;

	vertigo_vectorrom = (UINT16 *)memory_region(REGION_USER1);
	mcode = (UINT64 *)memory_region(REGION_PROMS);

	for (i=0; i<16; i++)
		vs.reg[i]=0;

	for (i=0; i<64; i++)
		vs.ram[i]=0;

	vs.ramff = 0;
	vs.pc = 0;
}

static void v_update (int c_h, int c_v, int color, int intensity)
{
	vector_add_point (c_h << 15, (1800-c_v) << 15, VECTOR_COLOR444(color), intensity);
}

void vertigo_vproc(int cycles, int irq4)
{
	int d, s, r, q, f, ret_adr, rom_adr, y, jcond, sreg;
	int color, intensity, l1, l2, c_v, c_h, c_l;
	int adder_s, adder_a;
	int brez;
	int vfin;
	int hud1, hud2, vud1, vud2, hc1;

	d = r = s = f = y = q = 0;
	adder_s = adder_a = 0;
	hud1 = hud2 = vud1 = vud2 = hc1 = 0;
	c_h = c_v = l1 = l2 = color = c_l = intensity = 0;
	sreg=0;
	brez = 0;
	rom_adr = 0;
	ret_adr = 0;
	jcond = 0;
	vfin = 0;

	if (irq4)
		vector_clear_list();

	while (cycles)
	{
		cycles--;

		switch (MC_IF)
		{
		case S_RAMDE:
			d = vs.ramff;
			break;
		case S_ROMDE:
			if (rom_adr < 0x2000)
				d = vertigo_vectorram[rom_adr & 0xfff];
			else
				d = vertigo_vectorrom[rom_adr & 0x7fff];
			break;
		}

		switch(MC_RAMACC)
		{
		case S_RAMR:
			d = vs.ram[MC_X];
			break;
		case S_RAMW:
			vs.ram[MC_X]=d;
			break;
		}

		switch (MC_SRC_FUNC)
		{
		case 000: ADD(vs.reg[MC_A], q); break;
		case 001: ADD(vs.reg[MC_A], vs.reg[MC_B]); break;
		case 002: ADD(0, q); break;
		case 003: ADD(0, vs.reg[MC_B]); break;
		case 004: ADD(0, vs.reg[MC_A]); break;
		case 005: ADD(d, vs.reg[MC_A]); break;
		case 006: ADD(d, q); break;
		case 007: ADD(d, 0); break;

		case 010: SUBR(vs.reg[MC_A], q); break;
		case 011: SUBR(vs.reg[MC_A], vs.reg[MC_B]); break;
		case 012: SUBR(0, q); break;
		case 013: SUBR(0, vs.reg[MC_B]); break;
		case 014: SUBR(0, vs.reg[MC_A]); break;
		case 015: SUBR(d, vs.reg[MC_A]); break;
		case 016: SUBR(d, q); break;
		case 017: SUBR(d, 0); break;

		case 020: SUBS(vs.reg[MC_A], q); break;
		case 021: SUBS(vs.reg[MC_A], vs.reg[MC_B]); break;
		case 022: SUBS(0, q); break;
		case 023: SUBS(0, vs.reg[MC_B]); break;
		case 024: SUBS(0, vs.reg[MC_A]); break;
		case 025: SUBS(d, vs.reg[MC_A]); break;
		case 026: SUBS(d, q); break;
		case 027: SUBS(d, 0); break;

		case 030: OR(vs.reg[MC_A], q); break;
		case 031: OR(vs.reg[MC_A], vs.reg[MC_B]); break;
		case 032: OR(0, q); break;
		case 033: OR(0, vs.reg[MC_B]); break;
		case 034: OR(0, vs.reg[MC_A]); break;
		case 035: OR(d, vs.reg[MC_A]); break;
		case 036: OR(d, q); break;
		case 037: OR(d, 0); break;

		case 040: AND(vs.reg[MC_A], q); break;
		case 041: AND(vs.reg[MC_A], vs.reg[MC_B]); break;
		case 042: AND(0, q); break;
		case 043: AND(0, vs.reg[MC_B]); break;
		case 044: AND(0, vs.reg[MC_A]); break;
		case 045: AND(d, vs.reg[MC_A]); break;
		case 046: AND(d, q); break;
		case 047: AND(d, 0); break;

		case 050: NOTRS(vs.reg[MC_A], q); break;
		case 051: NOTRS(vs.reg[MC_A], vs.reg[MC_B]); break;
		case 052: NOTRS(0, q); break;
		case 053: NOTRS(0, vs.reg[MC_B]); break;
		case 054: NOTRS(0, vs.reg[MC_A]); break;
		case 055: NOTRS(d, vs.reg[MC_A]); break;
		case 056: NOTRS(d, q); break;
		case 057: NOTRS(d, 0); break;

		case 060: EXOR(vs.reg[MC_A], q); break;
		case 061: EXOR(vs.reg[MC_A], vs.reg[MC_B]); break;
		case 062: EXOR(0, q); break;
		case 063: EXOR(0, vs.reg[MC_B]); break;
		case 064: EXOR(0, vs.reg[MC_A]); break;
		case 065: EXOR(d, vs.reg[MC_A]); break;
		case 066: EXOR(d, q); break;
		case 067: EXOR(d, 0); break;

		case 070: EXNOR(vs.reg[MC_A], q); break;
		case 071: EXNOR(vs.reg[MC_A], vs.reg[MC_B]); break;
		case 072: EXNOR(0, q); break;
		case 073: EXNOR(0, vs.reg[MC_B]); break;
		case 074: EXNOR(0, vs.reg[MC_A]); break;
		case 075: EXNOR(d, vs.reg[MC_A]); break;
		case 076: EXNOR(d, q); break;
		case 077: EXNOR(d, 0); break;
		}

		switch (MC_DST)
		{
		case QREG:
			q = f;
			y = f;
			break;
		case NOP:
			y = f;
			break;
		case RAMA:
			y = vs.reg[MC_A];
			vs.reg[MC_B] = f;
			break;
		case RAMF:
			y = f;
			vs.reg[MC_B] = f;
			break;
		case RAMQD:
			y = f;
			q = (q>>1) & 0x7fff;              // Q3 is low
			vs.reg[MC_B] = (f >> 1) | 0x8000; // IN3 is high!
			break;
		case RAMD:
			y = f;
			vs.reg[MC_B] = (f >> 1) | 0x8000; // IN3 is high!
			break;
		case RAMQU:
			y = f;
			vs.reg[MC_B] = (f << 1) & 0xffff;
			q = (q << 1) & 0xffff;
			break;
		case RAMU:
			y = f;
			vs.reg[MC_B] = (f << 1) & 0xffff;
			break;
		}

		switch (MC_OA)
		{
		case S_RAMD:
			vs.ramff = y;
			if (MC_RAMACC==S_RAMW && MC_IF==S_RAMDE)
				vs.ram[MC_X] = vs.ramff;
			break;
		case S_ROMA:
			rom_adr = y;
			break;
		case S_SREG:
			sreg = (sreg >> 1) | ((f >> 9) & 4);
			break;
		default:
			break;
		}

		switch (MC_OF)
		{
		case 0:
			if (vfin && (color != (y & 0xfff)))
				v_update (c_h, c_v, color, intensity);
			color = y & 0xfff;
			break;
		case 1:
			if (vfin && (intensity != (y & 0xff)))
				v_update (c_h, c_v, color, intensity);
			intensity = y & 0xff;
			break;
		case 2:
			l1 = y & 0xfff;
			adder_s = 0;
			adder_a = l2;
			hud1 = sreg & 1;
			vud1 = sreg & 2;
			hc1  = sreg & 4;
			brez = 1;
			break;
		case 3:
			l2 = y & 0xfff;
			adder_s = (adder_s + adder_a) & 0xfff;
			if (adder_s & 0x800)
				adder_a = l1;
			else
				adder_a = l2;
			hud2 = sreg & 1;
			vud2 = sreg & 2;
			break;
		case 4:
			c_v = y & 0xfff;
			if (vfin)
				v_update (c_h, c_v, color, intensity);
			else
				v_update (c_h, c_v, 0, 0);
			break;
		case 5:
			c_h = y & 0xfff;
			break;
		case 6:
			c_l = y & 0xfff;
			/* Loading the c_l counter starts
             * drawing of a vector if MSB is set
             */
			break;
		default:
			break;
		}

		switch (MC_JCON)
		{
		case 0:
			jcond = 1;
			break;
		case 1:
			jcond = (f >> 15) & 1;  // ALU most significant bit
			break;
		case 2:
			jcond = (f==0)? 1 : 0;  // ALU is 0
			break;
		case 3:
			jcond = (y >> 10) & 1;  // Y10
			break;
		case 4:
			jcond = vfin;           // Vector finished drawing
			break;
		case 5:
			jcond = (f >> 11) & 1;  // FPOS bit #11
			break;
		case 6:
			jcond = irq4;           // intl4
			if (MC_JPOS != irq4)
				cycles=0;           // intl4 speed hack
			break;
		case 7:
			jcond = 1;
			break;
		}

		jcond ^= MC_JPOS;

		if (jcond)
		{
			switch (MC_JMP)
			{
			case 0:
				vs.pc = MC_MA;  // jbk jump to bank indicated by MA8
				break;
			case 1:
				ret_adr = vs.pc + 1;
				vs.pc = MC_MA;  // call and store return address
				break;
			case 2:             // OPT used for microcode jump tables
				vs.pc = (MC_MA & 0x1f0) | ((d >> 12) & 0xf);
				break;
			case 3:
				vs.pc = ret_adr; // return from call
				break;
			}
		}
		else
			vs.pc++;

		if (c_l & 0x800)
		{
			vfin = 1;
			c_l = (c_l+1) & 0xfff;

			if ((c_l & 0x800) == 0)
			{
				brez = 0;
				vfin = 0;
				v_update (c_h, c_v, color, intensity);
			}

			if (brez) /* H/V counter enabled */
			{
				/* Depending on MSB of adder only one or both
                   counters are de-/incremented. This is all
                   defined by the shift register which is again
                   latched with L1/L2.
                */
				if (adder_s & 0x800)
				{
					if (hc1)
						c_h += hud1? -1: 1;
					else
						c_v += vud1? -1: 1;
					adder_a = l1;
				}
				else
				{
					c_h += hud2? -1: 1;
					c_v += vud2? -1: 1;
					adder_a = l2;
				}

				/* H/V counter are 12 bit. Counting
                   0 down gives 0.
                */
				c_v = c_v < 0? 0: c_v & 0xfff;
				c_h = c_h < 0? 0: c_h & 0xfff;
			}

			adder_s = (adder_s + adder_a) & 0xfff;
		}
	}
}
