/*###################################################################################################
**	CONFIGURATION
**#################################################################################################*/

#define STRIP_NOPS			1		/* 1 is faster */
#define OPTIMIZE_LUI		1		/* 1 is faster */
#define USE_SSE				0		/* can't tell any speed difference here */



/*###################################################################################################
**	CONSTANTS
**#################################################################################################*/

/* recompiler flags */
#define RECOMPILE_UNIMPLEMENTED			0x0000
#define RECOMPILE_SUCCESSFUL			0x0001
#define RECOMPILE_SUCCESSFUL_CP(c,p)	(RECOMPILE_SUCCESSFUL | (((c) & 0xff) << 16) | (((p) & 0xff) << 24))
#define RECOMPILE_MAY_CAUSE_EXCEPTION	0x0002
#define RECOMPILE_END_OF_STRING			0x0004
#define RECOMPILE_CHECK_INTERRUPTS		0x0008
#define RECOMPILE_CHECK_SW_INTERRUPTS	0x0010
#define RECOMPILE_ADD_DISPATCH			0x0020



/*###################################################################################################
**	PROTOTYPES
**#################################################################################################*/

static UINT32 compile_one(struct drccore *drc, UINT32 pc);

static void append_generate_exception(struct drccore *drc, UINT8 exception);
static void append_update_cycle_counting(struct drccore *drc);
static void append_check_interrupts(struct drccore *drc, int inline_generate);
static void append_check_sw_interrupts(struct drccore *drc, int inline_generate);

static UINT32 recompile_instruction(struct drccore *drc, UINT32 pc);
static UINT32 recompile_special(struct drccore *drc, UINT32 pc, UINT32 op);
static UINT32 recompile_regimm(struct drccore *drc, UINT32 pc, UINT32 op);

static UINT32 recompile_cop0(struct drccore *drc, UINT32 pc, UINT32 op);
static UINT32 recompile_cop1(struct drccore *drc, UINT32 pc, UINT32 op);
static UINT32 recompile_cop1x(struct drccore *drc, UINT32 pc, UINT32 op);



/*###################################################################################################
**	PRIVATE GLOBAL VARIABLES
**#################################################################################################*/

static UINT64 dmult_temp1;
static UINT64 dmult_temp2;
static UINT32 jr_temp;

static UINT8 in_delay_slot = 0;

static UINT32 scratchspace[10];



/*###################################################################################################
**	RECOMPILER CALLBACKS
**#################################################################################################*/

/*------------------------------------------------------------------
	mips3drc_init
------------------------------------------------------------------*/

static void mips3drc_init(void)
{
	struct drcconfig drconfig;
	
	/* fill in the config */
	memset(&drconfig, 0, sizeof(drconfig));
	drconfig.cache_size       = CACHE_SIZE;
	drconfig.max_instructions = MAX_INSTRUCTIONS;
	drconfig.address_bits     = 32;
	drconfig.lsbs_to_ignore   = 2;
	drconfig.uses_fp          = 1;
	drconfig.uses_sse         = USE_SSE;
	drconfig.pc_in_memory     = 0;
	drconfig.icount_in_memory = 0;
	drconfig.pcptr            = (UINT32 *)&mips3.pc;
	drconfig.icountptr        = (UINT32 *)&mips3_icount;
	drconfig.esiptr           = NULL;
	drconfig.cb_reset         = mips3drc_reset;
	drconfig.cb_recompile     = mips3drc_recompile;
	drconfig.cb_entrygen      = mips3drc_entrygen;
	
	/* initialize the compiler */
	mips3.drc = drc_init(cpu_getactivecpu(), &drconfig);
	mips3.drcoptions = MIPS3DRC_FASTEST_OPTIONS;
}



/*------------------------------------------------------------------
	mips3drc_reset
------------------------------------------------------------------*/

static void mips3drc_reset(struct drccore *drc)
{
	mips3.generate_interrupt_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_INTERRUPT);
	
	mips3.generate_cop_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_BADCOP);
		
	mips3.generate_overflow_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_OVERFLOW);
		
	mips3.generate_invalidop_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_INVALIDOP);
		
	mips3.generate_syscall_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_SYSCALL);
		
	mips3.generate_break_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_BREAK);
		
	mips3.generate_trap_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_TRAP);
}



/*------------------------------------------------------------------
	mips3drc_recompile
------------------------------------------------------------------*/

static void mips3drc_recompile(struct drccore *drc)
{
	int remaining = MAX_INSTRUCTIONS;
	UINT32 pc = mips3.pc;
	
//	printf("recompile_callback @ PC=%08X\n", mips3.pc);
/*
	if (!ram_read_table)
	{
		ram_read_table = malloc(65536 * sizeof(void *));
		ram_write_table = malloc(65536 * sizeof(void *));
		if (ram_read_table && ram_write_table)
			for (i = 0; i < 65536; i++)
			{
				ram_read_table[i] = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, i << 16);
				if (ram_read_table[i]) ram_read_table[i] = (UINT8 *)ram_read_table[i] - (i << 16);
				ram_write_table[i] = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, i << 16);
				if (ram_write_table[i]) ram_write_table[i] = (UINT8 *)ram_write_table[i] - (i << 16);
			}
	}
*/
	/* begin the sequence */
	drc_begin_sequence(drc, pc);
	
	/* loose verification case: one verification here only */
	if (!(mips3.drcoptions & MIPS3DRC_STRICT_VERIFY))
	{
		change_pc(pc);
		drc_append_verify_code(drc, cpu_opptr(pc), 4);
	}

	/* loop until we hit an unconditional branch */
	while (--remaining != 0)
	{
		UINT32 result;

		/* compile one instruction */
		result = compile_one(drc, pc);
		pc += (INT8)(result >> 24);
		if (result & RECOMPILE_END_OF_STRING)
			break;
	}
	
	/* add dispatcher just in case */
	if (remaining == 0)
		drc_append_dispatcher(drc);
	
	/* end the sequence */
	drc_end_sequence(drc);
	log_code(drc);
}


/*------------------------------------------------------------------
	mips3drc_entrygen
------------------------------------------------------------------*/

static void mips3drc_entrygen(struct drccore *drc)
{
	append_check_interrupts(drc, 1);
}



/*###################################################################################################
**	RECOMPILER CORE
**#################################################################################################*/

/*------------------------------------------------------------------
	compile_one
------------------------------------------------------------------*/

static UINT32 compile_one(struct drccore *drc, UINT32 pc)
{
	int pcdelta, cycles;
	UINT32 *opptr;
	UINT32 result;
	
	/* register this instruction */
	drc_register_code_at_cache_top(drc, pc);

	/* get a pointer to the current instruction */
	change_pc(pc);
	opptr = cpu_opptr(pc);
	
	log_symbol(drc, ~2);
	log_symbol(drc, pc);
	
	/* emit debugging and self-modifying code checks */
	drc_append_call_debugger(drc);
	if (mips3.drcoptions & MIPS3DRC_STRICT_VERIFY)
		drc_append_verify_code(drc, opptr, 4);
	
	/* compile the instruction */
	result = recompile_instruction(drc, pc);

	/* handle the results */		
	if (!(result & RECOMPILE_SUCCESSFUL))
	{
		printf("Unimplemented op %08X (%02X,%02X)\n", *opptr, *opptr >> 26, *opptr & 0x3f);
		mips3_exit();
		exit(1);
	}
	pcdelta = (INT8)(result >> 24);
	cycles = (INT8)(result >> 16);
	
	/* absorb any NOPs following */
	#if (STRIP_NOPS)
	{
		if (!(result & (RECOMPILE_END_OF_STRING | RECOMPILE_CHECK_INTERRUPTS | RECOMPILE_CHECK_SW_INTERRUPTS)))
			while (pcdelta < 120 && opptr[pcdelta/4] == 0)
			{
				pcdelta += 4;
				cycles += 1;
			}
	}
	#endif

	/* epilogue */
	drc_append_standard_epilogue(drc, cycles, pcdelta, 1);

	/* check interrupts */
	if (result & RECOMPILE_CHECK_INTERRUPTS)
		append_check_interrupts(drc, 0);
	if (result & RECOMPILE_CHECK_SW_INTERRUPTS)
		append_check_sw_interrupts(drc, 0);
	if (result & RECOMPILE_ADD_DISPATCH)
		drc_append_dispatcher(drc);
	
	return (result & 0xffff) | ((UINT8)cycles << 16) | ((UINT8)pcdelta << 24);
}



/*###################################################################################################
**	COMMON ROUTINES
**#################################################################################################*/

/*------------------------------------------------------------------
	append_generate_exception
------------------------------------------------------------------*/

static void append_generate_exception(struct drccore *drc, UINT8 exception)
{
	UINT32 offset = (exception >= EXCEPTION_TLBMOD && exception <= EXCEPTION_TLBSTORE) ? 0x80 : 0x180;
	struct linkdata link1, link2;
	
	_mov_m32abs_r32(&mips3.cpr[0][COP0_EPC], REG_EDI);					// mov	[mips3.cpr[0][COP0_EPC]],edi
	_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Cause]);				// mov	eax,[mips3.cpr[0][COP0_Cause]]
	_and_r32_imm(REG_EAX, ~0x800000ff);									// and	eax,~0x800000ff
	if (exception)
		_or_r32_imm(REG_EAX, exception << 2);							// or	eax,exception << 2
	_cmp_m32abs_imm(&mips3.nextpc, ~0);									// cmp	[mips3.nextpc],~0
	_jcc_short_link(COND_E, &link1);									// je	skip
	_mov_m32abs_imm(&mips3.nextpc, ~0);									// mov	[mips3.nextpc],~0
	_sub_m32abs_imm(&mips3.cpr[0][COP0_EPC], 4);						// sub	[mips3.cpr[0][COP0_EPC]],4
	_or_r32_imm(REG_EAX, 0x80000000);									// or	eax,0x80000000
	_resolve_link(&link1);												// skip:
	_mov_m32abs_r32(&mips3.cpr[0][COP0_Cause], REG_EAX);				// mov	[mips3.cpr[0][COP0_Cause]],eax
	_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Status]);				// mov	eax,[[mips3.cpr[0][COP0_Status]]
	_or_r32_imm(REG_EAX, SR_EXL);										// or	eax,SR_EXL
	_test_r32_imm(REG_EAX, SR_BEV);										// test	eax,SR_BEV
	_mov_m32abs_r32(&mips3.cpr[0][COP0_Status], REG_EAX);				// mov	[[mips3.cpr[0][COP0_Status]],eax
	_mov_r32_imm(REG_EDI, 0xbfc00200 + offset);							// mov	edi,0xbfc00200+offset
	_jcc_short_link(COND_NZ, &link2);									// jnz	skip2
	_mov_r32_imm(REG_EDI, 0x80000000 + offset);							// mov	edi,0x80000000+offset
	_resolve_link(&link2);												// skip2:
	drc_append_dispatcher(drc);											// dispatch
}


/*------------------------------------------------------------------
	append_update_cycle_counting
------------------------------------------------------------------*/

static void append_update_cycle_counting(struct drccore *drc)
{
	_mov_m32abs_r32(&mips3_icount, REG_EBP);							// mov	[mips3_icount],ebp
	_call((void *)update_cycle_counting);								// call	update_cycle_counting
	_mov_r32_m32abs(REG_EBP, &mips3_icount);							// mov	ebp,[mips3_icount]
}


/*------------------------------------------------------------------
	append_check_interrupts
------------------------------------------------------------------*/

static void append_check_interrupts(struct drccore *drc, int inline_generate)
{
	struct linkdata link1, link2, link3;
	_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Cause]);				// mov	eax,[mips3.cpr[0][COP0_Cause]]
	_and_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Status]);				// and	eax,[mips3.cpr[0][COP0_Status]]
	_and_r32_imm(REG_EAX, 0xfc00);										// and	eax,0xfc00
	if (!inline_generate)
		_jcc_short_link(COND_Z, &link1);								// jz	skip
	else
		_jcc_near_link(COND_Z, &link1);									// jz	skip
	_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_IE);				// test	[mips3.cpr[0][COP0_Status],SR_IE
	if (!inline_generate)
		_jcc_short_link(COND_Z, &link2);								// jz	skip
	else
		_jcc_near_link(COND_Z, &link2);									// jz	skip
	_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_EXL | SR_ERL);		// test	[mips3.cpr[0][COP0_Status],SR_EXL | SR_ERL
	if (!inline_generate)
		_jcc(COND_Z, mips3.generate_interrupt_exception);				// jz	generate_interrupt_exception
	else
	{
		_jcc_near_link(COND_NZ, &link3);								// jnz	skip
		append_generate_exception(drc, EXCEPTION_INTERRUPT);			// <generate exception>
		_resolve_link(&link3);											// skip:
	}
	_resolve_link(&link1);												// skip:
	_resolve_link(&link2);
}


/*------------------------------------------------------------------
	append_check_sw_interrupts
------------------------------------------------------------------*/

static void append_check_sw_interrupts(struct drccore *drc, int inline_generate)
{
	_test_m32abs_imm(&mips3.cpr[0][COP0_Cause], 0x300);					// test	[mips3.cpr[0][COP0_Cause]],0x300
	_jcc(COND_NZ, mips3.generate_interrupt_exception);					// jnz	generate_interrupt_exception
}


/*------------------------------------------------------------------
	append_branch_or_dispatch
------------------------------------------------------------------*/

static void append_branch_or_dispatch(struct drccore *drc, UINT32 newpc, int cycles)
{
	void *code = drc_get_code_at_pc(drc, newpc);
	_mov_r32_imm(REG_EDI, newpc);
	drc_append_standard_epilogue(drc, cycles, 0, 1);

	if (code)
		_jmp(code);
	else
		drc_append_tentative_fixed_dispatcher(drc, newpc);
}



/*###################################################################################################
**	USEFUL PRIMITIVES
**#################################################################################################*/

#define _zero_m64abs(addr) 							\
do { 												\
	if (USE_SSE) 									\
	{												\
		_pxor_r128_r128(REG_XMM0, REG_XMM0);		\
		_movsd_m64abs_r128(addr, REG_XMM0);			\
	}												\
	else											\
		_mov_m64abs_imm32(addr, 0);					\
} while (0)											\

#define _mov_m64abs_m64abs(dst, src)				\
do { 												\
	if (USE_SSE) 									\
	{												\
		_movsd_r128_m64abs(REG_XMM0, src);			\
		_movsd_m64abs_r128(dst, REG_XMM0);			\
	}												\
	else											\
	{												\
		_mov_r64_m64abs(REG_EDX, REG_EAX, src);		\
		_mov_m64abs_r64(dst, REG_EDX, REG_EAX);		\
	}												\
} while (0)											\

#define _save_pc_before_call()						\
do { 												\
	if (mips3.drcoptions & MIPS3DRC_FLUSH_PC)		\
		_mov_m32abs_r32(drc->pcptr, REG_EDI);		\
} while (0)


/*###################################################################################################
**	CORE RECOMPILATION
**#################################################################################################*/

static void ddiv(INT64 *rs, INT64 *rt)
{
	if (*rt)
	{
		mips3.lo = *rs / *rt;
		mips3.hi = *rs % *rt;
	}
}

static void ddivu(UINT64 *rs, UINT64 *rt)
{
	if (*rt)
	{
		mips3.lo = *rs / *rt;
		mips3.hi = *rs % *rt;
	}
}


/*------------------------------------------------------------------
	recompile_delay_slot
------------------------------------------------------------------*/

static int recompile_delay_slot(struct drccore *drc, UINT32 pc)
{
	UINT8 *saved_top = drc->cache_top;
	UINT32 result;

	/* recompile the instruction as-is */
	in_delay_slot = 1;
	result = recompile_instruction(drc, pc);								// <next instruction>
	in_delay_slot = 0;

	/* if the instruction can cause an exception, recompile setting nextpc */
	if (result & RECOMPILE_MAY_CAUSE_EXCEPTION)
	{
		drc->cache_top = saved_top;
		_mov_m32abs_imm(&mips3.nextpc, 0);									// bogus nextpc for exceptions
		result = recompile_instruction(drc, pc);							// <next instruction>
		_mov_m32abs_imm(&mips3.nextpc, ~0);									// reset nextpc
	}

	return (INT8)(result >> 16);
}


/*------------------------------------------------------------------
	recompile_lui
------------------------------------------------------------------*/

