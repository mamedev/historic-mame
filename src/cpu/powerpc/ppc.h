#ifndef _PPC_H
#define _PPC_H

#define SPR_XER			1		/* Fixed Point Exception Register				Read / Write */
#define SPR_LR			8		/* Link Register								Read / Write */
#define SPR_CTR			9		/* Count Register								Read / Write */
#define SPR_SRR0		26		/* Save/Restore Register 0						Read / Write */
#define SPR_SRR1		27		/* Save/Restore Register 1						Read / Write */
#define SPR_SPRG0		272		/* SPR General 0								Read / Write */
#define SPR_SPRG1		273		/* SPR General 1								Read / Write */
#define SPR_SPRG2		274		/* SPR General 2								Read / Write */
#define SPR_SPRG3		275		/* SPR General 3								Read / Write */
#define SPR_PVR			287		/* Processor Version Number						Read Only */

#define SPR_ICDBDR		0x3D3	/* 406GA Instruction Cache Debug Data Register	Rad Only */
#define SPR_ESR			0x3D4	/* 406GA Exception Syndrome Register			Read / Write */
#define SPR_DEAR		0x3D5	/* 406GA Data Exception Address Register		Read Only */
#define SPR_EVPR		0x3D6	/* 406GA Exception Vector Prefix Register		Read / Write */
#define SPR_CDBCR		0x3D7	/* 406GA Cache Debug Control Register			Read/Write */
#define SPR_TSR			0x3D8	/* 406GA Timer Status Register					Read / Clear */
#define SPR_TCR			0x3DA	/* 406GA Timer Control Register					Read / Write */
#define SPR_PIT			0x3DB	/* 406GA Programmable Interval Timer			Read / Write */
#define SPR_TBHI		988		/* 406GA Time Base High							Read / Write */
#define SPR_TBLO		989		/* 406GA Time Base Low							Read / Write */
#define SPR_SRR2		0x3DE	/* 406GA Save/Restore Register 2				Read / Write */
#define SPR_SRR3		0x3DF	/* 406GA Save/Restore Register 3				Read / Write */
#define SPR_DBSR		0x3F0	/* 406GA Debug Status Register					Read / Clear */
#define SPR_DBCR		0x3F2	/* 406GA Debug Control Register					Read / Write */
#define SPR_IAC1		0x3F4	/* 406GA Instruction Address Compare 1			Read / Write */
#define SPR_IAC2		0x3F5	/* 406GA Instruction Address Compare 2			Read / Write */
#define SPR_DAC1		0x3F6	/* 406GA Data Address Compare 1					Read / Write */
#define SPR_DAC2		0x3F7	/* 406GA Data Address Compare 2					Read / Write */
#define SPR_DCCR		0x3FA	/* 406GA Data Cache Cacheability Register		Read / Write */
#define SPR_ICCR		0x3FB	/* 406GA I Cache Cacheability Registe			Read / Write */
#define SPR_PBL1		0x3FC	/* 406GA Protection Bound Lower 1				Read / Write */
#define SPR_PBU1		0x3FD	/* 406GA Protection Bound Upper 1				Read / Write */
#define SPR_PBL2		0x3FE	/* 406GA Protection Bound Lower 2				Read / Write */
#define SPR_PBU2		0x3FF	/* 406GA Protection Bound Upper 2				Read / Write */

