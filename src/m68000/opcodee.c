#include "cpudefs.h"
void op_e000(ULONG opcode) /* ASR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg];
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
	NFLG = sign != 0;
	ZFLG = val == 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e008(ULONG opcode) /* LSR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg];
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
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e010(ULONG opcode) /* ROXR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg];
{	UBYTE val = data;
	ULONG cmask = 0x80;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&1; val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e018(ULONG opcode) /* ROR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e020(ULONG opcode) /* ASR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg];
{	BYTE data = regs.d[dstreg];
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
	NFLG = sign != 0;
	ZFLG = val == 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e028(ULONG opcode) /* LSR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg];
{	BYTE data = regs.d[dstreg];
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
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e030(ULONG opcode) /* ROXR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg];
{	BYTE data = regs.d[dstreg];
{	UBYTE val = data;
	ULONG cmask = 0x80;
	ULONG carry;
	cnt &= 63;
	for(;cnt;--cnt){
	carry=val&1; val >>= 1;
	if(regs.x) val |= cmask;
	regs.x = carry;
	}
	CFLG = regs.x;
	NFLG = (val & cmask) != 0; ZFLG = val == 0; VFLG = 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e038(ULONG opcode) /* ROR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg];
{	BYTE data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e040(ULONG opcode) /* ASR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e048(ULONG opcode) /* LSR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e050(ULONG opcode) /* ROXR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e058(ULONG opcode) /* ROR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e060(ULONG opcode) /* ASR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg];
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e068(ULONG opcode) /* LSR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg];
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e070(ULONG opcode) /* ROXR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg];
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e078(ULONG opcode) /* ROR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg];
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e080(ULONG opcode) /* ASR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e088(ULONG opcode) /* LSR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e090(ULONG opcode) /* ROXR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e098(ULONG opcode) /* ROR */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e0a0(ULONG opcode) /* ASR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg];
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e0a8(ULONG opcode) /* LSR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg];
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e0b0(ULONG opcode) /* ROXR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg];
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e0b8(ULONG opcode) /* ROR */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg];
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e0d0(ULONG opcode) /* ASRW */
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
void op_e0d8(ULONG opcode) /* ASRW */
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
void op_e0e0(ULONG opcode) /* ASRW */
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
void op_e0e8(ULONG opcode) /* ASRW */
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
void op_e0f0(ULONG opcode) /* ASRW */
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
void op_e0f8(ULONG opcode) /* ASRW */
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
void op_e0f9(ULONG opcode) /* ASRW */
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
void op_e100(ULONG opcode) /* ASL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg];
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
		ULONG mask = (0xff << (7 - cnt)) & 0xff;
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e108(ULONG opcode) /* LSL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e110(ULONG opcode) /* ROXL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e118(ULONG opcode) /* ROL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	BYTE data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e120(ULONG opcode) /* ASL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg];
{	BYTE data = regs.d[dstreg];
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
		ULONG mask = (0xff << (7 - cnt)) & 0xff;
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e128(ULONG opcode) /* LSL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg];
{	BYTE data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e130(ULONG opcode) /* ROXL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg];
{	BYTE data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e138(ULONG opcode) /* ROL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE cnt = regs.d[srcreg];
{	BYTE data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xff) | ((val) & 0xff);
}}}}}
void op_e140(ULONG opcode) /* ASL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg];
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
		ULONG mask = (0xffff << (15 - cnt)) & 0xffff;
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e148(ULONG opcode) /* LSL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e150(ULONG opcode) /* ROXL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e158(ULONG opcode) /* ROL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e160(ULONG opcode) /* ASL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg];
{	WORD data = regs.d[dstreg];
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
		ULONG mask = (0xffff << (15 - cnt)) & 0xffff;
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e168(ULONG opcode) /* LSL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg];
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e170(ULONG opcode) /* ROXL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg];
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e178(ULONG opcode) /* ROL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD cnt = regs.d[srcreg];
{	WORD data = regs.d[dstreg];
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
	regs.d[dstreg] = (regs.d[dstreg] & ~0xffff) | ((val) & 0xffff);
}}}}}
void op_e180(ULONG opcode) /* ASL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg];
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
		ULONG mask = (0xffffffff << (31 - cnt)) & 0xffffffff;
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg] = (val);
}}}}}
void op_e188(ULONG opcode) /* LSL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e190(ULONG opcode) /* ROXL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e198(ULONG opcode) /* ROL */
{
	ULONG srcreg = imm8_table[((opcode >> 9) & 7)];
	ULONG dstreg = opcode & 7;
{{	ULONG cnt = srcreg;
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e1a0(ULONG opcode) /* ASL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg];
{	LONG data = regs.d[dstreg];
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
		ULONG mask = (0xffffffff << (31 - cnt)) & 0xffffffff;
		CFLG=regs.x=(val << (cnt-1)) & cmask ? 1 : 0;
		VFLG = (val & mask) != mask && (val & mask) != 0;
		val <<= cnt;
	}}
	NFLG = (val&cmask) != 0;
	ZFLG = val == 0;
	regs.d[dstreg] = (val);
}}}}}
void op_e1a8(ULONG opcode) /* LSL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg];
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e1b0(ULONG opcode) /* ROXL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg];
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e1b8(ULONG opcode) /* ROL */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG cnt = regs.d[srcreg];
{	LONG data = regs.d[dstreg];
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
	regs.d[dstreg] = (val);
}}}}}
void op_e1d0(ULONG opcode) /* ASLW */
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
void op_e1d8(ULONG opcode) /* ASLW */
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
void op_e1e0(ULONG opcode) /* ASLW */
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
void op_e1e8(ULONG opcode) /* ASLW */
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
void op_e1f0(ULONG opcode) /* ASLW */
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
void op_e1f8(ULONG opcode) /* ASLW */
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
void op_e1f9(ULONG opcode) /* ASLW */
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
void op_e2d0(ULONG opcode) /* LSRW */
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
void op_e2d8(ULONG opcode) /* LSRW */
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
void op_e2e0(ULONG opcode) /* LSRW */
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
void op_e2e8(ULONG opcode) /* LSRW */
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
void op_e2f0(ULONG opcode) /* LSRW */
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
void op_e2f8(ULONG opcode) /* LSRW */
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
void op_e2f9(ULONG opcode) /* LSRW */
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
void op_e3d0(ULONG opcode) /* LSLW */
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
void op_e3d8(ULONG opcode) /* LSLW */
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
void op_e3e0(ULONG opcode) /* LSLW */
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
void op_e3e8(ULONG opcode) /* LSLW */
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
void op_e3f0(ULONG opcode) /* LSLW */
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
void op_e3f8(ULONG opcode) /* LSLW */
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
void op_e3f9(ULONG opcode) /* LSLW */
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
void op_e4d0(ULONG opcode) /* ROXRW */
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
void op_e4d8(ULONG opcode) /* ROXRW */
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
void op_e4e0(ULONG opcode) /* ROXRW */
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
void op_e4e8(ULONG opcode) /* ROXRW */
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
void op_e4f0(ULONG opcode) /* ROXRW */
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
void op_e4f8(ULONG opcode) /* ROXRW */
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
void op_e4f9(ULONG opcode) /* ROXRW */
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
void op_e5d0(ULONG opcode) /* ROXLW */
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
void op_e5d8(ULONG opcode) /* ROXLW */
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
void op_e5e0(ULONG opcode) /* ROXLW */
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
void op_e5e8(ULONG opcode) /* ROXLW */
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
void op_e5f0(ULONG opcode) /* ROXLW */
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
void op_e5f8(ULONG opcode) /* ROXLW */
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
void op_e5f9(ULONG opcode) /* ROXLW */
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
void op_e6d0(ULONG opcode) /* RORW */
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
void op_e6d8(ULONG opcode) /* RORW */
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
void op_e6e0(ULONG opcode) /* RORW */
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
void op_e6e8(ULONG opcode) /* RORW */
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
void op_e6f0(ULONG opcode) /* RORW */
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
void op_e6f8(ULONG opcode) /* RORW */
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
void op_e6f9(ULONG opcode) /* RORW */
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
void op_e7d0(ULONG opcode) /* ROLW */
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
void op_e7d8(ULONG opcode) /* ROLW */
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
void op_e7e0(ULONG opcode) /* ROLW */
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
void op_e7e8(ULONG opcode) /* ROLW */
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
void op_e7f0(ULONG opcode) /* ROLW */
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
void op_e7f8(ULONG opcode) /* ROLW */
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
void op_e7f9(ULONG opcode) /* ROLW */
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
