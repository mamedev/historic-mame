#include "cpudefs.h"
void op_0(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	regs.d[dstreg].B.l = src;
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_10(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src |= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_18(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	regs.a[dstreg] += areg_byteinc[dstreg];
	src |= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_20(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src |= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}

void op_28(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src |= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_30(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	BYTE dst = get_byte(dsta);
	src |= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_38(void) /* OR */
{
	BYTE src = nextiword();
	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src |= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_39(void) /* OR */
{
	BYTE src = nextiword();
	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src |= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_3c(void) /* ORSR */
{
{	MakeSR();
{	WORD src = nextiword();
	src &= 0xFF;
	regs.sr |= src;
	MakeFromSR();
}}}

void op_40(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	regs.d[dstreg].W.l = src;
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_50(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src |= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_58(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src |= dst;
	put_word(dsta,src);
	regs.a[dstreg] += 2;
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_60(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src |= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}

void op_68(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	src |= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_70(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	WORD dst = get_word(dsta);
	src |= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_78(void) /* OR */
{
	WORD src = nextiword();
	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	src |= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_79(void) /* OR */
{
	WORD src = nextiword();
	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
	src |= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_7c(void) /* ORSR */
{
	if (!regs.s)
		{
		regs.pc -= 2;
		Exception(8,0);
		}
	else
		{
		MakeSR();
			{	WORD src = nextiword();
		regs.sr |= src;
		MakeFromSR();
			}
		}
}

void op_80(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	LONG dst = regs.d[dstreg].D;
	src |= dst;
	regs.d[dstreg].D = (src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_90(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	src |= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_98(void) /* OR */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	regs.a[dstreg] += 4;
	src |= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_a0(void) /* OR */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	src |= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}

void op_a8(void) /* OR */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src |= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_b0(void) /* OR */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	LONG dst = get_long(dsta);
	src |= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_b8(void) /* OR */
{
{{	LONG src = nextilong();
{	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src |= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_b9(void) /* OR */
{
{{	LONG src = nextilong();
{	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
	src |= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}

void op_100(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
	LONG src = regs.d[srcreg].D;
	LONG dst = regs.d[dstreg].D;
	src &= 31;
	ZFLG = !(dst & (1 << src));
}

void op_108(void) /* MVPMR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR memp = regs.a[srcreg] + nextiword();
	UWORD val = (get_byte(memp) << 8) + get_byte(memp + 2);
	regs.d[dstreg].W.l = val;
}

void op_110(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
	BYTE src = regs.d[srcreg].B.l;
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}

void op_118(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}}
void op_120(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}}
void op_128(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}
void op_130(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}}
void op_138(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}
void op_139(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}
void op_13a(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg;
	dstreg = 2;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}}
void op_13b(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg;
	dstreg = 3;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(m68k_getpc());
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}}
void op_13c(void) /* BTST */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	BYTE dst = nextiword();
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}
void op_140(void) /* BCHG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.d[dstreg].D;
	src &= 31;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	regs.d[dstreg].D = (dst);
}}}}
void op_148(void) /* MVPMR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR memp = regs.a[srcreg] + nextiword();
{	ULONG val = (get_byte(memp) << 24) + (get_byte(memp + 2) << 16)
							+ (get_byte(memp + 4) << 8) + get_byte(memp + 6);
	regs.d[dstreg].D = (val);
}}}
void op_150(void) /* BCHG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_158(void) /* BCHG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_160(void) /* BCHG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_168(void) /* BCHG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_170(void) /* BCHG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_178(void) /* BCHG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_179(void) /* BCHG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_17a(void) /* BCHG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg;
	dstreg = 2;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_17b(void) /* BCHG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg;
	dstreg = 3;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(m68k_getpc());
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_180(void) /* BCLR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.d[dstreg].D;
	src &= 31;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	regs.d[dstreg].D = (dst);
}}}}
void op_188(void) /* MVPRM */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
	CPTR memp = regs.a[dstreg] + nextiword();
	put_byte(memp, src >> 8); put_byte(memp + 2, src);
}}}
void op_190(void) /* BCLR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}
void op_198(void) /* BCLR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}}
void op_1a0(void) /* BCLR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}}
void op_1a8(void) /* BCLR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}
void op_1b0(void) /* BCLR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}}
void op_1b8(void) /* BCLR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}
void op_1b9(void) /* BCLR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}
void op_1ba(void) /* BCLR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg;
	dstreg = 2;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}}
void op_1bb(void) /* BCLR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg;
	dstreg = 3;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(m68k_getpc());
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}}
void op_1c0(void) /* BSET */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.d[dstreg].D;
	src &= 31;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	regs.d[dstreg].D = (dst);
}}}}
void op_1c8(void) /* MVPRM */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
	CPTR memp = regs.a[dstreg] + nextiword();
	put_byte(memp, src >> 24); put_byte(memp + 2, src >> 16);
	put_byte(memp + 4, src >> 8); put_byte(memp + 6, src);
}}}
void op_1d0(void) /* BSET */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_1d8(void) /* BSET */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_1e0(void) /* BSET */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_1e8(void) /* BSET */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_1f0(void) /* BSET */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_1f8(void) /* BSET */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_1f9(void) /* BSET */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_1fa(void) /* BSET */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg;
	dstreg = 2;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_1fb(void) /* BSET */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg;
	dstreg = 3;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(m68k_getpc());
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_200(void) /* AND */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	regs.d[dstreg].B.l = src;
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_210(void) /* AND */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_218(void) /* AND */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	regs.a[dstreg] += areg_byteinc[dstreg];
	src &= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_220(void) /* AND */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}

void op_228(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	BYTE src = nextiword();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_230(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	BYTE src = nextiword();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src &= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_238(void) /* AND */
{
{{	BYTE src = nextiword();
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_239(void) /* AND */
{
{{	BYTE src = nextiword();
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src &= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_23c(void) /* ANDSR */
{
{	MakeSR();
{	WORD src = nextiword();
	src |= 0xFF00;
	regs.sr &= src;
	MakeFromSR();
}}}
void op_240(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	regs.d[dstreg].W.l = src;
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_250(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src &= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_258(void) /* AND */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src &= dst;
	put_word(dsta,src);
	regs.a[dstreg] += 2;
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_260(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src &= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_268(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	src &= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_270(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	WORD dst = get_word(dsta);
	src &= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_278(void) /* AND */
{
{{	WORD src = nextiword();
{	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	src &= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_279(void) /* AND */
{
{{	WORD src = nextiword();
{	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
	src &= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_27c(void) /* ANDSR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{	MakeSR();
{	WORD src = nextiword();
	regs.sr &= src;
	MakeFromSR();
}}}}
void op_280(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	regs.d[dstreg].D = (src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_290(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	src &= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_298(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{	regs.a[dstreg] += 4;
	src &= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_2a0(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	src &= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_2a8(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src &= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_2b0(void) /* AND */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	LONG dst = get_long(dsta);
	src &= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_2b8(void) /* AND */
{
{{	LONG src = nextilong();
{	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src &= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_2b9(void) /* AND */
{
{{	LONG src = nextilong();
{	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
	src &= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_400(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	BYTE dst = regs.d[dstreg].B.l;
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_410(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_418(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	regs.a[dstreg] += areg_byteinc[dstreg];
{	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
}}}

void op_420(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
}}}

void op_428(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_430(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_438(void) /* SUB */
{
	BYTE src = nextiword();
	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_439(void) /* SUB */
{
	BYTE src = nextiword();
	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_440(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	WORD dst = regs.d[dstreg].W.l;
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_450(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_458(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	regs.a[dstreg] += 2;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}

void op_460(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
}}}

void op_468(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_470(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_478(void) /* SUB */
{
	WORD src = nextiword();
	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_479(void) /* SUB */
{
	WORD src = nextiword();
	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_480(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	LONG dst = regs.d[dstreg].D;
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_490(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_498(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
	regs.a[dstreg] += 4;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_4a0(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
}}}

void op_4a8(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_4b0(void) /* SUB */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_4b8(void) /* SUB */
{
	LONG src = nextilong();
	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_4b9(void) /* SUB */
{
	LONG src = nextilong();
	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_600(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	BYTE dst = regs.d[dstreg].B.l;
	ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
}}

void op_610(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
}}

void op_618(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	regs.a[dstreg] += areg_byteinc[dstreg];
{	ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
}}}

void op_620(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
}}}

void op_628(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
}}

void op_630(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
}}

void op_638(void) /* ADD */
{
	BYTE src = nextiword();
	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
}}

void op_639(void) /* ADD */
{
	BYTE src = nextiword();
	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
}}

void op_640(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	WORD dst = regs.d[dstreg].W.l;
	ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	regs.d[dstreg].W.l = newv;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
}}

void op_650(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
}}

void op_658(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
	regs.a[dstreg] += 2;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
}}

void op_660(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
}}}

void op_668(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
}}

void op_670(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
}}

void op_678(void) /* ADD */
{
	WORD src = nextiword();
	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
}}

void op_679(void) /* ADD */
{
	WORD src = nextiword();
	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
}}

void op_680(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	LONG dst = regs.d[dstreg].D;
	ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	regs.d[dstreg].D = (newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
}}

void op_690(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
}}

void op_698(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
	regs.a[dstreg] += 4;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
}}

void op_6a0(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
}}}

void op_6a8(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
}}

void op_6b0(void) /* ADD */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
}}

void op_6b8(void) /* ADD */
{
	LONG src = nextilong();
	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
}}

void op_6b9(void) /* ADD */
{
	LONG src = nextilong();
	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
}}

void op_800(void) /* BTST */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	LONG dst = regs.d[dstreg].D;
	src &= 31;
	ZFLG = !(dst & (1 << src));
}

void op_810(void) /* BTST */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}

void op_818(void) /* BTST */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	regs.a[dstreg] += areg_byteinc[dstreg];
	src &= 7;
	ZFLG = !(dst & (1 << src));
}

void op_820(void) /* BTST */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}

void op_828(void) /* BTST */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}
void op_830(void) /* BTST */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}}
void op_838(void) /* BTST */
{
{{	WORD src = nextiword();
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}
void op_839(void) /* BTST */
{
{{	WORD src = nextiword();
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}
void op_83a(void) /* BTST */
{
	ULONG dstreg;
	dstreg = 2;
{{	WORD src = nextiword();
{	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}}
void op_83b(void) /* BTST */
{
	ULONG dstreg;
	dstreg = 3;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(m68k_getpc());
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}}
void op_83c(void) /* BTST */
{
{{	WORD src = nextiword();
{	BYTE dst = nextiword();
	src &= 7;
	ZFLG = !(dst & (1 << src));
}}}}
void op_840(void) /* BCHG */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	LONG dst = regs.d[dstreg].D;
	src &= 31;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	regs.d[dstreg].D = (dst);
}}}}
void op_850(void) /* BCHG */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_858(void) /* BCHG */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_860(void) /* BCHG */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_868(void) /* BCHG */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_870(void) /* BCHG */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_878(void) /* BCHG */
{
{{	WORD src = nextiword();
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_879(void) /* BCHG */
{
{{	WORD src = nextiword();
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_87a(void) /* BCHG */
{
	ULONG dstreg;
	dstreg = 2;
{{	WORD src = nextiword();
{	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_87b(void) /* BCHG */
{
	ULONG dstreg;
	dstreg = 3;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(m68k_getpc());
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst ^= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_880(void) /* BCLR */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	LONG dst = regs.d[dstreg].D;
	src &= 31;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	regs.d[dstreg].D = (dst);
}}}}
void op_890(void) /* BCLR */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}
void op_898(void) /* BCLR */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}}
void op_8a0(void) /* BCLR */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}}
void op_8a8(void) /* BCLR */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}
void op_8b0(void) /* BCLR */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}}
void op_8b8(void) /* BCLR */
{
{{	WORD src = nextiword();
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}
void op_8b9(void) /* BCLR */
{
{{	WORD src = nextiword();
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}
void op_8ba(void) /* BCLR */
{
	ULONG dstreg;
	dstreg = 2;
{{	WORD src = nextiword();
{	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}}
void op_8bb(void) /* BCLR */
{
	ULONG dstreg;
	dstreg = 3;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(m68k_getpc());
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst &= ~(1 << src);
	put_byte(dsta,dst);
}}}}}
void op_8c0(void) /* BSET */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	LONG dst = regs.d[dstreg].D;
	src &= 31;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	regs.d[dstreg].D = (dst);
}}}}
void op_8d0(void) /* BSET */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_8d8(void) /* BSET */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_8e0(void) /* BSET */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_8e8(void) /* BSET */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_8f0(void) /* BSET */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_8f8(void) /* BSET */
{
{{	WORD src = nextiword();
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_8f9(void) /* BSET */
{
{{	WORD src = nextiword();
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}
void op_8fa(void) /* BSET */
{
	ULONG dstreg;
	dstreg = 2;
{{	WORD src = nextiword();
{	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_8fb(void) /* BSET */
{
	ULONG dstreg;
	dstreg = 3;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(m68k_getpc());
{	BYTE dst = get_byte(dsta);
	src &= 7;
	ZFLG = !(dst & (1 << src));
	dst |= (1 << src);
	put_byte(dsta,dst);
}}}}}
void op_a00(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	BYTE src = nextiword();
{	BYTE dst = regs.d[dstreg].B.l;
	src ^= dst;
	regs.d[dstreg].B.l = src;
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a10(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	BYTE src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src ^= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a18(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	BYTE src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
	src ^= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_a20(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	BYTE src = nextiword();
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src ^= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_a28(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	BYTE src = nextiword();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src ^= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a30(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	BYTE src = nextiword();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src ^= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_a38(void) /* EOR */
{
{{	BYTE src = nextiword();
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src ^= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a39(void) /* EOR */
{
{{	BYTE src = nextiword();
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src ^= dst;
	put_byte(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a3c(void) /* EORSR */
{
{	MakeSR();
{	WORD src = nextiword();
	src &= 0xFF;
	regs.sr ^= src;
	MakeFromSR();
}}}
void op_a40(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg].W.l;
	src ^= dst;
	regs.d[dstreg].W.l = src;
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a50(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src ^= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_a58(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src ^= dst;
	put_word(dsta,src);
	regs.a[dstreg] += 2;
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_a60(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src ^= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_a68(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	src ^= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a70(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	WORD dst = get_word(dsta);
	src ^= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_a78(void) /* EOR */
{
{{	WORD src = nextiword();
{	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	src ^= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a79(void) /* EOR */
{
{{	WORD src = nextiword();
{	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
	src ^= dst;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a7c(void) /* EORSR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{	MakeSR();
{	WORD src = nextiword();
	regs.sr ^= src;
	MakeFromSR();
}}}}
void op_a80(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	LONG dst = regs.d[dstreg].D;
	src ^= dst;
	regs.d[dstreg].D = (src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a90(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	src ^= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_a98(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{	regs.a[dstreg] += 4;
	src ^= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_aa0(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	src ^= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_aa8(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src ^= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_ab0(void) /* EOR */
{
	ULONG dstreg = opcode & 7;
{{	LONG src = nextilong();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	LONG dst = get_long(dsta);
	src ^= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_ab8(void) /* EOR */
{
{{	LONG src = nextilong();
{	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src ^= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}
void op_ab9(void) /* EOR */
{
	LONG src = nextilong();
	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
	src ^= dst;
	put_long(dsta,src);
	CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}

void op_c00(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	BYTE dst = regs.d[dstreg].B.l;
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_c10(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_c18(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	regs.a[dstreg] += areg_byteinc[dstreg];
{	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UBYTE)(src)) > ((UBYTE)(dst));
}}}

void op_c20(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UBYTE)(src)) > ((UBYTE)(dst));
}}}

void op_c28(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_c30(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	BYTE src = nextiword();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_c38(void) /* CMP */
{
	BYTE src = nextiword();
	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_c39(void) /* CMP */
{
	BYTE src = nextiword();
	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_c3a(void) /* CMP */
{
	BYTE src = nextiword();
	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UBYTE)(src)) > ((UBYTE)(dst));
}}}

void op_c3b(void) /* CMP */
{
	BYTE src = nextiword();
	CPTR dsta = get_disp_ea(m68k_getpc());
	BYTE dst = get_byte(dsta);
	ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UBYTE)(src)) > ((UBYTE)(dst));
}}

void op_c40(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	WORD dst = regs.d[dstreg].W.l;
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_c50(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_c58(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	regs.a[dstreg] += 2;
{	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UWORD)(src)) > ((UWORD)(dst));
}}}

void op_c60(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UWORD)(src)) > ((UWORD)(dst));
}}}

void op_c68(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_c70(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	WORD src = nextiword();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_c78(void) /* CMP */
{
	WORD src = nextiword();
	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_c79(void) /* CMP */
{
	WORD src = nextiword();
	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_c7a(void) /* CMP */
{
	WORD src = nextiword();
	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UWORD)(src)) > ((UWORD)(dst));
}}}

void op_c7b(void) /* CMP */
{
	WORD src = nextiword();
	CPTR dsta = get_disp_ea(m68k_getpc());
	WORD dst = get_word(dsta);
	ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((UWORD)(src)) > ((UWORD)(dst));
}}

void op_c80(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	LONG dst = regs.d[dstreg].D;
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_c90(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_c98(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	regs.a[dstreg] += 4;
{	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((ULONG)(src)) > ((ULONG)(dst));
}}}

void op_ca0(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((ULONG)(src)) > ((ULONG)(dst));
}}}

void op_ca8(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_cb0(void) /* CMP */
{
	ULONG dstreg = opcode & 7;
	LONG src = nextilong();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_cb8(void) /* CMP */
{
	LONG src = nextilong();
	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_cb9(void) /* CMP */
{
	LONG src = nextilong();
	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((ULONG)(src)) > ((ULONG)(dst));
}}

void op_cba(void) /* CMP */
{
	LONG src = nextilong();
	CPTR dsta = m68k_getpc();
	dsta += (LONG)(WORD)nextiword();
{	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((ULONG)(src)) > ((ULONG)(dst));
}}}

void op_cbb(void) /* CMP */
{
	LONG src = nextilong();
	CPTR dsta = get_disp_ea(m68k_getpc());
	LONG dst = get_long(dsta);
	ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	NFLG = flgn != 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = ((ULONG)(src)) > ((ULONG)(dst));
}}
