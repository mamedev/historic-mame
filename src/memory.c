/***************************************************************************

	memory.c

	Functions which handle the CPU memory access.

	Caveats:

	* If your driver executes an opcode which crosses a bank-switched
	boundary, it will pull the wrong data out of memory. Although not
	a common case, you may need to revert to memcpy to work around this.
	See machine/tnzs.c for an example.

	To do:

	- Add local banks for RAM/ROM to reduce pressure on banking
	- Always mirror everything out to 32 bits so we don't have to mask the address?
	- Add the ability to start with another memory map and modify it
	- Add fourth memory space for encrypted opcodes
	- Automatically mirror program space into data space if no data space
	- Get rid of association between memory_regions and RAM
	- Get rid of opcode/data separation by using address spaces?
	- Add support for internal addressing (maybe just accessors - see TMS3202x)
	- Fix debugger issues
	- Evaluate min/max opcode ranges and do we include a check in cpu_readop?

****************************************************************************

	Address map fields and restrictions:

	AM_RANGE(start, end)
		Specifies a range of consecutive addresses beginning with 'start' and
		ending with 'end' inclusive. An address hits in this bucket if the
		'address' >= 'start' and 'address' <= 'end'.

	AM_SPACE(match, mask)
		Specifies at the bit level (closer to real hardware) how to determine
		if an address matches a given bit pattern. An address hits in this
		bucket if 'address' & 'mask' == 'match'

	AM_MASK(mask)
		Specifies a mask for the addresses in the current bucket. This mask
		is applied after a positive hit in the bucket specified by AM_RANGE
		or AM_SPACE, and is computed before accessing the RAM or calling
		through to the read/write handler. If you use AM_SPACE, the mask
		is implicitly set equal to the logical NOT of the mask specified in
		the AM_SPACE macro. If you use AM_MIRROR, below, the mask is ANDed
		implicitly with the logical NOT of the mirror. The mask specified
		by this macro is ANDed against any implicit masks.

	AM_MIRROR(mirror)
		Specifies mirror addresses for the given bucket. The current bucket
		is mapped repeatedly according to the mirror mask, once where each
		mirror bit is 0, and once where it is 1. For example, a 'mirror'
		value of 0x14000 would map the bucket at 0x00000, 0x04000, 0x10000,
		and 0x14000.

	AM_READ(read)
		Specifies the read handler for this bucket. All reads will pass
		through the given callback handler. Special static values representing
		RAM, ROM, or BANKs are also allowed here.

	AM_WRITE(write)
		Specifies the write handler for this bucket. All writes will pass
		through the given callback handler. Special static values representing
		RAM, ROM, or BANKs are also allowed here.

	AM_REGION(region, offs)
		Only useful if AM_READ/WRITE point to RAM, ROM, or BANK memory. By
		default, memory is allocated to back each bucket. By specifying
		AM_REGION, you can tell the memory system to point the base of the
		memory backing this bucket to a given memory 'region' at the
		specified 'offs'.

	AM_SHARE(index)
		Similar to AM_REGION, this specifies that the memory backing the
		current bucket is shared with other buckets. The first bucket to
		specify the share 'index' will use its memory as backing for all
		future buckets that specify AM_SHARE with the same 'index'.

	AM_BASE(base)
		Specifies a pointer to a pointer to the base of the memory backing
		the current bucket.

	AM_SIZE(size)
		Specifies a pointer to a size_t variable which will be filled in
		with the size, in bytes, of the current bucket.

***************************************************************************/

#include "driver.h"
#include "osd_cpu.h"
#include "state.h"

#include <stdarg.h>


#define MEM_DUMP		(0)
#define VERBOSE			(0)


#if VERBOSE
#define VPRINTF(x)	printf x
#else
#define VPRINTF(x)
#endif



/***************************************************************************

	Basic theory of memory handling:

	An address with up to 32 bits is passed to a memory handler. First,
	an address mask is applied to the address, removing unused bits.

	Next, the address is broken into two halves, an upper half and a
	lower half. The number of bits in each half can be controlled via
	macros in memory.h, but they default to the upper 18 bits and the
	lower 14 bits. The upper half is then used as an index into the
	base_lookup table.

	If the value pulled from the table is within the range 192-255, then
	the lower half of the address is needed to resolve the final handler.
	The value from the table (192-255) is combined with the lower address
	bits to form an index into a subtable.

	Table values in the range 0-63 are reserved for internal handling
	(such as RAM, ROM, NOP, and banking). Table values between 64 and 192
	are assigned dynamically at startup.

***************************************************************************/

/* macros for the profiler */
#define MEMREADSTART()			do { profiler_mark(PROFILER_MEMREAD); } while (0)
#define MEMREADEND(ret)			do { profiler_mark(PROFILER_END); return ret; } while (0)
#define MEMWRITESTART()			do { profiler_mark(PROFILER_MEMWRITE); } while (0)
#define MEMWRITEEND(ret)		do { (ret); profiler_mark(PROFILER_END); return; } while (0)

/* helper macros */
#define HANDLER_IS_RAM(h)		((FPTR)(h) == STATIC_RAM)
#define HANDLER_IS_ROM(h)		((FPTR)(h) == STATIC_ROM)
#define HANDLER_IS_RAMROM(h)	((FPTR)(h) == STATIC_RAMROM)
#define HANDLER_IS_NOP(h)		((FPTR)(h) == STATIC_NOP)
#define HANDLER_IS_BANK(h)		((FPTR)(h) >= STATIC_BANK1 && (FPTR)(h) <= STATIC_BANKMAX)
#define HANDLER_IS_STATIC(h)	((FPTR)(h) < STATIC_COUNT)

#define HANDLER_TO_BANK(h)		((FPTR)(h))
#define BANK_TO_HANDLER(b)		((void *)(b))

#define SPACE_SHIFT(s,a)		(((s)->ashift < 0) ? ((a) << -(s)->ashift) : ((a) >> (s)->ashift))
#define SPACE_SHIFT_END(s,a)	(((s)->ashift < 0) ? (((a) << -(s)->ashift) | ((1 << -(s)->ashift) - 1)) : ((a) >> (s)->ashift))
#define INV_SPACE_SHIFT(s,a)	(((s)->ashift < 0) ? ((a) >> -(s)->ashift) : ((a) << (s)->ashift))

#define SUBTABLE_PTR(tabledata, entry) (&(tabledata)->table[(1 << LEVEL1_BITS) + (((entry) - SUBTABLE_BASE) << LEVEL2_BITS)])


/*-------------------------------------------------
	TYPE DEFINITIONS
-------------------------------------------------*/

struct memory_block_t
{
	UINT8					cpunum;					/* which CPU are we associated with? */
	UINT8					spacenum;				/* which address space are we associated with? */
	UINT8					isallocated;			/* did we allocate this ourselves? */
	offs_t 					start, end;				/* start/end or match/mask for verifying a match */
    UINT8 *					data;					/* pointer to the data for this block */
};

struct bank_data_t
{
	UINT8 					used;					/* is this bank used? */
	UINT8 					dynamic;				/* is this bank allocated dynamically? */
	UINT8 					cpunum;					/* the CPU it is used for */
	UINT8 					spacenum;				/* the address space it is used for */
	offs_t 					base;					/* the base offset */
};

union rwhandlers_t
{
	void *					generic;				/* generic handler void */
	union read_handlers_t	read;					/* read handlers */
	union write_handlers_t	write;					/* write handlers */
};

struct handler_data_t
{
	union rwhandlers_t		handler;				/* function pointer for handler */
	offs_t					offset;					/* base offset for handler */
	offs_t					top;					/* maximum offset for handler */
	offs_t					mask;					/* mask against the final address */
};

struct subtable_data_t
{
	UINT8					checksum_valid;			/* is the checksum valid */
	UINT32					checksum;				/* checksum over all the bytes */
	UINT32					usecount;				/* number of times this has been used */
};

struct table_data_t
{
	UINT8 *					table;					/* pointer to base of table */
	UINT8 					subtable_alloc;			/* number of subtables allocated */
	struct subtable_data_t	subtable[SUBTABLE_COUNT]; /* info about each subtable */
	struct handler_data_t	handlers[ENTRY_COUNT];	/* array of user-installed handlers */
};

struct addrspace_data_t
{
	UINT8					cpunum;					/* CPU index */
	UINT8					spacenum;				/* address space index */
	INT8					ashift;					/* address shift */
	UINT8					abits;					/* address bits */
	UINT8 					dbits;					/* data bits */
	offs_t					rawmask;				/* raw address mask, before adjusting to bytes */
	offs_t					mask;					/* address mask */
	data64_t				unmap;					/* unmapped value */
	struct table_data_t		read;					/* memory read lookup table */
	struct table_data_t		write;					/* memory write lookup table */
	struct data_accessors_t *accessors;				/* pointer to the memory accessors */
	struct address_map_t *	map;					/* original memory map */
	struct address_map_t *	adjmap;					/* adjusted memory map */
};

struct cpu_data_t
{
	void *					rambase;				/* RAM base pointer */
	size_t					ramlength;				/* RAM length pointer */
	opbase_handler 			opbase;					/* opcode base handler */

	void *					op_ram;					/* dynamic RAM base pointer */
	void *					op_rom;					/* dynamic ROM base pointer */
	offs_t					op_mask;				/* dynamic ROM address mask */
	offs_t					op_mem_min;				/* dynamic ROM/RAM min */
	offs_t					op_mem_max;				/* dynamic ROM/RAM max */
	UINT8		 			opcode_entry;			/* opcode base handler */

	UINT8					spacemask;				/* mask of which address spaces are used */
	struct addrspace_data_t space[ADDRESS_SPACES];	/* info about each address space */
};


/*-------------------------------------------------
	GLOBAL VARIABLES
-------------------------------------------------*/

UINT8 *						opcode_base;					/* opcode base */
UINT8 *						opcode_arg_base;				/* opcode argument base */
offs_t						opcode_mask;					/* mask to apply to the opcode address */
offs_t						opcode_memory_min;				/* opcode memory minimum */
offs_t						opcode_memory_max;				/* opcode memory maximum */
UINT8		 				opcode_entry;					/* opcode readmem entry */

struct address_space_t		address_space[ADDRESS_SPACES];	/* address space data */

static UINT8 *				bank_ptr[MAX_BANKS + 1];		/* array of bank pointers */
static void *				shared_ptr[MAX_SHARED_POINTERS];/* array of shared pointers */

static struct memory_block_t memory_block[MAX_MEMORY_BLOCKS];/* array of memory blocks we are tracking */
static int 					memory_block_count = 0;			/* number of memory_block[] entries used */

static int					cur_context;					/* current CPU context */

static opbase_handler		opbasefunc;						/* opcode base override */

static struct cpu_data_t	cpudata[MAX_CPU];				/* data gathered for each CPU */
static struct bank_data_t 	bankdata[MAX_BANKS + 1];		/* data gathered for each bank */

static struct data_accessors_t memory_accessors[ADDRESS_SPACES][4][2] =
{
	/* program accessors */
	{
		{
			{ program_read_byte_8, NULL, NULL, NULL, program_write_byte_8, NULL, NULL, NULL },
			{ program_read_byte_8, NULL, NULL, NULL, program_write_byte_8, NULL, NULL, NULL }
		},
		{
			{ program_read_byte_16le, program_read_word_16le, NULL, NULL, program_write_byte_16le, program_write_word_16le, NULL, NULL },
			{ program_read_byte_16be, program_read_word_16be, NULL, NULL, program_write_byte_16be, program_write_word_16be, NULL, NULL }
		},
		{
			{ program_read_byte_32le, program_read_word_32le, program_read_dword_32le, NULL, program_write_byte_32le, program_write_word_32le, program_write_dword_32le, NULL },
			{ program_read_byte_32be, program_read_word_32be, program_read_dword_32be, NULL, program_write_byte_32be, program_write_word_32be, program_write_dword_32be, NULL }
		},
		{
			{ program_read_byte_64le, program_read_word_64le, program_read_dword_64le, program_read_qword_64le, program_write_byte_64le, program_write_word_64le, program_write_dword_64le, program_write_qword_64le },
			{ program_read_byte_64be, program_read_word_64be, program_read_dword_64be, program_read_qword_64be, program_write_byte_64be, program_write_word_64be, program_write_dword_64be, program_write_qword_64be }
		}
	},

	/* data accessors */
	{
		{
			{ data_read_byte_8, NULL, NULL, NULL, data_write_byte_8, NULL, NULL, NULL },
			{ data_read_byte_8, NULL, NULL, NULL, data_write_byte_8, NULL, NULL, NULL }
		},
		{
			{ data_read_byte_16le, data_read_word_16le, NULL, NULL, data_write_byte_16le, data_write_word_16le, NULL, NULL },
			{ data_read_byte_16be, data_read_word_16be, NULL, NULL, data_write_byte_16be, data_write_word_16be, NULL, NULL }
		},
		{
			{ data_read_byte_32le, data_read_word_32le, data_read_dword_32le, NULL, data_write_byte_32le, data_write_word_32le, data_write_dword_32le, NULL },
			{ data_read_byte_32be, data_read_word_32be, data_read_dword_32be, NULL, data_write_byte_32be, data_write_word_32be, data_write_dword_32be, NULL }
		},
		{
			{ data_read_byte_64le, data_read_word_64le, data_read_dword_64le, data_read_qword_64le, data_write_byte_64le, data_write_word_64le, data_write_dword_64le, data_write_qword_64le },
			{ data_read_byte_64be, data_read_word_64be, data_read_dword_64be, data_read_qword_64be, data_write_byte_64be, data_write_word_64be, data_write_dword_64be, data_write_qword_64be }
		}
	},

