#include "cpudefs.h"
void op_7000(void) /* MOVE */
{
	ULONG srcreg = (LONG)(BYTE)(opcode & 255);
	ULONG dstreg = (opcode >> 9) & 7;
{{	ULONG src = srcreg;
{	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
