#include "cpudefs.h"
void op_6000(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	WORD src = nextiword();
	if (cctrue(0))
		{
		regs.pc = oldpc + (LONG)src;
		change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
		}
}

void op_6001(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
	ULONG oldpc = regs.pc;
	ULONG src = srcreg;
	if (cctrue(0))
		{
		regs.pc = oldpc + (LONG)src;
		}
}
void op_60ff(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	LONG src = nextilong();
	if (cctrue(0))
		{
		regs.pc = oldpc + (LONG)src;
		change_pc24 (regs.pc&0xffffff);
		}
}

void op_6100(void) /* BSR */
{
	ULONG oldpc = regs.pc;
	WORD src = nextiword();
	regs.a[7] -= 4;
{	CPTR spa = regs.a[7];
	put_long(spa,m68k_getpc());
	regs.pc = oldpc + (LONG)src;
	change_pc24 (regs.pc&0xffffff);
}}

void op_6101(void) /* BSR */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
	ULONG oldpc = regs.pc;
	ULONG src = srcreg;
	regs.a[7] -= 4;
{	CPTR spa = regs.a[7];
	put_long(spa,m68k_getpc());
	regs.pc = oldpc + (LONG)src;
}}

void op_61ff(void) /* BSR */
{
	ULONG oldpc = regs.pc;
	LONG src = nextilong();
	regs.a[7] -= 4;
{	CPTR spa = regs.a[7];
	put_long(spa,m68k_getpc());
	regs.pc = oldpc + (LONG)src;
	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
}}

void op_6200(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	WORD src = nextiword();
	if (cctrue(2))
		{
		regs.pc = oldpc + (LONG)src;
		change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
		}
}

void op_6201(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
	ULONG oldpc = regs.pc;
	ULONG src = srcreg;
	if (cctrue(2))
		{
		regs.pc = oldpc + (LONG)src;
		}
}

void op_62ff(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	LONG src = nextilong();
	if (cctrue(2))
		{
		regs.pc = oldpc + (LONG)src;
		change_pc24 (regs.pc&0xffffff);
		}
}

void op_6300(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	WORD src = nextiword();
	if (cctrue(3))
		{
		regs.pc = oldpc + (LONG)src;
		change_pc24 (regs.pc&0xffffff);
		}
}

void op_6301(void) /* Bcc */
{
	if (cctrue(3))
		{
		regs.pc += (LONG)(BYTE)(opcode&255);
		}
}

void op_63ff(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	LONG src = nextilong();
	if (cctrue(3))
		{
		regs.pc = oldpc + (LONG)src;
		change_pc24 (regs.pc&0xffffff);
		}
}

void op_6400(void) /* Bcc */
{
	WORD src = nextiword();
	if (cctrue(4))
		{
		regs.pc += (LONG)src-2;
		change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
		}
}

void op_6401(void) /* Bcc */
{
	if (cctrue(4))
		{
		regs.pc += (LONG)(BYTE)(opcode&255);
		}
}

void op_64ff(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	LONG src = nextilong();
	if (cctrue(4)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}

void op_6500(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	WORD src = nextiword();
	if (cctrue(5)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}

void op_6501(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
	ULONG oldpc = regs.pc;
	ULONG src = srcreg;
	if (cctrue(5)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}

void op_65ff(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	LONG src = nextilong();
	if (cctrue(5)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}

void op_6600(void) /* Bcc */
{
	WORD src = nextiword();
	if (cctrue(6)) {
		regs.pc += (LONG)src-2;
		change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
		}
}

void op_6601(void) /* Bcc */
{
	if (cctrue(6))
		{
		regs.pc += (LONG)(BYTE)(opcode&255);
		}
}

void op_66ff(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	LONG src = nextilong();
	if (cctrue(6)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}

void op_6700(void) /* Bcc */
{
	WORD src = nextiword();
	if (cctrue(7))
		{
		regs.pc += (LONG)src-2;
		change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
		}
}

void op_6701(void) /* Bcc */
{
	if (cctrue(7))
		{
		regs.pc += (LONG)(BYTE)(opcode & 255);
		}
}

void op_67ff(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	LONG src = nextilong();
	if (cctrue(7)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}

void op_6800(void) /* Bcc */
{
	ULONG oldpc = regs.pc;
	WORD src = nextiword();
	if (cctrue(8)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}

void op_6801(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(8)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_68ff(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(8)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6900(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(9)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6901(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(9)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_69ff(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(9)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6a00(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(10)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6a01(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(10)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6aff(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(10)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6b00(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(11)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6b01(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(11)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6bff(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(11)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6c00(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(12)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6c01(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(12)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6cff(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(12)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6d00(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(13)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6d01(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(13)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6dff(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(13)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6e00(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(14)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6e01(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(14)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6eff(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(14)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6f00(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	WORD src = nextiword();
	if (cctrue(15)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
void op_6f01(void) /* Bcc */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
{	ULONG oldpc = regs.pc;
{	ULONG src = srcreg;
	if (cctrue(15)) {
	regs.pc = oldpc + (LONG)src;	/*change_pc24(regs.pc&0xffffff);*/	/* ASG 971108 */
	}
}}}
void op_6fff(void) /* Bcc */
{
{	ULONG oldpc = regs.pc;
{	LONG src = nextilong();
	if (cctrue(15)) {
	regs.pc = oldpc + (LONG)src;	change_pc24 (regs.pc&0xffffff);	/* ASG 971108 */
	}
}}}
