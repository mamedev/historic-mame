#include "cpudefs.h"
void op_8000(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_8010(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_8018(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}}
void op_8020(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}}
void op_8028(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_8030(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}}
void op_8038(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_8039(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_803a(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}}
void op_803b(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}}
void op_803c(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = nextiword();
{	BYTE dst = regs.d[dstreg].B.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_8040(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_8050(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_8058(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}}
void op_8060(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}}
void op_8068(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_8070(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}}
void op_8078(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_8079(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_807a(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}}
void op_807b(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}}
void op_807c(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg].W.l;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_8080(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_8090(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_8098(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}}
void op_80a0(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}}
void op_80a8(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_80b0(ULONG opcode) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}}
void op_80b8(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_80b9(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_80ba(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}}
void op_80bb(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}}
void op_80bc(ULONG opcode) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	LONG dst = regs.d[dstreg].D;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_80c0(ULONG opcode) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	WORD src = regs.d[srcreg].W.l;
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_80d0(ULONG opcode) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_80d8(ULONG opcode) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}}
void op_80e0(ULONG opcode) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}}
void op_80e8(ULONG opcode) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_80f0(ULONG opcode) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}}
void op_80f8(ULONG opcode) /* DIVU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_80f9(ULONG opcode) /* DIVU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_80fa(ULONG opcode) /* DIVU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}}
void op_80fb(ULONG opcode) /* DIVU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}}
void op_80fc(ULONG opcode) /* DIVU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	WORD src = nextiword();
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	ULONG newv = (ULONG)dst / (ULONG)(UWORD)src;
	ULONG rem = (ULONG)dst % (ULONG)(UWORD)src;
	if (newv > 0xffff) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_8100(ULONG opcode) /* SBCD */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	BYTE dst = regs.d[dstreg].B.l;
{	UWORD newv_lo = (dst & 0xF) - (src & 0xF) - (regs.x ? 1 : 0);
	UWORD newv_hi = (dst & 0xF0) - (src & 0xF0);
	UWORD newv;
	if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
	newv = newv_hi + (newv_lo & 0xF);	CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
	if (CFLG) newv -= 0x60;
	if (((BYTE)(newv)) != 0) ZFLG = 0;
	NFLG = ((BYTE)(newv)) < 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	regs.d[dstreg].B.l = newv;
}}}}}}
void op_8108(ULONG opcode) /* SBCD */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	UWORD newv_lo = (dst & 0xF) - (src & 0xF) - (regs.x ? 1 : 0);
	UWORD newv_hi = (dst & 0xF0) - (src & 0xF0);
	UWORD newv;
	if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
	newv = newv_hi + (newv_lo & 0xF);	CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
	if (CFLG) newv -= 0x60;
	if (((BYTE)(newv)) != 0) ZFLG = 0;
	NFLG = ((BYTE)(newv)) < 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	put_byte(dsta,newv);
}}}}}}}}
void op_8110(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}
void op_8118(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}}
void op_8120(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}}
void op_8128(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}
void op_8130(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}}
void op_8138(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}
void op_8139(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}
void op_8150(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_8158(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{	regs.a[dstreg] += 2;
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_8160(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_8168(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_8170(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	WORD dst = get_word(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_8178(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_8179(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_8190(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_8198(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{	regs.a[dstreg] += 4;
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_81a0(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_81a8(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_81b0(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	LONG dst = get_long(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_81b8(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_81b9(ULONG opcode) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
	src |= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_81c0(ULONG opcode) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	WORD src = regs.d[srcreg].W.l;
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_81d0(ULONG opcode) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_81d8(ULONG opcode) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}}
void op_81e0(ULONG opcode) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}}
void op_81e8(ULONG opcode) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_81f0(ULONG opcode) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}}
void op_81f8(ULONG opcode) /* DIVS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_81f9(ULONG opcode) /* DIVS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
void op_81fa(ULONG opcode) /* DIVS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}}
void op_81fb(ULONG opcode) /* DIVS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}}
void op_81fc(ULONG opcode) /* DIVS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	WORD src = nextiword();
{	LONG dst = regs.d[dstreg].D;
	if(src == 0) Exception(5,oldpc-2); else {
	LONG newv = (LONG)dst / (LONG)(WORD)src;
	UWORD rem = (LONG)dst % (LONG)(WORD)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { VFLG = NFLG = 1; CFLG = 0; } else
	{
	if (((WORD)rem < 0) != ((LONG)dst < 0)) rem = -rem;
	CLEARVC;
	ZFLG = ((WORD)(newv)) == 0;
	NFLG = ((WORD)(newv)) < 0;
	newv = (newv & 0xffff) | ((ULONG)rem << 16);
	regs.d[dstreg].D = (newv);
	}
	}
}}}}
