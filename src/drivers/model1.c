/*
**      V60 + 68k + 4xTGP
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "system16.h"
#include "vidhrdw/segaic24.h"
#include "cpu/m68000/m68k.h"

WRITE16_HANDLER( model1_paletteram_w );
VIDEO_START(model1);
VIDEO_UPDATE(model1);
VIDEO_EOF(model1);
extern UINT16 *model1_display_list0, *model1_display_list1;
extern UINT16 *model1_color_xlat;
READ16_HANDLER( model1_listctl_r );
WRITE16_HANDLER( model1_listctl_w );

READ16_HANDLER( model1_tgp_copro_r );
WRITE16_HANDLER( model1_tgp_copro_w );
READ16_HANDLER( model1_tgp_copro_adr_r );
WRITE16_HANDLER( model1_tgp_copro_adr_w );
READ16_HANDLER( model1_tgp_copro_ram_r );
WRITE16_HANDLER( model1_tgp_copro_ram_w );

void model1_tgp_reset(int swa);

static READ16_HANDLER( io_r )
{
	if(offset < 0x8)
		return readinputport(offset);
	if(offset < 0x10) {
		offset -= 0x8;
		if(offset < 3)
			return readinputport(offset+8) | 0xff00;
		return 0xff;
	}

	logerror("IOR: %02x\n", offset);
	return 0xffff;
}

static READ16_HANDLER( fifoin_status_r )
{
	return 0xffff;
}

static WRITE16_HANDLER( bank_w )
{
	if(ACCESSING_LSB) {
		switch(data & 0xf) {
		case 0x1: // 100000-1fffff data roms banking
			cpu_setbank(1, memory_region(REGION_CPU1) + 0x1000000 + 0x100000*((data >> 4) & 0xf));
			logerror("BANK %x\n", 0x1000000 + 0x100000*((data >> 4) & 0xf));
			break;
		case 0x2: // 200000-2fffff data roms banking (unused, all known games have only one bank)
			break;
		case 0xf: // f00000-ffffff program roms banking (unused, all known games have only one bank)
			break;
		}
	}
}


static int last_irq;

static void irq_raise(int level)
{
	//	logerror("irq: raising %d\n", level);
	//	irq_status |= (1 << level);
	last_irq = level;
	cpunum_set_input_line(0, 0, HOLD_LINE);
}

static int irq_callback(int irqline)
{
	return last_irq;
}
// vf
// 1 = fe3ed4
// 3 = fe3f5c
// other = fe3ec8 / fe3ebc

// vr
// 1 = fe02bc
// other = f302a4 / fe02b0

// swa
// 1 = ff504
// 3 = ff54c
// other = ff568/ff574

static void irq_init(void)
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	cpu_set_irq_callback(0, irq_callback);
}

extern void tgp_tick(void);
static INTERRUPT_GEN(model1_interrupt)
{
	if (cpu_getiloops())
	{
		irq_raise(1);
		tgp_tick();
	}
	else
	{
		irq_raise(3);
	}
}

static INTERRUPT_GEN(swa_interrupt)
{
	irq_raise(1);
	tgp_tick();
}


static MACHINE_INIT(model1)
{
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x1000000);
	irq_init();
	model1_tgp_reset(!strcmp(Machine->gamedrv->name, "swa") || !strcmp(Machine->gamedrv->name, "wingwar"));
}

static READ16_HANDLER( network_ctl_r )
{
	if(offset)
		return 0x40;
	else
		return 0x00;
}

static WRITE16_HANDLER( network_ctl_w )
{
}

static WRITE16_HANDLER(md1_w)
{
	extern int model1_dump;
	COMBINE_DATA(model1_display_list1+offset);
	if(0 && offset)
		return;
	if(1 && model1_dump)
		logerror("TGP: md1_w %x, %04x @ %04x (%x)\n", offset, data, mem_mask, activecpu_get_pc());
}

static WRITE16_HANDLER(md0_w)
{
	extern int model1_dump;
	COMBINE_DATA(model1_display_list0+offset);
	if(0 && offset)
		return;
	if(1 && model1_dump)
		logerror("TGP: md0_w %x, %04x @ %04x (%x)\n", offset, data, mem_mask, activecpu_get_pc());
}

static WRITE16_HANDLER(p_w)
{
	UINT16 old = paletteram16[offset];
	paletteram16_xBBBBBGGGGGRRRRR_word_w(offset, data, mem_mask);
	if(0 && paletteram16[offset] != old)
		logerror("XVIDEO: p_w %x, %04x @ %04x (%x)\n", offset, data, mem_mask, activecpu_get_pc());
}

static UINT16 *mr;
static WRITE16_HANDLER(mr_w)
{
	COMBINE_DATA(mr+offset);
	if(0 && offset == 0x1138/2)
		logerror("MR.w %x, %04x @ %04x (%x)\n", offset*2+0x500000, data, mem_mask, activecpu_get_pc());
}

static UINT16 *mr2;
static WRITE16_HANDLER(mr2_w)
{
	COMBINE_DATA(mr2+offset);
#if 0
	if(0 && offset == 0x6e8/2) {
		logerror("MR.w %x, %04x @ %04x (%x)\n", offset*2+0x400000, data, mem_mask, activecpu_get_pc());
	}
	if(offset/2 == 0x3680/4)
		logerror("MW f80[r25], %04x%04x (%x)\n", mr2[0x3680/2+1], mr2[0x3680/2], activecpu_get_pc());
	if(offset/2 == 0x06ca/4)
		logerror("MW fca[r19], %04x%04x (%x)\n", mr2[0x06ca/2+1], mr2[0x06ca/2], activecpu_get_pc());
	if(offset/2 == 0x1eca/4)
		logerror("MW fca[r22], %04x%04x (%x)\n", mr2[0x1eca/2+1], mr2[0x1eca/2], activecpu_get_pc());
#endif

	// wingwar scene position, pc=e1ce -> d735
	if(offset/2 == 0x1f08/4)
		logerror("MW  8[r10], %f (%x)\n", *(float *)(mr2+0x1f08/2), activecpu_get_pc());
	if(offset/2 == 0x1f0c/4)
		logerror("MW  c[r10], %f (%x)\n", *(float *)(mr2+0x1f0c/2), activecpu_get_pc());
	if(offset/2 == 0x1f10/4)
		logerror("MW 10[r10], %f (%x)\n", *(float *)(mr2+0x1f10/2), activecpu_get_pc());
}

static int to_68k;

static READ16_HANDLER( snd_68k_ready_r )
{
	int sr = cpunum_get_reg(1, M68K_REG_SR);

	if ((sr & 0x0700) > 0x0100)
	{
		cpu_spinuntil_time(TIME_IN_USEC(40));
		return 0;	// not ready yet, interrupts disabled
	}
	
	return 0xff;
}

static WRITE16_HANDLER( snd_latch_to_68k_w )
{
	while (!snd_68k_ready_r(0, 0))
	{
		cpu_spinuntil_time(TIME_IN_USEC(40));
	}

	to_68k = data;
	
	cpunum_set_input_line(1, 2, HOLD_LINE);
	// give the 68k time to reply
	cpu_spinuntil_time(TIME_IN_USEC(40));
}

static ADDRESS_MAP_START( model1_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x1fffff) AM_ROMBANK(1)
	AM_RANGE(0x200000, 0x2fffff) AM_ROM

	AM_RANGE(0x400000, 0x40ffff) AM_READWRITE(MRA16_RAM, mr2_w) AM_BASE(&mr2)
	AM_RANGE(0x500000, 0x53ffff) AM_READWRITE(MRA16_RAM, mr_w)  AM_BASE(&mr)

	AM_RANGE(0x600000, 0x60ffff) AM_READWRITE(MRA16_RAM, md0_w) AM_BASE(&model1_display_list0)
	AM_RANGE(0x610000, 0x61ffff) AM_READWRITE(MRA16_RAM, md1_w) AM_BASE(&model1_display_list1)
	AM_RANGE(0x680000, 0x680003) AM_READWRITE(model1_listctl_r, model1_listctl_w)

	AM_RANGE(0x700000, 0x70ffff) AM_READWRITE(sys24_tile_r, sys24_tile_w)
	AM_RANGE(0x720000, 0x720001) AM_WRITENOP		// Unknown, always 0
	AM_RANGE(0x740000, 0x740001) AM_WRITENOP		// Horizontal synchronization register
	AM_RANGE(0x760000, 0x760001) AM_WRITENOP		// Vertical synchronization register
	AM_RANGE(0x770000, 0x770001) AM_WRITENOP		// Video synchronization switch
	AM_RANGE(0x780000, 0x7fffff) AM_READWRITE(sys24_char_r, sys24_char_w)

	AM_RANGE(0x900000, 0x903fff) AM_READWRITE(MRA16_RAM, p_w) AM_BASE(&paletteram16)
	AM_RANGE(0x910000, 0x91bfff) AM_RAM  AM_BASE(&model1_color_xlat)

	AM_RANGE(0xc00000, 0xc0003f) AM_READ(io_r)

	AM_RANGE(0xc00040, 0xc00043) AM_READWRITE(network_ctl_r, network_ctl_w)

	AM_RANGE(0xc00200, 0xc002ff) AM_RAM AM_BASE((data16_t **)&generic_nvram) AM_SIZE(&generic_nvram_size)

	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(snd_latch_to_68k_w)
	AM_RANGE(0xc40002, 0xc40003) AM_READ(snd_68k_ready_r)

	AM_RANGE(0xd00000, 0xd00001) AM_READWRITE(model1_tgp_copro_adr_r, model1_tgp_copro_adr_w)
	AM_RANGE(0xd20000, 0xd20003) AM_WRITE(model1_tgp_copro_ram_w )
	AM_RANGE(0xd80000, 0xd80003) AM_WRITE(model1_tgp_copro_w) AM_MIRROR(0x10)
	AM_RANGE(0xdc0000, 0xdc0003) AM_READ(fifoin_status_r)

	AM_RANGE(0xe00000, 0xe00001) AM_WRITENOP        // Watchdog?  IRQ ack? Always 0x20, usually on irq
	AM_RANGE(0xe00004, 0xe00005) AM_WRITE(bank_w)
	AM_RANGE(0xe0000c, 0xe0000f) AM_WRITENOP

	AM_RANGE(0xfc0000, 0xffffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( model1_io, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(0xd20000, 0xd20003) AM_READ(model1_tgp_copro_ram_r)
	AM_RANGE(0xd80000, 0xd80003) AM_READ(model1_tgp_copro_r)
ADDRESS_MAP_END

static READ16_HANDLER( m1_snd_68k_latch_r )
{
	return to_68k;
}

static READ16_HANDLER( m1_snd_v60_ready_r )
{
	return 0;
}

static READ16_HANDLER( m1_snd_mpcm0_r )
{
	return MultiPCM_reg_0_r(0);
}

static WRITE16_HANDLER( m1_snd_mpcm0_w )
{
	MultiPCM_reg_0_w(offset, data);
}

static WRITE16_HANDLER( m1_snd_mpcm0_bnk_w )
{
	MultiPCM_bank_0_w(0, data);
}

static READ16_HANDLER( m1_snd_mpcm1_r )
{
	return MultiPCM_reg_1_r(0);
}

static WRITE16_HANDLER( m1_snd_mpcm1_w )
{
	MultiPCM_reg_1_w(offset, data);
}

static WRITE16_HANDLER( m1_snd_mpcm1_bnk_w )
{
	MultiPCM_bank_1_w(0, data);
}

static READ16_HANDLER( m1_snd_ym_r )
{
	return YM2612_status_port_0_A_r(0);
}

static WRITE16_HANDLER( m1_snd_ym_w )
{
	switch (offset)
	{
		case 0:
			YM2612_control_port_0_A_w(0, data);
			break;

		case 1:
			YM2612_data_port_0_A_w(0, data);
			break;

		case 2:
			YM2612_control_port_0_B_w(0, data);
			break;

		case 3:
			YM2612_data_port_0_B_w(0, data);
			break;
	}
}

static WRITE16_HANDLER( m1_snd_68k_latch1_w )
{
}

static WRITE16_HANDLER( m1_snd_68k_latch2_w )
{
}

static ADDRESS_MAP_START( model1_snd, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0bffff) AM_ROM
	AM_RANGE(0xc20000, 0xc20001) AM_READWRITE( m1_snd_68k_latch_r, m1_snd_68k_latch1_w )
	AM_RANGE(0xc20002, 0xc20003) AM_READWRITE( m1_snd_v60_ready_r, m1_snd_68k_latch2_w )
	AM_RANGE(0xc40000, 0xc40007) AM_READWRITE( m1_snd_mpcm0_r, m1_snd_mpcm0_w )
	AM_RANGE(0xc40012, 0xc40013) AM_WRITENOP
	AM_RANGE(0xc50000, 0xc50001) AM_WRITE( m1_snd_mpcm0_bnk_w )
	AM_RANGE(0xc60000, 0xc60007) AM_READWRITE( m1_snd_mpcm1_r, m1_snd_mpcm1_w )
	AM_RANGE(0xc70000, 0xc70001) AM_WRITE( m1_snd_mpcm1_bnk_w )
	AM_RANGE(0xd00000, 0xd00007) AM_READWRITE( m1_snd_ym_r, m1_snd_ym_w )
	AM_RANGE(0xf00000, 0xf0ffff) AM_RAM
ADDRESS_MAP_END

static struct MultiPCM_interface m1_multipcm_interface =
{
	2,
	{ 8000000, 8000000 },
	{ MULTIPCM_MODE_MODEL1, MULTIPCM_MODE_MODEL1 },
	{ (1024*1024), (1024*1024) },
	{ REGION_SOUND1, REGION_SOUND2 },
	{ YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT), YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT) }
};

static struct YM2612interface m1_ym3438_interface =
{
	1,
	8000000,
	{ 60,60 },
	{ 0 },	{ 0 },	{ 0 },	{ 0 }
};

INPUT_PORTS_START( vf1 )
	PORT_START  /* Unused analog port 0 */
	PORT_START  /* Unused analog port 1 */
	PORT_START  /* Unused analog port 2 */
	PORT_START  /* Unused analog port 3 */
	PORT_START  /* Unused analog port 4 */
	PORT_START  /* Unused analog port 5 */
	PORT_START  /* Unused analog port 6 */
	PORT_START  /* Unused analog port 7 */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
