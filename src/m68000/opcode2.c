#include "cpudefs.h"
void op_2000(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg];
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}
void op_2008(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}
void op_2010(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}
void op_2018(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}}
void op_2020(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}}
void op_2028(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}
void op_2030(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}}
void op_2038(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}
void op_2039(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}
void op_203a(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}}
void op_203b(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}}
void op_203c(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}
void op_2040(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg];
{	regs.a[dstreg] = (src);
}}}}
void op_2048(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	regs.a[dstreg] = (src);
}}}}
void op_2050(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[dstreg] = (src);
}}}}
void op_2058(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	regs.a[dstreg] = (src);
}}}}}
void op_2060(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[dstreg] = (src);
}}}}}
void op_2068(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	regs.a[dstreg] = (src);
}}}}
void op_2070(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	regs.a[dstreg] = (src);
}}}}}
void op_2078(ULONG opcode) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	regs.a[dstreg] = (src);
}}}}
void op_2079(ULONG opcode) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	regs.a[dstreg] = (src);
}}}}
void op_207a(ULONG opcode) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	regs.a[dstreg] = (src);
}}}}}
void op_207b(ULONG opcode) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	regs.a[dstreg] = (src);
}}}}}
void op_207c(ULONG opcode) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	regs.a[dstreg] = (src);
}}}}
void op_2080(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2088(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2090(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2098(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20a0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20a8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_20b0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20b8(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_20b9(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_20ba(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20bb(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20bc(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_20c0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20c8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20d0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20d8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}}
void op_20e0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}}
void op_20e8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20f0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}}
void op_20f8(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20f9(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_20fa(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}}
void op_20fb(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}}
void op_20fc(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 4;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_2100(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg];
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_2108(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_2110(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_2118(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}}
void op_2120(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}}
void op_2128(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_2130(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}}
void op_2138(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_2139(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_213a(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}}
void op_213b(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}}
void op_213c(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_2140(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2148(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2150(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2158(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_2160(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_2168(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2170(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_2178(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2179(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_217a(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_217b(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_217c(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2180(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2188(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2190(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_2198(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_21a0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_21a8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_21b0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_21b8(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_21b9(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_21ba(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_21bb(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_21bc(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_21c0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_21c8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	LONG src = regs.a[srcreg];
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_21d0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_21d8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_21e0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_21e8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_21f0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_21f8(ULONG opcode) /* MOVE */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_21f9(ULONG opcode) /* MOVE */
{
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_21fa(ULONG opcode) /* MOVE */
{
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_21fb(ULONG opcode) /* MOVE */
{
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_21fc(ULONG opcode) /* MOVE */
{
{{	LONG src = nextilong();
{	CPTR dsta = (LONG)(WORD)nextiword();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_23c0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_23c8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	LONG src = regs.a[srcreg];
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_23d0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_23d8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_23e0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_23e8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_23f0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_23f8(ULONG opcode) /* MOVE */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_23f9(ULONG opcode) /* MOVE */
{
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_23fa(ULONG opcode) /* MOVE */
{
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_23fb(ULONG opcode) /* MOVE */
{
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_23fc(ULONG opcode) /* MOVE */
{
{{	LONG src = nextilong();
{	CPTR dsta = nextilong();
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
