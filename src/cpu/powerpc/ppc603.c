
// why only this stuff?  Is everything else initted elsewhere?
static void ppc603_reset(void *param)
{
	ppc.pc = ppc.npc = 0xFFF00100;

	ppc_set_msr(0x40);
	change_pc(ppc.pc);
}

static void ppc603_exception(int exception)
{
	switch( exception )
	{
		case EXCEPTION_IRQ:		/* External Interrupt */
			if( ppc_get_msr() & MSR_EE ) {
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = (msr & 0x0000ff73) | (SRR1 & ~0x0000ff73);

				//msr &= ~(MSR_WE | MSR_PR | MSR_EE | MSR_PE);	// Which flags need to be cleared ?
				if( msr & MSR_LE )
					msr |= MSR_ILE;
				else
					msr &= ~MSR_ILE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0500;
				else
					ppc.npc = 0x00000000 | 0x0500;
			}
			break;

		case EXCEPTION_DECREMENTER:		/* Decrementer overflow exception */
			if( ppc_get_msr() & MSR_EE ) {
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = (msr & 0x0000ff73) | (SRR1 & ~0x0000ff73);

				//msr &= ~(MSR_WE | MSR_PR | MSR_EE | MSR_PE);	// Which flags need to be cleared ?
				if( msr & MSR_LE )
					msr |= MSR_ILE;
				else
					msr &= ~MSR_ILE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0900;
				else
					ppc.npc = 0x00000000 | 0x0900;
			}
			break;

		default:
			osd_die("ppc: Unhandled exception %d\n", exception);
			break;
	}
}

static void ppc603_set_irq_line(int irqline, int state)
{
	if( state ) {
		ppc603_exception(EXCEPTION_IRQ);
	}
}

static int ppc603_execute(int cycles)
{
	ppc_icount = cycles;
	change_pc(ppc.npc);

	while( ppc_icount > 0 )
	{
		int cc = (ppc_icount >> 2) & 0x1;
		UINT32 opcode;
		UINT32 dec_old = DEC;
		ppc.pc = ppc.npc;
		ppc.npc += 4;
		opcode = ROPCODE64(ppc.pc);

		CALL_MAME_DEBUG;

		switch(opcode >> 26)
		{
			case 19:	optable19[(opcode >> 1) & 0x3ff](opcode); break;
			case 31:	optable31[(opcode >> 1) & 0x3ff](opcode); break;
			case 59:	optable59[(opcode >> 1) & 0x3ff](opcode); break;
			case 63:	optable63[(opcode >> 1) & 0x3ff](opcode); break;
			default:	optable[opcode >> 26](opcode); break;
		}

		ppc_icount--;

		ppc.tb += cc;
		
		DEC -= cc;
		if(DEC > dec_old)
			ppc603_exception(EXCEPTION_DECREMENTER);
	}

	return cycles - ppc_icount;
}
