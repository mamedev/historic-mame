/***************************************************************************

  memory.c

  Functions which handle the CPU memory and I/O port access.

***************************************************************************/

#include "driver.h"

/* in the mame.c
unsigned char *RAM;
unsigned char *ROM;
*/
unsigned char *OP_RAM;
unsigned char *OP_ROM;
MHELE ophw;				/* op-code hardware number */

static unsigned char *ramptr[MAX_CPU],*romptr[MAX_CPU];
// #define PORT_NEW		/* newer i/o port handler ( not fix yet) */
// #define MEM_DUMP


#if LSB_FIRST
	#define BIG_WORD(x) ((((x) >> 8) & 0xff) + (((x) & 0xff) << 8))
	#define BIG_DWORD(x) (((x) >> 24) + (((x) >> 8) & 0xff00) + (((x) & 0xff00) << 8) + ((x) << 24))
#else
	#define BIG_WORD(x) (x)
	#define BIG_DWORD(x) (x)
#endif


/* element shift bits, mask bits */
int mhshift[MAX_CPU][3] , mhmask[MAX_CPU][3];
/* current hardware element map */
static MHELE *cur_mr_element[MAX_CPU];
static MHELE *cur_mw_element[MAX_CPU];
#ifdef PORT_NEW
static MHELE cur_pr_element[MAX_CPU][1<<MH_PBITS];
static MHELE cur_pw_element[MAX_CPU][1<<MH_PBITS];
#endif

/* bank ram base address */
unsigned char *cpu_bankbase[6];
int cpu_bankoffset[6];	/* ASG 971005 */

/* sub memory/port hardware element map */
static MHELE readhardware[MH_ELEMAX<<MH_SBITS];  /* mem/port read  */
static MHELE writehardware[MH_ELEMAX<<MH_SBITS]; /* mem/port write */
/* memory hardware element map */
/* value:                      */
#define HT_RAM    0		/* RAM direct        */
#define HT_BANK1  1		/* bank memory #1    */
#define HT_BANK2  2		/* bank memory #2    */
#define HT_BANK3  3		/* bank memory #3    */
#define HT_BANK4  4		/* bank memory #4    */
#define HT_BANK5  5		/* bank memory #5    */
#define HT_NON    6		/* non mapped memory */
#define HT_NOP    7		/* NOP memory        */
#define HT_RAMROM 8		/* RAM ROM memory    */
#define HT_ROM    9		/* ROM memory        */

#define HT_USER   10	/* user functions */
/* [MH_HARDMAX]-0xff	   link to sub memory element  */
/*                         (value-MH_HARDMAX)<<MH_SBITS -> element bank */

/* memory hardware handler */
static int (*memoryreadhandler[MH_HARDMAX])(int address);
static int memoryreadoffset[MH_HARDMAX];
static void (*memorywritehandler[MH_HARDMAX])(int address,int data);
static int memorywriteoffset[MH_HARDMAX];

/* current cpu current hardware element map point */
MHELE *cur_mrhard;
MHELE *cur_mwhard;
#ifdef POR_NEW
static MHELE *cur_prhard;
static MHELE *cur_pwhard;
#endif


#ifdef macintosh
#include "macmemory.c"
#endif


/***************************************************************************

  Memory handling

***************************************************************************/
int mrh_bank1(int address){return cpu_bankbase[1][address];}
int mrh_bank2(int address){return cpu_bankbase[2][address];}
int mrh_bank3(int address){return cpu_bankbase[3][address];}
int mrh_bank4(int address){return cpu_bankbase[4][address];}
int mrh_bank5(int address){return cpu_bankbase[5][address];}

int mrh_error(int address)
{
	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - read unmapped memory address %04x\n",cpu_getactivecpu(),cpu_getpc(),address);
	return RAM[address];
}
int mrh_nop(int address)
{
	return 0;
}

void mwh_bank1(int address,int data){cpu_bankbase[1][address] = data;}
void mwh_bank2(int address,int data){cpu_bankbase[2][address] = data;}
void mwh_bank3(int address,int data){cpu_bankbase[3][address] = data;}
void mwh_bank4(int address,int data){cpu_bankbase[4][address] = data;}
void mwh_bank5(int address,int data){cpu_bankbase[5][address] = data;}

