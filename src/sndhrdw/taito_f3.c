#include "driver.h"

static int counter,vector_reg,imr_status;
static data16_t es5510_dsp_ram[0x200];
static data32_t	es5510_gpr[0xc0];
static data32_t	es5510_gpr_latch;
static void *timer_68681=NULL;
extern data32_t *f3_shared_ram;
static int timer_mode;

static int es_tmp=1;

#define M68000_CLOCK	16000000
#define M68681_CLOCK	4000000

enum { TIMER_SINGLESHOT, TIMER_PULSE };

READ16_HANDLER(f3_68000_share_r)
{
	if ((offset&3)==0) return (f3_shared_ram[offset/4]&0xff000000)>>16;
	if ((offset&3)==1) return (f3_shared_ram[offset/4]&0x00ff0000)>>8;
	if ((offset&3)==2) return (f3_shared_ram[offset/4]&0x0000ff00)>>0;
	return (f3_shared_ram[offset/4]&0x000000ff)<<8;
}

WRITE16_HANDLER(f3_68000_share_w)
{
	if ((offset&3)==0) f3_shared_ram[offset/4]=(f3_shared_ram[offset/4]&0x00ffffff)|((data&0xff00)<<16);
	else if ((offset&3)==1) f3_shared_ram[offset/4]=(f3_shared_ram[offset/4]&0xff00ffff)|((data&0xff00)<<8);
	else if ((offset&3)==2) f3_shared_ram[offset/4]=(f3_shared_ram[offset/4]&0xffff00ff)|((data&0xff00)<<0);
	else f3_shared_ram[offset/4]=(f3_shared_ram[offset/4]&0xffffff00)|((data&0xff00)>>8);
}

static void timer_callback(int param)
{
	if (timer_mode==TIMER_SINGLESHOT) timer_68681=NULL;
	cpu_irq_line_vector_w(1, 7, vector_reg);
	cpu_set_irq_line(1, 7, ASSERT_LINE);
	imr_status|=0x8;
}

void f3_68681_reset(void)
{
	if (timer_68681) {
		timer_remove(timer_68681);
		timer_68681=NULL;
	}
}

READ16_HANDLER(f3_68681_r)
{
	if (offset==0x5) {
		int ret=imr_status;
		imr_status=0;

//		logerror("%06x: 68681 read offset %04x (%04x)\n",cpu_get_pc(),offset,ret);
		return ret;
	}
//	logerror("%06x: 68681 read offset %04x\n",cpu_get_pc(),offset);

	if (offset==0xe)
		return 1;

	/* IRQ ack */
	if (offset==0xf) {
		cpu_set_irq_line(1, 7, CLEAR_LINE);
		return 0;
	}

	return 0xff;
}
//c109e8: 68681 read offset 001c is end of init sequence
WRITE16_HANDLER(f3_68681_w)
{
	switch (offset) {
		case 0x04: /* ACR */
			logerror("68681:  %02x %02x\n",offset,data&0xff);
			if ((data&0xff)==0x30) {
				logerror("68681:  Counter started: %d counts, tick is X1/16, fires in %d 68k cycles\n",counter,(M68000_CLOCK/M68681_CLOCK)*counter);
				if (timer_68681) timer_remove(timer_68681);
//				timer_68681=timer_pulse(TIME_IN_CYCLES(1000000/48,1), 0, timer_callback);
				timer_68681=timer_set(TIME_IN_CYCLES((M68000_CLOCK/M68681_CLOCK)*counter*4,1), 0, timer_callback);
//timer_68681=timer_set(TIME_IN_CYCLES(1000000/16,1), 0, timer_callback);
				timer_mode=TIMER_SINGLESHOT;
		} else if ((data&0xff)==0x60) {
				logerror("68681:  Timer started: %d counts, tick is X1\n",counter);
				if (timer_68681) timer_remove(timer_68681);
				timer_68681=timer_pulse(TIME_IN_CYCLES(1000000/50,1), 0, timer_callback);
//this time is wrong..
//timer_68681=timer_set(TIME_IN_CYCLES(1000000/16,1), 0, timer_callback);
				timer_mode=TIMER_PULSE;
			} else {
				logerror("68681:  %02x %02x - Unsupported timer mode\n",offset,data&0xff);
			}
			break;

		case 0x05: /* IMR */
			logerror("68681:  %02x %02x\n",offset,data&0xff);
			break;

		case 0x06: /* CTUR */
			counter=((data&0xff)<<8)|(counter&0xff);
			break;
		case 0x07: /* CTLR */
			counter=(counter&0xff00)|(data&0xff);
			break;
		case 0x08: break; /* MR1B (Mode register B) */
		case 0x09: break; /* CSRB (Clock select register B) */
		case 0x0a: break; /* CRB (Command register B) */
		case 0x0b: break; /* TBB (Transmit buffer B) */
		case 0x0c: /* IVR (Interrupt vector) */
			vector_reg=data&0xff;
			break;
		default:
			logerror("68681:  %02x %02x\n",offset,data&0xff);
			break;
	}
}

