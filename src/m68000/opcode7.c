#include "cpudefs.h"
void op_7000(ULONG opcode) /* MOVE */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
	ULONG dstreg = (opcode >> 9) & 7;
{{	ULONG src = srcreg;
{	VFLG = CFLG = 0;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg] = (src);
}}}}
