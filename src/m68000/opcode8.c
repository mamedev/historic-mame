#include "cpudefs.h"

/* BFV 061298 - DIVU/DIVS opcodes are modified to decrement the icount */

void op_8000(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	BYTE src = regs.d[srcreg].B.l;
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

void op_8010(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_8018(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}}
void op_8020(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}}
void op_8028(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_8030(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}}
void op_8038(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_8039(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_803a(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}}
void op_803b(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}}
void op_803c(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = nextiword();
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_8040(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;

{{	WORD src = regs.d[srcreg].W.l;
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}
void op_8050(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}
void op_8058(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}}
void op_8060(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}}
void op_8068(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}
void op_8070(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}}
void op_8078(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}
void op_8079(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}
void op_807a(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}}
void op_807b(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}}
void op_807c(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg].W.l;
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
}}}}
void op_8080(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.d[dstreg].D;
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
}}}}
void op_8090(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
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
}}}}
void op_8098(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	LONG dst = regs.d[dstreg].D;
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
}}}}}
void op_80a0(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
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
}}}}}
void op_80a8(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
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
}}}}
void op_80b0(void) /* OR */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
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
}}}}}
void op_80b8(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
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
}}}}
void op_80b9(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
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
}}}}
void op_80ba(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
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
}}}}}
void op_80bb(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
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
}}}}}
void op_80bc(void) /* OR */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	LONG dst = regs.d[dstreg].D;
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
}}}}
void op_80c0(void) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	WORD src = regs.d[srcreg].W.l;
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 38;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 133;
	}
}}}}
void op_80d0(void) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 42;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 137;
	}
}}}}
void op_80d8(void) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 42;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 137;
	}
}}}}}
void op_80e0(void) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 44;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 139;
	}
}}}}}
void op_80e8(void) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 46;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 141;
	}
}}}}
void op_80f0(void) /* DIVU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 48;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 143;
	}
}}}}}
void op_80f8(void) /* DIVU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 46;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 141;
	}
}}}}
void op_80f9(void) /* DIVU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 50;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 145;
	}
}}}}
void op_80fa(void) /* DIVU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 46;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 141;
	}
}}}}}
void op_80fb(void) /* DIVU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 48;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 143;
	}
}}}}}
void op_80fc(void) /* DIVU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	WORD src = nextiword();
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 42;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 137;
	}
}}}}
void op_8100(void) /* SBCD */
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
void op_8108(void) /* SBCD */
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
void op_8110(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
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
}}}}
void op_8118(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
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
}}}}}
void op_8120(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
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
}}}}}
void op_8128(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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
}}}}
void op_8130(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
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
}}}}}
void op_8138(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
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
}}}}
void op_8139(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = nextilong();
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
}}}}
void op_8150(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
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
}}}}
void op_8158(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{	regs.a[dstreg] += 2;
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
}}}}}
void op_8160(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	regs.a[dstreg] -= 2;
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
}}}}}
void op_8168(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
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
}}}}
void op_8170(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	WORD dst = get_word(dsta);
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
}}}}}
void op_8178(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
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
}}}}
void op_8179(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = nextilong();
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
}}}}
void op_8190(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
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
}}}}
void op_8198(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{	regs.a[dstreg] += 4;
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
void op_81a0(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
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
void op_81a8(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
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
void op_81b0(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	LONG dst = get_long(dsta);
	src |= dst;
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
	put_long(dsta,src);
}}}}}
void op_81b8(void) /* OR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src |= dst;
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
	put_long(dsta,src);
}}}}
void op_81b9(void) /* OR */
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
void op_81c0(void) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	WORD src = regs.d[srcreg].W.l;
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 38;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 150;
	}
}}}}
void op_81d0(void) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 42;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 154;
	}
}}}}
void op_81d8(void) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 42;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 154;
	}
}}}}}
void op_81e0(void) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 44;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 156;
	}
}}}}}
void op_81e8(void) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 46;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 158;
	}
}}}}
void op_81f0(void) /* DIVS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 48;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 160;
	}
}}}}}
void op_81f8(void) /* DIVS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 46;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 158;
	}
}}}}
void op_81f9(void) /* DIVS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 50;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 162;
	}
}}}}
void op_81fa(void) /* DIVS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 46;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 158;
	}
}}}}}
void op_81fb(void) /* DIVS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 48;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 160;
	}
}}}}}
void op_81fc(void) /* DIVS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{	CPTR oldpc = m68k_getpc();
{	WORD src = nextiword();
{	LONG dst = regs.d[dstreg].D;
	if(src == 0){	MC68000_ICount -= 42;
	Exception(5,oldpc-2);} else {
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
	MC68000_ICount -= 154;
	}
}}}}
