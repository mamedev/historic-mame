/****************************************************************************
*			  real mode i286 emulator v1.4 by Fabrice Frances				*
*				(initial work based on David Hedley's pcemu)                *
****************************************************************************/

// file will be included in all cpu variants
// function renaming will be added when neccessary
// timing value should move to separate array

static void PREFIX186(_pusha)(void)    /* Opcode 0x60 */
{
	unsigned tmp=I.regs.w[SP];
#ifdef V20
	nec_ICount-=51;
#else
	i86_ICount-=17;
#endif
	PUSH(I.regs.w[AX]);
	PUSH(I.regs.w[CX]);
	PUSH(I.regs.w[DX]);
	PUSH(I.regs.w[BX]);
    PUSH(tmp);
	PUSH(I.regs.w[BP]);
	PUSH(I.regs.w[SI]);
	PUSH(I.regs.w[DI]);
}

static void PREFIX186(_popa)(void)    /* Opcode 0x61 */
{
	 unsigned tmp;
#ifdef V20
	nec_ICount-=59;
#else
	i86_ICount-=19;
#endif
	POP(I.regs.w[DI]);
	POP(I.regs.w[SI]);
	POP(I.regs.w[BP]);
	 POP(tmp);
	POP(I.regs.w[BX]);
	POP(I.regs.w[DX]);
	POP(I.regs.w[CX]);
	POP(I.regs.w[AX]);
}

static void PREFIX186(_bound)(void)    /* Opcode 0x62 */
{
	unsigned ModRM = FETCHOP;
	int low = (INT16)GetRMWord(ModRM);
    int high= (INT16)GetnextRMWord;
	int tmp= (INT16)RegWord(ModRM);
	if (tmp<low || tmp>high) {
		/* OB: on NECs CS:IP points to instruction
		   FOLLOWING the BOUND instruction ! */
#if !defined(V20)
		I.ip-=2;
		PREFIX86(_interrupt)(5);
#else
		PREFIX(_interrupt)(5,0);
#endif
	}
#ifdef V20
	nec_ICount-=20;
#endif
}

static void PREFIX186(_push_d16)(void)    /* Opcode 0x68 */
{
	unsigned tmp = FETCH;
#ifdef V20
	nec_ICount-=12;
#else
	i86_ICount-=3;
#endif
	tmp += FETCH << 8;
	PUSH(tmp);
}

static void PREFIX186(_imul_d16)(void)    /* Opcode 0x69 */
{
	DEF_r16w(dst,src);
	unsigned src2=FETCH;
	src+=(FETCH<<8);

#ifdef V20
	nec_ICount-=(ModRM >=0xc0 )?38:47;
#else
	i86_ICount-=150;
#endif
	dst = (INT32)((INT16)src)*(INT32)((INT16)src2);
	I.CarryVal = I.OverVal = (((INT32)dst) >> 15 != 0) && (((INT32)dst) >> 15 != -1);
	RegWord(ModRM)=(WORD)dst;
}


static void PREFIX186(_push_d8)(void)    /* Opcode 0x6a */
{
	unsigned tmp = (WORD)((INT16)((INT8)FETCH));
#ifdef V20
	nec_ICount-=7;
#else
	i86_ICount-=3;
#endif
	PUSH(tmp);
}

static void PREFIX186(_imul_d8)(void)    /* Opcode 0x6b */
{
	DEF_r16w(dst,src);
	unsigned src2= (WORD)((INT16)((INT8)FETCH));

#ifdef V20
	nec_ICount-=(ModRM >=0xc0 )?31:39;
#else
	i86_ICount-=150;
#endif
	dst = (INT32)((INT16)src)*(INT32)((INT16)src2);
	I.CarryVal = I.OverVal = (((INT32)dst) >> 15 != 0) && (((INT32)dst) >> 15 != -1);
	RegWord(ModRM)=(WORD)dst;
}

static void PREFIX186(_insb)(void)    /* Opcode 0x6c */
{
	i86_ICount-=5;
	PutMemB(ES,I.regs.w[DI],read_port(I.regs.w[DX]));
	I.regs.w[DI]+= -2 * I.DF + 1;
}

static void PREFIX186(_insw)(void)    /* Opcode 0x6d */
{
	i86_ICount-=5;
	PutMemB(ES,I.regs.w[DI],read_port(I.regs.w[DX]));
	PutMemB(ES,I.regs.w[DI]+1,read_port(I.regs.w[DX]+1));
	I.regs.w[DI]+= -4 * I.DF + 2;
}

static void PREFIX186(_outsb)(void)    /* Opcode 0x6e */
{
#ifdef V20
	nec_ICount-=8;	
#else
	i86_ICount-=5;
#endif
	write_port(I.regs.w[DX],GetMemB(DS,I.regs.w[SI]));
	I.regs.w[DI]+= -2 * I.DF + 1;
}

static void PREFIX186(_outsw)(void)    /* Opcode 0x6f */
{
#ifdef V20
	nec_ICount-=8;
#else
	i86_ICount-=5;
#endif
	write_port(I.regs.w[DX],GetMemB(DS,I.regs.w[SI]));
	write_port(I.regs.w[DX]+1,GetMemB(DS,I.regs.w[SI]+1));
	I.regs.w[DI]+= -4 * I.DF + 2;
}

static void PREFIX186(_rotshft_bd8)(void)    /* Opcode 0xc0 */
{
	unsigned ModRM = FETCH;
	unsigned count = FETCH;

	PREFIX86(_rotate_shift_Byte)(ModRM,count);
}

static void PREFIX186(_rotshft_wd8)(void)    /* Opcode 0xc1 */
{
	unsigned ModRM = FETCH;
	unsigned count = FETCH;

	PREFIX86(_rotate_shift_Word)(ModRM,count);
}

static void PREFIX186(_enter)(void)    /* Opcode 0xc8 */
{
	unsigned nb = FETCH;	 unsigned i,level;

#ifdef V20
	nec_ICount-=23;
#else
	i86_ICount-=11;
#endif
	nb += FETCH << 8;
	level = FETCH;
	PUSH(I.regs.w[BP]);
	I.regs.w[BP]=I.regs.w[SP];
	I.regs.w[SP] -= nb;
	for (i=1;i<level;i++) {
		PUSH(GetMemW(SS,I.regs.w[BP]-i*2));
#ifdef V20
		nec_ICount-=16;
#else
		i86_ICount-=4;
#endif
	}
	if (level) PUSH(I.regs.w[BP]);
}

static void PREFIX186(_leave)(void)    /* Opcode 0xc9 */
{
#ifdef V20
	nec_ICount-=8;
#else
	i86_ICount-=5;
#endif
	I.regs.w[SP]=I.regs.w[BP];
	POP(I.regs.w[BP]);
}