static UINT32 recompile_lui(struct drccore *drc, UINT32 pc, UINT32 op)
{
	UINT32 address = UIMMVAL << 16;
	UINT32 targetreg = RTREG;

#if OPTIMIZE_LUI	
	UINT32 nextop = cpu_readop32(pc + 4);
	UINT8 nextrsreg = (nextop >> 21) & 31;
	UINT8 nextrtreg = (nextop >> 16) & 31;
	INT32 nextsimm = (INT16)nextop;
	void *memory;

	/* if the next instruction is a load or store, see if we can consolidate */
	if (!in_delay_slot)
		switch (nextop >> 26)
		{
			case 0x08:	/* addi */
			case 0x09:	/* addiu */
				if (nextrsreg != targetreg || nextrtreg != targetreg)
					break;
				_mov_m64abs_imm32(&mips3.r[targetreg], address + nextsimm);	// mov	[targetreg],const
				return RECOMPILE_SUCCESSFUL_CP(2,8);
				
			case 0x0d:	/* ori */
				if (nextrsreg != targetreg || nextrtreg != targetreg)
					break;
				_mov_m64abs_imm32(&mips3.r[targetreg], address | (UINT16)nextsimm);	// mov	[targetreg],const
				return RECOMPILE_SUCCESSFUL_CP(2,8);
				
			case 0x20:	/* lb */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, BYTE4_XOR_BE(address + nextsimm));
				else
					memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		// mov	[targetreg],const << 16
				_movsx_r32_m8abs(REG_EAX, memory);							// movsx eax,byte [memory]
				_cdq();														// cdq
				_mov_m64abs_r64(&mips3.r[nextrtreg], REG_EDX, REG_EAX);		// mov	[nextrtreg],edx:eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x21:	/* lh */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, WORD_XOR_BE(address + nextsimm));
				else
					memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		// mov	[targetreg],const << 16
				_movsx_r32_m16abs(REG_EAX, memory);							// movsx eax,word [memory]
				_cdq();														// cdq
				_mov_m64abs_r64(&mips3.r[nextrtreg], REG_EDX, REG_EAX);		// mov	[nextrtreg],edx:eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x23:	/* lw */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		// mov	[targetreg],const << 16
				_mov_r32_m32abs(REG_EAX, memory);							// mov	eax,[memory]
				_cdq();														// cdq
				_mov_m64abs_r64(&mips3.r[nextrtreg], REG_EDX, REG_EAX);		// mov	[nextrtreg],edx:eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x24:	/* lbu */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, BYTE4_XOR_BE(address + nextsimm));
				else
					memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		// mov	[targetreg],const << 16
				_movzx_r32_m8abs(REG_EAX, memory);							// movzx eax,byte [memory]
				_mov_m32abs_imm(HI(&mips3.r[nextrtreg]), 0);				// mov	[nextrtreg].hi,0
				_mov_m32abs_r32(LO(&mips3.r[nextrtreg]), REG_EAX);			// mov	[nextrtreg].lo,eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x25:	/* lhu */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, WORD_XOR_BE(address + nextsimm));
				else
					memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		// mov	[targetreg],const << 16
				_movzx_r32_m16abs(REG_EAX, memory);							// movzx eax,word [memory]
				_mov_m32abs_imm(HI(&mips3.r[nextrtreg]), 0);				// mov	[nextrtreg].hi,0
				_mov_m32abs_r32(LO(&mips3.r[nextrtreg]), REG_EAX);			// mov	[nextrtreg].lo,eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x27:	/* lwu */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		// mov	[targetreg],const << 16
				_mov_r32_m32abs(REG_EAX, memory);							// mov	eax,[memory]
				_mov_m32abs_imm(HI(&mips3.r[nextrtreg]), 0);				// mov	[nextrtreg].hi,0
				_mov_m32abs_r32(LO(&mips3.r[nextrtreg]), REG_EAX);			// mov	[nextrtreg].lo,eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x31:	/* lwc1 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				_mov_r32_m32abs(REG_EAX, memory);							// mov	eax,[memory]
				_mov_m32abs_r32(LO(&mips3.cpr[1][nextrtreg]), REG_EAX);		// mov	cpr[1][nextrtreg].lo,eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x32:	/* lwc2 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				_mov_r32_m32abs(REG_EAX, memory);							// mov	eax,[memory]
				_mov_m32abs_r32(LO(&mips3.cpr[2][nextrtreg]), REG_EAX);		// mov	cpr[2][nextrtreg].lo,eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x35:	/* ldc1 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				_mov_r64_m64abs(REG_EDX, REG_EAX, memory);					// mov	edx:eax,[memory]
				_mov_m64abs_r64(&mips3.cpr[1][nextrtreg], REG_EDX, REG_EAX);// mov	cpr[1][nextrtreg],edx:eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x36:	/* ldc2 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				_mov_r64_m64abs(REG_EDX, REG_EAX, memory);					// mov	edx:eax,[memory]
				_mov_m64abs_r64(&mips3.cpr[2][nextrtreg], REG_EDX, REG_EAX);// mov	cpr[2][nextrtreg],edx:eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x37:	/* ld */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		// mov	[targetreg],const << 16
				_mov_r64_m64abs(REG_EDX, REG_EAX, memory);					// mov	eax,[memory]
				_mov_m64abs_r64(&mips3.r[nextrtreg], REG_EDX, REG_EAX);		// mov	[nextrtreg],eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x28:	/* sb */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, BYTE4_XOR_BE(address + nextsimm));
				else
					memory = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				if (nextrtreg != 0)
				{
					_mov_r8_m8abs(REG_AL, &mips3.r[nextrtreg]);				// mov	ax,[nextrtreg]
					_mov_m8abs_r8(memory, REG_AL);							// mov	[memory],ax
				}
				else
					_mov_m8abs_imm(memory, 0);								// mov	[memory],0
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x29:	/* sh */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, WORD_XOR_BE(address + nextsimm));
				else
					memory = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				if (nextrtreg != 0)
				{
					_mov_r16_m16abs(REG_AX, &mips3.r[nextrtreg]);			// mov	ax,[nextrtreg]
					_mov_m16abs_r16(memory, REG_AX);						// mov	[memory],ax
				}
				else
					_mov_m16abs_imm(memory, 0);								// mov	[memory],0
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x2b:	/* sw */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				if (nextrtreg != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[nextrtreg]);			// mov	eax,[nextrtreg]
					_mov_m32abs_r32(memory, REG_EAX);						// mov	[memory],eax
				}
				else
					_mov_m32abs_imm(memory, 0);								// mov	[memory],0
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x39:	/* swc1 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][nextrtreg]);			// mov	eax,cpr[1][nextrtreg]
				_mov_m32abs_r32(memory, REG_EAX);							// mov	[memory],eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x3a:	/* swc2 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				_mov_r32_m32abs(REG_EAX, &mips3.cpr[2][nextrtreg]);			// mov	eax,cpr[2][nextrtreg]
				_mov_m32abs_r32(memory, REG_EAX);							// mov	[memory],eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x3d:	/* sdc1 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.cpr[1][nextrtreg]);// mov	edx:eax,cpr[1][nextrtreg]
				_mov_m64abs_r64(memory, REG_EDX, REG_EAX);					// mov	[memory],edx:eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x3e:	/* sdc2 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.cpr[2][nextrtreg]);// mov	edx:eax,cpr[2][nextrtreg]
				_mov_m64abs_r64(memory, REG_EDX, REG_EAX);					// mov	[memory],edx:eax
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x3f:	/* sd */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			// mov	[targetreg],const << 16
				if (nextrtreg != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[nextrtreg]);	// mov	edx:eax,[nextrtreg]
					_mov_m64abs_r64(memory, REG_EDX, REG_EAX);				// mov	[memory],edx:eax
				}
				else
					_mov_m64abs_imm32(memory, 0);							// mov	[memory],0
				return RECOMPILE_SUCCESSFUL_CP(2,8);
		}
#endif

	/* default case: standard LUI */	
	_mov_m64abs_imm32(&mips3.r[targetreg], address);					// mov	[rtreg],const << 16
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}


/*------------------------------------------------------------------
	recompile_ldlr_le
------------------------------------------------------------------*/

static UINT32 recompile_ldlr_le(struct drccore *drc, UINT8 rtreg, UINT8 rsreg, INT16 simmval)
{
	struct linkdata link1, link2;
	_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
	_save_pc_before_call();													// save pc
	_mov_r32_m32abs(REG_EAX, &mips3.r[rsreg]);								// mov	eax,[rsreg]
	if (simmval)
		_add_r32_imm(REG_EAX, simmval);										// add	eax,simmval
	_mov_m32abs_r32(&scratchspace[0], REG_EAX);								// mov	[scratchspace[0]],eax
	_and_r32_imm(REG_EAX, ~3);												// and	eax,~3
	_push_r32(REG_EAX);														// push	eax
	_call((void *)mips3.memory.readlong);									// call	readlong
	_mov_m32abs_r32(&scratchspace[1], REG_EAX);								// mov	[scratchspace[1]],eax
	_add_m32bd_imm(REG_ESP, 0, 4);											// add	[esp],4
	_call((void *)mips3.memory.readlong);									// call	readlong
	_test_m32abs_imm(&scratchspace[0], 3);									// test	[scratchspace[0]],3
	_jcc_short_link(COND_Z, &link1);										// jz	link1
	_mov_m32abs_r32(&scratchspace[2], REG_EAX);								// mov	[scratchspace[2]],eax
	_add_m32bd_imm(REG_ESP, 0, 4);											// add	[esp],4
	_call((void *)mips3.memory.readlong);									// call	readlong
	_mov_r32_m32abs(REG_ECX, &scratchspace[0]);								// mov	ecx,[scratchspace[0]]
	_mov_r32_m32abs(REG_EBX, &scratchspace[1]);								// mov	ebx,[scratchspace[1]]
	_mov_r32_m32abs(REG_EDX, &scratchspace[2]);								// mov	edx,[scratchspace[2]]
	_shl_r32_imm(REG_ECX, 3);												// shl	ecx,3
	_shrd_r32_r32_cl(REG_EBX, REG_EDX);										// shrd	ebx,edx,cl
	_shrd_r32_r32_cl(REG_EDX, REG_EAX);										// shrd edx,eax,cl
	_mov_m64abs_r64(&mips3.r[rtreg], REG_EDX, REG_EBX);						// mov	[rtreg],edx:ebx
	_jmp_short_link(&link2);												// jmp	done
	_resolve_link(&link1);													// link1:
	_mov_r32_m32abs(REG_EDX, &scratchspace[1]);								// mov	edx,[scratchspace[1]]
	_mov_m64abs_r64(&mips3.r[rtreg], REG_EAX, REG_EDX);						// mov	[rtreg],eax:edx
	_resolve_link(&link2);													// link2:
	_add_r32_imm(REG_ESP, 4);												// add	esp,4
	_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
	return RECOMPILE_SUCCESSFUL_CP(2,8);
}


/*------------------------------------------------------------------
	recompile_lwlr_le
------------------------------------------------------------------*/

static UINT32 recompile_lwlr_le(struct drccore *drc, UINT8 rtreg, UINT8 rsreg, INT16 simmval)
{
	struct linkdata link1;
	_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
	_save_pc_before_call();													// save pc
	_mov_r32_m32abs(REG_EAX, &mips3.r[rsreg]);								// mov	eax,[rsreg]
	if (simmval)
		_add_r32_imm(REG_EAX, simmval);										// add	eax,simmval
	_mov_m32abs_r32(&scratchspace[0], REG_EAX);								// mov	[scratchspace[0]],eax
	_and_r32_imm(REG_EAX, ~3);												// and	eax,~3
	_push_r32(REG_EAX);														// push	eax
	_call((void *)mips3.memory.readlong);									// call	readlong
	_test_m32abs_imm(&scratchspace[0], 3);									// test	[scratchspace[0]],3
	_jcc_short_link(COND_Z, &link1);										// jz	link1
	_mov_m32abs_r32(&scratchspace[1], REG_EAX);								// mov	[scratchspace[1]],eax
	_add_m32bd_imm(REG_ESP, 0, 4);											// add	[esp],4
	_call((void *)mips3.memory.readlong);									// call	readlong
	_mov_r32_m32abs(REG_ECX, &scratchspace[0]);								// mov	ecx,[scratchspace[0]]
	_mov_r32_m32abs(REG_EDX, &scratchspace[1]);								// mov	edx,[scratchspace[1]]
	_shl_r32_imm(REG_ECX, 3);												// shl	ecx,3
	_shrd_r32_r32_cl(REG_EDX, REG_EAX);										// shrd edx,eax,cl
	_mov_r32_r32(REG_EAX, REG_EDX);											// mov	eax,edx
	_resolve_link(&link1);													// link1:
	_cdq();																	// cdq
	_mov_m64abs_r64(&mips3.r[rtreg], REG_EDX, REG_EAX);						// mov	[rtreg],edx:eax
	_add_r32_imm(REG_ESP, 4);												// add	esp,4
	_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
	return RECOMPILE_SUCCESSFUL_CP(2,8);
}


/*------------------------------------------------------------------
	recompile_instruction
------------------------------------------------------------------*/

