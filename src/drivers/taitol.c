/*
  Taito L-System

  Monoprocessor games (1 main z80, no sound z80)
  - Plotting
  - Puzznic
  - Palamedes
  - Cachat
  - American Horseshoes

  Dual processor games (2 main z80, 1 sound z80)
  - Fighting hawk
  - Raimais
  - Champion Wrestler
*/


#include "driver.h"
#include "cpu/z80/z80.h"

int taitol_vh_start(void);
void taitol_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void taitol_chardef14_m(int offset);
void taitol_chardef15_m(int offset);
void taitol_chardef16_m(int offset);
void taitol_chardef17_m(int offset);
void taitol_chardef1c_m(int offset);
void taitol_chardef1d_m(int offset);
void taitol_chardef1e_m(int offset);
void taitol_chardef1f_m(int offset);
void taitol_bg18_m(int offset);
void taitol_bg19_m(int offset);
void taitol_char1a_m(int offset);
void taitol_obj1b_m(int offset);

void taitol_control_w(int offset, int data);
int taitol_control_r(int offset);
void taitol_bankg_w(int offset, int data);
int taitol_bankg_r(int offset);
void taitol_bankc_w(int offset, int data);
int taitol_bankc_r(int offset);

static void (*rambank_modify_notifiers[12])(int) = {
	taitol_chardef14_m,	// 14
	taitol_chardef15_m,	// 15
	taitol_chardef16_m,	// 16
	taitol_chardef17_m,	// 17

	taitol_bg18_m,		// 18
	taitol_bg19_m,		// 19
	taitol_char1a_m,	// 1a
	taitol_obj1b_m,		// 1b

	taitol_chardef1c_m,	// 1c
	taitol_chardef1d_m,	// 1d
	taitol_chardef1e_m,	// 1e
	taitol_chardef1f_m,	// 1f
};

static void (*current_notifier[4])(int);
static unsigned char *current_base[4];

static int cur_rombank, cur_rombank2, cur_rambank[4];
static int irq_adr_table[3];
static int irq_enable = 0;

unsigned char *taitol_rambanks;
int taitol_bg18_deltax, taitol_bg19_deltax;

static unsigned char *palette_ram;
static unsigned char *shared_ram;

static int (*porte0_r)(int);
static int (*porte1_r)(int);
static int (*portf0_r)(int);
static int (*portf1_r)(int);

static void palette_notifier(int addr)
{
	unsigned char *p = palette_ram + (addr & ~1);
	unsigned char byte0 = *p++;
	unsigned char byte1 = *p;

	unsigned int b = (byte1 & 0xf) * 0x11;
	unsigned int g = ((byte0 & 0xf0)>>4) * 0x11;
	unsigned int r = (byte0 & 0xf) * 0x11;

	//	addr &= 0x1ff;

	if(addr > 0x200) {
		if(errorlog)
			fprintf(errorlog, "Large palette ? %03x (%04x)\n", addr, cpu_get_pc());
	} else {
		//		r = g = b = ((addr & 0x1e) != 0)*255;
		palette_change_color(addr/2, r, g, b);
	}
}

static void machine_init(void)
{
	int i;

	taitol_rambanks = malloc(0x1000*12);
	palette_ram = malloc(0x1000);

	for(i=0;i<3;i++)
		irq_adr_table[i] = 0;

	irq_enable = 0;

	for(i=0;i<4;i++) {
		cur_rambank[i] = 0x80;
		current_base[i] = palette_ram;
		current_notifier[i] = palette_notifier;
		cpu_setbank(2+i, current_base[i]);
	}
	cur_rombank = cur_rombank2 = 0;
	cpu_setbank(1, Machine->memory_region[0] + 0x10000);

	for(i=0;i<512;i++) {
		decodechar(Machine->gfx[2], i, taitol_rambanks,
				   Machine->drv->gfxdecodeinfo[2].gfxlayout);
		decodechar(Machine->gfx[2], i+512, taitol_rambanks + 0x4000,
				   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	}

	taitol_bg18_deltax = 0x1e4;
	taitol_bg19_deltax = 0x1da;
}

static void puzznic_init(void)
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = input_port_1_r;
	portf0_r = input_port_2_r;
	portf1_r = input_port_3_r;
}

static void plotting_init(void)
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = input_port_1_r;
	portf0_r = input_port_2_r;
	portf1_r = input_port_3_r;
}

static void palamed_init(void)
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = 0;
	portf0_r = input_port_1_r;
	portf1_r = 0;
}

static void cachat_init(void)
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = 0;
	portf0_r = input_port_1_r;
	portf1_r = 0;

	taitol_bg18_deltax = 0x1e4 + 51;
	taitol_bg19_deltax = 0x1da + 64;
}

