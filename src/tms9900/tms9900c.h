#define R0  I.WP
#define R1  I.WP+2
#define R2  I.WP+4
#define R3  I.WP+6
#define R4  I.WP+8
#define R5  I.WP+10
#define R6  I.WP+12
#define R7  I.WP+14
#define R8  I.WP+16
#define R9  I.WP+18
#define R10 I.WP+20
#define R11 I.WP+22
#define R12 I.WP+24
#define R13 I.WP+26
#define R14 I.WP+28
#define R15 I.WP+30

#define PARCHECK	0x6996
#define BYTE_OPERATION	I.IR&0x1000
#define SIGN		0x8000

/* Instruction format 1 */

#define TMS9900_CYCLES_SZC	14;
#define TMS9900_CYCLES_SZCB	14;
#define TMS9900_CYCLES_S	14;
#define TMS9900_CYCLES_SB	14;
#define TMS9900_CYCLES_C	14;
#define TMS9900_CYCLES_CB	14;
#define TMS9900_CYCLES_A	14;
#define TMS9900_CYCLES_AB	14;
#define TMS9900_CYCLES_MOV	14;
#define TMS9900_CYCLES_MOVB	14;
#define TMS9900_CYCLES_SOC	14;
#define TMS9900_CYCLES_SOCB	14;

/* Instruction format 2 */

#define TMS9900_CYCLES_JMP	10;
#define TMS9900_CYCLES_JLT	10;
#define TMS9900_CYCLES_JLE	10;
#define TMS9900_CYCLES_JEQ	10;
#define TMS9900_CYCLES_JHE	10;
#define TMS9900_CYCLES_JGT	10;
#define TMS9900_CYCLES_JNE	10;
#define TMS9900_CYCLES_JNC	10;
#define TMS9900_CYCLES_JOC	10;
#define TMS9900_CYCLES_JNO	10;
#define TMS9900_CYCLES_JL	10;
#define TMS9900_CYCLES_JH	10;
#define TMS9900_CYCLES_JOP	10;
#define TMS9900_CYCLES_SBO	12;
#define TMS9900_CYCLES_SBZ	12;
#define TMS9900_CYCLES_TB	12;

/* Instruction format 3 */

#define TMS9900_CYCLES_COC	14;
#define TMS9900_CYCLES_CZC	14;
#define TMS9900_CYCLES_XOR	14;

/* Instruction format 4 */

#define TMS9900_CYCLES_LDCR	52;
#define TMS9900_CYCLES_STCR	60;

/* Instruction format 5 */

#define TMS9900_CYCLES_SRA	52;
#define TMS9900_CYCLES_SRC	52;
#define TMS9900_CYCLES_SRL	52;
#define TMS9900_CYCLES_SLA	52;

/* Instruction format 6 */

#define TMS9900_CYCLES_BLWP	26;
#define TMS9900_CYCLES_B	 8;
#define TMS9900_CYCLES_eX	 8;
#define TMS9900_CYCLES_CLR	10;
#define TMS9900_CYCLES_NEG	12;
#define TMS9900_CYCLES_INV	10;
#define TMS9900_CYCLES_INC	10;
#define TMS9900_CYCLES_INCT	10;
#define TMS9900_CYCLES_DEC	10;
#define TMS9900_CYCLES_DECT	10;
#define TMS9900_CYCLES_BL	12;
#define TMS9900_CYCLES_SWPB	10;
#define TMS9900_CYCLES_SETO	10;
#define TMS9900_CYCLES_ABSv	14;

/* Instruction format 7 */

#define TMS9900_CYCLES_IDLE	12;
#define TMS9900_CYCLES_RSET	12;
#define TMS9900_CYCLES_RTWP	14;
#define TMS9900_CYCLES_CKON	12;
#define TMS9900_CYCLES_CKOF	12;
#define TMS9900_CYCLES_LREX	12;

/* Instruction format 8 */

