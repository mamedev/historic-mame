#include "cpudefs.h"
void op_9000(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = regs.d[srcreg];
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}
void op_9010(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}
void op_9018(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}}
void op_9020(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}}
void op_9028(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}
void op_9030(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}}
void op_9038(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}
void op_9039(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}
void op_903a(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}}
void op_903b(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}}
void op_903c(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = nextiword();
{	BYTE dst = regs.d[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}}
void op_9040(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg];
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}
void op_9048(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}
void op_9050(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}
void op_9058(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}
void op_9060(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}
void op_9068(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}
void op_9070(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}
void op_9078(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}
void op_9079(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}
void op_907a(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}
void op_907b(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}
void op_907c(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg];
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}}
void op_9080(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg];
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}
void op_9088(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}
void op_9090(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}
void op_9098(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}}
void op_90a0(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}}
void op_90a8(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}
void op_90b0(ULONG opcode) /* SUB */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}}
void op_90b8(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}
void op_90b9(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}
void op_90ba(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}}
void op_90bb(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}}
void op_90bc(ULONG opcode) /* SUB */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	LONG dst = regs.d[dstreg];
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	regs.d[dstreg] = (newv);
}}}}}}}
void op_90c0(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg];
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90c8(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.a[srcreg];
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90d0(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90d8(ULONG opcode) /* SUBA */
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
void op_90e0(ULONG opcode) /* SUBA */
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
void op_90e8(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90f0(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_90f8(ULONG opcode) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90f9(ULONG opcode) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_90fa(ULONG opcode) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_90fb(ULONG opcode) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_90fc(ULONG opcode) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_9100(ULONG opcode) /* SUBX */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = regs.d[srcreg];
{	BYTE dst = regs.d[dstreg];
{	ULONG newv = dst - src - (regs.x ? 1 : 0);
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((BYTE)(newv)) != 0) ZFLG = 0;
	NFLG = ((BYTE)(newv)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((newv) & 0xff);
}}}}}}
void op_9108(ULONG opcode) /* SUBX */
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
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((BYTE)(newv)) != 0) ZFLG = 0;
	NFLG = ((BYTE)(newv)) < 0;
	put_byte(dsta,newv);
}}}}}}}}
void op_9110(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	put_byte(dsta,newv);
}}}}}}}
void op_9118(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	put_byte(dsta,newv);
}}}}}}}}
void op_9120(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg];
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	put_byte(dsta,newv);
}}}}}}}}
void op_9128(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	put_byte(dsta,newv);
}}}}}}}
void op_9130(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg];
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	put_byte(dsta,newv);
}}}}}}}}
void op_9138(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg];
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	put_byte(dsta,newv);
}}}}}}}
void op_9139(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg];
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	ZFLG = ((BYTE)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
	put_byte(dsta,newv);
}}}}}}}
void op_9140(ULONG opcode) /* SUBX */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg];
{	WORD dst = regs.d[dstreg];
{	ULONG newv = dst - src - (regs.x ? 1 : 0);
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((WORD)(newv)) != 0) ZFLG = 0;
	NFLG = ((WORD)(newv)) < 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((newv) & 0xffff);
}}}}}}
void op_9148(ULONG opcode) /* SUBX */
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
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((WORD)(newv)) != 0) ZFLG = 0;
	NFLG = ((WORD)(newv)) < 0;
	put_word(dsta,newv);
}}}}}}}}
void op_9150(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	put_word(dsta,newv);
}}}}}}}
void op_9158(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{	regs.a[dstreg] += 2;
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	put_word(dsta,newv);
}}}}}}}}
void op_9160(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg];
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	put_word(dsta,newv);
}}}}}}}}
void op_9168(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	put_word(dsta,newv);
}}}}}}}
void op_9170(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	put_word(dsta,newv);
}}}}}}}}
void op_9178(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	put_word(dsta,newv);
}}}}}}}
void op_9179(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg];
{	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) - ((WORD)(src));
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	ZFLG = ((WORD)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(src)) > ((UWORD)(dst));
	NFLG = flgn != 0;
	put_word(dsta,newv);
}}}}}}}
void op_9180(ULONG opcode) /* SUBX */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg];
{	LONG dst = regs.d[dstreg];
{	ULONG newv = dst - src - (regs.x ? 1 : 0);
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((LONG)(newv)) != 0) ZFLG = 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg] = (newv);
}}}}}}
void op_9188(ULONG opcode) /* SUBX */
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
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (!flgs && flgo && !flgn) || (flgs && !flgo && flgn);
	regs.x = CFLG = (flgs && !flgo) || (flgn && (!flgo || flgs));
	if (((LONG)(newv)) != 0) ZFLG = 0;
	NFLG = ((LONG)(newv)) < 0;
	put_long(dsta,newv);
}}}}}}}}
void op_9190(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	put_long(dsta,newv);
}}}}}}}
void op_9198(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{	regs.a[dstreg] += 4;
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	put_long(dsta,newv);
}}}}}}}}
void op_91a0(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg];
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	put_long(dsta,newv);
}}}}}}}}
void op_91a8(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	put_long(dsta,newv);
}}}}}}}
void op_91b0(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	put_long(dsta,newv);
}}}}}}}}
void op_91b8(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	put_long(dsta,newv);
}}}}}}}
void op_91b9(ULONG opcode) /* SUB */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg];
{	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) - ((LONG)(src));
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	ZFLG = ((LONG)(newv)) == 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(src)) > ((ULONG)(dst));
	NFLG = flgn != 0;
	put_long(dsta,newv);
}}}}}}}
void op_91c0(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg];
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91c8(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.a[srcreg];
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91d0(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91d8(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_91e0(ULONG opcode) /* SUBA */
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
void op_91e8(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91f0(ULONG opcode) /* SUBA */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_91f8(ULONG opcode) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91f9(ULONG opcode) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_91fa(ULONG opcode) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_91fb(ULONG opcode) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}}
void op_91fc(ULONG opcode) /* SUBA */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