void mwh_error(int address,int data)
{
	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to unmapped memory address %04x\n",cpu_getactivecpu(),cpu_getpc(),data,address);
	RAM[address] = data;
}
void mwh_rom(int address,int data)
{
	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to ROM address %04x\n",cpu_getactivecpu(),cpu_getpc(),data,address);
}
void mwh_ramrom(int address,int data)
{
	RAM[address] = ROM[address] = data;
}
void mwh_nop(int address,int data)
{
}

#ifdef MEM_DUMP
static void mem_dump( void )
{
	int cpu;
	int naddr,addr;
	MHELE nhw,hw;

	if (!errorlog) return;

	for( cpu = 0 ; cpu < totalcpu ; cpu++ )
	{
		fprintf(errorlog,"cpu %d read memory \n",cpu);
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
	fprintf(errorlog,"  %06x(%06x) - %06x = %02x\n",naddr,memoryreadoffset[nhw],addr-1,nhw);
				nhw = hw;
				naddr = addr;
			}
			addr++;
		}
		fprintf(errorlog,"  %06x(%06x) - %06x = %02x\n",naddr,memoryreadoffset[nhw],addr-1,nhw);

		fprintf(errorlog,"cpu %d write memory \n",cpu);
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
	fprintf(errorlog,"  %06x(%06x) - %06x = %02x\n",naddr,memorywriteoffset[nhw],addr-1,nhw);
				nhw = hw;
				naddr = addr;
			}
			addr++;
		}
	fprintf(errorlog,"  %06x(%06x) - %06x = %02x\n",naddr,memorywriteoffset[nhw],addr-1,nhw);
	}
}
#endif

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
		if (errorlog) fprintf(errorlog,"memory element size over \n");
		return 0;
	}
	/* get new element nunber */
	ele = *ele_max;
	(*ele_max)+=banks;
#ifdef MEM_DUMP
	if (errorlog) fprintf(errorlog,"create element %2d(%2d)\n",ele,banks);
#endif
	/* set link mark to current element */
	element[ad] = ele + MH_HARDMAX;
	/* get next subelement top */
	subelement  = &subelement[ele<<MH_SBITS];
	/* initialize new block */
	for( i = 0 ; i < (1<<MH_SBITS) ; i++ )
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
	if (errorlog) fprintf(errorlog,"set_element %6X-%6X = %2X\n",sp,ep,type);