#define TMS9900_CYCLES_LI	12;
#define TMS9900_CYCLES_AI	14;
#define TMS9900_CYCLES_ANDI	14;
#define TMS9900_CYCLES_ORI	14;
#define TMS9900_CYCLES_CI	14;
#define TMS9900_CYCLES_STWP	 8;
#define TMS9900_CYCLES_STST	 8;
#define TMS9900_CYCLES_LWPI	10;
#define TMS9900_CYCLES_LIMI	16;

/* Instruction format 9 */

#define TMS9900_CYCLES_XOP	36;
#define TMS9900_CYCLES_MPY	52;
#define TMS9900_CYCLES_DIV	16;

word	LGT,AGT,EQ,CY,OV,OP,X,IMASK;	/* Individual bits in the status reg. */

word	SourceOpAdr,	/* address in memory of source operand */
	SourceVal,	/* contents of source operand */
	DestOpAdr,	/* address in memory of destination operand */
	DestVal,	/* value of destination operand */
	CRUBaseAdr;	/* value of CRU base address (R12&1FFE)>>1 */

void	typeIoperands(void);
void	typeIIIoperands(void);
void	typeIVoperands(void);
void	typeVoperands(void);
void	typeVIoperands(void);
void	typeVIIoperands(void);
void	typeVIIIoperands(void);
void	typeIXoperands(void);
void	typeXoperands(void);

void	get_source_operand(void);
void	get_dest_operand(void);

void	do_jmp(void);

void	set_status(word);
word	get_status(void);
void	set_parity(word);
void	set_mask(word);
word	add(word,word);
void	compare(word,word);

void	TAB2(void);

void	SRA(void);
void	SRL(void);
void	SLA(void);
void	SRC(void);
void	SZC(void);
void	SZCB(void);
void	S(void);
void	SB(void);
void	C(void);
void	CB(void);
void	A(void);
void	AB(void);
void	MOV(void);
void	MOVB(void);
void	SOC(void);
void	SOCB(void);
void	JMP(void);
void	JLT(void);
void	JLE(void);
void	JEQ(void);
void	JHE(void);
void	JGT(void);
void	JNE(void);
void	JNC(void);
void	JOC(void);
void	JNO(void);
void	JL(void);
void	JH(void);
void	JOP(void);
void	SBO(void);
void	SBZ(void);
void	TB(void);
void	COC(void);
void	CZC(void);
void	XOR(void);
void	XOP(void);
void	LDCR(void);
void	STCR(void);
void	MPY(void);
void	DIV(void);
void	LI(void);
void	AI(void);
void	ANDI(void);
void	ORI(void);
void	CI(void);
void	STWP(void);
void	STST(void);
void	LWPI(void);
void	LIMI(void);
void	RTWP(void);
void	BLWP(void);
void	B(void);
void	eX(void);
void	CLR(void);
void	NEG(void);
void	INV(void);
void	INC(void);
void	INCT(void);
void	DEC(void);
void	DECT(void);
void	BL(void);
void	SWPB(void);
void	SETO(void);
void	ABSv(void);
void	IDLE(void);
void	RSET(void);
void	CKON(void);
void	CKOF(void);
void	LREX(void);
void	ILL(void);

