/* Sega System 16 etc. Math chip */

#include "driver.h"

static struct {
	UINT16 ops[2];
	UINT16 res[2];
} mathp[3];

static struct {
	UINT16 ops[4];
	UINT16 res[2];
} mathq[3];

static struct {
	UINT16 ops[3];
	UINT16 res;
} mathc[3];

static void mathp_w(int chip, offs_t offset, data16_t data, data16_t mem_mask)
{
	if(offset < 2)
	{
		INT32 res;
		COMBINE_DATA(&mathp[chip].ops[offset]);
		res = ((INT16)mathp[chip].ops[0]) * ((INT16)mathp[chip].ops[1]);
		mathp[chip].res[0] = res >> 16;
		mathp[chip].res[1] = res;
	}
	else
		logerror("mathp[%d] unknown write (%02x, %d:%06x)", chip, offset*2, cpu_getactivecpu(), activecpu_get_pc());
}

static data16_t mathp_r(int chip, offs_t offset, data16_t mem_mask)
{
	switch(offset) {
	case 0x0:
	case 0x1:
		return mathp[chip].ops[offset & 1];
	case 0x2:
	case 0x3:
		return mathp[chip].res[offset & 1];
	}
	logerror("mathp[%d] unknown read (%02x, %d:%06x)", chip, offset*2, cpu_getactivecpu(), activecpu_get_pc());
	return 0;
}

WRITE16_HANDLER( sys16_mathp_0_w )
{
	mathp_w(0, offset, data, mem_mask);
}

READ16_HANDLER( sys16_mathp_0_r )
{
	return mathp_r(0, offset, mem_mask);
}

WRITE16_HANDLER( sys16_mathp_1_w )
{
	mathp_w(1, offset, data, mem_mask);
}

READ16_HANDLER( sys16_mathp_1_r )
{
	return mathp_r(1, offset, mem_mask);
}

WRITE16_HANDLER( sys16_mathp_2_w )
{
	mathp_w(2, offset, data, mem_mask);
}

READ16_HANDLER( sys16_mathp_2_r )
{
	return mathp_r(2, offset, mem_mask);
}

static void mathq_w(int chip, offs_t offset, data16_t data, data16_t mem_mask)
{
	COMBINE_DATA(&mathq[chip].ops[offset & 3]);
	if(offset & 8) {
		INT32 op0, op1;
		op0 = (mathq[chip].ops[0] << 16) | mathq[chip].ops[1];
		if((offset & 3) == 3)
			op1 = (mathq[chip].ops[2] << 16) | mathq[chip].ops[3];
		else
			op1 = (INT16)mathq[chip].ops[2];
		if(offset & 4) {
			INT32 res = op1 ? op0/op1 : 0x7fffffff;
			mathq[chip].res[0] = res >> 16;
			mathq[chip].res[1] = res;
		} else {
			mathq[chip].res[0] = op1 ? op0/op1 : 0x7fff;
			mathq[chip].res[1] = op1 ? op0%op1 : 0x7fff;
		}
	}
}

static data16_t mathq_r(int chip, offs_t offset, data16_t mem_mask)
{
	switch(offset) {
	case 0x0:
	case 0x1:
	case 0x2:
	case 0x3:
		return mathq[chip].ops[offset & 3];
	case 0x4:
	case 0x5:
		return mathq[chip].res[offset & 1];
	}
	logerror("mathq[%d] unknown read (%02x, %d:%06x)", chip, offset*2, cpu_getactivecpu(), activecpu_get_pc());
	return 0;
}

WRITE16_HANDLER( sys16_mathq_0_w )
{
	mathq_w(0, offset, data, mem_mask);
}

READ16_HANDLER( sys16_mathq_0_r )
{
	return mathq_r(0, offset, mem_mask);
}

WRITE16_HANDLER( sys16_mathq_1_w )
{
	mathq_w(1, offset, data, mem_mask);
}

READ16_HANDLER( sys16_mathq_1_r )
{
	return mathq_r(1, offset, mem_mask);
}

WRITE16_HANDLER( sys16_mathq_2_w )
{
	mathq_w(2, offset, data, mem_mask);
}

READ16_HANDLER( sys16_mathq_2_r )
{
	return mathq_r(2, offset, mem_mask);
}

static void mathc_w(int chip, offs_t offset, data16_t data, data16_t mem_mask)
{
	if(offset < 3)
	{
		INT16 F, G, H;
		COMBINE_DATA(&mathc[chip].ops[offset]);
		F = mathc[chip].ops[0];
		G = mathc[chip].ops[1];
		H = mathc[chip].ops[2];
		if(F<=G)
			mathc[chip].res = H<F ? 0xffff : H>G ? 1 : 0;
		else
			mathc[chip].res = H<0 ? 0xffff : H>0 ? 1 : 0;
	}
	else
		logerror("mathc[%d] unknown write (%02x, %d:%06x)", chip, offset*2, cpu_getactivecpu(), activecpu_get_pc());
}

static data16_t mathc_r(int chip, offs_t offset, data16_t mem_mask)
{
	switch(offset) {
	case 0x0:
	case 0x1:
	case 0x2:
		return mathc[chip].ops[offset & 3];
	case 0x3:
		return mathc[chip].res;
	}
	logerror("mathc[%d] unknown read (%02x, %d:%06x)", chip, offset*2, cpu_getactivecpu(), activecpu_get_pc());
	return 0;
}

WRITE16_HANDLER( sys16_mathc_0_w )
{
	mathc_w(0, offset, data, mem_mask);
}

READ16_HANDLER( sys16_mathc_0_r )
{
	return mathc_r(0, offset, mem_mask);
}

WRITE16_HANDLER( sys16_mathc_1_w )
{
	mathc_w(1, offset, data, mem_mask);
}

READ16_HANDLER( sys16_mathc_1_r )
{
	return mathc_r(1, offset, mem_mask);
}

WRITE16_HANDLER( sys16_mathc_2_w )
{
	mathc_w(2, offset, data, mem_mask);
}

READ16_HANDLER( sys16_mathc_2_r )
{
	return mathc_r(2, offset, mem_mask);
}