#endif
	if( sp > ep ) return;
	do{
		mask  = mhmask[cpu][edepth];
		shift = mhshift[cpu][edepth];

		/* center element */
		ss = sp >> shift;
		sb = sp ? ((sp-1) >> shift) + 1 : 0;
		eb = ((ep+1) >> shift) - 1;
		ee = ep >> shift;

		if( sb <= eb )
		{
			if( (sb|mask)==(eb|mask) )
			{
				/* same reasion */
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

/* ASG 971007 added to allocate cur_m*_element() dynamic */
/* return = FALSE:can't allocate element memory */
int initmemoryhandlers(void)
{
	int i, cpu;
	const struct MemoryReadAddress *memoryread;
	const struct MemoryWriteAddress *memorywrite;
	const struct MemoryReadAddress *mra;
	const struct MemoryWriteAddress *mwa;
	int rdelement_max = 0;
	int wrelement_max = 0;
	int rdhard_max = HT_USER;
	int wrhard_max = HT_USER;
	MHELE hardware;
	int abits1,abits2,abits3,abitsmin;

	for( cpu = 0 ; cpu < MAX_CPU ; cpu++ )
		cur_mr_element[cpu] = cur_mw_element[cpu] = 0;

	for( cpu = 0 ; cpu < cpu_gettotalcpu() ; cpu++ )
	{
		const struct MemoryReadAddress *mra;
		const struct MemoryWriteAddress *mwa;


		ramptr[cpu] = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];

		/* opcode decryption is currently supported only for the first memory region */
		if (cpu == 0) romptr[cpu] = ROM;
		else romptr[cpu] = ramptr[cpu];


		/* initialize the memory base pointers for memory hooks */
		mra = Machine->drv->cpu[cpu].memory_read;
		while (mra->start != -1)
		{
			if (mra->base) *mra->base = &ramptr[cpu][mra->start];
			if (mra->size) *mra->size = mra->end - mra->start + 1;
			mra++;
		}
		mwa = Machine->drv->cpu[cpu].memory_write;
		while (mwa->start != -1)
		{
			if (mwa->base) *mwa->base = &ramptr[cpu][mwa->start];
			if (mwa->size) *mwa->size = mwa->end - mwa->start + 1;
			mwa++;
		}
	}

	/* initialize grobal handler */
	for( i = 0 ; i < MH_HARDMAX ; i++ ){
		memoryreadoffset[i] = 0;
		memorywriteoffset[i] = 0;
	}
	/* bank1 memory */
	memoryreadhandler[HT_BANK1] = mrh_bank1;
	memorywritehandler[HT_BANK1] = mwh_bank1;
	/* bank2 memory */
	memoryreadhandler[HT_BANK2] = mrh_bank2;
	memorywritehandler[HT_BANK2] = mwh_bank2;
	/* bank3 memory */
	memoryreadhandler[HT_BANK3] = mrh_bank3;
	memorywritehandler[HT_BANK3] = mwh_bank3;
	/* bank4 memory */
	memoryreadhandler[HT_BANK4] = mrh_bank4;
	memorywritehandler[HT_BANK4] = mwh_bank4;
	/* bank5 memory */
	memoryreadhandler[HT_BANK5] = mrh_bank5;
	memorywritehandler[HT_BANK5] = mwh_bank5;
	/* non map mamery */
	memoryreadhandler[HT_NON] = mrh_error;
	memorywritehandler[HT_NON] = mwh_error;
	/* NOP memory */
	memoryreadhandler[HT_NOP] = mrh_nop;
	memorywritehandler[HT_NOP] = mwh_nop;
	/* RAMROM memory */
	memorywritehandler[HT_RAMROM] = mwh_ramrom;
	/* ROM memory */
	memorywritehandler[HT_ROM] = mwh_rom;

	for( cpu = 0 ; cpu < cpu_gettotalcpu() ; cpu++ )
	{
		/* cpu selection */
		switch(Machine->drv->cpu[cpu].cpu_type & ~CPU_FLAGS_MASK){
		case CPU_I86:
			abits1 = ABITS1_20;
			abits2 = ABITS2_20;
			abits3 = ABITS3_20;
			abitsmin = ABITS_MIN_20;
			break;
		case CPU_M68000:
			abits1 = ABITS1_24;
			abits2 = ABITS2_24;
			abits3 = ABITS3_24;
			abitsmin = ABITS_MIN_24;
			break;
		default:
			abits1 = ABITS1_16;
			abits2 = ABITS2_16;
			abits3 = ABITS3_16;
			abitsmin = ABITS_MIN_16;
			break;
		}
		/* ASG 971005 -- changed to use the macros in memory.h */
		/* element shifter , mask set */
		mhshift[cpu][0] = (abits2+abits3);
		mhshift[cpu][1] = abits3;			/* 2nd */
		mhshift[cpu][2] = 0;				/* 3rd (used by set_element)*/
		mhmask[cpu][0]  = MHMASK(abits1);		/*1st(used by set_element)*/
		mhmask[cpu][1]  = MHMASK(abits2);		/*2nd*/
		mhmask[cpu][2]  = MHMASK(abits3);		/*3rd*/

		/* allocate current element */
		if( (cur_mr_element[cpu] = malloc(sizeof(MHELE)<<abits1)) == 0 )
		{
			shutdownmemoryhandler();
			return 0;
		}
		if( (cur_mw_element[cpu] = malloc(sizeof(MHELE)<<abits1)) == 0 )
		{
			shutdownmemoryhandler();
			return 0;
		}

		/* initialize curent element table */
		for( i = 0 ; i < (1<<abits1) ; i++ )
		{
			cur_mr_element[cpu][i] = HT_NON;	/* no map memory */
			cur_mw_element[cpu][i] = HT_NON;	/* no map memory */
		}

		memoryread = Machine->drv->cpu[cpu].memory_read;
		memorywrite = Machine->drv->cpu[cpu].memory_write;

		/* memory read handler build */
		mra = memoryread;
		while (mra->start != -1) mra++;
		mra--;

		while (mra >= memoryread)
		{
			int (*handler)() = mra->handler;
			switch( (int)handler )
			{
			case (int)MRA_RAM:
			case (int)MRA_ROM:
				hardware = HT_RAM;	/* sprcial case ram read */
				break;
			case (int)MRA_BANK1:
				hardware = HT_BANK1;
				cpu_bankoffset[1] = mra->start; /* ASG 971005 */
				cpu_bankbase[1] = &RAM[mra->start];	 /* ASG 971005 */
				break;
			case (int)MRA_BANK2:
				hardware = HT_BANK2;
				cpu_bankoffset[2] = mra->start; /* ASG 971005 */
				cpu_bankbase[2] = &RAM[mra->start];	 /* ASG 971005 */
				break;
			case (int)MRA_BANK3:
				hardware = HT_BANK3;
				cpu_bankoffset[3] = mra->start; /* ASG 971005 */
				cpu_bankbase[3] = &RAM[mra->start];	 /* ASG 971005 */
				break;
			case (int)MRA_BANK4:
				hardware = HT_BANK4;
				cpu_bankoffset[4] = mra->start; /* ASG 971005 */
				cpu_bankbase[4] = &RAM[mra->start];	 /* ASG 971005 */
				break;
			case (int)MRA_BANK5:
				hardware = HT_BANK5;
				cpu_bankoffset[5] = mra->start; /* ASG 971005 */
				cpu_bankbase[5] = &RAM[mra->start];	 /* ASG 971005 */
				break;
			case (int)MRA_NOP:
				hardware = HT_NOP;
				break;
			default:
				/* create newer haraware handler */
				if( rdhard_max == MH_HARDMAX )
				{
					if (errorlog)
					 fprintf(errorlog,"read memory hardware pattern over !\n");
					hardware = 0;
				}
				else
				{
					/* regist hardware function */
					hardware = rdhard_max++;
					memoryreadhandler[hardware] = handler;
					memoryreadoffset[hardware] = mra->start;
				}
			}
			/* hardware element table make */
			set_element( cpu , cur_mr_element[cpu] ,
				mra->start >> abitsmin , mra->end >> abitsmin ,
				hardware , readhardware , &rdelement_max );
			mra--;
		}

		/* memory write handler build */
		mwa = memorywrite;
		while (mwa->start != -1) mwa++;
		mwa--;

		while (mwa >= memorywrite)
		{
			void (*handler)() = mwa->handler;
			switch( (int)handler )
			{
			case (int)MWA_RAM:
				hardware = HT_RAM;	/* sprcial case ram write */
				break;
			case (int)MWA_BANK1:
				hardware = HT_BANK1;
				cpu_bankoffset[1] = mwa->start; /* ASG 971005 */
				cpu_bankbase[1] = &RAM[mwa->start];	 /* ASG 971005 */
				break;
			case (int)MWA_BANK2:
				hardware = HT_BANK2;
				cpu_bankoffset[2] = mwa->start; /* ASG 971005 */
				cpu_bankbase[2] = &RAM[mwa->start];	 /* ASG 971005 */
				break;
			case (int)MWA_BANK3:
				hardware = HT_BANK3;
				cpu_bankoffset[3] = mwa->start; /* ASG 971005 */
				cpu_bankbase[3] = &RAM[mwa->start];	 /* ASG 971005 */
				break;
			case (int)MWA_BANK4:
				hardware = HT_BANK4;
				cpu_bankoffset[4] = mwa->start; /* ASG 971005 */
				cpu_bankbase[4] = &RAM[mwa->start];	 /* ASG 971005 */
				break;
			case (int)MWA_BANK5:
				hardware = HT_BANK5;
				cpu_bankoffset[5] = mwa->start; /* ASG 971005 */
				cpu_bankbase[5] = &RAM[mwa->start];	 /* ASG 971005 */
				break;
			case (int)MWA_NOP:
				hardware = HT_NOP;
				break;
			case (int)MWA_RAMROM:
				hardware = HT_RAMROM;
				break;
			case (int)MWA_ROM:
				hardware = HT_ROM;
				break;
			default:
				/* create newer haraware handler */
				if( wrhard_max == MH_HARDMAX ){
					if (errorlog)
					 fprintf(errorlog,"write memory hardware pattern over !\n");
					hardware = 0;
				}else{
					/* regist hardware function */
					hardware = wrhard_max++;
					memorywritehandler[hardware] = handler;
					memorywriteoffset[hardware]  = mwa->start;
				}
			}
			/* hardware element table make */
			set_element( cpu , cur_mw_element[cpu] ,
				mwa->start >> abitsmin , mwa->end >> abitsmin ,
				hardware , (MHELE *)writehardware , &wrelement_max );
			mwa--;
		}
#ifdef PORT_NEW
		/* mapping for port mapper */
#endif
	}

	if (errorlog){
		fprintf(errorlog,"used read  elements %d/%d , functions %d/%d\n"
		    ,rdelement_max,MH_ELEMAX , rdhard_max,MH_HARDMAX );
		fprintf(errorlog,"used write elements %d/%d , functions %d/%d\n"
		    ,wrelement_max,MH_ELEMAX , wrhard_max,MH_HARDMAX );
	}
#ifdef MEM_DUMP
	mem_dump();
#endif
	return 1;	/* ok */
}

void memorycontextswap(int activecpu)
{
	RAM = ramptr[activecpu];
	ROM = romptr[activecpu];

	cur_mrhard = cur_mr_element[activecpu];
	cur_mwhard = cur_mw_element[activecpu];

	/* op code memory pointer */
	ophw = HT_RAM;
	OP_RAM = RAM;
	OP_ROM = ROM;
}

/* ASG 971007 */
void shutdownmemoryhandler(void)
{
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
	}
}

void updatememorybase(int activecpu)
{
	/* keep track of changes to RAM and ROM pointers (bank switching) */
	ramptr[activecpu] = RAM;
	romptr[activecpu] = ROM;
}



/***************************************************************************

  Perform a memory read. This function is called by the CPU emulation.

***************************************************************************/

/* ASG 971005 -- broke into three versions, using constant shift values */
int cpu_readmem16(int address)
{
	MHELE hw = cur_mrhard[address >> (ABITS2_16+ABITS_MIN_16)];

	if( !hw ) return RAM[address];
	if( hw >= MH_HARDMAX )
	{	/* 2nd element link */
		hw = readhardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((address>>ABITS_MIN_16) & MHMASK(ABITS2_16))];
		if( !hw ) return RAM[address];
	}
	return memoryreadhandler[hw](address - memoryreadoffset[hw]);
}