void (*opcode_table[]) (void) =
{
	TAB2,TAB2,TAB2,TAB2,TAB2,TAB2,TAB2,TAB2, 	/* 00-0F */
	SRA ,SRL ,SLA ,SRC ,ILL ,ILL ,ILL ,ILL ,
	JMP ,JLT ,JLE ,JEQ ,JHE ,JGT ,JNE ,JNC ,        /* 10-1F */
	JOC ,JNO ,JL  ,JH  ,JOP ,SBO ,SBZ ,TB  ,
	COC ,COC ,COC ,COC ,CZC ,CZC ,CZC ,CZC ,	/* 20-2F */
	XOR ,XOR ,XOR ,XOR ,XOP ,XOP ,XOP ,XOP ,
	LDCR,LDCR,LDCR,LDCR,STCR,STCR,STCR,STCR,	/* 30-3F */
	MPY ,MPY ,MPY ,MPY ,DIV ,DIV ,DIV ,DIV ,
	SZC ,SZC ,SZC ,SZC ,SZC ,SZC ,SZC ,SZC ,	/* 40-4F */
	SZC ,SZC ,SZC ,SZC ,SZC ,SZC ,SZC ,SZC ,
	SZCB,SZCB,SZCB,SZCB,SZCB,SZCB,SZCB,SZCB,	/* 50-5F */
	SZCB,SZCB,SZCB,SZCB,SZCB,SZCB,SZCB,SZCB,
	S   ,S   ,S   ,S   ,S   ,S   ,S   ,S   ,	/* 60-6F */
	S   ,S   ,S   ,S   ,S   ,S   ,S   ,S   ,
	SB  ,SB  ,SB  ,SB  ,SB  ,SB  ,SB  ,SB  ,	/* 70-7F */
	SB  ,SB  ,SB  ,SB  ,SB  ,SB  ,SB  ,SB  ,
	C   ,C   ,C   ,C   ,C   ,C   ,C   ,C   ,	/* 80-7F */
	C   ,C   ,C   ,C   ,C   ,C   ,C   ,C   ,
	CB  ,CB  ,CB  ,CB  ,CB  ,CB  ,CB  ,CB  ,	/* 90-7F */
	CB  ,CB  ,CB  ,CB  ,CB  ,CB  ,CB  ,CB  ,
	A   ,A   ,A   ,A   ,A   ,A   ,A   ,A   ,	/* A0-AF */
	A   ,A   ,A   ,A   ,A   ,A   ,A   ,A   ,
	AB  ,AB  ,AB  ,AB  ,AB  ,AB  ,AB  ,AB  ,	/* B0-BF */
	AB  ,AB  ,AB  ,AB  ,AB  ,AB  ,AB  ,AB  ,
	MOV ,MOV ,MOV ,MOV ,MOV ,MOV ,MOV ,MOV ,	/* C0-CF */
	MOV ,MOV ,MOV ,MOV ,MOV ,MOV ,MOV ,MOV ,
	MOVB,MOVB,MOVB,MOVB,MOVB,MOVB,MOVB,MOVB,	/* D0-DF */
	MOVB,MOVB,MOVB,MOVB,MOVB,MOVB,MOVB,MOVB,
	SOC ,SOC ,SOC ,SOC ,SOC ,SOC ,SOC ,SOC ,	/* E0-EF */
	SOC ,SOC ,SOC ,SOC ,SOC ,SOC ,SOC ,SOC ,
	SOCB,SOCB,SOCB,SOCB,SOCB,SOCB,SOCB,SOCB,	/* F0-FF */
	SOCB,SOCB,SOCB,SOCB,SOCB,SOCB,SOCB,SOCB
} ;

void (*low_opcode_table[]) (void) =
{
	ILL ,ILL ,ILL ,ILL ,ILL ,ILL ,ILL ,ILL ,	/* 0000-00FF */
	ILL ,ILL ,ILL ,ILL ,ILL ,ILL ,ILL ,ILL ,	/* 0100-01FF */
	LI  ,AI  ,ANDI,ORI ,CI  ,STWP,STST,LWPI,        /* 0200-02FF */
	LIMI,ILL ,IDLE,RSET,RTWP,CKON,CKOF,LREX,	/* 0300-03FF */
	BLWP,BLWP,B   ,B   ,eX  ,eX  ,CLR ,CLR ,	/* 0400-04FF */
	NEG ,NEG ,INV ,INV ,INC ,INC ,INCT,INCT,	/* 0500-05FF */
	DEC ,DEC ,DECT,DECT,BL  ,BL  ,SWPB,SWPB,	/* 0600-06FF */
	SETO,SETO,ABSv,ABSv,ILL ,ILL ,ILL ,ILL 		/* 0700-07FF */
} ;