	/* I/O accessors */
	{
		{
			{ io_read_byte_8, NULL, NULL, NULL, io_write_byte_8, NULL, NULL, NULL },
			{ io_read_byte_8, NULL, NULL, NULL, io_write_byte_8, NULL, NULL, NULL }
		},
		{
			{ io_read_byte_16le, io_read_word_16le, NULL, NULL, io_write_byte_16le, io_write_word_16le, NULL, NULL },
			{ io_read_byte_16be, io_read_word_16be, NULL, NULL, io_write_byte_16be, io_write_word_16be, NULL, NULL }
		},
		{
			{ io_read_byte_32le, io_read_word_32le, io_read_dword_32le, NULL, io_write_byte_32le, io_write_word_32le, io_write_dword_32le, NULL },
			{ io_read_byte_32be, io_read_word_32be, io_read_dword_32be, NULL, io_write_byte_32be, io_write_word_32be, io_write_dword_32be, NULL }
		},
		{
			{ io_read_byte_64le, io_read_word_64le, io_read_dword_64le, io_read_qword_64le, io_write_byte_64le, io_write_word_64le, io_write_dword_64le, io_write_qword_64le },
			{ io_read_byte_64be, io_read_word_64be, io_read_dword_64be, io_read_qword_64be, io_write_byte_64be, io_write_word_64be, io_write_dword_64be, io_write_qword_64be }
		}
	},
};



/*-------------------------------------------------
	PROTOTYPES
-------------------------------------------------*/

static int init_cpudata(void);
static int init_addrspace(UINT8 cpunum, UINT8 spacenum);
static int preflight_memory(void);
static int populate_memory(void);
static void install_mem_handler(struct addrspace_data_t *space, int iswrite, int databits, int ismatchmask, offs_t start, offs_t end, offs_t mask, offs_t mirror, void *handler, int isfixed);
static void *assign_dynamic_bank(int cpunum, int spacenum, offs_t start, offs_t mirror, int isfixed);
static UINT8 get_handler_index(struct handler_data_t *table, void *handler, offs_t start, offs_t end, offs_t mask);
static void populate_table_range(struct addrspace_data_t *space, int iswrite, offs_t start, offs_t stop, UINT8 handler);
static void populate_table_match(struct addrspace_data_t *space, int iswrite, offs_t matchval, offs_t matchmask, UINT8 handler);
static UINT8 allocate_subtable(struct table_data_t *tabledata);
static int merge_subtables(struct table_data_t *tabledata);
static void release_subtable(struct table_data_t *tabledata, UINT8 subentry);
static UINT8 *open_subtable(struct table_data_t *tabledata, offs_t l1index);
static void close_subtable(struct table_data_t *tabledata, offs_t l1index);
static int allocate_memory(void);
static void *allocate_memory_block(int cpunum, int spacenum, offs_t start, offs_t end, void *memory);
static void register_for_save(int cpunum, int spacenum, offs_t start, void *base, size_t numbytes);
static struct address_map_t *assign_intersecting_blocks(struct addrspace_data_t *space, offs_t start, offs_t end, UINT8 *base);
static int find_memory(void);
static void *memory_find_base(int cpunum, int spacenum, int readwrite, offs_t offset);
static void *get_static_handler(int databits, int readorwrite, int spacenum, int which);

#if (MEM_DUMP)
static void mem_dump(void);
#else
#define mem_dump()
#endif



/*-------------------------------------------------
	memory_init - initialize the memory system
-------------------------------------------------*/

int memory_init(void)
{
	/* no current context to start */
	cur_context = -1;

	/* reset the shared pointers and bank pointers */
	memset(shared_ptr, 0, sizeof(shared_ptr));
	memset(bank_ptr, 0, sizeof(bank_ptr));

	/* reset our hardcoded and allocated pointer tracking */
	memset(memory_block, 0, sizeof(memory_block));
	memory_block_count = 0;

	/* init the CPUs */
	if (!init_cpudata())
		return 0;

	/* preflight the memory handlers and check banks */
	if (!preflight_memory())
		return 0;

	/* then fill in the tables */
	if (!populate_memory())
		return 0;

	/* allocate any necessary memory */
	if (!allocate_memory())
		return 0;

	/* find all the allocated pointers */
	if (!find_memory())
		return 0;

	/* dump the final memory configuration */
	mem_dump();
	return 1;
}


/*-------------------------------------------------
	memory_shutdown - free memory
-------------------------------------------------*/

void memory_shutdown(void)
{
	int cpunum, spacenum, blocknum;
	struct memory_block_t *block;

	/* free all the tables */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
		{
			if (cpudata[cpunum].space[spacenum].read.table)
				free(cpudata[cpunum].space[spacenum].read.table);
			if (cpudata[cpunum].space[spacenum].write.table)
				free(cpudata[cpunum].space[spacenum].write.table);
			if (cpudata[cpunum].space[spacenum].map)
				free(cpudata[cpunum].space[spacenum].map);
		}
	memset(&cpudata, 0, sizeof(cpudata));

	/* free all the allocated memory */
	for (blocknum = 0, block = memory_block; blocknum < memory_block_count; blocknum++, block++)
		if (block->isallocated)
			free(block->data);
}


/*-------------------------------------------------
	memory_set_context - set the memory context
-------------------------------------------------*/

void memory_set_context(int activecpu)
{
	/* remember dynamic RAM/ROM */
	if (cur_context != -1)
	{
		cpudata[cur_context].op_ram = opcode_arg_base;
		cpudata[cur_context].op_rom = opcode_base;
		cpudata[cur_context].op_mask = opcode_mask;
		cpudata[cur_context].op_mem_min = opcode_memory_min;
		cpudata[cur_context].op_mem_max = opcode_memory_max;
		cpudata[cur_context].opcode_entry = opcode_entry;
	}
	cur_context = activecpu;

	bank_ptr[STATIC_RAM] = cpudata[activecpu].rambase;
	opcode_arg_base = cpudata[activecpu].op_ram;
	opcode_base = cpudata[activecpu].op_rom;
	opcode_mask = cpudata[activecpu].op_mask;
	opcode_memory_min = cpudata[activecpu].op_mem_min;
	opcode_memory_max = cpudata[activecpu].op_mem_max;
	opcode_entry = cpudata[activecpu].opcode_entry;

	/* program address space */
	address_space[ADDRESS_SPACE_PROGRAM].addrmask = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].mask;
	address_space[ADDRESS_SPACE_PROGRAM].readlookup = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].read.table;
	address_space[ADDRESS_SPACE_PROGRAM].writelookup = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].write.table;
	address_space[ADDRESS_SPACE_PROGRAM].readhandlers = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].read.handlers;
	address_space[ADDRESS_SPACE_PROGRAM].writehandlers = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].write.handlers;
	address_space[ADDRESS_SPACE_PROGRAM].accessors = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].accessors;

	/* data address space */
	if (cpudata[activecpu].spacemask & (1 << ADDRESS_SPACE_DATA))
	{
		address_space[ADDRESS_SPACE_DATA].addrmask = cpudata[activecpu].space[ADDRESS_SPACE_DATA].mask;
		address_space[ADDRESS_SPACE_DATA].readlookup = cpudata[activecpu].space[ADDRESS_SPACE_DATA].read.table;
		address_space[ADDRESS_SPACE_DATA].writelookup = cpudata[activecpu].space[ADDRESS_SPACE_DATA].write.table;
		address_space[ADDRESS_SPACE_DATA].readhandlers = cpudata[activecpu].space[ADDRESS_SPACE_DATA].read.handlers;
		address_space[ADDRESS_SPACE_DATA].writehandlers = cpudata[activecpu].space[ADDRESS_SPACE_DATA].write.handlers;
		address_space[ADDRESS_SPACE_DATA].accessors = cpudata[activecpu].space[ADDRESS_SPACE_DATA].accessors;
	}

	/* I/O address space */
	if (cpudata[activecpu].spacemask & (1 << ADDRESS_SPACE_IO))
	{
		address_space[ADDRESS_SPACE_IO].addrmask = cpudata[activecpu].space[ADDRESS_SPACE_IO].mask;
		address_space[ADDRESS_SPACE_IO].readlookup = cpudata[activecpu].space[ADDRESS_SPACE_IO].read.table;
		address_space[ADDRESS_SPACE_IO].writelookup = cpudata[activecpu].space[ADDRESS_SPACE_IO].write.table;
		address_space[ADDRESS_SPACE_IO].readhandlers = cpudata[activecpu].space[ADDRESS_SPACE_IO].read.handlers;
		address_space[ADDRESS_SPACE_IO].writehandlers = cpudata[activecpu].space[ADDRESS_SPACE_IO].write.handlers;
		address_space[ADDRESS_SPACE_IO].accessors = cpudata[activecpu].space[ADDRESS_SPACE_IO].accessors;
	}

	opbasefunc = cpudata[activecpu].opbase;
}


/*-------------------------------------------------
	memory_get_map - return a pointer to a CPU's
	memory map
-------------------------------------------------*/

const struct address_map_t *memory_get_map(int cpunum, int spacenum)
{
	return cpudata[cpunum].space[spacenum].map;
}


/*-------------------------------------------------
	memory_set_opbase_handler - change op-code
	memory base
-------------------------------------------------*/

opbase_handler memory_set_opbase_handler(int cpunum, opbase_handler function)
{
	opbase_handler old = cpudata[cpunum].opbase;
	cpudata[cpunum].opbase = function;
	if (cpunum == cpu_getactivecpu())
		opbasefunc = function;
	return old;
}


/*-------------------------------------------------
	memory_set_opbase - generic opcode base changer
-------------------------------------------------*/

void memory_set_opbase(offs_t pc)
{
	UINT8 *base;
	UINT8 entry;

	/* allow overrides */
	if (opbasefunc)
	{
		pc = (*opbasefunc)(pc);
		if (pc == ~0)
			return;
	}

	/* perform the lookup */
	pc &= address_space[ADDRESS_SPACE_PROGRAM].addrmask;
	entry = address_space[ADDRESS_SPACE_PROGRAM].readlookup[LEVEL1_INDEX(pc)];
	if (entry >= SUBTABLE_BASE)
		entry = address_space[ADDRESS_SPACE_PROGRAM].readlookup[LEVEL2_INDEX(entry,pc)];
	opcode_entry = entry;

	/* RAM/ROM/RAMROM */
	if (entry >= STATIC_RAM && entry <= STATIC_RAMROM)
		base = bank_ptr[STATIC_RAM];

	/* banked memory */
	else if (entry >= STATIC_BANK1 && entry <= STATIC_RAM)
		base = bank_ptr[entry];

	/* other memory -- could be very slow! */
	else
	{
		logerror("cpu #%d (PC=%08X): warning - op-code execute on mapped I/O\n",
					cpu_getactivecpu(), activecpu_get_pc());
		return;
	}

	/* compute the adjusted base */
	opcode_mask = address_space[ADDRESS_SPACE_PROGRAM].readhandlers[entry].mask;
	opcode_base = base - (address_space[ADDRESS_SPACE_PROGRAM].readhandlers[entry].offset & opcode_mask) + (opcode_base - opcode_arg_base);
	opcode_arg_base = base - (address_space[ADDRESS_SPACE_PROGRAM].readhandlers[entry].offset & opcode_mask);
	opcode_memory_min = address_space[ADDRESS_SPACE_PROGRAM].readhandlers[entry].offset;
	opcode_memory_max = (entry >= STATIC_RAM && entry <= STATIC_RAMROM)
		? cpudata[cpu_getactivecpu()].ramlength - 1
		: address_space[ADDRESS_SPACE_PROGRAM].readhandlers[entry].top;
}


/*-------------------------------------------------
	memory_set_opcode_base - set the base of
	ROM
-------------------------------------------------*/

void memory_set_opcode_base(int cpunum, void *base)
{
	if (cur_context == cpunum)
	{
		opcode_base = base;
		opcode_memory_min = (offs_t) 0x00000000;
		opcode_memory_max = (offs_t) 0x7fffffff;
	}
	else
	{
		cpudata[cpunum].op_rom = base;
		cpudata[cpunum].op_mem_min = (offs_t) 0x00000000;
		cpudata[cpunum].op_mem_max = (offs_t) 0x7fffffff;
	}
}


/*-------------------------------------------------
	memory_get_read_ptr - return a pointer to the
	base of RAM associated with the given CPU
	and offset
-------------------------------------------------*/

void *memory_get_read_ptr(int cpunum, int spacenum, offs_t offset)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	UINT8 entry;

	/* perform the lookup */
	offset &= space->mask;
	entry = space->read.table[LEVEL1_INDEX(offset)];
	if (entry >= SUBTABLE_BASE)
		entry = space->read.table[LEVEL2_INDEX(entry, offset)];

	/* 8-bit case: RAM/ROM/RAMROM */
	if (entry > STATIC_RAM)
		return NULL;
	offset = (offset - space->read.handlers[entry].offset) & space->read.handlers[entry].mask;
	return &bank_ptr[entry][offset];
}


/*-------------------------------------------------
	memory_get_write_ptr - return a pointer to the
	base of RAM associated with the given CPU
	and offset
-------------------------------------------------*/

void *memory_get_write_ptr(int cpunum, int spacenum, offs_t offset)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	UINT8 entry;

	/* perform the lookup */
	offset &= space->mask;
	entry = space->write.table[LEVEL1_INDEX(offset)];
	if (entry >= SUBTABLE_BASE)
		entry = space->write.table[LEVEL2_INDEX(entry, offset)];

	/* 8-bit case: RAM/ROM/RAMROM */
	if (entry > STATIC_RAM)
		return NULL;
	offset = (offset - space->write.handlers[entry].offset) & space->write.handlers[entry].mask;
	return &bank_ptr[entry][offset];
}


/*-------------------------------------------------
	memory_set_bankptr - set the base of a bank
-------------------------------------------------*/

