#include "cpudefs.h"
void op_e000(void) /* ASR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	ULONG sign = cmask & val;
	cnt &= 63;
	CLEARFLGS;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 8) {
		val = sign ? 0xff : 0;
		CFLG=regs.x= sign ? 1 : 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
		if (sign) val |= 0xff << (8 - cnt);
	}}
	regs.d[dstreg].B.l = val;
	NFLG = sign != 0;
	ZFLG = val == 0;
}}}}}
void op_e008(void) /* LSR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 8) {
		CFLG=regs.x = cnt == 8 ? (val & cmask ? 1 : 0) : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
	}}
	regs.d[dstreg].B.l = val;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
}}}}}
void op_e010(void) /* ROXR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&1; val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry;
	}
	regs.d[dstreg].B.l = val;
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
}}}}}
void op_e018(void) /* ROR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {	ULONG carry;
	for(;cnt;--cnt){
	carry=val&1; val = val >> 1;
	if(carry) val |= cmask;
	}
	CFLG = carry;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].B.l = val;
}}}}}
void op_e020(void) /* ASR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg].D;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	ULONG sign = cmask & val;
	cnt &= 63;
	VFLG = 0;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 8) {
		val = sign ? 0xff : 0;
		CFLG=regs.x= sign ? 1 : 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
		if (sign) val |= 0xff << (8 - cnt);
	}}
	regs.d[dstreg].B.l = val;
	NFLG = sign != 0;
	ZFLG = val == 0;
}}}}}
void op_e028(void) /* LSR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg].D;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 8) {
		CFLG=regs.x = cnt == 8 ? (val & cmask ? 1 : 0) : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
	}}
	regs.d[dstreg].B.l = val;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
}}}}}
void op_e030(void) /* ROXR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg].D;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&1; val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry;
	}
	regs.d[dstreg].B.l = val;
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
}}}}}
void op_e038(void) /* ROR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg].D;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {	ULONG carry;
	for(;cnt;--cnt){
	carry=val&1; val = val >> 1;
	if(carry) val |= cmask;
	}
	CFLG = carry;
}
	regs.d[dstreg].B.l = val;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
}}}}}
void op_e040(void) /* ASR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	cnt &= 63;
	CLEARFLGS;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 16) {
		val = sign ? 0xffff : 0;
		CFLG=regs.x= sign ? 1 : 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
		if (sign) val |= 0xffff << (16 - cnt);
	}}
	regs.d[dstreg].W.l = val;
	NFLG = sign != 0;
	ZFLG = val == 0;
}}}}}
void op_e048(void) /* LSR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	CLEARFLGS;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 16) {
		CFLG=regs.x = cnt == 16 ? (val & cmask ? 1 : 0) : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
	}}
	regs.d[dstreg].W.l = val;
	NFLG = (val & cmask) != 0; ZFLG = val == 0;
}}}}}

void op_e050(void) /* ROXR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&1; val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e058(void) /* ROR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {	ULONG carry;
	for(;cnt;--cnt){
	carry=val&1; val = val >> 1;
	if(carry) val |= cmask;
	}
	CFLG = carry;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e060(void) /* ASR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg].D;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	cnt &= 63;
	VFLG = 0;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 16) {
		val = sign ? 0xffff : 0;
		CFLG=regs.x= sign ? 1 : 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
		if (sign) val |= 0xffff << (16 - cnt);
	}}
	NFLG = sign != 0;
	ZFLG = val == 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e068(void) /* LSR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg].D;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 16) {
		CFLG=regs.x = cnt == 16 ? (val & cmask ? 1 : 0) : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
	}}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e070(void) /* ROXR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg].D;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&1; val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e078(void) /* ROR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg].D;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {	ULONG carry;
	for(;cnt;--cnt){
	carry=val&1; val = val >> 1;
	if(carry) val |= cmask;
	}
	CFLG = carry;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e080(void) /* ASR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	ULONG sign = cmask & val;
	cnt &= 63;
	VFLG = 0;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 32) {
		val = sign ? 0xffffffff : 0;
		CFLG=regs.x= sign ? 1 : 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
		if (sign) val |= 0xffffffff << (32 - cnt);
	}}
	NFLG = sign != 0;
	ZFLG = val == 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e088(void) /* LSR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 32) {
		CFLG=regs.x = cnt == 32 ? (val & cmask ? 1 : 0) : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
	}}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e090(void) /* ROXR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&1; val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e098(void) /* ROR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {	ULONG carry;
	for(;cnt;--cnt){
	carry=val&1; val = val >> 1;
	if(carry) val |= cmask;
	}
	CFLG = carry;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e0a0(void) /* ASR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg].D;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	ULONG sign = cmask & val;
	cnt &= 63;
	VFLG = 0;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 32) {
		val = sign ? 0xffffffff : 0;
		CFLG=regs.x= sign ? 1 : 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
		if (sign) val |= 0xffffffff << (32 - cnt);
	}}
	NFLG = sign != 0;
	ZFLG = val == 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e0a8(void) /* LSR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg].D;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 32) {
		CFLG=regs.x = cnt == 32 ? (val & cmask ? 1 : 0) : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val >> (cnt-1)) & 1;
		val >>= cnt;
	}}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e0b0(void) /* ROXR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg].D;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&1; val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e0b8(void) /* ROR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg].D;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {	ULONG carry;
	for(;cnt;--cnt){
	carry=val&1; val = val >> 1;
	if(carry) val |= cmask;
	}
	CFLG = carry;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e0d0(void) /* ASRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=val&1; val = (val >> 1) | sign;
	NFLG = sign != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}
void op_e0d8(void) /* ASRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	regs.a[srcreg] += 2;
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=val&1; val = (val >> 1) | sign;
	NFLG = sign != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}}
void op_e0e0(void) /* ASRW */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=val&1; val = (val >> 1) | sign;
	NFLG = sign != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}}