static UINT32 recompile_instruction(struct drccore *drc, UINT32 pc)
{
	static UINT32 ldl_mask[] =
	{
		0x00000000,0x00000000,
		0x00000000,0x000000ff,
		0x00000000,0x0000ffff,
		0x00000000,0x00ffffff,
		0x00000000,0xffffffff,
		0x000000ff,0xffffffff,
		0x0000ffff,0xffffffff,
		0x00ffffff,0xffffffff
	};
	static UINT32 ldr_mask[] =
	{
		0x00000000,0x00000000,
		0xff000000,0x00000000,
		0xffff0000,0x00000000,
		0xffffff00,0x00000000,
		0xffffffff,0x00000000,
		0xffffffff,0xff000000,
		0xffffffff,0xffff0000,
		0xffffffff,0xffffff00
	};
	static UINT32 sdl_mask[] =
	{
		0x00000000,0x00000000,
		0xff000000,0x00000000,
		0xffff0000,0x00000000,
		0xffffff00,0x00000000,
		0xffffffff,0x00000000,
		0xffffffff,0xff000000,
		0xffffffff,0xffff0000,
		0xffffffff,0xffffff00
	};
	static UINT32 sdr_mask[] =
	{
		0x00000000,0x00000000,
		0x00000000,0x000000ff,
		0x00000000,0x0000ffff,
		0x00000000,0x00ffffff,
		0x00000000,0xffffffff,
		0x000000ff,0xffffffff,
		0x0000ffff,0xffffffff,
		0x00ffffff,0xffffffff
	};
	struct linkdata link1, link2, link3;
	UINT32 op = cpu_readop32(pc);
	int cycles;

	switch (op >> 26)
	{
		case 0x00:	/* SPECIAL */
			return recompile_special(drc, pc, op);

		case 0x01:	/* REGIMM */
			return recompile_regimm(drc, pc, op);

		case 0x02:	/* J */
			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, (pc & 0xf0000000) | (LIMMVAL << 2), 1+cycles);// <branch or dispatch>
			return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;

		case 0x03:	/* JAL */
			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								// mov	[31],pc + 8
			append_branch_or_dispatch(drc, (pc & 0xf0000000) | (LIMMVAL << 2), 1+cycles);// <branch or dispatch>
			return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;

		case 0x04:	/* BEQ */
			if (RSREG == RTREG)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else if (RSREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// mov	eax,[rtreg].lo
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// or	eax,[rtreg].hi
				_jcc_near_link(COND_NZ, &link1);									// jnz	skip
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						// mov	eax,[rsreg].lo
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						// or	eax,[rsreg].hi
				_jcc_near_link(COND_NZ, &link1);									// jnz	skip
			}
			else
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						// mov	eax,[rsreg].lo
				_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// cmp	eax,[rtreg].lo
				_jcc_near_link(COND_NE, &link1);									// jne	skip
				_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						// mov	eax,[rsreg].hi
				_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// cmp	eax,[rtreg].hi
				_jcc_near_link(COND_NE, &link2);									// jne	skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			if (RSREG != 0 && RTREG != 0)
				_resolve_link(&link2);												// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x05:	/* BNE */
			if (RSREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// mov	eax,[rtreg].lo
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// or	eax,[rtreg].hi
				_jcc_near_link(COND_Z, &link1);										// jz	skip
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						// mov	eax,[rsreg].lo
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						// or	eax,[rsreg].hi
				_jcc_near_link(COND_Z, &link1);										// jz	skip
			}
			else
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						// mov	eax,[rsreg].lo
				_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// cmp	eax,[rtreg].lo
				_jcc_short_link(COND_NE, &link2);									// jne	takeit
				_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						// mov	eax,[rsreg].hi
				_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// cmp	eax,[rtreg].hi
				_jcc_near_link(COND_E, &link1);										// je	skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x06:	/* BLEZ */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_G, &link1);										// jg	skip
				_jcc_short_link(COND_L, &link2);									// jl	takeit
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), 0);							// cmp	[rsreg].lo,0
				_jcc_near_link(COND_NE, &link3);									// jne	skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			_resolve_link(&link3);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x07:	/* BGTZ */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl	skip
				_jcc_short_link(COND_G, &link2);									// jg	takeit
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), 0);							// cmp	[rsreg].lo,0
				_jcc_near_link(COND_E, &link3);										// je	skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			_resolve_link(&link3);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x08:	/* ADDI */
			if (RSREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo	generate_overflow_exception
				if (RTREG != 0)
				{
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				// mov	[rtreg],edx:eax
				}
			}
			else if (RTREG != 0)
				_mov_m64abs_imm32(&mips3.r[RTREG], SIMMVAL);						// mov	[rtreg],const
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x09:	/* ADDIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						// mov	eax,[rsreg]
					_add_r32_imm(REG_EAX, SIMMVAL);									// add	eax,SIMMVAL
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				// mov	[rtreg],edx:eax
				}
				else
					_mov_m64abs_imm32(&mips3.r[RTREG], SIMMVAL);					// mov	[rtreg],const
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0a:	/* SLTI */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				// mov	edx:eax,[rsreg]
					_sub_r32_imm(REG_EAX, SIMMVAL);									// sub	eax,[rtreg].lo
					_sbb_r32_imm(REG_EDX, ((INT32)SIMMVAL >> 31));					// sbb	edx,[rtreg].lo
					_shr_r32_imm(REG_EDX, 31);										// shr	edx,31
					_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_EDX);					// mov	[rdreg].lo,edx
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						// mov	[rdreg].hi,0
				}
				else
				{
					_mov_m32abs_imm(LO(&mips3.r[RTREG]), (0 < SIMMVAL));			// mov	[rtreg].lo,const
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						// mov	[rtreg].hi,sign-extend(const)
				}
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0b:	/* SLTIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_xor_r32_r32(REG_ECX, REG_ECX);									// xor	ecx,ecx
					_cmp_m32abs_imm(HI(&mips3.r[RSREG]), ((INT32)SIMMVAL >> 31));	// cmp	[rsreg].hi,upper
					_jcc_short_link(COND_B, &link1);								// jb	takeit
					_jcc_short_link(COND_A, &link2);								// ja	skip
					_cmp_m32abs_imm(LO(&mips3.r[RSREG]), SIMMVAL);					// cmp	[rsreg].lo,lower
					_jcc_short_link(COND_AE, &link3);								// jae	skip
					_resolve_link(&link1);											// takeit:
					_add_r32_imm(REG_ECX, 1);										// add	ecx,1
					_resolve_link(&link2);											// skip:
					_resolve_link(&link3);											// skip:
					_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_ECX);					// mov	[rtreg].lo,ecx
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						// mov	[rtreg].hi,sign-extend(const)
				}
				else
				{
					_mov_m32abs_imm(LO(&mips3.r[RTREG]), (0 < SIMMVAL));			// mov	[rtreg].lo,const
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						// mov	[rtreg].hi,sign-extend(const)
				}
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0c:	/* ANDI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
				{
					_and_m32abs_imm(&mips3.r[RTREG], UIMMVAL);						// and	[rtreg],UIMMVAL
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						// mov	[rtreg].hi,0
				}
				else if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						// mov	eax,[rsreg].lo
					_and_r32_imm(REG_EAX, UIMMVAL);									// and	eax,UIMMVAL
					_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_EAX);					// mov	[rtreg].lo,eax
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						// mov	[rtreg].hi,0
				}
				else
					_mov_m64abs_imm32(&mips3.r[RTREG], 0);							// mov	[rtreg],0
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0d:	/* ORI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
					_or_m32abs_imm(&mips3.r[RTREG], UIMMVAL);						// or	[rtreg],UIMMVAL
				else if (RSREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				// mov	edx:eax,[rsreg]
					_or_r32_imm(REG_EAX, UIMMVAL);									// or	eax,UIMMVAL
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				// mov	[rtreg],edx:eax
				}
				else
					_mov_m64abs_imm32(&mips3.r[RTREG], UIMMVAL);					// mov	[rtreg],const
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0e:	/* XORI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
					_xor_m32abs_imm(&mips3.r[RTREG], UIMMVAL);						// xor	[rtreg],UIMMVAL
				else if (RSREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				// mov	edx:eax,[rsreg]
					_xor_r32_imm(REG_EAX, UIMMVAL);									// xor	eax,UIMMVAL
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				// mov	[rtreg],edx:eax
				}
				else
					_mov_m64abs_imm32(&mips3.r[RTREG], UIMMVAL);					// mov	[rtreg],const
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0f:	/* LUI */
			if (RTREG != 0)
				return recompile_lui(drc, pc, op);
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x10:	/* COP0 */
			return recompile_cop0(drc, pc, op) | RECOMPILE_MAY_CAUSE_EXCEPTION;
			
		case 0x11:	/* COP1 */
			return recompile_cop1(drc, pc, op) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x12:	/* COP2 */
			_jmp((void *)mips3.generate_invalidop_exception);						// jmp	generate_invalidop_exception
			return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;

		case 0x13:	/* COP1X - R5000 */
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					// jmp	generate_invalidop_exception
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			return recompile_cop1x(drc, pc, op) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x14:	/* BEQL */
			if (RSREG == RTREG)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else if (RSREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// mov	eax,[rtreg].lo
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// or	eax,[rtreg].hi
				_jcc_near_link(COND_NZ, &link1);									// jnz	skip
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						// mov	eax,[rsreg].lo
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						// or	eax,[rsreg].hi
				_jcc_near_link(COND_NZ, &link1);									// jnz	skip
			}
			else
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						// mov	eax,[rsreg].lo
				_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// cmp	eax,[rtreg].lo
				_jcc_near_link(COND_NE, &link1);									// jne	skip
				_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						// mov	eax,[rsreg].hi
				_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// cmp	eax,[rtreg].hi
				_jcc_near_link(COND_NE, &link2);									// jne	skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			if (RSREG != 0 && RTREG != 0)
				_resolve_link(&link2);												// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x15:	/* BNEL */
			if (RSREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// mov	eax,[rtreg].lo
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// or	eax,[rtreg].hi
				_jcc_near_link(COND_Z, &link1);										// jz	skip
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						// mov	eax,[rsreg].lo
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						// or	eax,[rsreg].hi
				_jcc_near_link(COND_Z, &link1);										// jz	skip
			}
			else
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						// mov	eax,[rsreg].lo
				_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// cmp	eax,[rtreg].lo
				_jcc_short_link(COND_NE, &link2);									// jne	takeit
				_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						// mov	eax,[rsreg].hi
				_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// cmp	eax,[rtreg].hi
				_jcc_near_link(COND_E, &link1);										// je	skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x16:	/* BLEZL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_G, &link1);										// jg	skip
				_jcc_short_link(COND_L, &link2);									// jl	takeit
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), 0);							// cmp	[rsreg].lo,0
				_jcc_near_link(COND_NE, &link3);									// jne	skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			_resolve_link(&link3);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x17:	/* BGTZL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,8);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl	skip
				_jcc_short_link(COND_G, &link2);									// jg	takeit
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), 0);							// cmp	[rsreg].lo,0
				_jcc_near_link(COND_E, &link3);										// je	skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			_resolve_link(&link3);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x18:	/* DADDI */
			if (RSREG != 0)
			{
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);					// mov	edx:eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_adc_r32_imm(REG_EDX, (SIMMVAL < 0) ? -1 : 0);						// adc	edx,signext(SIMMVAL)
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo	generate_overflow_exception
				if (RTREG != 0)
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				// mov	[rtreg],edx:eax
			}
			else if (RTREG != 0)
				_mov_m64abs_imm32(&mips3.r[RTREG], SIMMVAL);						// mov	[rtreg],const
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x19:	/* DADDIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				// mov	edx:eax,[rsreg]
					_add_r32_imm(REG_EAX, SIMMVAL);									// add	eax,SIMMVAL
					_adc_r32_imm(REG_EDX, (SIMMVAL < 0) ? -1 : 0);					// adc	edx,signext(SIMMVAL)
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				// mov	[rtreg],edx:eax
				}
				else
					_mov_m64abs_imm32(&mips3.r[RTREG], SIMMVAL);					// mov	[rtreg],const
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x1a:	/* LDL */
			if (!mips3.bigendian)
			{
				UINT32 nextop = cpu_readop32(pc + 4);
				if ((nextop >> 26) == 0x1b &&
					(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
					(INT16)nextop == SIMMVAL - 7)
					return recompile_ldlr_le(drc, RTREG, RSREG, SIMMVAL - 7);
			}
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
			_push_r32(REG_EAX);														// push	eax
			_and_r32_imm(REG_EAX, ~7);												// and	eax,~7
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_push_r32(REG_EAX);														// push	eax
			_mov_r32_m32bd(REG_EAX, REG_ESP, 4);									// mov	eax,[esp+4]
			_add_r32_imm(REG_EAX, 4);												// add	eax,4
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			if (mips3.bigendian)
				_pop_r32(REG_EDX);													// pop	edx
			else
			{
				_mov_r32_r32(REG_EDX, REG_EAX);										// mov	edx,eax
				_pop_r32(REG_EAX);													// pop	eax
			}
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_pop_r32(REG_ECX);														// pop	ecx
			
			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 7);											// and	ecx,7
				_shl_r32_imm(REG_ECX, 3);											// shl	ecx,3
				if (!mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x38);									// xor	ecx,0x38
				_test_r32_imm(REG_ECX, 0x20);										// test	ecx,0x20
				_jcc_short_link(COND_Z, &link1);									// jz	skip
				_mov_r32_r32(REG_EDX, REG_EAX);										// mov	edx,eax
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor	eax,eax
				_resolve_link(&link1);												// skip:
				_shld_r32_r32_cl(REG_EDX, REG_EAX);									// shld	edx,eax,cl
				_shl_r32_cl(REG_EAX);												// shl	eax,cl
				_mov_r32_m32abs(REG_EBX, LO(&mips3.r[RTREG]));						// mov	ebx,[rtreg].lo
				_and_r32_m32bd(REG_EBX, REG_ECX, ldl_mask + 1);						// and	ebx,[ldl_mask + ecx + 4]
				_or_r32_r32(REG_EAX, REG_EBX);										// or	eax,ebx
				_mov_r32_m32abs(REG_EBX, HI(&mips3.r[RTREG]));						// mov	ebx,[rtreg].hi
				_and_r32_m32bd(REG_EBX, REG_ECX, ldl_mask);							// and	ebx,[ldl_mask + ecx]
				_or_r32_r32(REG_EDX, REG_EBX);										// or	edx,ebx
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x1b:	/* LDR */
			if (!mips3.bigendian)
			{
				UINT32 nextop = cpu_readop32(pc + 4);
				if ((nextop >> 26) == 0x1a &&
					(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
					(INT16)nextop == SIMMVAL + 7)
					return recompile_ldlr_le(drc, RTREG, RSREG, SIMMVAL);
			}
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
			_push_r32(REG_EAX);														// push	eax
			_and_r32_imm(REG_EAX, ~7);												// and	eax,~7
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_push_r32(REG_EAX);														// push	eax
			_mov_r32_m32bd(REG_EAX, REG_ESP, 4);									// mov	eax,[esp+4]
			_add_r32_imm(REG_EAX, 4);												// add	eax,4
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			if (mips3.bigendian)
				_pop_r32(REG_EDX);													// pop	edx
			else
			{
				_mov_r32_r32(REG_EDX, REG_EAX);										// mov	edx,eax
				_pop_r32(REG_EAX);													// pop	eax
			}
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_pop_r32(REG_ECX);														// pop	ecx
			
			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 7);											// and	ecx,7
				_shl_r32_imm(REG_ECX, 3);											// shl	ecx,3
				if (mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x38);									// xor	ecx,0x38
				_test_r32_imm(REG_ECX, 0x20);										// test	ecx,0x20
				_jcc_short_link(COND_Z, &link1);									// jz	skip
				_mov_r32_r32(REG_EAX, REG_EDX);										// mov	eax,edx
				_xor_r32_r32(REG_EDX, REG_EDX);										// xor	edx,edx
				_resolve_link(&link1);												// skip:
				_shrd_r32_r32_cl(REG_EAX, REG_EDX);									// shrd	eax,edx,cl
				_shr_r32_cl(REG_EDX);												// shr	edx,cl
				_mov_r32_m32abs(REG_EBX, LO(&mips3.r[RTREG]));						// mov	ebx,[rtreg].lo
				_and_r32_m32bd(REG_EBX, REG_ECX, ldr_mask + 1);						// and	ebx,[ldr_mask + ecx + 4]
				_or_r32_r32(REG_EAX, REG_EBX);										// or	eax,ebx
				_mov_r32_m32abs(REG_EBX, HI(&mips3.r[RTREG]));						// mov	ebx,[rtreg].hi
				_and_r32_m32bd(REG_EBX, REG_ECX, ldr_mask);							// and	ebx,[ldr_mask + ecx]
				_or_r32_r32(REG_EDX, REG_EBX);										// or	edx,ebx
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x20:	/* LB */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.readbyte);											// call	readbyte
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_movsx_r32_r8(REG_EAX, REG_AL);										// movsx eax,al
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x21:	/* LH */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.readword);											// call	readword
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_movsx_r32_r16(REG_EAX, REG_AX);									// movsx eax,ax
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x22:	/* LWL */
			if (!mips3.bigendian)
			{
				UINT32 nextop = cpu_readop32(pc + 4);
				if ((nextop >> 26) == 0x26 &&
					(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
					(INT16)nextop == SIMMVAL - 3)
					return recompile_lwlr_le(drc, RTREG, RSREG, SIMMVAL - 3);
			}
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
			_push_r32(REG_EAX);														// push	eax
			_and_r32_imm(REG_EAX, ~3);												// and	eax,~3
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_pop_r32(REG_ECX);														// pop	ecx
			
			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 3);											// and	ecx,3
				_shl_r32_imm(REG_ECX, 3);											// shl	ecx,3
				if (!mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x18);									// xor	ecx,0x18
				_shl_r32_cl(REG_EAX);												// shl	eax,cl
				_mov_r32_m32abs(REG_EBX, LO(&mips3.r[RTREG]));						// mov	ebx,[rtreg].lo
				_and_r32_m32bd(REG_EBX, REG_ECX, ldl_mask + 1);						// and	ebx,[ldl_mask + ecx + 4]
				_or_r32_r32(REG_EAX, REG_EBX);										// or	eax,ebx
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);									// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x23:	/* LW */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.readlong);											// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x24:	/* LBU */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.readbyte);											// call	readbyte
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_and_r32_imm(REG_EAX, 0xff);										// and	eax,0xff
				_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);							// mov	[rtreg].hi,0
				_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_EAX);						// mov	[rtreg].lo,eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x25:	/* LHU */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.readword);											// call	readword
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_and_r32_imm(REG_EAX, 0xffff);										// and	eax,0xffff
				_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);							// mov	[rtreg].hi,0
				_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_EAX);						// mov	[rtreg].lo,eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x26:	/* LWR */
			if (!mips3.bigendian)
			{
				UINT32 nextop = cpu_readop32(pc + 4);
				if ((nextop >> 26) == 0x22 &&
					(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
					(INT16)nextop == SIMMVAL + 3)
					return recompile_lwlr_le(drc, RTREG, RSREG, SIMMVAL);
			}
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
			_push_r32(REG_EAX);														// push	eax
			_and_r32_imm(REG_EAX, ~3);												// and	eax,~3
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_pop_r32(REG_ECX);														// pop	ecx
			
			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 3);											// and	ecx,3
				_shl_r32_imm(REG_ECX, 3);											// shl	ecx,3
				if (mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x18);									// xor	ecx,0x18
				_shr_r32_cl(REG_EAX);												// shr	eax,cl
				_mov_r32_m32abs(REG_EBX, LO(&mips3.r[RTREG]));						// mov	ebx,[rtreg].lo
				_and_r32_m32bd(REG_EBX, REG_ECX, ldr_mask);							// and	ebx,[ldr_mask + ecx]
				_or_r32_r32(REG_EAX, REG_EBX);										// or	eax,ebx
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x27:	/* LWU */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.readlong);											// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);							// mov	[rtreg].hi,0
				_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_EAX);						// mov	[rtreg].lo,eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x28:	/* SB */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RTREG != 0)
				_push_m32abs(&mips3.r[RTREG]);										// push	dword [rtreg]
			else
				_push_imm(0);														// push	0
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.writebyte);											// call	writebyte
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x29:	/* SH */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RTREG != 0)
				_push_m32abs(&mips3.r[RTREG]);										// push	dword [rtreg]
			else
				_push_imm(0);														// push	0
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.writeword);											// call	writeword
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2a:	/* SWL */
/*{
UINT32 nextop = cpu_readop32(pc + 4);
if ((nextop >> 26) == 0x2e &&
	(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
	(INT16)nextop == SIMMVAL - 3)
	_add_m32abs_imm(&swlr_hits, 1);
}*/
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
			_push_r32(REG_EAX);														// push	eax
			_and_r32_imm(REG_EAX, ~3);												// and	eax,~3
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_mov_r32_m32bd(REG_ECX, REG_ESP, 0);									// mov	ecx,[esp]
			
			_and_r32_imm(REG_ECX, 3);												// and	ecx,3
			_shl_r32_imm(REG_ECX, 3);												// shl	ecx,3
			if (!mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x18);										// xor	ecx,0x18
			
			_and_r32_m32bd(REG_EAX, REG_ECX, sdl_mask);								// and	eax,[sdl_mask + ecx]

			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EBX, &mips3.r[RTREG]);							// mov	ebx,[rtreg]
				_shr_r32_cl(REG_EBX);												// shr	ebx,cl
				_or_r32_r32(REG_EAX, REG_EBX);										// or	eax,ebx
			}
			
			_pop_r32(REG_EBX);														// pop	ebx
			_and_r32_imm(REG_EBX, ~3);												// and	ebx,~3
			_push_r32(REG_EAX);														// push	eax
			_push_r32(REG_EBX);														// push	ebx
			_call((void *)mips3.memory.writelong);									// call	writelong
			_add_r32_imm(REG_ESP, 8);												// add	esp,8

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2b:	/* SW */
/*			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			if (SIMMVAL != 0)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
			_mov_r32_r32(REG_EBX, REG_EAX);											// mov	ebx,eax
			_shr_r32_imm(REG_EBX, 16);												// shr	ebx,16
			_mov_r32_m32isd(REG_EBX, REG_EBX, 4, ram_write_table);					// mov	ebx,[ebx*4 + ram_write_table]
			_cmp_r32_imm(REG_EBX, 0);												// cmp	ebx,0
			_jcc_short_link(COND_NE, &link1);										// jne	fast
			if (RTREG != 0)
				_push_m32abs(&mips3.r[RTREG]);										// push	dword [rtreg]
			else
				_push_imm(0);														// push	0
			_push_r32(REG_EAX);														// push	eax
			drc_append_save_call_restore(drc, (void *)mips3.memory.writelong, 8);	// call	writelong
			_jmp_short_link(&link2);												// jmp	done
			_resolve_link(&link1);													// fast:
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_ECX, &mips3.r[RTREG]);							// mov  ecx,[rtreg]
				_mov_m32bisd_r32(REG_EBX, REG_EAX, 1, 0, REG_ECX);					// mov	[ebx+eax],ecx
			}
			else
				_mov_m32bisd_imm(REG_EBX, REG_EAX, 1, 0, 0);						// mov	[ebx+eax],0
			_resolve_link(&link2);													// fast:
*/

			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RTREG != 0)
				_push_m32abs(&mips3.r[RTREG]);										// push	dword [rtreg]
			else
				_push_imm(0);														// push	0
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.writelong);											// call	writelong
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x2c:	/* SDL */
/*{
UINT32 nextop = cpu_readop32(pc + 4);
if ((nextop >> 26) == 0x2d &&
	(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
	(INT16)nextop == SIMMVAL - 7)
	_add_m32abs_imm(&sdlr_hits, 1);
}*/
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
			_push_r32(REG_EAX);														// push	eax
			_and_r32_imm(REG_EAX, ~7);												// and	eax,~7
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_push_r32(REG_EAX);														// push	eax
			_mov_r32_m32bd(REG_EAX, REG_ESP, 4);									// mov	eax,[esp+4]
			_add_r32_imm(REG_EAX, 4);												// add	eax,4
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			if (mips3.bigendian)
				_pop_r32(REG_EDX);													// pop	edx
			else
			{
				_mov_r32_r32(REG_EDX, REG_EAX);										// mov	edx,eax
				_pop_r32(REG_EAX);													// pop	eax
			}
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_mov_r32_m32bd(REG_ECX, REG_ESP, 0);									// mov	ecx,[esp]
			
			_and_r32_imm(REG_ECX, 7);												// and	ecx,7
			_shl_r32_imm(REG_ECX, 3);												// shl	ecx,3
			if (!mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x38);										// xor	ecx,0x38
			
			_and_r32_m32bd(REG_EAX, REG_ECX, sdl_mask + 1);							// and	eax,[sdl_mask + ecx + 4]
			_and_r32_m32bd(REG_EDX, REG_ECX, sdl_mask);								// and	eax,[sdl_mask + ecx]

			if (RTREG != 0)
			{
				_test_r32_imm(REG_ECX, 0x20);										// test	ecx,0x20
				_mov_r64_m64abs(REG_ESI, REG_EBX, &mips3.r[RTREG]);					// mov	esi:ebx,[rtreg]
				_jcc_short_link(COND_Z, &link1);									// jz	skip
				_mov_r32_r32(REG_EBX, REG_ESI);										// mov	ebx,esi
				_xor_r32_r32(REG_ESI, REG_ESI);										// xor	esi,esi
				_resolve_link(&link1);												// skip:
				_shrd_r32_r32_cl(REG_EBX, REG_ESI);									// shrd	ebx,esi,cl
				_shr_r32_cl(REG_ESI);												// shr	esi,cl
				_or_r32_r32(REG_EAX, REG_EBX);										// or	eax,ebx
				_or_r32_r32(REG_EDX, REG_ESI);										// or	edx,esi
			}
			
			_pop_r32(REG_EBX);														// pop	ebx
			_and_r32_imm(REG_EBX, ~7);												// and	ebx,~7
			_lea_r32_m32bd(REG_ECX, REG_EBX, 4);									// lea	ecx,[ebx+4]
			_push_r32(mips3.bigendian ? REG_EAX : REG_EDX);							// push	eax/edx
			_push_r32(REG_ECX);														// push ecx
			_push_r32(mips3.bigendian ? REG_EDX : REG_EAX);							// push	edx/eax
			_push_r32(REG_EBX);														// push	ebx
			_call((void *)mips3.memory.writelong);									// call	writelong
			_add_r32_imm(REG_ESP, 8);												// add	esp,8
			_call((void *)mips3.memory.writelong);									// call	writelong
			_add_r32_imm(REG_ESP, 8);												// add	esp,8

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2d:	/* SDR */
/*{
UINT32 nextop = cpu_readop32(pc + 4);
if ((nextop >> 26) == 0x2c &&
	(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
	(INT16)nextop == SIMMVAL + 7)
	_add_m32abs_imm(&sdlr_hits, 1);
}*/
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
			_push_r32(REG_EAX);														// push	eax
			_and_r32_imm(REG_EAX, ~7);												// and	eax,~7
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_push_r32(REG_EAX);														// push	eax
			_mov_r32_m32bd(REG_EAX, REG_ESP, 4);									// mov	eax,[esp+4]
			_add_r32_imm(REG_EAX, 4);												// add	eax,4
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			if (mips3.bigendian)
				_pop_r32(REG_EDX);													// pop	edx
			else
			{
				_mov_r32_r32(REG_EDX, REG_EAX);										// mov	edx,eax
				_pop_r32(REG_EAX);													// pop	eax
			}
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_mov_r32_m32bd(REG_ECX, REG_ESP, 0);									// mov	ecx,[esp]
			
			_and_r32_imm(REG_ECX, 7);												// and	ecx,7
			_shl_r32_imm(REG_ECX, 3);												// shl	ecx,3
			if (mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x38);										// xor	ecx,0x38
			
			_and_r32_m32bd(REG_EAX, REG_ECX, sdr_mask + 1);							// and	eax,[sdr_mask + ecx + 4]
			_and_r32_m32bd(REG_EDX, REG_ECX, sdr_mask);								// and	eax,[sdr_mask + ecx]

			if (RTREG != 0)
			{
				_test_r32_imm(REG_ECX, 0x20);										// test	ecx,0x20
				_mov_r64_m64abs(REG_ESI, REG_EBX, &mips3.r[RTREG]);					// mov	esi:ebx,[rtreg]
				_jcc_short_link(COND_Z, &link1);									// jz	skip
				_mov_r32_r32(REG_ESI, REG_EBX);										// mov	esi,ebx
				_xor_r32_r32(REG_EBX, REG_EBX);										// xor	ebx,ebx
				_resolve_link(&link1);												// skip:
				_shld_r32_r32_cl(REG_ESI, REG_EBX);									// shld	esi,ebx,cl
				_shl_r32_cl(REG_EBX);												// shl	ebx,cl
				_or_r32_r32(REG_EAX, REG_EBX);										// or	eax,ebx
				_or_r32_r32(REG_EDX, REG_ESI);										// or	edx,esi
			}
			
			_pop_r32(REG_EBX);														// pop	ebx
			_and_r32_imm(REG_EBX, ~7);												// and	ebx,~7
			_lea_r32_m32bd(REG_ECX, REG_EBX, 4);									// lea	ecx,[ebx+4]
			_push_r32(mips3.bigendian ? REG_EAX : REG_EDX);							// push	eax/edx
			_push_r32(REG_ECX);														// push ecx
			_push_r32(mips3.bigendian ? REG_EDX : REG_EAX);							// push	edx/eax
			_push_r32(REG_EBX);														// push	ebx
			_call((void *)mips3.memory.writelong);									// call	writelong
			_add_r32_imm(REG_ESP, 8);												// add	esp,8
			_call((void *)mips3.memory.writelong);									// call	writelong
			_add_r32_imm(REG_ESP, 8);												// add	esp,8

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2e:	/* SWR */
/*{
UINT32 nextop = cpu_readop32(pc + 4);
if ((nextop >> 26) == 0x2a &&
	(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
	(INT16)nextop == SIMMVAL + 3)
	_add_m32abs_imm(&swlr_hits, 1);
}*/
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
			_push_r32(REG_EAX);														// push	eax
			_and_r32_imm(REG_EAX, ~3);												// and	eax,~3
			_push_r32(REG_EAX);														// push	eax
			_call((void *)mips3.memory.readlong);									// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_mov_r32_m32bd(REG_ECX, REG_ESP, 0);									// mov	ecx,[esp]
			
			_and_r32_imm(REG_ECX, 3);												// and	ecx,3
			_shl_r32_imm(REG_ECX, 3);												// shl	ecx,3
			if (mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x18);										// xor	ecx,0x18
			
			_and_r32_m32bd(REG_EAX, REG_ECX, sdr_mask + 1);							// and	eax,[sdr_mask + ecx + 4]

			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EBX, &mips3.r[RTREG]);							// mov	ebx,[rtreg]
				_shl_r32_cl(REG_EBX);												// shl	ebx,cl
				_or_r32_r32(REG_EAX, REG_EBX);										// or	eax,ebx
			}
			
			_pop_r32(REG_EBX);														// pop	ebx
			_and_r32_imm(REG_EBX, ~3);												// and	ebx,~3
			_push_r32(REG_EAX);														// push	eax
			_push_r32(REG_EBX);														// push	ebx
			_call((void *)mips3.memory.writelong);									// call	writelong
			_add_r32_imm(REG_ESP, 8);												// add	esp,8

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2f:	/* CACHE */
			return RECOMPILE_SUCCESSFUL_CP(1,4);