int cpu_readmem20(int address)
{
	MHELE hw = cur_mrhard[address >> (ABITS2_20+ABITS_MIN_20)];

	if( !hw ) return RAM[address];
	if( hw >= MH_HARDMAX )
	{	/* 2nd element link */
		hw = readhardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((address>>ABITS_MIN_20) & MHMASK(ABITS2_20))];
		if( !hw ) return RAM[address];
	}
	return memoryreadhandler[hw](address - memoryreadoffset[hw]);
}

int cpu_readmem24(int address)
{
	MHELE hw = cur_mrhard[address >> (ABITS2_24+ABITS_MIN_24)];

	if( !hw ) return RAM[address];
	if( hw >= MH_HARDMAX )
	{	/* 2nd element link */
		hw = readhardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((address>>ABITS_MIN_24) & MHMASK(ABITS2_24))];
		if( !hw ) return RAM[address];
	}
	return memoryreadhandler[hw](address - memoryreadoffset[hw]);
}

int cpu_readmem24_word(int address)
{
	MHELE hw = cur_mrhard[address >> (ABITS2_24+ABITS_MIN_24)];

	if( !hw )
	{
		unsigned short val = *(unsigned short *)&RAM[address];
		return BIG_WORD(val);
	}
	if( hw >= MH_HARDMAX )
	{	/* 2nd element link */
		hw = readhardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((address>>ABITS_MIN_24) & MHMASK(ABITS2_24))];
		if( !hw )
		{
			unsigned short val = *(unsigned short *)&RAM[address];
			return BIG_WORD(val);
		}
	}
	address -= memoryreadoffset[hw];
	return (memoryreadhandler[hw](address) << 8) +
			 (memoryreadhandler[hw](address + 1));
}

