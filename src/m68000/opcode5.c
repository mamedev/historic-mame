#include "cpudefs.h"
void op_5000(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	BYTE dst = regs.d[dstreg].B.l;
{{ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	regs.d[dstreg].B.l = newv;
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5010(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5018(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
{{ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
	NFLG = flgn != 0;
}}}}}}}}
void op_5020(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
	NFLG = flgn != 0;
}}}}}}}}
void op_5028(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5030(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
	NFLG = flgn != 0;
}}}}}}}}
void op_5038(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5039(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) + ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(~dst)) < ((UBYTE)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5040(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	WORD dst = regs.d[dstreg].W.l;
{{ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	regs.d[dstreg].W.l = newv;
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5048(void) /* ADDA */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst + src;
	regs.a[dstreg] = (newv);
}}}}}
void op_5050(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5058(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{	regs.a[dstreg] += 2;
{{ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
	NFLG = flgn != 0;
}}}}}}}}
void op_5060(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
	NFLG = flgn != 0;
}}}}}}}}
void op_5068(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5070(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
	NFLG = flgn != 0;
}}}}}}}}
void op_5078(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
{	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5079(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
{	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
{{ULONG newv = ((WORD)(dst)) + ((WORD)(src));
	put_word(dsta,newv);
	ZFLG = ((WORD)(newv)) == 0;
{	int flgs = ((WORD)(src)) < 0;
	int flgo = ((WORD)(dst)) < 0;
	int flgn = ((WORD)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((UWORD)(~dst)) < ((UWORD)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5080(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	LONG dst = regs.d[dstreg].D;
{{ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	regs.d[dstreg].D = (newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5088(void) /* ADDA */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst + src;
	regs.a[dstreg] = (newv);
}}}}}
void op_5090(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_5098(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{	regs.a[dstreg] += 4;
{{ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
	NFLG = flgn != 0;
}}}}}}}}
void op_50a0(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
	NFLG = flgn != 0;
}}}}}}}}
void op_50a8(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_50b0(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
	NFLG = flgn != 0;
}}}}}}}}
void op_50b8(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
{	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
	NFLG = flgn != 0;
}}}}}}}
void op_50b9(void) /* ADD */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
{	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
{{ULONG newv = ((LONG)(dst)) + ((LONG)(src));
	put_long(dsta,newv);
	ZFLG = ((LONG)(newv)) == 0;
{	int flgs = ((LONG)(src)) < 0;
	int flgo = ((LONG)(dst)) < 0;
	int flgn = ((LONG)(newv)) < 0;
	VFLG = (flgs == flgo) && (flgn != flgo);
	CFLG = regs.x = ((ULONG)(~dst)) < ((ULONG)(src));
	NFLG = flgn != 0;
}}}}}}}

void op_50c0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
	int val = cctrue(0) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}

void op_50c8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(0))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}
void op_50d0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	int val = cctrue(0) ? 0xff : 0;
	put_byte(srca,val);
}

void op_50d8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(0) ? 0xff : 0;
	put_byte(srca,val);
}}

void op_50e0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	int val = cctrue(0) ? 0xff : 0;
	put_byte(srca,val);
}}

void op_50e8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(0) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_50f0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(0) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_50f8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(0) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_50f9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(0) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5100(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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

void op_5110(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_5118(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_5120(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_5128(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_5130(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}}
void op_5138(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_5139(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
{{ULONG newv = ((BYTE)(dst)) - ((BYTE)(src));
	put_byte(dsta,newv);
	ZFLG = ((BYTE)(newv)) == 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	CFLG = regs.x = ((UBYTE)(src)) > ((UBYTE)(dst));
	NFLG = flgn != 0;
}}}}}}}
void op_5140(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_5148(void) /* SUBA */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_5150(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_5158(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_5160(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_5168(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_5170(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_5178(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
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
void op_5179(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
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
void op_5180(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_5188(void) /* SUBA */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	LONG dst = regs.a[dstreg];
{	ULONG newv = dst - src;
	regs.a[dstreg] = (newv);
}}}}}
void op_5190(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_5198(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{	regs.a[dstreg] += 4;
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
void op_51a0(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_51a8(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_51b0(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG src = srcreg;
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
void op_51b8(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
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
void op_51b9(void) /* SUB */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
{{	ULONG src = srcreg;
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
void op_51c0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(1) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}

void op_51c8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(1))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_51d0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(1) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_51d8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(1) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_51e0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(1) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_51e8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(1) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_51f0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(1) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_51f8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(1) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_51f9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(1) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_52c0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(2) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}
void op_52c8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(2))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_52d0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	int val = cctrue(2) ? 0xff : 0;
	put_byte(srca,val);
}

void op_52d8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(2) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_52e0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(2) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_52e8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(2) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_52f0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(2) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_52f8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(2) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_52f9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(2) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_53c0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(3) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}

void op_53c8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(3))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_53d0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(3) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_53d8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(3) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_53e0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(3) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_53e8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(3) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_53f0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(3) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_53f8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(3) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_53f9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(3) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_54c0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(4) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}
void op_54c8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(4))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_54d0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(4) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_54d8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(4) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_54e0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(4) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_54e8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(4) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_54f0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(4) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_54f8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(4) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_54f9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(4) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_55c0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(5) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}
void op_55c8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(5))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_55d0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(5) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_55d8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(5) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_55e0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(5) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_55e8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(5) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_55f0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(5) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_55f8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(5) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_55f9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(5) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_56c0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(6) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}
void op_56c8(void) /* DBcc */
{
	ULONG srcreg = (opcode & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	WORD offs = nextiword();
	if (!cctrue(6)) {
	if (src--) { regs.pc += (LONG)offs - 2; /*change_pc24(regs.pc&0xffffff);*/ }	/* ASG 971108 */
	regs.d[srcreg].W.l = src;
	}
}}}}
void op_56d0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(6) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_56d8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(6) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_56e0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(6) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_56e8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(6) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_56f0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(6) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_56f8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(6) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_56f9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(6) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_57c0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(7) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}
void op_57c8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(7))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_57d0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
	CPTR srca = regs.a[srcreg];
	int val = cctrue(7) ? 0xff : 0;
	put_byte(srca,val);
}

