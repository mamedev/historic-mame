#include "cpudefs.h"
void op_9000(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9010(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9018(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9020(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9028(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9030(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9038(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9039(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_903a(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_903b(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_903c(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = nextiword();
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9040(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9048(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9050(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9058(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9060(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9068(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9070(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9078(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9079(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_907a(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_907b(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_907c(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9080(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9088(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9090(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9098(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_90a0(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_90a8(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_90b0(void) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_90b8(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_90b9(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_90ba(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_90bb(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_90bc(void) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_90c0(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90c8(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90d0(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90d8(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_90e0(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_90e8(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90f0(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_90f8(void) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90f9(void) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90fa(void) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_90fb(void) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_90fc(void) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_9100(void) /* SUBX */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	BYTE dst = regs.d[dstreg].B.l;
{	ULONG newv = dst - src - (regs.x ? 1 : 0);
	regs.d[dstreg].B.l = newv;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((BYTE)(newv)) != 0) ZFLG = 0;
	NFLG = ((BYTE)(newv)) < 0;
}}}}}}
void op_9108(void) /* SUBX */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	ULONG newv = dst - src - (regs.x ? 1 : 0);
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((BYTE)(newv)) != 0) ZFLG = 0;
	NFLG = ((BYTE)(newv)) < 0;
}}}}}}}}
void op_9110(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9118(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9120(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9128(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9130(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9138(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9139(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9140(void) /* SUBX */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = dst - src - (regs.x ? 1 : 0);
	regs.d[dstreg].W.l = newv;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((WORD)(newv)) != 0) ZFLG = 0;
	NFLG = ((WORD)(newv)) < 0;
}}}}}}
void op_9148(void) /* SUBX */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{	ULONG newv = dst - src - (regs.x ? 1 : 0);
	put_word(dsta,newv);
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((WORD)(newv)) != 0) ZFLG = 0;
	NFLG = ((WORD)(newv)) < 0;
}}}}}}}}
void op_9150(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9158(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{	regs.a[dstreg] += 2;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9160(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9168(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9170(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_9178(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9179(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9180(void) /* SUBX */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.d[dstreg].D;
{	ULONG newv = dst - src - (regs.x ? 1 : 0);
	regs.d[dstreg].D = (newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((LONG)(newv)) != 0) ZFLG = 0;
	NFLG = ((LONG)(newv)) < 0;
}}}}}}
void op_9188(void) /* SUBX */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{	ULONG newv = dst - src - (regs.x ? 1 : 0);
	put_long(dsta,newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((LONG)(newv)) != 0) ZFLG = 0;
	NFLG = ((LONG)(newv)) < 0;
}}}}}}}}
void op_9190(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_9198(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{	regs.a[dstreg] += 4;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_91a0(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_91a8(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_91b0(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_91b8(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_91b9(void) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_91c0(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91c8(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91d0(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91d8(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
	CPTR srca = regs.a[srcreg];
	regs.a[srcreg] += 4;
{	LONG src = get_long(srca);
	LONG dst = regs.a[dstreg];
	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}

void op_91e0(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_91e8(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91f0(void) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_91f8(void) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91f9(void) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91fa(void) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_91fb(void) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_91fc(void) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