int cpu_readmem24_dword(int address)
{
	MHELE hw = cur_mrhard[address >> (ABITS2_24+ABITS_MIN_24)];

	if( !hw )
	{
		unsigned long val = *(unsigned long *)&RAM[address];
		return BIG_DWORD(val);
	}
	if( hw >= MH_HARDMAX )
	{	/* 2nd element link */
		hw = readhardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((address>>ABITS_MIN_24) & MHMASK(ABITS2_24))];
		if( !hw )
		{
			unsigned long val = *(unsigned long *)&RAM[address];
			return BIG_DWORD(val);
		}
	}
	address -= memoryreadoffset[hw];
	return (memoryreadhandler[hw](address) << 24) +
			 (memoryreadhandler[hw](address + 1) << 16) +
			 (memoryreadhandler[hw](address + 2) << 8) +
			 (memoryreadhandler[hw](address + 3));
}

/***************************************************************************

  Perform a memory write. This function is called by the CPU emulation.

***************************************************************************/

/* ASG 971005 -- broke into three versions, using constant shift values */
void cpu_writemem16(int address,int data)
{
	MHELE hw = cur_mwhard[address >> (ABITS2_16+ABITS_MIN_16)];

	if( !hw )
	{
		RAM[address] = data;
		return;
	}
	if( hw >= MH_HARDMAX )
	{	/* 2nd element link */
		hw = writehardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((address>>ABITS_MIN_16) & MHMASK(ABITS2_16))];
		if( !hw )
		{
			RAM[address] = data;
			return;
		}
	}
	memorywritehandler[hw](address - memorywriteoffset[hw],data);
}