#define SPR_DSISR		18		/* 603E */
#define SPR_DAR			19		/* 603E */
#define SPR_DEC			22		/* 603E */
#define SPR_SDR1		25		/* 603E */
#define SPR_TBL_R		268		/* 603E Time Base Low (Read-only) */
#define SPR_TBU_R		269		/* 603E Time Base High (Read-only) */
#define SPR_TBL_W		284		/* 603E Time Base Low (Write-only) */
#define SPR_TBU_W		285		/* 603E Time Base Hight (Write-only) */
#define SPR_EAR			282		/* 603E */
#define SPR_IBAT0U		528		/* 603E */
#define SPR_IBAT0L		529		/* 603E */
#define SPR_IBAT1U		530		/* 603E */
#define SPR_IBAT1L		531		/* 603E */
#define SPR_IBAT2U		532		/* 603E */
#define SPR_IBAT2L		533		/* 603E */
#define SPR_IBAT3U		534		/* 603E */
#define SPR_IBAT3L		535		/* 603E */
#define SPR_DBAT0U		536		/* 603E */
#define SPR_DBAT0L		537		/* 603E */
#define SPR_DBAT1U		538		/* 603E */
#define SPR_DBAT1L		539		/* 603E */
#define SPR_DBAT2U		540		/* 603E */
#define SPR_DBAT2L		541		/* 603E */
#define SPR_DBAT3U		542		/* 603E */
#define SPR_DBAT3L		543		/* 603E */
#define SPR_DABR		1013	/* 603E */
#define SPR_DMISS		0x3d0	/* 603E */
#define SPR_DCMP		0x3d1	/* 603E */
#define SPR_HASH1		0x3d2	/* 603E */
#define SPR_HASH2		0x3d3	/* 603E */
#define SPR_IMISS		0x3d4	/* 603E */
#define SPR_ICMP		0x3d5	/* 603E */
#define SPR_RPA			0x3d6	/* 603E */
#define SPR_HID0		1008	/* 603E */
#define SPR_HID1		1009	/* 603E */
#define SPR_IABR		1010	/* 603E */

#define DCR_BEAR		0x90	/* bus error address */
#define DCR_BESR		0x91	/* bus error syndrome */
#define DCR_BR0			0x80	/* bank */
#define DCR_BR1			0x81	/* bank */
#define DCR_BR2			0x82	/* bank */
#define DCR_BR3			0x83	/* bank */
#define DCR_BR4			0x84	/* bank */
#define DCR_BR5			0x85	/* bank */
#define DCR_BR6			0x86	/* bank */
#define DCR_BR7			0x87	/* bank */
#define DCR_DMACC0		0xc4	/* dma chained count */
#define DCR_DMACC1		0xcc	/* dma chained count */
#define DCR_DMACC2		0xd4	/* dma chained count */
#define DCR_DMACC3		0xdc	/* dma chained count */
#define DCR_DMACR0		0xc0	/* dma channel control */
#define DCR_DMACR1		0xc8	/* dma channel control */
#define DCR_DMACR2		0xd0	/* dma channel control */
#define DCR_DMACR3		0xd8	/* dma channel control */
#define DCR_DMACT0		0xc1	/* dma destination address */
#define DCR_DMACT1		0xc9	/* dma destination address */
#define DCR_DMACT2		0xd1	/* dma destination address */
#define DCR_DMACT3		0xd9	/* dma destination address */
#define DCR_DMADA0		0xc2	/* dma destination address */
#define DCR_DMADA1		0xca	/* dma destination address */
#define DCR_DMADA2		0xd2	/* dma source address */
#define DCR_DMADA3		0xda	/* dma source address */
#define DCR_DMASA0		0xc3	/* dma source address */
#define DCR_DMASA1		0xcb	/* dma source address */
#define DCR_DMASA2		0xd3	/* dma source address */
#define DCR_DMASA3		0xdb	/* dma source address */
#define DCR_DMASR		0xe0	/* dma status */
#define DCR_EXIER		0x42	/* external interrupt enable */
#define DCR_EXISR		0x40	/* external interrupt status */
#define DCR_IOCR		0xa0	/* io configuration */

enum {
	EXCEPTION_IRQ,
	EXCEPTION_DECREMENTER
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

// OSD - WARNING
typedef float FLOAT32 ;
typedef double FLOAT64 ;

#if (HAS_PPC403)
void ppc403_get_info(UINT32 state, union cpuinfo *info);
#endif

#if (HAS_PPC603)
void ppc603_get_info(UINT32 state, union cpuinfo *info);
#endif

#ifdef MAME_DEBUG
extern int ppc_dasm_one(char *buffer, offs_t pc, UINT32 op);
#endif

#endif	/* _PPC_H */