void memory_set_bankptr(int banknum, void *base)
{
	/* validation checks */
	if (banknum < STATIC_BANK1 || banknum > MAX_EXPLICIT_BANKS || !bankdata[banknum].used)
		osd_die("memory_set_bankptr called with invalid bank %d\n", banknum);
	if (bankdata[banknum].dynamic)
		osd_die("memory_set_bankptr called with dynamic bank %d\n", banknum);

	/* set the base */
	bank_ptr[banknum] = base;

	/* if we're executing out of this bank, adjust the opbase pointer */
	if (opcode_entry == banknum && cpu_getactivecpu() >= 0)
	{
		opcode_entry = 0xff;
		memory_set_opbase(activecpu_get_pc_byte());
	}
}


/*-------------------------------------------------
	memory_install_readX_handler - install dynamic
	read handler for X-bit case
-------------------------------------------------*/

data8_t *memory_install_read8_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, read8_handler handler)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, 0, 8, 0, start, end, mask, 0, (void *)handler, 0);
	mem_dump();
	return memory_find_base(cpunum, spacenum, 0, SPACE_SHIFT(space, start));
}

data16_t *memory_install_read16_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, read16_handler handler)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, 0, 16, 0, start, end, mask, 0, (void *)handler, 0);
	mem_dump();
	return memory_find_base(cpunum, spacenum, 0, SPACE_SHIFT(space, start));
}

data32_t *memory_install_read32_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, read32_handler handler)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, 0, 32, 0, start, end, mask, 0, (void *)handler, 0);
	mem_dump();
	return memory_find_base(cpunum, spacenum, 0, SPACE_SHIFT(space, start));
}

data64_t *memory_install_read64_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, read64_handler handler)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, 0, 64, 0, start, end, mask, 0, (void *)handler, 0);
	mem_dump();
	return memory_find_base(cpunum, spacenum, 0, SPACE_SHIFT(space, start));
}


/*-------------------------------------------------
	memory_install_writeX_handler - install dynamic
	write handler for X-bit case
-------------------------------------------------*/

data8_t *memory_install_write8_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, write8_handler handler)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, 1, 8, 0, start, end, mask, 0, (void *)handler, 0);
	mem_dump();
	return memory_find_base(cpunum, spacenum, 1, SPACE_SHIFT(space, start));
}

data16_t *memory_install_write16_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, write16_handler handler)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, 1, 16, 0, start, end, mask, 0, (void *)handler, 0);
	mem_dump();
	return memory_find_base(cpunum, spacenum, 1, SPACE_SHIFT(space, start));
}

data32_t *memory_install_write32_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, write32_handler handler)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, 1, 32, 0, start, end, mask, 0, (void *)handler, 0);
	mem_dump();
	return memory_find_base(cpunum, spacenum, 1, SPACE_SHIFT(space, start));
}

data64_t *memory_install_write64_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, write64_handler handler)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, 1, 64, 0, start, end, mask, 0, (void *)handler, 0);
	mem_dump();
	return memory_find_base(cpunum, spacenum, 1, SPACE_SHIFT(space, start));
}


/*-------------------------------------------------
	construct_map_0 - NULL memory map
-------------------------------------------------*/

struct address_map_t *construct_map_0(struct address_map_t *map)
{
	map->flags = AM_FLAGS_END;
	return map;
}


/*-------------------------------------------------
	init_cpudata - initialize the cpudata
	structure for each CPU
-------------------------------------------------*/

static int init_cpudata(void)
{
	int cpunum, spacenum;

	/* zap the cpudata structure */
	memset(&cpudata, 0, sizeof(cpudata));

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		int cputype = Machine->drv->cpu[cpunum].cpu_type;

		/* set the RAM/ROM base */
		cpudata[cpunum].rambase = cpudata[cpunum].op_ram = cpudata[cpunum].op_rom = memory_region(REGION_CPU1 + cpunum);
		cpudata[cpunum].op_mem_max = cpudata[cpunum].ramlength = memory_region_length(REGION_CPU1 + cpunum);
		cpudata[cpunum].op_mem_min = 0;
		cpudata[cpunum].opcode_entry = STATIC_ROM;
		cpudata[cpunum].opbase = NULL;

		/* TODO: make this dynamic */
		cpudata[cpunum].spacemask = 0;
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (!init_addrspace(cpunum, spacenum))
				return 0;
		cpudata[cpunum].op_mask = cpudata[cpunum].space[ADDRESS_SPACE_PROGRAM].mask;

		/* Z80 port mask kludge */
		if (cputype == CPU_Z80 || cputype == CPU_Z180)
			if (!(Machine->drv->cpu[cpunum].cpu_flags & CPU_16BIT_PORT))
				cpudata[cpunum].space[ADDRESS_SPACE_IO].mask = 0xff;
	}
	return 1;
}


/*-------------------------------------------------
	adjust_addresses - adjust addresses for a
	given address space in a standard fashion
-------------------------------------------------*/

INLINE void adjust_addresses(struct addrspace_data_t *space, int ismatchmask, offs_t *start, offs_t *end, offs_t *mask, offs_t *mirror)
{
	/* adjust start/end/mask values */
	if (!*mask) *mask = space->rawmask;
	if (ismatchmask) *mask |= *end & space->rawmask;
	*mask &= ~*mirror;
	*start &= ~*mirror & space->rawmask;
	*end &= ~*mirror & space->rawmask;

	/* adjust to byte values */
	*mask = SPACE_SHIFT(space, *mask);
	*start = SPACE_SHIFT(space, *start);
	*end = ismatchmask ? SPACE_SHIFT(space, *end) : SPACE_SHIFT_END(space, *end);
	*mirror = SPACE_SHIFT(space, *mirror);
}


/*-------------------------------------------------
	init_addrspace - initialize the address space
	data structure
-------------------------------------------------*/

static int init_addrspace(UINT8 cpunum, UINT8 spacenum)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	int cputype = Machine->drv->cpu[cpunum].cpu_type;
	int abits = cputype_addrbus_width(cputype, spacenum);
	int dbits = cputype_databus_width(cputype, spacenum);
	int accessorindex = (dbits == 8) ? 0 : (dbits == 16) ? 1 : (dbits == 32) ? 2 : 3;
	construct_map_t internal_map = (construct_map_t)cputype_get_info_ptr(cputype, CPUINFO_PTR_INTERNAL_MEMORY_MAP + spacenum);
	int entrynum;

	/* determine the address and data bits */
	space->cpunum = cpunum;
	space->spacenum = spacenum;
	space->ashift = cputype_addrbus_shift(cputype, spacenum);
	space->abits = abits - space->ashift;
	space->dbits = dbits;
	space->rawmask = 0xffffffffUL >> (32 - abits);
	space->mask = SPACE_SHIFT_END(space, space->rawmask);
	space->accessors = &memory_accessors[spacenum][accessorindex][cputype_endianness(cputype) == CPU_IS_LE ? 0 : 1];
	space->map = NULL;
	space->adjmap = NULL;

	/* if there's nothing here, just punt */
	if (space->abits == 0)
		return 1;
	cpudata[cpunum].spacemask |= 1 << spacenum;

	/* construct the combined memory map */
	if (internal_map || Machine->drv->cpu[cpunum].construct_map[spacenum][0] || Machine->drv->cpu[cpunum].construct_map[spacenum][1])
	{
		/* allocate and clear memory for 2 copies of the map */
		struct address_map_t *map = malloc(sizeof(space->map[0]) * MAX_ADDRESS_MAP_SIZE * 4);
		if (!map)
		{
			osd_die("cpu #%d couldn't allocate memory map\n", cpunum);
			return -1;
		}
		memset(map, 0, sizeof(space->map[0]) * MAX_ADDRESS_MAP_SIZE * 4);

		/* make pointers to the standard and adjusted maps */
		space->map = map;
		space->adjmap = &map[MAX_ADDRESS_MAP_SIZE * 2];
		
		/* start by constructing the internal CPU map */
		if (internal_map)
			map = (*internal_map)(map);

		/* construct the standard map */
		if (Machine->drv->cpu[cpunum].construct_map[spacenum][0])
			map = (*Machine->drv->cpu[cpunum].construct_map[spacenum][0])(map);
		if (Machine->drv->cpu[cpunum].construct_map[spacenum][1])
			map = (*Machine->drv->cpu[cpunum].construct_map[spacenum][1])(map);

		/* make the adjusted map */
		memcpy(space->adjmap, space->map, sizeof(space->map[0]) * MAX_ADDRESS_MAP_SIZE * 2);
		for (map = space->adjmap; !IS_AMENTRY_END(map); map++)
			if (!IS_AMENTRY_EXTENDED(map))
				adjust_addresses(space, IS_AMENTRY_MATCH_MASK(map), &map->start, &map->end, &map->mask, &map->mirror);
	}

	/* init the static handlers */
	memset(space->read.handlers, 0, sizeof(space->read.handlers));
	memset(space->write.handlers, 0, sizeof(space->write.handlers));
	for (entrynum = 0; entrynum < ENTRY_COUNT; entrynum++)
	{
		space->read.handlers[entrynum].handler.generic = get_static_handler(dbits, 0, spacenum, entrynum);
		space->read.handlers[entrynum].mask = space->mask;
		space->write.handlers[entrynum].handler.generic = get_static_handler(dbits, 1, spacenum, entrynum);
		space->write.handlers[entrynum].mask = space->mask;
	}

	/* allocate memory */
	space->read.table = malloc(1 << LEVEL1_BITS);
	space->write.table = malloc(1 << LEVEL1_BITS);
	if (!space->read.table)
	{
		osd_die("cpu #%d couldn't allocate read table\n", cpunum);
		return -1;
	}
	if (!space->write.table)
	{
		osd_die("cpu #%d couldn't allocate write table\n", cpunum);
		return -1;
	}

	/* initialize everything to unmapped */
	memset(space->read.table, STATIC_UNMAP, 1 << LEVEL1_BITS);
	memset(space->write.table, STATIC_UNMAP, 1 << LEVEL1_BITS);
	return 1;
}


/*-------------------------------------------------
	preflight_memory - verify the memory structs
	and track which banks are referenced
-------------------------------------------------*/

static int preflight_memory(void)
{
	int cpunum, spacenum, entrynum;

	/* zap the bank data */
	memset(&bankdata, 0, sizeof(bankdata));

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (cpudata[cpunum].spacemask & (1 << spacenum))
			{
				struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
				const struct address_map_t *map;

				/* scan the adjusted map */
				for (map = space->adjmap; map && !IS_AMENTRY_END(map); map++)
				{
					/* look for extended flags */
					if (IS_AMENTRY_EXTENDED(map))
					{
						UINT32 flags = AM_EXTENDED_FLAGS(map);
						UINT32 val;

						/* if we specify an address space, make sure it matches the current space */
						if (flags & AMEF_SPECIFIES_SPACE)
						{
							val = (flags & AMEF_SPACE_MASK) >> AMEF_SPACE_SHIFT;
							if (val != spacenum)
							{
								osd_die("cpu #%d has address space %d handlers in place of address space %d handlers!\n", cpunum, val, spacenum);
								return -1;
							}
						}

						/* if we specify an databus width, make sure it matches the current address space's */
						if (flags & AMEF_SPECIFIES_DBITS)
						{
							val = (flags & AMEF_DBITS_MASK) >> AMEF_DBITS_SHIFT;
							val = (val + 1) * 8;
							if (val != space->dbits)
							{
								osd_die("cpu #%d uses wrong %d-bit handlers for address space %d (should be %d-bit)!\n", cpunum, val, spacenum, space->dbits);
								return -1;
							}
						}

						/* if we specify an addressbus width, adjust the mask */
						if (flags & AMEF_SPECIFIES_ABITS)
						{
							space->rawmask = 0xffffffffUL >> (32 - ((flags & AMEF_ABITS_MASK) >> AMEF_ABITS_SHIFT));
							space->mask = SPACE_SHIFT_END(space, space->rawmask);
						}

						/* if we specify an unmap value, set it */
						if (flags & AMEF_SPECIFIES_UNMAP)
							space->unmap = ((flags & AMEF_UNMAP_MASK) == 0) ? (data64_t)0 : (data64_t)-1;
					}

					/* otherwise, just track banks and hardcoded memory pointers */
					else
					{
						int bank = -1;

						/* look for a bank handler in eithe read or write */
						if (HANDLER_IS_BANK(map->read.handler))
							bank = HANDLER_TO_BANK(map->read.handler);
						else if (HANDLER_IS_BANK(map->write.handler))
							bank = HANDLER_TO_BANK(map->write.handler);

						/* if we got one, add the data */
						if (bank >= 1 && bank <= MAX_EXPLICIT_BANKS)
						{
							struct bank_data_t *bdata = &bankdata[bank];
							bdata->used = 1;
							bdata->dynamic = 0;
							bdata->cpunum = cpunum;
							bdata->spacenum = spacenum;
							bdata->base = map->start;
						}
					}
				}

				/* now loop over all the handlers and enforce the address mask (which may have changed) */
				for (entrynum = 0; entrynum < ENTRY_COUNT; entrynum++)
				{
					space->read.handlers[entrynum].mask &= space->mask;
					space->write.handlers[entrynum].mask &= space->mask;
				}
			}
	return 1;
}


/*-------------------------------------------------
	populate_memory - populate the memory mapping
	tables with entries
-------------------------------------------------*/

static int populate_memory(void)
{
	int cpunum, spacenum;

	/* loop over CPUs and address spaces */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (cpudata[cpunum].spacemask & (1 << spacenum))
			{
				struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
				const struct address_map_t *map;

				/* install the handlers, using the original, unadjusted memory map */
				if (space->map)
				{
					/* first find the end */
					for (map = space->map; !IS_AMENTRY_END(map); map++) ;

					/* then work backwards, populating the address map */
					for (map--; map >= space->map; map--)
						if (!IS_AMENTRY_EXTENDED(map))
						{
							int ismatchmask = ((map->flags & AM_FLAGS_MATCH_MASK) != 0);
							int isfixed = (map->memory != NULL) || (map->share != 0);
							if (map->read.handler)
								install_mem_handler(space, 0, space->dbits, ismatchmask, map->start, map->end, map->mask, map->mirror, map->read.handler, isfixed);
							if (map->write.handler)
								install_mem_handler(space, 1, space->dbits, ismatchmask, map->start, map->end, map->mask, map->mirror, map->write.handler, isfixed);
						}
				}
			}

	return 1;
}