//		case 0x30:	/* LL */		logerror("mips3 Unhandled op: LL\n");									break;

		case 0x31:	/* LWC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.readlong);											// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_mov_m32abs_r32(&mips3.cpr[1][RTREG], REG_EAX);							// mov	[rtreg],eax
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x32:	/* LWC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.readlong);											// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_mov_m32abs_r32(&mips3.cpr[2][RTREG], REG_EAX);							// mov	[rtreg],eax
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x33:	/* PREF */
			if (mips3.is_mips4)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_jmp((void *)mips3.generate_invalidop_exception);					// jmp	generate_invalidop_exception
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}

//		case 0x34:	/* LLD */		logerror("mips3 Unhandled op: LLD\n");									break;

		case 0x35:	/* LDC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call((void *)mips3.memory.readlong);									// call	readlong
			_mov_m32abs_r32(mips3.bigendian ? HI(&mips3.cpr[1][RTREG]) : LO(&mips3.cpr[1][RTREG]), REG_EAX);// mov	[rtreg].hi/lo,eax

			_add_m32bd_imm(REG_ESP, 0, 4);											// add	[esp],4
			_call((void *)mips3.memory.readlong);									// call	readlong
			_mov_m32abs_r32(mips3.bigendian ? LO(&mips3.cpr[1][RTREG]) : HI(&mips3.cpr[1][RTREG]), REG_EAX);// mov	[rtreg].lo/hi,eax

			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x36:	/* LDC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call((void *)mips3.memory.readlong);									// call	readlong
			_mov_m32abs_r32(mips3.bigendian ? HI(&mips3.cpr[2][RTREG]) : LO(&mips3.cpr[2][RTREG]), REG_EAX);// mov	[rtreg].hi/lo,eax

			_add_m32bd_imm(REG_ESP, 0, 4);											// add	[esp],4
			_call((void *)mips3.memory.readlong);									// call	readlong
			_mov_m32abs_r32(mips3.bigendian ? LO(&mips3.cpr[2][RTREG]) : HI(&mips3.cpr[2][RTREG]), REG_EAX);// mov	[rtreg].lo/hi,eax

			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x37:	/* LD */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call((void *)mips3.memory.readlong);									// call	readlong
			if (RTREG != 0)
				_mov_m32abs_r32(mips3.bigendian ? HI(&mips3.r[RTREG]) : LO(&mips3.r[RTREG]), REG_EAX);	// mov	[rtreg].hi/lo,eax

			_add_m32bd_imm(REG_ESP, 0, 4);											// add	[esp],4
			_call((void *)mips3.memory.readlong);									// call	readlong
			if (RTREG != 0)
				_mov_m32abs_r32(mips3.bigendian ? LO(&mips3.r[RTREG]) : HI(&mips3.r[RTREG]), REG_EAX);	// mov	[rtreg].lo/hi,eax

			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

//		case 0x38:	/* SC */		logerror("mips3 Unhandled op: SC\n");									break;

		case 0x39:	/* SWC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(&mips3.cpr[1][RTREG]);										// push	dword [rtreg]
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.writelong);											// call	writelong
			_add_r32_imm(REG_ESP, 8);												// add	esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3a:	/* SWC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(&mips3.cpr[2][RTREG]);										// push	dword [rtreg]
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call(mips3.memory.writelong);											// call	writelong
			_add_r32_imm(REG_ESP, 8);												// add	esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

//		case 0x3b:	/* SWC3 */		invalid_instruction(op);												break;
//		case 0x3c:	/* SCD */		logerror("mips3 Unhandled op: SCD\n");									break;

		case 0x3d:	/* SDC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(mips3.bigendian ? HI(&mips3.cpr[1][RTREG]) : LO(&mips3.cpr[1][RTREG]));// push	dword [rtreg].lo/hi
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call((void *)mips3.memory.writelong);									// call	writelong
			
			_push_m32abs(mips3.bigendian ? LO(&mips3.cpr[1][RTREG]) : HI(&mips3.cpr[1][RTREG]));// push	dword [rtreg].hi/lo
			if (RSREG != 0 && (SIMMVAL+4) != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL+4);									// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else if ((SIMMVAL+4) != 0)
				_push_imm(SIMMVAL+4);												// push	SIMMVAL
			_call((void *)mips3.memory.writelong);									// call	writelong
			
			_add_r32_imm(REG_ESP, 16);												// add	esp,16
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3e:	/* SDC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(mips3.bigendian ? HI(&mips3.cpr[2][RTREG]) : LO(&mips3.cpr[2][RTREG]));// push	dword [rtreg].lo/hi
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call((void *)mips3.memory.writelong);									// call	writelong
			
			_push_m32abs(mips3.bigendian ? LO(&mips3.cpr[2][RTREG]) : HI(&mips3.cpr[2][RTREG]));// push	dword [rtreg].hi/lo
			if (RSREG != 0 && (SIMMVAL+4) != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL+4);									// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else if ((SIMMVAL+4) != 0)
				_push_imm(SIMMVAL+4);												// push	SIMMVAL
			_call((void *)mips3.memory.writelong);									// call	writelong
			
			_add_r32_imm(REG_ESP, 16);												// add	esp,16
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3f:	/* SD */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RTREG != 0)
				_push_m32abs(mips3.bigendian ? HI(&mips3.r[RTREG]) : LO(&mips3.r[RTREG]));// push	dword [rtreg].lo/hi
			else
				_push_imm(0);														// push	0
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else
				_push_imm(SIMMVAL);													// push	SIMMVAL
			_call((void *)mips3.memory.writelong);									// call	writelong
			
			if (RTREG != 0)
				_push_m32abs(mips3.bigendian ? LO(&mips3.r[RTREG]) : HI(&mips3.r[RTREG]));// push	dword [rtreg].hi/lo
			else
				_push_imm(0);														// push	0
			if (RSREG != 0 && (SIMMVAL+4) != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL+4);									// add	eax,SIMMVAL
				_push_r32(REG_EAX);													// push	eax
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										// push	[rsreg]
			else if ((SIMMVAL+4) != 0)
				_push_imm(SIMMVAL+4);												// push	SIMMVAL
			_call((void *)mips3.memory.writelong);									// call	writelong
			
			_add_r32_imm(REG_ESP, 16);												// add	esp,16
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);
				
//		default:	/* ??? */		invalid_instruction(op);												break;
	}
	return RECOMPILE_UNIMPLEMENTED;
}


/*------------------------------------------------------------------
	recompile_special
------------------------------------------------------------------*/

