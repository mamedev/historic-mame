#include "cpudefs.h"
void op_2000(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.d[srcreg].D;
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

void op_2008(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.a[srcreg];
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

void op_2010(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
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

void op_2018(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[srcreg] += 4;
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

void op_2020(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
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
}}

void op_2028(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
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

void op_2030(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	LONG src = get_long(srca);
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

void op_2038(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
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

void op_2039(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	LONG src = get_long(srca);
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

void op_203a(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
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
}}

void op_203b(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	LONG src = get_long(srca);
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

void op_203c(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = nextilong();
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

void op_2040(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.d[srcreg].D;
	regs.a[dstreg] = (src);
}

void op_2048(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.a[srcreg];
	regs.a[dstreg] = (src);
}

void op_2050(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[dstreg] = (src);
}

void op_2058(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[srcreg] += 4;
	regs.a[dstreg] = (src);
}

void op_2060(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[dstreg] = (src);
}}

void op_2068(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	regs.a[dstreg] = (src);
}

void op_2070(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	LONG src = get_long(srca);
	regs.a[dstreg] = (src);
}

void op_2078(void) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	regs.a[dstreg] = (src);
}

void op_2079(void) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	LONG src = get_long(srca);
	regs.a[dstreg] = (src);
}

void op_207a(void) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
	regs.a[dstreg] = (src);
}}

void op_207b(void) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	LONG src = get_long(srca);
	regs.a[dstreg] = (src);
}

void op_207c(void) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = nextilong();
	regs.a[dstreg] = (src);
}

void op_2080(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.d[srcreg].D;
	CPTR dsta = regs.a[dstreg];
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

void op_2088(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.a[srcreg];
	CPTR dsta = regs.a[dstreg];
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

void op_2090(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_2098(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[srcreg] += 4;
{	CPTR dsta = regs.a[dstreg];
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
}}
void op_20a0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
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
}}

void op_20a8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_20b0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_20b8(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_20b9(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_20ba(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
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
}}

void op_20bb(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
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

void op_20bc(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg];
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

void op_20c0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.d[srcreg].D;
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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

void op_20c8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.a[srcreg];
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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

void op_20d0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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

void op_20d8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[srcreg] += 4;
{	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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
}}

void op_20e0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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
}}

void op_20e8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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

void op_20f0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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

void op_20f8(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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

void op_20f9(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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

void op_20fa(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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
}}

void op_20fb(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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

void op_20fc(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg];
	regs.a[dstreg] += 4;
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

void op_2100(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.d[srcreg].D;
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}

void op_2108(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.a[srcreg];
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}

void op_2110(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}

void op_2118(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[srcreg] += 4;
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}

void op_2120(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}}

void op_2128(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}

void op_2130(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	LONG src = get_long(srca);
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}

void op_2138(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}

void op_2139(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	LONG src = get_long(srca);
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}

void op_213a(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}}

void op_213b(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	LONG src = get_long(srca);
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}

void op_213c(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = nextilong();
	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
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
}}

void op_2140(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.d[srcreg].D;
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_2148(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.a[srcreg];
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_2150(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_2158(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[srcreg] += 4;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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
}}

void op_2160(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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
}}

void op_2168(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_2170(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_2178(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_2179(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_217a(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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
}}

void op_217b(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	LONG src = get_long(srca);
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_217c(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = nextilong();
	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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

void op_2180(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.d[srcreg].D;
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_2188(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = regs.a[srcreg];
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_2190(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_2198(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[srcreg] += 4;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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
}}

void op_21a0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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
}}

void op_21a8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_21b0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	LONG src = get_long(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_21b8(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_21b9(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = nextilong();
	LONG src = get_long(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_21ba(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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
}}

void op_21bb(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = get_disp_ea(m68k_getpc());
	LONG src = get_long(srca);
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_21bc(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
	LONG src = nextilong();
	CPTR dsta = get_disp_ea(regs.a[dstreg]);
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

void op_21c0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	LONG src = regs.d[srcreg].D;
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_21c8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	LONG src = regs.a[srcreg];
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_21d0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_21d8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[srcreg] += 4;
{	CPTR dsta = (LONG)(WORD)nextiword();
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
}}

void op_21e0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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
}}

void op_21e8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_21f0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	LONG src = get_long(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_21f8(void) /* MOVE */
{
	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_21f9(void) /* MOVE */
{
	CPTR srca = nextilong();
	LONG src = get_long(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_21fa(void) /* MOVE */
{
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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
}}

void op_21fb(void) /* MOVE */
{
	CPTR srca = get_disp_ea(m68k_getpc());
	LONG src = get_long(srca);
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_21fc(void) /* MOVE */
{
	LONG src = nextilong();
	CPTR dsta = (LONG)(WORD)nextiword();
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

void op_23c0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	LONG src = regs.d[srcreg].D;
	CPTR dsta = nextilong();
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

void op_23c8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	LONG src = regs.a[srcreg];
	CPTR dsta = nextilong();
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

void op_23d0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = nextilong();
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

void op_23d8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	regs.a[srcreg] += 4;
{	CPTR dsta = nextilong();
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
}}

void op_23e0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
	CPTR dsta = nextilong();
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
}}

void op_23e8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = nextilong();
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

void op_23f0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = get_disp_ea(regs.a[srcreg]);
	LONG src = get_long(srca);
	CPTR dsta = nextilong();
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

void op_23f8(void) /* MOVE */
{
	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
	CPTR dsta = nextilong();
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

void op_23f9(void) /* MOVE */
{
	CPTR srca = nextilong();
	LONG src = get_long(srca);
	CPTR dsta = nextilong();
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

void op_23fa(void) /* MOVE */
{
	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
	CPTR dsta = nextilong();
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
}}

void op_23fb(void) /* MOVE */
{
	CPTR srca = get_disp_ea(m68k_getpc());
	LONG src = get_long(srca);
	CPTR dsta = nextilong();
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

void op_23fc(void) /* MOVE */
{
	LONG src = nextilong();
	CPTR dsta = nextilong();
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
