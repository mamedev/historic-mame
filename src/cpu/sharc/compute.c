/* compute operations */

#include <math.h>

#define CLEAR_ALU_FLAGS()		(sharc.astat &= ~(AZ|AN|AV|AC|AS|AI))

#define SET_FLAG_AZ(r)			{ sharc.astat |= (((r) == 0) ? AZ : 0); }
#define SET_FLAG_AN(r)			{ sharc.astat |= (((r) & 0x80000000) ? AN : 0); }
#define SET_FLAG_AC_ADD(r,a,b)	{ sharc.astat |= (((UINT32)r < (UINT32)a) ? AC : 0); }
#define SET_FLAG_AV_ADD(r,a,b)	{ sharc.astat |= (((~((a) ^ (b)) & ((a) ^ (r))) & 0x80000000) ? AV : 0); }
#define SET_FLAG_AC_SUB(r,a,b)	{ sharc.astat |= ((!((UINT32)a < (UINT32)b)) ? AC : 0); }
#define SET_FLAG_AV_SUB(r,a,b)	{ sharc.astat |= ((( ((a) ^ (b)) & ((a) ^ (r))) & 0x80000000) ? AV : 0); }

#define CLEAR_MULTIPLIER_FLAGS()	(sharc.astat &= ~(MN|MV|MU|MI))

#define SET_FLAG_MN(r)			{ sharc.astat |= (((r) & 0x80000000) ? MN : 0); }
#define SET_FLAG_MV(r)			{ sharc.astat |= ((((UINT32)((r) >> 32) != 0) && ((UINT32)((r) >> 32) != 0xffffffff)) ? MV : 0); }

/* TODO: MU needs 80-bit result */
#define SET_FLAG_MU(r)			{ sharc.astat |= ((((UINT32)((r) >> 32) == 0) && ((UINT32)(r)) != 0) ? MU : 0); }


/*****************************************************************************/
/* Integer ALU operations */

/* Rn = Rx + Ry */
INLINE void compute_add(int rn, int rx, int ry)
{
	UINT32 r = REG(rx) + REG(ry);

	if(sharc.mode1 & ALUSAT)
		osd_die("SHARC: compute_add: ALU saturation not implemented !\n");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_ADD(r, REG(rx), REG(ry));
	SET_FLAG_AC_ADD(r, REG(rx), REG(ry));
	REG(rn) = r;
}

/* Rn = Rx - Ry */
INLINE void compute_sub(int rn, int rx, int ry)
{
	UINT32 r = REG(rx) - REG(ry);

	if(sharc.mode1 & ALUSAT)
		osd_die("SHARC: compute_sub: ALU saturation not implemented !\n");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_SUB(r, REG(rx), REG(ry));
	SET_FLAG_AC_SUB(r, REG(rx), REG(ry));
	REG(rn) = r;
}

/* Rn = Rx + Ry + CI */
INLINE void compute_add_ci(int rn, int rx, int ry)
{
	int c = (sharc.astat & AC) ? 1 : 0;
	UINT32 r = REG(rx) + REG(ry) + c;

	if(sharc.mode1 & ALUSAT)
		osd_die("SHARC: compute_add_ci: ALU saturation not implemented !\n");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_ADD(r, REG(rx), REG(ry)+c);
	SET_FLAG_AC_ADD(r, REG(rx), REG(ry)+c);
	REG(rn) = r;
}

/* Rn = Rx - Ry + CI - 1 */
INLINE void compute_sub_ci(int rn, int rx, int ry)
{
	int c = (sharc.astat & AC) ? 1 : 0;
	UINT32 r = REG(rx) - REG(ry) + c - 1;

	if(sharc.mode1 & ALUSAT)
		osd_die("SHARC: compute_sub_ci: ALU saturation not implemented !\n");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_SUB(r, REG(rx), REG(ry)+c-1);
	SET_FLAG_AC_SUB(r, REG(rx), REG(ry)+c-1);
	REG(rn) = r;
}

/* Rn = Rx AND Ry */
INLINE void compute_and(int rn, int rx, int ry)
{
	UINT32 r = REG(rx) & REG(ry);

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	REG(rn) = r;
}

/* COMP(Rx, Ry) */
INLINE void compute_comp(int rx, int ry)
{
	CLEAR_ALU_FLAGS();
	if( REG(rx) == REG(ry) )
		sharc.astat |= AZ;
	if( (INT32)REG(rx) < (INT32)REG(ry) )
		sharc.astat |= AN;
	/* TODO: Update ASTAT compare accumulation register */
}

