
#ifndef AUDIT_H
#define AUDIT_H

/* return values from VerifyRomSet and VerifySampleSet */
#define CORRECT   0
#define NOTFOUND  1
#define INCORRECT 2

/* rom status values for tAuditRecord.status */
#define AUD_ROM_GOOD		0
#define AUD_ROM_NOT_FOUND	1
#define AUD_BAD_CHECKSUM	2
#define AUD_MEM_ERROR		3
#define AUD_LENGTH_MISMATCH	4

#define AUD_MAX_ROMS		50	/* maximum roms in a single romset */
#define AUD_MAX_SAMPLES		100	/* maximum samples per driver */

typedef struct
{
	char	rom[20];		/* name of rom file */
	int		length;			/* expected length of rom file */
	int		expchecksum;	/* expected checksum of rom file */
	int		checksum;		/* actual checksum of rom file */
	int		status;			/* status of rom file */
} tAuditRecord;

typedef struct
{
	char	name[20];		/* name of missing sample file */
} tMissingSample;

typedef void (*verify_printf_proc)(char *fmt,...);

int AuditRomSet (int game, tAuditRecord **audit);
int VerifyRomSet(int game,verify_printf_proc verify_printf);
int AuditSampleSet (int game, tMissingSample **audit);
int VerifySampleSet(int game,verify_printf_proc verify_printf);


#endif
