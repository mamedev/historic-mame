/*************************************************************************

    Driver for Midway Zeus games

    driver by Aaron Giles

    Games supported:
        * Invasion
        * Mortal Kombat 4
        * Cruis'n Exotica
        * The Grid

    Known bugs:
        * not done yet

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/adsp2100/adsp2100.h"
#include "includes/midzeus.h"
#include "machine/midwayic.h"
#include "audio/dcs.h"


#define CPU_CLOCK		60000000

#define BEAM_DY			3
#define BEAM_DX			3
#define BEAM_XOFFS		20		/* table in the code indicates an offset of 20 with a beam height of 7 */

static UINT32 			gun_control;
static UINT8 			gun_irq_state;
static emu_timer *		gun_timer[2];
static INT32 			gun_x[2], gun_y[2];

static UINT32 *ram_base;
static UINT8 cmos_protected;

static emu_timer *timer[2];

static UINT32 *tms32031_control;


static TIMER_CALLBACK( invasn_gun_callback );



/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_RESET( midzeus )
{
	memcpy(ram_base, memory_region(REGION_USER1), 0x40000*4);

	*(UINT32 *)ram_base *= 2;

	timer[0] = timer_alloc(NULL, NULL);
	timer[1] = timer_alloc(NULL, NULL);

	gun_timer[0] = timer_alloc(invasn_gun_callback, NULL);
	gun_timer[1] = timer_alloc(invasn_gun_callback, NULL);
}



/*************************************
 *
 *  CMOS access
 *
 *************************************/

static WRITE32_HANDLER( cmos_protect_w )
{
	cmos_protected = 0;
}


static WRITE32_HANDLER( cmos_w )
{
	if (!cmos_protected)
		COMBINE_DATA(generic_nvram32 + offset);
}


static READ32_HANDLER( cmos_r )
{
	return generic_nvram32[offset];
}



/*************************************
 *
 *  TMS32031 I/O accesses
 *
 *************************************/

static READ32_HANDLER( tms32031_control_r )
{
	/* watch for accesses to the timers */
	if (offset == 0x24 || offset == 0x34)
	{
		/* timer is clocked at 100ns */
		int which = (offset >> 4) & 1;
		INT32 result = attotime_to_double(attotime_mul(timer_timeelapsed(timer[which]), 10000000));
		return result;
	}

	/* log anything else except the memory control register */
	if (offset != 0x64)
		logerror("%06X:tms32031_control_r(%02X)\n", activecpu_get_pc(), offset);

	return tms32031_control[offset];
}


static WRITE32_HANDLER( tms32031_control_w )
{
	COMBINE_DATA(&tms32031_control[offset]);

	/* ignore changes to the memory control register */
	if (offset == 0x64)
		;

	/* watch for accesses to the timers */
	else if (offset == 0x20 || offset == 0x30)
	{
		int which = (offset >> 4) & 1;
		if (data & 0x40)
			timer_adjust(timer[which], attotime_never, 0, attotime_never);
	}
	else
		logerror("%06X:tms32031_control_w(%02X) = %08X\n", activecpu_get_pc(), offset, data);
}


/*************************************
 *
 *  Lightgun handling
 *
 *************************************/

static void update_gun_irq(void)
{
	if (gun_irq_state & gun_control & 0x03)
		cpunum_set_input_line(0, 3, ASSERT_LINE);
	else
		cpunum_set_input_line(0, 3, CLEAR_LINE);
}


static TIMER_CALLBACK( invasn_gun_callback )
{
	int player = param;
	int beamy = video_screen_get_vpos(0);

	gun_irq_state |= 0x01 << player;
	update_gun_irq();

	beamy++;
	if (beamy <= machine->screen[0].visarea.max_y && beamy <= gun_y[player] + BEAM_DY)
		timer_adjust(gun_timer[player], video_screen_get_time_until_pos(0, beamy, MAX(0, gun_x[player] - BEAM_DX)), player, attotime_never);
}


static WRITE32_HANDLER( invasn_gun_w )
{
	UINT32 old_control = gun_control;
	int player;

	COMBINE_DATA(&gun_control);

	/* bits 0-1 enable IRQs (?) */
	/* bits 2-3 reset IRQ states */
	gun_irq_state &= ~((gun_control >> 2) & 3);
	update_gun_irq();

	for (player = 0; player < 2; player++)
	{
		UINT8 pmask = 0x04 << player;
		if (((old_control ^ gun_control) & pmask) != 0 && (gun_control & pmask) == 0)
		{
			const rectangle *visarea = &Machine->screen[0].visarea;
			static const char *const names[2][2] =
			{
				{ "GUNX1", "GUNY1" },
				{ "GUNX2", "GUNY2" }
			};
			gun_x[player] = readinputportbytag(names[player][0]) * (visarea->max_x + 1 - visarea->min_x) / 255 + visarea->min_x + BEAM_XOFFS;
			gun_y[player] = readinputportbytag(names[player][1]) * (visarea->max_y + 1 - visarea->min_y) / 255 + visarea->min_y;
			timer_adjust(gun_timer[player], video_screen_get_time_until_pos(0, MAX(0, gun_y[player] - BEAM_DY), MAX(0, gun_x[player] - BEAM_DX)), player, attotime_never);
		}
	}
}

