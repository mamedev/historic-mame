/*************************************************************************

	Sega g80 common sound hardware

*************************************************************************/

#include "driver.h"
#include "sega.h"
#include "cpu/i8039/i8039.h"

/* SP0250-based speechboard */

static UINT8 sega_speechboard_latch, sega_speechboard_t0, sega_speechboard_p2, sega_speechboard_drq;


static READ8_HANDLER( speechboard_t0_r )
{
	return sega_speechboard_t0;
}

static READ8_HANDLER( speechboard_t1_r )
{
	return sega_speechboard_drq;
}

static READ8_HANDLER( speechboard_p1_r )
{
	return sega_speechboard_latch;
}

static READ8_HANDLER( speechboard_rom_r )
{
	return memory_region(REGION_CPU2)[0x800 + 0x100*(sega_speechboard_p2 & 0x3f) + offset];
}

static WRITE8_HANDLER( speechboard_p1_w )
{
	if(!(data & 0x80))
		sega_speechboard_t0 = 0;
}

static WRITE8_HANDLER( speechboard_p2_w )
{
	sega_speechboard_p2 = data;
}

static void speechboard_drq_w(int level)
{
	sega_speechboard_drq = level == ASSERT_LINE;
}

WRITE8_HANDLER( sega_sh_speechboard_w )
{
	sega_speechboard_latch = data & 0x7f;
	cpunum_set_input_line(1, 0, data & 0x80 ? CLEAR_LINE : ASSERT_LINE);
	if(!(data & 0x80))
		sega_speechboard_t0 = 1;
}

ADDRESS_MAP_START( sega_speechboard_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END


ADDRESS_MAP_START( sega_speechboard_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

ADDRESS_MAP_START( sega_speechboard_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_READ(speechboard_rom_r)
	AM_RANGE(I8039_p1, I8039_p1) AM_READ(speechboard_p1_r)
	AM_RANGE(I8039_t0, I8039_t0) AM_READ(speechboard_t0_r)
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(speechboard_t1_r)
ADDRESS_MAP_END

ADDRESS_MAP_START( sega_speechboard_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_WRITE(sp0250_w)
	AM_RANGE(I8039_p1, I8039_p1) AM_WRITE(speechboard_p1_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_WRITE(speechboard_p2_w)
ADDRESS_MAP_END

struct sp0250_interface sega_sp0250_interface =
{
	100,
	speechboard_drq_w
};