static UINT32 recompile_special(struct drccore *drc, UINT32 pc, UINT32 op)
{
	struct linkdata link1, link2, link3;
	int cycles;
	
	switch (op & 63)
	{
		case 0x00:	/* SLL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// mov	eax,[rtreg]
					if (SHIFT != 0)
						_shl_r32_imm(REG_EAX, SHIFT);								// shl	eax,SHIFT
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x01:	/* MOVF - R5000*/
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					// jmp	generate_invalidop_exception
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			_cmp_m8abs_imm(&mips3.cf[1][(op >> 18) & 7], 0);						// cmp	[cf[x]],0
			_jcc_short_link(((op >> 16) & 1) ? COND_Z : COND_NZ, &link1);			// jz/nz skip
			_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);						// mov	edx:eax,[rsreg]
			_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);						// mov	[rdreg],edx:eax
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x02:	/* SRL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// mov	eax,[rtreg]
					if (SHIFT != 0)
						_shr_r32_imm(REG_EAX, SHIFT);								// shr	eax,SHIFT
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x03:	/* SRA */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// mov	eax,[rtreg]
					if (SHIFT != 0)
						_sar_r32_imm(REG_EAX, SHIFT);								// sar	eax,SHIFT
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x04:	/* SLLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// mov	eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					// mov	ecx,[rsreg]
						_shl_r32_cl(REG_EAX);										// shl	eax,cl
					}
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x06:	/* SRLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// mov	eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					// mov	ecx,[rsreg]
						_shr_r32_cl(REG_EAX);										// shr	eax,cl
					}
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x07:	/* SRAV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// mov	eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					// mov	ecx,[rsreg]
						_sar_r32_cl(REG_EAX);										// sar	eax,cl
					}
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x08:	/* JR */
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			_mov_m32abs_r32(&jr_temp, REG_EAX);										// mov	jr_temp,eax
			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_r32_m32abs(REG_EDI, &jr_temp);										// mov	edi,jr_temp
			return RECOMPILE_SUCCESSFUL_CP(1+cycles,0) | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;

		case 0x09:	/* JALR */
			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			if (RDREG != 0)
				_mov_m64abs_imm32(&mips3.r[RDREG], pc + 8);							// mov	[rdreg],pc + 8
			_mov_r32_m32abs(REG_EDI, &mips3.r[RSREG]);								// mov	edi,[rsreg]
			return RECOMPILE_SUCCESSFUL_CP(1+cycles,0) | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;

		case 0x0a:	/* MOVZ - R5000 */
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					// jmp	generate_invalidop_exception
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			if (RDREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// mov	eax,[rtreg].lo
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// or	eax,[rtreg].hi
				_jcc_short_link(COND_NZ, &link1);									// jnz	skip
				_mov_m64abs_m64abs(&mips3.r[RDREG],  &mips3.r[RSREG]);				// mov	[rdreg],[rsreg]
				_resolve_link(&link1);												// skip:
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0b:	/* MOVN - R5000 */
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					// jmp	generate_invalidop_exception
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			if (RDREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// mov	eax,[rtreg].lo
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// or	eax,[rtreg].hi
				_jcc_short_link(COND_Z, &link1);									// jz	skip
				_mov_m64abs_m64abs(&mips3.r[RDREG],  &mips3.r[RSREG]);				// mov	[rdreg],[rsreg]
				_resolve_link(&link1);												// skip:
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0c:	/* SYSCALL */
			_jmp((void *)mips3.generate_syscall_exception);							// jmp	generate_syscall_exception
			return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;

		case 0x0d:	/* BREAK */
			_jmp((void *)mips3.generate_break_exception);							// jmp	generate_break_exception
			return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;

		case 0x0f:	/* SYNC */
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x10:	/* MFHI */
			if (RDREG != 0)
				_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.hi);						// mov	[rdreg],[hi]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x11:	/* MTHI */
			_mov_m64abs_m64abs(&mips3.hi, &mips3.r[RSREG]);							// mov	[hi],[rsreg]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x12:	/* MFLO */
			if (RDREG != 0)
				_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.lo);						// mov	[rdreg],[lo]
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x13:	/* MTLO */
			_mov_m64abs_m64abs(&mips3.lo,  &mips3.r[RSREG]);						// mov	[lo],[rsreg]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x14:	/* DSLLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);				// mov	edx:eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					// mov	ecx,[rsreg]
						_test_r32_imm(REG_ECX, 0x20);								// test	ecx,0x20
						_jcc_short_link(COND_Z, &link1);							// jz	skip
						_mov_r32_r32(REG_EDX, REG_EAX);								// mov	edx,eax
						_xor_r32_r32(REG_EAX, REG_EAX);								// xor	eax,eax
						_resolve_link(&link1);										// skip:
						_shld_r32_r32_cl(REG_EDX, REG_EAX);							// shld	edx,eax,cl
						_shl_r32_cl(REG_EAX);										// shl	eax,cl
					}
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x16:	/* DSRLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);				// mov	edx:eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					// mov	ecx,[rsreg]
						_test_r32_imm(REG_ECX, 0x20);								// test	ecx,0x20
						_jcc_short_link(COND_Z, &link1);							// jz	skip
						_mov_r32_r32(REG_EAX, REG_EDX);								// mov	eax,edx
						_xor_r32_r32(REG_EDX, REG_EDX);								// xor	edx,edx
						_resolve_link(&link1);										// skip:
						_shrd_r32_r32_cl(REG_EAX, REG_EDX);							// shrd	eax,edx,cl
						_shr_r32_cl(REG_EDX);										// shr	edx,cl
					}
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x17:	/* DSRAV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);				// mov	edx:eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					// mov	ecx,[rsreg]
						_test_r32_imm(REG_ECX, 0x20);								// test	ecx,0x20
						_jcc_short_link(COND_Z, &link1);							// jz	skip
						_mov_r32_r32(REG_EAX, REG_EDX);								// mov	eax,edx
						_cdq();														// cdq
						_resolve_link(&link1);										// skip:
						_shrd_r32_r32_cl(REG_EAX, REG_EDX);							// shrd	eax,edx,cl
						_sar_r32_cl(REG_EDX);										// sar	edx,cl
					}
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x18:	/* MULT */
			_mov_r32_m32abs(REG_ECX, &mips3.r[RTREG]);								// mov	ecx,[rtreg].lo
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg].lo
			_imul_r32(REG_ECX);														// imul	ecx
			_push_r32(REG_EDX);														// push	edx
			_cdq();																	// cdq
			_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);							// mov	[lo],edx:eax
			_pop_r32(REG_EAX);														// pop	eax
			_cdq();																	// cdq
			_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);							// mov	[hi],edx:eax
			return RECOMPILE_SUCCESSFUL_CP(4,4);

		case 0x19:	/* MULTU */
			_mov_r32_m32abs(REG_ECX, &mips3.r[RTREG]);								// mov	ecx,[rtreg].lo
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg].lo
			_mul_r32(REG_ECX);														// mul	ecx
			_push_r32(REG_EDX);														// push	edx
			_cdq();																	// cdq
			_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);							// mov	[lo],edx:eax
			_pop_r32(REG_EAX);														// pop	eax
			_cdq();																	// cdq
			_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);							// mov	[hi],edx:eax
			return RECOMPILE_SUCCESSFUL_CP(4,4);

		case 0x1a:	/* DIV */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_ECX, &mips3.r[RTREG]);							// mov	ecx,[rtreg].lo
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg].lo
				_cdq();																// cdq
				_cmp_r32_imm(REG_ECX, 0);											// cmp	ecx,0
				_jcc_short_link(COND_E, &link1);									// je	skip
				_idiv_r32(REG_ECX);													// idiv	ecx
				_push_r32(REG_EDX);													// push	edx
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);						// mov	[lo],edx:eax
				_pop_r32(REG_EAX);													// pop	eax
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);						// mov	[hi],edx:eax
				_resolve_link(&link1);												// skip:
			}
			return RECOMPILE_SUCCESSFUL_CP(36,4);
			
		case 0x1b:	/* DIVU */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_ECX, &mips3.r[RTREG]);							// mov	ecx,[rtreg].lo
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg].lo
				_xor_r32_r32(REG_EDX, REG_EDX);										// xor	edx,edx
				_cmp_r32_imm(REG_ECX, 0);											// cmp	ecx,0
				_jcc_short_link(COND_E, &link1);									// je	skip
				_div_r32(REG_ECX);													// div	ecx
				_push_r32(REG_EDX);													// push	edx
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);						// mov	[lo],edx:eax
				_pop_r32(REG_EAX);													// pop	eax
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);						// mov	[hi],edx:eax
				_resolve_link(&link1);												// skip:
			}
			return RECOMPILE_SUCCESSFUL_CP(36,4);

		case 0x1c:	/* DMULT */
			_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);						// mov	edx:eax,[rsreg]
			_cmp_r32_imm(REG_EDX, 0);												// cmp	edx,0
			_jcc_short_link(COND_GE, &link1);										// jge	skip1
			_mov_r32_r32(REG_ECX, REG_EDX);											// mov	ecx,edx
			_xor_r32_r32(REG_EDX, REG_EDX);											// xor	edx,edx
			_neg_r32(REG_EAX);														// neg	eax
			_sbb_r32_r32(REG_EDX, REG_ECX);											// sbb	edx,ecx
			_resolve_link(&link1);													// skip1:
			_mov_m64abs_r64(&dmult_temp1, REG_EDX, REG_EAX);						// mov	[dmult_temp1],edx:eax
		
			_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);						// mov	edx:eax,[rtreg]
			_cmp_r32_imm(REG_EDX, 0);												// cmp	edx,0
			_jcc_short_link(COND_GE, &link2);										// jge	skip2
			_mov_r32_r32(REG_ECX, REG_EDX);											// mov	ecx,edx
			_xor_r32_r32(REG_EDX, REG_EDX);											// xor	edx,edx
			_neg_r32(REG_EAX);														// neg	eax
			_sbb_r32_r32(REG_EDX, REG_ECX);											// sbb	edx,ecx
			_resolve_link(&link2);													// skip2:
			_mov_m64abs_r64(&dmult_temp2, REG_EDX, REG_EAX);						// mov	[dmult_temp2],edx:eax
		
			_mov_r32_m32abs(REG_EAX, LO(&dmult_temp1));								// mov	eax,[dmult_temp1].lo
			_mul_m32abs(LO(&dmult_temp2));											// mul	[dmult_temp2].lo
			_mov_r32_r32(REG_ECX, REG_EDX);											// mov	ecx,edx
			_xor_r32_r32(REG_EBX, REG_EBX);											// xor	ebx,ebx
			_mov_m32abs_r32(LO(&mips3.lo), REG_EAX);								// mov	[lo].lo,eax

			_mov_r32_m32abs(REG_EAX, HI(&dmult_temp1));								// mov	eax,[dmult_temp1].hi
			_mul_m32abs(LO(&dmult_temp2));											// mul	[dmult_temp2].lo
			_add_r32_r32(REG_ECX, REG_EAX);											// add	ecx,eax
			_adc_r32_r32(REG_EBX, REG_EDX);											// adc	ebx,edx
			
			_mov_r32_m32abs(REG_EAX, LO(&dmult_temp1));								// mov	eax,[dmult_temp1].lo
			_mul_m32abs(HI(&dmult_temp2));											// mul	[dmult_temp2].hi
			_add_r32_r32(REG_ECX, REG_EAX);											// add	ecx,eax
			_adc_r32_r32(REG_EBX, REG_EDX);											// adc	ebx,edx
			_mov_m32abs_r32(HI(&mips3.lo), REG_ECX);								// mov	[lo].hi,ecx
			
			_mov_r32_m32abs(REG_EAX, HI(&dmult_temp1));								// mov	eax,[dmult_temp1].hi
			_mul_m32abs(HI(&dmult_temp2));											// mul	[dmult_temp2].hi
			_add_r32_r32(REG_EBX, REG_EAX);											// add	ebx,eax
			_adc_r32_imm(REG_EDX, 0);												// adc	edx,0
			_mov_m32abs_r32(LO(&mips3.hi), REG_EBX);								// mov	[hi].lo,ebx
			_mov_m32abs_r32(HI(&mips3.hi), REG_EDX);								// mov	[hi].hi,edx
			
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							// mov	eax,[rsreg].hi
			_xor_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));							// xor	eax,[rtreg].hi
			_jcc_short_link(COND_NS, &link3);										// jns	noflip
			_xor_r32_r32(REG_EAX, REG_EAX);											// xor	eax,eax
			_xor_r32_r32(REG_EBX, REG_EBX);											// xor	ebx,ebx
			_xor_r32_r32(REG_ECX, REG_ECX);											// xor	ecx,ecx
			_xor_r32_r32(REG_EDX, REG_EDX);											// xor	edx,edx
			_sub_r32_m32abs(REG_EAX, LO(&mips3.lo));								// sub	eax,[lo].lo
			_sbb_r32_m32abs(REG_EBX, HI(&mips3.lo));								// sbb	ebx,[lo].hi
			_sbb_r32_m32abs(REG_ECX, LO(&mips3.hi));								// sbb	ecx,[hi].lo
			_sbb_r32_m32abs(REG_EDX, HI(&mips3.hi));								// sbb	edx,[hi].hi
			_mov_m64abs_r64(&mips3.lo, REG_EBX, REG_EAX);							// mov	[lo],ebx:eax
			_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_ECX);							// mov	[lo],edx:ecx
			_resolve_link(&link3);													// noflip:
			return RECOMPILE_SUCCESSFUL_CP(8,4);

		case 0x1d:	/* DMULTU */
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							// mov	eax,[rsreg].lo
			_mul_m32abs(LO(&mips3.r[RTREG]));										// mul	[rtreg].lo
			_mov_r32_r32(REG_ECX, REG_EDX);											// mov	ecx,edx
			_xor_r32_r32(REG_EBX, REG_EBX);											// xor	ebx,ebx
			_mov_m32abs_r32(LO(&mips3.lo), REG_EAX);								// mov	[lo].lo,eax

			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							// mov	eax,[rsreg].hi
			_mul_m32abs(LO(&mips3.r[RTREG]));										// mul	[rtreg].lo
			_add_r32_r32(REG_ECX, REG_EAX);											// add	ecx,eax
			_adc_r32_r32(REG_EBX, REG_EDX);											// adc	ebx,edx
			
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							// mov	eax,[rsreg].lo
			_mul_m32abs(HI(&mips3.r[RTREG]));										// mul	[rtreg].hi
			_add_r32_r32(REG_ECX, REG_EAX);											// add	ecx,eax
			_adc_r32_r32(REG_EBX, REG_EDX);											// adc	ebx,edx
			_mov_m32abs_r32(HI(&mips3.lo), REG_ECX);								// mov	[lo].hi,ecx
			
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							// mov	eax,[rsreg].hi
			_mul_m32abs(HI(&mips3.r[RTREG]));										// mul	[rtreg].hi
			_add_r32_r32(REG_EBX, REG_EAX);											// add	ebx,eax
			_adc_r32_imm(REG_EDX, 0);												// adc	edx,0
			_mov_m32abs_r32(LO(&mips3.hi), REG_EBX);								// mov	[hi].lo,ebx
			_mov_m32abs_r32(HI(&mips3.hi), REG_EDX);								// mov	[hi].hi,edx
			return RECOMPILE_SUCCESSFUL_CP(8,4);
					
		case 0x1e:	/* DDIV */
			_push_imm(&mips3.r[RTREG]);												// push	[rtreg]
			_push_imm(&mips3.r[RSREG]);												// push	[rsreg]
			_call((void *)ddiv);													// call ddiv
			_add_r32_imm(REG_ESP, 8);												// add	esp,8
			return RECOMPILE_SUCCESSFUL_CP(68,4);

		case 0x1f:	/* DDIVU */
			_push_imm(&mips3.r[RTREG]);												// push	[rtreg]
			_push_imm(&mips3.r[RSREG]);												// push	[rsreg]
			_call((void *)ddivu);													// call ddivu
			_add_r32_imm(REG_ESP, 8);												// add	esp,8
			return RECOMPILE_SUCCESSFUL_CP(68,4);

		case 0x20:	/* ADD */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							// add	eax,[rtreg]
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo	generate_overflow_exception
				if (RDREG != 0)
				{
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						// mov	eax,[rsreg]
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// mov	eax,[rtreg]
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x21:	/* ADDU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						// mov	eax,[rsreg]
					_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// add	eax,[rtreg]
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						// mov	eax,[rsreg]
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// mov	eax,[rtreg]
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x22:	/* SUB */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							// mov	eax,[rsreg]
				_sub_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							// sub	eax,[rtreg]
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo	generate_overflow_exception
				if (RDREG != 0)
				{
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						// mov	eax,[rsreg]
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// mov	eax,[rtreg]
					_neg_r32(REG_EAX);												// neg	eax
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x23:	/* SUBU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						// mov	eax,[rsreg]
					_sub_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// sub	eax,[rtreg]
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						// mov	eax,[rsreg]
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						// mov	eax,[rtreg]
					_neg_r32(REG_EAX);												// neg	eax
					_cdq();															// cdq
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x24:	/* AND */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RSREG]);				// movsd xmm0,[rsreg]
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				// movsd xmm1,[rtreg]
						_pand_r128_r128(REG_XMM0, REG_XMM1);						// pand	xmm0,xmm1
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// mov	[rdreg],xmm0
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);			// mov	edx:eax,[rsreg]
						_and_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				// and	eax,[rtreg].lo
						_and_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				// and	edx,[rtreg].hi
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			// mov	[rdreg],edx:eax
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x25:	/* OR */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RSREG]);				// movsd xmm0,[rsreg]
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				// movsd xmm1,[rtreg]
						_por_r128_r128(REG_XMM0, REG_XMM1);							// por	xmm0,xmm1
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// mov	[rdreg],xmm0
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);			// mov	edx:eax,[rsreg]
						_or_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				// or	eax,[rtreg].lo
						_or_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				// or	edx,[rtreg].hi
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			// mov	[rdreg],edx:eax
					}
				}
				else if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			// mov	[rdreg],[rsreg]
				else if (RTREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RTREG]);			// mov	[rdreg],[rtreg]
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x26:	/* XOR */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RSREG]);				// movsd xmm0,[rsreg]
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				// movsd xmm1,[rtreg]
						_pxor_r128_r128(REG_XMM0, REG_XMM1);						// pxor	xmm0,xmm1
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// mov	[rdreg],xmm0
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);			// mov	edx:eax,[rsreg]
						_xor_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				// xor	eax,[rtreg].lo
						_xor_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				// xor	edx,[rtreg].hi
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			// mov	[rdreg],edx:eax
					}
				}
				else if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			// mov	[rdreg],[rsreg]
				else if (RTREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RTREG]);			// mov	[rdreg],[rtreg]
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x27:	/* NOR */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				// mov	edx:eax,[rsreg]
					_or_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));					// or	eax,[rtreg].lo
					_or_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));					// or	edx,[rtreg].hi
					_not_r32(REG_EDX);												// not	edx
					_not_r32(REG_EAX);												// not	eax
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else if (RSREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				// mov	edx:eax,[rsreg]
					_not_r32(REG_EDX);												// not	edx
					_not_r32(REG_EAX);												// not	eax
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else if (RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);				// mov	edx:eax,[rtreg]
					_not_r32(REG_EDX);												// not	edx
					_not_r32(REG_EAX);												// not	eax
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
				{
					_mov_m32abs_imm(LO(&mips3.r[RDREG]), ~0);						// mov	[rtreg].lo,~0
					_mov_m32abs_imm(HI(&mips3.r[RDREG]), ~0);						// mov	[rtreg].hi,~0
				}
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2a:	/* SLT */
			if (RDREG != 0)
			{
				if (RSREG != 0)
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				// mov	edx:eax,[rsreg]
				else
				{
					_xor_r32_r32(REG_EDX, REG_EDX);									// xor	edx,edx
					_xor_r32_r32(REG_EAX, REG_EAX);									// xor	eax,eax
				}
				if (RTREG != 0)
				{
					_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));					// sub	eax,[rtreg].lo
					_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));					// sbb	edx,[rtreg].lo
				}
				_shr_r32_imm(REG_EDX, 31);											// shr	edx,31
				_mov_m32abs_r32(LO(&mips3.r[RDREG]), REG_EDX);						// mov	[rdreg].lo,edx
				_mov_m32abs_imm(HI(&mips3.r[RDREG]), 0);							// mov	[rdreg].hi,0
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x2b:	/* SLTU */
			if (RDREG != 0)
			{
				_xor_r32_r32(REG_ECX, REG_ECX);										// xor	ecx,ecx
				_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						// mov	eax,[rsreg].hi
				_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						// cmp	eax,[rtreg].hi
				_jcc_short_link(COND_B, &link1);									// jb	setit
				_jcc_short_link(COND_A, &link2);									// ja	skipit
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						// mov	eax,[rsreg].lo
				_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// cmp	eax,[rtreg].lo
				_jcc_short_link(COND_AE, &link3);									// jae	skipit
				_resolve_link(&link1);												// setit:
				_add_r32_imm(REG_ECX, 1);											// add	ecx,1
				_resolve_link(&link2);												// skipit:
				_resolve_link(&link3);												// skipit:
				_mov_m32abs_r32(LO(&mips3.r[RDREG]), REG_ECX);						// mov	[rdreg].lo,ecx
				_mov_m32abs_imm(HI(&mips3.r[RDREG]), 0);							// mov	[rdreg].hi,0
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2c:	/* DADD */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);					// mov	edx:eax,[rsreg]
				_add_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// add	eax,[rtreg].lo
				_adc_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));						// adc	edx,[rtreg].hi
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo	generate_overflow_exception
				if (RDREG != 0)
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			// mov	[rdreg],[rsreg]
				else if (RTREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RTREG]);			// mov	[rdreg],[rtreg]
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;
			
		case 0x2d:	/* DADDU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RSREG]);				// movsd xmm0,[rsreg]
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				// movsd xmm1,[rtreg]
						_paddq_r128_r128(REG_XMM0, REG_XMM1);						// paddq xmm0,xmm1
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// mov	[rdreg],xmm0
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);			// mov	edx:eax,[rsreg]
						_add_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				// add	eax,[rtreg].lo
						_adc_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				// adc	edx,[rtreg].hi
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			// mov	[rdreg],edx:eax
					}
				}
				else if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			// mov	[rdreg],[rsreg]
				else if (RTREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RTREG]);			// mov	[rdreg],[rtreg]
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2e:	/* DSUB */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);					// mov	edx:eax,[rsreg]
				_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// sub	eax,[rtreg].lo
				_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));						// sbb	edx,[rtreg].hi
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo	generate_overflow_exception
				if (RDREG != 0)
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			// mov	[rdreg],[rsreg]
				else if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_pxor_r128_r128(REG_XMM0, REG_XMM0);						// pxor	xmm0,xmm0
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				// movsd xmm1,[rtreg]
						_psubq_r128_r128(REG_XMM0, REG_XMM1);						// psubq xmm0,xmm1
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// mov	[rdreg],xmm0
					}
					else
					{
						_xor_r32_r32(REG_EAX, REG_EAX);								// xor	eax,eax
						_xor_r32_r32(REG_EDX, REG_EDX);								// xor	edx,edx
						_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				// sub	eax,[rtreg].lo
						_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				// sbb	edx,[rtreg].hi
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			// mov	[rdreg],edx:eax
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x2f:	/* DSUBU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RSREG]);				// movsd xmm0,[rsreg]
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				// movsd xmm1,[rtreg]
						_psubq_r128_r128(REG_XMM0, REG_XMM1);						// psubq xmm0,xmm1
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// mov	[rdreg],xmm0
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);			// mov	edx:eax,[rsreg]
						_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				// sub	eax,[rtreg].lo
						_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				// sbb	edx,[rtreg].hi
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			// mov	[rdreg],edx:eax
					}
				}
				else if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			// mov	[rdreg],[rsreg]
				else if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_pxor_r128_r128(REG_XMM0, REG_XMM0);						// pxor	xmm0,xmm0
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				// movsd xmm1,[rtreg]
						_psubq_r128_r128(REG_XMM0, REG_XMM1);						// psubq xmm0,xmm1
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// mov	[rdreg],xmm0
					}
					else
					{
						_xor_r32_r32(REG_EAX, REG_EAX);								// xor	eax,eax
						_xor_r32_r32(REG_EDX, REG_EDX);								// xor	edx,edx
						_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				// sub	eax,[rtreg].lo
						_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				// sbb	edx,[rtreg].hi
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			// mov	[rdreg],edx:eax
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x30:	/* TGE */
			if (RSREG != 0)
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);					// mov	edx:eax,[rsreg]
			else
			{
				_xor_r32_r32(REG_EDX, REG_EDX);										// xor	edx,edx
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor	eax,eax
			}
			if (RTREG != 0)
			{
				_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// sub	eax,[rtreg].lo
				_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));						// sbb	edx,[rtreg].hi
			}
			else
				_cmp_r32_imm(REG_EDX, 0);											// cmp	edx,0
			_jcc(COND_GE, mips3.generate_trap_exception);							// jge	generate_trap_exception
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x31:	/* TGEU */
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							// mov	eax,[rsreg].hi
			_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));							// cmp	eax,[rtreg].hi
			_jcc(COND_A, mips3.generate_trap_exception);							// ja	generate_trap_exception
			_jcc_short_link(COND_B, &link1);										// jb	skipit
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							// mov	eax,[rsreg].lo
			_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));							// cmp	eax,[rtreg].lo
			_jcc(COND_AE, mips3.generate_trap_exception);							// jae	generate_trap_exception
			_resolve_link(&link1);													// skipit:
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x32:	/* TLT */
			if (RSREG != 0)
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);					// mov	edx:eax,[rsreg]
			else
			{
				_xor_r32_r32(REG_EDX, REG_EDX);										// xor	edx,edx
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor	eax,eax
			}
			if (RTREG != 0)
			{
				_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						// sub	eax,[rtreg].lo
				_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));						// sbb	edx,[rtreg].hi
			}
			else
				_cmp_r32_imm(REG_EDX, 0);											// cmp	edx,0
			_jcc(COND_L, mips3.generate_trap_exception);							// jl	generate_trap_exception
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x33:	/* TLTU */
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							// mov	eax,[rsreg].hi
			_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));							// cmp	eax,[rtreg].hi
			_jcc(COND_B, mips3.generate_trap_exception);							// jb	generate_trap_exception
			_jcc_short_link(COND_A, &link1);										// ja	skipit
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							// mov	eax,[rsreg].lo
			_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));							// cmp	eax,[rtreg].lo
			_jcc(COND_B, mips3.generate_trap_exception);							// jb	generate_trap_exception
			_resolve_link(&link1);													// skipit:
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x34:	/* TEQ */
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							// mov	eax,[rsreg].hi
			_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));							// cmp	eax,[rtreg].hi
			_jcc_short_link(COND_NE, &link1);										// jne	skipit
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							// mov	eax,[rsreg].lo
			_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));							// cmp	eax,[rtreg].lo
			_jcc(COND_E, mips3.generate_trap_exception);							// je	generate_trap_exception
			_resolve_link(&link1);													// skipit:
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x36:	/* TNE */
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							// mov	eax,[rsreg].hi
			_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));							// cmp	eax,[rtreg].hi
			_jcc_short_link(COND_E, &link1);										// je	skipit
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							// mov	eax,[rsreg].lo
			_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));							// cmp	eax,[rtreg].lo
			_jcc(COND_NE, mips3.generate_trap_exception);							// jne	generate_trap_exception
			_resolve_link(&link1);													// skipit:
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x38:	/* DSLL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RTREG]);				// movsd xmm0,[rtreg]
						if (SHIFT)
							_psllq_r128_imm(REG_XMM0, SHIFT);						// psllq xmm0,SHIFT
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// movsd [rdreg],xmm0
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);			// mov	edx:eax,[rtreg]
						if (SHIFT != 0)
						{
							_shld_r32_r32_imm(REG_EDX, REG_EAX, SHIFT);				// shld	edx,eax,SHIFT
							_shl_r32_imm(REG_EAX, SHIFT);							// shl	eax,SHIFT
						}
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			// mov	[rdreg],edx:eax
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3a:	/* DSRL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RTREG]);				// movsd xmm0,[rtreg]
						if (SHIFT)
							_psrlq_r128_imm(REG_XMM0, SHIFT);						// psrlq xmm0,SHIFT
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// movsd [rdreg],xmm0
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);			// mov	edx:eax,[rtreg]
						if (SHIFT != 0)
						{
							_shrd_r32_r32_imm(REG_EAX, REG_EDX, SHIFT);				// shrd	eax,edx,SHIFT
							_shr_r32_imm(REG_EDX, SHIFT);							// shr	edx,SHIFT
						}
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			// mov	[rdreg],edx:eax
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x3b:	/* DSRA */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);				// mov	edx:eax,[rtreg]
					if (SHIFT != 0)
					{
						_shrd_r32_r32_imm(REG_EAX, REG_EDX, SHIFT);					// shrd	eax,edx,SHIFT
						_sar_r32_imm(REG_EDX, SHIFT);								// sar	edx,SHIFT
					}
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3c:	/* DSLL32 */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RTREG]);				// movsd xmm0,[rtreg]
						_psllq_r128_imm(REG_XMM0, SHIFT+32);						// psllq xmm0,SHIFT+32
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// movsd [rdreg],xmm0
					}
					else
					{
						_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				// mov	eax,[rtreg].lo
						if (SHIFT != 0)
							_shl_r32_imm(REG_EAX, SHIFT);							// shl	eax,SHIFT
						_mov_m32abs_imm(LO(&mips3.r[RDREG]), 0);					// mov	[rdreg].lo,0
						_mov_m32abs_r32(HI(&mips3.r[RDREG]), REG_EAX);				// mov	[rdreg].hi,eax
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3e:	/* DSRL32 */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RTREG]);				// movsd xmm0,[rtreg]
						_psrlq_r128_imm(REG_XMM0, SHIFT+32);						// psrlq xmm0,SHIFT+32
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				// movsd [rdreg],xmm0
					}
					else
					{
						_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));				// mov	eax,[rtreg].hi
						if (SHIFT != 0)
							_shr_r32_imm(REG_EAX, SHIFT);							// shr	eax,SHIFT
						_mov_m32abs_imm(HI(&mips3.r[RDREG]), 0);					// mov	[rdreg].hi,0
						_mov_m32abs_r32(LO(&mips3.r[RDREG]), REG_EAX);				// mov	[rdreg].lo,eax
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3f:	/* DSRA32 */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));					// mov	eax,[rtreg].hi
					_cdq();															// cdq
					if (SHIFT != 0)
						_sar_r32_imm(REG_EAX, SHIFT);								// sar	eax,SHIFT
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				// mov	[rdreg],edx:eax
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
	}

	_jmp((void *)mips3.generate_invalidop_exception);								// jmp	generate_invalidop_exception
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}