void op_57d8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(7) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_57e0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(7) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_57e8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(7) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_57f0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(7) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_57f8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(7) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_57f9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(7) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_58c0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(8) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}

void op_58c8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(8))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_58d0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(8) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_58d8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(8) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_58e0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(8) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_58e8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(8) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_58f0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(8) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_58f8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(8) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_58f9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(8) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_59c0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(9) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}
void op_59c8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(9))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_59d0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(9) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_59d8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(9) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_59e0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(9) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_59e8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(9) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_59f0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(9) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_59f8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(9) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_59f9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(9) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5ac0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(10) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}
void op_5ac8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(10))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_5ad0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(10) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5ad8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(10) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5ae0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(10) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5ae8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(10) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5af0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(10) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5af8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(10) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5af9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(10) ? 0xff : 0;
	put_byte(srca,val);
}}}}

void op_5bc0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
	int val = cctrue(11) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}

void op_5bc8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(11))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_5bd0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(11) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5bd8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(11) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5be0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(11) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5be8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(11) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5bf0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(11) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5bf8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(11) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5bf9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(11) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5cc0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(12) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}
void op_5cc8(void) /* DBcc */
{
	WORD offs = nextiword();
	if (!cctrue(12))
		{
		ULONG srcreg = (opcode & 7);
		WORD src = regs.d[srcreg].W.l;
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_5cd0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(12) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5cd8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(12) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5ce0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(12) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5ce8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(12) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5cf0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(12) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5cf8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(12) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5cf9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(12) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5dc0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(13) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}

void op_5dc8(void) /* DBcc */
{
	ULONG srcreg = (opcode & 7);
	if (!cctrue(13))
		{
		WORD src = regs.d[srcreg].W.l;
		WORD offs = nextiword();
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_5dd0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(13) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5dd8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(13) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5de0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(13) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5de8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(13) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5df0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(13) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5df8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(13) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5df9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(13) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5ec0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(14) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}
void op_5ec8(void) /* DBcc */
{
	ULONG srcreg = (opcode & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	WORD offs = nextiword();
	if (!cctrue(14)) {
	if (src--) { regs.pc += (LONG)offs - 2; /*change_pc24(regs.pc&0xffffff);*/ }	/* ASG 971108 */
	regs.d[srcreg].W.l = src;
	}
}}}}
void op_5ed0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(14) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5ed8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(14) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5ee0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(14) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5ee8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(14) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5ef0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(14) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5ef8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(14) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5ef9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(14) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5fc0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{{	int val = cctrue(15) ? 0xff : 0;
	regs.d[srcreg].B.l = val;
}}}}
void op_5fc8(void) /* DBcc */
{
	ULONG srcreg = (opcode & 7);
	if (!cctrue(15))
		{
		WORD src = regs.d[srcreg].W.l;
		WORD offs = nextiword();
		if (src--)
			{
			regs.pc += (LONG)offs - 2;
			}
		regs.d[srcreg].W.l = src;
		}
}

void op_5fd0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(15) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5fd8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg];
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	int val = cctrue(15) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5fe0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
{	int val = cctrue(15) ? 0xff : 0;
	put_byte(srca,val);
}}}}}
void op_5fe8(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
{	int val = cctrue(15) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5ff0(void) /* Scc */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	int val = cctrue(15) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5ff8(void) /* Scc */
{
{{	CPTR srca = (LONG)(WORD)nextiword();
{	int val = cctrue(15) ? 0xff : 0;
	put_byte(srca,val);
}}}}
void op_5ff9(void) /* Scc */
{
{{	CPTR srca = nextilong();
{	int val = cctrue(15) ? 0xff : 0;
	put_byte(srca,val);
}}}}