/*-------------------------------------------------
	install_mem_handler - installs a handler for
	memory operations
-------------------------------------------------*/

static void install_mem_handler(struct addrspace_data_t *space, int iswrite, int databits, int ismatchmask, offs_t start, offs_t end, offs_t mask, offs_t mirror, void *handler, int isfixed)
{
	struct table_data_t *tabledata = iswrite ? &space->write : &space->read;
	offs_t mirrorbit[32], mirrorbits, mirrorcount;
	UINT8 idx;
	int i;

	/* sanity check */
	if (space->dbits != databits)
	{
		printf("fatal: install_mem_handler called with a %d-bit handler for a %d-bit address space\n", databits, space->dbits);
		exit(1);
	}

	/* if we're installing a new bank, make sure we mark it */
	if (HANDLER_IS_BANK(handler) && !bankdata[HANDLER_TO_BANK(handler)].used)
	{
		struct bank_data_t *bdata = &bankdata[HANDLER_TO_BANK(handler)];
		bdata->used = 1;
		bdata->dynamic = 0;
		bdata->cpunum = space->cpunum;
		bdata->spacenum = space->spacenum;
		bdata->base = start;
	}

	/* adjust the incoming addresses */
	adjust_addresses(space, ismatchmask, &start, &end, &mask, &mirror);

	/* translate ROM and RAMROM to RAM here for read cases */
	if (!iswrite)
	{
		if (HANDLER_IS_ROM(handler) || HANDLER_IS_RAMROM(handler))
			handler = (void *)MRA8_RAM;
	}

	/* also translate ROM to UNMAP for write cases */
	else
	{
		if (HANDLER_IS_ROM(handler))
			handler = (void *)STATIC_UNMAP;
	}

	/* assign banks for RAM/ROM areas */
	if (HANDLER_IS_RAM(handler) || HANDLER_IS_ROM(handler))
	{
		handler = (void *)assign_dynamic_bank(space->cpunum, space->spacenum, start, mirror, isfixed);
		if (!bank_ptr[HANDLER_TO_BANK(handler)])
			bank_ptr[HANDLER_TO_BANK(handler)] = memory_find_base(space->cpunum, space->spacenum, iswrite, start);
	}

	/* determine the mirror bits */
	mirrorbits = 0;
	for (i = 0; i < 32; i++)
		if (mirror & (1 << i))
			mirrorbit[mirrorbits++] = 1 << i;

	/* get the final handler index */
	idx = get_handler_index(tabledata->handlers, handler, start, end, mask);

	/* loop over mirrors */
	for (mirrorcount = 0; mirrorcount < (1 << mirrorbits); mirrorcount++)
	{
		/* compute the base of this mirror */
		offs_t mirrorbase = 0;
		for (i = 0; i < mirrorbits; i++)
			if (mirrorcount & (1 << i))
				mirrorbase |= mirrorbit[i];

		/* populate the tables */
		if (!ismatchmask)
			populate_table_range(space, iswrite, start + mirrorbase, end + mirrorbase, idx);
		else
			populate_table_match(space, iswrite, start + mirrorbase, end + mirrorbase, idx);
	}

	/* if this is being installed to a live CPU, update the context */
	if (space->cpunum == cur_context)
		memory_set_context(cur_context);
}


/*-------------------------------------------------
	assign_dynamic_bank - finds a free or exact
	matching bank
-------------------------------------------------*/

static void *assign_dynamic_bank(int cpunum, int spacenum, offs_t start, offs_t mirror, int isfixed)
{
	int bank;

	/* special case: never assign a dynamic bank to an offset that */
	/* intersects the CPU's region; always use RAM for that */
	if (spacenum == ADDRESS_SPACE_PROGRAM && start < memory_region_length(REGION_CPU1 + cpunum))
	{
		/* ...unless it's mirrored, in which case, we need to use a bank anyway */
		/* ...or unless we have a fixed pointer, in which case, we also need to use a bank */
		if (mirror == 0 && !isfixed)
			return (void *)STATIC_RAM;
	}

	/* loop over banks, searching for an exact match or an empty */
	for (bank = MAX_BANKS; bank >= 1; bank--)
		if (!bankdata[bank].used || (bankdata[bank].dynamic && bankdata[bank].cpunum == cpunum && bankdata[bank].spacenum == spacenum && bankdata[bank].base == start))
		{
			bankdata[bank].used = 1;
			bankdata[bank].dynamic = 1;
			bankdata[bank].cpunum = cpunum;
			bankdata[bank].spacenum = spacenum;
			bankdata[bank].base = start;
			VPRINTF(("Assigned bank %d to %d,%d,%08X\n", bank, cpunum, spacenum, start));
			return BANK_TO_HANDLER(bank);
		}

	/* if we got here, we failed */
	osd_die("cpu #%d: ran out of banks for RAM/ROM regions!\n", cpunum);
	return NULL;
}


/*-------------------------------------------------
	get_handler_index - finds the index of a
	handler, or allocates a new one as necessary
-------------------------------------------------*/

static UINT8 get_handler_index(struct handler_data_t *table, void *handler, offs_t start, offs_t end, offs_t mask)
{
	int i;

	start &= mask;

	/* all static handlers are hardcoded */
	if (HANDLER_IS_STATIC(handler))
	{
		i = (FPTR)handler;
		if (HANDLER_IS_BANK(handler))
		{
			table[i].offset = start;
			table[i].top = end;
			table[i].mask = mask;
		}
		return i;
	}

	/* otherwise, we have to search */
	for (i = STATIC_COUNT; i < SUBTABLE_BASE; i++)
	{
		if (table[i].handler.generic == NULL)
		{
			table[i].handler.generic = handler;
			table[i].offset = start;
			table[i].top = end;
			table[i].mask = mask;
			return i;
		}
		if (table[i].handler.generic == handler && table[i].offset == start && table[i].mask == mask)
			return i;
	}
	return 0;
}


/*-------------------------------------------------
	populate_table_range - assign a memory handler
	to a range of addresses
-------------------------------------------------*/

static void populate_table_range(struct addrspace_data_t *space, int iswrite, offs_t start, offs_t stop, UINT8 handler)
{
	struct table_data_t *tabledata = iswrite ? &space->write : &space->read;
	offs_t l2mask = (1 << LEVEL2_BITS) - 1;
	offs_t l1start = start >> LEVEL2_BITS;
	offs_t l2start = start & l2mask;
	offs_t l1stop = stop >> LEVEL2_BITS;
	offs_t l2stop = stop & l2mask;
	offs_t l1index;

	/* sanity check */
	if (start > stop)
		return;

	/* handle the starting edge if it's not on a block boundary */
	if (l2start != 0)
	{
		UINT8 *subtable = open_subtable(tabledata, l1start);

		/* if the start and stop end within the same block, handle that */
		if (l1start == l1stop)
		{
			memset(&subtable[l2start], handler, l2stop - l2start + 1);
			close_subtable(tabledata, l1start);
			return;
		}

		/* otherwise, fill until the end */
		memset(&subtable[l2start], handler, (1 << LEVEL2_BITS) - l2start);
		close_subtable(tabledata, l1start);
		if (l1start != (offs_t)~0) l1start++;
	}

	/* handle the trailing edge if it's not on a block boundary */
	if (l2stop != l2mask)
	{
		UINT8 *subtable = open_subtable(tabledata, l1stop);

		/* fill from the beginning */
		memset(&subtable[0], handler, l2stop + 1);
		close_subtable(tabledata, l1stop);

		/* if the start and stop end within the same block, handle that */
		if (l1start == l1stop)
			return;
		if (l1stop != 0) l1stop--;
	}

	/* now fill in the middle tables */
	for (l1index = l1start; l1index <= l1stop; l1index++)
	{
		/* if we have a subtable here, release it */
		if (tabledata->table[l1index] >= SUBTABLE_BASE)
			release_subtable(tabledata, tabledata->table[l1index]);
		tabledata->table[l1index] = handler;
	}
}


/*-------------------------------------------------
	populate_table_match - assign a memory handler
	to a range of addresses
-------------------------------------------------*/

static void populate_table_match(struct addrspace_data_t *space, int iswrite, offs_t matchval, offs_t matchmask, UINT8 handler)
{
	struct table_data_t *tabledata = iswrite ? &space->write : &space->read;
	int lowermask, lowermatch;
	int uppermask, uppermatch;
	int l1index, l2index;

	/* clear out any ignored bits in the matchval */
	matchval &= matchmask;

	/* compute the lower half of the match/mask pair */
	lowermask = matchmask & ((1<<LEVEL2_BITS)-1);
	lowermatch = matchval & ((1<<LEVEL2_BITS)-1);

	/* compute the upper half of the match/mask pair */
	uppermask = matchmask >> LEVEL2_BITS;
	uppermatch = matchval >> LEVEL2_BITS;

	/* if the lower bits of the mask are all 0, we can work exclusively at the top level */
	if (lowermask == 0)
	{
		/* loop over top level matches */
		for (l1index = 0; l1index <= (space->mask >> LEVEL2_BITS); l1index++)
			if ((l1index & uppermatch) == uppermask)
			{
				/* if we have a subtable here, release it */
				if (tabledata->table[l1index] >= SUBTABLE_BASE)
					release_subtable(tabledata, tabledata->table[l1index]);
				tabledata->table[l1index] = handler;
			}
	}

	/* okay, we need to work at both levels */
	else
	{
		/* loop over top level matches */
		for (l1index = 0; l1index <= (space->mask >> LEVEL2_BITS); l1index++)
			if ((l1index & uppermatch) == uppermask)
			{
				UINT8 *subtable = open_subtable(tabledata, l1index);

				/* now loop over lower level matches */
				for (l2index = 0; l2index < (1 << LEVEL2_BITS); l2index++)
					if ((l2index & lowermask) == lowermatch)
						subtable[l2index] = handler;
				close_subtable(tabledata, l1index);
			}
	}
}


/*-------------------------------------------------
	allocate_subtable - allocate a fresh subtable
	and set its usecount to 1
-------------------------------------------------*/

static UINT8 allocate_subtable(struct table_data_t *tabledata)
{
	/* loop */
	while (1)
	{
		UINT8 subindex;

		/* find a subtable with a usecount of 0 */
		for (subindex = 0; subindex < SUBTABLE_COUNT; subindex++)
			if (tabledata->subtable[subindex].usecount == 0)
			{
				/* if this is past our allocation budget, allocate some more */
				if (subindex >= tabledata->subtable_alloc)
				{
					tabledata->subtable_alloc += SUBTABLE_ALLOC;
					tabledata->table = realloc(tabledata->table, (1 << LEVEL1_BITS) + (tabledata->subtable_alloc << LEVEL2_BITS));
					if (!tabledata->table)
						osd_die("error: ran out of memory allocating memory subtable\n");
				}

				/* bump the usecount and return */
				tabledata->subtable[subindex].usecount++;
				return subindex + SUBTABLE_BASE;
			}

		/* merge any subtables we can */
		if (!merge_subtables(tabledata))
			osd_die("Ran out of subtables!\n");
	}

	/* hopefully this never happens */
	return 0;
}


/*-------------------------------------------------
	merge_subtables - merge any duplicate
	subtables
-------------------------------------------------*/

static int merge_subtables(struct table_data_t *tabledata)
{
	int merged = 0;
	UINT8 subindex;

	VPRINTF(("Merging subtables....\n"));

	/* okay, we failed; update all the checksums and merge tables */
	for (subindex = 0; subindex < SUBTABLE_COUNT; subindex++)
		if (!tabledata->subtable[subindex].checksum_valid)
		{
			UINT32 *subtable = (UINT32 *)SUBTABLE_PTR(tabledata, subindex + SUBTABLE_BASE);
			UINT32 checksum = 0;
			int l2index;

			/* update the checksum */
			for (l2index = 0; l2index < (1 << LEVEL2_BITS)/4; l2index++)
				checksum += subtable[l2index];
			tabledata->subtable[subindex].checksum = checksum;
			tabledata->subtable[subindex].checksum_valid = 1;
		}

	/* see if there's a matching checksum */
	for (subindex = 0; subindex < SUBTABLE_COUNT; subindex++)
	{
		UINT8 *subtable = SUBTABLE_PTR(tabledata, subindex + SUBTABLE_BASE);
		UINT32 checksum = tabledata->subtable[subindex].checksum;
		UINT8 sumindex;

		for (sumindex = subindex + 1; sumindex < SUBTABLE_COUNT; sumindex++)
			if (tabledata->subtable[sumindex].checksum == checksum &&
				!memcmp(subtable, SUBTABLE_PTR(tabledata, sumindex + SUBTABLE_BASE), 1 << LEVEL2_BITS))
			{
				int l1index;

				VPRINTF(("Merging subtable %d and %d....\n", subindex, sumindex));

				/* got a match -- merge the tables */
				tabledata->subtable[sumindex].usecount++;
				release_subtable(tabledata, subindex + SUBTABLE_BASE);
				merged++;

				/* find all the entries in the L1 tables that pointed to the old one, and point them to the merged table */
				for (l1index = 0; l1index <= (0xffffffffUL >> LEVEL2_BITS); l1index++)
					if (tabledata->table[l1index] == subindex + SUBTABLE_BASE)
						tabledata->table[l1index] = sumindex + SUBTABLE_BASE;
				break;
			}
	}

	return merged;
}


/*-------------------------------------------------
	release_subtable - decrement the usecount on
	a subtable and free it if we're done
-------------------------------------------------*/

