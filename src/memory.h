#ifndef MEMORY_H
#define MEMORY_H

/***************************************************************************

Note that the memory hooks are not passed the actual memory address where
the operation takes place, but the offset from the beginning of the block
they are assigned to. This makes handling of mirror addresses easier, and
makes the handlers a bit more "object oriented". If you handler needs to
read/write the main memory area, provide a "base" pointer: it will be
initialized by the main engine to point to the beginning of the memory block
assigned to the handler. You may also provided a pointer to "size": it
will be set to the length of the memory area processed by the handler.

***************************************************************************/
struct MemoryReadAddress
{
	int start,end;
	int (*handler)(int offset);   /* see special values below */
	unsigned char **base;         /* optional (see explanation above) */
	int *size;                    /* optional (see explanation above) */
};

#define MRA_NOP   0	              /* don't care, return 0 */
#define MRA_RAM   ((int(*)())-1)	  /* plain RAM location (return its contents) */
#define MRA_ROM   ((int(*)())-2)	  /* plain ROM location (return its contents) */
#define MRA_BANK1 ((int(*)())-10)  /* bank memory */
#define MRA_BANK2 ((int(*)())-11)  /* bank memory */
#define MRA_BANK3 ((int(*)())-12)  /* bank memory */
#define MRA_BANK4 ((int(*)())-13)  /* bank memory */
#define MRA_BANK5 ((int(*)())-14)  /* bank memory */

struct MemoryWriteAddress
{
	int start,end;
	void (*handler)(int offset,int data);	/* see special values below */
	unsigned char **base;	/* optional (see explanation above) */
	int *size;	/* optional (see explanation above) */
};

#define MWA_NOP 0	                  /* do nothing */
#define MWA_RAM ((void(*)())-1)	   /* plain RAM location (store the value) */
#define MWA_ROM ((void(*)())-2)	   /* plain ROM location (do nothing) */
/* RAM[] and ROM[] are usually the same, but they aren't if the CPU opcodes are */
/* encrypted. In such a case, opcodes are fetched from ROM[], and arguments from */
/* RAM[]. If the program dynamically creates code in RAM and executes it, it */
/* won't work unless writes to RAM affects both RAM[] and ROM[]. */
#define MWA_RAMROM ((void(*)())-3)	/* write to both the RAM[] and ROM[] array. */
#define MWA_BANK1 ((void(*)())-10)  /* bank memory */
#define MWA_BANK2 ((void(*)())-11)  /* bank memory */
#define MWA_BANK3 ((void(*)())-12)  /* bank memory */
#define MWA_BANK4 ((void(*)())-13)  /* bank memory */
#define MWA_BANK5 ((void(*)())-14)  /* bank memory */



/***************************************************************************

IN and OUT ports are handled like memory accesses, the hook template is the
same so you can interchange them. Of course there is no 'base' pointer for
IO ports.

***************************************************************************/
struct IOReadPort
{
	int start,end;
	int (*handler)(int offset);	/* see special values below */
};

#define IORP_NOP 0	/* don't care, return 0 */


struct IOWritePort
{
	int start,end;
	void (*handler)(int offset,int data);	/* see special values below */
};

#define IOWP_NOP 0	/* do nothing */


/* ASG 971005 -- moved into the header file */
/* memory element block size */
#define MH_SBITS    8			/* sub element bank size */
#define MH_PBITS    8			/* port   current element size */
#define MH_ELEMAX  64			/* sub elements       limit */
#define MH_HARDMAX 64			/* hardware functions limit */

/* ASG 971007 customize to elemet size period */
/* 24 bits address */
#define ABITS1_24    12
#define ABITS2_24    10
#define ABITS3_24     0
#define ABITS_MIN_24  2      /* minimum memory block is 4 bytes */
/* 20 bits address */
#define ABITS1_20    12
#define ABITS2_20     8
#define ABITS3_20     0
#define ABITS_MIN_20  0      /* minimum memory block is 1 byte */
/* 16 bits address */
#define ABITS1_16    12
#define ABITS2_16     4
#define ABITS3_16     0
#define ABITS_MIN_16  0      /* minimum memory block is 1 byte */
/* mask bits */
#define MHMASK(abits)    (0xffffffff>>(32-abits))

typedef unsigned char MHELE;

extern int cpu_mshift1;
extern MHELE *cur_mrhard;
extern MHELE *cur_mwhard;
extern MHELE curhw;

extern unsigned char *RAM;	/* pointer to the memory region of the active CPU */
extern unsigned char *ROM;
extern unsigned char *OP_RAM;	/* op_code used */
extern unsigned char *OP_ROM;	/* op_code used */

/* ----- memory setting subroutine ---- */
void cpu_setOPbase16(int pc );

/* ----- memory setup function ----- */
int initmemoryhandlers(void);
void shutdownmemoryhandler(void);

void memorycontextswap(int activecpu);
void updatememorybase(int activecpu);

/* ----- memory read /write function ----- */
int cpu_readmem16(int address);
int cpu_readmem20(int address);
int cpu_readmem24(int address);
int cpu_readmem24_word(int address);
int cpu_readmem24_dword(int address);
void cpu_writemem16(int address,int data);
void cpu_writemem20(int address,int data);
void cpu_writemem24(int address,int data);
void cpu_writemem24_word(int address,int data);
void cpu_writemem24_dword(int address,int data);

/* ----- port read / write function ----- */
int cpu_readport(int Port);
void cpu_writeport(int Port,int Value);

/* -----  bank memory function ----- */
#define cpu_setbank(B,A)  {cpu_bankbase[B]=(unsigned char *)(A)-cpu_bankoffset[B];if(ophw==B){ \
                               ophw=0xff;cpu_setOPbase16(cpu_getpc());}}
/* -----  op-code rompage select ----- */
#define cpu_setrombase(A) ROM=OP_ROM=(A)	/* ASG 971005 -- renamed to avoid confusion */

/* ----- op-code reasion set function ----- */
#define change_pc(pc) {if(cur_mrhard[pc>>ABITS2_16]!=ophw)cpu_setOPbase16(pc);}

/* bank memory functions */
extern MHELE ophw;
extern unsigned char *cpu_bankbase[];
extern int cpu_bankoffset[];	/* ASG 971005 */

#define cpu_readop(A) 		(OP_ROM[A])
#define cpu_readop_arg(A)	(OP_RAM[A])

#endif