void cpu_writemem20(int address,int data)
{
	MHELE hw = cur_mwhard[address >> (ABITS2_20+ABITS_MIN_20)];

	if( !hw )
	{
		RAM[address] = data;
		return;
	}
	if( hw >= MH_HARDMAX )
	{	/* 2nd element link */
		hw = writehardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((address>>ABITS_MIN_20) & MHMASK(ABITS2_20))];
		if( !hw )
		{
			RAM[address] = data;
			return;
		}
	}
	memorywritehandler[hw](address - memorywriteoffset[hw],data);
}

void cpu_writemem24(int address,int data)
{
	MHELE hw = cur_mwhard[address >> (ABITS2_24+ABITS_MIN_24)];

	if( !hw )
	{
		RAM[address] = data;
		return;
	}
	if( hw >= MH_HARDMAX )
	{	/* 2nd element link */
		hw = writehardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((address>>ABITS_MIN_24) & MHMASK(ABITS2_24))];
		if( !hw )
		{
			RAM[address] = data;
			return;
		}
	}
	memorywritehandler[hw](address - memorywriteoffset[hw],data);
}

void cpu_writemem24_word(int address,int data)
{
	MHELE hw = cur_mwhard[address >> (ABITS2_24+ABITS_MIN_24)];

	if( !hw )
	{
		data = BIG_WORD ((unsigned short)data);
		*(unsigned short *)&RAM[address] = data;
		return;
	}
	if( hw >= MH_HARDMAX )
	{	/* 2nd element link */
		hw = writehardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((address>>ABITS_MIN_24) & MHMASK(ABITS2_24))];
		if( !hw )
		{
			data = BIG_WORD ((unsigned short)data);
			*(unsigned short *)&RAM[address] = data;
			return;
		}
	}
	address -= memorywriteoffset[hw];
	memorywritehandler[hw](address,(data >> 8) & 0xff);
	memorywritehandler[hw](address+1,(data) & 0xff);
}