/* Rn = PASS Rx */
INLINE void compute_pass(int rn, int rx)
{
	CLEAR_ALU_FLAGS();
	/* TODO: floating-point extension field is set to 0 */

	REG(rn) = REG(rx);
	if (REG(rn) == 0)
		sharc.astat |= AZ;
	if (REG(rn) & 0x80000000)
		sharc.astat |= AN;
}

/* Rn = Rx XOR Ry */
INLINE void compute_xor(int rn, int rx, int ry)
{
	UINT32 r = REG(rx) ^ REG(ry);
	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	REG(rn) = r;
}

/* Rn = Rx OR Ry */
INLINE void compute_or(int rn, int rx, int ry)
{
	UINT32 r = REG(rx) | REG(ry);
	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	REG(rn) = r;
}

/* Rn = Rx + 1 */
INLINE void compute_inc(int rn, int rx)
{
	UINT32 r = REG(rx) + 1;

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_ADD(r, REG(rx), 1);
	SET_FLAG_AC_ADD(r, REG(rx), 1);

	REG(rn) = r;
}

/* Rn = Rx - 1 */
INLINE void compute_dec(int rn, int rx)
{
	UINT32 r = REG(rx) - 1;

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_SUB(r, REG(rx), 1);
	SET_FLAG_AC_SUB(r, REG(rx), 1);

	REG(rn) = r;
}

/* Rn = MAX(Rx, Ry) */
INLINE void compute_max(int rn, int rx, int ry)
{
	UINT32 r = ((INT32)REG(rx) > (INT32)REG(ry)) ? REG(rx) : REG(ry);

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);

	REG(rn) = r;
}

/* Rn = -Rx */
INLINE void compute_neg(int rn, int rx)
{
	UINT32 r = -REG(rx);

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(REG(rn));
	SET_FLAG_AZ(REG(rn));
	/* TODO: AV flag */
	/* TODO: AC flag */

	REG(rn) = r;
}

/*****************************************************************************/
/* Floating-point ALU operations */

/* Fn = FLOAT Rx */
INLINE void compute_float(int rn, int rx)
{
	FREG(rn) = (float)(INT32)REG(rx);

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(REG(rn));
	/* TODO: AZ flag */
	/* TODO: AU flag */
	/* TODO: AV flag */
}

/* Fn = FLOAT Rx BY Ry */
INLINE void compute_float_scaled(int rn, int rx, int ry)
{
	float r = (float)(INT32)REG(rx);

	r *= (float)pow(2.0, REG(ry));

	FREG(rn) = r;

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(REG(rn));
	/* TODO: set AV if overflowed */
	/* TODO: AZ flag */
	/* TODO: AU flag */
}

/* Rn = LOGB Fx */
INLINE void compute_logb(int rn, int rx)
{
	UINT32 r = REG(rx);

	int exponent = (r >> 23) & 0xff;
	exponent -= 127;

	REG(rn) = exponent;

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(REG(rn));
	SET_FLAG_AZ(REG(rn));
	/* TODO: AV flag */
	/* TODO: AI flag */
}

/* Fn = SCALB Fx BY Fy */
INLINE void compute_scalb(int rn, int rx, int ry)
{
	float r = FREG(rx);
	r *= (float)pow(2.0, REG(ry));

	FREG(rn) = r;

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(REG(rn));
	/* TODO: AV flag */
	/* TODO: AZ flag */
	/* TODO: AU flag */
}

/* Fn = Fx + Fy */
INLINE void compute_fadd(int rn, int rx, int ry)
{
	FREG(rn) = FREG(rx) + FREG(ry);

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(REG(rn));
	/* TODO: AZ flag */
	/* TODO: AU flag */
	/* TODO: AV flag */
	/* TODO: AI flag */
}

/* Fn = Fx - Fy */
INLINE void compute_fsub(int rn, int rx, int ry)
{
	FREG(rn) = FREG(rx) - FREG(ry);

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(REG(rn));
	/* TODO: AZ flag */
	/* TODO: AU flag */
	/* TODO: AV flag */
	/* TODO: AI flag */
}

/* Fn = -Fx */
INLINE void compute_fneg(int rn, int rx)
{
	FREG(rn) = -FREG(rx);

	CLEAR_ALU_FLAGS();
	SET_FLAG_AZ(REG(rn));
	SET_FLAG_AN(REG(rn));
	/* TODO: AI flag */
}

