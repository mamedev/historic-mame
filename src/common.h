/*********************************************************************

  common.h

  Generic functions, mostly ROM related.

*********************************************************************/

#ifndef COMMON_H
#define COMMON_H

struct RomModule
{
	const char *name;	/* name of the file to load */
	UINT32 offset;		/* offset to load it to */
	UINT32 length;		/* length of the file */
	UINT32 crc;			/* standard CRC-32 checksum */
};

/* there are some special cases for the above. name, offset and size all set to 0 */
/* mark the end of the array. If name is 0 and the others aren't, that means "continue */
/* reading the previous rom from this address". If length is 0 and offset is not 0, */
/* that marks the start of a new memory region. Confused? Well, don't worry, just use */
/* the macros below. */

#define ROMFLAG_MASK          0xf0000000           /* 4 bits worth of flags in the high nibble */
/* Masks for ROM regions */
#define ROMFLAG_DISPOSE       0x80000000           /* Dispose of this region when done */
#define ROMFLAG_IGNORE        0x40000000           /* BM: Ignored - drivers must load this region themselves */

/* Masks for individual ROMs */
#define ROMFLAG_ALTERNATE     0x80000000           /* Alternate bytes, either even or odd, or nibbles, low or high */
#define ROMFLAG_WIDE          0x40000000           /* 16-bit ROM; may need byte swapping */
#define ROMFLAG_SWAP          0x20000000           /* 16-bit ROM with bytes in wrong order */
#define ROMFLAG_NIBBLE        0x10000000           /* Nibble-wide ROM image */

/* start of table */
#define ROM_START(name) static struct RomModule name[] = {
/* start of memory region */
#define ROM_REGION(length) { 0, length, 0, 0 },
/* start of disposable memory region */
#define ROM_REGION_DISPOSE(length) { 0, length | ROMFLAG_DISPOSE, 0, 0 },

/* Optional */
#define ROM_REGION_OPTIONAL(length) { 0, length | ROMFLAG_IGNORE, 0, 0 },

#define BADCRC( crc ) (~(crc))

/* ROM to load */
#define ROM_LOAD(name,offset,length,crc) { name, offset, length, crc },

/* continue loading the previous ROM to a new address */
#define ROM_CONTINUE(offset,length) { 0, offset, length, 0 },
/* restart loading the previous ROM to a new address */
#define ROM_RELOAD(offset,length) { (char *)-1, offset, length, 0 },

/* These are for nibble-wide ROMs, can be used with code or data */
#define ROM_LOAD_NIB_LOW(name,offset,length,crc) { name, offset, length | ROMFLAG_NIBBLE, crc },
#define ROM_LOAD_NIB_HIGH(name,offset,length,crc) { name, offset, length | ROMFLAG_NIBBLE | ROMFLAG_ALTERNATE, crc },
#define ROM_RELOAD_NIB_LOW(offset,length) { (char *)-1, offset, length | ROMFLAG_NIBBLE, 0 },
#define ROM_RELOAD_NIB_HIGH(offset,length) { (char *)-1, offset, length | ROMFLAG_NIBBLE | ROMFLAG_ALTERNATE, 0 },

/* The following ones are for code ONLY - don't use for graphics data!!! */
/* load the ROM at even/odd addresses. Useful with 16 bit games */
#define ROM_LOAD_EVEN(name,offset,length,crc) { name, offset & ~1, length | ROMFLAG_ALTERNATE, crc },
#define ROM_RELOAD_EVEN(offset,length) { (char *)-1, offset & ~1, length | ROMFLAG_ALTERNATE, 0 },
#define ROM_LOAD_ODD(name,offset,length,crc)  { name, offset |  1, length | ROMFLAG_ALTERNATE, crc },
#define ROM_RELOAD_ODD(offset,length)  { (char *)-1, offset |  1, length | ROMFLAG_ALTERNATE, 0 },
/* load the ROM at even/odd addresses. Useful with 16 bit games */
#define ROM_LOAD_WIDE(name,offset,length,crc) { name, offset, length | ROMFLAG_WIDE, crc },
#define ROM_RELOAD_WIDE(offset,length) { (char *)-1, offset, length | ROMFLAG_WIDE, 0 },
#define ROM_LOAD_WIDE_SWAP(name,offset,length,crc) { name, offset, length | ROMFLAG_WIDE | ROMFLAG_SWAP, crc },
#define ROM_RELOAD_WIDE_SWAP(offset,length) { (char *)-1, offset, length | ROMFLAG_WIDE | ROMFLAG_SWAP, 0 },

#ifdef LSB_FIRST
#define ROM_LOAD_V20_EVEN	ROM_LOAD_EVEN
#define ROM_RELOAD_V20_EVEN  ROM_RELOAD_EVEN
#define ROM_LOAD_V20_ODD	ROM_LOAD_ODD
#define ROM_RELOAD_V20_ODD   ROM_RELOAD_ODD
#else
#define ROM_LOAD_V20_EVEN	ROM_LOAD_ODD
#define ROM_RELOAD_V20_EVEN  ROM_RELOAD_ODD
#define ROM_LOAD_V20_ODD	ROM_LOAD_EVEN
#define ROM_RELOAD_V20_ODD   ROM_RELOAD_EVEN
#endif

/* Use THESE ones for graphics data */
#ifdef LSB_FIRST
#define ROM_LOAD_GFX_EVEN    ROM_LOAD_ODD
#define ROM_LOAD_GFX_ODD     ROM_LOAD_EVEN
#define ROM_LOAD_GFX_SWAP    ROM_LOAD_WIDE
#else
#define ROM_LOAD_GFX_EVEN    ROM_LOAD_EVEN
#define ROM_LOAD_GFX_ODD     ROM_LOAD_ODD
#define ROM_LOAD_GFX_SWAP    ROM_LOAD_WIDE_SWAP
#endif

/* end of table */
#define ROM_END { 0, 0, 0, 0 } };



struct GameSample
{
	int length;
	int smpfreq;
	int resolution;
	signed char data[1];	/* extendable */
};

struct GameSamples
{
	int total;	/* total number of samples */
	struct GameSample *sample[1];	/* extendable */
};




void showdisclaimer(void);

/* LBO 042898 - added coin counters */
#define COIN_COUNTERS	4	/* total # of coin counters */
void coin_counter_w (int offset, int data);
void coin_lockout_w (int offset, int data);
void coin_lockout_global_w (int offset, int data);  /* Locks out all coin inputs */

int readroms(void);
void printromlist(const struct RomModule *romp,const char *name);
struct GameSamples *readsamples(const char **samplenames,const char *name);
void freesamples(struct GameSamples *samples);

void save_screen_snapshot(void);

#endif
