#include "cpudefs.h"
void op_3000(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3008(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3010(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3018(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3020(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3028(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3030(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3038(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3039(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_303a(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_303b(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_303c(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
	regs.d[dstreg].W.l = src;
{	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3040(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3048(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3050(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3058(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}}
void op_3060(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}}
void op_3068(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3070(void) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}}
void op_3078(void) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3079(void) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_307a(void) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}}
void op_307b(void) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}}
void op_307c(void) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3080(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3088(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3090(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3098(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30a0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30a8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_30b0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30b8(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_30b9(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_30ba(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30bb(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30bc(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_30c0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30c8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30d0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30d8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}}
void op_30e0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}}
void op_30e8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30f0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}}
void op_30f8(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30f9(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_30fa(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}}
void op_30fb(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}}
void op_30fc(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3100(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3108(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3110(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3118(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}}
void op_3120(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}}
void op_3128(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3130(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}}
void op_3138(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3139(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_313a(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}}
void op_313b(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}}
void op_313c(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3140(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3148(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3150(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3158(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3160(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3168(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3170(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_3178(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3179(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_317a(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_317b(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_317c(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3180(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3188(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3190(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_3198(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_31a0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_31a8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_31b0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_31b8(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_31b9(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_31ba(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_31bb(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_31bc(void) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_31c0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_31c8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_31d0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_31d8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_31e0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_31e8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_31f0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_31f8(void) /* MOVE */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_31f9(void) /* MOVE */
{
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_31fa(void) /* MOVE */
{
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_31fb(void) /* MOVE */
{
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_31fc(void) /* MOVE */
{
{{	WORD src = nextiword();
{	CPTR dsta = (LONG)(WORD)nextiword();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_33c0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_33c8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_33d0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_33d8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_33e0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_33e8(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_33f0(void) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_33f8(void) /* MOVE */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_33f9(void) /* MOVE */
{
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
void op_33fa(void) /* MOVE */
{
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_33fb(void) /* MOVE */
{
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}}
void op_33fc(void) /* MOVE */
{
{{	WORD src = nextiword();
{	CPTR dsta = nextilong();
	put_word(dsta,src);
	CLEARFLGS;
	if ( src == 0 )
		ZFLG = ZTRUE ;
	if ( src < 0 )
		NFLG = NTRUE ;
}}}}
