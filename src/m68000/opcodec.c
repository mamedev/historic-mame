#include "cpudefs.h"
void op_c000(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_c010(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_c018(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}}
void op_c020(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}}
void op_c028(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_c030(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}}
void op_c038(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_c039(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_c03a(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}}
void op_c03b(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}}
void op_c03c(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = nextiword();
{	BYTE dst = regs.d[dstreg].B.l;
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
}}}}
void op_c040(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
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
void op_c050(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
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
void op_c058(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
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
}}}}}
void op_c060(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
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
}}}}}
void op_c068(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
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
void op_c070(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
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
}}}}}
void op_c078(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
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
void op_c079(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
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
void op_c07a(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
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
}}}}}
void op_c07b(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
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
}}}}}
void op_c07c(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
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
void op_c080(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg].D;
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
void op_c090(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
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
void op_c098(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
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
}}}}}
void op_c0a0(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
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
}}}}}
void op_c0a8(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
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
void op_c0b0(void) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
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
}}}}}
void op_c0b8(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
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
void op_c0b9(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
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
void op_c0ba(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
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
}}}}}
void op_c0bb(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
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
}}}}}
void op_c0bc(void) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
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
void op_c0c0(void) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c0d0(void) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARFLGS;
	regs.d[dstreg].D = (newv);
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c0d8(void) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}}
void op_c0e0(void) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}}
void op_c0e8(void) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c0f0(void) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}}
void op_c0f8(void) /* MULU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c0f9(void) /* MULU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c0fa(void) /* MULU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}}
void op_c0fb(void) /* MULU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}}
void op_c0fc(void) /* MULU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c100(void) /* ABCD */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	BYTE dst = regs.d[dstreg].B.l;
{	UWORD newv_lo = (src & 0xF) + (dst & 0xF) + (regs.x ? 1 : 0);
	UWORD newv_hi = (src & 0xF0) + (dst & 0xF0);
	UWORD newv;
	if (newv_lo > 9) { newv_lo +=6; }
	newv = newv_hi + newv_lo;
	regs.d[dstreg].B.l = newv;
	CFLG = regs.x = (newv & 0x1F0) > 0x90;
	if (CFLG) newv += 0x60;
	if (((BYTE)(newv)) != 0) ZFLG = 0;
	NFLG = ((BYTE)(newv)) < 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
}}}}}}
void op_c108(void) /* ABCD */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	UWORD newv_lo = (src & 0xF) + (dst & 0xF) + (regs.x ? 1 : 0);
	UWORD newv_hi = (src & 0xF0) + (dst & 0xF0);
	UWORD newv;
	if (newv_lo > 9) { newv_lo +=6; }
	newv = newv_hi + newv_lo;	CFLG = regs.x = (newv & 0x1F0) > 0x90;
	if (CFLG) newv += 0x60;
	if (((BYTE)(newv)) != 0) ZFLG = 0;
	NFLG = ((BYTE)(newv)) < 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	put_byte(dsta,newv);
}}}}}}}}
void op_c110(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
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
}}}}
void op_c118(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
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
void op_c120(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
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
}}}}}
void op_c128(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
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
void op_c130(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
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
void op_c138(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
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
void op_c139(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
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
void op_c140(void) /* EXG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
	LONG src = regs.d[srcreg].D;
	regs.d[srcreg].D = regs.d[dstreg].D;
	regs.d[dstreg].D = src;
}

void op_c148(void) /* EXG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
	LONG src = regs.a[srcreg];
	regs.a[srcreg] = regs.a[dstreg];
	regs.a[dstreg] = src;
}

void op_c150(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
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
void op_c158(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{	regs.a[dstreg] += 2;
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
void op_c160(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
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
void op_c168(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
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
void op_c170(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
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
void op_c178(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg].W.l;
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
void op_c179(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg].W.l;
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
void op_c188(void) /* EXG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
	LONG src = regs.d[srcreg].D;
	regs.d[srcreg].D = regs.a[dstreg];
	regs.a[dstreg] = (src);
}

void op_c190(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
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
void op_c198(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
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
void op_c1a0(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
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
void op_c1a8(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
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
void op_c1b0(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
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
void op_c1b8(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg].D;
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
void op_c1b9(void) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg].D;
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
void op_c1c0(void) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c1d0(void) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c1d8(void) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}}
void op_c1e0(void) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}}
void op_c1e8(void) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c1f0(void) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}}
void op_c1f8(void) /* MULS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c1f9(void) /* MULS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
void op_c1fa(void) /* MULS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}}
void op_c1fb(void) /* MULS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}}
void op_c1fc(void) /* MULS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg].W.l;
{	LONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	regs.d[dstreg].D = (newv);
	CLEARFLGS;
	if ( newv <= 0 )
		{
		if ( newv ==  0 )
			{
			ZFLG = ZTRUE;
			}
		else
			{
			NFLG = NTRUE;
			}
		}
}}}}}
