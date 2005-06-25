void ppc602_exception(int exception)
{
	switch( exception )
	{
		case EXCEPTION_IRQ:		/* External Interrupt */
			if( ppc_get_msr() & MSR_EE ) {
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = msr & 0xff73;

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0500;
				else
					ppc.npc = ppc.ibr | 0x0500;

				ppc.interrupt_pending &= ~0x1;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_DECREMENTER:		/* Decrementer overflow exception */
			if( ppc_get_msr() & MSR_EE ) {
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = msr & 0xff73;

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0900;
				else
					ppc.npc = ppc.ibr | 0x0900;

				ppc.interrupt_pending &= ~0x2;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_TRAP:			/* Program exception / Trap */
			{
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.pc;
				SRR1 = (msr & 0xff73) | 0x20000;	/* 0x20000 = TRAP bit */

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0700;
				else
					ppc.npc = ppc.ibr | 0x0700;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_SYSTEM_CALL:		/* System call */
			{
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = (msr & 0xff73);

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0c00;
				else
					ppc.npc = ppc.ibr | 0x0c00;
				change_pc(ppc.npc);
			}
			break;


		default:
			osd_die("ppc: Unhandled exception %d\n", exception);
			break;
	}
}

static void ppc602_set_irq_line(int irqline, int state)
{
	if( state ) {
		ppc.interrupt_pending |= 0x1;
		if (ppc.irq_callback)
		{
			ppc.irq_callback(irqline);
		}
	}
}

INLINE void ppc602_check_interrupts(void)
{
	if (MSR & MSR_EE)
	{
		if (ppc.interrupt_pending != 0)
		{
			if (ppc.interrupt_pending & 0x1)
			{
				ppc602_exception(EXCEPTION_IRQ);
			}
			else if (ppc.interrupt_pending & 0x2)
			{
				ppc602_exception(EXCEPTION_DECREMENTER);
			}
		}
	}
}

static void ppc602_reset(void *param)
{
	float multiplier;
	ppc_config *config = param;
	ppc.pc = ppc.npc = 0xfff00100;
	ppc.pvr = config->pvr;

	multiplier = (float)((config->bus_frequency_multiplier >> 4) & 0xf) +
				 (float)(config->bus_frequency_multiplier & 0xf) / 10.0f;
	ppc.bus_freq_multiplier = (int)(multiplier * 2);

	ppc_set_msr(0x40);
	change_pc(ppc.pc);

	ppc.hid0 = 1;

	ppc.interrupt_pending = 0;
}

static int ppc602_execute(int cycles)
{
	UINT32 opcode, dec_old;
	ppc_icount = cycles;
	change_pc(ppc.npc);

	while( ppc_icount > 0 )
	{
		//int cc = (ppc_icount >> 2) & 0x1;
		dec_old = DEC;
		ppc.pc = ppc.npc;
		CALL_MAME_DEBUG;

		ppc.npc = ppc.pc + 4;
		opcode = ROPCODE64(ppc.pc);

		switch(opcode >> 26)
		{
			case 19:	optable19[(opcode >> 1) & 0x3ff](opcode); break;
			case 31:	optable31[(opcode >> 1) & 0x3ff](opcode); break;
			case 59:	optable59[(opcode >> 1) & 0x3ff](opcode); break;
			case 63:	optable63[(opcode >> 1) & 0x3ff](opcode); break;
			default:	optable[opcode >> 26](opcode); break;
		}

		ppc_icount--;

		ppc.tb += 1;

		DEC -= 1;
		if(DEC == 0) {
			ppc.interrupt_pending |= 0x2;
		}

		ppc602_check_interrupts();
	}

	return cycles - ppc_icount;
}
