#include "cpudefs.h"
void op_1000(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = regs.d[srcreg].B.l;
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

void op_1010(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
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

void op_1018(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	regs.a[srcreg] += areg_byteinc[srcreg];
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

void op_1020(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
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
}}

void op_1028(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
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

void op_1030(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	BYTE src = get_byte(srca);
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

void op_1038(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
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

void op_1039(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
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

void op_103a(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
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
}}

void op_103b(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	BYTE src = get_byte(srca);
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

void op_103c(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = nextiword();
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

void op_1080(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = regs.d[srcreg].B.l;
	CPTR dsta = regs.a[dstreg];
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

void op_1090(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_1098(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	regs.a[srcreg] += areg_byteinc[srcreg];
{	CPTR dsta = regs.a[dstreg];
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

void op_10a0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_10a8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_10b0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_10b8(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_10b9(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_10ba(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_10bb(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_10bc(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
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

void op_10c0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = regs.d[srcreg].B.l;
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_10d0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_10d8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	regs.a[srcreg] += areg_byteinc[srcreg];
{	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_10e0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_10e8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_10f0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_10f8(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_10f9(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_10fa(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_10fb(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_10fc(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += areg_byteinc[dstreg];
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

void op_1100(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = regs.d[srcreg].B.l;
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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

void op_1110(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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

void op_1118(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	regs.a[srcreg] += areg_byteinc[srcreg];
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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

void op_1120(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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
}}}

void op_1128(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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

void op_1130(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	BYTE src = get_byte(srca);
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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

void op_1138(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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

void op_1139(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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

void op_113a(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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
}}}

void op_113b(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	BYTE src = get_byte(srca);
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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

void op_113c(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = nextiword();
	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
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

void op_1140(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = regs.d[srcreg].B.l;
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_1150(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_1158(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	regs.a[srcreg] += areg_byteinc[srcreg];
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_1160(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_1168(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_1170(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_1178(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_1179(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_117a(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_117b(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	BYTE src = get_byte(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_117c(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = nextiword();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_1180(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = regs.d[srcreg].B.l;
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_1190(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_1198(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	regs.a[srcreg] += areg_byteinc[srcreg];
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_11a0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_11a8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_11b0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	BYTE src = get_byte(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_11b8(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_11b9(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_11ba(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_11bb(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	BYTE src = get_byte(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_11bc(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = nextiword();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_11c0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	BYTE src = regs.d[srcreg].B.l;
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_11d0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_11d8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	regs.a[srcreg] += areg_byteinc[srcreg];
{	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_11e0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_11e8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_11f0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	BYTE src = get_byte(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_11f8(void) /* MOVE */
{
	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_11f9(void) /* MOVE */
{
	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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
	put_byte(dsta,src);
}

void op_11fa(void) /* MOVE */
{
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_11fb(void) /* MOVE */
{
	CPTR srca = get_disp_ea(m68k_getpc());
	BYTE src = get_byte(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_11fc(void) /* MOVE */
{
	BYTE src = nextiword();
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_13c0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	BYTE src = regs.d[srcreg].B.l;
	CPTR dsta = nextilong();
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

void op_13d0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = nextilong();
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

void op_13d8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	regs.a[srcreg] += areg_byteinc[srcreg];
{	CPTR dsta = nextilong();
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

void op_13e0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
	CPTR dsta = nextilong();
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

void op_13e8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = nextilong();
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

void op_13f0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	BYTE src = get_byte(srca);
	CPTR dsta = nextilong();
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

void op_13f8(void) /* MOVE */
{
	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
	CPTR dsta = nextilong();
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

void op_13f9(void) /* MOVE */
{
	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
	CPTR dsta = nextilong();
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

void op_13fa(void) /* MOVE */
{
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
	CPTR dsta = nextilong();
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

void op_13fb(void) /* MOVE */
{
	CPTR srca = get_disp_ea(m68k_getpc());
	BYTE src = get_byte(srca);
	CPTR dsta = nextilong();
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

void op_13fc(void) /* MOVE */
{
	BYTE src = nextiword();
	CPTR dsta = nextilong();
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
