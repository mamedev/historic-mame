/* compute operations */

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

INLINE void compute_add(int rn, int rx, int ry)
{
	UINT32 r = sharc.r[rx] + sharc.r[ry];

	if(sharc.mode1 & ALUSAT)
		osd_die("SHARC: compute_add: ALU saturation not implemented !\n");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_ADD(r, sharc.r[rx], sharc.r[ry]);
	SET_FLAG_AC_ADD(r, sharc.r[rx], sharc.r[ry]);
	sharc.r[rn] = r;
}

INLINE void compute_sub(int rn, int rx, int ry)
{
	UINT32 r = sharc.r[rx] - sharc.r[ry];

	if(sharc.mode1 & ALUSAT)
		osd_die("SHARC: compute_sub: ALU saturation not implemented !\n");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_SUB(r, sharc.r[rx], sharc.r[ry]);
	SET_FLAG_AC_SUB(r, sharc.r[rx], sharc.r[ry]);
	sharc.r[rn] = r;
}

INLINE void compute_add_ci(int rn, int rx, int ry)
{
	int c = (sharc.astat & AC) ? 1 : 0;
	UINT32 r = sharc.r[rx] + sharc.r[ry] + c;

	if(sharc.mode1 & ALUSAT)
		osd_die("SHARC: compute_add_ci: ALU saturation not implemented !\n");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_ADD(r, sharc.r[rx], sharc.r[ry]+c);
	SET_FLAG_AC_ADD(r, sharc.r[rx], sharc.r[ry]+c);
	sharc.r[rn] = r;
}

INLINE void compute_sub_ci(int rn, int rx, int ry)
{
	int c = (sharc.astat & AC) ? 1 : 0;
	UINT32 r = sharc.r[rx] - sharc.r[ry] + c - 1;

	if(sharc.mode1 & ALUSAT)
		osd_die("SHARC: compute_sub_ci: ALU saturation not implemented !\n");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_SUB(r, sharc.r[rx], sharc.r[ry]+c-1);
	SET_FLAG_AC_SUB(r, sharc.r[rx], sharc.r[ry]+c-1);
	sharc.r[rn] = r;
}

INLINE void compute_and(int rn, int rx, int ry)
{
	UINT32 r = sharc.r[rx] & sharc.r[ry];

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	sharc.r[rn] = r;
}

INLINE void compute_comp(int rx, int ry)
{
	CLEAR_ALU_FLAGS();
	if( sharc.r[rx] == sharc.r[ry] )
		sharc.astat |= AZ;
	if( (INT32)sharc.r[rx] < (INT32)sharc.r[ry] )
		sharc.astat |= AN;
	/* TODO: Update ASTAT compare accumulation register */
}

INLINE void compute_pass(int rn, int rx)
{
	CLEAR_ALU_FLAGS();
	/* TODO: floating-point extension field is set to 0 */

	sharc.r[rn] = sharc.r[rx];
	if (sharc.r[rn] == 0)
		sharc.astat |= AZ;
	if (sharc.r[rn] & 0x80000000)
		sharc.astat |= AN;
}

INLINE void compute_xor(int rn, int rx, int ry)
{
	UINT32 r = sharc.r[rx] ^ sharc.r[ry];
	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	sharc.r[rn] = r;
}

INLINE void compute_or(int rn, int rx, int ry)
{
	UINT32 r = sharc.r[rx] | sharc.r[ry];
	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	sharc.r[rn] = r;
}

INLINE void compute_inc(int rn, int rx)
{
	UINT32 r = sharc.r[rx] + 1;

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_ADD(r, sharc.r[rx], 1);
	SET_FLAG_AC_ADD(r, sharc.r[rx], 1);

	sharc.r[rn] = r;
}

INLINE void compute_dec(int rn, int rx)
{
	UINT32 r = sharc.r[rx] - 1;

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_SUB(r, sharc.r[rx], 1);
	SET_FLAG_AC_SUB(r, sharc.r[rx], 1);

	sharc.r[rn] = r;
}

INLINE void compute_max(int rn, int rx, int ry)
{
	UINT32 r = ((INT32)sharc.r[rx] > (INT32)sharc.r[ry]) ? sharc.r[rx] : sharc.r[ry];

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);

	sharc.r[rn] = r;
}


/*****************************************************************************/
/* Multiplier opcodes */

/* Rn = (unsigned)Rx * (unsigned)Ry, integer, no rounding */
INLINE void compute_mul_uuir(int rn, int rx, int ry)
{
	UINT64 r = (UINT64)(UINT32)sharc.r[rx] * (UINT64)(UINT32)sharc.r[ry];

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN((UINT32)r);
	SET_FLAG_MV(r);
	SET_FLAG_MU(r);

	sharc.r[rn] = (UINT32)(r);
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