static void horshoes_init(void)
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = input_port_1_r;
	portf0_r = input_port_2_r;
	portf1_r = input_port_3_r;
}

static void fhawk_init(void)
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}

static void raimais_init(void)
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}

static void champwr_init(void)
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}


static void irq0(int v)
{
	cpu_cause_interrupt(0, irq_adr_table[0]);
}

static void irq1(int v)
{
	cpu_cause_interrupt(0, irq_adr_table[1]);
}

static int vbl_interrupt(void)
{
	// What is really generating interrupts 0 and 1 is still to be found

	if (irq_enable & 1)
		timer_set(TIME_IN_CYCLES(20000,0), 0, irq0);

	if (irq_enable & 2)
		timer_set(TIME_IN_CYCLES(10000,0), 0, irq1);

	return (irq_enable & 4) ? irq_adr_table[2] : Z80_IGNORE_INT;
}

static void irq_adr_w(int offset, int data)
{
//if (errorlog) fprintf(errorlog,"irq_adr_table[%d] = %02x\n",offset,data);
	irq_adr_table[offset] = data;
}

static int irq_adr_r(int offset)
{
	return irq_adr_table[offset];
}

static void irq_enable_w(int offset, int data)
{
	irq_enable = data;
}

static int irq_enable_r(int offset)
{
	return irq_enable;
}


static void rombankswitch_w(int offset, int data)
{
	static int high = 0;
	if(cur_rombank != data) {
		if(data>high) {
			high = data;
			if(errorlog)
				fprintf(errorlog, "New rom size : %x\n", (high+1)*0x2000);
		}

		if(0 && errorlog)
			fprintf(errorlog, "robs %d, %02x (%04x)\n", offset, data, cpu_get_pc());
		cur_rombank = data;
		cpu_setbank(1, Machine->memory_region[0]+0x10000+0x2000*cur_rombank);
	}
}

static void rombank2switch_w(int offset, int data)
{
	static int high = 0;

	data &= 0xf;

	if(cur_rombank2 != data) {
		if(data>high) {
			high = data;
			if(errorlog)
				fprintf(errorlog, "New rom2 size : %x\n", (high+1)*0x4000);
		}

		if(0 && errorlog)
			fprintf(errorlog, "robs2 %02x (%04x)\n", data, cpu_get_pc());

		cur_rombank2 = data;
		cpu_setbank(6, Machine->memory_region[2]+0x10000+0x4000*cur_rombank2);
	}
}

static int rombankswitch_r(int offset)
{
	return cur_rombank;
}

static int rombank2switch_r(int offset)
{
	return cur_rombank2;
}

static void rambankswitch_w(int offset, int data)
{
	if(cur_rambank[offset]!=data) {
		cur_rambank[offset]=data;
		if(0 && errorlog)
		  fprintf(errorlog, "rabs %d, %02x (%04x)\n", offset, data, cpu_get_pc());
		if(data>=0x14 && data<=0x1f) {
			data -= 0x14;
			current_notifier[offset] = rambank_modify_notifiers[data];
			current_base[offset] = taitol_rambanks+0x1000*data;
		} else {
			current_notifier[offset] = palette_notifier;
			current_base[offset] = palette_ram;
		}
		cpu_setbank(2+offset, current_base[offset]);
	}
}

static int rambankswitch_r(int offset)
{
	return cur_rambank[offset];
}

static void bank0_w(int offset, int data)
{
	if(current_base[0][offset]!=data) {
		current_base[0][offset] = data;
		if(current_notifier[0])
			current_notifier[0](offset);
	}
}

static void bank1_w(int offset, int data)
{
	if(current_base[1][offset]!=data) {
		current_base[1][offset] = data;
		if(current_notifier[1])
			current_notifier[1](offset);
	}
}

static void bank2_w(int offset, int data)
{
	if(current_base[2][offset]!=data) {
		current_base[2][offset] = data;
		if(current_notifier[2])
			current_notifier[2](offset);
	}
}

static void bank3_w(int offset, int data)
{
	if(current_base[3][offset]!=data) {
		current_base[3][offset] = data;
		if(current_notifier[3])
			current_notifier[3](offset);
	}
}

static void control2_w(int offset, int data)
{
}

static int extport;

static int portA_r(int offset)
{
	if (extport == 0) return porte0_r(0);
	else return porte1_r(0);
}

static int portB_r(int offset)
{
	if (extport == 0) return portf0_r(0);
	else return portf1_r(0);
}

static int ym2203_data0_r(int offset)
{
	extport = 0;
	return YM2203_read_port_0_r(offset);
}

static int ym2203_data1_r(int offset)
{
	extport = 1;
	return YM2203_read_port_0_r(offset);
}

static int *mcu_reply;
static int mcu_pos = 0, mcu_reply_len = 0;
static int last_data_adr, last_data;