void op_e0e8(void) /* ASRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=val&1; val = (val >> 1) | sign;
	NFLG = sign != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}
void op_e0f0(void) /* ASRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = get_disp_ea(regs.a[srcreg]);
{	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=val&1; val = (val >> 1) | sign;
	NFLG = sign != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}}
void op_e0f8(void) /* ASRW */
{
{{	CPTR dataa = (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=val&1; val = (val >> 1) | sign;
	NFLG = sign != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}
void op_e0f9(void) /* ASRW */
{
{{	CPTR dataa = nextilong();
	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=val&1; val = (val >> 1) | sign;
	NFLG = sign != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}
void op_e100(void) /* ASL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	cnt &= 63;
	VFLG = 0;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 8) {
		VFLG = val != 0;
		CFLG=regs.x = cnt == 8 ? val & 1 : 0;
		val = 0;
	} else {
		ULONG mask = aslmask_ubyte[cnt];
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg].B.l = val;
}}}}}
void op_e108(void) /* LSL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 8) {
		CFLG=regs.x = cnt == 8 ? val & 1 : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		val <<= cnt;
	}}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].B.l = val;
}}}}}
void op_e110(void) /* ROXL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].B.l = val;
}}}}}
void op_e118(void) /* ROL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	ULONG carry;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(carry) val |= 1;
	}
	CFLG = carry!=0;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].B.l = val;
}}}}}
void op_e120(void) /* ASL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg].D;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	cnt &= 63;
	VFLG = 0;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 8) {
		VFLG = val != 0;
		CFLG=regs.x = cnt == 8 ? val & 1 : 0;
		val = 0;
	} else {
		ULONG mask = aslmask_ubyte[cnt];
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg].B.l = val;
}}}}}
void op_e128(void) /* LSL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg].D;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 8) {
		CFLG=regs.x = cnt == 8 ? val & 1 : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		val <<= cnt;
	}}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].B.l = val;
}}}}}
void op_e130(void) /* ROXL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg].D;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].B.l = val;
}}}}}
void op_e138(void) /* ROL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg].D;
{	BYTE data = regs.d[dstreg].D;
{	UBYTE val = data;
	ULONG cmask = 0x80;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	ULONG carry;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(carry) val |= 1;
	}
	CFLG = carry!=0;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].B.l = val;
}}}}}
void op_e140(void) /* ASL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	cnt &= 63;
	VFLG = 0;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 16) {
		VFLG = val != 0;
		CFLG=regs.x = cnt == 16 ? val & 1 : 0;
		val = 0;
	} else {
		ULONG mask = aslmask_uword[cnt];
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e148(void) /* LSL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 16) {
		CFLG=regs.x = cnt == 16 ? val & 1 : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		val <<= cnt;
	}}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e150(void) /* ROXL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e158(void) /* ROL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	ULONG carry;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(carry) val |= 1;
	}
	CFLG = carry!=0;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e160(void) /* ASL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg].D;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	cnt &= 63;
	VFLG = 0;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 16) {
		VFLG = val != 0;
		CFLG=regs.x = cnt == 16 ? val & 1 : 0;
		val = 0;
	} else {
		ULONG mask = aslmask_uword[cnt];
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e168(void) /* LSL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg].D;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 16) {
		CFLG=regs.x = cnt == 16 ? val & 1 : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		val <<= cnt;
	}}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e170(void) /* ROXL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg].D;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e178(void) /* ROL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg].D;
{	WORD data = regs.d[dstreg].D;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	ULONG carry;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(carry) val |= 1;
	}
	CFLG = carry!=0;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].W.l = val;
}}}}}
void op_e180(void) /* ASL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	cnt &= 63;
	VFLG = 0;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 32) {
		VFLG = val != 0;
		CFLG=regs.x = cnt == 32 ? val & 1 : 0;
		val = 0;
	} else {
		ULONG mask = aslmask_ulong[cnt];
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e188(void) /* LSL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 32) {
		CFLG=regs.x = cnt == 32 ? val & 1 : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		val <<= cnt;
	}}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e190(void) /* ROXL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e198(void) /* ROL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	ULONG carry;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(carry) val |= 1;
	}
	CFLG = carry!=0;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e1a0(void) /* ASL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg].D;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	cnt &= 63;
	VFLG = 0;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 32) {
		VFLG = val != 0;
		CFLG=regs.x = cnt == 32 ? val & 1 : 0;
		val = 0;
	} else {
		ULONG mask = aslmask_ulong[cnt];
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e1a8(void) /* LSL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg].D;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	if (cnt >= 32) {
		CFLG=regs.x = cnt == 32 ? val & 1 : 0;
		val = 0;
	} else {
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		val <<= cnt;
	}}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e1b0(void) /* ROXL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg].D;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e1b8(void) /* ROL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg].D;
{	LONG data = regs.d[dstreg].D;
{	ULONG val = data;
	ULONG cmask = 0x80000000;
	cnt &= 63;
	if (!cnt) { CFLG = 0; } else {
	ULONG carry;
	for(;cnt;--cnt){
	carry=val&cmask; val <<= 1;
	if(carry) val |= 1;
	}
	CFLG = carry!=0;
}
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg].D = (val);
}}}}}
void op_e1d0(void) /* ASLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=(val&cmask)!=0; val <<= 1;
	if ((val&cmask)!=sign) VFLG=1;
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}
void op_e1d8(void) /* ASLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	regs.a[srcreg] += 2;
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=(val&cmask)!=0; val <<= 1;
	if ((val&cmask)!=sign) VFLG=1;
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}}
void op_e1e0(void) /* ASLW */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=(val&cmask)!=0; val <<= 1;
	if ((val&cmask)!=sign) VFLG=1;
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}}
void op_e1e8(void) /* ASLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=(val&cmask)!=0; val <<= 1;
	if ((val&cmask)!=sign) VFLG=1;
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}
void op_e1f0(void) /* ASLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = get_disp_ea(regs.a[srcreg]);
{	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=(val&cmask)!=0; val <<= 1;
	if ((val&cmask)!=sign) VFLG=1;
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}}
void op_e1f8(void) /* ASLW */
{
{{	CPTR dataa = (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=(val&cmask)!=0; val <<= 1;
	if ((val&cmask)!=sign) VFLG=1;
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}
void op_e1f9(void) /* ASLW */
{
{{	CPTR dataa = nextilong();
	WORD data = get_word(dataa);
	VFLG = 0;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG sign = cmask & val;
	CFLG=regs.x=(val&cmask)!=0; val <<= 1;
	if ((val&cmask)!=sign) VFLG=1;
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	put_word(dataa,val);
}}}}
void op_e2d0(void) /* LSRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	val >>= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}
void op_e2d8(void) /* LSRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	regs.a[srcreg] += 2;
{	UWORD val = data;
	ULONG carry = val&1;
	val >>= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e2e0(void) /* LSRW */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	val >>= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e2e8(void) /* LSRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	val >>= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}
void op_e2f0(void) /* LSRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = get_disp_ea(regs.a[srcreg]);
{	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	val >>= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e2f8(void) /* LSRW */
{
{{	CPTR dataa = (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	val >>= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}
void op_e2f9(void) /* LSRW */
{
{{	CPTR dataa = nextilong();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	val >>= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}
void op_e3d0(void) /* LSLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}
void op_e3d8(void) /* LSLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	regs.a[srcreg] += 2;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e3e0(void) /* LSLW */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e3e8(void) /* LSLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}
void op_e3f0(void) /* LSLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = get_disp_ea(regs.a[srcreg]);
{	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e3f8(void) /* LSLW */
{
{{	CPTR dataa = (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}
void op_e3f9(void) /* LSLW */
{
{{	CPTR dataa = nextilong();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = regs.x = carry!=0;
	put_word(dataa,val);
}}}}
void op_e4d0(void) /* ROXRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e4d8(void) /* ROXRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	regs.a[srcreg] += 2;
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e4e0(void) /* ROXRW */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e4e8(void) /* ROXRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e4f0(void) /* ROXRW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = get_disp_ea(regs.a[srcreg]);
{	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e4f8(void) /* ROXRW */
{
{{	CPTR dataa = (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e4f9(void) /* ROXRW */
{
{{	CPTR dataa = nextilong();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e5d0(void) /* ROXLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e5d8(void) /* ROXLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	regs.a[srcreg] += 2;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e5e0(void) /* ROXLW */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e5e8(void) /* ROXLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e5f0(void) /* ROXLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = get_disp_ea(regs.a[srcreg]);
{	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e5f8(void) /* ROXLW */
{
{{	CPTR dataa = (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e5f9(void) /* ROXLW */
{
{{	CPTR dataa = nextilong();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(regs.x) val |= 1;
	regs.x = carry != 0;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
regs.x = CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e6d0(void) /* RORW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(carry) val |= cmask;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e6d8(void) /* RORW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	regs.a[srcreg] += 2;
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(carry) val |= cmask;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e6e0(void) /* RORW */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(carry) val |= cmask;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e6e8(void) /* RORW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(carry) val |= cmask;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e6f0(void) /* RORW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = get_disp_ea(regs.a[srcreg]);
{	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(carry) val |= cmask;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e6f8(void) /* RORW */
{
{{	CPTR dataa = (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(carry) val |= cmask;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e6f9(void) /* RORW */
{
{{	CPTR dataa = nextilong();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG carry = val&1;
	ULONG cmask = 0x8000;
	val >>= 1;
	if(carry) val |= cmask;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e7d0(void) /* ROLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(carry)  val |= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e7d8(void) /* ROLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	regs.a[srcreg] += 2;
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(carry)  val |= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e7e0(void) /* ROLW */
{
	ULONG srcreg = (opcode & 7);
{{	regs.a[srcreg] -= 2;
{	CPTR dataa = regs.a[srcreg];
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(carry)  val |= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e7e8(void) /* ROLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(carry)  val |= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e7f0(void) /* ROLW */
{
	ULONG srcreg = (opcode & 7);
{{	CPTR dataa = get_disp_ea(regs.a[srcreg]);
{	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(carry)  val |= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}}
void op_e7f8(void) /* ROLW */
{
{{	CPTR dataa = (LONG)(WORD)nextiword();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(carry)  val |= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}
void op_e7f9(void) /* ROLW */
{
{{	CPTR dataa = nextilong();
	WORD data = get_word(dataa);
{	UWORD val = data;
	ULONG cmask = 0x8000;
	ULONG carry = val&cmask;
	val <<= 1;
	if(carry)  val |= 1;
	CLEARVC;
	ZFLG = ((WORD)(val)) == 0;
	NFLG = ((WORD)(val)) < 0;
CFLG = carry!=0;
	put_word(dataa,val);
}}}}
