#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "sound/samples.h"

static int walk = 0; /* used to determine if dkongjr is walking or climbing? */

WRITE8_HANDLER( dkong_sh_w )
{
	if (data)
		cpunum_set_input_line(1, 0, ASSERT_LINE);
	else
		cpunum_set_input_line(1, 0, CLEAR_LINE);
}

WRITE8_HANDLER( dkong_sh1_w )
{
	static int state[8];
	static int count = 0;
	static const int sample_order[7] = {1,2,1,2,0,1,0};

	if (state[offset] != data)
	{
		if (data)
			{
				if (offset)
					sample_start (offset, offset + 2, 0);
				else
				{
					sample_start (offset, sample_order[count], 0);
					count++;
					if (count == 7)
						count = 0;
				}
			}
		state[offset] = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_death_w )
{
	static int death = 0;

	if (death != data)
	{
		if (data)
			sample_stop (7);
		sample_start (6, 6, 0);
		death = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_drop_w )
{
	static int drop = 0;

	if (drop != data)
	{
		if (data)
			sample_start (7, 7, 0);
		drop = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_roar_w )
{
	static int roar = 0;

	if (roar != data)
	{
		if (data)
			sample_start (7,2,0);
		roar = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_jump_w )
{
	static int jump = 0;

	if (jump != data)
	{
		if (data)
			sample_start (6,0,0);
		jump = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_land_w )
{
	static int land = 0;

	if (land != data)
	{
		if (data)
			sample_stop (7);
		sample_start (4,1,0);
		land = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_climb_w )
{
	static int climb = 0;
	static int count;
	static const int sample_order[7] = {1,2,1,2,0,1,0};

	if (climb != data)
	{
		if (data && walk == 0)
		{
			sample_start (3,sample_order[count]+3,0);
			count++;
			if (count == 7) count = 0;
		}
		else if (data && walk == 1)
		{
			sample_start (3,sample_order[count]+8,0);
			count++;
			if (count == 7) count = 0;
		}
		climb = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_snapjaw_w )
{
	static int snapjaw = 0;

	if (snapjaw != data)
	{
		if (data)
			sample_stop (7);
		sample_start (4,11,0);
		snapjaw = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_walk_w )
{
	if (walk != data )
	{
		walk = data;
	}
}
