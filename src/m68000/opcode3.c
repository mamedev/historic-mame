#include "cpudefs.h"
void op_3000(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg];
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}
void op_3008(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}
void op_3010(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}
void op_3018(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}}
void op_3020(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}}
void op_3028(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}
void op_3030(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}}
void op_3038(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}
void op_3039(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}
void op_303a(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}}
void op_303b(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}}
void op_303c(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((src) & 0xffff);
}}}}
void op_3040(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg];
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3048(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3050(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3058(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}}
void op_3060(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}}
void op_3068(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3070(ULONG opcode) /* MOVEA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}}
void op_3078(ULONG opcode) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3079(ULONG opcode) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_307a(ULONG opcode) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}}
void op_307b(ULONG opcode) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}}
void op_307c(ULONG opcode) /* MOVEA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	regs.a[dstreg] = (LONG)(WORD)(src);
}}}}
void op_3080(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3088(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3090(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3098(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30a0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30a8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_30b0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30b8(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_30b9(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_30ba(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30bb(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30bc(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_30c0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30c8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30d0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30d8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}}
void op_30e0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}}
void op_30e8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30f0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}}
void op_30f8(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30f9(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_30fa(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}}
void op_30fb(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}}
void op_30fc(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg];
{	regs.a[dstreg] += 2;
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_3100(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg];
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_3108(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_3110(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_3118(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}}
void op_3120(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}}
void op_3128(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_3130(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}}
void op_3138(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_3139(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_313a(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}}
void op_313b(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}}
void op_313c(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_3140(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3148(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3150(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3158(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_3160(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_3168(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3170(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_3178(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3179(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_317a(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_317b(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_317c(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3180(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3188(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3190(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_3198(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_31a0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_31a8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_31b0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_31b8(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_31b9(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_31ba(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_31bb(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_31bc(ULONG opcode) /* MOVE */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_31c0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_31c8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_31d0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_31d8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_31e0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_31e8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_31f0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_31f8(ULONG opcode) /* MOVE */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_31f9(ULONG opcode) /* MOVE */
{
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_31fa(ULONG opcode) /* MOVE */
{
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_31fb(ULONG opcode) /* MOVE */
{
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_31fc(ULONG opcode) /* MOVE */
{
{{	WORD src = nextiword();
{	CPTR dsta = (LONG)(WORD)nextiword();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_33c0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_33c8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	WORD src = regs.a[srcreg];
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_33d0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_33d8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_33e0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_33e8(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_33f0(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_33f8(ULONG opcode) /* MOVE */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_33f9(ULONG opcode) /* MOVE */
{
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_33fa(ULONG opcode) /* MOVE */
{
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_33fb(ULONG opcode) /* MOVE */
{
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_33fc(ULONG opcode) /* MOVE */
{
{{	WORD src = nextiword();
{	CPTR dsta = nextilong();
	VFLG = CFLG = 0;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
