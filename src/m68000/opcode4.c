#include "cpudefs.h"
void op_4000(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg];
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xff) | ((newv) & 0xff);
}}}}}
void op_4010(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}}
void op_4018(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}}}
void op_4020(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}}}
void op_4028(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}}
void op_4030(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}}}
void op_4038(ULONG opcode) /* NEGX */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}}
void op_4039(ULONG opcode) /* NEGX */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        put_byte(srca,newv);
}}}}}
void op_4040(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.d[srcreg];
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}
void op_4050(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
        put_word(srca,newv);
}}}}}
void op_4058(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
        put_word(srca,newv);
}}}}}}
void op_4060(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
        put_word(srca,newv);
}}}}}}
void op_4068(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
        put_word(srca,newv);
}}}}}
void op_4070(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
        put_word(srca,newv);
}}}}}}
void op_4078(ULONG opcode) /* NEGX */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
        put_word(srca,newv);
}}}}}
void op_4079(ULONG opcode) /* NEGX */
{
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((WORD)(newv)) != 0) ZFLG = 0;
        NFLG = ((WORD)(newv)) < 0;
        put_word(srca,newv);
}}}}}
void op_4080(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg];
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
        regs.d[srcreg] = (newv);
}}}}}
void op_4090(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
        put_long(srca,newv);
}}}}}
void op_4098(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       regs.a[srcreg] += 4;
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
        put_long(srca,newv);
}}}}}}
void op_40a0(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
        put_long(srca,newv);
}}}}}}
void op_40a8(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
        put_long(srca,newv);
}}}}}
void op_40b0(ULONG opcode) /* NEGX */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
        put_long(srca,newv);
}}}}}}
void op_40b8(ULONG opcode) /* NEGX */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
        put_long(srca,newv);
}}}}}
void op_40b9(ULONG opcode) /* NEGX */
{
{{      CPTR srca = nextilong();
        LONG src = get_long(srca);
{       ULONG newv = 0 - src - (regs.x ? 1 : 0);
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(newv)) < 0;
        VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
        regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
        if (((LONG)(newv)) != 0) ZFLG = 0;
        NFLG = ((LONG)(newv)) < 0;
        put_long(srca,newv);
}}}}}
void op_40c0(ULONG opcode) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      MakeSR();
        regs.d[srcreg] = (regs.d[srcreg] & ~0xffff) | ((regs.sr) & 0xffff);
}}}
void op_40d0(ULONG opcode) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        MakeSR();
        put_word(srca,regs.sr);
}}}
void op_40d8(ULONG opcode) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[srcreg] += 2;
        MakeSR();
        put_word(srca,regs.sr);
}}}}
void op_40e0(ULONG opcode) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        MakeSR();
        put_word(srca,regs.sr);
}}}}
void op_40e8(ULONG opcode) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        MakeSR();
        put_word(srca,regs.sr);
}}}
void op_40f0(ULONG opcode) /* MVSR2 */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
        MakeSR();
        put_word(srca,regs.sr);
}}}
void op_40f8(ULONG opcode) /* MVSR2 */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        MakeSR();
        put_word(srca,regs.sr);
}}}
void op_40f9(ULONG opcode) /* MVSR2 */
{
{{      CPTR srca = nextilong();
        MakeSR();
        put_word(srca,regs.sr);
}}}
void op_4100(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       LONG src = regs.d[srcreg];
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4110(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4118(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       regs.a[srcreg] += 4;
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_4120(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_4128(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4130(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = get_disp_ea(regs.a[srcreg]);
{       LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_4138(ULONG opcode) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4139(ULONG opcode) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = nextilong();
        LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_413a(ULONG opcode) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_413b(ULONG opcode) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = get_disp_ea(m68k_getpc());
{       LONG src = get_long(srca);
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_413c(ULONG opcode) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       LONG src = nextilong();
{       LONG dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4180(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       WORD src = regs.d[srcreg];
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4190(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_4198(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_41a0(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_41a8(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_41b0(ULONG opcode) /* CHK */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_41b8(ULONG opcode) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_41b9(ULONG opcode) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = nextilong();
        WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_41ba(ULONG opcode) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_41bb(ULONG opcode) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       CPTR srca = get_disp_ea(m68k_getpc());
{       WORD src = get_word(srca);
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}}
void op_41bc(ULONG opcode) /* CHK */
{
        ULONG dstreg = (opcode >> 9) & 7;
{       CPTR oldpc = m68k_getpc();
{       WORD src = nextiword();
{       WORD dst = regs.d[dstreg];
        if ((LONG)dst < 0) { NFLG=1; Exception(6,oldpc-2); }
        else if (dst > src) { NFLG=0; Exception(6,oldpc-2); }
}}}}
void op_41d0(ULONG opcode) /* LEA */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = regs.a[srcreg];
{       regs.a[dstreg] = (srca);
}}}}
void op_41e8(ULONG opcode) /* LEA */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{       regs.a[dstreg] = (srca);
}}}}
void op_41f0(ULONG opcode) /* LEA */
{
        ULONG srcreg = (opcode & 7);
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       regs.a[dstreg] = (srca);
}}}}
void op_41f8(ULONG opcode) /* LEA */
{
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = (LONG)(WORD)nextiword();
{       regs.a[dstreg] = (srca);
}}}}
void op_41f9(ULONG opcode) /* LEA */
{
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = nextilong();
{       regs.a[dstreg] = (srca);
}}}}
void op_41fa(ULONG opcode) /* LEA */
{
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       regs.a[dstreg] = (srca);
}}}}
void op_41fb(ULONG opcode) /* LEA */
{
        ULONG dstreg = (opcode >> 9) & 7;
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       regs.a[dstreg] = (srca);
}}}}
void op_4200(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      VFLG = CFLG = 0;
        ZFLG = ((BYTE)(0)) == 0;
        NFLG = ((BYTE)(0)) < 0;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xff) | ((0) & 0xff);
}}}
void op_4210(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(0)) == 0;
        NFLG = ((BYTE)(0)) < 0;
        put_byte(srca,0);
}}}
void op_4218(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[srcreg] += areg_byteinc[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(0)) == 0;
        NFLG = ((BYTE)(0)) < 0;
        put_byte(srca,0);
}}}}
void op_4220(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(0)) == 0;
        NFLG = ((BYTE)(0)) < 0;
        put_byte(srca,0);
}}}}
void op_4228(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(0)) == 0;
        NFLG = ((BYTE)(0)) < 0;
        put_byte(srca,0);
}}}
void op_4230(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(0)) == 0;
        NFLG = ((BYTE)(0)) < 0;
        put_byte(srca,0);
}}}
void op_4238(ULONG opcode) /* CLR */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(0)) == 0;
        NFLG = ((BYTE)(0)) < 0;
        put_byte(srca,0);
}}}
void op_4239(ULONG opcode) /* CLR */
{
{{      CPTR srca = nextilong();
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(0)) == 0;
        NFLG = ((BYTE)(0)) < 0;
        put_byte(srca,0);
}}}
void op_4240(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      VFLG = CFLG = 0;
        ZFLG = ((WORD)(0)) == 0;
        NFLG = ((WORD)(0)) < 0;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xffff) | ((0) & 0xffff);
}}}
void op_4250(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(0)) == 0;
        NFLG = ((WORD)(0)) < 0;
        put_word(srca,0);
}}}
void op_4258(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[srcreg] += 2;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(0)) == 0;
        NFLG = ((WORD)(0)) < 0;
        put_word(srca,0);
}}}}
void op_4260(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(0)) == 0;
        NFLG = ((WORD)(0)) < 0;
        put_word(srca,0);
}}}}
void op_4268(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(0)) == 0;
        NFLG = ((WORD)(0)) < 0;
        put_word(srca,0);
}}}
void op_4270(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(0)) == 0;
        NFLG = ((WORD)(0)) < 0;
        put_word(srca,0);
}}}
void op_4278(ULONG opcode) /* CLR */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(0)) == 0;
        NFLG = ((WORD)(0)) < 0;
        put_word(srca,0);
}}}
void op_4279(ULONG opcode) /* CLR */
{
{{      CPTR srca = nextilong();
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(0)) == 0;
        NFLG = ((WORD)(0)) < 0;
        put_word(srca,0);
}}}
void op_4280(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      VFLG = CFLG = 0;
        ZFLG = ((LONG)(0)) == 0;
        NFLG = ((LONG)(0)) < 0;
        regs.d[srcreg] = (0);
}}}
void op_4290(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(0)) == 0;
        NFLG = ((LONG)(0)) < 0;
        put_long(srca,0);
}}}
void op_4298(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[srcreg] += 4;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(0)) == 0;
        NFLG = ((LONG)(0)) < 0;
        put_long(srca,0);
}}}}
void op_42a0(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(0)) == 0;
        NFLG = ((LONG)(0)) < 0;
        put_long(srca,0);
}}}}
void op_42a8(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(0)) == 0;
        NFLG = ((LONG)(0)) < 0;
        put_long(srca,0);
}}}
void op_42b0(ULONG opcode) /* CLR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(0)) == 0;
        NFLG = ((LONG)(0)) < 0;
        put_long(srca,0);
}}}
void op_42b8(ULONG opcode) /* CLR */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(0)) == 0;
        NFLG = ((LONG)(0)) < 0;
        put_long(srca,0);
}}}
void op_42b9(ULONG opcode) /* CLR */
{
{{      CPTR srca = nextilong();
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(0)) == 0;
        NFLG = ((LONG)(0)) < 0;
        put_long(srca,0);
}}}
void op_4400(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg];
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        ZFLG = ((BYTE)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xff) | ((dst) & 0xff);
}}}}}}
void op_4410(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        ZFLG = ((BYTE)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
        put_byte(srca,dst);
}}}}}}
void op_4418(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        ZFLG = ((BYTE)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
        put_byte(srca,dst);
}}}}}}}
void op_4420(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        ZFLG = ((BYTE)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
        put_byte(srca,dst);
}}}}}}}
void op_4428(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        ZFLG = ((BYTE)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
        put_byte(srca,dst);
}}}}}}
void op_4430(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        ZFLG = ((BYTE)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
        put_byte(srca,dst);
}}}}}}}
void op_4438(ULONG opcode) /* NEG */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        ZFLG = ((BYTE)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
        put_byte(srca,dst);
}}}}}}
void op_4439(ULONG opcode) /* NEG */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
{{ULONG dst = ((BYTE)(0)) - ((BYTE)(src));
{       int flgs = ((BYTE)(src)) < 0;
        int flgo = ((BYTE)(0)) < 0;
        int flgn = ((BYTE)(dst)) < 0;
        ZFLG = ((BYTE)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(0));
        NFLG = flgn != 0;
        put_byte(srca,dst);
}}}}}}
void op_4440(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.d[srcreg];
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        ZFLG = ((WORD)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xffff) | ((dst) & 0xffff);
}}}}}}
void op_4450(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        ZFLG = ((WORD)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
        put_word(srca,dst);
}}}}}}
void op_4458(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        ZFLG = ((WORD)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
        put_word(srca,dst);
}}}}}}}
void op_4460(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        ZFLG = ((WORD)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
        put_word(srca,dst);
}}}}}}}
void op_4468(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        ZFLG = ((WORD)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
        put_word(srca,dst);
}}}}}}
void op_4470(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        ZFLG = ((WORD)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
        put_word(srca,dst);
}}}}}}}
void op_4478(ULONG opcode) /* NEG */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        ZFLG = ((WORD)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
        put_word(srca,dst);
}}}}}}
void op_4479(ULONG opcode) /* NEG */
{
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
{{ULONG dst = ((WORD)(0)) - ((WORD)(src));
{       int flgs = ((WORD)(src)) < 0;
        int flgo = ((WORD)(0)) < 0;
        int flgn = ((WORD)(dst)) < 0;
        ZFLG = ((WORD)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(0));
        NFLG = flgn != 0;
        put_word(srca,dst);
}}}}}}
void op_4480(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg];
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        ZFLG = ((LONG)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
        regs.d[srcreg] = (dst);
}}}}}}
void op_4490(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        ZFLG = ((LONG)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
        put_long(srca,dst);
}}}}}}
void op_4498(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       regs.a[srcreg] += 4;
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        ZFLG = ((LONG)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
        put_long(srca,dst);
}}}}}}}
void op_44a0(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        ZFLG = ((LONG)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
        put_long(srca,dst);
}}}}}}}
void op_44a8(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        ZFLG = ((LONG)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
        put_long(srca,dst);
}}}}}}
void op_44b0(ULONG opcode) /* NEG */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        ZFLG = ((LONG)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
        put_long(srca,dst);
}}}}}}}
void op_44b8(ULONG opcode) /* NEG */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        ZFLG = ((LONG)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
        put_long(srca,dst);
}}}}}}
void op_44b9(ULONG opcode) /* NEG */
{
{{      CPTR srca = nextilong();
        LONG src = get_long(srca);
{{ULONG dst = ((LONG)(0)) - ((LONG)(src));
{       int flgs = ((LONG)(src)) < 0;
        int flgo = ((LONG)(0)) < 0;
        int flgn = ((LONG)(dst)) < 0;
        ZFLG = ((LONG)(dst)) == 0;
        VFLG = (flgs != flgo) && (flgn != flgo);
        CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(0));
        NFLG = flgn != 0;
        put_long(srca,dst);
}}}}}}
void op_44c0(ULONG opcode) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.d[srcreg];
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_44d0(ULONG opcode) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_44d8(ULONG opcode) /* MV2SR */
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
void op_44e0(ULONG opcode) /* MV2SR */
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
void op_44e8(ULONG opcode) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_44f0(ULONG opcode) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}}
void op_44f8(ULONG opcode) /* MV2SR */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_44f9(ULONG opcode) /* MV2SR */
{
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_44fa(ULONG opcode) /* MV2SR */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}}
void op_44fb(ULONG opcode) /* MV2SR */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       WORD src = get_word(srca);
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}}
void op_44fc(ULONG opcode) /* MV2SR */
{
{{      WORD src = nextiword();
        MakeSR();
        regs.sr &= 0xFF00;
        regs.sr |= src & 0xFF;
        MakeFromSR();
}}}
void op_4600(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg];
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(dst)) == 0;
        NFLG = ((BYTE)(dst)) < 0;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xff) | ((dst) & 0xff);
}}}}
void op_4610(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(dst)) == 0;
        NFLG = ((BYTE)(dst)) < 0;
        put_byte(srca,dst);
}}}}
void op_4618(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(dst)) == 0;
        NFLG = ((BYTE)(dst)) < 0;
        put_byte(srca,dst);
}}}}}
void op_4620(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(dst)) == 0;
        NFLG = ((BYTE)(dst)) < 0;
        put_byte(srca,dst);
}}}}}
void op_4628(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(dst)) == 0;
        NFLG = ((BYTE)(dst)) < 0;
        put_byte(srca,dst);
}}}}
void op_4630(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(dst)) == 0;
        NFLG = ((BYTE)(dst)) < 0;
        put_byte(srca,dst);
}}}}}
void op_4638(ULONG opcode) /* NOT */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(dst)) == 0;
        NFLG = ((BYTE)(dst)) < 0;
        put_byte(srca,dst);
}}}}
void op_4639(ULONG opcode) /* NOT */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(dst)) == 0;
        NFLG = ((BYTE)(dst)) < 0;
        put_byte(srca,dst);
}}}}
void op_4640(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.d[srcreg];
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(dst)) == 0;
        NFLG = ((WORD)(dst)) < 0;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xffff) | ((dst) & 0xffff);
}}}}
void op_4650(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(dst)) == 0;
        NFLG = ((WORD)(dst)) < 0;
        put_word(srca,dst);
}}}}
void op_4658(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(dst)) == 0;
        NFLG = ((WORD)(dst)) < 0;
        put_word(srca,dst);
}}}}}
void op_4660(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(dst)) == 0;
        NFLG = ((WORD)(dst)) < 0;
        put_word(srca,dst);
}}}}}
void op_4668(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(dst)) == 0;
        NFLG = ((WORD)(dst)) < 0;
        put_word(srca,dst);
}}}}
void op_4670(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(dst)) == 0;
        NFLG = ((WORD)(dst)) < 0;
        put_word(srca,dst);
}}}}}
void op_4678(ULONG opcode) /* NOT */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(dst)) == 0;
        NFLG = ((WORD)(dst)) < 0;
        put_word(srca,dst);
}}}}
void op_4679(ULONG opcode) /* NOT */
{
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(dst)) == 0;
        NFLG = ((WORD)(dst)) < 0;
        put_word(srca,dst);
}}}}
void op_4680(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg];
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        regs.d[srcreg] = (dst);
}}}}
void op_4690(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        put_long(srca,dst);
}}}}
void op_4698(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       regs.a[srcreg] += 4;
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        put_long(srca,dst);
}}}}}
void op_46a0(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        put_long(srca,dst);
}}}}}
void op_46a8(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        put_long(srca,dst);
}}}}
void op_46b0(ULONG opcode) /* NOT */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       LONG src = get_long(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        put_long(srca,dst);
}}}}}
void op_46b8(ULONG opcode) /* NOT */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        put_long(srca,dst);
}}}}
void op_46b9(ULONG opcode) /* NOT */
{
{{      CPTR srca = nextilong();
        LONG src = get_long(srca);
{       ULONG dst = ~src;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        put_long(srca,dst);
}}}}
void op_46c0(ULONG opcode) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      WORD src = regs.d[srcreg];
        regs.sr = src;
        MakeFromSR();
}}}}
void op_46d0(ULONG opcode) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}
void op_46d8(ULONG opcode) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
        regs.sr = src;
        MakeFromSR();
}}}}}
void op_46e0(ULONG opcode) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}}
void op_46e8(ULONG opcode) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}
void op_46f0(ULONG opcode) /* MV2SR */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}}
void op_46f8(ULONG opcode) /* MV2SR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}
void op_46f9(ULONG opcode) /* MV2SR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}
void op_46fa(ULONG opcode) /* MV2SR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}}
void op_46fb(ULONG opcode) /* MV2SR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       WORD src = get_word(srca);
        regs.sr = src;
        MakeFromSR();
}}}}}
void op_46fc(ULONG opcode) /* MV2SR */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      WORD src = nextiword();
        regs.sr = src;
        MakeFromSR();
}}}}
void op_4800(ULONG opcode) /* NBCD */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg];
{       UWORD newv_lo = - (src & 0xF) - (regs.x ? 1 : 0);
        UWORD newv_hi = - (src & 0xF0);
        UWORD newv;
        if (newv_lo > 9) { newv_lo-=6; newv_hi-=0x10; }
        newv = newv_hi + (newv_lo & 0xF);       CFLG = regs.x = (newv_hi & 0x1F0) > 0x90;
        if (CFLG) newv -= 0x60;
        if (((BYTE)(newv)) != 0) ZFLG = 0;
        NFLG = ((BYTE)(newv)) < 0;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xff) | ((newv) & 0xff);
}}}}
void op_4810(ULONG opcode) /* NBCD */
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
void op_4818(ULONG opcode) /* NBCD */
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
void op_4820(ULONG opcode) /* NBCD */
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
void op_4828(ULONG opcode) /* NBCD */
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
void op_4830(ULONG opcode) /* NBCD */
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
void op_4838(ULONG opcode) /* NBCD */
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
void op_4839(ULONG opcode) /* NBCD */
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
void op_4840(ULONG opcode) /* SWAP */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg];
{       ULONG dst = ((src >> 16)&0xFFFF) | ((src&0xFFFF)<<16);
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        regs.d[srcreg] = (dst);
}}}}
void op_4850(ULONG opcode) /* PEA */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_4868(ULONG opcode) /* PEA */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_4870(ULONG opcode) /* PEA */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_4878(ULONG opcode) /* PEA */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_4879(ULONG opcode) /* PEA */
{
{{      CPTR srca = nextilong();
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_487a(ULONG opcode) /* PEA */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_487b(ULONG opcode) /* PEA */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       regs.a[7] -= 4;
{       CPTR dsta = regs.a[7];
        put_long(dsta,srca);
}}}}}
void op_4880(ULONG opcode) /* EXT */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg];
{       UWORD dst = (WORD)(BYTE)src;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(dst)) == 0;
        NFLG = ((WORD)(dst)) < 0;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xffff) | ((dst) & 0xffff);
}}}}
void op_4890(ULONG opcode) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = regs.a[dstreg];
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
        while (dmask) { put_word(srca, regs.d[movem_index1[dmask]]); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { put_word(srca, regs.a[movem_index1[amask]]); srca += 2; amask = movem_next[amask]; }
}}}}
void op_48a0(ULONG opcode) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{{      CPTR srca = regs.a[dstreg];
{       UWORD amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
        while (amask) { srca -= 2; put_word(srca, regs.a[movem_index2[amask]]); amask = movem_next[amask]; }
        while (dmask) { srca -= 2; put_word(srca, regs.d[movem_index2[dmask]]); dmask = movem_next[dmask]; }
        regs.a[dstreg] = srca;
}}}}}
void op_48a8(ULONG opcode) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = regs.a[dstreg] + (LONG)(WORD)nextiword();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
        while (dmask) { put_word(srca, regs.d[movem_index1[dmask]]); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { put_word(srca, regs.a[movem_index1[amask]]); srca += 2; amask = movem_next[amask]; }
}}}}
void op_48b0(ULONG opcode) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = get_disp_ea(regs.a[dstreg]);
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
        while (dmask) { put_word(srca, regs.d[movem_index1[dmask]]); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { put_word(srca, regs.a[movem_index1[amask]]); srca += 2; amask = movem_next[amask]; }
}}}}
void op_48b8(ULONG opcode) /* MVMLE */
{
{       UWORD mask = nextiword();
{       CPTR srca = (LONG)(WORD)nextiword();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
        while (dmask) { put_word(srca, regs.d[movem_index1[dmask]]); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { put_word(srca, regs.a[movem_index1[amask]]); srca += 2; amask = movem_next[amask]; }
}}}}
void op_48b9(ULONG opcode) /* MVMLE */
{
{       UWORD mask = nextiword();
{       CPTR srca = nextilong();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
        while (dmask) { put_word(srca, regs.d[movem_index1[dmask]]); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { put_word(srca, regs.a[movem_index1[amask]]); srca += 2; amask = movem_next[amask]; }
}}}}
void op_48c0(ULONG opcode) /* EXT */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg];
{       ULONG dst = (LONG)(WORD)src;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        regs.d[srcreg] = (dst);
}}}}
void op_48d0(ULONG opcode) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = regs.a[dstreg];
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
        while (dmask) { put_long(srca, regs.d[movem_index1[dmask]]); srca += 4; dmask = movem_next[dmask]; }
        while (amask) { put_long(srca, regs.a[movem_index1[amask]]); srca += 4; amask = movem_next[amask]; }
}}}}
void op_48e0(ULONG opcode) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{{      CPTR srca = regs.a[dstreg];
{       UWORD amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
        while (amask) { srca -= 4; put_long(srca, regs.a[movem_index2[amask]]); amask = movem_next[amask]; }
        while (dmask) { srca -= 4; put_long(srca, regs.d[movem_index2[dmask]]); dmask = movem_next[dmask]; }
        regs.a[dstreg] = srca;
}}}}}
void op_48e8(ULONG opcode) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = regs.a[dstreg] + (LONG)(WORD)nextiword();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
        while (dmask) { put_long(srca, regs.d[movem_index1[dmask]]); srca += 4; dmask = movem_next[dmask]; }
        while (amask) { put_long(srca, regs.a[movem_index1[amask]]); srca += 4; amask = movem_next[amask]; }
}}}}
void op_48f0(ULONG opcode) /* MVMLE */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword();
{       CPTR srca = get_disp_ea(regs.a[dstreg]);
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
        while (dmask) { put_long(srca, regs.d[movem_index1[dmask]]); srca += 4; dmask = movem_next[dmask]; }
        while (amask) { put_long(srca, regs.a[movem_index1[amask]]); srca += 4; amask = movem_next[amask]; }
}}}}
void op_48f8(ULONG opcode) /* MVMLE */
{
{       UWORD mask = nextiword();
{       CPTR srca = (LONG)(WORD)nextiword();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
        while (dmask) { put_long(srca, regs.d[movem_index1[dmask]]); srca += 4; dmask = movem_next[dmask]; }
        while (amask) { put_long(srca, regs.a[movem_index1[amask]]); srca += 4; amask = movem_next[amask]; }
}}}}
void op_48f9(ULONG opcode) /* MVMLE */
{
{       UWORD mask = nextiword();
{       CPTR srca = nextilong();
{       UWORD dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
        while (dmask) { put_long(srca, regs.d[movem_index1[dmask]]); srca += 4; dmask = movem_next[dmask]; }
        while (amask) { put_long(srca, regs.a[movem_index1[amask]]); srca += 4; amask = movem_next[amask]; }
}}}}
void op_49c0(ULONG opcode) /* EXT */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg];
{       ULONG dst = (LONG)(BYTE)src;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(dst)) == 0;
        NFLG = ((LONG)(dst)) < 0;
        regs.d[srcreg] = (dst);
}}}}
void op_4a00(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}
void op_4a10(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}
void op_4a18(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}}
void op_4a20(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}}
void op_4a28(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}
void op_4a30(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}}
void op_4a38(ULONG opcode) /* TST */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}
void op_4a39(ULONG opcode) /* TST */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}
void op_4a3a(ULONG opcode) /* TST */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}}
void op_4a3b(ULONG opcode) /* TST */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}}
void op_4a3c(ULONG opcode) /* TST */
{
{{      BYTE src = nextiword();
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
}}}
void op_4a40(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.d[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}
void op_4a48(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      WORD src = regs.a[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}
void op_4a50(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}
void op_4a58(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
{       regs.a[srcreg] += 2;
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}}
void op_4a60(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 2;
{       CPTR srca = regs.a[srcreg];
        WORD src = get_word(srca);
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}}
void op_4a68(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}
void op_4a70(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       WORD src = get_word(srca);
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}}
void op_4a78(ULONG opcode) /* TST */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        WORD src = get_word(srca);
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}
void op_4a79(ULONG opcode) /* TST */
{
{{      CPTR srca = nextilong();
        WORD src = get_word(srca);
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}
void op_4a7a(ULONG opcode) /* TST */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       WORD src = get_word(srca);
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}}
void op_4a7b(ULONG opcode) /* TST */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       WORD src = get_word(srca);
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}}
void op_4a7c(ULONG opcode) /* TST */
{
{{      WORD src = nextiword();
        VFLG = CFLG = 0;
        ZFLG = ((WORD)(src)) == 0;
        NFLG = ((WORD)(src)) < 0;
}}}
void op_4a80(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.d[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}
void op_4a88(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.a[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}
void op_4a90(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}
void op_4a98(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
{       regs.a[srcreg] += 4;
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}}
void op_4aa0(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= 4;
{       CPTR srca = regs.a[srcreg];
        LONG src = get_long(srca);
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}}
void op_4aa8(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}
void op_4ab0(ULONG opcode) /* TST */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       LONG src = get_long(srca);
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}}
void op_4ab8(ULONG opcode) /* TST */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        LONG src = get_long(srca);
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}
void op_4ab9(ULONG opcode) /* TST */
{
{{      CPTR srca = nextilong();
        LONG src = get_long(srca);
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}
void op_4aba(ULONG opcode) /* TST */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       LONG src = get_long(srca);
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}}
void op_4abb(ULONG opcode) /* TST */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       LONG src = get_long(srca);
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}}
void op_4abc(ULONG opcode) /* TST */
{
{{      LONG src = nextilong();
        VFLG = CFLG = 0;
        ZFLG = ((LONG)(src)) == 0;
        NFLG = ((LONG)(src)) < 0;
}}}
void op_4ac0(ULONG opcode) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      BYTE src = regs.d[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
        src |= 0x80;
        regs.d[srcreg] = (regs.d[srcreg] & ~0xff) | ((src) & 0xff);
}}}
void op_4ad0(ULONG opcode) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
        src |= 0x80;
        put_byte(srca,src);
}}}
void op_4ad8(ULONG opcode) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
{       regs.a[srcreg] += areg_byteinc[srcreg];
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
        src |= 0x80;
        put_byte(srca,src);
}}}}
void op_4ae0(ULONG opcode) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      regs.a[srcreg] -= areg_byteinc[srcreg];
{       CPTR srca = regs.a[srcreg];
        BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
        src |= 0x80;
        put_byte(srca,src);
}}}}
void op_4ae8(ULONG opcode) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
        src |= 0x80;
        put_byte(srca,src);
}}}
void op_4af0(ULONG opcode) /* TAS */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
        src |= 0x80;
        put_byte(srca,src);
}}}}
void op_4af8(ULONG opcode) /* TAS */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
        src |= 0x80;
        put_byte(srca,src);
}}}
void op_4af9(ULONG opcode) /* TAS */
{
{{      CPTR srca = nextilong();
        BYTE src = get_byte(srca);
        VFLG = CFLG = 0;
        ZFLG = ((BYTE)(src)) == 0;
        NFLG = ((BYTE)(src)) < 0;
        src |= 0x80;
        put_byte(srca,src);
}}}
void op_4c90(ULONG opcode) /* MVMEL */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg];
{       while (dmask) { regs.d[movem_index1[dmask]] = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; }
}}}}
void op_4c98(ULONG opcode) /* MVMEL */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg];
{       while (dmask) { regs.d[movem_index1[dmask]] = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; }
        regs.a[dstreg] = srca;
}}}}
void op_4ca8(ULONG opcode) /* MVMEL */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg] + (LONG)(WORD)nextiword();
{       while (dmask) { regs.d[movem_index1[dmask]] = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; }
}}}}
void op_4cb0(ULONG opcode) /* MVMEL */
{
        ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = get_disp_ea(regs.a[dstreg]);
{       while (dmask) { regs.d[movem_index1[dmask]] = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; }
}}}}
void op_4cb8(ULONG opcode) /* MVMEL */
{
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = (LONG)(WORD)nextiword();
{       while (dmask) { regs.d[movem_index1[dmask]] = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; }
}}}}
void op_4cb9(ULONG opcode) /* MVMEL */
{
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = nextilong();
{       while (dmask) { regs.d[movem_index1[dmask]] = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; }
        while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; }
}}}}
void op_4cba(ULONG opcode) /* MVMEL */
{
				ULONG dstreg;
				dstreg = 2;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = m68k_getpc();
				srca += (LONG)(WORD)nextiword();
{       while (dmask) { regs.d[movem_index1[dmask]] = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; }
				while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; }
}}}}
void op_4cbb(ULONG opcode) /* MVMEL */
{
				ULONG dstreg;
				dstreg = 3;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = get_disp_ea(m68k_getpc());
{       while (dmask) { regs.d[movem_index1[dmask]] = (LONG)(WORD)get_word(srca); srca += 2; dmask = movem_next[dmask]; }
				while (amask) { regs.a[movem_index1[amask]] = (LONG)(WORD)get_word(srca); srca += 2; amask = movem_next[amask]; }
}}}}
void op_4cd0(ULONG opcode) /* MVMEL */
{
				ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg];
{       while (dmask) { regs.d[movem_index1[dmask]] = get_long(srca); srca += 4; dmask = movem_next[dmask]; }
				while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; }
}}}}
void op_4cd8(ULONG opcode) /* MVMEL */
{
				ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg];
{       while (dmask) { regs.d[movem_index1[dmask]] = get_long(srca); srca += 4; dmask = movem_next[dmask]; }
				while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; }
				regs.a[dstreg] = srca;
}}}}
void op_4ce8(ULONG opcode) /* MVMEL */
{
				ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = regs.a[dstreg] + (LONG)(WORD)nextiword();
{       while (dmask) { regs.d[movem_index1[dmask]] = get_long(srca); srca += 4; dmask = movem_next[dmask]; }
				while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; }
}}}}
void op_4cf0(ULONG opcode) /* MVMEL */
{
				ULONG dstreg = opcode & 7;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = get_disp_ea(regs.a[dstreg]);
{       while (dmask) { regs.d[movem_index1[dmask]] = get_long(srca); srca += 4; dmask = movem_next[dmask]; }
				while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; }
}}}}
void op_4cf8(ULONG opcode) /* MVMEL */
{
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = (LONG)(WORD)nextiword();
{       while (dmask) { regs.d[movem_index1[dmask]] = get_long(srca); srca += 4; dmask = movem_next[dmask]; }
				while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; }
}}}}
void op_4cf9(ULONG opcode) /* MVMEL */
{
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = nextilong();
{       while (dmask) { regs.d[movem_index1[dmask]] = get_long(srca); srca += 4; dmask = movem_next[dmask]; }
				while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; }
}}}}
void op_4cfa(ULONG opcode) /* MVMEL */
{
				ULONG dstreg;
				dstreg = 2;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = m68k_getpc();
				srca += (LONG)(WORD)nextiword();
{       while (dmask) { regs.d[movem_index1[dmask]] = get_long(srca); srca += 4; dmask = movem_next[dmask]; }
				while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; }
}}}}
void op_4cfb(ULONG opcode) /* MVMEL */
{
				ULONG dstreg;
				dstreg = 3;
{       UWORD mask = nextiword(), dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{       CPTR srca = get_disp_ea(m68k_getpc());
{       while (dmask) { regs.d[movem_index1[dmask]] = get_long(srca); srca += 4; dmask = movem_next[dmask]; }
				while (amask) { regs.a[movem_index1[amask]] = get_long(srca); srca += 4; amask = movem_next[amask]; }
}}}}
void op_4e40(ULONG opcode) /* TRAP */
{
				ULONG srcreg = (opcode & 15);
{{      ULONG src = srcreg;
				Exception(src+32,0);
}}}
void op_4e50(ULONG opcode) /* LINK */
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
void op_4e58(ULONG opcode) /* UNLK */
{
        ULONG srcreg = (opcode & 7);
{{      LONG src = regs.a[srcreg];
        regs.a[7] = src;
{       CPTR olda = regs.a[7];
        LONG old = get_long(olda);
{       regs.a[7] += 4;
        regs.a[srcreg] = (old);
}}}}}
void op_4e60(ULONG opcode) /* MVR2USP */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      LONG src = regs.a[srcreg];
        regs.usp = src;
}}}}
void op_4e68(ULONG opcode) /* MVUSP2R */
{
        ULONG srcreg = (opcode & 7);
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      regs.a[srcreg] = (regs.usp);
}}}}
void op_4e70(ULONG opcode) /* RESET */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{       
}}}
void op_4e71(ULONG opcode) /* NOP */
{
{}}
void op_4e72(ULONG opcode) /* STOP */
{
{if (!regs.s) { regs.pc -= 2; Exception(8,0); } else
{{      WORD src = nextiword();
        regs.sr = src;
        MakeFromSR();
}}}}
void op_4e73(ULONG opcode) /* RTE */
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
void op_4e74(ULONG opcode) /* RTD */
{
{{      CPTR pca = regs.a[7];
        LONG pc = get_long(pca);
{       regs.a[7] += 4;
{       WORD offs = nextiword();
        regs.a[7] += offs;
        m68k_setpc(pc);
}}}}}
void op_4e75(ULONG opcode) /* RTS */
{
{{      CPTR pca = regs.a[7];
        LONG pc = get_long(pca);
{       regs.a[7] += 4;
        m68k_setpc(pc);
}}}}
void op_4e76(ULONG opcode) /* TRAPV */
{
{       if(VFLG) Exception(7,m68k_getpc()-2);
}}
void op_4e77(ULONG opcode) /* RTR */
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
void op_4e90(ULONG opcode) /* JSR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4ea8(ULONG opcode) /* JSR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4eb0(ULONG opcode) /* JSR */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4eb8(ULONG opcode) /* JSR */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4eb9(ULONG opcode) /* JSR */
{
{{      CPTR srca = nextilong();
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4eba(ULONG opcode) /* JSR */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4ebb(ULONG opcode) /* JSR */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
{       regs.a[7] -= 4;
{       CPTR spa = regs.a[7];
        put_long(spa,m68k_getpc());
        m68k_setpc(srca);
}}}}}
void op_4ed0(ULONG opcode) /* JMP */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg];
        m68k_setpc(srca);
}}}
void op_4ee8(ULONG opcode) /* JMP */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
        m68k_setpc(srca);
}}}
void op_4ef0(ULONG opcode) /* JMP */
{
        ULONG srcreg = (opcode & 7);
{{      CPTR srca = get_disp_ea(regs.a[srcreg]);
        m68k_setpc(srca);
}}}
void op_4ef8(ULONG opcode) /* JMP */
{
{{      CPTR srca = (LONG)(WORD)nextiword();
        m68k_setpc(srca);
}}}
void op_4ef9(ULONG opcode) /* JMP */
{
{{      CPTR srca = nextilong();
        m68k_setpc(srca);
}}}
void op_4efa(ULONG opcode) /* JMP */
{
{{      CPTR srca = m68k_getpc();
        srca += (LONG)(WORD)nextiword();
        m68k_setpc(srca);
}}}
void op_4efb(ULONG opcode) /* JMP */
{
{{      CPTR srca = get_disp_ea(m68k_getpc());
        m68k_setpc(srca);
}}}