static int puzznic_mcu_reply[] = { 0x50, 0x1f, 0xb6, 0xba, 0x06, 0x03, 0x47, 0x05, 0x00 };

static void mcu_data_w(int offset, int data)
{
	last_data = data;
	last_data_adr = cpu_get_pc();
	if(0 && errorlog)
		fprintf(errorlog, "mcu write %02x (%04x)\n", data, cpu_get_pc());
	switch(data) {
	case 0x43:
		mcu_pos = 0;
		mcu_reply = puzznic_mcu_reply;
		mcu_reply_len = sizeof(puzznic_mcu_reply);
		break;
	}
}

static void mcu_control_w(int offset, int data)
{
	if(0 && errorlog)
		fprintf(errorlog, "mcu control %02x (%04x)\n", data, cpu_get_pc());
}

static int mcu_data_r(int offset)
{
	if(0 && errorlog)
		fprintf(errorlog, "mcu read (%04x) [%02x, %04x]\n", cpu_get_pc(), last_data, last_data_adr);
	if(mcu_pos==mcu_reply_len)
		return 0;

	return mcu_reply[mcu_pos++];
}

static int mcu_control_r(int offset)
{
	if(0 && errorlog)
		fprintf(errorlog, "mcu control read (%04x)\n", cpu_get_pc());
	return 0x1;
}

static void sound_w(int offset, int data)
{
	if(errorlog)
		fprintf(errorlog, "Sound_w %02x (%04x)\n", data, cpu_get_pc());
}

static int shared_r(int offset)
{
	return shared_ram[offset];
}

static void shared_w(int offset, int data)
{
	shared_ram[offset] = data;
}

static int mux_ctrl = 0;

static int mux_r(int offset)
{
	switch(mux_ctrl) {
	case 0:
		return input_port_0_r(0);
	case 1:
		return input_port_1_r(0);
	case 2:
		return input_port_2_r(0);
	case 3:
		return input_port_3_r(0);
	case 7:
		return input_port_4_r(0);
	default:
		if(errorlog)
			fprintf(errorlog, "Mux read from unknown port %d (%04x)\n", mux_ctrl, cpu_get_pc());
		return 0xff;
	}
}

static void mux_w(int offset, int data)
{
	switch(mux_ctrl) {
	case 4:
		control2_w(0, data);
		break;
	default:
		if(errorlog)
			fprintf(errorlog, "Mux read to unknown port %d, %02x (%04x)\n", mux_ctrl, data, cpu_get_pc());
	}
}

static void mux_ctrl_w(int offset, int data)
{
	mux_ctrl = data;
}


#define COMMON_BANKS_READ \
	{ 0x0000, 0x5fff, MRA_ROM },			\
	{ 0x6000, 0x7fff, MRA_BANK1 },			\
	{ 0xc000, 0xcfff, MRA_BANK2 },			\
	{ 0xd000, 0xdfff, MRA_BANK3 },			\
	{ 0xe000, 0xefff, MRA_BANK4 },			\
	{ 0xf000, 0xfdff, MRA_BANK5 },			\
	{ 0xfe00, 0xfe03, taitol_bankc_r },		\
	{ 0xfe04, 0xfe04, taitol_control_r },	\
	{ 0xff00, 0xff02, irq_adr_r },			\
	{ 0xff03, 0xff03, irq_enable_r },		\
	{ 0xff04, 0xff07, rambankswitch_r },	\
	{ 0xff08, 0xff08, rombankswitch_r }

#define COMMON_BANKS_WRITE \
	{ 0x0000, 0x7fff, MWA_ROM },			\
	{ 0xc000, 0xcfff, bank0_w },			\
	{ 0xd000, 0xdfff, bank1_w },			\
	{ 0xe000, 0xefff, bank2_w },			\
	{ 0xf000, 0xfdff, bank3_w },			\
	{ 0xfe00, 0xfe03, taitol_bankc_w },		\
	{ 0xfe04, 0xfe04, taitol_control_w },	\
	{ 0xff00, 0xff02, irq_adr_w },			\
	{ 0xff03, 0xff03, irq_enable_w },		\
	{ 0xff04, 0xff07, rambankswitch_w },	\
	{ 0xff08, 0xff08, rombankswitch_w }

#define COMMON_SINGLE_READ \
	{ 0xa000, 0xa000, YM2203_status_port_0_r },	\
	{ 0xa001, 0xa001, ym2203_data0_r },			\
	{ 0xa003, 0xa003, ym2203_data1_r },			\
	{ 0x8000, 0x9fff, MRA_RAM }

#define COMMON_SINGLE_WRITE \
	{ 0xa000, 0xa000, YM2203_control_port_0_w },	\
	{ 0xa001, 0xa001, YM2203_write_port_0_w },		\
	{ 0x8000, 0x9fff, MWA_RAM }