static READ32_HANDLER( invasn_gun_r )
{
	int beamx = video_screen_get_hpos(0);
	int beamy = video_screen_get_vpos(0);
	UINT32 result = 0xffff;
	int player;

	for (player = 0; player < 2; player++)
	{
		int diffx = beamx - gun_x[player];
		int diffy = beamy - gun_y[player];
		if (diffx >= -BEAM_DX && diffx <= BEAM_DX && diffy >= -BEAM_DY && diffy <= BEAM_DY)
			result ^= 0x1000 << player;
	}
	return result;
}


/*************************************
 *
 *  Memory maps
 *
 *************************************/


// read 8d0003, check bit 1, skip some stuff if 0
// write junk to 9e0000

static UINT32 *unknown_8d0000;
static READ32_HANDLER( unknown_8d0000_r )
{
	return unknown_8d0000[offset];
}
static WRITE32_HANDLER( unknown_8d0000_w )
{
	logerror("%06X:write to %06X = %08X\n", activecpu_get_pc(), 0x8d0000 + offset, data);
	COMBINE_DATA(&unknown_8d0000[offset]);
}


static ADDRESS_MAP_START( zeus_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x03ffff) AM_RAM AM_BASE(&ram_base)
	AM_RANGE(0x400000, 0x41ffff) AM_RAM
	AM_RANGE(0x808000, 0x80807f) AM_READWRITE(tms32031_control_r, tms32031_control_w) AM_BASE(&tms32031_control)
	AM_RANGE(0x880000, 0x8803ff) AM_READWRITE(zeus_r, zeus_w) AM_BASE(&zeusbase)
	AM_RANGE(0x8d0000, 0x8d0003) AM_READWRITE(unknown_8d0000_r, unknown_8d0000_w) AM_BASE(&unknown_8d0000)
	AM_RANGE(0x990000, 0x99000f) AM_READWRITE(midway_ioasic_r, midway_ioasic_w)
	AM_RANGE(0x9e0000, 0x9e0000) AM_WRITENOP		// watchdog?
	AM_RANGE(0x9f0000, 0x9f7fff) AM_READWRITE(cmos_r, cmos_w) AM_BASE(&generic_nvram32) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x9f8000, 0x9f8000) AM_WRITE(cmos_protect_w)
	AM_RANGE(0xa00000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END


static ADDRESS_MAP_START( zeus2_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x03ffff) AM_RAM AM_BASE(&ram_base)
	AM_RANGE(0x400000, 0x43ffff) AM_RAM
	AM_RANGE(0x808000, 0x80807f) AM_READWRITE(tms32031_control_r, tms32031_control_w) AM_BASE(&tms32031_control)
	AM_RANGE(0x880000, 0x8801ff) AM_READWRITE(zeus2_r, zeus2_w) AM_BASE(&zeusbase)
	AM_RANGE(0x8d0000, 0x8d0003) AM_READWRITE(unknown_8d0000_r, unknown_8d0000_w) AM_BASE(&unknown_8d0000)
	AM_RANGE(0x990000, 0x99000f) AM_READWRITE(midway_ioasic_r, midway_ioasic_w)
	AM_RANGE(0x9e0000, 0x9e0000) AM_WRITENOP		// watchdog?
	AM_RANGE(0x9f0000, 0x9f7fff) AM_READWRITE(cmos_r, cmos_w) AM_BASE(&generic_nvram32) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x9f8000, 0x9f8000) AM_WRITE(cmos_protect_w)
	AM_RANGE(0xa00000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

static INPUT_PORTS_START( mk4 )
	PORT_START	    /* DS1 */
	PORT_DIPNAME( 0x0001, 0x0001, "Coinage Source" )
	PORT_DIPSETTING(      0x0001, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x003e, 0x003e, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x003e, "USA-1" )
	PORT_DIPSETTING(      0x003c, "USA-2" )
	PORT_DIPSETTING(      0x003a, "USA-3" )
	PORT_DIPSETTING(      0x0038, "USA-4" )
	PORT_DIPSETTING(      0x0034, "USA-9" )
	PORT_DIPSETTING(      0x0032, "USA-11" )
	PORT_DIPSETTING(      0x0036, "USA-ECA" )
	PORT_DIPSETTING(      0x002e, "German-1" )
	PORT_DIPSETTING(      0x002c, "German-2" )
	PORT_DIPSETTING(      0x002a, "German-3" )
	PORT_DIPSETTING(      0x0028, "German-4" )
	PORT_DIPSETTING(      0x0026, "German-ECA" )
	PORT_DIPSETTING(      0x001e, "French-1" )
	PORT_DIPSETTING(      0x001c, "French-2" )
	PORT_DIPSETTING(      0x001a, "French-3" )
	PORT_DIPSETTING(      0x0018, "French-4" )
	PORT_DIPSETTING(      0x0016, "French-ECA" )
	PORT_DIPSETTING(      0x0030, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ))	/* Manual lists this dip as Unused */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, "Test Switch" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0100, 0x0100, "Fatalities" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0100, DEF_STR( On ))
 	PORT_DIPNAME( 0x0200, 0x0200, "Blood" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0200, DEF_STR( On ))
 	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown )) /* Manual states that switches 3-7 are Unused */
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ))
 	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
 	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME(DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BILL1 )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static INPUT_PORTS_START( invasn )
	PORT_START	    /* DS1 */
	PORT_DIPNAME( 0x0001, 0x0001, "Coinage Source" )
	PORT_DIPSETTING(      0x0001, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x003e, 0x003e, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x003e, "USA-1" )
	PORT_DIPSETTING(      0x003c, "USA-2" )
	PORT_DIPSETTING(      0x003a, "USA-3" )
	PORT_DIPSETTING(      0x0038, "USA-4" )
	PORT_DIPSETTING(      0x0034, "USA-9" )
	PORT_DIPSETTING(      0x0032, "USA-10" )
	PORT_DIPSETTING(      0x0036, "USA-ECA" )
	PORT_DIPSETTING(      0x002e, "German-1" )
	PORT_DIPSETTING(      0x002c, "German-2" )
	PORT_DIPSETTING(      0x002a, "German-3" )
	PORT_DIPSETTING(      0x0028, "German-4" )
	PORT_DIPSETTING(      0x0024, "German-5" )
	PORT_DIPSETTING(      0x0026, "German-ECA" )
	PORT_DIPSETTING(      0x001e, "French-1" )
	PORT_DIPSETTING(      0x001c, "French-2" )
	PORT_DIPSETTING(      0x001a, "French-3" )
	PORT_DIPSETTING(      0x0018, "French-4" )
	PORT_DIPSETTING(      0x0014, "French-11" )
	PORT_DIPSETTING(      0x0012, "French-12" )
	PORT_DIPSETTING(      0x0016, "French-ECA" )
	PORT_DIPSETTING(      0x0030, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0040, 0x0040, "Flip Y" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0000, "Test Switch" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Mirrored Display" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Show Blood" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0200, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME(DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BILL1 )	/* Bill */

	PORT_START
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0f00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("GUNX1")		/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START_TAG("GUNY1")		/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START_TAG("GUNX2")		/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START_TAG("GUNY2")		/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END


static INPUT_PORTS_START( crusnexo )
	PORT_START	    /* DS1 */
 	PORT_DIPNAME( 0x0001, 0x0001, "Game Type" )	/* Manual states "*DIP 1, Switch 1 MUST be set */
 	PORT_DIPSETTING(      0x0001, "Dedicated" )	/*   to OFF position for proper operation" */
 	PORT_DIPSETTING(      0x0000, "Kit" )
 	PORT_DIPNAME( 0x0002, 0x0002, "Seat Motion" )	/* For dedicated Sit Down models with Motion Seat */
  	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
  	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Cabinet ))
 	PORT_DIPSETTING(      0x0004, "Stand Up" )
 	PORT_DIPSETTING(      0x0000, "Sit Down" )
 	PORT_DIPNAME( 0x0008, 0x0008, "Wheel Invert" )
  	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
  	PORT_DIPSETTING(      0x0008, DEF_STR( On ))
 	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ))	/* Manual lists this dip as Unused */
 	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
  	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0020, 0x0020, "Link" )
 	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
  	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x00c0, 0x00c0, "Linking I.D.")
 	PORT_DIPSETTING(      0x00c0, "Master #1" )
 	PORT_DIPSETTING(      0x0080, "Slave #2" )
 	PORT_DIPSETTING(      0x0040, "Slave #3" )
 	PORT_DIPSETTING(      0x0000, "Slave #4" )
 	PORT_DIPNAME( 0x1f00, 0x1f00, "Country Code" )
 	PORT_DIPSETTING(      0x1f00, DEF_STR( USA ) )
 	PORT_DIPSETTING(      0x1e00, "Germany" )
 	PORT_DIPSETTING(      0x1d00, "France" )
 	PORT_DIPSETTING(      0x1c00, "Canada" )
 	PORT_DIPSETTING(      0x1b00, "Switzerland" )
 	PORT_DIPSETTING(      0x1a00, "Italy" )
 	PORT_DIPSETTING(      0x1900, "UK" )
 	PORT_DIPSETTING(      0x1800, "Spain" )
 	PORT_DIPSETTING(      0x1700, "Austrilia" )
 	PORT_DIPSETTING(      0x1600, DEF_STR( Japan ) )
 	PORT_DIPSETTING(      0x1500, "Taiwan" )
 	PORT_DIPSETTING(      0x1400, "Austria" )
 	PORT_DIPSETTING(      0x1300, "Belgium" )
 	PORT_DIPSETTING(      0x0f00, "Sweden" )
 	PORT_DIPSETTING(      0x0e00, "Findland" )
	PORT_DIPSETTING(      0x0d00, "Netherlands" )
 	PORT_DIPSETTING(      0x0c00, "Norway" )
 	PORT_DIPSETTING(      0x0b00, "Denmark" )
 	PORT_DIPSETTING(      0x0a00, "Hungary" )
 	PORT_DIPSETTING(      0x0800, "General" )
 	PORT_DIPNAME( 0x6000, 0x6000, "Coin Mode" )
 	PORT_DIPSETTING(      0x6000, "Mode 1" ) /* USA1/GER1/FRA1/SPN1/AUSTRIA1/GEN1/CAN1/SWI1/ITL1/JPN1/TWN1/BLGN1/NTHRLND1/FNLD1/NRWY1/DNMK1/HUN1 */
 	PORT_DIPSETTING(      0x4000, "Mode 2" ) /* USA3/GER1/FRA1/SPN1/AUSTRIA1/GEN3/CAN2/SWI2/ITL2/JPN2/TWN2/BLGN2/NTHRLND2 */
 	PORT_DIPSETTING(      0x2000, "Mode 3" ) /* USA7/GER1/FRA1/SPN1/AUSTRIA1/GEN5/CAN3/SWI3/ITL3/JPN3/TWN3/BLGN3 */
 	PORT_DIPSETTING(      0x0000, "Mode 4" ) /* USA8/GER1/FRA1/SPN1/AUSTRIA1/GEN7 */
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME(DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BILL1 )	/* Bill */

	PORT_START /* Listed "names" are via the manual's "JAMMA" pinout sheet" */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1) PORT_8WAY /* Radio Switch */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) /* View 2 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) /* View 3 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) /* View 1 */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2) PORT_8WAY /* Gear 1 */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2) PORT_8WAY /* Gear 2 */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2) PORT_8WAY /* Gear 3 */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY /* Gear 4 */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) /* Not Used */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) /* Not Used */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) /* Not Used */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static INPUT_PORTS_START( thegrid )
	PORT_START	    /* DS1 */
 	PORT_DIPNAME( 0x0001, 0x0001, "Show Blood" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown )) /* Manual states that switches 2-7 are Unused */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0008, DEF_STR( On ))
 	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ))
 	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
 	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Coinage Source" )
	PORT_DIPSETTING(      0x0100, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x3e00, 0x3e00, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x3e00, "USA-1" )
	PORT_DIPSETTING(      0x3800, "USA-2" )
	PORT_DIPSETTING(      0x3c00, "USA-10" )
	PORT_DIPSETTING(      0x3a00, "USA-14" )
	PORT_DIPSETTING(      0x3600, "USA-DC1" )
	PORT_DIPSETTING(      0x3000, "USA-DC2" )
	PORT_DIPSETTING(      0x3200, "USA-DC4" )
	PORT_DIPSETTING(      0x3400, "USA-DC5" )
	PORT_DIPSETTING(      0x2e00, "French-ECA1" )
	PORT_DIPSETTING(      0x2c00, "French-ECA2" )
	PORT_DIPSETTING(      0x2a00, "French-ECA3" )
	PORT_DIPSETTING(      0x2800, "French-ECA4" )
	PORT_DIPSETTING(      0x2600, "French-ECA5" )
	PORT_DIPSETTING(      0x2400, "French-ECA6" )
	PORT_DIPSETTING(      0x2200, "French-ECA7" )
	PORT_DIPSETTING(      0x2000, "French-ECA8" )
	PORT_DIPSETTING(      0x1e00, "German-1" )
	PORT_DIPSETTING(      0x1c00, "German-2" )
	PORT_DIPSETTING(      0x1a00, "German-3" )
	PORT_DIPSETTING(      0x1800, "German-4" )
	PORT_DIPSETTING(      0x1600, "German-5" )
	PORT_DIPSETTING(      0x1400, "German-ECA1" )
	PORT_DIPSETTING(      0x1200, "German-ECA2" )
	PORT_DIPSETTING(      0x1000, "German-ECA3" )
	PORT_DIPSETTING(      0x0800, "UK-4" )
	PORT_DIPSETTING(      0x0600, "UK-5" )
	PORT_DIPSETTING(      0x0e00, "UK-1 ECA" )
	PORT_DIPSETTING(      0x0c00, "UK-2 ECA" )
	PORT_DIPSETTING(      0x0a00, "UK-3 ECA" )
	PORT_DIPSETTING(      0x0400, "UK-6 ECA" )
	PORT_DIPSETTING(      0x0200, "UK-7 ECA" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ))	/* Manual states switches 7 & 8 are Unused */
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME(DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BILL1 )	/* Bill */

	PORT_START /* Listed "names" are via the manual's "JAMMA" pinout sheet" */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) /* Trigger */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) /* Fire */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) /* Action */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2) PORT_8WAY /* No Connection */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2) PORT_8WAY /* No Connection */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2) PORT_8WAY /* No Connection */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY /* No Connection */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) /* No Connection */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) /* No Connection */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) /* No Connection */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( midzeus )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", TMS32032, CPU_CLOCK)
	MDRV_CPU_PROGRAM_MAP(zeus_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_SCREEN_REFRESH_RATE(57)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(midzeus)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512/*+256*/, 278/*+256*/)
	MDRV_SCREEN_VISIBLE_AREA(0, 399/*+256*/, 0, 255/*+256*/)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(midzeus)
	MDRV_VIDEO_UPDATE(midzeus)

	/* sound hardware */
	MDRV_IMPORT_FROM(dcs2_audio_2104)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( midzeus2 )
	MDRV_IMPORT_FROM(midzeus)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(zeus2_map,0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( mk4 )
	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "mk4_l2.u2", 0x000000, 0x200000, CRC(4c01b493) SHA1(dcd6da9ee30d0f6645bfdb1762d926be0a26090a) )
	ROM_LOAD16_BYTE( "mk4_l2.u3", 0x400000, 0x200000, CRC(8fbcf0ac) SHA1(c53704e72cfcba800c7af3a03267041f1e29a784) )
	ROM_LOAD16_BYTE( "mk4_l2.u4", 0x800000, 0x200000, CRC(dee91696) SHA1(00a182a36a414744cd014fcfc53c2e1a66ab5189) )
	ROM_LOAD16_BYTE( "mk4_l2.u5", 0xc00000, 0x200000, CRC(44d072be) SHA1(8a636c2801d799dfb84e69607ade76d2b49cf09f) )

	ROM_REGION32_LE( 0x1800000, REGION_USER1, 0 )
	ROM_LOAD32_WORD( "mk4_l3.u10", 0x0000000, 0x200000, CRC(84efe5a9) SHA1(e2a9bf6fab971691017371a87ab87b1bf66f96d0) )
	ROM_LOAD32_WORD( "mk4_l3.u11", 0x0000002, 0x200000, CRC(0c026ccb) SHA1(7531fe81ff8d8dd9ec3cd915acaf14cbe6bdc90a) )
	ROM_LOAD32_WORD( "mk4_l3.u12", 0x0400000, 0x200000, CRC(7816c07f) SHA1(da94b4391e671f915c61b5eb9bece4acb3382e31) )
	ROM_LOAD32_WORD( "mk4_l3.u13", 0x0400002, 0x200000, CRC(b3c237cd) SHA1(9e71e60cc92c17524f85f36543c174ca138104cd) )
	ROM_LOAD32_WORD( "mk4_l3.u14", 0x0800000, 0x200000, CRC(fd33eb1a) SHA1(59d9d2e5251679d19cab031f51731c85f429ba18) )
	ROM_LOAD32_WORD( "mk4_l3.u15", 0x0800002, 0x200000, CRC(b907518f) SHA1(cfb56538746895bdca779957fec6a872019b23c3) )
	ROM_LOAD32_WORD( "mk4_l3.u16", 0x0c00000, 0x200000, CRC(24371d57) SHA1(c90134b17c23a182d391d1679bf457d251e641f7) )
	ROM_LOAD32_WORD( "mk4_l3.u17", 0x0c00002, 0x200000, CRC(3a1a082c) SHA1(5f8e8ce760d8ebadd1240ef08f1382a37cf11d0b) )
