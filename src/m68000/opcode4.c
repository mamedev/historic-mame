#include "cpudefs.h"
#include "M68000.h"
extern int pending_interrupts;

void op_4000(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg].B.l;
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        regs.d[srcreg].B.l = newv;
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
}}}}}
void op_4010(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_byte(srca,newv);
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
}}}}}
void op_4018(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_byte(srca,newv);
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
}}}}}}
void op_4020(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_byte(srca,newv);
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
}}}}}}
void op_4028(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_byte(srca,newv);
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
}}}}}
void op_4030(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_byte(srca,newv);
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
}}}}}}
void op_4038(void) /* NEGX */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_byte(srca,newv);
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
}}}}}
void op_4039(void) /* NEGX */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_byte(srca,newv);
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
}}}}}
void op_4040(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.d[srcreg].W.l;
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        regs.d[srcreg].W.l = newv;
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
}}}}}
void op_4050(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_word(srca,newv);
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
}}}}}
void op_4058(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_word(srca,newv);
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
}}}}}}
void op_4060(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_word(srca,newv);
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
}}}}}}
void op_4068(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_word(srca,newv);
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
}}}}}
void op_4070(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_word(srca,newv);
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
}}}}}}
void op_4078(void) /* NEGX */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_word(srca,newv);
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
}}}}}
void op_4079(void) /* NEGX */
{
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_word(srca,newv);
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
}}}}}
void op_4080(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg].D;
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        regs.d[srcreg].D = (newv);
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
}}}}}
void op_4090(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_long(srca,newv);
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
}}}}}
void op_4098(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       regs.a[srcreg] += 4;
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_long(srca,newv);
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
}}}}}}
void op_40a0(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_long(srca,newv);
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
}}}}}}
void op_40a8(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_long(srca,newv);
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
}}}}}
void op_40b0(void) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_long(srca,newv);
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
}}}}}}
void op_40b8(void) /* NEGX */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_long(srca,newv);
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
}}}}}
void op_40b9(void) /* NEGX */
{
{{      CPTR srca = nextilong();
        LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
        put_long(srca,newv);
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (flgs && flgn);
        regs.x = CFLG = (flgs || flgn);
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
}}}}}
void op_40c0(void) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      MakeSR();
        regs.d[srcreg].W.l = regs.sr;
}}}
void op_40d0(void) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        MakeSR();
        put_word(srca,regs.sr);
}}}
void op_40d8(void) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[srcreg] += 2;
        MakeSR();
        put_word(srca,regs.sr);
}}}}
void op_40e0(void) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        MakeSR();
        put_word(srca,regs.sr);
}}}}
void op_40e8(void) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        MakeSR();
        put_word(srca,regs.sr);
}}}
void op_40f0(void) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
        MakeSR();
        put_word(srca,regs.sr);
}}}
void op_40f8(void) /* MVSR2 */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        MakeSR();
        put_word(srca,regs.sr);
}}}
void op_40f9(void) /* MVSR2 */
{
{{      CPTR srca = nextilong();
        MakeSR();
        put_word(srca,regs.sr);
}}}
void op_4100(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       LONG src = regs.d[srcreg].D;
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4110(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4118(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       regs.a[srcreg] += 4;
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_4120(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_4128(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4130(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = get_disp_ea(regs.a[srcreg]);
{       LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_4138(void) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4139(void) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = nextilong();
        LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_413a(void) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_413b(void) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = get_disp_ea(m68k_getpc());
{       LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_413c(void) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       LONG src = nextilong();
{       LONG dst = regs.d[dstreg].D;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4180(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       WORD src = regs.d[srcreg].W.l;
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4190(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4198(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_41a0(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_41a8(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_41b0(void) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_41b8(void) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_41b9(void) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = nextilong();
        WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_41ba(void) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_41bb(void) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = get_disp_ea(m68k_getpc());
{       WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_41bc(void) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       WORD src = nextiword();
{       WORD dst = regs.d[dstreg].W.l;
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_41d0(void) /* LEA */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = regs.a[srcreg];
{       regs.a[dstreg] = (srca);
}}}}
void op_41e8(void) /* LEA */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{       regs.a[dstreg] = (srca);
}}}}
void op_41f0(void) /* LEA */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       regs.a[dstreg] = (srca);
}}}}
void op_41f8(void) /* LEA */
{
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = (LONG)(WORD)nextiword();
{       regs.a[dstreg] = (srca);
}}}}
void op_41f9(void) /* LEA */
{
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = nextilong();
{       regs.a[dstreg] = (srca);
}}}}
void op_41fa(void) /* LEA */
{
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       regs.a[dstreg] = (srca);
}}}}
void op_41fb(void) /* LEA */
{
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       regs.a[dstreg] = (srca);
}}}}
void op_4200(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
        regs.d[srcreg].B.l = 0;
{{      CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4210(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        put_byte(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4218(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[srcreg] += areg_byteinc[srcreg];
        put_byte(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}}
void op_4220(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        put_byte(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}}
void op_4228(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        put_byte(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4230(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
        put_byte(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4238(void) /* CLR */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        put_byte(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4239(void) /* CLR */
{
{{      CPTR srca = nextilong();
        put_byte(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4240(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
        regs.d[srcreg].W.l = 0;
{{      CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4250(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        put_word(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4258(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[srcreg] += 2;
        put_word(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}}
void op_4260(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        put_word(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}}
void op_4268(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        put_word(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4270(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
        put_word(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4278(void) /* CLR */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        put_word(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4279(void) /* CLR */
{
{{      CPTR srca = nextilong();
        put_word(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4280(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
        regs.d[srcreg].D = (0);
{{      CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4290(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        put_long(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4298(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[srcreg] += 4;
        put_long(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}}
void op_42a0(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        put_long(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}}
void op_42a8(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        put_long(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_42b0(void) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
        put_long(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_42b8(void) /* CLR */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        put_long(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_42b9(void) /* CLR */
{
{{      CPTR srca = nextilong();
        put_long(srca,0);
        CLEARFLGS;
        ZFLG = ZTRUE ;
}}}
void op_4400(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg].B.l;
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
        regs.d[srcreg].B.l = dst;
        ZFLG = ((BYTE)(dst)) == 0;
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4410(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
        put_byte(srca,dst);
        ZFLG = ((BYTE)(dst)) == 0;
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4418(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
        put_byte(srca,dst);
        ZFLG = ((BYTE)(dst)) == 0;
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
}}}}}}}
void op_4420(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
        put_byte(srca,dst);
        ZFLG = ((BYTE)(dst)) == 0;
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
}}}}}}}
void op_4428(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
        put_byte(srca,dst);
        ZFLG = ((BYTE)(dst)) == 0;
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4430(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
        put_byte(srca,dst);
        ZFLG = ((BYTE)(dst)) == 0;
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
}}}}}}}
void op_4438(void) /* NEG */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
        put_byte(srca,dst);
        ZFLG = ((BYTE)(dst)) == 0;
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4439(void) /* NEG */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
        put_byte(srca,dst);
        ZFLG = ((BYTE)(dst)) == 0;
{       int flgs = ((BYTE)(src)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4440(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.d[srcreg].W.l;
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
        regs.d[srcreg].W.l = dst;
        ZFLG = ((WORD)(dst)) == 0;
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4450(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
        put_word(srca,dst);
        ZFLG = ((WORD)(dst)) == 0;
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4458(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
        put_word(srca,dst);
        ZFLG = ((WORD)(dst)) == 0;
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
}}}}}}}
void op_4460(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
        put_word(srca,dst);
        ZFLG = ((WORD)(dst)) == 0;
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
}}}}}}}
void op_4468(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
        put_word(srca,dst);
        ZFLG = ((WORD)(dst)) == 0;
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4470(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
        put_word(srca,dst);
        ZFLG = ((WORD)(dst)) == 0;
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
}}}}}}}
void op_4478(void) /* NEG */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
        put_word(srca,dst);
        ZFLG = ((WORD)(dst)) == 0;
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4479(void) /* NEG */
{
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
        put_word(srca,dst);
        ZFLG = ((WORD)(dst)) == 0;
{       int flgs = ((WORD)(src)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4480(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg].D;
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
        regs.d[srcreg].D = (dst);
        ZFLG = ((LONG)(dst)) == 0;
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4490(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
        put_long(srca,dst);
        ZFLG = ((LONG)(dst)) == 0;
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
}}}}}}
void op_4498(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       regs.a[srcreg] += 4;
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
        put_long(srca,dst);
        ZFLG = ((LONG)(dst)) == 0;
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
}}}}}}}
void op_44a0(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
        put_long(srca,dst);
        ZFLG = ((LONG)(dst)) == 0;
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
}}}}}}}
void op_44a8(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
        put_long(srca,dst);
        ZFLG = ((LONG)(dst)) == 0;
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
}}}}}}
void op_44b0(void) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
        put_long(srca,dst);
        ZFLG = ((LONG)(dst)) == 0;
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
}}}}}}}
void op_44b8(void) /* NEG */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
        put_long(srca,dst);
        ZFLG = ((LONG)(dst)) == 0;
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
}}}}}}
void op_44b9(void) /* NEG */
{
{{      CPTR srca = nextilong();
        LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
        put_long(srca,dst);
        ZFLG = ((LONG)(dst)) == 0;
{       int flgs = ((LONG)(src)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        VFLG = (flgs && flgn);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
}}}}}}
void op_44c0(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.d[srcreg].W.l;
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_44d0(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_44d8(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}}
void op_44e0(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}}
void op_44e8(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_44f0(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}}
void op_44f8(void) /* MV2SR */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_44f9(void) /* MV2SR */
{
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_44fa(void) /* MV2SR */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}}
void op_44fb(void) /* MV2SR */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}}
void op_44fc(void) /* MV2SR */
{
{{      WORD src = nextiword();
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_4600(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg].B.l;
{       ULONG dst = ~src;
        regs.d[srcreg].B.l = dst;
        CLEARFLGS;
        ZFLG = ((BYTE)(dst)) == 0;
        NFLG = ((BYTE)(dst)) < 0;
}}}}
void op_4610(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        put_byte(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4618(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
{       ULONG dst = ~src;
        put_byte(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}}
void op_4620(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        put_byte(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}}
void op_4628(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        put_byte(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4630(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        put_byte(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}}
void op_4638(void) /* NOT */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        put_byte(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4639(void) /* NOT */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        put_byte(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4640(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.d[srcreg].W.l;
{       ULONG dst = ~src;
        regs.d[srcreg].W.l = dst;
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4650(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       ULONG dst = ~src;
        put_word(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4658(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
{       ULONG dst = ~src;
        put_word(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}}
void op_4660(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       ULONG dst = ~src;
        put_word(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}}
void op_4668(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       ULONG dst = ~src;
        put_word(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4670(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
{       ULONG dst = ~src;
        put_word(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}}
void op_4678(void) /* NOT */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       ULONG dst = ~src;
        put_word(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4679(void) /* NOT */
{
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
{       ULONG dst = ~src;
        put_word(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4680(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg].D;
{       ULONG dst = ~src;
        regs.d[srcreg].D = (dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4690(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       ULONG dst = ~src;
        put_long(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4698(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       regs.a[srcreg] += 4;
{       ULONG dst = ~src;
        put_long(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}}
void op_46a0(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       ULONG dst = ~src;
        put_long(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}}
void op_46a8(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       ULONG dst = ~src;
        put_long(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_46b0(void) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       LONG src = get_long(srca);
{       ULONG dst = ~src;
        put_long(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}}
void op_46b8(void) /* NOT */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       ULONG dst = ~src;
        put_long(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_46b9(void) /* NOT */
{
{{      CPTR srca = nextilong();
        LONG src = get_long(srca);
{       ULONG dst = ~src;
        put_long(srca,dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_46c0(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      WORD src = regs.d[srcreg].W.l;
        regs.sr = src;
        MakeFromSR();
}}}}
void op_46d0(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}
void op_46d8(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
        regs.sr = src;
        MakeFromSR();
}}}}}
void op_46e0(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}}
void op_46e8(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}
void op_46f0(void) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}}
void op_46f8(void) /* MV2SR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}
void op_46f9(void) /* MV2SR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}
void op_46fa(void) /* MV2SR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}}
void op_46fb(void) /* MV2SR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}}
void op_46fc(void) /* MV2SR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      WORD src = nextiword();
        regs.sr = src;
        MakeFromSR();
}}}}
void op_4800(void) /* NBCD */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg].B.l;
{       UWORD newv_lo = - (src & 0xF) - (regs.x ? 1 : 0);
        UWORD newv_hi = - (src & 0xF0);
        UWORD newv;
        if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
        newv = newv_hi + (newv_lo & 0xF);       CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
        if (CFLG) newv -= 0x60;
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        regs.d[srcreg].B.l = newv;
}}}}
void op_4810(void) /* NBCD */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       UWORD newv_lo = - (src & 0xF) - (regs.x ? 1 : 0);
        UWORD newv_hi = - (src & 0xF0);
        UWORD newv;
        if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
        newv = newv_hi + (newv_lo & 0xF);       CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
        if (CFLG) newv -= 0x60;
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}
void op_4818(void) /* NBCD */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
{       UWORD newv_lo = - (src & 0xF) - (regs.x ? 1 : 0);
        UWORD newv_hi = - (src & 0xF0);
        UWORD newv;
        if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
        newv = newv_hi + (newv_lo & 0xF);       CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
        if (CFLG) newv -= 0x60;
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}}
void op_4820(void) /* NBCD */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       UWORD newv_lo = - (src & 0xF) - (regs.x ? 1 : 0);
        UWORD newv_hi = - (src & 0xF0);
        UWORD newv;
        if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
        newv = newv_hi + (newv_lo & 0xF);       CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
        if (CFLG) newv -= 0x60;
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}}
void op_4828(void) /* NBCD */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{       UWORD newv_lo = - (src & 0xF) - (regs.x ? 1 : 0);
        UWORD newv_hi = - (src & 0xF0);
        UWORD newv;
        if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
        newv = newv_hi + (newv_lo & 0xF);       CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
        if (CFLG) newv -= 0x60;
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}
void op_4830(void) /* NBCD */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
{       UWORD newv_lo = - (src & 0xF) - (regs.x ? 1 : 0);
        UWORD newv_hi = - (src & 0xF0);
        UWORD newv;
        if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
        newv = newv_hi + (newv_lo & 0xF);       CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
        if (CFLG) newv -= 0x60;
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}}
void op_4838(void) /* NBCD */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{       UWORD newv_lo = - (src & 0xF) - (regs.x ? 1 : 0);
        UWORD newv_hi = - (src & 0xF0);
        UWORD newv;
        if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
        newv = newv_hi + (newv_lo & 0xF);       CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
        if (CFLG) newv -= 0x60;
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}
void op_4839(void) /* NBCD */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
{       UWORD newv_lo = - (src & 0xF) - (regs.x ? 1 : 0);
        UWORD newv_hi = - (src & 0xF0);
        UWORD newv;
        if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
        newv = newv_hi + (newv_lo & 0xF);       CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
        if (CFLG) newv -= 0x60;
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}
void op_4840(void) /* SWAP */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg].D;
{       ULONG dst = ((src >> 16)&0xFFFF) | ((src&0xFFFF)<<16);
        CLEARFLGS;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        regs.d[srcreg].D = (dst);
}}}}
void op_4850(void) /* PEA */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_4868(void) /* PEA */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_4870(void) /* PEA */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_4878(void) /* PEA */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_4879(void) /* PEA */
{
{{      CPTR srca = nextilong();
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_487a(void) /* PEA */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_487b(void) /* PEA */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_4880(void) /* EXT */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg].D;
{       UWORD dst = (WORD)(BYTE)src;
        CLEARFLGS;
        ZFLG = ((WORD)(dst)) == 0;
        NFLG = ((WORD)(dst)) < 0;
        regs.d[srcreg].W.l = dst;
}}}}
void op_4890(void) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = regs.a[dstreg];
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_word(srca, regs.d[movem_index1[dmask]].D); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { put_word(srca, regs.a[movem_index1[amask]]); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_48a0(void) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{{      CPTR srca = regs.a[dstreg];
{       UWORD amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
        while (amask) { srca -= 2; put_word(srca, regs.a[movem_index2[amask]]); amask = movem_next[amask]; MC68000_ICount -= 4; }
	while (dmask) { srca -= 2; put_word(srca, regs.d[movem_index2[dmask]].D); dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        regs.a[dstreg] = srca;
}}}}}
void op_48a8(void) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = regs.a[dstreg] + (LONG)(WORD)nextiword();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_word(srca, regs.d[movem_index1[dmask]].D); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { put_word(srca, regs.a[movem_index1[amask]]); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_48b0(void) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = get_disp_ea(regs.a[dstreg]);
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_word(srca, regs.d[movem_index1[dmask]].D); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { put_word(srca, regs.a[movem_index1[amask]]); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_48b8(void) /* MVMLE */
{
{       UWORD mask = nextiword();
{       CPTR srca = (LONG)(WORD)nextiword();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_word(srca, regs.d[movem_index1[dmask]].D); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { put_word(srca, regs.a[movem_index1[amask]]); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_48b9(void) /* MVMLE */
{
{       UWORD mask = nextiword();
{       CPTR srca = nextilong();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_word(srca, regs.d[movem_index1[dmask]].D); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { put_word(srca, regs.a[movem_index1[amask]]); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_48c0(void) /* EXT */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg].D;
{       ULONG dst = (LONG)(WORD)src;
        CLEARFLGS;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        regs.d[srcreg].D = (dst);
}}}}
void op_48d0(void) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = regs.a[dstreg];
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_long(srca, regs.d[movem_index1[dmask]].D); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
        while (amask) { put_long(srca, regs.a[movem_index1[amask]]); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_48e0(void) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{{      CPTR srca = regs.a[dstreg];
{       UWORD amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
        while (amask) { srca -= 4; put_long(srca, regs.a[movem_index2[amask]]); amask = movem_next[amask]; MC68000_ICount -= 8; }
	while (dmask) { srca -= 4; put_long(srca, regs.d[movem_index2[dmask]].D); dmask = movem_next[dmask]; MC68000_ICount -= 8; }
        regs.a[dstreg] = srca;
}}}}}
void op_48e8(void) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = regs.a[dstreg] + (LONG)(WORD)nextiword();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_long(srca, regs.d[movem_index1[dmask]].D); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
        while (amask) { put_long(srca, regs.a[movem_index1[amask]]); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_48f0(void) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = get_disp_ea(regs.a[dstreg]);
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_long(srca, regs.d[movem_index1[dmask]].D); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
        while (amask) { put_long(srca, regs.a[movem_index1[amask]]); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_48f8(void) /* MVMLE */
{
{       UWORD mask = nextiword();
{       CPTR srca = (LONG)(WORD)nextiword();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_long(srca, regs.d[movem_index1[dmask]].D); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
        while (amask) { put_long(srca, regs.a[movem_index1[amask]]); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_48f9(void) /* MVMLE */
{
{       UWORD mask = nextiword();
{       CPTR srca = nextilong();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_long(srca, regs.d[movem_index1[dmask]].D); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
        while (amask) { put_long(srca, regs.a[movem_index1[amask]]); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_49c0(void) /* EXT */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg].D;
{       ULONG dst = (LONG)(BYTE)src;
        regs.d[srcreg].D = (dst);
        CLEARFLGS;
	if ( dst <= 0 )
		{
		if ( dst == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a00(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg].B.l;
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a10(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a18(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a20(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a28(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a30(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a38(void) /* TST */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a39(void) /* TST */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a3a(void) /* TST */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       BYTE src = get_byte(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a3b(void) /* TST */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       BYTE src = get_byte(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a3c(void) /* TST */
{
{{      BYTE src = nextiword();
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a40(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.d[srcreg].W.l;
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a48(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.a[srcreg];
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a50(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a58(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a60(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a68(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a70(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a78(void) /* TST */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a79(void) /* TST */
{
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a7a(void) /* TST */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       WORD src = get_word(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a7b(void) /* TST */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       WORD src = get_word(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4a7c(void) /* TST */
{
{{      WORD src = nextiword();
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a80(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg].D;
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a88(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.a[srcreg];
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a90(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4a98(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       regs.a[srcreg] += 4;
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4aa0(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4aa8(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4ab0(void) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       LONG src = get_long(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4ab8(void) /* TST */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4ab9(void) /* TST */
{
{{      CPTR srca = nextilong();
        LONG src = get_long(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4aba(void) /* TST */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       LONG src = get_long(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4abb(void) /* TST */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       LONG src = get_long(srca);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4abc(void) /* TST */
{
{{      LONG src = nextilong();
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4ac0(void) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg].B.l;
        regs.d[srcreg].B.l = src | 0x80;
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4ad0(void) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
        put_byte(srca,src | 0x80 );
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4ad8(void) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
        put_byte(srca,src | 0x80 );
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4ae0(void) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
        put_byte(srca,src | 0x80 );
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4ae8(void) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
        put_byte(srca,src | 0x80);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4af0(void) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
        put_byte(srca,src | 0x80);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}}
void op_4af8(void) /* TAS */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
        put_byte(srca,src | 0x80);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4af9(void) /* TAS */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
        put_byte(srca,src | 0x80);
        CLEARFLGS;
	if ( src <= 0 )
		{
		if ( src == 0 )
			ZFLG = ZTRUE ;
		else
			NFLG = NTRUE ;
		}
}}}
void op_4c90(void) /* MVMEL */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg];
{	while (dmask) { regs.d[movem_index1[dmask]].D = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_4c98(void) /* MVMEL */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg];
{	while (dmask) { regs.d[movem_index1[dmask]].D = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
        regs.a[dstreg] = srca;
}}}}
void op_4ca8(void) /* MVMEL */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg] + (LONG)(WORD)nextiword();
{	while (dmask) { regs.d[movem_index1[dmask]].D = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_4cb0(void) /* MVMEL */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = get_disp_ea(regs.a[dstreg]);
{	while (dmask) { regs.d[movem_index1[dmask]].D = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_4cb8(void) /* MVMEL */
{
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = (LONG)(WORD)nextiword();
{	while (dmask) { regs.d[movem_index1[dmask]].D = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_4cb9(void) /* MVMEL */
{
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = nextilong();
{	while (dmask) { regs.d[movem_index1[dmask]].D = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_4cba(void) /* MVMEL */
{
				ULONG dstreg;
				dstreg = 2;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = m68k_getpc();
				srca += (LONG)(WORD)nextiword();
{	while (dmask) { regs.d[movem_index1[dmask]].D = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
				while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_4cbb(void) /* MVMEL */
{
				ULONG dstreg;
				dstreg = 3;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = get_disp_ea(m68k_getpc());
{	while (dmask) { regs.d[movem_index1[dmask]].D = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; MC68000_ICount -= 4; }
	while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; MC68000_ICount -= 4; }
}}}}
void op_4cd0(void) /* MVMEL */
{
				ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg];
{	while (dmask) { regs.d[movem_index1[dmask]].D = get_long(srca); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
	while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_4cd8(void) /* MVMEL */
{
				ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg];
{	while (dmask) { regs.d[movem_index1[dmask]].D = get_long(srca); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
	while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
	regs.a[dstreg] = srca;
}}}}
void op_4ce8(void) /* MVMEL */
{
				ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg] + (LONG)(WORD)nextiword();
{	while (dmask) { regs.d[movem_index1[dmask]].D = get_long(srca); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
	while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_4cf0(void) /* MVMEL */
{
				ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = get_disp_ea(regs.a[dstreg]);
{	while (dmask) { regs.d[movem_index1[dmask]].D = get_long(srca); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
	while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_4cf8(void) /* MVMEL */
{
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = (LONG)(WORD)nextiword();
{	while (dmask) { regs.d[movem_index1[dmask]].D = get_long(srca); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
	while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_4cf9(void) /* MVMEL */
{
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = nextilong();
{	while (dmask) { regs.d[movem_index1[dmask]].D = get_long(srca); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
	while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_4cfa(void) /* MVMEL */
{
				ULONG dstreg;
				dstreg = 2;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = m68k_getpc();
				srca += (LONG)(WORD)nextiword();
{	while (dmask) { regs.d[movem_index1[dmask]].D = get_long(srca); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
	while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_4cfb(void) /* MVMEL */
{
				ULONG dstreg;
				dstreg = 3;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = get_disp_ea(m68k_getpc());
{	while (dmask) { regs.d[movem_index1[dmask]].D = get_long(srca); srca += 4; dmask = movem_next[dmask]; MC68000_ICount -= 8; }
	while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; MC68000_ICount -= 8; }
}}}}
void op_4e40(void) /* TRAP */
{
				ULONG srcreg = (opcode & 15);
{{      ULONG src = srcreg;
				Exception(src+32,0);
}}}
void op_4e50(void) /* LINK */
{
				ULONG srcreg = (opcode & 7);
{{      regs.a[7] -= 4;
{       CPTR olda = regs.a[7];
{       LONG src = regs.a[srcreg];
	put_long(olda,src);
	regs.a[srcreg] = (regs.a[7]);
{       WORD offs = nextiword();
        regs.a[7] += offs;
}}}}}}
void op_4e58(void) /* UNLK */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.a[srcreg];
        regs.a[7] = src;
{       CPTR olda = regs.a[7];
        LONG old = get_long(olda);
{       regs.a[7] += 4;
        regs.a[srcreg] = (old);
}}}}}
void op_4e60(void) /* MVR2USP */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      LONG src = regs.a[srcreg];
        regs.usp = src;
}}}}
void op_4e68(void) /* MVUSP2R */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      regs.a[srcreg] = (regs.usp);
}}}}
void op_4e70(void) /* RESET */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{
}}}
void op_4e71(void) /* NOP */
{
{}}
void op_4e72(void) /* STOP */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      WORD src = nextiword();
        regs.sr = src;
        MakeFromSR();
        pending_interrupts |= MC68000_STOP;
        MC68000_ICount = 0;
}}}}
void op_4e73(void) /* RTE */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR sra = regs.a[7];
        WORD sr = get_word(sra);
{       regs.a[7] += 2;
{       CPTR pca = regs.a[7];
        LONG pc = get_long(pca);
{       regs.a[7] += 4;
        regs.sr = sr; m68k_setpc(pc);
        MakeFromSR();
}}}}}}}
void op_4e74(void) /* RTD */
{
{{      CPTR pca = regs.a[7];
        LONG pc = get_long(pca);
{       regs.a[7] += 4;
{       WORD offs = nextiword();
        regs.a[7] += offs;
        m68k_setpc(pc);
}}}}}
void op_4e75(void) /* RTS */
{
{{      CPTR pca = regs.a[7];
        LONG pc = get_long(pca);
{       regs.a[7] += 4;
        m68k_setpc(pc);
}}}}
void op_4e76(void) /* TRAPV */
{
{       if(VFLG) Exception(7,m68k_getpc()-2);
}}
void op_4e77(void) /* RTR */
{
{       MakeSR();
{       CPTR sra = regs.a[7];
        WORD sr = get_word(sra);
{       regs.a[7] += 2;
{       CPTR pca = regs.a[7];
        LONG pc = get_long(pca);
{       regs.a[7] += 4;
        regs.sr &= 0xFF00; sr &= 0xFF;
        regs.sr |= sr; m68k_setpc(pc);
        MakeFromSR();
}}}}}}
void op_4e90(void) /* JSR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4ea8(void) /* JSR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4eb0(void) /* JSR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4eb8(void) /* JSR */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4eb9(void) /* JSR */
{
{{      CPTR srca = nextilong();
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4eba(void) /* JSR */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4ebb(void) /* JSR */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4ed0(void) /* JMP */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        m68k_setpc(srca);
}}}
void op_4ee8(void) /* JMP */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        m68k_setpc(srca);
}}}
void op_4ef0(void) /* JMP */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
        m68k_setpc(srca);
}}}
void op_4ef8(void) /* JMP */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        m68k_setpc(srca);
}}}
void op_4ef9(void) /* JMP */
{
{{      CPTR srca = nextilong();
        m68k_setpc(srca);
}}}
void op_4efa(void) /* JMP */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
        m68k_setpc(srca);
}}}
void op_4efb(void) /* JMP */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
        m68k_setpc(srca);
}}}