void cpu_writemem24_dword(int address,int data)
{
	MHELE hw = cur_mwhard[address >> (ABITS2_24+ABITS_MIN_24)];

	if( !hw )
	{
		data = BIG_DWORD ((unsigned long)data);
		*(unsigned long *)&RAM[address] = data;
		return;
	}
	if( hw >= MH_HARDMAX )
	{	/* 2nd element link */
		hw = writehardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((address>>ABITS_MIN_24) & MHMASK(ABITS2_24))];
		if( !hw )
		{
			data = BIG_DWORD ((unsigned long)data);
			*(unsigned long *)&RAM[address] = data;
			return;
		}
	}
	address -= memorywriteoffset[hw];
	memorywritehandler[hw](address,(data >> 24) & 0xff);
	memorywritehandler[hw](address+1,(data >> 16) & 0xff);
	memorywritehandler[hw](address+2,(data >> 8) & 0xff);
	memorywritehandler[hw](address+3,(data) & 0xff);
}


/***************************************************************************

  Perform an I/O port read. This function is called by the CPU emulation.

***************************************************************************/
int cpu_readport(int Port)
{
	const struct IOReadPort *iorp;


	Port &= 0xff;	/* we can't handle 16 bit adresses yet */

	iorp = Machine->drv->cpu[cpu_getactivecpu()].port_read;
	if (iorp)
	{
		while (iorp->start != -1)
		{
			if (Port >= iorp->start && Port <= iorp->end)
			{
				int (*handler)() = iorp->handler;


				if (handler == IORP_NOP) return 0;
				else return (*handler)(Port - iorp->start);
			}

			iorp++;
		}
	}

	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - read unmapped I/O port %02x\n",cpu_getactivecpu(),cpu_getpc(),Port);
	return 0;
}



/***************************************************************************

  Perform an I/O port write. This function is called by the CPU emulation.

***************************************************************************/
void cpu_writeport(int Port,int Value)
{
	const struct IOWritePort *iowp;


	Port &= 0xff;	/* we can't handle 16 bit adresses yet */

	iowp = Machine->drv->cpu[cpu_getactivecpu()].port_write;
	if (iowp)
	{
		while (iowp->start != -1)
		{
			if (Port >= iowp->start && Port <= iowp->end)
			{
				void (*handler)() = iowp->handler;


				if (handler == IOWP_NOP) return;
				else (*handler)(Port - iowp->start,Value);

				return;
			}

			iowp++;
		}
	}

	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to unmapped I/O port %02x\n",cpu_getactivecpu(),cpu_getpc(),Value,Port);
}

/* cpu change op-code memory base */
/* Need to called after CPU or PC changed (JP,JR,BRA,CALL,RET) */

/* ASG 971005 -- made 16-bit addressing-specific; hopefully I86 and 68000-based games won't need this */
void cpu_setOPbase16(int pc )
{
	MHELE hw = cur_mrhard[pc>>(ABITS2_16+ABITS_MIN_16)];

	/* change memory reasion */
	if( hw >= MH_HARDMAX )	/* 2nd element link */
		hw = readhardware[((hw-MH_HARDMAX)<<MH_SBITS) + ((pc>>ABITS_MIN_16)&MHMASK(ABITS2_16))];
	if( !(ophw = hw) ){ /* memory direct */
		OP_RAM = RAM;
		OP_ROM = ROM;
		return;
	}
	if( hw <= HT_BANK5 ){	/* banked memory select */
		OP_RAM = cpu_bankbase[hw];
		if( RAM == ROM ) OP_ROM = OP_RAM;
		return;
	}
	/* do not support on callbank memory reasion */
	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - op-code execute on mapped i/o\n",cpu_getactivecpu(),cpu_getpc());
}