ROM_END

ROM_START( mk4a )
	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "mk4_l2.2", 0x000000, 0x200000, CRC(4c01b493) SHA1(dcd6da9ee30d0f6645bfdb1762d926be0a26090a) )
	ROM_LOAD16_BYTE( "mk4_l2.3", 0x400000, 0x200000, CRC(8fbcf0ac) SHA1(c53704e72cfcba800c7af3a03267041f1e29a784) )
	ROM_LOAD16_BYTE( "mk4_l2.4", 0x800000, 0x200000, CRC(dee91696) SHA1(00a182a36a414744cd014fcfc53c2e1a66ab5189) )
	ROM_LOAD16_BYTE( "mk4_l2.5", 0xc00000, 0x200000, CRC(44d072be) SHA1(8a636c2801d799dfb84e69607ade76d2b49cf09f) )

	ROM_REGION32_LE( 0x1800000, REGION_USER1, 0 )
	ROM_LOAD32_WORD( "mk4_l2.1.u10", 0x000000, 0x200000, CRC(42d0f1c9) SHA1(5ac0ded8bf6e756319be2691e3b555eac079ebdc) )
	ROM_LOAD32_WORD( "mk4_l2.1.u11", 0x000002, 0x200000, CRC(6e21b243) SHA1(6d4768a5972db05c1409e0d16e79df9eff8918a0) )
	ROM_LOAD32_WORD( "mk4_l3.u12", 0x0400000, 0x200000, CRC(7816c07f) SHA1(da94b4391e671f915c61b5eb9bece4acb3382e31) )
	ROM_LOAD32_WORD( "mk4_l3.u13", 0x0400002, 0x200000, CRC(b3c237cd) SHA1(9e71e60cc92c17524f85f36543c174ca138104cd) )
	ROM_LOAD32_WORD( "mk4_l3.u14", 0x0800000, 0x200000, CRC(fd33eb1a) SHA1(59d9d2e5251679d19cab031f51731c85f429ba18) )
	ROM_LOAD32_WORD( "mk4_l3.u15", 0x0800002, 0x200000, CRC(b907518f) SHA1(cfb56538746895bdca779957fec6a872019b23c3) )
	ROM_LOAD32_WORD( "mk4_l3.u16", 0x0c00000, 0x200000, CRC(24371d57) SHA1(c90134b17c23a182d391d1679bf457d251e641f7) )
	ROM_LOAD32_WORD( "mk4_l3.u17", 0x0c00002, 0x200000, CRC(3a1a082c) SHA1(5f8e8ce760d8ebadd1240ef08f1382a37cf11d0b) )