/* Fn = ABS(Fx + Fy) */
INLINE void compute_fabs_plus(int rn, int rx, int ry)
{
	FREG(rn) = fabs(FREG(rx) + FREG(ry));

	CLEAR_ALU_FLAGS();
	/* TODO: AZ flag */
	/* TODO: AU flag */
	/* TODO: AV flag */
	/* TODO: AI flag */
}

/* Fn = RECIPS Fx */
INLINE void compute_recips(int rn, int rx)
{
	/* TODO: calculate reciprocal, this is too accurate! */
	FREG(rn) = 1.0 / FREG(rx);

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(REG(rn));
	/* TODO: AZ flag */
	/* TODO: AV flag */
	/* TODO: AI flag */
}

/*****************************************************************************/
/* Multiplier opcodes */

/* Rn = (unsigned)Rx * (unsigned)Ry, integer, no rounding */
INLINE void compute_mul_uuir(int rn, int rx, int ry)
{
	UINT64 r = (UINT64)(UINT32)REG(rx) * (UINT64)(UINT32)REG(ry);

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN((UINT32)r);
	SET_FLAG_MV(r);
	SET_FLAG_MU(r);

	REG(rn) = (UINT32)(r);
}

/* Fn = Fx * Fy */
INLINE void compute_fmul(int rn, int rx, int ry)
{
	FREG(rn) = FREG(rx) * FREG(ry);

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(REG(rn));
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */
}

/*****************************************************************************/

/* multi function opcodes */

INLINE void compute_multi_mr_to_reg(int ai, int rk)
{
	switch(ai)
	{
		case 0:		SET_UREG(rk, (UINT32)(sharc.mrf)); break;
		case 1:		SET_UREG(rk, (UINT32)(sharc.mrf >> 32)); break;
		case 2:		osd_die("SHARC: tried to load MR2F"); break;
		case 4:		SET_UREG(rk, (UINT32)(sharc.mrb)); break;
		case 5:		SET_UREG(rk, (UINT32)(sharc.mrb >> 32)); break;
		case 6:		osd_die("SHARC: tried to load MR2B"); break;
		default:	osd_die("SHARC: unknown ai %d in mr_to_reg\n", ai);
	}
	/* TODO: clear multiplier flags */
}

INLINE void compute_multi_reg_to_mr(int ai, int rk)
{
	switch(ai)
	{
		case 0:		sharc.mrf &= ~0xffffffff; sharc.mrf |= GET_UREG(rk); break;
		case 1:		sharc.mrf &= 0xffffffff; sharc.mrf |= (UINT64)(GET_UREG(rk)) << 32; break;
		case 2:		osd_die("SHARC: tried to write MR2F"); break;
		case 4:		sharc.mrb &= ~0xffffffff; sharc.mrb |= GET_UREG(rk); break;
		case 5:		sharc.mrb &= 0xffffffff; sharc.mrb |= (UINT64)(GET_UREG(rk)) << 32; break;
		case 6:		osd_die("SHARC: tried to write MR2B"); break;
		default:	osd_die("SHARC: unknown ai %d in reg_to_mr\n", ai);
	}
	/* TODO: clear multiplier flags */
}

/* Ra = Rx + Ry,   Rs = Rx - Ry */
INLINE void compute_dual_add_sub(int ra, int rs, int rx, int ry)
{
	UINT32 r_add = REG(rx) + REG(ry);
	UINT32 r_sub = REG(rx) - REG(ry);

	REG(ra) = r_add;
	REG(rs) = r_sub;

	CLEAR_ALU_FLAGS();
	if (r_add == 0 || r_sub == 0)
	{
		sharc.astat |= AZ;
	}
	if (r_add & 0x80000000 || r_sub & 0x80000000)
	{
		sharc.astat |= AN;
	}
	if (((~(REG(rx) ^ REG(ry)) & (REG(rx) ^ r_add)) & 0x80000000) ||
		(( (REG(rx) ^ REG(ry)) & (REG(rx) ^ r_sub)) & 0x80000000))
	{
		sharc.astat |= AV;
	}
	if (((UINT32)r_add < (UINT32)REG(rx)) ||
		(!(UINT32)r_sub < (UINT32)REG(rx)))
	{
		sharc.astat |= AC;
	}
}