static void release_subtable(struct table_data_t *tabledata, UINT8 subentry)
{
	UINT8 subindex = subentry - SUBTABLE_BASE;

	/* sanity check */
	if (tabledata->subtable[subindex].usecount <= 0)
		osd_die("Called release_subtable on a table with a usecount of 0\n");

	/* decrement the usecount and clear the checksum if we're at 0 */
	tabledata->subtable[subindex].usecount--;
	if (tabledata->subtable[subindex].usecount == 0)
		tabledata->subtable[subindex].checksum = 0;
}


/*-------------------------------------------------
	open_subtable - gain access to a subtable for
	modification
-------------------------------------------------*/

static UINT8 *open_subtable(struct table_data_t *tabledata, offs_t l1index)
{
	UINT8 subentry = tabledata->table[l1index];

	/* if we don't have a subtable yet, allocate a new one */
	if (subentry < SUBTABLE_BASE)
	{
		UINT8 newentry = allocate_subtable(tabledata);
		memset(SUBTABLE_PTR(tabledata, newentry), subentry, 1 << LEVEL2_BITS);
		tabledata->table[l1index] = newentry;
		tabledata->subtable[newentry - SUBTABLE_BASE].checksum = (subentry + (subentry << 8) + (subentry << 16) + (subentry << 24)) * ((1 << LEVEL2_BITS)/4);
		subentry = newentry;
	}

	/* if we're sharing this subtable, we also need to allocate a fresh copy */
	else if (tabledata->subtable[subentry - SUBTABLE_BASE].usecount > 1)
	{
		UINT8 newentry = allocate_subtable(tabledata);
		memcpy(SUBTABLE_PTR(tabledata, newentry), SUBTABLE_PTR(tabledata, subentry), 1 << LEVEL2_BITS);
		release_subtable(tabledata, subentry);
		tabledata->table[l1index] = newentry;
		tabledata->subtable[newentry - SUBTABLE_BASE].checksum = tabledata->subtable[subentry - SUBTABLE_BASE].checksum;
		subentry = newentry;
	}

	/* mark the table dirty */
	tabledata->subtable[subentry - SUBTABLE_BASE].checksum_valid = 0;

	/* return the pointer to the subtable */
	return SUBTABLE_PTR(tabledata, subentry);
}


/*-------------------------------------------------
	close_subtable - stop access to a subtable
-------------------------------------------------*/

static void close_subtable(struct table_data_t *tabledata, offs_t l1index)
{
	/* defer any merging until we run out of tables */
}


/*-------------------------------------------------
	Return whether a given memory map entry implies
	the need of allocating and registering memory
-------------------------------------------------*/

static int amentry_needs_backing_store(int cpunum, int spacenum, const struct address_map_t *map)
{
	int handler;

	if (IS_AMENTRY_EXTENDED(map))
		return 0;
	if (map->base)
		return 1;

	handler = (int)map->write.handler;
	if (handler >= 0 && handler < STATIC_COUNT)
	{
		if (handler != STATIC_INVALID &&
			handler != STATIC_ROM &&
			handler != STATIC_NOP &&
			handler != STATIC_UNMAP)
			return 1;
	}

	handler = (int)map->read.handler;
	if (handler >= 0 && handler < STATIC_COUNT)
	{
		if (handler != STATIC_INVALID &&
			(handler < STATIC_BANK1 || handler > STATIC_BANK1 + MAX_BANKS - 1) &&
			(handler != STATIC_ROM || spacenum != ADDRESS_SPACE_PROGRAM || map->start >= memory_region_length(REGION_CPU1 + cpunum)) &&
			handler != STATIC_NOP &&
			handler != STATIC_UNMAP)
			return 1;
	}

	return 0;
}


/*-------------------------------------------------
	allocate_memory - allocate memory for
	CPU address spaces
-------------------------------------------------*/

static int allocate_memory(void)
{
	int cpunum, spacenum;

	/* don't do it for drivers that don't have ROM (MESS needs this) */
	if (Machine->gamedrv->rom == 0)
		return 1;

	/* loop over all CPUs and memory spaces */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (cpudata[cpunum].spacemask & (1 << spacenum))
			{
				struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
				struct address_map_t *map, *unassigned = NULL;
				int start_count = memory_block_count;
				int i;

				/* make a first pass over the memory map and track blocks with hardcoded pointers */
				/* we do this to make sure they are found by memory_find_base first */
				for (map = space->adjmap; map && !IS_AMENTRY_END(map); map++)
					if (!IS_AMENTRY_EXTENDED(map) && map->memory)
					{
						if (!IS_AMENTRY_MATCH_MASK(map))
							allocate_memory_block(cpunum, spacenum, map->start, map->end, map->memory);
						else
							allocate_memory_block(cpunum, spacenum, map->start, map->start + map->mask, map->memory);
					}

				/* if this is a program space, we also throw in the memory_region for the CPU */
				if (spacenum == ADDRESS_SPACE_PROGRAM && memory_region(REGION_CPU1 + cpunum))
					allocate_memory_block(cpunum, spacenum, 0, memory_region_length(REGION_CPU1 + cpunum) - 1, memory_region(REGION_CPU1 + cpunum));

				/* loop over all blocks just allocated and assign pointers from them */
				for (i = start_count; i < memory_block_count; i++)
					unassigned = assign_intersecting_blocks(space, memory_block[i].start, memory_block[i].end, memory_block[i].data);

				/* if we don't have an unassigned pointer yet, try to find one */
				if (!unassigned)
					unassigned = assign_intersecting_blocks(space, ~0, 0, NULL);

				/* loop until we've assigned all memory in this space */
				while (unassigned)
				{
					offs_t curstart, curend;
					int changed;
					void *block;

					/* work in MEMORY_BLOCK_SIZE-sized chunks */
					curstart = unassigned->start / MEMORY_BLOCK_SIZE;
					if (!IS_AMENTRY_MATCH_MASK(unassigned))
						curend = unassigned->end / MEMORY_BLOCK_SIZE;
					else
						curend = (unassigned->start + unassigned->mask) / MEMORY_BLOCK_SIZE;

					/* loop while we keep finding unassigned blocks in neighboring MEMORY_BLOCK_SIZE chunks */
					do
					{
						changed = 0;

						/* scan for unmapped blocks in the adjusted map */
						for (map = space->adjmap; map && !IS_AMENTRY_END(map); map++)
							if (!IS_AMENTRY_EXTENDED(map) && !map->memory && map != unassigned && amentry_needs_backing_store(cpunum, spacenum, map))
							{
								offs_t blockstart, blockend;

								/* get block start/end blocks for this block */
								blockstart = map->start / MEMORY_BLOCK_SIZE;
								if (!IS_AMENTRY_MATCH_MASK(map))
									blockend = map->end / MEMORY_BLOCK_SIZE;
								else
									blockend = (map->start + map->mask) / MEMORY_BLOCK_SIZE;

								/* if we intersect or are adjacent, adjust the start/end */
								if (blockstart <= curend + 1 && blockend >= curstart - 1)
								{
									if (blockstart < curstart)
										curstart = blockstart, changed = 1;
									if (blockend > curend)
										curend = blockend, changed = 1;
								}
							}
					} while (changed);

					/* we now have a block to allocate; do it */
					curstart = curstart * MEMORY_BLOCK_SIZE;
					curend = curend * MEMORY_BLOCK_SIZE + (MEMORY_BLOCK_SIZE - 1);
					block = allocate_memory_block(cpunum, spacenum, curstart, curend, NULL);

					/* assign memory that intersected the new block */
					unassigned = assign_intersecting_blocks(space, curstart, curend, block);
				}
			}

	return 1;
}


/*-------------------------------------------------
	allocate_memory_block - allocate a single
	memory block of data
-------------------------------------------------*/

static void *allocate_memory_block(int cpunum, int spacenum, offs_t start, offs_t end, void *memory)
{
	struct memory_block_t *block = &memory_block[memory_block_count];
	int allocatemem = (memory == NULL);

	VPRINTF(("allocate_memory_block(%d,%d,%08X,%08X,%08X)\n", cpunum, spacenum, start, end, (UINT32)memory));

	/* if we weren't passed a memory block, allocate one and clear it to zero */
	if (allocatemem)
	{
		memory = malloc(end - start + 1);
		if (!memory)
		{
			osd_die("Out of memory allocating %d bytes for CPU %d, space %d, range %X-%X\n", end - start + 1, cpunum, spacenum, start, end);
			return NULL;
		}
		memset(memory, 0, end - start + 1);
	}

	/* register for saving */
	register_for_save(cpunum, spacenum, start, memory, end - start + 1);

	/* fill in the tracking block */
	block->cpunum = cpunum;
	block->spacenum = spacenum;
	block->isallocated = allocatemem;
	block->start = start;
	block->end = end;
	block->data = memory;
	memory_block_count++;
	return memory;
}


/*-------------------------------------------------
	register_for_save - register a block of
	memory for save states
-------------------------------------------------*/

static void register_for_save(int cpunum, int spacenum, offs_t start, void *base, size_t numbytes)
{
	char name[256];

	sprintf(name, "%d.%08x-%08x", spacenum, start, (int)(start + numbytes - 1));
	switch (cpudata[cpunum].space[spacenum].dbits)
	{
		case 8:
			state_save_register_UINT8 ("memory", cpunum, name, base, numbytes);
			break;
		case 16:
			state_save_register_UINT16("memory", cpunum, name, base, numbytes/2);
			break;
		case 32:
			state_save_register_UINT32("memory", cpunum, name, base, numbytes/4);
			break;
		case 64:
			state_save_register_UINT64("memory", cpunum, name, base, numbytes/8);
			break;
	}
}


/*-------------------------------------------------
	assign_intersecting_blocks - find all
	intersecting blocks and assign their pointers
-------------------------------------------------*/

static struct address_map_t *assign_intersecting_blocks(struct addrspace_data_t *space, offs_t start, offs_t end, UINT8 *base)
{
	struct address_map_t *map, *unassigned = NULL;

	/* loop over the adjusted map and assign memory to any blocks we can */
	for (map = space->adjmap; map && !IS_AMENTRY_END(map); map++)
		if (!IS_AMENTRY_EXTENDED(map))
		{
			/* if we haven't assigned this block yet, do it against the last block */
			if (!map->memory)
			{
				/* inherit shared pointers first */
				if (map->share && shared_ptr[map->share])
				{
					map->memory = shared_ptr[map->share];
	 				VPRINTF(("memory range %08X-%08X -> shared_ptr[%d] [%08X]\n", map->start, map->end, map->share, (UINT32)map->memory));
	 			}

				/* otherwise, look for a match in this block */
				else
				{
					if (!IS_AMENTRY_MATCH_MASK(map))
					{
						if (map->start >= start && map->end <= end)
						{
							map->memory = base + (map->start - start);
	 						VPRINTF(("memory range %08X-%08X -> found in block from %08X-%08X [%08X]\n", map->start, map->end, start, end, (UINT32)map->memory));
	 					}
					}
					else
					{
						if (map->start >= start && map->start + map->mask <= end)
						{
							map->memory = base + (map->start - start);
	 						VPRINTF(("memory range %08X-%08X -> found in block from %08X-%08X [%08X]\n", map->start, map->end, start, end, (UINT32)map->memory));
	 					}
					}
				}
			}

			/* if we're the first match on a shared pointer, assign it now */
			if (map->memory && map->share && !shared_ptr[map->share])
				shared_ptr[map->share] = map->memory;

			/* keep track of the first unassigned entry */
			if (!map->memory && !unassigned && amentry_needs_backing_store(space->cpunum, space->spacenum, map))
				unassigned = map;
		}

	return unassigned;
}


/*-------------------------------------------------
	find_memory - find all the requested pointers
	into the final allocated memory
-------------------------------------------------*/

static int find_memory(void)
{
	int cpunum, spacenum, banknum;

	/* loop over CPUs and address spaces */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (cpudata[cpunum].spacemask & (1 << spacenum))
			{
				struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
				const struct address_map_t *map;

				/* fill in base/size entries, and handle shared memory */
				for (map = space->adjmap; map && !IS_AMENTRY_END(map); map++)
					if (!IS_AMENTRY_EXTENDED(map))
					{
						/* assign base/size values */
						if (map->base) *map->base = map->memory;
						if (map->size)
						{
							if (!IS_AMENTRY_MATCH_MASK(map))
								*map->size = map->end - map->start + 1;
							else
								*map->size = map->mask + 1;
						}
					}
			}

	/* once this is done, find the starting bases for the banks */
	for (banknum = 1; banknum <= MAX_BANKS; banknum++)
		if (bankdata[banknum].used)
		{
			struct address_map_t *map;
			for (map = cpudata[bankdata[banknum].cpunum].space[bankdata[banknum].spacenum].adjmap; map && !IS_AMENTRY_END(map); map++)
				if (!IS_AMENTRY_EXTENDED(map) && map->start == bankdata[banknum].base)
				{
					bank_ptr[banknum] = map->memory;
	 				VPRINTF(("assigned bank %d pointer to memory from range %08X-%08X [%08X]\n", banknum, map->start, map->end, (UINT32)map->memory));
					break;
				}
		}

	return 1;
}


/*-------------------------------------------------
	memory_find_base - return a pointer to the
	base of RAM associated with the given CPU
	and offset
-------------------------------------------------*/