#define COMMON_DUAL_READ \
	{ 0x8000, 0x9fff, MRA_RAM, &shared_ram  }, \
	{ 0xa000, 0xbfff, MRA_RAM, &shared2_ram }

#define COMMON_DUAL_WRITE \
	{ 0x8000, 0xbfff, MWA_RAM }

static struct MemoryReadAddress puzznic_readmem[] = {
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, MRA_NOP },	// Watchdog
	{ 0xb000, 0xb7ff, MRA_RAM },	// Wrong, used to overcome protection
	{ 0xb800, 0xb800, mcu_data_r },
	{ 0xb801, 0xb801, mcu_control_r },
	{ -1 }
};

static struct MemoryWriteAddress puzznic_writemem[] = {
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xb000, 0xb7ff, MWA_RAM },	// Wrong, used to overcome protection
	{ 0xb800, 0xb800, mcu_data_w },
	{ 0xb801, 0xb801, mcu_control_w },
	{ 0xbc00, 0xbc00, MWA_NOP },	// Control register, function unknown
	{ -1 }
};


static struct MemoryReadAddress plotting_readmem[] = {
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ -1 }
};

static struct MemoryWriteAddress plotting_writemem[] = {
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xa800, 0xa800, MWA_NOP },	// Watchdog or interrupt ack
	{ 0xb800, 0xb800, MWA_NOP },	// Control register, function unknown
	{ -1 }
};


static struct MemoryReadAddress palamed_readmem[] = {
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, input_port_2_r },
	{ 0xa801, 0xa801, input_port_3_r },
	{ 0xa802, 0xa802, input_port_4_r },
	{ 0xb001, 0xb001, MRA_NOP },	// Watchdog or interrupt ack
	{ -1 }
};

static struct MemoryWriteAddress palamed_writemem[] = {
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xa803, 0xa803, MWA_NOP },	// Control register, function unknown
	{ 0xb000, 0xb000, MWA_NOP },	// Control register, function unknown (copy of 8822)
	{ -1 }
};


static struct MemoryReadAddress cachat_readmem[] = {
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, input_port_2_r },
	{ 0xa801, 0xa801, input_port_3_r },
	{ 0xa802, 0xa802, input_port_4_r },
	{ 0xb001, 0xb001, MRA_NOP },	// Watchdog or interrupt ack (value ignored)
	{ 0xfff8, 0xfff8, rombankswitch_r },
	{ -1 }
};

static struct MemoryWriteAddress cachat_writemem[] = {
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xa803, 0xa803, MWA_NOP },	// Control register, function unknown
	{ 0xb000, 0xb000, MWA_NOP },	// Control register, function unknown
	{ 0xfff8, 0xfff8, rombankswitch_w },
	{ -1 }
};


static struct MemoryReadAddress horshoes_readmem[] = {
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, input_port_4_r },
	{ 0xa804, 0xa804, input_port_5_r },
	{ 0xa808, 0xa808, input_port_6_r },
	{ 0xa80c, 0xa80c, input_port_7_r },
	{ 0xb801, 0xb801, MRA_NOP },	// Watchdog or interrupt ack
	{ -1 }
};

static struct MemoryWriteAddress horshoes_writemem[] = {
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xb802, 0xb802, taitol_bankg_w },
	{ 0xbc00, 0xbc00, MWA_NOP },
	{ -1 }
};



static struct MemoryReadAddress fhawk_readmem[] = {
	COMMON_BANKS_READ,
	{ 0x8000, 0x9fff, MRA_RAM, &shared_ram },
	{ 0xa000, 0xbfff, MRA_RAM },
	{ -1 }
};

static struct MemoryWriteAddress fhawk_writemem[] = {
	COMMON_BANKS_WRITE,
	{ 0x8000, 0xbfff, MWA_RAM },
	{ -1 }
};

static struct MemoryReadAddress fhawk_2_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK6 },
	{ 0xc800, 0xc801, MRA_NOP }, // Pipe
	{ 0xe000, 0xffff, shared_r },
	{ 0xd000, 0xd000, input_port_0_r },
	{ 0xd001, 0xd001, input_port_1_r },
	{ 0xd002, 0xd002, input_port_2_r },
	{ 0xd003, 0xd003, input_port_3_r },
	{ 0xd007, 0xd007, input_port_4_r },
	{ -1 }
};

static struct MemoryWriteAddress fhawk_2_writemem[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc000, rombank2switch_w },
	{ 0xc800, 0xc801, MWA_NOP }, // Pipe
	{ 0xd000, 0xd000, MWA_NOP },	// Direct copy of input port 0
	{ 0xd004, 0xd004, control2_w },
	{ 0xd005, 0xd006, MWA_NOP },	// Always 0
	{ 0xe000, 0xffff, shared_w },
	{ -1 }
};

