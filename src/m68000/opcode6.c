#include "cpudefs.h"
void op_6000(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(0)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6001(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(0)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_60ff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(0)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6100(ULONG opcode) /* BSR */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
{	regs.a[7] -= 4;
{	CPTR spa = regs.a[7];
	put_long(spa,m68k_getpc());
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
}}}}}
void op_6101(ULONG opcode) /* BSR */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
{	regs.a[7] -= 4;
{	CPTR spa = regs.a[7];
	put_long(spa,m68k_getpc());
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
}}}}}
void op_61ff(ULONG opcode) /* BSR */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
{	regs.a[7] -= 4;
{	CPTR spa = regs.a[7];
	put_long(spa,m68k_getpc());
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
}}}}}
void op_6200(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(2)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6201(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(2)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_62ff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(2)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6300(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(3)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6301(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(3)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_63ff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(3)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6400(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(4)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6401(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(4)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_64ff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(4)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6500(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(5)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6501(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(5)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_65ff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(5)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6600(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(6)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6601(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(6)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_66ff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(6)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6700(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(7)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6701(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(7)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_67ff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(7)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6800(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(8)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6801(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(8)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_68ff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(8)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6900(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(9)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6901(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(9)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_69ff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(9)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6a00(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(10)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6a01(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(10)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6aff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(10)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6b00(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(11)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6b01(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(11)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6bff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(11)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6c00(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(12)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6c01(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(12)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6cff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(12)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6d00(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(13)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6d01(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(13)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6dff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(13)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6e00(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(14)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6e01(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(14)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6eff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(14)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6f00(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(15)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6f01(ULONG opcode) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(15)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6fff(ULONG opcode) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(15)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