static void *memory_find_base(int cpunum, int spacenum, int readwrite, offs_t offset)
{
	struct addrspace_data_t *space = &cpudata[cpunum].space[spacenum];
	struct address_map_t *map;
	struct memory_block_t *block;
	int blocknum;

	VPRINTF(("memory_find_base(%d,%d,%d,%08X) -> ", cpunum, spacenum, readwrite, offset));

	/* look in the adjusted map */
	for (map = space->adjmap; map && !IS_AMENTRY_END(map); map++)
		if (!IS_AMENTRY_EXTENDED(map))
		{
			offs_t maskoffs = offset & map->mask;
			if (!IS_AMENTRY_MATCH_MASK(map))
			{
				if (maskoffs >= map->start && maskoffs <= map->end)
				{
					VPRINTF(("found in entry %08X-%08X [%08X]\n", map->start, map->end, (UINT32)map->memory + (maskoffs - map->start)));
					return (UINT8 *)map->memory + (maskoffs - map->start);
				}
			}
			else
			{
				if ((maskoffs & map->end) == map->start)
				{
					VPRINTF(("found in entry %08X-%08X [%08X]\n", map->start, map->end, (UINT32)map->memory + (maskoffs - map->start)));
					return (UINT8 *)map->memory + (maskoffs - map->start);
				}
			}
		}

	/* if not found there, look in the allocated blocks */
	for (blocknum = 0, block = memory_block; blocknum < memory_block_count; blocknum++, block++)
		if (block->cpunum == cpunum && block->spacenum == spacenum && block->start <= offset && block->end > offset)
		{
			VPRINTF(("found in allocated memory block %08X-%08X [%08X]\n", block->start, block->end, (UINT32)block->data + (offset - block->start)));
			return block->data + offset - block->start;
		}

	VPRINTF(("did not find\n"));
	return NULL;
}


/*-------------------------------------------------
	PERFORM_LOOKUP - common lookup procedure
-------------------------------------------------*/

#define PERFORM_LOOKUP(lookup,space,extraand)											\
	/* perform lookup */																\
	address &= space.addrmask & extraand;												\
	entry = space.lookup[LEVEL1_INDEX(address)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = space.lookup[LEVEL2_INDEX(entry,address)];								\


/*-------------------------------------------------
	READBYTE - generic byte-sized read handler
-------------------------------------------------*/

#define READBYTE8(name,space)															\
data8_t name(offs_t address)															\
{																						\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,space,~0);												\
																						\
	/* handle banks inline */															\
	address = (address - space.readhandlers[entry].offset) & space.readhandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(bank_ptr[entry][address]);											\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMREADEND((*space.readhandlers[entry].handler.read.handler8)(address));		\
	return 0;																			\
}																						\

#define READBYTE(name,space,xormacro,handlertype,ignorebits,shiftbytes)					\
data8_t name(offs_t address)															\
{																						\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,space,~0);												\
																						\
	/* handle banks inline */															\
	address = (address - space.readhandlers[entry].offset) & space.readhandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(bank_ptr[entry][xormacro(address)]);									\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMREADEND((*space.readhandlers[entry].handler.read.handlertype)(address >> (ignorebits), ~(0xff << shift)) >> shift);		\
	}																					\
	return 0;																			\
}																						\

#define READBYTE16BE(name,space)	READBYTE(name,space,BYTE_XOR_BE, handler16,1,~address & 1)
#define READBYTE16LE(name,space)	READBYTE(name,space,BYTE_XOR_LE, handler16,1, address & 1)
#define READBYTE32BE(name,space)	READBYTE(name,space,BYTE4_XOR_BE,handler32,2,~address & 3)
#define READBYTE32LE(name,space)	READBYTE(name,space,BYTE4_XOR_LE,handler32,2, address & 3)
#define READBYTE64BE(name,space)	READBYTE(name,space,BYTE8_XOR_BE,handler64,3,~address & 7)
#define READBYTE64LE(name,space)	READBYTE(name,space,BYTE8_XOR_LE,handler64,3, address & 7)


/*-------------------------------------------------
	READWORD - generic word-sized read handler
	(16-bit, 32-bit and 64-bit aligned only!)
-------------------------------------------------*/

#define READWORD16(name,space)															\
data16_t name(offs_t address)															\
{																						\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,space,~1);												\
																						\
	/* handle banks inline */															\
	address = (address - space.readhandlers[entry].offset) & space.readhandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(*(data16_t *)&bank_ptr[entry][address]);								\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMREADEND((*space.readhandlers[entry].handler.read.handler16)(address >> 1,0));\
	return 0;																			\
}																						\

#define READWORD(name,space,xormacro,handlertype,ignorebits,shiftbytes)					\
data16_t name(offs_t address)															\
{																						\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,space,~1);												\
																						\
	/* handle banks inline */															\
	address = (address - space.readhandlers[entry].offset) & space.readhandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(*(data16_t *)&bank_ptr[entry][xormacro(address)]);					\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMREADEND((*space.readhandlers[entry].handler.read.handlertype)(address >> (ignorebits), ~(0xffff << shift)) >> shift);	\
	}																					\
	return 0;																			\
}																						\

#define READWORD32BE(name,space)	READWORD(name,space,WORD_XOR_BE, handler32,2,~address & 2)
#define READWORD32LE(name,space)	READWORD(name,space,WORD_XOR_LE, handler32,2, address & 2)
#define READWORD64BE(name,space)	READWORD(name,space,WORD2_XOR_BE,handler64,3,~address & 6)
#define READWORD64LE(name,space)	READWORD(name,space,WORD2_XOR_LE,handler64,3, address & 6)


/*-------------------------------------------------
	READDWORD - generic dword-sized read handler
	(32-bit and 64-bit aligned only!)
-------------------------------------------------*/

#define READDWORD32(name,space)															\
data32_t name(offs_t address)															\
{																						\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,space,~3);												\
																						\
	/* handle banks inline */															\
	address = (address - space.readhandlers[entry].offset) & space.readhandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(*(data32_t *)&bank_ptr[entry][address]);								\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMREADEND((*space.readhandlers[entry].handler.read.handler32)(address >> 2,0));\
	return 0;																			\
}																						\

#define READDWORD(name,space,xormacro,handlertype,ignorebits,shiftbytes)				\
data32_t name(offs_t address)															\
{																						\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,space,~3);												\
																						\
	/* handle banks inline */															\
	address = (address - space.readhandlers[entry].offset) & space.readhandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(*(data32_t *)&bank_ptr[entry][xormacro(address)]);					\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMREADEND((*space.readhandlers[entry].handler.read.handlertype)(address >> (ignorebits), ~(0xffffffff << shift)) >> shift);\
	}																					\
	return 0;																			\
}																						\

#define READDWORD64BE(name,space)	READDWORD(name,space,DWORD_XOR_BE,handler64,3,~address & 4)
#define READDWORD64LE(name,space)	READDWORD(name,space,DWORD_XOR_LE,handler64,3, address & 4)


/*-------------------------------------------------
	READQWORD - generic qword-sized read handler
	(64-bit aligned only!)
-------------------------------------------------*/

#define READQWORD64(name,space)															\
data64_t name(offs_t address)															\
{																						\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,space,~7);												\
																						\
	/* handle banks inline */															\
	address = (address - space.readhandlers[entry].offset) & space.readhandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(*(data64_t *)&bank_ptr[entry][address]);								\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMREADEND((*space.readhandlers[entry].handler.read.handler64)(address >> 3,0));\
	return 0;																			\
}																						\


/*-------------------------------------------------
	WRITEBYTE - generic byte-sized write handler
-------------------------------------------------*/

#define WRITEBYTE8(name,space)															\
void name(offs_t address, data8_t data)													\
{																						\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,space,~0);												\
																						\
	/* handle banks inline */															\
	address = (address - space.writehandlers[entry].offset) & space.writehandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(bank_ptr[entry][address] = data);									\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMWRITEEND((*space.writehandlers[entry].handler.write.handler8)(address, data));\
}																						\

#define WRITEBYTE(name,space,xormacro,handlertype,ignorebits,shiftbytes)				\
void name(offs_t address, data8_t data)													\
{																						\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,space,~0);												\
																						\
	/* handle banks inline */															\
	address = (address - space.writehandlers[entry].offset) & space.writehandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(bank_ptr[entry][xormacro(address)] = data);							\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMWRITEEND((*space.writehandlers[entry].handler.write.handlertype)(address >> (ignorebits), data << shift, ~(0xff << shift)));\
	}																					\
}																						\

#define WRITEBYTE16BE(name,space)	WRITEBYTE(name,space,BYTE_XOR_BE, handler16,1,~address & 1)
#define WRITEBYTE16LE(name,space)	WRITEBYTE(name,space,BYTE_XOR_LE, handler16,1, address & 1)
#define WRITEBYTE32BE(name,space)	WRITEBYTE(name,space,BYTE4_XOR_BE,handler32,2,~address & 3)
#define WRITEBYTE32LE(name,space)	WRITEBYTE(name,space,BYTE4_XOR_LE,handler32,2, address & 3)
#define WRITEBYTE64BE(name,space)	WRITEBYTE(name,space,BYTE8_XOR_BE,handler64,3,~address & 7)
#define WRITEBYTE64LE(name,space)	WRITEBYTE(name,space,BYTE8_XOR_LE,handler64,3, address & 7)


/*-------------------------------------------------
	WRITEWORD - generic word-sized write handler
	(16-bit, 32-bit and 64-bit aligned only!)
-------------------------------------------------*/

#define WRITEWORD16(name,space)															\
void name(offs_t address, data16_t data)												\
{																						\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,space,~1);												\
																						\
	/* handle banks inline */															\
	address = (address - space.writehandlers[entry].offset) & space.writehandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(*(data16_t *)&bank_ptr[entry][address] = data);						\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMWRITEEND((*space.writehandlers[entry].handler.write.handler16)(address >> 1, data, 0));\
}																						\

#define WRITEWORD(name,space,xormacro,handlertype,ignorebits,shiftbytes)				\
void name(offs_t address, data16_t data)												\
{																						\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,space,~1);												\
																						\
	/* handle banks inline */															\
	address = (address - space.writehandlers[entry].offset) & space.writehandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(*(data16_t *)&bank_ptr[entry][xormacro(address)] = data);			\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMWRITEEND((*space.writehandlers[entry].handler.write.handlertype)(address >> (ignorebits), data << shift, ~(0xffff << shift)));\
	}																					\
}																						\

#define WRITEWORD32BE(name,space)	WRITEWORD(name,space,WORD_XOR_BE, handler32,2,~address & 2)
#define WRITEWORD32LE(name,space)	WRITEWORD(name,space,WORD_XOR_LE, handler32,2, address & 2)
#define WRITEWORD64BE(name,space)	WRITEWORD(name,space,WORD2_XOR_BE,handler64,3,~address & 6)
#define WRITEWORD64LE(name,space)	WRITEWORD(name,space,WORD2_XOR_LE,handler64,3, address & 6)


/*-------------------------------------------------
	WRITEDWORD - dword-sized write handler
	(32-bit and 64-bit aligned only!)
-------------------------------------------------*/

#define WRITEDWORD32(name,space)														\
void name(offs_t address, data32_t data)												\
{																						\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,space,~3);												\
																						\
	/* handle banks inline */															\
	address = (address - space.writehandlers[entry].offset) & space.writehandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(*(data32_t *)&bank_ptr[entry][address] = data);						\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMWRITEEND((*space.writehandlers[entry].handler.write.handler32)(address >> 2, data, 0));\
}																						\

#define WRITEDWORD(name,space,xormacro,handlertype,ignorebits,shiftbytes)				\
void name(offs_t address, data32_t data)												\
{																						\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,space,~3);												\
																						\
	/* handle banks inline */															\
	address = (address - space.writehandlers[entry].offset) & space.writehandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(*(data32_t *)&bank_ptr[entry][xormacro(address)] = data);			\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMWRITEEND((*space.writehandlers[entry].handler.write.handlertype)(address >> (ignorebits), data << shift, ~(0xffffffff << shift)));\
	}																					\
}																						\

#define WRITEDWORD64BE(name,space)	WRITEDWORD(name,space,DWORD_XOR_BE,handler64,3,~address & 4)
#define WRITEDWORD64LE(name,space)	WRITEDWORD(name,space,DWORD_XOR_LE,handler64,3, address & 4)


/*-------------------------------------------------
	WRITEQWORD - qword-sized write handler
	(64-bit aligned only!)
-------------------------------------------------*/

#define WRITEQWORD64(name,space)														\
void name(offs_t address, data64_t data)												\
{																						\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,space,~7);												\
																						\
	/* handle banks inline */															\
	address = (address - space.writehandlers[entry].offset) & space.writehandlers[entry].mask;\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(*(data64_t *)&bank_ptr[entry][address] = data);						\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMWRITEEND((*space.writehandlers[entry].handler.write.handler64)(address >> 3, data, 0));\
}																						\


/*-------------------------------------------------
	Program memory handlers
-------------------------------------------------*/

     READBYTE8(program_read_byte_8,      address_space[ADDRESS_SPACE_PROGRAM])
    WRITEBYTE8(program_write_byte_8,     address_space[ADDRESS_SPACE_PROGRAM])

  READBYTE16BE(program_read_byte_16be,   address_space[ADDRESS_SPACE_PROGRAM])
    READWORD16(program_read_word_16be,   address_space[ADDRESS_SPACE_PROGRAM])
 WRITEBYTE16BE(program_write_byte_16be,  address_space[ADDRESS_SPACE_PROGRAM])
   WRITEWORD16(program_write_word_16be,  address_space[ADDRESS_SPACE_PROGRAM])

  READBYTE16LE(program_read_byte_16le,   address_space[ADDRESS_SPACE_PROGRAM])
    READWORD16(program_read_word_16le,   address_space[ADDRESS_SPACE_PROGRAM])
 WRITEBYTE16LE(program_write_byte_16le,  address_space[ADDRESS_SPACE_PROGRAM])
   WRITEWORD16(program_write_word_16le,  address_space[ADDRESS_SPACE_PROGRAM])

  READBYTE32BE(program_read_byte_32be,   address_space[ADDRESS_SPACE_PROGRAM])
  READWORD32BE(program_read_word_32be,   address_space[ADDRESS_SPACE_PROGRAM])
   READDWORD32(program_read_dword_32be,  address_space[ADDRESS_SPACE_PROGRAM])
 WRITEBYTE32BE(program_write_byte_32be,  address_space[ADDRESS_SPACE_PROGRAM])
 WRITEWORD32BE(program_write_word_32be,  address_space[ADDRESS_SPACE_PROGRAM])
  WRITEDWORD32(program_write_dword_32be, address_space[ADDRESS_SPACE_PROGRAM])

  READBYTE32LE(program_read_byte_32le,   address_space[ADDRESS_SPACE_PROGRAM])
  READWORD32LE(program_read_word_32le,   address_space[ADDRESS_SPACE_PROGRAM])
   READDWORD32(program_read_dword_32le,  address_space[ADDRESS_SPACE_PROGRAM])
 WRITEBYTE32LE(program_write_byte_32le,  address_space[ADDRESS_SPACE_PROGRAM])
 WRITEWORD32LE(program_write_word_32le,  address_space[ADDRESS_SPACE_PROGRAM])
  WRITEDWORD32(program_write_dword_32le, address_space[ADDRESS_SPACE_PROGRAM])

  READBYTE64BE(program_read_byte_64be,   address_space[ADDRESS_SPACE_PROGRAM])
  READWORD64BE(program_read_word_64be,   address_space[ADDRESS_SPACE_PROGRAM])
 READDWORD64BE(program_read_dword_64be,  address_space[ADDRESS_SPACE_PROGRAM])
   READQWORD64(program_read_qword_64be,  address_space[ADDRESS_SPACE_PROGRAM])
 WRITEBYTE64BE(program_write_byte_64be,  address_space[ADDRESS_SPACE_PROGRAM])
 WRITEWORD64BE(program_write_word_64be,  address_space[ADDRESS_SPACE_PROGRAM])