READ16_HANDLER(es5510_dsp_r)
{
	if (es_tmp) return es5510_dsp_ram[offset];
/*
	switch (offset) {
		case 0x00: return (es5510_gpr_latch>>16)&0xff;
		case 0x01: return (es5510_gpr_latch>> 8)&0xff;
		case 0x02: return (es5510_gpr_latch>> 0)&0xff;
		case 0x03: return 0;
	}
*/
//	offset<<=1;

//if (offset<7 && es5510_dsp_ram[0]!=0xff) return rand()%0xffff;

	if (offset==0x12) return 0;

//	if (offset>4)
//	logerror("%06x: DSP read offset %04x (data is %04x)\n",cpu_get_pc(),offset,es5510_dsp_ram[offset]);
	if (offset==0x16) return 0x27;

	return es5510_dsp_ram[offset];
}

WRITE16_HANDLER(es5510_dsp_w)
{
	UINT8 *snd_mem = (UINT8 *)memory_region(REGION_SOUND1);

//	if (offset>4 && offset!=0x80  && offset!=0xa0  && offset!=0xc0  && offset!=0xe0)
//		logerror("%06x: DSP write offset %04x %04x\n",cpu_get_pc(),offset,data);

	COMBINE_DATA(&es5510_dsp_ram[offset]);

	switch (offset) {
		case 0x00: es5510_gpr_latch=(es5510_gpr_latch&0x00ffff)|((data&0xff)<<16);
		case 0x01: es5510_gpr_latch=(es5510_gpr_latch&0xff00ff)|((data&0xff)<< 8);
		case 0x02: es5510_gpr_latch=(es5510_gpr_latch&0xffff00)|((data&0xff)<< 0);
		case 0x03: break;

		case 0x80: /* Read select - GPR + INSTR */
	//		logerror("ES5510:  Read GPR/INSTR %06x (%06x)\n",data,es5510_gpr[data]);

			/* Check if a GPR is selected */
			if (data<0xc0) {
				es_tmp=0;
				es5510_gpr_latch=es5510_gpr[data];
			} else es_tmp=1;
			break;

		case 0xa0: /* Write select - GPR */
	//		logerror("ES5510:  Write GPR %06x %06x (0x%04x:=0x%06x\n",data,es5510_gpr_latch,data,snd_mem[es5510_gpr_latch>>8]);
			es5510_gpr[data]=snd_mem[es5510_gpr_latch>>8];
			break;

		case 0xc0: /* Write select - INSTR */
	//		logerror("ES5510:  Write INSTR %06x %06x\n",data,es5510_gpr_latch);
			break;

		case 0xe0: /* Write select - GPR + INSTR */
	//		logerror("ES5510:  Write GPR/INSTR %06x %06x\n",data,es5510_gpr_latch);
			break;
	}
}