static struct MemoryReadAddress fhawk_3_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xe000, 0xe001, MRA_NOP }, // Pipe
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
	{ -1 }
};

static struct MemoryWriteAddress fhawk_3_writemem[] = {
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xe000, 0xe001, MWA_NOP }, // Pipe
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
	{ -1 }
};

static struct MemoryReadAddress raimais_readmem[] = {
	COMMON_BANKS_READ,
	{ 0x8000, 0x87ff, MRA_RAM, &shared_ram },
	{ 0x8800, 0x8800, mux_r },
	{ 0x8801, 0x8801, MRA_NOP },	// Watchdog or interrupt ack (value ignored)
	{ 0x8c00, 0x8c01, MRA_NOP }, // Pipe
	{ 0xa000, 0xbfff, MRA_RAM },
	{ -1 }
};
static struct MemoryWriteAddress raimais_writemem[] = {
	COMMON_BANKS_WRITE,
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, mux_w },
	{ 0x8801, 0x8801, mux_ctrl_w },
	{ 0x8c00, 0x8c01, MWA_NOP }, // Pipe
	{ 0xa000, 0xbfff, MWA_RAM },
	{ -1 }
};

static struct MemoryReadAddress raimais_2_readmem[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, shared_r },
	{ -1 }
};

static struct MemoryWriteAddress raimais_2_writemem[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, shared_w },
	{ -1 }
};

static struct MemoryReadAddress raimais_3_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2203_status_port_0_r },
	{ 0xe200, 0xe201, MRA_NOP }, // Pipe
	{ -1 }
};

static struct MemoryWriteAddress raimais_3_writemem[] = {
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe200, 0xe201, MWA_NOP }, // Pipe
	{ -1 }
};


static struct MemoryReadAddress champwr_readmem[] = {
	COMMON_BANKS_READ,
	{ 0x8000, 0x9fff, MRA_RAM, },
	{ 0xa000, 0xbfff, MRA_RAM, &shared_ram },
	{ -1 }
};


static struct MemoryWriteAddress champwr_writemem[] = {
	COMMON_BANKS_WRITE,
	{ 0x8000, 0xbfff, MWA_RAM },
	{ -1 }
};

static struct MemoryReadAddress champwr_2_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK6 },
	{ 0xc000, 0xdfff, shared_r },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe001, 0xe001, input_port_1_r },
	{ 0xe002, 0xe002, input_port_2_r },
	{ 0xe003, 0xe003, input_port_3_r },
	{ 0xe007, 0xe007, input_port_4_r },
	{ 0xe008, 0xe00f, MRA_NOP },
	{ 0xf000, 0xf000, rombank2switch_r },
	{ -1 }
};

static struct MemoryWriteAddress champwr_2_writemem[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, shared_w },
	{ 0xe000, 0xe000, MWA_NOP },	// Watchdog
	{ 0xe004, 0xe004, control2_w },
	{ 0xf000, 0xf000, rombank2switch_w },
	{ -1 }
};

static struct MemoryReadAddress champwr_3_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0xa000, 0xa001, MRA_NOP }, // Pipe
	{ -1 }
};

static struct MemoryWriteAddress champwr_3_writemem[] = {
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa001, MWA_NOP }, // Pipe
	{ -1 }
};



INPUT_PORTS_START(puzznic_input_ports) /* Plotting ports too */
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* Not read yet. Coin 2 and Tilt are probably here */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


INPUT_PORTS_START(palamed_input_ports)
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )  // Service
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START(horshoes_input_ports)
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Bit 3" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 4" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 5" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 6" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 7" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 8" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 9" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 10" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 11" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 12" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 13" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 14" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 15" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0a" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1a" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 2a" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 3a" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 4a" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 5a" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 6a" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 7a" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0b" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1b" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 2b" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 3b" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 4b" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 5b" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 6b" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 7b" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0c" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1c" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 2c" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 3c" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 4c" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 5c" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 6c" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 7c" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0d" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1d" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 2d" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 3d" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 4d" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 5d" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 6d" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 7d" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
INPUT_PORTS_END

INPUT_PORTS_START(fhawk_input_ports)
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Sound" )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 8" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 9" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 10" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 11" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 12" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 13" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 14" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 15" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 ) // Service
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 4 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END


INPUT_PORTS_START(raimais_input_ports)
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
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
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3?" )
	PORT_DIPSETTING(    0x20, "4?" )
	PORT_DIPSETTING(    0x10, "5?" )
	PORT_DIPSETTING(    0x00, "6?" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 ) // Service
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

INPUT_PORTS_START(champwr_input_ports)
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Bit 3" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 8" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 9" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 10" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 11" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 12" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 13" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 14" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 15" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
INPUT_PORTS_END