/*------------------------------------------------------------------
	recompile_regimm
------------------------------------------------------------------*/

static UINT32 recompile_regimm(struct drccore *drc, UINT32 pc, UINT32 op)
{
	struct linkdata link1;
	int cycles;
	
	switch (RTREG)
	{
		case 0x00:	/* BLTZ */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_GE, &link1);									// jge	skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x01:	/* BGEZ */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl	skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x02:	/* BLTZL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_GE, &link1);									// jge	skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x03:	/* BGEZL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl	skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x08:	/* TGEI */
			if (RSREG != 0)
			{
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				// mov	edx:eax,[rsreg]
				_sub_r32_imm(REG_EAX, SIMMVAL);									// sub	eax,[rtreg].lo
				_sbb_r32_imm(REG_EDX, ((INT32)SIMMVAL >> 31));					// sbb	edx,[rtreg].lo
				_jcc(COND_GE, mips3.generate_trap_exception);					// jge	generate_trap_exception
			}
			else
			{
				if (0 >= SIMMVAL)
					_jmp(mips3.generate_trap_exception);						// jmp	generate_trap_exception
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x09:	/* TGEIU */
			if (RSREG != 0)
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), ((INT32)SIMMVAL >> 31));	// cmp	[rsreg].hi,upper
				_jcc(COND_A, mips3.generate_trap_exception);					// ja	generate_trap_exception
				_jcc_short_link(COND_B, &link1);								// jb	skip
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), SIMMVAL);					// cmp	[rsreg].lo,lower
				_jcc(COND_AE, mips3.generate_trap_exception);					// jae	generate_trap_exception
				_resolve_link(&link1);											// skip:
			}
			else
			{
				if (0 >= SIMMVAL)
					_jmp(mips3.generate_trap_exception);						// jmp	generate_trap_exception
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0a:	/* TLTI */
			if (RSREG != 0)
			{
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				// mov	edx:eax,[rsreg]
				_sub_r32_imm(REG_EAX, SIMMVAL);									// sub	eax,[rtreg].lo
				_sbb_r32_imm(REG_EDX, ((INT32)SIMMVAL >> 31));					// sbb	edx,[rtreg].lo
				_jcc(COND_L, mips3.generate_trap_exception);					// jl	generate_trap_exception
			}
			else
			{
				if (0 < SIMMVAL)
					_jmp(mips3.generate_trap_exception);						// jmp	generate_trap_exception
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0b:	/* TLTIU */
			if (RSREG != 0)
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), ((INT32)SIMMVAL >> 31));	// cmp	[rsreg].hi,upper
				_jcc(COND_B, mips3.generate_trap_exception);					// jb	generate_trap_exception
				_jcc_short_link(COND_A, &link1);								// ja	skip
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), SIMMVAL);					// cmp	[rsreg].lo,lower
				_jcc(COND_B, mips3.generate_trap_exception);					// jb	generate_trap_exception
				_resolve_link(&link1);											// skip:
			}
			else
			{
				_mov_m32abs_imm(LO(&mips3.r[RTREG]), (0 < SIMMVAL));			// mov	[rtreg].lo,const
				_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						// mov	[rtreg].hi,sign-extend(const)
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0c:	/* TEQI */
			if (RSREG != 0)
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), ((INT32)SIMMVAL >> 31));	// cmp	[rsreg].hi,upper
				_jcc_short_link(COND_NE, &link1);								// jne	skip
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), SIMMVAL);					// cmp	[rsreg].lo,lower
				_jcc(COND_E, mips3.generate_trap_exception);					// je	generate_trap_exception
				_resolve_link(&link1);											// skip:
			}
			else
			{
				if (0 == SIMMVAL)
					_jmp(mips3.generate_trap_exception);						// jmp	generate_trap_exception
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0e:	/* TNEI */
			if (RSREG != 0)
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), ((INT32)SIMMVAL >> 31));	// cmp	[rsreg].hi,upper
				_jcc_short_link(COND_E, &link1);								// je	skip
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), SIMMVAL);					// cmp	[rsreg].lo,lower
				_jcc(COND_NE, mips3.generate_trap_exception);					// jne	generate_trap_exception
				_resolve_link(&link1);											// skip:
			}
			else
			{
				if (0 != SIMMVAL)
					_jmp(mips3.generate_trap_exception);						// jmp	generate_trap_exception
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x10:	/* BLTZAL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_GE, &link1);									// jge	skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								// mov	[31],pc + 8
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x11:	/* BGEZAL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				_mov_m64abs_imm32(&mips3.r[31], pc + 8);							// mov	[31],pc + 8
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl	skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								// mov	[31],pc + 8
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x12:	/* BLTZALL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_GE, &link1);									// jge	skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								// mov	[31],pc + 8
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x13:	/* BGEZALL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				_mov_m64abs_imm32(&mips3.r[31], pc + 8);							// mov	[31],pc + 8
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							// cmp	[rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl	skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								// mov	[31],pc + 8
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);
	}

	_jmp((void *)mips3.generate_invalidop_exception);								// jmp	generate_invalidop_exception
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}



/*###################################################################################################
**	COP0 RECOMPILATION
**#################################################################################################*/

/*------------------------------------------------------------------
	recompile_set_cop0_reg
------------------------------------------------------------------*/

static UINT32 recompile_set_cop0_reg(struct drccore *drc, UINT8 reg)
{
	struct linkdata link1;
	
	switch (reg)
	{
		case COP0_Cause:
			_mov_r32_m32abs(REG_EBX, &mips3.cpr[0][COP0_Cause]);					// mov	ebx,[mips3.cpr[0][COP0_Cause]]
			_and_r32_imm(REG_EAX, ~0xfc00);											// and	eax,~0xfc00
			_and_r32_imm(REG_EBX, 0xfc00);											// and	ebx,0xfc00
			_or_r32_r32(REG_EAX, REG_EBX);											// or	eax,ebx
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Cause], REG_EAX);					// mov	[mips3.cpr[0][COP0_Cause]],eax
			_and_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Status]);					// and	eax,[mips3.cpr[0][COP0_Status]]
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_CHECK_SW_INTERRUPTS;
		
		case COP0_Status:
			_mov_r32_m32abs(REG_EBX, &mips3.cpr[0][COP0_Status]);					// mov	ebx,[mips3.cpr[0][COP0_Status]]
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Status], REG_EAX);					// mov	[mips3.cpr[0][COP0_Status]],eax
			_xor_r32_r32(REG_EAX, REG_EBX);											// xor	eax,ebx
			_test_r32_imm(REG_EAX, 0x8000);											// test	eax,0x8000
			_jcc_short_link(COND_Z, &link1);										// jz	skip
			append_update_cycle_counting(drc);										// update cycle counting
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_CHECK_INTERRUPTS;
					
		case COP0_Count:
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Count], REG_EAX);					// mov	[mips3.cpr[0][COP0_Count]],eax
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_push_r32(REG_EAX);														// push eax
			_call((void *)activecpu_gettotalcycles64);								// call	activecpu_gettotalcycles64
			_pop_r32(REG_EBX);														// pop	ebx
			_sub_r32_r32(REG_EAX, REG_EBX);											// sub	eax,ebx
			_sbb_r32_imm(REG_EDX, 0);												// sbb	edx,0
			_sub_r32_r32(REG_EAX, REG_EBX);											// sub	eax,ebx
			_sbb_r32_imm(REG_EDX, 0);												// sbb	edx,0
			_mov_m64abs_r64(&mips3.count_zero_time, REG_EDX, REG_EAX);				// mov	[mips3.count_zero_time],edx:eax
			append_update_cycle_counting(drc);										// update cycle counting
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case COP0_Compare:
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Compare], REG_EAX);					// mov	[mips3.cpr[0][COP0_Compare]],eax
			_and_m32abs_imm(&mips3.cpr[0][COP0_Cause], ~0x8000);					// and	[mips3.cpr[0][COP0_Cause]],~0x8000
			append_update_cycle_counting(drc);										// update cycle counting
			return RECOMPILE_SUCCESSFUL_CP(1,4);
		
		case COP0_PRId:
			return RECOMPILE_SUCCESSFUL_CP(1,4);
		
		case COP0_Config:
			_mov_r32_m32abs(REG_EBX, &mips3.cpr[0][COP0_Config]);					// mov	ebx,[mips3.cpr[0][COP0_Cause]]
			_and_r32_imm(REG_EAX, 0x0007);											// and	eax,0x0007
			_and_r32_imm(REG_EBX, ~0x0007);											// and	ebx,~0x0007
			_or_r32_r32(REG_EAX, REG_EBX);											// or	eax,ebx
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Config], REG_EAX);					// mov	[mips3.cpr[0][COP0_Cause]],eax
			return RECOMPILE_SUCCESSFUL_CP(1,4);
		
		default:
			_mov_m32abs_r32(&mips3.cpr[0][reg], REG_EAX);							// mov	[mips3.cpr[0][reg]],eax
			return RECOMPILE_SUCCESSFUL_CP(1,4);
	}
	return RECOMPILE_UNIMPLEMENTED;
}


