/***************************************************************************

  memory.c

  Functions which handle the CPU memory and I/O port access.

  Caveats:

  * The install_mem/port_*_handler functions are only intended to be
	called at driver init time. Do not call them after this time.

  * If your driver executes an opcode which crosses a bank-switched
	boundary, it will pull the wrong data out of memory. Although not
	a common case, you may need to revert to memcpy to work around this.
	See machine/tnzs.c for an example.

***************************************************************************/

#include "driver.h"
#include "osd_cpu.h"


#define VERBOSE 0

/* #define MEM_DUMP */

#ifdef MEM_DUMP
static void mem_dump( void );
#endif

/* Convenience macros - not in cpuintrf.h because they shouldn't be used by everyone */
#define ADDRESS_BITS(index) (cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].address_bits)
#define ABITS1(index)		(cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].abits1)
#define ABITS2(index)		(cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].abits2)
#define ABITS3(index)		(0)
#define ABITSMIN(index) 	(cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].abitsmin)
#define ALIGNUNIT(index)	(cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].align_unit)

//#define READ_WORD(a)		(*(data16_t *)(a))
//#define WRITE_WORD(a,d) 	(*(data16_t *)(a) = (d))
#define READ_DWORD(a)		(*(data32_t *)(a))
#define WRITE_DWORD(a,d)	(*(data32_t *)(a) = (d))

#ifdef LSB_FIRST
	#define BYTE_XOR_BE(a) ((a) ^ 1)
	#define BYTE_XOR_LE(a) (a)
	#define BYTE4_XOR_BE(a) ((a) ^ 3)
	#define BYTE4_XOR_LE(a) (a)
	#define WORD_XOR_BE(a) ((a) ^ 2)
	#define WORD_XOR_LE(a) (a)
#else
	#define BYTE_XOR_BE(a) (a)
	#define BYTE_XOR_LE(a) ((a) ^ 1)
	#define BYTE4_XOR_BE(a) (a)
	#define BYTE4_XOR_LE(a) ((a) ^ 3)
	#define WORD_XOR_BE(a) (a)
	#define WORD_XOR_LE(a) ((a) ^ 2)
#endif

UINT8 *OP_RAM;
UINT8 *OP_ROM;

/* change bases preserving opcode/data shift for encrypted games */
#define SET_OP_RAMROM(base) 				\
	OP_ROM = (base) + (OP_ROM - OP_RAM);	\
	OP_RAM = (base);


MHELE ophw; 			/* op-code hardware number */

struct ExtMemory ext_memory[MAX_EXT_MEMORY];

static UINT8 *ramptr[MAX_CPU], *romptr[MAX_CPU];

/* element shift bits, mask bits */
int mhshift[MAX_CPU][3], mhmask[MAX_CPU][3];

/* global memory access width and mask (16-bit and 32-bit under-size accesses) */
//UINT32 mem_width;
//UINT32 mem_mask;
//UINT32 mem_offs;

/* pointers to port structs */
/* ASG: port speedup */
static struct IO_ReadPort *readport[MAX_CPU];
static struct IO_WritePort *writeport[MAX_CPU];
static int portmask[MAX_CPU];
static int readport_size[MAX_CPU];
static int writeport_size[MAX_CPU];
static const struct IO_ReadPort *cur_readport;
static const struct IO_WritePort *cur_writeport;
int cur_portmask;

/* current hardware element map */
static MHELE *cur_mr_element[MAX_CPU];
static MHELE *cur_mw_element[MAX_CPU];

/* sub memory/port hardware element map */
static MHELE readhardware[MH_ELEMAX << MH_SBITS];  /* mem/port read  */
static MHELE writehardware[MH_ELEMAX << MH_SBITS]; /* mem/port write */

/* memory hardware element map */
/* value:					   */
#define HT_RAM	  0 	/* RAM direct		 */
#define HT_BANK1  1 	/* bank memory #1	 */
#define HT_BANK2  2 	/* bank memory #2	 */
#define HT_BANK3  3 	/* bank memory #3	 */
#define HT_BANK4  4 	/* bank memory #4	 */
#define HT_BANK5  5 	/* bank memory #5	 */
#define HT_BANK6  6 	/* bank memory #6	 */
#define HT_BANK7  7 	/* bank memory #7	 */
#define HT_BANK8  8 	/* bank memory #8	 */
#define HT_BANK9  9 	/* bank memory #9	 */
#define HT_BANK10 10	/* bank memory #10	 */
#define HT_BANK11 11	/* bank memory #11	 */
#define HT_BANK12 12	/* bank memory #12	 */
#define HT_BANK13 13	/* bank memory #13	 */
#define HT_BANK14 14	/* bank memory #14	 */
#define HT_BANK15 15	/* bank memory #15	 */
#define HT_BANK16 16	/* bank memory #16	 */
#define HT_BANK17 17	/* bank memory #17	 */
#define HT_BANK18 18	/* bank memory #18	 */
#define HT_BANK19 19	/* bank memory #19	 */
#define HT_BANK20 20	/* bank memory #20	 */
#define HT_NON	  21	/* non mapped memory */
#define HT_NOP	  22	/* NOP memory		 */
#define HT_RAMROM 23	/* RAM ROM memory	 */
#define HT_ROM	  24	/* ROM memory		 */

#define HT_USER   25	/* user functions	 */
/* [MH_HARDMAX]-0xff	  link to sub memory element  */
/*						  (value-MH_HARDMAX)<<MH_SBITS -> element bank */

#define HT_BANKMAX (HT_BANK1 + MAX_BANKS - 1)

/* memory hardware handler */
static offs_t memoryreadoffset[MH_HARDMAX];
static offs_t memorywriteoffset[MH_HARDMAX];
static mem_read_handler memoryreadhandler[MH_HARDMAX];
static mem_write_handler memorywritehandler[MH_HARDMAX];
static mem_read16_handler memoryread16handler[MH_HARDMAX];
static mem_write16_handler memorywrite16handler[MH_HARDMAX];
static mem_read32_handler memoryread32handler[MH_HARDMAX];
static mem_write32_handler memorywrite32handler[MH_HARDMAX];

static void *internal_install_mem_read_handler(int cpu, int start, int end, mem_read_handler handler);
static void *internal_install_mem_write_handler(int cpu, int start, int end, mem_write_handler handler);

/* bank ram base address; RAM is bank 0 */
UINT8 *cpu_bankbase[HT_BANKMAX + 1];
static int bankreadoffset[HT_BANKMAX + 1];
static int bankwriteoffset[HT_BANKMAX + 1];

/* override OP base handler */
static opbase_handler setOPbasefunc[MAX_CPU];
static opbase_handler OPbasefunc;

/* current cpu current hardware element map point */
MHELE *cur_mrhard;
MHELE *cur_mwhard;

static void *install_port_read_handler_common(int cpu, int start, int end, mem_read_handler handler, int install_at_beginning);
static void *install_port_write_handler_common(int cpu, int start, int end, mem_write_handler handler, int install_at_beginning);


/***************************************************************************

  Memory read handling

***************************************************************************/

READ_HANDLER(mrh_ram)			{ return cpu_bankbase[0][offset]; }
READ_HANDLER(mrh_bank1) 		{ return cpu_bankbase[1][offset]; }
READ_HANDLER(mrh_bank2) 		{ return cpu_bankbase[2][offset]; }
READ_HANDLER(mrh_bank3) 		{ return cpu_bankbase[3][offset]; }
READ_HANDLER(mrh_bank4) 		{ return cpu_bankbase[4][offset]; }
READ_HANDLER(mrh_bank5) 		{ return cpu_bankbase[5][offset]; }
READ_HANDLER(mrh_bank6) 		{ return cpu_bankbase[6][offset]; }
READ_HANDLER(mrh_bank7) 		{ return cpu_bankbase[7][offset]; }
READ_HANDLER(mrh_bank8) 		{ return cpu_bankbase[8][offset]; }
READ_HANDLER(mrh_bank9) 		{ return cpu_bankbase[9][offset]; }
READ_HANDLER(mrh_bank10)		{ return cpu_bankbase[10][offset]; }
READ_HANDLER(mrh_bank11)		{ return cpu_bankbase[11][offset]; }
READ_HANDLER(mrh_bank12)		{ return cpu_bankbase[12][offset]; }
READ_HANDLER(mrh_bank13)		{ return cpu_bankbase[13][offset]; }
READ_HANDLER(mrh_bank14)		{ return cpu_bankbase[14][offset]; }
READ_HANDLER(mrh_bank15)		{ return cpu_bankbase[15][offset]; }
READ_HANDLER(mrh_bank16)		{ return cpu_bankbase[16][offset]; }
READ_HANDLER(mrh_bank17)		{ return cpu_bankbase[17][offset]; }
READ_HANDLER(mrh_bank18)		{ return cpu_bankbase[18][offset]; }
READ_HANDLER(mrh_bank19)		{ return cpu_bankbase[19][offset]; }
READ_HANDLER(mrh_bank20)		{ return cpu_bankbase[20][offset]; }
READ16_HANDLER(mrh_ram16)		{ return ((data16_t *)cpu_bankbase[0])[offset]; }
READ16_HANDLER(mrh_bank1_16) 	{ return ((data16_t *)cpu_bankbase[1])[offset]; }
READ16_HANDLER(mrh_bank2_16) 	{ return ((data16_t *)cpu_bankbase[2])[offset]; }
READ16_HANDLER(mrh_bank3_16) 	{ return ((data16_t *)cpu_bankbase[3])[offset]; }
READ16_HANDLER(mrh_bank4_16) 	{ return ((data16_t *)cpu_bankbase[4])[offset]; }
READ16_HANDLER(mrh_bank5_16) 	{ return ((data16_t *)cpu_bankbase[5])[offset]; }
READ16_HANDLER(mrh_bank6_16) 	{ return ((data16_t *)cpu_bankbase[6])[offset]; }
READ16_HANDLER(mrh_bank7_16) 	{ return ((data16_t *)cpu_bankbase[7])[offset]; }
READ16_HANDLER(mrh_bank8_16) 	{ return ((data16_t *)cpu_bankbase[8])[offset]; }
READ16_HANDLER(mrh_bank9_16) 	{ return ((data16_t *)cpu_bankbase[9])[offset]; }
READ16_HANDLER(mrh_bank10_16)	{ return ((data16_t *)cpu_bankbase[10])[offset]; }
READ16_HANDLER(mrh_bank11_16)	{ return ((data16_t *)cpu_bankbase[11])[offset]; }
READ16_HANDLER(mrh_bank12_16)	{ return ((data16_t *)cpu_bankbase[12])[offset]; }
READ16_HANDLER(mrh_bank13_16)	{ return ((data16_t *)cpu_bankbase[13])[offset]; }
READ16_HANDLER(mrh_bank14_16)	{ return ((data16_t *)cpu_bankbase[14])[offset]; }
READ16_HANDLER(mrh_bank15_16)	{ return ((data16_t *)cpu_bankbase[15])[offset]; }
READ16_HANDLER(mrh_bank16_16)	{ return ((data16_t *)cpu_bankbase[16])[offset]; }
READ16_HANDLER(mrh_bank17_16)	{ return ((data16_t *)cpu_bankbase[17])[offset]; }
READ16_HANDLER(mrh_bank18_16)	{ return ((data16_t *)cpu_bankbase[18])[offset]; }
READ16_HANDLER(mrh_bank19_16)	{ return ((data16_t *)cpu_bankbase[19])[offset]; }
READ16_HANDLER(mrh_bank20_16)	{ return ((data16_t *)cpu_bankbase[20])[offset]; }
READ32_HANDLER(mrh_ram32)		{ return ((data32_t *)cpu_bankbase[0])[offset]; }
READ32_HANDLER(mrh_bank1_32) 	{ return ((data32_t *)cpu_bankbase[1])[offset]; }
READ32_HANDLER(mrh_bank2_32) 	{ return ((data32_t *)cpu_bankbase[2])[offset]; }
READ32_HANDLER(mrh_bank3_32) 	{ return ((data32_t *)cpu_bankbase[3])[offset]; }
READ32_HANDLER(mrh_bank4_32) 	{ return ((data32_t *)cpu_bankbase[4])[offset]; }
READ32_HANDLER(mrh_bank5_32) 	{ return ((data32_t *)cpu_bankbase[5])[offset]; }
READ32_HANDLER(mrh_bank6_32) 	{ return ((data32_t *)cpu_bankbase[6])[offset]; }
READ32_HANDLER(mrh_bank7_32) 	{ return ((data32_t *)cpu_bankbase[7])[offset]; }
READ32_HANDLER(mrh_bank8_32) 	{ return ((data32_t *)cpu_bankbase[8])[offset]; }
READ32_HANDLER(mrh_bank9_32) 	{ return ((data32_t *)cpu_bankbase[9])[offset]; }
READ32_HANDLER(mrh_bank10_32)	{ return ((data32_t *)cpu_bankbase[10])[offset]; }
READ32_HANDLER(mrh_bank11_32)	{ return ((data32_t *)cpu_bankbase[11])[offset]; }
READ32_HANDLER(mrh_bank12_32)	{ return ((data32_t *)cpu_bankbase[12])[offset]; }
READ32_HANDLER(mrh_bank13_32)	{ return ((data32_t *)cpu_bankbase[13])[offset]; }
READ32_HANDLER(mrh_bank14_32)	{ return ((data32_t *)cpu_bankbase[14])[offset]; }
READ32_HANDLER(mrh_bank15_32)	{ return ((data32_t *)cpu_bankbase[15])[offset]; }
READ32_HANDLER(mrh_bank16_32)	{ return ((data32_t *)cpu_bankbase[16])[offset]; }
READ32_HANDLER(mrh_bank17_32)	{ return ((data32_t *)cpu_bankbase[17])[offset]; }
READ32_HANDLER(mrh_bank18_32)	{ return ((data32_t *)cpu_bankbase[18])[offset]; }
READ32_HANDLER(mrh_bank19_32)	{ return ((data32_t *)cpu_bankbase[19])[offset]; }
READ32_HANDLER(mrh_bank20_32)	{ return ((data32_t *)cpu_bankbase[20])[offset]; }
static mem_read_handler bank_read_handler[] =
{
	mrh_ram,
	mrh_bank1, mrh_bank2, mrh_bank3, mrh_bank4, mrh_bank5,
	mrh_bank6, mrh_bank7, mrh_bank8, mrh_bank9, mrh_bank10,
	mrh_bank11,mrh_bank12,mrh_bank13,mrh_bank14,mrh_bank15,
	mrh_bank16,mrh_bank17,mrh_bank18,mrh_bank19,mrh_bank20
};
static mem_read16_handler bank_read16_handler[] =
{
	mrh_ram16,
	mrh_bank1_16, mrh_bank2_16, mrh_bank3_16, mrh_bank4_16, mrh_bank5_16,
	mrh_bank6_16, mrh_bank7_16, mrh_bank8_16, mrh_bank9_16, mrh_bank10_16,
	mrh_bank11_16,mrh_bank12_16,mrh_bank13_16,mrh_bank14_16,mrh_bank15_16,
	mrh_bank16_16,mrh_bank17_16,mrh_bank18_16,mrh_bank19_16,mrh_bank20_16
};
static mem_read32_handler bank_read32_handler[] =
{
	mrh_ram32,
	mrh_bank1_32, mrh_bank2_32, mrh_bank3_32, mrh_bank4_32, mrh_bank5_32,
	mrh_bank6_32, mrh_bank7_32, mrh_bank8_32, mrh_bank9_32, mrh_bank10_32,
	mrh_bank11_32,mrh_bank12_32,mrh_bank13_32,mrh_bank14_32,mrh_bank15_32,
	mrh_bank16_32,mrh_bank17_32,mrh_bank18_32,mrh_bank19_32,mrh_bank20_32
};