static struct GfxLayout bg1_layout =
{
	8, 8,
	16384,
	4,
	{ 8*0x40000, 8*0x40000+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*8*2
};

static struct GfxLayout bg2_layout =
{
	8, 8,
	49152,
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*8*4
};

#define O 8*8*2
#define O2 2*O
static struct GfxLayout sp1_layout =
{
	16, 16,
	16384/4,
	4,
	{ 8*0x40000, 8*0x40000+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0, O+3, O+2, O+1, O+0, O+8+3, O+8+2, O+8+1, O+8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, O2+0*16, O2+1*16, O2+2*16, O2+3*16, O2+4*16, O2+5*16, O2+6*16, O2+7*16 },
	8*8*2*4
};
#undef O
#undef O2

#define O 8*8*4
#define O2 2*O
static struct GfxLayout sp2_layout =
{
	16, 16,
	49152/4,
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16, O+3, O+2, O+1, O+0, O+19, O+18, O+17, O+16 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, O2+0*32, O2+1*32, O2+2*32, O2+3*32, O2+4*32, O2+5*32, O2+6*32, O2+7*32 },
	8*8*4*4
};
#undef O
#undef O2

static struct GfxLayout char_layout =
{
	8, 8,
	1024,
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*8*4
};

static struct GfxDecodeInfo gfxdecodeinfo1[] =
{
	{ 1, 0x000000, &bg1_layout, 0, 16 },
	{ 1, 0x000000, &sp1_layout, 0, 16 },
	{ 0, 0x000000, &char_layout,  0, 16 },  // Ram-based
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo2[] =
{
	{ 1, 0x000000, &bg2_layout, 0, 16 },
	{ 1, 0x000000, &sp2_layout, 0, 16 },
	{ 0, 0x000000, &char_layout,  0, 16 },  // Ram-based
	{ -1 }
};



static struct YM2203interface ym2203_interface_single =
{
	1,			/* 1 chip */
	3000000,	/* ??? */
	{ YM2203_VOL(50,50) },
	AY8910_DEFAULT_GAIN,
	{ portA_r },
	{ portB_r },
	{ 0 },
	{ 0 },
	{ 0 }
};


static void irqhandler(int irq)
{
	cpu_set_irq_line(2,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static void portA_w(int offset,int data)
{
}

static struct YM2203interface ym2203_interface_double =
{
	1,			/* 1 chip */
	3000000,	/* ??? */
	{ YM2203_VOL(50,50) },
	AY8910_DEFAULT_GAIN,
	{ 0 },
	{ 0 },
	{ portA_w },
	{ 0 },
	{ irqhandler }
};



#define MCH_SINGLE(name) \
static struct MachineDriver name ## _machine_driver =	\
{														\
	{													\
		{												\
			CPU_Z80,									\
			3332640,	/* ? xtal is 13.33056 */		\
			0,											\
			name ## _readmem, name ## _writemem, 0, 0,	\
			vbl_interrupt, 1							\
		}												\
	},													\
	60, DEFAULT_60HZ_VBLANK_DURATION,					\
	1,													\
	name ## _init,										\
														\
	320, 256, { 0, 319, 16, 239 },						\
	gfxdecodeinfo1,										\
	256, 256, 0,										\
														\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,			\
	0,													\
	taitol_vh_start,									\
	0,													\
	taitol_vh_screenrefresh,							\
														\
	0,0,0,0,											\
	{													\
		{												\
			SOUND_YM2203,								\
			&ym2203_interface_single					\
		}												\
	}													\
};

#define MCH_DOUBLE(name) \
static struct MachineDriver name ## _machine_driver =		\
{															\
	{														\
		{													\
			CPU_Z80,										\
			3332640,	/* ? xtal is 13.33056 */			\
			0,												\
			name ## _readmem, name ## _writemem, 0, 0,		\
			vbl_interrupt, 1								\
		},													\
		{													\
			CPU_Z80,										\
			3332640,	/* ? xtal is 13.33056 */			\
			2,												\
			name ## _2_readmem, name ## _2_writemem, 0, 0,	\
			interrupt, 1									\
		},													\
		{													\
			CPU_Z80 | CPU_AUDIO_CPU,						\
			3332640,	/* ? xtal is 13.33056 */			\
			3,												\
			name ## _3_readmem, name ## _3_writemem, 0, 0,	\
			ignore_interrupt, 0								\
		}													\
	},														\
	60, DEFAULT_60HZ_VBLANK_DURATION,						\
	1,														\
	name ## _init,											\
															\
	320, 256, { 0, 319, 16, 239 },							\
	gfxdecodeinfo2,											\
	256, 256, 0,											\
															\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,				\
	0,														\
	taitol_vh_start,										\
	0,														\
	taitol_vh_screenrefresh,								\
															\
	0,0,0,0,												\
	{														\
		{													\
			SOUND_YM2203,									\
			&ym2203_interface_double						\
		}													\
	}														\
};


MCH_DOUBLE(fhawk)
MCH_DOUBLE(raimais)
MCH_DOUBLE(champwr)

MCH_SINGLE(puzznic)
MCH_SINGLE(plotting)
MCH_SINGLE(palamed)
MCH_SINGLE(horshoes)
MCH_SINGLE(cachat)



ROM_START( fhawk_rom )
	ROM_REGION(0xb0000)
	ROM_LOAD( "b70-07.bin", 0x00000, 0x20000, 0x939114af )
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_LOAD( "b70-03.bin", 0x30000, 0x80000, 0x42d5a9b8 )

	ROM_REGION_DISPOSE(0x180000)
	ROM_LOAD( "b70-01.bin", 0x00000, 0x80000, 0xfcdf67e2 )
	ROM_LOAD( "b70-02.bin", 0x80000, 0x80000, 0x35f7172e )

	ROM_REGION(0x30000)
	ROM_LOAD( "b70-08.bin", 0x00000, 0x20000, 0x4d795f48 )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION(0x10000)
	ROM_LOAD( "b70-09.bin", 0x00000, 0x10000, 0x85cccaa2 )
ROM_END

ROM_START( raimais_rom )
	ROM_REGION(0xb0000)
	ROM_LOAD( "b36-08-1.bin", 0x00000, 0x20000, 0x6cc8f79f )
	ROM_RELOAD(               0x10000, 0x20000 )
	ROM_LOAD( "b36-03.bin",   0x30000, 0x80000, 0x96166516 )

	ROM_REGION_DISPOSE(0x180000)
	ROM_LOAD( "b36-01.bin",   0x00000, 0x80000, 0x89355cb2 )
	ROM_LOAD( "b36-02.bin",   0x80000, 0x80000, 0xe71da5db )

	ROM_REGION(0x10000)
	ROM_LOAD( "b36-07.bin",   0x00000, 0x10000, 0x4f3737e6 )

	ROM_REGION(0x10000)
	ROM_LOAD( "b36-06.bin",   0x00000, 0x10000, 0x29bbc4f8 )
ROM_END

ROM_START( champwr_rom )
	ROM_REGION(0xf0000)
	ROM_LOAD( "c01-13.rom", 0x00000, 0x20000, 0x7ef47525 )
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_LOAD( "c01-04.rom", 0x30000, 0x20000, 0x358bd076 )
	ROM_LOAD( "c01-05.rom", 0x50000, 0x20000, 0x22efad4a )

	ROM_REGION_DISPOSE(0x180000)
	ROM_LOAD( "c01-01.rom", 0x000000, 0x80000, 0xf302e6e9 )
	ROM_LOAD( "c01-02.rom", 0x080000, 0x80000, 0x1e0476c4 )
	ROM_LOAD( "c01-03.rom", 0x100000, 0x80000, 0x2a142dbc )

	ROM_REGION(0x30000)
	ROM_LOAD( "c01-07.rom", 0x00000, 0x20000, 0x5117c98f )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION(0x10000)
	ROM_LOAD( "c01-08.rom", 0x00000, 0x10000, 0x810efff8 )
ROM_END


ROM_START( puzznic_rom )
	ROM_REGION(0x30000)
	ROM_LOAD( "u11.rom",  0x00000, 0x20000, 0xa4150b6c )
	ROM_RELOAD(           0x10000, 0x20000 )

	ROM_REGION_DISPOSE(0x80000)
	ROM_LOAD( "u10.rom",  0x00000, 0x20000, 0x4264056c )
	ROM_LOAD( "u09.rom",  0x40000, 0x20000, 0x3c115f8b )

	ROM_REGION(0x0800)	/* 2k for the microcontroller */
	ROM_LOAD( "mc68705p", 0x0000, 0x0800, 0x00000000 )
ROM_END

ROM_START( plotting_rom )
	ROM_REGION(0x20000)
	ROM_LOAD( "plot01.bin", 0x00000, 0x10000, 0x5b30bc25 )
	ROM_RELOAD(             0x10000, 0x10000 )

	ROM_REGION_DISPOSE(0x80000)
	ROM_LOAD( "plot07.bin", 0x00000, 0x10000, 0x6e0bad2a )
	ROM_LOAD( "plot08.bin", 0x40000, 0x10000, 0xfb5f3ca4 )
ROM_END

ROM_START( palamed_rom )
	ROM_REGION(0x30000)
	ROM_LOAD( "c63.02", 0x00000, 0x20000, 0x55a82bb2 )
	ROM_RELOAD(         0x10000, 0x20000 )

	ROM_REGION_DISPOSE(0x80000)
	ROM_LOAD( "c64.04", 0x00000, 0x20000, 0xc7bbe460 )
	ROM_LOAD( "c63.03", 0x40000, 0x20000, 0xfcd86e44 )
ROM_END

ROM_START( horshoes_rom )
	ROM_REGION(0x30000)
	ROM_LOAD( "c47.03", 0x00000, 0x20000, 0x37e15b20 )
	ROM_RELOAD(         0x10000, 0x20000 )

	ROM_REGION_DISPOSE(0x80000)
	ROM_LOAD( "c47.02", 0x00000, 0x10000, 0x35f96526 )
	ROM_CONTINUE (      0x20000, 0x10000 )
	ROM_LOAD( "c47.04", 0x40000, 0x10000, 0xaeac7121 )
	ROM_CONTINUE (      0x60000, 0x10000 )
	ROM_LOAD( "c47.01", 0x10000, 0x10000, 0x031c73d8 )
	ROM_CONTINUE (      0x30000, 0x10000 )
	ROM_LOAD( "c47.05", 0x50000, 0x10000, 0xb2a3dafe )
	ROM_CONTINUE (      0x70000, 0x10000 )
ROM_END

ROM_START( cachat_rom )
	ROM_REGION(0x30000)
	ROM_LOAD( "cac6",  0x00000, 0x20000, 0x8105cf5f )
	ROM_RELOAD(        0x10000, 0x20000 )

	ROM_REGION_DISPOSE(0x80000)
	ROM_LOAD( "cac9",  0x00000, 0x20000, 0xbc462914 )
	ROM_LOAD( "cac10", 0x20000, 0x20000, 0xecc64b31 )
	ROM_LOAD( "cac7",  0x40000, 0x20000, 0x7fb71578 )
	ROM_LOAD( "cac8",  0x60000, 0x20000, 0xd2a63799 )
ROM_END



// bits 7..0 => bits 0..7
static void plotting_decode(void)
{
	unsigned char tab[256];
	unsigned char *p;
	int i;

	for(i=0;i<256;i++) {
		int j, v=0;
		for(j=0;j<8;j++)
			if(i & (1<<j))
				v |= 1<<(7-j);
		tab[i] = v;
	}
	p = Machine->memory_region[0];
	for(i=0;i<0x20000;i++) {
		*p = tab[*p];
		p++;
	}
}



struct GameDriver fhawk_driver =
{
	__FILE__,
	0,
	"fhawk",
	"Fighting Hawk (Japan)",
	"1988",
	"Taito Corporation",
	"",
	0,
	&fhawk_machine_driver,
	0,

	fhawk_rom,
	0,0, 0, 0,

	fhawk_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0,0
};

struct GameDriver raimais_driver =
{
	__FILE__,
	0,
	"raimais",
	"Raimais (Japan)",
	"1988",
	"Taito Corporation",
	"",
	GAME_NOT_WORKING,
	&raimais_machine_driver,
	0,

	raimais_rom,
	0,0, 0, 0,

	raimais_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0,0
};

struct GameDriver champwr_driver =
{
	__FILE__,
	0,
	"champwr",
	"Champion Wrestler (World)",
	"1989",
	"Taito Corporation Japan",
	"",
	0,
	&champwr_machine_driver,
	0,

	champwr_rom,
	0,0, 0, 0,

	champwr_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0,0
};

struct GameDriver puzznic_driver =
{
	__FILE__,
	0,
	"puzznic",
	"Puzznic (Japan)",
	"1989",
	"Taito Corporation",
	"",
	0,
	&puzznic_machine_driver,
	0,

	puzznic_rom,
	0, 0, 0, 0,

	puzznic_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0,0
};

struct GameDriver plotting_driver =
{
	__FILE__,
	0,
	"plotting",
	"Plotting (World)",
	"1989",
	"Taito Corporation Japan",
	"",
	0,
	&plotting_machine_driver,
	0,

	plotting_rom,
	plotting_decode,
	0, 0, 0,

	puzznic_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0,0
};


struct GameDriver palamed_driver =
{
	__FILE__,
	0,
	"palamed",
	"Palamedes (Japan)",
	"1990",
	"Taito Corporation",
	"",
	0,
	&palamed_machine_driver,
	0,

	palamed_rom,
	0,0, 0, 0,

	palamed_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0,0
};

struct GameDriver horshoes_driver =
{
	__FILE__,
	0,
	"horshoes",
	"American Horseshoes (US)",
	"1990",
	"Taito America Corporation",
	"",
	0,
	&horshoes_machine_driver,
	0,

	horshoes_rom,
	0,0, 0, 0,

	horshoes_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0,0
};

struct GameDriver cachat_driver =
{
	__FILE__,
	0,
	"cachat",
	"Cachat (Japan)",
	"1993",
	"Taito Corporation",
	"",
	0,
	&cachat_machine_driver,
	0,

	cachat_rom,
	0,0, 0, 0,

	palamed_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0,0
};