/*------------------------------------------------------------------
	recompile_get_cop0_reg
------------------------------------------------------------------*/

static UINT32 recompile_get_cop0_reg(struct drccore *drc, UINT8 reg)
{
	struct linkdata link1;

	switch (reg)
	{
		case COP0_Count:
			_sub_r32_imm(REG_EBP, 250);												// sub  ebp,24
			_jcc_short_link(COND_NS, &link1);										// jns	notneg
			_xor_r32_r32(REG_EBP, REG_EBP);											// xor	ebp,ebp
			_resolve_link(&link1);													// notneg:
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_call((void *)activecpu_gettotalcycles64);								// call	activecpu_gettotalcycles64
			_sub_r32_m32abs(REG_EAX, LO(&mips3.count_zero_time));					// sub	eax,[mips3.count_zero_time+0]
			_sbb_r32_m32abs(REG_EDX, HI(&mips3.count_zero_time));					// sbb	edx,[mips3.count_zero_time+4]
			_shrd_r32_r32_imm(REG_EAX, REG_EDX, 1);									// shrd	eax,edx,1
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case COP0_Cause:
			_sub_r32_imm(REG_EBP, 250);												// sub  ebp,24
			_jcc_short_link(COND_NS, &link1);										// jns	notneg
			_xor_r32_r32(REG_EBP, REG_EBP);											// xor	ebp,ebp
			_resolve_link(&link1);													// notneg:
			_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][reg]);							// mov	eax,[mips3.cpr[0][reg]]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		default:
			_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][reg]);							// mov	eax,[mips3.cpr[0][reg]]
			return RECOMPILE_SUCCESSFUL_CP(1,4);
	}
	return RECOMPILE_UNIMPLEMENTED;
}


/*------------------------------------------------------------------
	recompile_cop0
------------------------------------------------------------------*/

static UINT32 recompile_cop0(struct drccore *drc, UINT32 pc, UINT32 op)
{
	struct linkdata checklink;
	UINT32 result;

	if (mips3.drcoptions & MIPS3DRC_STRICT_COP0)
	{
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_KSU_MASK);					// test	[mips3.cpr[0][COP0_Status]],SR_KSU_MASK
		_jcc_short_link(COND_Z, &checklink);										// jz	okay
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_COP0);						// test	[mips3.cpr[0][COP0_Status]],SR_COP0
		_jcc(COND_Z, mips3.generate_cop_exception);									// jz	generate_cop_exception
		_resolve_link(&checklink);													// okay:
	}
	
	switch (RSREG)
	{
		case 0x00:	/* MFCz */
			result = RECOMPILE_SUCCESSFUL_CP(1,4);
			if (RTREG != 0)
			{
				result = recompile_get_cop0_reg(drc, RDREG);									// read cop0 reg
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[rtreg],edx:eax
			}
			return result;
			
		case 0x01:	/* DMFCz */
			result = RECOMPILE_SUCCESSFUL_CP(1,4);
			if (RTREG != 0)
			{
				result = recompile_get_cop0_reg(drc, RDREG);									// read cop0 reg
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[rtreg],edx:eax
			}
			return result;

		case 0x02:	/* CFCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.ccr[0][RDREG]);						// mov	eax,[mips3.ccr[0][rdreg]]
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[rtreg],edx:eax
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x04:	/* MTCz */
			if (RTREG != 0)
				_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							// mov	eax,[mips3.r[RTREG]]
			else
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor	eax,eax
			result = recompile_set_cop0_reg(drc, RDREG);										// write cop0 reg
			return result;

		case 0x05:	/* DMTCz */
			if (RTREG != 0)
				_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							// mov	eax,[mips3.r[RTREG]]
			else
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor	eax,eax
			result = recompile_set_cop0_reg(drc, RDREG);										// write cop0 reg
			return result;

		case 0x06:	/* CTCz */
			if (RTREG != 0)
				_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							// mov	eax,[mips3.r[RTREG]]
			else
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor	eax,eax
			_mov_m32abs_r32(&mips3.ccr[0][RDREG], REG_EAX);							// mov	[mips3.ccr[0][RDREG]],eax
			return RECOMPILE_SUCCESSFUL_CP(1,4);

//		case 0x08:	/* BC */
//			switch (RTREG)
//			{
//				case 0x00:	/* BCzF */	if (!mips3.cf[0][0]) ADDPC(SIMMVAL);				break;
//				case 0x01:	/* BCzF */	if (mips3.cf[0][0]) ADDPC(SIMMVAL);					break;
//				case 0x02:	/* BCzFL */	invalid_instruction(op);							break;
//				case 0x03:	/* BCzTL */	invalid_instruction(op);							break;
//				default:	invalid_instruction(op);										break;
//			}
//			break;

		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:	/* COP */
			switch (op & 0x01ffffff)
			{
				case 0x01:	/* TLBR */
					return RECOMPILE_SUCCESSFUL_CP(1,4);
					
				case 0x02:	/* TLBWI */
					drc_append_save_call_restore(drc, (void *)logtlbentry, 0);		// call	logtlbentry
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x06:	/* TLBWR */
					drc_append_save_call_restore(drc, (void *)logtlbentry, 0);		// call	logtlbentry
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x08:	/* TLBP */
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x10:	/* RFE */
					_jmp(mips3.generate_invalidop_exception);						// jmp	generate_invalidop_exception
					return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;

				case 0x18:	/* ERET */
					_mov_r32_m32abs(REG_EDI, &mips3.cpr[0][COP0_EPC]);				// mov	edi,[mips3.cpr[0][COP0_EPC]]
					_and_m32abs_imm(&mips3.cpr[0][COP0_Status], ~SR_EXL);			// and	[mips3.cpr[0][COP0_Status]],~SR_EXL
					return RECOMPILE_SUCCESSFUL_CP(1,0) | RECOMPILE_CHECK_INTERRUPTS | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;

				default:
					_jmp(mips3.generate_invalidop_exception);						// jmp	generate_invalidop_exception
					return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
			}
			break;

//		default:
//			_jmp(mips3.generate_invalidop_exception);								// jmp	generate_invalidop_exception
//			return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
	}
	return RECOMPILE_UNIMPLEMENTED;
}



/*###################################################################################################
**	COP1 RECOMPILATION
**#################################################################################################*/

/*------------------------------------------------------------------
	recompile_cop1
------------------------------------------------------------------*/

static UINT32 recompile_cop1(struct drccore *drc, UINT32 pc, UINT32 op)
{
	struct linkdata link1;
	int cycles, i;

	if (mips3.drcoptions & MIPS3DRC_STRICT_COP1)
	{
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_COP1);						// test	[mips3.cpr[0][COP0_Status]],SR_COP1
		_jcc(COND_Z, mips3.generate_cop_exception);									// jz	generate_cop_exception
	}

	switch (RSREG)
	{
		case 0x00:	/* MFCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][RDREG]);						// mov	eax,[mips3.cpr[1][RDREG]]
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[mips3.r[RTREG]],edx:eax
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x01:	/* DMFCz */
			if (RTREG != 0)
				_mov_m64abs_m64abs(&mips3.r[RTREG], &mips3.cpr[1][RDREG]);
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x02:	/* CFCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.ccr[1][RDREG]);						// mov	eax,[mips3.ccr[1][RDREG]]
				if (RDREG == 31)
				{
					_and_r32_imm(REG_EAX, ~0xfe800000);								// and	eax,~0xfe800000
					_xor_r32_r32(REG_EBX, REG_EBX);									// xor	ebx,ebx
					_cmp_m8abs_imm(&mips3.cf[1][0], 0);								// cmp	[cf[0]],0
					_setcc_r8(COND_NZ, REG_BL);										// setnz bl
					_shl_r32_imm(REG_EBX, 23);										// shl	ebx,23
					_or_r32_r32(REG_EAX, REG_EBX);									// or	eax,ebx
					if (mips3.is_mips4)
						for (i = 1; i <= 7; i++)
						{
							_cmp_m8abs_imm(&mips3.cf[1][i], 0);						// cmp	[cf[i]],0
							_setcc_r8(COND_NZ, REG_BL);								// setnz bl
							_shl_r32_imm(REG_EBX, 24+i);							// shl	ebx,24+i
							_or_r32_r32(REG_EAX, REG_EBX);							// or	eax,ebx
						}
				}
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					// mov	[mips3.r[RTREG]],edx:eax
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x04:	/* MTCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							// mov	eax,[mips3.r[RTREG]]
				_mov_m32abs_r32(LO(&mips3.cpr[1][RDREG]), REG_EAX);					// mov	[mips3.cpr[1][RDREG]],eax
			}
			else
				_mov_m32abs_imm(&mips3.cpr[1][RDREG], 0);							// mov	[mips3.cpr[1][RDREG]],0
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x05:	/* DMTCz */
			if (RTREG != 0)
				_mov_m64abs_m64abs(&mips3.cpr[1][RDREG], &mips3.r[RTREG]);
			else
				_zero_m64abs(&mips3.cpr[1][RDREG]);									// mov	[mips3.cpr[1][RDREG]],0
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x06:	/* CTCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							// mov	eax,[mips3.r[RTREG]]
				_mov_m32abs_r32(&mips3.ccr[1][RDREG], REG_EAX);						// mov	[mips3.ccr[1][RDREG]],eax
			}
			else
				_mov_m32abs_imm(&mips3.ccr[1][RDREG], 0);							// mov	[mips3.ccr[1][RDREG]],0
			if (RDREG == 31)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.ccr[1][RDREG]));					// mov	eax,[mips3.ccr[1][RDREG]]
				_test_r32_imm(REG_EAX, 1 << 23);									// test	eax,1<<23
				_setcc_m8abs(COND_NZ, &mips3.cf[1][0]);								// setnz [cf[0]]
				if (mips3.is_mips4)
					for (i = 1; i <= 7; i++)
					{
						_test_r32_imm(REG_EAX, 1 << (24+i));						// test	eax,1<<(24+i)
						_setcc_m8abs(COND_NZ, &mips3.cf[1][i]);						// setnz [cf[i]]
					}
				_and_r32_imm(REG_EAX, 3);											// and	eax,3
				_test_r32_imm(REG_EAX, 1);											// test eax,1
				_jcc_near_link(COND_Z, &link1);										// jz	skip
				_xor_r32_imm(REG_EAX, 2);											// xor  eax,2
				_resolve_link(&link1);												// skip:
				drc_append_set_fp_rounding(drc, REG_EAX);							// set_rounding(EAX)
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x08:	/* BC */
			switch ((op >> 16) & 3)
			{
				case 0x00:	/* BCzF */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	// cmp	[cf[x]],0
					_jcc_near_link(COND_NZ, &link1);								// jnz	link1
					cycles = recompile_delay_slot(drc, pc + 4);						// <next instruction>
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);// <branch or dispatch>
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,4);
				
				case 0x01:	/* BCzT */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	// cmp	[cf[x]],0
					_jcc_near_link(COND_Z, &link1);									// jz	link1
					cycles = recompile_delay_slot(drc, pc + 4);						// <next instruction>
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);// <branch or dispatch>
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x02:	/* BCzFL */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	// cmp	[cf[x]],0
					_jcc_near_link(COND_NZ, &link1);								// jnz	link1
					cycles = recompile_delay_slot(drc, pc + 4);						// <next instruction>
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);// <branch or dispatch>
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,8);

				case 0x03:	/* BCzTL */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	// cmp	[cf[x]],0
					_jcc_near_link(COND_Z, &link1);									// jz	link1
					cycles = recompile_delay_slot(drc, pc + 4);						// <next instruction>
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);// <branch or dispatch>
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,8);
			}
			break;

		default:
			switch (op & 0x3f)
			{
				case 0x00:
					if (IS_SINGLE(op))	/* ADD.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movss xmm0,[fsreg]
							_addss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);			// addss xmm0,[ftreg]
							_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			// movss [fdreg],xmm0
						}
						else
						{
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
							_fld_m32abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_faddp();													// faddp
							_fstp_m32abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
						}
					}
					else				/* ADD.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movsd xmm0,[fsreg]
							_addsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);			// addsd xmm0,[ftreg]
							_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			// movsd [fdreg],xmm0
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
							_fld_m64abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_faddp();													// faddp
							_fstp_m64abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x01:
					if (IS_SINGLE(op))	/* SUB.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movss xmm0,[fsreg]
							_subss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);			// subss xmm0,[ftreg]
							_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			// movss [fdreg],xmm0
						}
						else
						{
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
							_fld_m32abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fsubp();													// fsubp
							_fstp_m32abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
						}
					}
					else				/* SUB.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movsd xmm0,[fsreg]
							_subsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);			// subsd xmm0,[ftreg]
							_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			// movsd [fdreg],xmm0
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
							_fld_m64abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fsubp();													// fsubp
							_fstp_m64abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x02:
					if (IS_SINGLE(op))	/* MUL.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movss xmm0,[fsreg]
							_mulss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);			// mulss xmm0,[ftreg]
							_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			// movss [fdreg],xmm0
						}
						else
						{
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
							_fld_m32abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fmulp();													// fmulp
							_fstp_m32abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
						}
					}
					else				/* MUL.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movsd xmm0,[fsreg]
							_mulsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);			// mulsd xmm0,[ftreg]
							_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			// movsd [fdreg],xmm0
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
							_fld_m64abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fmulp();													// fmulp
							_fstp_m64abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x03:
					if (IS_SINGLE(op))	/* DIV.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movss xmm0,[fsreg]
							_divss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);			// divss xmm0,[ftreg]
							_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			// movss [fdreg],xmm0
						}
						else
						{
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
							_fld_m32abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fdivp();													// fdivp
							_fstp_m32abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
						}
					}
					else				/* DIV.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movsd xmm0,[fsreg]
							_divsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);			// divsd xmm0,[ftreg]
							_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			// movsd [fdreg],xmm0
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
							_fld_m64abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fdivp();													// fdivp
							_fstp_m64abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x04:
					if (IS_SINGLE(op))	/* SQRT.S */
					{
						if (USE_SSE)
						{
							_sqrtss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);		// sqrtss xmm0,[fsreg]
							_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			// movss [fdreg],xmm0
						}
						else
						{
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
							_fsqrt();													// fsqrt
							_fstp_m32abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
						}
					}
					else				/* SQRT.D */
					{
						if (USE_SSE)
						{
							_sqrtsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);		// sqrtsd xmm0,[fsreg]
							_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			// movsd [fdreg],xmm0
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
							_fsqrt();													// fsqrt
							_fstp_m64abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x05:
					if (IS_SINGLE(op))	/* ABS.S */
					{
						_fld_m32abs(&mips3.cpr[1][FSREG]);								// fld	[fsreg]
						_fabs();														// fabs
						_fstp_m32abs(&mips3.cpr[1][FDREG]);								// fstp	[fdreg]
					}
					else				/* ABS.D */
					{
						_fld_m64abs(&mips3.cpr[1][FSREG]);								// fld	[fsreg]
						_fabs();														// fabs
						_fstp_m64abs(&mips3.cpr[1][FDREG]);								// fstp	[fdreg]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x06:
					if (IS_SINGLE(op))	/* MOV.S */
					{
						_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][FSREG]);					// mov	eax,[fsreg]
						_mov_m32abs_r32(&mips3.cpr[1][FDREG], REG_EAX);					// mov	[fdreg],eax
					}
					else				/* MOV.D */
						_mov_m64abs_m64abs(&mips3.cpr[1][FDREG], &mips3.cpr[1][FSREG]);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x07:
					if (IS_SINGLE(op))	/* NEG.S */
					{
						_fld_m32abs(&mips3.cpr[1][FSREG]);								// fld	[fsreg]
						_fchs();														// fchs
						_fstp_m32abs(&mips3.cpr[1][FDREG]);								// fstp	[fdreg]
					}
					else				/* NEG.D */
					{
						_fld_m64abs(&mips3.cpr[1][FSREG]);								// fld	[fsreg]
						_fchs();														// fchs
						_fstp_m64abs(&mips3.cpr[1][FDREG]);								// fstp	[fdreg]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x08:
					drc_append_set_temp_fp_rounding(drc, FPRND_NEAR);
					if (IS_SINGLE(op))	/* ROUND.L.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					else				/* ROUND.L.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fistp_m64abs(&mips3.cpr[1][FDREG]);							// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x09:
					drc_append_set_temp_fp_rounding(drc, FPRND_CHOP);
					if (IS_SINGLE(op))	/* TRUNC.L.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					else				/* TRUNC.L.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fistp_m64abs(&mips3.cpr[1][FDREG]);							// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0a:
					drc_append_set_temp_fp_rounding(drc, FPRND_UP);
					if (IS_SINGLE(op))	/* CEIL.L.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					else				/* CEIL.L.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fistp_m64abs(&mips3.cpr[1][FDREG]);							// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0b:
					drc_append_set_temp_fp_rounding(drc, FPRND_DOWN);
					if (IS_SINGLE(op))	/* FLOOR.L.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					else				/* FLOOR.L.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fistp_m64abs(&mips3.cpr[1][FDREG]);							// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0c:
					drc_append_set_temp_fp_rounding(drc, FPRND_NEAR);
					if (IS_SINGLE(op))	/* ROUND.W.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					else				/* ROUND.W.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fistp_m32abs(&mips3.cpr[1][FDREG]);							// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0d:
					drc_append_set_temp_fp_rounding(drc, FPRND_CHOP);
					if (IS_SINGLE(op))	/* TRUNC.W.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					else				/* TRUNC.W.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fistp_m32abs(&mips3.cpr[1][FDREG]);							// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0e:
					drc_append_set_temp_fp_rounding(drc, FPRND_UP);
					if (IS_SINGLE(op))	/* CEIL.W.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					else				/* CEIL.W.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fistp_m32abs(&mips3.cpr[1][FDREG]);							// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0f:
					drc_append_set_temp_fp_rounding(drc, FPRND_DOWN);
					if (IS_SINGLE(op))	/* FLOOR.W.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					else				/* FLOOR.W.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fistp_m32abs(&mips3.cpr[1][FDREG]);							// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x11:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			// jmp	generate_invalidop_exception
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_cmp_m8abs_imm(&mips3.cf[1][(op >> 18) & 7], 0);				// cmp	[cf[x]],0
					_jcc_short_link(((op >> 16) & 1) ? COND_Z : COND_NZ, &link1);	// jz/nz skip
					if (IS_SINGLE(op))	/* MOVT/F.S */
					{
						_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][FSREG]);				// mov	eax,[fsreg]
						_mov_m32abs_r32(&mips3.cpr[1][FDREG], REG_EAX);				// mov	[fdreg],eax
					}
					else				/* MOVT/F.D */
						_mov_m64abs_m64abs(&mips3.cpr[1][FDREG], &mips3.cpr[1][FSREG]);
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x12:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			// jmp	generate_invalidop_exception
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));					// mov	eax,[rtreg].lo
					_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));					// or	eax,[rtreg].hi
					_jcc_short_link(COND_NZ, &link1);								// jnz	skip
					if (IS_SINGLE(op))	/* MOVZ.S */
					{
						_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][FSREG]);				// mov	eax,[fsreg]
						_mov_m32abs_r32(&mips3.cpr[1][FDREG], REG_EAX);				// mov	[fdreg],eax
					}
					else				/* MOVZ.D */
						_mov_m64abs_m64abs(&mips3.cpr[1][FDREG], &mips3.cpr[1][FSREG]);
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x13:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			// jmp	generate_invalidop_exception
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));					// mov	eax,[rtreg].lo
					_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));					// or	eax,[rtreg].hi
					_jcc_short_link(COND_Z, &link1);								// jz	skip
					if (IS_SINGLE(op))	/* MOVN.S */
					{
						_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][FSREG]);				// mov	eax,[fsreg]
						_mov_m32abs_r32(&mips3.cpr[1][FDREG], REG_EAX);				// mov	[fdreg],eax
					}
					else				/* MOVN.D */
						_mov_m64abs_m64abs(&mips3.cpr[1][FDREG], &mips3.cpr[1][FSREG]);
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x15:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			// jmp	generate_invalidop_exception
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_fld1();														// fld1
					if (IS_SINGLE(op))	/* RECIP.S */
					{
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						_fdivp();													// fdivp
						_fstp_m32abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
					}
					else				/* RECIP.D */
					{
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						_fdivp();													// fdivp
						_fstp_m64abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x16:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			// jmp	generate_invalidop_exception
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_fld1();														// fld1
					if (IS_SINGLE(op))	/* RSQRT.S */
					{
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						_fsqrt();													// fsqrt
						_fdivp();													// fdivp
						_fstp_m32abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
					}
					else				/* RSQRT.D */
					{
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						_fsqrt();													// fsqrt
						_fdivp();													// fdivp
						_fstp_m64abs(&mips3.cpr[1][FDREG]);							// fstp	[fdreg]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x20:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.S.W */
							_fild_m32abs(&mips3.cpr[1][FSREG]);						// fild	[fsreg]
						else				/* CVT.S.L */
							_fild_m64abs(&mips3.cpr[1][FSREG]);						// fild	[fsreg]
					}
					else					/* CVT.S.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fstp_m32abs(&mips3.cpr[1][FDREG]);								// fstp	[fdreg]
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x21:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.D.W */
							_fild_m32abs(&mips3.cpr[1][FSREG]);						// fild	[fsreg]
						else				/* CVT.D.L */
							_fild_m64abs(&mips3.cpr[1][FSREG]);						// fild	[fsreg]
					}
					else					/* CVT.D.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fstp_m64abs(&mips3.cpr[1][FDREG]);								// fstp	[fdreg]
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x24:
					if (IS_SINGLE(op))	/* CVT.W.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					else				/* CVT.W.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fistp_m32abs(&mips3.cpr[1][FDREG]);							// fistp [fdreg]
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x25:
					if (IS_SINGLE(op))	/* CVT.L.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					else				/* CVT.L.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
					_fistp_m64abs(&mips3.cpr[1][FDREG]);							// fistp [fdreg]
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x30:
				case 0x38:
					_mov_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0], 0);	/* C.F.S/D */
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x31:
				case 0x39:
					_mov_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0], 0);	/* C.UN.S/D */
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x32:
				case 0x3a:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.EQ.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movss xmm0,[fsreg]
							_comiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// comiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movsd xmm0,[fsreg]
							_comisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// comisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // sete [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.EQ.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						_fcompp();														// fcompp
						_fnstsw_ax();													// fnstsw ax
						_sahf();														// sahf
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // sete [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x33:
				case 0x3b:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.UEQ.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movss xmm0,[fsreg]
							_ucomiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// ucomiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movsd xmm0,[fsreg]
							_ucomisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// ucomisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // sete [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.UEQ.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						_fucompp();														// fucompp
						_fnstsw_ax();													// fnstsw ax
						_sahf();														// sahf
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // sete [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x34:
				case 0x3c:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.OLT.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movss xmm0,[fsreg]
							_comiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// comiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movsd xmm0,[fsreg]
							_comisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// comisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setb [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.OLT.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						_fcompp();														// fcompp
						_fnstsw_ax();													// fnstsw ax
						_sahf();														// sahf
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setb [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x35:
				case 0x3d:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.ULT.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movss xmm0,[fsreg]
							_ucomiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// ucomiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movsd xmm0,[fsreg]
							_ucomisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// ucomisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setb [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.ULT.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						_fucompp();														// fucompp
						_fnstsw_ax();													// fnstsw ax
						_sahf();														// sahf
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setb [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x36:
				case 0x3e:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.OLE.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movss xmm0,[fsreg]
							_comiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// comiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movsd xmm0,[fsreg]
							_comisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// comisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setle [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.OLE.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						_fcompp();														// fcompp
						_fnstsw_ax();													// fnstsw ax
						_sahf();														// sahf
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setbe [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x37:
				case 0x3f:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.ULE.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movss xmm0,[fsreg]
							_ucomiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// ucomiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			// movsd xmm0,[fsreg]
							_ucomisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		// ucomisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setl [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.ULE.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m32abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							// fld	[ftreg]
							_fld_m64abs(&mips3.cpr[1][FSREG]);							// fld	[fsreg]
						}
						_fucompp();														// fucompp
						_fnstsw_ax();													// fnstsw ax
						_sahf();														// sahf
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setbe [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);
			}
			break;
	}
	_jmp((void *)mips3.generate_invalidop_exception);								// jmp	generate_invalidop_exception
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}