WRITEDWORD64BE(program_write_dword_64be, address_space[ADDRESS_SPACE_PROGRAM])
  WRITEQWORD64(program_write_qword_64be, address_space[ADDRESS_SPACE_PROGRAM])

  READBYTE64LE(program_read_byte_64le,   address_space[ADDRESS_SPACE_PROGRAM])
  READWORD64LE(program_read_word_64le,   address_space[ADDRESS_SPACE_PROGRAM])
 READDWORD64LE(program_read_dword_64le,  address_space[ADDRESS_SPACE_PROGRAM])
   READQWORD64(program_read_qword_64le,  address_space[ADDRESS_SPACE_PROGRAM])
 WRITEBYTE64LE(program_write_byte_64le,  address_space[ADDRESS_SPACE_PROGRAM])
 WRITEWORD64LE(program_write_word_64le,  address_space[ADDRESS_SPACE_PROGRAM])
WRITEDWORD64LE(program_write_dword_64le, address_space[ADDRESS_SPACE_PROGRAM])
  WRITEQWORD64(program_write_qword_64le, address_space[ADDRESS_SPACE_PROGRAM])


/*-------------------------------------------------
	Data memory handlers
-------------------------------------------------*/

     READBYTE8(data_read_byte_8,      address_space[ADDRESS_SPACE_DATA])
    WRITEBYTE8(data_write_byte_8,     address_space[ADDRESS_SPACE_DATA])

  READBYTE16BE(data_read_byte_16be,   address_space[ADDRESS_SPACE_DATA])
    READWORD16(data_read_word_16be,   address_space[ADDRESS_SPACE_DATA])
 WRITEBYTE16BE(data_write_byte_16be,  address_space[ADDRESS_SPACE_DATA])
   WRITEWORD16(data_write_word_16be,  address_space[ADDRESS_SPACE_DATA])

  READBYTE16LE(data_read_byte_16le,   address_space[ADDRESS_SPACE_DATA])
    READWORD16(data_read_word_16le,   address_space[ADDRESS_SPACE_DATA])
 WRITEBYTE16LE(data_write_byte_16le,  address_space[ADDRESS_SPACE_DATA])
   WRITEWORD16(data_write_word_16le,  address_space[ADDRESS_SPACE_DATA])

  READBYTE32BE(data_read_byte_32be,   address_space[ADDRESS_SPACE_DATA])
  READWORD32BE(data_read_word_32be,   address_space[ADDRESS_SPACE_DATA])
   READDWORD32(data_read_dword_32be,  address_space[ADDRESS_SPACE_DATA])
 WRITEBYTE32BE(data_write_byte_32be,  address_space[ADDRESS_SPACE_DATA])
 WRITEWORD32BE(data_write_word_32be,  address_space[ADDRESS_SPACE_DATA])
  WRITEDWORD32(data_write_dword_32be, address_space[ADDRESS_SPACE_DATA])

  READBYTE32LE(data_read_byte_32le,   address_space[ADDRESS_SPACE_DATA])
  READWORD32LE(data_read_word_32le,   address_space[ADDRESS_SPACE_DATA])
   READDWORD32(data_read_dword_32le,  address_space[ADDRESS_SPACE_DATA])
 WRITEBYTE32LE(data_write_byte_32le,  address_space[ADDRESS_SPACE_DATA])
 WRITEWORD32LE(data_write_word_32le,  address_space[ADDRESS_SPACE_DATA])
  WRITEDWORD32(data_write_dword_32le, address_space[ADDRESS_SPACE_DATA])

  READBYTE64BE(data_read_byte_64be,   address_space[ADDRESS_SPACE_DATA])
  READWORD64BE(data_read_word_64be,   address_space[ADDRESS_SPACE_DATA])
 READDWORD64BE(data_read_dword_64be,  address_space[ADDRESS_SPACE_DATA])
   READQWORD64(data_read_qword_64be,  address_space[ADDRESS_SPACE_DATA])
 WRITEBYTE64BE(data_write_byte_64be,  address_space[ADDRESS_SPACE_DATA])
 WRITEWORD64BE(data_write_word_64be,  address_space[ADDRESS_SPACE_DATA])
WRITEDWORD64BE(data_write_dword_64be, address_space[ADDRESS_SPACE_DATA])
  WRITEQWORD64(data_write_qword_64be, address_space[ADDRESS_SPACE_DATA])

  READBYTE64LE(data_read_byte_64le,   address_space[ADDRESS_SPACE_DATA])
  READWORD64LE(data_read_word_64le,   address_space[ADDRESS_SPACE_DATA])
 READDWORD64LE(data_read_dword_64le,  address_space[ADDRESS_SPACE_DATA])
   READQWORD64(data_read_qword_64le,  address_space[ADDRESS_SPACE_DATA])
 WRITEBYTE64LE(data_write_byte_64le,  address_space[ADDRESS_SPACE_DATA])
 WRITEWORD64LE(data_write_word_64le,  address_space[ADDRESS_SPACE_DATA])
WRITEDWORD64LE(data_write_dword_64le, address_space[ADDRESS_SPACE_DATA])
  WRITEQWORD64(data_write_qword_64le, address_space[ADDRESS_SPACE_DATA])


/*-------------------------------------------------
	I/O memory handlers
-------------------------------------------------*/

     READBYTE8(io_read_byte_8,      address_space[ADDRESS_SPACE_IO])
    WRITEBYTE8(io_write_byte_8,     address_space[ADDRESS_SPACE_IO])

  READBYTE16BE(io_read_byte_16be,   address_space[ADDRESS_SPACE_IO])
    READWORD16(io_read_word_16be,   address_space[ADDRESS_SPACE_IO])
 WRITEBYTE16BE(io_write_byte_16be,  address_space[ADDRESS_SPACE_IO])
   WRITEWORD16(io_write_word_16be,  address_space[ADDRESS_SPACE_IO])

  READBYTE16LE(io_read_byte_16le,   address_space[ADDRESS_SPACE_IO])
    READWORD16(io_read_word_16le,   address_space[ADDRESS_SPACE_IO])
 WRITEBYTE16LE(io_write_byte_16le,  address_space[ADDRESS_SPACE_IO])
   WRITEWORD16(io_write_word_16le,  address_space[ADDRESS_SPACE_IO])

  READBYTE32BE(io_read_byte_32be,   address_space[ADDRESS_SPACE_IO])
  READWORD32BE(io_read_word_32be,   address_space[ADDRESS_SPACE_IO])
   READDWORD32(io_read_dword_32be,  address_space[ADDRESS_SPACE_IO])
 WRITEBYTE32BE(io_write_byte_32be,  address_space[ADDRESS_SPACE_IO])
 WRITEWORD32BE(io_write_word_32be,  address_space[ADDRESS_SPACE_IO])
  WRITEDWORD32(io_write_dword_32be, address_space[ADDRESS_SPACE_IO])

  READBYTE32LE(io_read_byte_32le,   address_space[ADDRESS_SPACE_IO])
  READWORD32LE(io_read_word_32le,   address_space[ADDRESS_SPACE_IO])
   READDWORD32(io_read_dword_32le,  address_space[ADDRESS_SPACE_IO])
 WRITEBYTE32LE(io_write_byte_32le,  address_space[ADDRESS_SPACE_IO])
 WRITEWORD32LE(io_write_word_32le,  address_space[ADDRESS_SPACE_IO])
  WRITEDWORD32(io_write_dword_32le, address_space[ADDRESS_SPACE_IO])

  READBYTE64BE(io_read_byte_64be,   address_space[ADDRESS_SPACE_IO])
  READWORD64BE(io_read_word_64be,   address_space[ADDRESS_SPACE_IO])
 READDWORD64BE(io_read_dword_64be,  address_space[ADDRESS_SPACE_IO])
   READQWORD64(io_read_qword_64be,  address_space[ADDRESS_SPACE_IO])
 WRITEBYTE64BE(io_write_byte_64be,  address_space[ADDRESS_SPACE_IO])
 WRITEWORD64BE(io_write_word_64be,  address_space[ADDRESS_SPACE_IO])
WRITEDWORD64BE(io_write_dword_64be, address_space[ADDRESS_SPACE_IO])
  WRITEQWORD64(io_write_qword_64be, address_space[ADDRESS_SPACE_IO])

  READBYTE64LE(io_read_byte_64le,   address_space[ADDRESS_SPACE_IO])
  READWORD64LE(io_read_word_64le,   address_space[ADDRESS_SPACE_IO])
 READDWORD64LE(io_read_dword_64le,  address_space[ADDRESS_SPACE_IO])
   READQWORD64(io_read_qword_64le,  address_space[ADDRESS_SPACE_IO])
 WRITEBYTE64LE(io_write_byte_64le,  address_space[ADDRESS_SPACE_IO])
 WRITEWORD64LE(io_write_word_64le,  address_space[ADDRESS_SPACE_IO])
WRITEDWORD64LE(io_write_dword_64le, address_space[ADDRESS_SPACE_IO])
  WRITEQWORD64(io_write_qword_64le, address_space[ADDRESS_SPACE_IO])


/*-------------------------------------------------
	safe opcode reading
-------------------------------------------------*/

data8_t cpu_readop_safe(offs_t offset)
{
	activecpu_set_opbase(offset);
	return cpu_readop_unsafe(offset);
}

data16_t cpu_readop16_safe(offs_t offset)
{
	activecpu_set_opbase(offset);
	return cpu_readop16_unsafe(offset);
}

data32_t cpu_readop32_safe(offs_t offset)
{
	activecpu_set_opbase(offset);
	return cpu_readop32_unsafe(offset);
}

data64_t cpu_readop64_safe(offs_t offset)
{
	activecpu_set_opbase(offset);
	return cpu_readop64_unsafe(offset);
}

data8_t cpu_readop_arg_safe(offs_t offset)
{
	activecpu_set_opbase(offset);
	return cpu_readop_arg_unsafe(offset);
}

data16_t cpu_readop_arg16_safe(offs_t offset)
{
	activecpu_set_opbase(offset);
	return cpu_readop_arg16_unsafe(offset);
}

data32_t cpu_readop_arg32_safe(offs_t offset)
{
	activecpu_set_opbase(offset);
	return cpu_readop_arg32_unsafe(offset);
}

data64_t cpu_readop_arg64_safe(offs_t offset)
{
	activecpu_set_opbase(offset);
	return cpu_readop_arg64_unsafe(offset);
}


/*-------------------------------------------------
	unmapped memory handlers
-------------------------------------------------*/