ROM_END


ROM_START( invasn )
	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "invasion.u2", 0x000000, 0x200000, CRC(59d2e1d6) SHA1(994a4311ac4841d4341449c0c7480952b6f3855d) )
	ROM_LOAD16_BYTE( "invasion.u3", 0x400000, 0x200000, CRC(86b956ae) SHA1(f7fd4601a2ce3e7e9b67e7d77908bfa206ee7e62) )
	ROM_LOAD16_BYTE( "invasion.u4", 0x800000, 0x200000, CRC(5ef1fab5) SHA1(987afa0672fa89b18cf20d28644848a9e5ee9b17) )
	ROM_LOAD16_BYTE( "invasion.u5", 0xc00000, 0x200000, CRC(e42805c9) SHA1(e5b71eb1852809a649ac43a82168b3bdaf4b1526) )

	ROM_REGION32_LE( 0x1800000, REGION_USER1, 0 )
	ROM_LOAD32_WORD( "invasion.u10", 0x0000000, 0x200000, CRC(b3ce958b) SHA1(ed51c167d85bc5f6155b8046ec056a4f4ad5cf9d) )
	ROM_LOAD32_WORD( "invasion.u11", 0x0000002, 0x200000, CRC(0bd09359) SHA1(f40886bd2e5f5fbf506580e5baa2f733be200852) )
	ROM_LOAD32_WORD( "invasion.u12", 0x0400000, 0x200000, CRC(ce1eb06a) SHA1(ff17690a0cbca6dcccccde70e2c5812ae03db5bb) )
	ROM_LOAD32_WORD( "invasion.u13", 0x0400002, 0x200000, CRC(33fc6707) SHA1(11a39ad980ec320547319eca6ffa5aef3ab8b010) )
	ROM_LOAD32_WORD( "invasion.u14", 0x0800000, 0x200000, CRC(760682a1) SHA1(ff91210225d4aa750115c6219d4c35c9521a3f0b) )
	ROM_LOAD32_WORD( "invasion.u15", 0x0800002, 0x200000, CRC(90467d7a) SHA1(a143a3d3605e5626852e75937160ba6bcd891608) )
	ROM_LOAD32_WORD( "invasion.u16", 0x0c00000, 0x200000, CRC(3ef1b28d) SHA1(6f9a071b8830194fea84daa62aadabae86977c5f) )
	ROM_LOAD32_WORD( "invasion.u17", 0x0c00002, 0x200000, CRC(97aa677a) SHA1(4d21cc59e0ffd4985f89c97c71d605c3b404a8a3) )
	ROM_LOAD32_WORD( "invasion.u18", 0x1000000, 0x200000, CRC(6930c656) SHA1(28054ff9a6c6f5764a371f8defe4c1f5730618f3) )
	ROM_LOAD32_WORD( "invasion.u19", 0x1000002, 0x200000, CRC(89fa6ee5) SHA1(572565e1308142b0b062aa72315c68e928f2419c) )