/*###################################################################################################
**	COP1X RECOMPILATION
**#################################################################################################*/

/*------------------------------------------------------------------
	recompile_cop1x
------------------------------------------------------------------*/

static UINT32 recompile_cop1x(struct drccore *drc, UINT32 pc, UINT32 op)
{
	if (mips3.drcoptions & MIPS3DRC_STRICT_COP1)
	{
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_COP1);						// test	[mips3.cpr[0][COP0_Status]],SR_COP1
		_jcc(COND_Z, mips3.generate_cop_exception);									// jz	generate_cop_exception
	}

	switch (op & 0x3f)
	{
		case 0x00:		/* LWXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);								// add	eax,[rtreg]
			_push_r32(REG_EAX);														// push	eax
			_call(mips3.memory.readlong);											// call	readlong
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_mov_m32abs_r32(&mips3.cpr[1][FDREG], REG_EAX);							// mov	[fdreg],eax
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x01:		/* LDXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);								// add	eax,[rtreg]
			_push_r32(REG_EAX);														// push	eax
			_call(mips3.memory.readlong);											// call	readlong
			_mov_m32abs_r32(mips3.bigendian ? HI(&mips3.cpr[1][FDREG]) : LO(&mips3.cpr[1][FDREG]), REG_EAX);// mov	[fdreg].hi/lo,eax
			_pop_r32(REG_EAX);														// pop	eax
			_add_r32_imm(REG_EAX, 4);												// add	eax,4
			_push_r32(REG_EAX);														// push	eax
			_call(mips3.memory.readlong);											// call	readlong
			_mov_m32abs_r32(mips3.bigendian ? LO(&mips3.cpr[1][FDREG]) : HI(&mips3.cpr[1][FDREG]), REG_EAX);// mov	[fdreg].lo/hi,eax
			_add_r32_imm(REG_ESP, 4);												// add	esp,4
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);
		
		case 0x08:		/* SWXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(&mips3.cpr[1][FSREG]);										// push	[fsreg]
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);								// add	eax,[rtreg]
			_push_r32(REG_EAX);														// push	eax
			_call(mips3.memory.writelong);											// call	writelong
			_add_r32_imm(REG_ESP, 8);												// add	esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);
		
		case 0x09:		/* SDXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov	[mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(mips3.bigendian ? HI(&mips3.cpr[1][FSREG]) : LO(&mips3.cpr[1][FSREG]));// push	[fsreg].hi/lo
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);								// add	eax,[rtreg]
			_push_r32(REG_EAX);														// push	eax
			_call(mips3.memory.writelong);											// call	writelong
			_pop_r32(REG_EAX);														// pop	eax
			_add_r32_imm(REG_EAX, 4);												// add	eax,4
			_push_m32abs(mips3.bigendian ? LO(&mips3.cpr[1][FSREG]) : HI(&mips3.cpr[1][FSREG]));// push	[fsreg].lo/hi
			_push_r32(REG_EAX);														// push	eax
			_call(mips3.memory.writelong);											// call	writelong
			_add_r32_imm(REG_ESP, 12);												// add	esp,12
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov	ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);
		
		case 0x0f:		/* PREFX */
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x20:		/* MADD.S */
			if (USE_SSE)
			{
				_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);						// movss xmm0,[fsreg]
				_mulss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);						// mulss xmm0,[ftreg]
				_addss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FRREG]);						// addss xmm0,[frreg]
				_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);						// movss [fdreg],xmm0
			}
			else
			{
				_fld_m32abs(&mips3.cpr[1][FRREG]);										// fld	[frreg]
				_fld_m32abs(&mips3.cpr[1][FSREG]);										// fld	[fsreg]
				_fld_m32abs(&mips3.cpr[1][FTREG]);										// fld	[ftreg]
				_fmulp();																// fmulp
				_faddp();																// faddp
				_fstp_m32abs(&mips3.cpr[1][FDREG]);										// fstp	[fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x21:		/* MADD.D */
			if (USE_SSE)
			{
				_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);						// movsd xmm0,[fsreg]
				_mulsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);						// mulsd xmm0,[ftreg]
				_addsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FRREG]);						// addsd xmm0,[frreg]
				_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);						// movsd [fdreg],xmm0
			}
			else
			{
				_fld_m64abs(&mips3.cpr[1][FRREG]);										// fld	[frreg]
				_fld_m64abs(&mips3.cpr[1][FSREG]);										// fld	[fsreg]
				_fld_m64abs(&mips3.cpr[1][FTREG]);										// fld	[ftreg]
				_fmulp();																// fmulp
				_faddp();																// faddp
				_fstp_m64abs(&mips3.cpr[1][FDREG]);										// fstp	[fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x28:		/* MSUB.S */
			if (USE_SSE)
			{
				_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);						// movss xmm0,[fsreg]
				_mulss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);						// mulss xmm0,[ftreg]
				_subss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FRREG]);						// subss xmm0,[frreg]
				_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);						// movss [fdreg],xmm0
			}
			else
			{
				_fld_m32abs(&mips3.cpr[1][FRREG]);										// fld	[frreg]
				_fld_m32abs(&mips3.cpr[1][FSREG]);										// fld	[fsreg]
				_fld_m32abs(&mips3.cpr[1][FTREG]);										// fld	[ftreg]
				_fmulp();																// fmulp
				_fsubrp();																// fsubrp
				_fstp_m32abs(&mips3.cpr[1][FDREG]);										// fstp	[fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x29:		/* MSUB.D */
			if (USE_SSE)
			{
				_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);						// movsd xmm0,[fsreg]
				_mulsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);						// mulsd xmm0,[ftreg]
				_subsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FRREG]);						// subsd xmm0,[frreg]
				_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);						// movsd [fdreg],xmm0
			}
			else
			{
				_fld_m64abs(&mips3.cpr[1][FRREG]);										// fld	[frreg]
				_fld_m64abs(&mips3.cpr[1][FSREG]);										// fld	[fsreg]
				_fld_m64abs(&mips3.cpr[1][FTREG]);										// fld	[ftreg]
				_fmulp();																// fmulp
				_fsubrp();																// fsubrp
				_fstp_m64abs(&mips3.cpr[1][FDREG]);										// fstp	[fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x30:		/* NMADD.S */
			if (USE_SSE)
			{
				_pxor_r128_r128(REG_XMM1, REG_XMM1);									// pxor	xmm1,xmm1
				_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);						// movss xmm0,[fsreg]
				_mulss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);						// mulss xmm0,[ftreg]
				_addss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FRREG]);						// addss xmm0,[frreg]
				_subss_r128_r128(REG_XMM1, REG_XMM0);									// subss xmm1,xmm0
				_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM1);						// movss [fdreg],xmm1
			}
			else
			{
				_fld_m32abs(&mips3.cpr[1][FRREG]);										// fld	[frreg]
				_fld_m32abs(&mips3.cpr[1][FSREG]);										// fld	[fsreg]
				_fld_m32abs(&mips3.cpr[1][FTREG]);										// fld	[ftreg]
				_fmulp();																// fmulp
				_faddp();																// faddp
				_fchs();																// fchs
				_fstp_m32abs(&mips3.cpr[1][FDREG]);										// fstp	[fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x31:		/* NMADD.D */
			if (USE_SSE)
			{
				_pxor_r128_r128(REG_XMM1, REG_XMM1);									// pxor	xmm1,xmm1
				_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);						// movsd xmm0,[fsreg]
				_mulsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);						// mulsd xmm0,[ftreg]
				_addsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FRREG]);						// addsd xmm0,[frreg]
				_subss_r128_r128(REG_XMM1, REG_XMM0);									// subss xmm1,xmm0
				_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM1);						// movsd [fdreg],xmm1
			}
			else
			{
				_fld_m64abs(&mips3.cpr[1][FRREG]);										// fld	[frreg]
				_fld_m64abs(&mips3.cpr[1][FSREG]);										// fld	[fsreg]
				_fld_m64abs(&mips3.cpr[1][FTREG]);										// fld	[ftreg]
				_fmulp();																// fmulp
				_faddp();																// faddp
				_fchs();																// fchs
				_fstp_m64abs(&mips3.cpr[1][FDREG]);										// fstp	[fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x38:		/* NMSUB.S */
			if (USE_SSE)
			{
				_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);						// movss xmm0,[fsreg]
				_mulss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);						// mulss xmm0,[ftreg]
				_movss_r128_m32abs(REG_XMM1, &mips3.cpr[1][FRREG]);						// movss xmm1,[frreg]
				_subss_r128_r128(REG_XMM1, REG_XMM0);									// subss xmm1,xmm0
				_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM1);						// movss [fdreg],xmm1
			}
			else
			{
				_fld_m32abs(&mips3.cpr[1][FRREG]);										// fld	[frreg]
				_fld_m32abs(&mips3.cpr[1][FSREG]);										// fld	[fsreg]
				_fld_m32abs(&mips3.cpr[1][FTREG]);										// fld	[ftreg]
				_fmulp();																// fmulp
				_fsubp();																// fsubp
				_fstp_m32abs(&mips3.cpr[1][FDREG]);										// fstp	[fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x39:		/* NMSUB.D */
			if (USE_SSE)
			{
				_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);						// movsd xmm0,[fsreg]
				_mulsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);						// mulsd xmm0,[ftreg]
				_movsd_r128_m64abs(REG_XMM1, &mips3.cpr[1][FRREG]);						// movsd xmm1,[frreg]
				_subss_r128_r128(REG_XMM1, REG_XMM0);									// subss xmm1,xmm0
				_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM1);						// movsd [fdreg],xmm1
			}
			else
			{
				_fld_m64abs(&mips3.cpr[1][FRREG]);										// fld	[frreg]
				_fld_m64abs(&mips3.cpr[1][FSREG]);										// fld	[fsreg]
				_fld_m64abs(&mips3.cpr[1][FTREG]);										// fld	[ftreg]
				_fmulp();																// fmulp
				_fsubp();																// fsubrp
				_fstp_m64abs(&mips3.cpr[1][FDREG]);										// fstp	[fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x24:		/* MADD.W */
		case 0x25:		/* MADD.L */
		case 0x2c:		/* MSUB.W */
		case 0x2d:		/* MSUB.L */
		case 0x34:		/* NMADD.W */
		case 0x35:		/* NMADD.L */
		case 0x3c:		/* NMSUB.W */
		case 0x3d:		/* NMSUB.L */
		default:
			fprintf(stderr, "cop1x %X\n", op);
			break;
	}
	_jmp((void *)mips3.generate_invalidop_exception);								// jmp	generate_invalidop_exception
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}