static READ_HANDLER( mrh8_unmap_program )
{
	logerror("cpu #%d (PC=%08X): unmapped program memory byte read from %08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset));
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap;
}
static READ16_HANDLER( mrh16_unmap_program )
{
	logerror("cpu #%d (PC=%08X): unmapped program memory word read from %08X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*2), mem_mask ^ 0xffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap;
}
static READ32_HANDLER( mrh32_unmap_program )
{
	logerror("cpu #%d (PC=%08X): unmapped program memory dword read from %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*4), mem_mask ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap;
}
static READ64_HANDLER( mrh64_unmap_program )
{
	logerror("cpu #%d (PC=%08X): unmapped program memory qword read from %08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*8), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap;
}

static WRITE_HANDLER( mwh8_unmap_program )
{
	logerror("cpu #%d (PC=%08X): unmapped program memory byte write to %08X = %02X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset), data);
}
static WRITE16_HANDLER( mwh16_unmap_program )
{
	logerror("cpu #%d (PC=%08X): unmapped program memory word write to %08X = %04X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*2), data, mem_mask ^ 0xffff);
}
static WRITE32_HANDLER( mwh32_unmap_program )
{
	logerror("cpu #%d (PC=%08X): unmapped program memory dword write to %08X = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*4), data, mem_mask ^ 0xffffffff);
}
static WRITE64_HANDLER( mwh64_unmap_program )
{
	logerror("cpu #%d (PC=%08X): unmapped program memory qword write to %08X = %08X%08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*8), (int)(data >> 32), (int)(data & 0xffffffff), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
}

static READ_HANDLER( mrh8_unmap_data )
{
	logerror("cpu #%d (PC=%08X): unmapped data memory byte read from %08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset));
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap;
}
static READ16_HANDLER( mrh16_unmap_data )
{
	logerror("cpu #%d (PC=%08X): unmapped data memory word read from %08X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*2), mem_mask ^ 0xffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap;
}
static READ32_HANDLER( mrh32_unmap_data )
{
	logerror("cpu #%d (PC=%08X): unmapped data memory dword read from %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*4), mem_mask ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap;
}
static READ64_HANDLER( mrh64_unmap_data )
{
	logerror("cpu #%d (PC=%08X): unmapped data memory qword read from %08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*8), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap;
}

static WRITE_HANDLER( mwh8_unmap_data )
{
	logerror("cpu #%d (PC=%08X): unmapped data memory byte write to %08X = %02X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset), data);
}
static WRITE16_HANDLER( mwh16_unmap_data )
{
	logerror("cpu #%d (PC=%08X): unmapped data memory word write to %08X = %04X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*2), data, mem_mask ^ 0xffff);
}
static WRITE32_HANDLER( mwh32_unmap_data )
{
	logerror("cpu #%d (PC=%08X): unmapped data memory dword write to %08X = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*4), data, mem_mask ^ 0xffffffff);
}
static WRITE64_HANDLER( mwh64_unmap_data )
{
	logerror("cpu #%d (PC=%08X): unmapped data memory qword write to %08X = %08X%08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*8), (int)(data >> 32), (int)(data & 0xffffffff), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
}

static READ_HANDLER( mrh8_unmap_io )
{
	logerror("cpu #%d (PC=%08X): unmapped I/O byte read from %08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset));
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap;
}
static READ16_HANDLER( mrh16_unmap_io )
{
	logerror("cpu #%d (PC=%08X): unmapped I/O word read from %08X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*2), mem_mask ^ 0xffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap;
}
static READ32_HANDLER( mrh32_unmap_io )
{
	logerror("cpu #%d (PC=%08X): unmapped I/O dword read from %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*4), mem_mask ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap;
}
static READ64_HANDLER( mrh64_unmap_io )
{
	logerror("cpu #%d (PC=%08X): unmapped I/O qword read from %08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*8), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap;
}

static WRITE_HANDLER( mwh8_unmap_io )
{
	logerror("cpu #%d (PC=%08X): unmapped I/O byte write to %08X = %02X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset), data);
}
static WRITE16_HANDLER( mwh16_unmap_io )
{
	logerror("cpu #%d (PC=%08X): unmapped I/O word write to %08X = %04X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*2), data, mem_mask ^ 0xffff);
}
static WRITE32_HANDLER( mwh32_unmap_io )
{
	logerror("cpu #%d (PC=%08X): unmapped I/O dword write to %08X = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*4), data, mem_mask ^ 0xffffffff);
}
static WRITE64_HANDLER( mwh64_unmap_io )
{
	logerror("cpu #%d (PC=%08X): unmapped I/O qword write to %08X = %08X%08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), INV_SPACE_SHIFT(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*8), (int)(data >> 32), (int)(data & 0xffffffff), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
}


/*-------------------------------------------------
	no-op memory handlers
-------------------------------------------------*/

static READ_HANDLER( mrh8_nop_program )    { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap; }
static READ16_HANDLER( mrh16_nop_program ) { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap; }
static READ32_HANDLER( mrh32_nop_program ) { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap; }
static READ64_HANDLER( mrh64_nop_program ) { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap; }

static READ_HANDLER( mrh8_nop_data )       { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap; }
static READ16_HANDLER( mrh16_nop_data )    { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap; }
static READ32_HANDLER( mrh32_nop_data )    { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap; }
static READ64_HANDLER( mrh64_nop_data )    { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap; }

static READ_HANDLER( mrh8_nop_io )         { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap; }
static READ16_HANDLER( mrh16_nop_io )      { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap; }
static READ32_HANDLER( mrh32_nop_io )      { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap; }
static READ64_HANDLER( mrh64_nop_io )      { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap; }

static WRITE_HANDLER( mwh8_nop )           {  }
static WRITE16_HANDLER( mwh16_nop )        {  }
static WRITE32_HANDLER( mwh32_nop )        {  }
static WRITE64_HANDLER( mwh64_nop )        {  }


/*-------------------------------------------------
	other static handlers
-------------------------------------------------*/

static WRITE_HANDLER( mwh8_ramrom )    { bank_ptr[STATIC_RAM][offset] = bank_ptr[STATIC_RAM][offset + (opcode_base - opcode_arg_base)] = data; }
static WRITE16_HANDLER( mwh16_ramrom ) { COMBINE_DATA(&bank_ptr[STATIC_RAM][offset*2]); COMBINE_DATA(&bank_ptr[0][offset*2 + (opcode_base - opcode_arg_base)]); }
static WRITE32_HANDLER( mwh32_ramrom ) { COMBINE_DATA(&bank_ptr[STATIC_RAM][offset*4]); COMBINE_DATA(&bank_ptr[0][offset*4 + (opcode_base - opcode_arg_base)]); }
static WRITE64_HANDLER( mwh64_ramrom ) { COMBINE_DATA(&bank_ptr[STATIC_RAM][offset*8]); COMBINE_DATA(&bank_ptr[0][offset*8 + (opcode_base - opcode_arg_base)]); }


/*-------------------------------------------------
	get_static_handler - returns points to static
	memory handlers
-------------------------------------------------*/

static void *get_static_handler(int databits, int readorwrite, int spacenum, int which)
{
	static const struct
	{
		UINT8		databits;
		UINT8		handlernum;
		UINT8		spacenum;
		void *		read;
		void *		write;
	} static_handler_list[] =
	{
		{  8, STATIC_UNMAP,  ADDRESS_SPACE_PROGRAM, (void *)mrh8_unmap_program, (void *)mwh8_unmap_program },
		{  8, STATIC_UNMAP,  ADDRESS_SPACE_DATA,    (void *)mrh8_unmap_data,    (void *)mwh8_unmap_data },
		{  8, STATIC_UNMAP,  ADDRESS_SPACE_IO,      (void *)mrh8_unmap_io,      (void *)mwh8_unmap_io },
		{  8, STATIC_NOP,    ADDRESS_SPACE_PROGRAM, (void *)mrh8_nop_program,   (void *)mwh8_nop },
		{  8, STATIC_NOP,    ADDRESS_SPACE_DATA,    (void *)mrh8_nop_data,      (void *)mwh8_nop },
		{  8, STATIC_NOP,    ADDRESS_SPACE_IO,      (void *)mrh8_nop_io,        (void *)mwh8_nop },
		{  8, STATIC_RAMROM, ADDRESS_SPACE_PROGRAM, NULL,                       (void *)mwh8_ramrom },

		{ 16, STATIC_UNMAP,  ADDRESS_SPACE_PROGRAM, (void *)mrh16_unmap_program,(void *)mwh16_unmap_program },
		{ 16, STATIC_UNMAP,  ADDRESS_SPACE_DATA,    (void *)mrh16_unmap_data,   (void *)mwh16_unmap_data },
		{ 16, STATIC_UNMAP,  ADDRESS_SPACE_IO,      (void *)mrh16_unmap_io,     (void *)mwh16_unmap_io },
		{ 16, STATIC_NOP,    ADDRESS_SPACE_PROGRAM, (void *)mrh16_nop_program,  (void *)mwh16_nop },
		{ 16, STATIC_NOP,    ADDRESS_SPACE_DATA,    (void *)mrh16_nop_data,     (void *)mwh16_nop },
		{ 16, STATIC_NOP,    ADDRESS_SPACE_IO,      (void *)mrh16_nop_io,       (void *)mwh16_nop },
		{ 16, STATIC_RAMROM, ADDRESS_SPACE_PROGRAM, NULL,                       (void *)mwh16_ramrom },

		{ 32, STATIC_UNMAP,  ADDRESS_SPACE_PROGRAM, (void *)mrh32_unmap_program,(void *)mwh32_unmap_program },
		{ 32, STATIC_UNMAP,  ADDRESS_SPACE_DATA,    (void *)mrh32_unmap_data,   (void *)mwh32_unmap_data },
		{ 32, STATIC_UNMAP,  ADDRESS_SPACE_IO,      (void *)mrh32_unmap_io,     (void *)mwh32_unmap_io },
		{ 32, STATIC_NOP,    ADDRESS_SPACE_PROGRAM, (void *)mrh32_nop_program,  (void *)mwh32_nop },
		{ 32, STATIC_NOP,    ADDRESS_SPACE_DATA,    (void *)mrh32_nop_data,     (void *)mwh32_nop },
		{ 32, STATIC_NOP,    ADDRESS_SPACE_IO,      (void *)mrh32_nop_io,       (void *)mwh32_nop },
		{ 32, STATIC_RAMROM, ADDRESS_SPACE_PROGRAM, NULL,                       (void *)mwh32_ramrom },

		{ 64, STATIC_UNMAP,  ADDRESS_SPACE_PROGRAM, (void *)mrh64_unmap_program,(void *)mwh64_unmap_program },
		{ 64, STATIC_UNMAP,  ADDRESS_SPACE_DATA,    (void *)mrh64_unmap_data,   (void *)mwh64_unmap_data },
		{ 64, STATIC_UNMAP,  ADDRESS_SPACE_IO,      (void *)mrh64_unmap_io,     (void *)mwh64_unmap_io },
		{ 64, STATIC_NOP,    ADDRESS_SPACE_PROGRAM, (void *)mrh64_nop_program,  (void *)mwh64_nop },
		{ 64, STATIC_NOP,    ADDRESS_SPACE_DATA,    (void *)mrh64_nop_data,     (void *)mwh64_nop },
		{ 64, STATIC_NOP,    ADDRESS_SPACE_IO,      (void *)mrh64_nop_io,       (void *)mwh64_nop },
		{ 64, STATIC_RAMROM, ADDRESS_SPACE_PROGRAM, NULL,                       (void *)mwh64_ramrom }
	};
	int tablenum;

	for (tablenum = 0; tablenum < sizeof(static_handler_list) / sizeof(static_handler_list[0]); tablenum++)
		if (static_handler_list[tablenum].databits == databits && static_handler_list[tablenum].handlernum == which)
			if (static_handler_list[tablenum].spacenum == 0xff || static_handler_list[tablenum].spacenum == spacenum)
				return readorwrite ? static_handler_list[tablenum].write : static_handler_list[tablenum].read;

	return NULL;
}


/*-------------------------------------------------
	debugging
-------------------------------------------------*/

#if MEM_DUMP

static void dump_map(FILE *file, const struct addrspace_data_t *space, const struct table_data_t *table)
{
	static const char *strings[] =
	{
		"invalid",		"bank 1",		"bank 2",		"bank 3",
		"bank 4",		"bank 5",		"bank 6",		"bank 7",
		"bank 8",		"bank 9",		"bank 10",		"bank 11",
		"bank 12",		"bank 13",		"bank 14",		"bank 15",
		"bank 16",		"bank 17",		"bank 18",		"bank 19",
		"bank 20",		"bank 21",		"bank 22",		"bank 23",
		"bank 24",		"RAM",			"ROM",			"RAMROM",
		"nop",			"unused 1",		"unused 2",		"unmapped"
	};

	int l1count = 1 << LEVEL1_BITS;
	int l2count = 1 << LEVEL2_BITS;
	int i, j;

	fprintf(file, "  Address bits = %d\n", space->abits);
	fprintf(file, "     Data bits = %d\n", space->dbits);
	fprintf(file, "       L1 bits = %d\n", LEVEL1_BITS);
	fprintf(file, "       L2 bits = %d\n", LEVEL2_BITS);
	fprintf(file, "  Address mask = %X\n", space->mask);
	fprintf(file, "\n");

	for (i = 0; i < l1count; i++)
	{
		UINT8 entry = table->table[i];
		if (entry != STATIC_UNMAP)
		{
			fprintf(file, "%05X  %08X-%08X    = %02X: ", i,
					i << LEVEL2_BITS,
					((i+1) << LEVEL2_BITS) - 1, entry);
			if (entry < STATIC_COUNT)
				fprintf(file, "%s [offset=%08X]\n", strings[entry], table->handlers[entry].offset);
			else if (entry < SUBTABLE_BASE)
				fprintf(file, "handler(%08X) [offset=%08X]\n", (UINT32)table->handlers[entry].handler, table->handlers[entry].offset);
			else
			{
				fprintf(file, "subtable %d\n", entry - SUBTABLE_BASE);
				entry -= SUBTABLE_BASE;

				for (j = 0; j < l2count; j++)
				{
					UINT8 entry2 = table->table[(1 << LEVEL1_BITS) + (entry << LEVEL2_BITS) + j];
					if (entry2 != STATIC_UNMAP)
					{
						fprintf(file, "   %05X  %08X-%08X = %02X: ", j,
								((i << LEVEL2_BITS) | j),
								((i << LEVEL2_BITS) | (j+1)) - 1, entry2);
						if (entry2 < STATIC_COUNT)
							fprintf(file, "%s [offset=%08X]\n", strings[entry2], table->handlers[entry2].offset);
						else if (entry2 < SUBTABLE_BASE)
							fprintf(file, "handler(%08X) [offset=%08X]\n", (UINT32)table->handlers[entry2].handler, table->handlers[entry2].offset);
						else
							fprintf(file, "subtable %d???????????\n", entry2 - SUBTABLE_BASE);
					}
				}
			}
		}
	}
}

static void mem_dump(void)
{
	FILE *file = fopen("memdump.log", "w");
	int cpunum, spacenum;

	/* skip if we can't open the file */
	if (!file)
		return;

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (cpudata[cpunum].space[spacenum].abits)
			{
				fprintf(file, "\n\n"
				              "=========================================\n"
				              "CPU %d address space %d read handler dump\n"
				              "=========================================\n", cpunum, spacenum);
				dump_map(file, &cpudata[cpunum].space[spacenum], &cpudata[cpunum].space[spacenum].read);

				fprintf(file, "\n\n"
				              "==========================================\n"
				              "CPU %d address space %d write handler dump\n"
				              "==========================================\n", cpunum, spacenum);
				dump_map(file, &cpudata[cpunum].space[spacenum], &cpudata[cpunum].space[spacenum].write);
			}
	fclose(file);
}

#endif
