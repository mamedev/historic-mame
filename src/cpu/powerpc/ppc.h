#ifndef _PPC_H
#define _PPC_H

#define	SPR_CDBCR			0x3d7
#define SPR_CTR				0x9
#define SPR_DAC1			0x3f6
#define SPR_DAC2			0x3f7
#define SPR_DBCR			0x3f2
#define SPR_DBSR			0x3f0
#define SPR_DCCR			0x3fa
#define SPR_DEAR			0x3d5
#define SPR_ESR				0x3d4
#define SPR_EVPR			0x3d6
#define SPR_IAC1			0x3f4
#define SPR_IAC2			0x3f5
#define SPR_ICCR			0x3fb
#define SPR_ICDBDR			0x3d3
#define SPR_LR				0x8
#define SPR_PBL1			0x3fc
#define SPR_PBL2			0x3fe
#define SPR_PBU1			0x3fd
#define SPR_PBU2			0x3ff
#define SPR_PIT				0x3db
#define SPR_PVR				0x11f
#define SPR_SPRG0			0x110
#define SPR_SPRG1			0x111
#define SPR_SPRG2			0x112
#define SPR_SPRG3			0x113
#define SPR_SRR0			0x1a
#define SPR_SRR1			0x1b
#define SPR_SRR2			0x3de
#define SPR_SRR3			0x3df
#define SPR_TBHI			0x3dc
#define SPR_TBLO			0x3dd
#define SPR_TCR				0x3da
#define SPR_TSR				0x3d8
#define SPR_XER				0x1

#define DCR_BEAR			0x90
#define DCR_BESR			0x91
#define DCR_BR0				0x80
#define DCR_BR1				0x81
#define DCR_BR2				0x82
#define DCR_BR3				0x83
#define DCR_BR4				0x84
#define DCR_BR5				0x85
#define DCR_BR6				0x86
#define DCR_BR7				0x87
#define DCR_DMACC0			0xc4
#define DCR_DMACC1			0xcc
#define DCR_DMACC2			0xd4
#define DCR_DMACC3			0xdc
#define DCR_DMACR0			0xc0
#define DCR_DMACR1			0xc8
#define DCR_DMACR2			0xd0
#define DCR_DMACR3			0xd8
#define DCR_DMACT0			0xc1
#define DCR_DMACT1			0xc9
#define DCR_DMACT2			0xd1
#define DCR_DMACT3			0xd9
#define DCR_DMADA0			0xc2
#define DCR_DMADA1			0xca
#define DCR_DMADA2			0xd2
#define DCR_DMADA3			0xda
#define DCR_DMASA0			0xc3
#define DCR_DMASA1			0xcb
#define DCR_DMASA2			0xd3
#define DCR_DMASA3			0xdb
#define DCR_DMASR			0xe0
#define DCR_EXIER			0x42
#define DCR_EXISR			0x40
#define DCR_IOCR			0xa0

enum {
	EXCEPTION_IRQ
};

enum {
	PPC_PC=1,
	PPC_MSR,
	PPC_CR,
	PPC_LR,
	PPC_CTR,
	PPC_XER,
	PPC_R0,
	PPC_R1,
	PPC_R2,
	PPC_R3,
	PPC_R4,
	PPC_R5,
	PPC_R6,
	PPC_R7,
	PPC_R8,
	PPC_R9,
	PPC_R10,
	PPC_R11,
	PPC_R12,
	PPC_R13,
	PPC_R14,
	PPC_R15,
	PPC_R16,
	PPC_R17,
	PPC_R18,
	PPC_R19,
	PPC_R20,
	PPC_R21,
	PPC_R22,
	PPC_R23,
	PPC_R24,
	PPC_R25,
	PPC_R26,
	PPC_R27,
	PPC_R28,
	PPC_R29,
	PPC_R30,
	PPC_R31
};

#if (HAS_PPC403)
void ppc403_get_info(UINT32 state, union cpuinfo *info);
#endif

#ifdef MAME_DEBUG
extern int ppc_dasm_one(char *buffer, offs_t pc, UINT32 op);
#endif

#endif	/* _PPC_H */