INPUT_PORTS_END

INPUT_PORTS_START( vr )
	PORT_START	/* Steering */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(3)

	PORT_START	/* Accel / Decel */
	PORT_BIT( 0xff, 0x30, IPT_PEDAL ) PORT_MINMAX(1,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(16)

	PORT_START	/* Brake */
	PORT_BIT( 0xff, 0x30, IPT_PEDAL ) PORT_MINMAX(1,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(16) PORT_PLAYER(2)

	PORT_START  /* Unused analog port 3 */
	PORT_START  /* Unused analog port 4 */
	PORT_START  /* Unused analog port 5 */
	PORT_START  /* Unused analog port 6 */
	PORT_START  /* Unused analog port 7 */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( wingwar )
	PORT_START	/* X */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4)

	PORT_START	/* Y */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE

	PORT_START	/* Throttle */
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(1,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(16)

	PORT_START  /* Unused analog port 3 */
	PORT_START  /* Unused analog port 4 */
	PORT_START  /* Unused analog port 5 */
	PORT_START  /* Unused analog port 6 */
	PORT_START  /* Unused analog port 7 */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( swa )
	PORT_START	/* X */
	PORT_BIT( 0xff, 127, IPT_AD_STICK_X ) PORT_MINMAX(27,227) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE

	PORT_START	/* Y */
	PORT_BIT( 0xff, 127, IPT_AD_STICK_Y ) PORT_MINMAX(27,227) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE

	PORT_START	/* Throttle */
	PORT_BIT( 0xff, 228, IPT_PEDAL ) PORT_MINMAX(28,228) PORT_SENSITIVITY(100) PORT_KEYDELTA(16) PORT_REVERSE

	PORT_START  /* Unused analog port 3 */

	PORT_START	/* X */
	PORT_BIT( 0xff, 127, IPT_AD_STICK_X ) PORT_MINMAX(27,227) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE PORT_PLAYER(2)

	PORT_START	/* Y */
	PORT_BIT( 0xff, 127, IPT_AD_STICK_Y ) PORT_MINMAX(27,227) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_PLAYER(2)

	PORT_START  /* Unused analog port 6 */
	PORT_START  /* Unused analog port 7 */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

ROM_START( vf1 )
	ROM_REGION( 0x1400000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_BYTE( "vf_16082.rom", 0x200000, 0x80000, CRC(b23f22ee) SHA1(9fd5b5a5974703a60a54de3d2bce4301bfc0e533) )
	ROM_LOAD16_BYTE( "vf_16083.rom", 0x200001, 0x80000, CRC(d12c77f8) SHA1(b4aeba8d5f1ab4aec024391407a2cb58ce2e94b0) )

	ROM_LOAD( "vf_16080.rom", 0xfc0000, 0x20000, CRC(3662E1A5) SHA1(6bfceb1a7c1c7912679c907f2b7516ae9c7dda67) )
	ROM_LOAD( "vf_16081.rom", 0xfe0000, 0x20000, CRC(6DEC06CE) SHA1(7891544456bccd2fc647bccd058945ad50466636) )

	ROM_LOAD16_BYTE( "vf_16084.rom", 0x1000000, 0x80000, CRC(483f453b) SHA1(41a5527be73f5dd1c87b2a8113235bdd247ec049) )
	ROM_LOAD16_BYTE( "vf_16085.rom", 0x1000001, 0x80000, CRC(5fa01277) SHA1(dfa7ddff0a7daf29071431f26b93dd8e8e5793b6) )
	ROM_LOAD16_BYTE( "vf_16086.rom", 0x1100000, 0x80000, CRC(deac47a1) SHA1(3a8016124e4dc579d4aae745d4af1905ad0e4fbd) )
	ROM_LOAD16_BYTE( "vf_16087.rom", 0x1100001, 0x80000, CRC(7a64daac) SHA1(da6a9cad4b0cb2af4299e664c0889f3fbdc25530) )
	ROM_LOAD16_BYTE( "vf_16088.rom", 0x1200000, 0x80000, CRC(fcda2d1e) SHA1(0f7d0f604d429a1da0d1c3f31694520bada49680) )
	ROM_LOAD16_BYTE( "vf_16089.rom", 0x1200001, 0x80000, CRC(39befbe0) SHA1(362c493092cd0536fadee7326ecc7f973e23fb58) )
	ROM_LOAD16_BYTE( "vf_16090.rom", 0x1300000, 0x80000, CRC(90c76831) SHA1(5a3c25f2a131cfbb2ad067bef1ab7b1c95645d41) )
	ROM_LOAD16_BYTE( "vf_16091.rom", 0x1300001, 0x80000, CRC(53115448) SHA1(af798d5b1fcb720d7288a5ac48839d9ace16a2f2) )

	ROM_REGION( 0xc0000, REGION_CPU2, 0 )  /* 68K code */
	ROM_LOAD16_WORD_SWAP( "vf_16120.rom", 0x00000, 0x20000, CRC(2bff8378) SHA1(854b08ab983e4e98cb666f2f44de9a6829b1eb52) )
	ROM_LOAD16_WORD_SWAP( "vf_16121.rom", 0x20000, 0x20000, CRC(ff6723f9) SHA1(53498b8c103745883657dfd6efe27edfd48b356f) )
	ROM_RELOAD( 0x80000, 0x20000)

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "vf_16122.rom", 0x000000, 0x200000, CRC(568bc64e) SHA1(31fd0ef8319efe258011b4621adebb790b620770) )
	ROM_LOAD( "vf_16123.rom", 0x200000, 0x200000, CRC(15d78844) SHA1(37c17e38604cf7004a951408024941cd06b1d93e) )

	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) /* Samples */
	ROM_LOAD( "vf_16124.rom", 0x000000, 0x200000, CRC(45520ba1) SHA1(c33e3c12639961016e5fa6b5025d0a67dff28907) )
	ROM_LOAD( "vf_16125.rom", 0x200000, 0x200000, CRC(9b4998b6) SHA1(0418d9b0acf79f35d0f7575c21f1be9a0ea343da) )

	ROM_REGION32_LE( 0x1000000, REGION_USER1, 0 ) /* TGP model roms */
	ROM_LOAD32_WORD( "vf_16096.rom", 0x000000, 0x200000, CRC(a92b0bf3) SHA1(fd3adff5f41f0b0be98df548c848eda04fc0da48) )
	ROM_LOAD32_WORD( "vf_16097.rom", 0x000002, 0x200000, CRC(0232955a) SHA1(df934fb6d022032620932571ff5ed176d5dcb017) )
	ROM_LOAD32_WORD( "vf_16098.rom", 0x400000, 0x200000, CRC(cf2e1b84) SHA1(f3d16c72344f7f218a792ce7f1dd7cad910a8c97) )
	ROM_LOAD32_WORD( "vf_16099.rom", 0x400002, 0x200000, CRC(20e46854) SHA1(423d3642bd2f14e68d29029c027b791de2c1ec53) )
	ROM_LOAD32_WORD( "vf_16100.rom", 0x800000, 0x200000, CRC(e13e983d) SHA1(120637caa2404ad4124b676fd6fcd721f30948df) )
	ROM_LOAD32_WORD( "vf_16101.rom", 0x800002, 0x200000, CRC(0dbed94d) SHA1(df1cddcc1d3976816bd786c2d6211a8563f6f690) )
	ROM_LOAD32_WORD( "vf_16102.rom", 0xc00000, 0x200000, CRC(4cb41fb6) SHA1(4a07bfad4f221508de8c931861424dcc5be3f46a) )
	ROM_LOAD32_WORD( "vf_16103.rom", 0xc00002, 0x200000, CRC(526d1c76) SHA1(edc8dafc9261cd0e970c3b50e3c1ca51a32a4cdf) )
ROM_END

ROM_START( vr )
	ROM_REGION( 0x1400000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_BYTE( "vrrmem.14", 0x200000, 0x80000, CRC(547D75AD) SHA1(a57c11966886c37de1d7df131ad60457669231dd) )
	ROM_LOAD16_BYTE( "vrrmem.15", 0x200001, 0x80000, CRC(6BFAD8B1) SHA1(c1f780e456b405abd42d92f4e03e40aad88f8c22) )

	ROM_LOAD( "vrrmem.4", 0xfc0000, 0x20000, CRC(6D69E695) SHA1(12d3612d3dfd474b8020cdfb8ffc5dcc64e2e1a3) )
	ROM_LOAD( "vrrmem.5", 0xfe0000, 0x20000, CRC(D45AF9DD) SHA1(48f2bf940c78e3ae4273a56e9f32371d870e41e0) )

	ROM_LOAD16_BYTE( "vrrmem.6",  0x1000000, 0x80000, CRC(ADC7C208) SHA1(967b6f522011f17fd2821ccbe20bcb6d4680a4a0) )
	ROM_LOAD16_BYTE( "vrrmem.7",  0x1000001, 0x80000, CRC(E5AB89DF) SHA1(873dea86628457e69f732158e3efb05d133eaa44) )
	ROM_LOAD16_BYTE( "vrrmem.8",  0x1100000, 0x80000, CRC(6CF9C026) SHA1(f4d717958dba6b6402f5ffe7638fe0bf353b61ed) )
	ROM_LOAD16_BYTE( "vrrmem.9",  0x1100001, 0x80000, CRC(F65C9262) SHA1(511a22bcfaf199737bfc5a809fd66cb4439dd386) )
	ROM_LOAD16_BYTE( "vrrmem.10", 0x1200000, 0x80000, CRC(92868734) SHA1(e1dfb134dc3ba7e0b1d40681621e09ac5a222518) )
	ROM_LOAD16_BYTE( "vrrmem.11", 0x1200001, 0x80000, CRC(10C7C636) SHA1(c77d55460bba4354349e473e129f21afeed05e91) )
	ROM_LOAD16_BYTE( "vrrmem.12", 0x1300000, 0x80000, CRC(04BFDC5B) SHA1(bb8788a761620d0440a62ae51c3b41f70a04b5e4) )
	ROM_LOAD16_BYTE( "vrrmem.13", 0x1300001, 0x80000, CRC(C49F0486) SHA1(cc2bb9059c016ba2c4f6e7508bd1687df07b8b48) )

	ROM_REGION( 0xc0000, REGION_CPU2, 0 )  /* 68K code */
	ROM_LOAD16_WORD_SWAP( "vrrsnd.7", 0x00000, 0x20000, CRC(919d9b75) SHA1(27be79881cc9a2b5cf37e18f1e2d87251426b428) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "vrrsnd.32", 0x000000, 0x200000, CRC(b1965190) SHA1(fc47e9ed4a44d48477bd9a35e42c26508c0f4a0c) )

	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) /* Samples */
	ROM_LOAD( "vrrsndo.4", 0x000000, 0x200000, CRC(ba6b2327) SHA1(02285520624a4e612cb4b65510e3458b13b1c6ba) )

	ROM_REGION32_LE( 0x1000000, REGION_USER1, 0 ) /* TGP model roms */
	ROM_LOAD32_WORD( "vrrmem.26", 0x000000, 0x200000, CRC(dcbe006b) SHA1(195be7fabec405ca1b4e1338d3b8d7bb4a06dd73) )
	ROM_LOAD32_WORD( "vrrmem.27", 0x000002, 0x200000, CRC(25832b38) SHA1(a8d74538149c92bae661334e327b031eaca2a8f2) )
	ROM_LOAD32_WORD( "vrrmem.28", 0x400000, 0x200000, CRC(5136f3ba) SHA1(ce8312975764db09bbfa2068b99559a5c1798a36) )
	ROM_LOAD32_WORD( "vrrmem.29", 0x400002, 0x200000, CRC(1c531ada) SHA1(8b373ac97a3649c64f48eb3d1dd95c5833f330b6) )
	ROM_LOAD32_WORD( "vrrmem.30", 0x800000, 0x200000, CRC(830a71bc) SHA1(884378e8a5afeb819daf5285d0d205986d566340) )
	ROM_LOAD32_WORD( "vrrmem.31", 0x800002, 0x200000, CRC(af027ac5) SHA1(523f03d90358ddb7d0e96fd06b9a65cebfc09f24) )
	ROM_LOAD32_WORD( "vrrmem.32", 0xc00000, 0x200000, CRC(382091dc) SHA1(efa266f0f6bfe36ad1c365e588fff33b01e166dd) )
	ROM_LOAD32_WORD( "vrrmem.33", 0xc00002, 0x200000, CRC(74873195) SHA1(80705ec577d14570f9bba77cc26766f831c41f42) )
ROM_END

ROM_START( swa )
	ROM_REGION( 0x1200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_BYTE( "16468.14", 0x200000, 0x80000, CRC(681d03c0) SHA1(4d21e26ce211466d429b84bca69a8147ff31ec6c) )
	ROM_LOAD16_BYTE( "16469.15", 0x200001, 0x80000, CRC(6f281f7c) SHA1(6a9179e48d14838bb2a1a3f63fdd3a68ed009e03) )

	ROM_LOAD( "16467.5", 0xf80000, 0x80000, CRC(605068f5) SHA1(99d7e171ce3353477c282d7567dedb9947206f14) )
	ROM_RELOAD(          0x000000, 0x80000 )
	ROM_RELOAD(          0x080000, 0x80000 )

	ROM_LOAD16_BYTE( "16472.39",  0x1000000, 0x80000, CRC(5a0d7553) SHA1(ba8e08e5a0c6b7fbc10084ad7ad3edf61efb0d70) )
	ROM_LOAD16_BYTE( "16473.40",  0x1000001, 0x80000, CRC(876c5399) SHA1(be7e40c77a385600941f11c24852cd73c71696f0) )
	ROM_LOAD16_BYTE( "16474.41",  0x1100000, 0x80000, CRC(5864a26f) SHA1(be0c22dfff37408f6b401b1970f7fcc6fc7fbcd2) )
	ROM_LOAD16_BYTE( "16475.42",  0x1100001, 0x80000, CRC(b9266be9) SHA1(cf195cd89c9d191b9eb8c5299f8cc154c2b4bd82) )

	ROM_REGION( 0xc0000, REGION_CPU2, 0 )  /* 68K code - missing */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - missing */

	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) /* Samples - missing */

	ROM_REGION32_LE( 0xc00000, REGION_USER1, 0 ) /* TGP model roms */
	ROM_LOAD32_WORD( "16476.26", 0x000000, 0x200000, CRC(d48609ae) SHA1(8c8686a5c9ca4837447a7f70ed194e2f1882b66d) )
	ROM_LOAD32_WORD( "16477.27", 0x000002, 0x200000, CRC(b979b082) SHA1(0c60d259093e987f3856730b57b43bde7e9562e3) )
	ROM_LOAD32_WORD( "16478.28", 0x400000, 0x200000, CRC(80c780f7) SHA1(2f57c5373b02765d302bcd81e24f7b7bc4181387) )
	ROM_LOAD32_WORD( "16479.29", 0x400002, 0x200000, CRC(e43183b3) SHA1(4e62c67cdf7a6fdac0ded86d5f9e81044b9dea8d) )
	ROM_LOAD32_WORD( "16480.30", 0x800000, 0x200000, CRC(3185547a) SHA1(9871937372c2c755717802117a3ad39e1a11410e) )
	ROM_LOAD32_WORD( "16481.31", 0x800002, 0x200000, CRC(ce8d76fe) SHA1(0406f0500d19d6707515627b4143f92a9a5db769) )
ROM_END

ROM_START( wingwar )
	ROM_REGION( 0x1500000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_BYTE( "16729.14", 0x200000, 0x80000, CRC(7edec2cc) SHA1(3e423a868ca7c8475fbb5bc1a10526e69d94d865) )
	ROM_LOAD16_BYTE( "16730.15", 0x200001, 0x80000, CRC(bab24dee) SHA1(26c95139c1aa7f34b6a5cce39e5bd1dd2ef0dd49) )

	ROM_LOAD( "16951.4", 0xfc0000, 0x20000, CRC(8df5a798) SHA1(ef2756f237933ecf429dab0f362e572eb1965f4d) )
	ROM_RELOAD(          0x000000, 0x20000 )
	ROM_LOAD( "16950.5", 0xfe0000, 0x20000, CRC(841e2195) SHA1(66f465aaf71955496e6f83335f7b836ad1d8c724) )
	ROM_RELOAD(          0x020000, 0x20000 )

	ROM_LOAD16_BYTE( "16738.6",  0x1000000, 0x80000, CRC(51518ffa) SHA1(e4674ddfed4205957b14e133c6fdf6454872f324) )
	ROM_LOAD16_BYTE( "16737.7",  0x1000001, 0x80000, CRC(37b1379c) SHA1(98620c324268e1dd906c077ac8a8cd903b9de1f7) )
	ROM_LOAD16_BYTE( "16736.8",  0x1100000, 0x80000, CRC(10b6a025) SHA1(7a4f624ceb7c0b92044a5db8ff55440562ef836b) )
	ROM_LOAD16_BYTE( "16735.9",  0x1100001, 0x80000, CRC(c82fd198) SHA1(d9e53ae1e14dfc8e84a14c0026ef0b904863bb1b) )
	ROM_LOAD16_BYTE( "16734.10", 0x1200000, 0x80000, CRC(f76371c1) SHA1(0ff082db3877383d0dd977dc60c932b725e3d164) )
	ROM_LOAD16_BYTE( "16733.11", 0x1200001, 0x80000, CRC(e105847b) SHA1(8489a6c91fd6d1e9ba81e8eaf36c514da30dccbe) )
	ROM_LOAD16_BYTE( "16741.39", 0x1300000, 0x80000, CRC(84b2ffd8) SHA1(0eba3855d20b88567c6fa08046e12429664d87cb) )
	ROM_LOAD16_BYTE( "16742.40", 0x1300001, 0x80000, CRC(e9cc12bb) SHA1(40c83c968be3b11fad193a00e7b760f074450683) )
	ROM_LOAD16_BYTE( "16739.41", 0x1400000, 0x80000, CRC(6c73e98f) SHA1(7b31e62922ab6d0df97c3ecc52b78e6d086c8635) )
	ROM_LOAD16_BYTE( "16740.42", 0x1400001, 0x80000, CRC(44b31007) SHA1(4bb265fea25a7bbcbb8ab080fdcf09849b18f1de) )

	ROM_REGION( 0xc0000, REGION_CPU2, 0 )  /* 68K code - missing */
	ROM_LOAD16_WORD_SWAP("16751.epr", 0x000000, 0x20000, CRC(23ba5ebc) SHA1(b98aab546c5e980baeedbada4e7472eb4c588260) )
	ROM_LOAD16_WORD_SWAP("16752.epr", 0x020000, 0x20000, CRC(6541c48f) SHA1(9341eff160e31a8574b9545fafc1c4059323fa0c) )
	ROM_RELOAD(0x80000, 0x20000)

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - missing */
	ROM_LOAD("16753.mpr", 0x000000, 0x200000, CRC(324a8333) SHA1(960342e08db637c6f72615d49cffd9fb0889620b) )
	ROM_LOAD("16754.mpr", 0x200000, 0x200000, CRC(144f3cf5) SHA1(d2f8cc9086affbbc5ef2195272200230f724e5d1) )

	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) /* Samples - missing */
	ROM_LOAD("16755.mpr", 0x000000, 0x200000, CRC(4baaf878) SHA1(661d4ea9be6a4952852d0ef94becee7ed42bf4a1) )
	ROM_LOAD("16756.mpr", 0x200000, 0x200000, CRC(d9c40672) SHA1(83e6f1156b30888d3a00103f079dc74f4fca8446) )

	ROM_REGION32_LE( 0x1000000, REGION_USER1, 0 ) /* TGP model roms */
	ROM_LOAD32_WORD( "16743.26", 0x000000, 0x200000, CRC(a710d33c) SHA1(1d0184545b34789ed511caaa25d57db3cd9a8e2f) )
	ROM_LOAD32_WORD( "16744.27", 0x000002, 0x200000, CRC(de796e1f) SHA1(397efb86a21b178770f29d2464bacf5f893619a0) )
	ROM_LOAD32_WORD( "16745.28", 0x400000, 0x200000, CRC(905b689c) SHA1(685dec2a65d5b3a386bda0af1bb5ae7e2b73a01a) )
	ROM_LOAD32_WORD( "16746.29", 0x400002, 0x200000, CRC(163b312e) SHA1(6b45007d6a9d17c8a0b46d81ec84ce9bfefb1695) )
	ROM_LOAD32_WORD( "16747.30", 0x800000, 0x200000, CRC(7353bb12) SHA1(608c5d561e909b8ba31d53db18e6e199855eaaec) )
	ROM_LOAD32_WORD( "16748.31", 0x800002, 0x200000, CRC(8ce98d3a) SHA1(1978776a0e2ea817508e30ba232d5f8d9c158f70) )
	ROM_LOAD32_WORD( "16749.32", 0xc00000, 0x200000, CRC(0e36dc1a) SHA1(4939177a6ac51ca57d0acd118ff14af4f4e438bb) )
	ROM_LOAD32_WORD( "16750.33", 0xc00002, 0x200000, CRC(e4f0b98d) SHA1(e69de2e5ccea2834fb8326bdd61fc6b517bc60b7) )
ROM_END

static MACHINE_DRIVER_START( model1 )
	MDRV_CPU_ADD(V60, 16000000/12) // Reality is 16Mhz
	MDRV_CPU_PROGRAM_MAP(model1_mem, 0)
	MDRV_CPU_IO_MAP(model1_io, 0)
	MDRV_CPU_VBLANK_INT(model1_interrupt, 2)

	MDRV_CPU_ADD(M68000, 12000000)	// Confirmed 10 MHz on real PCB, run slightly faster here to prevent sync trouble
	MDRV_CPU_PROGRAM_MAP(model1_snd, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(model1)
	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(62*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 62*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(model1)
	MDRV_VIDEO_UPDATE(model1)
	MDRV_VIDEO_EOF(model1)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM3438, m1_ym3438_interface)
	MDRV_SOUND_ADD(MULTIPCM, m1_multipcm_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( model1nosnd )
	MDRV_CPU_ADD(V60, 16000000/12) // Reality is 16Mhz
	MDRV_CPU_PROGRAM_MAP(model1_mem, 0)
	MDRV_CPU_IO_MAP(model1_io, 0)
	MDRV_CPU_VBLANK_INT(swa_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(model1)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(62*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 62*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(model1)
	MDRV_VIDEO_UPDATE(model1)
	MDRV_VIDEO_EOF(model1)
MACHINE_DRIVER_END

GAMEX( 1993, vf1,      0, model1, vf1,      0, ROT0, "Sega", "Virtua Fighter 1", GAME_NOT_WORKING )
GAMEX( 1992, vr,       0, model1, vr,       0, ROT0, "Sega", "Virtua Racing", GAME_NOT_WORKING )
GAMEX( 1993, swa,      0, model1nosnd, swa,      0, ROT0, "Sega", "Star Wars Arcade", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1994, wingwar,  0, model1, wingwar,  0, ROT0, "Sega", "Wing War", GAME_NOT_WORKING )