ROM_END


ROM_START( crusnexo )
	ROM_REGION16_LE( 0xc00000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD( "exotica.u2", 0x000000, 0x200000, CRC(d2d54acf) SHA1(2b4d6fda30af807228bb281335939dfb6df9b530) )
	ROM_RELOAD(             0x200000, 0x200000 )
	ROM_LOAD( "exotica.u3", 0x400000, 0x400000, CRC(28a3a13d) SHA1(8d7d641b883df089adefdd144229afef79db9e8a) )
	ROM_LOAD( "exotica.u4", 0x800000, 0x400000, CRC(213f7fd8) SHA1(8528d524a62bc41a8e3b39f0dbeeba33c862ee27) )

	ROM_REGION32_LE( 0x3000000, REGION_USER1, 0 )
	ROM_LOAD32_WORD( "exotica.u10", 0x0000000, 0x200000, CRC(65450140) SHA1(cad41a2cad48426de01feb78d3f71f768e3fc872) )
	ROM_LOAD32_WORD( "exotica.u11", 0x0000002, 0x200000, CRC(e994891f) SHA1(bb088729b665864c7f3b79b97c3c86f9c8f68770) )
	ROM_LOAD32_WORD( "exotica.u12", 0x0400000, 0x200000, CRC(21f122b2) SHA1(5473401ec954bf9ab66a8283bd08d17c7960cd29) )
	ROM_LOAD32_WORD( "exotica.u13", 0x0400002, 0x200000, CRC(cf9d3609) SHA1(6376891f478185d26370466bef92f0c5304d58d3) )
	ROM_LOAD32_WORD( "exotica.u14", 0x0800000, 0x400000, CRC(84452fc2) SHA1(06d87263f83ef079e6c5fb9de620e0135040c858) )
	ROM_LOAD32_WORD( "exotica.u15", 0x0800002, 0x400000, CRC(b6aaebdb) SHA1(6ede6ea123be6a88d1ff38e90f059c9d1f822d6d) )
	ROM_LOAD32_WORD( "exotica.u16", 0x1000000, 0x400000, CRC(aac6d2a5) SHA1(6c336520269d593b46b82414d9352a3f16955cc3) )
	ROM_LOAD32_WORD( "exotica.u17", 0x1000002, 0x400000, CRC(71cf5404) SHA1(a6eed1a66fb4f4ddd749e4272a2cdb8e3e354029) )
	ROM_LOAD32_WORD( "exotica.u18", 0x1800000, 0x200000, CRC(60cf5caa) SHA1(629870a305802d632bd2681131d1ffc0086280d2) )
	ROM_LOAD32_WORD( "exotica.u19", 0x1800002, 0x200000, CRC(6b919a18) SHA1(20e40e195554146ed1d3fad54f7280823ae89d4b) )
	ROM_LOAD32_WORD( "exotica.u20", 0x1c00002, 0x200000, CRC(4855b68b) SHA1(1f6e557590b2621d0d5c782b95577f1be5cbc51d) )
	ROM_LOAD32_WORD( "exotica.u21", 0x1c00002, 0x200000, CRC(0011b9d6) SHA1(231d768c964a16b905857b0814d758fe93c2eefb) )
	ROM_LOAD32_WORD( "exotica.u22", 0x2000002, 0x400000, CRC(ad6dcda7) SHA1(5c9291753e1659f9adbe7e59fa2d0e030efae5bc) )
	ROM_LOAD32_WORD( "exotica.u23", 0x2000002, 0x400000, CRC(1f103a68) SHA1(3b3acc63a461677cd424e75e7211fa6f063a37ef) )
	ROM_LOAD32_WORD( "exotica.u24", 0x2800002, 0x400000, CRC(6312feef) SHA1(4113e4e5d39c99e8131d41a57c973df475b67d18) )
	ROM_LOAD32_WORD( "exotica.u25", 0x2800002, 0x400000, CRC(b8277b16) SHA1(1355e87affd78e195906aedc9aed9e230374e2bf) )
ROM_END


ROM_START( thegrid )
	ROM_REGION16_LE( 0xc00000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD( "the_grid.u2", 0x000000, 0x400000, CRC(e6a39ee9) SHA1(4ddc62f5d278ea9791205098fa5f018ab1e698b4) )
	ROM_LOAD( "the_grid.u3", 0x400000, 0x400000, CRC(40be7585) SHA1(e481081edffa07945412a6eab17b4d3e7b42cfd3) )
	ROM_LOAD( "the_grid.u4", 0x800000, 0x400000, CRC(7a15c203) SHA1(a0a49dd08bba92402640ed2d1fb4fee112c4ab5f) )

	ROM_REGION32_LE( 0x3000000, REGION_USER1, 0 )
 	ROM_LOAD32_WORD( "thegrid-11.u10", 0x0000000, 0x100000, CRC(87ea0e9e) SHA1(618de2ca87b7a3e0225d1f7e65f8fc1356de1421) )
	ROM_LOAD32_WORD( "thegrid-11.u11", 0x0000002, 0x100000, CRC(73d84b1a) SHA1(8dcfcab5ff64f46f8486e6439a10d91ad26fd48a) )
	ROM_LOAD32_WORD( "thegrid-11.u12", 0x0200000, 0x100000, CRC(78d16ca1) SHA1(7b893ec8af2f44d8bc293861fd8622d68d41ccbe) )
	ROM_LOAD32_WORD( "thegrid-11.u13", 0x0200002, 0x100000, CRC(8e00b400) SHA1(96581c5da62afc19e6d69b2352b3166665cb9918) )
	ROM_LOAD32_WORD( "the_grid.u18",   0x0400000, 0x400000, CRC(3a3460be) SHA1(e719dae8a2e54584cb6a074ed42e35e3debef2f6) )
	ROM_LOAD32_WORD( "the_grid.u19",   0x0400002, 0x400000, CRC(af262d5b) SHA1(3eb3980fa81a360a70aa74e793b2bc3028f68cf2) )
	ROM_LOAD32_WORD( "the_grid.u20",   0x0c00002, 0x400000, CRC(e6ad1917) SHA1(acab25e1251fd07b374badebe79f6ec1772b3589) )
	ROM_LOAD32_WORD( "the_grid.u21",   0x0c00002, 0x400000, CRC(48c03f8e) SHA1(50790bdae9f2234ffb4914c2c5c16374e3508b47) )
	ROM_LOAD32_WORD( "the_grid.u22",   0x1400002, 0x400000, CRC(84c3a8b6) SHA1(de0dcf9daf7ada7a6952b9e29a29571b2aa9d0b2) )
	ROM_LOAD32_WORD( "the_grid.u23",   0x1400002, 0x400000, CRC(f48ef409) SHA1(79d74b4fe38b06a02ae0351d13d7f0a7ed0f0c87) )
ROM_END



/*************************************
 *
 *  Driver init
 *
 *************************************/

static DRIVER_INIT( mk4 )
{
	dcs2_init(0, 0);
	midway_ioasic_init(MIDWAY_IOASIC_STANDARD, 461/* or 474 */, 94, NULL);
	midway_ioasic_set_shuffle_state(1);
}


static DRIVER_INIT( invasn )
{
	dcs2_init(0, 0);
	midway_ioasic_init(MIDWAY_IOASIC_STANDARD, 468/* or 488 */, 94, NULL);
	memory_install_readwrite32_handler(0, ADDRESS_SPACE_PROGRAM, 0x9c0000, 0x9c0000, 0, 0, invasn_gun_r, invasn_gun_w);
}


static DRIVER_INIT( crusnexo )
{
	dcs2_init(0, 0);
	midway_ioasic_init(MIDWAY_IOASIC_STANDARD, 461/* unknown */, 94, NULL);
}


static DRIVER_INIT( thegrid )
{
	cpunum_set_input_line(0, INPUT_LINE_HALT, ASSERT_LINE);
	dcs2_init(0, 0);
	midway_ioasic_init(MIDWAY_IOASIC_STANDARD, 461/* unknown */, 94, NULL);
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1997, mk4,      0,     midzeus,  mk4,      mk4,      ROT0, "Midway", "Mortal Kombat 4 (3.0)", GAME_IMPERFECT_GRAPHICS )
GAME( 1997, mk4a,     mk4,   midzeus,  mk4,      mk4,      ROT0, "Midway", "Mortal Kombat 4 (2.1)", GAME_IMPERFECT_GRAPHICS )
GAME( 1999, invasn,   0,     midzeus,  invasn,   invasn,   ROT0, "Midway", "Invasion (Midway)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS )
GAME( 1999, crusnexo, 0,     midzeus2, crusnexo, crusnexo, ROT0, "Midway", "Cruis'n Exotica", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 2001, thegrid,  0,     midzeus2, thegrid,  thegrid,  ROT0, "Midway", "The Grid (version 1.1)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