static READ_HANDLER(mrh_error)
{
	logerror("CPU #%d PC %04x: warning - read %02x from unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),cpu_bankbase[0][offset],offset);
	return cpu_bankbase[0][offset];
}

static READ16_HANDLER(mrh_error16)
{
	logerror("CPU #%d PC %04x: warning - read %02x from unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),((data16_t *)cpu_bankbase[0])[offset],offset*2);
	return ((data16_t *)cpu_bankbase[0])[offset];
}

static READ32_HANDLER(mrh_error32)
{
	logerror("CPU #%d PC %04x: warning - read %02x from unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),((data32_t *)cpu_bankbase[0])[offset],offset*4);
	return ((data32_t *)cpu_bankbase[0])[offset];
}

static READ_HANDLER(mrh_error_sparse)
{
	logerror("CPU #%d PC %08x: warning - read unmapped memory address %08x\n",cpu_getactivecpu(),cpu_get_pc(),offset);
	return 0;
}

static READ16_HANDLER(mrh_error_sparse16)
{
	logerror("CPU #%d PC %08x: warning - read unmapped memory address %08x\n",cpu_getactivecpu(),cpu_get_pc(),offset*2);
	return 0;
}

static READ32_HANDLER(mrh_error_sparse32)
{
	logerror("CPU #%d PC %08x: warning - read unmapped memory address %08x\n",cpu_getactivecpu(),cpu_get_pc(),offset*4);
	return 0;
}

static READ_HANDLER(mrh_error_sparse_bit)
{
	logerror("CPU #%d PC %08x: warning - read unmapped memory bit addr %08x (byte addr %08x)\n",cpu_getactivecpu(),cpu_get_pc(),(offset<<3), offset);
	return 0;
}

static READ16_HANDLER(mrh_error_sparse_bit16)
{
	logerror("CPU #%d PC %08x: warning - read unmapped memory bit addr %08x (byte addr %08x)\n",cpu_getactivecpu(),cpu_get_pc(),(offset<<3)*2, offset*2);
	return 0;
}

static READ32_HANDLER(mrh_error_sparse_bit32)
{
	logerror("CPU #%d PC %08x: warning - read unmapped memory bit addr %08x (byte addr %08x)\n",cpu_getactivecpu(),cpu_get_pc(),(offset<<3)*4, offset*4);
	return 0;
}

static READ_HANDLER(mrh_nop) { return 0; }
static READ16_HANDLER(mrh_nop16) { return 0; }
static READ32_HANDLER(mrh_nop32) { return 0; }


/***************************************************************************

  Memory write handling

***************************************************************************/

WRITE_HANDLER(mwh_ram)			{ cpu_bankbase[0][offset] = data; }
WRITE_HANDLER(mwh_bank1)		{ cpu_bankbase[1][offset] = data; }
WRITE_HANDLER(mwh_bank2)		{ cpu_bankbase[2][offset] = data; }
WRITE_HANDLER(mwh_bank3)		{ cpu_bankbase[3][offset] = data; }
WRITE_HANDLER(mwh_bank4)		{ cpu_bankbase[4][offset] = data; }
WRITE_HANDLER(mwh_bank5)		{ cpu_bankbase[5][offset] = data; }
WRITE_HANDLER(mwh_bank6)		{ cpu_bankbase[6][offset] = data; }
WRITE_HANDLER(mwh_bank7)		{ cpu_bankbase[7][offset] = data; }
WRITE_HANDLER(mwh_bank8)		{ cpu_bankbase[8][offset] = data; }
WRITE_HANDLER(mwh_bank9)		{ cpu_bankbase[9][offset] = data; }
WRITE_HANDLER(mwh_bank10)		{ cpu_bankbase[10][offset] = data; }
WRITE_HANDLER(mwh_bank11)		{ cpu_bankbase[11][offset] = data; }
WRITE_HANDLER(mwh_bank12)		{ cpu_bankbase[12][offset] = data; }
WRITE_HANDLER(mwh_bank13)		{ cpu_bankbase[13][offset] = data; }
WRITE_HANDLER(mwh_bank14)		{ cpu_bankbase[14][offset] = data; }
WRITE_HANDLER(mwh_bank15)		{ cpu_bankbase[15][offset] = data; }
WRITE_HANDLER(mwh_bank16)		{ cpu_bankbase[16][offset] = data; }
WRITE_HANDLER(mwh_bank17)		{ cpu_bankbase[17][offset] = data; }
WRITE_HANDLER(mwh_bank18)		{ cpu_bankbase[18][offset] = data; }
WRITE_HANDLER(mwh_bank19)		{ cpu_bankbase[19][offset] = data; }
WRITE_HANDLER(mwh_bank20)		{ cpu_bankbase[20][offset] = data; }
WRITE16_HANDLER(mwh_ram16)		{ ((data16_t *)cpu_bankbase[0])[offset] = data; }
WRITE16_HANDLER(mwh_bank1_16)	{ ((data16_t *)cpu_bankbase[1])[offset] = data; }
WRITE16_HANDLER(mwh_bank2_16)	{ ((data16_t *)cpu_bankbase[2])[offset] = data; }
WRITE16_HANDLER(mwh_bank3_16)	{ ((data16_t *)cpu_bankbase[3])[offset] = data; }
WRITE16_HANDLER(mwh_bank4_16)	{ ((data16_t *)cpu_bankbase[4])[offset] = data; }
WRITE16_HANDLER(mwh_bank5_16)	{ ((data16_t *)cpu_bankbase[5])[offset] = data; }
WRITE16_HANDLER(mwh_bank6_16)	{ ((data16_t *)cpu_bankbase[6])[offset] = data; }
WRITE16_HANDLER(mwh_bank7_16)	{ ((data16_t *)cpu_bankbase[7])[offset] = data; }
WRITE16_HANDLER(mwh_bank8_16)	{ ((data16_t *)cpu_bankbase[8])[offset] = data; }
WRITE16_HANDLER(mwh_bank9_16)	{ ((data16_t *)cpu_bankbase[9])[offset] = data; }
WRITE16_HANDLER(mwh_bank10_16)	{ ((data16_t *)cpu_bankbase[10])[offset] = data; }
WRITE16_HANDLER(mwh_bank11_16)	{ ((data16_t *)cpu_bankbase[11])[offset] = data; }
WRITE16_HANDLER(mwh_bank12_16)	{ ((data16_t *)cpu_bankbase[12])[offset] = data; }
WRITE16_HANDLER(mwh_bank13_16)	{ ((data16_t *)cpu_bankbase[13])[offset] = data; }
WRITE16_HANDLER(mwh_bank14_16)	{ ((data16_t *)cpu_bankbase[14])[offset] = data; }
WRITE16_HANDLER(mwh_bank15_16)	{ ((data16_t *)cpu_bankbase[15])[offset] = data; }
WRITE16_HANDLER(mwh_bank16_16)	{ ((data16_t *)cpu_bankbase[16])[offset] = data; }
WRITE16_HANDLER(mwh_bank17_16)	{ ((data16_t *)cpu_bankbase[17])[offset] = data; }
WRITE16_HANDLER(mwh_bank18_16)	{ ((data16_t *)cpu_bankbase[18])[offset] = data; }
WRITE16_HANDLER(mwh_bank19_16)	{ ((data16_t *)cpu_bankbase[19])[offset] = data; }
WRITE16_HANDLER(mwh_bank20_16)	{ ((data16_t *)cpu_bankbase[20])[offset] = data; }
WRITE32_HANDLER(mwh_ram32)		{ ((data32_t *)cpu_bankbase[0])[offset] = data; }
WRITE32_HANDLER(mwh_bank1_32)	{ ((data32_t *)cpu_bankbase[1])[offset] = data; }
WRITE32_HANDLER(mwh_bank2_32)	{ ((data32_t *)cpu_bankbase[2])[offset] = data; }
WRITE32_HANDLER(mwh_bank3_32)	{ ((data32_t *)cpu_bankbase[3])[offset] = data; }
WRITE32_HANDLER(mwh_bank4_32)	{ ((data32_t *)cpu_bankbase[4])[offset] = data; }
WRITE32_HANDLER(mwh_bank5_32)	{ ((data32_t *)cpu_bankbase[5])[offset] = data; }
WRITE32_HANDLER(mwh_bank6_32)	{ ((data32_t *)cpu_bankbase[6])[offset] = data; }
WRITE32_HANDLER(mwh_bank7_32)	{ ((data32_t *)cpu_bankbase[7])[offset] = data; }
WRITE32_HANDLER(mwh_bank8_32)	{ ((data32_t *)cpu_bankbase[8])[offset] = data; }
WRITE32_HANDLER(mwh_bank9_32)	{ ((data32_t *)cpu_bankbase[9])[offset] = data; }
WRITE32_HANDLER(mwh_bank10_32)	{ ((data32_t *)cpu_bankbase[10])[offset] = data; }
WRITE32_HANDLER(mwh_bank11_32)	{ ((data32_t *)cpu_bankbase[11])[offset] = data; }
WRITE32_HANDLER(mwh_bank12_32)	{ ((data32_t *)cpu_bankbase[12])[offset] = data; }
WRITE32_HANDLER(mwh_bank13_32)	{ ((data32_t *)cpu_bankbase[13])[offset] = data; }
WRITE32_HANDLER(mwh_bank14_32)	{ ((data32_t *)cpu_bankbase[14])[offset] = data; }
WRITE32_HANDLER(mwh_bank15_32)	{ ((data32_t *)cpu_bankbase[15])[offset] = data; }
WRITE32_HANDLER(mwh_bank16_32)	{ ((data32_t *)cpu_bankbase[16])[offset] = data; }
WRITE32_HANDLER(mwh_bank17_32)	{ ((data32_t *)cpu_bankbase[17])[offset] = data; }
WRITE32_HANDLER(mwh_bank18_32)	{ ((data32_t *)cpu_bankbase[18])[offset] = data; }
WRITE32_HANDLER(mwh_bank19_32)	{ ((data32_t *)cpu_bankbase[19])[offset] = data; }
WRITE32_HANDLER(mwh_bank20_32)	{ ((data32_t *)cpu_bankbase[20])[offset] = data; }
static mem_write_handler bank_write_handler[] =
{
	mwh_ram,
	mwh_bank1, mwh_bank2, mwh_bank3, mwh_bank4, mwh_bank5,
	mwh_bank6, mwh_bank7, mwh_bank8, mwh_bank9, mwh_bank10,
	mwh_bank11,mwh_bank12,mwh_bank13,mwh_bank14,mwh_bank15,
	mwh_bank16,mwh_bank17,mwh_bank18,mwh_bank19,mwh_bank20
};
static mem_write16_handler bank_write16_handler[] =
{
	mwh_ram16,
	mwh_bank1_16, mwh_bank2_16, mwh_bank3_16, mwh_bank4_16, mwh_bank5_16,
	mwh_bank6_16, mwh_bank7_16, mwh_bank8_16, mwh_bank9_16, mwh_bank10_16,
	mwh_bank11_16,mwh_bank12_16,mwh_bank13_16,mwh_bank14_16,mwh_bank15_16,
	mwh_bank16_16,mwh_bank17_16,mwh_bank18_16,mwh_bank19_16,mwh_bank20_16
};
static mem_write32_handler bank_write32_handler[] =
{
	mwh_ram32,
	mwh_bank1_32, mwh_bank2_32, mwh_bank3_32, mwh_bank4_32, mwh_bank5_32,
	mwh_bank6_32, mwh_bank7_32, mwh_bank8_32, mwh_bank9_32, mwh_bank10_32,
	mwh_bank11_32,mwh_bank12_32,mwh_bank13_32,mwh_bank14_32,mwh_bank15_32,
	mwh_bank16_32,mwh_bank17_32,mwh_bank18_32,mwh_bank19_32,mwh_bank20_32
};

static WRITE_HANDLER(mwh_error)
{
	logerror("CPU #%d PC %04x: warning - write %02x to unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset);
	cpu_bankbase[0][offset] = data;
}

static WRITE16_HANDLER(mwh_error16)
{
	if (mem_mask == 0xff00)
		logerror("CPU #%d PC %04x: warning - write --%02x to unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset*2);
	else if (mem_mask == 0x00ff)
		logerror("CPU #%d PC %04x: warning - write %02x-- to unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data>>8,offset*2);
	else
		logerror("CPU #%d PC %04x: warning - write %04x to unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset*2);
	((data16_t *)cpu_bankbase[0])[offset] = data;
}

static WRITE32_HANDLER(mwh_error32)
{
	logerror("CPU #%d PC %04x: warning - write %08x(mask %08x) to unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,mem_mask,offset*4);
	((data32_t *)cpu_bankbase[0])[offset] = data;
}

static WRITE_HANDLER(mwh_error_sparse)
{
	logerror("CPU #%d PC %08x: warning - write %02x to unmapped memory address %08x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset);
}

static WRITE16_HANDLER(mwh_error_sparse16)
{
	if (mem_mask == 0xff00)
		logerror("CPU #%d PC %08x: warning - write --%02x to unmapped memory address %08x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset*2);
	else if (mem_mask == 0x00ff)
		logerror("CPU #%d PC %08x: warning - write %02x-- to unmapped memory address %08x\n",cpu_getactivecpu(),cpu_get_pc(),data>>8,offset*2);
	else
		logerror("CPU #%d PC %08x: warning - write %04x to unmapped memory address %08x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset*2);
}

static WRITE32_HANDLER(mwh_error_sparse32)
{
	logerror("CPU #%d PC %04x: warning - write %08x(mask %08x) to unmapped memory address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,mem_mask,offset*4);
}

static WRITE_HANDLER(mwh_error_sparse_bit)
{
	logerror("CPU #%d PC %08x: warning - write %02x to unmapped memory bit addr %08x\n",cpu_getactivecpu(),cpu_get_pc(),data,(offset<<3));
}

static WRITE16_HANDLER(mwh_error_sparse_bit16)
{
	logerror("CPU #%d PC %08x: warning - write %04x to unmapped memory bit addr %08x\n",cpu_getactivecpu(),cpu_get_pc(),data,(offset<<3)*2);
}

static WRITE32_HANDLER(mwh_error_sparse_bit32)
{
	logerror("CPU #%d PC %08x: warning - write %08x to unmapped memory bit addr %08x\n",cpu_getactivecpu(),cpu_get_pc(),data,(offset<<3)*4);
}

static WRITE_HANDLER(mwh_rom)
{
	logerror("CPU #%d PC %04x: warning - write %02x to ROM address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset);
}

static WRITE16_HANDLER(mwh_rom16)
{
	logerror("CPU #%d PC %04x: warning - write %04x to ROM address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset*2);
}

static WRITE32_HANDLER(mwh_rom32)
{
	logerror("CPU #%d PC %04x: warning - write %08x to ROM address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset*4);
}

static WRITE_HANDLER(mwh_ramrom)
{
	cpu_bankbase[0][offset] = cpu_bankbase[0][offset + (OP_ROM - OP_RAM)] = data;
}

static WRITE16_HANDLER(mwh_ramrom16)
{
	((data16_t *)cpu_bankbase[0])[offset] = ((data16_t *)cpu_bankbase[0])[offset + (OP_ROM - OP_RAM)/2] = data;
}

static WRITE32_HANDLER(mwh_ramrom32)
{
	((data32_t *)cpu_bankbase[0])[offset] = ((data32_t *)cpu_bankbase[0])[offset + (OP_ROM - OP_RAM)/4] = data;
}

static WRITE_HANDLER(mwh_nop) { }
static WRITE16_HANDLER(mwh_nop16) { }
static WRITE32_HANDLER(mwh_nop32) { }


/***************************************************************************

  Memory structure building

***************************************************************************/

/* return element offset */
static MHELE *get_element( MHELE *element , int ad , int elemask ,
						MHELE *subelement , int *ele_max )
{
	MHELE hw = element[ad];
	int i,ele;
	int banks = ( elemask / (1<<MH_SBITS) ) + 1;

	if( hw >= MH_HARDMAX ) return &subelement[(hw-MH_HARDMAX)<<MH_SBITS];

	/* create new element block */
	if( (*ele_max)+banks > MH_ELEMAX )
	{
		logerror("memory element size overflow\n");
		return 0;
	}
	/* get new element nunber */
	ele = *ele_max;
	(*ele_max)+=banks;
#ifdef MEM_DUMP
	logerror("create element %2d(%2d)\n",ele,banks);
#endif
	/* set link mark to current element */
	element[ad] = ele + MH_HARDMAX;
	/* get next subelement top */
	subelement	= &subelement[ele<<MH_SBITS];
	/* initialize new block */
	for( i = 0 ; i < (banks<<MH_SBITS) ; i++ )
		subelement[i] = hw;

	return subelement;
}

static void set_element( int cpu , MHELE *celement , int sp , int ep , MHELE type , MHELE *subelement , int *ele_max )
{
	int i;
	int edepth = 0;
	int shift,mask;
	MHELE *eele = celement;
	MHELE *sele = celement;
	MHELE *ele;
	int ss,sb,eb,ee;

#ifdef MEM_DUMP
	logerror("set_element %8X-%8X = %2X\n",sp,ep,type);
#endif
	if( (unsigned int) sp > (unsigned int) ep ) return;
	do{
		mask  = mhmask[cpu][edepth];
		shift = mhshift[cpu][edepth];

		/* center element */
		ss = (unsigned int) sp >> shift;
		sb = (unsigned int) sp ? ((unsigned int) (sp-1) >> shift) + 1 : 0;
		eb = ((unsigned int) (ep+1) >> shift) - 1;
		ee = (unsigned int) ep >> shift;

		if( sb <= eb )
		{
			if( (sb|mask)==(eb|mask) )
			{
				/* same reason */
				ele = (sele ? sele : eele);
				for( i = sb ; i <= eb ; i++ ){
					ele[i & mask] = type;
				}
			}
			else
			{
				if( sele ) for( i = sb ; i <= (sb|mask) ; i++ )
					sele[i & mask] = type;
				if( eele ) for( i = eb&(~mask) ; i <= eb ; i++ )
					eele[i & mask] = type;
			}
		}

		edepth++;

		if( ss == sb ) sele = 0;
		else sele = get_element( sele , ss & mask , mhmask[cpu][edepth] ,
									subelement , ele_max );
		if( ee == eb ) eele = 0;
		else eele = get_element( eele , ee & mask , mhmask[cpu][edepth] ,
									subelement , ele_max );

	}while( sele || eele );
}

static int need_ram(int cpu, const struct Memory_ReadAddress *mra, const struct Memory_WriteAddress *mwa)
{
	if (mra)
	{
		switch( (FPTR)mra->handler )
		{
		case (FPTR)MRA_RAM:
		case (FPTR)MRA_ROM:
		case (FPTR)MRA_BANK1:
		case (FPTR)MRA_BANK2:
		case (FPTR)MRA_BANK3:
		case (FPTR)MRA_BANK4:
		case (FPTR)MRA_BANK5:
		case (FPTR)MRA_BANK6:
		case (FPTR)MRA_BANK7:
		case (FPTR)MRA_BANK8:
		case (FPTR)MRA_BANK9:
		case (FPTR)MRA_BANK10:
		case (FPTR)MRA_BANK11:
		case (FPTR)MRA_BANK12:
		case (FPTR)MRA_BANK13:
		case (FPTR)MRA_BANK14:
		case (FPTR)MRA_BANK15:
		case (FPTR)MRA_BANK16:
		case (FPTR)MRA_BANK17:
		case (FPTR)MRA_BANK18:
		case (FPTR)MRA_BANK19:
		case (FPTR)MRA_BANK20:
			return 1;
		case (FPTR)MRA_NOP:
			return 0;
		default:
			/* always allocate ram for CPUs with less than 21 address bits */
			if (ADDRESS_BITS (cpu) < 21)
				return 1;
			break;
		}
	}
	else
	if (mwa)
	{
		/* need RAM if it initializes a base pointer */
        if (mwa->base != NULL)
			return 1;
		switch( (FPTR)mwa->handler )
		{
		case (FPTR)MWA_RAM:
		case (FPTR)MWA_ROM:
		case (FPTR)MWA_RAMROM:
		case (FPTR)MWA_BANK1:
		case (FPTR)MWA_BANK2:
		case (FPTR)MWA_BANK3:
		case (FPTR)MWA_BANK4:
		case (FPTR)MWA_BANK5:
		case (FPTR)MWA_BANK6:
		case (FPTR)MWA_BANK7:
		case (FPTR)MWA_BANK8:
		case (FPTR)MWA_BANK9:
		case (FPTR)MWA_BANK10:
		case (FPTR)MWA_BANK11:
		case (FPTR)MWA_BANK12:
		case (FPTR)MWA_BANK13:
		case (FPTR)MWA_BANK14:
		case (FPTR)MWA_BANK15:
		case (FPTR)MWA_BANK16:
		case (FPTR)MWA_BANK17:
		case (FPTR)MWA_BANK18:
		case (FPTR)MWA_BANK19:
		case (FPTR)MWA_BANK20:
			return 1;
		case (FPTR)MWA_NOP:
			return 0;
		default:
			/* always allocate ram for CPUs with less than 21 address bits */
            if (ADDRESS_BITS (cpu) < 21)
				return 1;
			break;
		}
	}
	return 0;	/* does not need RAM */
}


/* ASG 000920 begin */
int handler_to_bank(void *handler)
{
	if ((FPTR)handler <= (FPTR)MRA_BANK1 && (FPTR)handler > (FPTR)MRA_BANK1 - MAX_BANKS)
		return (FPTR)MRA_BANK1 - (FPTR)handler + 1;
	else
		return 0;
}

void *bank_to_handler(int bank)
{
	return (void *)((FPTR)MRA_BANK1 - (bank - 1));
}
/* ASG 000920 end */


/* ASG 980121 -- allocate all the external memory */
static int memory_allocate_ext (void)
{
	struct ExtMemory *ext = ext_memory;
	int cpu;

	/* a change for MESS */
	if (Machine->gamedrv->rom == 0)  return 1;

	/* loop over all CPUs */
	for (cpu = 0; cpu < cpu_gettotalcpu (); cpu++)
	{
		const struct Memory_ReadAddress *mra;
		const struct Memory_WriteAddress *mwa;

		int region = REGION_CPU1+cpu;
		int size = memory_region_length(region);

		/* now it's time to loop */
		while (1)
		{
			offs_t lowest = 0xffffffff, end, lastend;

			mra = Machine->drv->cpu[cpu].memory_read;
			mwa = Machine->drv->cpu[cpu].memory_write;

			/* find the base of the lowest memory region that extends past the end */
			while (!IS_MEMORY_END(mra))
			{
				if (!IS_MEMORY_MARKER(mra))
				{
					if (mra->end >= size && mra->start < lowest && need_ram(cpu, mra, NULL))
						lowest = mra->start;
				}
				mra++;
			}
			while (!IS_MEMORY_END(mwa))
			{
				if (!IS_MEMORY_MARKER(mwa))
				{
					if (mwa->end >= size && mwa->start < lowest && need_ram(cpu, NULL, mwa))
						lowest = mwa->start;
				}
				mwa++;
			}

			/* done if nothing found */
			if (lowest == 0xffffffff)
				break;

			/* now loop until we find the end of this contiguous block of memory */
			lastend = -1;
			end = lowest;
			while (end != lastend)
			{
				lastend = end;

				mra = Machine->drv->cpu[cpu].memory_read;
				mwa = Machine->drv->cpu[cpu].memory_write;

				/* find the end of the contiguous block of memory */
				while (!IS_MEMORY_END(mra))
				{
					if (!IS_MEMORY_MARKER(mra))
					{
						if (mra->start <= end+1 && mra->end > end && need_ram(cpu, mra, NULL))
							end = mra->end;
					}
					mra++;
				}
				while (!IS_MEMORY_END(mwa))
				{
					if (!IS_MEMORY_MARKER(mwa))
					{
						if (mwa->start <= end+1 && mwa->end > end && need_ram(cpu, NULL, mwa))
							end = mwa->end;
					}
					mwa++;
				}
			}

			/* time to allocate */
			ext->start = lowest;
			ext->end = end;
			ext->region = region;
			ext->data = malloc (end+1 - lowest);

			/* if that fails, we're through */
			if (!ext->data)
			{
				logerror("malloc(%d) failed (lowest: %x - end: %x)\n", end + 1 - lowest, lowest, end);
				return 0;
			}

			/* reset the memory */
			memset (ext->data, 0, end+1 - lowest);
			size = ext->end + 1;
			ext++;
		}
	}

	return 1;
}


void *findmemorychunk(int cpu, int offset, int *chunkstart, int *chunkend)
{
	int region = REGION_CPU1+cpu;
	struct ExtMemory *ext;

	/* look in external memory first */
	for (ext = ext_memory; ext->data; ext++)
		if (ext->region == region && ext->start <= offset && ext->end >= offset)
		{
			*chunkstart = ext->start;
			*chunkend = ext->end;
			return ext->data;
		}

	/* return RAM */
	*chunkstart = 0;
	*chunkend = memory_region_length(region) - 1;
	return ramptr[cpu];
}


void *memory_find_base (int cpu, offs_t offset)
{
	int region = REGION_CPU1+cpu;
	struct ExtMemory *ext;

	/* look in external memory first */
	for (ext = ext_memory; ext->data; ext++)
		if (ext->region == region && ext->start <= offset && ext->end >= offset)
			return (void *)((UINT8 *)ext->data + (offset - ext->start));

	return ramptr[cpu] + offset;
}

/* make these static so they can be used in a callback by game drivers */

static int rdelement_max = 0;
static int wrelement_max = 0;
static int rdhard_max = HT_USER;
static int wrhard_max = HT_USER;

/* return = FALSE:can't allocate element memory */
int memory_init(void)
{
	int i, cpu;
	const struct Memory_ReadAddress *memoryread;
	const struct Memory_WriteAddress *memorywrite;
	const struct IO_ReadPort *ioread;
	const struct IO_WritePort *iowrite;
	int abits1,abits2,abits3,abitsmin;
	UINT8 bank_used[MAX_BANKS+1];	/* ASG 000920 */
	UINT8 bank_cpu[MAX_BANKS+1];	/* ASG 000920 */
	offs_t bank_base[MAX_BANKS+1];	/* ASG 000920 */
	int bank;						/* ASG 000920 */

	rdelement_max = 0;
	wrelement_max = 0;
	rdhard_max = HT_USER;
	wrhard_max = HT_USER;

	for( cpu = 0 ; cpu < MAX_CPU ; cpu++ )
		cur_mr_element[cpu] = cur_mw_element[cpu] = 0;

	ophw = 0xff;

	memset(bank_used, 0, sizeof(bank_used));	/* ASG 000920 */

	/* ASG 980121 -- allocate external memory */
	if (!memory_allocate_ext ())
	{
		logerror("memory_allocate_ext failed!\n");
		return 0;
	}

	for( cpu = 0 ; cpu < cpu_gettotalcpu() ; cpu++ )
	{
		const struct Memory_ReadAddress *_mra;
		const struct Memory_WriteAddress *_mwa;

		setOPbasefunc[cpu] = NULL;

		ramptr[cpu] = romptr[cpu] = memory_region(REGION_CPU1+cpu);

		/* initialize the memory base pointers for memory hooks */
		_mra = Machine->drv->cpu[cpu].memory_read;
		if (_mra)
		{
			/* MEMORY_READ_START header? */
			if (_mra->start == MEMORY_MARKER && _mra->end != 0)
			{
				if ((_mra->end & MEMORY_DIRECTION_MASK) != MEMORY_DIRECTION_READ)
				{
					printf("MEMORY_READ_START for cpu #%d is wrong!\n", cpu);
					return 0;	/* fail */
				}

				switch (cpunum_databus_width(cpu))
				{
					case 8:
						if ((_mra->end & MEMORY_WIDTH_MASK) != MEMORY_WIDTH_8)
						{
							printf("cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n", cpu,cpunum_databus_width(cpu),_mra->end);
							return 0;	/* fail */
						}
						break;
					case 16:
						if ((_mra->end & MEMORY_WIDTH_MASK) != MEMORY_WIDTH_16)
						{
							printf("cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n", cpu,cpunum_databus_width(cpu),_mra->end);
							return 0;	/* fail */
						}
						break;
					case 32:
						if ((_mra->end & MEMORY_WIDTH_MASK) != MEMORY_WIDTH_32)
						{
							printf("cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n", cpu,cpunum_databus_width(cpu),_mra->end);
							return 0;	/* fail */
						}
						break;
				}
				_mra++;
			}

			/* ASG 000920 begin */
			while (!IS_MEMORY_END(_mra))
			{
				if (!IS_MEMORY_MARKER(_mra))
				{
					bank = handler_to_bank((void *)_mra->handler);
					if (bank)
					{
						bank_used[bank] = 1;
						bank_cpu[bank] = -1;
					}
				}
				_mra++;
			}
			/* ASG 000920 end */
		}
		_mwa = Machine->drv->cpu[cpu].memory_write;
		if (_mwa)
		{
			/* MEMORY_WRITE_START header? */
			if (_mwa->start == MEMORY_MARKER && _mwa->end != 0)
			{
				if ((_mwa->end & MEMORY_DIRECTION_MASK) != MEMORY_DIRECTION_WRITE)
				{
					printf("MEMORY_WRITE_START for cpu #%d is wrong!\n", cpu);
					return 0;	/* fail */
				}

				switch (cpunum_databus_width(cpu))
				{
					case 8:
						if ((_mwa->end & MEMORY_WIDTH_MASK) != MEMORY_WIDTH_8)
						{
							printf("cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n", cpu,cpunum_databus_width(cpu),_mwa->end);
							return 0;	/* fail */
						}
						break;
					case 16:
						if ((_mwa->end & MEMORY_WIDTH_MASK) != MEMORY_WIDTH_16)
						{
							printf("cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n", cpu,cpunum_databus_width(cpu),_mwa->end);
							return 0;	/* fail */
						}
						break;
					case 32:
						if ((_mwa->end & MEMORY_WIDTH_MASK) != MEMORY_WIDTH_32)
						{
							printf("cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n", cpu,cpunum_databus_width(cpu),_mwa->end);
							return 0;	/* fail */
						}
						break;
				}
				_mwa++;
			}

			while (!IS_MEMORY_END(_mwa))
			{
				if (!IS_MEMORY_MARKER(_mwa))
				{
					if (_mwa->base) *_mwa->base = memory_find_base (cpu, _mwa->start);
					if (_mwa->size) *_mwa->size = _mwa->end - _mwa->start + 1;

					/* ASG 000920 begin */
					bank = handler_to_bank((void *)_mwa->handler);
					if (bank)
					{
						bank_used[bank] = 1;
						bank_cpu[bank] = -1;
					}
					/* ASG 000920 end */
				}
				_mwa++;
			}
		}

		/* initialize port structures */
		readport_size[cpu] = 0;
		writeport_size[cpu] = 0;
		readport[cpu] = 0;
		writeport[cpu] = 0;

		/* install port handlers - at least an empty one */
		ioread = Machine->drv->cpu[cpu].port_read;
		if (ioread)
		{
			while (!IS_MEMORY_END(ioread))
			{
				if (!IS_MEMORY_MARKER(ioread))
				{
					if (install_port_read_handler_common(cpu, ioread->start, ioread->end, ioread->handler, 0) == 0)
					{
						memory_shutdown();
						logerror("install_port_read_handler_common for cpu #%d failed!\n", cpu);
						return 0;
					}
				}

				ioread++;
			}
		}

		iowrite = Machine->drv->cpu[cpu].port_write;
		if (iowrite)
		{
			while (!IS_MEMORY_END(iowrite))
			{
				if (!IS_MEMORY_MARKER(iowrite))
				{
					if (install_port_write_handler_common(cpu, iowrite->start, iowrite->end, iowrite->handler, 0) == 0)
					{
						memory_shutdown();
						logerror("install_port_write_handler_common for cpu #%d failed!\n", cpu);
						return 0;
					}
				}

				iowrite++;
			}
		}

		portmask[cpu] = 0xffff;
#if (HAS_Z80)
		if ((Machine->drv->cpu[cpu].cpu_type & ~CPU_FLAGS_MASK) == CPU_Z80 &&
			(Machine->drv->cpu[cpu].cpu_type & CPU_16BIT_PORT) == 0)
			portmask[cpu] = 0xff;
#endif
	}

	/* initialize global handler */
	for (i = 0 ; i < MH_HARDMAX ; i++)
	{
		memoryreadoffset[i] = 0;
		memorywriteoffset[i] = 0;
		memoryreadhandler[i] = NULL;
		memorywritehandler[i] = NULL;
		memoryread16handler[i] = NULL;
		memorywrite16handler[i] = NULL;
		memoryread32handler[i] = NULL;
		memorywrite32handler[i] = NULL;
	}
	/* bank memory */
	for (i = 1; i <= MAX_BANKS; i++)
	{
		memoryreadhandler[i] = bank_read_handler[i];
		memorywritehandler[i] = bank_write_handler[i];
		memoryread16handler[i] = bank_read16_handler[i];
		memorywrite16handler[i] = bank_write16_handler[i];
		memoryread32handler[i] = bank_read32_handler[i];
		memorywrite32handler[i] = bank_write32_handler[i];
	}
	/* non map memory */
	memoryreadhandler[HT_NON] = mrh_error;
	memorywritehandler[HT_NON] = mwh_error;
	memoryread16handler[HT_NON] = mrh_error16;
	memorywrite16handler[HT_NON] = mwh_error16;
	memoryread32handler[HT_NON] = mrh_error32;
	memorywrite32handler[HT_NON] = mwh_error32;
	/* NOP memory */
	memoryreadhandler[HT_NOP] = mrh_nop;
	memorywritehandler[HT_NOP] = mwh_nop;
	memoryread16handler[HT_NOP] = mrh_nop16;
	memorywrite16handler[HT_NOP] = mwh_nop16;
	memoryread32handler[HT_NOP] = mrh_nop32;
	memorywrite32handler[HT_NOP] = mwh_nop32;
	/* RAMROM memory */
	memorywritehandler[HT_RAMROM] = mwh_ramrom;
	memorywrite16handler[HT_RAMROM] = mwh_ramrom16;
	memorywrite32handler[HT_RAMROM] = mwh_ramrom32;
	/* ROM memory */
	memorywritehandler[HT_ROM] = mwh_rom;
	memorywrite16handler[HT_ROM] = mwh_rom16;
	memorywrite32handler[HT_ROM] = mwh_rom32;

	/* if any CPU is 21-bit or more, we change the error handlers to be more benign */
	for (cpu = 0; cpu < cpu_gettotalcpu(); cpu++)
	{
		if (ADDRESS_BITS (cpu) >= 21)
		{
			memoryreadhandler[HT_NON] = mrh_error_sparse;
			memorywritehandler[HT_NON] = mwh_error_sparse;
			memoryread16handler[HT_NON] = mrh_error_sparse16;
			memorywrite16handler[HT_NON] = mwh_error_sparse16;
			memoryread32handler[HT_NON] = mrh_error_sparse32;
			memorywrite32handler[HT_NON] = mwh_error_sparse32;
#if (HAS_TMS34010)
			if ((Machine->drv->cpu[cpu].cpu_type & ~CPU_FLAGS_MASK)==CPU_TMS34010)
			{
				memoryreadhandler[HT_NON] = mrh_error_sparse_bit;
				memorywritehandler[HT_NON] = mwh_error_sparse_bit;
				memoryread16handler[HT_NON] = mrh_error_sparse_bit16;
				memorywrite16handler[HT_NON] = mwh_error_sparse_bit16;
				memoryread32handler[HT_NON] = mrh_error_sparse_bit32;
				memorywrite32handler[HT_NON] = mwh_error_sparse_bit32;
			}
#endif
		}
	}

	for( cpu = 0 ; cpu < cpu_gettotalcpu() ; cpu++ )
	{
		/* cpu selection */
		abits1 = ABITS1 (cpu);
		abits2 = ABITS2 (cpu);
		abits3 = ABITS3 (cpu);
		abitsmin = ABITSMIN (cpu);

		/* element shifter , mask set */
		mhshift[cpu][0] = (abits2+abits3);
		mhshift[cpu][1] = abits3;			/* 2nd */
		mhshift[cpu][2] = 0;				/* 3rd (used by set_element)*/
		mhmask[cpu][0]	= MHMASK(abits1);		/*1st(used by set_element)*/
		mhmask[cpu][1]	= MHMASK(abits2);		/*2nd*/
		mhmask[cpu][2]	= MHMASK(abits3);		/*3rd*/

		/* allocate current element */
		if( (cur_mr_element[cpu] = (MHELE *)malloc(sizeof(MHELE)<<abits1)) == 0 )
		{
			memory_shutdown();
			logerror("cur_mr_element malloc for cpu #%d failed!\n", cpu);
			return 0;
		}
		if( (cur_mw_element[cpu] = (MHELE *)malloc(sizeof(MHELE)<<abits1)) == 0 )
		{
			memory_shutdown();
			logerror("cur_mw_element malloc for cpu #%d failed!\n", cpu);
			return 0;
		}

		/* initialize current element table */
		for( i = 0 ; i < (1<<abits1) ; i++ )
		{
			cur_mr_element[cpu][i] = HT_NON;	/* no map memory */
			cur_mw_element[cpu][i] = HT_NON;	/* no map memory */
		}

		memoryread = Machine->drv->cpu[cpu].memory_read;
		memorywrite = Machine->drv->cpu[cpu].memory_write;

		/* memory read handler build */
		if (memoryread)
		{
			const struct Memory_ReadAddress *mra;

			mra = memoryread;
			while (!IS_MEMORY_END(mra)) mra++;
			mra--;

			while (mra >= memoryread)
			{
				if (!IS_MEMORY_MARKER(mra))
				{
					/* ASG 000920 begin */
					mem_read_handler handler = mra->handler;
					if (ADDRESS_BITS (cpu) >= 21 && mra->handler == MRA_RAM)
					{
						for (bank = 1; bank <= MAX_BANKS; bank++)
							if (!bank_used[bank] || (bank_used[bank] && bank_cpu[bank] == cpu && bank_base[bank] == mra->start))
							{
								bank_used[bank] = 1;
								bank_cpu[bank] = cpu;
								bank_base[bank] = mra->start;
								handler = (mem_read_handler)bank_to_handler(bank);
								break;
							}
						if (bank > MAX_BANKS)
						{
							logerror("Ran out of banks for sparse memory regions!\n");
							exit(1);
						}
					}
					internal_install_mem_read_handler(cpu,mra->start,mra->end,handler);
					/* ASG 000920 end */
				}
				mra--;
			}
		}

		/* memory write handler build */
		if (memorywrite)
		{
			const struct Memory_WriteAddress *mwa;

			mwa = memorywrite;
			while (!IS_MEMORY_END(mwa)) mwa++;
			mwa--;

			while (mwa >= memorywrite)
			{
				if (!IS_MEMORY_MARKER(mwa))
				{
					/* ASG 000920 begin */
					mem_write_handler handler = mwa->handler;
					if (ADDRESS_BITS (cpu) >= 21 && mwa->handler == MWA_RAM)
					{
						for (bank = 1; bank <= MAX_BANKS; bank++)
							if (!bank_used[bank] || (bank_used[bank] && bank_cpu[bank] == cpu && bank_base[bank] == mwa->start))
							{
								bank_used[bank] = 1;
								bank_cpu[bank] = cpu;
								bank_base[bank] = mwa->start;
								handler = (mem_write_handler)bank_to_handler(bank);
								break;
							}
						if (bank > MAX_BANKS)
						{
							logerror("Ran out of banks for sparse memory regions!\n");
							exit(1);
						}
					}
					internal_install_mem_write_handler (cpu, mwa->start, mwa->end, handler);
					/* ASG 000920 end */
				}
				mwa--;
			}
		}
	}

	logerror("used read  elements %d/%d , functions %d/%d\n"
			,rdelement_max,MH_ELEMAX , rdhard_max,MH_HARDMAX );
	logerror("used write elements %d/%d , functions %d/%d\n"
			,wrelement_max,MH_ELEMAX , wrhard_max,MH_HARDMAX );

#ifdef MEM_DUMP
	mem_dump();
#endif
	return 1;	/* ok */
}

void memory_set_opcode_base(int cpu, void *base)
{
	romptr[cpu] = base;
}


void memorycontextswap(int activecpu)
{
	cpu_bankbase[0] = ramptr[activecpu];

	cur_mrhard = cur_mr_element[activecpu];
	cur_mwhard = cur_mw_element[activecpu];

	/* ASG: port speedup */
	cur_readport = readport[activecpu];
	cur_writeport = writeport[activecpu];
	cur_portmask = portmask[activecpu];

	OPbasefunc = setOPbasefunc[activecpu];

	/* op code memory pointer */
	ophw = HT_RAM;
	OP_RAM = cpu_bankbase[0];
	OP_ROM = romptr[activecpu];
}

void memory_shutdown(void)
{
	struct ExtMemory *ext;
	int cpu;

	for( cpu = 0 ; cpu < MAX_CPU ; cpu++ )
	{
		if( cur_mr_element[cpu] != 0 )
		{
			free( cur_mr_element[cpu] );
			cur_mr_element[cpu] = 0;
		}
		if( cur_mw_element[cpu] != 0 )
		{
			free( cur_mw_element[cpu] );
			cur_mw_element[cpu] = 0;
		}

		if (readport[cpu] != 0)
		{
			free(readport[cpu]);
			readport[cpu] = 0;
		}

		if (writeport[cpu] != 0)
		{
			free(writeport[cpu]);
			writeport[cpu] = 0;
		}
	}

	/* ASG 980121 -- free all the external memory */
	for (ext = ext_memory; ext->data; ext++)
		free (ext->data);
	memset (ext_memory, 0, sizeof (ext_memory));
}



/***************************************************************************

  Perform a memory read. This function is called by the CPU emulation.

***************************************************************************/

/* use these constants to define which type of memory handler to build */
#define TYPE_8BIT					0		/* 8-bit aligned */
#define TYPE_16BIT_BE				1		/* 16-bit aligned, big-endian */
#define TYPE_16BIT_LE				2		/* 16-bit aligned, little-endian */
#define TYPE_32BIT_BE				3		/* 32-bit aligned, big-endian */
#define TYPE_32BIT_LE				4		/* 32-bit aligned, little-endian */

#define CAN_BE_MISALIGNED			0		/* word/dwords can be read on non-16-bit boundaries */
#define ALWAYS_ALIGNED				1		/* word/dwords are always read on 16-bit boundaries */

/* stupid workarounds so that we can generate an address mask that works even for 32 bits */
#define ADDRESS_TOPBIT(abits)		(1UL << (ABITS1_##abits + ABITS2_##abits + ABITS_MIN_##abits - 1))
#define ADDRESS_MASK(abits) 		(ADDRESS_TOPBIT(abits) | (ADDRESS_TOPBIT(abits) - 1))


#define MEMREADSTART		profiler_mark(PROFILER_MEMREAD);
#define MEMREADEND(ret)		{ profiler_mark(PROFILER_END); return ret; }
#define MEMWRITESTART		profiler_mark(PROFILER_MEMWRITE);
#define MEMWRITEEND			{ profiler_mark(PROFILER_END); return; }


/* generic byte-sized read handler */
#define READBYTE(dtype,name,type,abits) 												\
dtype name(offs_t address)																\
{																						\
	MHELE hw;																			\
																						\
	MEMREADSTART																		\
																						\
	/* first-level lookup */															\
	hw = cur_mrhard[address >> (ABITS2_##abits + ABITS_MIN_##abits)];					\
																						\
	/* for compatibility with setbankhandler, 8-bit systems must call handlers */		\
	/* for banked memory reads/writes */												\
	if (type == TYPE_8BIT && hw == HT_RAM)												\
		MEMREADEND(cpu_bankbase[HT_RAM][address])										\
	else if (type != TYPE_8BIT && hw <= HT_BANKMAX) 									\
	{																					\
		if (type == TYPE_16BIT_BE)														\
			MEMREADEND(cpu_bankbase[hw][BYTE_XOR_BE(address) - memoryreadoffset[hw]])	\
		else if (type == TYPE_16BIT_LE) 												\
			MEMREADEND(cpu_bankbase[hw][BYTE_XOR_LE(address) - memoryreadoffset[hw]])	\
		else if (type == TYPE_32BIT_BE) 												\
			MEMREADEND(cpu_bankbase[hw][BYTE4_XOR_BE(address) - memoryreadoffset[hw]])	\
		else if (type == TYPE_32BIT_LE) 												\
			MEMREADEND(cpu_bankbase[hw][BYTE4_XOR_LE(address) - memoryreadoffset[hw]])	\
	}																					\
																						\
	/* second-level lookup */															\
	if (hw >= MH_HARDMAX)																\
	{																					\
		hw -= MH_HARDMAX;																\
		hw = readhardware[(hw << MH_SBITS) + (((UINT32)address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))];	\
																						\
		/* for compatibility with setbankhandler, 8-bit systems must call handlers */	\
		/* for banked memory reads/writes */											\
		if (type == TYPE_8BIT && hw == HT_RAM)											\
			MEMREADEND(cpu_bankbase[HT_RAM][address])									\
		else if (type != TYPE_8BIT && hw <= HT_BANKMAX) 								\
		{																				\
			if (type == TYPE_16BIT_BE)													\
				MEMREADEND(cpu_bankbase[hw][BYTE_XOR_BE(address) - memoryreadoffset[hw]])\
			else if (type == TYPE_16BIT_LE) 											\
				MEMREADEND(cpu_bankbase[hw][BYTE_XOR_LE(address) - memoryreadoffset[hw]])\
			else if (type == TYPE_32BIT_BE) 											\
				MEMREADEND(cpu_bankbase[hw][BYTE4_XOR_BE(address) - memoryreadoffset[hw]])	\
			else if (type == TYPE_32BIT_LE) 											\
				MEMREADEND(cpu_bankbase[hw][BYTE4_XOR_LE(address) - memoryreadoffset[hw]])	\
		}																				\
	}																					\
																						\
	/* fall back to handler */															\
	if (type == TYPE_8BIT)																\
		MEMREADEND((*memoryreadhandler[hw])(address - memoryreadoffset[hw]))				\
	else																				\
	if (type == TYPE_32BIT_BE || type == TYPE_32BIT_LE) 								\
	{																					\
		data32_t data;																	\
		int shift = (/*mem_offs = */(address & 3)) << 3;												 \
		if (type == TYPE_32BIT_BE)														\
			shift ^= 24;																\
        /*mem_mask = 0xff000000UL >> shift;*/                                               \
        data = (*memoryread32handler[hw])((address - memoryreadoffset[hw]) >> 2);       \
		MEMREADEND((data >> shift) & 0xff)												\
	}																					\
    else                                                                                \
    if (type == TYPE_16BIT_BE || type == TYPE_16BIT_LE)                                 \
	{																					\
		data16_t data;																	\
		int shift = (/*mem_offs = */(address & 1)) << 3;												 \
        if (type == TYPE_16BIT_BE)                                                      \
			shift ^= 8; 																\
        /*mem_mask = 0xff00 >> shift;*/                                                     \
		data = (*memoryread16handler[hw])((address - memoryreadoffset[hw]) >> 1); 		\
		MEMREADEND((data >> shift) & 0xff)												\
	}																					\
}

/* generic word-sized read handler (16-bit and 32-bit aligned only!) */
#define READWORD(dtype,name,type,abits,align)											\
dtype name##_word(offs_t address)														\
{																						\
	MHELE hw;																			\
																						\
	MEMREADSTART																		\
																						\
	/* only supports 16-bit and 32-bit memory systems */								\
	if (type == TYPE_8BIT)																\
		printf("Unsupported type for READWORD macro!\n");                               \
																						\
	/* handle aligned case first */ 													\
	if ((type == TYPE_32BIT_BE || type == TYPE_32BIT_LE) && 							\
		(align == ALWAYS_ALIGNED || !(address & 1)))									\
	{																					\
		/* first-level lookup */														\
		hw = cur_mrhard[address >> (ABITS2_##abits + ABITS_MIN_##abits)];				\
		if (hw <= HT_BANKMAX)															\
		{																				\
			if (type == TYPE_32BIT_BE)													\
				MEMREADEND(READ_WORD(&cpu_bankbase[hw][WORD_XOR_BE(address) - memoryreadoffset[hw]])) \
			else if (type == TYPE_32BIT_LE) 											\
                MEMREADEND(READ_WORD(&cpu_bankbase[hw][WORD_XOR_LE(address) - memoryreadoffset[hw]])) \
		}																				\
																						\
		/* second-level lookup */														\
		if (hw >= MH_HARDMAX)															\
		{																				\
			hw -= MH_HARDMAX;															\
			hw = readhardware[(hw << MH_SBITS) + ((address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))];	\
			if (hw <= HT_BANKMAX)														\
			{																			\
				if (type == TYPE_32BIT_BE)												\
					MEMREADEND(READ_WORD(&cpu_bankbase[hw][WORD_XOR_LE(address) - memoryreadoffset[hw]])) \
				else if (type == TYPE_32BIT_LE) 										\
					MEMREADEND(READ_WORD(&cpu_bankbase[hw][WORD_XOR_BE(address) - memoryreadoffset[hw]])) \
			}																			\
		}																				\
																						\
        /* fall back to handler */                                                      \
		{																				\
			int shift = (address & 2) << 3; 											\
			data32_t data;																\
			/*mem_mask = 0xffff0000 >> shift;*/ 											\
            if (type == TYPE_32BIT_BE)                                                  \
				shift ^= 16;															\
			data = (*memoryread32handler[hw])((address - memoryreadoffset[hw]) >> 2); 	\
			MEMREADEND((data >> shift) & 0xffff)											\
        }                                                                               \
	}																					\
	else																				\
	if ((type == TYPE_16BIT_BE || type == TYPE_16BIT_LE) && 							\
		(align == ALWAYS_ALIGNED || !(address & 1)))									\
	{																					\
		/* first-level lookup */														\
		hw = cur_mrhard[address >> (ABITS2_##abits + ABITS_MIN_##abits)];				\
		if (hw <= HT_BANKMAX)															\
			MEMREADEND(READ_WORD(&cpu_bankbase[hw][address - memoryreadoffset[hw]]))		\
																						\
		/* second-level lookup */														\
		if (hw >= MH_HARDMAX)															\
		{																				\
			hw -= MH_HARDMAX;															\
			hw = readhardware[(hw << MH_SBITS) + ((address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))];	\
			if (hw <= HT_BANKMAX)														\
				MEMREADEND(READ_WORD(&cpu_bankbase[hw][address - memoryreadoffset[hw]]))	\
		}																				\
																						\
		/* fall back to handler */														\
		MEMREADEND((*memoryread16handler[hw])((address - memoryreadoffset[hw]) >> 1))		\
	}																					\
																						\
	/* unaligned case */																\
	else if (type == TYPE_32BIT_BE || type == TYPE_16BIT_BE)							\
	{																					\
		data16_t data = name(address) << 8; 											\
		MEMREADEND(data | (name((address + 1) & ADDRESS_MASK(abits)) & 0xff))			\
	}																					\
	else if (type == TYPE_32BIT_LE || type == TYPE_16BIT_LE)							\
	{																					\
		data16_t data = name(address) & 0xff;											\
		MEMREADEND(data | (name((address + 1) & ADDRESS_MASK(abits)) << 8)) 				\
	}																					\
}

/* generic dword-sized read handler (32-bit aligned only!) */
#define READLONG(dtype,name,type,abits,align)											\
dtype name##_dword(offs_t address)														\
{																						\
	MHELE hw1;		 																	\
																						\
	MEMREADSTART																		\
																						\
	/* only supports 32-bit memory systems */											\
	if (type == TYPE_8BIT || type == TYPE_16BIT_BE || type == TYPE_16BIT_LE)			\
		printf("Unsupported type for READLONG macro!\n");                               \
																						\
	/* handle aligned case first */ 													\
	if (align == ALWAYS_ALIGNED || !(address & 3))										\
	{																					\
		/* first-level lookup */														\
		hw1 = cur_mrhard[address >> (ABITS2_##abits + ABITS_MIN_##abits)];				\
																						\
		/* second-level lookup */														\
		if (hw1 >= MH_HARDMAX)															\
		{																				\
			hw1 -= MH_HARDMAX;															\
			hw1 = readhardware[(hw1 << MH_SBITS) + ((address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))];	\
		}																				\
		if (hw1 <= HT_BANKMAX)															\
			MEMREADEND(READ_DWORD(&cpu_bankbase[hw1][address - memoryreadoffset[hw1]]))	\
		/*mem_mask = 0xffffffff;*/															\
		MEMREADEND((*memoryread32handler[hw1])((address - memoryreadoffset[hw1]) >> 2))	\
	}																					\
	/* unaligned case */																\
	else if (type == TYPE_32BIT_BE && !(address & 1))									\
	{																					\
		data32_t data = name##_word(address) << 16; 									\
		MEMREADEND(data | (name##_word((address + 2) & ADDRESS_MASK(abits)) & 0xffff))	\
	}																					\
	else if (type == TYPE_32BIT_LE && !(address & 1))									\
	{																					\
		data32_t data = name##_word(address) & 0xffff;									\
		MEMREADEND(data | (name##_word((address + 2) & ADDRESS_MASK(abits)) << 16)) 		\
	}																					\
	else if (type == TYPE_32BIT_BE)														\
	{																					\
		data32_t data = name(address) << 24;											\
		data |= name##_word((address + 1) & ADDRESS_MASK(abits)) << 8;					\
		MEMREADEND(data | (name((address + 3) & ADDRESS_MASK(abits)) & 0xff))			\
	}																					\
	else if (type == TYPE_32BIT_LE)														\
	{																					\
		data32_t data = name(address) & 0xff;											\
		data |= name##_word((address + 1) & ADDRESS_MASK(abits)) << 8;					\
		MEMREADEND(data | (name((address + 3) & ADDRESS_MASK(abits)) << 24))				\
	}																					\
}


/* the handlers we need to generate */
READBYTE(data8_t, cpu_readmem16,	TYPE_8BIT,	   16)
READBYTE(data8_t, cpu_readmem20,	TYPE_8BIT,	   20)
READBYTE(data8_t, cpu_readmem21,	TYPE_8BIT,	   21)
READBYTE(data8_t, cpu_readmem24,	TYPE_8BIT,	   24)

READBYTE(data16_t,cpu_readmem16bew, TYPE_16BIT_BE, 16W)
READWORD(data16_t,cpu_readmem16bew, TYPE_16BIT_BE, 16W, ALWAYS_ALIGNED)

READBYTE(data16_t,cpu_readmem16lew, TYPE_16BIT_LE, 16W)
READWORD(data16_t,cpu_readmem16lew, TYPE_16BIT_LE, 16W, ALWAYS_ALIGNED)

READBYTE(data16_t,cpu_readmem24bew, TYPE_16BIT_BE, 24W)
READWORD(data16_t,cpu_readmem24bew, TYPE_16BIT_BE, 24W, ALWAYS_ALIGNED)

READBYTE(data16_t,cpu_readmem29lew,	TYPE_16BIT_LE, 29W)
READWORD(data16_t,cpu_readmem29lew,	TYPE_16BIT_LE, 29W, CAN_BE_MISALIGNED)

READBYTE(data16_t,cpu_readmem32bew,	TYPE_16BIT_BE, 32W)
READWORD(data16_t,cpu_readmem32bew,	TYPE_16BIT_BE, 32W, ALWAYS_ALIGNED)

READBYTE(data16_t,cpu_readmem32lew, TYPE_16BIT_LE, 32W)
READWORD(data16_t,cpu_readmem32lew, TYPE_16BIT_LE, 32W, ALWAYS_ALIGNED)

READBYTE(data32_t,cpu_readmem24bedw,TYPE_32BIT_BE, 24DW)
READWORD(data32_t,cpu_readmem24bedw,TYPE_32BIT_BE, 24DW, ALWAYS_ALIGNED)
READLONG(data32_t,cpu_readmem24bedw,TYPE_32BIT_BE, 24DW, ALWAYS_ALIGNED)

READBYTE(data32_t,cpu_readmem26ledw,TYPE_32BIT_LE, 26DW)
READWORD(data32_t,cpu_readmem26ledw,TYPE_32BIT_LE, 26DW, ALWAYS_ALIGNED)
READLONG(data32_t,cpu_readmem26ledw,TYPE_32BIT_LE, 26DW, ALWAYS_ALIGNED)

READBYTE(data32_t,cpu_readmem32bedw,TYPE_32BIT_BE, 32DW)
READWORD(data32_t,cpu_readmem32bedw,TYPE_32BIT_BE, 32DW, ALWAYS_ALIGNED)
READLONG(data32_t,cpu_readmem32bedw,TYPE_32BIT_BE, 32DW, ALWAYS_ALIGNED)


/***************************************************************************

  Perform a memory write. This function is called by the CPU emulation.

***************************************************************************/

/* generic byte-sized write handler */
#define WRITEBYTE(dtype,name,type,abits)													  \
void name(offs_t address,dtype data)													\
{																						\
	MHELE hw;																			\
																						\
	MEMWRITESTART																		\
																						\
	/* first-level lookup */															\
	hw = cur_mwhard[address >> (ABITS2_##abits + ABITS_MIN_##abits)];					\
																						\
	/* for compatibility with setbankhandler, 8-bit systems must call handlers */		\
	/* for banked memory reads/writes */												\
	if (type == TYPE_8BIT && hw == HT_RAM)												\
	{																					\
		cpu_bankbase[HT_RAM][address] = data;											\
		MEMWRITEEND																		\
	}																					\
	else if (type != TYPE_8BIT && hw <= HT_BANKMAX) 									\
	{																					\
		if (type == TYPE_16BIT_BE)														\
			cpu_bankbase[hw][BYTE_XOR_BE(address) - memorywriteoffset[hw]] = data;		\
		else if (type == TYPE_16BIT_LE) 												\
			cpu_bankbase[hw][BYTE_XOR_LE(address) - memorywriteoffset[hw]] = data;		\
		else if (type == TYPE_32BIT_BE) 												\
			cpu_bankbase[hw][BYTE4_XOR_BE(address) - memorywriteoffset[hw]] = data; 	\
		else if (type == TYPE_32BIT_LE) 												\
			cpu_bankbase[hw][BYTE4_XOR_LE(address) - memorywriteoffset[hw]] = data; 	\
		MEMWRITEEND																		\
	}																					\
																						\
	/* second-level lookup */															\
	if (hw >= MH_HARDMAX)																\
	{																					\
		hw -= MH_HARDMAX;																\
		hw = writehardware[(hw << MH_SBITS) + ((address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))]; \
																						\
		/* for compatibility with setbankhandler, 8-bit systems must call handlers */	\
		/* for banked memory reads/writes */											\
		if (type == TYPE_8BIT && hw == HT_RAM)											\
		{																				\
			cpu_bankbase[HT_RAM][address] = data;										\
			MEMWRITEEND																	\
		}																				\
		else if (type != TYPE_8BIT && hw <= HT_BANKMAX) 								\
		{																				\
			if (type == TYPE_16BIT_BE)													\
				cpu_bankbase[hw][BYTE_XOR_BE(address) - memorywriteoffset[hw]] = data;	\
			else if (type == TYPE_16BIT_LE) 											\
				cpu_bankbase[hw][BYTE_XOR_LE(address) - memorywriteoffset[hw]] = data;	\
			else if (type == TYPE_32BIT_BE) 											\
				cpu_bankbase[hw][BYTE4_XOR_BE(address) - memorywriteoffset[hw]] = data; \
			else if (type == TYPE_32BIT_LE) 											\
				cpu_bankbase[hw][BYTE4_XOR_LE(address) - memorywriteoffset[hw]] = data; \
			MEMWRITEEND																	\
		}																				\
	}																					\
																						\
	/* fall back to handler */															\
	if (type == TYPE_32BIT_BE || type == TYPE_32BIT_LE) 								\
	{																					\
		UINT32 mem_mask;																\
		int shift = (/*mem_offs = */(address & 3)) << 3;									\
		if (type == TYPE_32BIT_BE)														\
			shift ^= 24;																\
		/*mem_width = MEMORY_WIDTH_8;*/ 													\
		mem_mask = (0xff000000UL >> shift);												\
		(*memorywrite32handler[hw])((address - memorywriteoffset[hw]) >> 2, (data & 0xff) << shift, mem_mask); \
	}																					\
	else if (type == TYPE_16BIT_BE || type == TYPE_16BIT_LE)							\
	{																					\
		UINT32 mem_mask;																\
		int shift = (/*mem_offs = */(address & 1)) << 3;									\
		if (type == TYPE_16BIT_BE)														\
			shift ^= 8; 																\
		/*mem_width = MEMORY_WIDTH_8;*/ 													\
		mem_mask = (0xff00 >> shift);													\
		(*memorywrite16handler[hw])((address - memorywriteoffset[hw]) >> 1, (data & 0xff) << shift, mem_mask); \
	}																					\
	else																				\
		(*memorywritehandler[hw])(address - memorywriteoffset[hw], data);				\
	MEMWRITEEND																			\
}

/* generic word-sized write handler (16-bit and 32-bit aligned only!) */
#define WRITEWORD(dtype,name,type,abits,align)												  \
void name##_word(offs_t address,dtype data) 											\
{																						\
	MHELE hw;																			\
																						\
	MEMWRITESTART																		\
																						\
	/* only supports 16-bit and 32-bit memory systems */								\
	if (type == TYPE_8BIT)																\
		printf("Unsupported type for WRITEWORD macro!\n");                              \
																						\
	/* handle aligned case first */ 													\
	if ((type == TYPE_32BIT_BE || type == TYPE_32BIT_LE) && 							\
		(align == ALWAYS_ALIGNED || !(address & 1)))									\
	{																					\
		/* first-level lookup */														\
		hw = cur_mwhard[address >> (ABITS2_##abits + ABITS_MIN_##abits)];				\
		if (hw <= HT_BANKMAX)															\
		{																				\
			if (type == TYPE_32BIT_BE)													\
				WRITE_WORD(&cpu_bankbase[hw][WORD_XOR_BE(address - memorywriteoffset[hw])], data); \
			else																		\
				WRITE_WORD(&cpu_bankbase[hw][WORD_XOR_LE(address - memorywriteoffset[hw])], data); \
			MEMWRITEEND																	\
		}																				\
																						\
		/* second-level lookup */														\
		if (hw >= MH_HARDMAX)															\
		{																				\
			hw -= MH_HARDMAX;															\
			hw = writehardware[(hw << MH_SBITS) + ((address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))]; \
			if (hw <= HT_BANKMAX)														\
			{																			\
				if (type == TYPE_32BIT_BE)												\
					WRITE_WORD(&cpu_bankbase[hw][WORD_XOR_BE(address - memorywriteoffset[hw])], data); \
				else																	\
					WRITE_WORD(&cpu_bankbase[hw][WORD_XOR_LE(address - memorywriteoffset[hw])], data); \
				MEMWRITEEND																\
			}																			\
		}																				\
		else																			\
		{																				\
			UINT32 mem_mask;															\
			int shift = (/*mem_offs = */(address & 2)) << 3;								\
			if (type == TYPE_32BIT_BE)													\
				shift ^= 16;															\
			/*mem_width = MEMORY_WIDTH_16;*/												\
			mem_mask = 0xffff0000UL >> shift;											\
			/* fall back to handler */													\
			(*memorywrite32handler[hw]) ((address - memorywriteoffset[hw]) >> 2, (data & 0xffff) << shift, mem_mask); \
		}																				\
	}																					\
	else																				\
	if ((type == TYPE_16BIT_BE || type == TYPE_16BIT_LE) && 							\
		(align == ALWAYS_ALIGNED || !(address & 1)))									\
	{																					\
		/* first-level lookup */														\
		hw = cur_mwhard[address >> (ABITS2_##abits + ABITS_MIN_##abits)];				\
		if (hw <= HT_BANKMAX)															\
		{																				\
			WRITE_WORD(&cpu_bankbase[hw][address - memorywriteoffset[hw]], data);		\
			MEMWRITEEND																	\
		}																				\
																						\
		/* second-level lookup */														\
		if (hw >= MH_HARDMAX)															\
		{																				\
			hw -= MH_HARDMAX;															\
			hw = writehardware[(hw << MH_SBITS) + ((address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))]; \
			if (hw <= HT_BANKMAX)														\
			{																			\
				WRITE_WORD(&cpu_bankbase[hw][address - memorywriteoffset[hw]], data);	\
				MEMWRITEEND																\
			}																			\
		}																				\
																						\
		/* fall back to handler */														\
		/*mem_width = MEMORY_WIDTH_16;*/													\
		/*mem_mask = 0;*/																	\
		/*mem_offs = 0;*/																	\
		(*memorywrite16handler[hw]) ((address - memorywriteoffset[hw]) >> 1, data, 0);		\
	}																					\
																						\
	/* unaligned case */																\
	else if (type == TYPE_32BIT_LE || type == TYPE_16BIT_BE)							\
	{																					\
		name(address, data >> 8);														\
		name((address + 1) & ADDRESS_MASK(abits), (data & 0xff));						\
	}																					\
	else if (type == TYPE_32BIT_BE || type == TYPE_16BIT_LE)							\
	{																					\
		name(address, data & 0xff); 													\
		name((address + 1) & ADDRESS_MASK(abits), data >> 8);							\
	}																					\
	MEMWRITEEND																			\
}

/* generic dword-sized write handler (32-bit aligned only!) */
#define WRITELONG(dtype,name,type,abits,align)											\
void name##_dword(offs_t address,dtype data)											\
{																						\
	MHELE hw1;		 																	\
																						\
	MEMWRITESTART																		\
																						\
	/* only supports 32-bit memory systems */											\
	if (type == TYPE_8BIT || type == TYPE_16BIT_BE || type == TYPE_16BIT_LE)			\
		printf("Unsupported type for WRITELONG macro!\n");								\
																						\
	/* handle aligned case first */ 													\
	if (align == ALWAYS_ALIGNED || !(address & 3))										\
	{																					\
		/* first-level lookup */														\
		hw1 = cur_mwhard[address >> (ABITS2_##abits + ABITS_MIN_##abits)];				\
																						\
		/* second-level lookup */														\
		if (hw1 >= MH_HARDMAX)															\
		{																				\
			hw1 -= MH_HARDMAX;															\
			hw1 = writehardware[(hw1 << MH_SBITS) + ((address >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))]; \
		}																				\
		if (hw1 <= HT_BANKMAX)															\
			WRITE_DWORD(&cpu_bankbase[hw1][address - memoryreadoffset[hw1]], data); 	\
		else																			\
		{																				\
			/*mem_width = MEMORY_WIDTH_32;*/												\
			/*mem_mask = 0;*/																\
			/*mem_offs = 0;*/																\
			(*memorywrite32handler[hw1])((address - memoryreadoffset[hw1]) >> 2, data, 0);	\
		}																				\
	}																					\
	/* unaligned case */																\
	else if (type == TYPE_32BIT_BE && !(address & 1))									\
	{																					\
		name##_word(address, data >> 16);												\
		name##_word((address + 2) & ADDRESS_MASK(abits), data & 0xffff);				\
	}																					\
	else if (type == TYPE_32BIT_LE && !(address & 1))									\
	{																					\
		name##_word(address, data & 0xffff);											\
		name##_word((address + 2) & ADDRESS_MASK(abits), data >> 16);					\
	}																					\
	else if (type == TYPE_32BIT_BE)														\
	{																					\
		name(address, data >> 24);														\
		name##_word((address + 1) & ADDRESS_MASK(abits), (data >> 8) & 0xffff); 		\
		name((address + 3) & ADDRESS_MASK(abits), data & 0xff); 						\
	}																					\
	else if (type == TYPE_16BIT_BE)														\
	{																					\
		name(address, data & 0xff); 													\
		name##_word((address + 1) & ADDRESS_MASK(abits), (data >> 8) & 0xffff); 		\
		name((address + 3) & ADDRESS_MASK(abits), data >> 24);							\
	}																					\
	MEMWRITEEND																			\
}


/* the handlers we need to generate */
WRITEBYTE(data8_t,  cpu_writemem16,   TYPE_8BIT,	 16)
WRITEBYTE(data8_t,  cpu_writemem20,   TYPE_8BIT,	 20)
WRITEBYTE(data8_t,  cpu_writemem21,   TYPE_8BIT,	 21)
WRITEBYTE(data8_t,  cpu_writemem24,   TYPE_8BIT,	 24)

WRITEBYTE(data16_t,cpu_writemem16bew, TYPE_16BIT_BE, 16W)
WRITEWORD(data16_t,cpu_writemem16bew, TYPE_16BIT_BE, 16W, ALWAYS_ALIGNED)

WRITEBYTE(data16_t,cpu_writemem16lew, TYPE_16BIT_LE, 16W)
WRITEWORD(data16_t,cpu_writemem16lew, TYPE_16BIT_LE, 16W, ALWAYS_ALIGNED)

WRITEBYTE(data16_t,cpu_writemem24bew, TYPE_16BIT_BE, 24W)
WRITEWORD(data16_t,cpu_writemem24bew, TYPE_16BIT_BE, 24W, ALWAYS_ALIGNED)

WRITEBYTE(data16_t,cpu_writemem29lew, TYPE_16BIT_LE, 29W)
WRITEWORD(data16_t,cpu_writemem29lew, TYPE_16BIT_LE, 29W, CAN_BE_MISALIGNED)

WRITEBYTE(data16_t,cpu_writemem32bew, TYPE_16BIT_BE, 32W)
WRITEWORD(data16_t,cpu_writemem32bew, TYPE_16BIT_BE, 32W, ALWAYS_ALIGNED)

WRITEBYTE(data16_t,cpu_writemem32lew, TYPE_16BIT_LE, 32W)
WRITEWORD(data16_t,cpu_writemem32lew, TYPE_16BIT_LE, 32W, ALWAYS_ALIGNED)

WRITEBYTE(data32_t,cpu_writemem24bedw,TYPE_32BIT_BE, 24DW)
WRITEWORD(data32_t,cpu_writemem24bedw,TYPE_32BIT_BE, 24DW, ALWAYS_ALIGNED)
WRITELONG(data32_t,cpu_writemem24bedw,TYPE_32BIT_BE, 24DW, ALWAYS_ALIGNED)

WRITEBYTE(data32_t,cpu_writemem26ledw,TYPE_32BIT_LE, 26DW)
WRITEWORD(data32_t,cpu_writemem26ledw,TYPE_32BIT_LE, 26DW, ALWAYS_ALIGNED)
WRITELONG(data32_t,cpu_writemem26ledw,TYPE_32BIT_LE, 26DW, ALWAYS_ALIGNED)

WRITEBYTE(data32_t,cpu_writemem32bedw,TYPE_32BIT_BE, 32DW)
WRITEWORD(data32_t,cpu_writemem32bedw,TYPE_32BIT_BE, 32DW, ALWAYS_ALIGNED)
WRITELONG(data32_t,cpu_writemem32bedw,TYPE_32BIT_BE, 32DW, ALWAYS_ALIGNED)


/***************************************************************************

  Opcode base changers. This function is called by the CPU emulation.

***************************************************************************/

/* generic opcode base changer */
#define SETOPBASE(name,abits,shift) 													\
void name(offs_t pc)																	\
{																						\
	MHELE hw;																			\
																						\
	pc = pc >> shift;																	\
																						\
	/* allow overrides */																\
	if (OPbasefunc) 																	\
	{																					\
		pc = OPbasefunc(pc);															\
		if (pc == -1)																	\
			return; 																	\
	}																					\
																						\
	/* perform the lookup */															\
	hw = cur_mrhard[pc >> (ABITS2_##abits + ABITS_MIN_##abits)];						\
	if (hw >= MH_HARDMAX)																\
	{																					\
		hw -= MH_HARDMAX;																\
		hw = readhardware[(hw << MH_SBITS) + (((UINT32)pc >> ABITS_MIN_##abits) & MHMASK(ABITS2_##abits))]; \
	}																					\
	ophw = hw;																			\
																						\
	/* RAM or banked memory */															\
	if (hw <= HT_BANKMAX)																\
	{																					\
		SET_OP_RAMROM(cpu_bankbase[hw] - memoryreadoffset[hw])							\
		return; 																		\
	}																					\
																						\
	/* do not support on callback memory region */										\
	logerror("CPU #%d PC %04x: warning - op-code execute on mapped i/o\n",				\
				cpu_getactivecpu(),cpu_get_pc());										\
}


/* the handlers we need to generate */
SETOPBASE(cpu_setOPbase16,     16,   0)
SETOPBASE(cpu_setOPbase20,     20,   0)
SETOPBASE(cpu_setOPbase21,     21,   0)
SETOPBASE(cpu_setOPbase24,     24,   0)
SETOPBASE(cpu_setOPbase16bew,  16W,  0)
SETOPBASE(cpu_setOPbase16lew,  16W,  0)
SETOPBASE(cpu_setOPbase24bew,  24W,  0)
SETOPBASE(cpu_setOPbase29lew,  29W,  3)
SETOPBASE(cpu_setOPbase32bew,  32W,  0)
SETOPBASE(cpu_setOPbase32lew,  32W,  0)
SETOPBASE(cpu_setOPbase24bedw, 24DW, 0)
SETOPBASE(cpu_setOPbase26ledw, 26DW, 0)
SETOPBASE(cpu_setOPbase32bedw, 32DW, 0)


/***************************************************************************

  Perform an I/O port read. This function is called by the CPU emulation.

***************************************************************************/
int cpu_readport(int port)
{
	const struct IO_ReadPort *iorp = cur_readport;

	port &= cur_portmask;

	if (iorp)
	{
		/* search the handlers. The order is as follows: first the dynamically installed
		   handlers are searched, followed by the static ones in whatever order they were
		   specified in the driver */
		while (!IS_MEMORY_END(iorp))
		{
			if (!IS_MEMORY_MARKER(iorp))
			{
				if (port >= iorp->start && port <= iorp->end)
				{
					mem_read_handler handler = iorp->handler;


					if (handler == IORP_NOP) return 0;
					else return (*handler)(port - iorp->start);
				}
			}

			iorp++;
		}
	}

	logerror("CPU #%d PC %04x: warning - read unmapped I/O port %02x\n",cpu_getactivecpu(),cpu_get_pc(),port);
	return 0;
}


/***************************************************************************

  Perform an I/O port write. This function is called by the CPU emulation.

***************************************************************************/
void cpu_writeport(int port, int value)
{
	const struct IO_WritePort *iowp = cur_writeport;

	port &= cur_portmask;

	if (iowp)
	{
		/* search the handlers. The order is as follows: first the dynamically installed
		   handlers are searched, followed by the static ones in whatever order they were
		   specified in the driver */
		while (!IS_MEMORY_END(iowp))
		{
			if (!IS_MEMORY_MARKER(iowp))
			{
				if (port >= iowp->start && port <= iowp->end)
				{
					mem_write_handler handler = iowp->handler;


					if (handler == IOWP_NOP) return;
					else (*handler)(port - iowp->start,value);

					return;
				}
			}

			iowp++;
		}
	}

	logerror("CPU #%d PC %04x: warning - write %02x to unmapped I/O port %02x\n",cpu_getactivecpu(),cpu_get_pc(),value,port);
}


/* set readmemory handler for bank memory  */
void cpu_setbankhandler_r(int bank, mem_read_handler handler)
{
	int offset = 0;
	MHELE hardware;
	mem_read16_handler handler16 = (mem_read16_handler)handler;
	mem_read32_handler handler32 = (mem_read32_handler)handler;

	switch( (FPTR)handler )
	{
	case (FPTR)MRA_RAM:
	case (FPTR)MRA_ROM:
		handler = mrh_ram;
		handler16 = mrh_ram16;
		handler32 = mrh_ram32;
		break;
	case (FPTR)MRA_BANK1:
	case (FPTR)MRA_BANK2:
	case (FPTR)MRA_BANK3:
	case (FPTR)MRA_BANK4:
	case (FPTR)MRA_BANK5:
	case (FPTR)MRA_BANK6:
	case (FPTR)MRA_BANK7:
	case (FPTR)MRA_BANK8:
	case (FPTR)MRA_BANK9:
	case (FPTR)MRA_BANK10:
	case (FPTR)MRA_BANK11:
	case (FPTR)MRA_BANK12:
	case (FPTR)MRA_BANK13:
	case (FPTR)MRA_BANK14:
	case (FPTR)MRA_BANK15:
	case (FPTR)MRA_BANK16:
	case (FPTR)MRA_BANK17:
	case (FPTR)MRA_BANK18:
	case (FPTR)MRA_BANK19:
	case (FPTR)MRA_BANK20:
		hardware = (int)MWA_BANK1 - (int)handler + 1;
		offset = bankreadoffset[hardware];
		handler = bank_read_handler[hardware];
		handler16 = bank_read16_handler[hardware];
		handler32 = bank_read32_handler[hardware];
		break;
	case (FPTR)MRA_NOP:
		handler = mrh_nop;
		handler16 = mrh_nop16;
		handler32 = mrh_nop32;
		break;
	default:
		offset = bankreadoffset[bank];
		break;
	}
	memoryreadoffset[bank] = offset;
	memoryreadhandler[bank] = handler;
	memoryread16handler[bank] = handler16;
	memoryread32handler[bank] = handler32;
}

/* set writememory handler for bank memory	*/
void cpu_setbankhandler_w(int bank, mem_write_handler handler)
{
	int offset = 0;
	MHELE hardware;
	mem_write16_handler handler16 = (mem_write16_handler)handler;
	mem_write32_handler handler32 = (mem_write32_handler)handler;

	switch( (FPTR)handler )
	{
	case (FPTR)MWA_RAM:
		handler = mwh_ram;
		handler16 = mwh_ram16;
		handler32 = mwh_ram32;
		break;
	case (FPTR)MWA_BANK1:
	case (FPTR)MWA_BANK2:
	case (FPTR)MWA_BANK3:
	case (FPTR)MWA_BANK4:
	case (FPTR)MWA_BANK5:
	case (FPTR)MWA_BANK6:
	case (FPTR)MWA_BANK7:
	case (FPTR)MWA_BANK8:
	case (FPTR)MWA_BANK9:
	case (FPTR)MWA_BANK10:
	case (FPTR)MWA_BANK11:
	case (FPTR)MWA_BANK12:
	case (FPTR)MWA_BANK13:
	case (FPTR)MWA_BANK14:
	case (FPTR)MWA_BANK15:
	case (FPTR)MWA_BANK16:
	case (FPTR)MWA_BANK17:
	case (FPTR)MWA_BANK18:
	case (FPTR)MWA_BANK19:
	case (FPTR)MWA_BANK20:
		hardware = (int)MWA_BANK1 - (int)handler + 1;
		offset = bankwriteoffset[hardware];
		handler = bank_write_handler[hardware];
		handler16 = bank_write16_handler[hardware];
		handler32 = bank_write32_handler[hardware];
		break;
	case (FPTR)MWA_NOP:
		handler = mwh_nop;
		handler16 = mwh_nop16;
		handler32 = mwh_nop32;
		break;
	case (FPTR)MWA_RAMROM:
		handler = mwh_ramrom;
		handler16 = mwh_ramrom16;
		handler32 = mwh_ramrom32;
		break;
	case (FPTR)MWA_ROM:
		handler = mwh_rom;
		handler16 = mwh_rom16;
		handler32 = mwh_rom32;
		break;
	default:
		offset = bankwriteoffset[bank];
		break;
	}
	memorywriteoffset[bank] = offset;
	memorywritehandler[bank] = handler;
	memorywrite16handler[bank] = handler16;
	memorywrite32handler[bank] = handler32;
}

/* cpu change op-code memory base */
void cpu_setOPbaseoverride (int cpu,opbase_handler function)
{
	setOPbasefunc[cpu] = function;
	if (cpu == cpu_getactivecpu())
		OPbasefunc = function;
}


static void *internal_install_mem_read_handler(int cpu, int start, int end, mem_read_handler handler)
{
	MHELE hardware = 0;
	int abitsmin;
	int i, hw_set;


#if VERBOSE
	logerror("Install new memory read handler:\n");
	logerror("             cpu: %d\n", cpu);
	logerror("           start: 0x%08x\n", start);
	logerror("             end: 0x%08x\n", end);
#ifdef __LP64__
	logerror(" handler address: 0x%016lx\n", (unsigned long) handler);
#else
	logerror(" handler address: 0x%08x\n", (unsigned int) handler);
#endif
#endif
	abitsmin = ABITSMIN (cpu);

	if (end < start)
	{
		printf("fatal: install_mem_read_handler(), start = %08x > end = %08x\n",start,end);
		exit(1);
	}
	if ((start & (ALIGNUNIT(cpu)-1)) != 0 || (end & (ALIGNUNIT(cpu)-1)) != (ALIGNUNIT(cpu)-1))
	{
		printf("fatal: install_mem_read_handler(), start = %08x, end = %08x ALIGN = %d\n",start,end,ALIGNUNIT(cpu));
		exit(1);
	}

	/* see if this function is already registered */
	hw_set = 0;
	for ( i = 0 ; i < MH_HARDMAX ; i++)
	{
		/* record it if it matches */
		if (( memoryreadhandler[i] == handler ) &&
			(  memoryreadoffset[i] == start))
		{
#if VERBOSE
			logerror("handler match - use old one\n");
#endif
			hardware = i;
			hw_set = 1;
		}
	}
	switch ((FPTR)handler)
	{
		case (FPTR)MRA_RAM:
		case (FPTR)MRA_ROM:
			hardware = HT_RAM;	/* special case ram read */
			hw_set = 1;
			break;
		case (FPTR)MRA_BANK1:
		case (FPTR)MRA_BANK2:
		case (FPTR)MRA_BANK3:
		case (FPTR)MRA_BANK4:
		case (FPTR)MRA_BANK5:
		case (FPTR)MRA_BANK6:
		case (FPTR)MRA_BANK7:
		case (FPTR)MRA_BANK8:
		case (FPTR)MRA_BANK9:
		case (FPTR)MRA_BANK10:
		case (FPTR)MRA_BANK11:
		case (FPTR)MRA_BANK12:
		case (FPTR)MRA_BANK13:
		case (FPTR)MRA_BANK14:
		case (FPTR)MRA_BANK15:
		case (FPTR)MRA_BANK16:
		case (FPTR)MRA_BANK17:
		case (FPTR)MRA_BANK18:
		case (FPTR)MRA_BANK19:
		case (FPTR)MRA_BANK20:
		{
			hardware = (int)MRA_BANK1 - (int)handler + 1;
			memoryreadoffset[hardware] = bankreadoffset[hardware] = start;
			cpu_bankbase[hardware] = memory_find_base(cpu, start);
			hw_set = 1;
			break;
		}
		case (FPTR)MRA_NOP:
			hardware = HT_NOP;
			hw_set = 1;
			break;
	}
	if (!hw_set)  /* no match */
	{
		/* create newer hardware handler */
		if( rdhard_max == MH_HARDMAX )
		{
			logerror("read memory hardware pattern over !\n");
			logerror("Failed to install new memory handler.\n");
			return memory_find_base(cpu, start);
		}
		else
		{
			/* register hardware function */
			hardware = rdhard_max++;
			memoryreadoffset[hardware] = start;
			memoryreadhandler[hardware] = handler;
			memoryread16handler[hardware] = (mem_read16_handler)handler;
			memoryread32handler[hardware] = (mem_read32_handler)handler;
		}
	}
	/* set hardware element table entry */
	set_element( cpu , cur_mr_element[cpu] ,
		(((unsigned int) start) >> abitsmin) ,
		(((unsigned int) end) >> abitsmin) ,
		hardware , readhardware , &rdelement_max );
#if VERBOSE
	logerror("Done installing new memory handler.\n");
	logerror("used read  elements %d/%d , functions %d/%d\n"
			,rdelement_max,MH_ELEMAX , rdhard_max,MH_HARDMAX );
#endif
	return memory_find_base(cpu, start);
}

void *install_mem_read_handler(int cpu, int start, int end, mem_read_handler handler)
{
	if (cpunum_databus_width(cpu) != 8)
	{
		printf("fatal: install_mem_read_handler called on %d-bit cpu\n",cpunum_databus_width(cpu));
		exit(1);	/* fail */
	}

	return internal_install_mem_read_handler (cpu, start, end, handler);
}

void *install_mem_read16_handler(int cpu, int start, int end, mem_read16_handler handler)
{
	if (cpunum_databus_width(cpu) != 16)
	{
		printf("fatal: install_mem_read16_handler called on %d-bit cpu\n",cpunum_databus_width(cpu));
		exit(1);	/* fail */
	}

	return internal_install_mem_read_handler (cpu, start, end, (mem_read_handler)handler);
}

void *install_mem_read32_handler(int cpu, int start, int end, mem_read32_handler handler)
{
	if (cpunum_databus_width(cpu) != 32)
	{
		printf("fatal: install_mem_read32_handler called on %d-bit cpu\n",cpunum_databus_width(cpu));
		exit(1);	/* fail */
	}

	return internal_install_mem_read_handler (cpu, start, end, (mem_read_handler)handler);
}

static void *internal_install_mem_write_handler(int cpu, int start, int end, mem_write_handler handler)
{
	MHELE hardware = 0;
	int abitsmin;
	int i, hw_set;


#if VERBOSE
	logerror("Install new memory write handler:\n");
	logerror("             cpu: %d\n", cpu);
	logerror("           start: 0x%08x\n", start);
	logerror("             end: 0x%08x\n", end);
#ifdef __LP64__
	logerror(" handler address: 0x%016lx\n", (unsigned long) handler);
#else
	logerror(" handler address: 0x%08x\n", (unsigned int) handler);
#endif
#endif
	abitsmin = ABITSMIN (cpu);

	if (end < start)
	{
		printf("fatal: install_mem_write_handler(), start = %08x > end = %08x\n",start,end);
		exit(1);
	}
	if ((start & (ALIGNUNIT(cpu)-1)) != 0 || (end & (ALIGNUNIT(cpu)-1)) != (ALIGNUNIT(cpu)-1))
	{
		printf("fatal: install_mem_write_handler(), start = %08x, end = %08x ALIGN = %d\n",start,end,ALIGNUNIT(cpu));
		exit(1);
	}

	/* see if this function is already registered */
	hw_set = 0;
	for ( i = 0 ; i < MH_HARDMAX ; i++)
	{
		/* record it if it matches */
		if (( memorywritehandler[i] == handler ) &&
			(  memorywriteoffset[i] == start))
		{
#if VERBOSE
			logerror("handler match - use old one\n");
#endif
			hardware = i;
			hw_set = 1;
		}
	}

	switch( (FPTR)handler )
	{
		case (FPTR)MWA_RAM:
			hardware = HT_RAM;	/* special case ram write */
			hw_set = 1;
			break;
		case (FPTR)MWA_BANK1:
		case (FPTR)MWA_BANK2:
		case (FPTR)MWA_BANK3:
		case (FPTR)MWA_BANK4:
		case (FPTR)MWA_BANK5:
		case (FPTR)MWA_BANK6:
		case (FPTR)MWA_BANK7:
		case (FPTR)MWA_BANK8:
		case (FPTR)MWA_BANK9:
		case (FPTR)MWA_BANK10:
		case (FPTR)MWA_BANK11:
		case (FPTR)MWA_BANK12:
		case (FPTR)MWA_BANK13:
		case (FPTR)MWA_BANK14:
		case (FPTR)MWA_BANK15:
		case (FPTR)MWA_BANK16:
		case (FPTR)MWA_BANK17:
		case (FPTR)MWA_BANK18:
		case (FPTR)MWA_BANK19:
		case (FPTR)MWA_BANK20:
		{
			hardware = (int)MWA_BANK1 - (int)handler + 1;
			memorywriteoffset[hardware] = bankwriteoffset[hardware] = start;
			cpu_bankbase[hardware] = memory_find_base(cpu, start);
			hw_set = 1;
			break;
		}
		case (FPTR)MWA_NOP:
			hardware = HT_NOP;
			hw_set = 1;
			break;
		case (FPTR)MWA_RAMROM:
			hardware = HT_RAMROM;
			hw_set = 1;
			break;
		case (FPTR)MWA_ROM:
			hardware = HT_ROM;
			hw_set = 1;
			break;
	}
	if (!hw_set)  /* no match */
	{
		/* create newer hardware handler */
		if( wrhard_max == MH_HARDMAX )
		{
			logerror("write memory hardware pattern over !\n");
			logerror("Failed to install new memory handler.\n");

			return memory_find_base(cpu, start);
		}
		else
		{
			/* register hardware function */
			hardware = wrhard_max++;
			memorywriteoffset[hardware] = start;
			memorywritehandler[hardware] = handler;
			memorywrite16handler[hardware] = (mem_write16_handler)handler;
			memorywrite32handler[hardware] = (mem_write32_handler)handler;
		}
	}
	/* set hardware element table entry */
	set_element( cpu , cur_mw_element[cpu] ,
		(((unsigned int) start) >> abitsmin) ,
		(((unsigned int) end) >> abitsmin) ,
		hardware , writehardware , &wrelement_max );
#if VERBOSE
	logerror("Done installing new memory handler.\n");
	logerror("used write elements %d/%d , functions %d/%d\n"
			,wrelement_max,MH_ELEMAX , wrhard_max,MH_HARDMAX );
#endif
	return memory_find_base(cpu, start);
}

void *install_mem_write_handler(int cpu, int start, int end, mem_write_handler handler)
{
	if (cpunum_databus_width(cpu) != 8)
	{
		printf("fatal: install_mem_write_handler called on %d-bit cpu\n",cpunum_databus_width(cpu));
		exit(1);	/* fail */
	}

	return internal_install_mem_write_handler (cpu, start, end, handler);
}

void *install_mem_write16_handler(int cpu, int start, int end, mem_write16_handler handler)
{
	if (cpunum_databus_width(cpu) != 16)
	{
		printf("fatal: install_mem_write16_handler called on %d-bit cpu\n",cpunum_databus_width(cpu));
		exit(1);	/* fail */
	}

	return internal_install_mem_write_handler (cpu, start, end, (mem_write_handler)handler);
}

void *install_mem_write32_handler(int cpu, int start, int end, mem_write32_handler handler)
{
	if (cpunum_databus_width(cpu) != 32)
	{
		printf("fatal: install_mem_write32_handler called on %d-bit cpu\n",cpunum_databus_width(cpu));
		exit(1);	/* fail */
	}

	return internal_install_mem_write_handler (cpu, start, end, (mem_write_handler)handler);
}



void *install_port_read_handler(int cpu, int start, int end, mem_read_handler handler)
{
	return install_port_read_handler_common(cpu, start, end, handler, 1);
}

void *install_port_write_handler(int cpu, int start, int end, mem_write_handler handler)
{
	return install_port_write_handler_common(cpu, start, end, handler, 1);
}

static void *install_port_read_handler_common(int cpu, int start, int end,
											  mem_read_handler handler, int install_at_beginning)
{
	int i, oldsize;
	struct IO_ReadPort *old_readport;

	if (readport[cpu] == 0)
	{
		readport_size[cpu] = sizeof(struct IO_ReadPort);
		readport[cpu] = malloc(readport_size[cpu]);
		if (readport[cpu] == 0) return 0;
		readport[cpu][0].start = MEMORY_MARKER;
		readport[cpu][0].end = 0;
		readport[cpu][0].handler = 0;
	}

	old_readport = readport[cpu];
	oldsize = readport_size[cpu];
	readport_size[cpu] += sizeof(struct IO_ReadPort);

	readport[cpu] = realloc(readport[cpu], readport_size[cpu]);

	/* check if we're changing the current readport and ifso update it */
	if (cur_readport == old_readport)
	   cur_readport = readport[cpu];

	/* realloc leaves the old buffer intact if it fails, so free it */
	if(!readport[cpu])
	   free(old_readport);

	if (readport[cpu] == 0)  return 0;

	if (install_at_beginning)
	{
		i = 0;
		memmove(&readport[cpu][1], &readport[cpu][0], oldsize);
	}
	else
	{
		i = oldsize / sizeof(struct IO_ReadPort) - 1;
		memmove(&readport[cpu][i+1], &readport[cpu][i], sizeof(struct IO_ReadPort));
	}

#ifdef MEM_DUMP
	logerror("Installing port read handler: cpu %d  slot %X  start %X  end %X\n", cpu, i, start, end);
#endif

	readport[cpu][i].start = start;
	readport[cpu][i].end = end;
	readport[cpu][i].handler = handler;

	return readport[cpu];
}

static void *install_port_write_handler_common(int cpu, int start, int end,
											   mem_write_handler handler, int install_at_beginning)
{
	int i, oldsize;
	struct IO_WritePort *old_writeport;

	if (writeport[cpu] == 0)
	{
		writeport_size[cpu] = sizeof(struct IO_WritePort);
		writeport[cpu] = malloc(writeport_size[cpu]);
		if (writeport[cpu] == 0) return 0;
		writeport[cpu][0].start = MEMORY_MARKER;
		writeport[cpu][0].end = 0;
		writeport[cpu][0].handler = 0;
	}

	old_writeport = writeport[cpu];
	oldsize = writeport_size[cpu];
	writeport_size[cpu] += sizeof(struct IO_WritePort);

	writeport[cpu] = realloc(writeport[cpu], writeport_size[cpu]);

	/* check if we're changing the current writeport and ifso update it */
	if (cur_writeport == old_writeport)
		cur_writeport = writeport[cpu];

	/* realloc leaves the old buffer intact if it fails, so free it */
	if(!writeport[cpu])
		free(old_writeport);

	if (writeport[cpu] == 0)  return 0;

	if (install_at_beginning)
	{
		i = 0;
		memmove(&writeport[cpu][1], &writeport[cpu][0], oldsize);
	}
	else
	{
		i = oldsize / sizeof(struct IO_WritePort) - 1;
		memmove(&writeport[cpu][i+1], &writeport[cpu][i], sizeof(struct IO_WritePort));
	}

#ifdef MEM_DUMP
	logerror("Installing port write handler: cpu %d  slot %X  start %X  end %X\n", cpu, i, start, end);
#endif

	writeport[cpu][i].start = start;
	writeport[cpu][i].end = end;
	writeport[cpu][i].handler = handler;

	return writeport[cpu];
}

#ifdef MEM_DUMP
static void mem_dump( void )
{
	int cpu;
	int naddr,addr;
	MHELE nhw,hw;

	FILE *temp = fopen ("memdump.log", "w");

	if (!temp) return;

	for( cpu = 0 ; cpu < 1 ; cpu++ )
	{
		fprintf(temp,"cpu %d read memory \n",cpu);
		addr = 0;
		naddr = 0;
		nhw = 0xff;
		while( (addr >> mhshift[cpu][0]) <= mhmask[cpu][0] ){
			hw = cur_mr_element[cpu][addr >> mhshift[cpu][0]];
			if( hw >= MH_HARDMAX )
			{	/* 2nd element link */
				hw = readhardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((addr>>mhshift[cpu][1]) & mhmask[cpu][1])];
				if( hw >= MH_HARDMAX )
					hw = readhardware[((hw-MH_HARDMAX)<<MH_SBITS) + (addr & mhmask[cpu][2])];
			}
			if( nhw != hw )
			{
				if( addr )
					fprintf(temp,"  %08x(%08x) - %08x = %02x\n",naddr,memoryreadoffset[nhw],addr-1,nhw);
				nhw = hw;
				naddr = addr;
			}
			addr++;
		}
		fprintf(temp,"  %08x(%08x) - %08x = %02x\n",naddr,memoryreadoffset[nhw],addr-1,nhw);

		fprintf(temp,"cpu %d write memory \n",cpu);
		naddr = 0;
		addr = 0;
		nhw = 0xff;
		while( (addr >> mhshift[cpu][0]) <= mhmask[cpu][0] ){
			hw = cur_mw_element[cpu][addr >> mhshift[cpu][0]];
			if( hw >= MH_HARDMAX )
			{	/* 2nd element link */
				hw = writehardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((addr>>mhshift[cpu][1]) & mhmask[cpu][1])];
				if( hw >= MH_HARDMAX )
					hw = writehardware[((hw-MH_HARDMAX)<<MH_SBITS) + (addr & mhmask[cpu][2])];
			}
			if( nhw != hw )
			{
				if( addr )
					fprintf(temp,"  %08x(%08x) - %08x = %02x\n",naddr,memorywriteoffset[nhw],addr-1,nhw);
				nhw = hw;
				naddr = addr;
			}
			addr++;
		}
		fprintf(temp,"  %08x(%08x) - %08x = %02x\n",naddr,memorywriteoffset[nhw],addr-1,nhw);
	}
	fclose(temp);
}
#endif
